import type { AreaJson, EditorMeta } from "./types";

export interface WorldRepository {
  pickAreaDirectory(defaultPath?: string | null): Promise<string | null>;
  pickAreaFile(defaultPath?: string | null): Promise<string | null>;
  pickSaveFile(defaultPath?: string | null): Promise<string | null>;
  editorMetaPathForArea(areaPath: string): string;
  loadArea(path: string): Promise<AreaJson>;
  saveArea(path: string, area: AreaJson): Promise<void>;
  loadEditorMeta(path: string): Promise<EditorMeta | null>;
  saveEditorMeta(path: string, meta: EditorMeta): Promise<void>;
}
