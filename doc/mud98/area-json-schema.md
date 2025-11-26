# Area JSON Schema (builder-friendly)

This describes the JSON area format produced/consumed when `HAVE_JSON_AREAS` is enabled. Fields omitted when at default are treated as defaults on load.

Top-level object
- `formatVersion`: integer, currently `1`.
- `areadata`: object (see below).
- Sections as arrays (any may be absent/empty): `rooms`, `mobiles`, `objects`, `resets`, `shops`, `specials`, `mobprogs`, `quests`, `helps`, `factions`.

`areadata`
- `version`: int (`AREA_VERSION`, typically 2).
- `name`: string.
- `builders`: string (comma-separated names).
- `vnumRange`: `[min,max]` ints.
- `credits`: string.
- `security`: int.
- `sector`: string from `sector_flag_table` (e.g., `inside`).
- `lowLevel` / `highLevel`: ints.
- `reset`: int threshold; `alwaysReset`: bool.
- `instType`: `"multi"` only when not single (single is implied).

`rooms` (array of objects)
- Required: `vnum` int, `name` string, `description` string.
- Optional: `roomFlags` [string flags], `sectorType` string, `manaRate` int (default 100), `healRate` int (100), `clan` int, `owner` string.
- `exits`: array of exit objects; omit if none.
- `extraDescs`: array of `{ keyword, description }`; omit if none.

Exit object
- `dir`: string direction name.
- `toVnum`: int.
- Optional: `key` int (omit if 0), `flags` [string flags], `description` string, `keyword` string.

`mobiles` (array)
- Required text: `vnum`, `name`, `shortDescr`, `longDescr`, `description`.
- Flags as string arrays: `actFlags`, `affectFlags`, `offFlags`, `immFlags`, `resFlags`, `vulnFlags`, `formFlags`, `partFlags`.
- Stats: `alignment`, `group`, `level`, `hitroll`, `damType` string, `startPos` string, `defaultPos` string, `sex` string, `wealth`, `size` string, `material` string, `factionVnum` int.
- Dice objects (omit defaults): `hitDice`, `manaDice`, `damageDice` each `{ number, type, bonus? }`.
- `ac`: `{ pierce, bash, slash, exotic }` (values are tens, e.g., -20).
- Attacks: `damageNoun` string, `offensiveSpell` string, `mprogFlags` flags array.
- Affects on mob prototypes are not serialized (mob prototypes have no affect list).

`objects` (array)
- Required text: `vnum`, `name`, `shortDescr`, `description`, `material`, `itemType` string.
- Flags as string arrays: `extraFlags`, `wearFlags`.
- Generic `values`: array of 5 ints emitted only when no typed block is used.
- Common scalar fields (omit if default 0): `level`, `weight`, `cost`, `condition`.
- `extraDescs`: array `{ keyword, description }` if present.
- `affects`: array (see Affect below) if present.
- Typed blocks (emit when applicable; omit when empty):
  - `weapon`: `{ class, dice:[number,type], damageType, flags:[...] }`.
  - `container`: `{ capacity, flags:[...], keyVnum, maxWeight, weightMult }`.
  - `light`: `{ hours }`.
  - `armor`: `{ acPierce, acBash, acSlash, acExotic }`.
  - `drink` / `fountain`: `{ capacity?, remaining?, liquid, poisoned? }`.
  - `food`: `{ foodHours, fullHours?, poisoned? }`.
  - `money`: `{ gold, silver }`.
  - `wand` (and staff): `{ level, totalCharges, chargesLeft, spell }`.
  - `spells` (scroll/potion/pill): `{ level, spell1?, spell2?, spell3?, spell4? }`.
  - `portal`: `{ charges?, exitFlags:[...], portalFlags:[...], toVnum? }`.
  - `furniture`: `{ slots, weight, flags:[...], healBonus?, manaBonus?, maxPeople? }`.
  - `drink` also used for `ITEM_DRINK_CON`; `fountain` for `ITEM_FOUNTAIN`.

Affect object (used for object affects)
- `type`: spell/skill name, or integer when unknown.
- `where`: string from `apply_types`.
- `location`: string from `apply_flag_table` (or integer if unknown).
- `level`, `duration`, `modifier`: ints.
- `bitvector`: [string flags] from `bitvector_type[where]`, or `bitvectorValue` int fallback.

