/**
 * Zod form schemas for WorldEdit entity forms.
 * 
 * These schemas validate form input before it's normalized and saved.
 * They handle empty strings, optional values, and provide error messages.
 */

import { z } from "zod";
import {
  lootEntryTypeOptions,
  lootOpTypeOptions,
  classGuildCount,
  type ResetCommand
} from "../constants/appConfig";
import {
  groupSkillCount,
  raceSkillCount
} from "../constants/globalDefaults";
import {
  actFlagEnum,
  containerFlagEnum,
  damageTypeEnum,
  directionEnum,
  discoveryTypes,
  exitFlagEnum,
  extraFlagEnum,
  formFlagEnum,
  furnitureFlagEnum,
  liquidEnum,
  offFlagEnum,
  partFlagEnum,
  portalFlagEnum,
  positionEnum,
  roomFlagEnum,
  sectorEnum,
  sexEnum,
  sizeEnum,
  weaponClassEnum,
  weaponFlagEnum,
  wearFlagEnum,
  wearLocationEnum,
  workstationTypes
} from "./enums";

// =============================================================================
// Preprocessor Schemas
// =============================================================================

/**
 * Schema that converts empty strings, null, undefined, and NaN to undefined,
 * allowing optional integer fields in forms.
 */
export const optionalIntSchema = z.preprocess((value) => {
  if (value === "" || value === null || value === undefined) {
    return undefined;
  }
  if (typeof value === "number" && Number.isNaN(value)) {
    return undefined;
  }
  if (typeof value === "string") {
    const parsed = Number(value);
    return Number.isFinite(parsed) ? parsed : value;
  }
  return value;
}, z.number().int().optional());

/**
 * Creates an optional enum schema that handles empty strings.
 */
export const optionalEnumSchema = <T extends z.ZodTypeAny>(schema: T) =>
  z.preprocess((value) => {
    if (value === "" || value === null || value === undefined) {
      return undefined;
    }
    return value;
  }, schema.optional());

// =============================================================================
// Optional Enum Schema Instances
// =============================================================================

export const optionalSectorSchema = optionalEnumSchema(sectorEnum);
export const optionalPositionSchema = optionalEnumSchema(positionEnum);
export const optionalSexSchema = optionalEnumSchema(sexEnum);
export const optionalSizeSchema = optionalEnumSchema(sizeEnum);
export const optionalDamageTypeSchema = optionalEnumSchema(damageTypeEnum);
export const optionalDirectionSchema = optionalEnumSchema(directionEnum);
export const optionalWearLocationSchema = optionalEnumSchema(wearLocationEnum);

// =============================================================================
// Constant Arrays
// =============================================================================

export const checklistStatusOptions = ["todo", "inProgress", "done"] as const;

export const defaultDaycyclePeriods = [
  { name: "day", fromHour: 6, toHour: 18 },
  { name: "dusk", fromHour: 19, toHour: 19 },
  { name: "night", fromHour: 20, toHour: 4 },
  { name: "dawn", fromHour: 5, toHour: 5 }
] as const;

// =============================================================================
// Area Schema
// =============================================================================

export const areaFormSchema = z.object({
  name: z.string().min(1, "Name is required."),
  version: z.number().int(),
  vnumRangeStart: z.number().int(),
  vnumRangeEnd: z.number().int(),
  builders: z.string().optional(),
  credits: z.string().optional(),
  lootTable: z.string().optional(),
  security: optionalIntSchema,
  sector: optionalSectorSchema,
  lowLevel: optionalIntSchema,
  highLevel: optionalIntSchema,
  reset: optionalIntSchema,
  alwaysReset: z.boolean().optional(),
  instType: z.enum(["single", "multi"]),
  daycycleSuppress: z.boolean().optional(),
  daycyclePeriods: z.array(
    z.object({
      name: z.string().optional(),
      fromHour: optionalIntSchema,
      toHour: optionalIntSchema,
      description: z.string().optional(),
      enterMessage: z.string().optional(),
      exitMessage: z.string().optional()
    })
  ),
  storyBeats: z.array(
    z.object({
      title: z.string().min(1, "Title is required."),
      description: z.string().optional()
    })
  ),
  checklist: z.array(
    z.object({
      title: z.string().min(1, "Title is required."),
      status: z.enum(checklistStatusOptions),
      description: z.string().optional()
    })
  )
});

