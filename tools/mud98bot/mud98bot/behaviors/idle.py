"""
Idle Behavior - Fallback when nothing else to do.

This module contains the lowest priority behavior that runs
when no other behavior is active.
"""

from __future__ import annotations

import logging
import random
from typing import TYPE_CHECKING

from .base import Behavior, BehaviorContext, BehaviorResult
from ..config import PRIORITY_IDLE

if TYPE_CHECKING:
    from ..bot import Bot

logger = logging.getLogger(__name__)


class IdleBehavior(Behavior):
    """
    Fallback behavior when nothing else to do.
    
    Performs occasional random actions (look, score, inventory) to
    show the bot is still active. Very low priority - only runs when
    no other behavior wants to.
    """
    
    priority = PRIORITY_IDLE
    name = "Idle"
    tick_delay = 3.0
    
    # Actions to perform randomly while idle
    IDLE_ACTIONS = ['look', 'score', 'inventory']
    
    def __init__(self, actions: list[str] | None = None):
        """
        Args:
            actions: Custom list of idle actions. Defaults to look/score/inventory.
        """
        super().__init__()
        self.actions = actions or self.IDLE_ACTIONS
        self._idle_count = 0
    
    def can_start(self, ctx: BehaviorContext) -> bool:
        """Always available as fallback."""
        return True
    
    def tick(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        self._idle_count += 1
        
        # Do something random occasionally
        if self._idle_count % 3 == 0:
            action = random.choice(self.actions)
            bot.send_command(action)
        
        return BehaviorResult.WAITING
