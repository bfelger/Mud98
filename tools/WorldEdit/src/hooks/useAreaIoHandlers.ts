import { useCallback, useEffect, useRef } from "react";
import { message } from "@tauri-apps/plugin-dialog";
import {
  extractAreaLayout,
  type AreaLayoutMap
} from "../map/areaNodes";
import {
  extractRoomLayout,
  type RoomLayoutMap
} from "../map/roomNodes";
import type {
  AreaJson,
  EditorMeta,
  ProjectConfig,
  ReferenceData
} from "../repository/types";
import type { WorldRepository } from "../repository/worldRepository";

type AreaIoHandlersArgs = {
  areaDirectory: string | null;
  dataDirectory: string | null;
  projectConfig: ProjectConfig | null;
  areaPath: string | null;
  areaData: AreaJson | null;
  editorMeta: EditorMeta | null;
  activeTab: string;
  selectedEntity: string;
  roomLayout: RoomLayoutMap;
  areaLayout: AreaLayoutMap;
  repository: WorldRepository;
  setProjectConfig: (config: ProjectConfig | null) => void;
  setAreaDirectory: (dir: string | null) => void;
  setDataDirectory: (dir: string | null) => void;
  setReferenceData: (data: ReferenceData | null) => void;
  setAreaPath: (path: string | null) => void;
  setAreaData: (data: AreaJson | null) => void;
  setEditorMeta: (meta: EditorMeta | null) => void;
  setEditorMetaPath: (path: string | null) => void;
  setStatusMessage: (message: string) => void;
  setErrorMessage: (message: string | null) => void;
  setIsBusy: (busy: boolean) => void;
  applyLoadedRoomLayout: (layout: RoomLayoutMap) => void;
  applyLoadedAreaLayout: (layout: AreaLayoutMap) => void;
  buildEditorMeta: (
    existing: EditorMeta | null,
    activeTab: string,
    selectedEntity: string,
    roomLayout: RoomLayoutMap,
    areaLayout: AreaLayoutMap
  ) => EditorMeta;
  fileNameFromPath: (path: string | null) => string;
};

