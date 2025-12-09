# Mud98

This is a clearinghouse of selected topics.

## Worldcrafting from Scratch
Mud98 isn't a MUD; it's a codebase. These docs outline how to take Mud98 and, with no code whatsoever (expect for in-game scripting) build a customized MUD that plays by _your_ rules.

1. [Part 1 &mdash; Getting Started](wb-01-getting-started.md) 

    Ok, so you just downloaded `Mud98`. Now what? These are the first steps to building your own MUD. Compilation, configuration, and maintenance.

2. [Part 2 &mdash; New Beginnings](wb-02-new-beginnings.md)

    Create your new world starting with the beginning: by crafting new starting zones by race or class, and build a narrative introductory experience or new players. Wire up events with the Lox scripting language.
   - [Click here](wb-02-new-beginnings-old.md) for the "legacy" version, using MobProgs instead of Lox. This version is still valid and working _(albeit a little complicated with all the invisible "triggerbots")_.

## Engineering reference

- [Project Map](project-map.md): Repo layout, build/run/test at a glance.
- [Coding Guide](coding-guide.md): house rules and conventions.
- [Glossary](glossary.md): common domain terms.
- [Engineering Notes](engineering-notes.md): tips for safe changes and AI usage.
- [Unit Test Guide](unit-test-guide.md): how to add tests consistent with `src/tests/`.
- [JSON Schemas](json/index.md): schema used when saving JSON files.
- [Mud98 Lox](lox/index.md): reference material for Mud98's flavor of the Lox scripting language.
