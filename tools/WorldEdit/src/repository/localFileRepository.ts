import { open, save } from "@tauri-apps/plugin-dialog";
import {
  exists,
  mkdir,
  readDir,
  readTextFile,
  writeTextFile
} from "@tauri-apps/plugin-fs";
import { basename, dirname, homeDir, join } from "@tauri-apps/api/path";
import { canonicalStringify } from "../utils/canonicalJson";
import {
  actFlags,
  affectFlags,
  formFlags,
  immFlags,
  offFlags,
  partFlags,
  resFlags,
  vulnFlags
} from "../schemas/enums";
import type {
  AreaExitDirection,
  AreaGraphLink,
  AreaIndexEntry,
  AreaJson,
  ClassDataFile,
  ClassDataSource,
  ClassDefinition,
  CommandDataFile,
  CommandDataSource,
  CommandDefinition,
  EditorMeta,
  GroupDataFile,
  GroupDataSource,
  GroupDefinition,
  ProjectConfig,
  ProjectDataFiles,
  RaceDataFile,
  RaceDataSource,
  RaceDefinition,
  RaceStats,
  SocialDataFile,
  SocialDataSource,
  SocialDefinition,
  TutorialDataFile,
  TutorialDataSource,
  TutorialDefinition,
  TutorialStep,
  SkillDataFile,
  SkillDataSource,
  SkillDefinition,
  ReferenceData
} from "./types";
import type { WorldRepository } from "./worldRepository";

const jsonFilter = {
  name: "Mud98 Area JSON",
  extensions: ["json"]
};
const cfgFilter = {
  name: "Mud98 Config",
  extensions: ["cfg"]
};

const LEGACY_AREA_IGNORE = new Set(["help.are", "group.are", "music.txt", "olc.hlp", "rom.are"]);
const CLASS_MAX_LEVEL = 60;
const CLASS_TITLE_PAIRS = CLASS_MAX_LEVEL + 1;
const CLASS_GUILD_COUNT = 2;
const PRIME_STAT_NAMES = ["str", "int", "wis", "dex", "con"];
const ARMOR_PROF_NAMES = ["old_style", "cloth", "light", "medium", "heavy"];
const RACE_STAT_KEYS = ["str", "int", "wis", "dex", "con"] as const;
const RACE_SKILL_COUNT = 5;
const RACE_FORM_DEFAULTS: Record<string, string[]> = {
  humanoidDefault: ["edible", "biped", "mammal", "sentient", "bleeds"],
  animalDefault: ["edible", "animal", "bleeds"]
};
const RACE_PART_DEFAULTS: Record<string, string[]> = {
  humanoidDefault: [
    "head",
    "legs",
    "heart",
    "brains",
    "guts",
    "ear",
    "eye",
    "arms",
    "hands",
    "feet",
    "fingers"
  ],
  animalDefault: ["head", "legs", "heart", "brains", "guts", "ear", "eye"]
};
const VULN_FLAG_BITS = vulnFlags.filter((flag) => flag !== "none");
const FORM_FLAG_BITS = formFlags.filter((flag) => !flag.endsWith("Default"));
const PART_FLAG_BITS = partFlags.filter((flag) => !flag.endsWith("Default"));
const DEFAULT_SKILL_LEVEL = 53;
const DEFAULT_SKILL_RATING = 0;
const GROUP_SKILL_COUNT = 15;
const DEFAULT_COMMAND_FUNCTION = "do_nothing";
const DEFAULT_COMMAND_POSITION = "dead";
const DEFAULT_COMMAND_LOG = "log_normal";
const DEFAULT_COMMAND_CATEGORY = "undef";

async function defaultDialogPath(
  fallbackPath?: string | null
): Promise<string | null> {
  if (fallbackPath) {
    return fallbackPath;
  }
  return await homeDir();
}

function splitPath(pathValue: string): {
  dir: string;
  file: string;
  separator: string;
} {
  const lastSlash = pathValue.lastIndexOf("/");
  const lastBackslash = pathValue.lastIndexOf("\\");
  const index = Math.max(lastSlash, lastBackslash);
  if (index === -1) {
    return { dir: "", file: pathValue, separator: "/" };
  }
  const separator = lastBackslash > lastSlash ? "\\" : "/";
  return {
    dir: pathValue.slice(0, index),
    file: pathValue.slice(index + 1),
    separator
  };
}

function editorMetaPathForArea(areaPath: string): string {
  if (areaPath.endsWith(".editor.json")) {
    return areaPath;
  }
  const { dir, file, separator } = splitPath(areaPath);
  const metaFile = file.toLowerCase().endsWith(".json")
    ? file.replace(/\.json$/i, ".editor.json")
    : `${file}.editor.json`;
  const metaDir = dir ? `${dir}${separator}.worldedit` : ".worldedit";
  return `${metaDir}${separator}${metaFile}`;
}

function legacyMetaPathFor(pathValue: string): string | null {
  if (pathValue.includes("/.worldedit/")) {
    return pathValue.replace("/.worldedit/", "/");
  }
  if (pathValue.includes("\\.worldedit\\")) {
    return pathValue.replace("\\.worldedit\\", "\\");
  }
  return null;
}

function isMissingFileError(error: unknown): boolean {
  if (!error) {
    return false;
  }
  const message = typeof error === "string" ? error : String(error);
  return message.includes("No such file") || message.includes("NotFound");
}

function stripTilde(value: string): string {
  const trimmed = value.trim();
  return trimmed.endsWith("~") ? trimmed.slice(0, -1).trim() : trimmed;
}

function parseOlcNames(content: string): string[] {
  const names: string[] = [];
  let inBlock = false;

  for (const rawLine of content.split(/\r?\n/)) {
    const line = rawLine.trim();
    if (!line) {
      continue;
    }

    if (line.startsWith("#")) {
      inBlock = line !== "#END";
      continue;
    }

    if (!inBlock) {
      continue;
    }

    if (line.startsWith("name ")) {
      const value = stripTilde(line.slice(5));
      if (value) {
        names.push(value);
      }
    }
  }

  return names;
}

function isAbsolutePath(value: string): boolean {
  if (!value) {
    return false;
  }
  if (value.startsWith("/") || value.startsWith("\\")) {
    return true;
  }
  return /^[A-Za-z]:[\\/]/.test(value);
}

function stripInlineComment(line: string): string {
  let inQuote = false;
  let quoteChar = "";
  for (let i = 0; i < line.length; i += 1) {
    const char = line[i];
    if ((char === '"' || char === "'") && line[i - 1] !== "\\") {
      if (!inQuote) {
        inQuote = true;
        quoteChar = char;
      } else if (char === quoteChar) {
        inQuote = false;
        quoteChar = "";
      }
    }
    if (char === "#" && !inQuote) {
      return line.slice(0, i);
    }
  }
  return line;
}

function parseConfig(content: string): Record<string, string> {
  const entries: Record<string, string> = {};
  for (const rawLine of content.split(/\r?\n/)) {
    const cleaned = stripInlineComment(rawLine).trim();
    if (!cleaned) {
      continue;
    }
    const idx = cleaned.indexOf("=");
    if (idx === -1) {
      continue;
    }
    const key = cleaned.slice(0, idx).trim().toLowerCase();
    let value = cleaned.slice(idx + 1).trim();
    if (
      (value.startsWith('"') && value.endsWith('"')) ||
      (value.startsWith("'") && value.endsWith("'"))
    ) {
      value = value.slice(1, -1);
    }
    const normalized = value.trim();
    if (key && normalized.length) {
      entries[key] = normalized;
    }
  }
  return entries;
}

function normalizeFormat(value: string | undefined): "json" | "olc" {
  if (!value) {
    return "olc";
  }
  const normalized = value.trim().toLowerCase();
  return normalized === "json" ? "json" : "olc";
}

function buildDataFiles(
  config: Record<string, string>,
  format: "json" | "olc"
): ProjectDataFiles {
  const extension = format === "json" ? "json" : "olc";
  const file = (key: string, base: string) =>
    config[key] ?? `${base}.${extension}`;
  return {
    classes: file("classes_file", "classes"),
    races: file("races_file", "races"),
    skills: file("skills_file", "skills"),
    groups: file("groups_file", "groups"),
    commands: file("commands_file", "commands"),
    socials: file("socials_file", "socials"),
    tutorials: file("tutorials_file", "tutorials"),
    themes: file("themes_file", "themes"),
    loot: file("loot_file", "loot"),
    lox: file("lox_file", "lox")
  };
}

class OlcScanner {
  private content: string;
  private index = 0;

  constructor(content: string) {
    this.content = content;
  }

  eof(): boolean {
    return this.index >= this.content.length;
  }

  private skipWhitespace(): void {
    while (!this.eof() && /\s/.test(this.content[this.index])) {
      this.index += 1;
    }
  }

  readWord(): string | null {
    this.skipWhitespace();
    if (this.eof()) {
      return null;
    }
    const start = this.index;
    while (!this.eof() && !/\s/.test(this.content[this.index])) {
      this.index += 1;
    }
    return this.content.slice(start, this.index);
  }

  readString(): string {
    this.skipWhitespace();
    if (this.eof()) {
      return "";
    }
    const start = this.index;
    while (!this.eof() && this.content[this.index] !== "~") {
      this.index += 1;
    }
    const value = this.content.slice(start, this.index).trim();
    if (!this.eof() && this.content[this.index] === "~") {
      this.index += 1;
    }
    return value;
  }

  readNumber(): number {
    const word = this.readWord();
    if (!word) {
      return 0;
    }
    const value = Number.parseInt(word, 10);
    return Number.isNaN(value) ? 0 : value;
  }
}

function normalizePrimeStat(value: unknown): string | undefined {
  if (typeof value === "string") {
    const trimmed = value.trim();
    return PRIME_STAT_NAMES.includes(trimmed) ? trimmed : undefined;
  }
  if (typeof value === "number" && Number.isFinite(value)) {
    const index = Math.trunc(value);
    return PRIME_STAT_NAMES[index] ?? undefined;
  }
  return undefined;
}

function normalizeArmorProf(value: unknown): string | undefined {
  if (typeof value === "string") {
    const trimmed = value.trim();
    return ARMOR_PROF_NAMES.includes(trimmed) ? trimmed : undefined;
  }
  if (typeof value === "number" && Number.isFinite(value)) {
    const index = Math.trunc(value);
    return ARMOR_PROF_NAMES[index] ?? undefined;
  }
  return undefined;
}

function normalizeGuilds(value: unknown): number[] {
  const guilds: number[] = [];
  if (Array.isArray(value)) {
    value.forEach((entry) => {
      if (typeof entry === "number" && Number.isFinite(entry)) {
        guilds.push(Math.trunc(entry));
      }
    });
  }
  while (guilds.length < CLASS_GUILD_COUNT) {
    guilds.push(0);
  }
  return guilds.slice(0, CLASS_GUILD_COUNT);
}

function normalizeTitles(value: unknown): string[][] {
  const pairs: string[][] = [];
  for (let i = 0; i < CLASS_TITLE_PAIRS; i += 1) {
    pairs.push(["", ""]);
  }
  if (!Array.isArray(value)) {
    return pairs;
  }
  value.forEach((entry, index) => {
    if (index >= CLASS_TITLE_PAIRS) {
      return;
    }
    if (Array.isArray(entry)) {
      const male = entry[0];
      const female = entry[1];
      pairs[index][0] = typeof male === "string" ? male : "";
      pairs[index][1] = typeof female === "string" ? female : "";
    }
  });
  return pairs;
}

function normalizeTitlesFromList(entries: string[]): string[][] {
  const pairs: string[][] = [];
  for (let i = 0; i < CLASS_TITLE_PAIRS; i += 1) {
    const male = entries[i * 2] ?? "";
    const female = entries[i * 2 + 1] ?? "";
    pairs.push([male, female]);
  }
  return pairs;
}

function sanitizeOlcString(value: string): string {
  return value.replace(/~/g, "").trim();
}

