import { open, save } from "@tauri-apps/plugin-dialog";
import { readTextFile, writeTextFile } from "@tauri-apps/plugin-fs";
import { basename, dirname, homeDir, join } from "@tauri-apps/api/path";
import { canonicalStringify } from "../utils/canonicalJson";
import type { AreaJson, EditorMeta, ReferenceData } from "./types";
import type { WorldRepository } from "./worldRepository";

const jsonFilter = {
  name: "Mud98 Area JSON",
  extensions: ["json"]
};

async function defaultDialogPath(
  fallbackPath?: string | null
): Promise<string | null> {
  if (fallbackPath) {
    return fallbackPath;
  }
  return await homeDir();
}

function editorMetaPathForArea(areaPath: string): string {
  if (areaPath.endsWith(".editor.json")) {
    return areaPath;
  }
  if (areaPath.endsWith(".json")) {
    return areaPath.replace(/\.json$/i, ".editor.json");
  }
  return `${areaPath}.editor.json`;
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

  async loadArea(path: string): Promise<AreaJson> {
    const raw = await readTextFile(path);
    return JSON.parse(raw) as AreaJson;
  }

  async saveArea(path: string, area: AreaJson): Promise<void> {
    const payload = canonicalStringify(area);
    await writeTextFile(path, payload);
  }

  async loadEditorMeta(path: string): Promise<EditorMeta | null> {
    try {
      const raw = await readTextFile(path);
      return JSON.parse(raw) as EditorMeta;
    } catch (error) {
      if (isMissingFileError(error)) {
        return null;
      }
      throw error;
    }
  }

  async saveEditorMeta(path: string, meta: EditorMeta): Promise<void> {
    const payload = canonicalStringify(meta);
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
}
