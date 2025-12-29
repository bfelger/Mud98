#!/usr/bin/env python3
"""
Stress test script for Mud98Bot.

Runs multiple bot instances concurrently to stress test the MUD server.

Usage:
    python test_stress.py --accounts configs/bot_accounts.txt --duration 120

Prerequisites:
    - Server running on localhost:4000
    - Bot accounts file with username:password pairs
    - Accounts pre-created on server (or server allows "random" for random names)
"""

import sys
import argparse
import logging
import json
from pathlib import Path

sys.path.insert(0, '.')

from mud98bot.coordinator import (
    run_stress_test, CoordinatorConfig, Coordinator, 
    BotAccount, load_accounts
)
from mud98bot.metrics import get_collector

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(name)s: %(message)s"
)

logger = logging.getLogger(__name__)

# Reduce noise from low-level modules
logging.getLogger('mud98bot.connection').setLevel(logging.WARNING)
logging.getLogger('mud98bot.telnet').setLevel(logging.WARNING)
logging.getLogger('mud98bot.msdp').setLevel(logging.INFO)

# Enable command logging from client (DEBUG level for CMD: messages)
logging.getLogger('mud98bot.client').setLevel(logging.DEBUG)

# Default accounts file path
DEFAULT_ACCOUNTS_FILE = Path(__file__).parent / "configs" / "bot_accounts.txt"


def main():
    parser = argparse.ArgumentParser(
        description="Mud98Bot Stress Test",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
    # Run all bots from accounts file for 2 minutes
    python test_stress.py --accounts configs/bot_accounts.txt --duration 120

    # Run only first 5 bots from accounts file
    python test_stress.py --accounts configs/bot_accounts.txt --bots 5

    # Quick single-user test (creates account inline)
    python test_stress.py --user Tester --password test123 --duration 60

    # Save results to JSON
    python test_stress.py --accounts configs/bot_accounts.txt --output results.json

Account file format (one per line):
    username:password
    # Lines starting with # are comments
        """
    )
    
    # Connection options
    parser.add_argument('--host', default='localhost',
                        help='MUD server hostname (default: localhost)')
    parser.add_argument('--port', type=int, default=4000,
                        help='MUD server port (default: 4000)')
    parser.add_argument('--tls', action='store_true',
                        help='Use TLS connection')
    
    # Account configuration (mutually exclusive)
    acct_group = parser.add_mutually_exclusive_group(required=True)
    acct_group.add_argument('--accounts', '-a',
                            help='Path to accounts file (username:password per line)')
    acct_group.add_argument('--user', 
                            help='Single username for quick testing')
    
    parser.add_argument('--password', '-p',
                        help='Password (required with --user)')
    parser.add_argument('--bots', '-n', type=int, default=0,
                        help='Max number of bots to run (0 = all accounts)')
    
    # Timing
    parser.add_argument('--duration', '-d', type=float, default=120.0,
                        help='Test duration in seconds (default: 120)')
    parser.add_argument('--stagger', type=float, default=1.0,
                        help='Delay between bot spawns in seconds (default: 1.0)')
    
    # Behavior
    parser.add_argument('--targets', nargs='+', default=['monster'],
                        help='Mob keywords to attack (default: monster)')
    parser.add_argument('--flee-hp', type=float, default=20.0,
                        help='HP%% to flee at (default: 20)')
    parser.add_argument('--rest-hp', type=float, default=50.0,
                        help='HP%% to rest at (default: 50)')
    
    # Output
    parser.add_argument('--output', '-o',
                        help='Save results to JSON file')
    parser.add_argument('--log-level', default='INFO',
                        choices=['DEBUG', 'INFO', 'WARNING', 'ERROR'],
                        help='Logging level (default: INFO)')
    parser.add_argument('--verbose', '-v', action='store_true',
                        help='Enable verbose (DEBUG) logging')
    
    args = parser.parse_args()
    
    # Validate single-user mode
    if args.user and not args.password:
        parser.error("--password is required when using --user")
    
    # Configure logging level
    if args.verbose:
        logging.getLogger().setLevel(logging.DEBUG)
        logging.getLogger('mud98bot').setLevel(logging.DEBUG)
    else:
        logging.getLogger().setLevel(getattr(logging, args.log_level))
    
    # Load or create accounts list
    if args.accounts:
        accounts_file = args.accounts
        try:
            accounts = load_accounts(accounts_file)
        except FileNotFoundError as e:
            logger.error(str(e))
            sys.exit(1)
        
        if args.bots > 0:
            accounts = accounts[:args.bots]
    else:
        # Single user mode
        accounts = [BotAccount(username=args.user, password=args.password)]
        accounts_file = None
    
    if not accounts:
        logger.error("No accounts to run!")
        sys.exit(1)
    
    # Print banner
    print("\n" + "=" * 60)
    print("MUD98BOT STRESS TEST")
    print("=" * 60)
    print(f"Server:   {args.host}:{args.port}" + (" (TLS)" if args.tls else ""))
    print(f"Bots:     {len(accounts)}")
    print(f"Duration: {args.duration:.0f} seconds")
    print(f"Targets:  {', '.join(args.targets)}")
    if accounts_file:
        print(f"Accounts: {accounts_file}")
    print(f"Users:    {', '.join(a.username for a in accounts[:5])}" +
          (f" ... (+{len(accounts)-5} more)" if len(accounts) > 5 else ""))
    print("=" * 60)
    print()
    
    # Run the stress test
    try:
        metrics = run_stress_test(
            host=args.host,
            port=args.port,
            accounts=accounts,
            num_bots=0,  # Use all provided accounts
            duration=args.duration,
            stagger_delay=args.stagger,
            target_mobs=args.targets
        )
        
        # Save results if requested
        if args.output:
            output_path = Path(args.output)
            results = metrics.to_dict()
            results['config'] = {
                'host': args.host,
                'port': args.port,
                'num_bots': args.bots,
                'duration': args.duration,
                'targets': args.targets
            }
            
            with open(output_path, 'w') as f:
                json.dump(results, f, indent=2)
            
            print(f"\nResults saved to: {output_path}")
        
        # Exit with success
        agg = metrics.get_aggregate()
        if agg.total_connection_failures == agg.total_connection_attempts:
            logger.error("All connections failed!")
            sys.exit(1)
        
        print("\nStress test completed successfully!")
        
    except KeyboardInterrupt:
        print("\n\nTest interrupted by user")
        sys.exit(130)
    except Exception as e:
        logger.error(f"Stress test failed: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)


if __name__ == "__main__":
    main()
