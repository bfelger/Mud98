/**
 * Area validation functions.
 *
 * Contains validation rules that check area data for common issues:
 * - VNUM range violations
 * - Duplicate VNUMs
 * - Invalid reset references
 * - One-way exits
 * - Orphan rooms (no incoming exits)
 */

import { entityKindLabels } from "../constants/appConfig";
import {
  parseVnum,
  getEntityList,
  getAreaVnumBounds,
  resolveResetCommand
} from "../utils/areaUtils";
import type { AreaJson } from "../repository/types";
import type { ValidationIssueBase } from "./types";

/** Entity keys for validation checks. */
type EntityKey = "Rooms" | "Mobiles" | "Objects" | "Resets";

/**
 * Validate that all entity VNUMs fall within the area's declared range.
 */
export function validateVnumRanges(
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

/**
 * Validate that there are no duplicate VNUMs within each entity type.
 */
export function validateDuplicateVnums(
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

/**
 * Validate that all reset commands reference existing mobiles/objects.
 */
export function validateResetReferences(
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

/**
 * Validate that exits have reciprocal connections (warns on one-way exits).
 */
export function validateOneWayExits(
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

/**
 * Validate that all rooms have at least one incoming exit (warns on orphans).
 */
export function validateOrphanRooms(
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

/** Entity key type for validation summary. */
type SummaryEntityKey = "Rooms" | "Mobiles" | "Objects" | "Resets" | "Shops" | "Quests" | "Factions" | "Recipes" | "GatherSpawns";

/**
 * Build a human-readable summary of validation issues.
 * If the currently selected entity has an issue, shows that message.
 * Otherwise shows aggregate counts by severity and entity type.
 */
export function buildValidationSummary(
  issues: ValidationIssue[],
  selectedEntity: SummaryEntityKey,
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

/** Debug summary of area data structure. */
export type AreaDebugSummary = {
  keys: string[];
  arrayCounts: Array<{ key: string; count: number }>;
};

/**
 * Build a debug summary showing area data structure (keys and array counts).
 */
export function buildAreaDebugSummary(areaData: AreaJson | null): AreaDebugSummary {
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

// Import the full ValidationIssue type for buildValidationSummary
import type { ValidationIssue } from "./types";
