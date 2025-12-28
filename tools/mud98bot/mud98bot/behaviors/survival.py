"""
Survival Behaviors - Death Recovery, Fleeing, Healing.

This module contains behaviors for survival:
- DeathRecoveryBehavior: Handle death/ghost state
- SurviveBehavior: Emergency flee when HP is critical
- HealBehavior: Rest to recover HP/mana/moves
- RecallBehavior: Emergency recall when flee fails
"""

from __future__ import annotations

import logging
from typing import TYPE_CHECKING, Optional

from .base import Behavior, BehaviorContext, BehaviorResult
from ..config import (
    PRIORITY_DEATH_RECOVERY, PRIORITY_SURVIVE, PRIORITY_HEAL, PRIORITY_RECALL,
    PRIORITY_LIGHT_SOURCE,
    DEFAULT_FLEE_HP_PERCENT, DEFAULT_REST_HP_PERCENT,
    DEFAULT_REST_MANA_PERCENT, DEFAULT_REST_MOVE_PERCENT,
    CRITICAL_HP_PERCENT, MAX_FLEE_ATTEMPTS,
)

if TYPE_CHECKING:
    from ..bot import Bot

logger = logging.getLogger(__name__)


class DeathRecoveryBehavior(Behavior):
    """
    Highest priority behavior - handles death/dying states.
    
    Activates when:
    - Position is DEAD, MORTAL, or INCAP (truly dying)
    - OR Position is STUNNED AND HP <= 0 (combat death)
    
    Does NOT activate for STUNNED with normal HP (e.g., after login/teleport).
    Waits for respawn (position becomes RESTING/STANDING with HP > 0).
    """
    
    priority = PRIORITY_DEATH_RECOVERY  # Highest priority
    name = "DeathRecovery"
    tick_delay = 1.0
    
    def __init__(self):
        super().__init__()
        self._wait_count = 0
        self._death_recorded = False
    
    @staticmethod
    def _is_truly_dead(ctx: BehaviorContext) -> bool:
        """Check if the character is truly dead/dying, not just stunned."""
        # Definitely dead if position is DEAD, MORTAL, or INCAP
        if ctx.position.is_dead:
            return True
        # Stunned with zero or negative HP is also death
        if ctx.position.is_stunned_or_worse and ctx.hp_percent <= 0:
            return True
        return False
    
    def can_start(self, ctx: BehaviorContext) -> bool:
        """Start if truly dead (not just stunned with normal HP)."""
        return self._is_truly_dead(ctx)
    
    def start(self, bot: 'Bot', ctx: BehaviorContext) -> None:
        super().start(bot, ctx)
        self._wait_count = 0
        self._death_recorded = False
        logger.error(f"[{bot.bot_id}] DIED! Position={ctx.position.name}, HP={ctx.hp_percent:.0f}%")
        
        # Clear dark creature flag on death - we'll need to restart the quest
        if hasattr(bot, '_behavior_engine') and bot._behavior_engine:
            bot._behavior_engine._should_fight_dark_creature = False
            logger.info(f"[{bot.bot_id}] Cleared dark creature flag on death")
    
    def tick(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        # Record death once (metrics may not exist on bot, handle gracefully)
        if not self._death_recorded:
            if hasattr(bot, 'metrics') and bot.metrics:
                bot.metrics.record_death()
                logger.info(f"[{bot.bot_id}] Death recorded in metrics")
            self._death_recorded = True
        
        # Check if we've respawned (not dead AND HP > 0)
        if not self._is_truly_dead(ctx) and ctx.hp_percent > 0:
            logger.info(f"[{bot.bot_id}] Respawned! Position={ctx.position.name}, HP={ctx.hp_percent:.0f}%, room={ctx.room_vnum}")
            bot.send_command("look")  # Refresh room state
            return BehaviorResult.COMPLETED
        
        self._wait_count += 1
        if self._wait_count % 5 == 0:
            logger.debug(f"[{bot.bot_id}] Waiting for respawn... (position={ctx.position.name}, HP={ctx.hp_percent:.0f}%)")
        
        # Timeout after 30 seconds - something is very wrong
        if self._wait_count > 30:
            logger.error(f"[{bot.bot_id}] Respawn timeout! Still dead after 30s")
            return BehaviorResult.FAILED
        
        return BehaviorResult.WAITING


class SurviveBehavior(Behavior):
    """
    Emergency flee when HP is critical.
    
    When HP drops below the flee threshold, attempt to flee combat.
    This is a high-priority emergency behavior.
    """
    
    priority = PRIORITY_SURVIVE
    name = "Survive"
    tick_delay = 0.3  # Very fast - emergency
    
    def __init__(self, flee_hp_percent: float = DEFAULT_FLEE_HP_PERCENT):
        super().__init__()
        self.flee_hp_percent = flee_hp_percent
        self._flee_attempts = 0
    
    def can_start(self, ctx: BehaviorContext) -> bool:
        """Start when HP is critical and in combat."""
        return ctx.in_combat and ctx.hp_percent < self.flee_hp_percent
    
    def tick(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        # Check if we died while trying to flee (truly dead, not just stunned)
        if ctx.position.is_dead or (ctx.position.is_stunned_or_worse and ctx.hp_percent <= 0):
            logger.error(f"[{bot.bot_id}] Died while trying to flee! "
                        f"(position={ctx.position.name}, HP={ctx.hp_percent:.0f}%)")
            if hasattr(bot, 'metrics') and bot.metrics:
                bot.metrics.record_death()
            return BehaviorResult.FAILED
        
        if not ctx.in_combat:
            logger.info(f"[{bot.bot_id}] Successfully escaped combat!")
            self._flee_attempts = 0
            return BehaviorResult.COMPLETED
        
        self._flee_attempts += 1
        logger.warning(f"[{bot.bot_id}] Fleeing (attempt {self._flee_attempts})...")
        
        bot.send_command("flee")
        
        if self._flee_attempts >= MAX_FLEE_ATTEMPTS:
            logger.error(f"[{bot.bot_id}] Could not flee after {MAX_FLEE_ATTEMPTS} attempts!")
            bot.send_command("recall")
            self._flee_attempts = 0
            return BehaviorResult.FAILED
        
        return BehaviorResult.CONTINUE


class HealBehavior(Behavior):
    """
    Rest to recover HP, mana, and movement.
    
    When resources are low and not in immediate danger, rest or sleep
    to recover. Monitors all three resources and stops when healthy.
    """
    
    priority = PRIORITY_HEAL
    name = "Heal"
    tick_delay = 2.0
    
    def __init__(
        self,
        rest_hp_percent: float = DEFAULT_REST_HP_PERCENT,
        rest_mana_percent: float = DEFAULT_REST_MANA_PERCENT,
        rest_move_percent: float = DEFAULT_REST_MOVE_PERCENT,
    ):
        super().__init__()
        self.rest_hp_percent = rest_hp_percent
        self.rest_mana_percent = rest_mana_percent
        self.rest_move_percent = rest_move_percent
        self._resting = False
        self._sleeping = False
    
    def can_start(self, ctx: BehaviorContext) -> bool:
        """Start if resources are low and not in combat."""
        if ctx.in_combat:
            return False
        
        needs_hp = ctx.hp_percent < self.rest_hp_percent
        needs_mana = ctx.mana_percent < self.rest_mana_percent
        needs_move = ctx.move_percent < self.rest_move_percent
        
        return needs_hp or needs_mana or needs_move
    
    def tick(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        # Interrupt if combat starts
        if ctx.in_combat:
            if self._sleeping or self._resting:
                bot.send_command("wake")
                bot.send_command("stand")
            self._sleeping = False
            self._resting = False
            return BehaviorResult.COMPLETED
        
        # Check if fully recovered
        fully_healed = (
            ctx.hp_percent >= 99 and
            ctx.mana_percent >= 99 and
            ctx.move_percent >= 99
        )
        
        if fully_healed:
            if self._sleeping or self._resting:
                bot.send_command("wake")
                bot.send_command("stand")
            logger.info(f"[{bot.bot_id}] Fully healed!")
            self._sleeping = False
            self._resting = False
            return BehaviorResult.COMPLETED
        
        # Start resting/sleeping if not already
        if not self._resting and not self._sleeping:
            # Sleep for faster regen if HP is very low
            if ctx.hp_percent < 30:
                bot.send_command("sleep")
                self._sleeping = True
            else:
                bot.send_command("rest")
                self._resting = True
        
        return BehaviorResult.CONTINUE
    
    def stop(self) -> None:
        super().stop()
        self._resting = False
        self._sleeping = False


class RecallBehavior(Behavior):
    """
    Emergency recall when flee fails.
    
    This is a last-resort escape mechanism when fleeing has failed
    multiple times and HP is critical.
    """
    
    priority = PRIORITY_RECALL
    name = "Recall"
    tick_delay = 1.0
    
    def __init__(self):
        super().__init__()
        self._flee_failed = False
        self._flee_attempts = 0
    
    def notify_flee_failed(self) -> None:
        """Called by SurviveBehavior when flee fails."""
        self._flee_failed = True
        self._flee_attempts += 1
    
    def can_start(self, ctx: BehaviorContext) -> bool:
        """Start when flee has failed and HP is critical."""
        return self._flee_failed and ctx.hp_percent < CRITICAL_HP_PERCENT
    
    def tick(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        logger.info(f"[{bot.bot_id}] EMERGENCY RECALL! (Flee failed, HP: {ctx.hp_percent:.0f}%)")
        bot.send_command("recall")
        self._flee_failed = False
        return BehaviorResult.COMPLETED


class LightSourceBehavior(Behavior):
    """
    Handle light sources when entering/exiting dark rooms.
    
    When entering a dark room (room has 'dark' flag):
    1. Remember currently held off-hand item (if any)
    2. Hold lantern if we have one
    
    When leaving a dark room (entering lit room):
    1. Remove lantern
    2. Re-equip the remembered off-hand item
    
    This behavior is emergent - it activates automatically based on
    room lighting conditions and whether we have a lantern.
    """
    
    priority = PRIORITY_LIGHT_SOURCE
    name = "LightSource"
    tick_delay = 0.3
    
    def __init__(self):
        super().__init__()
        self._previous_offhand: Optional[str] = None
        self._holding_lantern: bool = False
        self._in_dark_room: bool = False
        self._wait_ticks: int = 0
    
    def can_start(self, ctx: BehaviorContext) -> bool:
        """Start when entering/exiting a dark room."""
        is_dark = 'dark' in ctx.bot_room_flags
        
        # Need to equip lantern: dark room and not holding lantern
        if is_dark and not self._holding_lantern:
            return True
        
        # Need to re-equip offhand: left dark room and still holding lantern
        if not is_dark and self._holding_lantern:
            return True
        
        return False
    
    def tick(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        is_dark = 'dark' in ctx.bot_room_flags
        
        if is_dark and not self._holding_lantern:
            return self._equip_lantern(bot, ctx)
        elif not is_dark and self._holding_lantern:
            return self._restore_offhand(bot, ctx)
        
        return BehaviorResult.COMPLETED
    
    def _equip_lantern(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        """Equip lantern when entering dark room."""
        self._wait_ticks += 1
        
        if self._wait_ticks == 1:
            # First, save current offhand if any (we'll use inventory check later)
            # For now, just try to remove offhand and hold lantern
            logger.info(f"[{bot.bot_id}] Dark room detected - equipping lantern")
            bot.send_command("remove held")  # Remove current offhand if any
            return BehaviorResult.CONTINUE
        
        if self._wait_ticks == 2:
            bot.send_command("wear lantern")
            self._holding_lantern = True
            self._in_dark_room = True
            logger.info(f"[{bot.bot_id}] Now wearing lantern for light")
            self._wait_ticks = 0
            return BehaviorResult.COMPLETED
        
        return BehaviorResult.CONTINUE
    
    def _restore_offhand(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        """Restore previous offhand when leaving dark room."""
        self._wait_ticks += 1
        
        if self._wait_ticks == 1:
            logger.info(f"[{bot.bot_id}] Left dark room - removing lantern")
            bot.send_command("remove lantern")
            return BehaviorResult.CONTINUE
        
        if self._wait_ticks == 2:
            # Try to re-equip a shield or other offhand item
            # The bot should auto-equip best gear via compare/wear
            # For now just put lantern away
            bot.send_command("put lantern bag")  # Attempt to store in bag
            self._holding_lantern = False
            self._in_dark_room = False
            logger.info(f"[{bot.bot_id}] Lantern stowed, back in light")
            self._wait_ticks = 0
            return BehaviorResult.COMPLETED
        
        return BehaviorResult.CONTINUE