function readOlcNumberList(scanner: OlcScanner): number[] {
  const values: number[] = [];
  while (!scanner.eof()) {
    const word = scanner.readWord();
    if (!word || word === "@") {
      break;
    }
    const parsed = Number.parseInt(word, 10);
    values.push(Number.isNaN(parsed) ? 0 : parsed);
  }
  return values;
}

function normalizeClassRecord(record: Partial<ClassDefinition>): ClassDefinition {
  return {
    name: record.name?.trim() || "unnamed",
    whoName: record.whoName?.trim() || "",
    baseGroup: record.baseGroup?.trim() || "",
    defaultGroup: record.defaultGroup?.trim() || "",
    weaponVnum:
      typeof record.weaponVnum === "number" && Number.isFinite(record.weaponVnum)
        ? Math.trunc(record.weaponVnum)
        : 0,
    armorProf: normalizeArmorProf(record.armorProf),
    guilds: normalizeGuilds(record.guilds),
    primeStat: normalizePrimeStat(record.primeStat),
    skillCap:
      typeof record.skillCap === "number" && Number.isFinite(record.skillCap)
        ? Math.trunc(record.skillCap)
        : 0,
    thac0_00:
      typeof record.thac0_00 === "number" && Number.isFinite(record.thac0_00)
        ? Math.trunc(record.thac0_00)
        : 0,
    thac0_32:
      typeof record.thac0_32 === "number" && Number.isFinite(record.thac0_32)
        ? Math.trunc(record.thac0_32)
        : 0,
    hpMin:
      typeof record.hpMin === "number" && Number.isFinite(record.hpMin)
        ? Math.trunc(record.hpMin)
        : 0,
    hpMax:
      typeof record.hpMax === "number" && Number.isFinite(record.hpMax)
        ? Math.trunc(record.hpMax)
        : 0,
    manaUser: record.manaUser ?? false,
    startLoc:
      typeof record.startLoc === "number" && Number.isFinite(record.startLoc)
        ? Math.trunc(record.startLoc)
        : 0,
    titles: normalizeTitles(record.titles)
  };
}

function parseClassJson(content: string): ClassDataFile {
  const parsed = JSON.parse(content) as Record<string, unknown>;
  const formatVersion =
    typeof parsed.formatVersion === "number" ? parsed.formatVersion : 1;
  const classesRaw = Array.isArray(parsed.classes) ? parsed.classes : [];
  const classes = classesRaw
    .map((entry) => {
      if (!entry || typeof entry !== "object") {
        return null;
      }
      const record = entry as Record<string, unknown>;
      const titles = normalizeTitles(record.titles);
      return normalizeClassRecord({
        name: typeof record.name === "string" ? record.name : "unnamed",
        whoName: typeof record.whoName === "string" ? record.whoName : "",
        baseGroup: typeof record.baseGroup === "string" ? record.baseGroup : "",
        defaultGroup:
          typeof record.defaultGroup === "string" ? record.defaultGroup : "",
        weaponVnum:
          typeof record.weaponVnum === "number" ? record.weaponVnum : 0,
        armorProf: normalizeArmorProf(record.armorProf),
        guilds: normalizeGuilds(record.guilds),
        primeStat: normalizePrimeStat(record.primeStat),
        skillCap: typeof record.skillCap === "number" ? record.skillCap : 0,
        thac0_00: typeof record.thac0_00 === "number" ? record.thac0_00 : 0,
        thac0_32: typeof record.thac0_32 === "number" ? record.thac0_32 : 0,
        hpMin: typeof record.hpMin === "number" ? record.hpMin : 0,
        hpMax: typeof record.hpMax === "number" ? record.hpMax : 0,
        manaUser: typeof record.manaUser === "boolean" ? record.manaUser : false,
        startLoc: typeof record.startLoc === "number" ? record.startLoc : 0,
        titles
      });
    })
    .filter((entry): entry is ClassDefinition => Boolean(entry));

  return { formatVersion, classes };
}

function parseClassOlc(content: string): ClassDataFile {
  const scanner = new OlcScanner(content);
  const classes: ClassDefinition[] = [];

  while (!scanner.eof()) {
    const word = scanner.readWord();
    if (!word) {
      break;
    }
    const upper = word.toUpperCase();
    if (upper !== "#CLASS") {
      if (upper === "#END") {
        break;
      }
      continue;
    }
    const record: Partial<ClassDefinition> = {};
    while (!scanner.eof()) {
      const field = scanner.readWord();
      if (!field) {
        break;
      }
      const fieldUpper = field.toUpperCase();
      if (fieldUpper === "#END") {
        break;
      }
      switch (field.toLowerCase()) {
        case "name":
          record.name = scanner.readString();
          break;
        case "wname":
          record.whoName = scanner.readString();
          break;
        case "basegrp":
          record.baseGroup = scanner.readString();
          break;
        case "dfltgrp":
          record.defaultGroup = scanner.readString();
          break;
        case "weap":
          record.weaponVnum = scanner.readNumber();
          break;
        case "armorprof":
          record.armorProf = scanner.readString();
          break;
        case "guild": {
          const guilds: number[] = [];
          while (!scanner.eof()) {
            const value = scanner.readWord();
            if (!value || value === "@") {
              break;
            }
            const parsed = Number.parseInt(value, 10);
            guilds.push(Number.isNaN(parsed) ? 0 : parsed);
          }
          record.guilds = guilds;
          break;
        }
        case "pstat":
          record.primeStat = normalizePrimeStat(scanner.readNumber());
          break;
        case "scap":
          record.skillCap = scanner.readNumber();
          break;
        case "t0_0":
          record.thac0_00 = scanner.readNumber();
          break;
        case "t0_32":
          record.thac0_32 = scanner.readNumber();
          break;
        case "hpmin":
          record.hpMin = scanner.readNumber();
          break;
        case "hpmax":
          record.hpMax = scanner.readNumber();
          break;
        case "fmana": {
          const value = scanner.readWord();
          record.manaUser = value ? value.toLowerCase() === "true" : false;
          break;
        }
        case "titles": {
          const titles: string[] = [];
          while (!scanner.eof()) {
            const value = scanner.readString();
            if (value === "@") {
              break;
            }
            titles.push(value);
          }
          record.titles = normalizeTitlesFromList(titles);
          break;
        }
        case "start_loc":
          record.startLoc = scanner.readNumber();
          break;
        default:
          break;
      }
    }
    classes.push(normalizeClassRecord(record));
  }

  return { formatVersion: 1, classes };
}

function serializeClassJson(data: ClassDataFile): string {
  const root: Record<string, unknown> = {
    formatVersion: 1,
    classes: data.classes.map((cls) => {
      const record = normalizeClassRecord(cls);
      const obj: Record<string, unknown> = {
        name: record.name,
        weaponVnum: record.weaponVnum ?? 0,
        guilds: normalizeGuilds(record.guilds),
        skillCap: record.skillCap ?? 0,
        thac0_00: record.thac0_00 ?? 0,
        thac0_32: record.thac0_32 ?? 0,
        hpMin: record.hpMin ?? 0,
        hpMax: record.hpMax ?? 0,
        manaUser: record.manaUser ?? false,
        startLoc: record.startLoc ?? 0,
        titles: normalizeTitles(record.titles)
      };
      if (record.whoName) {
        obj.whoName = record.whoName;
      }
      if (record.baseGroup) {
        obj.baseGroup = record.baseGroup;
      }
      if (record.defaultGroup) {
        obj.defaultGroup = record.defaultGroup;
      }
      if (record.armorProf && record.armorProf !== "old_style") {
        obj.armorProf = record.armorProf;
      }
      if (record.primeStat) {
        obj.primeStat = record.primeStat;
      }
      return obj;
    })
  };
  return JSON.stringify(root, null, 2);
}

function serializeClassOlc(data: ClassDataFile): string {
  const classes = data.classes.map((cls) => normalizeClassRecord(cls));
  const lines: string[] = [];
  lines.push(String(classes.length));
  lines.push("");
  classes.forEach((cls) => {
    const guilds = normalizeGuilds(cls.guilds);
    const titles = normalizeTitles(cls.titles);
    const flatTitles = titles.flatMap((pair) =>
      pair.map((title) => sanitizeOlcString(title))
    );
    lines.push("#CLASS");
    lines.push(`name ${sanitizeOlcString(cls.name)}~`);
    lines.push(`wname ${sanitizeOlcString(cls.whoName ?? "")}~`);
    lines.push(`basegrp ${sanitizeOlcString(cls.baseGroup ?? "")}~`);
    lines.push(`dfltgrp ${sanitizeOlcString(cls.defaultGroup ?? "")}~`);
    lines.push(`weap ${cls.weaponVnum ?? 0}`);
    lines.push(`armorprof ${sanitizeOlcString(cls.armorProf ?? "old_style")}~`);
    lines.push(`guild ${guilds.join(" ")} @`);
    const primeIndex = cls.primeStat
      ? PRIME_STAT_NAMES.indexOf(cls.primeStat)
      : 0;
    lines.push(`pstat ${primeIndex < 0 ? 0 : primeIndex}`);
    lines.push(`scap ${cls.skillCap ?? 0}`);
    lines.push(`t0_0 ${cls.thac0_00 ?? 0}`);
    lines.push(`t0_32 ${cls.thac0_32 ?? 0}`);
    lines.push(`hpmin ${cls.hpMin ?? 0}`);
    lines.push(`hpmax ${cls.hpMax ?? 0}`);
    lines.push(`fmana ${cls.manaUser ? "true" : "false"}`);
    lines.push(
      `titles ${flatTitles.map((entry) => `${entry}~`).join(" ")} @~`
    );
    lines.push(`start_loc ${cls.startLoc ?? 0}`);
    lines.push("#END");
    lines.push("");
  });
  return lines.join("\n");
}

function parseFlagBits(value: string | undefined, table: readonly string[]): string[] {
  if (!value) {
    return [];
  }
  const trimmed = value.trim();
  if (!trimmed || trimmed === "0") {
    return [];
  }
  if (/^-?\d+$/.test(trimmed)) {
    const bits = Number.parseInt(trimmed, 10);
    if (!Number.isFinite(bits)) {
      return [];
    }
    const flags: string[] = [];
    for (let i = 0; i < table.length; i += 1) {
      if (bits & (1 << i)) {
        flags.push(table[i]);
      }
    }
    return flags;
  }
  const flags: string[] = [];
  for (const char of trimmed) {
    let index = -1;
    if (char >= "A" && char <= "Z") {
      index = char.charCodeAt(0) - 65;
    } else if (char >= "a" && char <= "z") {
      index = char.charCodeAt(0) - 97 + 26;
    }
    if (index < 0 || index >= table.length) {
      continue;
    }
    const flag = table[index];
    if (!flags.includes(flag)) {
      flags.push(flag);
    }
  }
  return flags;
}

function flagIndexToChar(index: number): string {
  if (index < 26) {
    return String.fromCharCode(65 + index);
  }
  return String.fromCharCode(97 + index - 26);
}

function encodeFlagBits(
  values: string[] | undefined,
  table: readonly string[],
  defaults?: Record<string, string[]>
): string {
  if (!values || !values.length) {
    return "0";
  }
  const expanded = new Set<string>();
  values.forEach((value) => {
    const key = value.trim();
    if (!key) {
      return;
    }
    const mapped = defaults?.[key];
    if (mapped) {
      mapped.forEach((entry) => expanded.add(entry));
      return;
    }
    expanded.add(key);
  });
  let encoded = "";
  table.forEach((flag, index) => {
    if (expanded.has(flag)) {
      encoded += flagIndexToChar(index);
    }
  });
  return encoded.length ? encoded : "0";
}

function normalizeRaceStats(value: unknown): RaceStats | undefined {
  if (!value) {
    return undefined;
  }
  const stats: RaceStats = {};
  if (Array.isArray(value)) {
    RACE_STAT_KEYS.forEach((key, index) => {
      const parsed = Number(value[index]);
      stats[key] = Number.isFinite(parsed) ? parsed : 0;
    });
    return stats;
  }
  if (typeof value === "object") {
    RACE_STAT_KEYS.forEach((key) => {
      const record = value as Record<string, unknown>;
      const parsed = Number(record[key]);
      stats[key] = Number.isFinite(parsed) ? parsed : 0;
    });
    return stats;
  }
  return undefined;
}

