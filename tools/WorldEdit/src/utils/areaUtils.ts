/**
 * Utility functions for area data manipulation and entity handling.
 * 
 * These functions provide parsing, normalization, and lookup operations
 * used throughout the WorldEdit application.
 */

import type { VnumOption } from "../components/VnumPicker";
import type { AreaJson } from "../repository/types";
import { entityDataKeys, resetCommandMap } from "../constants/appConfig";

// =============================================================================
// VNUM Parsing & Formatting
// =============================================================================

/**
 * Parse a value into a VNUM (number) or return null if invalid.
 */
export function parseVnum(value: unknown): number | null {
  if (typeof value === "number") {
    return value;
  }
  if (typeof value === "string" && value.trim().length) {
    const parsed = Number(value);
    return Number.isFinite(parsed) ? parsed : null;
  }
  return null;
}

/**
 * Parse a comma/space-separated string of numbers into an array.
 */
export function parseNumberList(value: string | undefined): number[] {
  if (!value) {
    return [];
  }
  return value
    .split(/[,\\s]+/)
    .map((token) => Number(token.trim()))
    .filter((num) => Number.isFinite(num));
}

/**
 * Format an array of values as a comma-separated string of VNUMs.
 */
export function formatNumberList(values: unknown): string {
  if (!Array.isArray(values)) {
    return "";
  }
  return values
    .map((value) => parseVnum(value))
    .filter((num): num is number => num !== null)
    .join(", ");
}

/**
 * Format a VNUM with its label from a lookup map.
 */
export function formatVnumLabel(
  vnum: number | null,
  map: Map<number, VnumOption>
): string {
  if (vnum === null) {
    return "?";
  }
  const option = map.get(vnum);
  if (option?.label) {
    return `${vnum} ${option.label}`;
  }
  return String(vnum);
}

/**
 * Format a VNUM range tuple as a string.
 */
export function formatVnumRange(range: [number, number] | null): string {
  if (!range) {
    return "n/a";
  }
  return `${range[0]}-${range[1]}`;
}

// =============================================================================
// String/Text Normalization
// =============================================================================

/**
 * Normalize optional text - trims and returns undefined if empty.
 */
export function normalizeOptionalText(value: string | undefined) {
  if (!value) {
    return undefined;
  }
  const trimmed = value.trim();
  return trimmed.length ? trimmed : undefined;
}

/**
 * Normalize line endings to Unix format for display (LF only).
 */
export function normalizeLineEndingsForDisplay(value: string | undefined) {
  if (typeof value !== "string") {
    return "";
  }
  return value.replace(/\r\n/g, "\n").replace(/\n\r/g, "\n").replace(/\r/g, "\n");
}

/**
 * Normalize line endings to MUD format (CR+LF).
 */
export function normalizeLineEndingsForMud(value: string) {
  const normalized = value
    .replace(/\r\n/g, "\n")
    .replace(/\n\r/g, "\n")
    .replace(/\r/g, "\n");
  return normalized.replace(/\n/g, "\n\r");
}

/**
 * Normalize optional MUD text - converts line endings and returns undefined if empty.
 */
export function normalizeOptionalMudText(value: string | undefined) {
  if (value === undefined) {
    return undefined;
  }
  if (!value.trim().length) {
    return undefined;
  }
  return normalizeLineEndingsForMud(value);
}

/**
 * Normalize optional script text.
 */
export function normalizeOptionalScript(value: string | undefined) {
  if (value === undefined) {
    return undefined;
  }
  const normalized = normalizeLineEndingsForMud(value);
  return normalized.trim().length ? normalized : undefined;
}

// =============================================================================
// Dice Normalization
// =============================================================================

export type DiceFormInput = {
  number?: number;
  type?: number;
  bonus?: number;
};

/**
 * Normalize dice form input, returning undefined if incomplete.
 */
