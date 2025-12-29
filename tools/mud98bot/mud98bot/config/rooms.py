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
"""General store - sells food (soup), drink (water skin), and lantern."""

INTERMEDIATE_ROOM: Final[int] = 3717
"""Room between central room and shop (down from 3712)."""


# =============================================================================
# MUD SCHOOL PROGRESSION
# =============================================================================

CORRIDOR_ROOM: Final[int] = 3719
"""Corridor room with locked door to east (needs key from creature)."""

DARK_CREATURE_ROOM: Final[int] = 3720
"""Dark room containing 'big creature' with key. Requires light source."""

MUD_SCHOOL_ENTRY: Final[int] = 3721
"""First room inside MUD School proper (east of corridor, through locked door)."""

# Route from shop (3718) to dark creature room (3720)
# Path: 3718 -> north -> 3717 -> east -> 3719 -> north -> 3720
ROUTE_SHOP_TO_CREATURE: Final[dict[int, str]] = {
    3718: 'north',   # Shop -> Intermediate
    3717: 'east',    # Intermediate -> Corridor
    3719: 'north',   # Corridor -> Dark creature room
}

# Route back from creature room to cage area
# Path: 3720 -> south -> 3719 -> west -> 3717 -> up -> 3712
ROUTE_CREATURE_TO_CAGE: Final[dict[int, str]] = {
    3720: 'south',   # Dark room -> Corridor
    3719: 'west',    # Corridor -> Intermediate
    3717: 'up',      # Intermediate -> Central cage room
}

# Route from Temple (3001) or death respawn to Corridor (3719)
# Used when bot dies after defeating creature but before reaching corridor
# Path: Temple -> up -> 3700 -> north -> 3757 -> north -> 3701 -> west -> 3702 -> 
#       north -> 3703 -> north -> 3709 -> west -> 3711 -> down -> 3712 -> down -> 3717 -> east -> 3719
ROUTE_TEMPLE_TO_CORRIDOR: Final[dict[int, str]] = {
    3054: 'south',   # Temple Altar (death respawn) -> Temple (3001)
    3001: 'up',      # Temple -> Mud School Entrance (3700)
    3700: 'north',   # Entrance -> Hub Room (3757)
    3757: 'north',   # Hub -> First Hallway (3701)
    3701: 'west',    # Turn west
    3702: 'north',   # Continue north
    3703: 'north',   # Continue north
    3709: 'west',    # Turn west
    3711: 'down',    # Go down to Cage Room
    3712: 'down',    # Cage Room -> Intermediate (3717)
    3717: 'east',    # Intermediate -> Corridor (must open door first!)
    # 3719 is the Corridor - target reached
}


# =============================================================================
# NAVIGATION ROUTES
# =============================================================================

# Route from Temple (3001) or death respawn to Cage Room (3712)
# Also includes routes from Training Room (3758) and Practice Room (3759)
ROUTE_TO_CAGE_ROOM: Final[dict[int, str]] = {
    3054: 'south',   # Temple Altar (death respawn) -> Temple (3001)
    3001: 'up',      # Temple -> Mud School Entrance (3700)
    3700: 'north',   # Entrance -> Hub Room (3757)
    3757: 'north',   # Hub -> First Hallway (3701)
    3758: 'east',    # Training Room -> Hub (3757)
    3759: 'west',    # Practice Room -> Hub (3757)
    3701: 'west',    # Turn west
    3702: 'north',   # Continue north
    3703: 'north',   # Continue north
    3709: 'west',    # Turn west
    3711: 'down',    # Go down
    # 3712 is the Cage Room - target reached
}

# Routes from central room (3712) to each cage
# Include return paths from other cages so bots can navigate between cages
ROUTE_TO_NORTH_CAGE: Final[dict[int, str]] = {
    3712: 'north',   # Cage room -> North cage (3713)
    3714: 'east',    # West cage -> Cage room
    3715: 'north',   # South cage -> Cage room
    3716: 'west',    # East cage -> Cage room
}

ROUTE_TO_SOUTH_CAGE: Final[dict[int, str]] = {
    3712: 'south',   # Cage room -> South cage (3715)
    3713: 'south',   # North cage -> Cage room
    3714: 'east',    # West cage -> Cage room
    3716: 'west',    # East cage -> Cage room
}

ROUTE_TO_EAST_CAGE: Final[dict[int, str]] = {
    3712: 'east',    # Cage room -> East cage (3716)
    3713: 'south',   # North cage -> Cage room
    3714: 'east',    # West cage -> Cage room
    3715: 'north',   # South cage -> Cage room
}

ROUTE_TO_WEST_CAGE: Final[dict[int, str]] = {
    3712: 'west',    # Cage room -> West cage (3714)
    3713: 'south',   # North cage -> Cage room
    3715: 'north',   # South cage -> Cage room
    3716: 'west',    # East cage -> Cage room
}

# Route to training room (3758)
ROUTE_TO_TRAIN_ROOM: Final[dict[int, str]] = {
    3054: 'south',   # Temple Altar (death respawn) -> Temple (3001)
    3001: 'up',      # Temple -> Mud School Entrance (3700)
    3700: 'north',   # Entrance -> Hub room (3757)
    3757: 'west',    # Hub -> Training Room (3758)
    # From practice room, go back to hub first
    3759: 'west',    # Practice Room -> Hub (3757)
}

# Route to practice room (3759)
ROUTE_TO_PRACTICE_ROOM: Final[dict[int, str]] = {
    3054: 'south',   # Temple Altar (death respawn) -> Temple (3001)
    3001: 'up',      # Temple -> Mud School Entrance (3700)
    3700: 'north',   # Entrance -> Hub room (3757)
    3757: 'east',    # Hub -> Practice Room (3759)
    # From training room, go back to hub first
    3758: 'east',    # Training Room -> Hub (3757)
}


# =============================================================================
# PATROL CONFIGURATION
# =============================================================================

PATROL_SEQUENCE: Final[list[int]] = [
    EAST_CAGE_ROOM,   # 3715 - south cage (wimpy monster) - easiest
    SOUTH_CAGE_ROOM,  # 3714 - west cage (wimpy aggressive monster)
    WEST_CAGE_ROOM,   # 3716 - east cage (normal monster)
    NORTH_CAGE_ROOM,  # 3713 - north cage (aggressive monster) - hardest
]
"""Order of cage rooms to visit during patrol (easiest to hardest)."""

CAGE_EXIT_DIRECTIONS: Final[dict[int, str]] = {
    NORTH_CAGE_ROOM: 'south',  # Exit north cage
    SOUTH_CAGE_ROOM: 'east',   # Exit "south" cage (3714)
    EAST_CAGE_ROOM: 'north',   # Exit "east" cage (3715)
    WEST_CAGE_ROOM: 'west',    # Exit "west" cage (3716)
}
"""Direction to exit each cage room back to central room."""