export function useAreaIoHandlers({
  areaDirectory,
  dataDirectory,
  projectConfig,
  areaPath,
  areaData,
  editorMeta,
  activeTab,
  selectedEntity,
  roomLayout,
  areaLayout,
  repository,
  setProjectConfig,
  setAreaDirectory,
  setDataDirectory,
  setReferenceData,
  setAreaPath,
  setAreaData,
  setEditorMeta,
  setEditorMetaPath,
  setStatusMessage,
  setErrorMessage,
  setIsBusy,
  applyLoadedRoomLayout,
  applyLoadedAreaLayout,
  buildEditorMeta,
  fileNameFromPath
}: AreaIoHandlersArgs) {
  const legacyScanDirRef = useRef<string | null>(null);

  const warnLegacyAreaFiles = useCallback(
    async (areaDir: string | null) => {
      if (!areaDir) {
        return;
      }
      if (legacyScanDirRef.current === areaDir) {
        return;
      }
      legacyScanDirRef.current = areaDir;
      try {
        const legacyFiles = await repository.listLegacyAreaFiles(areaDir);
        if (!legacyFiles.length) {
          return;
        }
        const warningMessage = [
          `Legacy ROM-OLC area files found in ${areaDir}:`,
          "",
          ...legacyFiles.map((file) => `- ${file}`),
          "",
          "WorldEdit edits JSON only. Load these areas in Mud98 and save to convert them to JSON before editing."
        ].join("\n");
        await message(warningMessage, {
          title: "Legacy .are files detected",
          kind: "warning"
        });
      } catch (error) {
        setErrorMessage(`Failed to scan area directory. ${String(error)}`);
      }
    },
    [repository, setErrorMessage]
  );

  useEffect(() => {
    if (!areaDirectory) {
      return;
    }
    void warnLegacyAreaFiles(areaDirectory);
  }, [areaDirectory, warnLegacyAreaFiles]);

  const handleOpenProject = useCallback(async () => {
    setErrorMessage(null);
    setIsBusy(true);
    try {
      const cfgPath = await repository.pickConfigFile(
        areaDirectory ?? dataDirectory
      );
      if (!cfgPath) {
        return;
      }
      const config = await repository.loadProjectConfig(cfgPath);
      setProjectConfig(config);
      if (config.areaDir) {
        setAreaDirectory(config.areaDir);
        localStorage.setItem("worldedit.areaDir", config.areaDir);
        await warnLegacyAreaFiles(config.areaDir);
      }
      if (config.dataDir) {
        setDataDirectory(config.dataDir);
        const refs = await repository.loadReferenceData(
          config.dataDir,
          config.dataFiles
        );
        setReferenceData(refs);
      }
      setStatusMessage(`Loaded project ${fileNameFromPath(cfgPath)}`);
    } catch (error) {
      setErrorMessage(`Failed to load config. ${String(error)}`);
    } finally {
      setIsBusy(false);
    }
  }, [
    areaDirectory,
    dataDirectory,
    fileNameFromPath,
    repository,
    setAreaDirectory,
    setDataDirectory,
    setErrorMessage,
    setIsBusy,
    setProjectConfig,
    setReferenceData,
    setStatusMessage,
    warnLegacyAreaFiles
  ]);

  const handleOpenArea = useCallback(async () => {
    setErrorMessage(null);
    setIsBusy(true);
    try {
      const path = await repository.pickAreaFile(areaDirectory);
      if (!path) {
        return;
      }
      const areaDir = await repository.resolveAreaDirectory(path);
      await warnLegacyAreaFiles(areaDir);
      const loaded = await repository.loadArea(path);
      const metaPath = repository.editorMetaPathForArea(path);
      let loadedMeta: EditorMeta | null = null;
      setAreaPath(path);
      setAreaData(loaded);
      setEditorMetaPath(metaPath);
      try {
        loadedMeta = await repository.loadEditorMeta(metaPath);
      } catch (error) {
        setErrorMessage(`Failed to load editor meta. ${String(error)}`);
      }
      setEditorMeta(loadedMeta);
      applyLoadedRoomLayout(extractRoomLayout(loadedMeta?.layout));
      applyLoadedAreaLayout(extractAreaLayout(loadedMeta?.layout));
      setStatusMessage(
        loadedMeta
          ? `Loaded ${fileNameFromPath(path)} + editor meta`
          : `Loaded ${fileNameFromPath(path)}`
      );
      try {
        const resolvedDataDir =
          projectConfig?.dataDir ??
          (await repository.resolveDataDirectory(path, areaDirectory));
        if (resolvedDataDir) {
          const refs = await repository.loadReferenceData(
            resolvedDataDir,
            projectConfig?.dataFiles
          );
          setReferenceData(refs);
          setDataDirectory(resolvedDataDir);
          setStatusMessage(
            `Loaded ${fileNameFromPath(path)} + reference data`
          );
        }
      } catch (error) {
        setErrorMessage(`Failed to load reference data. ${String(error)}`);
      }
    } catch (error) {
      setErrorMessage(`Failed to load area JSON. ${String(error)}`);
    } finally {
      setIsBusy(false);
    }
  }, [
    applyLoadedAreaLayout,
    applyLoadedRoomLayout,
    areaDirectory,
    fileNameFromPath,
    projectConfig,
    repository,
    setAreaData,
    setAreaPath,
    setDataDirectory,
    setEditorMeta,
    setEditorMetaPath,
    setErrorMessage,
    setIsBusy,
    setReferenceData,
    setStatusMessage,
    warnLegacyAreaFiles
  ]);

  const saveToPath = useCallback(
    async (path: string) => {
      if (!areaData) {
        return;
      }
      await repository.saveArea(path, areaData);
      setStatusMessage(`Saved ${fileNameFromPath(path)}`);
    },
    [areaData, fileNameFromPath, repository, setStatusMessage]
  );

  const saveEditorMetaToPath = useCallback(
    async (path: string) => {
      const metaPath = repository.editorMetaPathForArea(path);
      const nextMeta = buildEditorMeta(
        editorMeta,
        activeTab,
        selectedEntity,
        roomLayout,
        areaLayout
      );
      await repository.saveEditorMeta(metaPath, nextMeta);
      setEditorMeta(nextMeta);
      setEditorMetaPath(metaPath);
      setStatusMessage(`Saved editor meta ${fileNameFromPath(metaPath)}`);
    },
    [
      activeTab,
      areaLayout,
      buildEditorMeta,
      editorMeta,
      fileNameFromPath,
      repository,
      roomLayout,
      selectedEntity,
      setEditorMeta,
      setEditorMetaPath,
      setStatusMessage
    ]
  );

  const handleSaveArea = useCallback(async () => {
    if (!areaData) {
      setErrorMessage("No area loaded to save.");
      return;
    }
    setErrorMessage(null);
    setIsBusy(true);
    try {
      let targetPath = areaPath;
      if (!targetPath) {
        targetPath = await repository.pickSaveFile(areaDirectory);
      }
      if (!targetPath) {
        return;
      }
      setAreaPath(targetPath);
      setEditorMetaPath(repository.editorMetaPathForArea(targetPath));
      await saveToPath(targetPath);
    } catch (error) {
      setErrorMessage(`Failed to save area JSON. ${String(error)}`);
    } finally {
      setIsBusy(false);
    }
  }, [
    areaData,
    areaDirectory,
    areaPath,
    repository,
    saveToPath,
    setAreaPath,
    setEditorMetaPath,
    setErrorMessage,
    setIsBusy
  ]);

  const handleSaveAreaAs = useCallback(async () => {
    if (!areaData) {
      setErrorMessage("No area loaded to save.");
      return;
    }
    setErrorMessage(null);
    setIsBusy(true);
    try {
      const targetPath = await repository.pickSaveFile(
        areaPath ?? areaDirectory
      );
      if (!targetPath) {
        return;
      }
      setAreaPath(targetPath);
      setEditorMetaPath(repository.editorMetaPathForArea(targetPath));
      await saveToPath(targetPath);
    } catch (error) {
      setErrorMessage(`Failed to save area JSON. ${String(error)}`);
    } finally {
      setIsBusy(false);
    }
  }, [
    areaData,
    areaDirectory,
    areaPath,
    repository,
    saveToPath,
    setAreaPath,
    setEditorMetaPath,
    setErrorMessage,
    setIsBusy
  ]);

  const handleSaveEditorMeta = useCallback(async () => {
    if (!areaPath) {
      setErrorMessage("No area loaded to save editor metadata.");
      return;
    }
    setErrorMessage(null);
    setIsBusy(true);
    try {
      await saveEditorMetaToPath(areaPath);
    } catch (error) {
      setErrorMessage(`Failed to save editor meta. ${String(error)}`);
    } finally {
      setIsBusy(false);
    }
  }, [areaPath, saveEditorMetaToPath, setErrorMessage, setIsBusy]);

  const handleSetAreaDirectory = useCallback(async () => {
    setErrorMessage(null);
    setIsBusy(true);
    try {
      const selected = await repository.pickAreaDirectory(areaDirectory);
      if (!selected) {
        return;
      }
      setAreaDirectory(selected);
      localStorage.setItem("worldedit.areaDir", selected);
      setStatusMessage(`Area directory set to ${selected}`);
      await warnLegacyAreaFiles(selected);
    } catch (error) {
      setErrorMessage(`Failed to set area directory. ${String(error)}`);
    } finally {
      setIsBusy(false);
    }
  }, [
    areaDirectory,
    repository,
    setAreaDirectory,
    setErrorMessage,
    setIsBusy,
    setStatusMessage,
    warnLegacyAreaFiles
  ]);

  const handleLoadReferenceData = useCallback(async () => {
    setErrorMessage(null);
    setIsBusy(true);
    try {
      const resolvedDataDir =
        projectConfig?.dataDir ??
        (await repository.resolveDataDirectory(areaPath, areaDirectory));
      if (!resolvedDataDir) {
        setErrorMessage("Unable to resolve data directory.");
        return;
      }
      const refs = await repository.loadReferenceData(
        resolvedDataDir,
        projectConfig?.dataFiles
      );
      setReferenceData(refs);
      setDataDirectory(resolvedDataDir);
      setStatusMessage(`Loaded reference data from ${resolvedDataDir}`);
    } catch (error) {
      setErrorMessage(`Failed to load reference data. ${String(error)}`);
    } finally {
      setIsBusy(false);
    }
  }, [
    areaDirectory,
    areaPath,
    projectConfig,
    repository,
    setDataDirectory,
    setErrorMessage,
    setIsBusy,
    setReferenceData,
    setStatusMessage
  ]);

  return {
    handleOpenProject,
    handleOpenArea,
    handleSaveArea,
    handleSaveAreaAs,
    handleSaveEditorMeta,
    handleSetAreaDirectory,
    handleLoadReferenceData
  };
}
