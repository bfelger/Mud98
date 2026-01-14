import { useEffect, useMemo, useRef, useState } from "react";
import type { ColDef, GridApi } from "ag-grid-community";
import { AgGridReact } from "ag-grid-react";
import ReactFlow, {
  Background,
  Controls,
  Handle,
  Position,
  type Edge,
  type Node,
  type NodeProps
} from "reactflow";
import { zodResolver } from "@hookform/resolvers/zod";
import {
  useFieldArray,
  useForm,
  useWatch,
  type FieldPath,
  type FieldValues,
  type UseFormRegister
} from "react-hook-form";
import { z } from "zod";
import { LocalFileRepository } from "./repository/localFileRepository";
import type { AreaJson, EditorMeta, ReferenceData } from "./repository/types";
import {
  containerFlagEnum,
  containerFlags,
  damageTypeEnum,
  damageTypes,
  directionEnum,
  directions,
  exitFlagEnum,
  exitFlags,
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
  weaponFlags
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
    id: "Script",
    title: "Script View",
    description: "Lox script editing with events panel."
  }
] as const;

const entityOrder = [
  "Rooms",
  "Mobiles",
  "Objects",
  "Resets",
  "Shops",
  "Quests",
  "Factions",
  "Helps"
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

type TabId = (typeof tabs)[number]["id"];
type EntityKey = (typeof entityOrder)[number];
type VnumOption = {
  vnum: number;
  label: string;
};

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

type RoomNodeData = {
  vnum: number;
  label: string;
  sector: string;
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
  manaRate: optionalIntSchema,
  healRate: optionalIntSchema,
  clan: optionalIntSchema,
  owner: z.string().optional(),
  exits: z.array(exitFormSchema).optional()
});

type RoomFormValues = z.infer<typeof roomFormSchema>;
type MobileFormValues = z.infer<typeof mobileFormSchema>;
type ObjectFormValues = z.infer<typeof objectFormSchema>;
type ResetFormValues = z.infer<typeof resetFormSchema>;

const entityKindLabels: Record<EntityKey, string> = {
  Rooms: "Room",
  Mobiles: "Mobile",
  Objects: "Object",
  Resets: "Reset",
  Shops: "Shop",
  Quests: "Quest",
  Factions: "Faction",
  Helps: "Help"
};

const entityDataKeys: Record<EntityKey, string> = {
  Rooms: "rooms",
  Mobiles: "mobiles",
  Objects: "objects",
  Resets: "resets",
  Shops: "shops",
  Quests: "quests",
  Factions: "factions",
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
  selectedEntity: string
): EditorMeta {
  const now = new Date().toISOString();
  const base: EditorMeta = existing ?? {
    version: 1,
    updatedAt: now,
    view: { activeTab },
    selection: { entityType: selectedEntity },
    layout: {}
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
    }
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

function getFirstString(value: unknown, fallback: string): string {
  return typeof value === "string" && value.trim().length ? value : fallback;
}

type VnumPickerProps<TFieldValues extends FieldValues> = {
  id: string;
  label: string;
  name: FieldPath<TFieldValues>;
  register: UseFormRegister<TFieldValues>;
  options: VnumOption[];
  error?: string;
};

function VnumPicker<TFieldValues extends FieldValues>({
  id,
  label,
  name,
  register,
  options,
  error
}: VnumPickerProps<TFieldValues>) {
  const listId = `${id}-options`;
  return (
    <div className="form-field">
      <label className="form-label" htmlFor={id}>
        {label}
      </label>
      <input
        id={id}
        className="form-input"
        type="text"
        inputMode="numeric"
        list={listId}
        {...register(name)}
      />
      <datalist id={listId}>
        {options.map((option) => (
          <option key={option.vnum} value={String(option.vnum)}>
            {option.label}
          </option>
        ))}
      </datalist>
      {error ? <span className="form-error">{error}</span> : null}
    </div>
  );
}

function RoomNode({ data, selected }: NodeProps<RoomNodeData>) {
  return (
    <div className={`room-node${selected ? " room-node--selected" : ""}`}>
      <Handle type="source" position={Position.Top} className="room-node__handle" />
      <Handle type="source" position={Position.Right} className="room-node__handle" />
      <Handle type="source" position={Position.Bottom} className="room-node__handle" />
      <Handle type="source" position={Position.Left} className="room-node__handle" />
      <Handle type="target" position={Position.Top} className="room-node__handle" />
      <Handle type="target" position={Position.Right} className="room-node__handle" />
      <Handle type="target" position={Position.Bottom} className="room-node__handle" />
      <Handle type="target" position={Position.Left} className="room-node__handle" />
      <div className="room-node__vnum">{data.vnum}</div>
      <div className="room-node__name">{data.label}</div>
      <div className="room-node__sector">{data.sector}</div>
    </div>
  );
}

const roomNodeTypes = { room: RoomNode };

function buildVnumOptions(
  areaData: AreaJson | null,
  entity: "Rooms" | "Mobiles" | "Objects"
): VnumOption[] {
  if (!areaData) {
    return [];
  }
  const list = getEntityList(areaData, entity);
  const options: VnumOption[] = [];
  for (const entry of list) {
    if (!entry || typeof entry !== "object") {
      continue;
    }
    const record = entry as Record<string, unknown>;
    const vnum = parseVnum(record.vnum);
    if (vnum === null) {
      continue;
    }
    let label = "VNUM";
    if (entity === "Rooms") {
      label = getFirstString(record.name, "(unnamed room)");
    } else if (entity === "Mobiles") {
      label = getFirstString(record.shortDescr, "(unnamed mobile)");
    } else {
      label = getFirstString(record.shortDescr, "(unnamed object)");
    }
    options.push({ vnum, label: `${vnum} - ${label}` });
  }
  return options;
}

function buildSelectionSummary(
  selectedEntity: EntityKey,
  areaData: AreaJson | null,
  selectedVnums: Partial<Record<EntityKey, number | null>>
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
        : findByVnum(list, selectedValue)
      : null;
  const first = (selected ?? list[0] ?? {}) as Record<string, unknown>;
  const vnumRange = getAreaVnumRange(areaData);
  const vnum = parseVnum(first.vnum);
  const emptyLabel = `No ${selectedEntity.toLowerCase()} loaded`;

  let selectionLabel = emptyLabel;
  let flags: string[] = [];
  let exits = "n/a";

  switch (selectedEntity) {
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
    selectedEntity === "Objects"
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

function buildRoomNodes(
  areaData: AreaJson | null,
  selectedVnum: number | null
): Node<RoomNodeData>[] {
  if (!areaData) {
    return [];
  }
  const rooms = getEntityList(areaData, "Rooms");
  if (!rooms.length) {
    return [];
  }
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
    const col = index % columns;
    const row = Math.floor(index / columns);
    return {
      id: vnum !== null ? String(vnum) : `room-${index}`,
      type: "room",
      position: { x: col * spacingX, y: row * spacingY },
      selected: selectedVnum !== null && vnum === selectedVnum,
      data: {
        vnum: vnum ?? -1,
        label: name,
        sector
      }
    };
  });
}

function buildRoomEdges(areaData: AreaJson | null): Edge[] {
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
      if (toVnum === null || !roomVnums.has(toVnum)) {
        return;
      }
      const dir =
        typeof exitRecord.dir === "string" ? exitRecord.dir : "exit";
      edges.push({
        id: `exit-${fromVnum}-${dir}-${toVnum}-${index}`,
        source: String(fromVnum),
        target: String(toVnum),
        label: dir,
        type: "smoothstep",
        style: { stroke: "rgba(47, 108, 106, 0.45)", strokeWidth: 1.5 },
        labelStyle: { fill: "#184a4a", fontSize: 11, fontWeight: 600 }
      });
    });
  }
  return edges;
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

function syncGridSelection(api: GridApi | null, vnum: number | null) {
  if (!api) {
    return;
  }
  api.deselectAll();
  if (vnum === null) {
    return;
  }
  const node = api.getRowNode(String(vnum));
  if (node) {
    node.setSelected(true);
  }
}