export function normalizeDice(dice: DiceFormInput | undefined) {
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

// =============================================================================
// Checklist Status
// =============================================================================

/**
 * Normalize checklist status from various input formats.
 */
export function normalizeChecklistStatus(
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

// =============================================================================
// Entity List Operations
// =============================================================================

type EntityKey = 
  | "Area"
  | "Rooms"
  | "Mobiles"
  | "Objects"
  | "Resets"
  | "Shops"
  | "Quests"
  | "Factions"
  | "Loot"
  | "Recipes"
  | "Gather Spawns"
  | "Helps";

/**
 * Get the list of entities for a given entity type from area data.
 */
export function getEntityList(areaData: AreaJson | null, key: EntityKey): unknown[] {
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

/**
 * Find an entity by VNUM in a list.
 */
export function findByVnum(list: unknown[], vnum: number): Record<string, unknown> | null {
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

/**
 * Find a shop by its keeper VNUM.
 */
export function findShopByKeeper(
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

/**
 * Get the first string value or a fallback.
 */
export function getFirstString(value: unknown, fallback: string): string {
  return typeof value === "string" && value.trim().length ? value : fallback;
}

// =============================================================================
// Area VNUM Operations
// =============================================================================

/**
 * Get the VNUM range string for an area (e.g., "1000-1099").
 */
export function getAreaVnumRange(areaData: AreaJson | null): string | null {
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

/**
 * Get the VNUM bounds (min/max) for an area.
 */
export function getAreaVnumBounds(
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

/**
 * Get the next available VNUM for an entity type.
 */
export function getNextEntityVnum(areaData: AreaJson, entity: EntityKey): number | null {
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

/**
 * Get the first VNUM for an entity type.
 */
export function getFirstEntityVnum(areaData: AreaJson, entity: EntityKey): number | null {
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

/**
 * Get the default selection for an entity type.
 */
export function getDefaultSelection(
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

// =============================================================================
// Reset Operations
// =============================================================================

/**
 * Resolve a reset command from a record (handles both old and new formats).
 */
export function resolveResetCommand(record: Record<string, unknown> | null): string {
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

/**
 * Check if a reset targets a specific room.
 */
export function isResetForRoom(
  record: Record<string, unknown>,
  roomVnum: number
): boolean {
  const target = parseVnum(record.roomVnum);
  return target !== null && target === roomVnum;
}

/**
 * Format a reset summary for display.
 */
export function formatResetSummary(
  record: Record<string, unknown>,
  command: string,
  roomMap: Map<number, VnumOption>,
  mobMap: Map<number, VnumOption>,
  objMap: Map<number, VnumOption>
): string {
  const roomVnum = parseVnum(record.roomVnum);
  const mobVnum = parseVnum(record.mobVnum);
  const objVnum = parseVnum(record.objVnum);
  const direction =
    typeof record.direction === "string" ? record.direction : null;
  const state = parseVnum(record.state);
  const exits = parseVnum(record.exits);
  switch (command) {
    case "loadMob":
      return `Load ${formatVnumLabel(mobVnum, mobMap)} in ${formatVnumLabel(
        roomVnum,
        roomMap
      )}`;
    case "placeObj":
      return `Place ${formatVnumLabel(objVnum, objMap)} in ${formatVnumLabel(
        roomVnum,
        roomMap
      )}`;
    case "setDoor":
      return `Set door ${direction ?? "?"} (${formatVnumLabel(
        roomVnum,
        roomMap
      )}) state ${state ?? "?"}`;
    case "randomizeExits":
      return `Randomize exits (${formatVnumLabel(roomVnum, roomMap)}: ${
        exits ?? "?"
      })`;
    default:
      if (roomVnum !== null) {
        return `${command || "Reset"} in ${formatVnumLabel(
          roomVnum,
          roomMap
        )}`;
      }
      return command || "Reset";
  }
}

// =============================================================================
// Object Type Block Mapping
// =============================================================================

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