function normalizeRaceFlagList(value: unknown): string[] | undefined {
  if (!Array.isArray(value)) {
    return undefined;
  }
  const flags = value
    .map((entry) => (typeof entry === "string" ? entry.trim() : ""))
    .filter((entry) => entry.length);
  return flags.length ? flags : undefined;
}

function normalizeRaceNumberList(value: unknown): number[] | undefined {
  if (!Array.isArray(value)) {
    return undefined;
  }
  const numbers = value
    .map((entry) => Number(entry))
    .filter((entry) => Number.isFinite(entry))
    .map((entry) => Math.trunc(entry));
  return numbers.length ? numbers : undefined;
}

function normalizeRaceMap(
  value: unknown
): Record<string, number> | number[] | undefined {
  if (Array.isArray(value)) {
    return normalizeRaceNumberList(value);
  }
  if (value && typeof value === "object") {
    const entries = Object.entries(value);
    if (!entries.length) {
      return undefined;
    }
    const next: Record<string, number> = {};
    entries.forEach(([key, entry]) => {
      const parsed = Number(entry);
      if (Number.isFinite(parsed)) {
        next[key] = Math.trunc(parsed);
      }
    });
    return Object.keys(next).length ? next : undefined;
  }
  return undefined;
}

function normalizeRaceSkills(value: unknown): string[] | undefined {
  if (!Array.isArray(value)) {
    return undefined;
  }
  const skills = value
    .map((entry) => (typeof entry === "string" ? entry.trim() : ""))
    .filter((entry) => entry.length);
  return skills.length ? skills.slice(0, RACE_SKILL_COUNT) : undefined;
}

function normalizeRaceRecord(record: Partial<RaceDefinition>): RaceDefinition {
  const name = record.name?.trim() || "unnamed";
  const whoName = record.whoName?.trim();
  const size = record.size?.trim();
  return {
    name,
    whoName,
    pc: record.pc ?? false,
    points:
      typeof record.points === "number" && Number.isFinite(record.points)
        ? Math.trunc(record.points)
        : 0,
    size: size && size.length ? size : undefined,
    stats: normalizeRaceStats(record.stats),
    maxStats: normalizeRaceStats(record.maxStats),
    actFlags: normalizeRaceFlagList(record.actFlags),
    affectFlags: normalizeRaceFlagList(record.affectFlags),
    offFlags: normalizeRaceFlagList(record.offFlags),
    immFlags: normalizeRaceFlagList(record.immFlags),
    resFlags: normalizeRaceFlagList(record.resFlags),
    vulnFlags: normalizeRaceFlagList(record.vulnFlags),
    formFlags: normalizeRaceFlagList(record.formFlags),
    partFlags: normalizeRaceFlagList(record.partFlags),
    classMult: normalizeRaceMap(record.classMult),
    startLoc:
      typeof record.startLoc === "number" && Number.isFinite(record.startLoc)
        ? Math.trunc(record.startLoc)
        : 0,
    classStart: normalizeRaceMap(record.classStart),
    skills: normalizeRaceSkills(record.skills)
  };
}

function buildClassNumberList(
  value: Record<string, number> | number[] | undefined,
  classNames: string[] | undefined,
  defaultValue: number
): number[] {
  if (Array.isArray(value)) {
    return value.map((entry) =>
      Number.isFinite(entry) ? Math.trunc(entry) : defaultValue
    );
  }
  if (classNames && classNames.length) {
    return classNames.map((name) => {
      if (value && typeof value === "object") {
        const parsed = Number((value as Record<string, unknown>)[name]);
        if (Number.isFinite(parsed)) {
          return Math.trunc(parsed);
        }
      }
      return defaultValue;
    });
  }
  if (value && typeof value === "object") {
    return Object.values(value).map((entry) =>
      Number.isFinite(entry) ? Math.trunc(entry) : defaultValue
    );
  }
  return [];
}

function hasAnyNonZero(stats: RaceStats | undefined): boolean {
  if (!stats) {
    return false;
  }
  return RACE_STAT_KEYS.some((key) => Number(stats[key]) !== 0);
}

function normalizeSkillClassMap(
  value: SkillDefinition["levels"] | SkillDefinition["ratings"]
): Record<string, number> | number[] | undefined {
  if (Array.isArray(value)) {
    return normalizeRaceNumberList(value);
  }
  if (value && typeof value === "object") {
    const entries = Object.entries(value);
    if (!entries.length) {
      return undefined;
    }
    const next: Record<string, number> = {};
    entries.forEach(([key, entry]) => {
      const parsed = Number(entry);
      if (Number.isFinite(parsed)) {
        next[key] = Math.trunc(parsed);
      }
    });
    return Object.keys(next).length ? next : undefined;
  }
  return undefined;
}

function normalizeSkillRecord(record: Partial<SkillDefinition>): SkillDefinition {
  return {
    name: record.name?.trim() || "unnamed",
    levels: normalizeSkillClassMap(record.levels),
    ratings: normalizeSkillClassMap(record.ratings),
    spell: record.spell?.trim(),
    loxSpell: record.loxSpell?.trim(),
    target: record.target?.trim(),
    minPosition: record.minPosition?.trim(),
    gsn: record.gsn?.trim(),
    slot:
      typeof record.slot === "number" && Number.isFinite(record.slot)
        ? Math.trunc(record.slot)
        : 0,
    minMana:
      typeof record.minMana === "number" && Number.isFinite(record.minMana)
        ? Math.trunc(record.minMana)
        : 0,
    beats:
      typeof record.beats === "number" && Number.isFinite(record.beats)
        ? Math.trunc(record.beats)
        : 0,
    nounDamage: record.nounDamage?.trim(),
    msgOff: record.msgOff?.trim(),
    msgObj: record.msgObj?.trim()
  };
}

function parseSkillJson(content: string): SkillDataFile {
  const parsed = JSON.parse(content) as Record<string, unknown>;
  const formatVersion = Number(parsed.formatVersion);
  const skillsRaw = parsed.skills;
  const skills = Array.isArray(skillsRaw)
    ? skillsRaw.map((skill) =>
        normalizeSkillRecord((skill ?? {}) as Partial<SkillDefinition>)
      )
    : [];
  return {
    formatVersion: Number.isFinite(formatVersion) ? formatVersion : 1,
    skills
  };
}

function parseSkillOlc(content: string): SkillDataFile {
  const scanner = new OlcScanner(content);
  const skills: SkillDefinition[] = [];

  while (!scanner.eof()) {
    const word = scanner.readWord();
    if (!word) {
      break;
    }
    const upper = word.toUpperCase();
    if (upper !== "#SKILL") {
      if (upper === "#END") {
        break;
      }
      continue;
    }
    const record: Partial<SkillDefinition> = {};
    while (!scanner.eof()) {
      const field = scanner.readWord();
      if (!field) {
        break;
      }
      const fieldUpper = field.toUpperCase();
      if (fieldUpper === "#END") {
        break;
      }
      switch (field.toLowerCase()) {
        case "name":
          record.name = scanner.readString();
          break;
        case "skill_level":
          record.levels = readOlcNumberList(scanner);
          break;
        case "rating":
          record.ratings = readOlcNumberList(scanner);
          break;
        case "spell_fun": {
          const value = scanner.readString();
          record.spell = value.trim().length ? value.trim() : undefined;
          break;
        }
        case "lox_spell": {
          const value = scanner.readString();
          record.loxSpell = value.trim().length ? value.trim() : undefined;
          break;
        }
        case "target":
          record.target = scanner.readString();
          break;
        case "minimum_position":
          record.minPosition = scanner.readString();
          break;
        case "pgsn": {
          const value = scanner.readString();
          record.gsn = value.trim().length ? value.trim() : undefined;
          break;
        }
        case "slot":
          record.slot = scanner.readNumber();
          break;
        case "min_mana":
          record.minMana = scanner.readNumber();
          break;
        case "beats":
          record.beats = scanner.readNumber();
          break;
        case "noun_damage": {
          const value = scanner.readString();
          record.nounDamage = value.trim().length ? value.trim() : undefined;
          break;
        }
        case "msg_off": {
          const value = scanner.readString();
          record.msgOff = value.trim().length ? value.trim() : undefined;
          break;
        }
        case "msg_obj": {
          const value = scanner.readString();
          record.msgObj = value.trim().length ? value.trim() : undefined;
          break;
        }
        default:
          break;
      }
    }
    skills.push(normalizeSkillRecord(record));
  }

  return { formatVersion: 1, skills };
}

function serializeSkillJson(
  data: SkillDataFile,
  classNames?: string[]
): string {
  const root: Record<string, unknown> = {
    formatVersion: 1,
    skills: data.skills.map((skill) => {
      const record = normalizeSkillRecord(skill);
      const obj: Record<string, unknown> = {
        name: record.name
      };
      if (record.levels && Object.keys(record.levels).length) {
        if (Array.isArray(record.levels) && classNames?.length) {
          obj.levels = Object.fromEntries(
            classNames.map((name, index) => [
              name,
              record.levels?.[index] ?? DEFAULT_SKILL_LEVEL
            ])
          );
        } else {
          obj.levels = record.levels;
        }
      }
      if (record.ratings && Object.keys(record.ratings).length) {
        if (Array.isArray(record.ratings) && classNames?.length) {
          obj.ratings = Object.fromEntries(
            classNames.map((name, index) => [
              name,
              record.ratings?.[index] ?? DEFAULT_SKILL_RATING
            ])
          );
        } else {
          obj.ratings = record.ratings;
        }
      }
      if (record.spell) {
        obj.spell = record.spell;
      }
      if (record.loxSpell) {
        obj.loxSpell = record.loxSpell;
      }
      if (record.target) {
        obj.target = record.target;
      }
      if (record.minPosition) {
        obj.minPosition = record.minPosition;
      }
      if (record.gsn) {
        obj.gsn = record.gsn;
      }
      if (record.slot) {
        obj.slot = record.slot;
      }
      if (record.minMana) {
        obj.minMana = record.minMana;
      }
      if (record.beats) {
        obj.beats = record.beats;
      }
      if (record.nounDamage) {
        obj.nounDamage = record.nounDamage;
      }
      if (record.msgOff) {
        obj.msgOff = record.msgOff;
      }
      if (record.msgObj) {
        obj.msgObj = record.msgObj;
      }
      return obj;
    })
  };
  return JSON.stringify(root, null, 2);
}

function serializeSkillOlc(
  data: SkillDataFile,
  classNames?: string[]
): string {
  const skills = data.skills.map((skill) => normalizeSkillRecord(skill));
  const lines: string[] = [];
  lines.push(String(skills.length));
  lines.push("");
  skills.forEach((skill) => {
    const levels = buildClassNumberList(
      skill.levels as Record<string, number> | number[] | undefined,
      classNames,
      DEFAULT_SKILL_LEVEL
    );
    const ratings = buildClassNumberList(
      skill.ratings as Record<string, number> | number[] | undefined,
      classNames,
      DEFAULT_SKILL_RATING
    );
    lines.push("#SKILL");
    lines.push(`name ${sanitizeOlcString(skill.name)}~`);
    lines.push(`skill_level ${levels.join(" ")} @`);
    lines.push(`rating ${ratings.join(" ")} @`);
    lines.push(`spell_fun ${sanitizeOlcString(skill.spell ?? "")}~`);
    lines.push(`lox_spell ${sanitizeOlcString(skill.loxSpell ?? "")}~`);
    lines.push(`target ${sanitizeOlcString(skill.target ?? "tar_ignore")}~`);
    lines.push(
      `minimum_position ${sanitizeOlcString(skill.minPosition ?? "dead")}~`
    );
    lines.push(`pgsn ${sanitizeOlcString(skill.gsn ?? "")}~`);
    lines.push(`slot ${skill.slot ?? 0}`);
    lines.push(`min_mana ${skill.minMana ?? 0}`);
    lines.push(`beats ${skill.beats ?? 0}`);
    lines.push(`noun_damage ${sanitizeOlcString(skill.nounDamage ?? "")}~`);
    lines.push(`msg_off ${sanitizeOlcString(skill.msgOff ?? "")}~`);
    lines.push(`msg_obj ${sanitizeOlcString(skill.msgObj ?? "")}~`);
    lines.push("#END");
    lines.push("");
  });
  return lines.join("\n");
}

