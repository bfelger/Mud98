"""
Combat Behaviors - Fighting and Target Selection.

This module contains behaviors related to combat:
- AttackBehavior: Initiate fights with appropriate targets
- CombatBehavior: Continue ongoing combat, use skills
"""

from __future__ import annotations

import logging
from typing import TYPE_CHECKING, Optional

from .base import Behavior, BehaviorContext, BehaviorResult
from ..config import (
    PRIORITY_ATTACK, PRIORITY_COMBAT,
    MIN_ATTACK_HP_PERCENT,
)

if TYPE_CHECKING:
    from ..bot import Bot

logger = logging.getLogger(__name__)


import random


class CombatBehavior(Behavior):
    """
    Continue fighting when in combat.
    
    This behavior maintains an ongoing fight by issuing attack commands
    and optionally using skills. It has high priority to ensure combat
    continues without interruption.
    """
    
    priority = PRIORITY_COMBAT
    name = "Combat"
    tick_delay = 1.0  # Match combat round timing
    
    def __init__(self, skills: Optional[list[str]] = None):
        super().__init__()
        self.skills = skills or []
    
    def can_start(self, ctx: BehaviorContext) -> bool:
        """Start when in combat."""
        return ctx.in_combat
    
    def tick(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        # Check if we died
        if ctx.position.is_dead or (ctx.position.is_stunned_or_worse and ctx.hp_percent <= 0):
            logger.error(f"[{bot.bot_id}] Combat ended - DIED! "
                        f"(position={ctx.position.name}, HP={ctx.hp_percent:.0f}%)")
            if hasattr(bot, 'metrics') and bot.metrics:
                bot.metrics.record_death()
            return BehaviorResult.FAILED
        
        if not ctx.in_combat:
            logger.info(f"[{bot.bot_id}] Combat ended - victory!")
            # Refresh room data to clear stale mob info
            bot.send_command("look")
            return BehaviorResult.COMPLETED
        
        # Use skills occasionally (30% chance)
        if self.skills and random.random() < 0.3:
            skill = random.choice(self.skills)
            bot.send_command(skill)
        
        # Auto-attack continues automatically, just wait
        return BehaviorResult.WAITING


import re
import time


class AttackBehavior(Behavior):
    """
    Initiate combat with appropriate targets.
    
    Selects targets based on:
    - Target whitelist (if provided)
    - Level difference (avoid too-strong mobs)
    - Current HP (don't start fights when hurt)
    
    Uses BOT protocol data when available for reliable target detection.
    
    After sending an attack command, enters a cooldown period to let the
    server respond and combat state to update.
    """
    
    priority = PRIORITY_ATTACK
    name = "Attack"
    tick_delay = 0.5
    
    # Mobs to avoid attacking
    AVOID_KEYWORDS = ['guard', 'cityguard', 'hassan', 'acolyte', 'adept',
                      'shopkeeper', 'healer', 'receptionist']
    
    # Mob flags that indicate we should not attack
    AVOID_FLAGS = ['pet', 'train', 'practice', 'healer', 'changer', 'skill_train']
    
    def __init__(self, targets: Optional[list[str]] = None, max_level_diff: int = 5):
        """
        Args:
            targets: List of mob name keywords to attack. If None, attacks
                    any attackable mob within level range.
            max_level_diff: Maximum level difference to attack.
        """
        super().__init__()
        self.targets = targets  # Specific mobs to target, or None for any
        self.max_level_diff = max_level_diff
        self._cooldown_until: float = 0.0
    
    def can_start(self, ctx: BehaviorContext) -> bool:
        """Start if not in combat, healthy enough, off cooldown, and there's a target."""
        # Check cooldown first
        if time.time() < self._cooldown_until:
            return False
        if ctx.in_combat:
            return False
        if not ctx.position.can_fight:
            return False
        if ctx.hp_percent < MIN_ATTACK_HP_PERCENT:
            return False
        
        # Only start if there's actually a target to attack
        if ctx.bot_mode and ctx.bot_mobs:
            for mob in ctx.bot_mobs:
                if self._can_attack_bot_mob(mob, ctx.level):
                    return True
            return False
        elif ctx.bot_mode:
            # Bot mode but no mob data yet - wait for look command to populate
            return False
        else:
            # Fall back to text parsing
            for mob_desc in ctx.room_mobs:
                if self._can_attack_text(mob_desc):
                    return True
            return False
    
    def tick(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        if ctx.in_combat:
            return BehaviorResult.COMPLETED
        
        # Try BOT protocol data first (more reliable)
        if ctx.bot_mode and ctx.bot_mobs:
            for mob in ctx.bot_mobs:
                if self._can_attack_bot_mob(mob, ctx.level):
                    # Use first word of mob name as keyword
                    keyword = mob.name.split()[0]
                    logger.info(f"[{bot.bot_id}] Attacking: {mob.name} lvl{mob.level} "
                               f"hp={mob.hp_percent:.0f}% (using '{keyword}')")
                    bot.send_command(f"kill {keyword}")
                    # Cooldown to let server respond and combat state update
                    self._cooldown_until = time.time() + 2.0
                    return BehaviorResult.COMPLETED
        else:
            # Fall back to text-based targeting
            for mob_desc in ctx.room_mobs:
                if self._can_attack_text(mob_desc):
                    keyword = self._get_target_keyword(mob_desc)
                    logger.info(f"[{bot.bot_id}] Attacking: {mob_desc} (using '{keyword}')")
                    bot.send_command(f"kill {keyword}")
                    self._cooldown_until = time.time() + 2.0
                    return BehaviorResult.COMPLETED
        
        return BehaviorResult.FAILED
    
    def _can_attack_bot_mob(self, mob, player_level: int) -> bool:
        """Check if we can attack a mob using structured bot data."""
        # Check if mob is too high level
        if player_level > 0 and mob.level > player_level + self.max_level_diff:
            return False
        
        # Check flags for mobs we should avoid
        for flag in self.AVOID_FLAGS:
            if flag in mob.flags:
                return False
        
        # Check avoid keywords
        name_lower = mob.name.lower()
        for avoid in self.AVOID_KEYWORDS:
            if avoid in name_lower:
                return False
        
        # If target list specified, only attack those
        if self.targets:
            for target in self.targets:
                if target.lower() in name_lower:
                    return True
            return False
        
        return True
    
    def _can_attack_text(self, mob_desc: str) -> bool:
        """Check if we can attack a mob from text description."""
        mob_lower = mob_desc.lower()
        
        # Check avoid list
        for avoid in self.AVOID_KEYWORDS:
            if avoid in mob_lower:
                return False
        
        # If target list specified, only attack those
        if self.targets:
            for target in self.targets:
                if target.lower() in mob_lower:
                    return True
            return False
        
        return True
    
    def _get_target_keyword(self, mob_desc: str) -> str:
        """Extract a keyword to target the mob."""
        skip_words = {'a', 'an', 'the', 'is', 'are', 'here', 'there', 'to',
                      'in', 'on', 'at', 'for', 'and', 'but', 'or', 'of',
                      'leashed', 'resting', 'sleeping', 'standing', 'sitting',
                      'waiting', 'watching', 'looking'}
        
        # Strip ANSI codes and parenthetical prefixes like "(White Aura)"
        clean = re.sub(r'\x1b\[[0-9;]*m', '', mob_desc)
        clean = re.sub(r'\([^)]+\)\s*', '', clean)
        
        keywords = clean.lower().split()
        for word in keywords:
            word_clean = word.strip('.,!?;:')
            if word_clean and word_clean not in skip_words:
                return word_clean
        
        return keywords[-1] if keywords else "mob"
