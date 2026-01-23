import {
  useCallback,
  useEffect,
  useMemo,
  useRef,
  useState,
  type MouseEvent
} from "react";
import type { ColDef, GridApi } from "ag-grid-community";
import {
  applyNodeChanges,
  BaseEdge,
  EdgeLabelRenderer,
  Handle,
  Position,
  type Edge,
  type NodeChange,
  type EdgeProps,
  type Node,
  type NodeProps,
  useStore
} from "reactflow";
import { message } from "@tauri-apps/plugin-dialog";
import { zodResolver } from "@hookform/resolvers/zod";
import {
  useFieldArray,
  useForm,
  useWatch,
  type FieldPath,
} from "react-hook-form";
import { z } from "zod";
import { ScriptView } from "./components/ScriptView";
import { AreaForm } from "./components/AreaForm";
import { AreaGraphView } from "./components/AreaGraphView";
import { RoomForm } from "./components/RoomForm";
import { MobileForm } from "./components/MobileForm";
import { ObjectForm } from "./components/ObjectForm";
import { ResetForm } from "./components/ResetForm";
import { ShopForm } from "./components/ShopForm";
import { QuestForm } from "./components/QuestForm";
import { FactionForm } from "./components/FactionForm";
import { ClassForm } from "./components/ClassForm";
import { RaceForm } from "./components/RaceForm";
import { SkillForm } from "./components/SkillForm";
import { GroupForm } from "./components/GroupForm";
import { CommandForm } from "./components/CommandForm";
import { SocialForm } from "./components/SocialForm";
import { TutorialForm } from "./components/TutorialForm";
import { RecipeForm } from "./components/RecipeForm";
import { GatherSpawnForm } from "./components/GatherSpawnForm";
import { LootForm } from "./components/LootForm";
import { GlobalFormActions } from "./components/GlobalFormActions";
import { TableView } from "./components/TableView";
import { ClassTableView } from "./components/ClassTableView";
import { RaceTableView } from "./components/RaceTableView";
import { SkillTableView } from "./components/SkillTableView";
import { GroupTableView } from "./components/GroupTableView";
import { CommandTableView } from "./components/CommandTableView";
import { SocialTableView } from "./components/SocialTableView";
import { TutorialTableView } from "./components/TutorialTableView";
import { LootTableView } from "./components/LootTableView";
import { InspectorPanel } from "./components/InspectorPanel";
import { EntityTree } from "./components/EntityTree";
import { Topbar } from "./components/Topbar";
import { Statusbar } from "./components/Statusbar";
import { ViewCardHeader } from "./components/ViewCardHeader";
import { ViewTabs } from "./components/ViewTabs";
import { MapView } from "./components/MapView";
import { ViewBody } from "./components/ViewBody";
import type { VnumOption } from "./components/VnumPicker";
import type { EventBinding } from "./data/eventTypes";
import type {
  AreaGraphLink,
  AreaIndexEntry,
  AreaJson,
  ClassDataFile,
  ClassDefinition,
  CommandDataFile,
  CommandDefinition,
  AreaLayoutEntry,
  EditorLayout,
  EditorMeta,
  GroupDataFile,
  GroupDefinition,
  LootDataFile,
  LootEntry,
  LootGroup,
  LootOp,
  LootTable,
  ProjectConfig,
  RaceDataFile,
  RaceDefinition,
  RecipeDefinition,
  GatherSpawnDefinition,
  SocialDataFile,
  SocialDefinition,
  TutorialDataFile,
  TutorialDefinition,
  SkillDataFile,
  SkillDefinition,
  ReferenceData,
  RoomLayoutEntry
} from "./repository/types";
import type { WorldRepository } from "./repository/worldRepository";
import type {
  ValidationIssue,
  ValidationIssueBase,
  ValidationRule,
  ValidationSeverity
} from "./validation/types";
import {
  buildValidationIssues,
  loadValidationConfig
} from "./validation/registry";
import { loadPluginRules } from "./validation/plugins";
import {
  layoutAreaGraphNodes,
  layoutRoomNodes,
  areaNodeSize,
  roomNodeSize
} from "./map/graphLayout";
import {
  containerFlagEnum,
  containerFlags,
  damageTypeEnum,
  damageTypes,
  directionEnum,
  directions,
  exitFlagEnum,
  exitFlags,
  actFlags,
  affectFlags,
  offFlags,
  immFlags,
  resFlags,
  vulnFlags,
  formFlags,
  partFlags,
  skillTargets,
  logFlags,
  showFlags,
  extraFlagEnum,
  extraFlags,
  furnitureFlagEnum,
  furnitureFlags,
  liquidEnum,
  liquids,
  portalFlagEnum,
  portalFlags,
  positionEnum,
  positions,
  roomFlagEnum,
  roomFlags as roomFlagOptions,
  sectorEnum,
  sectors,
  sexEnum,
  sexes,
  sizeEnum,
  sizes,
  wearLocationEnum,
  wearLocations,
  wearFlagEnum,
  wearFlags,
  weaponClassEnum,
  weaponClasses,
  weaponFlagEnum,
  weaponFlags,
  workstationTypes,
  discoveryTypes
} from "./schemas/enums";

const tabs = [
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

type AppProps = {
  repository: WorldRepository;
};

const entityOrder = [
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

const globalEntityOrder = [
  "Classes",
  "Races",
  "Skills",
  "Groups",
  "Commands",
  "Socials",
  "Tutorials",
  "Loot"
] as const;

const primeStatOptions = ["str", "int", "wis", "dex", "con"] as const;
const armorProfOptions = ["old_style", "cloth", "light", "medium", "heavy"] as const;
const classTitleCount = 61;
const classGuildCount = 2;
const raceSkillCount = 5;
const raceStatKeys = ["str", "int", "wis", "dex", "con"] as const;
const defaultSkillLevel = 53;
const defaultSkillRating = 0;
const groupSkillCount = 15;
const lootEntryTypeOptions = ["item", "cp"] as const;
const lootOpTypeOptions = [
  "use_group",
  "add_item",
  "add_cp",
  "mul_cp",
  "mul_all_chances",
  "remove_item",
  "remove_group"
] as const;

const itemTypeOptions = [
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

const resetCommandOptions = [
  "loadMob",
  "placeObj",
  "putObj",
  "giveObj",
  "equipObj",
  "setDoor",
  "randomizeExits"
] as const;

const questTypeOptions = ["visit_mob", "kill_mob", "get_obj"] as const;

type ResetCommand = (typeof resetCommandOptions)[number];

const resetCommandMap: Record<string, ResetCommand> = {
  M: "loadMob",
  O: "placeObj",
  P: "putObj",
  G: "giveObj",
  E: "equipObj",
  D: "setDoor",
  R: "randomizeExits"
};

const resetCommandLabels: Record<ResetCommand, string> = {
  loadMob: "Load Mobile",
  placeObj: "Place Object",
  putObj: "Put Object",
  giveObj: "Give Object",
  equipObj: "Equip Object",
  setDoor: "Set Door",
  randomizeExits: "Randomize Exits"
};

const directionHandleMap: Record<string, { source: string; target: string }> = {
  north: { source: "north-out", target: "south-in" },
  east: { source: "east-out", target: "west-in" },
  south: { source: "south-out", target: "north-in" },
  west: { source: "west-out", target: "east-in" }
};
const areaDirectionHandleMap: Record<string, { source: string; target: string }> =
  {
    north: { source: "north-out", target: "south-in" },
    east: { source: "east-out", target: "west-in" },
    south: { source: "south-out", target: "north-in" },
    west: { source: "west-out", target: "east-in" },
    up: { source: "north-out", target: "south-in" },
    down: { source: "south-out", target: "north-in" }
  };
const externalSourceHandles: Record<string, string> = {
  up: "north-out",
  down: "south-out"
};
const externalDirectionMap: Record<string, Position> = {
  north: Position.Top,
  south: Position.Bottom,
  east: Position.Right,
  west: Position.Left,
  up: Position.Top,
  down: Position.Bottom
};
const externalDirectionLabels: Record<string, string> = {
  north: "N",
  south: "S",
  east: "E",
  west: "W",
  up: "U",
  down: "D"
};
const edgeStubSize = 22;
const edgeClearance = 10;
const externalStubSize = 34;
const edgeDirectionPriority: Record<Position, "horizontal" | "vertical"> = {
  [Position.Left]: "horizontal",
  [Position.Right]: "horizontal",
  [Position.Top]: "vertical",
  [Position.Bottom]: "vertical"
};

type TabId = (typeof tabs)[number]["id"];
type EntityKey = (typeof entityOrder)[number];
type GlobalEntityKey = (typeof globalEntityOrder)[number];
type EditorMode = "Area" | "Global";
type RoomRow = {
  vnum: number;
  name: string;
  sector: string;
  exits: number;
  flags: string;
};

type MobileRow = {
  vnum: number;
  name: string;
  level: number;
  race: string;
  alignment: number;
};

type ObjectRow = {
  vnum: number;
  name: string;
  itemType: string;
  level: number;
};

type ResetRow = {
  index: number;
  command: string;
  entityVnum: string;
  roomVnum: string;
  details: string;
};

type ClassRow = {
  index: number;
  name: string;
  whoName: string;
  primeStat: string;
  armorProf: string;
  weaponVnum: number;
  startLoc: number;
};

type RaceRow = {
  index: number;
  name: string;
  whoName: string;
  pc: string;
  points: number;
  size: string;
  startLoc: number;
};

type SkillRow = {
  index: number;
  name: string;
  target: string;
  minPosition: string;
  spell: string;
  slot: number;
};

type GroupRow = {
  index: number;
  name: string;
  ratingSummary: string;
  skills: number;
};

type CommandRow = {
  index: number;
  name: string;
  function: string;
  position: string;
  level: number;
  log: string;
  category: string;
  loxFunction: string;
};

type SocialRow = {
  index: number;
  name: string;
  noTarget: string;
  target: string;
  self: string;
};

type TutorialRow = {
  index: number;
  name: string;
  minLevel: number;
  steps: number;
};

type LootRow = {
  id: string;
  kind: "group" | "table";
  index: number;
  name: string;
  details: string;
};

type RecipeRow = {
  vnum: number;
  name: string;
  skill: string;
  inputs: string;
  output: string;
};

type GatherSpawnRow = {
  index: number;
  vnum: number;
  sector: string;
  quantity: number;
  respawnTimer: number;
};

type ShopRow = {
  keeper: number;
  buyTypes: string;
  profitBuy: number;
  profitSell: number;
  hours: string;
};

type QuestRow = {
  vnum: number;
  name: string;
  type: string;
  level: number;
  target: string;
  rewards: string;
};

type FactionRow = {
  vnum: number;
  name: string;
  defaultStanding: number;
  allies: string;
  opposing: string;
};

type ExternalExit = {
  fromVnum: number;
  fromName: string;
  direction: string;
  toVnum: number;
  areaName: string | null;
};

type RoomLayoutMap = Record<string, RoomLayoutEntry>;
type AreaLayoutMap = Record<string, AreaLayoutEntry>;

type AreaGraphEntry = {
  id: string;
  name: string;
  vnumRange: [number, number] | null;
};

type AreaGraphNodeData = {
  label: string;
  range: string;
  isCurrent?: boolean;
  isMatch?: boolean;
  locked?: boolean;
  dirty?: boolean;
};

type ExitValidationResult = {
  issues: ValidationIssueBase[];
  invalidEdgeIds: Set<string>;
};

type RoomNodeData = {
  vnum: number;
  label: string;
  sector: string;
  dirty?: boolean;
  locked?: boolean;
  upExitTargets?: number[];
  downExitTargets?: number[];
  onNavigate?: (vnum: number) => void;
  grid?: {
    x: number;
    y: number;
  };
};

const optionalIntSchema = z.preprocess((value) => {
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

const optionalEnumSchema = <T extends z.ZodTypeAny>(schema: T) =>
  z.preprocess((value) => {
    if (value === "" || value === null || value === undefined) {
      return undefined;
    }
    return value;
  }, schema.optional());

const optionalSectorSchema = optionalEnumSchema(sectorEnum);
const optionalPositionSchema = optionalEnumSchema(positionEnum);
const optionalSexSchema = optionalEnumSchema(sexEnum);
const optionalSizeSchema = optionalEnumSchema(sizeEnum);
const optionalDamageTypeSchema = optionalEnumSchema(damageTypeEnum);
const optionalDirectionSchema = optionalEnumSchema(directionEnum);
const optionalWearLocationSchema = optionalEnumSchema(wearLocationEnum);
const checklistStatusOptions = ["todo", "inProgress", "done"] as const;

const areaFormSchema = z.object({
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

const exitFormSchema = z.object({
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

const diceFormSchema = z
  .object({
    number: optionalIntSchema,
    type: optionalIntSchema,
    bonus: optionalIntSchema
  })
  .optional();

const mobileFormSchema = z.object({
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
  damageDice: diceFormSchema
});

const weaponFormSchema = z
  .object({
    class: optionalEnumSchema(weaponClassEnum),
    diceNumber: optionalIntSchema,
    diceType: optionalIntSchema,
    damageType: optionalDamageTypeSchema,
    flags: z.array(weaponFlagEnum).optional()
  })
  .optional();

const armorFormSchema = z
  .object({
    acPierce: optionalIntSchema,
    acBash: optionalIntSchema,
    acSlash: optionalIntSchema,
    acExotic: optionalIntSchema
  })
  .optional();

const containerFormSchema = z
  .object({
    capacity: optionalIntSchema,
    flags: z.array(containerFlagEnum).optional(),
    keyVnum: optionalIntSchema,
    maxWeight: optionalIntSchema,
    weightMult: optionalIntSchema
  })
  .optional();

const lightFormSchema = z
  .object({
    hours: optionalIntSchema
  })
  .optional();

const drinkFormSchema = z
  .object({
    capacity: optionalIntSchema,
    remaining: optionalIntSchema,
    liquid: optionalEnumSchema(liquidEnum),
    poisoned: z.boolean().optional()
  })
  .optional();

const foodFormSchema = z
  .object({
    foodHours: optionalIntSchema,
    fullHours: optionalIntSchema,
    poisoned: z.boolean().optional()
  })
  .optional();

const moneyFormSchema = z
  .object({
    gold: optionalIntSchema,
    silver: optionalIntSchema
  })
  .optional();

const wandFormSchema = z
  .object({
    level: optionalIntSchema,
    totalCharges: optionalIntSchema,
    chargesLeft: optionalIntSchema,
    spell: z.string().optional()
  })
  .optional();

const spellsFormSchema = z
  .object({
    level: optionalIntSchema,
    spell1: z.string().optional(),
    spell2: z.string().optional(),
    spell3: z.string().optional(),
    spell4: z.string().optional()
  })
  .optional();

const portalFormSchema = z
  .object({
    charges: optionalIntSchema,
    exitFlags: z.array(exitFlagEnum).optional(),
    portalFlags: z.array(portalFlagEnum).optional(),
    toVnum: optionalIntSchema
  })
  .optional();

const furnitureFormSchema = z
  .object({
    slots: optionalIntSchema,
    weight: optionalIntSchema,
    flags: z.array(furnitureFlagEnum).optional(),
    healBonus: optionalIntSchema,
    manaBonus: optionalIntSchema,
    maxPeople: optionalIntSchema
  })
  .optional();

const objectFormSchema = z.object({
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

const resetFormSchema = z
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

const roomFormSchema = z.object({
  vnum: z.number().int(),
  name: z.string().min(1, "Name is required."),
  description: z.string().min(1, "Description is required."),
  sectorType: optionalSectorSchema,
  roomFlags: z.array(roomFlagEnum).optional(),
  exits: z.array(exitFormSchema).optional()
});

const shopFormSchema = z.object({
  keeper: optionalIntSchema,
  buyTypes: z.array(optionalIntSchema).length(5),
  profitBuy: optionalIntSchema,
  profitSell: optionalIntSchema,
  openHour: optionalIntSchema,
  closeHour: optionalIntSchema
});

const questFormSchema = z.object({
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

const factionFormSchema = z.object({
  vnum: z.number().int(),
  name: z.string().optional(),
  defaultStanding: optionalIntSchema,
  alliesCsv: z.string().optional(),
  opposingCsv: z.string().optional()
});

const classFormSchema = z.object({
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

const raceFormSchema = z.object({
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

const skillFormSchema = z.object({
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

const groupFormSchema = z.object({
  name: z.string().min(1),
  ratings: z.record(z.number().int()),
  skills: z.array(z.string()).length(groupSkillCount)
});

const commandFormSchema = z.object({
  name: z.string().min(1),
  function: z.string().optional(),
  position: z.string().optional(),
  level: z.number().int(),
  log: z.string().optional(),
  category: z.string().optional(),
  loxFunction: z.string().optional()
});

const socialFormSchema = z.object({
  name: z.string().min(1),
  charNoArg: z.string().optional(),
  othersNoArg: z.string().optional(),
  charFound: z.string().optional(),
  othersFound: z.string().optional(),
  victFound: z.string().optional(),
  charAuto: z.string().optional(),
  othersAuto: z.string().optional()
});

const tutorialFormSchema = z.object({
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

const lootEntryFormSchema = z.object({
  type: z.enum(lootEntryTypeOptions),
  vnum: optionalIntSchema,
  minQty: z.number().int().min(0).optional(),
  maxQty: z.number().int().min(0).optional(),
  weight: z.number().int().min(0).optional()
});

const lootOpFormSchema = z.object({
  op: z.enum(lootOpTypeOptions),
  group: z.string().optional(),
  vnum: optionalIntSchema,
  chance: z.number().int().min(0).optional(),
  minQty: z.number().int().min(0).optional(),
  maxQty: z.number().int().min(0).optional(),
  multiplier: z.number().int().min(0).optional()
});

const lootFormSchema = z.object({
  kind: z.enum(["group", "table"]),
  name: z.string().min(1),
  rolls: z.number().int().min(0).optional(),
  entries: z.array(lootEntryFormSchema).optional(),
  parent: z.string().optional(),
  ops: z.array(lootOpFormSchema).optional()
});

const recipeInputFormSchema = z.object({
  vnum: optionalIntSchema,
  quantity: optionalIntSchema
});

const recipeFormSchema = z.object({
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

const gatherSpawnFormSchema = z.object({
  spawnSector: optionalSectorSchema,
  vnum: optionalIntSchema,
  quantity: optionalIntSchema,
  respawnTimer: optionalIntSchema
});

type RoomFormValues = z.infer<typeof roomFormSchema>;
type AreaFormValues = z.infer<typeof areaFormSchema>;
type MobileFormValues = z.infer<typeof mobileFormSchema>;
type ObjectFormValues = z.infer<typeof objectFormSchema>;
type ResetFormValues = z.infer<typeof resetFormSchema>;
type ShopFormValues = z.infer<typeof shopFormSchema>;
type QuestFormValues = z.infer<typeof questFormSchema>;
type FactionFormValues = z.infer<typeof factionFormSchema>;
type ClassFormValues = z.infer<typeof classFormSchema>;
type RaceFormValues = z.infer<typeof raceFormSchema>;
type SkillFormValues = z.infer<typeof skillFormSchema>;
type GroupFormValues = z.infer<typeof groupFormSchema>;
type CommandFormValues = z.infer<typeof commandFormSchema>;
type SocialFormValues = z.infer<typeof socialFormSchema>;
type TutorialFormValues = z.infer<typeof tutorialFormSchema>;
type LootFormValues = z.infer<typeof lootFormSchema>;
type RecipeFormValues = z.infer<typeof recipeFormSchema>;
type GatherSpawnFormValues = z.infer<typeof gatherSpawnFormSchema>;

const entityKindLabels: Record<EntityKey, string> = {
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

const entityDataKeys: Record<EntityKey, string> = {
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

function fileNameFromPath(path: string | null): string {
  if (!path) {
    return "No area loaded";
  }
  const normalized = path.replace(/[/\\\\]+/g, "/");
  return normalized.split("/").pop() ?? path;
}

function buildEditorMeta(
  existing: EditorMeta | null,
  activeTab: string,
  selectedEntity: string,
  roomLayout: RoomLayoutMap,
  areaLayout: AreaLayoutMap
): EditorMeta {
  const now = new Date().toISOString();
  const base: EditorMeta = existing ?? {
    version: 1,
    updatedAt: now,
    view: { activeTab },
    selection: { entityType: selectedEntity },
    layout: {}
  };
  const nextLayout: EditorLayout = {
    ...(base.layout ?? {}),
    rooms: roomLayout,
    areas: areaLayout
  };

  return {
    ...base,
    version: 1,
    updatedAt: now,
    view: {
      ...base.view,
      activeTab
    },
    selection: {
      ...base.selection,
      entityType: selectedEntity
    },
    layout: nextLayout
  };
}

function parseVnum(value: unknown): number | null {
  if (typeof value === "number") {
    return value;
  }
  if (typeof value === "string" && value.trim().length) {
    const parsed = Number(value);
    return Number.isFinite(parsed) ? parsed : null;
  }
  return null;
}

function parseNumberList(value: string | undefined): number[] {
  if (!value) {
    return [];
  }
  return value
    .split(/[,\\s]+/)
    .map((token) => Number(token.trim()))
    .filter((num) => Number.isFinite(num));
}

function formatNumberList(values: unknown): string {
  if (!Array.isArray(values)) {
    return "";
  }
  return values
    .map((value) => parseVnum(value))
    .filter((num): num is number => num !== null)
    .join(", ");
}

type DiceFormInput = {
  number?: number;
  type?: number;
  bonus?: number;
};

function normalizeDice(dice: DiceFormInput | undefined) {
  if (!dice) {
    return undefined;
  }
  if (dice.number === undefined || dice.type === undefined) {
    return undefined;
  }
  const next: Record<string, number> = {
    number: dice.number,
    type: dice.type
  };
  if (dice.bonus !== undefined) {
    next.bonus = dice.bonus;
  }
  return next;
}

function normalizeOptionalText(value: string | undefined) {
  if (!value) {
    return undefined;
  }
  const trimmed = value.trim();
  return trimmed.length ? trimmed : undefined;
}

function normalizeChecklistStatus(
  value: unknown
): "todo" | "inProgress" | "done" {
  if (value === "todo" || value === "inProgress" || value === "done") {
    return value;
  }
  if (typeof value === "number") {
    if (value === 1) {
      return "inProgress";
    }
    if (value === 2) {
      return "done";
    }
    return "todo";
  }
  if (typeof value === "string") {
    const trimmed = value.trim().toLowerCase();
    if (trimmed === "inprogress") {
      return "inProgress";
    }
    if (trimmed === "done") {
      return "done";
    }
    const numeric = Number(trimmed);
    if (Number.isFinite(numeric)) {
      return normalizeChecklistStatus(numeric);
    }
  }
  return "todo";
}

function normalizeLineEndingsForDisplay(value: string | undefined) {
  if (typeof value !== "string") {
    return "";
  }
  return value.replace(/\r\n/g, "\n").replace(/\n\r/g, "\n").replace(/\r/g, "\n");
}

function normalizeLineEndingsForMud(value: string) {
  const normalized = value
    .replace(/\r\n/g, "\n")
    .replace(/\n\r/g, "\n")
    .replace(/\r/g, "\n");
  return normalized.replace(/\n/g, "\n\r");
}

function normalizeOptionalMudText(value: string | undefined) {
  if (value === undefined) {
    return undefined;
  }
  if (!value.trim().length) {
    return undefined;
  }
  return normalizeLineEndingsForMud(value);
}

function normalizeOptionalScript(value: string | undefined) {
  if (value === undefined) {
    return undefined;
  }
  const normalized = normalizeLineEndingsForMud(value);
  return normalized.trim().length ? normalized : undefined;
}

type ObjectBlockKey =
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

const itemTypeBlockMap: Record<string, ObjectBlockKey> = {
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

function findByVnum(list: unknown[], vnum: number): Record<string, unknown> | null {
  for (const entry of list) {
    if (!entry || typeof entry !== "object") {
      continue;
    }
    const record = entry as Record<string, unknown>;
    const entryVnum = parseVnum(record.vnum);
    if (entryVnum === vnum) {
      return record;
    }
  }
  return null;
}

function findShopByKeeper(
  list: unknown[],
  keeper: number
): Record<string, unknown> | null {
  for (const entry of list) {
    if (!entry || typeof entry !== "object") {
      continue;
    }
    const record = entry as Record<string, unknown>;
    const entryKeeper = parseVnum(record.keeper);
    if (entryKeeper === keeper) {
      return record;
    }
  }
  return null;
}

function resolveResetCommand(record: Record<string, unknown> | null): string {
  if (record && typeof record.commandName === "string") {
    const trimmed = record.commandName.trim();
    if (trimmed) {
      return trimmed;
    }
  }
  const command = record && typeof record.command === "string" ? record.command : "";
  if (!command) {
    return "";
  }
  const mapped = resetCommandMap[command.toUpperCase()];
  return mapped ?? command;
}

function getDefaultSelection(
  areaData: AreaJson | null,
  entity: EntityKey,
  current: number | null
): number | null {
  const list = getEntityList(areaData, entity);
  if (!list.length) {
    return null;
  }
  if (entity === "Resets") {
    if (current !== null && current >= 0 && current < list.length) {
      return current;
    }
    return 0;
  }
  if (entity === "Shops") {
    if (current !== null && findShopByKeeper(list, current)) {
      return current;
    }
    const first = list[0] as Record<string, unknown>;
    return parseVnum(first?.keeper);
  }
  if (current !== null && findByVnum(list, current)) {
    return current;
  }
  const first = list[0] as Record<string, unknown>;
  return parseVnum(first?.vnum);
}

function getEntityList(areaData: AreaJson | null, key: EntityKey): unknown[] {
  if (!areaData) {
    return [];
  }
  if (key === "Area") {
    const areadata = (areaData as Record<string, unknown>).areadata;
    return areadata && typeof areadata === "object" ? [areadata] : [];
  }
  if (key === "Loot") {
    const loot = (areaData as Record<string, unknown>).loot;
    if (!loot || typeof loot !== "object") {
      return [];
    }
    const record = loot as Record<string, unknown>;
    const groups = Array.isArray(record.groups) ? record.groups : [];
    const tables = Array.isArray(record.tables) ? record.tables : [];
    return [...groups, ...tables];
  }
  const list = (areaData as Record<string, unknown>)[entityDataKeys[key]];
  return Array.isArray(list) ? list : [];
}

function getAreaVnumRange(areaData: AreaJson | null): string | null {
  if (!areaData) {
    return null;
  }
  const areadata = (areaData as Record<string, unknown>).areadata;
  if (!areadata || typeof areadata !== "object") {
    return null;
  }
  const vnumRange = (areadata as Record<string, unknown>).vnumRange;
  if (
    Array.isArray(vnumRange) &&
    vnumRange.length === 2 &&
    typeof vnumRange[0] === "number" &&
    typeof vnumRange[1] === "number"
  ) {
    return `${vnumRange[0]}-${vnumRange[1]}`;
  }
  return null;
}

function formatVnumRange(range: [number, number] | null): string {
  if (!range) {
    return "n/a";
  }
  return `${range[0]}-${range[1]}`;
}

function getAreaVnumBounds(
  areaData: AreaJson | null
): { min: number; max: number } | null {
  if (!areaData) {
    return null;
  }
  const areadata = (areaData as Record<string, unknown>).areadata;
  if (!areadata || typeof areadata !== "object") {
    return null;
  }
  const vnumRange = (areadata as Record<string, unknown>).vnumRange;
  if (
    Array.isArray(vnumRange) &&
    vnumRange.length === 2 &&
    typeof vnumRange[0] === "number" &&
    typeof vnumRange[1] === "number"
  ) {
    return { min: vnumRange[0], max: vnumRange[1] };
  }
  return null;
}

function getNextEntityVnum(areaData: AreaJson, entity: EntityKey): number | null {
  const entries = getEntityList(areaData, entity);
  const used = new Set<number>();
  entries.forEach((entry) => {
    if (!entry || typeof entry !== "object") {
      return;
    }
    const vnum = parseVnum((entry as Record<string, unknown>).vnum);
    if (vnum !== null) {
      used.add(vnum);
    }
  });
  const bounds = getAreaVnumBounds(areaData);
  if (bounds) {
    const { min, max } = bounds;
    for (let candidate = min; candidate <= max; candidate += 1) {
      if (!used.has(candidate)) {
        return candidate;
      }
    }
    return null;
  }
  if (!used.size) {
    return 1;
  }
  return Math.max(...used) + 1;
}

function getFirstEntityVnum(areaData: AreaJson, entity: EntityKey): number | null {
  const entries = getEntityList(areaData, entity);
  for (const entry of entries) {
    if (!entry || typeof entry !== "object") {
      continue;
    }
    const vnum = parseVnum((entry as Record<string, unknown>).vnum);
    if (vnum !== null) {
      return vnum;
    }
  }
  return null;
}

function getFirstString(value: unknown, fallback: string): string {
  return typeof value === "string" && value.trim().length ? value : fallback;
}

function RoomNode({ data, selected }: NodeProps<RoomNodeData>) {
  const upTargets = data.upExitTargets ?? [];
  const downTargets = data.downExitTargets ?? [];
  const handleNavigate =
    (targets: number[]) => (event: MouseEvent<HTMLButtonElement>) => {
      event.stopPropagation();
      if (!targets.length) {
        return;
      }
      data.onNavigate?.(targets[0]);
    };
  const buildTitle = (label: string, targets: number[]) =>
    targets.length
      ? `${label} to ${targets.join(", ")}`
      : `${label} exit`;
  return (
    <div className={`room-node${selected ? " room-node--selected" : ""}`}>
      {data.dirty ? (
        <div className="room-node__dirty" title="Position moved but not locked">
          Dirty
        </div>
      ) : null}
      <Handle
        id="north-out"
        type="source"
        position={Position.Top}
        className="room-node__handle"
      />
      <Handle
        id="north-in"
        type="target"
        position={Position.Top}
        className="room-node__handle"
      />
      <Handle
        id="east-out"
        type="source"
        position={Position.Right}
        className="room-node__handle"
      />
      <Handle
        id="east-in"
        type="target"
        position={Position.Right}
        className="room-node__handle"
      />
      <Handle
        id="south-out"
        type="source"
        position={Position.Bottom}
        className="room-node__handle"
      />
      <Handle
        id="south-in"
        type="target"
        position={Position.Bottom}
        className="room-node__handle"
      />
      <Handle
        id="west-out"
        type="source"
        position={Position.Left}
        className="room-node__handle"
      />
      <Handle
        id="west-in"
        type="target"
        position={Position.Left}
        className="room-node__handle"
      />
      {upTargets.length || downTargets.length ? (
        <div className="room-node__exits">
          {upTargets.length ? (
            <button
              className="room-node__exit-button room-node__exit-button--up"
              type="button"
              onClick={handleNavigate(upTargets)}
              title={buildTitle("Up exit", upTargets)}
            >
              <span className="room-node__exit-icon">⬆</span>
              {upTargets.length > 1 ? (
                <span className="room-node__exit-count">{upTargets.length}</span>
              ) : null}
            </button>
          ) : null}
          {downTargets.length ? (
            <button
              className="room-node__exit-button room-node__exit-button--down"
              type="button"
              onClick={handleNavigate(downTargets)}
              title={buildTitle("Down exit", downTargets)}
            >
              <span className="room-node__exit-icon">⬇</span>
              {downTargets.length > 1 ? (
                <span className="room-node__exit-count">{downTargets.length}</span>
              ) : null}
            </button>
          ) : null}
        </div>
      ) : null}
      <div className="room-node__vnum">{data.vnum}</div>
      <div className="room-node__name">{data.label}</div>
      <div className="room-node__sector">{data.sector}</div>
    </div>
  );
}

const roomNodeTypes = { room: RoomNode };

type Point = { x: number; y: number };
type NodeBounds = {
  id: string;
  left: number;
  right: number;
  top: number;
  bottom: number;
};
type OrthogonalPath = {
  points: Point[];
  orientation: "horizontal" | "vertical";
};

function offsetPoint(point: Point, direction: Position, distance: number): Point {
  switch (direction) {
    case Position.Left:
      return { x: point.x - distance, y: point.y };
    case Position.Right:
      return { x: point.x + distance, y: point.y };
    case Position.Top:
      return { x: point.x, y: point.y - distance };
    case Position.Bottom:
      return { x: point.x, y: point.y + distance };
    default:
      return point;
  }
}

function pointsToPath(points: Point[]): string {
  if (!points.length) {
    return "";
  }
  return points
    .map((point, index) =>
      `${index === 0 ? "M" : "L"}${point.x} ${point.y}`
    )
    .join(" ");
}

function getPathLength(points: Point[]): number {
  let length = 0;
  for (let index = 1; index < points.length; index += 1) {
    const prev = points[index - 1];
    const next = points[index];
    length += Math.abs(next.x - prev.x) + Math.abs(next.y - prev.y);
  }
  return length;
}

function getPathMidpoint(points: Point[]): Point {
  if (!points.length) {
    return { x: 0, y: 0 };
  }
  const total = getPathLength(points);
  if (total === 0) {
    return points[0];
  }
  let remaining = total / 2;
  for (let index = 1; index < points.length; index += 1) {
    const prev = points[index - 1];
    const next = points[index];
    const segmentLength = Math.abs(next.x - prev.x) + Math.abs(next.y - prev.y);
    if (segmentLength >= remaining) {
      const ratio = segmentLength === 0 ? 0 : remaining / segmentLength;
      return {
        x: prev.x + (next.x - prev.x) * ratio,
        y: prev.y + (next.y - prev.y) * ratio
      };
    }
    remaining -= segmentLength;
  }
  return points[points.length - 1];
}

function segmentIntersectsRect(
  start: Point,
  end: Point,
  rect: NodeBounds,
  padding: number
): boolean {
  const left = rect.left - padding;
  const right = rect.right + padding;
  const top = rect.top - padding;
  const bottom = rect.bottom + padding;
  if (start.x === end.x) {
    const x = start.x;
    const minY = Math.min(start.y, end.y);
    const maxY = Math.max(start.y, end.y);
    return x >= left && x <= right && maxY >= top && minY <= bottom;
  }
  if (start.y === end.y) {
    const y = start.y;
    const minX = Math.min(start.x, end.x);
    const maxX = Math.max(start.x, end.x);
    return y >= top && y <= bottom && maxX >= left && minX <= right;
  }
  return false;
}

function countPathIntersections(
  points: Point[],
  obstacles: NodeBounds[],
  padding: number
): number {
  let count = 0;
  for (let index = 1; index < points.length; index += 1) {
    const start = points[index - 1];
    const end = points[index];
    for (const obstacle of obstacles) {
      if (segmentIntersectsRect(start, end, obstacle, padding)) {
        count += 1;
        break;
      }
    }
  }
  return count;
}

function createOrthogonalPath(
  start: Point,
  end: Point,
  sourceDirection: Position,
  targetDirection: Position,
  orientation: "horizontal" | "vertical"
): OrthogonalPath {
  const sourceStub = offsetPoint(start, sourceDirection, edgeStubSize);
  const targetStub = offsetPoint(end, targetDirection, edgeStubSize);
  if (orientation === "horizontal") {
    const midX = (sourceStub.x + targetStub.x) / 2;
    return {
      orientation,
      points: [
        start,
        sourceStub,
        { x: midX, y: sourceStub.y },
        { x: midX, y: targetStub.y },
        targetStub,
        end
      ]
    };
  }
  const midY = (sourceStub.y + targetStub.y) / 2;
  return {
    orientation,
    points: [
      start,
      sourceStub,
      { x: sourceStub.x, y: midY },
      { x: targetStub.x, y: midY },
      targetStub,
      end
    ]
  };
}

function adjustOrthogonalPath(
  path: OrthogonalPath,
  obstacles: NodeBounds[]
): OrthogonalPath {
  if (path.points.length < 4) {
    return path;
  }
  const points = [...path.points];
  if (path.orientation === "horizontal") {
    const start = points[2];
    const end = points[3];
    const blockers = obstacles.filter((obstacle) =>
      segmentIntersectsRect(start, end, obstacle, edgeClearance)
    );
    if (!blockers.length) {
      return path;
    }
    const candidateXs = new Set<number>([start.x]);
    blockers.forEach((blocker) => {
      candidateXs.add(blocker.left - edgeClearance * 2);
      candidateXs.add(blocker.right + edgeClearance * 2);
    });
    const sortedCandidates = [...candidateXs].sort(
      (a, b) => Math.abs(a - start.x) - Math.abs(b - start.x)
    );
    for (const candidate of sortedCandidates) {
      points[2] = { x: candidate, y: points[2].y };
      points[3] = { x: candidate, y: points[3].y };
      if (!countPathIntersections(points, obstacles, edgeClearance)) {
        return { ...path, points };
      }
    }
    return path;
  }
  const start = points[2];
  const end = points[3];
  const blockers = obstacles.filter((obstacle) =>
    segmentIntersectsRect(start, end, obstacle, edgeClearance)
  );
  if (!blockers.length) {
    return path;
  }
  const candidateYs = new Set<number>([start.y]);
  blockers.forEach((blocker) => {
    candidateYs.add(blocker.top - edgeClearance * 2);
    candidateYs.add(blocker.bottom + edgeClearance * 2);
  });
  const sortedCandidates = [...candidateYs].sort(
    (a, b) => Math.abs(a - start.y) - Math.abs(b - start.y)
  );
  for (const candidate of sortedCandidates) {
    points[2] = { x: points[2].x, y: candidate };
    points[3] = { x: points[3].x, y: candidate };
    if (!countPathIntersections(points, obstacles, edgeClearance)) {
      return { ...path, points };
    }
  }
  return path;
}

function buildOrthogonalEdgePath(
  start: Point,
  end: Point,
  sourceDirection: Position,
  targetDirection: Position,
  obstacles: NodeBounds[]
): { path: string; labelPoint: Point } {
  const preferred = edgeDirectionPriority[sourceDirection] ?? "horizontal";
  const alternate = preferred === "horizontal" ? "vertical" : "horizontal";
  const primaryPath = createOrthogonalPath(
    start,
    end,
    sourceDirection,
    targetDirection,
    preferred
  );
  const secondaryPath = createOrthogonalPath(
    start,
    end,
    sourceDirection,
    targetDirection,
    alternate
  );
  const primaryScore = countPathIntersections(
    primaryPath.points,
    obstacles,
    edgeClearance
  );
  const secondaryScore = countPathIntersections(
    secondaryPath.points,
    obstacles,
    edgeClearance
  );
  let chosen =
    primaryScore < secondaryScore
      ? primaryPath
      : secondaryScore < primaryScore
        ? secondaryPath
        : getPathLength(primaryPath.points) <=
            getPathLength(secondaryPath.points)
          ? primaryPath
          : secondaryPath;
  chosen = adjustOrthogonalPath(chosen, obstacles);
  return {
    path: pointsToPath(chosen.points),
    labelPoint: getPathMidpoint(chosen.points)
  };
}

function RoomEdge({
  id,
  sourceX,
  sourceY,
  targetX,
  targetY,
  source,
  target,
  sourcePosition,
  targetPosition,
  data,
  label,
  selected,
  markerEnd,
  interactionWidth
}: EdgeProps<{
  dirKey?: string;
  exitFlags?: string[];
  invalid?: boolean;
  nodeType?: string;
  nodeSize?: { width: number; height: number };
}>) {
  const nodeInternals = useStore((state) => state.nodeInternals);
  const nodeType =
    data?.nodeType && typeof data.nodeType === "string"
      ? data.nodeType
      : "room";
  const fallbackSize =
    data?.nodeSize ??
    (nodeType === "area" ? areaNodeSize : roomNodeSize);
  const obstacles = useMemo(() => {
    const entries = Array.from(nodeInternals.values());
    return entries
      .filter((node) => node.type === nodeType)
      .map((node) => {
        const width = node.width ?? fallbackSize.width;
        const height = node.height ?? fallbackSize.height;
        const position = node.positionAbsolute ?? node.position;
        return {
          id: node.id,
          left: position.x,
          right: position.x + width,
          top: position.y,
          bottom: position.y + height
        };
      });
  }, [nodeInternals, nodeType, fallbackSize]);
  const filteredObstacles = useMemo(
    () =>
      obstacles.filter(
        (obstacle) => obstacle.id !== source && obstacle.id !== target
      ),
    [obstacles, source, target]
  );
  const safeSourcePosition = sourcePosition ?? Position.Right;
  const safeTargetPosition = targetPosition ?? Position.Left;
  const { path, labelPoint } = useMemo(
    () =>
      buildOrthogonalEdgePath(
        { x: sourceX, y: sourceY },
        { x: targetX, y: targetY },
        safeSourcePosition,
        safeTargetPosition,
        filteredObstacles
      ),
    [
      sourceX,
      sourceY,
      targetX,
      targetY,
      safeSourcePosition,
      safeTargetPosition,
      filteredObstacles
    ]
  );
  const exitFlags = Array.isArray(data?.exitFlags) ? data.exitFlags : [];
  const isInvalid = data?.invalid === true;
  const isLocked = exitFlags.includes("locked");
  const isDoor = isLocked || exitFlags.includes("door") || exitFlags.includes("closed");
  const stroke = isInvalid
    ? "rgba(167, 60, 60, 0.9)"
    : isLocked
      ? "rgba(167, 60, 60, 0.85)"
      : isDoor
        ? "rgba(138, 92, 34, 0.75)"
        : "rgba(47, 108, 106, 0.55)";
  const edgeStyle = {
    stroke,
    strokeWidth: selected
      ? 2.4
      : isInvalid
        ? 2.2
        : isLocked
          ? 2.1
          : isDoor
            ? 1.8
            : 1.4,
    strokeDasharray: isInvalid
      ? "3 3"
      : isLocked
        ? "4 3"
        : isDoor
          ? "6 4"
          : undefined
  };
  const labelClass = isInvalid
    ? "room-edge__label room-edge__label--invalid"
    : isLocked
      ? "room-edge__label room-edge__label--locked"
      : isDoor
        ? "room-edge__label room-edge__label--door"
        : "room-edge__label";
  return (
    <>
      <BaseEdge
        id={id}
        path={path}
        markerEnd={markerEnd}
        style={edgeStyle}
        interactionWidth={interactionWidth}
      />
      {typeof label === "string" && label.length ? (
        <EdgeLabelRenderer>
          <div
            className={labelClass}
            style={{
              transform: `translate(-50%, -50%) translate(${labelPoint.x}px, ${labelPoint.y}px)`
            }}
          >
            {label}
          </div>
        </EdgeLabelRenderer>
      ) : null}
    </>
  );
}

function VerticalEdge({
  id,
  sourceX,
  sourceY,
  targetX,
  targetY,
  markerEnd,
  interactionWidth
}: EdgeProps<{ dirKey?: string }>) {
  const path = `M${sourceX} ${sourceY} L${targetX} ${targetY}`;
  return (
    <BaseEdge
      id={id}
      path={path}
      markerEnd={markerEnd}
      interactionWidth={interactionWidth}
      style={{
        stroke: "rgba(47, 108, 106, 0.4)",
        strokeWidth: 1.2,
        strokeDasharray: "4 5"
      }}
    />
  );
}

function ExternalStubEdge({
  id,
  sourceX,
  sourceY,
  sourcePosition,
  data,
  markerEnd,
  interactionWidth
}: EdgeProps<{ dirKey?: string; targetVnum?: number; invalid?: boolean }>) {
  const dirKey = data?.dirKey ?? "";
  const rawLabel =
    externalDirectionLabels[dirKey] ?? dirKey.trim().toUpperCase();
  const labelCode = rawLabel.length ? rawLabel : "X";
  const labelSuffix =
    typeof data?.targetVnum === "number" ? ` ${data.targetVnum}` : "";
  const isInvalid = data?.invalid === true;
  const direction =
    externalDirectionMap[dirKey] ?? sourcePosition ?? Position.Right;
  const start = { x: sourceX, y: sourceY };
  const end = offsetPoint(start, direction, externalStubSize);
  const labelPoint = offsetPoint(start, direction, externalStubSize + 16);
  const path = `M${start.x} ${start.y} L${end.x} ${end.y}`;
  return (
    <>
      <BaseEdge
        id={id}
        path={path}
        markerEnd={markerEnd}
        interactionWidth={interactionWidth}
        style={{
          stroke: isInvalid ? "rgba(167, 60, 60, 0.7)" : "rgba(47, 108, 106, 0.35)",
          strokeWidth: isInvalid ? 1.6 : 1.1,
          strokeDasharray: isInvalid ? "3 4" : "2 6"
        }}
      />
      <EdgeLabelRenderer>
        <div
          className={`room-edge__label room-edge__label--external${
            isInvalid ? " room-edge__label--invalid" : ""
          }`}
          style={{
            transform: `translate(-50%, -50%) translate(${labelPoint.x}px, ${labelPoint.y}px)`
          }}
        >
          {labelCode}
          {labelSuffix}
        </div>
      </EdgeLabelRenderer>
    </>
  );
}

const roomEdgeTypes = {
  room: RoomEdge,
  vertical: VerticalEdge,
  external: ExternalStubEdge
};
const areaEdgeTypes = { area: RoomEdge };

function buildVnumOptions(
  areaData: AreaJson | null,
  entity: "Rooms" | "Mobiles" | "Objects"
): VnumOption[] {
  if (!areaData) {
    return [];
  }
  const list = getEntityList(areaData, entity);
  const options: VnumOption[] = [];
  const entityLabel =
    entity === "Rooms" ? "Room" : entity === "Mobiles" ? "Mobile" : "Object";
  for (const entry of list) {
    if (!entry || typeof entry !== "object") {
      continue;
    }
    const record = entry as Record<string, unknown>;
    const vnum = parseVnum(record.vnum);
    if (vnum === null) {
      continue;
    }
    let name = "VNUM";
    if (entity === "Rooms") {
      name = getFirstString(record.name, "(unnamed room)");
    } else if (entity === "Mobiles") {
      name = getFirstString(record.shortDescr, "(unnamed mobile)");
    } else {
      name = getFirstString(record.shortDescr, "(unnamed object)");
    }
    options.push({
      vnum,
      label: `${entityLabel} · ${name}`,
      entityType: entityLabel,
      name
    });
  }
  return options;
}

function buildSelectionSummary(
  selectedEntity: EntityKey,
  areaData: AreaJson | null,
  selectedVnums: Partial<Record<EntityKey, number | null>>,
  lootContext?: {
    data: LootDataFile | null;
    kind: "group" | "table" | null;
    index: number | null;
  }
) {
  const list = getEntityList(areaData, selectedEntity);
  const count = list.length;
  const selectedValue =
    selectedVnums[selectedEntity] !== null &&
    selectedVnums[selectedEntity] !== undefined
      ? (selectedVnums[selectedEntity] as number)
      : null;
  const selected =
    selectedValue !== null
      ? selectedEntity === "Resets"
        ? (list[selectedValue] as Record<string, unknown> | undefined) ?? null
        : selectedEntity === "Shops"
          ? findShopByKeeper(list, selectedValue)
          : findByVnum(list, selectedValue)
      : null;
  const first = (selected ?? list[0] ?? {}) as Record<string, unknown>;
  const vnumRange = getAreaVnumRange(areaData);
  const vnum = parseVnum(first.vnum);
  const emptyLabel = `No ${selectedEntity.toLowerCase()} loaded`;

  let selectionLabel = emptyLabel;
  let flags: string[] = [];
  let exits = "n/a";
  const lootData = lootContext?.data ?? null;
  const lootKind = lootContext?.kind ?? null;
  const lootIndex = lootContext?.index ?? null;

  switch (selectedEntity) {
    case "Area": {
      selectionLabel = count
        ? getFirstString(first.name, "Area data")
        : emptyLabel;
      exits = "n/a";
      break;
    }
    case "Rooms": {
      selectionLabel =
        vnum !== null
          ? `${vnum} - ${getFirstString(first.name, "(unnamed room)")}`
          : emptyLabel;
      flags = Array.isArray(first.roomFlags)
        ? first.roomFlags.filter((flag) => typeof flag === "string")
        : [];
      const exitList = Array.isArray(first.exits) ? first.exits : [];
      exits =
        exitList.length > 0
          ? exitList
              .map((exit) =>
                typeof exit === "object" && exit
                  ? (exit as Record<string, unknown>).dir
                  : null
              )
              .filter((dir): dir is string => typeof dir === "string")
              .join(", ")
          : "none";
      break;
    }
    case "Mobiles": {
      selectionLabel =
        vnum !== null
          ? `${vnum} - ${getFirstString(first.shortDescr, "(unnamed mobile)")}`
          : emptyLabel;
      flags = Array.isArray(first.actFlags)
        ? first.actFlags.filter((flag) => typeof flag === "string")
        : [];
      break;
    }
    case "Objects": {
      selectionLabel =
        vnum !== null
          ? `${vnum} - ${getFirstString(first.shortDescr, "(unnamed object)")}`
          : emptyLabel;
      flags = Array.isArray(first.wearFlags)
        ? first.wearFlags.filter((flag) => typeof flag === "string")
        : [];
      break;
    }
    case "Resets": {
      const commandName = getFirstString(first.commandName, "reset");
      const roomVnum = parseVnum(first.roomVnum);
      selectionLabel =
        roomVnum !== null
          ? `${commandName} -> room ${roomVnum}`
          : count
            ? commandName
            : emptyLabel;
      break;
    }
    case "Shops": {
      const keeper = parseVnum(first.keeper);
      selectionLabel =
        keeper !== null ? `Keeper ${keeper}` : count ? "Shop entry" : emptyLabel;
      break;
    }
    case "Quests": {
      selectionLabel =
        vnum !== null
          ? `${vnum} - ${getFirstString(first.name, "Quest")}`
          : count
            ? "Quest entry"
            : emptyLabel;
      break;
    }
    case "Factions": {
      selectionLabel =
        vnum !== null
          ? `${vnum} - ${getFirstString(first.name, "Faction")}`
          : count
            ? "Faction entry"
            : emptyLabel;
      break;
    }
    case "Loot": {
      if (!lootData || lootKind === null || lootIndex === null) {
        selectionLabel = count ? "Loot entry" : emptyLabel;
        break;
      }
      if (lootKind === "group") {
        const group = lootData.groups[lootIndex];
        selectionLabel = group
          ? `Group: ${getFirstString(group.name, "(unnamed)")}`
          : emptyLabel;
        break;
      }
      const table = lootData.tables[lootIndex];
      selectionLabel = table
        ? `Table: ${getFirstString(table.name, "(unnamed)")}`
        : emptyLabel;
      break;
    }
    case "Recipes": {
      selectionLabel =
        vnum !== null
          ? `${vnum} - ${getFirstString(first.name, "Recipe")}`
          : count
            ? "Recipe entry"
            : emptyLabel;
      break;
    }
    case "Gather Spawns": {
      selectionLabel =
        vnum !== null
          ? `${vnum} - ${getFirstString(first.spawnSector, "spawn")}`
          : count
            ? "Gather spawn entry"
            : emptyLabel;
      break;
    }
    case "Helps": {
      const keyword = getFirstString(first.keyword, "help");
      selectionLabel = count ? `help: ${keyword}` : emptyLabel;
      break;
    }
    default:
      break;
  }

  const range =
    selectedEntity === "Rooms" ||
    selectedEntity === "Mobiles" ||
    selectedEntity === "Objects" ||
    selectedEntity === "Area"
      ? vnumRange ?? "n/a"
      : "n/a";

  return {
    kindLabel: entityKindLabels[selectedEntity],
    selectionLabel,
    vnumRange: range,
    lastSave: areaData ? "loaded" : "n/a",
    flags,
    exits,
    validation: "validation not run"
  };
}

function validateVnumRanges(
  areaData: AreaJson | null
): ValidationIssueBase[] {
  if (!areaData) {
    return [];
  }
  const bounds = getAreaVnumBounds(areaData);
  if (!bounds) {
    return [];
  }
  const { min, max } = bounds;
  const issues: ValidationIssueBase[] = [];
  const checkEntity = (entity: EntityKey) => {
    const list = getEntityList(areaData, entity);
    list.forEach((entry, index) => {
      if (!entry || typeof entry !== "object") {
        return;
      }
      const record = entry as Record<string, unknown>;
      const vnum = parseVnum(record.vnum);
      if (vnum === null) {
        return;
      }
      if (vnum < min || vnum > max) {
        const kindLabel = entityKindLabels[entity] ?? "Entity";
        issues.push({
          id: `vnum-range-${entity}-${vnum}-${index}`,
          severity: "error",
          entityType: entity,
          vnum,
          message: `${kindLabel} vnum ${vnum} outside area range ${min}-${max}`
        });
      }
    });
  };

  checkEntity("Rooms");
  checkEntity("Mobiles");
  checkEntity("Objects");

  return issues;
}

function validateDuplicateVnums(
  areaData: AreaJson | null
): ValidationIssueBase[] {
  if (!areaData) {
    return [];
  }
  const issues: ValidationIssueBase[] = [];
  const checkEntity = (entity: EntityKey) => {
    const list = getEntityList(areaData, entity);
    const counts = new Map<number, number>();
    list.forEach((entry) => {
      if (!entry || typeof entry !== "object") {
        return;
      }
      const vnum = parseVnum((entry as Record<string, unknown>).vnum);
      if (vnum === null) {
        return;
      }
      counts.set(vnum, (counts.get(vnum) ?? 0) + 1);
    });
    counts.forEach((count, vnum) => {
      if (count <= 1) {
        return;
      }
      const kindLabel = entityKindLabels[entity] ?? "Entity";
      issues.push({
        id: `duplicate-${entity}-${vnum}`,
        severity: "error",
        entityType: entity,
        vnum,
        message: `${kindLabel} vnum ${vnum} is duplicated (${count} entries)`
      });
    });
  };

  checkEntity("Rooms");
  checkEntity("Mobiles");
  checkEntity("Objects");

  return issues;
}

function buildExitTargetValidation(
  areaData: AreaJson | null,
  areaIndex: AreaIndexEntry[]
): ExitValidationResult {
  const empty = { issues: [], invalidEdgeIds: new Set<string>() };
  if (!areaData) {
    return empty;
  }
  const rooms = getEntityList(areaData, "Rooms");
  if (!rooms.length) {
    return empty;
  }
  const bounds = getAreaVnumBounds(areaData);
  const roomVnums = new Set<number>();
  rooms.forEach((room) => {
    if (!room || typeof room !== "object") {
      return;
    }
    const vnum = parseVnum((room as Record<string, unknown>).vnum);
    if (vnum !== null) {
      roomVnums.add(vnum);
    }
  });
  const issues: ValidationIssueBase[] = [];
  const invalidEdgeIds = new Set<string>();
  rooms.forEach((room, index) => {
    if (!room || typeof room !== "object") {
      return;
    }
    const record = room as Record<string, unknown>;
    const fromVnum = parseVnum(record.vnum);
    if (fromVnum === null) {
      return;
    }
    const exits = Array.isArray(record.exits) ? record.exits : [];
    exits.forEach((exit, exitIndex) => {
      if (!exit || typeof exit !== "object") {
        return;
      }
      const exitRecord = exit as Record<string, unknown>;
      const toVnum = parseVnum(exitRecord.toVnum);
      const dir =
        typeof exitRecord.dir === "string" && exitRecord.dir.trim().length
          ? exitRecord.dir.trim()
          : "exit";
      if (toVnum === null) {
        issues.push({
          id: `exit-target-${fromVnum}-${dir}-${index}-${exitIndex}`,
          severity: "error",
          entityType: "Rooms",
          vnum: fromVnum,
          message: `Exit ${dir} from room ${fromVnum} has no target vnum`
        });
        return;
      }
      if (roomVnums.has(toVnum)) {
        return;
      }
      const internalEdgeId = `exit-${fromVnum}-${dir}-${toVnum}-${exitIndex}`;
      const externalEdgeId = `external-${fromVnum}-${dir}-${toVnum}-${exitIndex}`;
      if (bounds && (toVnum < bounds.min || toVnum > bounds.max)) {
        if (areaIndex.length && !findAreaForVnum(areaIndex, toVnum)) {
          issues.push({
            id: `exit-target-${fromVnum}-${dir}-${toVnum}-${index}-${exitIndex}`,
            severity: "error",
            entityType: "Rooms",
            vnum: fromVnum,
            message: `Exit ${dir} from room ${fromVnum} targets unknown area vnum ${toVnum}`
          });
          invalidEdgeIds.add(externalEdgeId);
        }
        return;
      }
      issues.push({
        id: `exit-target-${fromVnum}-${dir}-${toVnum}-${index}-${exitIndex}`,
        severity: "error",
        entityType: "Rooms",
        vnum: fromVnum,
        message: `Exit ${dir} from room ${fromVnum} targets missing room ${toVnum}`
      });
      invalidEdgeIds.add(internalEdgeId);
    });
  });
  return { issues, invalidEdgeIds };
}

function validateResetReferences(
  areaData: AreaJson | null
): ValidationIssueBase[] {
  if (!areaData) {
    return [];
  }
  const resets = getEntityList(areaData, "Resets");
  if (!resets.length) {
    return [];
  }
  const mobiles = getEntityList(areaData, "Mobiles");
  const objects = getEntityList(areaData, "Objects");
  const mobVnums = new Set<number>();
  const objVnums = new Set<number>();

  mobiles.forEach((entry) => {
    if (!entry || typeof entry !== "object") {
      return;
    }
    const vnum = parseVnum((entry as Record<string, unknown>).vnum);
    if (vnum !== null) {
      mobVnums.add(vnum);
    }
  });

  objects.forEach((entry) => {
    if (!entry || typeof entry !== "object") {
      return;
    }
    const vnum = parseVnum((entry as Record<string, unknown>).vnum);
    if (vnum !== null) {
      objVnums.add(vnum);
    }
  });

  const issues: ValidationIssueBase[] = [];
  const addIssue = (
    message: string,
    index: number,
    vnum?: number
  ) => {
    issues.push({
      id: `reset-ref-${index}-${issues.length}`,
      severity: "error",
      entityType: "Resets",
      resetIndex: index,
      vnum,
      message
    });
  };

  resets.forEach((reset, index) => {
    if (!reset || typeof reset !== "object") {
      return;
    }
    const record = reset as Record<string, unknown>;
    const command = resolveResetCommand(record);
    const label = `Reset #${index + 1}`;
    if (command === "loadMob") {
      const mobVnum = parseVnum(record.mobVnum);
      if (mobVnum === null) {
        addIssue(`${label} loadMob missing mob vnum`, index);
        return;
      }
      if (!mobVnums.has(mobVnum)) {
        addIssue(
          `${label} loadMob references missing mob ${mobVnum}`,
          index,
          mobVnum
        );
      }
    } else if (command === "placeObj") {
      const objVnum = parseVnum(record.objVnum);
      if (objVnum === null) {
        addIssue(`${label} placeObj missing object vnum`, index);
        return;
      }
      if (!objVnums.has(objVnum)) {
        addIssue(
          `${label} placeObj references missing object ${objVnum}`,
          index,
          objVnum
        );
      }
    } else if (command === "putObj") {
      const objVnum = parseVnum(record.objVnum);
      const containerVnum = parseVnum(record.containerVnum);
      if (objVnum === null) {
        addIssue(`${label} putObj missing object vnum`, index);
      } else if (!objVnums.has(objVnum)) {
        addIssue(
          `${label} putObj references missing object ${objVnum}`,
          index,
          objVnum
        );
      }
      if (containerVnum === null) {
        addIssue(`${label} putObj missing container vnum`, index);
      } else if (!objVnums.has(containerVnum)) {
        addIssue(
          `${label} putObj references missing container ${containerVnum}`,
          index,
          containerVnum
        );
      }
    } else if (command === "giveObj") {
      const objVnum = parseVnum(record.objVnum);
      if (objVnum === null) {
        addIssue(`${label} giveObj missing object vnum`, index);
        return;
      }
      if (!objVnums.has(objVnum)) {
        addIssue(
          `${label} giveObj references missing object ${objVnum}`,
          index,
          objVnum
        );
      }
    } else if (command === "equipObj") {
      const objVnum = parseVnum(record.objVnum);
      if (objVnum === null) {
        addIssue(`${label} equipObj missing object vnum`, index);
        return;
      }
      if (!objVnums.has(objVnum)) {
        addIssue(
          `${label} equipObj references missing object ${objVnum}`,
          index,
          objVnum
        );
      }
    }
  });

  return issues;
}

function validateOneWayExits(
  areaData: AreaJson | null
): ValidationIssueBase[] {
  if (!areaData) {
    return [];
  }
  const rooms = getEntityList(areaData, "Rooms");
  if (!rooms.length) {
    return [];
  }
  const roomVnums = new Set<number>();
  const roomExitMap = new Map<number, Set<string>>();
  rooms.forEach((room) => {
    if (!room || typeof room !== "object") {
      return;
    }
    const record = room as Record<string, unknown>;
    const vnum = parseVnum(record.vnum);
    if (vnum === null) {
      return;
    }
    roomVnums.add(vnum);
    const exits = Array.isArray(record.exits) ? record.exits : [];
    const exitSet = new Set<string>();
    exits.forEach((exit) => {
      if (!exit || typeof exit !== "object") {
        return;
      }
      const exitRecord = exit as Record<string, unknown>;
      const toVnum = parseVnum(exitRecord.toVnum);
      if (toVnum === null) {
        return;
      }
      const dirKey =
        typeof exitRecord.dir === "string"
          ? exitRecord.dir.trim().toLowerCase()
          : "";
      if (!dirKey) {
        return;
      }
      exitSet.add(`${dirKey}:${toVnum}`);
    });
    if (exitSet.size) {
      roomExitMap.set(vnum, exitSet);
    }
  });

  const oppositeMap: Record<string, string> = {
    north: "south",
    south: "north",
    east: "west",
    west: "east",
    up: "down",
    down: "up"
  };
  const issues: ValidationIssueBase[] = [];
  rooms.forEach((room, index) => {
    if (!room || typeof room !== "object") {
      return;
    }
    const record = room as Record<string, unknown>;
    const fromVnum = parseVnum(record.vnum);
    if (fromVnum === null) {
      return;
    }
    const exits = Array.isArray(record.exits) ? record.exits : [];
    exits.forEach((exit, exitIndex) => {
      if (!exit || typeof exit !== "object") {
        return;
      }
      const exitRecord = exit as Record<string, unknown>;
      const toVnum = parseVnum(exitRecord.toVnum);
      if (toVnum === null || !roomVnums.has(toVnum)) {
        return;
      }
      const dirKey =
        typeof exitRecord.dir === "string"
          ? exitRecord.dir.trim().toLowerCase()
          : "";
      if (!dirKey) {
        return;
      }
      const opposite = oppositeMap[dirKey];
      if (!opposite) {
        return;
      }
      const targetExits = roomExitMap.get(toVnum);
      const reciprocalKey = `${opposite}:${fromVnum}`;
      if (targetExits && targetExits.has(reciprocalKey)) {
        return;
      }
      issues.push({
        id: `one-way-${fromVnum}-${dirKey}-${toVnum}-${index}-${exitIndex}`,
        severity: "warning",
        entityType: "Rooms",
        vnum: fromVnum,
        message: `One-way exit: ${dirKey} from room ${fromVnum} to ${toVnum} has no ${opposite} return`
      });
    });
  });

  return issues;
}

function validateOrphanRooms(
  areaData: AreaJson | null
): ValidationIssueBase[] {
  if (!areaData) {
    return [];
  }
  const rooms = getEntityList(areaData, "Rooms");
  if (!rooms.length) {
    return [];
  }
  const roomVnums = new Set<number>();
  const incomingCounts = new Map<number, number>();
  rooms.forEach((room) => {
    if (!room || typeof room !== "object") {
      return;
    }
    const record = room as Record<string, unknown>;
    const vnum = parseVnum(record.vnum);
    if (vnum === null) {
      return;
    }
    roomVnums.add(vnum);
    incomingCounts.set(vnum, 0);
  });

  rooms.forEach((room) => {
    if (!room || typeof room !== "object") {
      return;
    }
    const record = room as Record<string, unknown>;
    const exits = Array.isArray(record.exits) ? record.exits : [];
    exits.forEach((exit) => {
      if (!exit || typeof exit !== "object") {
        return;
      }
      const exitRecord = exit as Record<string, unknown>;
      const toVnum = parseVnum(exitRecord.toVnum);
      if (toVnum === null || !roomVnums.has(toVnum)) {
        return;
      }
      incomingCounts.set(toVnum, (incomingCounts.get(toVnum) ?? 0) + 1);
    });
  });

  const issues: ValidationIssueBase[] = [];
  incomingCounts.forEach((count, vnum) => {
    if (count > 0) {
      return;
    }
    issues.push({
      id: `orphan-${vnum}`,
      severity: "warning",
      entityType: "Rooms",
      vnum,
      message: `Orphan room ${vnum} has no incoming exits`
    });
  });

  return issues;
}

function buildValidationSummary(
  issues: ValidationIssue[],
  selectedEntity: EntityKey,
  selectedVnum: number | null,
  selectedResetIndex: number | null
): string {
  if (!issues.length) {
    return "No validation issues";
  }
  if (selectedEntity === "Resets" && selectedResetIndex !== null) {
    const selectedIssue = issues.find(
      (issue) =>
        issue.entityType === "Resets" &&
        issue.resetIndex === selectedResetIndex
    );
    if (selectedIssue) {
      return selectedIssue.message;
    }
  } else if (selectedVnum !== null) {
    const selectedIssue = issues.find(
      (issue) =>
        issue.entityType === selectedEntity && issue.vnum === selectedVnum
    );
    if (selectedIssue) {
      return selectedIssue.message;
    }
  }
  const countsByEntity = issues.reduce<Record<string, number>>((acc, issue) => {
    acc[issue.entityType] = (acc[issue.entityType] ?? 0) + 1;
    return acc;
  }, {});
  const severityCounts = issues.reduce<Record<string, number>>((acc, issue) => {
    acc[issue.severity] = (acc[issue.severity] ?? 0) + 1;
    return acc;
  }, {});
  const severityParts = Object.entries(severityCounts).map(
    ([severity, count]) =>
      `${severity === "error" ? "Errors" : "Warnings"} ${count}`
  );
  const entityParts = Object.entries(countsByEntity).map(
    ([entity, count]) => `${entity} ${count}`
  );
  const severitySummary = severityParts.length
    ? ` (${severityParts.join(", ")})`
    : "";
  return `Validation issues${severitySummary}: ${entityParts.join(", ")}`;
}

function buildAreaDebugSummary(areaData: AreaJson | null): {
  keys: string[];
  arrayCounts: Array<{ key: string; count: number }>;
} {
  if (!areaData || typeof areaData !== "object") {
    return { keys: [], arrayCounts: [] };
  }
  const keys = Object.keys(areaData as Record<string, unknown>).sort();
  const arrayCounts = keys
    .map((key) => {
      const value = (areaData as Record<string, unknown>)[key];
      return Array.isArray(value) ? { key, count: value.length } : null;
    })
    .filter((entry): entry is { key: string; count: number } => entry !== null);
  return { keys, arrayCounts };
}

function buildRoomNodes(areaData: AreaJson | null): Node<RoomNodeData>[] {
  if (!areaData) {
    return [];
  }
  const rooms = getEntityList(areaData, "Rooms");
  if (!rooms.length) {
    return [];
  }
  const roomVnums = new Set<number>();
  rooms.forEach((room) => {
    if (!room || typeof room !== "object") {
      return;
    }
    const vnum = parseVnum((room as Record<string, unknown>).vnum);
    if (vnum !== null) {
      roomVnums.add(vnum);
    }
  });
  const columns = 6;
  const spacingX = 220;
  const spacingY = 140;
  return rooms.map((room, index) => {
    const record = room as Record<string, unknown>;
    const vnum = parseVnum(record.vnum);
    const name = getFirstString(record.name, "(unnamed room)");
    const sector =
      typeof record.sectorType === "string"
        ? record.sectorType
        : typeof record.sector === "string"
          ? record.sector
          : "unknown";
    const exits = Array.isArray(record.exits) ? record.exits : [];
    const upExitTargets: number[] = [];
    const downExitTargets: number[] = [];
    exits.forEach((exit) => {
      if (!exit || typeof exit !== "object") {
        return;
      }
      const exitRecord = exit as Record<string, unknown>;
      const dir = typeof exitRecord.dir === "string" ? exitRecord.dir : "";
      const dirKey = dir.trim().toLowerCase();
      if (dirKey !== "up" && dirKey !== "down") {
        return;
      }
      const targetVnum = parseVnum(exitRecord.toVnum);
      if (targetVnum === null || !roomVnums.has(targetVnum)) {
        return;
      }
      if (dirKey === "up") {
        upExitTargets.push(targetVnum);
      } else {
        downExitTargets.push(targetVnum);
      }
    });
    const col = index % columns;
    const row = Math.floor(index / columns);
    return {
      id: vnum !== null ? String(vnum) : `room-${index}`,
      type: "room",
      position: { x: col * spacingX, y: row * spacingY },
      data: {
        vnum: vnum ?? -1,
        label: name,
        sector,
        upExitTargets,
        downExitTargets
      }
    };
  });
}

function applyRoomSelection(
  nodes: Node<RoomNodeData>[],
  selectedVnum: number | null
): Node<RoomNodeData>[] {
  if (selectedVnum === null) {
    return nodes.map((node) => ({ ...node, selected: false }));
  }
  return nodes.map((node) => ({
    ...node,
    selected: node.data.vnum === selectedVnum
  }));
}

function extractRoomLayout(layout: EditorLayout | null | undefined): RoomLayoutMap {
  const rooms =
    layout && typeof layout === "object" && "rooms" in layout
      ? layout.rooms
      : undefined;
  if (!rooms || typeof rooms !== "object") {
    return {};
  }
  const entries: RoomLayoutMap = {};
  Object.entries(rooms).forEach(([key, value]) => {
    if (!value || typeof value !== "object") {
      return;
    }
    const record = value as Record<string, unknown>;
    const x = record.x;
    const y = record.y;
    if (typeof x !== "number" || typeof y !== "number") {
      return;
    }
    if (!Number.isFinite(x) || !Number.isFinite(y)) {
      return;
    }
    if (!record.locked) {
      return;
    }
    entries[key] = {
      x,
      y,
      locked: true
    };
  });
  return entries;
}

function extractAreaLayout(layout: EditorLayout | null | undefined): AreaLayoutMap {
  const areas =
    layout && typeof layout === "object" && "areas" in layout
      ? layout.areas
      : undefined;
  if (!areas || typeof areas !== "object") {
    return {};
  }
  const entries: AreaLayoutMap = {};
  Object.entries(areas).forEach(([key, value]) => {
    if (!value || typeof value !== "object") {
      return;
    }
    const record = value as Record<string, unknown>;
    const x = record.x;
    const y = record.y;
    if (typeof x !== "number" || typeof y !== "number") {
      return;
    }
    if (!Number.isFinite(x) || !Number.isFinite(y)) {
      return;
    }
    if (!record.locked) {
      return;
    }
    entries[key] = {
      x,
      y,
      locked: true
    };
  });
  return entries;
}

function applyRoomLayoutOverrides(
  nodes: Node<RoomNodeData>[],
  layout: RoomLayoutMap
): Node<RoomNodeData>[] {
  if (!Object.keys(layout).length) {
    return nodes.map((node) => ({
      ...node,
      draggable: true,
      data: {
        ...node.data,
        locked: false
      }
    }));
  }
  return nodes.map((node) => {
    const override = layout[node.id];
    const isLocked = override?.locked === true;
    return {
      ...node,
      position: isLocked ? { x: override.x, y: override.y } : node.position,
      draggable: !isLocked,
      data: {
        ...node.data,
        locked: isLocked
      }
    };
  });
}

function applyAreaLayoutOverrides(
  nodes: Node<AreaGraphNodeData>[],
  layout: AreaLayoutMap,
  dirtyNodes: Set<string>
): Node<AreaGraphNodeData>[] {
  return nodes.map((node) => {
    const override = layout[node.id];
    const isLocked = override?.locked === true;
    const isDirty = dirtyNodes.has(node.id);
    return {
      ...node,
      position: isLocked
        ? {
            x: override.x,
            y: override.y
          }
        : node.position,
      draggable: !isLocked,
      data: {
        ...node.data,
        locked: isLocked,
        dirty: isDirty
      }
    };
  });
}

function getDominantExitDirection(
  directionCounts: Partial<Record<string, number>> | null | undefined
): string | null {
  if (!directionCounts) {
    return null;
  }
  const order = ["north", "east", "south", "west", "up", "down"];
  let best: string | null = null;
  let bestCount = -1;
  order.forEach((dir) => {
    const count = directionCounts[dir];
    if (typeof count !== "number") {
      return;
    }
    if (count > bestCount) {
      best = dir;
      bestCount = count;
    }
  });
  return best;
}

function buildRoomEdges(
  areaData: AreaJson | null,
  includeVerticalEdges: boolean,
  includeExternalEdges: boolean,
  invalidEdgeIds?: Set<string>
): Edge[] {
  if (!areaData) {
    return [];
  }
  const rooms = getEntityList(areaData, "Rooms");
  if (!rooms.length) {
    return [];
  }
  const roomVnums = new Set<number>();
  for (const room of rooms) {
    if (!room || typeof room !== "object") {
      continue;
    }
    const vnum = parseVnum((room as Record<string, unknown>).vnum);
    if (vnum !== null) {
      roomVnums.add(vnum);
    }
  }
  const edges: Edge[] = [];
  for (const room of rooms) {
    if (!room || typeof room !== "object") {
      continue;
    }
    const record = room as Record<string, unknown>;
    const fromVnum = parseVnum(record.vnum);
    if (fromVnum === null) {
      continue;
    }
    const exits = Array.isArray(record.exits) ? record.exits : [];
    exits.forEach((exit, index) => {
      if (!exit || typeof exit !== "object") {
        return;
      }
      const exitRecord = exit as Record<string, unknown>;
      const toVnum = parseVnum(exitRecord.toVnum);
      if (toVnum === null) {
        return;
      }
      const dir =
        typeof exitRecord.dir === "string" ? exitRecord.dir : "exit";
      const dirKey = dir.trim().toLowerCase();
      const handles = directionHandleMap[dirKey];
      const isVertical = dirKey === "up" || dirKey === "down";
      const isInternal = roomVnums.has(toVnum);
      const exitFlags = Array.isArray(exitRecord.flags)
        ? exitRecord.flags
            .filter((flag) => typeof flag === "string")
            .map((flag) => flag.trim().toLowerCase())
        : [];
      if (!isInternal && includeExternalEdges) {
        const externalHandle =
          handles?.source ?? externalSourceHandles[dirKey];
        const edgeId = `external-${fromVnum}-${dir}-${toVnum}-${index}`;
        const isInvalid = invalidEdgeIds?.has(edgeId) ?? false;
        edges.push({
          id: edgeId,
          source: String(fromVnum),
          target: String(fromVnum),
          sourceHandle: externalHandle,
          type: "external",
          data: { dirKey, exitFlags, targetVnum: toVnum, invalid: isInvalid }
        });
        return;
      }
      if (!isInternal) {
        return;
      }
      if (!handles) {
        if (includeVerticalEdges && isVertical) {
          const edgeId = `exit-${fromVnum}-${dir}-${toVnum}-${index}`;
          const isInvalid = invalidEdgeIds?.has(edgeId) ?? false;
          edges.push({
            id: edgeId,
            source: String(fromVnum),
            target: String(toVnum),
            type: "vertical",
            data: { dirKey, exitFlags, invalid: isInvalid }
          });
        }
        return;
      }
      const edgeId = `exit-${fromVnum}-${dir}-${toVnum}-${index}`;
      const isInvalid = invalidEdgeIds?.has(edgeId) ?? false;
      edges.push({
        id: edgeId,
        source: String(fromVnum),
        target: String(toVnum),
        label: dir,
        sourceHandle: handles.source,
        targetHandle: handles.target,
        type: "room",
        data: { dirKey, exitFlags, invalid: isInvalid }
      });
    });
  }
  return edges;
}

function findAreaForVnum(
  areaIndex: AreaIndexEntry[],
  vnum: number
): AreaIndexEntry | null {
  for (const entry of areaIndex) {
    if (!entry.vnumRange) {
      continue;
    }
    const [start, end] = entry.vnumRange;
    if (vnum >= start && vnum <= end) {
      return entry;
    }
  }
  return null;
}

function buildExternalExits(
  areaData: AreaJson | null,
  areaIndex: AreaIndexEntry[]
): ExternalExit[] {
  if (!areaData) {
    return [];
  }
  const rooms = getEntityList(areaData, "Rooms");
  if (!rooms.length) {
    return [];
  }
  const roomVnums = new Set<number>();
  const roomNames = new Map<number, string>();
  rooms.forEach((room) => {
    if (!room || typeof room !== "object") {
      return;
    }
    const record = room as Record<string, unknown>;
    const vnum = parseVnum(record.vnum);
    if (vnum === null) {
      return;
    }
    roomVnums.add(vnum);
    roomNames.set(vnum, getFirstString(record.name, "(unnamed room)"));
  });
  const externals: ExternalExit[] = [];
  const seen = new Set<string>();
  rooms.forEach((room) => {
    if (!room || typeof room !== "object") {
      return;
    }
    const record = room as Record<string, unknown>;
    const fromVnum = parseVnum(record.vnum);
    if (fromVnum === null) {
      return;
    }
    const exits = Array.isArray(record.exits) ? record.exits : [];
    exits.forEach((exit) => {
      if (!exit || typeof exit !== "object") {
        return;
      }
      const exitRecord = exit as Record<string, unknown>;
      const toVnum = parseVnum(exitRecord.toVnum);
      if (toVnum === null || roomVnums.has(toVnum)) {
        return;
      }
      const dir =
        typeof exitRecord.dir === "string" ? exitRecord.dir : "exit";
      const key = `${fromVnum}-${dir}-${toVnum}`;
      if (seen.has(key)) {
        return;
      }
      seen.add(key);
      const area = findAreaForVnum(areaIndex, toVnum);
      externals.push({
        fromVnum,
        fromName: roomNames.get(fromVnum) ?? "(unnamed room)",
        direction: dir.trim() ? dir.trim() : "exit",
        toVnum,
        areaName: area?.name ?? null
      });
    });
  });
  return externals.sort((a, b) => {
    if (a.fromVnum !== b.fromVnum) {
      return a.fromVnum - b.fromVnum;
    }
    return a.direction.localeCompare(b.direction);
  });
}

function buildRoomRows(areaData: AreaJson | null): RoomRow[] {
  if (!areaData) {
    return [];
  }
  const rooms = (areaData as { rooms?: unknown }).rooms;
  if (!Array.isArray(rooms)) {
    return [];
  }
  return rooms.map((room) => {
    const data = room as Record<string, unknown>;
    const vnum = parseVnum(data.vnum) ?? -1;
    const name = typeof data.name === "string" ? data.name : "(unnamed room)";
    const sectorType =
      typeof data.sectorType === "string"
        ? data.sectorType
        : typeof data.sector === "string"
          ? data.sector
          : "inside";
    const roomFlags = Array.isArray(data.roomFlags)
      ? data.roomFlags.filter((flag) => typeof flag === "string")
      : [];
    const exits = Array.isArray(data.exits) ? data.exits.length : 0;
    return {
      vnum,
      name,
      sector: sectorType,
      exits,
      flags: roomFlags.length ? roomFlags.join(", ") : "—"
    };
  });
}

function buildMobileRows(areaData: AreaJson | null): MobileRow[] {
  if (!areaData) {
    return [];
  }
  const mobiles = (areaData as { mobiles?: unknown }).mobiles;
  if (!Array.isArray(mobiles)) {
    return [];
  }
  return mobiles.map((mob) => {
    const data = mob as Record<string, unknown>;
    const vnum = parseVnum(data.vnum) ?? -1;
    const name = getFirstString(data.shortDescr, "(unnamed mobile)");
    const level = parseVnum(data.level) ?? 0;
    const race = getFirstString(data.race, "unknown");
    const alignment = parseVnum(data.alignment) ?? 0;
    return { vnum, name, level, race, alignment };
  });
}

function buildObjectRows(areaData: AreaJson | null): ObjectRow[] {
  if (!areaData) {
    return [];
  }
  const objects = (areaData as { objects?: unknown }).objects;
  if (!Array.isArray(objects)) {
    return [];
  }
  return objects.map((obj) => {
    const data = obj as Record<string, unknown>;
    const vnum = parseVnum(data.vnum) ?? -1;
    const name = getFirstString(data.shortDescr, "(unnamed object)");
    const itemType = getFirstString(data.itemType, "unknown");
    const level = parseVnum(data.level) ?? 0;
    return { vnum, name, itemType, level };
  });
}

function buildResetRows(areaData: AreaJson | null): ResetRow[] {
  if (!areaData) {
    return [];
  }
  const resets = (areaData as { resets?: unknown }).resets;
  if (!Array.isArray(resets)) {
    return [];
  }
  return resets.map((reset, index) => {
    const data = reset as Record<string, unknown>;
    const command = getFirstString(data.commandName, getFirstString(data.command, "reset"));
    const mobVnum = parseVnum(data.mobVnum);
    const objVnum = parseVnum(data.objVnum);
    const containerVnum = parseVnum(data.containerVnum);
    const entityVnum =
      mobVnum !== null
        ? String(mobVnum)
        : objVnum !== null
          ? String(objVnum)
          : containerVnum !== null
            ? String(containerVnum)
            : "—";
    const roomVnum = parseVnum(data.roomVnum);
    const toVnum = parseVnum(data.toVnum);
    const roomLabel =
      roomVnum !== null ? String(roomVnum) : toVnum !== null ? String(toVnum) : "—";
    const count = parseVnum(data.count);
    const maxInArea = parseVnum(data.maxInArea);
    const maxInRoom = parseVnum(data.maxInRoom);
    const maxInContainer = parseVnum(data.maxInContainer);
    const wearLoc = getFirstString(data.wearLoc, "");
    const direction = getFirstString(data.direction, "");
    const state = parseVnum(data.state);
    const exits = parseVnum(data.exits);
    const detailsParts: string[] = [];
    if (count !== null) {
      detailsParts.push(`count ${count}`);
    }
    if (maxInArea !== null) {
      detailsParts.push(`max area ${maxInArea}`);
    }
    if (maxInRoom !== null) {
      detailsParts.push(`max room ${maxInRoom}`);
    }
    if (maxInContainer !== null) {
      detailsParts.push(`max container ${maxInContainer}`);
    }
    if (wearLoc) {
      detailsParts.push(`wear ${wearLoc}`);
    }
    if (direction) {
      detailsParts.push(`dir ${direction}`);
    }
    if (state !== null) {
      detailsParts.push(`state ${state}`);
    }
    if (exits !== null) {
      detailsParts.push(`exits ${exits}`);
    }
    const details = detailsParts.length ? detailsParts.join(", ") : "—";
    return {
      index,
      command,
      entityVnum,
      roomVnum: roomLabel,
      details
    };
  });
}

function buildShopRows(areaData: AreaJson | null): ShopRow[] {
  if (!areaData) {
    return [];
  }
  const shops = getEntityList(areaData, "Shops");
  if (!shops.length) {
    return [];
  }
  return shops.map((shop) => {
    const record = shop as Record<string, unknown>;
    const keeper = parseVnum(record.keeper) ?? -1;
    const buyTypes = Array.isArray(record.buyTypes)
      ? record.buyTypes
          .map((value) => parseVnum(value))
          .filter((value): value is number => value !== null)
          .join(", ")
      : "—";
    const profitBuy = parseVnum(record.profitBuy) ?? 0;
    const profitSell = parseVnum(record.profitSell) ?? 0;
    const openHour = parseVnum(record.openHour);
    const closeHour = parseVnum(record.closeHour);
    const hours =
      openHour !== null && closeHour !== null
        ? `${openHour}-${closeHour}`
        : "—";
    return {
      keeper,
      buyTypes,
      profitBuy,
      profitSell,
      hours
    };
  });
}

function buildQuestRows(areaData: AreaJson | null): QuestRow[] {
  if (!areaData) {
    return [];
  }
  const quests = getEntityList(areaData, "Quests");
  if (!quests.length) {
    return [];
  }
  return quests.map((quest) => {
    const record = quest as Record<string, unknown>;
    const vnum = parseVnum(record.vnum) ?? -1;
    const name = getFirstString(record.name, "(unnamed quest)");
    const type = getFirstString(record.type, "—");
    const level = parseVnum(record.level) ?? 0;
    const target = parseVnum(record.target);
    const end = parseVnum(record.end);
    const targetLabel =
      target !== null && end !== null ? `${target} -> ${end}` : "—";
    const rewardGold = parseVnum(record.rewardGold) ?? 0;
    const rewardSilver = parseVnum(record.rewardSilver) ?? 0;
    const rewardCopper = parseVnum(record.rewardCopper) ?? 0;
    const rewards = `G:${rewardGold} S:${rewardSilver} C:${rewardCopper}`;
    return {
      vnum,
      name,
      type,
      level,
      target: targetLabel,
      rewards
    };
  });
}

function buildFactionRows(areaData: AreaJson | null): FactionRow[] {
  if (!areaData) {
    return [];
  }
  const factions = getEntityList(areaData, "Factions");
  if (!factions.length) {
    return [];
  }
  return factions.map((faction) => {
    const record = faction as Record<string, unknown>;
    const vnum = parseVnum(record.vnum) ?? -1;
    const name = getFirstString(record.name, "(unnamed faction)");
    const defaultStanding = parseVnum(record.defaultStanding) ?? 0;
    const allies = Array.isArray(record.allies)
      ? String(record.allies.length)
      : "0";
    const opposing = Array.isArray(record.opposing)
      ? String(record.opposing.length)
      : "0";
    return {
      vnum,
      name,
      defaultStanding,
      allies,
      opposing
    };
  });
}

function buildClassRows(classData: ClassDataFile | null): ClassRow[] {
  if (!classData) {
    return [];
  }
  return classData.classes.map((cls, index) => ({
    index,
    name: cls.name ?? "(unnamed)",
    whoName: cls.whoName ?? "",
    primeStat: cls.primeStat ?? "default",
    armorProf: cls.armorProf ?? "default",
    weaponVnum: cls.weaponVnum ?? 0,
    startLoc: cls.startLoc ?? 0
  }));
}

function normalizeRaceStats(
  value: RaceDefinition["stats"] | RaceDefinition["maxStats"]
): Record<string, number> {
  const stats: Record<string, number> = {
    str: 0,
    int: 0,
    wis: 0,
    dex: 0,
    con: 0
  };
  if (Array.isArray(value)) {
    raceStatKeys.forEach((key, index) => {
      const parsed = Number(value[index]);
      if (Number.isFinite(parsed)) {
        stats[key] = parsed;
      }
    });
    return stats;
  }
  if (value && typeof value === "object") {
    raceStatKeys.forEach((key) => {
      const record = value as Record<string, unknown>;
      const parsed = Number(record[key]);
      if (Number.isFinite(parsed)) {
        stats[key] = parsed;
      }
    });
  }
  return stats;
}

function normalizeRaceClassMap(
  value: Record<string, number> | number[] | undefined,
  classNames: string[],
  defaultValue: number
): Record<string, number> {
  const result: Record<string, number> = {};
  if (!classNames.length) {
    return result;
  }
  classNames.forEach((name, index) => {
    let parsed: number | null = null;
    if (Array.isArray(value)) {
      parsed = Number(value[index]);
    } else if (value && typeof value === "object") {
      parsed = Number((value as Record<string, unknown>)[name]);
    }
    result[name] = Number.isFinite(parsed) ? Math.trunc(parsed) : defaultValue;
  });
  return result;
}

function normalizeRaceSkills(value: string[] | undefined): string[] {
  return Array.from({ length: raceSkillCount }).map((_, index) => {
    const entry = value?.[index];
    return typeof entry === "string" ? entry : "";
  });
}

function buildRaceRows(raceData: RaceDataFile | null): RaceRow[] {
  if (!raceData) {
    return [];
  }
  return raceData.races.map((race, index) => ({
    index,
    name: race.name ?? "(unnamed)",
    whoName: race.whoName ?? "",
    pc: race.pc ? "yes" : "no",
    points: race.points ?? 0,
    size: race.size ?? "medium",
    startLoc: race.startLoc ?? 0
  }));
}

function buildSkillRows(skillData: SkillDataFile | null): SkillRow[] {
  if (!skillData) {
    return [];
  }
  return skillData.skills.map((skill, index) => ({
    index,
    name: skill.name ?? "(unnamed)",
    target: skill.target ?? "tar_ignore",
    minPosition: skill.minPosition ?? "dead",
    spell: skill.spell ?? "",
    slot: skill.slot ?? 0
  }));
}

function normalizeGroupSkills(value: string[] | undefined): string[] {
  return Array.from({ length: groupSkillCount }).map((_, index) => {
    const entry = value?.[index];
    return typeof entry === "string" ? entry : "";
  });
}

function buildGroupRows(groupData: GroupDataFile | null): GroupRow[] {
  if (!groupData) {
    return [];
  }
  return groupData.groups.map((group, index) => ({
    index,
    name: group.name ?? "(unnamed)",
    ratingSummary: Array.isArray(group.ratings)
      ? `${group.ratings.length} ratings`
      : group.ratings
        ? `${Object.keys(group.ratings).length} ratings`
        : "default",
    skills: group.skills?.length ?? 0
  }));
}

function buildCommandRows(commandData: CommandDataFile | null): CommandRow[] {
  if (!commandData) {
    return [];
  }
  return commandData.commands.map((command, index) => ({
    index,
    name: command.name ?? "(unnamed)",
    function: command.function ?? "do_nothing",
    position: command.position ?? "dead",
    level: command.level ?? 0,
    log: command.log ?? "log_normal",
    category: command.category ?? "undef",
    loxFunction: command.loxFunction ?? ""
  }));
}

function buildSocialRows(socialData: SocialDataFile | null): SocialRow[] {
  if (!socialData) {
    return [];
  }
  return socialData.socials.map((social, index) => {
    const noTargetCount = [
      social.charNoArg,
      social.othersNoArg
    ].filter((value) => typeof value === "string" && value.trim().length)
      .length;
    const targetCount = [
      social.charFound,
      social.othersFound,
      social.victFound
    ].filter((value) => typeof value === "string" && value.trim().length)
      .length;
    const selfCount = [
      social.charAuto,
      social.othersAuto
    ].filter((value) => typeof value === "string" && value.trim().length)
      .length;
    return {
      index,
      name: social.name ?? "(unnamed)",
      noTarget: `${noTargetCount}/2`,
      target: `${targetCount}/3`,
      self: `${selfCount}/2`
    };
  });
}

function buildTutorialRows(
  tutorialData: TutorialDataFile | null
): TutorialRow[] {
  if (!tutorialData) {
    return [];
  }
  return tutorialData.tutorials.map((tutorial, index) => ({
    index,
    name: tutorial.name ?? "(unnamed)",
    minLevel: tutorial.minLevel ?? 0,
    steps: tutorial.steps?.length ?? 0
  }));
}

function buildLootRows(lootData: LootDataFile | null): LootRow[] {
  if (!lootData) {
    return [];
  }
  const rows: LootRow[] = [];
  lootData.groups.forEach((group, index) => {
    rows.push({
      id: `group:${index}`,
      kind: "group",
      index,
      name: group.name ?? "(unnamed group)",
      details: `rolls ${group.rolls ?? 1} · entries ${
        group.entries?.length ?? 0
      }`
    });
  });
  lootData.tables.forEach((table, index) => {
    const parent = table.parent ? `parent ${table.parent}` : "no parent";
    rows.push({
      id: `table:${index}`,
      kind: "table",
      index,
      name: table.name ?? "(unnamed table)",
      details: `${parent} · ops ${table.ops?.length ?? 0}`
    });
  });
  return rows;
}

function extractAreaLootData(areaData: AreaJson | null): LootDataFile | null {
  if (!areaData) {
    return null;
  }
  const loot = (areaData as Record<string, unknown>).loot;
  if (!loot || typeof loot !== "object") {
    return null;
  }
  const record = loot as Record<string, unknown>;
  const groups = Array.isArray(record.groups)
    ? (record.groups as LootGroup[])
    : [];
  const tables = Array.isArray(record.tables)
    ? (record.tables as LootTable[])
    : [];
  return {
    formatVersion: 1,
    groups,
    tables
  };
}

function buildRecipeRows(areaData: AreaJson | null): RecipeRow[] {
  const recipes = getEntityList(areaData, "Recipes");
  return recipes
    .filter((entry): entry is Record<string, unknown> =>
      Boolean(entry && typeof entry === "object")
    )
    .map((record) => {
      const vnum = parseVnum(record.vnum) ?? 0;
      const inputs = Array.isArray(record.inputs) ? record.inputs : [];
      const outputVnum = parseVnum(record.outputVnum);
      const outputQty = parseVnum(record.outputQuantity) ?? 1;
      return {
        vnum,
        name: getFirstString(record.name, "(unnamed recipe)"),
        skill: getFirstString(record.skill, "—"),
        inputs: `${inputs.length} input${inputs.length === 1 ? "" : "s"}`,
        output:
          outputVnum !== null
            ? `${outputVnum}${outputQty > 1 ? ` x${outputQty}` : ""}`
            : "—"
      };
    });
}

function buildGatherSpawnRows(areaData: AreaJson | null): GatherSpawnRow[] {
  const spawns = getEntityList(areaData, "Gather Spawns");
  return spawns
    .filter((entry): entry is Record<string, unknown> =>
      Boolean(entry && typeof entry === "object")
    )
    .map((record, index) => {
      const vnum = parseVnum(record.vnum) ?? 0;
      const sector =
        typeof record.spawnSector === "string" ? record.spawnSector : "—";
      return {
        index,
        vnum,
        sector,
        quantity: parseVnum(record.quantity) ?? 0,
        respawnTimer: parseVnum(record.respawnTimer) ?? 0
      };
    });
}

function titlesToText(titles: string[][] | undefined, column: 0 | 1): string {
  const lines: string[] = [];
  for (let i = 0; i < classTitleCount; i += 1) {
    const value = titles?.[i]?.[column] ?? "";
    lines.push(value);
  }
  return lines.join("\n");
}

function normalizeTitleLines(value: string | undefined): string[] {
  if (!value) {
    return [];
  }
  return value.split(/\r?\n/).map((line) => line.trim());
}

function buildTitlePairs(maleText: string, femaleText: string): string[][] {
  const maleLines = normalizeTitleLines(maleText);
  const femaleLines = normalizeTitleLines(femaleText);
  const pairs: string[][] = [];
  for (let i = 0; i < classTitleCount; i += 1) {
    pairs.push([maleLines[i] ?? "", femaleLines[i] ?? ""]);
  }
  return pairs;
}

function cleanOptionalString(value: string | undefined): string | undefined {
  if (!value) {
    return undefined;
  }
  const trimmed = value.trim();
  return trimmed.length ? trimmed : undefined;
}

function syncGridSelection(
  api: GridApi | null,
  rowId: number | string | null
) {
  if (!api) {
    return;
  }
  api.deselectAll();
  if (rowId === null) {
    return;
  }
  const node = api.getRowNode(String(rowId));
  if (node) {
    node.setSelected(true);
  }
}

export default function App({ repository }: AppProps) {
  const [activeTab, setActiveTab] = useState<TabId>(tabs[0].id);
  const [editorMode, setEditorMode] = useState<EditorMode>("Area");
  const [selectedEntity, setSelectedEntity] =
    useState<EntityKey>("Rooms");
  const [selectedGlobalEntity, setSelectedGlobalEntity] =
    useState<GlobalEntityKey>("Classes");
  const [selectedClassIndex, setSelectedClassIndex] = useState<number | null>(
    null
  );
  const [selectedRaceIndex, setSelectedRaceIndex] = useState<number | null>(
    null
  );
  const [selectedSkillIndex, setSelectedSkillIndex] = useState<number | null>(
    null
  );
  const [selectedGroupIndex, setSelectedGroupIndex] = useState<number | null>(
    null
  );
  const [selectedCommandIndex, setSelectedCommandIndex] = useState<number | null>(
    null
  );
  const [selectedSocialIndex, setSelectedSocialIndex] = useState<number | null>(
    null
  );
  const [selectedTutorialIndex, setSelectedTutorialIndex] = useState<
    number | null
  >(null);
  const [selectedLootKind, setSelectedLootKind] = useState<
    "group" | "table" | null
  >(null);
  const [selectedLootIndex, setSelectedLootIndex] = useState<number | null>(
    null
  );
  const [selectedRoomVnum, setSelectedRoomVnum] = useState<number | null>(null);
  const [selectedMobileVnum, setSelectedMobileVnum] = useState<number | null>(
    null
  );
  const [selectedObjectVnum, setSelectedObjectVnum] = useState<number | null>(
    null
  );
  const [selectedResetIndex, setSelectedResetIndex] = useState<number | null>(
    null
  );
  const [selectedShopKeeper, setSelectedShopKeeper] = useState<number | null>(
    null
  );
  const [selectedQuestVnum, setSelectedQuestVnum] = useState<number | null>(null);
  const [selectedFactionVnum, setSelectedFactionVnum] = useState<number | null>(
    null
  );
  const [selectedAreaLootKind, setSelectedAreaLootKind] = useState<
    "group" | "table" | null
  >(null);
  const [selectedAreaLootIndex, setSelectedAreaLootIndex] = useState<
    number | null
  >(null);
  const [selectedRecipeVnum, setSelectedRecipeVnum] = useState<number | null>(
    null
  );
  const [selectedGatherVnum, setSelectedGatherVnum] = useState<number | null>(
    null
  );
  const [areaPath, setAreaPath] = useState<string | null>(null);
  const [areaData, setAreaData] = useState<AreaJson | null>(null);
  const [classData, setClassData] = useState<ClassDataFile | null>(null);
  const [classDataPath, setClassDataPath] = useState<string | null>(null);
  const [classDataFormat, setClassDataFormat] = useState<
    "json" | "olc" | null
  >(null);
  const [classDataDir, setClassDataDir] = useState<string | null>(null);
  const [raceData, setRaceData] = useState<RaceDataFile | null>(null);
  const [raceDataPath, setRaceDataPath] = useState<string | null>(null);
  const [raceDataFormat, setRaceDataFormat] = useState<
    "json" | "olc" | null
  >(null);
  const [raceDataDir, setRaceDataDir] = useState<string | null>(null);
  const [skillData, setSkillData] = useState<SkillDataFile | null>(null);
  const [skillDataPath, setSkillDataPath] = useState<string | null>(null);
  const [skillDataFormat, setSkillDataFormat] = useState<
    "json" | "olc" | null
  >(null);
  const [skillDataDir, setSkillDataDir] = useState<string | null>(null);
  const [groupData, setGroupData] = useState<GroupDataFile | null>(null);
  const [groupDataPath, setGroupDataPath] = useState<string | null>(null);
  const [groupDataFormat, setGroupDataFormat] = useState<
    "json" | "olc" | null
  >(null);
  const [groupDataDir, setGroupDataDir] = useState<string | null>(null);
  const [commandData, setCommandData] = useState<CommandDataFile | null>(null);
  const [commandDataPath, setCommandDataPath] = useState<string | null>(null);
  const [commandDataFormat, setCommandDataFormat] = useState<
    "json" | "olc" | null
  >(null);
  const [commandDataDir, setCommandDataDir] = useState<string | null>(null);
  const [socialData, setSocialData] = useState<SocialDataFile | null>(null);
  const [socialDataPath, setSocialDataPath] = useState<string | null>(null);
  const [socialDataFormat, setSocialDataFormat] = useState<
    "json" | "olc" | null
  >(null);
  const [socialDataDir, setSocialDataDir] = useState<string | null>(null);
  const [tutorialData, setTutorialData] = useState<TutorialDataFile | null>(null);
  const [tutorialDataPath, setTutorialDataPath] = useState<string | null>(null);
  const [tutorialDataFormat, setTutorialDataFormat] = useState<
    "json" | "olc" | null
  >(null);
  const [tutorialDataDir, setTutorialDataDir] = useState<string | null>(null);
  const [lootData, setLootData] = useState<LootDataFile | null>(null);
  const [lootDataPath, setLootDataPath] = useState<string | null>(null);
  const [lootDataFormat, setLootDataFormat] = useState<
    "json" | "olc" | null
  >(null);
  const [lootDataDir, setLootDataDir] = useState<string | null>(null);
  const [projectConfig, setProjectConfig] = useState<ProjectConfig | null>(null);
  const [editorMeta, setEditorMeta] = useState<EditorMeta | null>(null);
  const [editorMetaPath, setEditorMetaPath] = useState<string | null>(null);
  const [referenceData, setReferenceData] = useState<ReferenceData | null>(null);
  const [dataDirectory, setDataDirectory] = useState<string | null>(null);
  const [statusMessage, setStatusMessage] = useState("No area loaded");
  const [errorMessage, setErrorMessage] = useState<string | null>(null);
  const [isBusy, setIsBusy] = useState(false);
  const [areaDirectory, setAreaDirectory] = useState<string | null>(() =>
    localStorage.getItem("worldedit.areaDir")
  );
  const [layoutNodes, setLayoutNodes] = useState<Node<RoomNodeData>[]>([]);
  const [roomLayout, setRoomLayout] = useState<RoomLayoutMap>({});
  const [areaLayout, setAreaLayout] = useState<AreaLayoutMap>({});
  const [dirtyRoomNodes, setDirtyRoomNodes] = useState<Set<string>>(
    () => new Set()
  );
  const [dirtyAreaNodes, setDirtyAreaNodes] = useState<Set<string>>(
    () => new Set()
  );
  const [diagnosticFilter, setDiagnosticFilter] = useState<
    "All" | EntityKey
  >("All");
  const [autoLayoutEnabled, setAutoLayoutEnabled] = useState(true);
  const [preferCardinalLayout, setPreferCardinalLayout] = useState(() => {
    const stored = localStorage.getItem("worldedit.preferCardinalLayout");
    return stored ? stored === "true" : true;
  });
  const [preferAreaCardinalLayout, setPreferAreaCardinalLayout] = useState(
    () => {
      const stored = localStorage.getItem(
        "worldedit.preferAreaCardinalLayout"
      );
      return stored ? stored === "true" : true;
    }
  );
  const [showVerticalEdges, setShowVerticalEdges] = useState(() => {
    const stored = localStorage.getItem("worldedit.showVerticalEdges");
    return stored ? stored === "true" : false;
  });
  const [areaGraphFilter, setAreaGraphFilter] = useState("");
  const [areaGraphVnumQuery, setAreaGraphVnumQuery] = useState("");
  const [areaIndex, setAreaIndex] = useState<AreaIndexEntry[]>([]);
  const [areaGraphLinks, setAreaGraphLinks] = useState<AreaGraphLink[]>([]);
  const [areaGraphLayoutNodes, setAreaGraphLayoutNodes] = useState<
    Node<AreaGraphNodeData>[]
  >([]);
  const [layoutNonce, setLayoutNonce] = useState(0);
  const roomLayoutRef = useRef<RoomLayoutMap>({});
  const legacyScanDirRef = useRef<string | null>(null);
  const roomGridApi = useRef<GridApi | null>(null);
  const mobileGridApi = useRef<GridApi | null>(null);
  const objectGridApi = useRef<GridApi | null>(null);
  const resetGridApi = useRef<GridApi | null>(null);
  const shopGridApi = useRef<GridApi | null>(null);
  const questGridApi = useRef<GridApi | null>(null);
  const factionGridApi = useRef<GridApi | null>(null);
  const areaLootGridApi = useRef<GridApi | null>(null);
  const recipeGridApi = useRef<GridApi | null>(null);
  const gatherGridApi = useRef<GridApi | null>(null);
  const classGridApi = useRef<GridApi | null>(null);
  const raceGridApi = useRef<GridApi | null>(null);
  const skillGridApi = useRef<GridApi | null>(null);
  const groupGridApi = useRef<GridApi | null>(null);
  const commandGridApi = useRef<GridApi | null>(null);
  const socialGridApi = useRef<GridApi | null>(null);
  const tutorialGridApi = useRef<GridApi | null>(null);
  const lootGridApi = useRef<GridApi | null>(null);
  const areaForm = useForm<AreaFormValues>({
    resolver: zodResolver(areaFormSchema),
    defaultValues: {
      name: "",
      version: 1,
      vnumRangeStart: 0,
      vnumRangeEnd: 0,
      builders: "",
      credits: "",
      security: undefined,
      sector: undefined,
      lowLevel: undefined,
      highLevel: undefined,
      reset: undefined,
      alwaysReset: false,
      instType: "single",
      storyBeats: [],
      checklist: []
    }
  });
  const {
    register: registerArea,
    handleSubmit: handleAreaSubmitForm,
    formState: areaFormState,
    control: areaFormControl
  } = areaForm;
  const {
    fields: storyBeatFields,
    append: appendStoryBeat,
    remove: removeStoryBeat,
    move: moveStoryBeat
  } = useFieldArray({
    control: areaFormControl,
    name: "storyBeats"
  });
  const {
    fields: checklistFields,
    append: appendChecklist,
    remove: removeChecklist,
    move: moveChecklist
  } = useFieldArray({
    control: areaFormControl,
    name: "checklist"
  });
  const roomForm = useForm<RoomFormValues>({
    resolver: zodResolver(roomFormSchema),
    defaultValues: {
      vnum: 0,
      name: "",
      description: "",
      sectorType: undefined,
      roomFlags: [],
      exits: []
    }
  });
  const {
    register: registerRoom,
    handleSubmit: handleRoomSubmitForm,
    formState: roomFormState,
    control: roomFormControl
  } = roomForm;
  const {
    fields: exitFields,
    append: appendExit,
    remove: removeExit
  } = useFieldArray({
    control: roomFormControl,
    name: "exits"
  });
  const mobileForm = useForm<MobileFormValues>({
    resolver: zodResolver(mobileFormSchema),
    defaultValues: {
      vnum: 0,
      name: "",
      shortDescr: "",
      longDescr: "",
      description: "",
      race: "",
      level: undefined,
      hitroll: undefined,
      alignment: undefined,
      damType: undefined,
      startPos: undefined,
      defaultPos: undefined,
      sex: undefined,
      size: undefined,
      material: "",
      factionVnum: undefined,
      damageNoun: "",
      offensiveSpell: "",
      hitDice: { number: undefined, type: undefined, bonus: undefined },
      manaDice: { number: undefined, type: undefined, bonus: undefined },
      damageDice: { number: undefined, type: undefined, bonus: undefined }
    }
  });
  const {
    register: registerMobile,
    handleSubmit: handleMobileSubmitForm,
    formState: mobileFormState
  } = mobileForm;
  const objectForm = useForm<ObjectFormValues>({
    resolver: zodResolver(objectFormSchema),
    defaultValues: {
      vnum: 0,
      name: "",
      shortDescr: "",
      description: "",
      material: "",
      itemType: "",
      extraFlags: [],
      wearFlags: [],
      level: undefined,
      weight: undefined,
      cost: undefined,
      condition: undefined,
      weapon: {
        class: undefined,
        diceNumber: undefined,
        diceType: undefined,
        damageType: undefined,
        flags: []
      },
      armor: {
        acPierce: undefined,
        acBash: undefined,
        acSlash: undefined,
        acExotic: undefined
      },
      container: {
        capacity: undefined,
        flags: [],
        keyVnum: undefined,
        maxWeight: undefined,
        weightMult: undefined
      },
      light: {
        hours: undefined
      },
      drink: {
        capacity: undefined,
        remaining: undefined,
        liquid: undefined,
        poisoned: undefined
      },
      fountain: {
        capacity: undefined,
        remaining: undefined,
        liquid: undefined,
        poisoned: undefined
      },
      food: {
        foodHours: undefined,
        fullHours: undefined,
        poisoned: undefined
      },
      money: {
        gold: undefined,
        silver: undefined
      },
      wand: {
        level: undefined,
        totalCharges: undefined,
        chargesLeft: undefined,
        spell: ""
      },
      staff: {
        level: undefined,
        totalCharges: undefined,
        chargesLeft: undefined,
        spell: ""
      },
      spells: {
        level: undefined,
        spell1: "",
        spell2: "",
        spell3: "",
        spell4: ""
      },
      portal: {
        charges: undefined,
        exitFlags: [],
        portalFlags: [],
        toVnum: undefined
      },
      furniture: {
        slots: undefined,
        weight: undefined,
        flags: [],
        healBonus: undefined,
        manaBonus: undefined,
        maxPeople: undefined
      }
    }
  });
  const {
    register: registerObject,
    handleSubmit: handleObjectSubmitForm,
    formState: objectFormState,
    control: objectFormControl
  } = objectForm;
  const resetForm = useForm<ResetFormValues>({
    resolver: zodResolver(resetFormSchema),
    defaultValues: {
      index: 0,
      commandName: resetCommandOptions[0],
      mobVnum: undefined,
      objVnum: undefined,
      roomVnum: undefined,
      containerVnum: undefined,
      maxInArea: undefined,
      maxInRoom: undefined,
      count: undefined,
      maxInContainer: undefined,
      wearLoc: undefined,
      direction: undefined,
      state: undefined,
      exits: undefined
    }
  });
  const {
    register: registerReset,
    handleSubmit: handleResetSubmitForm,
    formState: resetFormState,
    control: resetFormControl
  } = resetForm;
  const shopForm = useForm<ShopFormValues>({
    resolver: zodResolver(shopFormSchema),
    defaultValues: {
      keeper: undefined,
      buyTypes: [0, 0, 0, 0, 0],
      profitBuy: undefined,
      profitSell: undefined,
      openHour: undefined,
      closeHour: undefined
    }
  });
  const {
    register: registerShop,
    handleSubmit: handleShopSubmitForm,
    formState: shopFormState,
    control: shopFormControl
  } = shopForm;
  const questForm = useForm<QuestFormValues>({
    resolver: zodResolver(questFormSchema),
    defaultValues: {
      vnum: 0,
      name: "",
      entry: "",
      type: "",
      xp: undefined,
      level: undefined,
      end: undefined,
      target: undefined,
      upper: undefined,
      count: undefined,
      rewardFaction: undefined,
      rewardReputation: undefined,
      rewardGold: undefined,
      rewardSilver: undefined,
      rewardCopper: undefined,
      rewardObjs: [0, 0, 0],
      rewardCounts: [0, 0, 0]
    }
  });
  const {
    register: registerQuest,
    handleSubmit: handleQuestSubmitForm,
    formState: questFormState,
    control: questFormControl
  } = questForm;
  const factionForm = useForm<FactionFormValues>({
    resolver: zodResolver(factionFormSchema),
    defaultValues: {
      vnum: 0,
      name: "",
      defaultStanding: undefined,
      alliesCsv: "",
      opposingCsv: ""
    }
  });
  const {
    register: registerFaction,
    handleSubmit: handleFactionSubmitForm,
    formState: factionFormState
  } = factionForm;
  const recipeForm = useForm<RecipeFormValues>({
    resolver: zodResolver(recipeFormSchema),
    defaultValues: {
      vnum: 0,
      name: "",
      skill: "",
      minSkill: undefined,
      minSkillPct: undefined,
      minLevel: undefined,
      stationType: [],
      stationVnum: undefined,
      discovery: undefined,
      inputs: [],
      outputVnum: undefined,
      outputQuantity: undefined
    }
  });
  const {
    register: registerRecipe,
    handleSubmit: handleRecipeSubmitForm,
    formState: recipeFormState,
    reset: resetRecipeForm,
    control: recipeFormControl
  } = recipeForm;
  const {
    fields: recipeInputFields,
    append: appendRecipeInput,
    remove: removeRecipeInput,
    move: moveRecipeInput
  } = useFieldArray({
    control: recipeFormControl,
    name: "inputs"
  });
  const gatherSpawnForm = useForm<GatherSpawnFormValues>({
    resolver: zodResolver(gatherSpawnFormSchema),
    defaultValues: {
      spawnSector: undefined,
      vnum: undefined,
      quantity: undefined,
      respawnTimer: undefined
    }
  });
  const {
    register: registerGatherSpawn,
    handleSubmit: handleGatherSpawnSubmitForm,
    formState: gatherSpawnFormState,
    reset: resetGatherSpawnForm,
    control: gatherSpawnFormControl
  } = gatherSpawnForm;
  const classFormDefaults = useMemo<ClassFormValues>(
    () => ({
      name: "",
      whoName: "",
      baseGroup: "",
      defaultGroup: "",
      weaponVnum: 0,
      armorProf: "",
      guilds: new Array(classGuildCount).fill(0),
      primeStat: "",
      skillCap: 0,
      thac0_00: 0,
      thac0_32: 0,
      hpMin: 0,
      hpMax: 0,
      manaUser: false,
      startLoc: 0,
      titlesMale: "",
      titlesFemale: ""
    }),
    []
  );
  const classForm = useForm<ClassFormValues>({
    resolver: zodResolver(classFormSchema),
    defaultValues: classFormDefaults
  });
  const {
    register: registerClass,
    handleSubmit: handleClassSubmitForm,
    formState: classFormState,
    reset: resetClassForm
  } = classForm;
  const raceClassNames = referenceData?.classes ?? [];
  const raceFormDefaults = useMemo<RaceFormValues>(
    () => ({
      name: "",
      whoName: "",
      pc: false,
      points: 0,
      size: "",
      startLoc: 0,
      stats: {
        str: 0,
        int: 0,
        wis: 0,
        dex: 0,
        con: 0
      },
      maxStats: {
        str: 0,
        int: 0,
        wis: 0,
        dex: 0,
        con: 0
      },
      actFlags: [],
      affectFlags: [],
      offFlags: [],
      immFlags: [],
      resFlags: [],
      vulnFlags: [],
      formFlags: [],
      partFlags: [],
      classMult: Object.fromEntries(
        raceClassNames.map((name) => [name, 100])
      ),
      classStart: Object.fromEntries(
        raceClassNames.map((name) => [name, 0])
      ),
      skills: new Array(raceSkillCount).fill("")
    }),
    [raceClassNames]
  );
  const raceForm = useForm<RaceFormValues>({
    resolver: zodResolver(raceFormSchema),
    defaultValues: raceFormDefaults
  });
  const {
    register: registerRace,
    handleSubmit: handleRaceSubmitForm,
    formState: raceFormState,
    reset: resetRaceForm
  } = raceForm;
  const skillClassNames = referenceData?.classes ?? [];
  const skillFormDefaults = useMemo<SkillFormValues>(
    () => ({
      name: "",
      levels: Object.fromEntries(
        skillClassNames.map((name) => [name, defaultSkillLevel])
      ),
      ratings: Object.fromEntries(
        skillClassNames.map((name) => [name, defaultSkillRating])
      ),
      spell: "",
      loxSpell: "",
      target: "",
      minPosition: "",
      gsn: "",
      slot: 0,
      minMana: 0,
      beats: 0,
      nounDamage: "",
      msgOff: "",
      msgObj: ""
    }),
    [skillClassNames]
  );
  const skillForm = useForm<SkillFormValues>({
    resolver: zodResolver(skillFormSchema),
    defaultValues: skillFormDefaults
  });
  const {
    register: registerSkill,
    handleSubmit: handleSkillSubmitForm,
    formState: skillFormState,
    reset: resetSkillForm
  } = skillForm;
  const groupClassNames = referenceData?.classes ?? [];
  const groupSkillOptions = referenceData?.skills ?? [];
  const groupFormDefaults = useMemo<GroupFormValues>(
    () => ({
      name: "",
      ratings: Object.fromEntries(
        groupClassNames.map((name) => [name, defaultSkillRating])
      ),
      skills: new Array(groupSkillCount).fill("")
    }),
    [groupClassNames]
  );
  const groupForm = useForm<GroupFormValues>({
    resolver: zodResolver(groupFormSchema),
    defaultValues: groupFormDefaults
  });
  const {
    register: registerGroup,
    handleSubmit: handleGroupSubmitForm,
    formState: groupFormState,
    reset: resetGroupForm
  } = groupForm;
  const commandFormDefaults = useMemo<CommandFormValues>(
    () => ({
      name: "",
      function: "",
      position: "",
      level: 0,
      log: "",
      category: "",
      loxFunction: ""
    }),
    []
  );
  const commandForm = useForm<CommandFormValues>({
    resolver: zodResolver(commandFormSchema),
    defaultValues: commandFormDefaults
  });
  const {
    register: registerCommand,
    handleSubmit: handleCommandSubmitForm,
    formState: commandFormState,
    reset: resetCommandForm
  } = commandForm;
  const socialFormDefaults = useMemo<SocialFormValues>(
    () => ({
      name: "",
      charNoArg: "",
      othersNoArg: "",
      charFound: "",
      othersFound: "",
      victFound: "",
      charAuto: "",
      othersAuto: ""
    }),
    []
  );
  const socialForm = useForm<SocialFormValues>({
    resolver: zodResolver(socialFormSchema),
    defaultValues: socialFormDefaults
  });
  const {
    register: registerSocial,
    handleSubmit: handleSocialSubmitForm,
    formState: socialFormState,
    reset: resetSocialForm
  } = socialForm;
  const tutorialFormDefaults = useMemo<TutorialFormValues>(
    () => ({
      name: "",
      blurb: "",
      finish: "",
      minLevel: 0,
      steps: []
    }),
    []
  );
  const tutorialForm = useForm<TutorialFormValues>({
    resolver: zodResolver(tutorialFormSchema),
    defaultValues: tutorialFormDefaults
  });
  const {
    register: registerTutorial,
    handleSubmit: handleTutorialSubmitForm,
    formState: tutorialFormState,
    reset: resetTutorialForm,
    control: tutorialFormControl
  } = tutorialForm;
  const {
    fields: tutorialStepFields,
    append: appendTutorialStep,
    remove: removeTutorialStep,
    move: moveTutorialStep
  } = useFieldArray({
    control: tutorialFormControl,
    name: "steps"
  });
  const lootFormDefaults = useMemo<LootFormValues>(
    () => ({
      kind: "group",
      name: "",
      rolls: 1,
      entries: [],
      parent: "",
      ops: []
    }),
    []
  );
  const areaLootForm = useForm<LootFormValues>({
    resolver: zodResolver(lootFormSchema),
    defaultValues: lootFormDefaults
  });
  const {
    register: registerAreaLoot,
    handleSubmit: handleAreaLootSubmitForm,
    formState: areaLootFormState,
    reset: resetAreaLootForm,
    control: areaLootFormControl
  } = areaLootForm;
  const {
    fields: areaLootEntryFields,
    append: appendAreaLootEntry,
    remove: removeAreaLootEntry,
    move: moveAreaLootEntry
  } = useFieldArray({
    control: areaLootFormControl,
    name: "entries"
  });
  const {
    fields: areaLootOpFields,
    append: appendAreaLootOp,
    remove: removeAreaLootOp,
    move: moveAreaLootOp
  } = useFieldArray({
    control: areaLootFormControl,
    name: "ops"
  });
  const lootForm = useForm<LootFormValues>({
    resolver: zodResolver(lootFormSchema),
    defaultValues: lootFormDefaults
  });
  const {
    register: registerLoot,
    handleSubmit: handleLootSubmitForm,
    formState: lootFormState,
    reset: resetLootForm,
    control: lootFormControl
  } = lootForm;
  const {
    fields: lootEntryFields,
    append: appendLootEntry,
    remove: removeLootEntry,
    move: moveLootEntry
  } = useFieldArray({
    control: lootFormControl,
    name: "entries"
  });
  const {
    fields: lootOpFields,
    append: appendLootOp,
    remove: removeLootOp,
    move: moveLootOp
  } = useFieldArray({
    control: lootFormControl,
    name: "ops"
  });
  const watchedResetCommand = useWatch({
    control: resetForm.control,
    name: "commandName"
  });
  const watchedObjectItemType = useWatch({
    control: objectForm.control,
    name: "itemType"
  });
  const handleLoadClassesData = useCallback(
    async (overrideDir?: string) => {
      const targetDir =
        typeof overrideDir === "string" ? overrideDir : dataDirectory;
      if (!targetDir) {
        setErrorMessage("No data directory set for classes.");
        return;
      }
      const classesFile = projectConfig?.dataFiles.classes;
      const defaultFormat = projectConfig?.defaultFormat;
      setErrorMessage(null);
      setIsBusy(true);
      try {
        const source = await repository.loadClassesData(
          targetDir,
          classesFile,
          defaultFormat
        );
        setClassData(source.data);
        setClassDataPath(source.path);
        setClassDataFormat(source.format);
        setClassDataDir(targetDir);
        setStatusMessage(`Loaded classes (${source.format})`);
      } catch (error) {
        setErrorMessage(`Failed to load classes. ${String(error)}`);
      } finally {
        setIsBusy(false);
      }
    },
    [dataDirectory, projectConfig, repository]
  );
  const handleSaveClassesData = useCallback(async () => {
    if (!classData) {
      setErrorMessage("No classes loaded to save.");
      return;
    }
    if (!dataDirectory) {
      setErrorMessage("No data directory set for classes.");
      return;
    }
    setErrorMessage(null);
    setIsBusy(true);
    try {
      const format = classDataFormat ?? projectConfig?.defaultFormat ?? "json";
      const classesFile = projectConfig?.dataFiles.classes;
      const path = await repository.saveClassesData(
        dataDirectory,
        classData,
        format,
        classesFile
      );
      setClassDataPath(path);
      setClassDataFormat(format);
      setClassDataDir(dataDirectory);
      setStatusMessage(`Saved classes (${format})`);
    } catch (error) {
      setErrorMessage(`Failed to save classes. ${String(error)}`);
    } finally {
      setIsBusy(false);
    }
  }, [classData, classDataFormat, dataDirectory, projectConfig, repository]);

  const handleLoadRacesData = useCallback(
    async (overrideDir?: string) => {
      const targetDir =
        typeof overrideDir === "string" ? overrideDir : dataDirectory;
      if (!targetDir) {
        setErrorMessage("No data directory set for races.");
        return;
      }
      const racesFile = projectConfig?.dataFiles.races;
      const defaultFormat = projectConfig?.defaultFormat;
      setErrorMessage(null);
      setIsBusy(true);
      try {
        const source = await repository.loadRacesData(
          targetDir,
          racesFile,
          defaultFormat
        );
        setRaceData(source.data);
        setRaceDataPath(source.path);
        setRaceDataFormat(source.format);
        setRaceDataDir(targetDir);
        setStatusMessage(`Loaded races (${source.format})`);
      } catch (error) {
        setErrorMessage(`Failed to load races. ${String(error)}`);
      } finally {
        setIsBusy(false);
      }
    },
    [dataDirectory, projectConfig, repository]
  );

  const handleSaveRacesData = useCallback(async () => {
    if (!raceData) {
      setErrorMessage("No races loaded to save.");
      return;
    }
    if (!dataDirectory) {
      setErrorMessage("No data directory set for races.");
      return;
    }
    setErrorMessage(null);
    setIsBusy(true);
    try {
      const format = raceDataFormat ?? projectConfig?.defaultFormat ?? "json";
      const racesFile = projectConfig?.dataFiles.races;
      const classNames = referenceData?.classes ?? [];
      const path = await repository.saveRacesData(
        dataDirectory,
        raceData,
        format,
        racesFile,
        classNames
      );
      setRaceDataPath(path);
      setRaceDataFormat(format);
      setRaceDataDir(dataDirectory);
      setStatusMessage(`Saved races (${format})`);
    } catch (error) {
      setErrorMessage(`Failed to save races. ${String(error)}`);
    } finally {
      setIsBusy(false);
    }
  }, [
    dataDirectory,
    projectConfig,
    raceData,
    raceDataFormat,
    referenceData,
    repository
  ]);

  const handleLoadSkillsData = useCallback(
    async (overrideDir?: string) => {
      const targetDir =
        typeof overrideDir === "string" ? overrideDir : dataDirectory;
      if (!targetDir) {
        setErrorMessage("No data directory set for skills.");
        return;
      }
      const skillsFile = projectConfig?.dataFiles.skills;
      const defaultFormat = projectConfig?.defaultFormat;
      setErrorMessage(null);
      setIsBusy(true);
      try {
        const source = await repository.loadSkillsData(
          targetDir,
          skillsFile,
          defaultFormat
        );
        setSkillData(source.data);
        setSkillDataPath(source.path);
        setSkillDataFormat(source.format);
        setSkillDataDir(targetDir);
        setStatusMessage(`Loaded skills (${source.format})`);
      } catch (error) {
        setErrorMessage(`Failed to load skills. ${String(error)}`);
      } finally {
        setIsBusy(false);
      }
    },
    [dataDirectory, projectConfig, repository]
  );

  const handleSaveSkillsData = useCallback(async () => {
    if (!skillData) {
      setErrorMessage("No skills loaded to save.");
      return;
    }
    if (!dataDirectory) {
      setErrorMessage("No data directory set for skills.");
      return;
    }
    setErrorMessage(null);
    setIsBusy(true);
    try {
      const format = skillDataFormat ?? projectConfig?.defaultFormat ?? "json";
      const skillsFile = projectConfig?.dataFiles.skills;
      const classNames = referenceData?.classes ?? [];
      const path = await repository.saveSkillsData(
        dataDirectory,
        skillData,
        format,
        skillsFile,
        classNames
      );
      setSkillDataPath(path);
      setSkillDataFormat(format);
      setSkillDataDir(dataDirectory);
      setStatusMessage(`Saved skills (${format})`);
    } catch (error) {
      setErrorMessage(`Failed to save skills. ${String(error)}`);
    } finally {
      setIsBusy(false);
    }
  }, [
    dataDirectory,
    projectConfig,
    referenceData,
    repository,
    skillData,
    skillDataFormat
  ]);

  const handleLoadGroupsData = useCallback(
    async (overrideDir?: string) => {
      const targetDir =
        typeof overrideDir === "string" ? overrideDir : dataDirectory;
      if (!targetDir) {
        setErrorMessage("No data directory set for groups.");
        return;
      }
      const groupsFile = projectConfig?.dataFiles.groups;
      const defaultFormat = projectConfig?.defaultFormat;
      setErrorMessage(null);
      setIsBusy(true);
      try {
        const source = await repository.loadGroupsData(
          targetDir,
          groupsFile,
          defaultFormat
        );
        setGroupData(source.data);
        setGroupDataPath(source.path);
        setGroupDataFormat(source.format);
        setGroupDataDir(targetDir);
        setStatusMessage(`Loaded groups (${source.format})`);
      } catch (error) {
        setErrorMessage(`Failed to load groups. ${String(error)}`);
      } finally {
        setIsBusy(false);
      }
    },
    [dataDirectory, projectConfig, repository]
  );

  const handleSaveGroupsData = useCallback(async () => {
    if (!groupData) {
      setErrorMessage("No groups loaded to save.");
      return;
    }
    if (!dataDirectory) {
      setErrorMessage("No data directory set for groups.");
      return;
    }
    setErrorMessage(null);
    setIsBusy(true);
    try {
      const format = groupDataFormat ?? projectConfig?.defaultFormat ?? "json";
      const groupsFile = projectConfig?.dataFiles.groups;
      const classNames = referenceData?.classes ?? [];
      const path = await repository.saveGroupsData(
        dataDirectory,
        groupData,
        format,
        groupsFile,
        classNames
      );
      setGroupDataPath(path);
      setGroupDataFormat(format);
      setGroupDataDir(dataDirectory);
      setStatusMessage(`Saved groups (${format})`);
    } catch (error) {
      setErrorMessage(`Failed to save groups. ${String(error)}`);
    } finally {
      setIsBusy(false);
    }
  }, [
    dataDirectory,
    groupData,
    groupDataFormat,
    projectConfig,
    referenceData,
    repository
  ]);

  const handleLoadCommandsData = useCallback(
    async (overrideDir?: string) => {
      const targetDir =
        typeof overrideDir === "string" ? overrideDir : dataDirectory;
      if (!targetDir) {
        setErrorMessage("No data directory set for commands.");
        return;
      }
      const commandsFile = projectConfig?.dataFiles.commands;
      const defaultFormat = projectConfig?.defaultFormat;
      setErrorMessage(null);
      setIsBusy(true);
      try {
        const source = await repository.loadCommandsData(
          targetDir,
          commandsFile,
          defaultFormat
        );
        setCommandData(source.data);
        setCommandDataPath(source.path);
        setCommandDataFormat(source.format);
        setCommandDataDir(targetDir);
        setStatusMessage(`Loaded commands (${source.format})`);
      } catch (error) {
        setErrorMessage(`Failed to load commands. ${String(error)}`);
      } finally {
        setIsBusy(false);
      }
    },
    [dataDirectory, projectConfig, repository]
  );

  const handleSaveCommandsData = useCallback(async () => {
    if (!commandData) {
      setErrorMessage("No commands loaded to save.");
      return;
    }
    if (!dataDirectory) {
      setErrorMessage("No data directory set for commands.");
      return;
    }
    setErrorMessage(null);
    setIsBusy(true);
    try {
      const format =
        commandDataFormat ?? projectConfig?.defaultFormat ?? "json";
      const commandsFile = projectConfig?.dataFiles.commands;
      const path = await repository.saveCommandsData(
        dataDirectory,
        commandData,
        format,
        commandsFile
      );
      setCommandDataPath(path);
      setCommandDataFormat(format);
      setCommandDataDir(dataDirectory);
      setStatusMessage(`Saved commands (${format})`);
    } catch (error) {
      setErrorMessage(`Failed to save commands. ${String(error)}`);
    } finally {
      setIsBusy(false);
    }
  }, [
    commandData,
    commandDataFormat,
    dataDirectory,
    projectConfig,
    repository
  ]);

  const handleLoadSocialsData = useCallback(
    async (overrideDir?: string) => {
      const targetDir =
        typeof overrideDir === "string" ? overrideDir : dataDirectory;
      if (!targetDir) {
        setErrorMessage("No data directory set for socials.");
        return;
      }
      const socialsFile = projectConfig?.dataFiles.socials;
      const defaultFormat = projectConfig?.defaultFormat;
      setErrorMessage(null);
      setIsBusy(true);
      try {
        const source = await repository.loadSocialsData(
          targetDir,
          socialsFile,
          defaultFormat
        );
        setSocialData(source.data);
        setSocialDataPath(source.path);
        setSocialDataFormat(source.format);
        setSocialDataDir(targetDir);
        setStatusMessage(`Loaded socials (${source.format})`);
      } catch (error) {
        setErrorMessage(`Failed to load socials. ${String(error)}`);
      } finally {
        setIsBusy(false);
      }
    },
    [dataDirectory, projectConfig, repository]
  );

  const handleSaveSocialsData = useCallback(async () => {
    if (!socialData) {
      setErrorMessage("No socials loaded to save.");
      return;
    }
    if (!dataDirectory) {
      setErrorMessage("No data directory set for socials.");
      return;
    }
    setErrorMessage(null);
    setIsBusy(true);
    try {
      const format = socialDataFormat ?? projectConfig?.defaultFormat ?? "json";
      const socialsFile = projectConfig?.dataFiles.socials;
      const path = await repository.saveSocialsData(
        dataDirectory,
        socialData,
        format,
        socialsFile
      );
      setSocialDataPath(path);
      setSocialDataFormat(format);
      setSocialDataDir(dataDirectory);
      setStatusMessage(`Saved socials (${format})`);
    } catch (error) {
      setErrorMessage(`Failed to save socials. ${String(error)}`);
    } finally {
      setIsBusy(false);
    }
  }, [
    dataDirectory,
    projectConfig,
    repository,
    socialData,
    socialDataFormat
  ]);

  const handleLoadTutorialsData = useCallback(
    async (overrideDir?: string) => {
      const targetDir =
        typeof overrideDir === "string" ? overrideDir : dataDirectory;
      if (!targetDir) {
        setErrorMessage("No data directory set for tutorials.");
        return;
      }
      const tutorialsFile = projectConfig?.dataFiles.tutorials;
      const defaultFormat = projectConfig?.defaultFormat;
      setErrorMessage(null);
      setIsBusy(true);
      try {
        const source = await repository.loadTutorialsData(
          targetDir,
          tutorialsFile,
          defaultFormat
        );
        setTutorialData(source.data);
        setTutorialDataPath(source.path);
        setTutorialDataFormat(source.format);
        setTutorialDataDir(targetDir);
        setStatusMessage(`Loaded tutorials (${source.format})`);
      } catch (error) {
        setErrorMessage(`Failed to load tutorials. ${String(error)}`);
      } finally {
        setIsBusy(false);
      }
    },
    [dataDirectory, projectConfig, repository]
  );

  const handleSaveTutorialsData = useCallback(async () => {
    if (!tutorialData) {
      setErrorMessage("No tutorials loaded to save.");
      return;
    }
    if (!dataDirectory) {
      setErrorMessage("No data directory set for tutorials.");
      return;
    }
    setErrorMessage(null);
    setIsBusy(true);
    try {
      const format =
        tutorialDataFormat ?? projectConfig?.defaultFormat ?? "json";
      const tutorialsFile = projectConfig?.dataFiles.tutorials;
      const path = await repository.saveTutorialsData(
        dataDirectory,
        tutorialData,
        format,
        tutorialsFile
      );
      setTutorialDataPath(path);
      setTutorialDataFormat(format);
      setTutorialDataDir(dataDirectory);
      setStatusMessage(`Saved tutorials (${format})`);
    } catch (error) {
      setErrorMessage(`Failed to save tutorials. ${String(error)}`);
    } finally {
      setIsBusy(false);
    }
  }, [
    dataDirectory,
    projectConfig,
    repository,
    tutorialData,
    tutorialDataFormat
  ]);

  const handleLoadLootData = useCallback(
    async (overrideDir?: string) => {
      const targetDir =
        typeof overrideDir === "string" ? overrideDir : dataDirectory;
      if (!targetDir) {
        setErrorMessage("No data directory set for loot.");
        return;
      }
      const lootFile = projectConfig?.dataFiles.loot;
      const defaultFormat = projectConfig?.defaultFormat;
      setErrorMessage(null);
      setIsBusy(true);
      try {
        const source = await repository.loadLootData(
          targetDir,
          lootFile,
          defaultFormat
        );
        setLootData(source.data);
        setLootDataPath(source.path);
        setLootDataFormat(source.format);
        setLootDataDir(targetDir);
        setStatusMessage(`Loaded loot (${source.format})`);
      } catch (error) {
        setErrorMessage(`Failed to load loot. ${String(error)}`);
      } finally {
        setIsBusy(false);
      }
    },
    [dataDirectory, projectConfig, repository]
  );

  const handleSaveLootData = useCallback(async () => {
    if (!lootData) {
      setErrorMessage("No loot loaded to save.");
      return;
    }
    if (!dataDirectory) {
      setErrorMessage("No data directory set for loot.");
      return;
    }
    setErrorMessage(null);
    setIsBusy(true);
    try {
      const format = lootDataFormat ?? projectConfig?.defaultFormat ?? "json";
      const lootFile = projectConfig?.dataFiles.loot;
      const path = await repository.saveLootData(
        dataDirectory,
        lootData,
        format,
        lootFile
      );
      setLootDataPath(path);
      setLootDataFormat(format);
      setLootDataDir(dataDirectory);
      setStatusMessage(`Saved loot (${format})`);
    } catch (error) {
      setErrorMessage(`Failed to save loot. ${String(error)}`);
    } finally {
      setIsBusy(false);
    }
  }, [dataDirectory, lootData, lootDataFormat, projectConfig, repository]);

  useEffect(() => {
    const nextRoom = getDefaultSelection(areaData, "Rooms", selectedRoomVnum);
    if (selectedRoomVnum !== nextRoom) {
      setSelectedRoomVnum(nextRoom);
    }
    const nextMobile = getDefaultSelection(
      areaData,
      "Mobiles",
      selectedMobileVnum
    );
    if (selectedMobileVnum !== nextMobile) {
      setSelectedMobileVnum(nextMobile);
    }
    const nextObject = getDefaultSelection(
      areaData,
      "Objects",
      selectedObjectVnum
    );
    if (selectedObjectVnum !== nextObject) {
      setSelectedObjectVnum(nextObject);
    }
    const nextReset = getDefaultSelection(
      areaData,
      "Resets",
      selectedResetIndex
    );
    if (selectedResetIndex !== nextReset) {
      setSelectedResetIndex(nextReset);
    }
    const nextShop = getDefaultSelection(
      areaData,
      "Shops",
      selectedShopKeeper
    );
    if (selectedShopKeeper !== nextShop) {
      setSelectedShopKeeper(nextShop);
    }
    const nextQuest = getDefaultSelection(
      areaData,
      "Quests",
      selectedQuestVnum
    );
    if (selectedQuestVnum !== nextQuest) {
      setSelectedQuestVnum(nextQuest);
    }
    const nextFaction = getDefaultSelection(
      areaData,
      "Factions",
      selectedFactionVnum
    );
    if (selectedFactionVnum !== nextFaction) {
      setSelectedFactionVnum(nextFaction);
    }
    const nextRecipe = getDefaultSelection(
      areaData,
      "Recipes",
      selectedRecipeVnum
    );
    if (selectedRecipeVnum !== nextRecipe) {
      setSelectedRecipeVnum(nextRecipe);
    }
    const nextGather = getDefaultSelection(
      areaData,
      "Gather Spawns",
      selectedGatherVnum
    );
    if (selectedGatherVnum !== nextGather) {
      setSelectedGatherVnum(nextGather);
    }
  }, [
    areaData,
    selectedRoomVnum,
    selectedMobileVnum,
    selectedObjectVnum,
    selectedResetIndex,
    selectedShopKeeper,
    selectedQuestVnum,
    selectedFactionVnum,
    selectedRecipeVnum,
    selectedGatherVnum
  ]);

  const areaLootData = useMemo(
    () => extractAreaLootData(areaData),
    [areaData]
  );

  useEffect(() => {
    const groupCount = areaLootData?.groups.length ?? 0;
    const tableCount = areaLootData?.tables.length ?? 0;
    if (!areaLootData || (groupCount === 0 && tableCount === 0)) {
      if (selectedAreaLootKind !== null || selectedAreaLootIndex !== null) {
        setSelectedAreaLootKind(null);
        setSelectedAreaLootIndex(null);
      }
      return;
    }
    if (selectedAreaLootKind === "group") {
      if (selectedAreaLootIndex === null || selectedAreaLootIndex >= groupCount) {
        setSelectedAreaLootKind(groupCount ? "group" : "table");
        setSelectedAreaLootIndex(0);
      }
      return;
    }
    if (selectedAreaLootKind === "table") {
      if (selectedAreaLootIndex === null || selectedAreaLootIndex >= tableCount) {
        setSelectedAreaLootKind(groupCount ? "group" : "table");
        setSelectedAreaLootIndex(0);
      }
      return;
    }
    setSelectedAreaLootKind(groupCount ? "group" : "table");
    setSelectedAreaLootIndex(0);
  }, [areaLootData, selectedAreaLootKind, selectedAreaLootIndex]);

  useEffect(() => {
    setSelectedRoomVnum(null);
    setSelectedMobileVnum(null);
    setSelectedObjectVnum(null);
    setSelectedResetIndex(null);
    setSelectedShopKeeper(null);
    setSelectedQuestVnum(null);
    setSelectedFactionVnum(null);
    setSelectedRecipeVnum(null);
    setSelectedGatherVnum(null);
    setSelectedAreaLootKind(null);
    setSelectedAreaLootIndex(null);
  }, [areaPath]);

  useEffect(() => {
    if (
      editorMode === "Global" &&
      activeTab !== "Table" &&
      activeTab !== "Form"
    ) {
      setActiveTab("Table");
    }
  }, [editorMode, activeTab]);

  useEffect(() => {
    if (editorMode !== "Global" || selectedGlobalEntity !== "Classes") {
      return;
    }
    if (!dataDirectory) {
      return;
    }
    if (classData && classDataDir === dataDirectory) {
      return;
    }
    void handleLoadClassesData(dataDirectory);
  }, [
    editorMode,
    selectedGlobalEntity,
    dataDirectory,
    classData,
    classDataDir,
    handleLoadClassesData
  ]);

  useEffect(() => {
    if (editorMode !== "Global" || selectedGlobalEntity !== "Races") {
      return;
    }
    if (!dataDirectory) {
      return;
    }
    if (raceData && raceDataDir === dataDirectory) {
      return;
    }
    void handleLoadRacesData(dataDirectory);
  }, [
    editorMode,
    selectedGlobalEntity,
    dataDirectory,
    raceData,
    raceDataDir,
    handleLoadRacesData
  ]);

  useEffect(() => {
    if (editorMode !== "Global" || selectedGlobalEntity !== "Skills") {
      return;
    }
    if (!dataDirectory) {
      return;
    }
    if (skillData && skillDataDir === dataDirectory) {
      return;
    }
    void handleLoadSkillsData(dataDirectory);
  }, [
    editorMode,
    selectedGlobalEntity,
    dataDirectory,
    skillData,
    skillDataDir,
    handleLoadSkillsData
  ]);

  useEffect(() => {
    if (editorMode !== "Global" || selectedGlobalEntity !== "Groups") {
      return;
    }
    if (!dataDirectory) {
      return;
    }
    if (groupData && groupDataDir === dataDirectory) {
      return;
    }
    void handleLoadGroupsData(dataDirectory);
  }, [
    editorMode,
    selectedGlobalEntity,
    dataDirectory,
    groupData,
    groupDataDir,
    handleLoadGroupsData
  ]);

  useEffect(() => {
    if (editorMode !== "Global" || selectedGlobalEntity !== "Commands") {
      return;
    }
    if (!dataDirectory) {
      return;
    }
    if (commandData && commandDataDir === dataDirectory) {
      return;
    }
    void handleLoadCommandsData(dataDirectory);
  }, [
    editorMode,
    selectedGlobalEntity,
    dataDirectory,
    commandData,
    commandDataDir,
    handleLoadCommandsData
  ]);

  useEffect(() => {
    if (editorMode !== "Global" || selectedGlobalEntity !== "Socials") {
      return;
    }
    if (!dataDirectory) {
      return;
    }
    if (socialData && socialDataDir === dataDirectory) {
      return;
    }
    void handleLoadSocialsData(dataDirectory);
  }, [
    editorMode,
    selectedGlobalEntity,
    dataDirectory,
    socialData,
    socialDataDir,
    handleLoadSocialsData
  ]);

  useEffect(() => {
    if (editorMode !== "Global" || selectedGlobalEntity !== "Tutorials") {
      return;
    }
    if (!dataDirectory) {
      return;
    }
    if (tutorialData && tutorialDataDir === dataDirectory) {
      return;
    }
    void handleLoadTutorialsData(dataDirectory);
  }, [
    editorMode,
    selectedGlobalEntity,
    dataDirectory,
    tutorialData,
    tutorialDataDir,
    handleLoadTutorialsData
  ]);

  useEffect(() => {
    if (editorMode !== "Global" || selectedGlobalEntity !== "Loot") {
      return;
    }
    if (!dataDirectory) {
      return;
    }
    if (lootData && lootDataDir === dataDirectory) {
      return;
    }
    void handleLoadLootData(dataDirectory);
  }, [
    editorMode,
    selectedGlobalEntity,
    dataDirectory,
    lootData,
    lootDataDir,
    handleLoadLootData
  ]);

  useEffect(() => {
    if (!classData || classData.classes.length === 0) {
      if (selectedClassIndex !== null) {
        setSelectedClassIndex(null);
      }
      return;
    }
    if (
      selectedClassIndex === null ||
      selectedClassIndex >= classData.classes.length
    ) {
      setSelectedClassIndex(0);
    }
  }, [classData, selectedClassIndex]);

  useEffect(() => {
    if (!raceData || raceData.races.length === 0) {
      if (selectedRaceIndex !== null) {
        setSelectedRaceIndex(null);
      }
      return;
    }
    if (
      selectedRaceIndex === null ||
      selectedRaceIndex >= raceData.races.length
    ) {
      setSelectedRaceIndex(0);
    }
  }, [raceData, selectedRaceIndex]);

  useEffect(() => {
    if (!skillData || skillData.skills.length === 0) {
      if (selectedSkillIndex !== null) {
        setSelectedSkillIndex(null);
      }
      return;
    }
    if (
      selectedSkillIndex === null ||
      selectedSkillIndex >= skillData.skills.length
    ) {
      setSelectedSkillIndex(0);
    }
  }, [skillData, selectedSkillIndex]);

  useEffect(() => {
    if (!groupData || groupData.groups.length === 0) {
      if (selectedGroupIndex !== null) {
        setSelectedGroupIndex(null);
      }
      return;
    }
    if (
      selectedGroupIndex === null ||
      selectedGroupIndex >= groupData.groups.length
    ) {
      setSelectedGroupIndex(0);
    }
  }, [groupData, selectedGroupIndex]);

  useEffect(() => {
    if (!commandData || commandData.commands.length === 0) {
      if (selectedCommandIndex !== null) {
        setSelectedCommandIndex(null);
      }
      return;
    }
    if (
      selectedCommandIndex === null ||
      selectedCommandIndex >= commandData.commands.length
    ) {
      setSelectedCommandIndex(0);
    }
  }, [commandData, selectedCommandIndex]);

  useEffect(() => {
    if (!socialData || socialData.socials.length === 0) {
      if (selectedSocialIndex !== null) {
        setSelectedSocialIndex(null);
      }
      return;
    }
    if (
      selectedSocialIndex === null ||
      selectedSocialIndex >= socialData.socials.length
    ) {
      setSelectedSocialIndex(0);
    }
  }, [socialData, selectedSocialIndex]);

  useEffect(() => {
    if (!tutorialData || tutorialData.tutorials.length === 0) {
      if (selectedTutorialIndex !== null) {
        setSelectedTutorialIndex(null);
      }
      return;
    }
    if (
      selectedTutorialIndex === null ||
      selectedTutorialIndex >= tutorialData.tutorials.length
    ) {
      setSelectedTutorialIndex(0);
    }
  }, [tutorialData, selectedTutorialIndex]);

  useEffect(() => {
    const groupCount = lootData?.groups.length ?? 0;
    const tableCount = lootData?.tables.length ?? 0;
    if (!lootData || (groupCount === 0 && tableCount === 0)) {
      if (selectedLootKind !== null || selectedLootIndex !== null) {
        setSelectedLootKind(null);
        setSelectedLootIndex(null);
      }
      return;
    }
    if (selectedLootKind === "group") {
      if (selectedLootIndex === null || selectedLootIndex >= groupCount) {
        setSelectedLootKind(groupCount ? "group" : "table");
        setSelectedLootIndex(0);
      }
      return;
    }
    if (selectedLootKind === "table") {
      if (selectedLootIndex === null || selectedLootIndex >= tableCount) {
        setSelectedLootKind(groupCount ? "group" : "table");
        setSelectedLootIndex(0);
      }
      return;
    }
    setSelectedLootKind(groupCount ? "group" : "table");
    setSelectedLootIndex(0);
  }, [lootData, selectedLootKind, selectedLootIndex]);

  useEffect(() => {
    if (!classData || selectedClassIndex === null) {
      resetClassForm(classFormDefaults);
      return;
    }
    const cls = classData.classes[selectedClassIndex];
    resetClassForm({
      name: cls.name ?? "",
      whoName: cls.whoName ?? "",
      baseGroup: cls.baseGroup ?? "",
      defaultGroup: cls.defaultGroup ?? "",
      weaponVnum: cls.weaponVnum ?? 0,
      armorProf: cls.armorProf ?? "",
      guilds: Array.from({ length: classGuildCount }).map(
        (_, index) => cls.guilds?.[index] ?? 0
      ),
      primeStat: cls.primeStat ?? "",
      skillCap: cls.skillCap ?? 0,
      thac0_00: cls.thac0_00 ?? 0,
      thac0_32: cls.thac0_32 ?? 0,
      hpMin: cls.hpMin ?? 0,
      hpMax: cls.hpMax ?? 0,
      manaUser: cls.manaUser ?? false,
      startLoc: cls.startLoc ?? 0,
      titlesMale: titlesToText(cls.titles, 0),
      titlesFemale: titlesToText(cls.titles, 1)
    });
  }, [classData, classFormDefaults, resetClassForm, selectedClassIndex]);

  useEffect(() => {
    if (!raceData || selectedRaceIndex === null) {
      resetRaceForm(raceFormDefaults);
      return;
    }
    const race = raceData.races[selectedRaceIndex];
    const classNames = referenceData?.classes ?? [];
    resetRaceForm({
      name: race.name ?? "",
      whoName: race.whoName ?? "",
      pc: race.pc ?? false,
      points: race.points ?? 0,
      size: race.size ?? "",
      startLoc: race.startLoc ?? 0,
      stats: normalizeRaceStats(race.stats),
      maxStats: normalizeRaceStats(race.maxStats),
      actFlags: race.actFlags ?? [],
      affectFlags: race.affectFlags ?? [],
      offFlags: race.offFlags ?? [],
      immFlags: race.immFlags ?? [],
      resFlags: race.resFlags ?? [],
      vulnFlags: race.vulnFlags ?? [],
      formFlags: race.formFlags ?? [],
      partFlags: race.partFlags ?? [],
      classMult: normalizeRaceClassMap(race.classMult, classNames, 100),
      classStart: normalizeRaceClassMap(race.classStart, classNames, 0),
      skills: normalizeRaceSkills(race.skills)
    });
  }, [
    raceData,
    selectedRaceIndex,
    raceFormDefaults,
    referenceData,
    resetRaceForm
  ]);

  useEffect(() => {
    if (!skillData || selectedSkillIndex === null) {
      resetSkillForm(skillFormDefaults);
      return;
    }
    const skill = skillData.skills[selectedSkillIndex];
    const classNames = referenceData?.classes ?? [];
    resetSkillForm({
      name: skill.name ?? "",
      levels: normalizeRaceClassMap(
        skill.levels as Record<string, number> | number[] | undefined,
        classNames,
        defaultSkillLevel
      ),
      ratings: normalizeRaceClassMap(
        skill.ratings as Record<string, number> | number[] | undefined,
        classNames,
        defaultSkillRating
      ),
      spell: skill.spell ?? "",
      loxSpell: skill.loxSpell ?? "",
      target: skill.target ?? "",
      minPosition: skill.minPosition ?? "",
      gsn: skill.gsn ?? "",
      slot: skill.slot ?? 0,
      minMana: skill.minMana ?? 0,
      beats: skill.beats ?? 0,
      nounDamage: skill.nounDamage ?? "",
      msgOff: skill.msgOff ?? "",
      msgObj: skill.msgObj ?? ""
    });
  }, [
    skillData,
    selectedSkillIndex,
    skillFormDefaults,
    referenceData,
    resetSkillForm
  ]);

  useEffect(() => {
    if (!groupData || selectedGroupIndex === null) {
      resetGroupForm(groupFormDefaults);
      return;
    }
    const group = groupData.groups[selectedGroupIndex];
    const classNames = referenceData?.classes ?? [];
    resetGroupForm({
      name: group.name ?? "",
      ratings: normalizeRaceClassMap(
        group.ratings as Record<string, number> | number[] | undefined,
        classNames,
        defaultSkillRating
      ),
      skills: normalizeGroupSkills(group.skills)
    });
  }, [
    groupData,
    selectedGroupIndex,
    groupFormDefaults,
    referenceData,
    resetGroupForm
  ]);

  useEffect(() => {
    if (!commandData || selectedCommandIndex === null) {
      resetCommandForm(commandFormDefaults);
      return;
    }
    const command = commandData.commands[selectedCommandIndex];
    resetCommandForm({
      name: command.name ?? "",
      function: command.function ?? "",
      position: command.position ?? "",
      level: command.level ?? 0,
      log: command.log ?? "",
      category: command.category ?? "",
      loxFunction: command.loxFunction ?? ""
    });
  }, [
    commandData,
    selectedCommandIndex,
    commandFormDefaults,
    resetCommandForm
  ]);

  useEffect(() => {
    if (!socialData || selectedSocialIndex === null) {
      resetSocialForm(socialFormDefaults);
      return;
    }
    const social = socialData.socials[selectedSocialIndex];
    resetSocialForm({
      name: social.name ?? "",
      charNoArg: normalizeLineEndingsForDisplay(social.charNoArg),
      othersNoArg: normalizeLineEndingsForDisplay(social.othersNoArg),
      charFound: normalizeLineEndingsForDisplay(social.charFound),
      othersFound: normalizeLineEndingsForDisplay(social.othersFound),
      victFound: normalizeLineEndingsForDisplay(social.victFound),
      charAuto: normalizeLineEndingsForDisplay(social.charAuto),
      othersAuto: normalizeLineEndingsForDisplay(social.othersAuto)
    });
  }, [
    socialData,
    selectedSocialIndex,
    socialFormDefaults,
    resetSocialForm
  ]);

  useEffect(() => {
    if (!tutorialData || selectedTutorialIndex === null) {
      resetTutorialForm(tutorialFormDefaults);
      return;
    }
    const tutorial = tutorialData.tutorials[selectedTutorialIndex];
    resetTutorialForm({
      name: tutorial.name ?? "",
      blurb: normalizeLineEndingsForDisplay(tutorial.blurb),
      finish: normalizeLineEndingsForDisplay(tutorial.finish),
      minLevel: tutorial.minLevel ?? 0,
      steps: (tutorial.steps ?? []).map((step) => ({
        prompt: normalizeLineEndingsForDisplay(step.prompt),
        match: step.match ?? ""
      }))
    });
  }, [
    tutorialData,
    selectedTutorialIndex,
    tutorialFormDefaults,
    resetTutorialForm
  ]);

  useEffect(() => {
    if (
      !areaLootData ||
      selectedAreaLootKind === null ||
      selectedAreaLootIndex === null
    ) {
      resetAreaLootForm(lootFormDefaults);
      return;
    }
    if (selectedAreaLootKind === "group") {
      const group = areaLootData.groups[selectedAreaLootIndex];
      if (!group) {
        resetAreaLootForm(lootFormDefaults);
        return;
      }
      resetAreaLootForm({
        kind: "group",
        name: group.name ?? "",
        rolls: group.rolls ?? 1,
        entries: (group.entries ?? []).map((entry) => ({
          type: entry.type === "cp" ? "cp" : "item",
          vnum: entry.vnum ?? 0,
          minQty: entry.minQty ?? 1,
          maxQty: entry.maxQty ?? 1,
          weight: entry.weight ?? 100
        })),
        parent: "",
        ops: []
      });
      return;
    }
    const table = areaLootData.tables[selectedAreaLootIndex];
    if (!table) {
      resetAreaLootForm(lootFormDefaults);
      return;
    }
    resetAreaLootForm({
      kind: "table",
      name: table.name ?? "",
      parent: table.parent ?? "",
      rolls: 1,
      entries: [],
      ops: (table.ops ?? []).map((op) => ({
        op: op.op,
        group: op.group ?? "",
        vnum: op.vnum ?? 0,
        chance: op.chance ?? 100,
        minQty: op.minQty ?? 1,
        maxQty: op.maxQty ?? 1,
        multiplier: op.multiplier ?? 100
      }))
    });
  }, [
    areaLootData,
    selectedAreaLootKind,
    selectedAreaLootIndex,
    lootFormDefaults,
    resetAreaLootForm
  ]);

  useEffect(() => {
    if (
      !lootData ||
      selectedLootKind === null ||
      selectedLootIndex === null
    ) {
      resetLootForm(lootFormDefaults);
      return;
    }
    if (selectedLootKind === "group") {
      const group = lootData.groups[selectedLootIndex];
      if (!group) {
        resetLootForm(lootFormDefaults);
        return;
      }
      resetLootForm({
        kind: "group",
        name: group.name ?? "",
        rolls: group.rolls ?? 1,
        entries: (group.entries ?? []).map((entry) => ({
          type: entry.type === "cp" ? "cp" : "item",
          vnum: entry.vnum ?? 0,
          minQty: entry.minQty ?? 1,
          maxQty: entry.maxQty ?? 1,
          weight: entry.weight ?? 100
        })),
        parent: "",
        ops: []
      });
      return;
    }
    const table = lootData.tables[selectedLootIndex];
    if (!table) {
      resetLootForm(lootFormDefaults);
      return;
    }
    resetLootForm({
      kind: "table",
      name: table.name ?? "",
      parent: table.parent ?? "",
      rolls: 1,
      entries: [],
      ops: (table.ops ?? []).map((op) => ({
        op: op.op,
        group: op.group ?? "",
        vnum: op.vnum ?? 0,
        chance: op.chance ?? 100,
        minQty: op.minQty ?? 1,
        maxQty: op.maxQty ?? 1,
        multiplier: op.multiplier ?? 100
      }))
    });
  }, [
    lootData,
    selectedLootKind,
    selectedLootIndex,
    lootFormDefaults,
    resetLootForm
  ]);

  const selection = useMemo(
    () =>
      buildSelectionSummary(selectedEntity, areaData, {
        Rooms: selectedRoomVnum,
        Mobiles: selectedMobileVnum,
        Objects: selectedObjectVnum,
        Resets: selectedResetIndex,
        Shops: selectedShopKeeper,
        Quests: selectedQuestVnum,
        Factions: selectedFactionVnum,
        Recipes: selectedRecipeVnum,
        "Gather Spawns": selectedGatherVnum
      },
      {
        data: areaLootData,
        kind: selectedAreaLootKind,
        index: selectedAreaLootIndex
      }),
    [
      areaData,
      selectedEntity,
      selectedRoomVnum,
      selectedMobileVnum,
      selectedObjectVnum,
      selectedResetIndex,
      selectedShopKeeper,
      selectedQuestVnum,
      selectedFactionVnum,
      selectedRecipeVnum,
      selectedGatherVnum,
      areaLootData,
      selectedAreaLootKind,
      selectedAreaLootIndex
    ]
  );
  const selectedEntityVnum = useMemo(() => {
    if (selectedEntity === "Rooms") {
      return selectedRoomVnum;
    }
    if (selectedEntity === "Mobiles") {
      return selectedMobileVnum;
    }
    if (selectedEntity === "Objects") {
      return selectedObjectVnum;
    }
    if (selectedEntity === "Recipes") {
      return selectedRecipeVnum;
    }
    if (selectedEntity === "Gather Spawns") {
      return selectedGatherVnum;
    }
    return null;
  }, [
    selectedEntity,
    selectedRoomVnum,
    selectedMobileVnum,
    selectedObjectVnum,
    selectedRecipeVnum,
    selectedGatherVnum
  ]);
  const validationConfig = useMemo(() => loadValidationConfig(), []);
  const pluginValidationRules = useMemo(() => loadPluginRules(), []);
  const exitValidation = useMemo(
    () => buildExitTargetValidation(areaData, areaIndex),
    [areaData, areaIndex]
  );
  const coreValidationRules = useMemo<ValidationRule[]>(
    () => [
      {
        id: "vnum-range",
        label: "VNUM range",
        source: "core",
        run: ({ areaData: data }) => validateVnumRanges(data)
      },
      {
        id: "duplicate-vnum",
        label: "Duplicate VNUMs",
        source: "core",
        run: ({ areaData: data }) => validateDuplicateVnums(data)
      },
      {
        id: "exit-targets",
        label: "Exit targets",
        source: "core",
        run: () => exitValidation.issues
      },
      {
        id: "reset-references",
        label: "Reset references",
        source: "core",
        run: ({ areaData: data }) => validateResetReferences(data)
      },
      {
        id: "one-way-exits",
        label: "One-way exits",
        source: "core",
        run: ({ areaData: data }) => validateOneWayExits(data)
      },
      {
        id: "orphan-rooms",
        label: "Orphan rooms",
        source: "core",
        run: ({ areaData: data }) => validateOrphanRooms(data)
      }
    ],
    [exitValidation]
  );
  const validationIssues = useMemo(
    () =>
      buildValidationIssues(
        [...coreValidationRules, ...pluginValidationRules],
        { areaData, areaIndex },
        validationConfig
      ),
    [areaData, areaIndex, coreValidationRules, pluginValidationRules, validationConfig]
  );
  const validationSummary = useMemo(
    () =>
      buildValidationSummary(
        validationIssues,
        selectedEntity,
        selectedEntityVnum,
        selectedResetIndex
      ),
    [validationIssues, selectedEntity, selectedEntityVnum, selectedResetIndex]
  );
  const diagnosticsList = useMemo(() => {
    const filtered =
      diagnosticFilter === "All"
        ? validationIssues
        : validationIssues.filter(
            (issue) => issue.entityType === diagnosticFilter
          );
    const priority: Record<ValidationSeverity, number> = {
      error: 0,
      warning: 1
    };
    return [...filtered].sort((a, b) => {
      const severityOrder = priority[a.severity] - priority[b.severity];
      if (severityOrder !== 0) {
        return severityOrder;
      }
      if (a.entityType !== b.entityType) {
        return a.entityType.localeCompare(b.entityType);
      }
      const aKey = a.resetIndex ?? a.vnum ?? 0;
      const bKey = b.resetIndex ?? b.vnum ?? 0;
      return aKey - bKey;
    });
  }, [validationIssues, diagnosticFilter]);
  const diagnosticsCount = diagnosticsList.length;
  const handleDiagnosticsClick = useCallback(
    (issue: ValidationIssue) => {
      if (issue.entityType === "Rooms" && typeof issue.vnum === "number") {
        setSelectedEntity("Rooms");
        setSelectedRoomVnum(issue.vnum);
        return;
      }
      if (issue.entityType === "Mobiles" && typeof issue.vnum === "number") {
        setSelectedEntity("Mobiles");
        setSelectedMobileVnum(issue.vnum);
        return;
      }
      if (issue.entityType === "Objects" && typeof issue.vnum === "number") {
        setSelectedEntity("Objects");
        setSelectedObjectVnum(issue.vnum);
        return;
      }
      if (
        issue.entityType === "Resets" &&
        typeof issue.resetIndex === "number"
      ) {
        setSelectedEntity("Resets");
        setSelectedResetIndex(issue.resetIndex);
      }
    },
    [
      setSelectedEntity,
      setSelectedRoomVnum,
      setSelectedMobileVnum,
      setSelectedObjectVnum,
      setSelectedResetIndex
    ]
  );
  const areaName = fileNameFromPath(areaPath);
  const areaDebug = useMemo(() => buildAreaDebugSummary(areaData), [areaData]);
  const areaVnumRange = useMemo(() => getAreaVnumRange(areaData), [areaData]);
  const entityItems = useMemo(
    () =>
      entityOrder.map((key) => ({
        key,
        count: getEntityList(areaData, key).length
      })),
    [areaData]
  );
  const lootCount = lootData
    ? lootData.groups.length + lootData.tables.length
    : 0;
  const globalItems = useMemo(() => {
    const counts: Record<GlobalEntityKey, number> = {
      Classes: referenceData?.classes.length ?? 0,
      Races: referenceData?.races.length ?? 0,
      Skills: referenceData?.skills.length ?? 0,
      Groups: referenceData?.groups.length ?? 0,
      Commands: referenceData?.commands.length ?? 0,
      Socials: referenceData?.socials.length ?? 0,
      Tutorials: referenceData?.tutorials.length ?? 0,
      Loot: lootCount
    };
    return globalEntityOrder.map((key) => ({
      key,
      count: counts[key] ?? 0
    }));
  }, [lootCount, referenceData]);
  const classRows = useMemo(() => buildClassRows(classData), [classData]);
  const raceRows = useMemo(() => buildRaceRows(raceData), [raceData]);
  const skillRows = useMemo(() => buildSkillRows(skillData), [skillData]);
  const groupRows = useMemo(() => buildGroupRows(groupData), [groupData]);
  const commandRows = useMemo(
    () => buildCommandRows(commandData),
    [commandData]
  );
  const socialRows = useMemo(
    () => buildSocialRows(socialData),
    [socialData]
  );
  const tutorialRows = useMemo(
    () => buildTutorialRows(tutorialData),
    [tutorialData]
  );
  const areaLootRows = useMemo(
    () => buildLootRows(areaLootData),
    [areaLootData]
  );
  const recipeRows = useMemo(() => buildRecipeRows(areaData), [areaData]);
  const gatherSpawnRows = useMemo(
    () => buildGatherSpawnRows(areaData),
    [areaData]
  );
  const lootRows = useMemo(() => buildLootRows(lootData), [lootData]);
  const roomRows = useMemo(() => buildRoomRows(areaData), [areaData]);
  const mobileRows = useMemo(() => buildMobileRows(areaData), [areaData]);
  const objectRows = useMemo(() => buildObjectRows(areaData), [areaData]);
  const resetRows = useMemo(() => buildResetRows(areaData), [areaData]);
  const shopRows = useMemo(() => buildShopRows(areaData), [areaData]);
  const questRows = useMemo(() => buildQuestRows(areaData), [areaData]);
  const factionRows = useMemo(() => buildFactionRows(areaData), [areaData]);
  const baseRoomNodes = useMemo(() => buildRoomNodes(areaData), [areaData]);
  const layoutSourceNodes = useMemo(
    () => (layoutNodes.length ? layoutNodes : baseRoomNodes),
    [layoutNodes, baseRoomNodes]
  );
  const roomNodesWithLayout = useMemo(
    () => applyRoomLayoutOverrides(layoutSourceNodes, roomLayout),
    [layoutSourceNodes, roomLayout]
  );
  const roomEdges = useMemo(
    () =>
      buildRoomEdges(
        areaData,
        showVerticalEdges,
        true,
        exitValidation.invalidEdgeIds
      ),
    [areaData, showVerticalEdges, exitValidation.invalidEdgeIds]
  );
  const externalExits = useMemo(
    () => buildExternalExits(areaData, areaIndex),
    [areaData, areaIndex]
  );
  const areaGraphContext = useMemo(() => {
    const entries: AreaGraphEntry[] = areaIndex.map((entry) => ({
      id: entry.fileName,
      name: entry.name,
      vnumRange: entry.vnumRange ?? null
    }));
    let currentId: string | null = null;
    if (areaData) {
      const currentFile = areaPath ? fileNameFromPath(areaPath) : null;
      const bounds = getAreaVnumBounds(areaData);
      if (currentFile) {
        const match = entries.find((entry) => entry.id === currentFile);
        if (match) {
          currentId = match.id;
        } else {
          const areadata =
            (areaData as Record<string, unknown>).areadata &&
            typeof (areaData as Record<string, unknown>).areadata === "object"
              ? ((areaData as Record<string, unknown>).areadata as Record<
                  string,
                  unknown
                >)
              : {};
          const name = getFirstString(areadata.name, currentFile);
          const syntheticId = `current:${currentFile}`;
          entries.unshift({
            id: syntheticId,
            name,
            vnumRange: bounds ? [bounds.min, bounds.max] : null
          });
          currentId = syntheticId;
        }
      } else {
        const areadata =
          (areaData as Record<string, unknown>).areadata &&
          typeof (areaData as Record<string, unknown>).areadata === "object"
            ? ((areaData as Record<string, unknown>).areadata as Record<
                string,
                unknown
              >)
            : {};
        entries.unshift({
          id: "current-area",
          name: getFirstString(areadata.name, "Current area"),
          vnumRange: bounds ? [bounds.min, bounds.max] : null
        });
        currentId = "current-area";
      }
    }
    return { entries, currentId };
  }, [areaIndex, areaData, areaPath]);
  const areaGraphFilterValue = areaGraphFilter.trim().toLowerCase();
  const areaGraphFilteredEntries = useMemo(() => {
    if (!areaGraphFilterValue) {
      return areaGraphContext.entries;
    }
    return areaGraphContext.entries.filter((entry) => {
      const name = entry.name.toLowerCase();
      const id = entry.id.toLowerCase();
      return name.includes(areaGraphFilterValue) || id.includes(areaGraphFilterValue);
    });
  }, [areaGraphContext.entries, areaGraphFilterValue]);
  const areaGraphMatch = useMemo(() => {
    const vnum = parseVnum(areaGraphVnumQuery);
    if (vnum === null) {
      return null;
    }
    return (
      areaGraphContext.entries.find((entry) => {
        if (!entry.vnumRange) {
          return false;
        }
        const [start, end] = entry.vnumRange;
        return vnum >= start && vnum <= end;
      }) ?? null
    );
  }, [areaGraphContext.entries, areaGraphVnumQuery]);
  const areaGraphMatchLabel = useMemo(() => {
    if (!areaGraphMatch) {
      return null;
    }
    return `${areaGraphMatch.name} (${formatVnumRange(areaGraphMatch.vnumRange)})`;
  }, [areaGraphMatch]);
  const areaGraphNodes = useMemo(() => {
    if (!areaGraphFilteredEntries.length) {
      return [];
    }
    return areaGraphFilteredEntries.map((entry, index) => ({
      id: entry.id,
      type: "area",
      position: {
        x: 0,
        y: index * 10
      },
      data: {
        label: entry.name || entry.id,
        range: formatVnumRange(entry.vnumRange),
        isCurrent: entry.id === areaGraphContext.currentId,
        isMatch: entry.id === areaGraphMatch?.id
      }
    }));
  }, [areaGraphFilteredEntries, areaGraphContext.currentId, areaGraphMatch]);
  const areaGraphEdges = useMemo(() => {
    const visible = new Set(areaGraphFilteredEntries.map((entry) => entry.id));
    const edges: Edge[] = [];
    for (const link of areaGraphLinks) {
      if (!visible.has(link.fromFile) || !visible.has(link.toFile)) {
        continue;
      }
      const dirKey = getDominantExitDirection(link.directionCounts);
      const handles = dirKey ? areaDirectionHandleMap[dirKey] : null;
      const edgeData = {
        nodeType: "area",
        ...(dirKey ? { dirKey } : {})
      };
      edges.push({
        id: `area-link-${link.fromFile}-${link.toFile}`,
        source: link.fromFile,
        target: link.toFile,
        label: link.count === 1 ? "1 exit" : `${link.count} exits`,
        animated: false,
        type: "area",
        sourceHandle: handles?.source,
        targetHandle: handles?.target,
        data: edgeData
      });
    }

    const currentId = areaGraphContext.currentId;
    if (!currentId) {
      return edges;
    }
    const indexIds = new Set(areaIndex.map((entry) => entry.fileName));
    const shouldUseExternal =
      !areaGraphLinks.length || !indexIds.has(currentId);
    if (!shouldUseExternal) {
      return edges;
    }
    const counts = new Map<string, number>();
    const directions = new Map<string, Record<string, number>>();
    for (const exit of externalExits) {
      const target = findAreaForVnum(areaIndex, exit.toVnum);
      if (!target) {
        continue;
      }
      const targetId = target.fileName;
      if (targetId === currentId) {
        continue;
      }
      counts.set(targetId, (counts.get(targetId) ?? 0) + 1);
      const dirKey = exit.direction.trim().toLowerCase();
      if (dirKey) {
        if (!directions.has(targetId)) {
          directions.set(targetId, {});
        }
        const record = directions.get(targetId);
        if (record) {
          record[dirKey] = (record[dirKey] ?? 0) + 1;
        }
      }
    }
    for (const [targetId, count] of counts.entries()) {
      if (!visible.has(currentId) || !visible.has(targetId)) {
        continue;
      }
      const dirKey = getDominantExitDirection(directions.get(targetId));
      const handles = dirKey ? areaDirectionHandleMap[dirKey] : null;
      const edgeData = {
        nodeType: "area",
        ...(dirKey ? { dirKey } : {})
      };
      edges.push({
        id: `area-link-${currentId}-${targetId}`,
        source: currentId,
        target: targetId,
        label: count === 1 ? "1 exit" : `${count} exits`,
        animated: false,
        type: "area",
        sourceHandle: handles?.source,
        targetHandle: handles?.target,
        data: edgeData
      });
    }
    return edges;
  }, [
    areaGraphContext.currentId,
    areaGraphFilteredEntries,
    areaGraphLinks,
    areaIndex,
    externalExits
  ]);
  const handleMapNavigate = useCallback(
    (vnum: number) => {
      setSelectedRoomVnum(vnum);
      setSelectedEntity("Rooms");
    },
    [setSelectedRoomVnum, setSelectedEntity]
  );
  const handleMapNodeClick = useCallback(
    (node: Node<RoomNodeData>) => {
      const vnum =
        typeof node.data?.vnum === "number"
          ? node.data.vnum
          : parseVnum(node.id);
      if (vnum !== null && vnum >= 0) {
        setSelectedRoomVnum(vnum);
        setSelectedEntity("Rooms");
      }
    },
    [setSelectedRoomVnum, setSelectedEntity]
  );
  const handleRelayout = useCallback(
    () => setLayoutNonce((value) => value + 1),
    []
  );
  const handleTogglePreferGrid = useCallback(
    (nextValue: boolean) => {
      setPreferCardinalLayout(nextValue);
      if (!autoLayoutEnabled) {
        setLayoutNonce((value) => value + 1);
      }
    },
    [autoLayoutEnabled]
  );
  const handleTogglePreferAreaGrid = useCallback((nextValue: boolean) => {
    setPreferAreaCardinalLayout(nextValue);
  }, []);
  const roomNodesWithHandlers = useMemo(
    () =>
      roomNodesWithLayout.map((node) => ({
        ...node,
        data: {
          ...node.data,
          onNavigate: handleMapNavigate,
          dirty: dirtyRoomNodes.has(node.id)
        }
      })),
    [roomNodesWithLayout, handleMapNavigate, dirtyRoomNodes]
  );
  const mapNodes = useMemo(
    () => applyRoomSelection(roomNodesWithHandlers, selectedRoomVnum),
    [roomNodesWithHandlers, selectedRoomVnum]
  );
  const selectedRoomNode = useMemo(() => {
    if (selectedRoomVnum === null) {
      return null;
    }
    return mapNodes.find((node) => node.data.vnum === selectedRoomVnum) ?? null;
  }, [mapNodes, selectedRoomVnum]);
  const selectedRoomLocked = Boolean(selectedRoomNode?.data.locked);
  const dirtyRoomCount = dirtyRoomNodes.size;
  const hasRoomLayout = Object.keys(roomLayout).length > 0;
  const worldMapNodes = useMemo(
    () =>
      applyAreaLayoutOverrides(
        areaGraphLayoutNodes.length ? areaGraphLayoutNodes : areaGraphNodes,
        areaLayout,
        dirtyAreaNodes
      ),
    [areaGraphLayoutNodes, areaGraphNodes, areaLayout, dirtyAreaNodes]
  );
  const selectedAreaNode = useMemo(
    () => worldMapNodes.find((node) => node.selected) ?? null,
    [worldMapNodes]
  );
  const selectedAreaLocked = Boolean(
    selectedAreaNode && areaLayout[selectedAreaNode.id]?.locked
  );
  const dirtyAreaCount = dirtyAreaNodes.size;
  const hasAreaLayout = Object.keys(areaLayout).length > 0;
  const handleLockSelectedRoom = useCallback(() => {
    if (!selectedRoomNode) {
      return;
    }
    setRoomLayout((current) => ({
      ...current,
      [selectedRoomNode.id]: {
        x: selectedRoomNode.position.x,
        y: selectedRoomNode.position.y,
        locked: true
      }
    }));
    setDirtyRoomNodes((current) => {
      if (!current.has(selectedRoomNode.id)) {
        return current;
      }
      const next = new Set(current);
      next.delete(selectedRoomNode.id);
      return next;
    });
  }, [selectedRoomNode]);
  const handleUnlockSelectedRoom = useCallback(() => {
    if (!selectedRoomNode) {
      return;
    }
    setLayoutNodes((current) => {
      const source = current.length ? current : roomNodesWithLayout;
      return source.map((node) =>
        node.id === selectedRoomNode.id
          ? { ...node, position: selectedRoomNode.position }
          : node
      );
    });
    setRoomLayout((current) => {
      if (!current[selectedRoomNode.id]) {
        return current;
      }
      const next = { ...current };
      delete next[selectedRoomNode.id];
      return next;
    });
    setDirtyRoomNodes((current) => {
      if (current.has(selectedRoomNode.id)) {
        return current;
      }
      const next = new Set(current);
      next.add(selectedRoomNode.id);
      return next;
    });
  }, [selectedRoomNode, roomNodesWithLayout]);
  const runAreaGraphLayout = useCallback(
    async (cancelRef?: { current: boolean }) => {
      if (!areaGraphNodes.length) {
        if (!cancelRef?.current) {
          setAreaGraphLayoutNodes([]);
        }
        return;
      }
      try {
        const nextNodes = await layoutAreaGraphNodes(
          areaGraphNodes,
          areaGraphEdges,
          preferAreaCardinalLayout
        );
        if (!cancelRef?.current) {
          setAreaGraphLayoutNodes(nextNodes);
          setDirtyAreaNodes(new Set());
        }
      } catch {
        if (!cancelRef?.current) {
          setAreaGraphLayoutNodes(areaGraphNodes);
          setDirtyAreaNodes(new Set());
        }
      }
    },
    [areaGraphNodes, areaGraphEdges, preferAreaCardinalLayout]
  );
  const handleLockSelectedArea = useCallback(() => {
    if (!selectedAreaNode) {
      return;
    }
    setAreaLayout((current) => ({
      ...current,
      [selectedAreaNode.id]: {
        x: selectedAreaNode.position.x,
        y: selectedAreaNode.position.y,
        locked: true
      }
    }));
    setDirtyAreaNodes((current) => {
      if (!current.has(selectedAreaNode.id)) {
        return current;
      }
      const next = new Set(current);
      next.delete(selectedAreaNode.id);
      return next;
    });
  }, [selectedAreaNode]);
  const handleUnlockSelectedArea = useCallback(() => {
    if (!selectedAreaNode) {
      return;
    }
    setAreaGraphLayoutNodes((current) => {
      const source = current.length ? current : areaGraphNodes;
      return source.map((node) =>
        node.id === selectedAreaNode.id
          ? { ...node, position: selectedAreaNode.position }
          : node
      );
    });
    setAreaLayout((current) => {
      if (!current[selectedAreaNode.id]) {
        return current;
      }
      const next = { ...current };
      delete next[selectedAreaNode.id];
      return next;
    });
    setDirtyAreaNodes((current) => {
      if (current.has(selectedAreaNode.id)) {
        return current;
      }
      const next = new Set(current);
      next.add(selectedAreaNode.id);
      return next;
    });
  }, [selectedAreaNode, areaGraphNodes]);
  const handleLockDirtyAreas = useCallback(() => {
    if (!dirtyAreaNodes.size) {
      return;
    }
    setAreaLayout((current) => {
      const next = { ...current };
      dirtyAreaNodes.forEach((id) => {
        const node = worldMapNodes.find((entry) => entry.id === id);
        if (!node) {
          return;
        }
        next[id] = {
          x: node.position.x,
          y: node.position.y,
          locked: true
        };
      });
      return next;
    });
    setDirtyAreaNodes(new Set());
  }, [dirtyAreaNodes, worldMapNodes]);
  const handleClearAreaLayout = useCallback(() => {
    setAreaLayout({});
    setDirtyAreaNodes(new Set());
    void runAreaGraphLayout();
  }, [runAreaGraphLayout]);
  const handleRelayoutArea = useCallback(() => {
    void runAreaGraphLayout();
  }, [runAreaGraphLayout]);
  const handleClearRoomLayout = useCallback(() => {
    setRoomLayout({});
    setLayoutNodes([]);
    setDirtyRoomNodes(new Set());
    if (autoLayoutEnabled) {
      setLayoutNonce((value) => value + 1);
    }
  }, [autoLayoutEnabled]);
  const handleNodesChange = useCallback(
    (changes: NodeChange[]) => {
      setLayoutNodes((current) => {
        const source = current.length ? current : roomNodesWithLayout;
        return applyNodeChanges(changes, source);
      });
      const movedIds = changes
        .filter((change) => change.type === "position")
        .map((change) => change.id);
      if (movedIds.length) {
        setDirtyRoomNodes((current) => {
          let changed = false;
          const next = new Set(current);
          movedIds.forEach((id) => {
            if (!next.has(id)) {
              next.add(id);
              changed = true;
            }
          });
          return changed ? next : current;
        });
      }
    },
    [roomNodesWithLayout]
  );
  const handleNodeDragStop = useCallback(
    (_: unknown, node: Node<RoomNodeData>) => {
      setLayoutNodes((current) => {
        const source = current.length ? current : roomNodesWithLayout;
        return source.map((entry) =>
          entry.id === node.id
            ? { ...entry, position: node.position }
            : entry
        );
      });
      setDirtyRoomNodes((current) => {
        if (current.has(node.id)) {
          return current;
        }
        const next = new Set(current);
        next.add(node.id);
        return next;
      });
    },
    [roomNodesWithLayout]
  );
  const handleAreaGraphNodesChange = useCallback(
    (changes: NodeChange[]) => {
      setAreaGraphLayoutNodes((current) => {
        const source = current.length ? current : areaGraphNodes;
        return applyNodeChanges(changes, source);
      });
      const movedIds = changes
        .filter((change) => change.type === "position")
        .map((change) => change.id);
      if (movedIds.length) {
        setDirtyAreaNodes((current) => {
          let changed = false;
          const next = new Set(current);
          movedIds.forEach((id) => {
            if (!next.has(id)) {
              next.add(id);
              changed = true;
            }
          });
          return changed ? next : current;
        });
      }
    },
    [areaGraphNodes]
  );
  const handleAreaGraphNodeDragStop = useCallback(
    (_: unknown, node: Node<AreaGraphNodeData>) => {
      setAreaGraphLayoutNodes((current) => {
        const source = current.length ? current : areaGraphNodes;
        return source.map((entry) =>
          entry.id === node.id ? { ...entry, position: node.position } : entry
        );
      });
      setDirtyAreaNodes((current) => {
        if (current.has(node.id)) {
          return current;
        }
        const next = new Set(current);
        next.add(node.id);
        return next;
      });
    },
    [areaGraphNodes]
  );
  useEffect(() => {
    const cancelRef = { current: false };
    runAreaGraphLayout(cancelRef);
    return () => {
      cancelRef.current = true;
    };
  }, [runAreaGraphLayout]);
  const runRoomLayout = useCallback(
    async (cancelRef?: { current: boolean }) => {
      if (activeTab !== "Map") {
        return;
      }
      if (!baseRoomNodes.length) {
        if (!cancelRef?.current) {
          setLayoutNodes([]);
          setDirtyRoomNodes(new Set());
        }
        return;
      }
      try {
        const nextNodes = await layoutRoomNodes(
          baseRoomNodes,
          roomEdges,
          preferCardinalLayout
        );
        if (!cancelRef?.current) {
          setLayoutNodes(
            applyRoomLayoutOverrides(nextNodes, roomLayoutRef.current)
          );
          setDirtyRoomNodes(new Set());
        }
      } catch (error) {
        if (!cancelRef?.current) {
          setLayoutNodes(
            applyRoomLayoutOverrides(baseRoomNodes, roomLayoutRef.current)
          );
          setDirtyRoomNodes(new Set());
        }
      }
    },
    [activeTab, baseRoomNodes, preferCardinalLayout, roomEdges]
  );

  useEffect(() => {
    roomLayoutRef.current = roomLayout;
  }, [roomLayout]);

  useEffect(() => {
    localStorage.setItem(
      "worldedit.preferCardinalLayout",
      String(preferCardinalLayout)
    );
  }, [preferCardinalLayout]);

  useEffect(() => {
    localStorage.setItem(
      "worldedit.preferAreaCardinalLayout",
      String(preferAreaCardinalLayout)
    );
  }, [preferAreaCardinalLayout]);

  useEffect(() => {
    localStorage.setItem(
      "worldedit.showVerticalEdges",
      String(showVerticalEdges)
    );
  }, [showVerticalEdges]);

  useEffect(() => {
    let cancelled = false;
    const loadIndex = async () => {
      const baseDir =
        areaDirectory ??
        (areaPath ? await repository.resolveAreaDirectory(areaPath) : null);
      if (!baseDir) {
        if (!cancelled) {
          setAreaIndex([]);
        }
        return;
      }
      try {
        const entries = await repository.loadAreaIndex(
          baseDir,
          projectConfig?.areaList
        );
        if (!cancelled) {
          setAreaIndex(entries);
        }
      } catch {
        if (!cancelled) {
          setAreaIndex([]);
        }
      }
    };
    loadIndex();
    return () => {
      cancelled = true;
    };
  }, [areaDirectory, areaPath, projectConfig, repository]);
  useEffect(() => {
    let cancelled = false;
    const loadLinks = async () => {
      if (!areaIndex.length) {
        if (!cancelled) {
          setAreaGraphLinks([]);
        }
        return;
      }
      const baseDir =
        areaDirectory ??
        (areaPath ? await repository.resolveAreaDirectory(areaPath) : null);
      if (!baseDir) {
        if (!cancelled) {
          setAreaGraphLinks([]);
        }
        return;
      }
      try {
        const links = await repository.loadAreaGraphLinks(baseDir, areaIndex);
        if (!cancelled) {
          setAreaGraphLinks(links);
        }
      } catch {
        if (!cancelled) {
          setAreaGraphLinks([]);
        }
      }
    };
    loadLinks();
    return () => {
      cancelled = true;
    };
  }, [areaDirectory, areaPath, areaIndex, repository]);
  const selectedRoomRecord = useMemo(() => {
    if (!areaData || selectedRoomVnum === null) {
      return null;
    }
    return findByVnum(getEntityList(areaData, "Rooms"), selectedRoomVnum);
  }, [areaData, selectedRoomVnum]);
  const selectedMobileRecord = useMemo(() => {
    if (!areaData || selectedMobileVnum === null) {
      return null;
    }
    return findByVnum(getEntityList(areaData, "Mobiles"), selectedMobileVnum);
  }, [areaData, selectedMobileVnum]);
  const selectedObjectRecord = useMemo(() => {
    if (!areaData || selectedObjectVnum === null) {
      return null;
    }
    return findByVnum(getEntityList(areaData, "Objects"), selectedObjectVnum);
  }, [areaData, selectedObjectVnum]);
  const selectedResetRecord = useMemo(() => {
    if (!areaData || selectedResetIndex === null) {
      return null;
    }
    const resets = getEntityList(areaData, "Resets");
    if (selectedResetIndex < 0 || selectedResetIndex >= resets.length) {
      return null;
    }
    const record = resets[selectedResetIndex];
    return record && typeof record === "object"
      ? (record as Record<string, unknown>)
      : null;
  }, [areaData, selectedResetIndex]);
  const selectedShopRecord = useMemo(() => {
    if (!areaData || selectedShopKeeper === null) {
      return null;
    }
    const shops = getEntityList(areaData, "Shops");
    return findShopByKeeper(shops, selectedShopKeeper);
  }, [areaData, selectedShopKeeper]);
  const selectedQuestRecord = useMemo(() => {
    if (!areaData || selectedQuestVnum === null) {
      return null;
    }
    return findByVnum(getEntityList(areaData, "Quests"), selectedQuestVnum);
  }, [areaData, selectedQuestVnum]);
  const selectedFactionRecord = useMemo(() => {
    if (!areaData || selectedFactionVnum === null) {
      return null;
    }
    return findByVnum(getEntityList(areaData, "Factions"), selectedFactionVnum);
  }, [areaData, selectedFactionVnum]);
  const selectedRecipeRecord = useMemo(() => {
    if (!areaData || selectedRecipeVnum === null) {
      return null;
    }
    return findByVnum(getEntityList(areaData, "Recipes"), selectedRecipeVnum);
  }, [areaData, selectedRecipeVnum]);
  const selectedGatherRecord = useMemo(() => {
    if (!areaData || selectedGatherVnum === null) {
      return null;
    }
    return findByVnum(
      getEntityList(areaData, "Gather Spawns"),
      selectedGatherVnum
    );
  }, [areaData, selectedGatherVnum]);
  const selectedLootRecord = useMemo(() => {
    if (!lootData || selectedLootKind === null || selectedLootIndex === null) {
      return null;
    }
    if (selectedLootKind === "group") {
      return lootData.groups[selectedLootIndex] ?? null;
    }
    return lootData.tables[selectedLootIndex] ?? null;
  }, [lootData, selectedLootKind, selectedLootIndex]);
  const scriptContext = useMemo(() => {
    if (selectedEntity === "Rooms") {
      return {
        entity: "Rooms" as const,
        vnum: selectedRoomVnum,
        record: selectedRoomRecord
      };
    }
    if (selectedEntity === "Mobiles") {
      return {
        entity: "Mobiles" as const,
        vnum: selectedMobileVnum,
        record: selectedMobileRecord
      };
    }
    if (selectedEntity === "Objects") {
      return {
        entity: "Objects" as const,
        vnum: selectedObjectVnum,
        record: selectedObjectRecord
      };
    }
    return null;
  }, [
    selectedEntity,
    selectedRoomVnum,
    selectedRoomRecord,
    selectedMobileVnum,
    selectedMobileRecord,
    selectedObjectVnum,
    selectedObjectRecord
  ]);
  const scriptValue = useMemo(() => {
    if (!scriptContext?.record) {
      return "";
    }
    const raw = (scriptContext.record as Record<string, unknown>).loxScript;
    return typeof raw === "string" ? normalizeLineEndingsForDisplay(raw) : "";
  }, [scriptContext]);
  const scriptTitle = useMemo(() => {
    if (!scriptContext) {
      return "Script View";
    }
    if (scriptContext.vnum === null) {
      return entityKindLabels[scriptContext.entity];
    }
    return `${entityKindLabels[scriptContext.entity]} ${scriptContext.vnum}`;
  }, [scriptContext]);
  const scriptSubtitle = useMemo(() => {
    if (!scriptContext?.record) {
      return "";
    }
    const record = scriptContext.record as Record<string, unknown>;
    if (scriptContext.entity === "Rooms") {
      return getFirstString(record.name, "");
    }
    if (scriptContext.entity === "Mobiles") {
      return getFirstString(record.shortDescr, "");
    }
    if (scriptContext.entity === "Objects") {
      return getFirstString(record.shortDescr, "");
    }
    return "";
  }, [scriptContext]);
  const eventBindings = useMemo<EventBinding[]>(() => {
    if (!scriptContext?.record) {
      return [];
    }
    const raw = (scriptContext.record as Record<string, unknown>).events;
    if (!Array.isArray(raw)) {
      return [];
    }
    return raw
      .filter((event) => event && typeof event === "object")
      .map((event) => {
        const record = event as Record<string, unknown>;
        const trigger = typeof record.trigger === "string" ? record.trigger : "";
        const callback =
          typeof record.callback === "string" ? record.callback : "";
        const criteria =
          typeof record.criteria === "string" || typeof record.criteria === "number"
            ? record.criteria
            : undefined;
        return {
          trigger,
          callback,
          criteria
        };
      });
  }, [scriptContext]);
  const canEditScript = Boolean(
    scriptContext?.record && scriptContext?.vnum !== null
  );
  const scriptEventEntity = scriptContext?.entity ?? null;
  const handleApplyScript = useCallback(
    (nextScript: string) => {
      if (!scriptContext || scriptContext.vnum === null) {
        return;
      }
      const normalized = normalizeOptionalScript(nextScript);
      setAreaData((current) => {
        if (!current) {
          return current;
        }
        const list = getEntityList(current, scriptContext.entity);
        if (!list.length) {
          return current;
        }
        const nextList = list.map((entry) => {
          if (!entry || typeof entry !== "object") {
            return entry;
          }
          const record = entry as Record<string, unknown>;
          const vnum = parseVnum(record.vnum);
          if (vnum !== scriptContext.vnum) {
            return record;
          }
          const nextRecord: Record<string, unknown> = { ...record };
          if (normalized) {
            nextRecord.loxScript = normalized;
          } else {
            delete nextRecord.loxScript;
          }
          return nextRecord;
        });
        return {
          ...(current as Record<string, unknown>),
          [entityDataKeys[scriptContext.entity]]: nextList
        };
      });
      setStatusMessage(
        `Updated ${entityKindLabels[scriptContext.entity].toLowerCase()} ${
          scriptContext.vnum
        } script (unsaved)`
      );
    },
    [scriptContext]
  );
  const handleEventBindingsChange = useCallback(
    (nextEvents: EventBinding[]) => {
      if (!scriptContext || scriptContext.vnum === null) {
        return;
      }
      setAreaData((current) => {
        if (!current) {
          return current;
        }
        const list = getEntityList(current, scriptContext.entity);
        if (!list.length) {
          return current;
        }
        const nextList = list.map((entry) => {
          if (!entry || typeof entry !== "object") {
            return entry;
          }
          const record = entry as Record<string, unknown>;
          const vnum = parseVnum(record.vnum);
          if (vnum !== scriptContext.vnum) {
            return record;
          }
          const nextRecord: Record<string, unknown> = { ...record };
          if (nextEvents.length) {
            nextRecord.events = nextEvents.map((event) => {
              const trigger = event.trigger.trim();
              const callback = event.callback.trim();
              const nextEvent: Record<string, unknown> = {
                trigger,
                callback
              };
              if (event.criteria !== undefined && event.criteria !== "") {
                nextEvent.criteria = event.criteria;
              }
              return nextEvent;
            });
          } else {
            delete nextRecord.events;
          }
          return nextRecord;
        });
        return {
          ...(current as Record<string, unknown>),
          [entityDataKeys[scriptContext.entity]]: nextList
        };
      });
      setStatusMessage(
        `Updated ${entityKindLabels[scriptContext.entity].toLowerCase()} ${
          scriptContext.vnum
        } events (unsaved)`
      );
    },
    [scriptContext]
  );
  const areaFormDefaults = useMemo<AreaFormValues>(() => {
    const areadata =
      areaData && typeof (areaData as Record<string, unknown>).areadata === "object"
        ? ((areaData as Record<string, unknown>).areadata as Record<string, unknown>)
        : {};
    const vnumRange = Array.isArray(areadata.vnumRange) ? areadata.vnumRange : [];
    const vnumRangeStart = parseVnum(vnumRange[0]) ?? 0;
    const vnumRangeEnd = parseVnum(vnumRange[1]) ?? 0;
    const storyBeats = Array.isArray((areaData as Record<string, unknown>)?.storyBeats)
      ? ((areaData as Record<string, unknown>).storyBeats as unknown[])
          .filter((entry) => entry && typeof entry === "object")
          .map((entry) => {
            const record = entry as Record<string, unknown>;
            return {
              title: typeof record.title === "string" ? record.title : "",
              description:
                typeof record.description === "string"
                  ? normalizeLineEndingsForDisplay(record.description)
                  : ""
            };
          })
      : [];
    const checklist = Array.isArray((areaData as Record<string, unknown>)?.checklist)
      ? ((areaData as Record<string, unknown>).checklist as unknown[])
          .filter((entry) => entry && typeof entry === "object")
          .map((entry) => {
            const record = entry as Record<string, unknown>;
            return {
              title: typeof record.title === "string" ? record.title : "",
              status: normalizeChecklistStatus(record.status),
              description:
                typeof record.description === "string"
                  ? normalizeLineEndingsForDisplay(record.description)
                  : ""
            };
          })
      : [];
    return {
      name: typeof areadata.name === "string" ? areadata.name : "",
      version: parseVnum(areadata.version) ?? 1,
      vnumRangeStart,
      vnumRangeEnd,
      builders: typeof areadata.builders === "string" ? areadata.builders : "",
      credits: typeof areadata.credits === "string" ? areadata.credits : "",
      lootTable:
        typeof areadata.lootTable === "string" ? areadata.lootTable : "",
      security: parseVnum(areadata.security) ?? undefined,
      sector: typeof areadata.sector === "string" ? areadata.sector : undefined,
      lowLevel: parseVnum(areadata.lowLevel) ?? undefined,
      highLevel: parseVnum(areadata.highLevel) ?? undefined,
      reset: parseVnum(areadata.reset) ?? undefined,
      alwaysReset:
        typeof areadata.alwaysReset === "boolean" ? areadata.alwaysReset : false,
      instType: areadata.instType === "multi" ? "multi" : "single",
      storyBeats,
      checklist
    };
  }, [areaData]);
  const roomFormDefaults = useMemo<RoomFormValues>(() => {
    const record = selectedRoomRecord;
    const vnum =
      selectedRoomVnum ?? parseVnum(record?.vnum) ?? 0;
    const sectorType =
      typeof record?.sectorType === "string"
        ? record.sectorType
        : typeof record?.sector === "string"
          ? record.sector
          : undefined;
    const roomFlags = Array.isArray(record?.roomFlags)
      ? record.roomFlags.filter((flag) => typeof flag === "string")
      : [];
    return {
      vnum,
      name: typeof record?.name === "string" ? record.name : "",
      description:
        typeof record?.description === "string"
          ? normalizeLineEndingsForDisplay(record.description)
          : "",
      sectorType,
      roomFlags,
      exits: Array.isArray(record?.exits)
        ? record.exits
            .filter((exit): exit is Record<string, unknown> =>
              Boolean(exit && typeof exit === "object")
            )
            .map((exit) => ({
              dir:
                typeof exit.dir === "string"
                  ? exit.dir
                  : directions[0],
              toVnum: parseVnum(exit.toVnum) ?? 0,
              key: parseVnum(exit.key) ?? undefined,
              flags: Array.isArray(exit.flags)
                ? exit.flags.filter((flag) => typeof flag === "string")
                : [],
              description:
                typeof exit.description === "string"
                  ? normalizeLineEndingsForDisplay(exit.description)
                  : "",
              keyword: typeof exit.keyword === "string" ? exit.keyword : ""
            }))
        : []
    };
  }, [selectedRoomRecord, selectedRoomVnum]);
  const mobileFormDefaults = useMemo<MobileFormValues>(() => {
    const record = selectedMobileRecord;
    const vnum = selectedMobileVnum ?? parseVnum(record?.vnum) ?? 0;
    const resolveDice = (value: unknown) => {
      if (!value || typeof value !== "object") {
        return { number: undefined, type: undefined, bonus: undefined };
      }
      const dice = value as Record<string, unknown>;
      return {
        number: parseVnum(dice.number) ?? undefined,
        type: parseVnum(dice.type) ?? undefined,
        bonus: parseVnum(dice.bonus) ?? undefined
      };
    };
    return {
      vnum,
      name: typeof record?.name === "string" ? record.name : "",
      shortDescr:
        typeof record?.shortDescr === "string" ? record.shortDescr : "",
      longDescr:
        typeof record?.longDescr === "string"
          ? normalizeLineEndingsForDisplay(record.longDescr)
          : "",
      description:
        typeof record?.description === "string"
          ? normalizeLineEndingsForDisplay(record.description)
          : "",
      race: typeof record?.race === "string" ? record.race : "",
      level: parseVnum(record?.level) ?? undefined,
      hitroll: parseVnum(record?.hitroll) ?? undefined,
      alignment: parseVnum(record?.alignment) ?? undefined,
      damType: typeof record?.damType === "string" ? record.damType : undefined,
      startPos:
        typeof record?.startPos === "string" ? record.startPos : undefined,
      defaultPos:
        typeof record?.defaultPos === "string" ? record.defaultPos : undefined,
      sex: typeof record?.sex === "string" ? record.sex : undefined,
      size: typeof record?.size === "string" ? record.size : undefined,
      material: typeof record?.material === "string" ? record.material : "",
      factionVnum: parseVnum(record?.factionVnum) ?? undefined,
      damageNoun:
        typeof record?.damageNoun === "string" ? record.damageNoun : "",
      offensiveSpell:
        typeof record?.offensiveSpell === "string"
          ? record.offensiveSpell
          : "",
      lootTable:
        typeof record?.lootTable === "string" ? record.lootTable : "",
      hitDice: resolveDice(record?.hitDice),
      manaDice: resolveDice(record?.manaDice),
      damageDice: resolveDice(record?.damageDice)
    };
  }, [selectedMobileRecord, selectedMobileVnum]);
  const objectFormDefaults = useMemo<ObjectFormValues>(() => {
    const record = selectedObjectRecord;
    const vnum = selectedObjectVnum ?? parseVnum(record?.vnum) ?? 0;
    const resolveWeapon = (value: unknown) => {
      if (!value || typeof value !== "object") {
        return {
          class: undefined,
          diceNumber: undefined,
          diceType: undefined,
          damageType: undefined,
          flags: []
        };
      }
      const weapon = value as Record<string, unknown>;
      const dice = Array.isArray(weapon.dice) ? weapon.dice : [];
      return {
        class: typeof weapon.class === "string" ? weapon.class : undefined,
        diceNumber: parseVnum(dice[0]) ?? undefined,
        diceType: parseVnum(dice[1]) ?? undefined,
        damageType:
          typeof weapon.damageType === "string" ? weapon.damageType : undefined,
        flags: Array.isArray(weapon.flags)
          ? weapon.flags.filter((flag) => typeof flag === "string")
          : []
      };
    };
    const resolveArmor = (value: unknown) => {
      if (!value || typeof value !== "object") {
        return {
          acPierce: undefined,
          acBash: undefined,
          acSlash: undefined,
          acExotic: undefined
        };
      }
      const armor = value as Record<string, unknown>;
      return {
        acPierce: parseVnum(armor.acPierce) ?? undefined,
        acBash: parseVnum(armor.acBash) ?? undefined,
        acSlash: parseVnum(armor.acSlash) ?? undefined,
        acExotic: parseVnum(armor.acExotic) ?? undefined
      };
    };
    const resolveContainer = (value: unknown) => {
      if (!value || typeof value !== "object") {
        return {
          capacity: undefined,
          flags: [],
          keyVnum: undefined,
          maxWeight: undefined,
          weightMult: undefined
        };
      }
      const container = value as Record<string, unknown>;
      return {
        capacity: parseVnum(container.capacity) ?? undefined,
        flags: Array.isArray(container.flags)
          ? container.flags.filter((flag) => typeof flag === "string")
          : [],
        keyVnum: parseVnum(container.keyVnum) ?? undefined,
        maxWeight: parseVnum(container.maxWeight) ?? undefined,
        weightMult: parseVnum(container.weightMult) ?? undefined
      };
    };
    const resolveLight = (value: unknown) => {
      if (!value || typeof value !== "object") {
        return { hours: undefined };
      }
      const light = value as Record<string, unknown>;
      return { hours: parseVnum(light.hours) ?? undefined };
    };
    const resolveDrink = (value: unknown) => {
      if (!value || typeof value !== "object") {
        return {
          capacity: undefined,
          remaining: undefined,
          liquid: undefined,
          poisoned: undefined
        };
      }
      const drink = value as Record<string, unknown>;
      return {
        capacity: parseVnum(drink.capacity) ?? undefined,
        remaining: parseVnum(drink.remaining) ?? undefined,
        liquid: typeof drink.liquid === "string" ? drink.liquid : undefined,
        poisoned: typeof drink.poisoned === "boolean" ? drink.poisoned : undefined
      };
    };
    const resolveFood = (value: unknown) => {
      if (!value || typeof value !== "object") {
        return { foodHours: undefined, fullHours: undefined, poisoned: undefined };
      }
      const food = value as Record<string, unknown>;
      return {
        foodHours: parseVnum(food.foodHours) ?? undefined,
        fullHours: parseVnum(food.fullHours) ?? undefined,
        poisoned: typeof food.poisoned === "boolean" ? food.poisoned : undefined
      };
    };
    const resolveMoney = (value: unknown) => {
      if (!value || typeof value !== "object") {
        return { gold: undefined, silver: undefined };
      }
      const money = value as Record<string, unknown>;
      return {
        gold: parseVnum(money.gold) ?? undefined,
        silver: parseVnum(money.silver) ?? undefined
      };
    };
    const resolveWand = (value: unknown) => {
      if (!value || typeof value !== "object") {
        return {
          level: undefined,
          totalCharges: undefined,
          chargesLeft: undefined,
          spell: ""
        };
      }
      const wand = value as Record<string, unknown>;
      return {
        level: parseVnum(wand.level) ?? undefined,
        totalCharges: parseVnum(wand.totalCharges) ?? undefined,
        chargesLeft: parseVnum(wand.chargesLeft) ?? undefined,
        spell: typeof wand.spell === "string" ? wand.spell : ""
      };
    };
    const resolveSpells = (value: unknown) => {
      if (!value || typeof value !== "object") {
        return { level: undefined, spell1: "", spell2: "", spell3: "", spell4: "" };
      }
      const spells = value as Record<string, unknown>;
      return {
        level: parseVnum(spells.level) ?? undefined,
        spell1: typeof spells.spell1 === "string" ? spells.spell1 : "",
        spell2: typeof spells.spell2 === "string" ? spells.spell2 : "",
        spell3: typeof spells.spell3 === "string" ? spells.spell3 : "",
        spell4: typeof spells.spell4 === "string" ? spells.spell4 : ""
      };
    };
    const resolvePortal = (value: unknown) => {
      if (!value || typeof value !== "object") {
        return { charges: undefined, exitFlags: [], portalFlags: [], toVnum: undefined };
      }
      const portal = value as Record<string, unknown>;
      return {
        charges: parseVnum(portal.charges) ?? undefined,
        exitFlags: Array.isArray(portal.exitFlags)
          ? portal.exitFlags.filter((flag) => typeof flag === "string")
          : [],
        portalFlags: Array.isArray(portal.portalFlags)
          ? portal.portalFlags.filter((flag) => typeof flag === "string")
          : [],
        toVnum: parseVnum(portal.toVnum) ?? undefined
      };
    };
    const resolveFurniture = (value: unknown) => {
      if (!value || typeof value !== "object") {
        return {
          slots: undefined,
          weight: undefined,
          flags: [],
          healBonus: undefined,
          manaBonus: undefined,
          maxPeople: undefined
        };
      }
      const furniture = value as Record<string, unknown>;
      return {
        slots: parseVnum(furniture.slots) ?? undefined,
        weight: parseVnum(furniture.weight) ?? undefined,
        flags: Array.isArray(furniture.flags)
          ? furniture.flags.filter((flag) => typeof flag === "string")
          : [],
        healBonus: parseVnum(furniture.healBonus) ?? undefined,
        manaBonus: parseVnum(furniture.manaBonus) ?? undefined,
        maxPeople: parseVnum(furniture.maxPeople) ?? undefined
      };
    };
    return {
      vnum,
      name: typeof record?.name === "string" ? record.name : "",
      shortDescr:
        typeof record?.shortDescr === "string" ? record.shortDescr : "",
      description:
        typeof record?.description === "string"
          ? normalizeLineEndingsForDisplay(record.description)
          : "",
      material: typeof record?.material === "string" ? record.material : "",
      itemType: typeof record?.itemType === "string" ? record.itemType : "",
      extraFlags: Array.isArray(record?.extraFlags)
        ? record.extraFlags.filter((flag) => typeof flag === "string")
        : [],
      wearFlags: Array.isArray(record?.wearFlags)
        ? record.wearFlags.filter((flag) => typeof flag === "string")
        : [],
      level: parseVnum(record?.level) ?? undefined,
      weight: parseVnum(record?.weight) ?? undefined,
      cost: parseVnum(record?.cost) ?? undefined,
      condition: parseVnum(record?.condition) ?? undefined,
      weapon: resolveWeapon(record?.weapon),
      armor: resolveArmor(record?.armor),
      container: resolveContainer(record?.container),
      light: resolveLight(record?.light),
      drink: resolveDrink(record?.drink),
      fountain: resolveDrink(record?.fountain),
      food: resolveFood(record?.food),
      money: resolveMoney(record?.money),
      wand: resolveWand(record?.wand),
      staff: resolveWand(record?.staff),
      spells: resolveSpells(record?.spells),
      portal: resolvePortal(record?.portal),
      furniture: resolveFurniture(record?.furniture)
    };
  }, [selectedObjectRecord, selectedObjectVnum]);
  const resetFormDefaults = useMemo<ResetFormValues>(() => {
    const record = selectedResetRecord;
    const index = selectedResetIndex ?? 0;
    const commandName = resolveResetCommand(record);
    return {
      index,
      commandName: commandName || resetCommandOptions[0],
      mobVnum: parseVnum(record?.mobVnum) ?? undefined,
      objVnum: parseVnum(record?.objVnum) ?? undefined,
      roomVnum: parseVnum(record?.roomVnum) ?? undefined,
      containerVnum: parseVnum(record?.containerVnum) ?? undefined,
      maxInArea: parseVnum(record?.maxInArea) ?? undefined,
      maxInRoom: parseVnum(record?.maxInRoom) ?? undefined,
      count: parseVnum(record?.count) ?? undefined,
      maxInContainer: parseVnum(record?.maxInContainer) ?? undefined,
      wearLoc: typeof record?.wearLoc === "string" ? record.wearLoc : undefined,
      direction:
        typeof record?.direction === "string" ? record.direction : undefined,
      state: parseVnum(record?.state) ?? undefined,
      exits: parseVnum(record?.exits) ?? undefined
    };
  }, [selectedResetIndex, selectedResetRecord]);
  const shopFormDefaults = useMemo<ShopFormValues>(() => {
    const record = selectedShopRecord ?? {};
    const buyTypes = Array.isArray(record.buyTypes) ? record.buyTypes : [];
    return {
      keeper: parseVnum(record.keeper) ?? undefined,
      buyTypes: Array.from({ length: 5 }).map(
        (_, index) => parseVnum(buyTypes[index]) ?? 0
      ),
      profitBuy: parseVnum(record.profitBuy) ?? undefined,
      profitSell: parseVnum(record.profitSell) ?? undefined,
      openHour: parseVnum(record.openHour) ?? undefined,
      closeHour: parseVnum(record.closeHour) ?? undefined
    };
  }, [selectedShopRecord]);
  const questFormDefaults = useMemo<QuestFormValues>(() => {
    const record = selectedQuestRecord ?? {};
    const rewardObjs = Array.isArray(record.rewardObjs) ? record.rewardObjs : [];
    const rewardCounts = Array.isArray(record.rewardCounts)
      ? record.rewardCounts
      : [];
    return {
      vnum: parseVnum(record.vnum) ?? 0,
      name: getFirstString(record.name, ""),
      entry:
        typeof record.entry === "string"
          ? normalizeLineEndingsForDisplay(record.entry)
          : "",
      type: getFirstString(record.type, ""),
      xp: parseVnum(record.xp) ?? undefined,
      level: parseVnum(record.level) ?? undefined,
      end: parseVnum(record.end) ?? undefined,
      target: parseVnum(record.target) ?? undefined,
      upper: parseVnum(record.upper) ?? undefined,
      count: parseVnum(record.count) ?? undefined,
      rewardFaction: parseVnum(record.rewardFaction) ?? undefined,
      rewardReputation: parseVnum(record.rewardReputation) ?? undefined,
      rewardGold: parseVnum(record.rewardGold) ?? undefined,
      rewardSilver: parseVnum(record.rewardSilver) ?? undefined,
      rewardCopper: parseVnum(record.rewardCopper) ?? undefined,
      rewardObjs: Array.from({ length: 3 }).map(
        (_, index) => parseVnum(rewardObjs[index]) ?? 0
      ),
      rewardCounts: Array.from({ length: 3 }).map(
        (_, index) => parseVnum(rewardCounts[index]) ?? 0
      )
    };
  }, [selectedQuestRecord]);
  const factionFormDefaults = useMemo<FactionFormValues>(() => {
    const record = selectedFactionRecord ?? {};
    return {
      vnum: parseVnum(record.vnum) ?? 0,
      name: getFirstString(record.name, ""),
      defaultStanding: parseVnum(record.defaultStanding) ?? undefined,
      alliesCsv: formatNumberList(record.allies),
      opposingCsv: formatNumberList(record.opposing)
    };
  }, [selectedFactionRecord]);
  const recipeFormDefaults = useMemo<RecipeFormValues>(() => {
    const record = selectedRecipeRecord ?? {};
    const vnum = selectedRecipeVnum ?? parseVnum(record.vnum) ?? 0;
    const stationType = Array.isArray(record.stationType)
      ? record.stationType.filter((value): value is string => typeof value === "string")
      : [];
    const inputs = Array.isArray(record.inputs) ? record.inputs : [];
    return {
      vnum,
      name: getFirstString(record.name, ""),
      skill: getFirstString(record.skill, ""),
      minSkill: parseVnum(record.minSkill) ?? undefined,
      minSkillPct: parseVnum(record.minSkillPct) ?? undefined,
      minLevel: parseVnum(record.minLevel) ?? undefined,
      stationType,
      stationVnum: parseVnum(record.stationVnum) ?? undefined,
      discovery:
        typeof record.discovery === "string" ? record.discovery : undefined,
      inputs: inputs
        .filter(
          (input): input is Record<string, unknown> =>
            Boolean(input && typeof input === "object")
        )
        .map((input) => ({
          vnum: parseVnum(input.vnum) ?? 0,
          quantity: parseVnum(input.quantity) ?? 1
        })),
      outputVnum: parseVnum(record.outputVnum) ?? undefined,
      outputQuantity: parseVnum(record.outputQuantity) ?? undefined
    };
  }, [selectedRecipeRecord, selectedRecipeVnum]);
  const gatherSpawnFormDefaults = useMemo<GatherSpawnFormValues>(() => {
    const record = selectedGatherRecord ?? {};
    const vnum = selectedGatherVnum ?? parseVnum(record.vnum) ?? undefined;
    return {
      spawnSector:
        typeof record.spawnSector === "string"
          ? record.spawnSector
          : undefined,
      vnum,
      quantity: parseVnum(record.quantity) ?? undefined,
      respawnTimer: parseVnum(record.respawnTimer) ?? undefined
    };
  }, [selectedGatherRecord, selectedGatherVnum]);
  const activeResetCommand = (watchedResetCommand ??
    resetFormDefaults.commandName ??
    "").trim();
  const roomVnumOptions = useMemo(
    () => buildVnumOptions(areaData, "Rooms"),
    [areaData]
  );
  const mobileVnumOptions = useMemo(
    () => buildVnumOptions(areaData, "Mobiles"),
    [areaData]
  );
  const objectVnumOptions = useMemo(
    () => buildVnumOptions(areaData, "Objects"),
    [areaData]
  );
  const lootTableOptions = useMemo(() => {
    if (!areaData) {
      return [];
    }
    const loot = (areaData as Record<string, unknown>).loot;
    if (!loot || typeof loot !== "object") {
      return [];
    }
    const tables = Array.isArray((loot as Record<string, unknown>).tables)
      ? ((loot as Record<string, unknown>).tables as unknown[])
      : [];
    return tables
      .map((table) => {
        if (!table || typeof table !== "object") {
          return "";
        }
        const name = (table as Record<string, unknown>).name;
        return typeof name === "string" ? name.trim() : "";
      })
      .filter((name) => name.length);
  }, [areaData]);
  const activeObjectBlock = useMemo(() => {
    const key = (
      watchedObjectItemType ??
      objectFormDefaults.itemType ??
      ""
    )
      .trim()
      .toLowerCase();
    return itemTypeBlockMap[key] ?? null;
  }, [objectFormDefaults.itemType, watchedObjectItemType]);
  const classColumns = useMemo<ColDef<ClassRow>[]>(
    () => [
      { headerName: "#", field: "index", width: 80, sort: "asc" },
      { headerName: "Name", field: "name", flex: 2, minWidth: 200 },
      { headerName: "Who", field: "whoName", width: 110 },
      { headerName: "Prime", field: "primeStat", width: 100 },
      { headerName: "Armor", field: "armorProf", width: 120 },
      { headerName: "Weapon", field: "weaponVnum", width: 120 },
      { headerName: "Start", field: "startLoc", width: 110 }
    ],
    []
  );
  const raceColumns = useMemo<ColDef<RaceRow>[]>(
    () => [
      { headerName: "#", field: "index", width: 80, sort: "asc" },
      { headerName: "Name", field: "name", flex: 2, minWidth: 200 },
      { headerName: "Who", field: "whoName", width: 120 },
      { headerName: "PC", field: "pc", width: 80 },
      { headerName: "Points", field: "points", width: 110 },
      { headerName: "Size", field: "size", width: 110 },
      { headerName: "Start", field: "startLoc", width: 110 }
    ],
    []
  );
  const skillColumns = useMemo<ColDef<SkillRow>[]>(
    () => [
      { headerName: "#", field: "index", width: 80, sort: "asc" },
      { headerName: "Name", field: "name", flex: 2, minWidth: 220 },
      { headerName: "Target", field: "target", width: 150 },
      { headerName: "Position", field: "minPosition", width: 140 },
      { headerName: "Spell", field: "spell", flex: 1, minWidth: 160 },
      { headerName: "Slot", field: "slot", width: 110 }
    ],
    []
  );
  const groupColumns = useMemo<ColDef<GroupRow>[]>(
    () => [
      { headerName: "#", field: "index", width: 80, sort: "asc" },
      { headerName: "Name", field: "name", flex: 2, minWidth: 220 },
      { headerName: "Ratings", field: "ratingSummary", width: 140 },
      { headerName: "Skills", field: "skills", width: 110 }
    ],
    []
  );
  const commandColumns = useMemo<ColDef<CommandRow>[]>(
    () => [
      { headerName: "#", field: "index", width: 80, sort: "asc" },
      { headerName: "Name", field: "name", flex: 1, minWidth: 180 },
      { headerName: "Function", field: "function", flex: 1, minWidth: 180 },
      { headerName: "Position", field: "position", width: 140 },
      { headerName: "Level", field: "level", width: 110 },
      { headerName: "Log", field: "log", width: 130 },
      { headerName: "Category", field: "category", width: 140 },
      { headerName: "Lox", field: "loxFunction", flex: 1, minWidth: 160 }
    ],
    []
  );
  const socialColumns = useMemo<ColDef<SocialRow>[]>(
    () => [
      { headerName: "#", field: "index", width: 80, sort: "asc" },
      { headerName: "Name", field: "name", flex: 2, minWidth: 220 },
      { headerName: "No Target", field: "noTarget", width: 120 },
      { headerName: "Targeted", field: "target", width: 120 },
      { headerName: "Self", field: "self", width: 100 }
    ],
    []
  );
  const tutorialColumns = useMemo<ColDef<TutorialRow>[]>(
    () => [
      { headerName: "#", field: "index", width: 80, sort: "asc" },
      { headerName: "Name", field: "name", flex: 2, minWidth: 220 },
      { headerName: "Min Level", field: "minLevel", width: 120 },
      { headerName: "Steps", field: "steps", width: 110 }
    ],
    []
  );
  const lootColumns = useMemo<ColDef<LootRow>[]>(
    () => [
      { headerName: "Type", field: "kind", width: 110, sort: "asc" },
      { headerName: "Name", field: "name", flex: 2, minWidth: 220 },
      { headerName: "Details", field: "details", flex: 2, minWidth: 240 }
    ],
    []
  );
  const roomColumns = useMemo<ColDef<RoomRow>[]>(
    () => [
      { headerName: "VNUM", field: "vnum", width: 110, sort: "asc" },
      { headerName: "Name", field: "name", flex: 2, minWidth: 200 },
      { headerName: "Sector", field: "sector", flex: 1, minWidth: 140 },
      { headerName: "Exits", field: "exits", width: 110 },
      { headerName: "Room Flags", field: "flags", flex: 2, minWidth: 220 }
    ],
    []
  );
  const mobileColumns = useMemo<ColDef<MobileRow>[]>(
    () => [
      { headerName: "VNUM", field: "vnum", width: 110, sort: "asc" },
      { headerName: "Name", field: "name", flex: 2, minWidth: 200 },
      { headerName: "Level", field: "level", width: 110 },
      { headerName: "Race", field: "race", flex: 1, minWidth: 140 },
      { headerName: "Alignment", field: "alignment", width: 130 }
    ],
    []
  );
  const objectColumns = useMemo<ColDef<ObjectRow>[]>(
    () => [
      { headerName: "VNUM", field: "vnum", width: 110, sort: "asc" },
      { headerName: "Name", field: "name", flex: 2, minWidth: 200 },
      { headerName: "Item Type", field: "itemType", flex: 1, minWidth: 160 },
      { headerName: "Level", field: "level", width: 110 }
    ],
    []
  );
  const resetColumns = useMemo<ColDef<ResetRow>[]>(
    () => [
      { headerName: "#", field: "index", width: 80, sort: "asc" },
      { headerName: "Type", field: "command", flex: 1, minWidth: 160 },
      { headerName: "Entity VNUM", field: "entityVnum", width: 140 },
      { headerName: "Room VNUM", field: "roomVnum", width: 140 },
      { headerName: "Details", field: "details", flex: 2, minWidth: 220 }
    ],
    []
  );
  const shopColumns = useMemo<ColDef<ShopRow>[]>(
    () => [
      { headerName: "Keeper", field: "keeper", width: 120, sort: "asc" },
      { headerName: "Buy Types", field: "buyTypes", flex: 2, minWidth: 220 },
      { headerName: "Profit Buy", field: "profitBuy", width: 120 },
      { headerName: "Profit Sell", field: "profitSell", width: 120 },
      { headerName: "Hours", field: "hours", width: 120 }
    ],
    []
  );
  const questColumns = useMemo<ColDef<QuestRow>[]>(
    () => [
      { headerName: "VNUM", field: "vnum", width: 110, sort: "asc" },
      { headerName: "Name", field: "name", flex: 2, minWidth: 200 },
      { headerName: "Type", field: "type", flex: 1, minWidth: 140 },
      { headerName: "Level", field: "level", width: 110 },
      { headerName: "Target", field: "target", flex: 1, minWidth: 160 },
      { headerName: "Rewards", field: "rewards", flex: 1, minWidth: 160 }
    ],
    []
  );
  const factionColumns = useMemo<ColDef<FactionRow>[]>(
    () => [
      { headerName: "VNUM", field: "vnum", width: 110, sort: "asc" },
      { headerName: "Name", field: "name", flex: 2, minWidth: 200 },
      { headerName: "Default", field: "defaultStanding", width: 120 },
      { headerName: "Allies", field: "allies", width: 110 },
      { headerName: "Opposing", field: "opposing", width: 120 }
    ],
    []
  );
  const recipeColumns = useMemo<ColDef<RecipeRow>[]>(
    () => [
      { headerName: "VNUM", field: "vnum", width: 110, sort: "asc" },
      { headerName: "Name", field: "name", flex: 2, minWidth: 200 },
      { headerName: "Skill", field: "skill", width: 160 },
      { headerName: "Inputs", field: "inputs", width: 130 },
      { headerName: "Output", field: "output", width: 140 }
    ],
    []
  );
  const gatherSpawnColumns = useMemo<ColDef<GatherSpawnRow>[]>(
    () => [
      { headerName: "#", field: "index", width: 80, sort: "asc" },
      { headerName: "VNUM", field: "vnum", width: 120 },
      { headerName: "Sector", field: "sector", width: 140 },
      { headerName: "Qty", field: "quantity", width: 110 },
      { headerName: "Respawn", field: "respawnTimer", width: 120 }
    ],
    []
  );
  const roomDefaultColDef = useMemo<ColDef>(
    () => ({
      sortable: true,
      resizable: true,
      filter: true
    }),
    []
  );
  const mobileDefaultColDef = roomDefaultColDef;
  const objectDefaultColDef = roomDefaultColDef;
  const classDefaultColDef = roomDefaultColDef;
  const raceDefaultColDef = roomDefaultColDef;
  const skillDefaultColDef = roomDefaultColDef;
  const groupDefaultColDef = roomDefaultColDef;
  const commandDefaultColDef = roomDefaultColDef;
  const socialDefaultColDef = roomDefaultColDef;
  const tutorialDefaultColDef = roomDefaultColDef;
  const lootDefaultColDef = roomDefaultColDef;
  const recipeDefaultColDef = roomDefaultColDef;
  const gatherSpawnDefaultColDef = roomDefaultColDef;

  useEffect(() => {
    areaForm.reset(areaFormDefaults);
  }, [areaForm, areaFormDefaults]);

  useEffect(() => {
    roomForm.reset(roomFormDefaults);
  }, [roomForm, roomFormDefaults]);

  useEffect(() => {
    mobileForm.reset(mobileFormDefaults);
  }, [mobileForm, mobileFormDefaults]);

  useEffect(() => {
    objectForm.reset(objectFormDefaults);
  }, [objectForm, objectFormDefaults]);

  useEffect(() => {
    resetForm.reset(resetFormDefaults);
  }, [resetForm, resetFormDefaults]);

  useEffect(() => {
    shopForm.reset(shopFormDefaults);
  }, [shopForm, shopFormDefaults]);

  useEffect(() => {
    questForm.reset(questFormDefaults);
  }, [questForm, questFormDefaults]);

  useEffect(() => {
    factionForm.reset(factionFormDefaults);
  }, [factionForm, factionFormDefaults]);

  useEffect(() => {
    recipeForm.reset(recipeFormDefaults);
  }, [recipeForm, recipeFormDefaults]);

  useEffect(() => {
    gatherSpawnForm.reset(gatherSpawnFormDefaults);
  }, [gatherSpawnForm, gatherSpawnFormDefaults]);

  useEffect(() => {
    if (activeTab !== "Table" || selectedEntity !== "Rooms") {
      return;
    }
    syncGridSelection(roomGridApi.current, selectedRoomVnum);
  }, [activeTab, selectedEntity, selectedRoomVnum, roomRows]);

  useEffect(() => {
    if (activeTab !== "Table" || selectedEntity !== "Mobiles") {
      return;
    }
    syncGridSelection(mobileGridApi.current, selectedMobileVnum);
  }, [activeTab, selectedEntity, selectedMobileVnum, mobileRows]);

  useEffect(() => {
    if (activeTab !== "Table" || selectedEntity !== "Objects") {
      return;
    }
    syncGridSelection(objectGridApi.current, selectedObjectVnum);
  }, [activeTab, selectedEntity, selectedObjectVnum, objectRows]);

  useEffect(() => {
    if (activeTab !== "Table" || selectedEntity !== "Resets") {
      return;
    }
    syncGridSelection(resetGridApi.current, selectedResetIndex);
  }, [activeTab, selectedEntity, selectedResetIndex, resetRows]);

  useEffect(() => {
    if (activeTab !== "Table" || selectedEntity !== "Shops") {
      return;
    }
    syncGridSelection(shopGridApi.current, selectedShopKeeper);
  }, [activeTab, selectedEntity, selectedShopKeeper, shopRows]);

  useEffect(() => {
    if (activeTab !== "Table" || selectedEntity !== "Quests") {
      return;
    }
    syncGridSelection(questGridApi.current, selectedQuestVnum);
  }, [activeTab, selectedEntity, selectedQuestVnum, questRows]);

  useEffect(() => {
    if (activeTab !== "Table" || selectedEntity !== "Factions") {
      return;
    }
    syncGridSelection(factionGridApi.current, selectedFactionVnum);
  }, [activeTab, selectedEntity, selectedFactionVnum, factionRows]);

  useEffect(() => {
    if (activeTab !== "Table" || selectedEntity !== "Loot") {
      return;
    }
    const rowId =
      selectedAreaLootKind && selectedAreaLootIndex !== null
        ? `${selectedAreaLootKind}:${selectedAreaLootIndex}`
        : null;
    syncGridSelection(areaLootGridApi.current, rowId);
  }, [
    activeTab,
    selectedEntity,
    selectedAreaLootKind,
    selectedAreaLootIndex,
    areaLootRows
  ]);

  useEffect(() => {
    if (activeTab !== "Table" || selectedEntity !== "Recipes") {
      return;
    }
    syncGridSelection(recipeGridApi.current, selectedRecipeVnum);
  }, [activeTab, selectedEntity, selectedRecipeVnum, recipeRows]);

  useEffect(() => {
    if (activeTab !== "Table" || selectedEntity !== "Gather Spawns") {
      return;
    }
    syncGridSelection(gatherGridApi.current, selectedGatherVnum);
  }, [activeTab, selectedEntity, selectedGatherVnum, gatherSpawnRows]);

  useEffect(() => {
    if (
      editorMode !== "Global" ||
      selectedGlobalEntity !== "Classes" ||
      activeTab !== "Table"
    ) {
      return;
    }
    syncGridSelection(classGridApi.current, selectedClassIndex);
  }, [
    editorMode,
    selectedGlobalEntity,
    activeTab,
    selectedClassIndex,
    classRows
  ]);

  useEffect(() => {
    if (
      editorMode !== "Global" ||
      selectedGlobalEntity !== "Races" ||
      activeTab !== "Table"
    ) {
      return;
    }
    syncGridSelection(raceGridApi.current, selectedRaceIndex);
  }, [
    editorMode,
    selectedGlobalEntity,
    activeTab,
    selectedRaceIndex,
    raceRows
  ]);

  useEffect(() => {
    if (
      editorMode !== "Global" ||
      selectedGlobalEntity !== "Skills" ||
      activeTab !== "Table"
    ) {
      return;
    }
    syncGridSelection(skillGridApi.current, selectedSkillIndex);
  }, [
    editorMode,
    selectedGlobalEntity,
    activeTab,
    selectedSkillIndex,
    skillRows
  ]);

  useEffect(() => {
    if (
      editorMode !== "Global" ||
      selectedGlobalEntity !== "Groups" ||
      activeTab !== "Table"
    ) {
      return;
    }
    syncGridSelection(groupGridApi.current, selectedGroupIndex);
  }, [
    editorMode,
    selectedGlobalEntity,
    activeTab,
    selectedGroupIndex,
    groupRows
  ]);

  useEffect(() => {
    if (
      editorMode !== "Global" ||
      selectedGlobalEntity !== "Commands" ||
      activeTab !== "Table"
    ) {
      return;
    }
    syncGridSelection(commandGridApi.current, selectedCommandIndex);
  }, [
    editorMode,
    selectedGlobalEntity,
    activeTab,
    selectedCommandIndex,
    commandRows
  ]);

  useEffect(() => {
    if (
      editorMode !== "Global" ||
      selectedGlobalEntity !== "Socials" ||
      activeTab !== "Table"
    ) {
      return;
    }
    syncGridSelection(socialGridApi.current, selectedSocialIndex);
  }, [
    editorMode,
    selectedGlobalEntity,
    activeTab,
    selectedSocialIndex,
    socialRows
  ]);

  useEffect(() => {
    if (
      editorMode !== "Global" ||
      selectedGlobalEntity !== "Tutorials" ||
      activeTab !== "Table"
    ) {
      return;
    }
    syncGridSelection(tutorialGridApi.current, selectedTutorialIndex);
  }, [
    editorMode,
    selectedGlobalEntity,
    activeTab,
    selectedTutorialIndex,
    tutorialRows
  ]);

  useEffect(() => {
    if (
      editorMode !== "Global" ||
      selectedGlobalEntity !== "Loot" ||
      activeTab !== "Table"
    ) {
      return;
    }
    const rowId =
      selectedLootKind && selectedLootIndex !== null
        ? `${selectedLootKind}:${selectedLootIndex}`
        : null;
    syncGridSelection(lootGridApi.current, rowId);
  }, [
    editorMode,
    selectedGlobalEntity,
    activeTab,
    selectedLootKind,
    selectedLootIndex,
    lootRows
  ]);

  useEffect(() => {
    if (activeTab === "Map" && selectedEntity !== "Rooms") {
      setSelectedEntity("Rooms");
    }
  }, [activeTab, selectedEntity]);

  useEffect(() => {
    if (!autoLayoutEnabled) {
      return;
    }
    const cancelRef = { current: false };
    runRoomLayout(cancelRef);
    return () => {
      cancelRef.current = true;
    };
  }, [autoLayoutEnabled, runRoomLayout]);

  useEffect(() => {
    if (layoutNonce === 0) {
      return;
    }
    const cancelRef = { current: false };
    runRoomLayout(cancelRef);
    return () => {
      cancelRef.current = true;
    };
  }, [layoutNonce, runRoomLayout]);

  const handleCreateRoom = useCallback(() => {
    if (!areaData) {
      setStatusMessage("Load an area before creating rooms.");
      return;
    }
    const nextVnum = getNextEntityVnum(areaData, "Rooms");
    if (nextVnum === null) {
      setStatusMessage("No available room VNUMs in the area range.");
      return;
    }
    const areadata = (areaData as Record<string, unknown>).areadata;
    const areaSector =
      areadata && typeof areadata === "object"
        ? (areadata as Record<string, unknown>).sector
        : null;
    const sectorType =
      typeof areaSector === "string" && areaSector.trim().length
        ? areaSector
        : "inside";
    const newRoom: Record<string, unknown> = {
      vnum: nextVnum,
      name: "New Room",
      description: "An unfinished room.\n\r",
      sectorType,
      roomFlags: [],
      exits: []
    };
    setAreaData((current) => {
      if (!current) {
        return current;
      }
      const rooms = Array.isArray((current as Record<string, unknown>).rooms)
        ? [...((current as Record<string, unknown>).rooms as unknown[])]
        : [];
      rooms.push(newRoom);
      return {
        ...current,
        rooms
      };
    });
    setSelectedEntity("Rooms");
    setSelectedRoomVnum(nextVnum);
    setActiveTab("Form");
    setStatusMessage(`Created room ${nextVnum} (unsaved)`);
  }, [areaData, setStatusMessage, setSelectedEntity, setSelectedRoomVnum, setActiveTab]);

  const handleCreateMobile = useCallback(() => {
    if (!areaData) {
      setStatusMessage("Load an area before creating mobiles.");
      return;
    }
    const nextVnum = getNextEntityVnum(areaData, "Mobiles");
    if (nextVnum === null) {
      setStatusMessage("No available mobile VNUMs in the area range.");
      return;
    }
    const defaultRace =
      referenceData?.races?.[0] ? referenceData.races[0] : "human";
    const newMobile: Record<string, unknown> = {
      vnum: nextVnum,
      name: "new mobile",
      shortDescr: "a new mobile",
      longDescr: "A new mobile is here.\n\r",
      description: "It looks unfinished.\n\r",
      race: defaultRace,
      level: 1,
      alignment: 0
    };
    setAreaData((current) => {
      if (!current) {
        return current;
      }
      const mobiles = Array.isArray((current as Record<string, unknown>).mobiles)
        ? [...((current as Record<string, unknown>).mobiles as unknown[])]
        : [];
      mobiles.push(newMobile);
      return {
        ...current,
        mobiles
      };
    });
    setSelectedEntity("Mobiles");
    setSelectedMobileVnum(nextVnum);
    setActiveTab("Form");
    setStatusMessage(`Created mobile ${nextVnum} (unsaved)`);
  }, [
    areaData,
    referenceData,
    setStatusMessage,
    setSelectedEntity,
    setSelectedMobileVnum,
    setActiveTab
  ]);

  const handleDeleteMobile = useCallback(() => {
    if (!areaData) {
      setStatusMessage("Load an area before deleting mobiles.");
      return;
    }
    if (selectedMobileVnum === null) {
      setStatusMessage("Select a mobile to delete.");
      return;
    }
    setAreaData((current) => {
      if (!current) {
        return current;
      }
      const mobiles = Array.isArray((current as Record<string, unknown>).mobiles)
        ? ((current as Record<string, unknown>).mobiles as unknown[])
        : [];
      const nextMobiles = mobiles.filter((mob) => {
        if (!mob || typeof mob !== "object") {
          return true;
        }
        const vnum = parseVnum((mob as Record<string, unknown>).vnum);
        return vnum !== selectedMobileVnum;
      });
      return {
        ...current,
        mobiles: nextMobiles
      };
    });
    setSelectedMobileVnum(null);
    setStatusMessage(`Deleted mobile ${selectedMobileVnum} (unsaved)`);
  }, [areaData, selectedMobileVnum, setStatusMessage]);

  const handleCreateObject = useCallback(() => {
    if (!areaData) {
      setStatusMessage("Load an area before creating objects.");
      return;
    }
    const nextVnum = getNextEntityVnum(areaData, "Objects");
    if (nextVnum === null) {
      setStatusMessage("No available object VNUMs in the area range.");
      return;
    }
    const newObject: Record<string, unknown> = {
      vnum: nextVnum,
      name: "new object",
      shortDescr: "a new object",
      description: "An unfinished object sits here.\n\r",
      material: "wood",
      itemType: "trash",
      level: 1,
      weight: 1,
      cost: 0,
      extraFlags: [],
      wearFlags: []
    };
    setAreaData((current) => {
      if (!current) {
        return current;
      }
      const objects = Array.isArray((current as Record<string, unknown>).objects)
        ? [...((current as Record<string, unknown>).objects as unknown[])]
        : [];
      objects.push(newObject);
      return {
        ...current,
        objects
      };
    });
    setSelectedEntity("Objects");
    setSelectedObjectVnum(nextVnum);
    setActiveTab("Form");
    setStatusMessage(`Created object ${nextVnum} (unsaved)`);
  }, [
    areaData,
    setStatusMessage,
    setSelectedEntity,
    setSelectedObjectVnum,
    setActiveTab
  ]);

  const handleDeleteObject = useCallback(() => {
    if (!areaData) {
      setStatusMessage("Load an area before deleting objects.");
      return;
    }
    if (selectedObjectVnum === null) {
      setStatusMessage("Select an object to delete.");
      return;
    }
    setAreaData((current) => {
      if (!current) {
        return current;
      }
      const objects = Array.isArray((current as Record<string, unknown>).objects)
        ? ((current as Record<string, unknown>).objects as unknown[])
        : [];
      const nextObjects = objects.filter((obj) => {
        if (!obj || typeof obj !== "object") {
          return true;
        }
        const vnum = parseVnum((obj as Record<string, unknown>).vnum);
        return vnum !== selectedObjectVnum;
      });
      return {
        ...current,
        objects: nextObjects
      };
    });
    setSelectedObjectVnum(null);
    setStatusMessage(`Deleted object ${selectedObjectVnum} (unsaved)`);
  }, [areaData, selectedObjectVnum, setStatusMessage]);

  const handleCreateShop = useCallback(() => {
    if (!areaData) {
      setStatusMessage("Load an area before creating shops.");
      return;
    }
    const mobiles = getEntityList(areaData, "Mobiles");
    const shops = getEntityList(areaData, "Shops");
    const usedKeepers = new Set<number>();
    shops.forEach((shop) => {
      if (!shop || typeof shop !== "object") {
        return;
      }
      const keeper = parseVnum((shop as Record<string, unknown>).keeper);
      if (keeper !== null) {
        usedKeepers.add(keeper);
      }
    });
    let keeper: number | null = null;
    for (const mob of mobiles) {
      if (!mob || typeof mob !== "object") {
        continue;
      }
      const vnum = parseVnum((mob as Record<string, unknown>).vnum);
      if (vnum !== null && !usedKeepers.has(vnum)) {
        keeper = vnum;
        break;
      }
    }
    if (keeper === null && mobiles.length) {
      const fallback = mobiles.find((mob) => mob && typeof mob === "object");
      keeper = fallback
        ? parseVnum((fallback as Record<string, unknown>).vnum)
        : null;
    }
    if (keeper === null) {
      keeper = 0;
    }
    const newShop: Record<string, unknown> = {
      keeper,
      buyTypes: [0, 0, 0, 0, 0],
      profitBuy: 100,
      profitSell: 100,
      openHour: 0,
      closeHour: 23
    };
    setAreaData((current) => {
      if (!current) {
        return current;
      }
      const nextShops = Array.isArray(
        (current as Record<string, unknown>).shops
      )
        ? [...((current as Record<string, unknown>).shops as unknown[])]
        : [];
      nextShops.push(newShop);
      return {
        ...current,
        shops: nextShops
      };
    });
    setSelectedEntity("Shops");
    setSelectedShopKeeper(keeper);
    setActiveTab("Form");
    setStatusMessage(`Created shop ${keeper} (unsaved)`);
  }, [
    areaData,
    setStatusMessage,
    setSelectedEntity,
    setSelectedShopKeeper,
    setActiveTab
  ]);

  const handleDeleteShop = useCallback(() => {
    if (!areaData) {
      setStatusMessage("Load an area before deleting shops.");
      return;
    }
    if (selectedShopKeeper === null) {
      setStatusMessage("Select a shop to delete.");
      return;
    }
    setAreaData((current) => {
      if (!current) {
        return current;
      }
      const shops = getEntityList(current, "Shops");
      if (!shops.length) {
        return current;
      }
      const nextShops = shops.filter((shop) => {
        if (!shop || typeof shop !== "object") {
          return true;
        }
        const keeper = parseVnum((shop as Record<string, unknown>).keeper);
        return keeper !== selectedShopKeeper;
      });
      return {
        ...(current as Record<string, unknown>),
        shops: nextShops
      };
    });
    setSelectedShopKeeper(null);
    setStatusMessage(`Deleted shop ${selectedShopKeeper} (unsaved)`);
  }, [areaData, selectedShopKeeper, setStatusMessage]);

  const handleCreateQuest = useCallback(() => {
    if (!areaData) {
      setStatusMessage("Load an area before creating quests.");
      return;
    }
    const nextVnum = getNextEntityVnum(areaData, "Quests");
    if (nextVnum === null) {
      setStatusMessage("No available quest VNUMs in the area range.");
      return;
    }
    const newQuest: Record<string, unknown> = {
      vnum: nextVnum,
      name: "New Quest",
      entry: "",
      type: "",
      xp: 0,
      level: 1,
      rewardObjs: [0, 0, 0],
      rewardCounts: [0, 0, 0]
    };
    setAreaData((current) => {
      if (!current) {
        return current;
      }
      const quests = Array.isArray((current as Record<string, unknown>).quests)
        ? [...((current as Record<string, unknown>).quests as unknown[])]
        : [];
      quests.push(newQuest);
      return {
        ...current,
        quests
      };
    });
    setSelectedEntity("Quests");
    setSelectedQuestVnum(nextVnum);
    setActiveTab("Form");
    setStatusMessage(`Created quest ${nextVnum} (unsaved)`);
  }, [
    areaData,
    setStatusMessage,
    setSelectedEntity,
    setSelectedQuestVnum,
    setActiveTab
  ]);

  const handleDeleteQuest = useCallback(() => {
    if (!areaData) {
      setStatusMessage("Load an area before deleting quests.");
      return;
    }
    if (selectedQuestVnum === null) {
      setStatusMessage("Select a quest to delete.");
      return;
    }
    setAreaData((current) => {
      if (!current) {
        return current;
      }
      const quests = Array.isArray((current as Record<string, unknown>).quests)
        ? ((current as Record<string, unknown>).quests as unknown[])
        : [];
      const nextQuests = quests.filter((quest) => {
        if (!quest || typeof quest !== "object") {
          return true;
        }
        const vnum = parseVnum((quest as Record<string, unknown>).vnum);
        return vnum !== selectedQuestVnum;
      });
      return {
        ...current,
        quests: nextQuests
      };
    });
    setSelectedQuestVnum(null);
    setStatusMessage(`Deleted quest ${selectedQuestVnum} (unsaved)`);
  }, [areaData, selectedQuestVnum, setStatusMessage]);

  const handleCreateFaction = useCallback(() => {
    if (!areaData) {
      setStatusMessage("Load an area before creating factions.");
      return;
    }
    const nextVnum = getNextEntityVnum(areaData, "Factions");
    if (nextVnum === null) {
      setStatusMessage("No available faction VNUMs in the area range.");
      return;
    }
    const newFaction: Record<string, unknown> = {
      vnum: nextVnum,
      name: "New Faction",
      defaultStanding: 0,
      allies: [],
      opposing: []
    };
    setAreaData((current) => {
      if (!current) {
        return current;
      }
      const factions = Array.isArray(
        (current as Record<string, unknown>).factions
      )
        ? [...((current as Record<string, unknown>).factions as unknown[])]
        : [];
      factions.push(newFaction);
      return {
        ...current,
        factions
      };
    });
    setSelectedEntity("Factions");
    setSelectedFactionVnum(nextVnum);
    setActiveTab("Form");
    setStatusMessage(`Created faction ${nextVnum} (unsaved)`);
  }, [
    areaData,
    setStatusMessage,
    setSelectedEntity,
    setSelectedFactionVnum,
    setActiveTab
  ]);

  const handleDeleteFaction = useCallback(() => {
    if (!areaData) {
      setStatusMessage("Load an area before deleting factions.");
      return;
    }
    if (selectedFactionVnum === null) {
      setStatusMessage("Select a faction to delete.");
      return;
    }
    setAreaData((current) => {
      if (!current) {
        return current;
      }
      const factions = Array.isArray(
        (current as Record<string, unknown>).factions
      )
        ? ((current as Record<string, unknown>).factions as unknown[])
        : [];
      const nextFactions = factions.filter((faction) => {
        if (!faction || typeof faction !== "object") {
          return true;
        }
        const vnum = parseVnum((faction as Record<string, unknown>).vnum);
        return vnum !== selectedFactionVnum;
      });
      return {
        ...current,
        factions: nextFactions
      };
    });
    setSelectedFactionVnum(null);
    setStatusMessage(`Deleted faction ${selectedFactionVnum} (unsaved)`);
  }, [areaData, selectedFactionVnum, setStatusMessage]);

  const handleCreateAreaLoot = useCallback((kind: "group" | "table") => {
    if (!areaData) {
      setStatusMessage("Load an area before creating loot.");
      return;
    }
    const currentLoot = extractAreaLootData(areaData);
    const nextGroups = currentLoot ? [...currentLoot.groups] : [];
    const nextTables = currentLoot ? [...currentLoot.tables] : [];
    let nextIndex = 0;
    if (kind === "group") {
      nextGroups.push({
        name: "New Loot Group",
        rolls: 1,
        entries: [],
        ops: []
      });
      nextIndex = nextGroups.length - 1;
    } else {
      nextTables.push({
        name: "New Loot Table",
        parent: "",
        ops: []
      });
      nextIndex = nextTables.length - 1;
    }
    setAreaData((current) => {
      if (!current) {
        return current;
      }
      return {
        ...(current as Record<string, unknown>),
        loot: {
          groups: nextGroups,
          tables: nextTables
        }
      };
    });
    setSelectedEntity("Loot");
    setSelectedAreaLootKind(kind);
    setSelectedAreaLootIndex(nextIndex);
    setActiveTab("Form");
    setStatusMessage(
      `Created loot ${kind} ${nextIndex} (unsaved)`
    );
  }, [
    areaData,
    setStatusMessage,
    setSelectedEntity,
    setSelectedAreaLootKind,
    setSelectedAreaLootIndex,
    setActiveTab
  ]);

  const handleDeleteAreaLoot = useCallback(() => {
    if (!areaData) {
      setStatusMessage("Load an area before deleting loot.");
      return;
    }
    if (selectedAreaLootKind === null || selectedAreaLootIndex === null) {
      setStatusMessage("Select a loot entry to delete.");
      return;
    }
    const currentLoot = extractAreaLootData(areaData);
    if (!currentLoot) {
      setStatusMessage("No loot data loaded.");
      return;
    }
    const nextGroups = [...currentLoot.groups];
    const nextTables = [...currentLoot.tables];
    if (selectedAreaLootKind === "group") {
      nextGroups.splice(selectedAreaLootIndex, 1);
    } else {
      nextTables.splice(selectedAreaLootIndex, 1);
    }
    setAreaData((current) => {
      if (!current) {
        return current;
      }
      return {
        ...(current as Record<string, unknown>),
        loot: {
          groups: nextGroups,
          tables: nextTables
        }
      };
    });
    setSelectedAreaLootKind(null);
    setSelectedAreaLootIndex(null);
    setStatusMessage(
      `Deleted loot ${selectedAreaLootKind} ${selectedAreaLootIndex} (unsaved)`
    );
  }, [
    areaData,
    selectedAreaLootKind,
    selectedAreaLootIndex,
    setStatusMessage
  ]);

  const handleCreateRecipe = useCallback(() => {
    if (!areaData) {
      setStatusMessage("Load an area before creating recipes.");
      return;
    }
    const nextVnum = getNextEntityVnum(areaData, "Recipes");
    if (nextVnum === null) {
      setStatusMessage("No available recipe VNUMs in the area range.");
      return;
    }
    const newRecipe: Record<string, unknown> = {
      vnum: nextVnum,
      name: "New Recipe",
      skill: "",
      minSkill: 0,
      minLevel: 1,
      stationType: [],
      inputs: [],
      outputs: []
    };
    setAreaData((current) => {
      if (!current) {
        return current;
      }
      const recipes = Array.isArray((current as Record<string, unknown>).recipes)
        ? [...((current as Record<string, unknown>).recipes as unknown[])]
        : [];
      recipes.push(newRecipe);
      return {
        ...current,
        recipes
      };
    });
    setSelectedEntity("Recipes");
    setSelectedRecipeVnum(nextVnum);
    setActiveTab("Form");
    setStatusMessage(`Created recipe ${nextVnum} (unsaved)`);
  }, [
    areaData,
    setStatusMessage,
    setSelectedEntity,
    setSelectedRecipeVnum,
    setActiveTab
  ]);

  const handleDeleteRecipe = useCallback(() => {
    if (!areaData) {
      setStatusMessage("Load an area before deleting recipes.");
      return;
    }
    if (selectedRecipeVnum === null) {
      setStatusMessage("Select a recipe to delete.");
      return;
    }
    setAreaData((current) => {
      if (!current) {
        return current;
      }
      const recipes = Array.isArray((current as Record<string, unknown>).recipes)
        ? ((current as Record<string, unknown>).recipes as unknown[])
        : [];
      const nextRecipes = recipes.filter((recipe) => {
        if (!recipe || typeof recipe !== "object") {
          return true;
        }
        const vnum = parseVnum((recipe as Record<string, unknown>).vnum);
        return vnum !== selectedRecipeVnum;
      });
      return {
        ...current,
        recipes: nextRecipes
      };
    });
    setSelectedRecipeVnum(null);
    setStatusMessage(`Deleted recipe ${selectedRecipeVnum} (unsaved)`);
  }, [areaData, selectedRecipeVnum, setStatusMessage]);

  const handleCreateGatherSpawn = useCallback(() => {
    if (!areaData) {
      setStatusMessage("Load an area before creating gather spawns.");
      return;
    }
    const nextVnum = getNextEntityVnum(areaData, "Gather Spawns");
    if (nextVnum === null) {
      setStatusMessage("No available gather spawn VNUMs in the area range.");
      return;
    }
    const newSpawn: Record<string, unknown> = {
      vnum: nextVnum,
      name: "New Gather Spawn",
      skill: "",
      roomVnum: getFirstEntityVnum(areaData, "Rooms") ?? 0,
      successTable: "",
      failureTable: "",
      minSkill: 0,
      minLevel: 1
    };
    setAreaData((current) => {
      if (!current) {
        return current;
      }
      const spawns = Array.isArray(
        (current as Record<string, unknown>).gatherSpawns
      )
        ? [...((current as Record<string, unknown>).gatherSpawns as unknown[])]
        : [];
      spawns.push(newSpawn);
      return {
        ...current,
        gatherSpawns: spawns
      };
    });
    setSelectedEntity("Gather Spawns");
    setSelectedGatherVnum(nextVnum);
    setActiveTab("Form");
    setStatusMessage(`Created gather spawn ${nextVnum} (unsaved)`);
  }, [
    areaData,
    setStatusMessage,
    setSelectedEntity,
    setSelectedGatherVnum,
    setActiveTab
  ]);

  const handleDeleteGatherSpawn = useCallback(() => {
    if (!areaData) {
      setStatusMessage("Load an area before deleting gather spawns.");
      return;
    }
    if (selectedGatherVnum === null) {
      setStatusMessage("Select a gather spawn to delete.");
      return;
    }
    setAreaData((current) => {
      if (!current) {
        return current;
      }
      const spawns = Array.isArray(
        (current as Record<string, unknown>).gatherSpawns
      )
        ? ((current as Record<string, unknown>).gatherSpawns as unknown[])
        : [];
      const nextSpawns = spawns.filter((spawn) => {
        if (!spawn || typeof spawn !== "object") {
          return true;
        }
        const vnum = parseVnum((spawn as Record<string, unknown>).vnum);
        return vnum !== selectedGatherVnum;
      });
      return {
        ...current,
        gatherSpawns: nextSpawns
      };
    });
    setSelectedGatherVnum(null);
    setStatusMessage(`Deleted gather spawn ${selectedGatherVnum} (unsaved)`);
  }, [areaData, selectedGatherVnum, setStatusMessage]);

  const handleCreateClass = useCallback(() => {
    if (!classData) {
      setStatusMessage("Load classes before creating classes.");
      return;
    }
    const nextIndex = classData.classes.length;
    const newRecord: ClassDefinition = {
      name: `New Class ${nextIndex + 1}`,
      whoName: "",
      baseGroup: "",
      defaultGroup: "",
      weaponVnum: 0,
      armorProf: "",
      guilds: new Array(classGuildCount).fill(0),
      primeStat: "",
      skillCap: 0,
      thac0_00: 0,
      thac0_32: 0,
      hpMin: 0,
      hpMax: 0,
      manaUser: false,
      startLoc: 0,
      titles: []
    };
    setClassData((current) => {
      if (!current) {
        return current;
      }
      return {
        ...current,
        classes: [...current.classes, newRecord]
      };
    });
    setSelectedGlobalEntity("Classes");
    setSelectedClassIndex(nextIndex);
    setActiveTab("Form");
    setStatusMessage(`Created class ${newRecord.name} (unsaved)`);
  }, [
    classData,
    setStatusMessage,
    setSelectedGlobalEntity,
    setSelectedClassIndex,
    setActiveTab
  ]);

  const handleDeleteClass = useCallback(() => {
    if (!classData) {
      setStatusMessage("Load classes before deleting classes.");
      return;
    }
    if (selectedClassIndex === null) {
      setStatusMessage("Select a class to delete.");
      return;
    }
    const deletedName =
      classData.classes[selectedClassIndex]?.name ?? "class";
    setClassData((current) => {
      if (!current) {
        return current;
      }
      const nextClasses = current.classes.filter(
        (_, index) => index !== selectedClassIndex
      );
      return {
        ...current,
        classes: nextClasses
      };
    });
    setSelectedClassIndex(null);
    setStatusMessage(`Deleted ${deletedName} (unsaved)`);
  }, [classData, selectedClassIndex, setStatusMessage]);

  const handleCreateRace = useCallback(() => {
    if (!raceData) {
      setStatusMessage("Load races before creating races.");
      return;
    }
    const nextIndex = raceData.races.length;
    const classNames = referenceData?.classes ?? [];
    const classMult = Object.fromEntries(
      classNames.map((name) => [name, 100])
    );
    const classStart = Object.fromEntries(
      classNames.map((name) => [name, 0])
    );
    const newRace: RaceDefinition = {
      name: `New Race ${nextIndex + 1}`,
      whoName: "",
      pc: false,
      points: 0,
      size: "",
      startLoc: 0,
      stats: {
        str: 0,
        int: 0,
        wis: 0,
        dex: 0,
        con: 0
      },
      maxStats: {
        str: 0,
        int: 0,
        wis: 0,
        dex: 0,
        con: 0
      },
      actFlags: [],
      affectFlags: [],
      offFlags: [],
      immFlags: [],
      resFlags: [],
      vulnFlags: [],
      formFlags: [],
      partFlags: [],
      classMult,
      classStart,
      skills: new Array(raceSkillCount).fill("")
    };
    setRaceData((current) => {
      if (!current) {
        return current;
      }
      return {
        ...current,
        races: [...current.races, newRace]
      };
    });
    setSelectedGlobalEntity("Races");
    setSelectedRaceIndex(nextIndex);
    setActiveTab("Form");
    setStatusMessage(`Created race ${newRace.name} (unsaved)`);
  }, [
    raceData,
    referenceData,
    setStatusMessage,
    setSelectedGlobalEntity,
    setSelectedRaceIndex,
    setActiveTab
  ]);

  const handleDeleteRace = useCallback(() => {
    if (!raceData) {
      setStatusMessage("Load races before deleting races.");
      return;
    }
    if (selectedRaceIndex === null) {
      setStatusMessage("Select a race to delete.");
      return;
    }
    const deletedName = raceData.races[selectedRaceIndex]?.name ?? "race";
    setRaceData((current) => {
      if (!current) {
        return current;
      }
      const nextRaces = current.races.filter(
        (_, index) => index !== selectedRaceIndex
      );
      return {
        ...current,
        races: nextRaces
      };
    });
    setSelectedRaceIndex(null);
    setStatusMessage(`Deleted ${deletedName} (unsaved)`);
  }, [raceData, selectedRaceIndex, setStatusMessage]);

  const handleCreateSkill = useCallback(() => {
    if (!skillData) {
      setStatusMessage("Load skills before creating skills.");
      return;
    }
    const nextIndex = skillData.skills.length;
    const classNames = referenceData?.classes ?? [];
    const levels = Object.fromEntries(
      classNames.map((name) => [name, defaultSkillLevel])
    );
    const ratings = Object.fromEntries(
      classNames.map((name) => [name, defaultSkillRating])
    );
    const newSkill: SkillDefinition = {
      name: `New Skill ${nextIndex + 1}`,
      levels,
      ratings,
      spell: "",
      loxSpell: "",
      target: "",
      minPosition: "",
      gsn: "",
      slot: 0,
      minMana: 0,
      beats: 0,
      nounDamage: "",
      msgOff: "",
      msgObj: ""
    };
    setSkillData((current) => {
      if (!current) {
        return current;
      }
      return {
        ...current,
        skills: [...current.skills, newSkill]
      };
    });
    setSelectedGlobalEntity("Skills");
    setSelectedSkillIndex(nextIndex);
    setActiveTab("Form");
    setStatusMessage(`Created skill ${newSkill.name} (unsaved)`);
  }, [
    skillData,
    referenceData,
    setStatusMessage,
    setSelectedGlobalEntity,
    setSelectedSkillIndex,
    setActiveTab
  ]);

  const handleDeleteSkill = useCallback(() => {
    if (!skillData) {
      setStatusMessage("Load skills before deleting skills.");
      return;
    }
    if (selectedSkillIndex === null) {
      setStatusMessage("Select a skill to delete.");
      return;
    }
    const deletedName = skillData.skills[selectedSkillIndex]?.name ?? "skill";
    setSkillData((current) => {
      if (!current) {
        return current;
      }
      const nextSkills = current.skills.filter(
        (_, index) => index !== selectedSkillIndex
      );
      return {
        ...current,
        skills: nextSkills
      };
    });
    setSelectedSkillIndex(null);
    setStatusMessage(`Deleted ${deletedName} (unsaved)`);
  }, [skillData, selectedSkillIndex, setStatusMessage]);

  const handleCreateReset = useCallback(() => {
    if (!areaData) {
      setStatusMessage("Load an area before creating resets.");
      return;
    }
    const roomVnum = getFirstEntityVnum(areaData, "Rooms") ?? 0;
    const mobVnum = getFirstEntityVnum(areaData, "Mobiles") ?? 0;
    const resets = getEntityList(areaData, "Resets");
    const nextIndex = resets.length;
    const newReset: Record<string, unknown> = {
      commandName: "loadMob",
      mobVnum,
      roomVnum,
      maxInArea: 1,
      maxInRoom: 1
    };
    setAreaData((current) => {
      if (!current) {
        return current;
      }
      const nextResets = Array.isArray(
        (current as Record<string, unknown>).resets
      )
        ? [...((current as Record<string, unknown>).resets as unknown[])]
        : [];
      nextResets.push(newReset);
      return {
        ...current,
        resets: nextResets
      };
    });
    setSelectedEntity("Resets");
    setSelectedResetIndex(nextIndex);
    setActiveTab("Form");
    setStatusMessage(`Created reset #${nextIndex} (unsaved)`);
  }, [
    areaData,
    setStatusMessage,
    setSelectedEntity,
    setSelectedResetIndex,
    setActiveTab
  ]);

  const handleDeleteReset = useCallback(() => {
    if (!areaData) {
      setStatusMessage("Load an area before deleting resets.");
      return;
    }
    if (selectedResetIndex === null) {
      setStatusMessage("Select a reset to delete.");
      return;
    }
    setAreaData((current) => {
      if (!current) {
        return current;
      }
      const resets = Array.isArray((current as Record<string, unknown>).resets)
        ? ((current as Record<string, unknown>).resets as unknown[])
        : [];
      const nextResets = resets.filter((_, index) => index !== selectedResetIndex);
      return {
        ...current,
        resets: nextResets
      };
    });
    setSelectedResetIndex(null);
    setStatusMessage(`Deleted reset #${selectedResetIndex} (unsaved)`);
  }, [areaData, selectedResetIndex, setStatusMessage]);

  const handleDeleteRoom = useCallback(() => {
    if (!areaData) {
      setStatusMessage("Load an area before deleting rooms.");
      return;
    }
    if (selectedRoomVnum === null) {
      setStatusMessage("Select a room to delete.");
      return;
    }
    const roomId = String(selectedRoomVnum);
    setAreaData((current) => {
      if (!current) {
        return current;
      }
      const rooms = Array.isArray((current as Record<string, unknown>).rooms)
        ? ((current as Record<string, unknown>).rooms as unknown[])
        : [];
      const nextRooms = rooms.filter((room) => {
        if (!room || typeof room !== "object") {
          return true;
        }
        const vnum = parseVnum((room as Record<string, unknown>).vnum);
        return vnum !== selectedRoomVnum;
      });
      return {
        ...current,
        rooms: nextRooms
      };
    });
    setRoomLayout((current) => {
      if (!current[roomId]) {
        return current;
      }
      const next = { ...current };
      delete next[roomId];
      return next;
    });
    setLayoutNodes((current) => current.filter((node) => node.id !== roomId));
    setDirtyRoomNodes((current) => {
      if (!current.has(roomId)) {
        return current;
      }
      const next = new Set(current);
      next.delete(roomId);
      return next;
    });
    setSelectedRoomVnum(null);
    setStatusMessage(`Deleted room ${selectedRoomVnum} (unsaved)`);
  }, [areaData, selectedRoomVnum, setStatusMessage]);

  const handleAreaSubmit = (data: AreaFormValues) => {
    if (!areaData) {
      return;
    }
    const nextStoryBeats = (data.storyBeats ?? [])
      .map((beat) => {
        const title = beat.title.trim();
        const description = normalizeOptionalText(beat.description);
        const nextBeat: Record<string, unknown> = {
          title
        };
        if (description) {
          nextBeat.description = normalizeLineEndingsForMud(description);
        }
        return nextBeat;
      })
      .filter((beat) => Boolean(beat.title));
    const nextChecklist = (data.checklist ?? [])
      .map((item) => {
        const title = item.title.trim();
        const description = normalizeOptionalText(item.description);
        const nextItem: Record<string, unknown> = {
          title,
          status: item.status
        };
        if (description) {
          nextItem.description = normalizeLineEndingsForMud(description);
        }
        return nextItem;
      })
      .filter((item) => Boolean(item.title));
    setAreaData((current) => {
      if (!current) {
        return current;
      }
      const record = current as Record<string, unknown>;
      const areadata =
        record.areadata && typeof record.areadata === "object"
          ? (record.areadata as Record<string, unknown>)
          : {};
      const nextAreadata: Record<string, unknown> = {
        ...areadata,
        version: data.version,
        name: data.name,
        vnumRange: [data.vnumRangeStart, data.vnumRangeEnd],
        builders: normalizeOptionalText(data.builders),
        credits: normalizeOptionalText(data.credits),
        lootTable: cleanOptionalString(data.lootTable),
        security: data.security ?? undefined,
        sector: data.sector ?? undefined,
        lowLevel: data.lowLevel ?? undefined,
        highLevel: data.highLevel ?? undefined,
        reset: data.reset ?? undefined,
        alwaysReset: data.alwaysReset ? true : undefined,
        instType: data.instType === "multi" ? "multi" : undefined
      };
      return {
        ...record,
        areadata: nextAreadata,
        storyBeats: nextStoryBeats.length ? nextStoryBeats : undefined,
        checklist: nextChecklist.length ? nextChecklist : undefined
      };
    });
    setStatusMessage("Updated area data (unsaved)");
  };

  const handleRoomSubmit = (data: RoomFormValues) => {
    if (!areaData || selectedRoomVnum === null) {
      return;
    }
    const normalizedExits = (data.exits ?? []).map((exit) => {
      const nextExit: Record<string, unknown> = {
        dir: exit.dir,
        toVnum: exit.toVnum
      };
      if (exit.key !== undefined) {
        nextExit.key = exit.key;
      }
      if (exit.flags && exit.flags.length) {
        nextExit.flags = exit.flags;
      }
      const description = normalizeOptionalMudText(exit.description);
      if (description) {
        nextExit.description = description;
      }
      if (exit.keyword && exit.keyword.trim().length) {
        nextExit.keyword = exit.keyword;
      }
      return nextExit;
    });
    setAreaData((current) => {
      if (!current) {
        return current;
      }
      const rooms = getEntityList(current, "Rooms");
      if (!rooms.length) {
        return current;
      }
      const nextRooms = rooms.map((room) => {
        const record = room as Record<string, unknown>;
        const roomVnum = parseVnum(record.vnum);
        if (roomVnum !== selectedRoomVnum) {
          return record;
        }
        const nextRoom: Record<string, unknown> = {
          ...record,
          name: data.name,
          description: normalizeLineEndingsForMud(data.description),
          sectorType: data.sectorType ?? undefined,
          roomFlags: data.roomFlags?.length ? data.roomFlags : undefined,
          manaRate: undefined,
          healRate: undefined,
          clan: undefined,
          owner: undefined,
          exits: normalizedExits.length ? normalizedExits : undefined
        };
        if (!("sectorType" in record) && "sector" in record) {
          nextRoom.sector = data.sectorType ?? undefined;
        }
        return nextRoom;
      });
      return {
        ...(current as Record<string, unknown>),
        rooms: nextRooms
      };
    });
    setStatusMessage(`Updated room ${data.vnum} (unsaved)`);
  };

  const handleMobileSubmit = (data: MobileFormValues) => {
    if (!areaData || selectedMobileVnum === null) {
      return;
    }
    const hitDice = normalizeDice(data.hitDice);
    const manaDice = normalizeDice(data.manaDice);
    const damageDice = normalizeDice(data.damageDice);
    setAreaData((current) => {
      if (!current) {
        return current;
      }
      const mobiles = getEntityList(current, "Mobiles");
      if (!mobiles.length) {
        return current;
      }
      const nextMobiles = mobiles.map((mob) => {
        const record = mob as Record<string, unknown>;
        const mobVnum = parseVnum(record.vnum);
        if (mobVnum !== selectedMobileVnum) {
          return record;
        }
        const nextMobile: Record<string, unknown> = {
          ...record,
          name: data.name,
          shortDescr: data.shortDescr,
          longDescr: normalizeLineEndingsForMud(data.longDescr),
          description: normalizeLineEndingsForMud(data.description),
          race: data.race,
          level: data.level ?? undefined,
          hitroll: data.hitroll ?? undefined,
          alignment: data.alignment ?? undefined,
          damType: data.damType ?? undefined,
          startPos: data.startPos ?? undefined,
          defaultPos: data.defaultPos ?? undefined,
          sex: data.sex ?? undefined,
          size: data.size ?? undefined,
          material: normalizeOptionalText(data.material),
          factionVnum: data.factionVnum ?? undefined,
          damageNoun: normalizeOptionalText(data.damageNoun),
          offensiveSpell: normalizeOptionalText(data.offensiveSpell),
          lootTable: cleanOptionalString(data.lootTable),
          hitDice,
          manaDice,
          damageDice
        };
        return nextMobile;
      });
      return {
        ...(current as Record<string, unknown>),
        mobiles: nextMobiles
      };
    });
    setStatusMessage(`Updated mobile ${data.vnum} (unsaved)`);
  };

  const handleObjectSubmit = (data: ObjectFormValues) => {
    if (!areaData || selectedObjectVnum === null) {
      return;
    }
    const itemTypeKey = data.itemType.trim().toLowerCase();
    const activeBlock = itemTypeBlockMap[itemTypeKey] ?? null;
    const buildWeapon = () => {
      if (!data.weapon) {
        return undefined;
      }
      const next: Record<string, unknown> = {};
      if (data.weapon.class) {
        next.class = data.weapon.class;
      }
      if (
        data.weapon.diceNumber !== undefined &&
        data.weapon.diceType !== undefined
      ) {
        next.dice = [data.weapon.diceNumber, data.weapon.diceType];
      }
      if (data.weapon.damageType) {
        next.damageType = data.weapon.damageType;
      }
      if (data.weapon.flags && data.weapon.flags.length) {
        next.flags = data.weapon.flags;
      }
      return Object.keys(next).length ? next : undefined;
    };
    const buildArmor = () => {
      if (!data.armor) {
        return undefined;
      }
      const next: Record<string, unknown> = {};
      if (data.armor.acPierce !== undefined) next.acPierce = data.armor.acPierce;
      if (data.armor.acBash !== undefined) next.acBash = data.armor.acBash;
      if (data.armor.acSlash !== undefined) next.acSlash = data.armor.acSlash;
      if (data.armor.acExotic !== undefined) next.acExotic = data.armor.acExotic;
      return Object.keys(next).length ? next : undefined;
    };
    const buildContainer = () => {
      if (!data.container) {
        return undefined;
      }
      const next: Record<string, unknown> = {};
      if (data.container.capacity !== undefined)
        next.capacity = data.container.capacity;
      if (data.container.flags && data.container.flags.length)
        next.flags = data.container.flags;
      if (data.container.keyVnum !== undefined)
        next.keyVnum = data.container.keyVnum;
      if (data.container.maxWeight !== undefined)
        next.maxWeight = data.container.maxWeight;
      if (data.container.weightMult !== undefined)
        next.weightMult = data.container.weightMult;
      return Object.keys(next).length ? next : undefined;
    };
    const buildLight = () => {
      if (!data.light) {
        return undefined;
      }
      const next: Record<string, unknown> = {};
      if (data.light.hours !== undefined) next.hours = data.light.hours;
      return Object.keys(next).length ? next : undefined;
    };
    const buildDrink = (block?: ObjectFormValues["drink"]) => {
      if (!block) {
        return undefined;
      }
      const next: Record<string, unknown> = {};
      if (block.capacity !== undefined) next.capacity = block.capacity;
      if (block.remaining !== undefined) next.remaining = block.remaining;
      if (block.liquid) next.liquid = block.liquid;
      if (block.poisoned) next.poisoned = true;
      return Object.keys(next).length ? next : undefined;
    };
    const buildFood = () => {
      if (!data.food) {
        return undefined;
      }
      const next: Record<string, unknown> = {};
      if (data.food.foodHours !== undefined)
        next.foodHours = data.food.foodHours;
      if (data.food.fullHours !== undefined)
        next.fullHours = data.food.fullHours;
      if (data.food.poisoned) next.poisoned = true;
      return Object.keys(next).length ? next : undefined;
    };
    const buildMoney = () => {
      if (!data.money) {
        return undefined;
      }
      const next: Record<string, unknown> = {};
      if (data.money.gold !== undefined) next.gold = data.money.gold;
      if (data.money.silver !== undefined) next.silver = data.money.silver;
      return Object.keys(next).length ? next : undefined;
    };
    const buildWand = (block?: ObjectFormValues["wand"]) => {
      if (!block) {
        return undefined;
      }
      const next: Record<string, unknown> = {};
      if (block.level !== undefined) next.level = block.level;
      if (block.totalCharges !== undefined)
        next.totalCharges = block.totalCharges;
      if (block.chargesLeft !== undefined)
        next.chargesLeft = block.chargesLeft;
      const spell = normalizeOptionalText(block.spell);
      if (spell) next.spell = spell;
      return Object.keys(next).length ? next : undefined;
    };
    const buildSpells = () => {
      if (!data.spells) {
        return undefined;
      }
      const next: Record<string, unknown> = {};
      if (data.spells.level !== undefined) next.level = data.spells.level;
      const spell1 = normalizeOptionalText(data.spells.spell1);
      const spell2 = normalizeOptionalText(data.spells.spell2);
      const spell3 = normalizeOptionalText(data.spells.spell3);
      const spell4 = normalizeOptionalText(data.spells.spell4);
      if (spell1) next.spell1 = spell1;
      if (spell2) next.spell2 = spell2;
      if (spell3) next.spell3 = spell3;
      if (spell4) next.spell4 = spell4;
      return Object.keys(next).length ? next : undefined;
    };
    const buildPortal = () => {
      if (!data.portal) {
        return undefined;
      }
      const next: Record<string, unknown> = {};
      if (data.portal.charges !== undefined) next.charges = data.portal.charges;
      if (data.portal.exitFlags && data.portal.exitFlags.length)
        next.exitFlags = data.portal.exitFlags;
      if (data.portal.portalFlags && data.portal.portalFlags.length)
        next.portalFlags = data.portal.portalFlags;
      if (data.portal.toVnum !== undefined) next.toVnum = data.portal.toVnum;
      return Object.keys(next).length ? next : undefined;
    };
    const buildFurniture = () => {
      if (!data.furniture) {
        return undefined;
      }
      const next: Record<string, unknown> = {};
      if (data.furniture.slots !== undefined) next.slots = data.furniture.slots;
      if (data.furniture.weight !== undefined)
        next.weight = data.furniture.weight;
      if (data.furniture.flags && data.furniture.flags.length)
        next.flags = data.furniture.flags;
      if (data.furniture.healBonus !== undefined)
        next.healBonus = data.furniture.healBonus;
      if (data.furniture.manaBonus !== undefined)
        next.manaBonus = data.furniture.manaBonus;
      if (data.furniture.maxPeople !== undefined)
        next.maxPeople = data.furniture.maxPeople;
      return Object.keys(next).length ? next : undefined;
    };

    setAreaData((current) => {
      if (!current) {
        return current;
      }
      const objects = getEntityList(current, "Objects");
      if (!objects.length) {
        return current;
      }
      const nextObjects = objects.map((obj) => {
        const record = obj as Record<string, unknown>;
        const objVnum = parseVnum(record.vnum);
        if (objVnum !== selectedObjectVnum) {
          return record;
        }
        const nextObject: Record<string, unknown> = {
          ...record,
          name: data.name,
          shortDescr: data.shortDescr,
          description: normalizeLineEndingsForMud(data.description),
          material: data.material,
          itemType: data.itemType,
          extraFlags: data.extraFlags?.length ? data.extraFlags : undefined,
          wearFlags: data.wearFlags?.length ? data.wearFlags : undefined,
          level: data.level ?? undefined,
          weight: data.weight ?? undefined,
          cost: data.cost ?? undefined,
          condition: data.condition ?? undefined,
          weapon: activeBlock === "weapon" ? buildWeapon() : undefined,
          armor: activeBlock === "armor" ? buildArmor() : undefined,
          container: activeBlock === "container" ? buildContainer() : undefined,
          light: activeBlock === "light" ? buildLight() : undefined,
          drink: activeBlock === "drink" ? buildDrink(data.drink) : undefined,
          fountain:
            activeBlock === "fountain" ? buildDrink(data.fountain) : undefined,
          food: activeBlock === "food" ? buildFood() : undefined,
          money: activeBlock === "money" ? buildMoney() : undefined,
          wand: activeBlock === "wand" ? buildWand(data.wand) : undefined,
          staff: activeBlock === "staff" ? buildWand(data.staff) : undefined,
          spells: activeBlock === "spells" ? buildSpells() : undefined,
          portal: activeBlock === "portal" ? buildPortal() : undefined,
          furniture: activeBlock === "furniture" ? buildFurniture() : undefined
        };
        return nextObject;
      });
      return {
        ...(current as Record<string, unknown>),
        objects: nextObjects
      };
    });
    setStatusMessage(`Updated object ${data.vnum} (unsaved)`);
  };

  const handleResetSubmit = (data: ResetFormValues) => {
    if (!areaData || selectedResetIndex === null) {
      return;
    }
    const commandName = data.commandName.trim();
    const isKnown = resetCommandOptions.includes(commandName as ResetCommand);
    setAreaData((current) => {
      if (!current) {
        return current;
      }
      const resets = getEntityList(current, "Resets");
      if (!resets.length) {
        return current;
      }
      const nextResets = resets.map((reset, index) => {
        if (index !== selectedResetIndex) {
          return reset as Record<string, unknown>;
        }
        const record = reset as Record<string, unknown>;
        if (!isKnown) {
          return {
            ...record,
            commandName,
            command: undefined
          };
        }
        const nextReset: Record<string, unknown> = {
          ...record,
          commandName,
          command: undefined,
          mobVnum: undefined,
          objVnum: undefined,
          roomVnum: undefined,
          containerVnum: undefined,
          maxInArea: undefined,
          maxInRoom: undefined,
          count: undefined,
          maxInContainer: undefined,
          wearLoc: undefined,
          direction: undefined,
          state: undefined,
          exits: undefined,
          arg1: undefined,
          arg2: undefined,
          arg3: undefined,
          arg4: undefined
        };
        if (commandName === "loadMob") {
          nextReset.mobVnum = data.mobVnum ?? undefined;
          nextReset.roomVnum = data.roomVnum ?? undefined;
          nextReset.maxInArea = data.maxInArea ?? undefined;
          nextReset.maxInRoom = data.maxInRoom ?? undefined;
        } else if (commandName === "placeObj") {
          nextReset.objVnum = data.objVnum ?? undefined;
          nextReset.roomVnum = data.roomVnum ?? undefined;
        } else if (commandName === "putObj") {
          nextReset.objVnum = data.objVnum ?? undefined;
          nextReset.containerVnum = data.containerVnum ?? undefined;
          nextReset.count = data.count ?? undefined;
          nextReset.maxInContainer = data.maxInContainer ?? undefined;
        } else if (commandName === "giveObj") {
          nextReset.objVnum = data.objVnum ?? undefined;
        } else if (commandName === "equipObj") {
          nextReset.objVnum = data.objVnum ?? undefined;
          nextReset.wearLoc = data.wearLoc ?? undefined;
        } else if (commandName === "setDoor") {
          nextReset.roomVnum = data.roomVnum ?? undefined;
          nextReset.direction = data.direction ?? undefined;
          nextReset.state = data.state ?? undefined;
        } else if (commandName === "randomizeExits") {
          nextReset.roomVnum = data.roomVnum ?? undefined;
          nextReset.exits = data.exits ?? undefined;
        }
        return nextReset;
      });
      return {
        ...(current as Record<string, unknown>),
        resets: nextResets
      };
    });
    setStatusMessage(`Updated reset #${data.index} (unsaved)`);
  };

  const handleShopSubmit = (data: ShopFormValues) => {
    if (!areaData || selectedShopKeeper === null) {
      return;
    }
    const keeper =
      data.keeper !== undefined ? data.keeper : selectedShopKeeper;
    const buyTypes = Array.from({ length: 5 }).map(
      (_, index) => data.buyTypes?.[index] ?? 0
    );
    setAreaData((current) => {
      if (!current) {
        return current;
      }
      const shops = getEntityList(current, "Shops");
      if (!shops.length) {
        return current;
      }
      const nextShops = shops.map((shop) => {
        const record = shop as Record<string, unknown>;
        const shopKeeper = parseVnum(record.keeper);
        if (shopKeeper !== selectedShopKeeper) {
          return record;
        }
        return {
          ...record,
          keeper,
          buyTypes,
          profitBuy: data.profitBuy ?? 0,
          profitSell: data.profitSell ?? 0,
          openHour: data.openHour ?? 0,
          closeHour: data.closeHour ?? 0
        };
      });
      return {
        ...(current as Record<string, unknown>),
        shops: nextShops
      };
    });
    setSelectedShopKeeper(keeper);
    setStatusMessage(`Updated shop ${keeper} (unsaved)`);
  };

  const handleQuestSubmit = (data: QuestFormValues) => {
    if (!areaData || selectedQuestVnum === null) {
      return;
    }
    const rewardObjs = Array.from({ length: 3 }).map(
      (_, index) => data.rewardObjs?.[index] ?? 0
    );
    const rewardCounts = Array.from({ length: 3 }).map(
      (_, index) => data.rewardCounts?.[index] ?? 0
    );
    setAreaData((current) => {
      if (!current) {
        return current;
      }
      const quests = getEntityList(current, "Quests");
      if (!quests.length) {
        return current;
      }
      const nextQuests = quests.map((quest) => {
        const record = quest as Record<string, unknown>;
        const vnum = parseVnum(record.vnum);
        if (vnum !== selectedQuestVnum) {
          return record;
        }
        return {
          ...record,
          vnum: data.vnum,
          name: data.name ?? "",
          entry: normalizeLineEndingsForMud(data.entry ?? ""),
          type: data.type ?? "",
          xp: data.xp ?? 0,
          level: data.level ?? 0,
          end: data.end ?? 0,
          target: data.target ?? 0,
          upper: data.upper ?? 0,
          count: data.count ?? 0,
          rewardFaction: data.rewardFaction ?? 0,
          rewardReputation: data.rewardReputation ?? 0,
          rewardGold: data.rewardGold ?? 0,
          rewardSilver: data.rewardSilver ?? 0,
          rewardCopper: data.rewardCopper ?? 0,
          rewardObjs,
          rewardCounts
        };
      });
      return {
        ...(current as Record<string, unknown>),
        quests: nextQuests
      };
    });
    setStatusMessage(`Updated quest ${data.vnum} (unsaved)`);
  };

  const handleFactionSubmit = (data: FactionFormValues) => {
    if (!areaData || selectedFactionVnum === null) {
      return;
    }
    const allies = parseNumberList(data.alliesCsv);
    const opposing = parseNumberList(data.opposingCsv);
    setAreaData((current) => {
      if (!current) {
        return current;
      }
      const factions = getEntityList(current, "Factions");
      if (!factions.length) {
        return current;
      }
      const nextFactions = factions.map((faction) => {
        const record = faction as Record<string, unknown>;
        const vnum = parseVnum(record.vnum);
        if (vnum !== selectedFactionVnum) {
          return record;
        }
        const nextFaction: Record<string, unknown> = {
          ...record,
          vnum: data.vnum,
          name: data.name ?? "",
          defaultStanding: data.defaultStanding ?? 0
        };
        if (allies.length) {
          nextFaction.allies = allies;
        } else {
          nextFaction.allies = undefined;
        }
        if (opposing.length) {
          nextFaction.opposing = opposing;
        } else {
          nextFaction.opposing = undefined;
        }
        return nextFaction;
      });
      return {
        ...(current as Record<string, unknown>),
        factions: nextFactions
      };
    });
    setStatusMessage(`Updated faction ${data.vnum} (unsaved)`);
  };

  const handleAreaLootSubmit = (data: LootFormValues) => {
    if (
      !areaData ||
      selectedAreaLootKind === null ||
      selectedAreaLootIndex === null
    ) {
      return;
    }
    const toInt = (value: number | undefined, fallback: number) => {
      if (typeof value !== "number" || !Number.isFinite(value)) {
        return fallback;
      }
      return Math.trunc(value);
    };
    const toOptionalInt = (value: number | undefined) => {
      if (typeof value !== "number" || !Number.isFinite(value)) {
        return undefined;
      }
      return Math.trunc(value);
    };
    const name = data.name.trim() || "unnamed";
    setAreaData((current) => {
      if (!current) {
        return current;
      }
      const record = current as Record<string, unknown>;
      const loot =
        record.loot && typeof record.loot === "object"
          ? (record.loot as Record<string, unknown>)
          : {};
      const groups = Array.isArray(loot.groups)
        ? [...(loot.groups as LootGroup[])]
        : [];
      const tables = Array.isArray(loot.tables)
        ? [...(loot.tables as LootTable[])]
        : [];
      if (selectedAreaLootKind === "group") {
        const entries = (data.entries ?? []).map((entry) => {
          const type = entry.type === "cp" ? "cp" : "item";
          const nextEntry: LootEntry = {
            type,
            minQty: toInt(entry.minQty, 1),
            maxQty: toInt(entry.maxQty, 1),
            weight: toInt(entry.weight, 100)
          };
          if (type === "item") {
            nextEntry.vnum = toInt(entry.vnum, 0);
          }
          return nextEntry;
        });
        const nextRecord: LootGroup = {
          name,
          rolls: toInt(data.rolls, 1),
          entries
        };
        if (
          selectedAreaLootIndex < 0 ||
          selectedAreaLootIndex >= groups.length
        ) {
          return current;
        }
        groups[selectedAreaLootIndex] = nextRecord;
        return { ...record, loot: { ...loot, groups, tables } };
      }
      const ops = (data.ops ?? []).map((op) => {
        const nextOp: LootOp = { op: op.op };
        const group = cleanOptionalString(op.group);
        const vnum = toOptionalInt(op.vnum);
        const chance = toOptionalInt(op.chance);
        const minQty = toOptionalInt(op.minQty);
        const maxQty = toOptionalInt(op.maxQty);
        const multiplier = toOptionalInt(op.multiplier);
        switch (op.op) {
          case "use_group":
            if (group) nextOp.group = group;
            if (chance !== undefined) nextOp.chance = chance;
            break;
          case "add_item":
            if (vnum !== undefined) nextOp.vnum = vnum;
            if (chance !== undefined) nextOp.chance = chance;
            if (minQty !== undefined) nextOp.minQty = minQty;
            if (maxQty !== undefined) nextOp.maxQty = maxQty;
            break;
          case "add_cp":
            if (chance !== undefined) nextOp.chance = chance;
            if (minQty !== undefined) nextOp.minQty = minQty;
            if (maxQty !== undefined) nextOp.maxQty = maxQty;
            break;
          case "mul_cp":
          case "mul_all_chances":
            if (multiplier !== undefined) nextOp.multiplier = multiplier;
            break;
          case "remove_item":
            if (vnum !== undefined) nextOp.vnum = vnum;
            break;
          case "remove_group":
            if (group) nextOp.group = group;
            break;
          default:
            break;
        }
        return nextOp;
      });
      const nextRecord: LootTable = {
        name,
        parent: cleanOptionalString(data.parent),
        ops
      };
      if (
        selectedAreaLootIndex < 0 ||
        selectedAreaLootIndex >= tables.length
      ) {
        return current;
      }
      tables[selectedAreaLootIndex] = nextRecord;
      return { ...record, loot: { ...loot, groups, tables } };
    });
    setStatusMessage(
      `Updated loot ${selectedAreaLootKind} ${name} (unsaved)`
    );
  };

  const handleRecipeSubmit = (data: RecipeFormValues) => {
    if (!areaData || selectedRecipeVnum === null) {
      return;
    }
    const inputs = (data.inputs ?? [])
      .map((input) => ({
        vnum: input.vnum ?? 0,
        quantity: input.quantity ?? 1
      }))
      .filter((input) => input.vnum > 0);
    const nextRecord: RecipeDefinition = {
      vnum: data.vnum,
      name: cleanOptionalString(data.name),
      skill: cleanOptionalString(data.skill),
      minSkill: data.minSkill ?? undefined,
      minSkillPct: data.minSkillPct ?? undefined,
      minLevel: data.minLevel ?? undefined,
      stationType:
        data.stationType && data.stationType.length
          ? data.stationType
          : undefined,
      stationVnum: data.stationVnum ?? undefined,
      discovery: cleanOptionalString(data.discovery),
      inputs: inputs.length ? inputs : undefined,
      outputVnum: data.outputVnum ?? undefined,
      outputQuantity: data.outputQuantity ?? undefined
    };
    setAreaData((current) => {
      if (!current) {
        return current;
      }
      const recipes = getEntityList(current, "Recipes");
      if (!recipes.length) {
        return current;
      }
      const nextRecipes = recipes.map((recipe) => {
        const record = recipe as Record<string, unknown>;
        const vnum = parseVnum(record.vnum);
        if (vnum !== selectedRecipeVnum) {
          return record;
        }
        return nextRecord;
      });
      return {
        ...(current as Record<string, unknown>),
        recipes: nextRecipes
      };
    });
    setStatusMessage(`Updated recipe ${data.vnum} (unsaved)`);
  };

  const handleGatherSpawnSubmit = (data: GatherSpawnFormValues) => {
    if (!areaData || selectedGatherVnum === null) {
      return;
    }
    const nextRecord: GatherSpawnDefinition = {
      spawnSector: data.spawnSector ?? undefined,
      vnum: data.vnum ?? 0,
      quantity: data.quantity ?? undefined,
      respawnTimer: data.respawnTimer ?? undefined
    };
    setAreaData((current) => {
      if (!current) {
        return current;
      }
      const spawns = getEntityList(current, "Gather Spawns");
      if (!spawns.length) {
        return current;
      }
      const nextSpawns = spawns.map((spawn) => {
        const record = spawn as Record<string, unknown>;
        const vnum = parseVnum(record.vnum);
        if (vnum !== selectedGatherVnum) {
          return record;
        }
        return nextRecord;
      });
      return {
        ...(current as Record<string, unknown>),
        gatherSpawns: nextSpawns
      };
    });
    setStatusMessage(`Updated gather spawn ${nextRecord.vnum} (unsaved)`);
  };

  const handleClassSubmit = (data: ClassFormValues) => {
    if (!classData || selectedClassIndex === null) {
      return;
    }
    const guilds = Array.from({ length: classGuildCount }).map(
      (_, index) => data.guilds?.[index] ?? 0
    );
    const titles = buildTitlePairs(data.titlesMale ?? "", data.titlesFemale ?? "");
    const nextRecord: ClassDefinition = {
      name: data.name,
      whoName: cleanOptionalString(data.whoName),
      baseGroup: cleanOptionalString(data.baseGroup),
      defaultGroup: cleanOptionalString(data.defaultGroup),
      weaponVnum: data.weaponVnum,
      armorProf: cleanOptionalString(data.armorProf),
      guilds,
      primeStat: cleanOptionalString(data.primeStat),
      skillCap: data.skillCap,
      thac0_00: data.thac0_00,
      thac0_32: data.thac0_32,
      hpMin: data.hpMin,
      hpMax: data.hpMax,
      manaUser: data.manaUser,
      startLoc: data.startLoc,
      titles
    };
    setClassData((current) => {
      if (!current) {
        return current;
      }
      const nextClasses = [...current.classes];
      nextClasses[selectedClassIndex] = nextRecord;
      return { ...current, classes: nextClasses };
    });
    setStatusMessage(`Updated class ${data.name} (unsaved)`);
  };

  const handleRaceSubmit = (data: RaceFormValues) => {
    if (!raceData || selectedRaceIndex === null) {
      return;
    }
    const classNames = referenceData?.classes ?? [];
    const classMult: Record<string, number> = {};
    const classStart: Record<string, number> = {};
    classNames.forEach((name) => {
      const mult = Number(data.classMult?.[name] ?? 100);
      classMult[name] = Number.isFinite(mult) ? Math.trunc(mult) : 100;
      const start = Number(data.classStart?.[name] ?? 0);
      classStart[name] = Number.isFinite(start) ? Math.trunc(start) : 0;
    });
    const skills = (data.skills ?? [])
      .map((entry) => entry.trim())
      .filter((entry) => entry.length);
    const nextRecord: RaceDefinition = {
      name: data.name,
      whoName: cleanOptionalString(data.whoName),
      pc: data.pc,
      points: data.points,
      size: cleanOptionalString(data.size),
      startLoc: data.startLoc,
      stats: data.stats,
      maxStats: data.maxStats,
      actFlags: data.actFlags?.length ? data.actFlags : undefined,
      affectFlags: data.affectFlags?.length ? data.affectFlags : undefined,
      offFlags: data.offFlags?.length ? data.offFlags : undefined,
      immFlags: data.immFlags?.length ? data.immFlags : undefined,
      resFlags: data.resFlags?.length ? data.resFlags : undefined,
      vulnFlags: data.vulnFlags?.length ? data.vulnFlags : undefined,
      formFlags: data.formFlags?.length ? data.formFlags : undefined,
      partFlags: data.partFlags?.length ? data.partFlags : undefined,
      classMult: classNames.length ? classMult : undefined,
      classStart: classNames.length ? classStart : undefined,
      skills
    };
    setRaceData((current) => {
      if (!current) {
        return current;
      }
      const nextRaces = [...current.races];
      nextRaces[selectedRaceIndex] = nextRecord;
      return { ...current, races: nextRaces };
    });
    setStatusMessage(`Updated race ${data.name} (unsaved)`);
  };

  const handleSkillSubmit = (data: SkillFormValues) => {
    if (!skillData || selectedSkillIndex === null) {
      return;
    }
    const classNames = referenceData?.classes ?? [];
    const levels: Record<string, number> = {};
    const ratings: Record<string, number> = {};
    classNames.forEach((name) => {
      const level = Number(data.levels?.[name] ?? defaultSkillLevel);
      levels[name] = Number.isFinite(level) ? Math.trunc(level) : defaultSkillLevel;
      const rating = Number(data.ratings?.[name] ?? defaultSkillRating);
      ratings[name] = Number.isFinite(rating) ? Math.trunc(rating) : defaultSkillRating;
    });
    const nextRecord: SkillDefinition = {
      name: data.name,
      levels: classNames.length ? levels : undefined,
      ratings: classNames.length ? ratings : undefined,
      spell: cleanOptionalString(data.spell),
      loxSpell: cleanOptionalString(data.loxSpell),
      target: cleanOptionalString(data.target),
      minPosition: cleanOptionalString(data.minPosition),
      gsn: cleanOptionalString(data.gsn),
      slot: data.slot,
      minMana: data.minMana,
      beats: data.beats,
      nounDamage: cleanOptionalString(data.nounDamage),
      msgOff: cleanOptionalString(data.msgOff),
      msgObj: cleanOptionalString(data.msgObj)
    };
    setSkillData((current) => {
      if (!current) {
        return current;
      }
      const nextSkills = [...current.skills];
      nextSkills[selectedSkillIndex] = nextRecord;
      return { ...current, skills: nextSkills };
    });
    setStatusMessage(`Updated skill ${data.name} (unsaved)`);
  };

  const handleGroupSubmit = (data: GroupFormValues) => {
    if (!groupData || selectedGroupIndex === null) {
      return;
    }
    const classNames = referenceData?.classes ?? [];
    const ratings: Record<string, number> = {};
    classNames.forEach((name) => {
      const rating = Number(data.ratings?.[name] ?? defaultSkillRating);
      ratings[name] = Number.isFinite(rating)
        ? Math.trunc(rating)
        : defaultSkillRating;
    });
    const skills = (data.skills ?? [])
      .map((entry) => entry.trim())
      .filter((entry) => entry.length);
    const nextRecord: GroupDefinition = {
      name: data.name,
      ratings: classNames.length ? ratings : undefined,
      skills
    };
    setGroupData((current) => {
      if (!current) {
        return current;
      }
      const nextGroups = [...current.groups];
      nextGroups[selectedGroupIndex] = nextRecord;
      return { ...current, groups: nextGroups };
    });
    setStatusMessage(`Updated group ${data.name} (unsaved)`);
  };

  const handleCommandSubmit = (data: CommandFormValues) => {
    if (!commandData || selectedCommandIndex === null) {
      return;
    }
    const nextRecord: CommandDefinition = {
      name: data.name,
      function: cleanOptionalString(data.function),
      position: cleanOptionalString(data.position),
      level: data.level,
      log: cleanOptionalString(data.log),
      category: cleanOptionalString(data.category),
      loxFunction: cleanOptionalString(data.loxFunction)
    };
    setCommandData((current) => {
      if (!current) {
        return current;
      }
      const nextCommands = [...current.commands];
      nextCommands[selectedCommandIndex] = nextRecord;
      return { ...current, commands: nextCommands };
    });
    setStatusMessage(`Updated command ${data.name} (unsaved)`);
  };

  const handleSocialSubmit = (data: SocialFormValues) => {
    if (!socialData || selectedSocialIndex === null) {
      return;
    }
    const nextRecord: SocialDefinition = {
      name: data.name,
      charNoArg: normalizeOptionalMudText(data.charNoArg),
      othersNoArg: normalizeOptionalMudText(data.othersNoArg),
      charFound: normalizeOptionalMudText(data.charFound),
      othersFound: normalizeOptionalMudText(data.othersFound),
      victFound: normalizeOptionalMudText(data.victFound),
      charAuto: normalizeOptionalMudText(data.charAuto),
      othersAuto: normalizeOptionalMudText(data.othersAuto)
    };
    setSocialData((current) => {
      if (!current) {
        return current;
      }
      const nextSocials = [...current.socials];
      nextSocials[selectedSocialIndex] = nextRecord;
      return { ...current, socials: nextSocials };
    });
    setStatusMessage(`Updated social ${data.name} (unsaved)`);
  };

  const handleTutorialSubmit = (data: TutorialFormValues) => {
    if (!tutorialData || selectedTutorialIndex === null) {
      return;
    }
    const steps = (data.steps ?? [])
      .map((step) => {
        const prompt = normalizeOptionalMudText(step.prompt);
        if (!prompt) {
          return null;
        }
        const match = cleanOptionalString(step.match);
        return {
          prompt,
          match
        };
      })
      .filter(
        (
          step
        ): step is {
          prompt: string;
          match?: string;
        } => Boolean(step)
      );
    const nextRecord: TutorialDefinition = {
      name: data.name,
      blurb: normalizeOptionalMudText(data.blurb),
      finish: normalizeOptionalMudText(data.finish),
      minLevel: data.minLevel ?? 0,
      steps
    };
    setTutorialData((current) => {
      if (!current) {
        return current;
      }
      const nextTutorials = [...current.tutorials];
      nextTutorials[selectedTutorialIndex] = nextRecord;
      return { ...current, tutorials: nextTutorials };
    });
    setStatusMessage(`Updated tutorial ${data.name} (unsaved)`);
  };

  const handleLootSubmit = (data: LootFormValues) => {
    if (!lootData || selectedLootKind === null || selectedLootIndex === null) {
      return;
    }
    const toInt = (value: number | undefined, fallback: number) => {
      if (typeof value !== "number" || !Number.isFinite(value)) {
        return fallback;
      }
      return Math.trunc(value);
    };
    const toOptionalInt = (value: number | undefined) => {
      if (typeof value !== "number" || !Number.isFinite(value)) {
        return undefined;
      }
      return Math.trunc(value);
    };
    const name = data.name.trim() || "unnamed";
    if (selectedLootKind === "group") {
      const entries = (data.entries ?? []).map((entry) => {
        const type = entry.type === "cp" ? "cp" : "item";
        const nextEntry: LootEntry = {
          type,
          minQty: toInt(entry.minQty, 1),
          maxQty: toInt(entry.maxQty, 1),
          weight: toInt(entry.weight, 100)
        };
        if (type === "item") {
          nextEntry.vnum = toInt(entry.vnum, 0);
        }
        return nextEntry;
      });
      const nextRecord: LootGroup = {
        name,
        rolls: toInt(data.rolls, 1),
        entries
      };
      setLootData((current) => {
        if (!current) {
          return current;
        }
        const nextGroups = [...current.groups];
        if (
          selectedLootIndex < 0 ||
          selectedLootIndex >= nextGroups.length
        ) {
          return current;
        }
        nextGroups[selectedLootIndex] = nextRecord;
        return { ...current, groups: nextGroups };
      });
      setStatusMessage(`Updated loot group ${name} (unsaved)`);
      return;
    }
    const ops = (data.ops ?? []).map((op) => {
      const nextOp: LootOp = { op: op.op };
      const group = cleanOptionalString(op.group);
      const vnum = toOptionalInt(op.vnum);
      const chance = toOptionalInt(op.chance);
      const minQty = toOptionalInt(op.minQty);
      const maxQty = toOptionalInt(op.maxQty);
      const multiplier = toOptionalInt(op.multiplier);
      switch (op.op) {
        case "use_group":
          if (group) nextOp.group = group;
          if (chance !== undefined) nextOp.chance = chance;
          break;
        case "add_item":
          if (vnum !== undefined) nextOp.vnum = vnum;
          if (chance !== undefined) nextOp.chance = chance;
          if (minQty !== undefined) nextOp.minQty = minQty;
          if (maxQty !== undefined) nextOp.maxQty = maxQty;
          break;
        case "add_cp":
          if (chance !== undefined) nextOp.chance = chance;
          if (minQty !== undefined) nextOp.minQty = minQty;
          if (maxQty !== undefined) nextOp.maxQty = maxQty;
          break;
        case "mul_cp":
        case "mul_all_chances":
          if (multiplier !== undefined) nextOp.multiplier = multiplier;
          break;
        case "remove_item":
          if (vnum !== undefined) nextOp.vnum = vnum;
          break;
        case "remove_group":
          if (group) nextOp.group = group;
          break;
        default:
          break;
      }
      return nextOp;
    });
    const nextRecord: LootTable = {
      name,
      parent: cleanOptionalString(data.parent),
      ops
    };
    setLootData((current) => {
      if (!current) {
        return current;
      }
      const nextTables = [...current.tables];
      if (
        selectedLootIndex < 0 ||
        selectedLootIndex >= nextTables.length
      ) {
        return current;
      }
      nextTables[selectedLootIndex] = nextRecord;
      return { ...current, tables: nextTables };
    });
    setStatusMessage(`Updated loot table ${name} (unsaved)`);
  };

  const warnLegacyAreaFiles = useCallback(
    async (areaDir: string | null) => {
      if (!areaDir) {
        return;
      }
      if (legacyScanDirRef.current === areaDir) {
        return;
      }
      legacyScanDirRef.current = areaDir;
      try {
        const legacyFiles = await repository.listLegacyAreaFiles(areaDir);
        if (!legacyFiles.length) {
          return;
        }
        const warningMessage = [
          `Legacy ROM-OLC area files found in ${areaDir}:`,
          "",
          ...legacyFiles.map((file) => `- ${file}`),
          "",
          "WorldEdit edits JSON only. Load these areas in Mud98 and save to convert them to JSON before editing."
        ].join("\n");
        await message(warningMessage, {
          title: "Legacy .are files detected",
          kind: "warning"
        });
      } catch (error) {
        setErrorMessage(`Failed to scan area directory. ${String(error)}`);
      }
    },
    [repository]
  );

  const handleOpenProject = async () => {
    setErrorMessage(null);
    setIsBusy(true);
    try {
      const cfgPath = await repository.pickConfigFile(
        areaDirectory ?? dataDirectory
      );
      if (!cfgPath) {
        return;
      }
      const config = await repository.loadProjectConfig(cfgPath);
      setProjectConfig(config);
      if (config.areaDir) {
        setAreaDirectory(config.areaDir);
        localStorage.setItem("worldedit.areaDir", config.areaDir);
        await warnLegacyAreaFiles(config.areaDir);
      }
      if (config.dataDir) {
        setDataDirectory(config.dataDir);
        const refs = await repository.loadReferenceData(
          config.dataDir,
          config.dataFiles
        );
        setReferenceData(refs);
      }
      setStatusMessage(`Loaded project ${fileNameFromPath(cfgPath)}`);
    } catch (error) {
      setErrorMessage(`Failed to load config. ${String(error)}`);
    } finally {
      setIsBusy(false);
    }
  };

  useEffect(() => {
    if (!areaDirectory) {
      return;
    }
    void warnLegacyAreaFiles(areaDirectory);
  }, [areaDirectory, warnLegacyAreaFiles]);

  const handleOpenArea = async () => {
    setErrorMessage(null);
    setIsBusy(true);
    try {
      const path = await repository.pickAreaFile(areaDirectory);
      if (!path) {
        return;
      }
      const areaDir = await repository.resolveAreaDirectory(path);
      await warnLegacyAreaFiles(areaDir);
      const loaded = await repository.loadArea(path);
      const metaPath = repository.editorMetaPathForArea(path);
      let loadedMeta: EditorMeta | null = null;
      setAreaPath(path);
      setAreaData(loaded);
      setEditorMetaPath(metaPath);
      try {
        loadedMeta = await repository.loadEditorMeta(metaPath);
      } catch (error) {
        setErrorMessage(`Failed to load editor meta. ${String(error)}`);
      }
      setEditorMeta(loadedMeta);
      setRoomLayout(extractRoomLayout(loadedMeta?.layout));
      setAreaLayout(extractAreaLayout(loadedMeta?.layout));
      setDirtyRoomNodes(new Set());
      setDirtyAreaNodes(new Set());
      setStatusMessage(
        loadedMeta
          ? `Loaded ${fileNameFromPath(path)} + editor meta`
          : `Loaded ${fileNameFromPath(path)}`
      );
      try {
        const resolvedDataDir =
          projectConfig?.dataDir ??
          (await repository.resolveDataDirectory(path, areaDirectory));
        if (resolvedDataDir) {
          const refs = await repository.loadReferenceData(
            resolvedDataDir,
            projectConfig?.dataFiles
          );
          setReferenceData(refs);
          setDataDirectory(resolvedDataDir);
          setStatusMessage(
            `Loaded ${fileNameFromPath(path)} + reference data`
          );
        }
      } catch (error) {
        setErrorMessage(`Failed to load reference data. ${String(error)}`);
      }
    } catch (error) {
      setErrorMessage(`Failed to load area JSON. ${String(error)}`);
    } finally {
      setIsBusy(false);
    }
  };

  const saveToPath = async (path: string) => {
    if (!areaData) {
      return;
    }
    await repository.saveArea(path, areaData);
    setStatusMessage(`Saved ${fileNameFromPath(path)}`);
  };

  const saveEditorMetaToPath = async (path: string) => {
    const metaPath = repository.editorMetaPathForArea(path);
    const nextMeta = buildEditorMeta(
      editorMeta,
      activeTab,
      selectedEntity,
      roomLayout,
      areaLayout
    );
    await repository.saveEditorMeta(metaPath, nextMeta);
    setEditorMeta(nextMeta);
    setEditorMetaPath(metaPath);
    setStatusMessage(`Saved editor meta ${fileNameFromPath(metaPath)}`);
  };

  const handleSaveArea = async () => {
    if (!areaData) {
      setErrorMessage("No area loaded to save.");
      return;
    }
    setErrorMessage(null);
    setIsBusy(true);
    try {
      let targetPath = areaPath;
      if (!targetPath) {
        targetPath = await repository.pickSaveFile(areaDirectory);
      }
      if (!targetPath) {
        return;
      }
      setAreaPath(targetPath);
      setEditorMetaPath(repository.editorMetaPathForArea(targetPath));
      await saveToPath(targetPath);
    } catch (error) {
      setErrorMessage(`Failed to save area JSON. ${String(error)}`);
    } finally {
      setIsBusy(false);
    }
  };

  const handleSaveAreaAs = async () => {
    if (!areaData) {
      setErrorMessage("No area loaded to save.");
      return;
    }
    setErrorMessage(null);
    setIsBusy(true);
    try {
      const targetPath = await repository.pickSaveFile(areaPath ?? areaDirectory);
      if (!targetPath) {
        return;
      }
      setAreaPath(targetPath);
      setEditorMetaPath(repository.editorMetaPathForArea(targetPath));
      await saveToPath(targetPath);
    } catch (error) {
      setErrorMessage(`Failed to save area JSON. ${String(error)}`);
    } finally {
      setIsBusy(false);
    }
  };

  const handleSaveEditorMeta = async () => {
    if (!areaPath) {
      setErrorMessage("No area loaded to save editor metadata.");
      return;
    }
    setErrorMessage(null);
    setIsBusy(true);
    try {
      await saveEditorMetaToPath(areaPath);
    } catch (error) {
      setErrorMessage(`Failed to save editor meta. ${String(error)}`);
    } finally {
      setIsBusy(false);
    }
  };

  const handleSetAreaDirectory = async () => {
    setErrorMessage(null);
    setIsBusy(true);
    try {
      const selected = await repository.pickAreaDirectory(areaDirectory);
      if (!selected) {
        return;
      }
      setAreaDirectory(selected);
      localStorage.setItem("worldedit.areaDir", selected);
      setStatusMessage(`Area directory set to ${selected}`);
      await warnLegacyAreaFiles(selected);
    } catch (error) {
      setErrorMessage(`Failed to set area directory. ${String(error)}`);
    } finally {
      setIsBusy(false);
    }
  };

  const handleLoadReferenceData = async () => {
    setErrorMessage(null);
    setIsBusy(true);
    try {
      const resolvedDataDir =
        projectConfig?.dataDir ??
        (await repository.resolveDataDirectory(areaPath, areaDirectory));
      if (!resolvedDataDir) {
        setErrorMessage("Unable to resolve data directory.");
        return;
      }
      const refs = await repository.loadReferenceData(
        resolvedDataDir,
        projectConfig?.dataFiles
      );
      setReferenceData(refs);
      setDataDirectory(resolvedDataDir);
      setStatusMessage(`Loaded reference data from ${resolvedDataDir}`);
    } catch (error) {
      setErrorMessage(`Failed to load reference data. ${String(error)}`);
    } finally {
      setIsBusy(false);
    }
  };

  const areaFormNode = (
    <AreaForm
      onSubmit={handleAreaSubmitForm(handleAreaSubmit)}
      register={registerArea}
      formState={areaFormState}
      sectors={sectors}
      storyBeatFields={storyBeatFields}
      appendStoryBeat={appendStoryBeat}
      removeStoryBeat={removeStoryBeat}
      moveStoryBeat={moveStoryBeat}
      checklistFields={checklistFields}
      appendChecklist={appendChecklist}
      removeChecklist={removeChecklist}
      moveChecklist={moveChecklist}
      lootTableOptions={lootTableOptions}
    />
  );
  const roomFormNode = (
    <RoomForm
      onSubmit={handleRoomSubmitForm(handleRoomSubmit)}
      register={registerRoom}
      control={roomFormControl}
      formState={roomFormState}
      exitFields={exitFields}
      appendExit={appendExit}
      removeExit={removeExit}
      directions={directions}
      sectors={sectors}
      roomFlags={roomFlagOptions}
      exitFlags={exitFlags}
      roomVnumOptions={roomVnumOptions}
      objectVnumOptions={objectVnumOptions}
      canEditScript={canEditScript}
      scriptEventEntity={scriptEventEntity}
      eventBindings={eventBindings}
      scriptValue={scriptValue}
      onEventBindingsChange={handleEventBindingsChange}
    />
  );
  const mobileFormNode = (
    <MobileForm
      onSubmit={handleMobileSubmitForm(handleMobileSubmit)}
      register={registerMobile}
      formState={mobileFormState}
      positions={positions}
      sexes={sexes}
      sizes={sizes}
      damageTypes={damageTypes}
      canEditScript={canEditScript}
      scriptEventEntity={scriptEventEntity}
      eventBindings={eventBindings}
      scriptValue={scriptValue}
      onEventBindingsChange={handleEventBindingsChange}
      lootTableOptions={lootTableOptions}
    />
  );
  const objectFormNode = (
    <ObjectForm
      onSubmit={handleObjectSubmitForm(handleObjectSubmit)}
      register={registerObject}
      control={objectFormControl}
      formState={objectFormState}
      itemTypeOptions={itemTypeOptions}
      wearFlags={wearFlags}
      extraFlags={extraFlags}
      weaponClasses={weaponClasses}
      weaponFlags={weaponFlags}
      damageTypes={damageTypes}
      liquids={liquids}
      exitFlags={exitFlags}
      portalFlags={portalFlags}
      containerFlags={containerFlags}
      furnitureFlags={furnitureFlags}
      activeObjectBlock={activeObjectBlock}
      objectVnumOptions={objectVnumOptions}
      roomVnumOptions={roomVnumOptions}
      canEditScript={canEditScript}
      scriptEventEntity={scriptEventEntity}
      eventBindings={eventBindings}
      scriptValue={scriptValue}
      onEventBindingsChange={handleEventBindingsChange}
    />
  );
  const resetFormNode = (
    <ResetForm
      onSubmit={handleResetSubmitForm(handleResetSubmit)}
      register={registerReset}
      control={resetFormControl}
      formState={resetFormState}
      activeResetCommand={activeResetCommand}
      resetCommandOptions={resetCommandOptions}
      resetCommandLabels={resetCommandLabels}
      directions={directions}
      wearLocations={wearLocations}
      roomVnumOptions={roomVnumOptions}
      objectVnumOptions={objectVnumOptions}
      mobileVnumOptions={mobileVnumOptions}
    />
  );
  const shopFormNode = (
    <ShopForm
      onSubmit={handleShopSubmitForm(handleShopSubmit)}
      register={registerShop}
      control={shopFormControl}
      formState={shopFormState}
      mobileVnumOptions={mobileVnumOptions}
    />
  );
  const questFormNode = (
    <QuestForm
      onSubmit={handleQuestSubmitForm(handleQuestSubmit)}
      register={registerQuest}
      control={questFormControl}
      formState={questFormState}
      questTypeOptions={[...questTypeOptions]}
      objectVnumOptions={objectVnumOptions}
    />
  );
  const factionFormNode = (
    <FactionForm
      onSubmit={handleFactionSubmitForm(handleFactionSubmit)}
      register={registerFaction}
      formState={factionFormState}
    />
  );
  const activeAreaLootKind =
    selectedAreaLootKind ??
    (areaLootData && areaLootData.groups.length === 0 ? "table" : "group");
  const areaLootFormNode = (
    <LootForm
      onSubmit={handleAreaLootSubmitForm(handleAreaLootSubmit)}
      register={registerAreaLoot}
      control={areaLootFormControl}
      formState={areaLootFormState}
      kind={activeAreaLootKind}
      entryFields={areaLootEntryFields}
      opFields={areaLootOpFields}
      appendEntry={appendAreaLootEntry}
      removeEntry={removeAreaLootEntry}
      moveEntry={moveAreaLootEntry}
      appendOp={appendAreaLootOp}
      removeOp={removeAreaLootOp}
      moveOp={moveAreaLootOp}
      entryTypeOptions={[...lootEntryTypeOptions]}
      opTypeOptions={[...lootOpTypeOptions]}
      vnumOptions={objectVnumOptions}
    />
  );
  const recipeFormNode = (
    <RecipeForm
      onSubmit={handleRecipeSubmitForm(handleRecipeSubmit)}
      register={registerRecipe}
      control={recipeFormControl}
      formState={recipeFormState}
      inputFields={recipeInputFields}
      appendInput={appendRecipeInput}
      removeInput={removeRecipeInput}
      moveInput={moveRecipeInput}
      workstationTypeOptions={[...workstationTypes]}
      discoveryOptions={[...discoveryTypes]}
      objectVnumOptions={objectVnumOptions}
    />
  );
  const gatherSpawnFormNode = (
    <GatherSpawnForm
      onSubmit={handleGatherSpawnSubmitForm(handleGatherSpawnSubmit)}
      register={registerGatherSpawn}
      control={gatherSpawnFormControl}
      formState={gatherSpawnFormState}
      sectors={[...sectors]}
      objectVnumOptions={objectVnumOptions}
    />
  );
  const classFormNode = (
    <ClassForm
      onSubmit={handleClassSubmitForm(handleClassSubmit)}
      register={registerClass}
      formState={classFormState}
      primeStatOptions={[...primeStatOptions]}
      armorProfOptions={[...armorProfOptions]}
      titleCount={classTitleCount}
    />
  );
  const raceFormNode = (
    <RaceForm
      onSubmit={handleRaceSubmitForm(handleRaceSubmit)}
      register={registerRace}
      formState={raceFormState}
      sizeOptions={[...sizes]}
      statKeys={[...raceStatKeys]}
      actFlags={[...actFlags]}
      affectFlags={[...affectFlags]}
      offFlags={[...offFlags]}
      immFlags={[...immFlags]}
      resFlags={[...resFlags]}
      vulnFlags={[...vulnFlags.filter((flag) => flag !== "none")]}
      formFlags={[...formFlags]}
      partFlags={[...partFlags]}
      classNames={raceClassNames}
      skillCount={raceSkillCount}
    />
  );
  const skillFormNode = (
    <SkillForm
      onSubmit={handleSkillSubmitForm(handleSkillSubmit)}
      register={registerSkill}
      formState={skillFormState}
      classNames={skillClassNames}
      targetOptions={[...skillTargets]}
      positionOptions={[...positions]}
    />
  );
  const groupFormNode = (
    <GroupForm
      onSubmit={handleGroupSubmitForm(handleGroupSubmit)}
      register={registerGroup}
      formState={groupFormState}
      classNames={groupClassNames}
      skillOptions={groupSkillOptions}
      skillCount={groupSkillCount}
    />
  );
  const commandFormNode = (
    <CommandForm
      onSubmit={handleCommandSubmitForm(handleCommandSubmit)}
      register={registerCommand}
      formState={commandFormState}
      positionOptions={[...positions]}
      logOptions={[...logFlags]}
      categoryOptions={[...showFlags]}
    />
  );
  const socialFormNode = (
    <SocialForm
      onSubmit={handleSocialSubmitForm(handleSocialSubmit)}
      register={registerSocial}
      formState={socialFormState}
    />
  );
  const tutorialFormNode = (
    <TutorialForm
      onSubmit={handleTutorialSubmitForm(handleTutorialSubmit)}
      register={registerTutorial}
      formState={tutorialFormState}
      stepFields={tutorialStepFields}
      appendStep={appendTutorialStep}
      removeStep={removeTutorialStep}
      moveStep={moveTutorialStep}
    />
  );
  const activeLootKind =
    selectedLootKind ??
    (lootData && lootData.groups.length === 0 ? "table" : "group");
  const lootFormNode = (
    <LootForm
      onSubmit={handleLootSubmitForm(handleLootSubmit)}
      register={registerLoot}
      control={lootFormControl}
      formState={lootFormState}
      kind={activeLootKind}
      entryFields={lootEntryFields}
      opFields={lootOpFields}
      appendEntry={appendLootEntry}
      removeEntry={removeLootEntry}
      moveEntry={moveLootEntry}
      appendOp={appendLootOp}
      removeOp={removeLootOp}
      moveOp={moveLootOp}
      entryTypeOptions={[...lootEntryTypeOptions]}
      opTypeOptions={[...lootOpTypeOptions]}
      vnumOptions={objectVnumOptions}
    />
  );
  const canCreateRoom = Boolean(areaData);
  const tableToolbar = useMemo(() => {
    if (selectedEntity === "Rooms") {
      return {
        title: "Rooms",
        count: roomRows.length,
        newLabel: "New Room",
        deleteLabel: "Delete Room",
        canCreate: canCreateRoom,
        canDelete: selectedRoomVnum !== null,
        onCreate: handleCreateRoom,
        onDelete: handleDeleteRoom
      };
    }
    if (selectedEntity === "Mobiles") {
      return {
        title: "Mobiles",
        count: mobileRows.length,
        newLabel: "New Mobile",
        deleteLabel: "Delete Mobile",
        canCreate: Boolean(areaData),
        canDelete: selectedMobileVnum !== null,
        onCreate: handleCreateMobile,
        onDelete: handleDeleteMobile
      };
    }
    if (selectedEntity === "Objects") {
      return {
        title: "Objects",
        count: objectRows.length,
        newLabel: "New Object",
        deleteLabel: "Delete Object",
        canCreate: Boolean(areaData),
        canDelete: selectedObjectVnum !== null,
        onCreate: handleCreateObject,
        onDelete: handleDeleteObject
      };
    }
    if (selectedEntity === "Resets") {
      return {
        title: "Resets",
        count: resetRows.length,
        newLabel: "New Reset",
        deleteLabel: "Delete Reset",
        canCreate: Boolean(areaData),
        canDelete: selectedResetIndex !== null,
        onCreate: handleCreateReset,
        onDelete: handleDeleteReset
      };
    }
    if (selectedEntity === "Shops") {
      return {
        title: "Shops",
        count: shopRows.length,
        newLabel: "New Shop",
        deleteLabel: "Delete Shop",
        canCreate: Boolean(areaData),
        canDelete: selectedShopKeeper !== null,
        onCreate: handleCreateShop,
        onDelete: handleDeleteShop
      };
    }
    if (selectedEntity === "Quests") {
      return {
        title: "Quests",
        count: questRows.length,
        newLabel: "New Quest",
        deleteLabel: "Delete Quest",
        canCreate: Boolean(areaData),
        canDelete: selectedQuestVnum !== null,
        onCreate: handleCreateQuest,
        onDelete: handleDeleteQuest
      };
    }
    if (selectedEntity === "Factions") {
      return {
        title: "Factions",
        count: factionRows.length,
        newLabel: "New Faction",
        deleteLabel: "Delete Faction",
        canCreate: Boolean(areaData),
        canDelete: selectedFactionVnum !== null,
        onCreate: handleCreateFaction,
        onDelete: handleDeleteFaction
      };
    }
    if (selectedEntity === "Loot") {
      const hasSelection =
        selectedAreaLootKind !== null && selectedAreaLootIndex !== null;
      const kindLabel =
        selectedAreaLootKind === "table" ? "Table" : "Group";
      return {
        title: "Loot",
        count: areaLootRows.length,
        newLabel: "New Group",
        newLabelSecondary: "New Table",
        deleteLabel: `Delete ${kindLabel}`,
        canCreate: Boolean(areaData),
        canDelete: hasSelection,
        onCreate: () => handleCreateAreaLoot("group"),
        onCreateSecondary: () => handleCreateAreaLoot("table"),
        onDelete: handleDeleteAreaLoot
      };
    }
    if (selectedEntity === "Recipes") {
      return {
        title: "Recipes",
        count: recipeRows.length,
        newLabel: "New Recipe",
        deleteLabel: "Delete Recipe",
        canCreate: Boolean(areaData),
        canDelete: selectedRecipeVnum !== null,
        onCreate: handleCreateRecipe,
        onDelete: handleDeleteRecipe
      };
    }
    if (selectedEntity === "Gather Spawns") {
      return {
        title: "Gather Spawns",
        count: gatherSpawnRows.length,
        newLabel: "New Gather Spawn",
        deleteLabel: "Delete Gather Spawn",
        canCreate: Boolean(areaData),
        canDelete: selectedGatherVnum !== null,
        onCreate: handleCreateGatherSpawn,
        onDelete: handleDeleteGatherSpawn
      };
    }
    return null;
  }, [
    selectedEntity,
    roomRows.length,
    mobileRows.length,
    objectRows.length,
    resetRows.length,
    shopRows.length,
    questRows.length,
    factionRows.length,
    areaLootRows.length,
    recipeRows.length,
    gatherSpawnRows.length,
    selectedRoomVnum,
    selectedMobileVnum,
    selectedObjectVnum,
    selectedResetIndex,
    selectedShopKeeper,
    selectedQuestVnum,
    selectedFactionVnum,
    selectedAreaLootKind,
    selectedAreaLootIndex,
    selectedRecipeVnum,
    selectedGatherVnum,
    canCreateRoom,
    areaData,
    handleCreateRoom,
    handleDeleteRoom,
    handleCreateMobile,
    handleDeleteMobile,
    handleCreateObject,
    handleDeleteObject,
    handleCreateReset,
    handleDeleteReset,
    handleCreateShop,
    handleDeleteShop,
    handleCreateQuest,
    handleDeleteQuest,
    handleCreateFaction,
    handleDeleteFaction,
    handleCreateAreaLoot,
    handleDeleteAreaLoot,
    handleCreateRecipe,
    handleDeleteRecipe,
    handleCreateGatherSpawn,
    handleDeleteGatherSpawn
  ]);
  const tableViewNode = (
    <TableView
      selectedEntity={selectedEntity}
      roomRows={roomRows}
      mobileRows={mobileRows}
      objectRows={objectRows}
      resetRows={resetRows}
      shopRows={shopRows}
      questRows={questRows}
      factionRows={factionRows}
      lootRows={areaLootRows}
      recipeRows={recipeRows}
      gatherSpawnRows={gatherSpawnRows}
      roomColumns={roomColumns}
      mobileColumns={mobileColumns}
      objectColumns={objectColumns}
      resetColumns={resetColumns}
      shopColumns={shopColumns}
      questColumns={questColumns}
      factionColumns={factionColumns}
      lootColumns={lootColumns}
      recipeColumns={recipeColumns}
      gatherSpawnColumns={gatherSpawnColumns}
      roomDefaultColDef={roomDefaultColDef}
      mobileDefaultColDef={mobileDefaultColDef}
      objectDefaultColDef={objectDefaultColDef}
      resetDefaultColDef={roomDefaultColDef}
      shopDefaultColDef={roomDefaultColDef}
      questDefaultColDef={roomDefaultColDef}
      factionDefaultColDef={roomDefaultColDef}
      lootDefaultColDef={lootDefaultColDef}
      recipeDefaultColDef={recipeDefaultColDef}
      gatherSpawnDefaultColDef={gatherSpawnDefaultColDef}
      toolbar={tableToolbar}
      onSelectRoom={setSelectedRoomVnum}
      onSelectMobile={setSelectedMobileVnum}
      onSelectObject={setSelectedObjectVnum}
      onSelectReset={setSelectedResetIndex}
      onSelectShop={setSelectedShopKeeper}
      onSelectQuest={setSelectedQuestVnum}
      onSelectFaction={setSelectedFactionVnum}
      onSelectLoot={(kind, index) => {
        setSelectedAreaLootKind(kind);
        setSelectedAreaLootIndex(index);
      }}
      onSelectRecipe={setSelectedRecipeVnum}
      onSelectGatherSpawn={setSelectedGatherVnum}
      roomGridApiRef={roomGridApi}
      mobileGridApiRef={mobileGridApi}
      objectGridApiRef={objectGridApi}
      resetGridApiRef={resetGridApi}
      shopGridApiRef={shopGridApi}
      questGridApiRef={questGridApi}
      factionGridApiRef={factionGridApi}
      lootGridApiRef={areaLootGridApi}
      recipeGridApiRef={recipeGridApi}
      gatherSpawnGridApiRef={gatherGridApi}
    />
  );
  const classTableToolbar = useMemo(
    () => ({
      title: "Classes",
      count: classRows.length,
      newLabel: "New Class",
      deleteLabel: "Delete Class",
      canCreate: Boolean(classData),
      canDelete: selectedClassIndex !== null,
      onCreate: handleCreateClass,
      onDelete: handleDeleteClass
    }),
    [
      classRows.length,
      classData,
      selectedClassIndex,
      handleCreateClass,
      handleDeleteClass
    ]
  );
  const classTableViewNode = (
    <ClassTableView
      rows={classRows}
      columns={classColumns}
      defaultColDef={classDefaultColDef}
      toolbar={classTableToolbar}
      onSelectClass={setSelectedClassIndex}
      gridApiRef={classGridApi}
    />
  );
  const raceTableToolbar = useMemo(
    () => ({
      title: "Races",
      count: raceRows.length,
      newLabel: "New Race",
      deleteLabel: "Delete Race",
      canCreate: Boolean(raceData),
      canDelete: selectedRaceIndex !== null,
      onCreate: handleCreateRace,
      onDelete: handleDeleteRace
    }),
    [
      raceRows.length,
      raceData,
      selectedRaceIndex,
      handleCreateRace,
      handleDeleteRace
    ]
  );
  const raceTableViewNode = (
    <RaceTableView
      rows={raceRows}
      columns={raceColumns}
      defaultColDef={raceDefaultColDef}
      toolbar={raceTableToolbar}
      onSelectRace={setSelectedRaceIndex}
      gridApiRef={raceGridApi}
    />
  );
  const skillTableToolbar = useMemo(
    () => ({
      title: "Skills",
      count: skillRows.length,
      newLabel: "New Skill",
      deleteLabel: "Delete Skill",
      canCreate: Boolean(skillData),
      canDelete: selectedSkillIndex !== null,
      onCreate: handleCreateSkill,
      onDelete: handleDeleteSkill
    }),
    [
      skillRows.length,
      skillData,
      selectedSkillIndex,
      handleCreateSkill,
      handleDeleteSkill
    ]
  );
  const skillTableViewNode = (
    <SkillTableView
      rows={skillRows}
      columns={skillColumns}
      defaultColDef={skillDefaultColDef}
      toolbar={skillTableToolbar}
      onSelectSkill={setSelectedSkillIndex}
      gridApiRef={skillGridApi}
    />
  );
  const groupTableViewNode = (
    <GroupTableView
      rows={groupRows}
      columns={groupColumns}
      defaultColDef={groupDefaultColDef}
      onSelectGroup={setSelectedGroupIndex}
      gridApiRef={groupGridApi}
    />
  );
  const commandTableViewNode = (
    <CommandTableView
      rows={commandRows}
      columns={commandColumns}
      defaultColDef={commandDefaultColDef}
      onSelectCommand={setSelectedCommandIndex}
      gridApiRef={commandGridApi}
    />
  );
  const socialTableViewNode = (
    <SocialTableView
      rows={socialRows}
      columns={socialColumns}
      defaultColDef={socialDefaultColDef}
      onSelectSocial={setSelectedSocialIndex}
      gridApiRef={socialGridApi}
    />
  );
  const tutorialTableViewNode = (
    <TutorialTableView
      rows={tutorialRows}
      columns={tutorialColumns}
      defaultColDef={tutorialDefaultColDef}
      onSelectTutorial={setSelectedTutorialIndex}
      gridApiRef={tutorialGridApi}
    />
  );
  const lootTableViewNode = (
    <LootTableView
      rows={lootRows}
      columns={lootColumns}
      defaultColDef={lootDefaultColDef}
      onSelectLoot={(kind, index) => {
        setSelectedLootKind(kind);
        setSelectedLootIndex(index);
      }}
      gridApiRef={lootGridApi}
    />
  );
  const mapViewNode = (
    <MapView
      mapNodes={mapNodes}
      roomEdges={roomEdges}
      roomNodeTypes={roomNodeTypes}
      roomEdgeTypes={roomEdgeTypes}
      areaVnumRange={areaVnumRange}
      externalExits={externalExits}
      onNavigateExit={handleMapNavigate}
      onNodeClick={handleMapNodeClick}
      onNodesChange={handleNodesChange}
      onNodeDragStop={handleNodeDragStop}
      autoLayoutEnabled={autoLayoutEnabled}
      preferCardinalLayout={preferCardinalLayout}
      showVerticalEdges={showVerticalEdges}
      dirtyRoomCount={dirtyRoomCount}
      selectedRoomNode={Boolean(selectedRoomNode)}
      selectedRoomLocked={selectedRoomLocked}
      hasRoomLayout={hasRoomLayout}
      onRelayout={handleRelayout}
      onToggleAutoLayout={setAutoLayoutEnabled}
      onTogglePreferGrid={handleTogglePreferGrid}
      onToggleVerticalEdges={setShowVerticalEdges}
      onLockSelected={handleLockSelectedRoom}
      onUnlockSelected={handleUnlockSelectedRoom}
      onClearLayout={handleClearRoomLayout}
    />
  );
  const worldViewNode = (
    <AreaGraphView
      nodes={worldMapNodes}
      edges={areaGraphEdges}
      edgeTypes={areaEdgeTypes}
      nodesDraggable={true}
      dirtyCount={dirtyAreaCount}
      selectedNodeLocked={selectedAreaLocked}
      hasLayout={hasAreaLayout}
      preferCardinalLayout={preferAreaCardinalLayout}
      filterValue={areaGraphFilter}
      vnumQuery={areaGraphVnumQuery}
      matchLabel={areaGraphMatchLabel}
      onFilterChange={setAreaGraphFilter}
      onVnumQueryChange={setAreaGraphVnumQuery}
      onNodeClick={() => null}
      onNodesChange={handleAreaGraphNodesChange}
      onNodeDragStop={handleAreaGraphNodeDragStop}
      onTogglePreferGrid={handleTogglePreferAreaGrid}
      onRelayout={handleRelayoutArea}
      onLockSelected={handleLockSelectedArea}
      onUnlockSelected={handleUnlockSelectedArea}
      onLockDirty={handleLockDirtyAreas}
      onClearLayout={handleClearAreaLayout}
    />
  );
  const scriptViewNode = (
    <ScriptView
      title={scriptTitle}
      subtitle={scriptSubtitle}
      value={scriptValue}
      canEdit={canEditScript}
      emptyState="Select a room, mobile, or object to edit its Lox script."
      onApply={handleApplyScript}
    />
  );
  const referenceSummary = referenceData
    ? `classes ${referenceData.classes.length}, races ${referenceData.races.length}, skills ${referenceData.skills.length}, groups ${referenceData.groups.length}, commands ${referenceData.commands.length}, socials ${referenceData.socials.length}, tutorials ${referenceData.tutorials.length}`
    : "not loaded";
  const classFileLabel = classDataPath
    ? fileNameFromPath(classDataPath)
    : "not loaded";
  const raceFileLabel = raceDataPath
    ? fileNameFromPath(raceDataPath)
    : "not loaded";
  const skillFileLabel = skillDataPath
    ? fileNameFromPath(skillDataPath)
    : "not loaded";
  const groupFileLabel = groupDataPath
    ? fileNameFromPath(groupDataPath)
    : "not loaded";
  const commandFileLabel = commandDataPath
    ? fileNameFromPath(commandDataPath)
    : "not loaded";
  const socialFileLabel = socialDataPath
    ? fileNameFromPath(socialDataPath)
    : "not loaded";
  const tutorialFileLabel = tutorialDataPath
    ? fileNameFromPath(tutorialDataPath)
    : "not loaded";
  const lootFileLabel = lootDataPath
    ? fileNameFromPath(lootDataPath)
    : "not loaded";
  const globalFileLabels: Record<GlobalEntityKey, string> = {
    Classes: classFileLabel,
    Races: raceFileLabel,
    Skills: skillFileLabel,
    Groups: groupFileLabel,
    Commands: commandFileLabel,
    Socials: socialFileLabel,
    Tutorials: tutorialFileLabel,
    Loot: lootFileLabel
  };
  const dataFileLabel = globalFileLabels[selectedGlobalEntity] ?? "not loaded";
  const classActionsNode = (
    <GlobalFormActions
      label="Classes"
      dataFileLabel={classFileLabel}
      loadDisabled={!dataDirectory || isBusy}
      saveDisabled={!classData || isBusy}
      onLoad={handleLoadClassesData}
      onSave={handleSaveClassesData}
    />
  );
  const raceActionsNode = (
    <GlobalFormActions
      label="Races"
      dataFileLabel={raceFileLabel}
      loadDisabled={!dataDirectory || isBusy}
      saveDisabled={!raceData || isBusy}
      onLoad={handleLoadRacesData}
      onSave={handleSaveRacesData}
    />
  );
  const skillActionsNode = (
    <GlobalFormActions
      label="Skills"
      dataFileLabel={skillFileLabel}
      loadDisabled={!dataDirectory || isBusy}
      saveDisabled={!skillData || isBusy}
      onLoad={handleLoadSkillsData}
      onSave={handleSaveSkillsData}
    />
  );
  const groupActionsNode = (
    <GlobalFormActions
      label="Groups"
      dataFileLabel={groupFileLabel}
      loadDisabled={!dataDirectory || isBusy}
      saveDisabled={!groupData || isBusy}
      onLoad={handleLoadGroupsData}
      onSave={handleSaveGroupsData}
    />
  );
  const commandActionsNode = (
    <GlobalFormActions
      label="Commands"
      dataFileLabel={commandFileLabel}
      loadDisabled={!dataDirectory || isBusy}
      saveDisabled={!commandData || isBusy}
      onLoad={handleLoadCommandsData}
      onSave={handleSaveCommandsData}
    />
  );
  const socialActionsNode = (
    <GlobalFormActions
      label="Socials"
      dataFileLabel={socialFileLabel}
      loadDisabled={!dataDirectory || isBusy}
      saveDisabled={!socialData || isBusy}
      onLoad={handleLoadSocialsData}
      onSave={handleSaveSocialsData}
    />
  );
  const tutorialActionsNode = (
    <GlobalFormActions
      label="Tutorials"
      dataFileLabel={tutorialFileLabel}
      loadDisabled={!dataDirectory || isBusy}
      saveDisabled={!tutorialData || isBusy}
      onLoad={handleLoadTutorialsData}
      onSave={handleSaveTutorialsData}
    />
  );
  const lootActionsNode = (
    <GlobalFormActions
      label="Loot"
      dataFileLabel={lootFileLabel}
      loadDisabled={!dataDirectory || isBusy}
      saveDisabled={!lootData || isBusy}
      onLoad={handleLoadLootData}
      onSave={handleSaveLootData}
    />
  );
  const viewTitle =
    editorMode === "Area" ? selectedEntity : selectedGlobalEntity;
  const viewMeta =
    editorMode === "Area"
      ? [
          `VNUM range ${selection.vnumRange}`,
          `Last save ${selection.lastSave}`,
          `Active view ${activeTab}`
        ]
      : [
          `Data dir ${dataDirectory ?? "not set"}`,
          `Data file ${dataFileLabel}`,
          `Reference ${referenceSummary}`,
          `Active view ${activeTab}`
        ];
  const visibleTabs =
    editorMode === "Area"
      ? tabs
      : tabs.filter((tab) => tab.id === "Table" || tab.id === "Form");
  const treeItems = editorMode === "Area" ? entityItems : globalItems;
  const treeSelection =
    editorMode === "Area" ? selectedEntity : selectedGlobalEntity;
  const statusSelection =
    editorMode === "Area"
      ? selectedEntity
      : `Global · ${selectedGlobalEntity}`;
  const handleTreeSelect = (entity: string) => {
    if (editorMode === "Area") {
      setSelectedEntity(entity as EntityKey);
    } else {
      setSelectedGlobalEntity(entity as GlobalEntityKey);
    }
  };

  return (
    <div className="app-shell">
      <Topbar
        areaName={areaName}
        errorMessage={errorMessage}
        statusMessage={statusMessage}
        isBusy={isBusy}
        hasArea={Boolean(areaData)}
        hasAreaPath={Boolean(areaPath)}
        onOpenProject={handleOpenProject}
        onOpenArea={handleOpenArea}
        onSetAreaDirectory={handleSetAreaDirectory}
        onLoadReferenceData={handleLoadReferenceData}
        onSaveArea={handleSaveArea}
        onSaveEditorMeta={handleSaveEditorMeta}
        onSaveAreaAs={handleSaveAreaAs}
      />

      <section className="workspace">
        <EntityTree
          mode={editorMode}
          onModeChange={setEditorMode}
          items={treeItems}
          selectedEntity={treeSelection}
          onSelectEntity={handleTreeSelect}
        />

        <main className="center">
          <ViewTabs
            tabs={visibleTabs}
            activeTab={activeTab}
            onSelectTab={setActiveTab}
          />

          <div className="view-card">
            <ViewCardHeader
              title={viewTitle}
              meta={viewMeta}
            />
            <div className="view-card__body">
              <ViewBody
                activeTab={activeTab}
                editorMode={editorMode}
                selectedEntity={selectedEntity}
                selectedGlobalEntity={selectedGlobalEntity}
                tabs={visibleTabs}
                hasAreaData={Boolean(areaData)}
                classCount={classRows.length}
                raceCount={raceRows.length}
                skillCount={skillRows.length}
                groupCount={groupRows.length}
                commandCount={commandRows.length}
                socialCount={socialRows.length}
                tutorialCount={tutorialRows.length}
                lootCount={lootCount}
                classActions={classActionsNode}
                raceActions={raceActionsNode}
                skillActions={skillActionsNode}
                groupActions={groupActionsNode}
                commandActions={commandActionsNode}
                socialActions={socialActionsNode}
                tutorialActions={tutorialActionsNode}
                lootActions={lootActionsNode}
                areaLootCount={areaLootRows.length}
                recipeCount={recipeRows.length}
                gatherSpawnCount={gatherSpawnRows.length}
                roomCount={roomRows.length}
                mobileCount={mobileRows.length}
                objectCount={objectRows.length}
                resetCount={resetRows.length}
                shopCount={shopRows.length}
                questCount={questRows.length}
                factionCount={factionRows.length}
                mapHasRooms={mapNodes.length > 0}
                areaForm={areaFormNode}
                roomForm={roomFormNode}
                mobileForm={mobileFormNode}
                objectForm={objectFormNode}
                resetForm={resetFormNode}
                shopForm={shopFormNode}
                questForm={questFormNode}
                factionForm={factionFormNode}
                classForm={classFormNode}
                raceForm={raceFormNode}
                skillForm={skillFormNode}
                groupForm={groupFormNode}
                commandForm={commandFormNode}
                socialForm={socialFormNode}
                tutorialForm={tutorialFormNode}
                lootForm={lootFormNode}
                areaLootForm={areaLootFormNode}
                recipeForm={recipeFormNode}
                gatherSpawnForm={gatherSpawnFormNode}
                tableView={tableViewNode}
                classTableView={classTableViewNode}
                raceTableView={raceTableViewNode}
                skillTableView={skillTableViewNode}
                groupTableView={groupTableViewNode}
                commandTableView={commandTableViewNode}
                socialTableView={socialTableViewNode}
                tutorialTableView={tutorialTableViewNode}
                lootTableView={lootTableViewNode}
                mapView={mapViewNode}
                worldView={worldViewNode}
                scriptView={scriptViewNode}
              />
            </div>
          </div>
        </main>

        <InspectorPanel
          selection={selection}
          validationSummary={validationSummary}
          diagnosticsCount={diagnosticsCount}
          diagnosticFilter={diagnosticFilter}
          entityOrder={entityOrder}
          diagnosticsList={diagnosticsList}
          entityKindLabels={entityKindLabels}
          onDiagnosticFilterChange={(value) =>
            setDiagnosticFilter(value as "All" | EntityKey)
          }
          onDiagnosticsClick={handleDiagnosticsClick}
          areaDebug={areaDebug}
        />
      </section>

      <Statusbar
        errorMessage={errorMessage}
        statusMessage={statusMessage}
        areaPath={areaPath}
        editorMetaPath={editorMetaPath}
        areaDirectory={areaDirectory}
        dataDirectory={dataDirectory}
        projectPath={projectConfig?.path ?? null}
        selectedEntity={statusSelection}
        referenceSummary={referenceSummary}
      />
    </div>
  );
}
