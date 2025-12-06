# Mud98 Lox Runtime Guide

This guide explains how Lox scripts live inside Mud98: how they’re stored, compiled, attached to entities, triggered by events, and executed at runtime. It consolidates builder workflow instructions (see `doc/mud98/wb-02-new-beginnings.md`) and engine internals (e.g., `src/entities`, `src/lox`, `src/data/events`).

## Table of Contents
1. [Script Lifecycle](#script-lifecycle)
2. [Entity Classes & Naming](#entity-classes--naming)
3. [OLC Script Editor](#olc-script-editor)
4. [Events & Triggers](#events--triggers)
5. [Execution Context](#execution-context)
6. [Globals & Persistence](#globals--persistence)
7. [Delayed Execution & Timers](#delayed-execution--timers)
8. [Testing & Troubleshooting](#testing--troubleshooting)

---

## Script Lifecycle
1. **Author**: Enter `lox` while editing an entity (room/mob/object) via OLC.
2. **Compile/Validate**: Use `.v` to compile in-memory; errors reference file/line inside the script.
3. **Assign**: Use `@` to compile and attach bytecode to the entity prototype (message: `Class "room_<vnum>" ... assigned.`).
4. **Bind Events**: `event set <trigger>` to wire callbacks declared in the script (e.g., `on_login`, `on_greet`).
5. **Persist**: `asave <scope>` writes JSON/ROM data including `loxScript` and `events`.

Instance creation (e.g., room resets) automatically instantiates the compiled class, so new copies inherit the script without further action.

## Entity Classes & Naming
- Entities (`RoomData`, `MobPrototype`, `ObjPrototype`, `AreaData`) embed an `Entity` header (see `src/entities/entity.h`).
- When you save a script, Mud98 wraps it in a generated class named `<type>_<vnum>` (`room_12000`, `mob_12005`, etc.).
- The runtime uses `create_entity_class()` (`src/entities/entity.c:23-64`) to compile the class code, store it on the entity (`ent->klass`), and attach the source string.
- Methods you define correspond to event callbacks or utility helpers; there is no need to manually declare `class` blocks inside OLC scripts.

## OLC Script Editor
Commands available on a blank line inside the editor:
- `.s` — show current script (with syntax highlighting in-game).
- `.v` — validate compilation (no assignment).
- `@` — compile and assign script to the entity.
- `.x` — cancel edits, revert to stored version.
- `.clear` — wipe current buffer.
- `.li/.ld/.lr` — insert/delete/replace specific lines.
- `.r` — search/replace substring.

Reference: tutorial section at `doc/mud98/wb-02-new-beginnings.md:551`.

## Events & Triggers
- Event metadata lives in `src/data/events.c` and `src/entities/event.h`.
- Each event entry specifies:
  - `trigger` flag (e.g., `TRIG_LOGIN`, `TRIG_GREET`).
  - `name` string used in OLC commands.
  - Default callback name (`on_login`, `on_greet`, etc.).
  - Valid entity types (rooms, mobs, objs).
  - Optional `criteria` (integer or string).
- Use `event set <name>` inside OLC to add or edit events; e.g., `event set greet` attaches `on_greet`.
- JSON persistence stores scripts under `loxScript` and events under `events[]` (`doc/mud98/json/area-json-schema.md:140`).
- Runtime dispatch:
  1. Engine raises triggers via `raise_*` helpers (see `src/entities/event.h:59-78`).
  2. Event lookup uses `get_event_by_trigger()`; if a callback exists, Lox invokes it with appropriate arguments (victim, room, object, etc.).
  3. Criteria filter events (percentage chance, speech keywords, etc.).
- Common triggers (`src/data/events.c`):

| Trigger | Default Callback | Valid Entities | Criteria |
| ------- | ---------------- | -------------- | -------- |
| `act` | `on_act(actor, message)` | Rooms, mobs, objs | Speech text filter (`string`) |
| `attacked` | `on_attacked(attacker, pct)` | Mobs | Percent chance (int) |
| `bribe` | `on_bribe(ch, amount)` | Mobs | Minimum amount (int) |
| `death` | `on_death(killer)` | Mobs | None |
| `entry` | `on_entry(ch, pct)` | Mobs | Percent chance |
| `fight` | `on_fight(target, pct)` | Mobs | Percent chance |
| `give` | `on_give(ch, data)` | Mobs | Item vnum or keyword (string/int) |
| `greet` / `grall` | `on_greet(ch)` | Rooms, mobs, objs | None |
| `hpcnt` | `on_hpcnt(attacker)` | Mobs | Percent threshold (int) |
| `random` | `on_random(pct)` | Mobs | Percent chance |
| `speech` | `on_speech(ch, message)` | Mobs | Keyword filter (`string`) |
| `exit` / `exall` | `on_exit(ch, dir)` | Mobs | Direction or percent |
| `delay` | `on_delay(ticks)` | Mobs | Delay ticks (int) |
| `surr` | `on_surr(ch, pct)` | Mobs | Percent chance |
| `login` | `on_login(ch)` | Rooms | None |
| `given` / `taken` / `dropped` | `on_given`, `on_taken`, `on_dropped` | Objects | None |

> Full list (including `exit`, `speech`, `sur`, etc.) lives in `src/data/events.c`; extend this table as new triggers are added.

**Criteria reference**
- `CRIT_NONE`: omit criteria entirely; `event set greet` is enough.
- `CRIT_INT`: integer literal (`event set random 50`) interpreted as percent chance or threshold.
- `CRIT_STR`: quoted string (`event set act "hello"`) used as a keyword filter.
- `CRIT_INT_OR_STR`: accepts either form (e.g., `give` event can match item VNUM or keyword).

In OLC, `event set <trigger> [criteria]` enforces these rules; invalid types are rejected before the script fires. JSON serialization stores `criteria` as either `criteria` (string) or `criteriaValue` (integer), mirroring the type table.

Common triggers used in the tutorial:
- `login` (`TRIG_LOGIN`): fires before the player sees the room description.
- `greet`: fires when a character enters a room or approaches a mob.
- `random`, `fight`, `entry`, etc., are available for richer logic.

## Execution Context
- **`this` / `this_`**: Inside entity scripts, `this` refers to the current entity instance (room/mob/object). The engine sets `compile_context.this_` during compilation.
- **`exec_context.me`**: When scripts run due to player actions, this pointer references the acting mobile (player or NPC). Some native functions (`do()`) require it (see `src/lox/native_funcs.c:121-143`).
- **Arguments**: Event callbacks receive arguments documented per trigger (e.g., `on_login(vch)`, `on_greet(vch)`).
- **Return values**: Ignored unless the native caller expects a boolean (rare). Use returns primarily for early exits.

## Globals & Persistence
- Global tables injected during world load (`src/lox/native.c:80-85`):
  - `global_areas`, `global_objs`, `global_mobs`: iterable collections of live instances.
  - Enum classes (`Race`, `DamageType`, etc.) populated by `init_const_natives()`.
- Scripts persist through area saves:
  - ROM files: stored as `L` sections via `fix_lox_script()` (`src/olc/olc_save.c`).
  - JSON: `loxScript` string plus `events` array (see schema).
  - On load, Mud98 re-compiles each script, recreates entity classes, and registers events.

## Delayed Execution & Timers
- Use `delay(ticks, closure)` to schedule work after a number of pulses (tick = 8 seconds).
- Implementation: `delay()` (native function) captures a closure and registers an `EventTimer` (`src/lox/native_funcs.c:173-186`, `src/entities/event.h:28-33`).
- Typical use: send follow-up instructions after room text (`doc/mud98/wb-02-new-beginnings.md:763`).

## Testing & Troubleshooting
- **In-game**: Use `.v` to catch compile errors early; the VM prints stack traces (function names + line numbers).
- **Unit tests**: `src/tests/lox_tests.c` and `src/tests/lox_ext_tests.c` cover arithmetic, scoping, lambdas, strings, enums, etc. Reproduce failing behavior there to verify fixes.
- **Instrumentation flags** (`src/tests/lox_tests.c`):
  - `test_disassemble_on_error`, `test_trace_exec`, etc., can be toggled in test harnesses.
- **Runtime errors**: `runtime_error()` logs to player/immortal console; ensure guard checks (e.g., `if (!IS_MOBILE(args[0]))`) to emit friendly diagnostics.

Next steps: explore the [API Reference](api.md) for callable natives or the [Recipes](recipes.md) section for hands-on scripting patterns.
