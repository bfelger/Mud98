"""
Behavior engine for Mud98Bot.

Implements priority-based behavior selection and execution.
"""

import logging
import random
import time
from abc import ABC, abstractmethod
from typing import Optional, TYPE_CHECKING
from dataclasses import dataclass, field
from enum import Enum, auto

if TYPE_CHECKING:
    from .client import Bot
    from .parser import BotMob, BotObj, BotExit, BotRoom

from .msdp import Position

logger = logging.getLogger(__name__)


@dataclass
class BehaviorContext:
    """Context passed to behaviors for decision making."""
    # Bot stats (from MSDP)
    health: int = 0
    health_max: int = 0
    mana: int = 0
    mana_max: int = 0
    movement: int = 0
    movement_max: int = 0
    level: int = 0
    experience: int = 0
    money: int = 0
    
    # Combat state
    in_combat: bool = False
    opponent_name: str = ""
    opponent_health: int = 0
    opponent_health_max: int = 0
    opponent_level: int = 0
    
    # Room state
    room_vnum: int = 0
    room_exits: list[str] = field(default_factory=list)
    
    # Character position state
    position: Position = Position.STANDING
    
    # Parsed from text
    room_mobs: list[str] = field(default_factory=list)
    room_items: list[str] = field(default_factory=list)
    has_corpse: bool = False
    
    # BOT protocol data (structured, from PLR_BOT flag)
    bot_mode: bool = False  # True if using structured BOT protocol
    bot_mobs: list = field(default_factory=list)  # list[BotMob]
    bot_objects: list = field(default_factory=list)  # list[BotObj]
    bot_exits: list = field(default_factory=list)  # list[BotExit]
    bot_room_flags: list[str] = field(default_factory=list)
    bot_sector: str = ""
    
    @property
    def hp_percent(self) -> float:
        if self.health_max <= 0:
            return 100.0
        return (self.health / self.health_max) * 100.0
    
    @property
    def mana_percent(self) -> float:
        if self.mana_max <= 0:
            return 100.0
        return (self.mana / self.mana_max) * 100.0
    
    @property
    def move_percent(self) -> float:
        if self.movement_max <= 0:
            return 100.0
        return (self.movement / self.movement_max) * 100.0


class BehaviorResult(Enum):
    """Result of a behavior's tick."""
    CONTINUE = auto()   # Behavior is still running
    COMPLETED = auto()  # Behavior finished successfully
    FAILED = auto()     # Behavior failed
    WAITING = auto()    # Behavior is waiting (no command this tick)


