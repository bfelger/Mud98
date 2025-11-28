# Race JSON Schema

Goal: Optional JSON persistence for `data/races` while keeping the ROM text format fully supported. The file is selected by `races_file` in `mud98.cfg`; `.json` uses this schema, otherwise the legacy text loader is used.

Top-level object
- `formatVersion`: integer, currently `1`.
- `races`: array of race objects.

Race object fields
- `name`: string (required).
- `whoName`: string (short race tag for who list).
- `pc`: bool (true if selectable by players).
- `points`: int (creation point cost).
- `size`: string from mob size table (e.g., `tiny`, `small`, `medium`, `large`, `huge`, `giant`).
- `stats`: object keyed by stat name (`str`, `int`, `wis`, `dex`, `con`) with int values (base stats). Array form is still accepted on load for backward compatibility.
- `maxStats`: object keyed by stat name; array form accepted on load.
- Flags as name arrays:
  - `actFlags` (`act_flag_table`)
  - `affectFlags` (`affect_flag_table`)
  - `offFlags` (`off_flag_table`)
  - `immFlags` (`imm_flag_table`)
  - `resFlags` (`res_flag_table`)
  - `vulnFlags` (`vuln_flag_table`)
  - `formFlags` (`form_flag_table`)
  - `partFlags` (`part_flag_table`)
- `classMult`: object keyed by class name with exp multiplier *100. Array form accepted on load (by class order).
- `startLoc`: int VNUM for default start.
- `classStart`: object keyed by class name with VNUMs for class-specific starts. Array form accepted on load.
- `skills`: array of up to 5 skill names (strings).

Defaults/omissions
- Missing flags imply zero.
- Missing stats/maxStats leave defaults as in code.
- `skills` can be omitted/empty; unused entries are ignored.
- Unknown/omitted size falls back to `medium`.
- `formatVersion` allows future additive changes; keep compatible when possible.

Notes
- Flags use string names; numeric fallback is not emitted but the loader can be extended to accept integers if needed.
- JSON ordering is not significant; loaders ignore unknown keys for forward compatibility.