// =============================================================================
// Exit and Room Nested Schemas
// =============================================================================

export const exitFormSchema = z.object({
  dir: directionEnum,
  toVnum: z.preprocess(
    (value) => {
      if (value === "" || value === null || value === undefined) {
        return value;
      }
      if (typeof value === "string") {
        const parsed = Number(value);
        return Number.isFinite(parsed) ? parsed : value;
      }
      return value;
    },
    z.number().int()
  ),
  key: optionalIntSchema,
  flags: z.array(exitFlagEnum).optional(),
  description: z.preprocess(
    (value) => (typeof value === "string" ? value : ""),
    z.string().optional()
  ),
  keyword: z.preprocess(
    (value) => (typeof value === "string" ? value : ""),
    z.string().optional()
  )
});

export const daycyclePeriodFormSchema = z.object({
  name: z.string().optional(),
  fromHour: optionalIntSchema,
  toHour: optionalIntSchema,
  description: z.string().optional(),
  enterMessage: z.string().optional(),
  exitMessage: z.string().optional()
});

// =============================================================================
// Dice Schema (reusable)
// =============================================================================

export const diceFormSchema = z
  .object({
    number: optionalIntSchema,
    type: optionalIntSchema,
    bonus: optionalIntSchema
  })
  .optional();

// =============================================================================
// Mobile Schema
// =============================================================================

export const mobileFormSchema = z.object({
  vnum: z.number().int(),
  name: z.string().min(1, "Name is required."),
  shortDescr: z.string().min(1, "Short description is required."),
  longDescr: z.string().min(1, "Long description is required."),
  description: z.string().min(1, "Description is required."),
  race: z.string().min(1, "Race is required."),
  level: optionalIntSchema,
  hitroll: optionalIntSchema,
  alignment: optionalIntSchema,
  damType: optionalDamageTypeSchema,
  startPos: optionalPositionSchema,
  defaultPos: optionalPositionSchema,
  sex: optionalSexSchema,
  size: optionalSizeSchema,
  material: z.string().optional(),
  factionVnum: optionalIntSchema,
  damageNoun: z.string().optional(),
  offensiveSpell: z.string().optional(),
  lootTable: z.string().optional(),
  hitDice: diceFormSchema,
  manaDice: diceFormSchema,
  damageDice: diceFormSchema,
  actFlags: z.array(actFlagEnum).optional(),
  atkFlags: z.array(offFlagEnum).optional(),
  formFlags: z.array(formFlagEnum).optional(),
  partFlags: z.array(partFlagEnum).optional()
});

// =============================================================================
// Object Type-Specific Schemas
// =============================================================================

export const weaponFormSchema = z
  .object({
    class: optionalEnumSchema(weaponClassEnum),
    diceNumber: optionalIntSchema,
    diceType: optionalIntSchema,
    damageType: optionalDamageTypeSchema,
    flags: z.array(weaponFlagEnum).optional()
  })
  .optional();

export const armorFormSchema = z
  .object({
    acPierce: optionalIntSchema,
    acBash: optionalIntSchema,
    acSlash: optionalIntSchema,
    acExotic: optionalIntSchema
  })
  .optional();

export const containerFormSchema = z
  .object({
    capacity: optionalIntSchema,
    flags: z.array(containerFlagEnum).optional(),
    keyVnum: optionalIntSchema,
    maxWeight: optionalIntSchema,
    weightMult: optionalIntSchema
  })
  .optional();

export const lightFormSchema = z
  .object({
    hours: optionalIntSchema
  })
  .optional();

export const drinkFormSchema = z
  .object({
    capacity: optionalIntSchema,
    remaining: optionalIntSchema,
    liquid: optionalEnumSchema(liquidEnum),
    poisoned: z.boolean().optional()
  })
  .optional();

export const foodFormSchema = z
  .object({
    foodHours: optionalIntSchema,
    fullHours: optionalIntSchema,
    poisoned: z.boolean().optional()
  })
  .optional();

export const moneyFormSchema = z
  .object({
    gold: optionalIntSchema,
    silver: optionalIntSchema
  })
  .optional();

