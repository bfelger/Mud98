"""
Behavior Engine - Selection and Execution.

The BehaviorEngine manages a collection of behaviors, selecting the highest
priority one that can run each tick. It also tracks persistent state like
hunger/thirst that persists across behavior changes.
"""

from __future__ import annotations

import logging
from typing import TYPE_CHECKING, Optional

from .base import Behavior, BehaviorContext, BehaviorResult
from ..config import MAX_TEXT_BUFFER_LINES

if TYPE_CHECKING:
    from ..bot import Bot

logger = logging.getLogger(__name__)


class BehaviorEngine:
    """
    Manages behavior selection and execution.
    
    Behaviors are sorted by priority and the highest priority
    behavior that can run is selected each tick.
    
    The engine maintains:
    - A sorted list of behaviors
    - The currently active behavior
    - A text buffer for parsing game output
    - Persistent hunger/thirst state
    - Proactive shopping trigger flag
    
    Attributes:
        bot: The bot instance being controlled.
    """
    
    def __init__(self, bot: 'Bot'):
        self.bot = bot
        self._behaviors: list[Behavior] = []
        self._current_behavior: Optional[Behavior] = None
        self._last_room_text: str = ""
        # Accumulate recent text for behaviors that need to parse multiple chunks
        self._text_buffer: list[str] = []
        self._max_buffer_lines = MAX_TEXT_BUFFER_LINES
        # Hunger/thirst state persists until eating/drinking
        self._is_hungry: bool = False
        self._is_thirsty: bool = False
        # Proactive shopping trigger (set when patrol circuit completes)
        self._should_proactive_shop: bool = False
    
    def add_behavior(self, behavior: Behavior) -> None:
        """Add a behavior to the engine."""
        self._behaviors.append(behavior)
        # Keep sorted by priority (highest first)
        self._behaviors.sort(key=lambda b: -b.priority)
        logger.debug(f"[{self.bot.bot_id}] Added behavior: {behavior.name} (priority {behavior.priority})")
    
    def remove_behavior(self, behavior_name: str) -> None:
        """Remove a behavior by name."""
        self._behaviors = [b for b in self._behaviors if b.name != behavior_name]
    
    def get_behavior(self, behavior_name: str) -> Optional[Behavior]:
        """Get a behavior by name."""
        for b in self._behaviors:
            if b.name == behavior_name:
                return b
        return None
    
    def reset_needs_state(self) -> None:
        """Reset hunger, thirst, and proactive shopping state.
        
        Called during hard bot reset to ensure a clean slate.
        """
        self._is_hungry = False
        self._is_thirsty = False
        self._should_proactive_shop = False
        logger.debug(f"[{self.bot.bot_id}] Reset hunger/thirst/proactive shop state")
    
    def update_room_text(self, text: str) -> None:
        """Update the last room text for mob/item parsing."""
        self._last_room_text = text
        # Also append to buffer for behaviors that need recent history
        if text:
            lines = text.split('\n')
            self._text_buffer.extend(lines)
            # Trim to max size
            if len(self._text_buffer) > self._max_buffer_lines:
                self._text_buffer = self._text_buffer[-self._max_buffer_lines:]
            
            # Check for hunger/thirst messages
            text_lower = text.lower()
            if 'you are hungry' in text_lower:
                if not self._is_hungry:
                    logger.info(f"[{self.bot.bot_id}] Detected: hungry!")
                self._is_hungry = True
            if 'you are thirsty' in text_lower:
                if not self._is_thirsty:
                    logger.info(f"[{self.bot.bot_id}] Detected: thirsty!")
                self._is_thirsty = True
            # Check for eating/drinking messages to clear state
            if 'you are no longer hungry' in text_lower or 'you eat' in text_lower:
                if self._is_hungry:
                    logger.info(f"[{self.bot.bot_id}] No longer hungry")
                self._is_hungry = False
            if 'you are no longer thirsty' in text_lower or 'you drink' in text_lower:
                if self._is_thirsty:
                    logger.info(f"[{self.bot.bot_id}] No longer thirsty")
                self._is_thirsty = False
    
    def clear_text_buffer(self) -> None:
        """Clear the text buffer (e.g., after processing loot)."""
        self._text_buffer.clear()
    
    def get_text_buffer(self) -> str:
        """Get all accumulated text as a single string."""
        return '\n'.join(self._text_buffer)
    
    def get_context(self) -> BehaviorContext:
        """Build current behavior context from bot state."""
        stats = self.bot.stats
        room = self.bot.room
        
        ctx = BehaviorContext(
            health=stats.health,
            health_max=stats.health_max,
            mana=stats.mana,
            mana_max=stats.mana_max,
            movement=stats.movement,
            movement_max=stats.movement_max,
            level=stats.level,
            experience=stats.experience,
            money=stats.money,
            in_combat=stats.in_combat,
            opponent_name=stats.opponent_name,
            opponent_health=stats.opponent_health,
            opponent_health_max=stats.opponent_health_max,
            opponent_level=stats.opponent_level,
            room_vnum=stats.room_vnum,
            room_exits=room.exits.copy(),
            position=stats.position,
            room_mobs=[],
            room_items=[],
            has_corpse=False,
        )
        
        # Use BOT protocol data if available (more reliable for room contents)
        if self.bot.bot_mode and self.bot.bot_data:
            bot_data = self.bot.bot_data
            ctx.bot_mode = True
            ctx.bot_mobs = bot_data.mobs
            ctx.bot_objects = bot_data.objects
            ctx.bot_exits = bot_data.exits
            
            if bot_data.mobs:
                logger.debug(f"[{self.bot.bot_id}] BOT data: room={ctx.room_vnum}, "
                           f"mobs={[m.name for m in bot_data.mobs]}")
            elif ctx.room_vnum == 3713:
                # Debug: why no mobs in the cage room?
                logger.debug(f"[{self.bot.bot_id}] BOT data: room=3713 but NO MOBS! "
                           f"objects={len(bot_data.objects)}, exits={len(bot_data.exits)}")
            
            if bot_data.room:
                ctx.bot_room_flags = bot_data.room.flags
                ctx.bot_sector = bot_data.room.sector
                # NOTE: We do NOT override room_vnum from BOT data because 
                # BOT data is only updated on 'look', while MSDP ROOM_VNUM
                # updates instantly on room change. For navigation, we need
                # the instant update from MSDP.
            
            # Populate exits from bot data (still useful for exit checking)
            if bot_data.exits:
                ctx.room_exits = [e.direction for e in bot_data.exits]
            
            # Populate room_mobs from bot data for text-based behaviors
            for mob in bot_data.mobs:
                ctx.room_mobs.append(mob.name)
            
            # Check for corpses in objects
            for obj in bot_data.objects:
                # Item types are 'npccorpse' or 'pc_corpse'
                if 'corpse' in obj.item_type.lower() or 'corpse' in obj.name.lower():
                    ctx.has_corpse = True
                    break
        else:
            # Fall back to text parsing
            if ctx.room_vnum == 3713:
                logger.warning(f"[{self.bot.bot_id}] At room 3713 but NO BOT MODE! "
                           f"bot_mode={self.bot.bot_mode}, bot_data={self.bot.bot_data}")
            self._update_context_from_text(ctx, self._last_room_text)
        
        # Add last received text for behaviors that need to parse output
        # Use the accumulated buffer for more complete text history
        ctx.last_text = self.get_text_buffer()
        
        # Set hunger/thirst state
        ctx.is_hungry = self._is_hungry
        ctx.is_thirsty = self._is_thirsty
        
        # Set proactive shopping flag
        ctx.should_proactive_shop = self._should_proactive_shop
        
        return ctx
    
    def _update_context_from_text(self, ctx: BehaviorContext, text: str) -> None:
        """Update context by parsing room text for mobs/items."""
        lines = text.split('\n')
        
        for line in lines:
            line = line.strip()
            if not line:
                continue
            
            # Detect corpses
            if 'corpse of' in line.lower() or 'corpse is' in line.lower():
                ctx.has_corpse = True
            
            # Common mob indicators (lines describing mobs present)
            mob_indicators = [
                'is here', 'are here', 'stands here', 'stand here',
                'is leashed here', 'leashed here',  # "There is a monster leashed here."
                'is resting here', 'is sleeping here',
                'welcomes you', 'waiting'
            ]
            
            for indicator in mob_indicators:
                if indicator in line.lower():
                    # This line likely describes a mob
                    ctx.room_mobs.append(line)
                    break
    
    def tick(self) -> Optional[str]:
        """
        Run one tick of the behavior engine.
        
        Returns the name of the current behavior, or None if idle.
        """
        ctx = self.get_context()
        
        # Log context state for debugging (INFO level for room 3713)
        if ctx.room_vnum == 3713:
            logger.debug(f"[{self.bot.bot_id}] Tick at 3713: bot_mode={ctx.bot_mode}, "
                       f"bot_mobs={len(ctx.bot_mobs) if ctx.bot_mobs else 0}, "
                       f"in_combat={ctx.in_combat}, position={ctx.position.name}, "
                       f"current={self._current_behavior.name if self._current_behavior else 'None'}")
        else:
            logger.debug(f"[{self.bot.bot_id}] Tick: room={ctx.room_vnum}, bot_mode={ctx.bot_mode}, "
                        f"bot_mobs={len(ctx.bot_mobs) if ctx.bot_mobs else 0}, "
                        f"in_combat={ctx.in_combat}, position={ctx.position.name}")
        
        # Check for higher priority behaviors that should interrupt
        for behavior in self._behaviors:
            can_start = behavior.can_start(ctx)
            logger.debug(f"[{self.bot.bot_id}] Behavior {behavior.name} (pri={behavior.priority}): can_start={can_start}")
            if can_start:
                if (self._current_behavior is None or 
                    behavior.priority > self._current_behavior.priority):
                    
                    # Interrupt current behavior
                    if self._current_behavior and self._current_behavior != behavior:
                        logger.debug(f"[{self.bot.bot_id}] Interrupting {self._current_behavior.name} "
                                   f"with {behavior.name}")
                        self._current_behavior.stop()
                    
                    # Start new behavior
                    if not behavior._started:
                        behavior.start(self.bot, ctx)
                    self._current_behavior = behavior
                    break
        
        # Run current behavior
        if self._current_behavior:
            if self._current_behavior.should_tick():
                result = self._current_behavior.tick(self.bot, ctx)
                
                if result in (BehaviorResult.COMPLETED, BehaviorResult.FAILED):
                    self._current_behavior.stop()
                    name = self._current_behavior.name
                    self._current_behavior = None
                    return name
            
            return self._current_behavior.name
        
        return None
    
    @property
    def current_behavior_name(self) -> Optional[str]:
        """Name of the currently active behavior, or None."""
        if self._current_behavior:
            return self._current_behavior.name
        return None
