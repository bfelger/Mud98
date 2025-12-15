# Mud98 AI Coding Agent Instructions

Mud98 is a modernized Rivers of MUD (ROM) 2.4b6 codebase — a classic text-based MUD server updated to compile cleanly on modern C compilers (C23/C17). It's a multi-user dungeon game engine with networking, scripting, persistence, and live editing (OLC).

## Architecture Overview

**Core domains** live under `src/`:
- **`entities/`** — Game objects: `Mobile` (player/NPC), `Object` (items), `Room`, `Area`, `Descriptor` (network connections)
- **`data/`** — Templates and metadata: `Class`, `Race`, `Skill`, `Quest`, `Social`, `Tutorial`
- **`persist/`** — Pluggable I/O layer supporting both legacy ROM `.are` text format and JSON (via Jansson)
- **`lox/`** — Embedded Lox scripting VM (bytecode-interpreted language for in-game automation)
- **`mth/`** — MUD Telnet Handler (GMCP, MCCP, MSDP, MSSP protocols)
- **`tests/`** — Unit test suite; run via `./src/run unittest`
- **`benchmarks/`** — Performance benchmarks; run via `./src/run bench`

**Commands** are registered in `cmd_table` (see `command.h`, `interp.c`). Each has signature `DECLARE_DO_FUN(do_foo)` which expands to `void do_foo(Mobile* ch, char* argument)`. Command definitions live in `act_*.c` files; the command table is loaded from `data/commands.olc` or `data/commands.json` at runtime.

**Prototypes vs. instances**: Mobs and objects are spawned from prototypes (`MobPrototype`, `ObjPrototype`) at runtime. Areas can be single-instance or multi-instance (per-player sandboxes).

## Build & Run

- **Configure**: `cmake --preset ninja-multi` (or use legacy `src/config`)
- **Build**: `cmake --build --preset debug` (or `src/compile`)
- **Binaries**: Land in `bin/<Config>/Mud98`
- **Run**: From repo root (so `mud98.cfg`, `area/`, `data/` resolve): `./bin/Debug/Mud98`
- **Quick helper**: `./src/run [debug|release|relwithdebinfo] [bench|unittest]`

**Runtime config**: `mud98.cfg` at repo root (TLS/telnet ports, GMCP/MCCP toggles, paths, gameplay settings).

## Coding Standards (from `doc/mud98/coding-guide.md`)

- **Warnings as errors**: GCC/Clang use `-Wall -Werror -Wextra -pedantic`; MSVC uses `/Wall /WX`
- **C standards**: Targets GNU2x/Clang C2x; MSVC uses `/std:c17`
- **Style**: 
  - Small, focused functions; prefer `static` at file scope
  - `const` for inputs; `bool` for single-toggles; size-appropriate unsigned types for counts
  - Header includes use project-relative paths: `#include <entities/mobile.h>`
