# Mud98 Stress Testing Bot — Design Document

## Overview

This document describes the design for **Mud98Bot**, an external stress testing tool that connects to Mud98 as a player client, performs realistic gameplay actions, and enables load testing and behavior validation.

## Goals

1. **Stress Testing**: Run multiple concurrent bot instances to simulate player load
2. **Behavior Validation**: Verify server responses to player actions work correctly
3. **Regression Testing**: Detect breakages in gameplay loops (fight, loot, train, etc.)
4. **Protocol Testing**: Validate telnet negotiation, MSDP/GMCP, MCCP2/3 handling
5. **Latency Measurement**: Measure server response times under various conditions

## Architecture

```
┌─────────────────┐     ┌─────────────────┐     ┌─────────────────┐
│   Mud98Bot 1    │     │   Mud98Bot 2    │     │   Mud98Bot N    │
│ (process/thread)│     │ (process/thread)│     │ (process/thread)│
└────────┬────────┘     └────────┬────────┘     └────────┬────────┘
         │                       │                       │
         │    TCP/TLS Socket     │                       │
         ▼                       ▼                       ▼
┌─────────────────────────────────────────────────────────────────┐
│                         Mud98 Server                            │
│                   (port 4000 telnet / 4043 TLS)                 │
└─────────────────────────────────────────────────────────────────┘
```

### Key Design Principles

- **External Process**: Runs completely outside the MUD, connecting via sockets like a real client
- **No Mud98 Code Dependencies**: Pure network client; can be written in any language
- **Stateful Session Management**: Maintains login state, character state, and world model
- **Scriptable Behaviors**: Actions defined via configuration or simple scripting
- **Protocol Aware**: Handles telnet negotiation, ANSI codes, MSDP/GMCP

## Implementation Language Options

| Language | Pros | Cons |
|----------|------|------|
| **Python** | Quick prototyping, excellent `telnetlib3`, async support | Slower for many instances |
| **C** | Native performance, reuse MTH protocol code | More development time |
| **Go** | Goroutines ideal for concurrent bots, single binary | New dependency |
| **Rust** | Memory safe, excellent async, `tokio` ecosystem | Steeper learning curve |

**Recommendation**: Python for initial prototype (fastest iteration), with optional C rewrite for high-volume stress testing if needed.

---

## Component Design

### 1. Connection Manager

Handles raw socket communication with Mud98.

```
ConnectionManager
├── connect(host, port, use_tls)
├── disconnect()
├── send(data: bytes)
├── recv() -> bytes
├── is_connected() -> bool
└── set_timeout(seconds)
```

**Responsibilities**:
- TCP socket management (with optional TLS/SSL)
- Buffered I/O with line-based and raw modes
- Connection health monitoring (keepalive via NOP)
- Reconnection logic with backoff

### 2. Telnet Protocol Handler

Processes telnet negotiation sequences (IAC commands).

```
TelnetHandler
├── process_input(raw_bytes) -> (clean_text, telopt_events)
├── negotiate_option(option, action)
├── send_subnegotiation(option, data)
├── supports_mccp2() -> bool
├── supports_msdp() -> bool
└── supports_gmcp() -> bool
```

**Telnet Options to Support**:

| Option | Code | Purpose |
|--------|------|---------|
| ECHO | 1 | Password hiding |
| SGA | 3 | Suppress Go-Ahead (character mode) |
| NAWS | 31 | Window size reporting |
| MSDP | 69 | MUD Server Data Protocol |
| GMCP | 201 | Generic MUD Communication Protocol |
| MCCP2 | 86 | Compression (optional) |
| EOR | 25 | End-of-record (prompt detection) |

**Negotiation Flow**:
```
Server: IAC WILL MSDP
Bot:    IAC DO MSDP
Server: IAC SB MSDP ... IAC SE
Bot:    (process MSDP data)
```

### 3. MSDP/GMCP Handler

Parses structured data from the server for bot decision-making.

```
MsdpHandler
├── parse_msdp(subneg_data) -> dict
├── request_variable(var_name)
├── subscribe_reports(var_list)
└── on_update(callback)
```

**Available MSDP Variables** (from Mud98):

| Variable | Description |
|----------|-------------|
| `HEALTH` / `HEALTH_MAX` | Current and max HP |
| `MANA` / `MANA_MAX` | Current and max mana |
| `MOVEMENT` / `MOVEMENT_MAX` | Current and max moves |
| `LEVEL` | Character level |
| `EXPERIENCE` / `EXPERIENCE_MAX` | XP progress |
| `ALIGNMENT` | Good/neutral/evil alignment |
| `MONEY` | Current wealth |
| `ROOM` | Room information |
| `ROOM_EXITS` | Available exits |

