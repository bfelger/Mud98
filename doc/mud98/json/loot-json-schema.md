# Loot JSON Schema (builder-friendly)

This describes the JSON format for loot groups/tables. The same structure is
embedded in area JSON under the `loot` field (without `formatVersion`).

Top-level object (global loot file)
- `formatVersion`: integer, currently `1`.
- `groups`: array of loot group objects.
- `tables`: array of loot table objects.

Loot group object
- `name`: string (unique group name).
- `rolls`: int (default `1`).
- `entries`: array of loot entry objects.

Loot entry object
- `type`: `"item"` or `"cp"` (`cp` = copper pieces).
- `vnum`: int (required for `"item"` entries).
- `minQty`: int (default `1`).
- `maxQty`: int (default `1`).
- `weight`: int (default `100`, used for weighted rolls).

Loot table object
- `name`: string (unique table name).
- `parent`: string (optional parent table name).
- `ops`: array of loot op objects (processed in order).

Loot op object
- `op`: `"use_group"`, `"add_item"`, `"add_cp"`, `"mul_cp"`, `"mul_all_chances"`, `"remove_item"`, or `"remove_group"`.
- `use_group`: `{ op:"use_group", group, chance? }` (`chance` default `100`).
- `add_item`: `{ op:"add_item", vnum, chance?, minQty?, maxQty? }` (defaults: `chance=100`, `minQty=1`, `maxQty=1`).
- `add_cp`: `{ op:"add_cp", chance?, minQty?, maxQty? }` (defaults: `chance=100`, `minQty=1`, `maxQty=1`).
- `mul_cp`: `{ op:"mul_cp", multiplier? }` (`multiplier` default `100`).
- `mul_all_chances`: `{ op:"mul_all_chances", multiplier? }` (`multiplier` default `100`).
- `remove_item`: `{ op:"remove_item", vnum }`.
- `remove_group`: `{ op:"remove_group", group }`.

Notes
- `chance` and `multiplier` values are percentages.
- Parent tables are resolved on load; ops in child tables apply after parent ops.
