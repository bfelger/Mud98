# Abstraction Boundaries and Seams

This document describes the architectural boundaries in Mud98 that isolate implementations from their public interfaces, enabling testability and maintainability.

## Overview

Mud98 uses "seams" (as described in Michael Feathers' "Working Effectively with Legacy Code") to create points where behavior can be changed without modifying calling code. This enables:
- Testing in isolation
- Swapping implementations (e.g., ROM-OLC vs JSON formats)
- Maintaining backward compatibility with legacy code while modernizing
- Clear separation between stable APIs and volatile implementations

## Core Principle: Make the Wrong Thing Hard

Abstractions should be structurally enforced, not discipline-enforced:

**Good**: Public API only exposes abstractions; implementations are `static` or in internal headers  
**Bad**: Implementation details exposed in public headers, requiring discipline not to call them

## Abstraction Layers

### Persistence Layer (`src/persist/`)

**Public API**: Format-agnostic interfaces
- `persist_io.h` — Generic reader/writer abstractions
- `area_persist.h` — Area persistence interface
- `class_persist.h`, `race_persist.h`, etc. — Domain-specific persistence

**Internal APIs**: Format-specific implementations
- `persist/rom-olc/db_rom_olc.h` — ROM legacy functions (internal only)
- `persist/area/rom-olc/area_persist_rom_olc.c` — ROM-OLC format implementation
- `persist/area/json/area_persist_json.c` — JSON format implementation

**Pattern**:
```c
// Public header (area_persist.h) - users include this
const AreaPersistFormat* area_persist_select_format(const char* file_name);

// Internal header (db_rom_olc.h) - only format implementations include this
void load_helps(FILE* fp, char* fname);  // ROM legacy function
```

**License Compliance**: ROM legacy code (`load_helps`, `load_shops`, etc.) retains ROM copyright headers and is isolated in `persist/rom-olc/` directory, separate from clean-room implementations.

### Command System (`src/command.h`, `src/interp.c`)

**Public API**: Command registration and dispatch
- `COMMAND(do_foo)` macro declares commands
- `cmd_table` loaded from data files at runtime
- `interpret()` dispatches to registered commands

**Seam**: Commands are data-driven, not compiled in. Tests can:
- Mock command implementations
- Test command parsing without invoking implementations
- Add test-only commands

### Skill System (`src/data/skill.h`)

**Public API**: Skill lookup and metadata
- `skill_table[]` — Skill definitions
- `gsn_*` variables — Global skill number pointers
- `HAS_SPELL_FUNC(sn)` — Check if skill is executable

**Seam**: Skills can have either:
- C function (`spell_fun` pointer)
- Lox script (`lox_closure` closure)

This allows skills to be implemented in either language without changing calling code.

### Event System (`src/entities/event.h`, `src/data/events.h`)

**Public API**: Event registration and raising
- `HAS_EVENT_TRIGGER(entity, trigger)` — Check if entity has handler
- `raise_*_event()` functions — Trigger callbacks

**Seam**: Event handlers can be:
- Lox closures (editable via OLC)
- MobProgs (legacy format)

Tests can attach custom handlers without modifying entity code.

## Creating New Seams

When adding testability to tightly coupled code:

### 1. Identify the Coupling
What external dependency blocks testing?
- File I/O? → Abstract to `PersistReader`/`PersistWriter`
- Random numbers? → Abstract to RNG ops table
- Network I/O? → Abstract to descriptor gateway
- Database? → Abstract to repository interface

### 2. Extract to Interface
Create an ops table or function pointer struct:

```c
// rng.h
typedef struct rng_ops_t {
    int (*number_range)(int from, int to);
    int (*number_percent)(void);
} RngOps;

extern RngOps* rng;  // Global pointer
```

### 3. Provide Implementations
Production version + test mock:

```c
// rng_production.c (ROM legacy code, retains ROM license)
static int prod_number_range(int from, int to) {
    return pcg_number_range(from, to);
}

RngOps production_rng = { .number_range = prod_number_range, /* ... */ };

// tests/mock_rng.c (clean-room test code)
static int mock_number_range(int from, int to) {
    return from;  // Deterministic for tests
}

RngOps mock_rng = { .number_range = mock_number_range, /* ... */ };
```

### 4. Inject Dependency
Use the abstraction in production code:

```c
// Before (tight coupling)
int damage = number_range(1, 10);

// After (seam)
int damage = rng->number_range(1, 10);
```

Tests can swap `rng = &mock_rng` before running.

## Performance Considerations

Function pointer indirection adds ~2-3 nanoseconds per call on modern CPUs. For context:
- Combat updates: every 750ms (4x/second)
- String operations: 100-1000x slower than function pointers
- File I/O: 100,000x slower than function pointers

**Hot paths** (millions of calls/second): Profile before abstracting  
**Warm paths** (thousands of calls/second): Abstractions are fine  
**Cold paths** (occasional): Use abstractions freely

Combat, command processing, and area updates are **warm paths** where testability benefits far outweigh nanosecond costs.

## Directory Structure for Abstractions

```
src/
  <domain>.h              # Public API (users include this)
  <domain>.c              # Implementation or dispatcher
  
  persist/
    <domain>/
      <domain>_persist.h  # Public persistence API
      rom-olc/
        <domain>_persist_rom_olc.h    # ROM-OLC format interface
        <domain>_persist_rom_olc.c    # ROM-OLC implementation
      json/
        <domain>_persist_json.h       # JSON format interface  
        <domain>_persist_json.c       # JSON implementation
    
    rom-olc/
      db_rom_olc.h        # ROM legacy internals (internal only)
```

**Rule**: `persist/rom-olc/` contains ROM legacy code with ROM license. `persist/json/` and `persist/<domain>/` contain clean-room code.

## Checklist for Adding Abstractions

- [ ] Create public API header with abstract interface
- [ ] Create internal headers for implementation details
- [ ] Make implementation functions `static` or move to internal headers
- [ ] Document which headers are public vs internal
- [ ] Update callers to use abstraction, not implementation
- [ ] Provide test mock implementation
- [ ] Add tests exercising both production and mock paths
- [ ] Verify no implementation details leak into public headers

## Examples in Codebase

**Good abstraction** (persist layer):
- Public: `area_persist.h` exposes `AreaPersistFormat` interface
- Internal: `db_rom_olc.h` exposes ROM functions only to ROM-OLC modules
- Users call `area_persist_select_format()`, not `load_helps()`

**Needs improvement** (RNG):
- Currently: Direct calls to `number_range()` throughout codebase
- Proposal: Extract to `RngOps` interface for deterministic testing

**Good abstraction** (events):
- Public: `raise_greet_event()` triggers callbacks
- Internal: Handlers can be Lox or MobProgs
- Tests attach custom handlers without modifying event system

## See Also

- **Persistence API**: `src/persist/persist_io.h` — Generic I/O abstractions
- **Unit Test Guide**: `doc/mud98/unit-test-guide.md` — Testing with mocks
- **Coding Guide**: `doc/mud98/coding-guide.md` — Style and structure
- **Legacy Code**: "Working Effectively with Legacy Code" by Michael Feathers (book)
