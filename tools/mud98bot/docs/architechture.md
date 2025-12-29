# Mud98Bot Architecture

This document provides an overview of the Mud98Bot architecture, designed for both human developers and AI coding assistants.

## Overview

Mud98Bot is an automated MUD client for stress testing and autonomous play on the Mud98 server. It connects via TCP/TLS, speaks the MUD Telnet protocol (including MSDP, GMCP), and uses a priority-based behavior system to make decisions.

## Directory Structure

```
mud98bot/
├── __init__.py          # Package exports
├── client.py            # Bot class - main entry point
├── connection.py        # Low-level TCP/TLS socket handling
├── telnet.py            # Telnet protocol (IAC, WILL/WONT, etc.)
├── msdp.py              # MSDP protocol (structured game data)
├── parser.py            # Text parsing for room/mob detection
├── metrics.py           # Performance metrics collection
├── coordinator.py       # Multi-bot orchestration
│
├── config/              # Centralized configuration
│   ├── __init__.py      # Re-exports all config
│   ├── rooms.py         # Room VNUMs and navigation routes
│   └── constants.py     # Priorities, thresholds, timing
│
└── behaviors/           # Behavior system (see BEHAVIORS.md)
    ├── __init__.py      # Package exports + create_default_engine()
    ├── base.py          # BehaviorContext, BehaviorResult, Behavior ABC
    ├── engine.py        # BehaviorEngine - selection and execution
    ├── combat.py        # AttackBehavior, CombatBehavior
    ├── survival.py      # DeathRecovery, Survive, Heal, Recall
    ├── inventory.py     # Loot, Shop, BuySupplies
    ├── navigation.py    # Navigate, Explore, Patrol, ReturnToCage
    ├── training.py      # Train, Practice, BotReset
    └── idle.py          # IdleBehavior fallback
```

## Core Components

### 1. Connection Layer (`connection.py`, `telnet.py`)

**Purpose**: Handle raw network communication and Telnet protocol.

- `Connection`: Manages TCP/TLS socket, threading, read/write buffers
- `TelnetHandler`: Interprets Telnet IAC sequences, negotiates options
- Supports: MSDP, GMCP, MCCP (compression), TLS encryption

**Key Pattern**: The connection runs a read thread that buffers incoming data. The telnet handler strips control sequences and emits clean text.

### 2. Protocol Parsers (`msdp.py`, `parser.py`)

**Purpose**: Extract structured data from game output.

- `MSDPParser`: Decodes MSDP table/array data into Python objects
- `CharacterStats`: HP, mana, movement, level, money, combat state
- `RoomInfo`: Room VNUM, exits, name
- `TextParser`: Parses room descriptions, mob names from plain text
- `BotRoomData`: Structured BOT protocol data (when available)

**Key Pattern**: MSDP provides real-time stat updates. BOT protocol provides structured room contents. Text parsing is fallback.

### 3. Bot Client (`client.py`)

**Purpose**: High-level bot interface combining all components.

- `Bot`: Main class - handles login, commands, state management
- `BotConfig`: Configuration (host, port, credentials, timing)
- Coordinates: Connection ↔ Telnet ↔ MSDP ↔ Parser ↔ Behaviors

**Key Pattern**: Bot runs a main loop that:
1. Processes incoming data
2. Updates MSDP stats
3. Runs behavior engine tick
4. Sends queued commands

### 4. Behavior System (`behaviors/`)

**Purpose**: Decision-making and autonomous action.

See [BEHAVIORS.md](BEHAVIORS.md) for detailed documentation.

**Key Pattern**: Priority-based selection. Highest priority behavior that `can_start()` returns True runs each tick.

### 5. Configuration (`config/`)

**Purpose**: Centralize all magic numbers and routes.

- `rooms.py`: All room VNUMs (temple, cages, train, shop, etc.) and navigation route dictionaries
- `constants.py`: Behavior priorities, HP/mana thresholds, timing values

