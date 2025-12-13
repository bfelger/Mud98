# Command JSON Schema

Selected via `commands_file` in `mud98.cfg` (files ending in `.json` use this format). `formatVersion` is `1`.

Top-level
- `formatVersion`: int (1)
- `commands`: array of command objects

Command object
- `name`: string (required)
- `function`: string name from `command.h` (defaults to `do_nothing` if omitted)
- `position`: string from `position_table` (`dead`, `sleeping`, `standing`, etc.). Defaults to `dead`.
- `level`: int required level for the command (defaults to `0` when missing)
- `log`: string value from `log_flag_table` (`log_normal`, `log_always`, `log_never`). Defaults to `log_normal`.
- `category`: string from `show_flag_table` (e.g., `communication`, `combat`). Defaults to `undef`.
- `loxFunction`: optional string naming a callable defined in `data/scripts/*.lox`. When set, the command ignores `function`/`do_fun` and instead invokes the referenced Lox code with signature `fun command_name(ch, args)`. Leave empty to run the compiled C function.

Compatibility notes
- Unknown fields are ignored.
- All strings are case-insensitive compared to ROM data.
- Missing scalar values fall back to the compiled defaults (`0`, `POS_DEAD`, etc.).