export default function App() {
  const repository = useMemo(() => new LocalFileRepository(), []);
  const [activeTab, setActiveTab] = useState<TabId>(tabs[0].id);
  const [selectedEntity, setSelectedEntity] = useState<EntityKey>(
    entityOrder[0]
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
  const [areaPath, setAreaPath] = useState<string | null>(null);
  const [areaData, setAreaData] = useState<AreaJson | null>(null);
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
  const roomGridApi = useRef<GridApi | null>(null);
  const mobileGridApi = useRef<GridApi | null>(null);
  const objectGridApi = useRef<GridApi | null>(null);
  const resetGridApi = useRef<GridApi | null>(null);
  const roomForm = useForm<RoomFormValues>({
    resolver: zodResolver(roomFormSchema),
    defaultValues: {
      vnum: 0,
      name: "",
      description: "",
      sectorType: undefined,
      roomFlags: [],
      manaRate: undefined,
      healRate: undefined,
      clan: undefined,
      owner: "",
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
    formState: objectFormState
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
    formState: resetFormState
  } = resetForm;
  const watchedResetCommand = useWatch({
    control: resetForm.control,
    name: "commandName"
  });
  const watchedObjectItemType = useWatch({
    control: objectForm.control,
    name: "itemType"
  });

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
  }, [
    areaData,
    selectedRoomVnum,
    selectedMobileVnum,
    selectedObjectVnum,
    selectedResetIndex
  ]);

  useEffect(() => {
    setSelectedRoomVnum(null);
    setSelectedMobileVnum(null);
    setSelectedObjectVnum(null);
    setSelectedResetIndex(null);
  }, [areaData]);

  const selection = useMemo(
    () =>
      buildSelectionSummary(selectedEntity, areaData, {
        Rooms: selectedRoomVnum,
        Mobiles: selectedMobileVnum,
        Objects: selectedObjectVnum,
        Resets: selectedResetIndex
      }),
    [
      areaData,
      selectedEntity,
      selectedRoomVnum,
      selectedMobileVnum,
      selectedObjectVnum,
      selectedResetIndex
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
  const roomRows = useMemo(() => buildRoomRows(areaData), [areaData]);
  const mobileRows = useMemo(() => buildMobileRows(areaData), [areaData]);
  const objectRows = useMemo(() => buildObjectRows(areaData), [areaData]);
  const resetRows = useMemo(() => buildResetRows(areaData), [areaData]);
  const roomNodes = useMemo(
    () => buildRoomNodes(areaData, selectedRoomVnum),
    [areaData, selectedRoomVnum]
  );
  const roomEdges = useMemo(() => buildRoomEdges(areaData), [areaData]);
  const selectedRoomId = selectedRoomVnum !== null ? String(selectedRoomVnum) : null;
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
    const manaRate = parseVnum(record?.manaRate) ?? undefined;
    const healRate = parseVnum(record?.healRate) ?? undefined;
    const clan = parseVnum(record?.clan) ?? undefined;
    return {
      vnum,
      name: typeof record?.name === "string" ? record.name : "",
      description:
        typeof record?.description === "string" ? record.description : "",
      sectorType,
      roomFlags,
      manaRate,
      healRate,
      clan,
      owner: typeof record?.owner === "string" ? record.owner : "",
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
                typeof exit.description === "string" ? exit.description : "",
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
        typeof record?.longDescr === "string" ? record.longDescr : "",
      description:
        typeof record?.description === "string" ? record.description : "",
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
        typeof record?.description === "string" ? record.description : "",
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
    if (activeTab === "Map" && selectedEntity !== "Rooms") {
      setSelectedEntity("Rooms");
    }
  }, [activeTab, selectedEntity]);

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
      if (exit.description && exit.description.trim().length) {
        nextExit.description = exit.description;
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
          description: data.description,
          sectorType: data.sectorType ?? undefined,
          roomFlags: data.roomFlags?.length ? data.roomFlags : undefined,
          manaRate: data.manaRate ?? undefined,
          healRate: data.healRate ?? undefined,
          clan: data.clan ?? undefined,
          owner: data.owner ?? undefined,
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
        return {
          ...record,
          name: data.name,
          shortDescr: data.shortDescr,
          longDescr: data.longDescr,
          description: data.description,
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
          hitDice,
          manaDice,
          damageDice
        };
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
          description: data.description,
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

  const handleOpenArea = async () => {
    setErrorMessage(null);
    setIsBusy(true);
    try {
      const path = await repository.pickAreaFile(areaDirectory);
      if (!path) {
        return;
      }
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
      setStatusMessage(
        loadedMeta
          ? `Loaded ${fileNameFromPath(path)} + editor meta`
          : `Loaded ${fileNameFromPath(path)}`
      );
      try {
        const resolvedDataDir = await repository.resolveDataDirectory(
          path,
          areaDirectory
        );
        if (resolvedDataDir) {
          const refs = await repository.loadReferenceData(resolvedDataDir);
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
    const nextMeta = buildEditorMeta(editorMeta, activeTab, selectedEntity);
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
      const resolvedDataDir = await repository.resolveDataDirectory(
        areaPath,
        areaDirectory
      );
      if (!resolvedDataDir) {
        setErrorMessage("Unable to resolve data directory.");
        return;
      }
      const refs = await repository.loadReferenceData(resolvedDataDir);
      setReferenceData(refs);
      setDataDirectory(resolvedDataDir);
      setStatusMessage(`Loaded reference data from ${resolvedDataDir}`);
    } catch (error) {
      setErrorMessage(`Failed to load reference data. ${String(error)}`);
    } finally {
      setIsBusy(false);
    }
  };

  return (
    <div className="app-shell">
      <header className="topbar">
        <div className="brand">
          <span className="brand__badge">Mud98</span>
          <div className="brand__text">
            <span className="brand__title">WorldEdit</span>
            <span className="brand__subtitle">Area JSON editor</span>
          </div>
        </div>
        <div className="topbar__status">
          <span
            className={`status-pill ${
              errorMessage ? "status-pill--error" : "status-pill--ok"
            }`}
          >
            {errorMessage ?? statusMessage}
          </span>
          <span className="status-pill">Area: {areaName}</span>
        </div>
        <div className="topbar__actions">
          <button
            className="action-button"
            type="button"
            onClick={handleOpenArea}
            disabled={isBusy}
          >
            Open Area
          </button>
          <button
            className="action-button"
            type="button"
            onClick={handleSetAreaDirectory}
            disabled={isBusy}
          >
            Set Area Dir
          </button>
          <button
            className="action-button"
            type="button"
            onClick={handleLoadReferenceData}
            disabled={isBusy}
          >
            Load Ref Data
          </button>
          <button
            className="action-button action-button--primary"
            type="button"
            onClick={handleSaveArea}
            disabled={!areaData || isBusy}
          >
            Save
          </button>
          <button
            className="action-button"
            type="button"
            onClick={handleSaveEditorMeta}
            disabled={!areaPath || isBusy}
          >
            Save Meta
          </button>
          <button
            className="action-button"
            type="button"
            onClick={handleSaveAreaAs}
            disabled={!areaData || isBusy}
          >
            Save As
          </button>
        </div>
      </header>

      <section className="workspace">
        <aside className="sidebar">
          <div className="panel-header">
            <h2>Entity Tree</h2>
            <button className="ghost-button" type="button">
              New
            </button>
          </div>
          <div className="tree">
            <div className="tree-group">
              <div className="tree-group__label">Area</div>
              <ul className="tree-list">
                {entityItems.map((child) => (
                  <li key={child.key}>
                    <button
                      className={`tree-item${
                        child.key === selectedEntity
                          ? " tree-item--active"
                          : ""
                      }`}
                      type="button"
                      aria-pressed={child.key === selectedEntity}
                      onClick={() => setSelectedEntity(child.key)}
                    >
                      <span className="tree-item__label">{child.key}</span>
                      <span className="tree-item__count">{child.count}</span>
                    </button>
                  </li>
                ))}
              </ul>
            </div>
          </div>
        </aside>

        <main className="center">
          <nav className="tabs" aria-label="View tabs">
            {tabs.map((tab) => (
              <button
                key={tab.id}
                type="button"
                className={`tab${tab.id === activeTab ? " tab--active" : ""}`}
                aria-pressed={tab.id === activeTab}
                onClick={() => setActiveTab(tab.id)}
              >
                {tab.id}
              </button>
            ))}
          </nav>

          <div className="view-card">
            <div className="view-card__header">
              <h2>{selectedEntity}</h2>
              <div className="view-card__meta">
                <span>VNUM range {selection.vnumRange}</span>
                <span>Last save {selection.lastSave}</span>
                <span>Active view {activeTab}</span>
              </div>
            </div>
            <div className="view-card__body">
              {activeTab === "Form" && selectedEntity === "Rooms" ? (
                roomRows.length ? (
                  <div className="form-view">
                    <form
                      className="form-shell"
                      onSubmit={handleRoomSubmitForm(handleRoomSubmit)}
                    >
                      <div className="form-grid">
                        <div className="form-field">
                          <label className="form-label" htmlFor="room-vnum">
                            VNUM
                          </label>
                          <input
                            id="room-vnum"
                            className="form-input"
                            type="number"
                            readOnly
                            {...registerRoom("vnum", { valueAsNumber: true })}
                          />
                        </div>
                        <div className="form-field form-field--wide">
                          <label className="form-label" htmlFor="room-name">
                            Name
                          </label>
                          <input
                            id="room-name"
                            className="form-input"
                            type="text"
                            {...registerRoom("name")}
                          />
                          {roomFormState.errors.name ? (
                            <span className="form-error">
                              {roomFormState.errors.name.message}
                            </span>
                          ) : null}
                        </div>
                        <div className="form-field">
                          <label
                            className="form-label"
                            htmlFor="room-sector"
                          >
                            Sector
                          </label>
                          <select
                            id="room-sector"
                            className="form-select"
                            {...registerRoom("sectorType")}
                          >
                            <option value="">Default (inside)</option>
                            {sectors.map((sector) => (
                              <option key={sector} value={sector}>
                                {sector}
                              </option>
                            ))}
                          </select>
                        </div>
                        <div className="form-field">
                          <label className="form-label" htmlFor="room-mana">
                            Mana Rate
                          </label>
                          <input
                            id="room-mana"
                            className="form-input"
                            type="number"
                            {...registerRoom("manaRate")}
                          />
                        </div>
                        <div className="form-field">
                          <label className="form-label" htmlFor="room-heal">
                            Heal Rate
                          </label>
                          <input
                            id="room-heal"
                            className="form-input"
                            type="number"
                            {...registerRoom("healRate")}
                          />
                        </div>
                        <div className="form-field">
                          <label className="form-label" htmlFor="room-clan">
                            Clan
                          </label>
                          <input
                            id="room-clan"
                            className="form-input"
                            type="number"
                            {...registerRoom("clan")}
                          />
                        </div>
                        <div className="form-field form-field--wide">
                          <label className="form-label" htmlFor="room-owner">
                            Owner
                          </label>
                          <input
                            id="room-owner"
                            className="form-input"
                            type="text"
                            {...registerRoom("owner")}
                          />
                        </div>
                      </div>
                      <div className="form-field form-field--full">
                        <label
                          className="form-label"
                          htmlFor="room-description"
                        >
                          Description
                        </label>
                        <textarea
                          id="room-description"
                          className="form-textarea"
                          rows={6}
                          {...registerRoom("description")}
                        />
                        {roomFormState.errors.description ? (
                          <span className="form-error">
                            {roomFormState.errors.description.message}
                          </span>
                        ) : null}
                      </div>
                      <div className="form-field form-field--full">
                        <label className="form-label">Room Flags</label>
                        <div className="form-checkboxes">
                          {roomFlagOptions.map((flag) => (
                            <label key={flag} className="checkbox-pill">
                              <input
                                type="checkbox"
                                value={flag}
                                {...registerRoom("roomFlags")}
                              />
                              <span>{flag}</span>
                            </label>
                          ))}
                        </div>
                      </div>
                      <div className="form-field form-field--full">
                        <div className="form-section-header">
                          <div>
                            <div className="form-label">Exits</div>
                            <div className="form-hint">
                              {exitFields.length
                                ? `${exitFields.length} exit${
                                    exitFields.length === 1 ? "" : "s"
                                  }`
                                : "No exits defined"}
                            </div>
                          </div>
                          <button
                            className="ghost-button"
                            type="button"
                            onClick={() =>
                              appendExit({
                                dir: directions[0],
                                toVnum: 0,
                                key: undefined,
                                flags: [],
                                description: "",
                                keyword: ""
                              })
                            }
                          >
                            Add Exit
                          </button>
                        </div>
                        <div className="exit-list">
                          {exitFields.map((field, index) => (
                            <div key={field.id} className="exit-card">
                              <div className="exit-card__header">
                                <span>Exit {index + 1}</span>
                                <button
                                  className="ghost-button"
                                  type="button"
                                  onClick={() => removeExit(index)}
                                >
                                  Remove
                                </button>
                              </div>
                              <div className="exit-grid">
                                <div className="form-field">
                                  <label
                                    className="form-label"
                                    htmlFor={`exit-dir-${field.id}`}
                                  >
                                    Direction
                                  </label>
                                  <select
                                    id={`exit-dir-${field.id}`}
                                    className="form-select"
                                    {...registerRoom(`exits.${index}.dir`)}
                                  >
                                    {directions.map((dir) => (
                                      <option key={dir} value={dir}>
                                        {dir}
                                      </option>
                                    ))}
                                  </select>
                                </div>
                                <VnumPicker<RoomFormValues>
                                  id={`exit-to-${field.id}`}
                                  label="To VNUM"
                                  name={`exits.${index}.toVnum` as FieldPath<RoomFormValues>}
                                  register={registerRoom}
                                  options={roomVnumOptions}
                                  error={
                                    roomFormState.errors.exits?.[index]?.toVnum
                                      ?.message
                                  }
                                />
                                <VnumPicker<RoomFormValues>
                                  id={`exit-key-${field.id}`}
                                  label="Key VNUM"
                                  name={`exits.${index}.key` as FieldPath<RoomFormValues>}
                                  register={registerRoom}
                                  options={objectVnumOptions}
                                />
                                <div className="form-field">
                                  <label
                                    className="form-label"
                                    htmlFor={`exit-keyword-${field.id}`}
                                  >
                                    Keyword
                                  </label>
                                  <input
                                    id={`exit-keyword-${field.id}`}
                                    className="form-input"
                                    type="text"
                                    {...registerRoom(`exits.${index}.keyword`)}
                                  />
                                </div>
                                <div className="form-field form-field--full">
                                  <label
                                    className="form-label"
                                    htmlFor={`exit-description-${field.id}`}
                                  >
                                    Description
                                  </label>
                                  <textarea
                                    id={`exit-description-${field.id}`}
                                    className="form-textarea"
                                    rows={3}
                                    {...registerRoom(
                                      `exits.${index}.description`
                                    )}
                                  />
                                </div>
                                <div className="form-field form-field--full">
                                  <label className="form-label">
                                    Exit Flags
                                  </label>
                                  <div className="form-checkboxes">
                                    {exitFlags.map((flag) => (
                                      <label
                                        key={flag}
                                        className="checkbox-pill"
                                      >
                                        <input
                                          type="checkbox"
                                          value={flag}
                                          {...registerRoom(
                                            `exits.${index}.flags`
                                          )}
                                        />
                                        <span>{flag}</span>
                                      </label>
                                    ))}
                                  </div>
                                </div>
                              </div>
                            </div>
                          ))}
                        </div>
                      </div>
                      <div className="form-actions">
                        <button
                          className="action-button action-button--primary"
                          type="submit"
                          disabled={!roomFormState.isDirty}
                        >
                          Apply Changes
                        </button>
                        <span className="form-hint">
                          {roomFormState.isDirty
                            ? "Unsaved changes"
                            : "Up to date"}
                        </span>
                      </div>
                    </form>
                  </div>
                ) : (
                  <div className="entity-table__empty">
                    <h3>No rooms available</h3>
                    <p>Load an area JSON file to edit room data.</p>
                  </div>
                )
              ) : activeTab === "Form" && selectedEntity === "Mobiles" ? (
                mobileRows.length ? (
                  <div className="form-view">
                    <form
                      className="form-shell"
                      onSubmit={handleMobileSubmitForm(handleMobileSubmit)}
                    >
                      <div className="form-grid">
                        <div className="form-field">
                          <label className="form-label" htmlFor="mob-vnum">
                            VNUM
                          </label>
                          <input
                            id="mob-vnum"
                            className="form-input"
                            type="number"
                            readOnly
                            {...registerMobile("vnum", { valueAsNumber: true })}
                          />
                        </div>
                        <div className="form-field form-field--wide">
                          <label className="form-label" htmlFor="mob-name">
                            Name
                          </label>
                          <input
                            id="mob-name"
                            className="form-input"
                            type="text"
                            {...registerMobile("name")}
                          />
                          {mobileFormState.errors.name ? (
                            <span className="form-error">
                              {mobileFormState.errors.name.message}
                            </span>
                          ) : null}
                        </div>
                        <div className="form-field form-field--wide">
                          <label
                            className="form-label"
                            htmlFor="mob-short"
                          >
                            Short Description
                          </label>
                          <input
                            id="mob-short"
                            className="form-input"
                            type="text"
                            {...registerMobile("shortDescr")}
                          />
                          {mobileFormState.errors.shortDescr ? (
                            <span className="form-error">
                              {mobileFormState.errors.shortDescr.message}
                            </span>
                          ) : null}
                        </div>
                        <div className="form-field form-field--wide">
                          <label
                            className="form-label"
                            htmlFor="mob-long"
                          >
                            Long Description
                          </label>
                          <input
                            id="mob-long"
                            className="form-input"
                            type="text"
                            {...registerMobile("longDescr")}
                          />
                          {mobileFormState.errors.longDescr ? (
                            <span className="form-error">
                              {mobileFormState.errors.longDescr.message}
                            </span>
                          ) : null}
                        </div>
                        <div className="form-field">
                          <label className="form-label" htmlFor="mob-race">
                            Race
                          </label>
                          <input
                            id="mob-race"
                            className="form-input"
                            type="text"
                            {...registerMobile("race")}
                          />
                          {mobileFormState.errors.race ? (
                            <span className="form-error">
                              {mobileFormState.errors.race.message}
                            </span>
                          ) : null}
                        </div>
                        <div className="form-field">
                          <label className="form-label" htmlFor="mob-level">
                            Level
                          </label>
                          <input
                            id="mob-level"
                            className="form-input"
                            type="number"
                            {...registerMobile("level")}
                          />
                        </div>
                        <div className="form-field">
                          <label className="form-label" htmlFor="mob-hitroll">
                            Hitroll
                          </label>
                          <input
                            id="mob-hitroll"
                            className="form-input"
                            type="number"
                            {...registerMobile("hitroll")}
                          />
                        </div>
                        <div className="form-field">
                          <label
                            className="form-label"
                            htmlFor="mob-alignment"
                          >
                            Alignment
                          </label>
                          <input
                            id="mob-alignment"
                            className="form-input"
                            type="number"
                            {...registerMobile("alignment")}
                          />
                        </div>
                        <div className="form-field">
                          <label className="form-label" htmlFor="mob-damtype">
                            Damage Type
                          </label>
                          <select
                            id="mob-damtype"
                            className="form-select"
                            {...registerMobile("damType")}
                          >
                            <option value="">Default</option>
                            {damageTypes.map((damageType) => (
                              <option key={damageType} value={damageType}>
                                {damageType}
                              </option>
                            ))}
                          </select>
                        </div>
                        <div className="form-field">
                          <label className="form-label" htmlFor="mob-startpos">
                            Start Pos
                          </label>
                          <select
                            id="mob-startpos"
                            className="form-select"
                            {...registerMobile("startPos")}
                          >
                            <option value="">Default</option>
                            {positions.map((position) => (
                              <option key={position} value={position}>
                                {position}
                              </option>
                            ))}
                          </select>
                        </div>
                        <div className="form-field">
                          <label
                            className="form-label"
                            htmlFor="mob-defaultpos"
                          >
                            Default Pos
                          </label>
                          <select
                            id="mob-defaultpos"
                            className="form-select"
                            {...registerMobile("defaultPos")}
                          >
                            <option value="">Default</option>
                            {positions.map((position) => (
                              <option key={position} value={position}>
                                {position}
                              </option>
                            ))}
                          </select>
                        </div>
                        <div className="form-field">
                          <label className="form-label" htmlFor="mob-sex">
                            Sex
                          </label>
                          <select
                            id="mob-sex"
                            className="form-select"
                            {...registerMobile("sex")}
                          >
                            <option value="">Default</option>
                            {sexes.map((sex) => (
                              <option key={sex} value={sex}>
                                {sex}
                              </option>
                            ))}
                          </select>
                        </div>
                        <div className="form-field">
                          <label className="form-label" htmlFor="mob-size">
                            Size
                          </label>
                          <select
                            id="mob-size"
                            className="form-select"
                            {...registerMobile("size")}
                          >
                            <option value="">Default</option>
                            {sizes.map((size) => (
                              <option key={size} value={size}>
                                {size}
                              </option>
                            ))}
                          </select>
                        </div>
                        <div className="form-field">
                          <label className="form-label" htmlFor="mob-material">
                            Material
                          </label>
                          <input
                            id="mob-material"
                            className="form-input"
                            type="text"
                            {...registerMobile("material")}
                          />
                        </div>
                        <div className="form-field">
                          <label
                            className="form-label"
                            htmlFor="mob-faction"
                          >
                            Faction VNUM
                          </label>
                          <input
                            id="mob-faction"
                            className="form-input"
                            type="number"
                            {...registerMobile("factionVnum")}
                          />
                        </div>
                        <div className="form-field">
                          <label
                            className="form-label"
                            htmlFor="mob-damage-noun"
                          >
                            Damage Noun
                          </label>
                          <input
                            id="mob-damage-noun"
                            className="form-input"
                            type="text"
                            {...registerMobile("damageNoun")}
                          />
                        </div>
                        <div className="form-field">
                          <label
                            className="form-label"
                            htmlFor="mob-offensive-spell"
                          >
                            Offensive Spell
                          </label>
                          <input
                            id="mob-offensive-spell"
                            className="form-input"
                            type="text"
                            {...registerMobile("offensiveSpell")}
                          />
                        </div>
                      </div>
                      <div className="form-field form-field--full">
                        <label
                          className="form-label"
                          htmlFor="mob-description"
                        >
                          Description
                        </label>
                        <textarea
                          id="mob-description"
                          className="form-textarea"
                          rows={6}
                          {...registerMobile("description")}
                        />
                        {mobileFormState.errors.description ? (
                          <span className="form-error">
                            {mobileFormState.errors.description.message}
                          </span>
                        ) : null}
                      </div>
                      <div className="form-field form-field--full">
                        <div className="form-section-header">
                          <div>
                            <div className="form-label">Dice</div>
                            <div className="form-hint">
                              Number, type, and optional bonus.
                            </div>
                          </div>
                        </div>
                        <div className="dice-grid">
                          <div className="dice-card">
                            <div className="dice-card__title">Hit Dice</div>
                            <div className="dice-fields">
                              <div className="form-field">
                                <label className="form-label">Number</label>
                                <input
                                  className="form-input"
                                  type="number"
                                  {...registerMobile("hitDice.number")}
                                />
                              </div>
                              <div className="form-field">
                                <label className="form-label">Type</label>
                                <input
                                  className="form-input"
                                  type="number"
                                  {...registerMobile("hitDice.type")}
                                />
                              </div>
                              <div className="form-field">
                                <label className="form-label">Bonus</label>
                                <input
                                  className="form-input"
                                  type="number"
                                  {...registerMobile("hitDice.bonus")}
                                />
                              </div>
                            </div>
                          </div>
                          <div className="dice-card">
                            <div className="dice-card__title">Mana Dice</div>
                            <div className="dice-fields">
                              <div className="form-field">
                                <label className="form-label">Number</label>
                                <input
                                  className="form-input"
                                  type="number"
                                  {...registerMobile("manaDice.number")}
                                />
                              </div>
                              <div className="form-field">
                                <label className="form-label">Type</label>
                                <input
                                  className="form-input"
                                  type="number"
                                  {...registerMobile("manaDice.type")}
                                />
                              </div>
                              <div className="form-field">
                                <label className="form-label">Bonus</label>
                                <input
                                  className="form-input"
                                  type="number"
                                  {...registerMobile("manaDice.bonus")}
                                />
                              </div>
                            </div>
                          </div>
                          <div className="dice-card">
                            <div className="dice-card__title">Damage Dice</div>
                            <div className="dice-fields">
                              <div className="form-field">
                                <label className="form-label">Number</label>
                                <input
                                  className="form-input"
                                  type="number"
                                  {...registerMobile("damageDice.number")}
                                />
                              </div>
                              <div className="form-field">
                                <label className="form-label">Type</label>
                                <input
                                  className="form-input"
                                  type="number"
                                  {...registerMobile("damageDice.type")}
                                />
                              </div>
                              <div className="form-field">
                                <label className="form-label">Bonus</label>
                                <input
                                  className="form-input"
                                  type="number"
                                  {...registerMobile("damageDice.bonus")}
                                />
                              </div>
                            </div>
                          </div>
                        </div>
                      </div>
                      <div className="form-actions">
                        <button
                          className="action-button action-button--primary"
                          type="submit"
                          disabled={!mobileFormState.isDirty}
                        >
                          Apply Changes
                        </button>
                        <span className="form-hint">
                          {mobileFormState.isDirty
                            ? "Unsaved changes"
                            : "Up to date"}
                        </span>
                      </div>
                    </form>
                  </div>
                ) : (
                  <div className="entity-table__empty">
                    <h3>No mobiles available</h3>
                    <p>Load an area JSON file to edit mobile data.</p>
                  </div>
                )
              ) : activeTab === "Form" && selectedEntity === "Objects" ? (
                objectRows.length ? (
                  <div className="form-view">
                    <form
                      className="form-shell"
                      onSubmit={handleObjectSubmitForm(handleObjectSubmit)}
                    >
                      <div className="form-grid">
                        <div className="form-field">
                          <label className="form-label" htmlFor="obj-vnum">
                            VNUM
                          </label>
                          <input
                            id="obj-vnum"
                            className="form-input"
                            type="number"
                            readOnly
                            {...registerObject("vnum", { valueAsNumber: true })}
                          />
                        </div>
                        <div className="form-field form-field--wide">
                          <label className="form-label" htmlFor="obj-name">
                            Name
                          </label>
                          <input
                            id="obj-name"
                            className="form-input"
                            type="text"
                            {...registerObject("name")}
                          />
                          {objectFormState.errors.name ? (
                            <span className="form-error">
                              {objectFormState.errors.name.message}
                            </span>
                          ) : null}
                        </div>
                        <div className="form-field form-field--wide">
                          <label className="form-label" htmlFor="obj-short">
                            Short Description
                          </label>
                          <input
                            id="obj-short"
                            className="form-input"
                            type="text"
                            {...registerObject("shortDescr")}
                          />
                          {objectFormState.errors.shortDescr ? (
                            <span className="form-error">
                              {objectFormState.errors.shortDescr.message}
                            </span>
                          ) : null}
                        </div>
                        <div className="form-field">
                          <label className="form-label" htmlFor="obj-itemtype">
                            Item Type
                          </label>
                          <select
                            id="obj-itemtype"
                            className="form-select"
                            {...registerObject("itemType")}
                          >
                            <option value="">Select</option>
                            {itemTypeOptions.map((itemType) => (
                              <option key={itemType} value={itemType}>
                                {itemType}
                              </option>
                            ))}
                          </select>
                          {objectFormState.errors.itemType ? (
                            <span className="form-error">
                              {objectFormState.errors.itemType.message}
                            </span>
                          ) : null}
                        </div>
                        <div className="form-field">
                          <label className="form-label" htmlFor="obj-material">
                            Material
                          </label>
                          <input
                            id="obj-material"
                            className="form-input"
                            type="text"
                            {...registerObject("material")}
                          />
                          {objectFormState.errors.material ? (
                            <span className="form-error">
                              {objectFormState.errors.material.message}
                            </span>
                          ) : null}
                        </div>
                        <div className="form-field">
                          <label className="form-label" htmlFor="obj-level">
                            Level
                          </label>
                          <input
                            id="obj-level"
                            className="form-input"
                            type="number"
                            {...registerObject("level")}
                          />
                        </div>
                        <div className="form-field">
                          <label className="form-label" htmlFor="obj-weight">
                            Weight
                          </label>
                          <input
                            id="obj-weight"
                            className="form-input"
                            type="number"
                            {...registerObject("weight")}
                          />
                        </div>
                        <div className="form-field">
                          <label className="form-label" htmlFor="obj-cost">
                            Cost
                          </label>
                          <input
                            id="obj-cost"
                            className="form-input"
                            type="number"
                            {...registerObject("cost")}
                          />
                        </div>
                        <div className="form-field">
                          <label className="form-label" htmlFor="obj-condition">
                            Condition
                          </label>
                          <input
                            id="obj-condition"
                            className="form-input"
                            type="number"
                            {...registerObject("condition")}
                          />
                        </div>
                      </div>
                      <div className="form-field form-field--full">
                        <label
                          className="form-label"
                          htmlFor="obj-description"
                        >
                          Description
                        </label>
                        <textarea
                          id="obj-description"
                          className="form-textarea"
                          rows={6}
                          {...registerObject("description")}
                        />
                        {objectFormState.errors.description ? (
                          <span className="form-error">
                            {objectFormState.errors.description.message}
                          </span>
                        ) : null}
                      </div>
                      <div className="form-field form-field--full">
                        <label className="form-label">Wear Flags</label>
                        <div className="form-checkboxes">
                          {wearFlags.map((flag) => (
                            <label key={flag} className="checkbox-pill">
                              <input
                                type="checkbox"
                                value={flag}
                                {...registerObject("wearFlags")}
                              />
                              <span>{flag}</span>
                            </label>
                          ))}
                        </div>
                      </div>
                      <div className="form-field form-field--full">
                        <label className="form-label">Extra Flags</label>
                        <div className="form-checkboxes">
                          {extraFlags.map((flag) => (
                            <label key={flag} className="checkbox-pill">
                              <input
                                type="checkbox"
                                value={flag}
                                {...registerObject("extraFlags")}
                              />
                              <span>{flag}</span>
                            </label>
                          ))}
                        </div>
                      </div>
                      <div className="form-field form-field--full">
                        <div className="form-section-header">
                          <div>
                            <div className="form-label">Typed Block</div>
                            <div className="form-hint">
                              Determined by item type.
                            </div>
                          </div>
                        </div>
                        <div className="block-list">
                          {!activeObjectBlock ? (
                            <div className="placeholder-block">
                              <div className="placeholder-title">
                                No typed block
                              </div>
                              <p>
                                Choose an item type with typed data to edit.
                              </p>
                            </div>
                          ) : null}
                          {activeObjectBlock === "weapon" ? (
                            <div className="block-card">
                              <div className="block-card__header">
                                <span>Weapon</span>
                              </div>
                              <div className="block-grid">
                                <div className="form-field">
                                  <label className="form-label">
                                    Weapon Class
                                  </label>
                                  <select
                                    className="form-select"
                                    {...registerObject("weapon.class")}
                                  >
                                    <option value="">Select</option>
                                    {weaponClasses.map((weaponClass) => (
                                      <option
                                        key={weaponClass}
                                        value={weaponClass}
                                      >
                                        {weaponClass}
                                      </option>
                                    ))}
                                  </select>
                                </div>
                                <div className="form-field">
                                  <label className="form-label">Dice #</label>
                                  <input
                                    className="form-input"
                                    type="number"
                                    {...registerObject("weapon.diceNumber")}
                                  />
                                </div>
                                <div className="form-field">
                                  <label className="form-label">Dice Type</label>
                                  <input
                                    className="form-input"
                                    type="number"
                                    {...registerObject("weapon.diceType")}
                                  />
                                </div>
                                <div className="form-field">
                                  <label className="form-label">
                                    Damage Type
                                  </label>
                                  <select
                                    className="form-select"
                                    {...registerObject("weapon.damageType")}
                                  >
                                    <option value="">Select</option>
                                    {damageTypes.map((damageType) => (
                                      <option
                                        key={damageType}
                                        value={damageType}
                                      >
                                        {damageType}
                                      </option>
                                    ))}
                                  </select>
                                </div>
                                <div className="form-field form-field--full">
                                  <label className="form-label">
                                    Weapon Flags
                                  </label>
                                  <div className="form-checkboxes">
                                    {weaponFlags.map((flag) => (
                                      <label
                                        key={flag}
                                        className="checkbox-pill"
                                      >
                                        <input
                                          type="checkbox"
                                          value={flag}
                                          {...registerObject("weapon.flags")}
                                        />
                                        <span>{flag}</span>
                                      </label>
                                    ))}
                                  </div>
                                </div>
                              </div>
                            </div>
                          ) : null}
                          {activeObjectBlock === "armor" ? (
                            <div className="block-card">
                              <div className="block-card__header">
                                <span>Armor</span>
                              </div>
                              <div className="block-grid">
                                <div className="form-field">
                                  <label className="form-label">AC Pierce</label>
                                  <input
                                    className="form-input"
                                    type="number"
                                    {...registerObject("armor.acPierce")}
                                  />
                                </div>
                                <div className="form-field">
                                  <label className="form-label">AC Bash</label>
                                  <input
                                    className="form-input"
                                    type="number"
                                    {...registerObject("armor.acBash")}
                                  />
                                </div>
                                <div className="form-field">
                                  <label className="form-label">AC Slash</label>
                                  <input
                                    className="form-input"
                                    type="number"
                                    {...registerObject("armor.acSlash")}
                                  />
                                </div>
                                <div className="form-field">
                                  <label className="form-label">AC Exotic</label>
                                  <input
                                    className="form-input"
                                    type="number"
                                    {...registerObject("armor.acExotic")}
                                  />
                                </div>
                              </div>
                            </div>
                          ) : null}
                          {activeObjectBlock === "container" ? (
                            <div className="block-card">
                              <div className="block-card__header">
                                <span>Container</span>
                              </div>
                              <div className="block-grid">
                                <div className="form-field">
                                  <label className="form-label">Capacity</label>
                                  <input
                                    className="form-input"
                                    type="number"
                                    {...registerObject("container.capacity")}
                                  />
                                </div>
                                <VnumPicker<ObjectFormValues>
                                  id="object-container-key"
                                  label="Key VNUM"
                                  name={
                                    "container.keyVnum" as FieldPath<ObjectFormValues>
                                  }
                                  register={registerObject}
                                  options={objectVnumOptions}
                                  error={
                                    objectFormState.errors.container?.keyVnum
                                      ?.message
                                  }
                                />
                                <div className="form-field">
                                  <label className="form-label">Max Weight</label>
                                  <input
                                    className="form-input"
                                    type="number"
                                    {...registerObject("container.maxWeight")}
                                  />
                                </div>
                                <div className="form-field">
                                  <label className="form-label">Weight Mult</label>
                                  <input
                                    className="form-input"
                                    type="number"
                                    {...registerObject("container.weightMult")}
                                  />
                                </div>
                                <div className="form-field form-field--full">
                                  <label className="form-label">
                                    Container Flags
                                  </label>
                                  <div className="form-checkboxes">
                                    {containerFlags.map((flag) => (
                                      <label
                                        key={flag}
                                        className="checkbox-pill"
                                      >
                                        <input
                                          type="checkbox"
                                          value={flag}
                                          {...registerObject("container.flags")}
                                        />
                                        <span>{flag}</span>
                                      </label>
                                    ))}
                                  </div>
                                </div>
                              </div>
                            </div>
                          ) : null}
                          {activeObjectBlock === "light" ? (
                            <div className="block-card">
                              <div className="block-card__header">
                                <span>Light</span>
                              </div>
                              <div className="block-grid">
                                <div className="form-field">
                                  <label className="form-label">Hours</label>
                                  <input
                                    className="form-input"
                                    type="number"
                                    {...registerObject("light.hours")}
                                  />
                                </div>
                              </div>
                            </div>
                          ) : null}
                          {activeObjectBlock === "drink" ? (
                            <div className="block-card">
                              <div className="block-card__header">
                                <span>Drink</span>
                              </div>
                              <div className="block-grid">
                                <div className="form-field">
                                  <label className="form-label">Capacity</label>
                                  <input
                                    className="form-input"
                                    type="number"
                                    {...registerObject("drink.capacity")}
                                  />
                                </div>
                                <div className="form-field">
                                  <label className="form-label">Remaining</label>
                                  <input
                                    className="form-input"
                                    type="number"
                                    {...registerObject("drink.remaining")}
                                  />
                                </div>
                                <div className="form-field">
                                  <label className="form-label">Liquid</label>
                                  <select
                                    className="form-select"
                                    {...registerObject("drink.liquid")}
                                  >
                                    <option value="">Select</option>
                                    {liquids.map((liquid) => (
                                      <option key={liquid} value={liquid}>
                                        {liquid}
                                      </option>
                                    ))}
                                  </select>
                                </div>
                                <div className="form-field">
                                  <label className="form-label">Poisoned</label>
                                  <label className="checkbox-pill">
                                    <input
                                      type="checkbox"
                                      {...registerObject("drink.poisoned")}
                                    />
                                    <span>Poisoned</span>
                                  </label>
                                </div>
                              </div>
                            </div>
                          ) : null}
                          {activeObjectBlock === "fountain" ? (
                            <div className="block-card">
                              <div className="block-card__header">
                                <span>Fountain</span>
                              </div>
                              <div className="block-grid">
                                <div className="form-field">
                                  <label className="form-label">Capacity</label>
                                  <input
                                    className="form-input"
                                    type="number"
                                    {...registerObject("fountain.capacity")}
                                  />
                                </div>
                                <div className="form-field">
                                  <label className="form-label">Remaining</label>
                                  <input
                                    className="form-input"
                                    type="number"
                                    {...registerObject("fountain.remaining")}
                                  />
                                </div>
                                <div className="form-field">
                                  <label className="form-label">Liquid</label>
                                  <select
                                    className="form-select"
                                    {...registerObject("fountain.liquid")}
                                  >
                                    <option value="">Select</option>
                                    {liquids.map((liquid) => (
                                      <option key={liquid} value={liquid}>
                                        {liquid}
                                      </option>
                                    ))}
                                  </select>
                                </div>
                                <div className="form-field">
                                  <label className="form-label">Poisoned</label>
                                  <label className="checkbox-pill">
                                    <input
                                      type="checkbox"
                                      {...registerObject("fountain.poisoned")}
                                    />
                                    <span>Poisoned</span>
                                  </label>
                                </div>
                              </div>
                            </div>
                          ) : null}
                          {activeObjectBlock === "food" ? (
                            <div className="block-card">
                              <div className="block-card__header">
                                <span>Food</span>
                              </div>
                              <div className="block-grid">
                                <div className="form-field">
                                  <label className="form-label">Food Hours</label>
                                  <input
                                    className="form-input"
                                    type="number"
                                    {...registerObject("food.foodHours")}
                                  />
                                </div>
                                <div className="form-field">
                                  <label className="form-label">Full Hours</label>
                                  <input
                                    className="form-input"
                                    type="number"
                                    {...registerObject("food.fullHours")}
                                  />
                                </div>
                                <div className="form-field">
                                  <label className="form-label">Poisoned</label>
                                  <label className="checkbox-pill">
                                    <input
                                      type="checkbox"
                                      {...registerObject("food.poisoned")}
                                    />
                                    <span>Poisoned</span>
                                  </label>
                                </div>
                              </div>
                            </div>
                          ) : null}
                          {activeObjectBlock === "money" ? (
                            <div className="block-card">
                              <div className="block-card__header">
                                <span>Money</span>
                              </div>
                              <div className="block-grid">
                                <div className="form-field">
                                  <label className="form-label">Gold</label>
                                  <input
                                    className="form-input"
                                    type="number"
                                    {...registerObject("money.gold")}
                                  />
                                </div>
                                <div className="form-field">
                                  <label className="form-label">Silver</label>
                                  <input
                                    className="form-input"
                                    type="number"
                                    {...registerObject("money.silver")}
                                  />
                                </div>
                              </div>
                            </div>
                          ) : null}
                          {activeObjectBlock === "wand" ? (
                            <div className="block-card">
                              <div className="block-card__header">
                                <span>Wand</span>
                              </div>
                              <div className="block-grid">
                                <div className="form-field">
                                  <label className="form-label">Level</label>
                                  <input
                                    className="form-input"
                                    type="number"
                                    {...registerObject("wand.level")}
                                  />
                                </div>
                                <div className="form-field">
                                  <label className="form-label">
                                    Total Charges
                                  </label>
                                  <input
                                    className="form-input"
                                    type="number"
                                    {...registerObject("wand.totalCharges")}
                                  />
                                </div>
                                <div className="form-field">
                                  <label className="form-label">
                                    Charges Left
                                  </label>
                                  <input
                                    className="form-input"
                                    type="number"
                                    {...registerObject("wand.chargesLeft")}
                                  />
                                </div>
                                <div className="form-field form-field--wide">
                                  <label className="form-label">Spell</label>
                                  <input
                                    className="form-input"
                                    type="text"
                                    {...registerObject("wand.spell")}
                                  />
                                </div>
                              </div>
                            </div>
                          ) : null}
                          {activeObjectBlock === "staff" ? (
                            <div className="block-card">
                              <div className="block-card__header">
                                <span>Staff</span>
                              </div>
                              <div className="block-grid">
                                <div className="form-field">
                                  <label className="form-label">Level</label>
                                  <input
                                    className="form-input"
                                    type="number"
                                    {...registerObject("staff.level")}
                                  />
                                </div>
                                <div className="form-field">
                                  <label className="form-label">
                                    Total Charges
                                  </label>
                                  <input
                                    className="form-input"
                                    type="number"
                                    {...registerObject("staff.totalCharges")}
                                  />
                                </div>
                                <div className="form-field">
                                  <label className="form-label">
                                    Charges Left
                                  </label>
                                  <input
                                    className="form-input"
                                    type="number"
                                    {...registerObject("staff.chargesLeft")}
                                  />
                                </div>
                                <div className="form-field form-field--wide">
                                  <label className="form-label">Spell</label>
                                  <input
                                    className="form-input"
                                    type="text"
                                    {...registerObject("staff.spell")}
                                  />
                                </div>
                              </div>
                            </div>
                          ) : null}
                          {activeObjectBlock === "spells" ? (
                            <div className="block-card">
                              <div className="block-card__header">
                                <span>Spells</span>
                              </div>
                              <div className="block-grid">
                                <div className="form-field">
                                  <label className="form-label">Level</label>
                                  <input
                                    className="form-input"
                                    type="number"
                                    {...registerObject("spells.level")}
                                  />
                                </div>
                                <div className="form-field">
                                  <label className="form-label">Spell 1</label>
                                  <input
                                    className="form-input"
                                    type="text"
                                    {...registerObject("spells.spell1")}
                                  />
                                </div>
                                <div className="form-field">
                                  <label className="form-label">Spell 2</label>
                                  <input
                                    className="form-input"
                                    type="text"
                                    {...registerObject("spells.spell2")}
                                  />
                                </div>
                                <div className="form-field">
                                  <label className="form-label">Spell 3</label>
                                  <input
                                    className="form-input"
                                    type="text"
                                    {...registerObject("spells.spell3")}
                                  />
                                </div>
                                <div className="form-field">
                                  <label className="form-label">Spell 4</label>
                                  <input
                                    className="form-input"
                                    type="text"
                                    {...registerObject("spells.spell4")}
                                  />
                                </div>
                              </div>
                            </div>
                          ) : null}
                          {activeObjectBlock === "portal" ? (
                            <div className="block-card">
                              <div className="block-card__header">
                                <span>Portal</span>
                              </div>
                              <div className="block-grid">
                                <div className="form-field">
                                  <label className="form-label">Charges</label>
                                  <input
                                    className="form-input"
                                    type="number"
                                    {...registerObject("portal.charges")}
                                  />
                                </div>
                                <VnumPicker<ObjectFormValues>
                                  id="object-portal-to"
                                  label="To VNUM"
                                  name={
                                    "portal.toVnum" as FieldPath<ObjectFormValues>
                                  }
                                  register={registerObject}
                                  options={roomVnumOptions}
                                  error={
                                    objectFormState.errors.portal?.toVnum?.message
                                  }
                                />
                                <div className="form-field form-field--full">
                                  <label className="form-label">Exit Flags</label>
                                  <div className="form-checkboxes">
                                    {exitFlags.map((flag) => (
                                      <label
                                        key={flag}
                                        className="checkbox-pill"
                                      >
                                        <input
                                          type="checkbox"
                                          value={flag}
                                          {...registerObject("portal.exitFlags")}
                                        />
                                        <span>{flag}</span>
                                      </label>
                                    ))}
                                  </div>
                                </div>
                                <div className="form-field form-field--full">
                                  <label className="form-label">
                                    Portal Flags
                                  </label>
                                  <div className="form-checkboxes">
                                    {portalFlags.map((flag) => (
                                      <label
                                        key={flag}
                                        className="checkbox-pill"
                                      >
                                        <input
                                          type="checkbox"
                                          value={flag}
                                          {...registerObject(
                                            "portal.portalFlags"
                                          )}
                                        />
                                        <span>{flag}</span>
                                      </label>
                                    ))}
                                  </div>
                                </div>
                              </div>
                            </div>
                          ) : null}
                          {activeObjectBlock === "furniture" ? (
                            <div className="block-card">
                              <div className="block-card__header">
                                <span>Furniture</span>
                              </div>
                              <div className="block-grid">
                                <div className="form-field">
                                  <label className="form-label">Slots</label>
                                  <input
                                    className="form-input"
                                    type="number"
                                    {...registerObject("furniture.slots")}
                                  />
                                </div>
                                <div className="form-field">
                                  <label className="form-label">Weight</label>
                                  <input
                                    className="form-input"
                                    type="number"
                                    {...registerObject("furniture.weight")}
                                  />
                                </div>
                                <div className="form-field">
                                  <label className="form-label">Heal Bonus</label>
                                  <input
                                    className="form-input"
                                    type="number"
                                    {...registerObject("furniture.healBonus")}
                                  />
                                </div>
                                <div className="form-field">
                                  <label className="form-label">Mana Bonus</label>
                                  <input
                                    className="form-input"
                                    type="number"
                                    {...registerObject("furniture.manaBonus")}
                                  />
                                </div>
                                <div className="form-field">
                                  <label className="form-label">Max People</label>
                                  <input
                                    className="form-input"
                                    type="number"
                                    {...registerObject("furniture.maxPeople")}
                                  />
                                </div>
                                <div className="form-field form-field--full">
                                  <label className="form-label">
                                    Furniture Flags
                                  </label>
                                  <div className="form-checkboxes">
                                    {furnitureFlags.map((flag) => (
                                      <label
                                        key={flag}
                                        className="checkbox-pill"
                                      >
                                        <input
                                          type="checkbox"
                                          value={flag}
                                          {...registerObject("furniture.flags")}
                                        />
                                        <span>{flag}</span>
                                      </label>
                                    ))}
                                  </div>
                                </div>
                              </div>
                            </div>
                          ) : null}
                        </div>
                      </div>
                      <div className="form-actions">
                        <button
                          className="action-button action-button--primary"
                          type="submit"
                          disabled={!objectFormState.isDirty}
                        >
                          Apply Changes
                        </button>
                        <span className="form-hint">
                          {objectFormState.isDirty
                            ? "Unsaved changes"
                            : "Up to date"}
                        </span>
                      </div>
                    </form>
                  </div>
                ) : (
                  <div className="entity-table__empty">
                    <h3>No objects available</h3>
                    <p>Load an area JSON file to edit object data.</p>
                  </div>
                )
              ) : activeTab === "Form" && selectedEntity === "Resets" ? (
                resetRows.length ? (
                  <div className="form-view">
                    <form
                      className="form-shell"
                      onSubmit={handleResetSubmitForm(handleResetSubmit)}
                    >
                      <div className="form-grid">
                        <div className="form-field">
                          <label className="form-label" htmlFor="reset-index">
                            Index
                          </label>
                          <input
                            id="reset-index"
                            className="form-input"
                            type="number"
                            readOnly
                            {...registerReset("index", { valueAsNumber: true })}
                          />
                        </div>
                        <div className="form-field form-field--wide">
                          <label className="form-label" htmlFor="reset-command">
                            Command
                          </label>
                          <select
                            id="reset-command"
                            className="form-select"
                            {...registerReset("commandName")}
                          >
                            <option value="">Select</option>
                            {activeResetCommand &&
                            !resetCommandOptions.includes(
                              activeResetCommand as ResetCommand
                            ) ? (
                              <option value={activeResetCommand}>
                                {activeResetCommand}
                              </option>
                            ) : null}
                            {resetCommandOptions.map((command) => (
                              <option key={command} value={command}>
                                {command}
                              </option>
                            ))}
                          </select>
                          {resetFormState.errors.commandName ? (
                            <span className="form-error">
                              {resetFormState.errors.commandName.message}
                            </span>
                          ) : null}
                        </div>
                      </div>
                      <div className="form-field form-field--full">
                        <div className="form-section-header">
                          <div>
                            <div className="form-label">Reset Details</div>
                            <div className="form-hint">
                              {resetCommandOptions.includes(
                                activeResetCommand as ResetCommand
                              )
                                ? resetCommandLabels[
                                    activeResetCommand as ResetCommand
                                  ]
                                : "Select a reset command."}
                            </div>
                          </div>
                        </div>
                        <div className="block-list">
                          {!resetCommandOptions.includes(
                            activeResetCommand as ResetCommand
                          ) ? (
                            <div className="placeholder-block">
                              <div className="placeholder-title">
                                No reset selected
                              </div>
                              <p>
                                Choose a command to edit reset-specific
                                fields.
                              </p>
                            </div>
                          ) : null}
                          {activeResetCommand === "loadMob" ? (
                            <div className="block-card">
                              <div className="block-card__header">
                                <span>Load Mobile</span>
                              </div>
                              <div className="block-grid">
                                <VnumPicker<ResetFormValues>
                                  id="reset-load-mob"
                                  label="Mob VNUM"
                                  name={"mobVnum" as FieldPath<ResetFormValues>}
                                  register={registerReset}
                                  options={mobileVnumOptions}
                                  error={resetFormState.errors.mobVnum?.message}
                                />
                                <VnumPicker<ResetFormValues>
                                  id="reset-load-room"
                                  label="Room VNUM"
                                  name={"roomVnum" as FieldPath<ResetFormValues>}
                                  register={registerReset}
                                  options={roomVnumOptions}
                                  error={resetFormState.errors.roomVnum?.message}
                                />
                                <div className="form-field">
                                  <label className="form-label">
                                    Max In Area
                                  </label>
                                  <input
                                    className="form-input"
                                    type="number"
                                    {...registerReset("maxInArea")}
                                  />
                                </div>
                                <div className="form-field">
                                  <label className="form-label">
                                    Max In Room
                                  </label>
                                  <input
                                    className="form-input"
                                    type="number"
                                    {...registerReset("maxInRoom")}
                                  />
                                </div>
                              </div>
                            </div>
                          ) : null}
                          {activeResetCommand === "placeObj" ? (
                            <div className="block-card">
                              <div className="block-card__header">
                                <span>Place Object</span>
                              </div>
                              <div className="block-grid">
                                <VnumPicker<ResetFormValues>
                                  id="reset-place-obj"
                                  label="Object VNUM"
                                  name={"objVnum" as FieldPath<ResetFormValues>}
                                  register={registerReset}
                                  options={objectVnumOptions}
                                  error={resetFormState.errors.objVnum?.message}
                                />
                                <VnumPicker<ResetFormValues>
                                  id="reset-place-room"
                                  label="Room VNUM"
                                  name={"roomVnum" as FieldPath<ResetFormValues>}
                                  register={registerReset}
                                  options={roomVnumOptions}
                                  error={resetFormState.errors.roomVnum?.message}
                                />
                              </div>
                            </div>
                          ) : null}
                          {activeResetCommand === "putObj" ? (
                            <div className="block-card">
                              <div className="block-card__header">
                                <span>Put Object</span>
                              </div>
                              <div className="block-grid">
                                <VnumPicker<ResetFormValues>
                                  id="reset-put-obj"
                                  label="Object VNUM"
                                  name={"objVnum" as FieldPath<ResetFormValues>}
                                  register={registerReset}
                                  options={objectVnumOptions}
                                  error={resetFormState.errors.objVnum?.message}
                                />
                                <VnumPicker<ResetFormValues>
                                  id="reset-put-container"
                                  label="Container VNUM"
                                  name={
                                    "containerVnum" as FieldPath<ResetFormValues>
                                  }
                                  register={registerReset}
                                  options={objectVnumOptions}
                                  error={
                                    resetFormState.errors.containerVnum?.message
                                  }
                                />
                                <div className="form-field">
                                  <label className="form-label">Count</label>
                                  <input
                                    className="form-input"
                                    type="number"
                                    {...registerReset("count")}
                                  />
                                </div>
                                <div className="form-field">
                                  <label className="form-label">
                                    Max In Container
                                  </label>
                                  <input
                                    className="form-input"
                                    type="number"
                                    {...registerReset("maxInContainer")}
                                  />
                                </div>
                              </div>
                            </div>
                          ) : null}
                          {activeResetCommand === "giveObj" ? (
                            <div className="block-card">
                              <div className="block-card__header">
                                <span>Give Object</span>
                              </div>
                              <div className="block-grid">
                                <VnumPicker<ResetFormValues>
                                  id="reset-give-obj"
                                  label="Object VNUM"
                                  name={"objVnum" as FieldPath<ResetFormValues>}
                                  register={registerReset}
                                  options={objectVnumOptions}
                                  error={resetFormState.errors.objVnum?.message}
                                />
                              </div>
                            </div>
                          ) : null}
                          {activeResetCommand === "equipObj" ? (
                            <div className="block-card">
                              <div className="block-card__header">
                                <span>Equip Object</span>
                              </div>
                              <div className="block-grid">
                                <VnumPicker<ResetFormValues>
                                  id="reset-equip-obj"
                                  label="Object VNUM"
                                  name={"objVnum" as FieldPath<ResetFormValues>}
                                  register={registerReset}
                                  options={objectVnumOptions}
                                  error={resetFormState.errors.objVnum?.message}
                                />
                                <div className="form-field">
                                  <label className="form-label">
                                    Wear Location
                                  </label>
                                  <select
                                    className="form-select"
                                    {...registerReset("wearLoc")}
                                  >
                                    <option value="">Select</option>
                                    {wearLocations.map((loc) => (
                                      <option key={loc} value={loc}>
                                        {loc}
                                      </option>
                                    ))}
                                  </select>
                                  {resetFormState.errors.wearLoc ? (
                                    <span className="form-error">
                                      {resetFormState.errors.wearLoc.message}
                                    </span>
                                  ) : null}
                                </div>
                              </div>
                            </div>
                          ) : null}
                          {activeResetCommand === "setDoor" ? (
                            <div className="block-card">
                              <div className="block-card__header">
                                <span>Set Door</span>
                              </div>
                              <div className="block-grid">
                                <VnumPicker<ResetFormValues>
                                  id="reset-door-room"
                                  label="Room VNUM"
                                  name={"roomVnum" as FieldPath<ResetFormValues>}
                                  register={registerReset}
                                  options={roomVnumOptions}
                                  error={resetFormState.errors.roomVnum?.message}
                                />
                                <div className="form-field">
                                  <label className="form-label">
                                    Direction
                                  </label>
                                  <select
                                    className="form-select"
                                    {...registerReset("direction")}
                                  >
                                    <option value="">Select</option>
                                    {directions.map((dir) => (
                                      <option key={dir} value={dir}>
                                        {dir}
                                      </option>
                                    ))}
                                  </select>
                                  {resetFormState.errors.direction ? (
                                    <span className="form-error">
                                      {
                                        resetFormState.errors.direction.message
                                      }
                                    </span>
                                  ) : null}
                                </div>
                                <div className="form-field">
                                  <label className="form-label">State</label>
                                  <input
                                    className="form-input"
                                    type="number"
                                    {...registerReset("state")}
                                  />
                                  {resetFormState.errors.state ? (
                                    <span className="form-error">
                                      {resetFormState.errors.state.message}
                                    </span>
                                  ) : null}
                                </div>
                              </div>
                            </div>
                          ) : null}
                          {activeResetCommand === "randomizeExits" ? (
                            <div className="block-card">
                              <div className="block-card__header">
                                <span>Randomize Exits</span>
                              </div>
                              <div className="block-grid">
                                <VnumPicker<ResetFormValues>
                                  id="reset-random-room"
                                  label="Room VNUM"
                                  name={"roomVnum" as FieldPath<ResetFormValues>}
                                  register={registerReset}
                                  options={roomVnumOptions}
                                  error={resetFormState.errors.roomVnum?.message}
                                />
                                <div className="form-field">
                                  <label className="form-label">
                                    Exit Count
                                  </label>
                                  <input
                                    className="form-input"
                                    type="number"
                                    {...registerReset("exits")}
                                  />
                                  {resetFormState.errors.exits ? (
                                    <span className="form-error">
                                      {resetFormState.errors.exits.message}
                                    </span>
                                  ) : null}
                                </div>
                              </div>
                            </div>
                          ) : null}
                        </div>
                      </div>
                      <div className="form-actions">
                        <button
                          className="action-button action-button--primary"
                          type="submit"
                          disabled={!resetFormState.isDirty}
                        >
                          Apply Changes
                        </button>
                        <span className="form-hint">
                          {resetFormState.isDirty
                            ? "Unsaved changes"
                            : "Up to date"}
                        </span>
                      </div>
                    </form>
                  </div>
                ) : (
                  <div className="entity-table__empty">
                    <h3>No resets available</h3>
                    <p>Load an area JSON file to edit reset data.</p>
                  </div>
                )
              ) : activeTab === "Map" ? (
                roomNodes.length ? (
                  <div className="map-shell">
                    <div className="map-canvas">
                      <ReactFlow
                        nodes={roomNodes}
                        edges={roomEdges}
                        nodeTypes={roomNodeTypes}
                        fitView
                        nodesDraggable={false}
                        nodesConnectable={false}
                        onNodeClick={(_, node) => {
                          const vnum =
                            typeof node.data?.vnum === "number"
                              ? node.data.vnum
                              : parseVnum(node.id);
                          if (vnum !== null && vnum >= 0) {
                            setSelectedRoomVnum(vnum);
                            setSelectedEntity("Rooms");
                          }
                        }}
                        panOnScroll
                      >
                        <Background gap={24} size={1} />
                        <Controls showInteractive={false} />
                      </ReactFlow>
                      {areaVnumRange ? (
                        <div className="map-overlay">
                          VNUM range: {areaVnumRange}
                        </div>
                      ) : null}
                    </div>
                  </div>
                ) : (
                  <div className="entity-table__empty">
                    <h3>No rooms available</h3>
                    <p>Load an area JSON file to view the room map.</p>
                  </div>
                )
              ) : activeTab === "Table" &&
              (selectedEntity === "Rooms" ||
                selectedEntity === "Mobiles" ||
                selectedEntity === "Objects" ||
                selectedEntity === "Resets") ? (
                <div className="entity-table">
                  {selectedEntity === "Rooms" ? (
                    roomRows.length ? (
                      <div className="ag-theme-quartz worldedit-grid">
                        <AgGridReact<RoomRow>
                          rowData={roomRows}
                          columnDefs={roomColumns}
                          defaultColDef={roomDefaultColDef}
                          animateRows
                          rowSelection="single"
                          getRowId={(params) => String(params.data.vnum)}
                          domLayout="autoHeight"
                          onRowClicked={(event) =>
                            setSelectedRoomVnum(event.data?.vnum ?? null)
                          }
                          onGridReady={(event) => {
                            roomGridApi.current = event.api;
                          }}
                        />
                      </div>
                    ) : (
                      <div className="entity-table__empty">
                        <h3>Rooms will appear here</h3>
                        <p>
                          Load an area JSON file to populate the room table.
                        </p>
                      </div>
                    )
                  ) : null}
                  {selectedEntity === "Mobiles" ? (
                    mobileRows.length ? (
                      <div className="ag-theme-quartz worldedit-grid">
                        <AgGridReact<MobileRow>
                          rowData={mobileRows}
                          columnDefs={mobileColumns}
                          defaultColDef={mobileDefaultColDef}
                          animateRows
                          rowSelection="single"
                          getRowId={(params) => String(params.data.vnum)}
                          domLayout="autoHeight"
                          onRowClicked={(event) =>
                            setSelectedMobileVnum(event.data?.vnum ?? null)
                          }
                          onGridReady={(event) => {
                            mobileGridApi.current = event.api;
                          }}
                        />
                      </div>
                    ) : (
                      <div className="entity-table__empty">
                        <h3>Mobiles will appear here</h3>
                        <p>
                          Load an area JSON file to populate the mobile table.
                        </p>
                      </div>
                    )
                  ) : null}
                  {selectedEntity === "Objects" ? (
                    objectRows.length ? (
                      <div className="ag-theme-quartz worldedit-grid">
                        <AgGridReact<ObjectRow>
                          rowData={objectRows}
                          columnDefs={objectColumns}
                          defaultColDef={objectDefaultColDef}
                          animateRows
                          rowSelection="single"
                          getRowId={(params) => String(params.data.vnum)}
                          domLayout="autoHeight"
                          onRowClicked={(event) =>
                            setSelectedObjectVnum(event.data?.vnum ?? null)
                          }
                          onGridReady={(event) => {
                            objectGridApi.current = event.api;
                          }}
                        />
                      </div>
                    ) : (
                      <div className="entity-table__empty">
                        <h3>Objects will appear here</h3>
                        <p>
                          Load an area JSON file to populate the object table.
                        </p>
                      </div>
                    )
                  ) : null}
                  {selectedEntity === "Resets" ? (
                    resetRows.length ? (
                      <div className="ag-theme-quartz worldedit-grid">
                        <AgGridReact<ResetRow>
                          rowData={resetRows}
                          columnDefs={resetColumns}
                          defaultColDef={roomDefaultColDef}
                          animateRows
                          rowSelection="single"
                          getRowId={(params) => String(params.data.index)}
                          domLayout="autoHeight"
                          onRowClicked={(event) =>
                            setSelectedResetIndex(event.data?.index ?? null)
                          }
                          onGridReady={(event) => {
                            resetGridApi.current = event.api;
                          }}
                        />
                      </div>
                    ) : (
                      <div className="entity-table__empty">
                        <h3>Resets will appear here</h3>
                        <p>
                          Load an area JSON file to populate the reset table.
                        </p>
                      </div>
                    )
                  ) : null}
                </div>
              ) : (
                <div className="placeholder-grid">
                  {tabs.map((tab) => (
                    <div
                      className={`placeholder-block${
                        tab.id === activeTab ? " placeholder-block--active" : ""
                      }`}
                      key={tab.id}
                    >
                      <div className="placeholder-title">{tab.title}</div>
                      <p>{tab.description}</p>
                    </div>
                  ))}
                </div>
              )}
            </div>
          </div>
        </main>

        <aside className="inspector">
          <div className="panel-header">
            <h2>Inspector</h2>
            <button className="ghost-button" type="button">
              Pin
            </button>
          </div>
          <div className="inspector__section">
            <div className="inspector__label">Selected {selection.kindLabel}</div>
            <div className="inspector__value">{selection.selectionLabel}</div>
          </div>
          <div className="inspector__section">
            <div className="inspector__label">Flags</div>
            <div className="inspector__tags">
              {selection.flags.length ? (
                selection.flags.map((flag) => <span key={flag}>{flag}</span>)
              ) : (
                <span className="inspector__empty">No flags</span>
              )}
            </div>
          </div>
          <div className="inspector__section">
            <div className="inspector__label">Exits</div>
            <div className="inspector__value">{selection.exits}</div>
          </div>
          <div className="inspector__section">
            <div className="inspector__label">Validation</div>
            <div className="inspector__value inspector__value--warn">
              {selection.validation}
            </div>
          </div>
          <div className="inspector__section">
            <div className="inspector__label">Area Data Debug</div>
            {areaDebug.keys.length ? (
              <>
                <div className="inspector__value">
                  Top-level keys: {areaDebug.keys.join(", ")}
                </div>
                {areaDebug.arrayCounts.length ? (
                  <div className="inspector__debug-list">
                    {areaDebug.arrayCounts.map((entry) => (
                      <div key={entry.key}>
                        {entry.key}: {entry.count}
                      </div>
                    ))}
                  </div>
                ) : null}
              </>
            ) : (
              <div className="inspector__empty">No area data loaded.</div>
            )}
          </div>
        </aside>
      </section>

      <footer className="statusbar">
        <span>Status: {errorMessage ?? statusMessage}</span>
        <span>Area file: {areaPath ?? "Not loaded"}</span>
        <span>Meta file: {editorMetaPath ?? "Not loaded"}</span>
        <span>Area dir: {areaDirectory ?? "Not set"}</span>
        <span>Data dir: {dataDirectory ?? "Not set"}</span>
        <span>Selection: {selectedEntity}</span>
        <span>
          Reference:{" "}
          {referenceData
            ? `classes ${referenceData.classes.length}, races ${referenceData.races.length}, skills ${referenceData.skills.length}, commands ${referenceData.commands.length}`
            : "not loaded"}
        </span>
      </footer>
    </div>
  );
}
