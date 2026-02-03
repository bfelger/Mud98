/**
 * Global data files store using Zustand.
 *
 * Manages state for global data files (classes, races, skills, etc.):
 * - Data content
 * - File paths
 * - File format (json/olc)
 * - Directory paths
 *
 * This consolidates ~32 useState calls into a single store with
 * a uniform structure for each data type.
 */

import { create } from "zustand";
import { devtools } from "zustand/middleware";
import type {
  ClassDataFile,
  RaceDataFile,
  SkillDataFile,
  GroupDataFile,
  CommandDataFile,
  SocialDataFile,
  TutorialDataFile,
  LootDataFile,
  ReferenceData,
  ProjectConfig,
  EditorMeta
} from "../repository/types";

/** File format options. */
export type DataFileFormat = "json" | "olc" | null;

/** State for a single global data file. */
export interface DataFileState<T> {
  data: T | null;
  path: string | null;
  format: DataFileFormat;
  dir: string | null;
}

/** Full global data store interface. */
export interface GlobalDataState {
  // Class data
  classData: ClassDataFile | null;
  classDataPath: string | null;
  classDataFormat: DataFileFormat;
  classDataDir: string | null;

  // Race data
  raceData: RaceDataFile | null;
  raceDataPath: string | null;
  raceDataFormat: DataFileFormat;
  raceDataDir: string | null;

  // Skill data
  skillData: SkillDataFile | null;
  skillDataPath: string | null;
  skillDataFormat: DataFileFormat;
  skillDataDir: string | null;

  // Group data
  groupData: GroupDataFile | null;
  groupDataPath: string | null;
  groupDataFormat: DataFileFormat;
  groupDataDir: string | null;

  // Command data
  commandData: CommandDataFile | null;
  commandDataPath: string | null;
  commandDataFormat: DataFileFormat;
  commandDataDir: string | null;

  // Social data
  socialData: SocialDataFile | null;
  socialDataPath: string | null;
  socialDataFormat: DataFileFormat;
  socialDataDir: string | null;

  // Tutorial data
  tutorialData: TutorialDataFile | null;
  tutorialDataPath: string | null;
  tutorialDataFormat: DataFileFormat;
  tutorialDataDir: string | null;

  // Loot data (global)
  lootData: LootDataFile | null;
  lootDataPath: string | null;
  lootDataFormat: DataFileFormat;
  lootDataDir: string | null;

  // Reference data (read-only lookups)
  referenceData: ReferenceData | null;

  // Project config
  projectConfig: ProjectConfig | null;

  // Editor metadata
  editorMeta: EditorMeta | null;
  editorMetaPath: string | null;

  // Directories
  dataDirectory: string | null;

  // Actions - Class data
  setClassData: (data: ClassDataFile | null) => void;
  setClassDataPath: (path: string | null) => void;
  setClassDataFormat: (format: DataFileFormat) => void;
  setClassDataDir: (dir: string | null) => void;
  setClassFile: (state: DataFileState<ClassDataFile>) => void;

  // Actions - Race data
  setRaceData: (data: RaceDataFile | null) => void;
  setRaceDataPath: (path: string | null) => void;
  setRaceDataFormat: (format: DataFileFormat) => void;
  setRaceDataDir: (dir: string | null) => void;
  setRaceFile: (state: DataFileState<RaceDataFile>) => void;

  // Actions - Skill data
  setSkillData: (data: SkillDataFile | null) => void;
  setSkillDataPath: (path: string | null) => void;
  setSkillDataFormat: (format: DataFileFormat) => void;
  setSkillDataDir: (dir: string | null) => void;
  setSkillFile: (state: DataFileState<SkillDataFile>) => void;

  // Actions - Group data
  setGroupData: (data: GroupDataFile | null) => void;
  setGroupDataPath: (path: string | null) => void;
  setGroupDataFormat: (format: DataFileFormat) => void;
  setGroupDataDir: (dir: string | null) => void;
  setGroupFile: (state: DataFileState<GroupDataFile>) => void;

