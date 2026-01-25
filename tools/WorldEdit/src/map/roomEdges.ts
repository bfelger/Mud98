import type { Edge } from "reactflow";
import type { AreaIndexEntry, AreaJson } from "../repository/types";
import type { ValidationIssueBase } from "../validation/types";
import { getAreaVnumBounds, getFirstString, parseVnum } from "../crud/area/utils";

export type ExternalExit = {
  fromVnum: number;
  fromName: string;
  direction: string;
  toVnum: number;
  areaName: string | null;
};

export type ExitValidationResult = {
  issues: ValidationIssueBase[];
  invalidEdgeIds: Set<string>;
};

const directionHandleMap: Record<string, { source: string; target: string }> = {
  north: { source: "north-out", target: "south-in" },
  east: { source: "east-out", target: "west-in" },
  south: { source: "south-out", target: "north-in" },
  west: { source: "west-out", target: "east-in" }
};

const externalSourceHandles: Record<string, string> = {
  up: "north-out",
  down: "south-out"
};

const getRooms = (areaData: AreaJson | null): unknown[] => {
  if (!areaData) {
    return [];
  }
  const list = (areaData as Record<string, unknown>).rooms;
  return Array.isArray(list) ? list : [];
};

export const findAreaForVnum = (
  areaIndex: AreaIndexEntry[],
  vnum: number
): AreaIndexEntry | null => {
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
};

export const buildExitTargetValidation = (
  areaData: AreaJson | null,
  areaIndex: AreaIndexEntry[]
): ExitValidationResult => {
  const empty = { issues: [], invalidEdgeIds: new Set<string>() };
  if (!areaData) {
    return empty;
  }
  const rooms = getRooms(areaData);
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
};

export const buildRoomEdges = (
  areaData: AreaJson | null,
  includeVerticalEdges: boolean,
  includeExternalEdges: boolean,
  invalidEdgeIds?: Set<string>
): Edge[] => {
  if (!areaData) {
    return [];
  }
  const rooms = getRooms(areaData);
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
};

export const buildExternalExits = (
  areaData: AreaJson | null,
  areaIndex: AreaIndexEntry[]
): ExternalExit[] => {
  if (!areaData) {
    return [];
  }
  const rooms = getRooms(areaData);
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
};