The bot should subscribe to reportable variables at login:
```
MSDP: REPORT HEALTH MANA MOVEMENT ROOM_EXITS
```

### 4. Text Parser

Parses server output to extract game state information.

```
TextParser
├── strip_ansi(text) -> clean_text
├── detect_prompt(text) -> PromptInfo
├── parse_room(text) -> RoomInfo
├── parse_combat_round(text) -> CombatInfo
├── parse_inventory(text) -> List[Item]
├── parse_score(text) -> CharacterStats
└── match_pattern(text, pattern) -> Optional[Match]
```

**Pattern Recognition Targets**:

| Pattern | Purpose |
|---------|---------|
| `<###hp ###m ###mv>` | Prompt (HP/mana/move) |
| `You are in ...` | Room description |
| `Exits: [N S E W]` | Available exits |
| `Your damage X ...` | Combat hit/miss |
| `X is DEAD!!` | Kill confirmation |
| `You gain ### experience` | XP gain |
| `You get X from ...` | Loot pickup |
| `You are carrying:` | Inventory list |
| `You could not find ...` | Item not found |

### 5. State Machine

Tracks bot's current state and valid transitions.

```
┌─────────────┐
│ DISCONNECTED │
└──────┬──────┘
       │ connect()
       ▼
┌─────────────┐
│   LOGIN     │◄────────────────┐
└──────┬──────┘                 │
       │ authenticate()         │
       ▼                        │
┌─────────────┐                 │
│   MOTD      │                 │
└──────┬──────┘                 │
       │ press enter            │
       ▼                        │
┌─────────────┐   death/quit    │
│   PLAYING   │─────────────────┘
└──────┬──────┘
       │
   ┌───┴───┬──────────┬──────────┐
   ▼       ▼          ▼          ▼
┌─────┐ ┌──────┐ ┌─────────┐ ┌───────┐
│ IDLE│ │COMBAT│ │EXPLORING│ │RESTING│
└─────┘ └──────┘ └─────────┘ └───────┘
```

### 6. World Model

Maintains bot's understanding of the game world.

```
WorldModel
├── current_room: Room
├── known_rooms: Dict[vnum, Room]
├── inventory: List[Item]
├── equipment: Dict[WearLoc, Item]
├── character: CharacterStats
└── combat_target: Optional[Mobile]

Room
├── name: str
├── description: str
├── exits: Dict[Direction, Exit]
├── mobiles: List[Mobile]
└── objects: List[Object]
```

### 7. Behavior Engine

Executes high-level gameplay behaviors.

```
BehaviorEngine
├── behaviors: List[Behavior]
├── current_behavior: Behavior
├── tick() -> List[Command]
└── interrupt(new_behavior)

Behavior (abstract)
├── priority: int
├── can_start(world_model) -> bool
├── execute(world_model) -> Generator[Command]
└── is_complete() -> bool
```

**Core Behaviors**:

| Behavior | Priority | Description |
|----------|----------|-------------|
| `SurviveBehavior` | 100 | Flee/recall if HP critical |
| `CombatBehavior` | 80 | Fight mobs, use skills |
| `HealBehavior` | 70 | Rest/sleep to regen, quaff potions |
| `LootBehavior` | 60 | Pick up corpse contents, gold |
| `EquipBehavior` | 50 | Compare/wear better gear |
| `ExploreBehavior` | 40 | Move through areas, map rooms |
| `TrainBehavior` | 30 | Visit trainer, spend trains |
| `ShopBehavior` | 20 | Buy supplies (food, potions) |
| `IdleBehavior` | 10 | Random socials, stand around |

### 8. Command Queue

Manages command pacing and wait states.

```
CommandQueue
├── queue: Deque[Command]
├── last_sent: timestamp
├── wait_for_prompt: bool
├── min_delay_ms: int  # throttle
├── enqueue(cmd)
├── process()
└── clear()
```

**Throttling**: Mud98 has built-in command lag (`ch->wait`). The bot should:
1. Wait for prompt before sending next command
2. Respect minimum delay between commands (avoid spam detection)
3. Handle "You can't do that yet" gracefully

---

## Login Sequence

