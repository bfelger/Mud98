/**
 * TypeScript types inferred from Zod form schemas.
 * 
 * These types are used for form state and handler type annotations.
 */

import { z } from "zod";
import {
  areaFormSchema,
  roomFormSchema,
  mobileFormSchema,
  objectFormSchema,
  resetFormSchema,
  shopFormSchema,
  questFormSchema,
  factionFormSchema,
  lootFormSchema,
  recipeFormSchema,
  gatherSpawnFormSchema,
  classFormSchema,
  raceFormSchema,
  skillFormSchema,
  groupFormSchema,
  commandFormSchema,
  socialFormSchema,
  tutorialFormSchema
} from "./formSchemas";

// =============================================================================
// Primary Entity Types
// =============================================================================

export type RoomFormValues = z.infer<typeof roomFormSchema>;
export type AreaFormValues = z.infer<typeof areaFormSchema>;
export type MobileFormValues = z.infer<typeof mobileFormSchema>;
export type ObjectFormValues = z.infer<typeof objectFormSchema>;
export type ResetFormValues = z.infer<typeof resetFormSchema>;
export type ShopFormValues = z.infer<typeof shopFormSchema>;
export type QuestFormValues = z.infer<typeof questFormSchema>;
export type FactionFormValues = z.infer<typeof factionFormSchema>;
export type LootFormValues = z.infer<typeof lootFormSchema>;
export type RecipeFormValues = z.infer<typeof recipeFormSchema>;
export type GatherSpawnFormValues = z.infer<typeof gatherSpawnFormSchema>;

// =============================================================================
// Global Data Types
// =============================================================================

export type ClassFormValues = z.infer<typeof classFormSchema>;
export type RaceFormValues = z.infer<typeof raceFormSchema>;
export type SkillFormValues = z.infer<typeof skillFormSchema>;
export type GroupFormValues = z.infer<typeof groupFormSchema>;
export type CommandFormValues = z.infer<typeof commandFormSchema>;
export type SocialFormValues = z.infer<typeof socialFormSchema>;
export type TutorialFormValues = z.infer<typeof tutorialFormSchema>;

// =============================================================================
// UI Helper Types
// =============================================================================

/**
 * Represents a reset item displayed in the room's reset list.
 */
export type RoomResetItem = {
  index: number;
  command: string;
  label: string;
};
