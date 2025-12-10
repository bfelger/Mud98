# Mud98 Lox Language Guide

This guide describes the full syntax and semantics of Mud98’s embedded Lox interpreter. Every feature documented here exists in the game today; examples reference automated tests (e.g., `src/tests/lox_tests.c`, `src/tests/lox_ext_tests.c`) or builder docs so you can trace behavior back to source.

## Table of Contents
1. [Lexical Structure](#lexical-structure)
2. [Values & Types](#values--types)
3. [Expressions](#expressions)
4. [Statements & Blocks](#statements--blocks)
5. [Functions, Methods & Classes](#functions-methods--classes)
6. [Closures & Lambdas](#closures--lambdas)
7. [Constants & Enums](#constants--enums)
8. [Data Structures](#data-structures)
9. [Strings & Interpolation](#strings--interpolation)
10. [Error Handling & Validation](#error-handling--validation)

---

## Lexical Structure
- **Identifiers**: ASCII letters, digits, `_`; case-sensitive (`mob_count`, `Mob_Count` differ).
- **Comments**: `//` single-line (see tutorial snippets around `doc/mud98/wb-02-new-beginnings.md:579`).
- **Literals**: integers, doubles, strings, booleans (`true`/`false`), `nil`.
- **Reserved Keywords**: `var`, `const`, `fun`, `class`, `this`, `return`, control keywords (`if`, `else`, `while`, `for`, `break`, `continue`), `enum`.
- **Editor notes**: In OLC, blank-line commands (e.g., `.s`, `.v`, `@`) wrap the script but don’t change the language grammar.

## Values & Types
Mud98 Lox uses tagged values. Primitive categories:
- **Numbers**: 32-bit signed integers or doubles. Many native APIs expect integers (damage, timers, etc.).
- **Booleans**: `true` / `false`.
- **Nil**: `nil` stands for “no value”.
- **Strings**: immutable sequences of bytes; concatenation creates new strings.
- **Objects**: arrays, lists, tables, enumerations, functions, classes, instances, closures, raw pointers (for interop), and built-in entity wrappers (rooms, mobs, objs, etc.).

Runtime type helpers (`is_obj`, `is_room`, etc.) are exposed as native methods (see [API Reference](api.md)).

## Expressions
- **Arithmetic / comparison / logical**: standard precedence, includes `and`, `or`, unary `-`, `!`.  
  > [!IMPORTANT]  
  > Lox uses the keywords `and` / `or` rather than C-style `&&` / `||`.
  >
  > Builders switching between C and Lox should double-check new scripts for the correct logical keywords; the scanner treats `&&` / `||` as unexpected characters.
- **Assignment**: `var a = 2; a += 1;`.
- **Call expressions**: `foo(1, 2)`, chained calls `obj.method(args).other()`.
- **Property access**: `object.field`, method dispatch `object.method(args)`.
- **Indexing**: arrays use `array[index] = value;`; prefer `.each` for tables/lists.
- **String interpolation**: `"HP ${mob.hp}"` (details below).
- **Raw pointer access**: direct use is rare; tests show wrapping C pointers via `WRAP_I16`, etc., when bridging C ↔ Lox (see `src/tests/lox_ext_tests.c:65-114`).

## Statements & Blocks
- **Variable declarations**: `var`, `const` (block-scoped).
- **Blocks**: `{ ... }` introduce new scopes, especially useful for limiting consts to tiny lifetimes (see `test_const_1`).
- **Conditionals**: `if (...) { } else { }` (use guard clauses liberally in callbacks).
- **Loops**:
  - `while` loops (classic sentinel form).
  - `for (init; condition; increment)` desugars to a scoped block + `while`; omit any clause you do not need (`src/tests/lox_ext_tests.c:19-43`).
  - `.each` loops iterate lists/tables/arrays by passing a lambda.
- **Break / continue**: allowed inside loops; the compiler tracks exit patches so nested loops behave correctly.
- **Return**:
  - `return expr;` returns the evaluated value.
  - Bare returns **must** use `return;` (semicolon required) so the parser doesn’t consume the next token as an implicit expression (`src/lox/compiler.c:1870-1883`).
  - Builders often `return;` early when quest or race checks fail.

See `src/tests/lox_tests.c` for canonical arithmetic, variable, and scoping behavior (e.g., `test_22_scoped_vars`).

## Functions, Methods & Classes
- **Functions**: `fun name(params) { ... }`. Stored as first-class values.
- **Classes**: `class Foo { init() { ... } method() { ... } }`.
- **Entity classes**: When editing a room/mob/obj via OLC, the script body implicitly defines methods on a generated class (e.g., `room_12000.on_login`). Do not wrap with `class ...` inside entity scripts—just declare methods (tutorial at `doc/mud98/wb-02-new-beginnings.md:579`).
- **Constructors**: `init()` acts as initializer when calling `Foo()`. Use `return this;` when you need fluent APIs.
- **Method dispatch**: `instance.method()` uses dynamic binding; inside class bodies `this` references the instance. You can define helper methods and call them via `this.helper()` to avoid repeating quest logic.
- **Native methods**: Entities gain additional methods (quests, factions, messaging) outlined in the API reference.

## Closures & Lambdas
- **Closures**: Functions capture outer variables automatically.
- **Lamdas** (lambda shorthand): `() -> { ... }` or `(arg) -> expr`. Valid anywhere a callable is expected.
  - Example (delayed tutorial prompt): `delay(1, () -> { vch.send("..."); })` (`doc/mud98/wb-02-new-beginnings.md:763`).
  - Tests: `test_lamdas`, `test_bare_lamdas`, `test_lamda_values` in `src/tests/lox_ext_tests.c`.
- Captured variables remain alive until the closure finishes; see the raw pointer interop test for a stress case (`src/tests/lox_ext_tests.c:65-114`).
- Store lambdas in variables or pass them as arguments (`do("command")` expects a string, but `.each` expects a lambda).

## Constants & Enums
- **Const**: `const speed = 4;` (optimized, prevents reassignment).
  - Tested via `test_const_1`, `test_const_2`, `test_const_folding`.
- **Enums**: `enum Damage { Bash, Slash = 3 }`.
  - Boot-time enums expose game tables (`Race`, `DamageType`, etc.)—see `test_enum_bootval`.
  - Access members as `Race.Elf`.
  - Auto-numbering starts at 0, but you can override values mid-definition and the following entries continue counting from there (`test_enum_auto`, `test_enum_assign`).

## Data Structures
- **Arrays** (`[]` literal):
  - Methods: `add(value)`, `contains(value)`, `.count`.
  - Indexed via `[ ]`, assignable.
  - Examples: `test_array_access`, `test_array_add`, `test_array_contains`.
- **Tables**: Associative maps; iteration order matches internal bucket order. `.each((key, value) -> { ... })` emits both key and value for each filled bucket.
- **Lists**: Doubly-linked lists surfaced by engine APIs (e.g., `global_mobs`). `.each((value) -> { ... })` walks each node’s value in order.
- **`.each` callback arity**:
  - Arrays: `(index, value)`.
  - Tables: `(key, value)`.
  - Lists: `(value)`.
- **Raw pointers / marshal**: advanced interop, typically hidden behind native APIs.

## Strings & Interpolation
- **Literal concatenation**: Adjacent string literals merge automatically, aiding OLC formatting (`doc/mud98/wb-02-new-beginnings.md:1046`).
- **Interpolation tokens**:
  - Escape literal `$` with `$$` (`test_string_interp_escape`).
  - Protect legacy `$n` style sequences by quoting (`test_string_interp_var`, `test_string_interp_var_start`).
  - Expressions inside `${ ... }` are evaluated (`test_string_interp_expr`, `test_string_interp_expr_2`).
  - Scanner behavior verified in `test_string_interp_expr_scanning`.

## Error Handling & Validation
- **Editor validation**: `.v` compiles without saving; errors show full stack traces (see VM runtime in `src/lox/vm.c`).
- **Runtime errors**: `runtime_error()` prints stack + message to the invoking character (or log). Use guard clauses and type checks when calling native APIs.
- **Testing**: Automated unit tests (mentioned above) double as executable docs; replicate their snippets to understand behavior.
- **Defensive scripting**: Use type predicates (`is_mob()`, `is_obj()`) before calling quest/faction helpers so your scripts fail gracefully.

Continue to the [Runtime Guide](runtime.md) for details on execution context, events, and persistence, or jump to the [API Reference](api.md) for callable surfaces.
