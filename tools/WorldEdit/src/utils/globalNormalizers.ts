import type { RaceDefinition } from "../repository/types";
import {
  classTitleCount,
  groupSkillCount,
  raceSkillCount,
  raceStatKeys
} from "../constants/globalDefaults";

export const normalizeRaceStats = (
  value: RaceDefinition["stats"] | RaceDefinition["maxStats"]
): Record<string, number> => {
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
};

export const normalizeRaceClassMap = (
  value: Record<string, number> | number[] | undefined,
  classNames: string[],
  defaultValue: number
): Record<string, number> => {
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
};

export const normalizeRaceSkills = (value: string[] | undefined): string[] =>
  Array.from({ length: raceSkillCount }).map((_, index) => {
    const entry = value?.[index];
    return typeof entry === "string" ? entry : "";
  });

export const normalizeGroupSkills = (value: string[] | undefined): string[] =>
  Array.from({ length: groupSkillCount }).map((_, index) => {
    const entry = value?.[index];
    return typeof entry === "string" ? entry : "";
  });

export const titlesToText = (
  titles: string[][] | undefined,
  column: 0 | 1
): string => {
  const lines: string[] = [];
  for (let i = 0; i < classTitleCount; i += 1) {
    const value = titles?.[i]?.[column] ?? "";
    lines.push(value);
  }
  return lines.join("\n");
};

const normalizeTitleLines = (value: string | undefined): string[] => {
  if (!value) {
    return [];
  }
  return value.split(/\r?\n/).map((line) => line.trim());
};

export const buildTitlePairs = (
  maleText: string,
  femaleText: string
): string[][] => {
  const maleLines = normalizeTitleLines(maleText);
  const femaleLines = normalizeTitleLines(femaleText);
  const pairs: string[][] = [];
  for (let i = 0; i < classTitleCount; i += 1) {
    pairs.push([maleLines[i] ?? "", femaleLines[i] ?? ""]);
  }
  return pairs;
};