export const wandFormSchema = z
  .object({
    level: optionalIntSchema,
    totalCharges: optionalIntSchema,
    chargesLeft: optionalIntSchema,
    spell: z.string().optional()
  })
  .optional();

export const spellsFormSchema = z
  .object({
    level: optionalIntSchema,
    spell1: z.string().optional(),
    spell2: z.string().optional(),
    spell3: z.string().optional(),
    spell4: z.string().optional()
  })
  .optional();

export const portalFormSchema = z
  .object({
    charges: optionalIntSchema,
    exitFlags: z.array(exitFlagEnum).optional(),
    portalFlags: z.array(portalFlagEnum).optional(),
    toVnum: optionalIntSchema
  })
  .optional();

export const furnitureFormSchema = z
  .object({
    slots: optionalIntSchema,
    weight: optionalIntSchema,
    flags: z.array(furnitureFlagEnum).optional(),
    healBonus: optionalIntSchema,
    manaBonus: optionalIntSchema,
    maxPeople: optionalIntSchema
  })
  .optional();

// =============================================================================
// Object Schema
// =============================================================================

export const objectFormSchema = z.object({
  vnum: z.number().int(),
  name: z.string().min(1, "Name is required."),
  shortDescr: z.string().min(1, "Short description is required."),
  description: z.string().min(1, "Description is required."),
  material: z.string().min(1, "Material is required."),
  itemType: z.string().min(1, "Item type is required."),
  extraFlags: z.array(extraFlagEnum).optional(),
  wearFlags: z.array(wearFlagEnum).optional(),
  level: optionalIntSchema,
  weight: optionalIntSchema,
  cost: optionalIntSchema,
  condition: optionalIntSchema,
  weapon: weaponFormSchema,
  armor: armorFormSchema,
  container: containerFormSchema,
  light: lightFormSchema,
  drink: drinkFormSchema,
  fountain: drinkFormSchema,
  food: foodFormSchema,
  money: moneyFormSchema,
  wand: wandFormSchema,
  staff: wandFormSchema,
  spells: spellsFormSchema,
  portal: portalFormSchema,
  furniture: furnitureFormSchema
});

// =============================================================================
// Reset Schema
// =============================================================================

export const resetFormSchema = z
  .object({
    index: z.number().int(),
    commandName: z.string().min(1, "Command is required."),
    mobVnum: optionalIntSchema,
    objVnum: optionalIntSchema,
    roomVnum: optionalIntSchema,
    containerVnum: optionalIntSchema,
    maxInArea: optionalIntSchema,
    maxInRoom: optionalIntSchema,
    count: optionalIntSchema,
    maxInContainer: optionalIntSchema,
    wearLoc: optionalWearLocationSchema,
    direction: optionalDirectionSchema,
    state: optionalIntSchema,
    exits: optionalIntSchema
  })
  .superRefine((value, ctx) => {
    const command = value.commandName as ResetCommand;
    const requireField = (
      condition: boolean,
      field: keyof typeof value,
      message: string
    ) => {
      if (!condition) {
        ctx.addIssue({
          code: z.ZodIssueCode.custom,
          path: [field],
          message
        });
      }
    };
    if (command === "loadMob") {
      requireField(
        value.mobVnum !== undefined,
        "mobVnum",
        "Mob VNUM is required."
      );
      requireField(
        value.roomVnum !== undefined,
        "roomVnum",
        "Room VNUM is required."
      );
    }
    if (command === "placeObj") {
      requireField(
        value.objVnum !== undefined,
        "objVnum",
        "Object VNUM is required."
      );
      requireField(
        value.roomVnum !== undefined,
        "roomVnum",
        "Room VNUM is required."
      );
    }
    if (command === "putObj") {
      requireField(
        value.objVnum !== undefined,
        "objVnum",
        "Object VNUM is required."
      );
      requireField(
        value.containerVnum !== undefined,
        "containerVnum",
        "Container VNUM is required."
      );
    }
    if (command === "giveObj") {
      requireField(
        value.objVnum !== undefined,
        "objVnum",
        "Object VNUM is required."
      );
    }
    if (command === "equipObj") {
      requireField(
        value.objVnum !== undefined,
        "objVnum",
        "Object VNUM is required."
      );
      requireField(
        value.wearLoc !== undefined,
        "wearLoc",
        "Wear location is required."
      );
    }
    if (command === "setDoor") {
      requireField(
        value.roomVnum !== undefined,
        "roomVnum",
        "Room VNUM is required."
      );
      requireField(
        value.direction !== undefined,
        "direction",
        "Direction is required."
      );
      requireField(
        value.state !== undefined,
        "state",
        "State is required."
      );
    }
    if (command === "randomizeExits") {
      requireField(
        value.roomVnum !== undefined,
        "roomVnum",
        "Room VNUM is required."
      );
      requireField(
        value.exits !== undefined,
        "exits",
        "Exit count is required."
      );
    }
  });

