# Mud98Bot

A stress testing and automation bot for Mud98 MUD server.

## Requirements

- Python 3.10+
- No external dependencies for basic functionality

## Installation

```bash
cd tools/mud98bot
python -m venv venv
source venv/bin/activate  # On Windows: venv\Scripts\activate
pip install -e .
```

## Quick Start

```bash
# Single bot connection
python -m mud98bot --host localhost --port 4000 --user TestBot --password secret

# Or with config file
python -m mud98bot --config configs/single_bot.yaml
```

## Configuration

See `configs/` directory for example configuration files.

## Features

- Telnet protocol handling (ECHO, SGA, EOR, NAWS)
- MSDP/GMCP support for structured data
- Automatic login flow
- Configurable behaviors (combat, explore, loot, etc.)
- Multi-bot orchestration for stress testing
- Metrics collection and reporting

## Development

```bash
# Run tests
python -m pytest tests/

# Run with debug logging
python -m mud98bot --config configs/single_bot.yaml --log-level DEBUG
```