`resets` (array)
- Each reset has `commandName` (preferred) or single-letter `command`.
- Common semantic fields by type:
  - `loadMob`: `mobVnum`, `maxInArea` (arg2), `roomVnum` (arg3), `maxInRoom` (arg4, optional).
  - `placeObj`: `objVnum`, `roomVnum`.
  - `putObj`: `objVnum`, `count` (optional arg2), `containerVnum`, `maxInContainer` (arg4).
  - `giveObj`: `objVnum`.
  - `equipObj`: `objVnum`, `wearLoc` string.
  - `setDoor`: `roomVnum`, `direction` string, `state` int.
  - `randomizeExits`: `roomVnum`, `exits` int.
- For compatibility, generic `arg1..arg4` and `command` may appear; `roomVnum` is always emitted for context.

`shops` (array)
- `{ keeper, buyTypes:[int*5], profitBuy, profitSell, openHour, closeHour }`.

`specials` (array)
- `{ mobVnum, spec }` where `spec` is the specproc name.

`mobprogs` (array)
- `{ vnum, code }`.

`quests` (array)
- Mirrors ROM save: `{ vnum, minLevel, maxLevel, race?, class?, time?, creator?, description?, objectives: [...], rewards: [...] }` (structure follows existing `save_quests`).

`helps` (array)
- `{ level, keyword, text }`. Empty helps array omitted.

`factions` (array)
- `{ vnum, name, defaultStanding, allies:[vnum...], opposing:[vnum...] }`.

Notes and defaults
- Flags are stored as arrays of flag names; empty arrays are omitted.
- Sector, wear locations, positions, sizes, sexes, damage types, etc. are stored as names (not numbers).
- Numeric fields omitted = default (e.g., healRate/manaRate 100, hours 0, key 0).
- Exits omitted entirely when a room has none; keys omitted when 0; empty `exits`/`affects`/`extraDescs` removed for brevity.
- `instType` omitted means single-instance.
- `formatVersion` lets the format evolve; additive changes should remain backward compatible.