  // Actions - Command data
  setCommandData: (data: CommandDataFile | null) => void;
  setCommandDataPath: (path: string | null) => void;
  setCommandDataFormat: (format: DataFileFormat) => void;
  setCommandDataDir: (dir: string | null) => void;
  setCommandFile: (state: DataFileState<CommandDataFile>) => void;

  // Actions - Social data
  setSocialData: (data: SocialDataFile | null) => void;
  setSocialDataPath: (path: string | null) => void;
  setSocialDataFormat: (format: DataFileFormat) => void;
  setSocialDataDir: (dir: string | null) => void;
  setSocialFile: (state: DataFileState<SocialDataFile>) => void;

  // Actions - Tutorial data
  setTutorialData: (data: TutorialDataFile | null) => void;
  setTutorialDataPath: (path: string | null) => void;
  setTutorialDataFormat: (format: DataFileFormat) => void;
  setTutorialDataDir: (dir: string | null) => void;
  setTutorialFile: (state: DataFileState<TutorialDataFile>) => void;

  // Actions - Loot data
  setLootData: (data: LootDataFile | null) => void;
  setLootDataPath: (path: string | null) => void;
  setLootDataFormat: (format: DataFileFormat) => void;
  setLootDataDir: (dir: string | null) => void;
  setLootFile: (state: DataFileState<LootDataFile>) => void;

  // Actions - Other
  setReferenceData: (data: ReferenceData | null) => void;
  setProjectConfig: (config: ProjectConfig | null) => void;
  setEditorMeta: (meta: EditorMeta | null) => void;
  setEditorMetaPath: (path: string | null) => void;
  setDataDirectory: (dir: string | null) => void;

  // Batch actions
  clearAllGlobalData: () => void;
}

/** Initial null state for a data file. */
const nullFileState = <T>(): DataFileState<T> => ({
  data: null,
  path: null,
  format: null,
  dir: null
});

/**
 * Zustand store for global data files.
 */
