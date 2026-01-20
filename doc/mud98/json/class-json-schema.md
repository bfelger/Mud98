# Class JSON Schema

Selected via `classes_file` in `mud98.cfg` (extension `.json` uses this format; otherwise ROM text loader is used). `formatVersion` is `1`.

Top-level
- `formatVersion`: int (1)
- `classes`: array of class objects

Class object
- `name`: string (required).
- `whoName`: string (optional, defaults to empty string).
- `baseGroup`: string (optional, defaults to compiled base in `class_table` or `NULL` if not defined).
- `defaultGroup`: string (optional, same default behavior as `baseGroup`).
- `weaponVnum`: int (optional, defaults to `0` if unspecified).
- `armorProf`: armor proficiency (`old_style`, `cloth`, `light`, `medium`, `heavy`). Optional; omitted means old-style armor handling.
- `guilds`: array of exactly `MAX_GUILD` VNUMs; missing entries default to `0`. Shorter arrays are padded with zero during load.
- `primeStat`: stat name (`str`, `int`, `wis`, `dex`, `con`). Optional; omitting leaves the compiled default prime stat.
- `skillCap`: int (optional, defaults to compiled value or `0`).
- `thac0_00`: int (defaults to compiled value/0).
- `thac0_32`: int (defaults to compiled value/0).
- `hpMin`: int (defaults to compiled value/0).
- `hpMax`: int (defaults to compiled value/0).
- `manaUser`: bool (optional, defaults to `false` meaning no mana gain).
- `startLoc`: int VNUM (optional, defaults to `0`).
- `titles`: array of `(MAX_LEVEL+1)` entries, each `[male,female]`. Omit individual strings to keep them empty.

Defaults/compatibility
- Unknown/missing scalar fields leave the zeroed/compiled defaults from ROM; the loader only overwrites values that appear in JSON.
- `primeStat` accepts the stat name; if omitted or unrecognized the previous value is left untouched.
- `guilds` shorter than `MAX_GUILD` are padded with zeros; longer arrays are truncated.
- Unknown fields are ignored to allow forward compatibility.