Reference: flag/enumeration values (strings)
- Sectors (area/room): `inside`, `city`, `field`, `forest`, `hills`, `mountain`, `water_swim`, `water_noswim`, `unused`, `air`, `desert`, `caves`, `road`.
- Room flags: `dark`, `no_mob`, `indoors`, `peace`, `soundproof`, `no_recall`, `no_quest`, `private`, `safe`, `solitary`, `pet_shop`, `no_donate`, `no_flee`, `law`, `imp_only`, `gods_only`, `heroes_only`, `newbies_only`, `no_magic`, `no_pkill`, `no_teleport`, `hotel`.
- Exit flags: `isdoor`, `closed`, `locked`, `pickproof`, `nopass`, `easy`, `hard`, `infuriating`, `noclose`, `nolock`, `nopassdoor`, `secret`.
- Wear locations (equip): `light`, `finger_l`, `finger_r`, `neck_1`, `neck_2`, `body`, `head`, `legs`, `feet`, `hands`, `arms`, `shield`, `about`, `waist`, `wrist_l`, `wrist_r`, `wield`, `hold`, `float`, `tattoo`.
- Wear flags (where an item can be worn): `take`, `finger`, `neck`, `body`, `head`, `legs`, `feet`, `hands`, `arms`, `shield`, `about`, `waist`, `wrist`, `wield`, `hold`, `float`, `tattoo`.
- Extra flags (objects): `glow`, `hum`, `dark`, `lock`, `evil`, `invis`, `magic`, `nodrop`, `bless`, `antigood`, `antievil`, `antineutral`, `noremove`, `inventory`, `nopurge`, `rot_death`, `vis_death`, `nonmetal`, `nolocate`, `meltdrop`, `hadtimer`, `sellextract`, `burnproof`, `nouncurse`.
- Weapon class: `exotic`, `sword`, `dagger`, `spear`, `mace`, `axe`, `flail`, `whip`, `polearm`.
- Weapon flags (`weapon` block): `flaming`, `frost`, `vampiric`, `sharp`, `vorpal`, `two_handed`, `shocking`, `poison`, `plague`.
- Container flags: `closeable`, `pickproof`, `closed`, `locked`, `put_on`.
- Portal flags: `normal_exit`, `no_curse`, `go_with`, `buggy`, `random`, `stay_in`, `no_flee`.
- Exit flags (portal): same as exit flags above.
- Furniture flags: `stand_at`, `stand_on`, `stand_in`, `sit_at`, `sit_on`, `sit_in`, `rest_at`, `rest_on`, `rest_in`, `sleep_at`, `sleep_on`, `sleep_in`, `put_at`, `put_on`, `put_in`, `put_inside`.
- Liquids: `water`, `beer`, `wine`, `ale`, `darkale`, `whisky`, `lemonade`, `firebreather`, `localbrew`, `slime`, `milk`, `tea`, `coffee`, `blood`, `saltwater`, `coke`, `rootbeer`, `chocolate`, `champagne`, `mead`.
- Positions: `dead`, `mortal`, `incapacitated`, `stunned`, `sleeping`, `resting`, `sitting`, `fighting`, `standing`.
- Sizes: `tiny`, `small`, `medium`, `large`, `huge`, `giant`.
- Sex: `neutral`, `male`, `female`.
- Damage types (weapon/spell nouns): `hit`, `slice`, `stab`, `slash`, `whip`, `claw`, `blast`, `pound`, `crush`, `grep`, `bite`, `pierce`, `suction`, `beating`, `digestion`, `charge`, `chop`, `sting`, `smash`, `shockingbite`, `flame`, `chill`, `lightning`, `acid`.
- Act flags (mobiles): `npc`, `sentinel`, `scavenger`, `aggressive`, `stay_area`, `wimpy`, `pet`, `train`, `practice`, `undead`, `cleric`, `mage`, `thief`, `warrior`, `noalign`, `nopurge`, `outdoors`, `indoors`, `healer`, `gain`, `update_always`, `changer`.
- Affect flags (mobiles): common names include `blind`, `invisible`, `detect_evil`, `detect_invis`, `detect_magic`, `detect_hidden`, `detect_good`, `sanctuary`, `faerie_fire`, `infrared`, `curse`, `poison`, `protect_evil`, `protect_good`, `sneak`, `hide`, `sleep`, `charm`, `flying`, `pass_door`, `haste`, `calm`, `plague`, `weaken`, `dark_vision`, `berserk`, `swim`, `regeneration`, `slow`.
- Off/imm/res/vuln flags (mobiles): use flag names from the tables—e.g., off: `backstab`, `berserk`, `disarm`, `dodge`, `fast`, `kick`, `parry`, `rescue`, `tail`, `trip`, `crush`, `assist_all`, `assist_vnum`, `assist_race`, `assist_align`, `assist_guard`, `assist_vnum`; imm/res/vuln: `summon`, `charm`, `magic`, `weapon`, `bash`, `pierce`, `slash`, `fire`, `cold`, `lightning`, `acid`, `poison`, `negative`, `holy`, `energy`, `mental`, `disease`, `drowning`, `light`, `sound`, `wood`, `silver`, `iron`.
- Form/part flags (mobiles): forms like `edible`, `poison`, `magical`, `instant_decay`, `other`, `animal`, `sentient`, `undead`, `construct`, `mist`, `intangible`, `biped`, `centaur`, `insect`, `spider`, `crustacean`, `worm`, `blob`; parts like `head`, `arms`, `legs`, `heart`, `brains`, `guts`, `hands`, `feet`, `fingers`, `ear`, `eye`, `long_tongue`, `eyestalks`, `tentacles`, `fins`, `wings`, `tail`, `claws`, `fangs`, `horns`, `scales`, `tusks`.
- Wear locations for equips: use `wearLoc` strings above.
- Apply locations (affects): names from `apply_flag_table`, e.g., `strength`, `dexterity`, `intelligence`, `wisdom`, `constitution`, `sex`, `level`, `age`, `height`, `weight`, `mana`, `hit`, `move`, `gold`, `experience`, `ac`, `hitroll`, `damroll`, `saves`, `savingpara`, `savingrod`, `savingpetri`, `savingbreath`, `savingspell`, `spell_affect`.
- Event triggers (Lox events; `events` entries use these names): `ACT`, `ATTACKED`, `BRIBE`, `DEATH`, `ENTRY`, `FIGHT`, `GIVE`, `GREET`, `GRALL`, `HPCNT`, `RANDOM`, `SPEECH`, `EXIT`, `EXALL`, `DELAY` (unused in events, mobprog only), `SURR`, `LOGIN`, `GIVEN`, `TAKEN`, `DROPPED`.
- Scripts/events: `rooms`, `mobiles`, and `objects` may each include `loxScript` (string source) and `events` (array of `{ trigger, callback, criteria? }`, where `callback` names a function defined in `loxScript`). Class names/wrappers are derived at load time from entity type + VNUM, so they are not stored. When absent, scripting is simply disabled for that entity.
