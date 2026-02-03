/**
 * Utility functions for App component.
 * 
 * These are pure functions that don't depend on React state/hooks.
 */

import type { GridApi } from "ag-grid-community";
import type {
  AreaJson,
  EditorMeta,
  LootDataFile,
  RoomLayoutMap,
  AreaLayoutMap,
  EditorLayout
} from "../repository/types";
import type { EntityKey } from "../constants/appConfig";
import { entityKindLabels } from "../constants/appConfig";
import {
  parseVnum,
  getEntityList,
  findByVnum,
  findShopByKeeper,
  getFirstString,
  getAreaVnumRange
} from "./areaUtils";
import type { VnumOption } from "../components/VnumPicker";

/** Extract filename from a path. */
export function fileNameFromPath(path: string | null): string {
  if (!path) {
    return "No area loaded";
  }
  const normalized = path.replace(/[/\\\\]+/g, "/");
  return normalized.split("/").pop() ?? path;
}

/** Build or update editor metadata. */
export function buildEditorMeta(
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

/** Build VNUM picker options for an entity type. */
export function buildVnumOptions(
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

/** Selection summary result. */
export interface SelectionSummary {
  kindLabel: string;
  selectionLabel: string;
  vnumRange: string;
  lastSave: string;
  flags: string[];
  exits: string;
  validation: string;
}

/** Build a summary of the current selection for display. */
export function buildSelectionSummary(
  selectedEntity: EntityKey,
  areaData: AreaJson | null,
  selectedVnums: Partial<Record<EntityKey, number | null>>,
  lootContext?: {
    data: LootDataFile | null;
    kind: "group" | "table" | null;
    index: number | null;
  }
): SelectionSummary {
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

/** Get the dominant exit direction from direction counts. */
export function getDominantExitDirection(
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

/** Clean an optional string value, returning undefined if empty. */
export function cleanOptionalString(value: string | undefined): string | undefined {
  if (!value) {
    return undefined;
  }
  const trimmed = value.trim();
  return trimmed.length ? trimmed : undefined;
}

/** Sync AG Grid selection to match the selected row ID. */
export function syncGridSelection(
  api: GridApi | null,
  rowId: number | string | null
): void {
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

/** Build a Map from vnum to VnumOption for fast lookups. */
export function buildVnumOptionMap(options: VnumOption[]): Map<number, VnumOption> {
  const map = new Map<number, VnumOption>();
  for (const option of options) {
    map.set(option.vnum, option);
  }
  return map;
}

/** Table toolbar configuration. */
export interface TableToolbarConfig {
  title: string;
  count: number;
  newLabel: string;
  deleteLabel: string;
  canCreate: boolean;
  canDelete: boolean;
  onCreate: () => void;
  onDelete: () => void;
}

/** Build a table toolbar config for global entity views. */
export function buildTableToolbar(
  entityName: string,
  rowCount: number,
  data: unknown | null,
  selectedIndex: number | null,
  onCreate: () => void,
  onDelete: () => void
): TableToolbarConfig {
  return {
    title: entityName,
    count: rowCount,
    newLabel: `New ${entityName.slice(0, -1)}`, // Remove trailing 's'
    deleteLabel: `Delete ${entityName.slice(0, -1)}`,
    canCreate: Boolean(data),
    canDelete: selectedIndex !== null,
    onCreate,
    onDelete
  };
}
