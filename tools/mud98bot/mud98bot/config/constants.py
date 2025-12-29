"""
Behavior Constants and Thresholds.

This module defines priorities, timing, and gameplay thresholds used by
the behavior system.
"""

from typing import Final

# =============================================================================
# BEHAVIOR PRIORITIES
# =============================================================================
# Higher priority = runs first. Priorities determine which behavior takes
# control when multiple behaviors want to run.

PRIORITY_DEATH_RECOVERY: Final[int] = 200
"""Highest priority - handle death/ghost state."""

PRIORITY_SURVIVE: Final[int] = 100
"""Emergency flee when HP is critical."""

PRIORITY_RECALL: Final[int] = 95
"""Emergency recall when flee fails."""

PRIORITY_LIGHT_SOURCE: Final[int] = 85
"""Handle light sources when in dark rooms."""

PRIORITY_COMBAT: Final[int] = 80
"""Continue an ongoing fight."""

PRIORITY_LOOT: Final[int] = 75
"""Loot corpses after combat."""

PRIORITY_HEAL: Final[int] = 70
"""Rest to recover HP/mana/moves."""

PRIORITY_BOT_RESET: Final[int] = 65
"""Initial bot setup (train/practice at startup)."""

PRIORITY_TRAIN: Final[int] = 62
"""Navigate to trainer and spend training sessions."""

PRIORITY_PRACTICE: Final[int] = 61
"""Navigate to practitioner and practice skills."""

PRIORITY_INITIAL_NAVIGATE: Final[int] = 60
"""Initial one-shot navigation to combat area (same priority as attack)."""

PRIORITY_ATTACK: Final[int] = 60
"""Initiate combat with appropriate targets."""

PRIORITY_DARK_CREATURE: Final[int] = 57
"""Navigate to dark room and fight creature after getting lantern."""

PRIORITY_BUY_SUPPLIES: Final[int] = 56
"""Go to shop when hungry/thirsty or proactively after patrol."""

PRIORITY_PATROL: Final[int] = 55
"""Patrol cage rooms in a circuit."""

PRIORITY_SHOP: Final[int] = 45
"""Legacy shop behavior (deprecated)."""

PRIORITY_NAVIGATE: Final[int] = 45
"""Goal-oriented navigation to a destination."""

PRIORITY_EXPLORE: Final[int] = 40
"""Random exploration when nothing else to do."""

PRIORITY_RETURN_TO_CAGE: Final[int] = 35
"""Return to cage area after being displaced."""

PRIORITY_IDLE: Final[int] = 10
"""Fallback - do random actions when truly idle."""


# =============================================================================
# HP / MANA / MOVEMENT THRESHOLDS
# =============================================================================

DEFAULT_FLEE_HP_PERCENT: Final[float] = 20.0
"""Default HP% to trigger flee behavior."""

DEFAULT_REST_HP_PERCENT: Final[float] = 50.0
"""Default HP% below which to rest."""

DEFAULT_REST_MANA_PERCENT: Final[float] = 30.0
"""Default Mana% below which to rest (for casters)."""

DEFAULT_REST_MOVE_PERCENT: Final[float] = 20.0
"""Default Movement% below which to rest."""

CRITICAL_HP_PERCENT: Final[float] = 30.0
"""HP% considered critical - triggers emergency behaviors."""

CRITICAL_MOVE_PERCENT: Final[float] = 15.0
"""Movement% considered critical - yields to healing."""


# =============================================================================
# COMBAT THRESHOLDS
# =============================================================================

MAX_LEVEL_DIFFERENCE: Final[int] = 5
"""Maximum level difference to engage a mob (avoid suicide)."""

MIN_ATTACK_HP_PERCENT: Final[float] = 50.0
"""Minimum HP% required to start a new fight."""


# =============================================================================
# TIMING / TICK CONFIGURATION
# =============================================================================

DEFAULT_TICK_DELAY: Final[float] = 1.0
"""Default seconds between behavior ticks."""

PATROL_LINGER_TICKS: Final[int] = 2
"""Ticks to wait in a cage room after combat ends."""

MAX_FLEE_ATTEMPTS: Final[int] = 5
"""Maximum flee attempts before giving up."""

MAX_WANDER_ATTEMPTS: Final[int] = 10
"""Maximum random wander attempts in explore behavior."""


# =============================================================================
# SHOPPING
# =============================================================================

MIN_SHOPPING_MONEY: Final[int] = 10
"""Minimum money required to attempt a shopping trip."""


# =============================================================================
# TEXT BUFFER
# =============================================================================

MAX_TEXT_BUFFER_LINES: Final[int] = 50
"""Maximum lines to keep in the text buffer for parsing."""


# =============================================================================
# PRACTICE DEFAULTS
# =============================================================================

DEFAULT_PRACTICE_PER_SKILL: Final[int] = 1
"""Default number of times to practice each skill (when using uniform count)."""

# Warriors start with 5 practice points. Optimal allocation:
# - sword x3 = 75% (max reachable via practice) for primary damage
# - shield block x2 = ~50% for defense with shield
DEFAULT_PRACTICE_SKILLS: Final[tuple[tuple[str, int], ...]] = (
    ('sword', 3),          # 3 practices -> 75% (max from practice command)
    ('shield block', 2),   # 2 practices -> ~50% for defense
)
"""Default skills to practice with counts (skill, times) for focused warrior build."""
