import type { AreaJson } from "../../repository/types";

export const parseVnum = (value: unknown): number | null => {
  if (typeof value === "number") {
    return value;
  }
  if (typeof value === "string" && value.trim().length) {
    const parsed = Number(value);
    return Number.isFinite(parsed) ? parsed : null;
  }
  return null;
};

export const getFirstString = (value: unknown, fallback: string): string =>
  typeof value === "string" && value.trim().length ? value : fallback;

export const getAreaVnumBounds = (
  areaData: AreaJson
): { min: number; max: number } | null => {
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
};

export const getNextVnumFromList = (
  areaData: AreaJson,
  entries: unknown[]
): number | null => {
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
};
