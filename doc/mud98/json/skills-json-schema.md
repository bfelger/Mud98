# Skill JSON Schema

Selected via `skills_file` in `mud98.cfg` (files ending with `.json` use this format). `formatVersion` is `1`.

Top-level
- `formatVersion`: int (1)
- `skills`: array of skill objects

Skill object
- `name`: string (required)
- `levels`: object mapping class names to the level required to learn the skill. Missing classes default to `DEFAULT_SKILL_LEVEL`.
- `ratings`: object mapping class names to the training cost. Missing classes default to `DEFAULT_SKILL_RATING`.
- `spell`: optional string naming the ROM spell function (e.g. `spell_fireball`). Defaults to `spell_null`.
- `loxSpell`: optional string naming a global Lox callable (looked up in the VM at load time). This must be just the callable's name; inline script bodies are not supported.
- `target`: string from `target_table` (`tar_ignore`, `tar_char_offensive`, etc.). Defaults to `tar_ignore`.
- `minPosition`: string from `position_table`. Defaults to `dead`.
- `gsn`: optional string naming the `gsn_*` entry associated with the skill.
- `slot`: int slot number for object casting (defaults to `0`).
- `minMana`: int (defaults to `0`).
- `beats`: int lag (defaults to `0`).
- `nounDamage`, `msgOff`, `msgObj`: optional message strings.

Compatibility notes
- Unknown fields are ignored.
- Class maps accept either full class names or unambiguous prefixes.
- Lox callbacks must already exist when the file is loaded; unknown names are reported and skipped.
