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
import type {
  AreaExitDirection,
  AreaGraphLink,
  AreaIndexEntry,
  AreaJson,
  ClassDataFile,
  ClassDataSource,
  ClassDefinition,
  EditorMeta,
  ReferenceData
} from "./types";
import type { WorldRepository } from "./worldRepository";

const jsonFilter = {
  name: "Mud98 Area JSON",
  extensions: ["json"]
};

const LEGACY_AREA_IGNORE = new Set(["help.are", "group.are", "music.txt", "olc.hlp", "rom.are"]);
const CLASS_MAX_LEVEL = 60;
const CLASS_TITLE_PAIRS = CLASS_MAX_LEVEL + 1;
const CLASS_GUILD_COUNT = 2;
const PRIME_STAT_NAMES = ["str", "int", "wis", "dex", "con"];
const ARMOR_PROF_NAMES = ["old_style", "cloth", "light", "medium", "heavy"];

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
  jsonKey: string
): Promise<string[]> {
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

  async loadAreaIndex(areaDir: string): Promise<AreaIndexEntry[]> {
    const listPath = await join(areaDir, "area.lst");
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

  async loadReferenceData(dataDir: string): Promise<ReferenceData> {
    const classes = await loadReferenceList(dataDir, "classes", "classes");
    const races = await loadReferenceList(dataDir, "races", "races");
    const skills = await loadReferenceList(dataDir, "skills", "skills");
    const groups = await loadReferenceList(dataDir, "groups", "groups");
    const commands = await loadReferenceList(dataDir, "commands", "commands");
    const socials = await loadReferenceList(dataDir, "socials", "socials");
    const tutorials = await loadReferenceList(dataDir, "tutorials", "tutorials");

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

  async loadClassesData(dataDir: string): Promise<ClassDataSource> {
    const jsonPath = await join(dataDir, "classes.json");
    const jsonRaw = await tryReadText(jsonPath);
    if (jsonRaw) {
      return {
        path: jsonPath,
        format: "json",
        data: parseClassJson(jsonRaw)
      };
    }

    const olcPath = await join(dataDir, "classes.olc");
    const olcRaw = await tryReadText(olcPath);
    if (olcRaw) {
      return {
        path: olcPath,
        format: "olc",
        data: parseClassOlc(olcRaw)
      };
    }

    throw new Error("Missing classes data file.");
  }

  async saveClassesData(
    dataDir: string,
    data: ClassDataFile,
    format: "json" | "olc"
  ): Promise<string> {
    const fileName = format === "json" ? "classes.json" : "classes.olc";
    const path = await join(dataDir, fileName);
    const payload =
      format === "json" ? serializeClassJson(data) : serializeClassOlc(data);
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
