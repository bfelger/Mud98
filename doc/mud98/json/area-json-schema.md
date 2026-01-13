# Area JSON Schema (builder-friendly)

This describes the JSON area format. Fields omitted when at default are treated as defaults on load.

Top-level object
- `formatVersion`: integer, currently `1`.
- `areadata`: object (see below).
- Sections as arrays (any may be absent/empty): `rooms`, `mobiles`, `objects`, `resets`, `shops`, `specials`, `mobprogs`, `quests`, `helps`, `factions`.
- Builder metadata (optional): `storyBeats`, `checklist`.

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

`storyBeats` (array, optional)
- `{ title, description }` entries capturing narrative beats for consistency across area.

`checklist` (array, optional)
- `{ title, status, description? }` where `status` is `todo`, `inProgress`, or `done` (numeric fallback 0/1/2 accepted). Description is optional and usually omitted; titles act as the checklist line-items.

`rooms` (array of objects)
- Required: `vnum` int, `name` string, `description` string.
- Optional: `roomFlags` [string flags] (defaults to none), `sectorType` string (defaults to `inside`), `manaRate` int (default `100`), `healRate` int (`100`), `clan` int (`0`), `owner` string (empty string by default).
- `exits`: array of exit objects; omit if none.
- `extraDescs`: array of `{ keyword, description }`; omit if none.

Exit object
- `dir`: string direction name.
- `toVnum`: int.
- Optional: `key` int (omit if 0), `flags` [string flags], `description` string, `keyword` string.