function normalizeGroupRecord(record: Partial<GroupDefinition>): GroupDefinition {
  return {
    name: record.name?.trim() || "unnamed",
    ratings: normalizeSkillClassMap(record.ratings),
    skills: Array.isArray(record.skills)
      ? record.skills
          .map((entry) => (typeof entry === "string" ? entry.trim() : ""))
          .filter((entry) => entry.length)
          .slice(0, GROUP_SKILL_COUNT)
      : undefined
  };
}

function parseGroupJson(content: string): GroupDataFile {
  const parsed = JSON.parse(content) as Record<string, unknown>;
  const formatVersion = Number(parsed.formatVersion);
  const groupsRaw = parsed.groups;
  const groups = Array.isArray(groupsRaw)
    ? groupsRaw.map((group) =>
        normalizeGroupRecord((group ?? {}) as Partial<GroupDefinition>)
      )
    : [];
  return {
    formatVersion: Number.isFinite(formatVersion) ? formatVersion : 1,
    groups
  };
}

function parseGroupOlc(content: string): GroupDataFile {
  const scanner = new OlcScanner(content);
  const groups: GroupDefinition[] = [];

  while (!scanner.eof()) {
    const word = scanner.readWord();
    if (!word) {
      break;
    }
    const upper = word.toUpperCase();
    if (upper !== "#GROUP") {
      if (upper === "#END") {
        break;
      }
      continue;
    }
    const record: Partial<GroupDefinition> = {};
    while (!scanner.eof()) {
      const field = scanner.readWord();
      if (!field) {
        break;
      }
      const fieldUpper = field.toUpperCase();
      if (fieldUpper === "#END") {
        break;
      }
      switch (field.toLowerCase()) {
        case "name":
          record.name = scanner.readString();
          break;
        case "rating":
          record.ratings = readOlcNumberList(scanner);
          break;
        case "skills": {
          const skills: string[] = [];
          while (!scanner.eof()) {
            const value = scanner.readString();
            if (value === "@") {
              break;
            }
            if (value.trim().length) {
              skills.push(value.trim());
            }
          }
          record.skills = skills;
          break;
        }
        default:
          break;
      }
    }
    groups.push(normalizeGroupRecord(record));
  }

  return { formatVersion: 1, groups };
}

function serializeGroupJson(
  data: GroupDataFile,
  classNames?: string[]
): string {
  const root: Record<string, unknown> = {
    formatVersion: 1,
    groups: data.groups.map((group) => {
      const record = normalizeGroupRecord(group);
      const obj: Record<string, unknown> = {
        name: record.name
      };
      if (record.ratings && Object.keys(record.ratings).length) {
        if (Array.isArray(record.ratings) && classNames?.length) {
          obj.ratings = Object.fromEntries(
            classNames.map((name, index) => [
              name,
              record.ratings?.[index] ?? DEFAULT_SKILL_RATING
            ])
          );
        } else {
          obj.ratings = record.ratings;
        }
      }
      if (record.skills?.length) {
        obj.skills = record.skills;
      }
      return obj;
    })
  };
  return JSON.stringify(root, null, 2);
}

function serializeGroupOlc(
  data: GroupDataFile,
  classNames?: string[]
): string {
  const groups = data.groups.map((group) => normalizeGroupRecord(group));
  const lines: string[] = [];
  lines.push(String(groups.length));
  lines.push("");
  groups.forEach((group) => {
    const ratings = buildClassNumberList(
      group.ratings as Record<string, number> | number[] | undefined,
      classNames,
      DEFAULT_SKILL_RATING
    );
    const skills = (group.skills ?? []).slice(0, GROUP_SKILL_COUNT);
    lines.push("#GROUP");
    lines.push(`name ${sanitizeOlcString(group.name)}~`);
    lines.push(`rating ${ratings.join(" ")} @`);
    lines.push(
      `skills ${skills.map((entry) => `${sanitizeOlcString(entry)}~`).join(" ")} @~`
    );
    lines.push("#END");
    lines.push("");
  });
  return lines.join("\n");
}

function normalizeCommandRecord(
  record: Partial<CommandDefinition>
): CommandDefinition {
  return {
    name: record.name?.trim() || "unnamed",
    function: record.function?.trim() || DEFAULT_COMMAND_FUNCTION,
    position: record.position?.trim() || DEFAULT_COMMAND_POSITION,
    level:
      typeof record.level === "number" && Number.isFinite(record.level)
        ? Math.trunc(record.level)
        : 0,
    log: record.log?.trim() || DEFAULT_COMMAND_LOG,
    category: record.category?.trim() || DEFAULT_COMMAND_CATEGORY,
    loxFunction: record.loxFunction?.trim()
  };
}

function parseCommandJson(content: string): CommandDataFile {
  const parsed = JSON.parse(content) as Record<string, unknown>;
  const formatVersion = Number(parsed.formatVersion);
  const commandsRaw = parsed.commands;
  const commands = Array.isArray(commandsRaw)
    ? commandsRaw.map((command) =>
        normalizeCommandRecord((command ?? {}) as Partial<CommandDefinition>)
      )
    : [];
  return {
    formatVersion: Number.isFinite(formatVersion) ? formatVersion : 1,
    commands
  };
}

function parseCommandOlc(content: string): CommandDataFile {
  const scanner = new OlcScanner(content);
  const commands: CommandDefinition[] = [];

  while (!scanner.eof()) {
    const word = scanner.readWord();
    if (!word) {
      break;
    }
    const upper = word.toUpperCase();
    if (upper !== "#COMMAND") {
      if (upper === "#END") {
        break;
      }
      continue;
    }
    const record: Partial<CommandDefinition> = {};
    while (!scanner.eof()) {
      const field = scanner.readWord();
      if (!field) {
        break;
      }
      const fieldUpper = field.toUpperCase();
      if (fieldUpper === "#END") {
        break;
      }
      switch (field.toLowerCase()) {
        case "name":
          record.name = scanner.readString();
          break;
        case "do_fun":
          record.function = scanner.readString();
          break;
        case "position":
          record.position = scanner.readString();
          break;
        case "level":
          record.level = scanner.readNumber();
          break;
        case "log":
          record.log = scanner.readString();
          break;
        case "show":
          record.category = scanner.readString();
          break;
        case "lox_fun": {
          const value = scanner.readString();
          record.loxFunction = value.trim().length ? value.trim() : undefined;
          break;
        }
        default:
          break;
      }
    }
    commands.push(normalizeCommandRecord(record));
  }

  return { formatVersion: 1, commands };
}

function serializeCommandJson(data: CommandDataFile): string {
  const root: Record<string, unknown> = {
    formatVersion: 1,
    commands: data.commands.map((command) => {
      const record = normalizeCommandRecord(command);
      const obj: Record<string, unknown> = {
        name: record.name
      };
      if (record.function && record.function !== DEFAULT_COMMAND_FUNCTION) {
        obj.function = record.function;
      }
      if (record.position && record.position !== DEFAULT_COMMAND_POSITION) {
        obj.position = record.position;
      }
      if (record.level) {
        obj.level = record.level;
      }
      if (record.log && record.log !== DEFAULT_COMMAND_LOG) {
        obj.log = record.log;
      }
      if (record.category && record.category !== DEFAULT_COMMAND_CATEGORY) {
        obj.category = record.category;
      }
      if (record.loxFunction) {
        obj.loxFunction = record.loxFunction;
      }
      return obj;
    })
  };
  return JSON.stringify(root, null, 2);
}

function serializeCommandOlc(data: CommandDataFile): string {
  const commands = data.commands.map((command) => normalizeCommandRecord(command));
  const lines: string[] = [];
  lines.push(String(commands.length));
  lines.push("");
  commands.forEach((command) => {
    lines.push("#COMMAND");
    lines.push(`name ${sanitizeOlcString(command.name)}~`);
    lines.push(`do_fun ${sanitizeOlcString(command.function ?? DEFAULT_COMMAND_FUNCTION)}~`);
    lines.push(
      `position ${sanitizeOlcString(command.position ?? DEFAULT_COMMAND_POSITION)}~`
    );
    lines.push(`level ${command.level ?? 0}`);
    lines.push(`log ${sanitizeOlcString(command.log ?? DEFAULT_COMMAND_LOG)}~`);
    lines.push(`show ${sanitizeOlcString(command.category ?? DEFAULT_COMMAND_CATEGORY)}~`);
    lines.push(`lox_fun ${sanitizeOlcString(command.loxFunction ?? "")}~`);
    lines.push("#END");
    lines.push("");
  });
  return lines.join("\n");
}

function normalizeSocialText(value: unknown): string | undefined {
  if (typeof value !== "string") {
    return undefined;
  }
  const trimmed = value.trim();
  return trimmed.length ? trimmed : undefined;
}

function normalizeSocialRecord(
  record: Partial<SocialDefinition>
): SocialDefinition {
  return {
    name: record.name?.trim() || "unnamed",
    charNoArg: normalizeSocialText(record.charNoArg),
    othersNoArg: normalizeSocialText(record.othersNoArg),
    charFound: normalizeSocialText(record.charFound),
    othersFound: normalizeSocialText(record.othersFound),
    victFound: normalizeSocialText(record.victFound),
    charAuto: normalizeSocialText(record.charAuto),
    othersAuto: normalizeSocialText(record.othersAuto)
  };
}

function parseSocialJson(content: string): SocialDataFile {
  const parsed = JSON.parse(content) as Record<string, unknown>;
  const formatVersion = Number(parsed.formatVersion);
  const socialsRaw = parsed.socials;
  const socials = Array.isArray(socialsRaw)
    ? socialsRaw.map((social) =>
        normalizeSocialRecord((social ?? {}) as Partial<SocialDefinition>)
      )
    : [];
  return {
    formatVersion: Number.isFinite(formatVersion) ? formatVersion : 1,
    socials
  };
}

function parseSocialOlc(content: string): SocialDataFile {
  const scanner = new OlcScanner(content);
  const socials: SocialDefinition[] = [];

  while (!scanner.eof()) {
    const word = scanner.readWord();
    if (!word) {
      break;
    }
    const upper = word.toUpperCase();
    if (upper !== "#SOCIAL") {
      if (upper === "#END") {
        break;
      }
      continue;
    }
    const record: Partial<SocialDefinition> = {};
    while (!scanner.eof()) {
      const field = scanner.readWord();
      if (!field) {
        break;
      }
      const fieldUpper = field.toUpperCase();
      if (fieldUpper === "#END") {
        break;
      }
      switch (field.toLowerCase()) {
        case "name":
          record.name = scanner.readString();
          break;
        case "char_no_arg":
          record.charNoArg = scanner.readString();
          break;
        case "others_no_arg":
          record.othersNoArg = scanner.readString();
          break;
        case "char_found":
          record.charFound = scanner.readString();
          break;
        case "others_found":
          record.othersFound = scanner.readString();
          break;
        case "vict_found":
          record.victFound = scanner.readString();
          break;
        case "char_auto":
          record.charAuto = scanner.readString();
          break;
        case "others_auto":
          record.othersAuto = scanner.readString();
          break;
        default:
          break;
      }
    }
    socials.push(normalizeSocialRecord(record));
  }

  return { formatVersion: 1, socials };
}

