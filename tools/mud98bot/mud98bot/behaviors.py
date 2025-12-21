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
        logger.debug(f"Starting behavior: {self.name}")
    
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
        logger.warning(f"HP critical ({ctx.hp_percent:.0f}%) - initiating flee!")
    
    def tick(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        if not ctx.in_combat:
            logger.info("Successfully escaped combat!")
            return BehaviorResult.COMPLETED
        
        if self._flee_attempts >= 5:
            logger.error("Could not flee after 5 attempts!")
            bot.send_command("recall")
            return BehaviorResult.FAILED
        
        self._flee_attempts += 1
        logger.warning(f"Fleeing (attempt {self._flee_attempts})...")
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
            logger.info("Combat ended - victory!")
            return BehaviorResult.COMPLETED
        
        # Use skills occasionally (30% chance)
        if self.use_skills and random.random() < 0.3:
            skill = random.choice(self.use_skills)
            bot.send_command(skill)
        
        # Auto-attack continues automatically, just wait
        return BehaviorResult.WAITING


class HealBehavior(Behavior):
    """Rest/sleep to recover HP when out of combat."""
    
    priority = 70
    name = "Heal"
    tick_delay = 2.0  # Regen ticks every ~15 seconds in ROM
    
    def __init__(self, rest_hp_percent: float = 50.0, 
                 sleep_hp_percent: float = 30.0,
                 target_hp_percent: float = 90.0):
        super().__init__()
        self.rest_hp_percent = rest_hp_percent
        self.sleep_hp_percent = sleep_hp_percent
        self.target_hp_percent = target_hp_percent
        self._is_resting = False
    
    def can_start(self, ctx: BehaviorContext) -> bool:
        return (not ctx.in_combat and 
                ctx.hp_percent < self.rest_hp_percent)
    
    def start(self, bot: 'Bot', ctx: BehaviorContext) -> None:
        super().start(bot, ctx)
        self._is_resting = False
        logger.info(f"Need to heal (HP: {ctx.hp_percent:.0f}%)")
    
    def tick(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        if ctx.in_combat:
            if self._is_resting:
                bot.send_command("wake")
            return BehaviorResult.FAILED
        
        if ctx.hp_percent >= self.target_hp_percent:
            if self._is_resting:
                bot.send_command("wake")
                self._is_resting = False
            logger.info(f"Healed to {ctx.hp_percent:.0f}%")
            return BehaviorResult.COMPLETED
        
        if not self._is_resting:
            if ctx.hp_percent < self.sleep_hp_percent:
                bot.send_command("sleep")
            else:
                bot.send_command("rest")
            self._is_resting = True
        
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
        logger.info("Looting corpse...")
    
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
                    logger.info(f"Attacking: {mob.name} lvl{mob.level} hp={mob.hp_percent}% (using '{keyword}')")
                    bot.send_command(f"kill {keyword}")
                    return BehaviorResult.COMPLETED
        else:
            # Fall back to text-based targeting
            for mob in ctx.room_mobs:
                if self._can_attack(mob):
                    keyword = self._get_target_keyword(mob)
                    logger.info(f"Attacking: {mob} (using '{keyword}')")
                    bot.send_command(f"kill {keyword}")
                    return BehaviorResult.COMPLETED
        
        return BehaviorResult.FAILED


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
            logger.info("Low movement, stopping exploration")
            return BehaviorResult.COMPLETED
        
        if len(self.visited_vnums) >= self.max_rooms:
            logger.info(f"Explored {self.max_rooms} rooms, done")
            return BehaviorResult.COMPLETED
        
        # Record current room
        if ctx.room_vnum:
            self.visited_vnums.add(ctx.room_vnum)
        
        # Choose next direction
        direction = self._choose_direction(ctx)
        if not direction:
            self._stuck_count += 1
            if self._stuck_count >= 3:
                logger.warning("Stuck, recalling to temple")
                bot.send_command("recall")
                self._stuck_count = 0
                return BehaviorResult.CONTINUE
            return BehaviorResult.WAITING
        
        self._stuck_count = 0
        self._last_direction = direction
        bot.send_command(direction)
        logger.debug(f"Moving {direction} (visited {len(self.visited_vnums)} rooms)")
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
                logger.debug(f"BOT: Choosing unexplored direction {direction}")
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


class TrainBehavior(Behavior):
    """Train stats when we have training sessions."""
    
    priority = 50
    name = "Train"
    tick_delay = 0.5
    
    # Priority order for stats to train
    DEFAULT_TRAIN_ORDER = ['str', 'con', 'dex', 'wis', 'int']
    
    # VNUM of training room (Furey's Training Room in Mud School)
    TRAIN_ROOM_VNUM = 3758
    
    def __init__(self, stat_priority: Optional[list[str]] = None):
        super().__init__()
        self.stat_priority = stat_priority or self.DEFAULT_TRAIN_ORDER
        self._navigating = False
        self._at_trainer = False
    
    def can_start(self, ctx: BehaviorContext) -> bool:
        # Would need training sessions - can't detect via MSDP currently
        # For now, always return False (training is done manually)
        return False
    
    def tick(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        # Future: navigate to trainer and train stats
        return BehaviorResult.FAILED


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
    """Recall to temple when movement is low or need to reset."""
    
    priority = 55
    name = "Recall"
    tick_delay = 1.0
    
    def __init__(self, low_move_percent: float = 15.0):
        super().__init__()
        self.low_move_percent = low_move_percent
    
    def can_start(self, ctx: BehaviorContext) -> bool:
        return (not ctx.in_combat and 
                ctx.move_percent < self.low_move_percent)
    
    def tick(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        logger.info("Low movement, recalling to temple")
        bot.send_command("recall")
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
        logger.debug(f"Added behavior: {behavior.name} (priority {behavior.priority})")
    
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
            room_mobs=[],
            room_items=[],
            has_corpse=False,
        )
        
        # Use BOT protocol data if available (more reliable)
        if self.bot.bot_mode and self.bot.bot_data:
            bot_data = self.bot.bot_data
            ctx.bot_mode = True
            ctx.bot_mobs = bot_data.mobs
            ctx.bot_objects = bot_data.objects
            ctx.bot_exits = bot_data.exits
            
            if bot_data.room:
                ctx.bot_room_flags = bot_data.room.flags
                ctx.bot_sector = bot_data.room.sector
                # Use room VNUM from bot data if available
                if bot_data.room.vnum > 0:
                    ctx.room_vnum = bot_data.room.vnum
            
            # Populate exits from bot data
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
                        logger.debug(f"Interrupting {self._current_behavior.name} "
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
    use_skills: Optional[list[str]] = None,
    target_mobs: Optional[list[str]] = None
) -> BehaviorEngine:
    """Create a behavior engine with default behaviors."""
    engine = BehaviorEngine(bot)
    
    # Add behaviors in priority order (will be sorted anyway)
    engine.add_behavior(SurviveBehavior(flee_hp_percent))
    engine.add_behavior(CombatBehavior(use_skills))
    engine.add_behavior(LootBehavior())
    engine.add_behavior(HealBehavior(rest_hp_percent))
    engine.add_behavior(AttackBehavior(target_mobs))
    engine.add_behavior(RecallBehavior())
    engine.add_behavior(ExploreBehavior())
    engine.add_behavior(IdleBehavior())
    
    return engine