class Behavior(ABC):
    """
    Abstract base class for bot behaviors.
    
    Behaviors are priority-ordered actions the bot can take.
    Higher priority behaviors preempt lower priority ones.
    """
    
    # Priority (higher = more important)
    priority: int = 0
    
    # Name for logging
    name: str = "Behavior"
    
    # Minimum time between ticks (seconds)
    tick_delay: float = 0.5
    
    def __init__(self):
        self._last_tick = 0.0
        self._started = False
        
    @abstractmethod
    def can_start(self, ctx: BehaviorContext) -> bool:
        """Check if this behavior can start given current context."""
        pass
    
    @abstractmethod
    def tick(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        """
        Execute one tick of the behavior.
        
        Returns a BehaviorResult indicating the behavior's state.
        """
        pass
    
    def start(self, bot: 'Bot', ctx: BehaviorContext) -> None:
        """Called when behavior is starting."""
        self._started = True
        logger.debug(f"[{bot.bot_id}] Starting behavior: {self.name}")
    
    def stop(self) -> None:
        """Called when behavior is being stopped."""
        self._started = False
    
    def should_tick(self) -> bool:
        """Check if enough time has passed for next tick."""
        now = time.time()
        if now - self._last_tick >= self.tick_delay:
            self._last_tick = now
            return True
        return False


class SurviveBehavior(Behavior):
    """Emergency survival - flee when HP is critical."""
    
    priority = 100
    name = "Survive"
    tick_delay = 0.25
    
    def __init__(self, flee_hp_percent: float = 20.0):
        super().__init__()
        self.flee_hp_percent = flee_hp_percent
        self._flee_attempts = 0
    
    def can_start(self, ctx: BehaviorContext) -> bool:
        return (ctx.in_combat and ctx.hp_percent <= self.flee_hp_percent)
    
    def start(self, bot: 'Bot', ctx: BehaviorContext) -> None:
        super().start(bot, ctx)
        self._flee_attempts = 0
        logger.warning(f"[{bot.bot_id}] HP critical ({ctx.hp_percent:.0f}%) - initiating flee!")
    
    def tick(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        if not ctx.in_combat:
            logger.info(f"[{bot.bot_id}] Successfully escaped combat!")
            return BehaviorResult.COMPLETED
        
        if self._flee_attempts >= 5:
            logger.error(f"[{bot.bot_id}] Could not flee after 5 attempts!")
            bot.send_command("recall")
            return BehaviorResult.FAILED
        
        self._flee_attempts += 1
        logger.warning(f"[{bot.bot_id}] Fleeing (attempt {self._flee_attempts})...")
        bot.send_command("flee")
        return BehaviorResult.CONTINUE


class CombatBehavior(Behavior):
    """Continue combat until enemy is dead."""
    
    priority = 80
    name = "Combat"
    tick_delay = 1.0  # Match combat round timing
    
    def __init__(self, use_skills: Optional[list[str]] = None):
        super().__init__()
        self.use_skills = use_skills or []
    
    def can_start(self, ctx: BehaviorContext) -> bool:
        return ctx.in_combat
    
    def tick(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        if not ctx.in_combat:
            logger.info(f"[{bot.bot_id}] Combat ended - victory!")
            return BehaviorResult.COMPLETED
        
        # Use skills occasionally (30% chance)
        if self.use_skills and random.random() < 0.3:
            skill = random.choice(self.use_skills)
            bot.send_command(skill)
        
        # Auto-attack continues automatically, just wait
        return BehaviorResult.WAITING


class HealBehavior(Behavior):
    """Rest/sleep to recover HP and movement when out of combat."""
    
    priority = 70
    name = "Heal"
    tick_delay = 2.0  # Regen ticks every ~15 seconds in ROM
    
    def __init__(self, rest_hp_percent: float = 50.0, 
                 sleep_hp_percent: float = 30.0,
                 target_hp_percent: float = 90.0,
                 rest_move_percent: float = 20.0,
                 target_move_percent: float = 80.0):
        super().__init__()
        self.rest_hp_percent = rest_hp_percent
        self.sleep_hp_percent = sleep_hp_percent
        self.target_hp_percent = target_hp_percent
        self.rest_move_percent = rest_move_percent
        self.target_move_percent = target_move_percent
        self._is_resting = False
        self._reason = ""  # Track why we're resting
    
    def can_start(self, ctx: BehaviorContext) -> bool:
        if ctx.in_combat:
            return False
        # Start if HP is low
        if ctx.hp_percent < self.rest_hp_percent:
            return True
        # Start if movement is low
        if ctx.move_percent < self.rest_move_percent:
            return True
        # Continue if we're already resting and haven't recovered
        if self._is_resting:
            if ctx.hp_percent < self.target_hp_percent:
                return True
            if ctx.move_percent < self.target_move_percent:
                return True
        return False
    
    def start(self, bot: 'Bot', ctx: BehaviorContext) -> None:
        super().start(bot, ctx)
        self._is_resting = False
        if ctx.hp_percent < self.rest_hp_percent:
            self._reason = f"HP: {ctx.hp_percent:.0f}%"
        else:
            self._reason = f"Move: {ctx.move_percent:.0f}%"
        logger.info(f"[{bot.bot_id}] Need to rest ({self._reason})")
    
    def tick(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        if ctx.in_combat:
            if self._is_resting:
                bot.send_command("wake")
            return BehaviorResult.FAILED
        
        # Check if we've recovered enough
        hp_ok = ctx.hp_percent >= self.target_hp_percent
        move_ok = ctx.move_percent >= self.target_move_percent
        
        if hp_ok and move_ok:
            if self._is_resting:
                bot.send_command("stand")
                self._is_resting = False
            logger.info(f"[{bot.bot_id}] Recovered (HP: {ctx.hp_percent:.0f}%, Move: {ctx.move_percent:.0f}%)")
            return BehaviorResult.COMPLETED
        
        if not self._is_resting:
            if ctx.hp_percent < self.sleep_hp_percent:
                logger.debug(f"[{bot.bot_id}] Sleeping (HP: {ctx.hp_percent:.0f}%)")
                bot.send_command("sleep")
            else:
                logger.debug(f"[{bot.bot_id}] Resting (HP: {ctx.hp_percent:.0f}%, Move: {ctx.move_percent:.0f}%)")
                bot.send_command("rest")
            self._is_resting = True
        else:
            logger.debug(f"[{bot.bot_id}] Still resting... HP: {ctx.hp_percent:.0f}%, Move: {ctx.move_percent:.0f}%")
        
        return BehaviorResult.CONTINUE
    
    def stop(self) -> None:
        super().stop()
        self._is_resting = False


class LootBehavior(Behavior):
    """Loot and sacrifice corpses after combat."""
    
    priority = 75  # Higher than heal so we loot before resting
    name = "Loot"
    tick_delay = 0.5
    
    def __init__(self):
        super().__init__()
        self._looted = False
        self._sacrificed = False
        self._cooldown_until = 0.0
    
    def can_start(self, ctx: BehaviorContext) -> bool:
        import time
        if time.time() < self._cooldown_until:
            return False
        return (not ctx.in_combat and ctx.has_corpse)
    
    def start(self, bot: 'Bot', ctx: BehaviorContext) -> None:
        super().start(bot, ctx)
        self._looted = False
        self._sacrificed = False
        logger.info(f"[{bot.bot_id}] Looting corpse...")
    
    def tick(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        if not self._looted:
            bot.send_command("get all corpse")
            self._looted = True
            return BehaviorResult.CONTINUE
        
        if not self._sacrificed:
            bot.send_command("sacrifice corpse")
            self._sacrificed = True
            return BehaviorResult.CONTINUE
        
        # Wear any new equipment
        bot.send_command("wear all")
        
        # Cooldown to avoid re-triggering on stale room text
        import time
        self._cooldown_until = time.time() + 10.0
        
        return BehaviorResult.COMPLETED


class AttackBehavior(Behavior):
    """Initiate combat with mobs in the room."""
    
    priority = 60
    name = "Attack"
    tick_delay = 0.5
    
    # Mobs to avoid attacking (sanctuary, guard, etc.)
    AVOID_KEYWORDS = ['guard', 'cityguard', 'hassan', 'acolyte', 'adept', 
                      'shopkeeper', 'healer', 'receptionist']
    
    # Mob act flags that indicate we should not attack
    AVOID_FLAGS = ['sentinel', 'aggressive', 'pet']  # sentinel might be fine, aggressive warns us
    
    def __init__(self, target_keywords: Optional[list[str]] = None, 
                 max_level_diff: int = 5):
        super().__init__()
        # If specified, only attack these mobs
        self.target_keywords = target_keywords
        self.max_level_diff = max_level_diff
    
    def can_start(self, ctx: BehaviorContext) -> bool:
        if ctx.in_combat:
            return False
        if ctx.hp_percent < 50:  # Don't start fights when low HP
            return False
        # Can't attack if not standing
        if not ctx.position.can_move:
            return False
        
        # Prefer bot data if available
        if ctx.bot_mode and ctx.bot_mobs:
            for mob in ctx.bot_mobs:
                if self._can_attack_bot_mob(mob, ctx.level):
                    return True
        else:
            # Check for attackable mobs from text parsing
            for mob in ctx.room_mobs:
                if self._can_attack(mob):
                    return True
        return False
    
    def _can_attack_bot_mob(self, mob, player_level: int) -> bool:
        """Check if we can attack a mob using structured bot data."""
        # Check if mob is too high level
        if player_level > 0 and mob.level > player_level + self.max_level_diff:
            logger.debug(f"Mob {mob.name} level {mob.level} too high for level {player_level}")
            return False
        
        # Check flags for dangerous mobs (e.g., aggressive is fine to attack)
        for flag in self.AVOID_FLAGS:
            if flag in mob.flags:
                logger.debug(f"Avoiding mob {mob.name} due to flag: {flag}")
                return False
        
        # Check avoid keywords
        for avoid in self.AVOID_KEYWORDS:
            if avoid in mob.name.lower():
                return False
        
        # If target list specified, only attack those
        if self.target_keywords:
            for target in self.target_keywords:
                if target.lower() in mob.name.lower():
                    return True
            return False
        
        return True
    
    def _can_attack(self, mob_name: str) -> bool:
        mob_lower = mob_name.lower()
        
        # Check avoid list
        for avoid in self.AVOID_KEYWORDS:
            if avoid in mob_lower:
                return False
        
        # If target list specified, only attack those
        if self.target_keywords:
            for target in self.target_keywords:
                if target.lower() in mob_lower:
                    return True
            return False
        
        return True
    
    def _get_target_keyword(self, mob_name: str) -> str:
        """Extract a keyword to target the mob."""
        # Common words to skip when looking for a target keyword
        skip_words = {'a', 'an', 'the', 'is', 'are', 'here', 'there', 'to', 
                      'in', 'on', 'at', 'for', 'and', 'but', 'or', 'of',
                      'leashed', 'resting', 'sleeping', 'standing', 'sitting',
                      'waiting', 'watching', 'looking'}
        
        # Strip ANSI codes and parenthetical prefixes like "(White Aura)"
        clean = mob_name
        # Remove ANSI escape sequences
        import re
        clean = re.sub(r'\x1b\[[0-9;]*m', '', clean)
        # Remove parenthetical prefixes
        clean = re.sub(r'\([^)]+\)\s*', '', clean)
        
        keywords = clean.lower().split()
        for word in keywords:
            # Skip common non-target words
            if word in skip_words:
                continue
            # Skip punctuation-only words
            word_clean = word.strip('.,!?;:')
            if word_clean and word_clean not in skip_words:
                return word_clean
        
        # Fallback to last word
        return keywords[-1] if keywords else "mob"
    
    def tick(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        if ctx.in_combat:
            return BehaviorResult.COMPLETED
        
        # Prefer bot data for precise targeting
        if ctx.bot_mode and ctx.bot_mobs:
            for mob in ctx.bot_mobs:
                if self._can_attack_bot_mob(mob, ctx.level):
                    # Use mob's name directly as keyword (it's from prototype name)
                    keyword = mob.name.split()[0]  # First word is usually the keyword
                    logger.info(f"[{bot.bot_id}] Attacking: {mob.name} lvl{mob.level} hp={mob.hp_percent}% (using '{keyword}')")
                    bot.send_command(f"kill {keyword}")
                    return BehaviorResult.COMPLETED
        else:
            # Fall back to text-based targeting
            for mob in ctx.room_mobs:
                if self._can_attack(mob):
                    keyword = self._get_target_keyword(mob)
                    logger.info(f"[{bot.bot_id}] Attacking: {mob} (using '{keyword}')")
                    bot.send_command(f"kill {keyword}")
                    return BehaviorResult.COMPLETED
        
        return BehaviorResult.FAILED


# =============================================================================
# Navigation Routes
# =============================================================================
# Route maps: from_vnum -> direction to reach a target
# These define waypoints for goal-oriented navigation

# Route from temple (3001) or recall to Mud School Cage Room (3712)
# Also includes routes from Training Room (3758) and Practice Room (3759)
ROUTE_TO_CAGE_ROOM: dict[int, str] = {
    3001: "up",       # Temple -> Mud School Entrance
    3700: "north",    # Entrance -> Hub Room (3757)
    3757: "north",    # Hub -> First Hallway (3701)
    3758: "east",     # Training Room -> Hub (3757)
    3759: "west",     # Practice Room -> Hub (3757)
    3701: "west",     # Turn west
    3702: "north",    # Continue north
    3703: "north",    # Continue north
    3709: "west",     # Turn west
    3711: "down",     # Go down
    # 3712 is the Cage Room - target reached
}

# Routes to individual cages from Cage Room (3712)
# These include return paths from other cages so bots can navigate between cages
# Cage layout based on school.are:
# - 3713: North cage (aggressive monster) - exit south to 3712
# - 3714: West cage (aggressive wimpy monster) - exit east to 3712
# - 3715: South cage (wimpy monster) - exit north to 3712
# - 3716: East cage (unmodified monster) - exit west to 3712

ROUTE_TO_NORTH_CAGE: dict[int, str] = {
    3712: "north",   # Cage room -> North cage (3713)
    3714: "east",    # West cage -> Cage room
    3715: "north",   # South cage -> Cage room
    3716: "west",    # East cage -> Cage room
}

ROUTE_TO_SOUTH_CAGE: dict[int, str] = {
    3712: "south",   # Cage room -> South cage (3715)
    3713: "south",   # North cage -> Cage room
    3714: "east",    # West cage -> Cage room
    3716: "west",    # East cage -> Cage room
}

ROUTE_TO_EAST_CAGE: dict[int, str] = {
    3712: "east",    # Cage room -> East cage (3716)
    3713: "south",   # North cage -> Cage room
    3714: "east",    # West cage -> Cage room
    3715: "north",   # South cage -> Cage room
}

ROUTE_TO_WEST_CAGE: dict[int, str] = {
    3712: "west",    # Cage room -> West cage (3714)
    3713: "south",   # North cage -> Cage room
    3715: "north",   # South cage -> Cage room
    3716: "west",    # East cage -> Cage room
}

# Routes to Training Room (3758) and Practice Room (3759) from temple
# These are early-game rooms for using train/practice points
TRAIN_ROOM_VNUM = 3758
PRACTICE_ROOM_VNUM = 3759

ROUTE_TO_TRAIN_ROOM: dict[int, str] = {
    3001: "up",       # Temple -> Mud School Entrance (3700)
    3700: "north",    # Entrance -> Hub room (3757)
    3757: "west",     # Hub -> Training Room (3758)
    # From practice room, go back to hub first
    3759: "west",     # Practice Room -> Hub (3757)
}

ROUTE_TO_PRACTICE_ROOM: dict[int, str] = {
    3001: "up",       # Temple -> Mud School Entrance (3700)
    3700: "north",    # Entrance -> Hub room (3757)
    3757: "east",     # Hub -> Practice Room (3759)
    # From training room, go back to hub first
    3758: "east",     # Training Room -> Hub (3757)
}


class NavigateBehavior(Behavior):
    """
    Goal-oriented navigation using waypoint maps.
    
    Instead of following a sequential path, this behavior uses a route map
    that says "if you're in room X, go direction Y to reach the goal."
    This makes navigation recoverable - if the bot gets displaced, it can
    find its way back.
    """
    
    priority = 45  # Higher than Explore, lower than Attack
    name = "Navigate"
    tick_delay = 0.5
    
    def __init__(self, 
                 target_vnum: int,
                 route: dict[int, str],
                 on_arrival: Optional[callable] = None):
        """
        Args:
            target_vnum: The goal room VNUM
            route: Dict mapping from_vnum -> direction
            on_arrival: Optional callback when target is reached
        """
        super().__init__()
        self.target_vnum = target_vnum
        self.route = route
        self.on_arrival = on_arrival
        self._arrived = False
        self._stuck_count = 0
        self._last_vnum = 0
    
    def can_start(self, ctx: BehaviorContext) -> bool:
        # Don't navigate if already at target
        if ctx.room_vnum == self.target_vnum:
            return False
        # Don't navigate during combat
        if ctx.in_combat:
            return False
        # Don't navigate with very low movement
        if ctx.move_percent < 10:
            return False
        # Only navigate if we know where we are and have a route
        return ctx.room_vnum in self.route
    
    def start(self, bot: 'Bot', ctx: BehaviorContext) -> None:
        super().start(bot, ctx)
        self._arrived = False
        self._stuck_count = 0
        self._last_vnum = ctx.room_vnum
        logger.info(f"[{bot.bot_id}] Navigating to room {self.target_vnum} from {ctx.room_vnum}")
    
    def tick(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        # Check if we've arrived
        if ctx.room_vnum == self.target_vnum:
            if not self._arrived:
                logger.info(f"[{bot.bot_id}] Arrived at target room {self.target_vnum}")
                self._arrived = True
                if self.on_arrival:
                    try:
                        self.on_arrival()
                    except Exception as e:
                        logger.error(f"[{bot.bot_id}] on_arrival callback error: {e}")
            return BehaviorResult.COMPLETED
        
        # Combat interrupts navigation
        if ctx.in_combat:
            return BehaviorResult.FAILED
        
        # Check position - can't move if not standing
        if not ctx.position.can_move:
            logger.debug(f"[{bot.bot_id}] Cannot move - position is {ctx.position.name}, waking up")
            bot.send_command("wake")  # Wake handles sleeping, resting, sitting
            bot.send_command("stand")
            return BehaviorResult.CONTINUE  # Wait for next tick
        
        # Check if we're stuck (same room multiple ticks)
        if ctx.room_vnum == self._last_vnum and ctx.room_vnum != 0:
            self._stuck_count += 1
            if self._stuck_count >= 5:
                logger.warning(f"[{bot.bot_id}] Stuck at room {ctx.room_vnum}, cannot navigate")
                return BehaviorResult.FAILED
        else:
            self._stuck_count = 0
            self._last_vnum = ctx.room_vnum
        
        # Find direction from current room
        if ctx.room_vnum not in self.route:
            # We're off the route - can't navigate from here
            logger.warning(f"[{bot.bot_id}] Room {ctx.room_vnum} not in route to {self.target_vnum}")
            return BehaviorResult.FAILED
        
        direction = self.route[ctx.room_vnum]
        
        # Verify the exit exists
        if direction not in ctx.room_exits:
            logger.warning(f"[{bot.bot_id}] Exit '{direction}' not available from room {ctx.room_vnum}, "
                         f"available exits: {ctx.room_exits}")
            return BehaviorResult.FAILED
        
        # Move!
        logger.debug(f"[{bot.bot_id}] Navigating: {ctx.room_vnum} -> {direction} -> {self.target_vnum}")
        bot.send_command(direction)
        # Send look to update BOT data after movement
        bot.send_command("look")
        return BehaviorResult.CONTINUE
    
    def reset(self) -> None:
        """Reset for reuse."""
        self._arrived = False
        self._stuck_count = 0
        self._last_vnum = 0


class NavigationTask:
    """
    A high-level navigation task that can chain multiple waypoints.
    
    Example usage:
        task = NavigationTask()
        task.add_waypoint(3712, ROUTE_TO_CAGE_ROOM)  # Get to cage room
        task.add_waypoint(3716, ROUTE_TO_EAST_CAGE)  # Then go to east cage
    """
    
    def __init__(self):
        self.waypoints: list[tuple[int, dict[int, str]]] = []
        self.current_index = 0
    
    def add_waypoint(self, target_vnum: int, route: dict[int, str]) -> 'NavigationTask':
        """Add a waypoint to the task."""
        self.waypoints.append((target_vnum, route))
        return self
    
    def get_current_target(self) -> Optional[tuple[int, dict[int, str]]]:
        """Get current waypoint (target_vnum, route)."""
        if self.current_index < len(self.waypoints):
            return self.waypoints[self.current_index]
        return None
    
    def advance(self) -> bool:
        """Move to next waypoint. Returns True if there are more."""
        self.current_index += 1
        return self.current_index < len(self.waypoints)
    
    def is_complete(self) -> bool:
        """Check if all waypoints have been reached."""
        return self.current_index >= len(self.waypoints)
    
    def reset(self) -> None:
        """Reset to start."""
        self.current_index = 0


class ExploreBehavior(Behavior):
    """Move around the world looking for mobs to fight."""
    
    priority = 40
    name = "Explore"
    tick_delay = 1.0
    
    # Directions in order of preference
    DIRECTIONS = ['north', 'east', 'south', 'west', 'up', 'down']
    
    def __init__(self, max_rooms: int = 100, avoid_vnums: Optional[set[int]] = None):
        super().__init__()
        self.max_rooms = max_rooms
        self.avoid_vnums = avoid_vnums or set()
        self.visited_vnums: set[int] = set()
        self._last_direction: Optional[str] = None
        self._stuck_count = 0
    
    def can_start(self, ctx: BehaviorContext) -> bool:
        return (not ctx.in_combat and 
                ctx.hp_percent > 50 and
                ctx.move_percent > 20 and
                len(ctx.room_exits) > 0)
    
    def start(self, bot: 'Bot', ctx: BehaviorContext) -> None:
        super().start(bot, ctx)
        if ctx.room_vnum:
            self.visited_vnums.add(ctx.room_vnum)
    
    def tick(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        if ctx.in_combat:
            return BehaviorResult.COMPLETED  # Let combat behavior take over
        
        if ctx.move_percent < 20:
            logger.info(f"[{bot.bot_id}] Low movement, stopping exploration")
            return BehaviorResult.COMPLETED
        
        if len(self.visited_vnums) >= self.max_rooms:
            logger.info(f"[{bot.bot_id}] Explored {self.max_rooms} rooms, done")
            return BehaviorResult.COMPLETED
        
        # Record current room
        if ctx.room_vnum:
            self.visited_vnums.add(ctx.room_vnum)
        
        # Choose next direction
        direction = self._choose_direction(ctx)
        if not direction:
            self._stuck_count += 1
            if self._stuck_count >= 3:
                logger.warning(f"[{bot.bot_id}] Stuck, recalling to temple")
                bot.send_command("recall")
                self._stuck_count = 0
                return BehaviorResult.CONTINUE
            return BehaviorResult.WAITING
        
        self._stuck_count = 0
        self._last_direction = direction
        bot.send_command(direction)
        logger.debug(f"[{bot.bot_id}] Moving {direction} (visited {len(self.visited_vnums)} rooms)")
        return BehaviorResult.CONTINUE
    
    def _choose_direction(self, ctx: BehaviorContext) -> Optional[str]:
        if not ctx.room_exits:
            return None
        
        # Avoid going back the way we came
        opposite = self._opposite_direction(self._last_direction)
        available = [e for e in ctx.room_exits if e != opposite]
        if not available:
            available = ctx.room_exits
        
        # If bot mode is active, use exit VNUMs to prefer unexplored rooms
        if ctx.bot_mode and ctx.bot_exits:
            unvisited = []
            for exit in ctx.bot_exits:
                if exit.direction in available and exit.vnum not in self.visited_vnums:
                    if exit.vnum not in self.avoid_vnums:
                        unvisited.append(exit.direction)
            if unvisited:
                direction = random.choice(unvisited)
                logger.debug(f"[{bot.bot_id}] BOT: Choosing unexplored direction {direction}")
                return direction
        
        # Fall back to random selection
        return random.choice(available)
    
    def _opposite_direction(self, direction: Optional[str]) -> Optional[str]:
        if not direction:
            return None
        opposites = {
            "north": "south", "south": "north",
            "east": "west", "west": "east",
            "up": "down", "down": "up",
        }
        return opposites.get(direction)


class BotResetBehavior(Behavior):
    """
    Perform a hard reset of the bot character at startup.
    
    This is a one-shot startup behavior that sends 'botreset hard' to
    completely reset the character to level 1 with fresh equipment,
    skills, and stats. After the reset, the character will be at the
    MUD school entrance (VNUM 3700).
    
    This replaces the need for separate train reset / practice reset
    commands and ensures each test session starts from a known baseline.
    """
    
    priority = 65  # Higher than Train/Practice so it runs first
    name = "BotReset"
    tick_delay = 0.5
    
    def __init__(self):
        super().__init__()
        self._completed = False
        self._sent_reset = False
    
    def can_start(self, ctx: BehaviorContext) -> bool:
        # One-shot behavior - only run if not completed
        if self._completed:
            return False
        # Don't start if in combat
        if ctx.in_combat:
            return False
        return True
    
    def tick(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        if ctx.in_combat:
            return BehaviorResult.WAITING
        
        if not self._sent_reset:
            bot.send_command("botreset hard")
            logger.info(f"[{bot.bot_id}] Sent 'botreset hard' to reset character to level 1")
            self._sent_reset = True
            return BehaviorResult.CONTINUE
        
        # Done - reset is complete
        logger.info(f"[{bot.bot_id}] Bot reset complete - character is now at MUD school entrance")
        self._completed = True
        return BehaviorResult.COMPLETED
    
    def reset(self) -> None:
        """Reset for re-use."""
        self._completed = False
        self._sent_reset = False


class TrainBehavior(Behavior):
    """
    Navigate to training room and train stats.
    
    This is a one-shot startup behavior that:
    1. Navigates to the Training Room (VNUM 3758)
    2. Optionally sends 'train reset' to reset stats/training points (bot-only)
    3. Sends 'train' commands for each stat in priority order
    4. Completes when done (allowing next startup behaviors to run)
    
    The behavior completes after attempting to train all stats once,
    regardless of whether the character had training sessions available.
    """
    
    priority = 62  # Above Navigate (50), but below Heal (70) so healing can interrupt
    name = "Train"
    tick_delay = 0.5
    
    # Yield to healing if movement drops below this threshold
    CRITICAL_MOVE_PERCENT = 15.0
    
    # Priority order for stats to train
    DEFAULT_TRAIN_ORDER = ['str', 'con', 'dex', 'wis', 'int']
    
    def __init__(self, stat_priority: Optional[list[str]] = None, 
                 route: Optional[dict[int, str]] = None,
                 reset_first: bool = True,
                 train_per_stat: int = 5):
        super().__init__()
        self.stat_priority = stat_priority or self.DEFAULT_TRAIN_ORDER
        self.route = route or ROUTE_TO_TRAIN_ROOM
        self.reset_first = reset_first  # Send 'train reset' before training
        self.train_per_stat = train_per_stat  # Times to train each stat
        self._navigating = True
        self._completed = False
        self._reset_sent = False
    
    def can_start(self, ctx: BehaviorContext) -> bool:
        # One-shot behavior - only run if not completed
        if self._completed:
            return False
        # Don't start if in combat
        if ctx.in_combat:
            return False
        # Yield to healing if movement is critical (let HealBehavior take over)
        if ctx.move_percent < self.CRITICAL_MOVE_PERCENT:
            return False
        # Must be able to move
        if not ctx.position.can_move:
            return False
        return True
    
    def tick(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        # If we were interrupted by combat, bail
        if ctx.in_combat:
            return BehaviorResult.WAITING
        
        # Phase 1: Navigate to training room
        if self._navigating:
            if ctx.room_vnum == TRAIN_ROOM_VNUM:
                # We've arrived at the training room
                logger.info(f"[{bot.bot_id}] Arrived at Training Room (VNUM {TRAIN_ROOM_VNUM})")
                self._navigating = False
                return BehaviorResult.CONTINUE
            
            # Need to make sure we're standing
            if not ctx.position.can_move:
                bot.send_command("wake")
                bot.send_command("stand")
                return BehaviorResult.CONTINUE
            
            # Navigate toward training room
            if ctx.room_vnum in self.route:
                direction = self.route[ctx.room_vnum]
                logger.debug(f"[{bot.bot_id}] Training nav: room {ctx.room_vnum} -> {direction}")
                bot.send_command(direction)
                return BehaviorResult.CONTINUE
            else:
                # Not on route - try recalling to get to temple
                logger.warning(f"[{bot.bot_id}] Training: room {ctx.room_vnum} not in route, recalling")
                bot.send_command("recall")
                return BehaviorResult.CONTINUE
        
        # Phase 2: Send reset command if enabled
        if self.reset_first and not self._reset_sent:
            bot.send_command("train reset")
            logger.info(f"[{bot.bot_id}] Sent 'train reset' to reset stats and training points")
            self._reset_sent = True
            return BehaviorResult.CONTINUE
        
        # Phase 3: Train stats - batch all commands in one tick
        # Bots are exempt from spam protection, so we can send many commands at once
        total_trained = 0
        for stat in self.stat_priority:
            for _ in range(self.train_per_stat):
                bot.send_command(f"train {stat}")
                total_trained += 1
        
        logger.info(f"[{bot.bot_id}] Training complete (sent {total_trained} train commands)")
        self._completed = True
        return BehaviorResult.COMPLETED
    
    def reset(self) -> None:
        """Reset for re-use."""
        self._navigating = True
        self._completed = False
        self._reset_sent = False


class PracticeBehavior(Behavior):
    """
    Navigate to practice room and practice skills.
    
    This is a one-shot startup behavior that:
    1. Navigates to the Practice Room (VNUM 3759)
    2. Optionally sends 'practice reset' to reset skills/practice points (bot-only)
    3. Sends 'practice' commands for each skill in the list
    4. Completes when done (allowing next startup behaviors to run)
    
    The behavior completes after attempting to practice all skills once.
    """
    
    priority = 61  # Slightly lower than Train (62), but above Navigate (50)
    name = "Practice"
    tick_delay = 0.5
    
    # Yield to healing if movement drops below this threshold
    CRITICAL_MOVE_PERCENT = 15.0
    
    # Default skills to practice (common combat skills)
    DEFAULT_SKILLS = [
        'sword', 'dagger', 'mace', 'axe', 'spear', 'polearm', 'whip', 'flail',
        'parry', 'dodge', 'shield block', 'second attack', 'third attack',
        'enhanced damage', 'fast healing', 'meditation',
        'recall', 'scrolls', 'staves', 'wands',
    ]
    
    def __init__(self, skills: Optional[list[str]] = None,
                 route: Optional[dict[int, str]] = None,
                 reset_first: bool = True,
                 practice_per_skill: int = 3):
        super().__init__()
        self.skills = skills or self.DEFAULT_SKILLS
        self.route = route or ROUTE_TO_PRACTICE_ROOM
        self.reset_first = reset_first  # Send 'practice reset' before practicing
        self.practice_per_skill = practice_per_skill  # Times to practice each skill
        self._navigating = True
        self._completed = False
        self._reset_sent = False
    
    def can_start(self, ctx: BehaviorContext) -> bool:
        # One-shot behavior - only run if not completed
        if self._completed:
            return False
        # Don't start if in combat
        if ctx.in_combat:
            return False
        # Yield to healing if movement is critical (let HealBehavior take over)
        if ctx.move_percent < self.CRITICAL_MOVE_PERCENT:
            return False
        # Must be able to move
        if not ctx.position.can_move:
            return False
        return True
    
    def tick(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        # If we were interrupted by combat, bail
        if ctx.in_combat:
            return BehaviorResult.WAITING
        
        # Phase 1: Navigate to practice room
        if self._navigating:
            if ctx.room_vnum == PRACTICE_ROOM_VNUM:
                # We've arrived at the practice room
                logger.info(f"[{bot.bot_id}] Arrived at Practice Room (VNUM {PRACTICE_ROOM_VNUM})")
                self._navigating = False
                return BehaviorResult.CONTINUE
            
            # Need to make sure we're standing
            if not ctx.position.can_move:
                bot.send_command("wake")
                bot.send_command("stand")
                return BehaviorResult.CONTINUE
            
            # Navigate toward practice room
            if ctx.room_vnum in self.route:
                direction = self.route[ctx.room_vnum]
                logger.debug(f"[{bot.bot_id}] Practice nav: room {ctx.room_vnum} -> {direction}")
                bot.send_command(direction)
                return BehaviorResult.CONTINUE
            else:
                # Not on route - try recalling to get to temple
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
        # Bots are exempt from spam protection, so we can send many commands at once
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


class ShopBehavior(Behavior):
    """Buy supplies from shops."""
    
    priority = 45
    name = "Shop"
    tick_delay = 0.5
    
    def __init__(self, buy_items: Optional[list[str]] = None):
        super().__init__()
        self.buy_items = buy_items or ['bread', 'water']
        self._at_shop = False
    
    def can_start(self, ctx: BehaviorContext) -> bool:
        # Would need hunger/thirst tracking - not in MSDP currently
        # For now, always return False
        return False
    
    def tick(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        # Future: navigate to shop and buy supplies
        return BehaviorResult.FAILED


class RecallBehavior(Behavior):
    """Emergency recall when flee fails - last resort escape."""
    
    priority = 95  # Very high - emergency only
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
        # Only activate if flee has failed and we're in danger
        return self._flee_failed and ctx.hp_percent < 30.0
    
    def tick(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        logger.info(f"[{bot.bot_id}] EMERGENCY RECALL! (Flee failed, HP: {ctx.hp_percent:.0f}%)")
        bot.send_command("recall")
        self._flee_failed = False
        return BehaviorResult.COMPLETED


class IdleBehavior(Behavior):
    """Fallback behavior when nothing else to do."""
    
    priority = 10
    name = "Idle"
    tick_delay = 3.0
    
    def __init__(self):
        super().__init__()
        self._idle_count = 0
    
    def can_start(self, ctx: BehaviorContext) -> bool:
        return True  # Always available as fallback
    
    def tick(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        self._idle_count += 1
        
        # Do something random occasionally
        if self._idle_count % 3 == 0:
            action = random.choice(['look', 'score', 'inventory'])
            bot.send_command(action)
        
        return BehaviorResult.WAITING


class BehaviorEngine:
    """
    Manages behavior selection and execution.
    
    Behaviors are sorted by priority and the highest priority
    behavior that can run is selected.
    """
    
    def __init__(self, bot: 'Bot'):
        self.bot = bot
        self._behaviors: list[Behavior] = []
        self._current_behavior: Optional[Behavior] = None
        self._last_room_text: str = ""
    
    def add_behavior(self, behavior: Behavior) -> None:
        """Add a behavior to the engine."""
        self._behaviors.append(behavior)
        # Keep sorted by priority (highest first)
        self._behaviors.sort(key=lambda b: -b.priority)
        logger.debug(f"[{self.bot.bot_id}] Added behavior: {behavior.name} (priority {behavior.priority})")
    
    def remove_behavior(self, behavior_name: str) -> None:
        """Remove a behavior by name."""
        self._behaviors = [b for b in self._behaviors if b.name != behavior_name]
    
    def update_room_text(self, text: str) -> None:
        """Update the last room text for mob/item parsing."""
        self._last_room_text = text
    
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
                if 'corpse' in obj.item_type.lower():
                    ctx.has_corpse = True
                    break
        else:
            # Fall back to text parsing
            self._update_context_from_text(ctx, self._last_room_text)
        
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
        
        # Check for higher priority behaviors that should interrupt
        for behavior in self._behaviors:
            if behavior.can_start(ctx):
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
        if self._current_behavior:
            return self._current_behavior.name
        return None


def create_default_engine(
    bot: 'Bot',
    flee_hp_percent: float = 20.0,
    rest_hp_percent: float = 50.0,
    rest_move_percent: float = 20.0,
    use_skills: Optional[list[str]] = None,
    target_mobs: Optional[list[str]] = None,
    include_explore: bool = True
) -> BehaviorEngine:
    """Create a behavior engine with default behaviors.
    
    Args:
        bot: The bot to control
        flee_hp_percent: HP percentage to flee at
        rest_hp_percent: HP percentage to rest at
        rest_move_percent: Move percentage to rest at
        use_skills: Skills to use in combat
        target_mobs: Specific mobs to target
        include_explore: If True, include ExploreBehavior (disable when using navigation)
    """
    engine = BehaviorEngine(bot)
    
    # Add behaviors in priority order (will be sorted anyway)
    engine.add_behavior(SurviveBehavior(flee_hp_percent))
    engine.add_behavior(CombatBehavior(use_skills))
    engine.add_behavior(LootBehavior())
    engine.add_behavior(HealBehavior(
        rest_hp_percent=rest_hp_percent,
        rest_move_percent=rest_move_percent
    ))
    engine.add_behavior(AttackBehavior(target_mobs))
    engine.add_behavior(RecallBehavior())
    
    # Only add explore if requested (disable when using navigation)
    if include_explore:
        engine.add_behavior(ExploreBehavior())
    
    engine.add_behavior(IdleBehavior())
    
    return engine
