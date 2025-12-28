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
    PRIORITY_DARK_CREATURE,
    ROUTE_TO_CAGE_ROOM, ROUTE_TO_NORTH_CAGE, ROUTE_TO_SOUTH_CAGE,
    ROUTE_TO_EAST_CAGE, ROUTE_TO_WEST_CAGE,
    ROUTE_CREATURE_TO_CAGE, ROUTE_TEMPLE_TO_CORRIDOR,
    CENTRAL_ROOM, CAGE_ROOMS, PATROL_ROOMS,
    NORTH_CAGE_ROOM, SOUTH_CAGE_ROOM, EAST_CAGE_ROOM, WEST_CAGE_ROOM,
    CORRIDOR_ROOM, DARK_CREATURE_ROOM, MUD_SCHOOL_ENTRY, INTERMEDIATE_ROOM,
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
        one_shot: bool = False,
        priority_override: Optional[int] = None,
    ):
        """
        Args:
            route: Dictionary mapping room VNUM to exit direction.
            destination_vnum: The target room VNUM.
            on_arrival: Optional callback when destination reached.
            one_shot: If True, disable behavior permanently after reaching target once.
            priority_override: Optional priority override for this instance.
        """
        super().__init__()
        self.route = route
        self.destination_vnum = destination_vnum
        self.on_arrival = on_arrival
        self.one_shot = one_shot
        if priority_override is not None:
            self.priority = priority_override
        self._completed = False
        self._permanently_done = False  # For one_shot mode
        self._ever_at_target = False  # Track if we've ever been at target
        self._stuck_count = 0
        self._last_room = 0
    
    def can_start(self, ctx: BehaviorContext) -> bool:
        """Start if not at destination, on route, and not permanently done."""
        # If one_shot and we've already reached target, never run again
        if self._permanently_done:
            return False
        # Track if we've ever been at the target room
        if ctx.room_vnum == self.destination_vnum:
            self._ever_at_target = True
            # For one_shot, mark as done so we don't keep re-checking
            if self.one_shot:
                self._permanently_done = True
            return False
        # For one_shot, also check if we've EVER been at target (handles
        # case where bot passed through target and left before tick completed)
        if self.one_shot and self._ever_at_target:
            self._permanently_done = True
            return False
        # Don't navigate during combat
        if ctx.in_combat:
            return False
        # Only navigate if we're on the route - don't try to navigate from
        # unknown rooms (e.g., cage rooms that aren't on this route)
        if ctx.room_vnum not in self.route:
            return False
        return True
    
    def tick(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        if ctx.in_combat:
            return BehaviorResult.WAITING
        
        # Check if arrived
        if ctx.room_vnum == self.destination_vnum:
            logger.info(f"[{bot.bot_id}] Arrived at destination {self.destination_vnum}")
            self._completed = True
            self._ever_at_target = True
            # If one_shot, mark as permanently done so we never run again
            if self.one_shot:
                self._permanently_done = True
            # Send look to ensure bot_data is populated for this room
            bot.send_command("look")
            if self.on_arrival:
                try:
                    self.on_arrival()
                except Exception as e:
                    logger.error(f"[{bot.bot_id}] on_arrival callback error: {e}")
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
        """Reset for re-use (does NOT reset one_shot permanently_done)."""
        self._completed = False
        self._stuck_count = 0
        self._last_room = 0
        # Note: _permanently_done and _ever_at_target are intentionally NOT reset
        # This preserves one_shot behavior across resets


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


class FightDarkCreatureBehavior(Behavior):
    """
    Navigate to dark creature room (3720), fight creature, unlock door, enter MUD School.
    
    This behavior activates after proactive shopping with lantern. The bot
    should be at the corridor (3719) and will:
    1. Enter the dark room (3720) north
    2. Fight the 'big creature'  
    3. Let looting happen (via LootBehavior) - gets key
    4. Go south back to corridor (3719)
    5. Unlock east door with key
    6. Open east door
    7. Go east into MUD School (3721)
    
    The creature drops a key needed to unlock the door to MUD School proper.
    """
    
    priority = PRIORITY_DARK_CREATURE
    name = "FightDarkCreature"
    tick_delay = 0.5
    
    # State machine
    STATE_GO_TO_DARK_ROOM = 'go_to_dark_room'
    STATE_FIGHT = 'fight'
    STATE_WAIT_LOOT = 'wait_loot'
    STATE_GO_TO_CORRIDOR = 'go_to_corridor'
    STATE_UNLOCK = 'unlock'
    STATE_OPEN = 'open'
    STATE_ENTER_SCHOOL = 'enter_school'
    
    def __init__(self):
        super().__init__()
        self._state = self.STATE_GO_TO_DARK_ROOM
        self._fight_started = False
        self._creature_defeated = False  # Track if we've beaten the creature
        self._wait_ticks = 0
    
    def can_start(self, ctx: BehaviorContext) -> bool:
        """Start when dark creature flag is set."""
        if not ctx.should_fight_dark_creature:
            return False
        if ctx.in_combat:
            return False
        return True
    
    def start(self, bot: 'Bot', ctx: BehaviorContext) -> None:
        super().start(bot, ctx)
        # If we've already defeated the creature, resume from where we were
        # (behavior may restart after LootBehavior completes)
        if self._creature_defeated:
            logger.info(f"[{bot.bot_id}] Resuming dark creature quest from state {self._state}")
            return
        
        self._state = self.STATE_GO_TO_DARK_ROOM
        self._fight_started = False
        self._wait_ticks = 0
        logger.info(f"[{bot.bot_id}] Starting dark creature hunt from room {ctx.room_vnum}")
    
    def tick(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        if self._state == self.STATE_GO_TO_DARK_ROOM:
            return self._navigate_to_dark_room(bot, ctx)
        elif self._state == self.STATE_FIGHT:
            return self._fight_creature(bot, ctx)
        elif self._state == self.STATE_WAIT_LOOT:
            return self._wait_for_loot(bot, ctx)
        elif self._state == self.STATE_GO_TO_CORRIDOR:
            return self._go_to_corridor(bot, ctx)
        elif self._state == self.STATE_UNLOCK:
            return self._unlock_door(bot, ctx)
        elif self._state == self.STATE_OPEN:
            return self._open_door(bot, ctx)
        elif self._state == self.STATE_ENTER_SCHOOL:
            return self._enter_school(bot, ctx)
        return BehaviorResult.FAILED
    
    def _navigate_to_dark_room(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        """Navigate from corridor (3719) to dark room (3720)."""
        if ctx.room_vnum == DARK_CREATURE_ROOM:
            logger.info(f"[{bot.bot_id}] Entered dark creature room (3720)")
            self._state = self.STATE_FIGHT
            return BehaviorResult.CONTINUE
        
        if ctx.room_vnum == CORRIDOR_ROOM:
            # Go north to dark room
            if not ctx.position.can_move:
                bot.send_command("wake")
                bot.send_command("stand")
                return BehaviorResult.CONTINUE
            bot.send_command("north")
            return BehaviorResult.CONTINUE
        
        # Somewhere unexpected - try to navigate via route
        logger.warning(f"[{bot.bot_id}] Unexpected room {ctx.room_vnum} on way to dark room")
        # If we're at intermediate (3717), go east
        if ctx.room_vnum == 3717:
            bot.send_command("east")
        else:
            bot.send_command("recall")
        return BehaviorResult.CONTINUE
    
    def _fight_creature(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        """Fight the creature in the dark room."""
        # Track if we've entered combat (combat has started at least once)
        if not hasattr(self, '_combat_entered'):
            self._combat_entered = False
        
        # If we're in combat, mark that combat has started and wait
        if ctx.in_combat:
            self._combat_entered = True
            logger.debug(f"[{bot.bot_id}] In combat with dark creature, letting CombatBehavior handle it")
            return BehaviorResult.WAITING
        
        # If combat was entered and now we're not in combat
        if self._combat_entered and not ctx.in_combat:
            self._combat_entered = False  # Reset for next use
            
            # Check if we're still in the dark creature room - if so, we won!
            if ctx.room_vnum == DARK_CREATURE_ROOM:
                logger.info(f"[{bot.bot_id}] Dark creature defeated!")
                self._creature_defeated = True
                # Refresh room data so LootBehavior can detect the corpse
                bot.send_command("look")
                self._state = self.STATE_WAIT_LOOT
                self._wait_ticks = 0
                return BehaviorResult.CONTINUE
            else:
                # We fled or died - reset and abandon the dark creature quest for now
                logger.warning(f"[{bot.bot_id}] Fled/died during dark creature fight, resetting behavior")
                self._fight_started = False
                self._creature_defeated = False
                # Clear the dark creature flag so patrol can resume
                if hasattr(bot, '_behavior_engine') and bot._behavior_engine:
                    bot._behavior_engine._should_fight_dark_creature = False
                return BehaviorResult.FAILED
        
        # If not in combat and haven't started fight, start it
        if not self._fight_started:
            # Look for creature to attack
            target = None
            if ctx.bot_mode and ctx.bot_mobs:
                for mob in ctx.bot_mobs:
                    if 'creature' in mob.name.lower():
                        target = mob.name.split()[0]
                        break
            if not target:
                # Fall back to generic "creature" keyword
                target = "creature"
            
            logger.info(f"[{bot.bot_id}] Attacking dark creature: '{target}'")
            bot.send_command(f"kill {target}")
            self._fight_started = True
            self._wait_ticks = 0
            return BehaviorResult.CONTINUE
        
        # Waiting for combat to start (MSDP takes ~1-2 seconds)
        self._wait_ticks += 1
        if self._wait_ticks % 4 == 0:  # Log every 2 seconds
            logger.debug(f"[{bot.bot_id}] Waiting for combat to start (tick {self._wait_ticks})")
        
        # If combat hasn't started after 10 seconds (20 ticks), something's wrong
        if self._wait_ticks > 20:
            logger.warning(f"[{bot.bot_id}] Combat never started after 10 seconds, retrying attack")
            self._fight_started = False
            self._wait_ticks = 0
        
        return BehaviorResult.CONTINUE
    
    def _wait_for_loot(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        """Wait for LootBehavior to do its thing, then go to corridor."""
        self._wait_ticks += 1
        
        # Give LootBehavior time to work (5 ticks = 2.5 seconds)
        if self._wait_ticks < 5:
            return BehaviorResult.WAITING
        
        logger.info(f"[{bot.bot_id}] Done waiting for loot, heading to corridor")
        self._state = self.STATE_GO_TO_CORRIDOR
        return BehaviorResult.CONTINUE
    
    def _go_to_corridor(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        """Navigate to corridor (3719), handling respawn at Temple if needed."""
        if ctx.room_vnum == CORRIDOR_ROOM:
            logger.info(f"[{bot.bot_id}] At corridor (3719), unlocking east door")
            self._state = self.STATE_UNLOCK
            self._wait_ticks = 0
            return BehaviorResult.CONTINUE
        
        if ctx.room_vnum == DARK_CREATURE_ROOM:
            if not ctx.position.can_move:
                bot.send_command("wake")
                bot.send_command("stand")
                return BehaviorResult.CONTINUE
            bot.send_command("south")
            return BehaviorResult.CONTINUE
        
        # Handle intermediate room (need to open door first)
        if ctx.room_vnum == INTERMEDIATE_ROOM:
            logger.debug(f"[{bot.bot_id}] At intermediate room, opening door and going east")
            bot.send_command("open east")
            bot.send_command("east")
            return BehaviorResult.CONTINUE
        
        # Use route from Temple to Corridor for other locations
        if ctx.room_vnum in ROUTE_TEMPLE_TO_CORRIDOR:
            direction = ROUTE_TEMPLE_TO_CORRIDOR[ctx.room_vnum]
            logger.debug(f"[{bot.bot_id}] Navigating to corridor: {ctx.room_vnum} -> {direction}")
            bot.send_command(direction)
            return BehaviorResult.CONTINUE
        
        # Truly unexpected location - try recall and navigate from Temple
        logger.warning(f"[{bot.bot_id}] Lost on way to corridor (room {ctx.room_vnum}), recalling")
        bot.send_command("recall")
        return BehaviorResult.CONTINUE
    
    def _unlock_door(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        """Unlock the east door with the key."""
        self._wait_ticks += 1
        
        if self._wait_ticks == 1:
            logger.info(f"[{bot.bot_id}] Unlocking east door...")
            bot.send_command("unlock east")
            return BehaviorResult.CONTINUE
        
        # Wait a tick for unlock to complete
        if self._wait_ticks >= 2:
            self._state = self.STATE_OPEN
            self._wait_ticks = 0
        return BehaviorResult.CONTINUE
    
    def _open_door(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        """Open the east door."""
        self._wait_ticks += 1
        
        if self._wait_ticks == 1:
            logger.info(f"[{bot.bot_id}] Opening east door...")
            bot.send_command("open east")
            return BehaviorResult.CONTINUE
        
        # Wait a tick for open to complete
        if self._wait_ticks >= 2:
            self._state = self.STATE_ENTER_SCHOOL
            self._wait_ticks = 0
        return BehaviorResult.CONTINUE
    
    def _enter_school(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        """Enter MUD School through the east door."""
        # Check if we've entered MUD School
        if ctx.room_vnum == MUD_SCHOOL_ENTRY:
            logger.info(f"[{bot.bot_id}] Entered MUD School (3721)! Dark creature quest complete.")
            # Clear the flag and reset state
            if hasattr(bot, '_behavior_engine') and bot._behavior_engine:
                bot._behavior_engine._should_fight_dark_creature = False
            self._creature_defeated = False  # Reset for next time
            return BehaviorResult.COMPLETED
        
        if ctx.room_vnum == CORRIDOR_ROOM:
            if not ctx.position.can_move:
                bot.send_command("wake")
                bot.send_command("stand")
                return BehaviorResult.CONTINUE
            logger.info(f"[{bot.bot_id}] Going east into MUD School...")
            bot.send_command("east")
            return BehaviorResult.CONTINUE
        
        # Unexpected location - try to navigate back
        logger.warning(f"[{bot.bot_id}] Unexpected room {ctx.room_vnum} trying to enter school")
        if ctx.room_vnum in ROUTE_TEMPLE_TO_CORRIDOR:
            direction = ROUTE_TEMPLE_TO_CORRIDOR[ctx.room_vnum]
            logger.info(f"[{bot.bot_id}] Navigating back to corridor: {ctx.room_vnum} -> {direction}")
            bot.send_command(direction)
            return BehaviorResult.CONTINUE
        return BehaviorResult.FAILED
