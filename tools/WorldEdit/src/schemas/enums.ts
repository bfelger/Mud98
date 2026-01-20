import { z } from "zod";

export const sectors = [
  "inside",
  "city",
  "field",
  "forest",
  "hills",
  "mountain",
  "swim",
  "noswim",
  "underground",
  "air",
  "desert"
] as const;

export const roomFlags = [
  "dark",
  "no_mob",
  "indoors",
  "private",
  "safe",
  "solitary",
  "pet_shop",
  "no_recall",
  "imp_only",
  "gods_only",
  "heroes_only",
  "newbies_only",
  "law",
  "nowhere",
  "recall"
] as const;

export const exitFlags = [
  "door",
  "closed",
  "locked",
  "pickproof",
  "nopass",
  "easy",
  "hard",
  "infuriating",
  "noclose",
  "nolock"
] as const;

export const directions = [
  "north",
  "east",
  "south",
  "west",
  "up",
  "down"
] as const;

export const wearLocations = [
  "in the inventory",
  "as a light",
  "on the left finger",
  "on the right finger",
  "around the neck (1)",
  "around the neck (2)",
  "on the body",
  "over the head",
  "on the legs",
  "on the feet",
  "on the hands",
  "on the arms",
  "as a shield",
  "about the shoulders",
  "around the waist",
  "on the left wrist",
  "on the right wrist",
  "wielded",
  "held in the hands",
  "floating nearby"
] as const;

export const wearFlags = [
  "take",
  "finger",
  "neck",
  "body",
  "head",
  "legs",
  "feet",
  "hands",
  "arms",
  "shield",
  "about",
  "waist",
  "wrist",
  "wield",
  "hold",
  "nosac",
  "wearfloat"
] as const;

export const extraFlags = [
  "glow",
  "hum",
  "dark",
  "lock",
  "evil",
  "invis",
  "magic",
  "nodrop",
  "bless",
  "antigood",
  "antievil",
  "antineutral",
  "noremove",
  "inventory",
  "nopurge",
  "rotdeath",
  "visdeath",
  "nonmetal",
  "nolocate",
  "meltdrop",
  "hadtimer",
  "sellextract",
  "burnproof",
  "nouncurse"
] as const;

export const weaponClasses = [
  "exotic",
  "sword",
  "dagger",
  "spear",
  "mace",
  "axe",
  "flail",
  "whip",
  "polearm",
  "skinningknife",
  "miningpick",
  "smithinghammer"
] as const;

export const weaponFlags = [
  "flaming",
  "frost",
  "vampiric",
  "sharp",
  "vorpal",
  "twohands",
  "shocking",
  "poison"
] as const;

export const containerFlags = [
  "closeable",
  "pickproof",
  "closed",
  "locked",
  "puton"
] as const;

export const portalFlags = [
  "normal_exit",
  "no_curse",
  "go_with",
  "buggy",
  "random"
] as const;

export const furnitureFlags = [
  "stand_at",
  "stand_on",
  "stand_in",
  "sit_at",
  "sit_on",
  "sit_in",
  "rest_at",
  "rest_on",
  "rest_in",
  "sleep_at",
  "sleep_on",
  "sleep_in",
  "put_at",
  "put_on",
  "put_in",
  "put_inside"
] as const;

export const liquids = [
  "water",
  "beer",
  "red wine",
  "ale",
  "dark ale",
  "whisky",
  "lemonade",
  "firebreather",
  "local specialty",
  "slime mold juice",
  "milk",
  "tea",
  "coffee",
  "blood",
  "salt water",
  "coke",
  "root beer",
  "elvish wine",
  "white wine",
  "champagne",
  "mead",
  "rose wine",
  "benedictine wine",
  "vodka",
  "cranberry juice",
  "orange juice",
  "absinthe",
  "brandy",
  "aquavit",
  "schnapps",
  "icewine",
  "amontillado",
  "sherry",
  "framboise",
  "rum",
  "cordial"
] as const;

export const positions = [
  "dead",
  "mortally wounded",
  "incapacitated",
  "stunned",
  "sleeping",
  "resting",
  "sitting",
  "fighting",
  "standing"
] as const;

export const sizes = ["tiny", "small", "medium", "large", "huge", "giant"] as const;

export const sexes = ["none", "male", "female", "either", "you"] as const;

export const damageTypes = [
  "none",
  "slice",
  "stab",
  "slash",
  "whip",
  "claw",
  "blast",
  "pound",
  "crush",
  "grep",
  "bite",
  "pierce",
  "suction",
  "beating",
  "digestion",
  "charge",
  "slap",
  "punch",
  "wrath",
  "magic",
  "divine",
  "cleave",
  "scratch",
  "peck",
  "peckb",
  "chop",
  "sting",
  "smash",
  "shbite",
  "flbite",
  "frbite",
  "acbite",
  "chomp",
  "drain",
  "thrust",
  "slime",
  "shock",
  "thwack",
  "flame",
  "chill"
] as const;

export const actFlags = [
  "npc",
  "sentinel",
  "scavenger",
  "aggressive",
  "stay_area",
  "wimpy",
  "pet",
  "train",
  "practice",
  "undead",
  "cleric",
  "mage",
  "thief",
  "warrior",
  "noalign",
  "nopurge",
  "outdoors",
  "indoors",
  "healer",
  "gain",
  "update_always",
  "changer"
] as const;

