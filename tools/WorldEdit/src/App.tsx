import { useEffect, useMemo, useRef, useState } from "react";
import type { ColDef, GridApi } from "ag-grid-community";
import { AgGridReact } from "ag-grid-react";
import { zodResolver } from "@hookform/resolvers/zod";
import { useFieldArray, useForm } from "react-hook-form";
import { z } from "zod";
import { LocalFileRepository } from "./repository/localFileRepository";
import type { AreaJson, EditorMeta, ReferenceData } from "./repository/types";
import {
  damageTypeEnum,
  damageTypes,
  directionEnum,
  directions,
  exitFlagEnum,
  exitFlags,
  positionEnum,
  positions,
  roomFlagEnum,
  roomFlags as roomFlagOptions,
  sectorEnum,
  sectors,
  sexEnum,
  sexes,
  sizeEnum,
  sizes
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

type TabId = (typeof tabs)[number]["id"];
type EntityKey = (typeof entityOrder)[number];

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
                                <div className="form-field">
                                  <label
                                    className="form-label"
                                    htmlFor={`exit-to-${field.id}`}
                                  >
                                    To VNUM
                                  </label>
                                  <input
                                    id={`exit-to-${field.id}`}
                                    className="form-input"
                                    type="number"
                                    {...registerRoom(`exits.${index}.toVnum`)}
                                  />
                                  {roomFormState.errors.exits?.[index]?.toVnum ? (
                                    <span className="form-error">
                                      {
                                        roomFormState.errors.exits?.[index]
                                          ?.toVnum?.message
                                      }
                                    </span>
                                  ) : null}
                                </div>
                                <div className="form-field">
                                  <label
                                    className="form-label"
                                    htmlFor={`exit-key-${field.id}`}
                                  >
                                    Key VNUM
                                  </label>
                                  <input
                                    id={`exit-key-${field.id}`}
                                    className="form-input"
                                    type="number"
                                    {...registerRoom(`exits.${index}.key`)}
                                  />
                                </div>
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
