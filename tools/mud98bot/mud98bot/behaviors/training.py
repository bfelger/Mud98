"""
Training Behaviors - Stats, Skills, and Bot Reset.

This module contains behaviors for character development:
- TrainBehavior: Navigate to trainer and spend training sessions
- PracticeBehavior: Navigate to practitioner and practice skills
- BotResetBehavior: One-time setup at bot startup
"""

from __future__ import annotations

import logging
from typing import TYPE_CHECKING, Optional

from .base import Behavior, BehaviorContext, BehaviorResult
from ..config import (
    PRIORITY_TRAIN, PRIORITY_PRACTICE, PRIORITY_BOT_RESET,
    TRAIN_ROOM, PRACTICE_ROOM,
    ROUTE_TO_TRAIN_ROOM, ROUTE_TO_PRACTICE_ROOM,
    CRITICAL_MOVE_PERCENT, DEFAULT_PRACTICE_PER_SKILL, DEFAULT_PRACTICE_SKILLS,
)

if TYPE_CHECKING:
    from ..bot import Bot

logger = logging.getLogger(__name__)


class TrainBehavior(Behavior):
    """
    Navigate to trainer and spend training sessions.
    
    One-shot behavior that navigates to the train room and spends
    all available training sessions on specified stats.
    """
    
    priority = PRIORITY_TRAIN
    name = "Train"
    tick_delay = 0.5
    
    # Yield to healing if movement drops below this threshold
    CRITICAL_MOVE_PERCENT = CRITICAL_MOVE_PERCENT
    
    # Default stats to train in priority order
    DEFAULT_STATS = ['con', 'str', 'dex', 'wis', 'int']
    
    def __init__(
        self,
        stats: Optional[list[str]] = None,
        route: Optional[dict[int, str]] = None,
        trains_per_stat: int = 5,
        reset_first: bool = False,
    ):
        """
        Args:
            stats: Stats to train in priority order.
            route: Route to trainer room.
            trains_per_stat: Training sessions per stat.
            reset_first: Whether to send 'train reset' first (resets train sessions).
        """
        super().__init__()
        self.stats = stats or self.DEFAULT_STATS
        self.route = route or ROUTE_TO_TRAIN_ROOM
        self.trains_per_stat = trains_per_stat
        self.reset_first = reset_first
        self._navigating = True
        self._reset_sent = False
        self._completed = False
    
    def can_start(self, ctx: BehaviorContext) -> bool:
        """Start if not completed and healthy."""
        if self._completed:
            return False
        if ctx.in_combat:
            return False
        if ctx.move_percent < self.CRITICAL_MOVE_PERCENT:
            return False
        if not ctx.position.can_move:
            return False
        return True
    
    def tick(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        if ctx.in_combat:
            return BehaviorResult.WAITING
        
        # Phase 1: Navigate to trainer
        if self._navigating:
            if ctx.room_vnum == TRAIN_ROOM:
                logger.info(f"[{bot.bot_id}] Arrived at Train Room (VNUM {TRAIN_ROOM})")
                self._navigating = False
                return BehaviorResult.CONTINUE
            
            if not ctx.position.can_move:
                bot.send_command("wake")
                bot.send_command("stand")
                return BehaviorResult.CONTINUE
            
            if ctx.room_vnum in self.route:
                direction = self.route[ctx.room_vnum]
                logger.debug(f"[{bot.bot_id}] Train nav: room {ctx.room_vnum} -> {direction}")
                bot.send_command(direction)
                return BehaviorResult.CONTINUE
            else:
                logger.warning(f"[{bot.bot_id}] Train: room {ctx.room_vnum} not in route, recalling")
                bot.send_command("recall")
                return BehaviorResult.CONTINUE
        
        # Phase 2: Reset training sessions if requested
        if self.reset_first and not self._reset_sent:
            logger.info(f"[{bot.bot_id}] Sending 'train reset' to reset training sessions")
            bot.send_command("train reset")
            self._reset_sent = True
            return BehaviorResult.CONTINUE
        
        # Phase 3: Train stats - batch all commands in one tick
        total_trained = 0
        for stat in self.stats:
            for _ in range(self.trains_per_stat):
                bot.send_command(f"train {stat}")
                total_trained += 1
        
        logger.info(f"[{bot.bot_id}] Training complete (sent {total_trained} train commands)")
        self._completed = True
        return BehaviorResult.COMPLETED
    
    def reset(self) -> None:
        """Reset for re-use."""
        self._navigating = True
        self._reset_sent = False
        self._completed = False


class PracticeBehavior(Behavior):
    """
    Navigate to practitioner and practice skills.
    
    One-shot behavior that navigates to the practice room and
    practices specified skills.
    """
    
    priority = PRIORITY_PRACTICE
    name = "Practice"
    tick_delay = 0.5
    
    CRITICAL_MOVE_PERCENT = CRITICAL_MOVE_PERCENT
    DEFAULT_SKILLS = list(DEFAULT_PRACTICE_SKILLS)
    
    def __init__(
        self,
        skills: Optional[list[str]] = None,
        route: Optional[dict[int, str]] = None,
        reset_first: bool = True,
        practice_per_skill: int = DEFAULT_PRACTICE_PER_SKILL,
    ):
        """
        Args:
            skills: Skills to practice.
            route: Route to practice room.
            reset_first: Whether to send 'practice reset' first.
            practice_per_skill: Times to practice each skill.
        """
        super().__init__()
        self.skills = skills or self.DEFAULT_SKILLS
        self.route = route or ROUTE_TO_PRACTICE_ROOM
        self.reset_first = reset_first
        self.practice_per_skill = practice_per_skill
        self._navigating = True
        self._completed = False
        self._reset_sent = False
    
    def can_start(self, ctx: BehaviorContext) -> bool:
        """Start if not completed and healthy."""
        if self._completed:
            return False
        if ctx.in_combat:
            return False
        if ctx.move_percent < self.CRITICAL_MOVE_PERCENT:
            return False
        if not ctx.position.can_move:
            return False
        return True
    
    def tick(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        if ctx.in_combat:
            return BehaviorResult.WAITING
        
        # Phase 1: Navigate to practice room
        if self._navigating:
            if ctx.room_vnum == PRACTICE_ROOM:
                logger.info(f"[{bot.bot_id}] Arrived at Practice Room (VNUM {PRACTICE_ROOM})")
                self._navigating = False
                return BehaviorResult.CONTINUE
            
            if not ctx.position.can_move:
                bot.send_command("wake")
                bot.send_command("stand")
                return BehaviorResult.CONTINUE
            
            if ctx.room_vnum in self.route:
                direction = self.route[ctx.room_vnum]
                logger.debug(f"[{bot.bot_id}] Practice nav: room {ctx.room_vnum} -> {direction}")
                bot.send_command(direction)
                return BehaviorResult.CONTINUE
            else:
                logger.warning(f"[{bot.bot_id}] Practice: room {ctx.room_vnum} not in route, recalling")
                bot.send_command("recall")
                return BehaviorResult.CONTINUE
        
        # Phase 2: Send reset command if enabled
        if self.reset_first and not self._reset_sent:
            bot.send_command("practice reset")
            logger.info(f"[{bot.bot_id}] Sent 'practice reset' to reset skills and practice points")
            self._reset_sent = True
            return BehaviorResult.CONTINUE
        
        # Phase 3: Practice skills - batch all commands in one tick
        total_practiced = 0
        for skill in self.skills:
            for _ in range(self.practice_per_skill):
                bot.send_command(f"practice {skill}")
                total_practiced += 1
        
        logger.info(f"[{bot.bot_id}] Practice complete (sent {total_practiced} practice commands)")
        self._completed = True
        return BehaviorResult.COMPLETED
    
    def reset(self) -> None:
        """Reset for re-use."""
        self._navigating = True
        self._completed = False
        self._reset_sent = False


class BotResetBehavior(Behavior):
    """
    Perform a hard reset of the bot character at startup.
    
    This is a one-shot startup behavior that sends 'botreset hard' to
    completely reset the character to level 1 with fresh equipment,
    skills, and stats. After the reset, the character will be at the
    MUD school entrance (VNUM 3700).
    
    This replaces the need for separate train reset / practice reset
    commands and ensures each test session starts from a known baseline.
    
    After reset, the normal behavior flow is:
    1. TrainBehavior - navigate to train room and train stats
    2. PracticeBehavior - navigate to practice room and practice skills
    3. NavigateBehavior - navigate to cage room (3712)
    4. PatrolCagesBehavior - patrol all 4 cages, killing and looting
    5. BuySuppliesBehavior - triggered after patrol circuit with earned money
    """
    
    priority = PRIORITY_BOT_RESET
    name = "BotReset"
    tick_delay = 0.5
    
    # VNUM of MUD school entrance where botreset teleports the player
    MUD_SCHOOL_ENTRANCE = 3700
    
    def __init__(self):
        super().__init__()
        self._completed = False
        self._sent_reset = False
        self._wait_ticks = 0
    
    def can_start(self, ctx: BehaviorContext) -> bool:
        """One-shot behavior - only run if not completed."""
        if self._completed:
            return False
        if ctx.in_combat:
            return False
        return True
    
    def start(self, bot: 'Bot', ctx: BehaviorContext) -> None:
        super().start(bot, ctx)
        # Reset hunger/thirst/proactive shopping state
        if hasattr(bot, '_behavior_engine') and bot._behavior_engine:
            bot._behavior_engine.reset_needs_state()
    
    def tick(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        if ctx.in_combat:
            return BehaviorResult.WAITING
        
        if not self._sent_reset:
            bot.send_command("botreset hard")
            # Send look to trigger MSDP/BOT updates after teleport
            bot.send_command("look")
            logger.info(f"[{bot.bot_id}] Sent 'botreset hard' to reset character to level 1")
            self._sent_reset = True
            return BehaviorResult.CONTINUE
        
        # Wait for MSDP to confirm we're at MUD school entrance
        # This prevents race conditions where TrainBehavior starts before
        # the server has processed our teleport
        if ctx.room_vnum != self.MUD_SCHOOL_ENTRANCE:
            self._wait_ticks += 1
            if self._wait_ticks > 20:  # 10 seconds at 0.5s tick_delay
                logger.warning(f"[{bot.bot_id}] Timeout waiting for teleport to {self.MUD_SCHOOL_ENTRANCE}, "
                             f"currently at {ctx.room_vnum}")
                # Complete anyway to avoid being stuck
                self._completed = True
                return BehaviorResult.COMPLETED
            # Send periodic look to ensure we get updates
            if self._wait_ticks % 4 == 0:  # Every 2 seconds
                bot.send_command("look")
            logger.debug(f"[{bot.bot_id}] Waiting for teleport to {self.MUD_SCHOOL_ENTRANCE}, "
                        f"currently at {ctx.room_vnum}")
            return BehaviorResult.CONTINUE
        
        # Done - reset is complete and we're at the right room
        logger.info(f"[{bot.bot_id}] Bot reset complete - character is now at MUD school entrance")
        self._completed = True
        return BehaviorResult.COMPLETED
    
    def reset(self) -> None:
        """Reset for re-use."""
        self._completed = False
        self._sent_reset = False
        self._wait_ticks = 0
