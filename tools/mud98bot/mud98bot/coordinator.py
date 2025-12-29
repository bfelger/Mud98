"""
Multi-bot coordinator for Mud98Bot stress testing.

Manages spawning, monitoring, and coordinating multiple bot instances.
"""

import logging
import time
import threading
import signal
from pathlib import Path
from typing import Optional, Callable
from dataclasses import dataclass, field
from concurrent.futures import ThreadPoolExecutor, Future
from enum import Enum, auto

from .client import Bot, BotConfig
from .behaviors import (
    BehaviorEngine, create_default_engine,
    NavigateBehavior, ROUTE_TO_CAGE_ROOM,
    ROUTE_TO_NORTH_CAGE, ROUTE_TO_SOUTH_CAGE,
    ROUTE_TO_EAST_CAGE, ROUTE_TO_WEST_CAGE,
    BotResetBehavior, TrainBehavior, PracticeBehavior,
    ROUTE_TO_TRAIN_ROOM, ROUTE_TO_PRACTICE_ROOM,
    ReturnToCageBehavior, PatrolCagesBehavior,
    BuySuppliesBehavior, FightDarkCreatureBehavior,
    LightSourceBehavior
)
from .config.constants import PRIORITY_INITIAL_NAVIGATE
from .metrics import MetricsCollector, BotMetrics, get_collector, reset_collector

logger = logging.getLogger(__name__)


@dataclass
class BotAccount:
    """A bot account with username and password."""
    username: str
    password: str


def load_accounts(filepath: str) -> list[BotAccount]:
    """
    Load bot accounts from a text file.
    
    Format: username:password (one per line)
    Lines starting with # are comments.
    """
    accounts = []
    path = Path(filepath)
    
    if not path.exists():
        raise FileNotFoundError(f"Account file not found: {filepath}")
    
    with open(path, 'r') as f:
        for line_num, line in enumerate(f, 1):
            line = line.strip()
            
            # Skip empty lines and comments
            if not line or line.startswith('#'):
                continue
            
            # Parse username:password
            if ':' not in line:
                logger.warning(f"Invalid account format at line {line_num}: {line}")
                continue
            
            parts = line.split(':', 1)
            username = parts[0].strip()
            password = parts[1].strip()
            
            if not username or not password:
                logger.warning(f"Empty username or password at line {line_num}")
                continue
            
            accounts.append(BotAccount(username=username, password=password))
    
    logger.info(f"Loaded {len(accounts)} accounts from {filepath}")
    return accounts


class BotStatus(Enum):
    """Status of a managed bot instance."""
    PENDING = auto()
    CONNECTING = auto()
    LOGGED_IN = auto()
    PLAYING = auto()
    DISCONNECTED = auto()
    ERROR = auto()


@dataclass
class ManagedBot:
    """A bot instance managed by the coordinator."""
    bot_id: str
    config: BotConfig
    bot: Optional[Bot] = None
    engine: Optional[BehaviorEngine] = None
    metrics: Optional[BotMetrics] = None
    status: BotStatus = BotStatus.PENDING
    thread: Optional[threading.Thread] = None
    error: Optional[str] = None
    _stop_flag: bool = False
    
    def should_stop(self) -> bool:
        return self._stop_flag
    
    def request_stop(self) -> None:
        self._stop_flag = True


# Legacy path constants (kept for backward compatibility)
PATH_TO_CAGE_ROOM = ['up', 'north', 'north', 'west', 'north', 'north', 'west', 'down']
CAGE_DIRECTIONS = ['north', 'south', 'east', 'west']

# Cage room VNUMs for route-based navigation
CAGE_ROOM_VNUM = 3712
# Actual cage VNUMs based on school.are exits:
# - North (D0): 3713 - aggressive monster
# - East (D1): 3716 - unmodified monster
# - South (D2): 3715 - wimpy monster
# - West (D3): 3714 - aggressive wimpy monster
CAGE_VNUMS = {
    'north': 3713,  # North cage (aggressive monster)
    'south': 3715,  # South cage (wimpy monster)
    'east': 3716,   # East cage (unmodified monster)
    'west': 3714,   # West cage (aggressive wimpy monster)
}

