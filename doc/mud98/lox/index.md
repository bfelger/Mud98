
# Mud98 Lox

Mud98 Lox is the scripting language that powers in-game automation, narrative beats, and world logic throughout Mud98. These docs walk builders through the entire toolchain—from editing scripts in OLC to wiring triggers, calling native APIs, and composing reusable recipes.

## Who This Guide Is For
- **Builders** adding bespoke interactions to rooms, mobs, and objects.
- **Scripters** authoring shared logic or public utilities.
- **Tooling/AI users** that need a consistent, machine-readable reference.

No prior knowledge of Crafting Interpreters or “vanilla” Lox is required; this documentation describes the language exactly as it exists inside Mud98.

## Quickstart
1. **Open an entity** via OLC (`redit`, `medit`, `oedit`) and enter the `lox` editor.
2. **Author or paste** the script body (class methods only; no `fun` keyword for methods).
3. **Validate** with `.v`, then **assign** with `@`.
4. **Attach events** via `event set <trigger>` to bind callbacks.
5. **Test in-game** and persist with `asave`.

## Documentation Map
- [Language Guide](language.md) — syntax, data structures, control flow, and idioms unique to Mud98.
- [Runtime Guide](runtime.md) — how scripts compile, how events fire, execution context, and persistence.
- [API Reference](api.md) — native functions, entity methods, globals, commands, and enums exposed to Lox.
- [Recipes](recipes.md) — practical walkthroughs derived from the Faladrin Forest tutorial and other examples.

Use this index as your landing page; each section cross-links to relevant source files and builder docs so both humans and AI tooling can navigate the scripting ecosystem efficiently.
