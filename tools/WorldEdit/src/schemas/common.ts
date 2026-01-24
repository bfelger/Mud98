import { z } from "zod";
import {
  applyLocationEnum,
  directionEnum,
  exitFlagEnum,
  eventTriggerEnum,
  roomFlagEnum,
  sectorEnum
} from "./enums";

export const vnumSchema = z.number().int();

export const stringSchema = z.string();

export const extraDescSchema = z
  .object({
    keyword: stringSchema,
    description: stringSchema
  })
  .passthrough();

export const exitSchema = z
  .object({
    dir: directionEnum,
    toVnum: vnumSchema,
    key: vnumSchema.optional(),
    flags: z.array(exitFlagEnum).optional(),
    description: stringSchema.optional(),
    keyword: stringSchema.optional()
  })
  .passthrough();

export const storyBeatSchema = z
  .object({
    title: stringSchema,
    description: stringSchema
  })
  .passthrough();

export const checklistStatusSchema = z.union([
  z.enum(["todo", "inProgress", "done"]),
  z.number().int().min(0).max(2)
]);

export const checklistSchema = z
  .object({
    title: stringSchema,
    status: checklistStatusSchema,
    description: stringSchema.optional()
  })
  .passthrough();

export const eventSchema = z
  .object({
    trigger: eventTriggerEnum,
    callback: stringSchema,
    criteria: z.union([stringSchema, z.number().int()]).optional()
  })
  .passthrough();

export const diceSchema = z
  .object({
    number: z.number().int(),
    type: z.number().int(),
    bonus: z.number().int().optional()
  })
  .passthrough();

export const acSchema = z
  .object({
    pierce: z.number().int(),
    bash: z.number().int(),
    slash: z.number().int(),
    exotic: z.number().int()
  })
  .passthrough();

export const affectSchema = z
  .object({
    type: z.union([stringSchema, z.number().int()]),
    where: stringSchema,
    location: z.union([applyLocationEnum, z.number().int()]),
    level: z.number().int(),
    duration: z.number().int(),
    modifier: z.number().int(),
    bitvector: z.array(stringSchema).optional(),
    bitvectorValue: z.number().int().optional()
  })
  .passthrough();

export const areadataSchema = z
  .object({
    version: z.number().int(),
    name: stringSchema,
    builders: stringSchema.optional(),
    vnumRange: z.tuple([vnumSchema, vnumSchema]),
    credits: stringSchema.optional(),
    security: z.number().int().optional(),
    sector: sectorEnum.optional(),
    lowLevel: z.number().int().optional(),
    highLevel: z.number().int().optional(),
    reset: z.number().int().optional(),
    alwaysReset: z.boolean().optional(),
    instType: z.literal("multi").optional(),
    lootTable: stringSchema.optional()
  })
  .passthrough();

export const roomSchema = z
  .object({
    vnum: vnumSchema,
    name: stringSchema,
    description: stringSchema,
    roomFlags: z.array(roomFlagEnum).optional(),
    sector: sectorEnum.optional(),
    exits: z.array(exitSchema).optional(),
    extraDescs: z.array(extraDescSchema).optional(),
    loxScript: stringSchema.optional(),
    events: z.array(eventSchema).optional()
  })
  .passthrough();