```python
# Pseudocode for login flow
async def login(bot, username, password):
    await bot.connect()
    
    # Wait for "By what name" prompt
    await bot.wait_for("By what name")
    await bot.send(username)
    
    # Handle new vs existing character
    response = await bot.read_until_prompt()
    
    if "Password:" in response:
        # Existing character
        await bot.send(password)
    elif "Did I get that right" in response:
        # New character - run character creation
        await bot.create_character(...)
    
    # Wait for MOTD
    await bot.wait_for("[Press Enter to continue]")
    await bot.send("")
    
    # Now in game
    bot.state = BotState.PLAYING
    
    # Subscribe to MSDP updates
    await bot.msdp.subscribe([
        "HEALTH", "HEALTH_MAX", 
        "MANA", "MANA_MAX",
        "ROOM_EXITS"
    ])
```

---

## Combat Loop Example

```python
class CombatBehavior(Behavior):
    priority = 80
    
    def can_start(self, world):
        # Start if there are attackable mobs in room
        return any(mob.is_attackable for mob in world.current_room.mobiles)
    
    async def execute(self, world):
        while True:
            # Check health - flee if critical
            if world.character.hp_percent < 20:
                yield Command("flee")
                return
            
            # Check if we're fighting
            if world.combat_target:
                # Use skill if available
                if world.character.can_use("bash"):
                    yield Command("bash")
                else:
                    yield Command("")  # auto-attack
                await sleep(1.0)  # wait for combat round
            else:
                # Find next target
                target = self.find_target(world)
                if target:
                    yield Command(f"kill {target.keyword}")
                else:
                    return  # No more targets
    
    def find_target(self, world):
        for mob in world.current_room.mobiles:
            if mob.is_attackable and mob.level <= world.character.level + 2:
                return mob
        return None
```

---

## Configuration Format

```yaml
# mud98bot.yaml

connection:
  host: localhost
  port: 4000
  use_tls: false
  
credentials:
  username: TestBot1
  password: secret123
  
# For character creation if new
new_character:
  race: human
  class: warrior
  sex: male
  alignment: neutral
  
behaviors:
  explore:
    enabled: true
    avoid_rooms: [3001, 3002]  # Temple, pit
    max_level_diff: 3
    
  combat:
    enabled: true
    flee_hp_percent: 20
    use_skills: [kick, bash]
    
  loot:
    enabled: true
    auto_sacrifice: true  # sacrifice worthless items
    
  heal:
    rest_hp_percent: 50
    sleep_hp_percent: 30
    
throttle:
  min_command_delay_ms: 250
  max_commands_per_second: 4
  
logging:
  level: INFO
  file: bot1.log
```

---

## Multi-Bot Orchestration

For stress testing, run multiple bots from a coordinator:

```
┌──────────────────────────────────────────┐
│           Bot Coordinator                │
│  ┌────────────────────────────────────┐  │
│  │  Config: 50 bots, port 4000        │  │
│  │  Metrics: connections, latency     │  │
│  └────────────────────────────────────┘  │
└──────────────────┬───────────────────────┘
                   │
    ┌──────────────┼──────────────┐
    ▼              ▼              ▼
┌───────┐     ┌───────┐     ┌───────┐
│ Bot 1 │     │ Bot 2 │ ... │Bot 50 │
│ user1 │     │ user2 │     │user50 │
└───────┘     └───────┘     └───────┘
```

**Coordinator Features**:
- Staggered bot startup (avoid connection storm)
- Aggregate metrics collection
- Central logging
- Graceful shutdown handling
- Dynamic bot count adjustment

---

## Metrics & Reporting

Track these metrics for stress testing analysis:

| Metric | Description |
|--------|-------------|
| `connections_active` | Current connected bot count |
| `connections_total` | Total connection attempts |
| `connections_failed` | Failed connections |
| `commands_sent` | Total commands sent |
| `responses_received` | Total server responses |
| `latency_avg_ms` | Average command-to-response time |
| `latency_p99_ms` | 99th percentile latency |
| `bytes_sent` | Network bytes uploaded |
| `bytes_received` | Network bytes downloaded |
| `mobs_killed` | Mobs killed across all bots |
| `deaths` | Bot character deaths |
| `errors` | Parse errors, unexpected states |

**Output Formats**:
- Real-time console dashboard
- JSON lines for log aggregation
- Prometheus metrics endpoint (optional)
- Summary report at end of test run

---

## Directory Structure

