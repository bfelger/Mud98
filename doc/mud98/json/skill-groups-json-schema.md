# Skill Group JSON Schema

Selected via `groups_file` in `mud98.cfg` (files ending with `.json` use this format). `formatVersion` is `1`.

Top-level
- `formatVersion`: int (1)
- `groups`: array of group objects

Group object
- `name`: string (required)
- `ratings`: object mapping class names to the group cost. Missing classes default to `DEFAULT_SKILL_RATING`.
- `skills`: array of up to `MAX_IN_GROUP` skill names. Entries beyond `MAX_IN_GROUP` are ignored; missing entries are left empty.

Compatibility notes
- Unknown properties are ignored.
- Class names are matched case-insensitively.
