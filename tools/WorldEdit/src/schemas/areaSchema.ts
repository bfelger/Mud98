import { z } from "zod";
import {
  actFlagEnum,
  affectFlagEnum,
  containerFlagEnum,
  damageTypeEnum,
  directionEnum,
  discoveryTypeEnum,
  extraFlagEnum,
  formFlagEnum,
  furnitureFlagEnum,
  immFlagEnum,
  liquidEnum,
  offFlagEnum,
  partFlagEnum,
  portalFlagEnum,
  positionEnum,
  resFlagEnum,
  sectorEnum,
  sexEnum,
  sizeEnum,
  vulnFlagEnum,
  wearFlagEnum,
  wearLocationEnum,
  weaponClassEnum,
  weaponFlagEnum,
  exitFlagEnum,
  workstationTypeEnum,
  lootEntryTypeEnum,
  lootOpTypeEnum
} from "./enums";
import {
  acSchema,
  affectSchema,
  areadataSchema,
  checklistSchema,
  diceSchema,
  eventSchema,
  extraDescSchema,
  roomSchema,
  storyBeatSchema,
  stringSchema,
  vnumSchema
} from "./common";

const mobileSchema = z
  .object({
    vnum: vnumSchema,
    name: stringSchema,
    shortDescr: stringSchema,
    longDescr: stringSchema,
    description: stringSchema,
    race: stringSchema,
    actFlags: z.array(actFlagEnum).optional(),
    affectFlags: z.array(affectFlagEnum).optional(),
    offFlags: z.array(offFlagEnum).optional(),
    immFlags: z.array(immFlagEnum).optional(),
    resFlags: z.array(resFlagEnum).optional(),
    vulnFlags: z.array(vulnFlagEnum).optional(),
    formFlags: z.array(formFlagEnum).optional(),
    partFlags: z.array(partFlagEnum).optional(),
    alignment: z.number().int().optional(),
    group: z.number().int().optional(),
    level: z.number().int().optional(),
    hitroll: z.number().int().optional(),
    damType: damageTypeEnum.optional(),
    startPos: positionEnum.optional(),
    defaultPos: positionEnum.optional(),
    sex: sexEnum.optional(),
    wealth: z.number().int().optional(),
    size: sizeEnum.optional(),
    material: stringSchema.optional(),
    factionVnum: vnumSchema.optional(),
    hitDice: diceSchema.optional(),
    manaDice: diceSchema.optional(),
    damageDice: diceSchema.optional(),
    ac: acSchema.optional(),
    damageNoun: stringSchema.optional(),
    offensiveSpell: stringSchema.optional(),
    lootTable: stringSchema.optional(),
    mprogFlags: z.array(stringSchema).optional(),
    loxScript: stringSchema.optional(),
    events: z.array(eventSchema).optional()
  })
  .passthrough();

const weaponSchema = z
  .object({
    class: weaponClassEnum,
    dice: z.tuple([z.number().int(), z.number().int()]),
    damageType: damageTypeEnum,
    flags: z.array(weaponFlagEnum).optional()
  })
  .passthrough();

const containerSchema = z
  .object({
    capacity: z.number().int(),
    flags: z.array(containerFlagEnum).optional(),
    keyVnum: vnumSchema.optional(),
    maxWeight: z.number().int().optional(),
    weightMult: z.number().int().optional()
  })
  .passthrough();

const lightSchema = z
  .object({
    hours: z.number().int()
  })
  .passthrough();

const armorSchema = z
  .object({
    acPierce: z.number().int(),
    acBash: z.number().int(),
    acSlash: z.number().int(),
    acExotic: z.number().int()
  })
  .passthrough();

const drinkSchema = z
  .object({
    capacity: z.number().int().optional(),
    remaining: z.number().int().optional(),
    liquid: liquidEnum,
    poisoned: z.boolean().optional()
  })
  .passthrough();

const foodSchema = z
  .object({
    foodHours: z.number().int(),
    fullHours: z.number().int().optional(),
    poisoned: z.boolean().optional()
  })
  .passthrough();