function serializeSocialJson(data: SocialDataFile): string {
  const root: Record<string, unknown> = {
    formatVersion: 1,
    socials: data.socials.map((social) => {
      const record = normalizeSocialRecord(social);
      const obj: Record<string, unknown> = {
        name: record.name
      };
      if (record.charNoArg) {
        obj.charNoArg = record.charNoArg;
      }
      if (record.othersNoArg) {
        obj.othersNoArg = record.othersNoArg;
      }
      if (record.charFound) {
        obj.charFound = record.charFound;
      }
      if (record.othersFound) {
        obj.othersFound = record.othersFound;
      }
      if (record.victFound) {
        obj.victFound = record.victFound;
      }
      if (record.charAuto) {
        obj.charAuto = record.charAuto;
      }
      if (record.othersAuto) {
        obj.othersAuto = record.othersAuto;
      }
      return obj;
    })
  };
  return JSON.stringify(root, null, 2);
}

function serializeSocialOlc(data: SocialDataFile): string {
  const socials = data.socials.map((social) => normalizeSocialRecord(social));
  const lines: string[] = [];
  lines.push(String(socials.length));
  lines.push("");
  socials.forEach((social) => {
    lines.push("#SOCIAL");
    lines.push(`name ${sanitizeOlcString(social.name)}~`);
    lines.push(`char_no_arg ${sanitizeOlcString(social.charNoArg ?? "")}~`);
    lines.push(`others_no_arg ${sanitizeOlcString(social.othersNoArg ?? "")}~`);
    lines.push(`char_found ${sanitizeOlcString(social.charFound ?? "")}~`);
    lines.push(`others_found ${sanitizeOlcString(social.othersFound ?? "")}~`);
    lines.push(`vict_found ${sanitizeOlcString(social.victFound ?? "")}~`);
    lines.push(`char_auto ${sanitizeOlcString(social.charAuto ?? "")}~`);
    lines.push(`others_auto ${sanitizeOlcString(social.othersAuto ?? "")}~`);
    lines.push("#END");
    lines.push("");
  });
  return lines.join("\n");
}

function normalizeTutorialText(value: unknown): string | undefined {
  if (typeof value !== "string") {
    return undefined;
  }
  const trimmed = value.trim();
  return trimmed.length ? trimmed : undefined;
}

function normalizeTutorialStep(step: Partial<TutorialStep>): TutorialStep | null {
  const prompt = normalizeTutorialText(step.prompt);
  if (!prompt) {
    return null;
  }
  const match = normalizeTutorialText(step.match);
  return {
    prompt,
    match
  };
}

function normalizeTutorialRecord(
  record: Partial<TutorialDefinition>
): TutorialDefinition {
  const minLevel =
    typeof record.minLevel === "number" && Number.isFinite(record.minLevel)
      ? Math.trunc(record.minLevel)
      : 0;
  const steps = Array.isArray(record.steps)
    ? record.steps
        .map((step) => normalizeTutorialStep(step ?? {}))
        .filter((step): step is TutorialStep => Boolean(step))
    : [];
  return {
    name: record.name?.trim() || "unnamed",
    blurb: normalizeTutorialText(record.blurb),
    finish: normalizeTutorialText(record.finish),
    minLevel,
    steps
  };
}

function parseTutorialJson(content: string): TutorialDataFile {
  const parsed = JSON.parse(content) as Record<string, unknown>;
  const formatVersion = Number(parsed.formatVersion);
  const tutorialsRaw = parsed.tutorials;
  const tutorials = Array.isArray(tutorialsRaw)
    ? tutorialsRaw.map((tutorial) =>
        normalizeTutorialRecord((tutorial ?? {}) as Partial<TutorialDefinition>)
      )
    : [];
  return {
    formatVersion: Number.isFinite(formatVersion) ? formatVersion : 1,
    tutorials
  };
}

function parseTutorialOlc(content: string): TutorialDataFile {
  const scanner = new OlcScanner(content);
  const tutorials: TutorialDefinition[] = [];
  let current: TutorialDefinition | null = null;

  while (!scanner.eof()) {
    const word = scanner.readWord();
    if (!word) {
      break;
    }
    const upper = word.toUpperCase();
    if (upper === "#TUTORIAL") {
      if (current) {
        tutorials.push(current);
      }
      const record: Partial<TutorialDefinition> = {};
      let stepCount: number | null = null;
      while (!scanner.eof()) {
        const field = scanner.readWord();
        if (!field) {
          break;
        }
        const fieldUpper = field.toUpperCase();
        if (fieldUpper === "#ENDTUTORIAL") {
          break;
        }
        switch (field.toLowerCase()) {
          case "name":
            record.name = scanner.readString();
            break;
          case "blurb":
            record.blurb = scanner.readString();
            break;
          case "finish":
            record.finish = scanner.readString();
            break;
          case "min_level":
            record.minLevel = scanner.readNumber();
            break;
          case "step_count":
            stepCount = scanner.readNumber();
            break;
          default:
            break;
        }
      }
      current = normalizeTutorialRecord(record);
      if (stepCount !== null) {
        current.steps = [];
      }
      continue;
    }
    if (upper === "#STEP") {
      const record: Partial<TutorialStep> = {};
      while (!scanner.eof()) {
        const field = scanner.readWord();
        if (!field) {
          break;
        }
        const fieldUpper = field.toUpperCase();
        if (fieldUpper === "#ENDSTEP") {
          break;
        }
        switch (field.toLowerCase()) {
          case "prompt":
            record.prompt = scanner.readString();
            break;
          case "match":
            record.match = scanner.readString();
            break;
          default:
            break;
        }
      }
      if (current) {
        const step = normalizeTutorialStep(record);
        if (step) {
          current.steps = [...(current.steps ?? []), step];
        }
      }
      continue;
    }
    if (upper === "#END") {
      break;
    }
  }

  if (current) {
    tutorials.push(current);
  }

  return { formatVersion: 1, tutorials };
}

function serializeTutorialJson(data: TutorialDataFile): string {
  const root: Record<string, unknown> = {
    formatVersion: 1,
    tutorials: data.tutorials.map((tutorial) => {
      const record = normalizeTutorialRecord(tutorial);
      const obj: Record<string, unknown> = {
        name: record.name
      };
      if (record.blurb) {
        obj.blurb = record.blurb;
      }
      if (record.finish) {
        obj.finish = record.finish;
      }
      if (record.minLevel && record.minLevel !== 0) {
        obj.minLevel = record.minLevel;
      }
      if (record.steps && record.steps.length) {
        obj.steps = record.steps.map((step) => {
          const stepObj: Record<string, unknown> = {
            prompt: step.prompt
          };
          if (step.match) {
            stepObj.match = step.match;
          }
          return stepObj;
        });
      }
      return obj;
    })
  };
  return JSON.stringify(root, null, 2);
}

function serializeTutorialOlc(data: TutorialDataFile): string {
  const tutorials = data.tutorials.map((tutorial) =>
    normalizeTutorialRecord(tutorial)
  );
  const lines: string[] = [];
  lines.push(String(tutorials.length));
  lines.push("");
  tutorials.forEach((tutorial) => {
    const steps = tutorial.steps ?? [];
    lines.push("#TUTORIAL");
    lines.push(`name ${sanitizeOlcString(tutorial.name)}~`);
    lines.push(`blurb ${sanitizeOlcString(tutorial.blurb ?? "")}~`);
    lines.push(`finish ${sanitizeOlcString(tutorial.finish ?? "")}~`);
    lines.push(`min_level ${tutorial.minLevel ?? 0}`);
    lines.push(`step_count ${steps.length}`);
    lines.push("#ENDTUTORIAL");
    lines.push("");
    steps.forEach((step) => {
      lines.push("#STEP");
      lines.push(`prompt ${sanitizeOlcString(step.prompt)}~`);
      lines.push(`match ${sanitizeOlcString(step.match ?? "")}~`);
      lines.push("#ENDSTEP");
      lines.push("");
    });
  });
  return lines.join("\n");
}

function parseRaceJson(content: string): RaceDataFile {
  const parsed = JSON.parse(content) as Record<string, unknown>;
  const formatVersion = Number(parsed.formatVersion);
  const racesRaw = parsed.races;
  const races = Array.isArray(racesRaw)
    ? racesRaw.map((race) =>
        normalizeRaceRecord((race ?? {}) as Partial<RaceDefinition>)
      )
    : [];
  return {
    formatVersion: Number.isFinite(formatVersion) ? formatVersion : 1,
    races
  };
}

function parseRaceOlc(content: string): RaceDataFile {
  const scanner = new OlcScanner(content);
  const races: RaceDefinition[] = [];

  while (!scanner.eof()) {
    const word = scanner.readWord();
    if (!word) {
      break;
    }
    const upper = word.toUpperCase();
    if (upper !== "#RACE") {
      if (upper === "#END") {
        break;
      }
      continue;
    }
    const record: Partial<RaceDefinition> = {};
    while (!scanner.eof()) {
      const field = scanner.readWord();
      if (!field) {
        break;
      }
      const fieldUpper = field.toUpperCase();
      if (fieldUpper === "#END") {
        break;
      }
      switch (field.toLowerCase()) {
        case "name":
          record.name = scanner.readString();
          break;
        case "pc": {
          const value = scanner.readWord();
          const normalized = value ? value.toLowerCase() : "";
          record.pc = normalized === "true" || normalized === "1";
          break;
        }
        case "act":
          record.actFlags = parseFlagBits(scanner.readWord(), actFlags);
          break;
        case "aff":
          record.affectFlags = parseFlagBits(scanner.readWord(), affectFlags);
          break;
        case "off":
          record.offFlags = parseFlagBits(scanner.readWord(), offFlags);
          break;
        case "imm":
          record.immFlags = parseFlagBits(scanner.readWord(), immFlags);
          break;
        case "res":
          record.resFlags = parseFlagBits(scanner.readWord(), resFlags);
          break;
        case "vuln":
          record.vulnFlags = parseFlagBits(scanner.readWord(), VULN_FLAG_BITS);
          break;
        case "form":
          record.formFlags = parseFlagBits(scanner.readWord(), FORM_FLAG_BITS);
          break;
        case "parts":
          record.partFlags = parseFlagBits(scanner.readWord(), PART_FLAG_BITS);
          break;
        case "points":
          record.points = scanner.readNumber();
          break;
        case "class_mult":
          record.classMult = normalizeRaceNumberList(readOlcNumberList(scanner));
          break;
        case "who_name":
          record.whoName = scanner.readString();
          break;
        case "skills": {
          const skills: string[] = [];
          while (!scanner.eof()) {
            const value = scanner.readString();
            if (value === "@") {
              break;
            }
            if (value.trim().length) {
              skills.push(value.trim());
            }
          }
          record.skills = skills;
          break;
        }
        case "stats":
          record.stats = normalizeRaceStats(readOlcNumberList(scanner));
          break;
        case "max_stats":
          record.maxStats = normalizeRaceStats(readOlcNumberList(scanner));
          break;
        case "size":
          record.size = scanner.readString();
          break;
        case "start_loc":
          record.startLoc = scanner.readNumber();
          break;
        case "class_start":
          record.classStart = normalizeRaceNumberList(readOlcNumberList(scanner));
          break;
        default:
          break;
      }
    }
    races.push(normalizeRaceRecord(record));
  }

  return { formatVersion: 1, races };
}

function serializeRaceJson(data: RaceDataFile): string {
  const root: Record<string, unknown> = {
    formatVersion: 1,
    races: data.races.map((race) => {
      const record = normalizeRaceRecord(race);
      const obj: Record<string, unknown> = {
        name: record.name
      };
      if (record.whoName) {
        obj.whoName = record.whoName;
      }
      if (record.pc) {
        obj.pc = true;
      }
      if (record.points) {
        obj.points = record.points;
      }
      if (record.size) {
        obj.size = record.size;
      }
      if (record.stats && hasAnyNonZero(record.stats)) {
        obj.stats = record.stats;
      }
      if (record.maxStats && hasAnyNonZero(record.maxStats)) {
        obj.maxStats = record.maxStats;
      }
      if (record.actFlags?.length) {
        obj.actFlags = record.actFlags;
      }
      if (record.affectFlags?.length) {
        obj.affectFlags = record.affectFlags;
      }
      if (record.offFlags?.length) {
        obj.offFlags = record.offFlags;
      }
      if (record.immFlags?.length) {
        obj.immFlags = record.immFlags;
      }
      if (record.resFlags?.length) {
        obj.resFlags = record.resFlags;
      }
      if (record.vulnFlags?.length) {
        obj.vulnFlags = record.vulnFlags;
      }
      if (record.formFlags?.length) {
        obj.formFlags = record.formFlags;
      }
      if (record.partFlags?.length) {
        obj.partFlags = record.partFlags;
      }
      if (record.classMult && Object.keys(record.classMult).length) {
        obj.classMult = record.classMult;
      }
      if (record.startLoc) {
        obj.startLoc = record.startLoc;
      }
      if (record.classStart && Object.keys(record.classStart).length) {
        obj.classStart = record.classStart;
      }
      if (record.skills?.length) {
        obj.skills = record.skills;
      }
      return obj;
    })
  };
  return JSON.stringify(root, null, 2);
}

