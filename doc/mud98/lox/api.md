# Mud98 Lox API Reference

This reference catalogs every native function, entity method, global constant, and command surface exposed to Lox scripts. Entries cite the authoritative C implementation so you can cross-check behavior or update the docs when code changes.

## Table of Contents
1. [Globals & Enums](#globals--enums)
2. [Native Functions](#native-functions)
3. [Entity Methods](#entity-methods)
4. [Native Commands](#native-commands)
5. [Collections & Iteration](#collections--iteration)

---

## Globals & Enums
- **Global entity lists** (`src/lox/native.c:80-85`):
  - `global_areas`, `global_objs`, `global_mobs`: iterable containers of live area/obj/mob instances. Support `each(callback)`, `count`, `[]` (implemented via Lox list/array interfaces).
- **Enums** (`src/lox/native.c:65-78`, `src/data/*.c`):
  - Automatically generated from game data: `Race`, `DamageType`, `FactionStanding`, etc.
  - Example usage: `if (mob.race == Race.Elf) { ... }`.
  - Verified via tests (`src/tests/lox_ext_tests.c:187-215`).

## Native Functions
Source: `src/lox/native_funcs.c`. All functions throw `runtime_error()` if parameter counts/types are wrong, so validate inputs before calling.

| Name | Signature | Description / Notes |
| ---- | --------- | ------------------- |
| `clock()` | `clock()` → double | Returns elapsed CPU seconds (`clock()/CLOCKS_PER_SEC`). Handy for profiling expensive scripts. |
| `marshal()` | `marshal(rawPtr)` → value | Converts a `RAW_PTR` into a first-class Lox value (string/int/object). Used internally for C interop (`src/tests/lox_ext_tests.c:65-114`). |
| `floor()` | `floor(value)` → int | Pass-through for integers; converts doubles by truncating toward negative infinity. |
| `string()` | `string(value)` → string | Produces a printable string (same formatter used by `print`). Use to capture descriptions or combine with `send()`. |
| `damage()` | `damage(ch, victim, amount, dt, damType, show)` → bool | Wraps the combat pipeline. `ch` or `victim` may be `nil`; `amount`/`dt`/`damType` are ints; `show` toggles combat messaging. Returns `false` if the call was invalid or the victim dodged due to standard combat logic. |
| `dice()` | `dice(number, size)` → int | Standard ROM dice roller; mirrors `dice()` macro used across the C codebase. |
| `do()` | `do(commandString)` → bool | Executes a player command (e.g., `do("say Hello")`). Requires `exec_context.me` to be a valid mobile (usually event callers like `on_greet`). Returns `true` on success. |
| `saves_spell()` | `saves_spell(level, victim, damType)` → bool | Tests a victim’s saving throw vs. the provided dam type. Useful for scripted spell effects. |
| `delay()` | `delay(ticks, closure)` → bool | Schedules a closure to run later (see Runtime guide). Prevents the event loop from blocking on long-running logic. |

> Tip: wrap risky native calls in guard functions (e.g., `function safe_damage(ch, v, amt) { if (!ch.is_mob()) return false; return damage(ch, v, amt, 0, DamageType.Slash, true); }`) so builders avoid common pitfalls.

## Entity Methods
Source: `src/lox/native_methods.c`, `src/entities/entity.c`, `src/data/quest.c`, `src/entities/faction.c`.

### Type Checkers
| Method | Applies To | Description |
| ------ | ---------- | ----------- |
| `is_area()` / `is_area_data()` | Any entity | Returns `true` if receiver is area or area data (`OBJ_AREA`, `OBJ_AREA_DATA`). |
| `is_room()` / `is_room_data()` | | Same for rooms. |
| `is_mob()` / `is_mob_proto()` | | Same for mobiles. |
| `is_obj()` / `is_obj_proto()` | | Same for objects. |

### Messaging & Interaction
| Method | Receiver | Arguments | Description |
| ------ | -------- | --------- | ----------- |
| `send(message, ...)` | Mob/player | `message` strings | Sends OOC/OOC-style text directly to players (skips NPCs). Accepts multiple fragments which are concatenated. |
| `say([target], message, ...)` | Any entity | Optional `target` entity + strings | Emits dialogue using the room’s `say` format (with color codes defined in `act_comm.h`). |
| `echo([target], message, ...)` | Any entity | Optional `target` entity + strings | Raw `act()` call for custom messaging; respects `$n/$N/$T` macros inside strings. |
| `do(command)` | Any entity | String | See native function `do()`—commonly invoked from scripts attached to mobiles/players. |

### Quest Helpers (`src/data/quest.c:738-812`)
| Method | Receiver | Description |
| ------ | -------- | ----------- |
| `can_quest(vnum)` | Mob/player | True if the character may start the quest. |
| `has_quest(vnum)` | | True if quest already on the log. |
| `grant_quest(vnum)` | | Adds quest to log and fires quest-start messaging. |
| `can_finish_quest(vnum)` | | True if completion requirements met. |
| `finish_quest(vnum)` | | Completes quest, grants rewards, updates reputation. |

### Faction Helpers (`src/entities/faction.c:643-853`)
| Method | Receiver | Description |
| ------ | -------- | ----------- |
| `get_reputation(faction)` | Mob/player | Returns current standing vs. faction (integer tiers). |
| `adjust_reputation(faction, delta)` | | Adds delta to standing, clamping per faction rules. |
| `set_reputation(faction, value)` | | Forces standing to a specific tier. |
| `is_enemy(faction)` / `is_ally(faction)` | | Boolean helpers for branching dialogue or combat logic. |

### Type & Safety Checks (`src/entities/entity.c:65-118`)
| Method | Receiver | Description |
| ------ | -------- | ----------- |
| `is_mob()`, `is_room()`, `is_obj()` | Any entity | Verify the runtime type before calling more specific APIs. |
| `is_mob_proto()`, `is_obj_proto()`, etc. | Any entity | Useful when scripting prototypes directly (OLC-level scripts). |

## Native Commands
Source: `src/lox/native_cmds.c`.

Mud98 exposes both player and mob command tables to scripts:
- `native_cmds`: player commands (`cmd_table`).
- `native_mob_cmds`: mob commands (`mob_cmd_table`).

Scripts access them through objects returned by `new_native_cmd(cmd)` (internal). Typical usage pattern:
```lox
// Have the acting character execute a command
do("say Hello world")
```
If you need direct command objects (e.g., to expose a curated subset to builders), fetch them from the global tables and store them on your entity:
```lox
var emote_cmd = native_cmds["emote"]
emote_cmd.call(this, "cheers loudly!") // advanced use; requires understanding DoFunc signatures
```
Most builders should stick with `do("command args")` for safety.

## Collections & Iteration
- **Arrays / Tables / Lists** (including those from `global_*`) support `.each(callback)` as shown in `test_lamdas`:
  ```lox
  var mob_count = 0
  global_mobs.each((mob) -> {
      mob_count++
  })
  ```
  - Arrays pass `(index, value)`, tables pass `(key, value)`, lists pass each node’s `value`.
- Entities also expose their `fields` table to scripts via `this.fieldName`. Use `set_name()` macros in C to surface additional metadata if necessary; from Lox you can assign new fields (`this.helpedPlayers = []`) to persist data on instances.

---

Refer back to the [Language Guide](language.md) for syntax and idioms or to the [Runtime Guide](runtime.md) for editor/event details. The [Recipes](recipes.md) provide end-to-end scenarios using these APIs in context.
