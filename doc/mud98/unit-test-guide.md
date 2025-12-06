# Unit Test Guide

How to add and run tests that match the existing `src/tests/` style.

## Running tests
- From repo root: `./src/run unittest` (passes `-U` to the binary).
- Or direct: `bin/<Config>/Mud98 -U`.
- Tests print failures with file/line and per-group summaries; keep output quiet on success.

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

## Adding sources
- Append new `*_tests.c` files to the `add_executable` list in `src/CMakeLists.txt`.
- Keep test-only headers local to `src/tests/` unless shared broadly.

## Tips
- Familiarize yourself with existing unit tests and how they work; especially the mocking functions available in `tests/mock.h`.
- One behavior per test; name it clearly (`"Parses quoted arg"`, `"Fails on NULL input"`).
- Mirror existing patterns in the subsystem you’re testing (e.g., see `event_tests.c`, `entity_tests.c`).
- If a test needs config, prefer in-memory setup over touching real files unless the feature is file-based.
