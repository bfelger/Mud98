import { open, save } from "@tauri-apps/plugin-dialog";
import { readTextFile, writeTextFile } from "@tauri-apps/plugin-fs";
import { homeDir } from "@tauri-apps/api/path";
import { canonicalStringify } from "../utils/canonicalJson";
import type { AreaJson } from "./types";
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

  async loadArea(path: string): Promise<AreaJson> {
    const raw = await readTextFile(path);
    return JSON.parse(raw) as AreaJson;
  }

  async saveArea(path: string, area: AreaJson): Promise<void> {
    const payload = canonicalStringify(area);
    await writeTextFile(path, payload);
  }
}
