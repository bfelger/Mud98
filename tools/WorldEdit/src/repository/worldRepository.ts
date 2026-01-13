import type { AreaJson } from "./types";

export interface WorldRepository {
  pickAreaDirectory(defaultPath?: string | null): Promise<string | null>;
  pickAreaFile(defaultPath?: string | null): Promise<string | null>;
  pickSaveFile(defaultPath?: string | null): Promise<string | null>;
  loadArea(path: string): Promise<AreaJson>;
  saveArea(path: string, area: AreaJson): Promise<void>;
}
