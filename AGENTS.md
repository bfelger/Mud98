# Mud98 AI Agent Instructions

> **Project**: Mud98 — modernized ROM 2.4b6 MUD server (C23/C17)  
> **Test Status**: 559 unit tests  
> **Build**: CMake + Ninja Multi-Config

---

## Quick Reference

| Task | Command |
|------|---------|
| Build (debug) | `cmake -S . -B build -G Ninja && cmake --build build` |
| Run tests | `./src/run test` or `./bin/Mud98Tests` |
| Run benchmarks | `./src/run bench` |
| Run server | `./src/run` (defaults to debug, port 4000) |

---

## Architecture

```
src/
├── entities/      # Mobile, Object, Room, Area, Descriptor
├── data/          # Class, Race, Skill, Quest, Social, Tutorial
├── persist/       # Pluggable I/O: ROM-OLC (.are/.olc) and JSON
├── olc/           # Online Creation system (live world editing)
├── lox/           # Embedded scripting VM (Crafting Interpreters)
├── mth/           # Telnet protocols: GMCP, MCCP, MSDP, MSSP
├── tests/         # Unit test suite
└── benchmarks/    # Performance benchmarks
```

**Runtime data**: `area/` (world files), `data/` (classes, races, skills, commands)

---

## Coding Standards

**Compiler flags**: `-Wall -Werror -Wextra -pedantic` (GCC/Clang), `/Wall /WX` (MSVC)

**Naming**:
- Types: Typedef'd to `UpperCamelCase`. If named type is required for forward decl, use `snake_case` + `_t` suffix → `struct mob_prototype_t` typedef'd to `MobPrototype`
- Functions/variables: `snake_case`
- Macros/constants: `SCREAMING_SNAKE_CASE`

**Includes**: Use project-relative paths: `#include <entities/mobile.h>`

**String handling**: Prefer `StringBuffer` (`sb_new()`, `sb_appendf()`, `sb_free()`) over stack buffers with `sprintf`. Use `printf_to_char()` for single formatted outputs. Avoid `sprintf`/`vsprintf`—use `snprintf`/`vsnprintf`.

**Sorting**: Use `qsort()` from stdlib, not custom sort implementations.

**New source files**: Must be added to `src/CMakeLists.txt`.

---

## Key Types

| Type | Description | Key fields/macros |
|------|-------------|-------------------|
| `Mobile` | Player or NPC | `IS_NPC(ch)`, `ch->pcdata` (NULL for NPCs) |
| `Object` | Item instance | `obj->item_type`, `obj->value[]` |
| `Room` | Location | `room->exits[]`, `room->people`, `room->contents` |
| `Descriptor` | Network connection | `desc->character`, `desc->connected` |
| `VNUM` | Virtual number (int32_t) | `VNUM_NONE = -1` |
| `LEVEL` | Character level (int16_t) | `LEVEL_HERO = 51` |
| `SKNUM` | Skill index (int16_t) | Use `gsn_*` variables, not literals |

---

## Commands

**Signature**: `void do_foo(Mobile* ch, char* argument)`

**Declaration**: `COMMAND(do_foo)` macro in `src/command.h`

**Implementation**: In `act_*.c` files

**Registration**: Commands loaded at runtime from `data/commands.olc` or `data/commands.json`

**Adding a command**:
1. Declare: `COMMAND(do_mycmd)` in `command.h`
2. Implement: `void do_mycmd(Mobile* ch, char* argument) { ... }` in appropriate `act_*.c`
3. Register: Add entry to `data/commands.olc` or `data/commands.json`

---

## Skills & Spells

**Skills** include both spells and abilities. Key distinction:
- **Spell**: `HAS_SPELL_FUNC(sn)` is true; executable via `cast` command
- **Skill**: No spell function; used via dedicated command (e.g., `backstab`)

**GSN System**: Skills referenced by cached indices (`gsn_backstab`, etc.) not literals.
- Declared via `GSN(gsn_foo)` macro in `gsn.h`
- Resolved during `boot_db()` from skill names
- **Always use `gsn_*` variables**, never hardcode skill numbers

**Spell signature**: `void spell_foo(SKNUM sn, LEVEL level, Mobile* ch, void* vo, SpellTarget target)`

---

## Entity Lifecycle

**Creation**:
- `new_mobile()`, `new_object()`, `new_room()` — allocate
- `create_mobile(MobPrototype*)`, `create_object(ObjPrototype*)` — clone from prototype

**Placement**:
- `char_to_room(ch, room)` — place mobile in room
- `transfer_mob(ch, room)` — convenience wrapper (removes from old room first)
- `obj_to_char(obj, ch)` — give object to mobile
- `obj_to_room(obj, room)` — place object in room