export const affectFlags = [
  "blind",
  "invisible",
  "detect_evil",
  "detect_invis",
  "detect_magic",
  "detect_hidden",
  "detect_good",
  "sanctuary",
  "faerie_fire",
  "infrared",
  "curse",
  "poison",
  "protect_evil",
  "protect_good",
  "sneak",
  "hide",
  "sleep",
  "charm",
  "flying",
  "pass_door",
  "haste",
  "calm",
  "plague",
  "weaken",
  "dark_vision",
  "berserk",
  "swim",
  "regeneration",
  "slow"
] as const;

export const offFlags = [
  "area_attack",
  "backstab",
  "bash",
  "berserk",
  "disarm",
  "dodge",
  "fade",
  "fast",
  "kick",
  "dirt_kick",
  "parry",
  "rescue",
  "tail",
  "trip",
  "crush",
  "assist_all",
  "assist_align",
  "assist_race",
  "assist_players",
  "assist_guard",
  "assist_vnum"
] as const;

export const immFlags = [
  "summon",
  "charm",
  "magic",
  "weapon",
  "bash",
  "pierce",
  "slash",
  "fire",
  "cold",
  "lightning",
  "acid",
  "poison",
  "negative",
  "holy",
  "energy",
  "mental",
  "disease",
  "drowning",
  "light",
  "sound",
  "wood",
  "silver",
  "iron"
] as const;

export const resFlags = [...immFlags] as const;

export const vulnFlags = ["none", ...immFlags] as const;

export const formFlags = [
  "edible",
  "poison",
  "magical",
  "instant_decay",
  "other",
  "bleeds",
  "animal",
  "sentient",
  "undead",
  "construct",
  "mist",
  "intangible",
  "biped",
  "centaur",
  "insect",
  "spider",
  "crustacean",
  "worm",
  "blob",
  "mammal",
  "bird",
  "reptile",
  "snake",
  "dragon",
  "amphibian",
  "fish",
  "cold_blood",
  "humanoidDefault",
  "animalDefault"
] as const;

export const partFlags = [
  "head",
  "arms",
  "legs",
  "heart",
  "brains",
  "guts",
  "hands",
  "feet",
  "fingers",
  "ear",
  "eye",
  "long_tongue",
  "eyestalks",
  "tentacles",
  "fins",
  "wings",
  "tail",
  "claws",
  "fangs",
  "horns",
  "scales",
  "tusks",
  "humanoidDefault",
  "animalDefault"
] as const;

export const skillTargets = [
  "tar_ignore",
  "tar_char_offensive",
  "tar_char_defensive",
  "tar_char_self",
  "tar_obj_inv",
  "tar_obj_char_def",
  "tar_obj_char_off"
] as const;

export const applyLocations = [
  "none",
  "strength",
  "dexterity",
  "intelligence",
  "wisdom",
  "constitution",
  "sex",
  "class",
  "level",
  "age",
  "height",
  "weight",
  "mana",
  "hp",
  "move",
  "gold",
  "experience",
  "ac",
  "hitroll",
  "damroll",
  "saves",
  "savingpara",
  "savingrod",
  "savingpetri",
  "savingbreath",
  "savingspell",
  "spellaffect"
] as const;

export const eventTriggers = [
  "act",
  "attacked",
  "bribe",
  "death",
  "entry",
  "fight",
  "give",
  "greet",
  "grall",
  "hpcnt",
  "random",
  "speech",
  "exit",
  "exall",
  "delay",
  "surr",
  "login",
  "given",
  "taken",
  "dropped",
  "prdstart",
  "prdstop"
] as const;

export const sectorEnum = z.enum(sectors);
export const roomFlagEnum = z.enum(roomFlags);
export const exitFlagEnum = z.enum(exitFlags);
export const directionEnum = z.enum(directions);
export const wearLocationEnum = z.enum(wearLocations);
export const wearFlagEnum = z.enum(wearFlags);
export const extraFlagEnum = z.enum(extraFlags);
export const weaponClassEnum = z.enum(weaponClasses);
export const weaponFlagEnum = z.enum(weaponFlags);
export const containerFlagEnum = z.enum(containerFlags);
export const portalFlagEnum = z.enum(portalFlags);
export const furnitureFlagEnum = z.enum(furnitureFlags);
export const liquidEnum = z.enum(liquids);
export const positionEnum = z.enum(positions);
export const sizeEnum = z.enum(sizes);
export const sexEnum = z.enum(sexes);
export const damageTypeEnum = z.enum(damageTypes);
export const actFlagEnum = z.enum(actFlags);
export const affectFlagEnum = z.enum(affectFlags);
export const offFlagEnum = z.enum(offFlags);
export const immFlagEnum = z.enum(immFlags);
export const resFlagEnum = z.enum(resFlags);
export const vulnFlagEnum = z.enum(vulnFlags);
export const formFlagEnum = z.enum(formFlags);
export const partFlagEnum = z.enum(partFlags);
export const applyLocationEnum = z.enum(applyLocations);
export const skillTargetEnum = z.enum(skillTargets);
export const eventTriggerEnum = z.enum(eventTriggers);
