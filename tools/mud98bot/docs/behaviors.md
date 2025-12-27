# Mud98Bot Behavior System

This document describes the behavior system architecture, priorities, and how to create custom behaviors.

## Overview

The behavior system uses a **priority-based selection** model. Each tick, the `BehaviorEngine`:

1. Builds a `BehaviorContext` from current game state
2. Iterates behaviors by priority (highest first)
3. Finds the first behavior where `can_start(ctx)` returns `True`
4. If it's higher priority than the current behavior, switches to it
5. Calls `tick(bot, ctx)` on the active behavior

This allows emergency behaviors (flee, death recovery) to preempt normal activities.

## Priority Hierarchy

| Priority | Behavior | Description |
|----------|----------|-------------|
| 200 | DeathRecoveryBehavior | Handle death/ghost state |
| 100 | SurviveBehavior | Emergency flee when HP critical |
| 95 | RecallBehavior | Emergency recall when flee fails |
| 80 | CombatBehavior | Continue ongoing fight |
| 75 | LootBehavior | Loot corpses after combat |
| 70 | HealBehavior | Rest to recover HP/mana/moves |
| 65 | BotResetBehavior | Initial setup (train/practice) |
| 62 | TrainBehavior | Navigate and train stats |
| 61 | PracticeBehavior | Navigate and practice skills |
| 60 | AttackBehavior | Initiate combat with targets |
| 56 | BuySuppliesBehavior | Shop for food/water |
| 55 | PatrolCagesBehavior | Patrol cage rooms in circuit |
| 45 | NavigateBehavior | Goal-oriented navigation |
| 40 | ExploreBehavior | Random exploration |
| 35 | ReturnToCageBehavior | Return home after displacement |
| 10 | IdleBehavior | Fallback random actions |

## Module Organization

```
behaviors/
├── base.py          # BehaviorContext, BehaviorResult, Behavior ABC
├── engine.py        # BehaviorEngine
├── combat.py        # AttackBehavior, CombatBehavior
├── survival.py      # DeathRecovery, Survive, Heal, Recall
├── inventory.py     # Loot, Shop, BuySupplies
├── navigation.py    # Navigate, Explore, Patrol, ReturnToCage
├── training.py      # Train, Practice, BotReset
└── idle.py          # IdleBehavior
```

## Key Classes

### BehaviorContext

Snapshot of game state passed to behaviors:

```python
@dataclass
class BehaviorContext:
    # Vital stats
    health: int
    health_max: int
    mana: int
    mana_max: int
    movement: int
    movement_max: int
    
    # Character info
    level: int
    experience: int
    money: int
    
    # Combat state
    in_combat: bool
    opponent_name: str
    opponent_health: int
    opponent_level: int
    
    # Location
    room_vnum: int
    room_exits: list[str]
    position: PositionState  # standing, sitting, sleeping, etc.
    
    # Room contents
    room_mobs: list[str]      # Mob names/descriptions
    has_corpse: bool          # Lootable corpse present
    
    # BOT protocol data (when available)
    bot_mode: bool
    bot_mobs: list[BotMob]    # Structured mob data
    bot_objects: list[BotObject]
    
    # Status
    is_hungry: bool
    is_thirsty: bool
    should_proactive_shop: bool  # Trigger for proactive shopping
    
    # Computed properties
    hp_percent: float
    mana_percent: float
    move_percent: float
```

### BehaviorResult

Return value from `tick()`:

```python
class BehaviorResult(Enum):
    CONTINUE = auto()   # Behavior ongoing, will continue
    COMPLETED = auto()  # Behavior finished successfully
    FAILED = auto()     # Behavior failed, stop
    WAITING = auto()    # Behavior waiting for external event
```

### Behavior Base Class

```python
class Behavior(ABC):
    priority: int = 0      # Higher = more important
    name: str = "Unknown"  # For logging
    tick_delay: float = 1.0  # Seconds between ticks
    
    @abstractmethod
    def can_start(self, ctx: BehaviorContext) -> bool:
        """Check if behavior should activate."""
        pass
    
    @abstractmethod
    def tick(self, bot: Bot, ctx: BehaviorContext) -> BehaviorResult:
        """Execute one tick of behavior."""
        pass
    
    def start(self, bot: Bot, ctx: BehaviorContext) -> None:
        """Called when behavior becomes active."""
        pass
    
    def stop(self) -> None:
        """Called when behavior loses control."""
        pass
```

## Behavior Categories

### Survival Behaviors

**DeathRecoveryBehavior** (priority 200)
- Activates when position is 'dead' or 'ghost'
- Attempts recovery via wake/stand/recall/pray
- Highest priority - nothing else runs while dead

**SurviveBehavior** (priority 100)
- Activates when HP < flee threshold (default 20%) and in combat
- Rapidly sends 'flee' commands
- Very fast tick (0.3s) for emergency response

**RecallBehavior** (priority 95)
- Activates when flee has failed and HP is critical
- Last resort escape mechanism
- Uses 'recall' command to return to temple

**HealBehavior** (priority 70)
- Activates when HP/mana/moves below thresholds
- Sends 'rest' or 'sleep' commands
- Waits until fully recovered before completing

### Combat Behaviors

**CombatBehavior** (priority 80)
- Activates when `ctx.in_combat` is True
- Maintains ongoing fight, optionally uses skills
- Fast tick (0.5s) for responsiveness