# Route maps for each cage (from cage room or other cages)
# Each route includes paths back from other cages through 3712
CAGE_ROUTES = {
    'north': {
        3712: 'north',   # Cage room -> North cage
        3714: 'east',    # West cage -> Cage room (then next tick: north)
        3715: 'north',   # South cage -> Cage room
        3716: 'west',    # East cage -> Cage room
    },
    'south': {
        3712: 'south',   # Cage room -> South cage
        3713: 'south',   # North cage -> Cage room (then next tick: south)
        3714: 'east',    # West cage -> Cage room
        3716: 'west',    # East cage -> Cage room
    },
    'east': {
        3712: 'east',    # Cage room -> East cage
        3713: 'south',   # North cage -> Cage room (then next tick: east)
        3714: 'east',    # West cage -> Cage room
        3715: 'north',   # South cage -> Cage room
    },
    'west': {
        3712: 'west',    # Cage room -> West cage
        3713: 'south',   # North cage -> Cage room (then next tick: west)
        3715: 'north',   # South cage -> Cage room
        3716: 'west',    # East cage -> Cage room
    },
}


@dataclass
class CoordinatorConfig:
    """Configuration for the bot coordinator."""
    # Connection
    host: str = "localhost"
    port: int = 4000
    use_tls: bool = False
    
    # Bot accounts (list of BotAccount or path to accounts file)
    accounts: list[BotAccount] = field(default_factory=list)
    accounts_file: str = ""  # Path to accounts file (alternative to accounts list)
    
    # How many bots to run (uses first N accounts)
    num_bots: int = 0  # 0 = use all accounts
    
    # Navigation (new route-based system)
    use_navigate_behavior: bool = True  # Use NavigateBehavior instead of path commands
    cage_room_vnum: int = 3712  # Target for initial navigation
    distribute_to_cages: bool = True  # Send bots to different cages
    
    # Startup behaviors
    hard_reset_on_startup: bool = True  # Send 'botreset hard' to reset to level 1
    train_on_startup: bool = True  # Visit Training Room and train stats
    practice_on_startup: bool = True  # Visit Practice Room and practice skills
    reset_on_startup: bool = True  # Reset stats/skills to starting values (bot-only)
    
    # Legacy navigation (used if use_navigate_behavior is False)
    navigate_path: list[str] = field(default_factory=lambda: PATH_TO_CAGE_ROOM.copy())
    recall_first: bool = True
    cage_directions: list[str] = field(default_factory=lambda: CAGE_DIRECTIONS.copy())
    
    # Timing
    stagger_delay: float = 1.0  # Seconds between bot spawns
    runtime_seconds: float = 300.0  # Total test duration
    tick_interval: float = 0.5  # Seconds between behavior ticks
    
    # Behavior settings
    flee_hp_percent: float = 20.0
    rest_hp_percent: float = 50.0
    target_mobs: list = field(default_factory=lambda: ["monster"])
    
    # Monitoring
    status_interval: float = 5.0  # Seconds between status prints
    
    def get_accounts(self) -> list[BotAccount]:
        """Get the list of accounts to use."""
        if self.accounts:
            accts = self.accounts
        elif self.accounts_file:
            accts = load_accounts(self.accounts_file)
        else:
            raise ValueError("No accounts configured. Set accounts or accounts_file.")
        
        # Limit to num_bots if specified
        if self.num_bots > 0:
            accts = accts[:self.num_bots]
        
        return accts