- **Naming**: 
  - Types use `UpperCamelCase` + `_t` suffix (e.g., `MobileData_t` typedef'd to `MobileData`)
  - Functions/variables use `snake_case`
  - Macros/constants use `SCREAMING_SNAKE_CASE`
- **Error handling**: Check all allocations and file I/O; return early on failure
- **Threading**: Non-Windows builds use Pthreads for networking; guard shared data (see `comm.c`, `entities/`)
- **Add new sources**: Update `src/CMakeLists.txt` executable list

## Persistence Layer (`src/persist/`)

**Pattern**: Pluggable format backends via `persist_io.h` abstractions.

- **`persist_io.h`**: Defines `PersistReader`/`PersistWriter` with ops tables (getc, putc, flush)
- **`persist_io_adapters.h`**: Provides FILE*, in-memory buffer adapters
- **`area_persist.h`** (and similar for class, race, skill, etc.): Domain-specific params + format registry
- **Format implementations**:
  - `rom-olc/` — Legacy ROM text format (e.g., `area_persist_rom_olc.c`)
  - `json/` — JSON format via Jansson (e.g., `area_persist_json.c`)

**JSON schemas**: See `doc/mud98/json/*.md` for authoritative format docs.

**Format selection**: Automatic via file extension—`.olc` or `.are` for ROM-OLC format, `.json` for JSON. The persist layer checks extensions at load/save time (see `*_persist.c` files for extension checks like `if (ext && !str_cmp(ext, ".json"))`).

**File locations**: `data/` for classes, races, skills, socials, commands, tutorials, themes; `area/` for world areas.

## Online Creation (OLC) (`src/olc/`)

**Mode-based editing**: OLC commands (`aedit`, `redit`, `medit`, `oedit`, etc.) enter "OLC mode" with a preemptive command parser distinct from the main command table. Each editor type (`ED_AREA`, `ED_ROOM`, `ED_MOBILE`, `ED_OBJECT`, etc.) has its own subcommand set.

**Key editors**:
- `aedit` — Areas
- `cedit` — Classes
- `cmdedit` — Commands
- `gedit` — Skill groups
- `hedit` — Help files
- `redit` — Rooms
- `medit` — Mobiles (NPCs)
- `oedit` — Objects
- `qedit` — Quests
- `raedit` — Races
- `scredit` — Scripts (Lox)
- `sedit` — Socials
- `skedit` — Skills
- `tedit` — Tutorials
- `theme` — Color themes

**Subeditors**: Many OLC editors support nested editors (e.g., `lox` for scripting, `event` for event bindings). Each uses `EditFunc` signature: `bool func(char* n_fun, Mobile* ch, char* argument, uintptr_t arg, const uintptr_t par)`.

**Security**: Min security levels defined per editor (`MIN_AEDIT_SECURITY`, `MIN_CEDIT_SECURITY`, etc.).

## Lox Scripting (`src/lox/`, `doc/mud98/lox/`)

- **Language**: Bytecode-interpreted OOP+FP language (from _Crafting Interpreters_)
- **Integration**: Scripts attach to entities (mobs, objects, rooms) via OLC; events (`on_enter`, `on_greet`, etc.) trigger callbacks
- **Editing**: Use `lox` subcommand in OLC editors (`redit`, `medit`, `oedit`)
- **Docs**: `doc/mud98/lox/index.md` (language guide, runtime, API reference, recipes)
- **Native API**: Exposed functions/enums in `lox/native.c`; script methods on `Mobile`, `Object`, `Room` classes

## Testing (`src/tests/`, `doc/mud98/unit-test-guide.md`)

- **Run**: `./src/run unittest` or `bin/<Config>/Mud98 -U`
- **Anatomy**: Each test group in `*_tests.c` uses `TestGroup`, `register_test()`, `ASSERT*` macros
- **Assertions**: `ASSERT()`, `ASSERT_STR_EQ()`, `ASSERT_MATCH()`, Lox-specific `ASSERT_LOX_*`, output buffer `ASSERT_OUTPUT_*`
- **Helpers**: `mock_*` for stubbed entities/descriptors; `safe_arg()` for string safety
- **Add tests**: Append to `src/CMakeLists.txt`, register in `tests.c`

### Critical Testing Patterns

**Entity Placement Requirement**:
- **ALL entities must be placed in rooms before executing commands** — commands assume world structure exists
- Pattern: `Room* room = mock_room(...); Mobile* ch = mock_player(...); transfer_mob(ch, room);`
- Forgetting this causes segfaults in command execution (entities reference room->area, room->people, etc.)

**Output Capture Mechanisms**:
- `test_socket_output_enabled = true` — Captures `send_to_char()` output to `test_output_buffer`
- `test_act_output_enabled = true` — Captures `act()` output (different code path)
- Most commands use `send_to_char()`; some use `act()` for third-person messages
- Always reset: `test_output_buffer = NIL_VAL` after assertions

**Mock Function Signatures** (check `src/tests/mock.h` for current signatures):
- `mock_mob(name, vnum, MobPrototype*)` — Requires prototype; use `mock_mob_proto(vnum)` first
- `mock_sword(name, vnum, level, dam_dice, dam_size)` — Creates weapon with specific damage
- `mock_obj(name, vnum, ObjPrototype*)` — Generic object creation
- `mock_player(name)` — Creates player with pcdata initialized
- **Keywords matter**: Object `header.name` field must contain searchable keywords for `get_obj_carry()`

**Test Failures as Diagnostics**:
- Failed assertion may indicate **bug in production code**, not test
- Legacy code may have untested edge cases, missing validation, undefined behavior
- When test fails: verify test setup first, then examine if failure reveals real bug
- Don't automatically "fix" tests to match potentially buggy behavior

## Entity Lifecycle (`entities/`, `recycle.h`, `handler.c`)

**Creation**: Entities are allocated via `new_*` functions (`new_mobile()`, `new_object()`, `new_room()`) and optionally cloned from prototypes (`create_mobile()` from `MobPrototype`, `create_object()` from `ObjPrototype`).

**Placement**: Entities are linked into the world via `*_to_*` functions:
- `char_to_room(ch, room)` — Place mobile in room
- `obj_to_char(obj, ch)` — Give object to mobile
- `obj_to_room(obj, room)` — Place object in room
- `obj_to_obj(obj, container)` — Put object in container

**Removal**: Use `*_from_*` to unlink without destroying:
- `char_from_room(ch)` — Remove from room (doesn't destroy)
- `obj_from_char(obj)` — Remove from inventory
- `obj_from_room(obj)` — Remove from room

**Destruction**: `extract_char(ch, fPull)` and `extract_obj(obj)` fully remove and recycle entities. `free_mobile(mob)` and `free_object(obj)` release memory (usually called by extract functions).

**Recycling**: Mud98 uses a memory pool system (`recycle.h`) for common allocations (buffers, skill arrays, etc.). Always use matching `new_*`/`free_*` pairs.

**Lox visibility**: Entities are exposed to Lox scripts as native objects. Scripts can interact with them via methods, but cannot directly allocate/free—lifecycle remains C-controlled.

## Key Types & Macros (from `src/merc.h`)

- **`VNUM`**: Virtual number (int32_t); `VNUM_NONE = -1`
- **`FLAGS`**: Bitfield (int32_t); use `BIT(x)` for bit flags
- **`LEVEL`**: Character level (int16_t)
- **`Mobile`**: Unified player/NPC type; check `IS_NPC(ch)` or `pcdata != NULL`
- **`Object`**: Item instance; `obj->item_type`, `obj->value[]`, `obj->extra_flags`
- **`Room`**: Location; `room->exits[]`, `room->people`, `room->contents`
- **`Descriptor`**: Network connection; `desc->character`, `desc->connected` (state machine)
- **`DoFunc`**: Command function pointer; `void (*)(Mobile* ch, char* argument)`
- **`EditFunc`**: OLC editor function pointer; signature includes context args for nested editing

## Networking Architecture (`comm.c`, `mth/`)

**Threading**: Non-Windows builds use Pthreads. New connections spawn threads (`pthread_create` in `handle_new_connection`); main thread runs game loop.

**Descriptor list**: Global `descriptor_list` is a singly-linked list of active connections. Access patterns:
- `FOR_EACH(d, descriptor_list)` — Iterate safely
- Descriptors added at head, removed by unlinking and cleanup
- Guard shared access in multithreaded contexts

**State machine**: `desc->connected` tracks login/character creation states (`CON_PLAYING`, `CON_GET_NAME`, etc.). Each state has a handler in the game loop.

**Protocols**: MTH (MUD Telnet Handler) in `mth/` implements GMCP, MCCP (compression), MSDP, MSSP. Protocol state lives in `desc->mth` struct. TLS via OpenSSL when enabled.

**Output**: `send_to_char()`, `page_to_char()`, `printf_to_char()` queue output to descriptor buffer; flushed each game pulse.

## Skills, Spells, and Combat (`data/skill.h`, `data/spell.h`, `fight.c`, `magic.c`)

**Skill vs. Spell vs. Command**:
- **Skills** (`Skill` struct in `skill_table`): Include both spells and abilities. Each has `spell_fun` pointer (C function) or `lox_closure` (Lox script).
- **Spells**: Skills with `spell_fun != spell_null` or Lox closure; executable via `cast` command. Signature: `void spell_foo(SKNUM sn, LEVEL level, Mobile* ch, void* vo, SpellTarget target)`.
- **Commands**: Registered in `cmd_table` via `COMMAND()` macro; signature: `void do_foo(Mobile* ch, char* argument)`. Some commands invoke skills internally (e.g., `do_backstab` calls skill lookup).

**Skill execution**: `HAS_SPELL_FUNC(sn)` checks if skill is executable. Skills track learning difficulty (`rating`), min level per class (`skill_level`), mana cost, beats (delay).

**Combat loop**: `violence_update()` (called each game pulse) iterates fighting mobiles, calls `multi_hit(ch, victim, dt)` → `one_hit()` → damage calculation. Combat ends via `stop_fighting()` or `extract_char()` on death.

**Overlap**: Many "skills" are also commands (e.g., `do_bash` command invokes `gsn_bash` skill). Skills can be practiced, have class restrictions; commands are access-level gated.

## High-Risk Areas (from `doc/mud98/engineering-notes.md`)

- **Networking**: `comm.c`, `mth/*`, TLS setup — easy to break; mirror existing patterns
- **Save/load**: `save.c`, `persist/*` — must maintain backward compatibility with published data
- **Protocol messages**: GMCP/MCCP/MSDP handlers — changes affect all clients

## Common Workflows

**Add a new command**:
1. Declare in `src/command.h`: `COMMAND(do_mycmd)`
2. Implement in appropriate `act_*.c` file: `void do_mycmd(Mobile* ch, char* argument) { ... }`
3. Register in `cmd_table` (usually auto-generated from `COMMAND()` macros)

**Add entity field**:
1. Update struct in `entities/*.h` (e.g., `mobile.h`)
2. Update save/load in `save.c` or relevant persist module
3. Update OLC editor if field is builder-editable
4. Add unit tests covering save/load

**Add new persist format**:
1. Implement `<domain>_persist_<format>.c` in `persist/<domain>/<format>/`
2. Register ops table in `<domain>_persist.h`
3. Document schema in `doc/mud98/json/<domain>-json-schema.md`

## Documentation Quick Reference

- **Project map**: `doc/mud98/project-map.md` (build/run/test, directory layout)
- **Coding guide**: `doc/mud98/coding-guide.md` (style, warnings, CMake)
- **Glossary**: `doc/mud98/glossary.md` (domain terms: area, mobile, OLC, MCCP, etc.)
- **Unit tests**: `doc/mud98/unit-test-guide.md`
- **Lox scripting**: `doc/mud98/lox/index.md`
- **JSON schemas**: `doc/mud98/json/index.md`

## Context-Gathering Tips

- **Find command**: `grep_search` for `COMMAND(do_<name>)` in `src/*.h`
- **Find type definition**: `grep_search` for `typedef struct <name>_t` in `src/**/*.h`
- **Find persist logic**: Check `src/persist/<domain>/` for format implementations
- **Find tests**: Look in `src/tests/*_tests.c`; test naming mirrors source files

## Deep-Context Areas (Require Multi-File Understanding)

### GSN System (Global Skill Numbers)
**Problem**: Skills referenced by pointer (`SKNUM*`) rather than index for dynamic loading.

**Key files**: `src/gsn.h`, `src/db.c`, `src/data/skill.h`
- `gsn.h` declares externs via `GSN(gsn_backstab)` macro (expands to `extern SKNUM gsn_backstab;`)
- Skills have `pgsn` field pointing to corresponding `gsn_*` variable
- `skill_lookup("backstab")` returns index; `gsn_backstab` caches it after `boot_db()`
- Use `HAS_SPELL_FUNC(sn)` to check if skill is executable (checks both C function and Lox closure)
- **Pattern**: Always use `gsn_*` variables for skill checks, not hardcoded numbers

### Linked List Iteration Macros
**Problem**: Custom iteration patterns for type-safe traversal with deletion support.

**Key file**: `src/merc.h` (lines 191-250)
- `FOR_EACH(i, list)` — Safe iteration; use for read-only traversal
- `NEXT_LINK(n)` — Advances to next element
- List heads often global (`descriptor_list`, `mobile_list`, etc.)
- Specialized macros: `FOR_EACH_AREA(area)`, `FOR_EACH_ROOM_OBJ(obj, room)`, etc.
- **Deletion pattern**: Use safe iteration with temp variable; unlink then free

### Entity/Lox Integration Layer
**Problem**: C entities exposed to Lox VM with bidirectional visibility.

**Key files**: `src/entities/entity.h`, `src/lox/native.c`, `src/entities/event.h`
- Every entity has `Entity header` with `Obj obj` (Lox object), `Table fields`, `ObjClass* klass`
- `SET_NATIVE_FIELD(inst, value, field, TYPE)` — Expose C field to Lox
- Entities cannot be allocated/freed from Lox; lifecycle is C-controlled
- Events (`entities/event.h`) bind Lox closures to triggers (`TRIG_GREET`, `TRIG_FIGHT`, etc.)
- Use `HAS_EVENT_TRIGGER(entity, trigger)` macro before raising events
- **Pattern**: Always `push()`/`pop()` when manipulating Lox stack from C

### Event System Architecture
**Problem**: Triggers defined in data, raised in code, callbacks in Lox or MobProgs.

**Key files**: `src/data/events.h`, `src/entities/event.h`, `src/entities/event.c`
- Triggers are bitflags (`TRIG_ACT`, `TRIG_BRIBE`, `TRIG_DEATH`, etc.) defined in `data/events.h`
- Each trigger has metadata in `event_type_info_table[]`: name, default callback, valid entities, criteria type
- Events stored per-entity in `entity->events` list
- `raise_*_event()` functions throughout codebase (e.g., `raise_greet_event()` in room entry)
- **Pattern**: Check `HAS_EVENT_TRIGGER()` before calling `raise_*_event()` for performance

### Game Loop & Update Pulses
**Problem**: Time-based systems with different update frequencies.

**Key files**: `src/update.c`, `src/merc.h` (pulse constants), `src/comm.c` (game_loop)
- `PULSE_PER_SECOND = 4` — 4 ticks per real second
- `PULSE_VIOLENCE = 3 * PULSE_PER_SECOND` — Combat updates every 0.75s
- `PULSE_MOBILE = 4 * PULSE_PER_SECOND` — AI updates every 1s
- `PULSE_TICK = 60 * PULSE_PER_SECOND` — HP/mana regen every 15s
- `update_handler()` called from game loop; dispatches to `violence_update()`, `mobile_update()`, etc.
- **Pattern**: New timed systems should add pulse constant and handler in `update_handler()`

### Command Table Dynamic Loading
**Problem**: Commands loaded from data files, not compiled in.

**Key files**: `src/interp.c`, `src/command.h`, `data/commands.olc` or `data/commands.json`
- `COMMAND(do_foo)` macro in headers declares function
- `cmd_table` populated at runtime from `data/commands.olc` (or `.json`)
- `interpret()` searches sorted `cmd_table` for prefix matches
- `do_function(ch, cmd->do_fun, argument)` wrapper for safe execution
- **Pattern**: Add command function to `act_*.c`, declare in `command.h`, register in data file

### Affect (Status Effect) System
**Problem**: Timed/stackable effects with multiple apply locations.

**Key files**: `src/entities/affect.h`, `src/entities/affect.c`
- Affects apply modifiers to stats (`APPLY_STR`, `APPLY_HITROLL`, etc.)
- `affect_to_mob(ch, affect)` — Apply affect to mobile
- `affect_join(ch, affect)` — Stack or refresh existing affect
- Each affect has duration (ticks), modifiers, bitvector (flags), type (skill number)
- Affects tick down in `affect_update()` (called from `update_handler()`)
- **Pattern**: Always use `affect_to_*` functions; never manually link affects to entity lists

## Common Pitfalls

- **Don't edit untitled notebook files by mistake** — always work with saved `.c`/`.h` files
- **Don't batch-edit persistence without testing** — save/load bugs corrupt player data
- **Don't add printf debugging to committed code** — use existing logging (`log_string`, `bugf`)
- **Don't forget to update CMakeLists.txt** — new `.c` files must be added to executable list
- **Test with multiple formats** — if JSON toggle exists, test both ROM-OLC and JSON paths
- **Don't hardcode skill numbers** — use `gsn_*` variables or `skill_lookup()`
- **Don't manually iterate descriptor_list** — use `FOR_EACH(d, descriptor_list)` macro
- **Don't allocate entities from Lox** — lifecycle must remain C-controlled

When in doubt, consult the existing code in the subsystem you're modifying and mirror its patterns. The codebase values consistency, backward compatibility, and zero-warning builds.
