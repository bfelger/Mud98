# Theme JSON Schema

Files listed in `themes_file` inside `mud98.cfg` use this format when their
extension is `.json`; any other extension falls back to the ROM-OLC loader.
`formatVersion` is currently `1`.

Top-level object
- `formatVersion`: integer (must be `1`).
- `themes`: array of theme objects (at least one entry is required).

Theme object fields
- `name`: string (required).
- `banner`: string (optional; omitted when empty).
- `mode`: string describing the overall color mode; accepted values are
  `ansi`, `256`, `rgb`, and `palette`. Omit to keep the compiled default.
- `paletteMax`: integer indicating how many palette slots are in use (0–16);
  missing entries leave the current value unchanged.
- `type`: string describing the theme type (`system`, `system_copy`, or
  `custom`). JSON inputs always end up registered as `system`, but the field
  is preserved for completeness.
- `public`: boolean; true marks the theme as publicly shareable (system
  themes load as non-public regardless of the stored value).
- `palette`: array of palette entries. Each entry is an object with:
  - `index`: integer slot (0–15).
  - `mode`: string (same set as `mode`).
  - `code`: array of integers whose length depends on the mode:
    - `ansi`: two integers (light flag 0/1, color index 0–7).
    - `256`: one integer (0–255).
    - `rgb`: three integers (red, green, blue 0–255).
    - `palette`: one integer referencing another palette slot.
- `channels`: object keyed by channel name (`text`, `say_text`, `room_title`,
  etc.; see `color_slot_entries`). Each entry is another `{ mode, code }`
  color object following the same shape as the palette entries.

Defaults and loader behavior
- Missing scalars (`mode`, `paletteMax`, `public`, `type`) keep their existing
  values from the compiled default.
- `palette` entries referencing invalid indices are ignored; unspecified slots
  retain their previous colors.
- `code` arrays may omit trailing values; any unspecified bytes default to `0`.
- Channels not listed in the JSON keep their current colors. Unknown channel
  keys are skipped for forward compatibility.
- Any additional/unrecognized fields on either the theme or palette objects
  are ignored.
- During load, all JSON-defined themes are registered as system themes. The
  `public` flag is forced to false and `type` is rewritten to
  `COLOR_THEME_TYPE_SYSTEM` before registration, ensuring runtime copies track
  changes correctly.

Notes
- Palette entries include their `index` alongside the color data for clarity.
- Channel colors are keyed by name to avoid hard-coding indices, and the same
  color object structure is reused for palette and channel definitions.
