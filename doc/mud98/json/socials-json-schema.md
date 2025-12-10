# Social JSON Schema

Selected via `socials_file` in `mud98.cfg` (files ending with `.json` use this format). `formatVersion` is `1`.

Top-level
- `formatVersion`: int (1)
- `socials`: array of social objects

Social object
- `name`: string (required)
- `charNoArg`: message shown to the actor when no target is supplied.
- `othersNoArg`: message shown to the room when no target is supplied.
- `charFound`: message shown to the actor when a target is found.
- `othersFound`: message shown to the room when a target is found.
- `victFound`: message shown to the victim when a target is found.
- `charAuto`: message shown to the actor when the actor targets themselves.
- `othersAuto`: message shown to the room when a social is used on the actor.

All strings may be omitted to keep the original ROM defaults (empty strings). Unknown fields are ignored.
