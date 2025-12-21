#!/usr/bin/env python3
"""
Test login flow for Mud98Bot.

Usage:
    python test_login.py --user <name> --password <pass> [--host <host>] [--port <port>]
"""

import sys
import argparse
import logging

sys.path.insert(0, '.')

from mud98bot.client import Bot, BotConfig

logging.basicConfig(
    level=logging.DEBUG,
    format="%(asctime)s [%(levelname)s] %(name)s: %(message)s"
)


def test_login(host: str, port: int, user: str, password: str) -> None:
    """Test the full login flow."""
    print(f"\n=== Testing login as {user}@{host}:{port} ===\n")
    
    config = BotConfig(
        host=host,
        port=port,
        username=user,
        password=password
    )
    
    with Bot(config) as bot:
        # Print received text
        def on_text(text: str) -> None:
            for line in text.split('\n'):
                if line.strip():
                    print(f"  > {line.rstrip()}")
        bot.add_text_callback(on_text)
        
        print("Attempting login...")
        if bot.login(timeout=60.0):
            print("\n=== LOGIN SUCCESSFUL ===")
            print(f"State: {bot.state}")
            
            # Wait a moment for MSDP data to arrive
            import time
            time.sleep(0.5)
            bot.read_text(timeout=1.0)
            
            print(f"Stats: HP={bot.stats.health}/{bot.stats.health_max}, "
                  f"Mana={bot.stats.mana}/{bot.stats.mana_max}, "
                  f"Move={bot.stats.movement}/{bot.stats.movement_max}")
            print(f"Level: {bot.stats.level}")
            print(f"Room exits: {bot.room.exits}")
            print(f"In combat: {bot.stats.in_combat}")
            
            # Try a few commands
            print("\n--- Sending 'look' command ---")
            output = bot.send_and_wait("look", timeout=5.0)
            
            print("\n--- Sending 'score' command ---")
            output = bot.send_and_wait("score", timeout=5.0)
            
            print("\n--- Quitting ---")
            bot.send_command("quit")
            bot.read_text(timeout=2.0)
        else:
            print("\n=== LOGIN FAILED ===")
            print(f"Final state: {bot.state}")


def main():
    parser = argparse.ArgumentParser(description="Test Mud98Bot login")
    parser.add_argument('--host', default='localhost')
    parser.add_argument('--port', type=int, default=4000)
    parser.add_argument('--user', required=True)
    parser.add_argument('--password', required=True)
    args = parser.parse_args()
    
    test_login(args.host, args.port, args.user, args.password)


if __name__ == "__main__":
    main()
