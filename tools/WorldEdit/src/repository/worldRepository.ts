import type {
  AreaGraphLink,
  AreaIndexEntry,
  AreaJson,
  ClassDataFile,
  ClassDataSource,
  EditorMeta,
  ProjectConfig,
  ProjectDataFiles,
  ReferenceData
} from "./types";

export interface WorldRepository {
  pickAreaDirectory(defaultPath?: string | null): Promise<string | null>;
  pickConfigFile(defaultPath?: string | null): Promise<string | null>;
  pickAreaFile(defaultPath?: string | null): Promise<string | null>;
  pickSaveFile(defaultPath?: string | null): Promise<string | null>;
  editorMetaPathForArea(areaPath: string): string;
  resolveAreaDirectory(areaPath: string): Promise<string | null>;
  resolveDataDirectory(
    areaPath: string | null,
    areaDirectory: string | null
  ): Promise<string | null>;
  loadProjectConfig(path: string): Promise<ProjectConfig>;
  loadArea(path: string): Promise<AreaJson>;
  saveArea(path: string, area: AreaJson): Promise<void>;
  loadAreaIndex(areaDir: string, areaListFile?: string): Promise<AreaIndexEntry[]>;
  loadAreaGraphLinks(
    areaDir: string,
    areaIndex: AreaIndexEntry[]
  ): Promise<AreaGraphLink[]>;
  loadEditorMeta(path: string): Promise<EditorMeta | null>;
  saveEditorMeta(path: string, meta: EditorMeta): Promise<void>;
  loadReferenceData(
    dataDir: string,
    dataFiles?: Partial<ProjectDataFiles>
  ): Promise<ReferenceData>;
  loadClassesData(
    dataDir: string,
    fileName?: string,
    defaultFormat?: "json" | "olc"
  ): Promise<ClassDataSource>;
  saveClassesData(
    dataDir: string,
    data: ClassDataFile,
    format: "json" | "olc",
    fileName?: string
  ): Promise<string>;
  listLegacyAreaFiles(areaDir: string): Promise<string[]>;
}
