"""
Navigation Behaviors - Movement and Patrolling.

This module contains behaviors for navigation:
- NavigateBehavior: Goal-oriented navigation using routes
- NavigationTask: Helper for chaining waypoints
- ExploreBehavior: Random exploration
- PatrolCagesBehavior: Circuit patrol of cage rooms
- ReturnToCageBehavior: Return home after displacement
"""

from __future__ import annotations

import logging
import random
from typing import TYPE_CHECKING, Optional

from .base import Behavior, BehaviorContext, BehaviorResult
from ..config import (
    PRIORITY_NAVIGATE, PRIORITY_EXPLORE, PRIORITY_PATROL, PRIORITY_RETURN_TO_CAGE,
    ROUTE_TO_CAGE_ROOM, ROUTE_TO_NORTH_CAGE, ROUTE_TO_SOUTH_CAGE,
    ROUTE_TO_EAST_CAGE, ROUTE_TO_WEST_CAGE,
    CENTRAL_ROOM, CAGE_ROOMS, PATROL_ROOMS,
    NORTH_CAGE_ROOM, SOUTH_CAGE_ROOM, EAST_CAGE_ROOM, WEST_CAGE_ROOM,
    PATROL_LINGER_TICKS, MAX_WANDER_ATTEMPTS, MIN_ATTACK_HP_PERCENT,
    PATROL_SEQUENCE, CAGE_EXIT_DIRECTIONS,
)

if TYPE_CHECKING:
    from ..bot import Bot

logger = logging.getLogger(__name__)


