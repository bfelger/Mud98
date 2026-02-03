/**
 * Application-wide constants and configuration.
 * Extracted from App.tsx for maintainability.
 */

import { Position } from "reactflow";

// View tab definitions
export const tabs = [
  {
    id: "Table",
    title: "Table View",
    description: "Filterable grid for Rooms, Mobiles, Objects, Resets."
  },
  {
    id: "Form",
    title: "Form View",
    description: "Schema-driven editor with validation and pickers."
  },
  {
    id: "Map",
    title: "Map View",
    description: "Orthogonal room layout with exit routing."
  },
  {
    id: "World",
    title: "World Graph",
    description: "Project-level view of area links."
  },
  {
    id: "Script",
    title: "Script View",
    description: "Lox script editing with events panel."
  }
] as const;

export type TabId = (typeof tabs)[number]["id"];

// Entity ordering for tree navigation
export const entityOrder = [
  "Area",
  "Rooms",
  "Mobiles",
  "Objects",
  "Resets",
  "Shops",
  "Quests",
  "Factions",
  "Loot",
  "Recipes",
  "Gather Spawns",
  "Helps"
] as const;

export const globalEntityOrder = [
  "Classes",
  "Races",
  "Skills",
  "Groups",
  "Commands",
  "Socials",
  "Tutorials",
  "Loot"
] as const;

export type EntityKey = (typeof entityOrder)[number];
export type GlobalEntityKey = (typeof globalEntityOrder)[number];
export type EditorMode = "Area" | "Global";

// Entity labels and data key mappings
export const entityKindLabels: Record<EntityKey, string> = {
  Area: "Area",
  Rooms: "Room",
  Mobiles: "Mobile",
  Objects: "Object",
  Resets: "Reset",
  Shops: "Shop",
  Quests: "Quest",
  Factions: "Faction",
  Loot: "Loot",
  Recipes: "Recipe",
  "Gather Spawns": "Gather Spawn",
  Helps: "Help"
};

export const entityDataKeys: Record<EntityKey, string> = {
  Area: "areadata",
  Rooms: "rooms",
  Mobiles: "mobiles",
  Objects: "objects",
  Resets: "resets",
  Shops: "shops",
  Quests: "quests",
  Factions: "factions",
  Loot: "loot",
  Recipes: "recipes",
  "Gather Spawns": "gatherSpawns",
  Helps: "helps"
};

// Form option arrays
export const primeStatOptions = ["str", "int", "wis", "dex", "con"] as const;
export const armorProfOptions = ["old_style", "cloth", "light", "medium", "heavy"] as const;
export const classGuildCount = 2;
export const lootEntryTypeOptions = ["item", "cp"] as const;
export const lootOpTypeOptions = [
  "use_group",
  "add_item",
  "add_cp",
  "mul_cp",
  "mul_all_chances",
  "remove_item",
  "remove_group"
] as const;

export const itemTypeOptions = [
  "none",
  "light",
  "scroll",
  "wand",
  "staff",
  "weapon",
  "gather",
  "treasure",
  "armor",
  "potion",
  "clothing",
  "furniture",
  "trash",
  "container",
  "drink",
  "key",
  "food",
  "money",
  "boat",
  "npc_corpse",
  "pc_corpse",
  "fountain",
  "pill",
  "protect",
  "map",
  "portal",
  "warp_stone",
  "room_key",
  "gem",
  "jewelry",
  "jukebox",
  "material",
  "workstation"
] as const;

export const resetCommandOptions = [
  "loadMob",
  "placeObj",
  "putObj",
  "giveObj",
  "equipObj",
  "setDoor",
  "randomizeExits"
] as const;

export const questTypeOptions = ["visit_mob", "kill_mob", "get_obj"] as const;
export const checklistStatusOptions = ["todo", "inProgress", "done"] as const;

export type ResetCommand = (typeof resetCommandOptions)[number];

export const resetCommandMap: Record<string, ResetCommand> = {
  M: "loadMob",
  O: "placeObj",
  P: "putObj",
  G: "giveObj",
  E: "equipObj",
  D: "setDoor",
  R: "randomizeExits"
};

export const resetCommandLabels: Record<ResetCommand, string> = {
  loadMob: "Load Mobile",
  placeObj: "Place Object",
  putObj: "Put Object",
  giveObj: "Give Object",
  equipObj: "Equip Object",
  setDoor: "Set Door",
  randomizeExits: "Randomize Exits"
};

// Map/edge configuration
export const areaDirectionHandleMap: Record<string, { source: string; target: string }> = {
  north: { source: "north-out", target: "south-in" },
  east: { source: "east-out", target: "west-in" },
  south: { source: "south-out", target: "north-in" },
  west: { source: "west-out", target: "east-in" },
  up: { source: "north-out", target: "south-in" },
  down: { source: "south-out", target: "north-in" }
};

export const externalDirectionMap: Record<string, Position> = {
  north: Position.Top,
  south: Position.Bottom,
  east: Position.Right,
  west: Position.Left,
  up: Position.Top,
  down: Position.Bottom
};

export const externalDirectionLabels: Record<string, string> = {
  north: "N",
  south: "S",
  east: "E",
  west: "W",
  up: "U",
  down: "D"
};

export const edgeStubSize = 22;
export const edgeClearance = 10;
export const externalStubSize = 34;

export const edgeDirectionPriority: Record<Position, "horizontal" | "vertical"> = {
  [Position.Left]: "horizontal",
  [Position.Right]: "horizontal",
  [Position.Top]: "vertical",
  [Position.Bottom]: "vertical"
};

// Default daycycle periods
export const defaultDaycyclePeriods = [
  { name: "day", fromHour: 6, toHour: 18 },
  { name: "dusk", fromHour: 19, toHour: 19 },
  { name: "night", fromHour: 20, toHour: 4 },
  { name: "dawn", fromHour: 5, toHour: 5 }
] as const;

// Object type to form block mapping
export type ObjectBlockKey =
  | "weapon"
  | "armor"
  | "container"
  | "light"
  | "drink"
  | "fountain"
  | "food"
  | "money"
  | "wand"
  | "staff"
  | "spells"
  | "portal"
  | "furniture";

export const itemTypeBlockMap: Record<string, ObjectBlockKey> = {
  weapon: "weapon",
  armor: "armor",
  container: "container",
  light: "light",
  drink: "drink",
  fountain: "fountain",
  food: "food",
  money: "money",
  wand: "wand",
  staff: "staff",
  scroll: "spells",
  potion: "spells",
  pill: "spells",
  portal: "portal",
  furniture: "furniture"
};

// Direction opposites for exit validation
export const oppositeDirectionMap: Record<string, string> = {
  north: "south",
  south: "north",
  east: "west",
  west: "east",
  up: "down",
  down: "up"
};
