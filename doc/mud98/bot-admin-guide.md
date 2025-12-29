# Mud98 Bot Administration Guide

This guide covers the setup, operation, and troubleshooting of the Mud98 automated bot system for stress testing and gameplay validation.

## Overview

The Mud98 bot system (`mud98bot`) is a Python-based automation framework that simulates player behavior. Bots can:

- Connect via telnet or TLS
- Create new characters or login to existing ones
- Navigate the MUD School training area
- Fight monsters, loot corpses, and equip gear
- Train stats and practice skills
- Handle death recovery and survival situations
- Execute multi-step quests

The primary use cases are:
- **Stress testing**: Run multiple concurrent bots to test server stability
- **Regression testing**: Verify game mechanics work correctly after code changes
- **Balance validation**: Observe combat outcomes at various levels

## Prerequisites

### Server Requirements

1. **MUD server running** with telnet enabled (port 4000 by default)
2. **MSDP enabled** (on by default):
   ```
   msdp_enabled = true
   ```

### Python Requirements

- Python 3.10 or later
- No external dependencies (uses only standard library)

The bot package is located at:
```
tools/mud98bot/
```

## Quick Start

### Single Bot Test

```bash
cd tools/mud98bot
python3 test_stress.py --user Testbot --password testpass --bots 1 --duration 60
```

This will:
1. Connect to localhost:4000
2. Login as "Testbot" (creating the character if needed)
3. Run autonomously for 60 seconds
4. Display metrics at completion

### Multi-Bot Stress Test

```bash
python3 test_stress.py --user Bot --password botpass --bots 5 --duration 300
```

This creates 5 bots named Bot1, Bot2, Bot3, Bot4, Bot5, all sharing the password "botpass".

## Command-Line Options

| Option | Default | Description |
|--------|---------|-------------|
| `--user` | (required) | Base username for bots |
| `--password` | (required) | Password for all bots |
| `--bots` | 1 | Number of concurrent bots |
| `--duration` | 60 | Test duration in seconds |
| `--host` | localhost | MUD server hostname |
| `--port` | 4000 | MUD server port |
| `--tls` | false | Use TLS encryption |
| `--stagger` | 2.0 | Seconds between bot spawns |

### Example: Remote Server with TLS

```bash
python3 test_stress.py \
    --host mud.example.com \
    --port 4443 \
    --tls \
    --user StressBot \
    --password secretpass \
    --bots 10 \
    --duration 600
```

## In-Game Bot Commands

When a character has the `PLR_BOT` flag set (automatic for bot connections), special commands are available:

### `botreset`

Resets a bot character for fresh testing.

```
botreset           - Soft reset (restore HP/mana, clear affects)
botreset hard      - Hard reset (return to level 1, clear all progress)
```

**Hard reset effects:**
- Level set to 1
- All equipment removed and purged
- Experience reset to 0
- Stats reset to base values
- Skills reset to 0%
- Teleported to MUD School entrance (room 3757)
- 5 trains and 5 practices granted
- Starting gold (25) granted

### `botdata`

Displays structured room data for bot parsing:

```
botdata
```

Outputs `[BOT:ROOM|...]`, `[BOT:EXIT|...]`, `[BOT:MOB|...]`, `[BOT:OBJ|...]` lines.

This is also triggered automatically when bots send `look`.

## Bot Behavior System

Bots use a priority-based behavior system. Higher priority behaviors preempt lower ones.

### Behavior Priority Order (highest to lowest)

| Priority | Behavior | Description |
|----------|----------|-------------|
| 1000 | DeathRecovery | Handle death/ghost state |
| 900 | Survive | Emergency flee when HP critical |
| 850 | Recall | Emergency recall if flee fails |
| 800 | LightSource | Equip/remove lantern in dark rooms |
| 700 | Combat | Attack execution during combat |
| 650 | Heal | Rest to recover HP/mana/moves |
| 600 | BuySupplies | Purchase food/drink/lantern |
| 550 | FightDarkCreature | Dark creature quest |
| 530 | Loot | Loot corpses and equip items |
| 500 | Attack | Initiate combat with targets |
| 450 | ReturnToCage | Navigate back after displacement |
| 400 | Patrol | Circuit through cage rooms |
| 300 | BotrReset | Initial character reset |
| 250 | Train | Spend train points on stats |
| 200 | Practice | Spend practice points on skills |
| 150 | NavigateToCage | One-shot navigation to cages |
| 100 | Explore | Random movement (fallback) |
| 0 | Idle | Do nothing |

### Behavior Flow

1. **Startup**: BotrReset → Train → Practice → NavigateToCage
2. **Normal operation**: Patrol → Attack → Combat → Loot → (repeat)
3. **After patrol circuit**: BuySupplies → FightDarkCreature
4. **Emergency**: Survive → Recall → DeathRecovery → Heal

## Configuration

### Bot Defaults

Edit `tools/mud98bot/mud98bot/config/constants.py`:

```python
# Combat thresholds
DEFAULT_FLEE_HP_PERCENT = 25.0      # Flee when HP below this
MIN_ATTACK_HP_PERCENT = 40.0        # Don't attack if HP below this

# Training defaults  
DEFAULT_TRAIN_STATS = ['str', 'con', 'hp']
DEFAULT_PRACTICE_SKILLS = (('sword', 3), ('shield block', 2))

# Shopping
MIN_SHOPPING_MONEY = 10             # Minimum gold to shop
```