const moneySchema = z
  .object({
    gold: z.number().int(),
    silver: z.number().int()
  })
  .passthrough();

const wandSchema = z
  .object({
    level: z.number().int(),
    totalCharges: z.number().int(),
    chargesLeft: z.number().int(),
    spell: stringSchema
  })
  .passthrough();

const spellsSchema = z
  .object({
    level: z.number().int(),
    spell1: stringSchema.optional(),
    spell2: stringSchema.optional(),
    spell3: stringSchema.optional(),
    spell4: stringSchema.optional()
  })
  .passthrough();

const portalSchema = z
  .object({
    charges: z.number().int().optional(),
    exitFlags: z.array(exitFlagEnum).optional(),
    portalFlags: z.array(portalFlagEnum).optional(),
    toVnum: vnumSchema.optional()
  })
  .passthrough();

const furnitureSchema = z
  .object({
    slots: z.number().int(),
    weight: z.number().int(),
    flags: z.array(furnitureFlagEnum).optional(),
    healBonus: z.number().int().optional(),
    manaBonus: z.number().int().optional(),
    maxPeople: z.number().int().optional()
  })
  .passthrough();

const objectSchema = z
  .object({
    vnum: vnumSchema,
    name: stringSchema,
    shortDescr: stringSchema,
    description: stringSchema,
    material: stringSchema,
    itemType: stringSchema,
    extraFlags: z.array(extraFlagEnum).optional(),
    wearFlags: z.array(wearFlagEnum).optional(),
    values: z.array(z.number().int()).optional(),
    level: z.number().int().optional(),
    weight: z.number().int().optional(),
    cost: z.number().int().optional(),
    condition: z.number().int().optional(),
    extraDescs: z.array(extraDescSchema).optional(),
    affects: z.array(affectSchema).optional(),
    weapon: weaponSchema.optional(),
    container: containerSchema.optional(),
    light: lightSchema.optional(),
    armor: armorSchema.optional(),
    drink: drinkSchema.optional(),
    fountain: drinkSchema.optional(),
    food: foodSchema.optional(),
    money: moneySchema.optional(),
    wand: wandSchema.optional(),
    staff: wandSchema.optional(),
    spells: spellsSchema.optional(),
    portal: portalSchema.optional(),
    furniture: furnitureSchema.optional(),
    loxScript: stringSchema.optional(),
    events: z.array(eventSchema).optional()
  })
  .passthrough();

const resetSchema = z
  .object({
    commandName: stringSchema.optional(),
    command: stringSchema.optional(),
    arg1: z.number().int().optional(),
    arg2: z.number().int().optional(),
    arg3: z.number().int().optional(),
    arg4: z.number().int().optional(),
    roomVnum: vnumSchema.optional(),
    mobVnum: vnumSchema.optional(),
    objVnum: vnumSchema.optional(),
    maxInArea: z.number().int().optional(),
    maxInRoom: z.number().int().optional(),
    count: z.number().int().optional(),
    containerVnum: vnumSchema.optional(),
    maxInContainer: z.number().int().optional(),
    wearLoc: wearLocationEnum.optional(),
    direction: directionEnum.optional(),
    state: z.number().int().optional(),
    exits: z.number().int().optional()
  })
  .passthrough();

const shopSchema = z
  .object({
    keeper: vnumSchema,
    buyTypes: z.array(z.number().int()).length(5),
    profitBuy: z.number().int(),
    profitSell: z.number().int(),
    openHour: z.number().int(),
    closeHour: z.number().int()
  })
  .passthrough();

const specialSchema = z
  .object({
    mobVnum: vnumSchema,
    spec: stringSchema
  })
  .passthrough();

const mobprogSchema = z
  .object({
    vnum: vnumSchema,
    code: stringSchema
  })
  .passthrough();

