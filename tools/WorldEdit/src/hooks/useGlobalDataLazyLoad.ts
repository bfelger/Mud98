/**
 * Hook to handle lazy loading of global data files.
 * 
 * Consolidates 8 nearly identical useEffects into a single data-driven approach.
 * Loads data when:
 * - Editor is in Global mode
 * - The corresponding entity is selected
 * - Data directory is available
 * - Data hasn't already been loaded from the same directory
 */

import { useEffect } from "react";
import { useSelectionStore } from "../stores/useSelectionStore";
import { useGlobalDataStore } from "../stores/useGlobalDataStore";
import type { GlobalEntity } from "../stores/useSelectionStore";

/** Configuration for lazy-loading a global data type. */
interface LazyLoadConfig {
  entity: GlobalEntity;
  getData: () => unknown;
  getDataDir: () => string | null;
  loadData: (dir: string) => Promise<void>;
}

interface UseGlobalDataLazyLoadParams {
  handleLoadClassesData: (dir: string) => Promise<void>;
  handleLoadRacesData: (dir: string) => Promise<void>;
  handleLoadSkillsData: (dir: string) => Promise<void>;
  handleLoadGroupsData: (dir: string) => Promise<void>;
  handleLoadCommandsData: (dir: string) => Promise<void>;
  handleLoadSocialsData: (dir: string) => Promise<void>;
  handleLoadTutorialsData: (dir: string) => Promise<void>;
  handleLoadLootData: (dir: string) => Promise<void>;
}

/**
 * Hook that manages lazy loading of global data files.
 * 
 * Replaces 8 separate useEffects (~160 lines) with a single data-driven effect.
 */
export function useGlobalDataLazyLoad({
  handleLoadClassesData,
  handleLoadRacesData,
  handleLoadSkillsData,
  handleLoadGroupsData,
  handleLoadCommandsData,
  handleLoadSocialsData,
  handleLoadTutorialsData,
  handleLoadLootData
}: UseGlobalDataLazyLoadParams): void {
  const { editorMode, selectedGlobalEntity } = useSelectionStore();
  const {
    dataDirectory,
    classData,
    classDataDir,
    raceData,
    raceDataDir,
    skillData,
    skillDataDir,
    groupData,
    groupDataDir,
    commandData,
    commandDataDir,
    socialData,
    socialDataDir,
    tutorialData,
    tutorialDataDir,
    lootData,
    lootDataDir
  } = useGlobalDataStore();

  // Configuration table mapping entities to their data/loader
  const configs: LazyLoadConfig[] = [
    {
      entity: "Classes",
      getData: () => classData,
      getDataDir: () => classDataDir,
      loadData: handleLoadClassesData
    },
    {
      entity: "Races",
      getData: () => raceData,
      getDataDir: () => raceDataDir,
      loadData: handleLoadRacesData
    },
    {
      entity: "Skills",
      getData: () => skillData,
      getDataDir: () => skillDataDir,
      loadData: handleLoadSkillsData
    },
    {
      entity: "Groups",
      getData: () => groupData,
      getDataDir: () => groupDataDir,
      loadData: handleLoadGroupsData
    },
    {
      entity: "Commands",
      getData: () => commandData,
      getDataDir: () => commandDataDir,
      loadData: handleLoadCommandsData
    },
    {
      entity: "Socials",
      getData: () => socialData,
      getDataDir: () => socialDataDir,
      loadData: handleLoadSocialsData
    },
    {
      entity: "Tutorials",
      getData: () => tutorialData,
      getDataDir: () => tutorialDataDir,
      loadData: handleLoadTutorialsData
    },
    {
      entity: "Loot",
      getData: () => lootData,
      getDataDir: () => lootDataDir,
      loadData: handleLoadLootData
    }
  ];

  useEffect(() => {
    // Only trigger in Global mode
    if (editorMode !== "Global") return;
    if (!dataDirectory) return;

    // Find config for currently selected entity
    const config = configs.find((c) => c.entity === selectedGlobalEntity);
    if (!config) return;

    // Skip if data already loaded from same directory
    const currentData = config.getData();
    const currentDir = config.getDataDir();
    if (currentData && currentDir === dataDirectory) return;

    // Load the data
    void config.loadData(dataDirectory);
  }, [
    editorMode,
    selectedGlobalEntity,
    dataDirectory,
    // Include all data/dir pairs so effect re-evaluates when they change
    classData,
    classDataDir,
    raceData,
    raceDataDir,
    skillData,
    skillDataDir,
    groupData,
    groupDataDir,
    commandData,
    commandDataDir,
    socialData,
    socialDataDir,
    tutorialData,
    tutorialDataDir,
    lootData,
    lootDataDir,
    // Handlers are stable (from useCallback), but include for completeness
    handleLoadClassesData,
    handleLoadRacesData,
    handleLoadSkillsData,
    handleLoadGroupsData,
    handleLoadCommandsData,
    handleLoadSocialsData,
    handleLoadTutorialsData,
    handleLoadLootData
  ]);
}