class NavigateBehavior(Behavior):
    """
    Navigate to a destination using a predefined route.
    
    Takes a route dictionary (VNUM -> direction) and follows it
    to reach a destination room.
    """
    
    priority = PRIORITY_NAVIGATE
    name = "Navigate"
    tick_delay = 0.5
    
    def __init__(
        self,
        route: dict[int, str],
        destination_vnum: int,
        on_arrival: Optional[callable] = None,
    ):
        """
        Args:
            route: Dictionary mapping room VNUM to exit direction.
            destination_vnum: The target room VNUM.
            on_arrival: Optional callback when destination reached.
        """
        super().__init__()
        self.route = route
        self.destination_vnum = destination_vnum
        self.on_arrival = on_arrival
        self._completed = False
        self._stuck_count = 0
        self._last_room = 0
    
    def can_start(self, ctx: BehaviorContext) -> bool:
        """Start if not at destination and not completed."""
        if self._completed:
            return False
        if ctx.in_combat:
            return False
        return ctx.room_vnum != self.destination_vnum
    
    def tick(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        if ctx.in_combat:
            return BehaviorResult.WAITING
        
        # Check if arrived
        if ctx.room_vnum == self.destination_vnum:
            logger.info(f"[{bot.bot_id}] Arrived at destination {self.destination_vnum}")
            self._completed = True
            if self.on_arrival:
                self.on_arrival()
            return BehaviorResult.COMPLETED
        
        # Check if stuck
        if ctx.room_vnum == self._last_room:
            self._stuck_count += 1
            if self._stuck_count >= 5:
                logger.warning(f"[{bot.bot_id}] Stuck at room {ctx.room_vnum}, aborting navigation")
                return BehaviorResult.FAILED
        else:
            self._stuck_count = 0
            self._last_room = ctx.room_vnum
        
        # Get next direction
        if ctx.room_vnum in self.route:
            direction = self.route[ctx.room_vnum]
            logger.debug(f"[{bot.bot_id}] Navigating: room {ctx.room_vnum} -> {direction}")
            
            # Make sure we can move
            if not ctx.position.can_move:
                bot.send_command("wake")
                bot.send_command("stand")
                return BehaviorResult.CONTINUE
            
            bot.send_command(direction)
            return BehaviorResult.CONTINUE
        else:
            # Not on route - try recalling
            logger.warning(f"[{bot.bot_id}] Room {ctx.room_vnum} not in route, recalling")
            bot.send_command("recall")
            return BehaviorResult.CONTINUE
    
    def reset(self) -> None:
        """Reset for re-use."""
        self._completed = False
        self._stuck_count = 0
        self._last_room = 0


class NavigationTask:
    """
    Helper for chaining navigation waypoints.
    
    Tracks a list of destinations and progresses through them.
    """
    
    def __init__(self, waypoints: list[tuple[dict[int, str], int]]):
        """
        Args:
            waypoints: List of (route, destination_vnum) tuples.
        """
        self.waypoints = waypoints
        self.current_index = 0
    
    @property
    def current_route(self) -> Optional[dict[int, str]]:
        if self.current_index < len(self.waypoints):
            return self.waypoints[self.current_index][0]
        return None
    
    @property
    def current_destination(self) -> Optional[int]:
        if self.current_index < len(self.waypoints):
            return self.waypoints[self.current_index][1]
        return None
    
    def advance(self) -> bool:
        """Move to next waypoint. Returns True if more waypoints remain."""
        self.current_index += 1
        return self.current_index < len(self.waypoints)
    
    def is_complete(self) -> bool:
        """Check if all waypoints visited."""
        return self.current_index >= len(self.waypoints)
    
    def reset(self) -> None:
        """Reset to beginning."""
        self.current_index = 0


class ExploreBehavior(Behavior):
    """
    Random exploration when nothing else to do.
    
    Wanders randomly through available exits. Low priority fallback
    when no other navigation behavior is active.
    """
    
    priority = PRIORITY_EXPLORE
    name = "Explore"
    tick_delay = 2.0
    
    def __init__(self, safe_rooms: Optional[set[int]] = None):
        """
        Args:
            safe_rooms: Optional set of "safe" room VNUMs to avoid leaving.
        """
        super().__init__()
        self.safe_rooms = safe_rooms
        self._wander_attempts = 0
    
    def can_start(self, ctx: BehaviorContext) -> bool:
        """Start when not in combat and can move."""
        if ctx.in_combat:
            return False
        if not ctx.position.can_move:
            return False
        return True
    
    def tick(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        if ctx.in_combat:
            return BehaviorResult.WAITING
        
        if not ctx.room_exits:
            logger.warning(f"[{bot.bot_id}] No exits in room {ctx.room_vnum}")
            self._wander_attempts += 1
            if self._wander_attempts >= MAX_WANDER_ATTEMPTS:
                return BehaviorResult.FAILED
            return BehaviorResult.WAITING
        
        # Pick a random exit
        direction = random.choice(ctx.room_exits)
        logger.debug(f"[{bot.bot_id}] Exploring: {direction}")
        bot.send_command(direction)
        
        self._wander_attempts = 0
        return BehaviorResult.CONTINUE


class PatrolCagesBehavior(Behavior):
    """
    Patrol the four cage rooms in a circuit.
    
    Visits each cage room in sequence, killing any monsters present,
    then returns to central room. Triggers proactive shopping after
    completing a full circuit.
    
    Patrol order: North -> East -> South -> West -> (repeat)
    """
    
    priority = PRIORITY_PATROL
    name = "Patrol"
    tick_delay = 0.5
    
    # Cage room configuration
    PATROL_ORDER = PATROL_SEQUENCE
    
    # Direction to enter each cage from central room
    CAGE_DIRECTIONS = {
        NORTH_CAGE_ROOM: 'north',
        EAST_CAGE_ROOM: 'south',   # Confusing but accurate
        SOUTH_CAGE_ROOM: 'west',
        WEST_CAGE_ROOM: 'east',
    }
    
    # Direction to exit each cage back to central
    EXIT_DIRECTIONS = CAGE_EXIT_DIRECTIONS
    
    def __init__(self):
        super().__init__()
        self._current_cage_index = 0
        self._in_cage = False
        self._linger_ticks = 0
        self._circuits_completed = 0
    
    def can_start(self, ctx: BehaviorContext) -> bool:
        """Start when in cage area and healthy."""
        if ctx.in_combat:
            return False
        if not ctx.position.can_move:
            return False
        if ctx.hp_percent < MIN_ATTACK_HP_PERCENT:
            return False
        
        # Only start from patrol rooms
        return ctx.room_vnum in PATROL_ROOMS
    
    def tick(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        if ctx.in_combat:
            return BehaviorResult.WAITING
        
        current_target = self.PATROL_ORDER[self._current_cage_index]
        
        # Phase: In a cage room
        if ctx.room_vnum in CAGE_ROOMS:
            self._in_cage = True
            
            # Check for attackable mobs
            if self._has_attackable_mob(ctx):
                return BehaviorResult.WAITING  # Let AttackBehavior handle it
            
            # Linger briefly after combat
            self._linger_ticks += 1
            if self._linger_ticks < PATROL_LINGER_TICKS:
                return BehaviorResult.CONTINUE
            
            # Exit cage
            exit_dir = self.EXIT_DIRECTIONS.get(ctx.room_vnum)
            if exit_dir:
                logger.debug(f"[{bot.bot_id}] Patrol: exiting cage {ctx.room_vnum} via {exit_dir}")
                bot.send_command(exit_dir)
                self._in_cage = False
                self._linger_ticks = 0
                
                # Advance to next cage
                self._current_cage_index = (self._current_cage_index + 1) % len(self.PATROL_ORDER)
                
                # Check for circuit completion
                if self._current_cage_index == 0:
                    self._circuits_completed += 1
                    logger.info(f"[{bot.bot_id}] Patrol circuit #{self._circuits_completed} complete!")
                    self._trigger_proactive_shopping(bot)
            
            return BehaviorResult.CONTINUE
        
        # Phase: At central room, enter next cage
        if ctx.room_vnum == CENTRAL_ROOM:
            direction = self.CAGE_DIRECTIONS.get(current_target)
            if direction:
                logger.debug(f"[{bot.bot_id}] Patrol: entering cage {current_target} via {direction}")
                bot.send_command(direction)
            return BehaviorResult.CONTINUE
        
        # Unexpected location
        logger.warning(f"[{bot.bot_id}] Patrol: unexpected room {ctx.room_vnum}")
        return BehaviorResult.FAILED
    
    def _has_attackable_mob(self, ctx: BehaviorContext) -> bool:
        """Check if there's an attackable mob in the room."""
        if not ctx.bot_mode or not ctx.bot_mobs:
            return False
        
        for mob in ctx.bot_mobs:
            name_lower = mob.name.lower()
            # Skip non-attackable mobs
            if any(x in name_lower for x in ['shopkeeper', 'guard', 'healer', 'receptionist', 'adept']):
                continue
            # Found an attackable mob
            return True
        
        return False
    
    def _trigger_proactive_shopping(self, bot: 'Bot') -> None:
        """Trigger proactive shopping after circuit completion."""
        if hasattr(bot, '_behavior_engine') and bot._behavior_engine:
            bot._behavior_engine._should_proactive_shop = True
            logger.info(f"[{bot.bot_id}] Triggered proactive shopping")


class ReturnToCageBehavior(Behavior):
    """
    Return to cage area after being displaced.
    
    If the bot finds itself outside the normal patrol area (e.g., after
    death/recall), navigate back to the central cage room.
    """
    
    priority = PRIORITY_RETURN_TO_CAGE
    name = "ReturnToCage"
    tick_delay = 0.5
    
    def __init__(self, route: Optional[dict[int, str]] = None):
        """
        Args:
            route: Custom route to cage area. Defaults to ROUTE_TO_CAGE_ROOM.
        """
        super().__init__()
        self.route = route or ROUTE_TO_CAGE_ROOM
        self._navigating = False
    
    def can_start(self, ctx: BehaviorContext) -> bool:
        """Start when outside cage area and healthy enough."""
        if ctx.in_combat:
            return False
        if not ctx.position.can_move:
            return False
        if ctx.hp_percent < MIN_ATTACK_HP_PERCENT:
            return False
        
        # Don't start if already in patrol area
        if ctx.room_vnum in PATROL_ROOMS:
            return False
        
        # Check if we know how to get there
        return ctx.room_vnum in self.route
    
    def tick(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        if ctx.in_combat:
            return BehaviorResult.WAITING
        
        # Check if arrived
        if ctx.room_vnum == CENTRAL_ROOM:
            logger.info(f"[{bot.bot_id}] Returned to cage area!")
            return BehaviorResult.COMPLETED
        
        # Navigate
        if ctx.room_vnum in self.route:
            direction = self.route[ctx.room_vnum]
            
            if not ctx.position.can_move:
                bot.send_command("wake")
                bot.send_command("stand")
                return BehaviorResult.CONTINUE
            
            logger.debug(f"[{bot.bot_id}] Returning to cage: room {ctx.room_vnum} -> {direction}")
            bot.send_command(direction)
            return BehaviorResult.CONTINUE
        else:
            logger.warning(f"[{bot.bot_id}] Lost on way to cage at room {ctx.room_vnum}")
            bot.send_command("recall")
            return BehaviorResult.CONTINUE
