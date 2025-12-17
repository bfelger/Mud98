# Unit Test Guide

How to add and run tests that match the existing `src/tests/` style.

## Running tests

Tests are compiled into a separate `Mud98Tests` executable (not the main game binary):

```sh
# Via helper script from repo root
./run test

# Direct execution
./bin/Mud98Tests
./bin/Debug/Mud98Tests  # Or Release, RelWithDebInfo

# Via CTest (from build directory)
cd build  # or .build-gcc, etc.
ctest --output-on-failure
```

Tests print failures with file/line and per-group summaries; keep output quiet on success.

## Architecture

Mud98 uses a library-based test architecture:
- Core game logic lives in `libMud98Core` static library
- Main executable (`Mud98`) links the library
- Test executable (`Mud98Tests`) also links the library + test files
- This keeps production binaries clean (37.5% smaller!) without test infrastructure

## Anatomy of a test group
- Each group lives in `src/tests/*_tests.c`.
- Pattern:
  ```c
  #include "tests.h"
  #include "test_registry.h"

  static TestGroup group;

  static void some_feature_test(void) {
      ASSERT(condition);
      ASSERT_STR_EQ("expected", actual);
  }

  void register_some_feature_tests() {
      init_test_group(&group, "Some Feature");
      register_test_group(&group);
      register_test(&group, "Does the thing", some_feature_test);
  }
  ```
- Add the new `register_*_tests` declaration in `tests.h` and call it from `register_unit_tests` (see `tests.c`/`all_tests.c`).

## Assertions to use
- `ASSERT(expr)` / `ASSERT_OR_GOTO(expr, cleanup)` for basic checks.
- Strings/patterns: `ASSERT_STR_EQ`, `ASSERT_MATCH`.
- Lox values: `ASSERT_LOX_INT_EQ`, `ASSERT_LOX_STR_EQ`, `ASSERT_LOX_STR_CONTAINS`.
- Output buffer (from VM/test harness): `ASSERT_OUTPUT_EQ`, `ASSERT_OUTPUT_MATCH`, `ASSERT_OUTPUT_CONTAINS`.
- Keep tests deterministic and minimal; no printf debugging in committed tests.

## Common helpers
- Use `mock_*` when you need stubbed entities, descriptors, sockets, etc.
- `safe_arg` helps print nullable strings safely, as well as protect string literals in mutative functions.
- `reset_stack()` is invoked after each test by the harness; avoid persistent global state.

## Mocking Functions

The `mock.h` library provides utilities to create test entities. All mocks are GC-tracked and automatically cleaned up between tests.

### Critical Rule: Entity Placement

**ALL entities must be placed in rooms before executing commands.** Commands assume world structure exists—entities reference `room->area`, `room->people`, etc.

```c
// WRONG - Will segfault!
Mobile* ch = mock_player("Bob");
do_north(ch, "");

// CORRECT - Entity placed in world
Room* room = mock_room(50000, NULL, NULL);
Mobile* ch = mock_player("Bob");
transfer_mob(ch, room);  // REQUIRED!
do_north(ch, "");
```

### Room Mocking

**Simple case** (most tests):
```c
Room* room = mock_room(50000, NULL, NULL);
```

**With area context**:
```c
AreaData* ad = mock_area_data();
Area* area = mock_area(ad);
Room* room = mock_room(50000, NULL, area);
```

### Room Connections for Movement Tests

**CRITICAL**: Movement commands check `Room->exit[]`, not `RoomData->exit_data[]`!

