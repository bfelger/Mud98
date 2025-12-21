"""
Main entry point for Mud98Bot.
"""

import argparse
import logging
import sys
import yaml
from pathlib import Path
from typing import Optional

from .client import Bot, BotConfig

logger = logging.getLogger("mud98bot")


def setup_logging(level: str = "INFO", log_file: Optional[str] = None) -> None:
    """Configure logging."""
    numeric_level = getattr(logging, level.upper(), logging.INFO)
    
    handlers = [logging.StreamHandler(sys.stdout)]
    if log_file:
        handlers.append(logging.FileHandler(log_file))
    
    logging.basicConfig(
        level=numeric_level,
        format="%(asctime)s [%(levelname)s] %(name)s: %(message)s",
        handlers=handlers
    )


def load_config(config_path: str) -> dict:
    """Load YAML configuration file."""
    with open(config_path, 'r') as f:
        return yaml.safe_load(f)


def config_from_dict(cfg: dict) -> BotConfig:
    """Create BotConfig from dictionary."""
    conn = cfg.get('connection', {})
    creds = cfg.get('credentials', {})
    new_char = cfg.get('new_character', {})
    throttle = cfg.get('throttle', {})
    
    return BotConfig(
        host=conn.get('host', 'localhost'),
        port=conn.get('port', 4000),
        use_tls=conn.get('use_tls', False),
        timeout=conn.get('timeout', 30.0),
        username=creds.get('username', ''),
        password=creds.get('password', ''),
        new_char_race=new_char.get('race', 'human'),
        new_char_class=new_char.get('class', 'warrior'),
        new_char_sex=new_char.get('sex', 'male'),
        new_char_alignment=new_char.get('alignment', 'neutral'),
        min_command_delay=throttle.get('min_command_delay_ms', 250) / 1000.0,
    )


def run_single_bot(config: BotConfig) -> None:
    """Run a single bot instance."""
    logger.info(f"Starting bot: {config.username}@{config.host}:{config.port}")
    
    with Bot(config) as bot:
        # Login
        if not bot.login():
            logger.error("Login failed")
            return
            
        logger.info("Login successful!")
        logger.info(f"Stats: HP={bot.stats.health}/{bot.stats.health_max}")
        
        # Simple interactive loop
        try:
            while bot.is_playing:
                # Read any output
                text = bot.read_text(timeout=0.5)
                if text:
                    print(text, end='', flush=True)
                
                # For now, just idle
                # In future: behavior engine would take over here
                
        except KeyboardInterrupt:
            logger.info("Interrupted by user")
            bot.send_command("quit")


def run_interactive(config: BotConfig) -> None:
    """Run bot in interactive mode (for testing)."""
    logger.info(f"Interactive mode: {config.username}@{config.host}:{config.port}")
    
    with Bot(config) as bot:
        # Add text callback to print output
        def on_text(text: str) -> None:
            print(text, end='', flush=True)
        bot.add_text_callback(on_text)
        
        # Login
        if not bot.login():
            logger.error("Login failed")
            return
            
        logger.info("Login successful! Type commands (Ctrl+C to quit)")
        
        try:
            while bot.is_playing:
                # Read input
                try:
                    cmd = input()
                except EOFError:
                    break
                    
                if cmd.lower() == 'quit':
                    bot.send_command('quit')
                    break
                    
                bot.send_command(cmd)
                
                # Read response
                bot.wait_for_prompt(timeout=5.0)
                
        except KeyboardInterrupt:
            logger.info("Interrupted")


def main() -> None:
    """Main entry point."""
    parser = argparse.ArgumentParser(
        description="Mud98Bot - Stress testing and automation bot"
    )
    
    # Connection options
    parser.add_argument('--host', default='localhost',
                       help='MUD server hostname')
    parser.add_argument('--port', type=int, default=4000,
                       help='MUD server port')
    parser.add_argument('--tls', action='store_true',
                       help='Use TLS connection')
    
    # Credentials
    parser.add_argument('--user', '--username',
                       help='Character name')
    parser.add_argument('--password',
                       help='Password')
    
    # Config file
    parser.add_argument('--config', '-c',
                       help='Path to YAML config file')
    
    # Modes
    parser.add_argument('--interactive', '-i', action='store_true',
                       help='Run in interactive mode')
    
    # Logging
    parser.add_argument('--log-level', default='INFO',
                       choices=['DEBUG', 'INFO', 'WARNING', 'ERROR'],
                       help='Logging level')
    parser.add_argument('--log-file',
                       help='Log to file')
    
    args = parser.parse_args()
    
    # Setup logging
    setup_logging(args.log_level, args.log_file)
    
    # Build config
    if args.config:
        cfg_dict = load_config(args.config)
        config = config_from_dict(cfg_dict)
        
        # Apply logging from config if present
        log_cfg = cfg_dict.get('logging', {})
        if log_cfg.get('level'):
            setup_logging(log_cfg['level'], log_cfg.get('file'))
    else:
        config = BotConfig()
    
    # Override with command line args
    if args.host:
        config.host = args.host
    if args.port:
        config.port = args.port
    if args.tls:
        config.use_tls = True
    if args.user:
        config.username = args.user
    if args.password:
        config.password = args.password
    
    # Validate
    if not config.username:
        logger.error("Username required (--user or config file)")
        sys.exit(1)
    if not config.password:
        logger.error("Password required (--password or config file)")
        sys.exit(1)
    
    # Run
    if args.interactive:
        run_interactive(config)
    else:
        run_single_bot(config)


if __name__ == '__main__':
    main()