const questSchema = z
  .object({
    vnum: vnumSchema,
    name: stringSchema.optional(),
    entry: stringSchema.optional(),
    type: stringSchema.optional(),
    xp: z.number().int().optional(),
    level: z.number().int().optional(),
    end: vnumSchema.optional(),
    target: vnumSchema.optional(),
    upper: vnumSchema.optional(),
    count: z.number().int().optional(),
    rewardFaction: vnumSchema.optional(),
    rewardReputation: z.number().int().optional(),
    rewardGold: z.number().int().optional(),
    rewardSilver: z.number().int().optional(),
    rewardCopper: z.number().int().optional(),
    rewardObjs: z.array(vnumSchema).optional(),
    rewardCounts: z.array(z.number().int()).optional()
  })
  .passthrough();

const helpSchema = z
  .object({
    level: z.number().int(),
    keyword: stringSchema,
    text: stringSchema
  })
  .passthrough();

const factionSchema = z
  .object({
    vnum: vnumSchema,
    name: stringSchema,
    defaultStanding: z.number().int(),
    allies: z.array(vnumSchema).optional(),
    opposing: z.array(vnumSchema).optional()
  })
  .passthrough();

const lootEntrySchema = z
  .object({
    type: lootEntryTypeEnum,
    vnum: vnumSchema.optional(),
    minQty: z.number().int().optional(),
    maxQty: z.number().int().optional(),
    weight: z.number().int().optional()
  })
  .passthrough();

const lootGroupSchema = z
  .object({
    name: stringSchema,
    rolls: z.number().int().optional(),
    entries: z.array(lootEntrySchema).optional()
  })
  .passthrough();

const lootOpSchema = z
  .object({
    op: lootOpTypeEnum,
    group: stringSchema.optional(),
    vnum: vnumSchema.optional(),
    chance: z.number().int().optional(),
    minQty: z.number().int().optional(),
    maxQty: z.number().int().optional(),
    multiplier: z.number().int().optional()
  })
  .passthrough();

const lootTableSchema = z
  .object({
    name: stringSchema,
    parent: stringSchema.optional(),
    ops: z.array(lootOpSchema).optional()
  })
  .passthrough();

const lootSchema = z
  .object({
    groups: z.array(lootGroupSchema).optional(),
    tables: z.array(lootTableSchema).optional()
  })
  .passthrough();

const recipeInputSchema = z
  .object({
    vnum: vnumSchema,
    quantity: z.number().int()
  })
  .passthrough();

const recipeSchema = z
  .object({
    vnum: vnumSchema,
    name: stringSchema.optional(),
    skill: stringSchema.optional(),
    minSkill: z.number().int().optional(),
    minSkillPct: z.number().int().optional(),
    minLevel: z.number().int().optional(),
    stationType: z.array(workstationTypeEnum).optional(),
    stationVnum: vnumSchema.optional(),
    discovery: discoveryTypeEnum.optional(),
    inputs: z.array(recipeInputSchema).optional(),
    outputVnum: vnumSchema.optional(),
    outputQuantity: z.number().int().optional()
  })
  .passthrough();

const gatherSpawnSchema = z
  .object({
    spawnSector: sectorEnum.optional(),
    vnum: vnumSchema,
    quantity: z.number().int().optional(),
    respawnTimer: z.number().int().optional()
  })
  .passthrough();

export const areaSchema = z
  .object({
    formatVersion: z.literal(1).optional(),
    areadata: areadataSchema,
    rooms: z.array(roomSchema).optional(),
    mobiles: z.array(mobileSchema).optional(),
    objects: z.array(objectSchema).optional(),
    resets: z.array(resetSchema).optional(),
    shops: z.array(shopSchema).optional(),
    specials: z.array(specialSchema).optional(),
    mobprogs: z.array(mobprogSchema).optional(),
    quests: z.array(questSchema).optional(),
    helps: z.array(helpSchema).optional(),
    factions: z.array(factionSchema).optional(),
    loot: lootSchema.optional(),
    recipes: z.array(recipeSchema).optional(),
    gatherSpawns: z.array(gatherSpawnSchema).optional(),
    storyBeats: z.array(storyBeatSchema).optional(),
    checklist: z.array(checklistSchema).optional()
  })
  .passthrough();