**Key Pattern**: Import from `mud98bot.config` for any game-specific constant. Never hardcode VNUMs or priorities in behavior code.

### 6. Multi-Bot Coordination (`coordinator.py`)

**Purpose**: Spawn and manage multiple bot instances.

- `Coordinator`: Thread pool, lifecycle management, error handling
- `CoordinatorConfig`: Bot count, timing, spawn patterns
- `BotAccount`: Username/password pairs loaded from file

**Key Pattern**: Each bot runs in its own thread. Coordinator provides callbacks for events (login, combat, death).

### 7. Metrics (`metrics.py`)

**Purpose**: Track performance for stress testing.

- `MetricsCollector`: Thread-safe counters and timing
- `BotMetrics`: Per-bot statistics (commands, kills, deaths, latency)

## Data Flow

```
[MUD Server]
     ↓ TCP/TLS
[Connection] → read thread → [TelnetHandler]
                                   ↓
                    ├── MSDP data → [MSDPParser] → CharacterStats
                    ├── BOT data → [BotRoomData]
                    └── Text → [TextParser] → room_mobs, room_items
                                   ↓
                            [Bot.stats, Bot.room]
                                   ↓
                            [BehaviorEngine]
                                   ↓
                    ├── get_context() → [BehaviorContext]
                    ├── behavior.can_start(ctx)
                    └── behavior.tick(bot, ctx)
                                   ↓
                         bot.send_command()
                                   ↓
                            [Connection] → TCP
                                   ↓
                            [MUD Server]
```

## Adding New Features

### New Behavior

1. Create class in appropriate module (`behaviors/combat.py`, etc.)
2. Inherit from `Behavior`
3. Set `priority`, `name`, `tick_delay`
4. Implement `can_start(ctx)` and `tick(bot, ctx)`
5. Add to `behaviors/__init__.py` exports
6. Add constant to `config/constants.py` if new priority

### New Room/Route

1. Add VNUM constant to `config/rooms.py`
2. Create route dictionary if navigation needed
3. Add to `config/__init__.py` exports

### New Threshold/Timing

1. Add constant to `config/constants.py`
2. Reference in behavior code via import

## Common Patterns

### Behavior State Machine

Many behaviors use internal state for multi-step actions:

```python
class MyBehavior(Behavior):
    STATE_PHASE1 = 'phase1'
    STATE_PHASE2 = 'phase2'
    
    def __init__(self):
        self._state = self.STATE_PHASE1
    
    def tick(self, bot, ctx):
        if self._state == self.STATE_PHASE1:
            # do phase 1
            self._state = self.STATE_PHASE2
        elif self._state == self.STATE_PHASE2:
            # do phase 2
            return BehaviorResult.COMPLETED
        return BehaviorResult.CONTINUE
```

### Interruptible Behaviors

Higher priority behaviors can interrupt lower ones:

```python
def tick(self, bot, ctx):
    if ctx.in_combat:
        return BehaviorResult.WAITING  # Let combat behavior handle it
    # ... normal logic
```

### Proactive vs Reactive

- **Reactive**: `can_start()` checks game state (hungry, in combat, etc.)
- **Proactive**: External trigger sets flag (e.g., `_should_proactive_shop`)

## Testing

- Unit tests in `tests/` (not yet comprehensive)
- `test_autonomous.py`: Live integration test with real server
- `test_multibot.py`: Multi-bot stress test

## Troubleshooting

### Bot Not Responding

1. Check `bot.stats.room_vnum` - is MSDP working?
2. Check `bot.bot_mode` - is BOT protocol enabled?
3. Enable debug logging: `logging.getLogger('mud98bot').setLevel(logging.DEBUG)`

### Behavior Not Activating

1. Check `behavior.can_start(ctx)` conditions
2. Verify priority is high enough to run
3. Check if another behavior is blocking

### Navigation Failing

1. Verify room VNUM is in route dictionary
2. Check `ctx.position.can_move` - is bot standing?
3. Look for "not in route" warnings in logs
