# Race JSON Schema

Goal: Optional JSON persistence for `data/races` while keeping the ROM text format fully supported. The file is selected by `races_file` in `mud98.cfg`; `.json` uses this schema, otherwise the legacy text loader is used.

Top-level object
- `formatVersion`: integer, currently `1`.
- `races`: array of race objects.

Race object fields
- `name`: string (required).
- `whoName`: string (optional, defaults to empty string; elided when blank).
- `pc`: bool (optional, defaults to `false`).
- `points`: int (optional, defaults to `0`).
- `size`: string from mob size table (defaults to `medium` when omitted or unknown).
- `stats`: object keyed by stat name (`str`, `int`, `wis`, `dex`, `con`). Omit to keep all base stats at `0`. Loader also accepts legacy arrays of five integers.
- `maxStats`: object keyed by stat name. Omit to keep all max stats at `0`. Array form accepted.
- Flags as name arrays (all optional; omitted arrays default to zero bitfields):
  - `actFlags` (`act_flag_table`)
  - `affectFlags` (`affect_flag_table`)
  - `offFlags` (`off_flag_table`)
  - `immFlags` (`imm_flag_table`)
  - `resFlags` (`res_flag_table`)
  - `vulnFlags` (`vuln_flag_table`)
  - `formFlags` (`form_flag_table`, accepts named defaults such as `humanoidDefault`, `animalDefault`)
  - `partFlags` (`part_flag_table`, accepts named defaults like `humanoidDefault`, `animalDefault`)
- `classMult`: optional object keyed by class name with experience multiplier *100. Any omitted class uses the compiled default of `100`. Loader also accepts arrays ordered by class index.
- `startLoc`: optional VNUM; defaults to `0`.
- `classStart`: optional object keyed by class name with VNUMs for class-specific starts (defaults to `0` per class). Array form accepted.
- `skills`: optional array of up to 5 skill names. Omit or specify fewer entries to leave the unused slots empty.

Defaults/omissions
- Missing flags imply zero; missing `formFlags` / `partFlags` do not automatically add defaults, so include `humanoidDefault`/`animalDefault` when you need them.
- Omitted stats/maxStats keep the zero baseline that legacy ROM provided before racial adjustments.
- `classMult` entries default to 100 (no multiplier). Negative / positive adjustments map directly to ROM’s `%` multipliers.
- `classStart` defaults to 0 (use start rooms on the race object or class).
- `skills` can be omitted/empty; undefined entries are ignored by the loader.
- Unknown/omitted size falls back to `medium`.
- `formatVersion` allows future additive changes; keep compatible when possible.

Notes
- Flags use string names; numeric fallback is not emitted but the loader can be extended to accept integers if needed.
- JSON ordering is not significant; loaders ignore unknown keys for forward compatibility.
