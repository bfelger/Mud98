import type {
  AreaIndexEntry,
  AreaJson,
  EditorMeta,
  ReferenceData
} from "./types";

export interface WorldRepository {
  pickAreaDirectory(defaultPath?: string | null): Promise<string | null>;
  pickAreaFile(defaultPath?: string | null): Promise<string | null>;
  pickSaveFile(defaultPath?: string | null): Promise<string | null>;
  editorMetaPathForArea(areaPath: string): string;
  resolveDataDirectory(
    areaPath: string | null,
    areaDirectory: string | null
  ): Promise<string | null>;
  loadArea(path: string): Promise<AreaJson>;
  saveArea(path: string, area: AreaJson): Promise<void>;
  loadAreaIndex(areaDir: string): Promise<AreaIndexEntry[]>;
  loadEditorMeta(path: string): Promise<EditorMeta | null>;
  saveEditorMeta(path: string, meta: EditorMeta): Promise<void>;
  loadReferenceData(dataDir: string): Promise<ReferenceData>;
  listLegacyAreaFiles(areaDir: string): Promise<string[]>;
}