**Removal**:
- `char_from_room(ch)` — unlink from room (doesn't destroy)
- `obj_from_char(obj)`, `obj_from_room(obj)` — unlink objects

**Destruction**:
- `extract_char(ch, fPull)`, `extract_obj(obj)` — fully remove and recycle

---

## Testing

**Run**: `./src/run test` or `./bin/Mud98Tests`

**Critical pattern**: Entities must be placed in rooms before command execution:
```c
Room* room = mock_room(60001, NULL, NULL);
Mobile* ch = mock_player("Tester");
transfer_mob(ch, room);  // REQUIRED before do_foo(ch, "...")
```

**Output capture**:
```c
test_socket_output_enabled = true;
do_foo(ch, "argument");
test_socket_output_enabled = false;
ASSERT_OUTPUT_CONTAINS("expected text");
test_output_buffer = NIL_VAL;  // Always reset
```

**Mock functions** (see `src/tests/mock.h`):
- `mock_player(name)` — player with pcdata
- `mock_mob(name, vnum, proto)` — NPC from prototype
- `mock_mob_proto(vnum)` — create prototype first
- `mock_room(vnum, area, name)` — room in optional area
- `mock_sword(name, vnum, level, dam_dice, dam_size)` — weapon

**Test failures may reveal production bugs** — don't assume the test is wrong.

---

## Persistence

**Format selection**: Automatic by file extension
- `.are`, `.olc` → ROM-OLC format (legacy text)
- `.json` → JSON format (Jansson library)

**File locations**:
- `area/` — world areas
- `data/` — classes, races, skills, commands, socials, themes, tutorials

**Adding new persist format**:
1. Implement `<domain>_persist_<format>.c` in `persist/<domain>/<format>/`
2. Register in `<domain>_persist.h`
3. Document in `doc/mud98/json/`

---

## Game Loop & Timing

| Constant | Value | Real time |
|----------|-------|-----------|
| `PULSE_PER_SECOND` | 4 | 250ms per pulse |
| `PULSE_VIOLENCE` | 12 | 3s (combat round) |
| `PULSE_MOBILE` | 16 | 4s (NPC AI) |
| `PULSE_TICK` | 240 | 60s (regen tick) |
| `PULSE_AREA` | 480 | 120s (area reset) |

**Update handler**: `update_handler()` in `update.c` dispatches to subsystem updates.

---

## Iteration Macros

```c
FOR_EACH(d, descriptor_list)     // Iterate linked list
FOR_EACH_AREA(area)              // Iterate all areas
FOR_EACH_ROOM_MOB(ch, room)      // Iterate mobiles in room
FOR_EACH_ROOM_OBJ(obj, room)     // Iterate objects in room
```

**Never manually iterate** — use macros for consistency and safety.

---

## Output Functions

| Function | Use case |
|----------|----------|
| `send_to_char(msg, ch)` | Static string to character |
| `printf_to_char(ch, fmt, ...)` | Formatted output (uses vsnprintf) |
| `page_to_char(msg, ch)` | Long output with paging |
| `act(fmt, ch, arg1, arg2, type)` | Third-person messages |

**For building complex output**: Use `StringBuffer`:
```c
StringBuffer* sb = sb_new();
sb_appendf(sb, "Value: %d\n\r", value);
send_to_char(sb_string(sb), ch);
sb_free(sb);
```

---

## High-Risk Areas

| Area | Files | Risk |
|------|-------|------|
| Networking | `comm.c`, `mth/*` | Threading, protocol compliance |
| Save/Load | `save.c`, `persist/*` | Data corruption, backwards compat |
| Combat | `fight.c`, `combat_rom.c` | Balance, death handling |

**Mirror existing patterns** when modifying these areas.

---

## Common Pitfalls

- **Forgetting `transfer_mob()`** in tests → segfault
- **Hardcoding skill numbers** → breaks when skills reorder
- **Using `sprintf`** → buffer overflow; use `snprintf` or `StringBuffer`
- **Missing CMakeLists.txt update** → new files not compiled
- **Testing only one format** → always test both ROM-OLC and JSON paths

---

## Documentation

| Topic | File |
|-------|------|
| Project overview | `doc/mud98/project-map.md` |
| Coding standards | `doc/mud98/coding-guide.md` |
| Unit testing | `doc/mud98/unit-test-guide.md` |
| Lox scripting | `doc/mud98/lox/index.md` |
| JSON schemas | `doc/mud98/json/index.md` |
| Glossary | `doc/mud98/glossary.md` |

Planning documents, such as system requirements and design documents, go in the `doc/planning/` folder.

---

## Search Patterns

| To find | Search for |
|---------|------------|
| Command implementation | `COMMAND(do_name)` |
| Type definition | `typedef struct name_t` |
| Skill by name | `gsn_skillname` in `gsn.h` |
| Persist logic | Files in `src/persist/<domain>/` |
| Tests for domain | `src/tests/<domain>_tests.c` |
| OLC editor | `src/olc/<x>edit.c` |