There are two connection functions:
- `mock_room_data_connection()` - Sets up template layer only (DON'T use for movement tests)
- `mock_room_connection()` - Sets up runtime layer that `move_char()` checks (USE THIS)

```c
// WRONG - Sets up templates but not runtime exits
RoomData* rd1 = mock_room_data(50000, NULL);
RoomData* rd2 = mock_room_data(50001, NULL);
mock_room_data_connection(rd1, rd2, DIR_NORTH, true);
Room* room1 = mock_room(50000, rd1, NULL);
Room* room2 = mock_room(50001, rd2, NULL);
// do_north() will fail - room1->exit[DIR_NORTH] is NULL!

// CORRECT - Sets up runtime exits
Room* room1 = mock_room(50000, NULL, NULL);
Room* room2 = mock_room(50001, NULL, NULL);
mock_room_connection(room1, room2, DIR_NORTH, true);
// do_north() works - room1->exit[DIR_NORTH] points to room2
```

See `test_greet_event_on_room` in `event_tests.c` for a working example.

### Mobile Mocking

**NPCs** (non-player characters):
```c
Mobile* npc = mock_mob("Guard", 53001, NULL);
transfer_mob(npc, room);
```

**With specific stats**:
```c
MobPrototype* proto = mock_mob_proto(53001);
proto->level = 10;
proto->hitroll = 50;
Mobile* mob = mock_mob("Guard", 53001, proto);
```

**Players** (with pcdata):
```c
Mobile* player = mock_player("Alice");
player->level = 5;
player->gold = 1000;
transfer_mob(player, room);
```

### Object Mocking

**Generic items**:
```c
Object* obj = mock_obj("torch", 54001, NULL);
obj_to_room(obj, room);
```

**CRITICAL: Object Keywords**

The `header.name` field must contain searchable keywords for `get_obj_carry()` to find items:

```c
// WRONG - Can only "get ruby"
Object* gem = mock_obj("ruby", 54002, NULL);

// CORRECT - Can "get ruby", "get gem", or "get red"
Object* gem = mock_obj("ruby gem red", 54002, NULL);
```

**Weapons**:
```c
Object* sword = mock_sword("sword", 54003, 10, 3, 6);  // level 10, 3d6 damage
obj_to_char(sword, ch);
equip_char(ch, sword, WEAR_WIELD);
```

**Containers** (using semantic union members):
```c
ObjPrototype* proto = mock_obj_proto(54004);
proto->item_type = ITEM_CONTAINER;
proto->container.capacity = 100;         // Max weight
proto->container.max_item_weight = 50;   // Individual item limit
proto->container.flags = CONT_CLOSEABLE;
Object* chest = mock_obj("chest", 54004, proto);
```

**Drinks/Food** (using semantic union members):
```c
// Fountain
ObjPrototype* fountain_proto = mock_obj_proto(54005);
fountain_proto->item_type = ITEM_FOUNTAIN;
fountain_proto->fountain.capacity = 100;
fountain_proto->fountain.liquid_type = LIQ_WATER;

// Food
ObjPrototype* food_proto = mock_obj_proto(54006);
food_proto->item_type = ITEM_FOOD;
food_proto->food.hours_full = 10;
food_proto->food.is_poisoned = false;
```

### Output Capture

Two mechanisms capture different output paths:

```c
// Enable output capture
test_socket_output_enabled = true;  // Captures send_to_char()
test_act_output_enabled = true;     // Captures act() messages

// Execute command
interpret(ch, "look");

// Check output
ASSERT_OUTPUT_EQ("Expected output\n");
ASSERT_OUTPUT_CONTAINS("partial match");
ASSERT_OUTPUT_MATCH("pattern with $N wildcards");

// ALWAYS reset after assertions
test_output_buffer = NIL_VAL;
```

**When to use which**:
- Most commands use `send_to_char()` - enable `test_socket_output_enabled`
- Some use `act()` for third-person messages - enable `test_act_output_enabled`
- If unsure, enable both

### Test Lifecycle

Tests are independent; state doesn't carry between them:

```c
static int test_feature()
{
    // 1. Setup - Create mocks
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("Bob");
    transfer_mob(ch, room);
    
    // 2. Execute - Run the code under test
    do_command(ch, "args");
    
    // 3. Assert - Verify results
    ASSERT(condition);
    ASSERT_OUTPUT_EQ("expected\n");
    
    // 4. Cleanup - Reset output buffer (mocks auto-cleaned)
    test_output_buffer = NIL_VAL;
    
    return 0;  // Success
}
```

### Common Patterns

**Movement test**:
```c
Room* r1 = mock_room(50000, NULL, NULL);
Room* r2 = mock_room(50001, NULL, NULL);
mock_room_connection(r1, r2, DIR_NORTH, true);

Mobile* ch = mock_player("Bob");
transfer_mob(ch, r1);

do_north(ch, "");

ASSERT(ch->in_room == r2);
```

**Combat test**:
```c
Room* room = mock_room(50000, NULL, NULL);

Mobile* attacker = mock_mob("Attacker", 50001, NULL);
transfer_mob(attacker, room);

Mobile* victim = mock_mob("Victim", 50002, NULL);
victim->hit = 100;
victim->max_hit = 100;
transfer_mob(victim, room);

Object* sword = mock_sword("sword", 50003, 1, 1, 1);
obj_to_char(sword, attacker);
equip_char(attacker, sword, WEAR_WIELD);

interpret(attacker, "kill Victim");
violence_update();  // Process combat round

ASSERT(victim->hit < victim->max_hit);
```

**Item manipulation test**:
```c
Room* room = mock_room(50000, NULL, NULL);
Mobile* ch = mock_player("Bob");
transfer_mob(ch, room);

Object* obj = mock_obj("coin gold", 50001, NULL);  // Keywords!
obj->wear_flags = ITEM_TAKE;
obj_to_room(obj, room);

test_socket_output_enabled = true;
interpret(ch, "get coin");

ASSERT_OUTPUT_CONTAINS("You get");
ASSERT(obj->carried_by == ch);

test_output_buffer = NIL_VAL;
```

## Adding sources
- Append new `*_tests.c` files to the `add_executable` list in `src/CMakeLists.txt`.
- Keep test-only headers local to `src/tests/` unless shared broadly.

## Tips
- Familiarize yourself with existing unit tests and how they work; especially the mocking functions available in `tests/mock.h`.
- One behavior per test; name it clearly (`"Parses quoted arg"`, `"Fails on NULL input"`).
- Mirror existing patterns in the subsystem you're testing (e.g., see `event_tests.c`, `entity_tests.c`).
- If a test needs config, prefer in-memory setup over touching real files unless the feature is file-based.
- **Test failures may indicate bugs in production code**, not the test—legacy code has untested edge cases.