class Coordinator:
    """
    Coordinates multiple bot instances for stress testing.
    
    Handles:
    - Staggered bot startup to avoid connection storms
    - Monitoring bot status and health
    - Graceful shutdown
    - Metrics aggregation
    """
    
    def __init__(self, config: CoordinatorConfig):
        self.config = config
        self._bots: dict[str, ManagedBot] = {}
        self._metrics = reset_collector()
        self._running = False
        self._shutdown_event = threading.Event()
        self._executor: Optional[ThreadPoolExecutor] = None
        
        # Callbacks
        self._on_bot_connected: list[Callable[[str], None]] = []
        self._on_bot_disconnected: list[Callable[[str, str], None]] = []
    
    @property
    def metrics(self) -> MetricsCollector:
        return self._metrics
    
    @property
    def running(self) -> bool:
        return self._running
    
    @property
    def bot_count(self) -> int:
        return len(self._bots)
    
    @property
    def connected_count(self) -> int:
        return sum(1 for b in self._bots.values() 
                   if b.status in (BotStatus.LOGGED_IN, BotStatus.PLAYING))
    
    def add_connected_callback(self, callback: Callable[[str], None]) -> None:
        """Add callback for when a bot connects."""
        self._on_bot_connected.append(callback)
    
    def add_disconnected_callback(self, callback: Callable[[str, str], None]) -> None:
        """Add callback for when a bot disconnects (bot_id, reason)."""
        self._on_bot_disconnected.append(callback)
    
    def start(self) -> None:
        """
        Start the coordinator and spawn bots.
        
        This method blocks until the test completes or is interrupted.
        """
        accounts = self.config.get_accounts()
        num_bots = len(accounts)
        
        logger.info(f"Starting coordinator with {num_bots} bots")
        
        self._running = True
        self._shutdown_event.clear()
        self._metrics.start()
        
        # Create thread pool for bot workers
        self._executor = ThreadPoolExecutor(
            max_workers=max(1, num_bots),
            thread_name_prefix="Bot"
        )
        
        try:
            # Spawn bots with staggered startup
            self._spawn_bots()
            
            # Monitor until timeout or shutdown
            self._monitor_loop()
            
        except KeyboardInterrupt:
            logger.info("Interrupted by user")
        finally:
            self.stop()
    
    def stop(self) -> None:
        """Stop all bots and shut down the coordinator."""
        if not self._running:
            return
            
        logger.info("Stopping coordinator...")
        self._running = False
        self._shutdown_event.set()
        
        # Request all bots to stop
        for managed in self._bots.values():
            managed.request_stop()
        
        # Wait for bots to finish
        if self._executor:
            self._executor.shutdown(wait=True, cancel_futures=True)
            self._executor = None
        
        # Disconnect any still connected
        for managed in self._bots.values():
            if managed.bot and managed.bot.is_playing:
                try:
                    managed.bot.send_command("quit")
                    managed.bot.disconnect()
                except Exception:
                    pass
        
        self._metrics.stop()
        logger.info("Coordinator stopped")
    
    def _spawn_bots(self) -> None:
        """Spawn all bots with staggered timing."""
        accounts = self.config.get_accounts()
        
        if not accounts:
            raise ValueError("No bot accounts configured!")
        
        logger.info(f"Spawning {len(accounts)} bots "
                    f"(stagger: {self.config.stagger_delay}s)")
        
        for i, account in enumerate(accounts):
            if self._shutdown_event.is_set():
                break
            
            bot_id = account.username
            
            # Create bot config
            bot_config = BotConfig(
                host=self.config.host,
                port=self.config.port,
                use_tls=self.config.use_tls,
                username=account.username,
                password=account.password
            )
            
            # Create managed bot
            managed = ManagedBot(
                bot_id=bot_id,
                config=bot_config,
                metrics=self._metrics.register_bot(bot_id)
            )
            self._bots[bot_id] = managed
            
            # Submit bot worker to thread pool
            if self._executor:
                future = self._executor.submit(self._run_bot, managed, i)
            
            # Stagger startup
            if i < len(accounts) - 1:
                time.sleep(self.config.stagger_delay)
        
        logger.info(f"All {len(self._bots)} bots spawned")
    
    def _navigate_bot(self, managed: ManagedBot, bot_index: int = 0) -> None:
        """
        Navigate a bot to the training area.
        
        Follows the configured path from recall point to the Cage Room,
        then distributes bots to different cages based on bot_index.
        """
        bot_id = managed.bot_id
        bot = managed.bot
        
        if not bot or not bot.is_playing:
            return
        
        logger.info(f"[{bot_id}] Navigating to training area...")
        
        try:
            # Wake up and stand (might be resting/sleeping)
            bot.send_and_wait("wake", timeout=2.0)
            time.sleep(0.3)
            bot.send_and_wait("stand", timeout=2.0)
            time.sleep(0.3)
            
            # Recall to known location first if configured
            if self.config.recall_first:
                bot.send_and_wait("recall", timeout=3.0)
                time.sleep(0.5)
            
            # Follow navigation path to Cage Room
            for i, direction in enumerate(self.config.navigate_path):
                if managed.should_stop():
                    break
                
                # Send direction command
                response = bot.send_and_wait(direction, timeout=2.0)
                
                # Short delay between moves
                time.sleep(0.3)
                
                # Check if we got stuck
                if response and ("can't go" in response.lower() or "alas" in response.lower()):
                    logger.warning(f"[{bot_id}] Navigation blocked at step {i+1}: {direction}")
                    break
            
            # Distribute bots to different cages (round-robin)
            if self.config.distribute_to_cages and self.config.cage_directions:
                cage_dir = self.config.cage_directions[bot_index % len(self.config.cage_directions)]
                logger.info(f"[{bot_id}] Entering {cage_dir} cage...")
                bot.send_and_wait(cage_dir, timeout=2.0)
                time.sleep(0.3)
            
            logger.info(f"[{bot_id}] Navigation complete (room {bot.stats.room_vnum})")
            
        except Exception as e:
            logger.error(f"[{bot_id}] Navigation error: {e}")
    
    def _run_bot(self, managed: ManagedBot, bot_index: int = 0) -> None:
        """
        Run a single bot instance (executed in worker thread).
        
        Handles login, navigation, behavior loop, and cleanup.
        """
        bot_id = managed.bot_id
        metrics = managed.metrics
        
        logger.debug(f"[{bot_id}] Worker starting")
        
        try:
            # Create bot
            managed.bot = Bot(managed.config)
            managed.bot.metrics = metrics  # Wire up metrics for behaviors to access
            managed.status = BotStatus.CONNECTING
            
            if metrics:
                metrics.connection_attempts += 1
            
            # Connect and login
            if not managed.bot.login(timeout=60.0):
                managed.status = BotStatus.ERROR
                managed.error = "Login failed"
                if metrics:
                    metrics.connection_failures += 1
                logger.error(f"[{bot_id}] Login failed")
                return
            
            managed.status = BotStatus.LOGGED_IN
            if metrics:
                metrics.connected = True
                metrics.connect_time = time.time()
            
            logger.info(f"[{bot_id}] Logged in successfully")
            
            # Notify callbacks
            for callback in self._on_bot_connected:
                try:
                    callback(bot_id)
                except Exception as e:
                    logger.error(f"Connected callback error: {e}")
            
            # Create behavior engine
            # Disable exploration when using navigation to prevent bots from wandering
            include_explore = not self.config.use_navigate_behavior
            managed.engine = create_default_engine(
                managed.bot,
                flee_hp_percent=self.config.flee_hp_percent,
                rest_hp_percent=self.config.rest_hp_percent,
                target_mobs=self.config.target_mobs,
                include_explore=include_explore
            )
            
            # Store back-reference to engine on bot for behaviors to access
            managed.bot._behavior_engine = managed.engine
            
            # Add navigation behavior if using route-based navigation
            if self.config.use_navigate_behavior:
                # Add hard reset behavior first (resets character to level 1 at MUD school)
                if self.config.hard_reset_on_startup:
                    managed.engine.add_behavior(BotResetBehavior())
                    logger.info(f"[{bot_id}] Will perform hard reset to level 1 at startup")
                
                # Add startup behaviors (train/practice before going to cages)
                # If hard_reset is enabled, skip individual reset commands (they're redundant)
                do_individual_resets = self.config.reset_on_startup and not self.config.hard_reset_on_startup
                
                if self.config.train_on_startup:
                    managed.engine.add_behavior(TrainBehavior(
                        route=ROUTE_TO_TRAIN_ROOM,
                        reset_first=do_individual_resets
                    ))
                    reset_msg = " (with reset)" if do_individual_resets else ""
                    logger.info(f"[{bot_id}] Will train stats{reset_msg} before navigating to cages")
                
                if self.config.practice_on_startup:
                    managed.engine.add_behavior(PracticeBehavior(
                        route=ROUTE_TO_PRACTICE_ROOM,
                        reset_first=do_individual_resets
                    ))
                    reset_msg = " (with reset)" if do_individual_resets else ""
                    logger.info(f"[{bot_id}] Will practice skills{reset_msg} before navigating to cages")
                
                # Navigate to cage room (3712) - patrol behavior will handle entering cages
                # Use one_shot=True so this behavior never re-activates after reaching 3712
                # (otherwise it would try to navigate from cage rooms back to 3712)
                # Use PRIORITY_INITIAL_NAVIGATE to run before AttackBehavior kicks in
                managed.engine.add_behavior(NavigateBehavior(
                    destination_vnum=self.config.cage_room_vnum,
                    route=ROUTE_TO_CAGE_ROOM,
                    one_shot=True,
                    priority_override=PRIORITY_INITIAL_NAVIGATE,
                ))
                logger.info(f"[{bot_id}] Will navigate to cage room ({self.config.cage_room_vnum}) (one-shot)")
                
                # Set up patrol and return-to-cage behaviors
                if self.config.distribute_to_cages:
                    # Build full route for returning to cage area from anywhere
                    # Merge cage room route with routes from individual cages
                    full_route = {**ROUTE_TO_CAGE_ROOM}
                    for cage_route in CAGE_ROUTES.values():
                        full_route.update(cage_route)
                    
                    # Add return-to-cage behavior to recover from death/displacement
                    managed.engine.add_behavior(ReturnToCageBehavior(
                        route=full_route,
                    ))
                    
                    # Add patrol behavior to cycle through all cages
                    managed.engine.add_behavior(PatrolCagesBehavior())
                    logger.info(f"[{bot_id}] Will patrol all 4 cage rooms in circuit")
                    
                    # Add buy supplies behavior for hunger/thirst
                    managed.engine.add_behavior(BuySuppliesBehavior())
                    logger.info(f"[{bot_id}] Will buy food/drink when hungry/thirsty")
                    
                    # Add dark creature behavior (fights creature in 3720 after proactive shopping)
                    managed.engine.add_behavior(FightDarkCreatureBehavior())
                    logger.info(f"[{bot_id}] Will fight dark creature after proactive shopping")
                    
                    # Add light source behavior for dark rooms
                    managed.engine.add_behavior(LightSourceBehavior())
                    logger.info(f"[{bot_id}] Will handle light sources in dark rooms")
            else:
                # Legacy path-based navigation
                if self.config.navigate_path:
                    self._navigate_bot(managed, bot_index)
            
            managed.status = BotStatus.PLAYING
            
            # Track XP for kill detection
            # Initialize to -1 so first MSDP reading sets baseline (don't count existing XP as kills)
            last_xp = -1
            
            # Behavior loop
            while not managed.should_stop() and managed.bot.is_playing:
                # Read any incoming data
                text = managed.bot.read_text(timeout=0.2)
                if text:
                    managed.engine.update_room_text(text)
                    if metrics:
                        metrics.record_response_received(len(text))
                
                # Run behavior tick
                behavior_name = managed.engine.tick()
                if metrics and behavior_name:
                    metrics.current_behavior = behavior_name
                
                # Update metrics from stats
                if metrics:
                    stats = managed.bot.stats
                    metrics.hp_percent = stats.hp_percent
                    metrics.current_room_vnum = stats.room_vnum
                    
                    # Track kills via XP changes
                    # First reading establishes baseline, subsequent increases are kills
                    if last_xp < 0:
                        # First valid XP reading - set baseline
                        last_xp = stats.experience
                    elif stats.experience > last_xp:
                        xp_gain = stats.experience - last_xp
                        metrics.record_kill(xp_gain)
                        logger.debug(f"[{bot_id}] Kill! +{xp_gain} XP")
                        last_xp = stats.experience
                
                # Small delay between ticks
                time.sleep(self.config.tick_interval)
            
        except Exception as e:
            managed.status = BotStatus.ERROR
            managed.error = str(e)
            if metrics:
                metrics.parse_errors += 1
            logger.error(f"[{bot_id}] Error: {e}")
            
        finally:
            # Cleanup
            managed.status = BotStatus.DISCONNECTED
            if metrics:
                metrics.connected = False
                metrics.disconnect_time = time.time()
            
            if managed.bot:
                try:
                    if managed.bot.is_playing:
                        managed.bot.send_command("quit")
                    managed.bot.disconnect()
                except Exception:
                    pass
            
            logger.debug(f"[{bot_id}] Worker stopped")
            
            # Notify callbacks
            for callback in self._on_bot_disconnected:
                try:
                    callback(bot_id, managed.error or "shutdown")
                except Exception as e:
                    logger.error(f"Disconnected callback error: {e}")
    
    def _monitor_loop(self) -> None:
        """Monitor bots and print status updates."""
        start_time = time.time()
        last_status_time = start_time
        
        print()  # Newline before status updates
        
        while self._running:
            now = time.time()
            elapsed = now - start_time
            
            # Check timeout
            if elapsed >= self.config.runtime_seconds:
                logger.info("Test duration reached")
                break
            
            # Print status update
            if now - last_status_time >= self.config.status_interval:
                self._metrics.print_live_status()
                last_status_time = now
            
            # Check if all bots have died
            alive_count = sum(1 for b in self._bots.values()
                              if b.status in (BotStatus.CONNECTING, 
                                               BotStatus.LOGGED_IN,
                                               BotStatus.PLAYING))
            if alive_count == 0 and len(self._bots) > 0:
                logger.warning("All bots have disconnected")
                break
            
            # Sleep briefly
            self._shutdown_event.wait(timeout=0.5)
        
        print()  # Newline after status updates
    
    def get_bot_status(self, bot_id: str) -> Optional[BotStatus]:
        """Get status of a specific bot."""
        if bot_id in self._bots:
            return self._bots[bot_id].status
        return None
    
    def get_all_status(self) -> dict[str, BotStatus]:
        """Get status of all bots."""
        return {bot_id: managed.status 
                for bot_id, managed in self._bots.items()}


