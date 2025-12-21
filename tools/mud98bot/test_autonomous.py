#!/usr/bin/env python3
"""
Autonomous play test for Mud98Bot.

The bot will:
1. Log in
2. Navigate to Mud School monster cages
3. Fight, loot, heal in a loop using the behavior engine
4. Run for a specified duration or number of kills

Usage:
    python test_autonomous.py --user <name> --password <pass> [--duration <seconds>]
"""

import sys
import argparse
import logging
import time
import signal

sys.path.insert(0, '.')

from mud98bot.client import Bot, BotConfig
from mud98bot.behaviors import (
    BehaviorEngine, BehaviorContext, BehaviorResult,
    SurviveBehavior, CombatBehavior, HealBehavior, LootBehavior,
    AttackBehavior, ExploreBehavior, RecallBehavior, IdleBehavior
)

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(name)s: %(message)s"
)

logger = logging.getLogger(__name__)

# Reduce noise from low-level modules
logging.getLogger('mud98bot.connection').setLevel(logging.WARNING)
logging.getLogger('mud98bot.telnet').setLevel(logging.WARNING)
logging.getLogger('mud98bot.msdp').setLevel(logging.INFO)


# Path to the monster cages in Mud School
# From temple (recall point) → up → Mud School entrance
# From entrance → north → north → west → north → north → west → down → east → (cage room)
PATH_TO_CAGES = ['up', 'north', 'north', 'west', 'north', 'north', 'west', 'down', 'east']


class AutonomousRunner:
    """Runs the bot autonomously with the behavior engine."""
    
    def __init__(self, bot: Bot, duration: float = 300.0):
        self.bot = bot
        self.duration = duration
        self.engine = self._create_engine()
        
        # Stats tracking
        self.kills = 0
        self.deaths = 0
        self.xp_gained = 0
        self.start_xp = 0
        self.rooms_visited = 0
        
        # Room text buffer for parsing
        self._room_buffer = ""
        self._running = True
        
    def _create_engine(self) -> BehaviorEngine:
        """Create behavior engine with appropriate settings."""
        engine = BehaviorEngine(self.bot)
        
        # Add behaviors tuned for Mud School
        engine.add_behavior(SurviveBehavior(flee_hp_percent=20.0))
        engine.add_behavior(CombatBehavior())
        engine.add_behavior(LootBehavior())
        engine.add_behavior(HealBehavior(
            rest_hp_percent=40.0,  # Only heal when below 40%
            sleep_hp_percent=25.0,
            target_hp_percent=80.0  # Heal to 80% and resume fighting
        ))
        engine.add_behavior(AttackBehavior(
            target_keywords=['monster']  # Mud School training mobs
        ))
        # Disable recall for testing - character depletes movement walking to cage
        # engine.add_behavior(RecallBehavior(low_move_percent=10.0))
        engine.add_behavior(ExploreBehavior(max_rooms=50))
        engine.add_behavior(IdleBehavior())
        
        return engine
    
    def run(self) -> None:
        """Run autonomous play for the specified duration."""
        logger.info(f"Starting autonomous play for {self.duration:.0f} seconds...")
        
        # Navigate to the cage room first
        self._navigate_to_cages()
        
        # Record starting XP
        self.start_xp = self.bot.stats.experience
        
        start_time = time.time()
        last_status = start_time
        last_xp = self.start_xp
        last_look = start_time  # Don't look again immediately after navigation
        
        # Main loop
        while self._running and (time.time() - start_time) < self.duration:
            # Read any incoming data
            text = self.bot.read_text(timeout=0.2)
            if text:
                self._room_buffer += text
                self.engine.update_room_text(self._room_buffer)
            
            # Periodically look to refresh room info (but not too often)
            if time.time() - last_look > 10.0 and not self.bot.stats.in_combat:
                look_text = self.bot.send_and_wait("look", timeout=2.0)
                last_look = time.time()
                self._room_buffer = look_text  # Use returned text directly
                self.engine.update_room_text(self._room_buffer)
            
            # Get context for debugging
            ctx = self.engine.get_context()
            
            # Debug: Show detected mobs
            if ctx.room_mobs and not self.bot.stats.in_combat:
                logger.info(f"Detected mobs in room: {ctx.room_mobs}")
            
            # Run behavior engine tick
            current = self.engine.tick()
            
            # Track kills via XP changes
            current_xp = self.bot.stats.experience
            if current_xp > last_xp:
                self.xp_gained += (current_xp - last_xp)
                self.kills += 1
                logger.info(f"Kill #{self.kills}! XP gained: {current_xp - last_xp}")
                last_xp = current_xp
                # Clear room buffer after kill to detect corpse
                self._room_buffer = ""
            
            # Status update every 30 seconds
            if time.time() - last_status >= 30.0:
                self._print_status(time.time() - start_time)
                last_status = time.time()
            
            # Small delay to avoid spinning
            time.sleep(0.1)
        
        # Final status
        total_time = time.time() - start_time
        self._print_final_status(total_time)
    
    def _navigate_to_cages(self) -> None:
        """Navigate from recall point to the monster cages."""
        logger.info("Navigating to monster cages...")
        
        # Make sure we're standing (player might be resting/sleeping)
        self.bot.send_and_wait("wake", timeout=2.0)
        time.sleep(0.3)
        self.bot.send_and_wait("stand", timeout=2.0)
        time.sleep(0.3)
        
        # Check movement - if low, rest to recover fully
        if self.bot.stats.movement < 80:
            print(f"Low movement ({self.bot.stats.movement}), resting to recover...")
            self.bot.send_and_wait("rest", timeout=2.0)
            # Wait for movement to recover (ROM ticks every ~15 seconds)
            for i in range(12):  # Wait up to ~3 minutes for full recovery
                time.sleep(5.0)
                self.bot.send_command("score")  # Trigger MSDP update
                time.sleep(0.5)
                self.bot.read_text(timeout=1.0)
                print(f"  Movement: {self.bot.stats.movement}/{self.bot.stats.movement_max}")
                if self.bot.stats.movement >= 80:
                    break
            self.bot.send_and_wait("stand", timeout=2.0)
            time.sleep(0.3)
        
        # First recall to get to known location
        self.bot.send_and_wait("recall", timeout=3.0)
        time.sleep(0.5)
        
        # Clear any accumulated text
        self.bot.read_text(timeout=0.5)
        
        # Follow the path - don't rely on VNUM, just send commands
        print("Following path to cage room...")
        for direction in PATH_TO_CAGES:
            self.bot.send_command(direction)
            time.sleep(0.5)  # Give time for movement
        
        # Wait for everything to settle
        time.sleep(1.0)
        self.bot.read_text(timeout=1.0)  # Clear buffer
        
        # Look around to populate room info - use the returned text!
        text = self.bot.send_and_wait("look", timeout=2.0)
        
        # Debug: print room text to see what we got
        print(f"\n--- ROOM TEXT ---\n{text}\n--- END ---\n")
        print(f"ROOM_VNUM: {self.bot.stats.room_vnum}")
        
        # Update engine with room text and preserve in buffer
        self.engine.update_room_text(text)
        self._room_buffer = text  # Preserve for main loop
        
        logger.info("Arrived at cage room!")
    
    def _print_status(self, elapsed: float) -> None:
        """Print current status."""
        stats = self.bot.stats
        remaining = max(0, self.duration - elapsed)
        
        print(f"\n=== Status ({elapsed:.0f}s elapsed, {remaining:.0f}s remaining) ===")
        print(f"HP: {stats.health}/{stats.health_max} ({stats.health/max(1,stats.health_max)*100:.0f}%)")
        print(f"Combat: {'Yes' if stats.in_combat else 'No'}", end="")
        if stats.in_combat:
            print(f" vs {stats.opponent_name} ({stats.opponent_health}/{stats.opponent_health_max})")
        else:
            print()
        print(f"Kills: {self.kills} | XP Gained: {self.xp_gained}")
        print(f"Behavior: {self.engine.current_behavior_name or 'None'}")
        print()
    
    def _print_final_status(self, total_time: float) -> None:
        """Print final status summary."""
        print("\n" + "=" * 50)
        print("AUTONOMOUS PLAY COMPLETE")
        print("=" * 50)
        print(f"Duration: {total_time:.0f} seconds")
        print(f"Kills: {self.kills}")
        print(f"XP Gained: {self.xp_gained}")
        print(f"XP/minute: {self.xp_gained / max(1, total_time) * 60:.0f}")
        print(f"Final HP: {self.bot.stats.health}/{self.bot.stats.health_max}")
        print(f"Final Level: {self.bot.stats.level}")
        print("=" * 50)
    
    def stop(self) -> None:
        """Stop the autonomous runner."""
        self._running = False