**AttackBehavior** (priority 60)
- Activates when not in combat and healthy
- Finds appropriate target from room mobs
- Respects level difference limits
- Supports target whitelist

### Inventory Behaviors

**LootBehavior** (priority 75)
- Activates when `ctx.has_corpse` is True
- Sends 'get all corpse', auto-equips items
- Parses loot output for equipment decisions

**BuySuppliesBehavior** (priority 56)
- Activates when hungry/thirsty OR proactive shop flag set
- Navigates to shop (3718)
- Buys soup, water skin, and lantern (for dark rooms)
- Returns to cage area

### Navigation Behaviors

**PatrolCagesBehavior** (priority 55)
- Visits cage rooms in sequence: North → East → South → West
- Waits for combat to finish in each cage
- Triggers proactive shopping after completing circuit

**NavigateBehavior** (priority 45)
- General-purpose navigation using route dictionaries
- Route format: `{room_vnum: direction, ...}`
- Handles stuck detection and recall fallback

**ReturnToCageBehavior** (priority 35)
- Activates when outside patrol area
- Uses ROUTE_TO_CAGE_ROOM to return home
- Useful after death/recall displacement

**ExploreBehavior** (priority 40)
- Random movement through available exits
- Low priority fallback exploration

### Training Behaviors

**BotResetBehavior** (priority 65)
- One-shot initial setup at bot startup
- Phases: Train → Practice → Navigate to cage
- Clears hunger/thirst state for clean start

**TrainBehavior** (priority 62)
- Navigates to train room (3758)
- Sends training commands for stats

**PracticeBehavior** (priority 61)
- Navigates to practice room (3759)
- Optionally sends 'practice reset' first
- Practices configured skill list

### Idle Behavior

**IdleBehavior** (priority 10)
- Always returns True from `can_start()`
- Performs occasional random actions (look, score, inventory)
- Prevents bot from appearing frozen

## Creating Custom Behaviors

### Basic Template

```python
from mud98bot.behaviors import Behavior, BehaviorContext, BehaviorResult
from mud98bot.config import PRIORITY_ATTACK  # or define your own

class MyBehavior(Behavior):
    priority = 50  # Adjust as needed
    name = "MyBehavior"
    tick_delay = 1.0
    
    def __init__(self, some_param: str = "default"):
        super().__init__()
        self.some_param = some_param
        self._internal_state = None
    
    def can_start(self, ctx: BehaviorContext) -> bool:
        # Return True if this behavior should activate
        if ctx.in_combat:
            return False  # Don't interrupt combat
        return some_condition(ctx)
    
    def tick(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        # Execute one tick of behavior
        if goal_achieved():
            return BehaviorResult.COMPLETED
        
        bot.send_command("do something")
        return BehaviorResult.CONTINUE
```

### State Machine Pattern

For multi-step behaviors:

```python
class MultiStepBehavior(Behavior):
    STATE_GOTO = 'goto'
    STATE_ACTION = 'action'
    STATE_RETURN = 'return'
    
    def __init__(self):
        super().__init__()
        self._state = self.STATE_GOTO
    
    def tick(self, bot, ctx):
        if self._state == self.STATE_GOTO:
            if ctx.room_vnum == TARGET_ROOM:
                self._state = self.STATE_ACTION
                return BehaviorResult.CONTINUE
            # Navigate...
            
        elif self._state == self.STATE_ACTION:
            bot.send_command("do action")
            self._state = self.STATE_RETURN
            
        elif self._state == self.STATE_RETURN:
            if ctx.room_vnum == HOME_ROOM:
                return BehaviorResult.COMPLETED
            # Navigate home...
        
        return BehaviorResult.CONTINUE
```

### Registering Behaviors

```python
from mud98bot.behaviors import BehaviorEngine, create_default_engine

# Option 1: Add to default engine
engine = create_default_engine(bot)
engine.add_behavior(MyBehavior())

# Option 2: Build custom engine
engine = BehaviorEngine(bot)
engine.add_behavior(MyBehavior())
engine.add_behavior(SurviveBehavior())
# ... add others as needed
```

## Common Patterns

### Yielding to Higher Priority

```python
def tick(self, bot, ctx):
    if ctx.in_combat:
        return BehaviorResult.WAITING  # Let CombatBehavior handle
    # ... normal logic
```

### Inter-Behavior Communication

```python
# Set flag for another behavior to pick up
if hasattr(bot, '_behavior_engine') and bot._behavior_engine:
    bot._behavior_engine._should_proactive_shop = True
```

### Checking BOT Protocol Data

```python
def tick(self, bot, ctx):
    if ctx.bot_mode and ctx.bot_mobs:
        # Prefer structured BOT data
        for mob in ctx.bot_mobs:
            if mob.level < ctx.level + 5:
                bot.send_command(f"kill {mob.name.split()[0]}")
    else:
        # Fall back to text parsing
        for mob_desc in ctx.room_mobs:
            # ...
```

## Best Practices

1. **Set appropriate priority**: High for emergencies, low for optional actions
2. **Use `can_start()` wisely**: Cheap checks only - called every tick
3. **Return early from `tick()`**: If interrupted (combat, movement blocked)
4. **Clean up in `stop()`**: Reset internal state for reuse
5. **Log important events**: Use `logger.info()` for state changes
6. **Import constants from config**: Don't hardcode VNUMs or thresholds
7. **Handle edge cases**: Unknown rooms, missing data, combat interruptions