function serializeRaceOlc(
  data: RaceDataFile,
  classNames?: string[]
): string {
  const races = data.races.map((race) => normalizeRaceRecord(race));
  const lines: string[] = [];
  lines.push(String(races.length));
  lines.push("");
  races.forEach((race) => {
    const classMult = buildClassNumberList(
      race.classMult as Record<string, number> | number[] | undefined,
      classNames,
      100
    );
    const classStart = buildClassNumberList(
      race.classStart as Record<string, number> | number[] | undefined,
      classNames,
      0
    );
    const stats =
      race.stats && hasAnyNonZero(race.stats)
        ? RACE_STAT_KEYS.map((key) => Number(race.stats?.[key] ?? 0))
        : RACE_STAT_KEYS.map(() => 0);
    const maxStats =
      race.maxStats && hasAnyNonZero(race.maxStats)
        ? RACE_STAT_KEYS.map((key) => Number(race.maxStats?.[key] ?? 0))
        : RACE_STAT_KEYS.map(() => 0);
    const skills = (race.skills ?? []).slice(0, RACE_SKILL_COUNT);
    lines.push("#RACE");
    lines.push(`name ${sanitizeOlcString(race.name)}~`);
    lines.push(`pc ${race.pc ? "true" : "false"}`);
    lines.push(`act ${encodeFlagBits(race.actFlags, actFlags)}`);
    lines.push(`aff ${encodeFlagBits(race.affectFlags, affectFlags)}`);
    lines.push(`off ${encodeFlagBits(race.offFlags, offFlags)}`);
    lines.push(`imm ${encodeFlagBits(race.immFlags, immFlags)}`);
    lines.push(`res ${encodeFlagBits(race.resFlags, resFlags)}`);
    lines.push(`vuln ${encodeFlagBits(race.vulnFlags, VULN_FLAG_BITS)}`);
    lines.push(
      `form ${encodeFlagBits(race.formFlags, FORM_FLAG_BITS, RACE_FORM_DEFAULTS)}`
    );
    lines.push(
      `parts ${encodeFlagBits(
        race.partFlags,
        PART_FLAG_BITS,
        RACE_PART_DEFAULTS
      )}`
    );
    lines.push(`points ${race.points ?? 0}`);
    lines.push(
      `class_mult ${classMult.length ? classMult.join(" ") : ""} @`
    );
    lines.push(`who_name ${sanitizeOlcString(race.whoName ?? "")}~`);
    lines.push(
      `skills ${skills.map((entry) => `${sanitizeOlcString(entry)}~`).join(" ")} @~`
    );
    lines.push(`stats ${stats.join(" ")} @`);
    lines.push(`max_stats ${maxStats.join(" ")} @`);
    lines.push(`size ${sanitizeOlcString(race.size ?? "medium")}~`);
    lines.push(`start_loc ${race.startLoc ?? 0}`);
    lines.push(
      `class_start ${classStart.length ? classStart.join(" ") : ""} @`
    );
    lines.push("#END");
    lines.push("");
  });
  return lines.join("\n");
}

function parseVnumRange(value: unknown): [number, number] | null {
  if (!Array.isArray(value) || value.length < 2) {
    return null;
  }
  const start = Number(value[0]);
  const end = Number(value[1]);
  if (!Number.isFinite(start) || !Number.isFinite(end)) {
    return null;
  }
  return [start, end];
}

const exitDirectionAliases: Record<string, AreaExitDirection> = {
  n: "north",
  north: "north",
  e: "east",
  east: "east",
  s: "south",
  south: "south",
  w: "west",
  west: "west",
  u: "up",
  up: "up",
  d: "down",
  down: "down"
};

