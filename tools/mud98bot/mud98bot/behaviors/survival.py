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
    DEFAULT_FLEE_HP_PERCENT, DEFAULT_REST_HP_PERCENT,
    DEFAULT_REST_MANA_PERCENT, DEFAULT_REST_MOVE_PERCENT,
    CRITICAL_HP_PERCENT, MAX_FLEE_ATTEMPTS,
)

if TYPE_CHECKING:
    from ..bot import Bot

logger = logging.getLogger(__name__)


class DeathRecoveryBehavior(Behavior):
    """
    Handle death/ghost state.
    
    When the character dies, they become a ghost and need to navigate
    back to their corpse or a healer. This behavior handles the recovery
    process.
    """
    
    priority = PRIORITY_DEATH_RECOVERY  # Highest priority
    name = "DeathRecovery"
    tick_delay = 2.0
    
    def __init__(self):
        super().__init__()
        self._death_count = 0
        self._recovery_attempts = 0
    
    def can_start(self, ctx: BehaviorContext) -> bool:
        """Start if position indicates death."""
        # Check if we're dead/ghost
        if ctx.position and ctx.position.name.lower() in ('dead', 'ghost'):
            return True
        # Also check for 0 HP (shouldn't happen but safety check)
        if ctx.health <= 0 and ctx.health_max > 0:
            return True
        return False
    
    def tick(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        self._recovery_attempts += 1
        
        if self._recovery_attempts == 1:
            self._death_count += 1
            logger.warning(f"[{bot.bot_id}] DEATH #{self._death_count}! Attempting recovery...")
        
        # Try various recovery methods
        if self._recovery_attempts <= 3:
            # Try standing up first
            bot.send_command("wake")
            bot.send_command("stand")
        elif self._recovery_attempts <= 6:
            # Try recalling
            bot.send_command("recall")
        elif self._recovery_attempts <= 10:
            # Try pray (some MUDs have this)
            bot.send_command("pray")
        else:
            # Give up and wait
            logger.error(f"[{bot.bot_id}] Death recovery failed after {self._recovery_attempts} attempts")
            self._recovery_attempts = 0
            return BehaviorResult.FAILED
        
        return BehaviorResult.CONTINUE
    
    def stop(self) -> None:
        super().stop()
        if self._recovery_attempts > 0:
            logger.info(f"[{self.name}] Recovered from death after {self._recovery_attempts} attempts")
        self._recovery_attempts = 0


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
        if not ctx.in_combat:
            logger.info(f"[{bot.bot_id}] Escaped combat!")
            self._flee_attempts = 0
            return BehaviorResult.COMPLETED
        
        self._flee_attempts += 1
        logger.warning(f"[{bot.bot_id}] FLEE attempt #{self._flee_attempts} "
                      f"(HP: {ctx.hp_percent:.0f}%)")
        
        bot.send_command("flee")
        
        if self._flee_attempts >= MAX_FLEE_ATTEMPTS:
            logger.error(f"[{bot.bot_id}] Flee failed after {MAX_FLEE_ATTEMPTS} attempts!")
            # Notify RecallBehavior to take over
            # This is handled by checking _flee_attempts in RecallBehavior
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