### Room Configuration

Edit `tools/mud98bot/mud98bot/config/rooms.py` to modify:

- Room VNUMs (cage rooms, shop, temple, etc.)
- Patrol routes
- Navigation waypoints

## Monitoring

### Log Output

Bots log to stderr with timestamps and bot IDs:

```
2025-12-28 14:07:48,936 [INFO] mud98bot.behaviors.combat: [Botrick] Attacking: monster wimpy lvl1 hp=100%
2025-12-28 14:08:21,951 [INFO] mud98bot.behaviors.combat: [Botrick] Combat ended - victory!
```

### Log Levels

- **INFO**: Normal operation (attacks, kills, navigation)
- **WARNING**: Recoverable issues (fleeing, stuck detection)
- **ERROR**: Serious problems (death, connection loss)
- **DEBUG**: Verbose debugging (every tick, MSDP updates)

### Saving Logs

```bash
python3 test_stress.py --user Bot --password pass --bots 5 --duration 300 2>&1 | tee test_run.log
```

### Metrics Summary

At test completion, a summary is displayed:

```
============================================================
                    TEST RESULTS SUMMARY
============================================================

Test Duration: 300.0 seconds
Bots Run: 5

Per-Bot Metrics:
  Bot1:
    Kills: 12, Deaths: 0, XP Gained: 1,450
  Bot2:
    Kills: 10, Deaths: 1, XP Gained: 980
  ...

Totals:
  Total Kills: 48
  Total Deaths: 2
  Total XP: 5,230
```

## Troubleshooting

### Bot Won't Connect

1. **Check server is running**: `netstat -tlnp | grep 4000`
2. **Check firewall**: Ensure port 4000 is accessible
3. **Check mud98.cfg**: Verify telnet is enabled

### Bot Stuck in Login

1. **Check password**: Must match existing character or be valid for new
2. **Check character exists**: For existing chars, verify player file exists
3. **Check name restrictions**: Some names may be reserved

### Bot Not Attacking

1. **Check HP threshold**: Bot won't attack below `MIN_ATTACK_HP_PERCENT`
2. **Check room**: Bot may be outside patrol area

### Bot Keeps Dying

1. **Reduce concurrent bots**: Monster respawn may not keep up
2. **Check flee threshold**: Increase `DEFAULT_FLEE_HP_PERCENT`
3. **Check training**: Ensure stats are being trained

### "Creating new instance" Messages

This is normal for MUD School (multi-instance area). Each bot gets their own copy. If you see this repeatedly for the same bot, they may be recalling out and losing their instance.

**Solution**: The server now has a 6-minute grace period before deleting empty instances.

### Dark Creature Quest Fails

The dark creature in room 3720 is tough. Common issues:

1. **Bot flees**: Normal if HP drops too low
2. **No creature found**: May have been killed by another bot, or instance was recreated
3. **Combat never starts**: Creature may not have respawned yet

The bot will retry 3 times before abandoning the quest and returning to patrol.

## Server Performance Notes

### Expected Load

- Each bot generates ~2-4 commands per second during active play
- Network traffic is minimal (telnet text)
- Most CPU usage is combat calculations

### Recommended Limits

| Server Specs | Recommended Max Bots |
|--------------|---------------------|
| 1 CPU, 1GB RAM | 10-20 bots |
| 2 CPU, 2GB RAM | 30-50 bots |
| 4+ CPU, 4GB+ RAM | 100+ bots |

### Monitoring Server

Watch for:
- High CPU usage in game loop
- Memory growth (potential leaks)
- Log spam from bot actions
- Network buffer backlogs

## Advanced Usage

### Custom Bot Behaviors

Create custom behaviors by extending the `Behavior` base class:

```python
from mud98bot.behaviors.base import Behavior, BehaviorContext, BehaviorResult

class MyCustomBehavior(Behavior):
    priority = 350  # Between Patrol and Attack
    name = "MyBehavior"
    
    def can_start(self, ctx: BehaviorContext) -> bool:
        # Return True when this behavior should activate
        return ctx.room_vnum == 1234
    
    def tick(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        # Execute one tick of behavior
        bot.send_command("say Hello!")
        return BehaviorResult.COMPLETED
```

### Programmatic Bot Control

```python
from mud98bot import Bot, BotConfig

config = BotConfig(
    host="localhost",
    port=4000,
    username="MyBot",
    password="secret"
)

bot = Bot(config)
await bot.connect()
await bot.login()

# Manual command
bot.send_command("look")

# Run behavior engine
from mud98bot.behaviors import create_default_engine
engine = create_default_engine(bot)

while True:
    engine.tick()
    await asyncio.sleep(0.5)
```

## Files Reference

| Path | Description |
|------|-------------|
| `tools/mud98bot/test_stress.py` | Main stress test runner |
| `tools/mud98bot/mud98bot/client.py` | Bot class and connection handling |
| `tools/mud98bot/mud98bot/coordinator.py` | Multi-bot management |
| `tools/mud98bot/mud98bot/behaviors/` | Behavior implementations |
| `tools/mud98bot/mud98bot/config/` | Configuration constants |
| `tools/mud98bot/docs/` | Additional documentation |

## See Also

- [Bot Design Document](mud98-bot-design.md) - Architecture and design decisions
- [Unit Test Guide](unit-test-guide.md) - Testing the MUD server
- [Engineering Notes](engineering-notes.md) - Server internals