function normalizeExitDirection(value: unknown): AreaExitDirection | null {
  if (typeof value !== "string") {
    return null;
  }
  const key = value.trim().toLowerCase();
  return exitDirectionAliases[key] ?? null;
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

function stripRoomLegacyFields(area: AreaJson): AreaJson {
  if (!area || typeof area !== "object") {
    return area;
  }
  const record = area as Record<string, unknown>;
  const rooms = record.rooms;
  if (!Array.isArray(rooms)) {
    return area;
  }
  const nextRooms = rooms.map((room) => {
    if (!room || typeof room !== "object") {
      return room;
    }
    const next = { ...(room as Record<string, unknown>) };
    delete next.manaRate;
    delete next.healRate;
    delete next.clan;
    delete next.owner;
    return next;
  });
  return { ...record, rooms: nextRooms } as AreaJson;
}

async function tryReadText(path: string): Promise<string | null> {
  try {
    return await readTextFile(path);
  } catch (error) {
    if (isMissingFileError(error)) {
      return null;
    }
    throw error;
  }
}

async function loadJsonNames(
  path: string,
  key: string
): Promise<string[] | null> {
  const raw = await tryReadText(path);
  if (!raw) {
    return null;
  }
  const parsed = JSON.parse(raw) as Record<string, unknown>;
  const list = parsed[key];
  if (!Array.isArray(list)) {
    return null;
  }
  const names = list
    .map((entry) => {
      if (typeof entry === "string") {
        return entry;
      }
      if (entry && typeof entry === "object" && "name" in entry) {
        return String((entry as Record<string, unknown>).name);
      }
      return null;
    })
    .filter((entry): entry is string => Boolean(entry));

  return names.length ? names : null;
}

async function loadOlcNames(path: string): Promise<string[] | null> {
  const raw = await tryReadText(path);
  if (!raw) {
    return null;
  }
  const names = parseOlcNames(raw);
  return names.length ? names : null;
}

async function loadReferenceList(
  dataDir: string,
  baseName: string,
  jsonKey: string,
  fileName?: string
): Promise<string[]> {
  if (fileName) {
    const path = isAbsolutePath(fileName) ? fileName : await join(dataDir, fileName);
    if (fileName.toLowerCase().endsWith(".json")) {
      const jsonList = await loadJsonNames(path, jsonKey);
      if (jsonList) {
        return jsonList;
      }
    } else if (fileName.toLowerCase().endsWith(".olc")) {
      const olcList = await loadOlcNames(path);
      if (olcList) {
        return olcList;
      }
    } else {
      const jsonList = await loadJsonNames(`${path}.json`, jsonKey);
      if (jsonList) {
        return jsonList;
      }
      const olcList = await loadOlcNames(`${path}.olc`);
      if (olcList) {
        return olcList;
      }
    }
  }

  const jsonPath = await join(dataDir, `${baseName}.json`);
  const jsonList = await loadJsonNames(jsonPath, jsonKey);
  if (jsonList) {
    return jsonList;
  }

  const olcPath = await join(dataDir, `${baseName}.olc`);
  const olcList = await loadOlcNames(olcPath);
  if (olcList) {
    return olcList;
  }

  throw new Error(`Missing reference data for ${baseName}.`);
}

export class LocalFileRepository implements WorldRepository {
  async pickAreaDirectory(defaultPath?: string | null): Promise<string | null> {
    const resolvedDefault = await defaultDialogPath(defaultPath);
    const selection = await open({
      multiple: false,
      directory: true,
      defaultPath: resolvedDefault ?? undefined
    });

    return typeof selection === "string" ? selection : null;
  }

  async pickConfigFile(defaultPath?: string | null): Promise<string | null> {
    const resolvedDefault = await defaultDialogPath(defaultPath);
    const selection = await open({
      multiple: false,
      directory: false,
      defaultPath: resolvedDefault ?? undefined,
      filters: [cfgFilter]
    });

    return typeof selection === "string" ? selection : null;
  }

  async pickAreaFile(defaultPath?: string | null): Promise<string | null> {
    const resolvedDefault = await defaultDialogPath(defaultPath);
    const selection = await open({
      multiple: false,
      directory: false,
      defaultPath: resolvedDefault ?? undefined,
      filters: [jsonFilter]
    });

    return typeof selection === "string" ? selection : null;
  }

  async pickSaveFile(defaultPath?: string | null): Promise<string | null> {
    const fallback = await defaultDialogPath(defaultPath ?? null);
    const selection = await save({
      defaultPath: fallback ?? undefined,
      filters: [jsonFilter]
    });

    return typeof selection === "string" ? selection : null;
  }

  editorMetaPathForArea(areaPath: string): string {
    return editorMetaPathForArea(areaPath);
  }

  async loadProjectConfig(path: string): Promise<ProjectConfig> {
    const raw = await readTextFile(path);
    const data = parseConfig(raw);
    const rootDir = await dirname(path);
    const format = normalizeFormat(data.default_format);
    const dataFiles = buildDataFiles(data, format);
    const areaDirValue = data.area_dir ?? "area";
    const dataDirValue = data.data_dir ?? "data";
    const areaList = data.area_list ?? "area.lst";
    const areaDir = isAbsolutePath(areaDirValue)
      ? areaDirValue
      : await join(rootDir, areaDirValue);
    const dataDir = isAbsolutePath(dataDirValue)
      ? dataDirValue
      : await join(rootDir, dataDirValue);
    return {
      path,
      rootDir,
      areaDir,
      areaList,
      dataDir,
      defaultFormat: format,
      dataFiles
    };
  }

  async resolveDataDirectory(
    areaPath: string | null,
    areaDirectory: string | null
  ): Promise<string | null> {
    let baseDir: string | null = areaDirectory;
    if (!baseDir && areaPath) {
      baseDir = await dirname(areaPath);
    }
    if (!baseDir) {
      return null;
    }

    const baseName = await basename(baseDir);
    if (baseName === "data") {
      return baseDir;
    }
    if (baseName === "area") {
      const parent = await dirname(baseDir);
      return await join(parent, "data");
    }

    return await join(baseDir, "data");
  }

  async resolveAreaDirectory(areaPath: string): Promise<string | null> {
    if (!areaPath) {
      return null;
    }
    return await dirname(areaPath);
  }

  async loadArea(path: string): Promise<AreaJson> {
    const raw = await readTextFile(path);
    const parsed = JSON.parse(raw) as AreaJson;
    return stripRoomLegacyFields(parsed);
  }

  async saveArea(path: string, area: AreaJson): Promise<void> {
    const payload = canonicalStringify(stripRoomLegacyFields(area));
    await writeTextFile(path, payload);
  }

  async loadAreaIndex(
    areaDir: string,
    areaListFile?: string
  ): Promise<AreaIndexEntry[]> {
    const listName = areaListFile ?? "area.lst";
    const listPath = isAbsolutePath(listName)
      ? listName
      : await join(areaDir, listName);
    const rawList = await tryReadText(listPath);
    if (!rawList) {
      return [];
    }
    const files = rawList
      .split(/\r?\n/)
      .map((line) => stripTilde(line).trim())
      .filter((line) => line && !line.startsWith("#"));
    const entries: AreaIndexEntry[] = [];
    const seen = new Set<string>();

    for (const fileName of files) {
      if (!fileName.toLowerCase().endsWith(".json")) {
        continue;
      }
      if (seen.has(fileName)) {
        continue;
      }
      seen.add(fileName);
      const path = await join(areaDir, fileName);
      const raw = await tryReadText(path);
      if (!raw) {
        continue;
      }
      try {
        const parsed = JSON.parse(raw) as Record<string, unknown>;
        const areadata =
          parsed.areadata && typeof parsed.areadata === "object"
            ? (parsed.areadata as Record<string, unknown>)
            : null;
        const name =
          areadata && typeof areadata.name === "string"
            ? areadata.name
            : fileName;
        const vnumRange = parseVnumRange(areadata?.vnumRange);
        entries.push({
          fileName,
          name,
          vnumRange
        });
      } catch {
        continue;
      }
    }

    return entries.sort((a, b) => {
      const aStart = a.vnumRange ? a.vnumRange[0] : Number.MAX_SAFE_INTEGER;
      const bStart = b.vnumRange ? b.vnumRange[0] : Number.MAX_SAFE_INTEGER;
      return aStart - bStart;
    });
  }

  async loadAreaGraphLinks(
    areaDir: string,
    areaIndex: AreaIndexEntry[]
  ): Promise<AreaGraphLink[]> {
    if (!areaIndex.length) {
      return [];
    }
    const entries = areaIndex.filter((entry) =>
      entry.fileName.toLowerCase().endsWith(".json")
    );
    if (!entries.length) {
      return [];
    }
    const links = new Map<string, AreaGraphLink>();

    for (const entry of entries) {
      const path = await join(areaDir, entry.fileName);
      const raw = await tryReadText(path);
      if (!raw) {
        continue;
      }
      let parsed: Record<string, unknown>;
      try {
        parsed = JSON.parse(raw) as Record<string, unknown>;
      } catch {
        continue;
      }
      const rooms = Array.isArray(parsed.rooms) ? parsed.rooms : [];
      if (!rooms.length) {
        continue;
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

      for (const room of rooms) {
        if (!room || typeof room !== "object") {
          continue;
        }
        const record = room as Record<string, unknown>;
        const exits = Array.isArray(record.exits) ? record.exits : [];
        for (const exit of exits) {
          if (!exit || typeof exit !== "object") {
            continue;
          }
          const exitRecord = exit as Record<string, unknown>;
          const toVnum = parseVnum(exitRecord.toVnum);
          if (toVnum === null) {
            continue;
          }
          const inRange = entry.vnumRange
            ? toVnum >= entry.vnumRange[0] && toVnum <= entry.vnumRange[1]
            : roomVnums.has(toVnum);
          if (inRange) {
            continue;
          }
          const target = findAreaForVnum(areaIndex, toVnum);
          if (!target || target.fileName === entry.fileName) {
            continue;
          }
          const key = `${entry.fileName}::${target.fileName}`;
          const existing = links.get(key);
          const dir = normalizeExitDirection(exitRecord.dir);
          if (existing) {
            existing.count += 1;
            if (dir) {
              existing.directionCounts[dir] =
                (existing.directionCounts[dir] ?? 0) + 1;
            }
          } else {
            links.set(key, {
              fromFile: entry.fileName,
              toFile: target.fileName,
              count: 1,
              directionCounts: dir ? { [dir]: 1 } : {}
            });
          }
        }
      }
    }

    return Array.from(links.values()).sort((a, b) => {
      if (a.fromFile === b.fromFile) {
        return a.toFile.localeCompare(b.toFile);
      }
      return a.fromFile.localeCompare(b.fromFile);
    });
  }

  async loadEditorMeta(path: string): Promise<EditorMeta | null> {
    try {
      const raw = await readTextFile(path);
      return JSON.parse(raw) as EditorMeta;
    } catch (error) {
      if (isMissingFileError(error)) {
        const legacyPath = legacyMetaPathFor(path);
        if (!legacyPath || legacyPath === path) {
          return null;
        }
        try {
          const raw = await readTextFile(legacyPath);
          return JSON.parse(raw) as EditorMeta;
        } catch (legacyError) {
          if (isMissingFileError(legacyError)) {
            return null;
          }
          throw legacyError;
        }
      }
      throw error;
    }
  }

  async saveEditorMeta(path: string, meta: EditorMeta): Promise<void> {
    const payload = canonicalStringify(meta);
    const { dir } = splitPath(path);
    if (dir) {
      await mkdir(dir, { recursive: true });
    }
    await writeTextFile(path, payload);
  }

  async loadReferenceData(
    dataDir: string,
    dataFiles?: Partial<ProjectDataFiles>
  ): Promise<ReferenceData> {
    const classes = await loadReferenceList(
      dataDir,
      "classes",
      "classes",
      dataFiles?.classes
    );
    const races = await loadReferenceList(
      dataDir,
      "races",
      "races",
      dataFiles?.races
    );
    const skills = await loadReferenceList(
      dataDir,
      "skills",
      "skills",
      dataFiles?.skills
    );
    const groups = await loadReferenceList(
      dataDir,
      "groups",
      "groups",
      dataFiles?.groups
    );
    const commands = await loadReferenceList(
      dataDir,
      "commands",
      "commands",
      dataFiles?.commands
    );
    const socials = await loadReferenceList(
      dataDir,
      "socials",
      "socials",
      dataFiles?.socials
    );
    const tutorials = await loadReferenceList(
      dataDir,
      "tutorials",
      "tutorials",
      dataFiles?.tutorials
    );

    return {
      classes,
      races,
      skills,
      groups,
      commands,
      socials,
      tutorials,
      sourceDir: dataDir
    };
  }

  async loadClassesData(
    dataDir: string,
    fileName?: string,
    defaultFormat?: "json" | "olc"
  ): Promise<ClassDataSource> {
    const candidates: Array<{ path: string; format: "json" | "olc" }> = [];
    if (fileName) {
      const lower = fileName.toLowerCase();
      if (lower.endsWith(".json")) {
        candidates.push({
          path: isAbsolutePath(fileName) ? fileName : await join(dataDir, fileName),
          format: "json"
        });
      } else if (lower.endsWith(".olc")) {
        candidates.push({
          path: isAbsolutePath(fileName) ? fileName : await join(dataDir, fileName),
          format: "olc"
        });
      } else if (defaultFormat) {
        candidates.push({
          path: isAbsolutePath(fileName)
            ? `${fileName}.${defaultFormat}`
            : await join(dataDir, `${fileName}.${defaultFormat}`),
          format: defaultFormat
        });
      } else {
        candidates.push({
          path: isAbsolutePath(fileName)
            ? `${fileName}.json`
            : await join(dataDir, `${fileName}.json`),
          format: "json"
        });
        candidates.push({
          path: isAbsolutePath(fileName)
            ? `${fileName}.olc`
            : await join(dataDir, `${fileName}.olc`),
          format: "olc"
        });
      }
    }
    if (!candidates.length) {
      candidates.push({
        path: await join(dataDir, "classes.json"),
        format: "json"
      });
      candidates.push({
        path: await join(dataDir, "classes.olc"),
        format: "olc"
      });
    }
    for (const candidate of candidates) {
      const raw = await tryReadText(candidate.path);
      if (!raw) {
        continue;
      }
      return {
        path: candidate.path,
        format: candidate.format,
        data:
          candidate.format === "json"
            ? parseClassJson(raw)
            : parseClassOlc(raw)
      };
    }

    throw new Error("Missing classes data file.");
  }

  async saveClassesData(
    dataDir: string,
    data: ClassDataFile,
    format: "json" | "olc",
    fileName?: string
  ): Promise<string> {
    const resolvedName = fileName ?? (format === "json" ? "classes.json" : "classes.olc");
    const lower = resolvedName.toLowerCase();
    const outputFormat =
      lower.endsWith(".json") ? "json" : lower.endsWith(".olc") ? "olc" : format;
    const path = isAbsolutePath(resolvedName)
      ? resolvedName
      : await join(dataDir, resolvedName);
    const payload =
      outputFormat === "json"
        ? serializeClassJson(data)
        : serializeClassOlc(data);
    await writeTextFile(path, payload);
    return path;
  }

  async loadRacesData(
    dataDir: string,
    fileName?: string,
    defaultFormat?: "json" | "olc"
  ): Promise<RaceDataSource> {
    const candidates: Array<{ path: string; format: "json" | "olc" }> = [];
    if (fileName) {
      const lower = fileName.toLowerCase();
      if (lower.endsWith(".json")) {
        candidates.push({
          path: isAbsolutePath(fileName) ? fileName : await join(dataDir, fileName),
          format: "json"
        });
      } else if (lower.endsWith(".olc")) {
        candidates.push({
          path: isAbsolutePath(fileName) ? fileName : await join(dataDir, fileName),
          format: "olc"
        });
      } else if (defaultFormat) {
        candidates.push({
          path: isAbsolutePath(fileName)
            ? `${fileName}.${defaultFormat}`
            : await join(dataDir, `${fileName}.${defaultFormat}`),
          format: defaultFormat
        });
      } else {
        candidates.push({
          path: isAbsolutePath(fileName)
            ? `${fileName}.json`
            : await join(dataDir, `${fileName}.json`),
          format: "json"
        });
        candidates.push({
          path: isAbsolutePath(fileName)
            ? `${fileName}.olc`
            : await join(dataDir, `${fileName}.olc`),
          format: "olc"
        });
      }
    }
    if (!candidates.length) {
      candidates.push({
        path: await join(dataDir, "races.json"),
        format: "json"
      });
      candidates.push({
        path: await join(dataDir, "races.olc"),
        format: "olc"
      });
    }
    for (const candidate of candidates) {
      const raw = await tryReadText(candidate.path);
      if (!raw) {
        continue;
      }
      return {
        path: candidate.path,
        format: candidate.format,
        data:
          candidate.format === "json"
            ? parseRaceJson(raw)
            : parseRaceOlc(raw)
      };
    }

    throw new Error("Missing races data file.");
  }

  async saveRacesData(
    dataDir: string,
    data: RaceDataFile,
    format: "json" | "olc",
    fileName?: string,
    classNames?: string[]
  ): Promise<string> {
    const resolvedName = fileName ?? (format === "json" ? "races.json" : "races.olc");
    const lower = resolvedName.toLowerCase();
    const outputFormat =
      lower.endsWith(".json") ? "json" : lower.endsWith(".olc") ? "olc" : format;
    const path = isAbsolutePath(resolvedName)
      ? resolvedName
      : await join(dataDir, resolvedName);
    const payload =
      outputFormat === "json"
        ? serializeRaceJson(data)
        : serializeRaceOlc(data, classNames);
    await writeTextFile(path, payload);
    return path;
  }

  async loadSkillsData(
    dataDir: string,
    fileName?: string,
    defaultFormat?: "json" | "olc"
  ): Promise<SkillDataSource> {
    const candidates: Array<{ path: string; format: "json" | "olc" }> = [];
    if (fileName) {
      const lower = fileName.toLowerCase();
      if (lower.endsWith(".json")) {
        candidates.push({
          path: isAbsolutePath(fileName) ? fileName : await join(dataDir, fileName),
          format: "json"
        });
      } else if (lower.endsWith(".olc")) {
        candidates.push({
          path: isAbsolutePath(fileName) ? fileName : await join(dataDir, fileName),
          format: "olc"
        });
      } else if (defaultFormat) {
        candidates.push({
          path: isAbsolutePath(fileName)
            ? `${fileName}.${defaultFormat}`
            : await join(dataDir, `${fileName}.${defaultFormat}`),
          format: defaultFormat
        });
      } else {
        candidates.push({
          path: isAbsolutePath(fileName)
            ? `${fileName}.json`
            : await join(dataDir, `${fileName}.json`),
          format: "json"
        });
        candidates.push({
          path: isAbsolutePath(fileName)
            ? `${fileName}.olc`
            : await join(dataDir, `${fileName}.olc`),
          format: "olc"
        });
      }
    }
    if (!candidates.length) {
      candidates.push({
        path: await join(dataDir, "skills.json"),
        format: "json"
      });
      candidates.push({
        path: await join(dataDir, "skills.olc"),
        format: "olc"
      });
    }
    for (const candidate of candidates) {
      const raw = await tryReadText(candidate.path);
      if (!raw) {
        continue;
      }
      return {
        path: candidate.path,
        format: candidate.format,
        data:
          candidate.format === "json"
            ? parseSkillJson(raw)
            : parseSkillOlc(raw)
      };
    }

    throw new Error("Missing skills data file.");
  }

  async saveSkillsData(
    dataDir: string,
    data: SkillDataFile,
    format: "json" | "olc",
    fileName?: string,
    classNames?: string[]
  ): Promise<string> {
    const resolvedName = fileName ?? (format === "json" ? "skills.json" : "skills.olc");
    const lower = resolvedName.toLowerCase();
    const outputFormat =
      lower.endsWith(".json") ? "json" : lower.endsWith(".olc") ? "olc" : format;
    const path = isAbsolutePath(resolvedName)
      ? resolvedName
      : await join(dataDir, resolvedName);
    const payload =
      outputFormat === "json"
        ? serializeSkillJson(data, classNames)
        : serializeSkillOlc(data, classNames);
    await writeTextFile(path, payload);
    return path;
  }

  async loadGroupsData(
    dataDir: string,
    fileName?: string,
    defaultFormat?: "json" | "olc"
  ): Promise<GroupDataSource> {
    const candidates: Array<{ path: string; format: "json" | "olc" }> = [];
    if (fileName) {
      const lower = fileName.toLowerCase();
      if (lower.endsWith(".json")) {
        candidates.push({
          path: isAbsolutePath(fileName) ? fileName : await join(dataDir, fileName),
          format: "json"
        });
      } else if (lower.endsWith(".olc")) {
        candidates.push({
          path: isAbsolutePath(fileName) ? fileName : await join(dataDir, fileName),
          format: "olc"
        });
      } else if (defaultFormat) {
        candidates.push({
          path: isAbsolutePath(fileName)
            ? `${fileName}.${defaultFormat}`
            : await join(dataDir, `${fileName}.${defaultFormat}`),
          format: defaultFormat
        });
      } else {
        candidates.push({
          path: isAbsolutePath(fileName)
            ? `${fileName}.json`
            : await join(dataDir, `${fileName}.json`),
          format: "json"
        });
        candidates.push({
          path: isAbsolutePath(fileName)
            ? `${fileName}.olc`
            : await join(dataDir, `${fileName}.olc`),
          format: "olc"
        });
      }
    }
    if (!candidates.length) {
      candidates.push({
        path: await join(dataDir, "groups.json"),
        format: "json"
      });
      candidates.push({
        path: await join(dataDir, "groups.olc"),
        format: "olc"
      });
    }
    for (const candidate of candidates) {
      const raw = await tryReadText(candidate.path);
      if (!raw) {
        continue;
      }
      return {
        path: candidate.path,
        format: candidate.format,
        data:
          candidate.format === "json"
            ? parseGroupJson(raw)
            : parseGroupOlc(raw)
      };
    }

    throw new Error("Missing groups data file.");
  }

  async saveGroupsData(
    dataDir: string,
    data: GroupDataFile,
    format: "json" | "olc",
    fileName?: string,
    classNames?: string[]
  ): Promise<string> {
    const resolvedName = fileName ?? (format === "json" ? "groups.json" : "groups.olc");
    const lower = resolvedName.toLowerCase();
    const outputFormat =
      lower.endsWith(".json") ? "json" : lower.endsWith(".olc") ? "olc" : format;
    const path = isAbsolutePath(resolvedName)
      ? resolvedName
      : await join(dataDir, resolvedName);
    const payload =
      outputFormat === "json"
        ? serializeGroupJson(data, classNames)
        : serializeGroupOlc(data, classNames);
    await writeTextFile(path, payload);
    return path;
  }

  async loadCommandsData(
    dataDir: string,
    fileName?: string,
    defaultFormat?: "json" | "olc"
  ): Promise<CommandDataSource> {
    const candidates: Array<{ path: string; format: "json" | "olc" }> = [];
    if (fileName) {
      const lower = fileName.toLowerCase();
      if (lower.endsWith(".json")) {
        candidates.push({
          path: isAbsolutePath(fileName) ? fileName : await join(dataDir, fileName),
          format: "json"
        });
      } else if (lower.endsWith(".olc")) {
        candidates.push({
          path: isAbsolutePath(fileName) ? fileName : await join(dataDir, fileName),
          format: "olc"
        });
      } else if (defaultFormat) {
        candidates.push({
          path: isAbsolutePath(fileName)
            ? `${fileName}.${defaultFormat}`
            : await join(dataDir, `${fileName}.${defaultFormat}`),
          format: defaultFormat
        });
      } else {
        candidates.push({
          path: isAbsolutePath(fileName)
            ? `${fileName}.json`
            : await join(dataDir, `${fileName}.json`),
          format: "json"
        });
        candidates.push({
          path: isAbsolutePath(fileName)
            ? `${fileName}.olc`
            : await join(dataDir, `${fileName}.olc`),
          format: "olc"
        });
      }
    }
    if (!candidates.length) {
      candidates.push({
        path: await join(dataDir, "commands.json"),
        format: "json"
      });
      candidates.push({
        path: await join(dataDir, "commands.olc"),
        format: "olc"
      });
    }
    for (const candidate of candidates) {
      const raw = await tryReadText(candidate.path);
      if (!raw) {
        continue;
      }
      return {
        path: candidate.path,
        format: candidate.format,
        data:
          candidate.format === "json"
            ? parseCommandJson(raw)
            : parseCommandOlc(raw)
      };
    }

    throw new Error("Missing commands data file.");
  }

  async saveCommandsData(
    dataDir: string,
    data: CommandDataFile,
    format: "json" | "olc",
    fileName?: string
  ): Promise<string> {
    const resolvedName =
      fileName ?? (format === "json" ? "commands.json" : "commands.olc");
    const lower = resolvedName.toLowerCase();
    const outputFormat =
      lower.endsWith(".json") ? "json" : lower.endsWith(".olc") ? "olc" : format;
    const path = isAbsolutePath(resolvedName)
      ? resolvedName
      : await join(dataDir, resolvedName);
    const payload =
      outputFormat === "json"
        ? serializeCommandJson(data)
        : serializeCommandOlc(data);
    await writeTextFile(path, payload);
    return path;
  }

  async loadSocialsData(
    dataDir: string,
    fileName?: string,
    defaultFormat?: "json" | "olc"
  ): Promise<SocialDataSource> {
    const candidates: Array<{ path: string; format: "json" | "olc" }> = [];
    if (fileName) {
      const lower = fileName.toLowerCase();
      if (lower.endsWith(".json")) {
        candidates.push({
          path: isAbsolutePath(fileName) ? fileName : await join(dataDir, fileName),
          format: "json"
        });
      } else if (lower.endsWith(".olc")) {
        candidates.push({
          path: isAbsolutePath(fileName) ? fileName : await join(dataDir, fileName),
          format: "olc"
        });
      } else if (defaultFormat) {
        candidates.push({
          path: isAbsolutePath(fileName)
            ? `${fileName}.${defaultFormat}`
            : await join(dataDir, `${fileName}.${defaultFormat}`),
          format: defaultFormat
        });
      } else {
        candidates.push({
          path: isAbsolutePath(fileName)
            ? `${fileName}.json`
            : await join(dataDir, `${fileName}.json`),
          format: "json"
        });
        candidates.push({
          path: isAbsolutePath(fileName)
            ? `${fileName}.olc`
            : await join(dataDir, `${fileName}.olc`),
          format: "olc"
        });
      }
    }
    if (!candidates.length) {
      candidates.push({
        path: await join(dataDir, "socials.json"),
        format: "json"
      });
      candidates.push({
        path: await join(dataDir, "socials.olc"),
        format: "olc"
      });
    }
    for (const candidate of candidates) {
      const raw = await tryReadText(candidate.path);
      if (!raw) {
        continue;
      }
      return {
        path: candidate.path,
        format: candidate.format,
        data:
          candidate.format === "json"
            ? parseSocialJson(raw)
            : parseSocialOlc(raw)
      };
    }

    throw new Error("Missing socials data file.");
  }

  async saveSocialsData(
    dataDir: string,
    data: SocialDataFile,
    format: "json" | "olc",
    fileName?: string
  ): Promise<string> {
    const resolvedName =
      fileName ?? (format === "json" ? "socials.json" : "socials.olc");
    const lower = resolvedName.toLowerCase();
    const outputFormat =
      lower.endsWith(".json") ? "json" : lower.endsWith(".olc") ? "olc" : format;
    const path = isAbsolutePath(resolvedName)
      ? resolvedName
      : await join(dataDir, resolvedName);
    const payload =
      outputFormat === "json"
        ? serializeSocialJson(data)
        : serializeSocialOlc(data);
    await writeTextFile(path, payload);
    return path;
  }

  async loadTutorialsData(
    dataDir: string,
    fileName?: string,
    defaultFormat?: "json" | "olc"
  ): Promise<TutorialDataSource> {
    const candidates: Array<{ path: string; format: "json" | "olc" }> = [];
    if (fileName) {
      const lower = fileName.toLowerCase();
      if (lower.endsWith(".json")) {
        candidates.push({
          path: isAbsolutePath(fileName) ? fileName : await join(dataDir, fileName),
          format: "json"
        });
      } else if (lower.endsWith(".olc")) {
        candidates.push({
          path: isAbsolutePath(fileName) ? fileName : await join(dataDir, fileName),
          format: "olc"
        });
      } else if (defaultFormat) {
        candidates.push({
          path: isAbsolutePath(fileName)
            ? `${fileName}.${defaultFormat}`
            : await join(dataDir, `${fileName}.${defaultFormat}`),
          format: defaultFormat
        });
      } else {
        candidates.push({
          path: isAbsolutePath(fileName)
            ? `${fileName}.json`
            : await join(dataDir, `${fileName}.json`),
          format: "json"
        });
        candidates.push({
          path: isAbsolutePath(fileName)
            ? `${fileName}.olc`
            : await join(dataDir, `${fileName}.olc`),
          format: "olc"
        });
      }
    }
    if (!candidates.length) {
      candidates.push({
        path: await join(dataDir, "tutorials.json"),
        format: "json"
      });
      candidates.push({
        path: await join(dataDir, "tutorials.olc"),
        format: "olc"
      });
    }
    for (const candidate of candidates) {
      const raw = await tryReadText(candidate.path);
      if (!raw) {
        continue;
      }
      return {
        path: candidate.path,
        format: candidate.format,
        data:
          candidate.format === "json"
            ? parseTutorialJson(raw)
            : parseTutorialOlc(raw)
      };
    }

    throw new Error("Missing tutorials data file.");
  }

  async saveTutorialsData(
    dataDir: string,
    data: TutorialDataFile,
    format: "json" | "olc",
    fileName?: string
  ): Promise<string> {
    const resolvedName =
      fileName ?? (format === "json" ? "tutorials.json" : "tutorials.olc");
    const lower = resolvedName.toLowerCase();
    const outputFormat =
      lower.endsWith(".json") ? "json" : lower.endsWith(".olc") ? "olc" : format;
    const path = isAbsolutePath(resolvedName)
      ? resolvedName
      : await join(dataDir, resolvedName);
    const payload =
      outputFormat === "json"
        ? serializeTutorialJson(data)
        : serializeTutorialOlc(data);
    await writeTextFile(path, payload);
    return path;
  }

  async listLegacyAreaFiles(areaDir: string): Promise<string[]> {
    const entries = await readDir(areaDir);
    const legacyFiles: string[] = [];

    for (const entry of entries) {
      if (!entry.isFile) {
        continue;
      }
      const name = entry.name ?? "";
      if (!name.toLowerCase().endsWith(".are")) {
        continue;
      }
      if (LEGACY_AREA_IGNORE.has(name.toLowerCase())) {
        continue;
      }
      const jsonName = name.replace(/\.are$/i, ".json");
      const jsonPath = await join(areaDir, jsonName);
      if (await exists(jsonPath)) {
        continue;
      }
      legacyFiles.push(name);
    }

    return legacyFiles.sort((a, b) => a.localeCompare(b));
  }
}
