import { open, save } from "@tauri-apps/plugin-dialog";
import { readTextFile, writeTextFile } from "@tauri-apps/plugin-fs";
import { homeDir } from "@tauri-apps/api/path";
import { canonicalStringify } from "../utils/canonicalJson";
import type { AreaJson, EditorMeta } from "./types";
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
}