`mobiles` (array)
- Required text: `vnum`, `name`, `shortDescr`, `longDescr`, `description`, `race`.
- Flags as string arrays (all optional, default to no bits set unless stated otherwise): `actFlags`, `affectFlags`, `offFlags`, `immFlags`, `resFlags`, `vulnFlags`. Use the names from the standard flag tables; omitted arrays imply `0`.
- Body flags: `formFlags` / `partFlags` accept the standard names plus defaults such as `humanoidDefault` and `animalDefault`. Omit either array to inherit the selected race’s `form` / `parts` values.
- Stats: `alignment`, `group`, `level`, `hitroll`, `damType` string (`attack_table` entry), `startPos` string, `defaultPos` string, `sex` string, `wealth`, `size` string, `material` string, `factionVnum` int. Any omitted scalar keeps the ROM default (typically `0`, `standing`, `neutral`, or `size medium`).
- Dice objects (omit defaults): `hitDice`, `manaDice`, `damageDice` each `{ number, type, bonus? }`. Leaving them out keeps the zeroed dice; builders should explicitly set all three numbers for predictable combat.
- `ac`: `{ pierce, bash, slash, exotic }` (values are tens, e.g., `-20`). Omit to keep the default armor array.
- Attacks/spells: `damageNoun`, `offensiveSpell`, and `mprogFlags` (array) are optional; unspecified fields default to empty strings / zero.
- Affects on mob prototypes are not serialized (mob prototypes have no persistent affect list).

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
- Sectors (area/room): `inside`, `city`, `field`, `forest`, `hills`, `mountain`, `swim`, `noswim`, `underground`, `air`, `desert`.
- Room flags: `dark`, `no_mob`, `indoors`, `private`, `safe`, `solitary`, `pet_shop`, `no_recall`, `imp_only`, `gods_only`, `heroes_only`, `newbies_only`, `law`, `nowhere`, `recall`.
- Exit flags: `door`, `closed`, `locked`, `pickproof`, `nopass`, `easy`, `hard`, `infuriating`, `noclose`, `nolock`.
- Wear locations (equip): `in the inventory`, `as a light`, `on the left finger`, `on the right finger`, `around the neck (1)`, `around the neck (2)`, `on the body`, `over the head`, `on the legs`, `on the feet`, `on the hands`, `on the arms`, `as a shield`, `about the shoulders`, `around the waist`, `on the left wrist`, `on the right wrist`, `wielded`, `held in the hands`, `floating nearby`.
- Wear flags (where an item can be worn): `take`, `finger`, `neck`, `body`, `head`, `legs`, `feet`, `hands`, `arms`, `shield`, `about`, `waist`, `wrist`, `wield`, `hold`, `nosac`, `wearfloat`.
- Extra flags (objects): `glow`, `hum`, `dark`, `lock`, `evil`, `invis`, `magic`, `nodrop`, `bless`, `antigood`, `antievil`, `antineutral`, `noremove`, `inventory`, `nopurge`, `rotdeath`, `visdeath`, `nonmetal`, `nolocate`, `meltdrop`, `hadtimer`, `sellextract`, `burnproof`, `nouncurse`.
- Weapon class: `exotic`, `sword`, `dagger`, `spear`, `mace`, `axe`, `flail`, `whip`, `polearm`.
- Weapon flags (`weapon` block): `flaming`, `frost`, `vampiric`, `sharp`, `vorpal`, `twohands`, `shocking`, `poison`.
- Container flags: `closeable`, `pickproof`, `closed`, `locked`, `puton`.
- Portal flags: `normal_exit`, `no_curse`, `go_with`, `buggy`, `random`.
- Exit flags (portal): same as exit flags above.
- Furniture flags: `stand_at`, `stand_on`, `stand_in`, `sit_at`, `sit_on`, `sit_in`, `rest_at`, `rest_on`, `rest_in`, `sleep_at`, `sleep_on`, `sleep_in`, `put_at`, `put_on`, `put_in`, `put_inside`.
- Liquids: `water`, `beer`, `red wine`, `ale`, `dark ale`, `whisky`, `lemonade`, `firebreather`, `local specialty`, `slime mold juice`, `milk`, `tea`, `coffee`, `blood`, `salt water`, `coke`, `root beer`, `elvish wine`, `white wine`, `champagne`, `mead`, `rose wine`, `benedictine wine`, `vodka`, `cranberry juice`, `orange juice`, `absinthe`, `brandy`, `aquavit`, `schnapps`, `icewine`, `amontillado`, `sherry`, `framboise`, `rum`, `cordial`.
- Positions: `dead`, `mortally wounded`, `incapacitated`, `stunned`, `sleeping`, `resting`, `sitting`, `fighting`, `standing`.
- Sizes: `tiny`, `small`, `medium`, `large`, `huge`, `giant`.
- Sex: `none`, `male`, `female`, `either`, `you`.
- Damage types (weapon/spell nouns): `none`, `slice`, `stab`, `slash`, `whip`, `claw`, `blast`, `pound`, `crush`, `grep`, `bite`, `pierce`, `suction`, `beating`, `digestion`, `charge`, `slap`, `punch`, `wrath`, `magic`, `divine`, `cleave`, `scratch`, `peck`, `peckb`, `chop`, `sting`, `smash`, `shbite`, `flbite`, `frbite`, `acbite`, `chomp`, `drain`, `thrust`, `slime`, `shock`, `thwack`, `flame`, `chill`.
- Act flags (mobiles): `npc`, `sentinel`, `scavenger`, `aggressive`, `stay_area`, `wimpy`, `pet`, `train`, `practice`, `undead`, `cleric`, `mage`, `thief`, `warrior`, `noalign`, `nopurge`, `outdoors`, `indoors`, `healer`, `gain`, `update_always`, `changer`.
- Affect flags (mobiles): `blind`, `invisible`, `detect_evil`, `detect_invis`, `detect_magic`, `detect_hidden`, `detect_good`, `sanctuary`, `faerie_fire`, `infrared`, `curse`, `poison`, `protect_evil`, `protect_good`, `sneak`, `hide`, `sleep`, `charm`, `flying`, `pass_door`, `haste`, `calm`, `plague`, `weaken`, `dark_vision`, `berserk`, `swim`, `regeneration`, `slow`.
- Off flags (mobiles): `area_attack`, `backstab`, `bash`, `berserk`, `disarm`, `dodge`, `fade`, `fast`, `kick`, `dirt_kick`, `parry`, `rescue`, `tail`, `trip`, `crush`, `assist_all`, `assist_align`, `assist_race`, `assist_players`, `assist_guard`, `assist_vnum`.
- Imm/res flags (mobiles): `summon`, `charm`, `magic`, `weapon`, `bash`, `pierce`, `slash`, `fire`, `cold`, `lightning`, `acid`, `poison`, `negative`, `holy`, `energy`, `mental`, `disease`, `drowning`, `light`, `sound`, `wood`, `silver`, `iron`.
- Vuln flags (mobiles): `none` plus the imm/res list above.
- Form flags (mobiles): `edible`, `poison`, `magical`, `instant_decay`, `other`, `bleeds`, `animal`, `sentient`, `undead`, `construct`, `mist`, `intangible`, `biped`, `centaur`, `insect`, `spider`, `crustacean`, `worm`, `blob`, `mammal`, `bird`, `reptile`, `snake`, `dragon`, `amphibian`, `fish`, `cold_blood`, `humanoidDefault`, `animalDefault`.
- Part flags (mobiles): `head`, `arms`, `legs`, `heart`, `brains`, `guts`, `hands`, `feet`, `fingers`, `ear`, `eye`, `long_tongue`, `eyestalks`, `tentacles`, `fins`, `wings`, `tail`, `claws`, `fangs`, `horns`, `scales`, `tusks`, `humanoidDefault`, `animalDefault`.
- Wear locations for equips: use `wearLoc` strings above.
- Apply locations (affects): `none`, `strength`, `dexterity`, `intelligence`, `wisdom`, `constitution`, `sex`, `class`, `level`, `age`, `height`, `weight`, `mana`, `hp`, `move`, `gold`, `experience`, `ac`, `hitroll`, `damroll`, `saves`, `savingpara`, `savingrod`, `savingpetri`, `savingbreath`, `savingspell`, `spellaffect`.
- Event triggers (Lox events; `events` entries use these names): `act`, `attacked`, `bribe`, `death`, `entry`, `fight`, `give`, `greet`, `grall`, `hpcnt`, `random`, `speech`, `exit`, `exall`, `delay` (unused in events, mobprog only), `surr`, `login`, `given`, `taken`, `dropped`, `prdstart`, `prdstop`.
- Scripts/events: `rooms`, `mobiles`, and `objects` may each include `loxScript` (string source) and `events` (array of `{ trigger, callback, criteria? }`, where `callback` names a function defined in `loxScript`). Class names/wrappers are derived at load time from entity type + VNUM, so they are not stored. When absent, scripting is simply disabled for that entity.