// =============================================================================
// Room Schema
// =============================================================================

export const roomFormSchema = z.object({
  vnum: z.number().int(),
  name: z.string().min(1, "Name is required."),
  description: z.string().min(1, "Description is required."),
  sectorType: optionalSectorSchema,
  roomFlags: z.array(roomFlagEnum).optional(),
  suppressDaycycleMessages: z.boolean().optional(),
  timePeriods: z.array(daycyclePeriodFormSchema).optional(),
  exits: z.array(exitFormSchema).optional()
});

// =============================================================================
// Shop Schema
// =============================================================================

export const shopFormSchema = z.object({
  keeper: optionalIntSchema,
  buyTypes: z.array(optionalIntSchema).length(5),
  profitBuy: optionalIntSchema,
  profitSell: optionalIntSchema,
  openHour: optionalIntSchema,
  closeHour: optionalIntSchema
});

// =============================================================================
// Quest Schema
// =============================================================================

export const questFormSchema = z.object({
  vnum: z.number().int(),
  name: z.string().optional(),
  entry: z.string().optional(),
  type: z.string().optional(),
  xp: optionalIntSchema,
  level: optionalIntSchema,
  end: optionalIntSchema,
  target: optionalIntSchema,
  upper: optionalIntSchema,
  count: optionalIntSchema,
  rewardFaction: optionalIntSchema,
  rewardReputation: optionalIntSchema,
  rewardGold: optionalIntSchema,
  rewardSilver: optionalIntSchema,
  rewardCopper: optionalIntSchema,
  rewardObjs: z.array(optionalIntSchema).length(3),
  rewardCounts: z.array(optionalIntSchema).length(3)
});

// =============================================================================
// Faction Schema
// =============================================================================

export const factionFormSchema = z.object({
  vnum: z.number().int(),
  name: z.string().optional(),
  defaultStanding: optionalIntSchema,
  alliesCsv: z.string().optional(),
  opposingCsv: z.string().optional()
});

// =============================================================================
// Loot Schemas
// =============================================================================

export const lootEntryFormSchema = z.object({
  type: z.enum(lootEntryTypeOptions),
  vnum: optionalIntSchema,
  minQty: z.number().int().min(0).optional(),
  maxQty: z.number().int().min(0).optional(),
  weight: z.number().int().min(0).optional()
});

export const lootOpFormSchema = z.object({
  op: z.enum(lootOpTypeOptions),
  group: z.string().optional(),
  vnum: optionalIntSchema,
  chance: z.number().int().min(0).optional(),
  minQty: z.number().int().min(0).optional(),
  maxQty: z.number().int().min(0).optional(),
  multiplier: z.number().int().min(0).optional()
});

export const lootFormSchema = z.object({
  kind: z.enum(["group", "table"]),
  name: z.string().min(1),
  rolls: z.number().int().min(0).optional(),
  entries: z.array(lootEntryFormSchema).optional(),
  parent: z.string().optional(),
  ops: z.array(lootOpFormSchema).optional()
});

// =============================================================================
// Recipe Schemas
// =============================================================================

export const recipeInputFormSchema = z.object({
  vnum: optionalIntSchema,
  quantity: optionalIntSchema
});

