# Tutorial JSON Schema

Selected via `tutorials_file` in `mud98.cfg` (files ending with `.json` use this format). `formatVersion` is `1`.

Top-level
- `formatVersion`: int (1)
- `tutorials`: array of tutorial objects

Tutorial object
- `name`: string (required)
- `blurb`: short description shown in the tutorial list (optional).
- `finish`: message displayed when the tutorial completes (optional).
- `minLevel`: minimum player level to access the tutorial (defaults to `0`).
- `steps`: array of step objects (order matters).

Step object
- `prompt`: text shown to the player at this step (required).
- `match`: optional string that must be entered to advance to the next step (empty or omitted to auto-advance).

Compatibility notes
- Tutorials are stored in the order encountered; the loader does not deduplicate names.
- Unknown fields are ignored.