export const useGlobalDataStore = create<GlobalDataState>()(devtools(
  (set) => ({
  // Initial state
  classData: null,
  classDataPath: null,
  classDataFormat: null,
  classDataDir: null,

  raceData: null,
  raceDataPath: null,
  raceDataFormat: null,
  raceDataDir: null,

  skillData: null,
  skillDataPath: null,
  skillDataFormat: null,
  skillDataDir: null,

  groupData: null,
  groupDataPath: null,
  groupDataFormat: null,
  groupDataDir: null,

  commandData: null,
  commandDataPath: null,
  commandDataFormat: null,
  commandDataDir: null,

  socialData: null,
  socialDataPath: null,
  socialDataFormat: null,
  socialDataDir: null,

  tutorialData: null,
  tutorialDataPath: null,
  tutorialDataFormat: null,
  tutorialDataDir: null,

  lootData: null,
  lootDataPath: null,
  lootDataFormat: null,
  lootDataDir: null,

  referenceData: null,
  projectConfig: null,
  editorMeta: null,
  editorMetaPath: null,
  dataDirectory: null,

  // Class data actions
  setClassData: (data) => set({ classData: data }),
  setClassDataPath: (path) => set({ classDataPath: path }),
  setClassDataFormat: (format) => set({ classDataFormat: format }),
  setClassDataDir: (dir) => set({ classDataDir: dir }),
  setClassFile: ({ data, path, format, dir }) =>
    set({
      classData: data,
      classDataPath: path,
      classDataFormat: format,
      classDataDir: dir
    }),

  // Race data actions
  setRaceData: (data) => set({ raceData: data }),
  setRaceDataPath: (path) => set({ raceDataPath: path }),
  setRaceDataFormat: (format) => set({ raceDataFormat: format }),
  setRaceDataDir: (dir) => set({ raceDataDir: dir }),
  setRaceFile: ({ data, path, format, dir }) =>
    set({
      raceData: data,
      raceDataPath: path,
      raceDataFormat: format,
      raceDataDir: dir
    }),

  // Skill data actions
  setSkillData: (data) => set({ skillData: data }),
  setSkillDataPath: (path) => set({ skillDataPath: path }),
  setSkillDataFormat: (format) => set({ skillDataFormat: format }),
  setSkillDataDir: (dir) => set({ skillDataDir: dir }),
  setSkillFile: ({ data, path, format, dir }) =>
    set({
      skillData: data,
      skillDataPath: path,
      skillDataFormat: format,
      skillDataDir: dir
    }),

  // Group data actions
  setGroupData: (data) => set({ groupData: data }),
  setGroupDataPath: (path) => set({ groupDataPath: path }),
  setGroupDataFormat: (format) => set({ groupDataFormat: format }),
  setGroupDataDir: (dir) => set({ groupDataDir: dir }),
  setGroupFile: ({ data, path, format, dir }) =>
    set({
      groupData: data,
      groupDataPath: path,
      groupDataFormat: format,
      groupDataDir: dir
    }),

  // Command data actions
  setCommandData: (data) => set({ commandData: data }),
  setCommandDataPath: (path) => set({ commandDataPath: path }),
  setCommandDataFormat: (format) => set({ commandDataFormat: format }),
  setCommandDataDir: (dir) => set({ commandDataDir: dir }),
  setCommandFile: ({ data, path, format, dir }) =>
    set({
      commandData: data,
      commandDataPath: path,
      commandDataFormat: format,
      commandDataDir: dir
    }),

  // Social data actions
  setSocialData: (data) => set({ socialData: data }),
  setSocialDataPath: (path) => set({ socialDataPath: path }),
  setSocialDataFormat: (format) => set({ socialDataFormat: format }),
  setSocialDataDir: (dir) => set({ socialDataDir: dir }),
  setSocialFile: ({ data, path, format, dir }) =>
    set({
      socialData: data,
      socialDataPath: path,
      socialDataFormat: format,
      socialDataDir: dir
    }),

  // Tutorial data actions
  setTutorialData: (data) => set({ tutorialData: data }),
  setTutorialDataPath: (path) => set({ tutorialDataPath: path }),
  setTutorialDataFormat: (format) => set({ tutorialDataFormat: format }),
  setTutorialDataDir: (dir) => set({ tutorialDataDir: dir }),
  setTutorialFile: ({ data, path, format, dir }) =>
    set({
      tutorialData: data,
      tutorialDataPath: path,
      tutorialDataFormat: format,
      tutorialDataDir: dir
    }),

  // Loot data actions
  setLootData: (data) => set({ lootData: data }),
  setLootDataPath: (path) => set({ lootDataPath: path }),
  setLootDataFormat: (format) => set({ lootDataFormat: format }),
  setLootDataDir: (dir) => set({ lootDataDir: dir }),
  setLootFile: ({ data, path, format, dir }) =>
    set({
      lootData: data,
      lootDataPath: path,
      lootDataFormat: format,
      lootDataDir: dir
    }),

  // Other actions
  setReferenceData: (data) => set({ referenceData: data }),
  setProjectConfig: (config) => set({ projectConfig: config }),
  setEditorMeta: (meta) => set({ editorMeta: meta }),
  setEditorMetaPath: (path) => set({ editorMetaPath: path }),
  setDataDirectory: (dir) => set({ dataDirectory: dir }),

  // Batch clear
  clearAllGlobalData: () =>
    set({
      classData: null,
      classDataPath: null,
      classDataFormat: null,
      classDataDir: null,
      raceData: null,
      raceDataPath: null,
      raceDataFormat: null,
      raceDataDir: null,
      skillData: null,
      skillDataPath: null,
      skillDataFormat: null,
      skillDataDir: null,
      groupData: null,
      groupDataPath: null,
      groupDataFormat: null,
      groupDataDir: null,
      commandData: null,
      commandDataPath: null,
      commandDataFormat: null,
      commandDataDir: null,
      socialData: null,
      socialDataPath: null,
      socialDataFormat: null,
      socialDataDir: null,
      tutorialData: null,
      tutorialDataPath: null,
      tutorialDataFormat: null,
      tutorialDataDir: null,
      lootData: null,
      lootDataPath: null,
      lootDataFormat: null,
      lootDataDir: null,
      referenceData: null
    })
}), { name: "GlobalDataStore" }));
