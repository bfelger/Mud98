"""
Base Behavior Classes and Context.

This module provides the foundation for the behavior system:
- BehaviorContext: Dataclass containing game state for decision-making
- BehaviorResult: Enum for behavior tick outcomes
- Behavior: Abstract base class for all behaviors

All behaviors inherit from Behavior and implement can_start() and tick().
"""

from __future__ import annotations

import logging
import time
from abc import ABC, abstractmethod
from dataclasses import dataclass, field
from enum import Enum, auto
from typing import TYPE_CHECKING, Optional

if TYPE_CHECKING:
    from ..bot import Bot, BotMob, BotObject, BotExit
    from ..msdp import PositionState

logger = logging.getLogger(__name__)


@dataclass
class BehaviorContext:
    """
    Snapshot of game state for behavior decision-making.
    
    Behaviors receive this context to decide whether to start and what
    actions to take. The context is rebuilt fresh each tick from the bot's
    current state.
    
    Attributes:
        health/mana/movement: Current and max vital stats
        level: Character level (affects target selection)
        experience: Current experience points
        money: Gold on hand (for shopping)
        in_combat: Whether actively fighting
        opponent_*: Info about current opponent (if any)
        room_vnum: Current room virtual number
        room_exits: Available exit directions
        position: Current position (standing, sitting, sleeping, etc.)
        room_mobs: Mob descriptions from room text
        room_items: Item descriptions from room text
        has_corpse: Whether there's a lootable corpse
        last_text: Recent text buffer for parsing
        bot_mode: Whether BOT protocol is active
        bot_mobs/bot_objects/bot_exits: Structured data from BOT protocol
        is_hungry/is_thirsty: Hunger/thirst status
        should_proactive_shop: Whether patrol circuit completed (buy supplies)
    """
    
    # Vital stats
    health: int = 0
    health_max: int = 100
    mana: int = 0
    mana_max: int = 100
    movement: int = 0
    movement_max: int = 100
    
    # Character info
    level: int = 1
    experience: int = 0
    money: int = 0
    
    # Combat state
    in_combat: bool = False
    opponent_name: str = ""
    opponent_health: int = 0
    opponent_health_max: int = 0
    opponent_level: int = 0
    
    # Location
    room_vnum: int = 0
    room_exits: list[str] = field(default_factory=list)
    position: Optional['PositionState'] = None
    
    # Room contents (text-parsed)
    room_mobs: list[str] = field(default_factory=list)
    room_items: list[str] = field(default_factory=list)
    has_corpse: bool = False
    
    # Text buffer
    last_text: str = ""
    
    # BOT protocol structured data
    bot_mode: bool = False
    bot_mobs: list['BotMob'] = field(default_factory=list)
    bot_objects: list['BotObject'] = field(default_factory=list)
    bot_exits: list['BotExit'] = field(default_factory=list)
    bot_room_flags: list[str] = field(default_factory=list)
    bot_sector: str = ""
    
    # Hunger/thirst state
    is_hungry: bool = False
    is_thirsty: bool = False
    
    # Proactive shopping (set when patrol circuit completes)
    should_proactive_shop: bool = False
    
    @property
    def hp_percent(self) -> float:
        """Current HP as percentage of max."""
        if self.health_max <= 0:
            return 100.0
        return (self.health / self.health_max) * 100.0
    
    @property
    def mana_percent(self) -> float:
        """Current mana as percentage of max."""
        if self.mana_max <= 0:
            return 100.0
        return (self.mana / self.mana_max) * 100.0
    
    @property
    def move_percent(self) -> float:
        """Current movement as percentage of max."""
        if self.movement_max <= 0:
            return 100.0
        return (self.movement / self.movement_max) * 100.0


class BehaviorResult(Enum):
    """Result of a behavior tick."""
    
    CONTINUE = auto()
    """Behavior is ongoing, will continue next tick."""
    
    COMPLETED = auto()
    """Behavior finished successfully."""
    
    FAILED = auto()
    """Behavior failed and should stop."""
    
    WAITING = auto()
    """Behavior is waiting for external event."""


class Behavior(ABC):
    """
    Abstract base class for bot behaviors.
    
    Behaviors encapsulate a specific goal or task (combat, healing, looting,
    navigation, etc.). The BehaviorEngine selects the highest priority
    behavior that can currently run.
    
    Class Attributes:
        priority: Higher = more important. Used to preempt lower behaviors.
        name: Human-readable identifier.
        tick_delay: Minimum seconds between ticks (rate limiting).
    
    To create a new behavior:
        1. Inherit from Behavior
        2. Set priority, name, and optionally tick_delay
        3. Implement can_start(ctx) -> bool
        4. Implement tick(bot, ctx) -> BehaviorResult
    """
    
    priority: int = 0
    name: str = "Unknown"
    tick_delay: float = 1.0
    
    def __init__(self):
        self._started: bool = False
        self._last_tick: float = 0.0
    
    @abstractmethod
    def can_start(self, ctx: BehaviorContext) -> bool:
        """
        Check if this behavior should take control.
        
        Called every engine tick for all behaviors. The highest priority
        behavior that returns True will run.
        
        Args:
            ctx: Current game state snapshot.
            
        Returns:
            True if behavior wants to run, False otherwise.
        """
        pass
    
    @abstractmethod
    def tick(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        """
        Execute one tick of this behavior.
        
        Called when this behavior is the active one. Should issue commands
        and return the current state.
        
        Args:
            bot: The bot instance to control.
            ctx: Current game state snapshot.
            
        Returns:
            BehaviorResult indicating whether to continue, complete, or fail.
        """
        pass
    
    def start(self, bot: 'Bot', ctx: BehaviorContext) -> None:
        """
        Called when behavior becomes active.
        
        Override to perform setup when taking control.
        """
        self._started = True
        logger.debug(f"[{bot.bot_id}] Behavior {self.name} started")
    
    def stop(self) -> None:
        """
        Called when behavior loses control.
        
        Override to perform cleanup when preempted or completing.
        """
        self._started = False
    
    def should_tick(self) -> bool:
        """
        Check if enough time has passed for next tick.
        
        Enforces tick_delay to prevent spamming commands.
        """
        now = time.time()
        if now - self._last_tick >= self.tick_delay:
            self._last_tick = now
            return True
        return False
