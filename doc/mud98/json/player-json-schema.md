# Player JSON Schema

Player characters saved in the directory referenced by `player_dir` gain a
`.json` extension whenever `default_format = json` inside `mud98.cfg`. On
login, the persistence layer first probes the configured default format; if
that file does not exist it automatically falls back to the other format.
After a successful fallback load, the character is immediately re-saved in
the default format and the legacy file is deleted so subsequent loads do not
need the migration path.

JSON player files always store a single top-level object with these fields:

- `formatVersion`: integer that must match the compiled constant (currently
  `1`).
- `player`: object describing base character state (required).
- `inventory`: array of carried/worn objects (required, may be empty).
- `pet`: optional object representing the player’s bonded pet.
- `questLog`: array describing tracked quests (required, may be empty).

## Player Object

Identity and metadata:

- `name`: string; canonical player name.
- `id`: integer character ID.
- `logoffTime`: integer unix timestamp written at save time.
- `version`: integer ROM character data revision (defaults to `5`).

Text and UI:

- `shortDescr`, `longDescr`, `description`: strings; omitted when empty.
- `prompt`: optional string; omitted when the default prompt is in use.

Character sheet:

- `race`: string race name (required).
- `clan`: optional string clan name.
- `sex`, `class`, `level`, `trust`, `security`, `played`, `screenLines`,
  `recall`, `room`, `exp`, `actFlags`, `affectFlags`, `commFlags`, `wiznet`,
  `invisLevel`, `incogLevel`, `position`, `practice`, `train`, `savingThrow`,
  `alignment`, `hitroll`, `damroll`, `wimpy`: integers mirroring ROM-OLC
  save fields (missing entries keep their in-memory defaults).
- `hmv`: object with `hit`, `maxHit`, `mana`, `maxMana`, `move`, `maxMove`.
- `money`: object with `gold`, `silver`, `copper`.
- `armor`: array of four integers ordered `pierce`, `bash`, `slash`, `exotic`.
- `permStats` / `modStats`: arrays of five integers ordered
  `str`, `int`, `wis`, `dex`, `con`.
- `affects`: array of affect objects. Each affect includes:
  - `skill`: string skill name (required; unknown skills are skipped on load).
  - `where`, `level`, `duration`, `modifier`, `location`, `bitvector`: integers.

- `pcdata`: object containing player-only data (detailed below).

## PCData Object

- `security`, `recall`, `points`, `trueSex`, `lastLevel`, `permHit`,
  `permMana`, `permMove`: integers.
- `conditions`: array of `COND_MAX` integers ordered
  `drunk`, `full`, `thirst`, `hunger`.
- `lastNote`, `lastIdea`, `lastPenalty`, `lastNews`, `lastChanges`: integers
  storing the last-read timestamps for each board.
- `currentTheme`: optional string with the active color theme name.
- `tutorial`: optional object `{ "name": string, "step": integer }`.
- `pwdDigest`: string hex-encoded password digest (required for logins).
- `bamfin`, `bamfout`, `title`: strings (omitted when empty).
- `themeConfig`: object with booleans `hide24bit`, `hide256`, `xterm`,
  `hideRgbHelp`.
- `aliases`: array of `{ "alias": string, "sub": string }` entries
  (maximum `MAX_ALIAS` entries).
- `skills`: array of `{ "skill": string, "value": integer }` entries.
- `groups`: array of strings naming every known skill group.
- `reputations`: array of `{ "vnum": integer, "value": integer }` faction
  reputation entries.
- `personalThemes`: array of color theme objects using the schema defined in
  [theme-json-schema.md](theme-json-schema.md).

Missing values keep the current in-memory defaults; `pcdata` only overwrites
fields that appear in JSON so older files can be forward-loaded.

## Inventory Array

Each entry is a full snapshot of a carried or worn object. Fields mirror
ROM-OLC object saves:

- `vnum`: integer template reference. Missing or unknown vnums create a blank
  object that still accepts other overrides.
- `enchanted`: boolean.
- `name`, `shortDescr`, `description`, `material`: strings.
- `extraFlags`, `wearFlags`, `itemType`, `weight`, `condition`, `wearLoc`,
  `level`, `timer`, `cost`: integers.
- `values`: array of five integers (`value[0..4]`).
- `affects`: same structure as player affects.
- `extraDescriptions`: array of `{ "keyword": string, "description": string }`.
- `spells`: array describing imbued spells. Each entry has `slot` (0–4) and
  `name` (skill name). Unsupported skills are ignored.
- `contents`: array of nested object entries (containers recurse naturally).

Load order preserves wear slots and nesting by replaying the serialized
tree; unknown fields are ignored for forward compatibility.

## Pet Object

Optional. Stored fields mirror `player` plus a strict subset of stats:

- Identity, descriptions, `race`, `clan`, `sex`, `level`, `hmv`, `gold`,
  `silver`, `copper`, `exp`, `actFlags`, `affectFlags`, `commFlags`,
  `position`, `savingThrow`, `alignment`, `hitroll`, `damroll`,
  `armor`, `permStats`, `modStats`, and `affects`.
- `vnum` selects the prototype. Unknown vnums fall back to `MOB_VNUM_FIDO`
  but still apply overrides so custom pets survive area edits.

Pets are only saved when they are present in the same room as the player
during logout.

## Quest Log Array

Each entry records a tracked quest:

- `vnum`: quest identifier (required; unknown quests are skipped on load).
- `progress`: integer progress counter.
- `state`: integer `QuestState` enum (defaults to `QSTAT_ACCEPTED`).

## Loader Behavior

- Unknown or malformed sections are skipped instead of aborting the load so
  players can survive schema evolutions.
- Arrays may omit trailing elements; any unspecified slots retain their prior
  runtime values.
- Additional fields are ignored, enabling incremental rollouts.
- Any file whose `formatVersion` does not match the compiled constant is
  rejected as `PERSIST_ERR_UNSUPPORTED`.