export const recipeFormSchema = z.object({
  vnum: z.number().int(),
  name: z.string().optional(),
  skill: z.string().optional(),
  minSkill: optionalIntSchema,
  minSkillPct: optionalIntSchema,
  minLevel: optionalIntSchema,
  stationType: z.array(z.enum(workstationTypes)).optional(),
  stationVnum: optionalIntSchema,
  discovery: optionalEnumSchema(z.enum(discoveryTypes)),
  inputs: z.array(recipeInputFormSchema).optional(),
  outputVnum: optionalIntSchema,
  outputQuantity: optionalIntSchema
});

// =============================================================================
// Gather Spawn Schema
// =============================================================================

export const gatherSpawnFormSchema = z.object({
  spawnSector: optionalSectorSchema,
  vnum: optionalIntSchema,
  quantity: optionalIntSchema,
  respawnTimer: optionalIntSchema
});

// =============================================================================
// Global Data Schemas
// =============================================================================

export const classFormSchema = z.object({
  name: z.string().min(1),
  whoName: z.string().optional(),
  baseGroup: z.string().optional(),
  defaultGroup: z.string().optional(),
  weaponVnum: z.number().int(),
  armorProf: z.string().optional(),
  guilds: z.array(z.number().int()).length(classGuildCount),
  primeStat: z.string().optional(),
  skillCap: z.number().int(),
  thac0_00: z.number().int(),
  thac0_32: z.number().int(),
  hpMin: z.number().int(),
  hpMax: z.number().int(),
  manaUser: z.boolean(),
  startLoc: z.number().int(),
  titlesMale: z.string().optional(),
  titlesFemale: z.string().optional()
});

export const raceFormSchema = z.object({
  name: z.string().min(1),
  whoName: z.string().optional(),
  pc: z.boolean(),
  points: z.number().int(),
  size: z.string().optional(),
  startLoc: z.number().int(),
  stats: z.object({
    str: z.number().int(),
    int: z.number().int(),
    wis: z.number().int(),
    dex: z.number().int(),
    con: z.number().int()
  }),
  maxStats: z.object({
    str: z.number().int(),
    int: z.number().int(),
    wis: z.number().int(),
    dex: z.number().int(),
    con: z.number().int()
  }),
  actFlags: z.array(z.string()).optional(),
  affectFlags: z.array(z.string()).optional(),
  offFlags: z.array(z.string()).optional(),
  immFlags: z.array(z.string()).optional(),
  resFlags: z.array(z.string()).optional(),
  vulnFlags: z.array(z.string()).optional(),
  formFlags: z.array(z.string()).optional(),
  partFlags: z.array(z.string()).optional(),
  classMult: z.record(z.number().int()).optional(),
  classStart: z.record(z.number().int()).optional(),
  skills: z.array(z.string()).length(raceSkillCount)
});

export const skillFormSchema = z.object({
  name: z.string().min(1),
  levels: z.record(z.number().int()),
  ratings: z.record(z.number().int()),
  spell: z.string().optional(),
  loxSpell: z.string().optional(),
  target: z.string().optional(),
  minPosition: z.string().optional(),
  gsn: z.string().optional(),
  slot: z.number().int(),
  minMana: z.number().int(),
  beats: z.number().int(),
  nounDamage: z.string().optional(),
  msgOff: z.string().optional(),
  msgObj: z.string().optional()
});

export const groupFormSchema = z.object({
  name: z.string().min(1),
  ratings: z.record(z.number().int()),
  skills: z.array(z.string()).length(groupSkillCount)
});

export const commandFormSchema = z.object({
  name: z.string().min(1),
  function: z.string().optional(),
  position: z.string().optional(),
  level: z.number().int(),
  log: z.string().optional(),
  category: z.string().optional(),
  loxFunction: z.string().optional()
});

export const socialFormSchema = z.object({
  name: z.string().min(1),
  charNoArg: z.string().optional(),
  othersNoArg: z.string().optional(),
  charFound: z.string().optional(),
  othersFound: z.string().optional(),
  victFound: z.string().optional(),
  charAuto: z.string().optional(),
  othersAuto: z.string().optional()
});

export const tutorialFormSchema = z.object({
  name: z.string().min(1),
  blurb: z.string().optional(),
  finish: z.string().optional(),
  minLevel: z.number().int(),
  steps: z.array(
    z.object({
      prompt: z.string().optional(),
      match: z.string().optional()
    })
  )
});
