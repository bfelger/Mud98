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
    MAX_LEVEL_DIFFERENCE, MIN_ATTACK_HP_PERCENT,
)

if TYPE_CHECKING:
    from ..bot import Bot

logger = logging.getLogger(__name__)


class CombatBehavior(Behavior):
    """
    Continue fighting when in combat.
    
    This behavior maintains an ongoing fight by issuing attack commands
    and optionally using skills. It has high priority to ensure combat
    continues without interruption.
    """
    
    priority = PRIORITY_COMBAT
    name = "Combat"
    tick_delay = 0.5  # Fast ticks during combat
    
    def __init__(self, skills: Optional[list[str]] = None):
        super().__init__()
        self.skills = skills or []
        self._skill_index = 0
    
    def can_start(self, ctx: BehaviorContext) -> bool:
        """Start when in combat."""
        return ctx.in_combat
    
    def tick(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        if not ctx.in_combat:
            return BehaviorResult.COMPLETED
        
        # Use skills if available, otherwise just attack
        if self.skills:
            skill = self.skills[self._skill_index % len(self.skills)]
            self._skill_index += 1
            bot.send_command(skill)
        # The game auto-attacks, so we just need to stay in the fight
        
        return BehaviorResult.CONTINUE


class AttackBehavior(Behavior):
    """
    Initiate combat with appropriate targets.
    
    Selects targets based on:
    - Target whitelist (if provided)
    - Level difference (avoid too-strong mobs)
    - Current HP (don't start fights when hurt)
    
    Uses BOT protocol data when available for reliable target detection.
    """
    
    priority = PRIORITY_ATTACK
    name = "Attack"
    tick_delay = 1.0
    
    def __init__(self, targets: Optional[list[str]] = None):
        """
        Args:
            targets: List of mob name keywords to attack. If None, attacks
                    any attackable mob within level range.
        """
        super().__init__()
        self.targets = targets  # Specific mobs to target, or None for any
    
    def can_start(self, ctx: BehaviorContext) -> bool:
        """Start if not in combat and healthy enough."""
        if ctx.in_combat:
            return False
        if not ctx.position.can_fight:
            return False
        if ctx.hp_percent < MIN_ATTACK_HP_PERCENT:
            return False
        return True
    
    def tick(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        if ctx.in_combat:
            return BehaviorResult.COMPLETED
        
        # Find a target
        target = self._find_target(ctx)
        
        if target:
            logger.info(f"[{bot.bot_id}] Attacking target: {target}")
            bot.send_command(f"kill {target}")
            return BehaviorResult.CONTINUE
        
        return BehaviorResult.WAITING
    
    def _find_target(self, ctx: BehaviorContext) -> Optional[str]:
        """Find a suitable target to attack."""
        # Use BOT protocol data if available (most reliable)
        if ctx.bot_mode and ctx.bot_mobs:
            for mob in ctx.bot_mobs:
                if self._is_valid_target(mob.name, mob.level, ctx):
                    # Return first keyword of mob name for 'kill' command
                    return mob.name.split()[0].lower()
        
        # Fall back to text-based detection
        for mob_desc in ctx.room_mobs:
            if self._is_valid_target_text(mob_desc, ctx):
                # Extract first word as keyword
                words = mob_desc.split()
                if words:
                    return words[0].lower()
        
        return None
    
    def _is_valid_target(self, name: str, level: int, ctx: BehaviorContext) -> bool:
        """Check if a mob is a valid target by name and level."""
        name_lower = name.lower()
        
        # Skip non-attackable mobs
        if any(x in name_lower for x in ['shopkeeper', 'guard', 'healer', 'receptionist', 'adept']):
            return False
        
        # Check level difference
        if abs(level - ctx.level) > MAX_LEVEL_DIFFERENCE:
            return False
        
        # Check target whitelist
        if self.targets:
            return any(t.lower() in name_lower for t in self.targets)
        
        return True
    
    def _is_valid_target_text(self, desc: str, ctx: BehaviorContext) -> bool:
        """Check if a mob description represents a valid target."""
        desc_lower = desc.lower()
        
        # Skip non-attackable mobs
        if any(x in desc_lower for x in ['shopkeeper', 'guard', 'healer', 'receptionist', 'adept']):
            return False
        
        # Check target whitelist
        if self.targets:
            return any(t.lower() in desc_lower for t in self.targets)
        
        # Accept any mob if no whitelist
        return True
