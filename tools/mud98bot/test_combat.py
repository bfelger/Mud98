#!/usr/bin/env python3
"""
Test combat flow for Mud98Bot.

Navigates through Mud School to find a mob and fights it,
monitoring MSDP combat variables.

Usage:
    python test_combat.py --user <name> --password <pass> [--host <host>] [--port <port>]
"""

import sys
import argparse
import logging
import time
import re

sys.path.insert(0, '.')

from mud98bot.client import Bot, BotConfig

logging.basicConfig(
    level=logging.DEBUG,
    format="%(asctime)s [%(levelname)s] %(name)s: %(message)s"
)

# Reduce noise from connection/telnet
logging.getLogger('mud98bot.connection').setLevel(logging.INFO)
logging.getLogger('mud98bot.telnet').setLevel(logging.INFO)


# Path from Mud School entrance to the monster cage
# 3700 → north → 3757 → north → 3701 → west → 3702 → north → 3703 
# → north → 3709 → west → 3711 → down → 3712 → east → 3716 (monster room)
PATH_TO_MONSTER = ['north', 'north', 'west', 'north', 'north', 'west', 'down', 'east']


def test_combat(host: str, port: int, user: str, password: str) -> None:
    """Test combat flow with MSDP monitoring."""
    print(f"\n=== Testing Combat as {user}@{host}:{port} ===\n")
    
    config = BotConfig(
        host=host,
        port=port,
        username=user,
        password=password
    )
    
    last_output = []
    
    with Bot(config) as bot:
        def on_text(text: str) -> None:
            last_output.append(text)
            for line in text.split('\n'):
                line = line.strip()
                if line and not line.startswith('<'):  # Skip prompts
                    print(f"  > {line}")
        
        bot.add_text_callback(on_text)
        
        print("Logging in...")
        if not bot.login(timeout=60.0):
            print("LOGIN FAILED")
            return
        
        print("\n=== LOGGED IN ===")
        
        # Wait for initial MSDP
        time.sleep(0.5)
        bot.read_text(timeout=1.0)
        
        print(f"Starting HP: {bot.stats.health}/{bot.stats.health_max}")
        print(f"Room VNUM: {bot.stats.room_vnum}")
        
        # Navigate to the monster using known path
        print("\n--- Navigating to Monster Cage ---")
        
        for direction in PATH_TO_MONSTER:
            print(f"Going {direction}...")
            last_output.clear()
            bot.send_and_wait(direction, timeout=3.0)
            time.sleep(0.3)
            bot.read_text(timeout=0.5)
        
        # Look at the room
        print("\n--- Current Room ---")
        last_output.clear()
        bot.send_and_wait("look", timeout=3.0)
        time.sleep(0.3)
        bot.read_text(timeout=0.5)
        
        # Attack the monster!
        print(f"\n--- Attacking monster ---")
        last_output.clear()
        bot.send_and_wait("kill monster", timeout=3.0)
        time.sleep(0.5)
        bot.read_text(timeout=1.0)
        
        # Check if attack succeeded
        attack_text = ''.join(last_output)
        if "isn't here" in attack_text.lower():
            print("Monster not here - may have been killed already.")
            print("Waiting for respawn or trying recall and retry...")
            bot.send_command("recall")
            time.sleep(1.0)
            bot.read_text(timeout=1.0)
            print("\n--- Quitting ---")
            bot.send_command("quit")
            bot.read_text(timeout=2.0)
            return
        
        # Combat loop - monitor MSDP
        print("\n--- Combat Loop (monitoring MSDP) ---")
        combat_rounds = 0
        max_rounds = 30
        
        while combat_rounds < max_rounds:
            # Read any incoming data (including MSDP)
            bot.read_text(timeout=1.0)
            
            # Check combat status via MSDP
            in_combat = bot.stats.in_combat
            hp = bot.stats.health
            hp_max = bot.stats.health_max
            opp_name = bot.stats.opponent_name
            opp_hp = bot.stats.opponent_health
            opp_hp_max = bot.stats.opponent_health_max
            
            hp_pct = (hp / hp_max * 100) if hp_max > 0 else 0
            opp_hp_pct = (opp_hp / opp_hp_max * 100) if opp_hp_max > 0 else 0
            
            print(f"Round {combat_rounds + 1}: "
                  f"HP={hp}/{hp_max} ({hp_pct:.0f}%) | "
                  f"In Combat={in_combat} | "
                  f"Opponent={opp_name or 'None'} HP={opp_hp}/{opp_hp_max} ({opp_hp_pct:.0f}%)")
            
            if not in_combat:
                if combat_rounds > 0:
                    print("\n*** Combat ended! ***")
                    break
                else:
                    # Not in combat yet, might need to wait or attack again
                    combat_rounds += 1
                    if combat_rounds >= 3:
                        print("Failed to enter combat")
                        break
                    continue
            
            # Check if we should flee
            if hp_pct < 30:
                print("HP LOW! Fleeing!")
                bot.send_command("flee")
                time.sleep(1.0)
                bot.read_text(timeout=1.0)
                break
            
            combat_rounds += 1
            time.sleep(1.0)  # Wait for next combat round
        
        # Final status
        print("\n--- Final Status ---")
        bot.read_text(timeout=1.0)
        print(f"HP: {bot.stats.health}/{bot.stats.health_max}")
        print(f"In Combat: {bot.stats.in_combat}")
        
        # Loot the corpse if we won
        if not bot.stats.in_combat:
            print("\n--- Looting corpse ---")
            bot.send_and_wait("get all corpse", timeout=3.0)
            time.sleep(0.3)
            bot.send_and_wait("sacrifice corpse", timeout=3.0)
            time.sleep(0.3)
            bot.read_text(timeout=1.0)
        
        # Rest if wounded
        if bot.stats.health < bot.stats.health_max:
            print("\nResting to recover HP...")
            bot.send_command("sleep")
            
            for _ in range(5):
                time.sleep(2.0)
                bot.read_text(timeout=1.0)
                print(f"  HP: {bot.stats.health}/{bot.stats.health_max}")
                if bot.stats.health >= bot.stats.health_max:
                    break
            
            bot.send_command("wake")
            bot.read_text(timeout=1.0)
        
        print("\n--- Returning to temple ---")
        bot.send_and_wait("recall", timeout=3.0)
        time.sleep(0.3)
        bot.read_text(timeout=1.0)
        
        print("\n--- Quitting ---")
        bot.send_command("quit")
        bot.read_text(timeout=2.0)
        
        print("\n=== COMBAT TEST COMPLETE ===")


def main():
    parser = argparse.ArgumentParser(description="Test Mud98Bot combat")
    parser.add_argument('--host', default='localhost')
    parser.add_argument('--port', type=int, default=4000)
    parser.add_argument('--user', required=True)
    parser.add_argument('--password', required=True)
    args = parser.parse_args()
    
    test_combat(args.host, args.port, args.user, args.password)


if __name__ == "__main__":
    main()