```
mud98bot/
├── README.md
├── requirements.txt          # Python dependencies
├── setup.py
├── mud98bot/
│   ├── __init__.py
│   ├── main.py               # Entry point
│   ├── connection.py         # Socket/TLS handling
│   ├── telnet.py             # Telnet protocol
│   ├── msdp.py               # MSDP/GMCP parsing
│   ├── parser.py             # Text parsing
│   ├── state.py              # State machine
│   ├── world.py              # World model
│   ├── behaviors/
│   │   ├── __init__.py
│   │   ├── base.py           # Behavior base class
│   │   ├── combat.py
│   │   ├── explore.py
│   │   ├── heal.py
│   │   ├── loot.py
│   │   ├── train.py
│   │   └── shop.py
│   ├── coordinator.py        # Multi-bot orchestration
│   └── metrics.py            # Stats collection
├── configs/
│   ├── stress_test.yaml      # 50-bot stress test
│   ├── single_bot.yaml       # Single bot for debugging
│   └── combat_test.yaml      # Combat-focused test
└── tests/
    ├── test_telnet.py
    ├── test_parser.py
    └── test_behaviors.py
```

---

## Implementation Phases

### Phase 1: Basic Connectivity (MVP) [COMPLETE]
- [x] TCP socket connection
- [x] Basic telnet negotiation (ECHO, SGA)
- [x] Login flow (existing character)
- [x] Simple command send/receive
- [x] Prompt detection

### Phase 2: Protocol Support [COMPLETE]
- [x] Full telnet option handling
- [x] MSDP parsing and subscription
- [x] GMCP support (MSDP over GMCP)
- [x] ANSI stripping
- [ ] Optional MCCP2 compression (declined for simplicity)

### Phase 3: Game Awareness [COMPLETE]
- [x] Text parser for room/combat output
- [x] World model (rooms, inventory, stats)
- [x] State machine

### Phase 4: Behaviors [COMPLETE]
- [x] Behavior engine framework
- [x] Combat behavior
- [x] Explore behavior  
- [x] Heal/rest behavior
- [x] Loot/sacrifice behavior
- [x] Survive/flee behavior
- [x] Recall behavior
- [x] BOT protocol integration (level-aware targeting, VNUM-based navigation)

### Phase 5: Stress Testing [COMPLETE]
- [x] Multi-bot coordinator (`coordinator.py`)
- [x] Metrics collection (`metrics.py`)
- [x] Configurable test scenarios (YAML configs)
- [x] Console reporting/summary
- [x] Stress test script (`test_stress.py`)

### Phase 6: Advanced Features [COMPLETE]
- [x] Character creation flow (basic)
- [x] TLS connection support
- [x] PLR_BOT structured output protocol
- [x] Training/practice behavior
- [x] Shop interactions
- [x] Group formation (bots in parties)
- [x] Equipment comparison/upgrade behavior

---

## Server-Side Enhancements (IMPLEMENTED)

### MSDP Variables

The following MSDP variables were added to Mud98 to support bot automation:

| Variable | Description |
|----------|-------------|
| `IN_COMBAT` | 1 if player is fighting, 0 otherwise |
| `OPPONENT_NAME` | Name/short_descr of combat target |
| `OPPONENT_LEVEL` | Level of combat target |
| `OPPONENT_HEALTH` | Current HP of combat target |
| `OPPONENT_HEALTH_MAX` | Max HP of combat target |
| `ROOM_VNUM` | Virtual number of current room |

These are updated automatically in `update_msdp_vars()` in [src/update.c](../../src/update.c).

### PLR_BOT Flag and Structured Output

A dedicated `PLR_BOT` player flag enables machine-readable structured output for bots.

**Flag Definition** (in [src/data/player.h](../../src/data/player.h)):
```c
#define PLR_BOT         BIT(9)    // Bot client - structured output
```

**Macro** (in [src/entities/mobile.h](../../src/entities/mobile.h)):
```c
#define IS_BOT(ch)      (IS_SET((ch)->act_flags, PLR_BOT))
```

**Enable via**: `flag self bot` (immortal command) or set in player file `actFlags`.

**BOT Protocol Format**:

When PLR_BOT is set, the `look` command appends structured data after normal room output:

```
[BOT:ROOM|vnum=3716|flags=indoors newbies_only|sector=inside]
[BOT:EXIT|dir=west|vnum=3712|flags=(none)]
[BOT:EXIT|dir=north|vnum=3703|flags=(none)]
[BOT:MOB|name=monster|vnum=3702|level=1|flags=npc sentinel noalign|hp=100%|align=0]
[BOT:OBJ|name=sword|vnum=3022|type=weapon|flags=take|wear=wield]
```

**Field Reference**:

