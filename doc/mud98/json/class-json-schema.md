# Class JSON Schema

Selected via `classes_file` in `mud98.cfg` (extension `.json` uses this format; otherwise ROM text loader is used). `formatVersion` is `1`.

Top-level
- `formatVersion`: int (1)
- `classes`: array of class objects

Class object
- `name`: string (required)
- `whoName`: string (short tag)
- `baseGroup`: string (base skill group)
- `defaultGroup`: string (default skill group)
- `weaponVnum`: int (starting weapon)
- `guilds`: array of up to `MAX_GUILD` VNUMs (order matches existing guild array)
- `primeStat`: stat name (`str`, `int`, `wis`, `dex`, `con`)
- `skillCap`: int
- `thac0_00`: int
- `thac0_32`: int
- `hpMin`: int
- `hpMax`: int
- `manaUser`: bool (true if gains mana on level)
- `startLoc`: int VNUM
- `titles`: array of `(MAX_LEVEL+1)*2` strings (male/female titles in order); empty strings allowed

Defaults/compatibility
- Unknown/missing fields fall back to code defaults.
- `primeStat` accepts stat name; loader can be extended to accept numeric fallback if needed.
- `guilds` may be shorter than `MAX_GUILD`; missing entries are treated as 0.
- Unknown fields are ignored to allow forward compatibility.
