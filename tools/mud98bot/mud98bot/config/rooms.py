"""
Room VNUMs and Navigation Routes.

This module defines all room virtual numbers (VNUMs) and the static routes
used for navigation between key locations in the game world.

The bot operates primarily in the Mob Factory area (VNUMs 3700-3799) which
contains:
- Central room (3712): Hub connecting to cages and down to shops
- Four cage rooms (3713-3716): Contain leashed monsters for training
- Train room (3758): For stat training
- Practice room (3759): For skill practice
- Shop (3718): Sells food and water

Routes are dictionaries mapping room VNUM -> direction to move.
"""

from typing import Final

# =============================================================================
# TEMPLE / RECALL POINT
# =============================================================================

TEMPLE_ROOM: Final[int] = 3001
"""The Temple of Midgaard - default recall point."""

RECALL_ROOM: Final[int] = TEMPLE_ROOM
"""Alias for the recall destination."""


# =============================================================================
# MOB FACTORY / CAGE AREA
# =============================================================================

CENTRAL_ROOM: Final[int] = 3712
"""Central hub of the Mob Factory, connects to all cage rooms."""

NORTH_CAGE_ROOM: Final[int] = 3713
"""Northern cage room - contains leashed monster."""

SOUTH_CAGE_ROOM: Final[int] = 3714
"""Southern cage room - contains leashed monster (entered from west)."""

EAST_CAGE_ROOM: Final[int] = 3715
"""Eastern cage room - contains leashed monster (entered from south)."""

WEST_CAGE_ROOM: Final[int] = 3716
"""Western cage room - contains leashed monster (entered from east)."""

CAGE_ROOMS: Final[frozenset[int]] = frozenset({
    NORTH_CAGE_ROOM, SOUTH_CAGE_ROOM, EAST_CAGE_ROOM, WEST_CAGE_ROOM
})
"""Set of all cage room VNUMs."""

PATROL_ROOMS: Final[frozenset[int]] = frozenset({
    CENTRAL_ROOM, NORTH_CAGE_ROOM, SOUTH_CAGE_ROOM, EAST_CAGE_ROOM, WEST_CAGE_ROOM
})
"""All rooms that are valid patrol starting/continuation points."""


# =============================================================================
# TRAINING ROOMS
# =============================================================================

TRAIN_ROOM: Final[int] = 3758
"""Room containing the trainer mob for stat training."""

PRACTICE_ROOM: Final[int] = 3759
"""Room containing the practice mob for skill practice."""


# =============================================================================
# SHOPPING AREA
# =============================================================================

SHOP_ROOM: Final[int] = 3718
"""General store - sells food (soup) and drink (water skin)."""

INTERMEDIATE_ROOM: Final[int] = 3717
"""Room between central room and shop (down from 3712)."""


# =============================================================================
# NAVIGATION ROUTES
# =============================================================================

# Route from Temple (3001) to Mob Factory central room (3712)
ROUTE_TO_CAGE_ROOM: Final[dict[int, str]] = {
    3001: 'south',  # Temple -> Temple Square
    3005: 'south',  # Temple Square -> Market Square
    3014: 'west',   # Market Square -> West of Market
    3049: 'south',  # West of Market -> continues south...
    3050: 'south',
    3051: 'south',
    3052: 'south',
    3053: 'south',
    3054: 'west',   # Turn west toward Mob Factory
    3738: 'west',
    3737: 'west',
    3736: 'west',
    3735: 'west',
    3734: 'north',  # Turn north
    3733: 'north',
    3732: 'north',
    3731: 'north',
    3730: 'north',
    3729: 'north',
    3728: 'east',   # Enter Mob Factory
    3761: 'east',
    3760: 'east',
    3759: 'east',
    3758: 'east',
    3721: 'east',
    3720: 'east',
    3719: 'up',     # Go up to central
    3717: 'up',
    # 3712 is the destination
}

# Routes from central room (3712) to each cage
ROUTE_TO_NORTH_CAGE: Final[dict[int, str]] = {
    3712: 'north',  # Central -> North cage
}

ROUTE_TO_SOUTH_CAGE: Final[dict[int, str]] = {
    3712: 'west',   # Central -> South cage (confusing naming, but west leads to 3714)
}

ROUTE_TO_EAST_CAGE: Final[dict[int, str]] = {
    3712: 'south',  # Central -> East cage (south leads to 3715)
}

ROUTE_TO_WEST_CAGE: Final[dict[int, str]] = {
    3712: 'east',   # Central -> West cage (east leads to 3716)
}

# Route to training room
ROUTE_TO_TRAIN_ROOM: Final[dict[int, str]] = {
    **ROUTE_TO_CAGE_ROOM,
    3712: 'down',   # From central to below
    3717: 'down',
    3719: 'west',
    3720: 'west',
    3721: 'west',
    # 3758 is the destination
}

# Route to practice room  
ROUTE_TO_PRACTICE_ROOM: Final[dict[int, str]] = {
    **ROUTE_TO_TRAIN_ROOM,
    3758: 'west',   # Train room -> Practice room
    # 3759 is the destination
}


# =============================================================================
# PATROL CONFIGURATION
# =============================================================================

PATROL_SEQUENCE: Final[list[int]] = [
    NORTH_CAGE_ROOM,  # 3713
    EAST_CAGE_ROOM,   # 3715 (actually south of central)
    SOUTH_CAGE_ROOM,  # 3714 (actually west of central)
    WEST_CAGE_ROOM,   # 3716 (actually east of central)
]
"""Order of cage rooms to visit during patrol."""

CAGE_EXIT_DIRECTIONS: Final[dict[int, str]] = {
    NORTH_CAGE_ROOM: 'south',  # Exit north cage
    SOUTH_CAGE_ROOM: 'east',   # Exit "south" cage (3714)
    EAST_CAGE_ROOM: 'north',   # Exit "east" cage (3715)
    WEST_CAGE_ROOM: 'west',    # Exit "west" cage (3716)
}
"""Direction to exit each cage room back to central room."""