| Line Type | Fields |
|-----------|--------|
| `BOT:ROOM` | `vnum`, `flags` (room flags), `sector` (terrain type) |
| `BOT:EXIT` | `dir` (direction), `vnum` (destination room), `flags` (exit flags) |
| `BOT:MOB` | `name`, `vnum`, `level`, `flags` (act flags), `hp` (percent), `align` |
| `BOT:OBJ` | `name`, `vnum`, `type` (item type), `flags` (extra flags), `wear` (wear locations) |

**Advantages over MSDP**:
- Synchronous with `look` command (no timing/race issues)
- Complete room state in one response
- Exit VNUMs enable smart navigation
- Mob levels/flags enable safe target selection

**Python Parser** (in `tools/mud98bot/mud98bot/parser.py`):
```python
parser = TextParser()
if parser.has_bot_data(text):
    data = parser.parse_bot_data(text)
    # data.room, data.exits, data.mobs, data.objects
```

---

## Server-Side Considerations

To support stress testing, consider adding to Mud98:

1. **PLR_TESTER flag** (already exists as `PLR_TESTER` / `IS_TESTER()` macro):
   - Could grant special permissions for test characters
   - Fast leveling, instant recall, etc.

2. **Bot-friendly MSDP extensions**:
   ```c
   { "ATTACKABLE_MOBS",  MSDP_FLAG_SENDABLE | MSDP_FLAG_REPORTABLE, NULL },
   { "ROOM_VNUM",        MSDP_FLAG_SENDABLE | MSDP_FLAG_REPORTABLE, NULL },
   { "COMBAT_TARGET",    MSDP_FLAG_SENDABLE | MSDP_FLAG_REPORTABLE, NULL },
   ```

3. **Test command** (imm-only):
   ```
   testspawn <n> - Spawn N test bots instantly (internal, not via network)
   ```

4. **Rate limiting awareness**:
   - Ensure spam detection doesn't block legitimate bot actions
   - Consider whitelist for known bot IPs during testing

---

## Example Usage

```bash
# Single bot for development
python -m mud98bot --config configs/single_bot.yaml

# 50-bot stress test
python -m mud98bot --config configs/stress_test.yaml --bots 50

# Combat test with metrics
python -m mud98bot --config configs/combat_test.yaml --metrics-port 9090
```

---

## Risks & Mitigations

| Risk | Mitigation |
|------|------------|
| Bot overwhelms server | Configurable throttling, staggered startup |
| Character file corruption | Use test-only accounts, regular backups |
| Network issues cause hanging | Timeouts, reconnection logic |
| Parser breaks on changed output | Flexible pattern matching, MSDP preference |
| Memory leaks in long tests | Periodic bot restart, monitoring |

---

## References

- [Mud98 MTH Protocol Implementation](../src/mth/)
- [MSDP Specification](https://tintin.mudhalla.net/protocols/msdp/)
- [GMCP Specification](https://tintin.mudhalla.net/protocols/gmcp/)
- [Telnet RFC 854](https://datatracker.ietf.org/doc/html/rfc854)
- [mud98.cfg](../../mud98.cfg) - Server configuration including protocol toggles

---

## Appendix A: Telnet Negotiation Quick Reference

```
IAC = 255   Interpret As Command
WILL = 251  Sender will do option
WONT = 252  Sender won't do option  
DO = 253    Sender wants receiver to do option
DONT = 254  Sender wants receiver to not do option
SB = 250    Subnegotiation Begin
SE = 240    Subnegotiation End
```

**Common sequences**:
```
Server: IAC WILL ECHO        (I will handle echo)
Bot:    IAC DO ECHO          (Please do)

Server: IAC WILL MSDP        (I support MSDP)
Bot:    IAC DO MSDP          (I want MSDP)

Bot:    IAC SB MSDP REPORT HEALTH IAC SE   (Subscribe to HEALTH)
Server: IAC SB MSDP VAR HEALTH VAL 100 IAC SE
```

## Appendix B: Sample Parsed Output

**Room parsing**:
```
Input:
  Temple Square
  You are in the main square of Midgaard. The road leads north, south, east, and west.
  [Exits: north east south west up]
  A janitor is sweeping the floor.

Parsed:
  Room(
    name="Temple Square",
    exits=[N, E, S, W, U],
    mobiles=[Mobile(keyword="janitor", short="A janitor")]
  )
```

**Combat parsing**:
```
Input:
  Your slash EVISCERATES the cityguard!
  The cityguard's punch scratches you.
  
Parsed:
  CombatRound(
    your_damage=DamageInfo(verb="EVISCERATES", target="cityguard", damage_tier=HIGH),
    enemy_damage=DamageInfo(verb="scratches", target="you", damage_tier=LOW)
  )
```
