import type {
  AreaGraphLink,
  AreaIndexEntry,
  AreaJson,
  ClassDataFile,
  ClassDataSource,
  EditorMeta,
  ReferenceData
} from "./types";

export interface WorldRepository {
  pickAreaDirectory(defaultPath?: string | null): Promise<string | null>;
  pickAreaFile(defaultPath?: string | null): Promise<string | null>;
  pickSaveFile(defaultPath?: string | null): Promise<string | null>;
  editorMetaPathForArea(areaPath: string): string;
  resolveAreaDirectory(areaPath: string): Promise<string | null>;
  resolveDataDirectory(
    areaPath: string | null,
    areaDirectory: string | null
  ): Promise<string | null>;
  loadArea(path: string): Promise<AreaJson>;
  saveArea(path: string, area: AreaJson): Promise<void>;
  loadAreaIndex(areaDir: string): Promise<AreaIndexEntry[]>;
  loadAreaGraphLinks(
    areaDir: string,
    areaIndex: AreaIndexEntry[]
  ): Promise<AreaGraphLink[]>;
  loadEditorMeta(path: string): Promise<EditorMeta | null>;
  saveEditorMeta(path: string, meta: EditorMeta): Promise<void>;
  loadReferenceData(dataDir: string): Promise<ReferenceData>;
  loadClassesData(dataDir: string): Promise<ClassDataSource>;
  saveClassesData(
    dataDir: string,
    data: ClassDataFile,
    format: "json" | "olc"
  ): Promise<string>;
  listLegacyAreaFiles(areaDir: string): Promise<string[]>;
}