def run_stress_test(
    host: str = "localhost",
    port: int = 4000,
    accounts: Optional[list[BotAccount]] = None,
    accounts_file: str = "",
    num_bots: int = 0,
    duration: float = 300.0,
    stagger_delay: float = 1.0,
    target_mobs: Optional[list[str]] = None
) -> MetricsCollector:
    """
    Run a stress test with the given configuration.
    
    Args:
        host: MUD server hostname
        port: MUD server port
        accounts: List of BotAccount objects (username/password pairs)
        accounts_file: Path to accounts file (alternative to accounts list)
        num_bots: Max number of bots to run (0 = use all accounts)
        duration: Test duration in seconds
        stagger_delay: Seconds between bot spawns
        target_mobs: Mob keywords to attack
    
    Returns:
        MetricsCollector for analysis
    """
    config = CoordinatorConfig(
        host=host,
        port=port,
        accounts=accounts or [],
        accounts_file=accounts_file,
        num_bots=num_bots,
        runtime_seconds=duration,
        stagger_delay=stagger_delay,
        target_mobs=target_mobs or ["monster"]
    )
    
    coordinator = Coordinator(config)
    
    # Handle Ctrl+C
    def signal_handler(sig, frame):
        print("\n\nInterrupted! Stopping...")
        coordinator.stop()
    
    old_handler = signal.signal(signal.SIGINT, signal_handler)
    
    try:
        coordinator.start()
    finally:
        signal.signal(signal.SIGINT, old_handler)
    
    # Print final summary
    coordinator.metrics.print_summary()
    
    return coordinator.metrics
