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
  AreaGraphLink,
  AreaIndexEntry,
  AreaJson,
  EditorMeta,
  ReferenceData
} from "./types";
import type { WorldRepository } from "./worldRepository";

const jsonFilter = {
  name: "Mud98 Area JSON",
  extensions: ["json"]
};

const LEGACY_AREA_IGNORE = new Set(["help.are", "group.are", "music.txt", "olc.hlp", "rom.are"]);

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
          const pair =
            entry.fileName < target.fileName
              ? [entry.fileName, target.fileName]
              : [target.fileName, entry.fileName];
          const key = `${pair[0]}::${pair[1]}`;
          const existing = links.get(key);
          if (existing) {
            existing.count += 1;
          } else {
            links.set(key, {
              fromFile: pair[0],
              toFile: pair[1],
              count: 1
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
    const commands = await loadReferenceList(dataDir, "commands", "commands");

    return {
      classes,
      races,
      skills,
      commands,
      sourceDir: dataDir
    };
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
