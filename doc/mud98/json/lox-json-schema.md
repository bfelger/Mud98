# Lox Script JSON Schema

Selected via `lox_file` in `mud98.cfg`. If the filename ends with `.json`, the loader expects this format. `formatVersion` is `1`.

Top-level
- `formatVersion`: int (`1`)
- `scripts`: array of script objects

Script object
- `category`: string folder under `data/scripts/` (optional; defaults to the root scripts directory when omitted)
- `file`: string filename of the `.lox` source file (required)
- `when`: string boot phase, either `pre` (run immediately after loading) or `post` (run during `area_update`), defaults to `pre`

Notes
- Script source code continues to live in `data/scripts/<category>/<file>`; this catalog only records metadata.
- Unknown fields are ignored.
- Case-insensitive comparisons apply for `when`.
