"""
Mud98Bot Behaviors Package.

This package contains the behavior system for automated bot control.
Behaviors are prioritized tasks that the bot can perform, with the
highest priority behavior that can run being selected each tick.

Quick Reference:
    Survival (highest priority):
        - DeathRecoveryBehavior (200) - Handle death/ghost state
        - SurviveBehavior (100) - Emergency flee
        - RecallBehavior (95) - Emergency recall when flee fails
        - LightSourceBehavior (85) - Handle light in dark rooms
    
    Combat:
        - CombatBehavior (80) - Continue ongoing fight
        - AttackBehavior (60) - Start fights with targets
    
    Recovery:
        - LootBehavior (75) - Loot corpses
        - HealBehavior (70) - Rest to recover HP/mana/moves
    
    Development:
        - BotResetBehavior (65) - Initial setup
        - TrainBehavior (62) - Train stats
        - PracticeBehavior (61) - Practice skills
    
    Logistics:
        - FightDarkCreatureBehavior (57) - Fight creature in dark room after shopping
        - BuySuppliesBehavior (56) - Shop for food/water
        - PatrolCagesBehavior (55) - Patrol cage circuit
    
    Navigation:
        - NavigateBehavior (45) - Goal-oriented navigation
        - ExploreBehavior (40) - Random exploration
        - ReturnToCageBehavior (35) - Return after displacement
    
    Fallback:
        - IdleBehavior (10) - Random actions when idle

Usage:
    from mud98bot.behaviors import (
        BehaviorEngine,
        create_default_engine,
        PatrolCagesBehavior,
        BuySuppliesBehavior,
    )
    
    # Create engine with default behaviors
    engine = create_default_engine(bot)
    
    # Add custom behaviors
    engine.add_behavior(PatrolCagesBehavior())
    engine.add_behavior(BuySuppliesBehavior())
"""

from .base import (
    Behavior,
    BehaviorContext,
    BehaviorResult,
)

from .engine import BehaviorEngine

from .combat import (
    AttackBehavior,
    CombatBehavior,
)

from .survival import (
    DeathRecoveryBehavior,
    HealBehavior,
    LightSourceBehavior,
    RecallBehavior,
    SurviveBehavior,
)

from .inventory import (
    BuySuppliesBehavior,
    LootBehavior,
    ShopBehavior,
)

from .navigation import (
    ExploreBehavior,
    FightDarkCreatureBehavior,
    NavigateBehavior,
    NavigationTask,
    PatrolCagesBehavior,
    ReturnToCageBehavior,
)

from .training import (
    BotResetBehavior,
    PracticeBehavior,
    TrainBehavior,
)

from .idle import IdleBehavior

# Re-export route constants for convenience
from ..config import (
    ROUTE_TO_CAGE_ROOM,
    ROUTE_TO_NORTH_CAGE,
    ROUTE_TO_SOUTH_CAGE,
    ROUTE_TO_EAST_CAGE,
    ROUTE_TO_WEST_CAGE,
    ROUTE_TO_TRAIN_ROOM,
    ROUTE_TO_PRACTICE_ROOM,
    TRAIN_ROOM,
    PRACTICE_ROOM,
)

# Legacy aliases for backward compatibility
TRAIN_ROOM_VNUM = TRAIN_ROOM
PRACTICE_ROOM_VNUM = PRACTICE_ROOM


def create_default_engine(
    bot: 'Bot',
    flee_hp_percent: float = 20.0,
    rest_hp_percent: float = 50.0,
    rest_move_percent: float = 20.0,
    use_skills: list[str] | None = None,
    target_mobs: list[str] | None = None,
    include_explore: bool = True
) -> BehaviorEngine:
    """
    Create a behavior engine with default behaviors.
    
    This sets up a standard behavior configuration suitable for
    general combat leveling.
    
    Args:
        bot: The bot to control.
        flee_hp_percent: HP percentage to flee at (default 20%).
        rest_hp_percent: HP percentage to rest at (default 50%).
        rest_move_percent: Move percentage to rest at (default 20%).
        use_skills: Skills to use in combat.
        target_mobs: Specific mobs to target.
        include_explore: If True, include ExploreBehavior.
    
    Returns:
        Configured BehaviorEngine.
    """
    engine = BehaviorEngine(bot)
    
    # Add behaviors in priority order (will be sorted anyway)
    engine.add_behavior(DeathRecoveryBehavior())
    engine.add_behavior(SurviveBehavior(flee_hp_percent))
    engine.add_behavior(CombatBehavior(use_skills))
    engine.add_behavior(LootBehavior())
    engine.add_behavior(HealBehavior(
        rest_hp_percent=rest_hp_percent,
        rest_move_percent=rest_move_percent,
    ))
    engine.add_behavior(AttackBehavior(target_mobs))
    engine.add_behavior(RecallBehavior())
    
    if include_explore:
        engine.add_behavior(ExploreBehavior())
    
    engine.add_behavior(IdleBehavior())
    
    return engine


__all__ = [
    # Base
    'Behavior',
    'BehaviorContext',
    'BehaviorResult',
    
    # Engine
    'BehaviorEngine',
    'create_default_engine',
    
    # Combat
    'AttackBehavior',
    'CombatBehavior',
    
    # Survival
    'DeathRecoveryBehavior',
    'HealBehavior',
    'LightSourceBehavior',
    'RecallBehavior',
    'SurviveBehavior',
    
    # Inventory
    'BuySuppliesBehavior',
    'LootBehavior',
    'ShopBehavior',
    
    # Navigation
    'ExploreBehavior',
    'FightDarkCreatureBehavior',
    'NavigateBehavior',
    'NavigationTask',
    'PatrolCagesBehavior',
    'ReturnToCageBehavior',
    
    # Training
    'BotResetBehavior',
    'PracticeBehavior',
    'TrainBehavior',
    
    # Idle
    'IdleBehavior',
    
    # Route constants
    'ROUTE_TO_CAGE_ROOM',
    'ROUTE_TO_NORTH_CAGE',
    'ROUTE_TO_SOUTH_CAGE',
    'ROUTE_TO_EAST_CAGE',
    'ROUTE_TO_WEST_CAGE',
    'ROUTE_TO_TRAIN_ROOM',
    'ROUTE_TO_PRACTICE_ROOM',
    'TRAIN_ROOM',
    'PRACTICE_ROOM',
    'TRAIN_ROOM_VNUM',  # Legacy alias
    'PRACTICE_ROOM_VNUM',  # Legacy alias
]
