import type { Edge, Node } from "reactflow";
import type { AreaGraphLink, AreaIndexEntry, AreaJson } from "../repository/types";
import type { AreaGraphNodeData } from "./areaNodes";

export type AreaGraphEntry = {
  id: string;
  name: string;
  vnumRange: [number, number] | null;
};

export type AreaGraphContext = {
  entries: AreaGraphEntry[];
  currentId: string | null;
};

type ExternalExit = {
  fromVnum: number;
  fromName: string;
  direction: string;
  toVnum: number;
  areaName: string | null;
};

type BuildAreaGraphContextParams = {
  areaIndex: AreaIndexEntry[];
  areaData: AreaJson | null;
  areaPath: string | null;
  getAreaVnumBounds: (areaData: AreaJson) => { min: number; max: number } | null;
  getFirstString: (value: unknown, fallback: string) => string;
  fileNameFromPath: (value: string) => string;
};

type BuildAreaGraphEdgesParams = {
  areaGraphContext: AreaGraphContext;
  areaGraphFilteredEntries: AreaGraphEntry[];
  areaGraphLinks: AreaGraphLink[];
  areaIndex: AreaIndexEntry[];
  externalExits: ExternalExit[];
  areaDirectionHandleMap: Record<string, { source: string; target: string }>;
  getDominantExitDirection: (
    directionCounts: Partial<Record<string, number>> | null | undefined
  ) => string | null;
  findAreaForVnum: (areaIndex: AreaIndexEntry[], vnum: number) => AreaIndexEntry | null;
};

type BuildAreaGraphNodesParams = {
  areaGraphFilteredEntries: AreaGraphEntry[];
  areaGraphContext: AreaGraphContext;
  areaGraphMatch: AreaGraphEntry | null;
  formatVnumRange: (range: [number, number] | null) => string;
};

export const buildAreaGraphContext = (
  params: BuildAreaGraphContextParams
): AreaGraphContext => {
  const {
    areaIndex,
    areaData,
    areaPath,
    getAreaVnumBounds,
    getFirstString,
    fileNameFromPath
  } = params;
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
};

export const buildAreaGraphMatch = (
  entries: AreaGraphEntry[],
  vnumQuery: string,
  parseVnum: (value: unknown) => number | null
): AreaGraphEntry | null => {
  const vnum = parseVnum(vnumQuery);
  if (vnum === null) {
    return null;
  }
  return (
    entries.find((entry) => {
      if (!entry.vnumRange) {
        return false;
      }
      const [start, end] = entry.vnumRange;
      return vnum >= start && vnum <= end;
    }) ?? null
  );
};

export const buildAreaGraphMatchLabel = (
  match: AreaGraphEntry | null,
  formatVnumRange: (range: [number, number] | null) => string
): string | null => {
  if (!match) {
    return null;
  }
  return `${match.name} (${formatVnumRange(match.vnumRange)})`;
};

export const buildAreaGraphFilteredEntries = (
  entries: AreaGraphEntry[],
  filterValue: string
): AreaGraphEntry[] => {
  const normalized = filterValue.trim().toLowerCase();
  if (!normalized) {
    return entries;
  }
  return entries.filter((entry) => {
    const name = entry.name.toLowerCase();
    const id = entry.id.toLowerCase();
    return name.includes(normalized) || id.includes(normalized);
  });
};

export const buildAreaGraphNodes = (
  params: BuildAreaGraphNodesParams
): Node<AreaGraphNodeData>[] => {
  const { areaGraphFilteredEntries, areaGraphContext, areaGraphMatch, formatVnumRange } = params;
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
};

export const buildAreaGraphEdges = (
  params: BuildAreaGraphEdgesParams
): Edge[] => {
  const {
    areaGraphContext,
    areaGraphFilteredEntries,
    areaGraphLinks,
    areaIndex,
    externalExits,
    areaDirectionHandleMap,
    getDominantExitDirection,
    findAreaForVnum
  } = params;
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
  const shouldUseExternal = !areaGraphLinks.length || !indexIds.has(currentId);
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
};