def run_autonomous(host: str, port: int, user: str, password: str, 
                   duration: float) -> None:
    """Run the autonomous bot."""
    print(f"\n=== Autonomous Play: {user}@{host}:{port} ===")
    print(f"Duration: {duration:.0f} seconds")
    print()
    
    config = BotConfig(
        host=host,
        port=port,
        username=user,
        password=password
    )
    
    runner = None
    
    # Handle Ctrl+C gracefully
    def signal_handler(sig, frame):
        print("\n\nInterrupted! Stopping...")
        if runner:
            runner.stop()
    
    signal.signal(signal.SIGINT, signal_handler)
    
    with Bot(config) as bot:
        # Text callback to capture room info
        room_buffer = []
        def on_text(text: str) -> None:
            room_buffer.append(text)
            # Keep only recent text
            while len(room_buffer) > 10:
                room_buffer.pop(0)
        
        bot.add_text_callback(on_text)
        
        print("Logging in...")
        if not bot.login(timeout=60.0):
            print("LOGIN FAILED")
            return
        
        print("Login successful!")
        
        # Wait for initial MSDP
        time.sleep(1.0)
        bot.read_text(timeout=2.0)
        
        print(f"Starting stats: Level {bot.stats.level}, "
              f"HP {bot.stats.health}/{bot.stats.health_max}, "
              f"XP {bot.stats.experience}")
        
        # Create and run the autonomous runner
        runner = AutonomousRunner(bot, duration)
        
        # Hook up room text updates
        def update_room():
            if room_buffer:
                runner.engine.update_room_text('\n'.join(room_buffer))
                room_buffer.clear()
        
        # Modify the runner to call update_room
        original_tick = runner.engine.tick
        def enhanced_tick():
            update_room()
            return original_tick()
        runner.engine.tick = enhanced_tick
        
        try:
            runner.run()
        except Exception as e:
            logger.error(f"Error during autonomous play: {e}")
            import traceback
            traceback.print_exc()
        
        # Clean exit
        print("\nQuitting...")
        bot.send_command("quit")
        bot.read_text(timeout=2.0)


def main():
    parser = argparse.ArgumentParser(description="Mud98Bot Autonomous Play")
    parser.add_argument('--host', default='localhost')
    parser.add_argument('--port', type=int, default=4000)
    parser.add_argument('--user', required=True)
    parser.add_argument('--password', required=True)
    parser.add_argument('--duration', type=float, default=120.0,
                        help="Duration in seconds (default: 120)")
    args = parser.parse_args()
    
    run_autonomous(args.host, args.port, args.user, args.password, args.duration)


if __name__ == "__main__":
    main()
