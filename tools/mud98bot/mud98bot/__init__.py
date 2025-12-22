"""Mud98Bot - Stress testing and automation bot for Mud98 MUD server."""

__version__ = "0.1.0"

from .client import Bot, BotConfig, BotState
from .connection import Connection, ConnectionConfig
from .telnet import TelnetHandler
from .msdp import MSDPParser, CharacterStats, RoomInfo, Position
from .parser import TextParser, BotRoomData
from .behaviors import (
    BehaviorEngine, Behavior, BehaviorResult, create_default_engine,
    NavigateBehavior, NavigationTask,
    ROUTE_TO_CAGE_ROOM, ROUTE_TO_NORTH_CAGE, ROUTE_TO_SOUTH_CAGE,
    ROUTE_TO_EAST_CAGE, ROUTE_TO_WEST_CAGE
)
from .metrics import MetricsCollector, BotMetrics, get_collector
from .coordinator import (
    Coordinator, CoordinatorConfig, run_stress_test,
    BotAccount, load_accounts, PATH_TO_CAGE_ROOM, CAGE_DIRECTIONS,
    CAGE_VNUMS, CAGE_ROUTES
)

__all__ = [
    # Client
    "Bot", "BotConfig", "BotState",
    # Connection
    "Connection", "ConnectionConfig",
    # Protocol
    "TelnetHandler", "MSDPParser", "CharacterStats", "RoomInfo", "Position",
    # Parsing
    "TextParser", "BotRoomData",
    # Behaviors
    "BehaviorEngine", "Behavior", "BehaviorResult", "create_default_engine",
    "NavigateBehavior", "NavigationTask",
    "ROUTE_TO_CAGE_ROOM", "ROUTE_TO_NORTH_CAGE", "ROUTE_TO_SOUTH_CAGE",
    "ROUTE_TO_EAST_CAGE", "ROUTE_TO_WEST_CAGE",
    # Metrics
    "MetricsCollector", "BotMetrics", "get_collector",
    # Coordinator
    "Coordinator", "CoordinatorConfig", "run_stress_test",
    "BotAccount", "load_accounts", "PATH_TO_CAGE_ROOM", "CAGE_DIRECTIONS",
    "CAGE_VNUMS", "CAGE_ROUTES",
]
