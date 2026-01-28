import { useCallback } from "react";
import type {
  ClassDataFile,
  CommandDataFile,
  GroupDataFile,
  LootDataFile,
  ProjectConfig,
  RaceDataFile,
  ReferenceData,
  SkillDataFile,
  SocialDataFile,
  TutorialDataFile
} from "../repository/types";
import type { WorldRepository } from "../repository/worldRepository";

type DataFormat = "json" | "olc" | null;

type GlobalIoHandlersArgs = {
  dataDirectory: string | null;
  projectConfig: ProjectConfig | null;
  referenceData: ReferenceData | null;
  repository: WorldRepository;
  classData: ClassDataFile | null;
  classDataFormat: DataFormat;
  raceData: RaceDataFile | null;
  raceDataFormat: DataFormat;
  skillData: SkillDataFile | null;
  skillDataFormat: DataFormat;
  groupData: GroupDataFile | null;
  groupDataFormat: DataFormat;
  commandData: CommandDataFile | null;
  commandDataFormat: DataFormat;
  socialData: SocialDataFile | null;
  socialDataFormat: DataFormat;
  tutorialData: TutorialDataFile | null;
  tutorialDataFormat: DataFormat;
  lootData: LootDataFile | null;
  lootDataFormat: DataFormat;
  setClassData: (data: ClassDataFile | null) => void;
  setClassDataPath: (path: string | null) => void;
  setClassDataFormat: (format: DataFormat) => void;
  setClassDataDir: (dir: string | null) => void;
  setRaceData: (data: RaceDataFile | null) => void;
  setRaceDataPath: (path: string | null) => void;
  setRaceDataFormat: (format: DataFormat) => void;
  setRaceDataDir: (dir: string | null) => void;
  setSkillData: (data: SkillDataFile | null) => void;
  setSkillDataPath: (path: string | null) => void;
  setSkillDataFormat: (format: DataFormat) => void;
  setSkillDataDir: (dir: string | null) => void;
  setGroupData: (data: GroupDataFile | null) => void;
  setGroupDataPath: (path: string | null) => void;
  setGroupDataFormat: (format: DataFormat) => void;
  setGroupDataDir: (dir: string | null) => void;
  setCommandData: (data: CommandDataFile | null) => void;
  setCommandDataPath: (path: string | null) => void;
  setCommandDataFormat: (format: DataFormat) => void;
  setCommandDataDir: (dir: string | null) => void;
  setSocialData: (data: SocialDataFile | null) => void;
  setSocialDataPath: (path: string | null) => void;
  setSocialDataFormat: (format: DataFormat) => void;
  setSocialDataDir: (dir: string | null) => void;
  setTutorialData: (data: TutorialDataFile | null) => void;
  setTutorialDataPath: (path: string | null) => void;
  setTutorialDataFormat: (format: DataFormat) => void;
  setTutorialDataDir: (dir: string | null) => void;
  setLootData: (data: LootDataFile | null) => void;
  setLootDataPath: (path: string | null) => void;
  setLootDataFormat: (format: DataFormat) => void;
  setLootDataDir: (dir: string | null) => void;
  setStatusMessage: (message: string) => void;
  setErrorMessage: (message: string | null) => void;
  setIsBusy: (busy: boolean) => void;
};

export function useGlobalIoHandlers({
  dataDirectory,
  projectConfig,
  referenceData,
  repository,
  classData,
  classDataFormat,
  raceData,
  raceDataFormat,
  skillData,
  skillDataFormat,
  groupData,
  groupDataFormat,
  commandData,
  commandDataFormat,
  socialData,
  socialDataFormat,
  tutorialData,
  tutorialDataFormat,
  lootData,
  lootDataFormat,
  setClassData,
  setClassDataPath,
  setClassDataFormat,
  setClassDataDir,
  setRaceData,
  setRaceDataPath,
  setRaceDataFormat,
  setRaceDataDir,
  setSkillData,
  setSkillDataPath,
  setSkillDataFormat,
  setSkillDataDir,
  setGroupData,
  setGroupDataPath,
  setGroupDataFormat,
  setGroupDataDir,
  setCommandData,
  setCommandDataPath,
  setCommandDataFormat,
  setCommandDataDir,
  setSocialData,
  setSocialDataPath,
  setSocialDataFormat,
  setSocialDataDir,
  setTutorialData,
  setTutorialDataPath,
  setTutorialDataFormat,
  setTutorialDataDir,
  setLootData,
  setLootDataPath,
  setLootDataFormat,
  setLootDataDir,
  setStatusMessage,
  setErrorMessage,
  setIsBusy
}: GlobalIoHandlersArgs) {
  const handleLoadClassesData = useCallback(
    async (overrideDir?: string) => {
      const targetDir =
        typeof overrideDir === "string" ? overrideDir : dataDirectory;
      if (!targetDir) {
        setErrorMessage("No data directory set for classes.");
        return;
      }
      const classesFile = projectConfig?.dataFiles.classes;
      const defaultFormat = projectConfig?.defaultFormat;
      setErrorMessage(null);
      setIsBusy(true);
      try {
        const source = await repository.loadClassesData(
          targetDir,
          classesFile,
          defaultFormat
        );
        setClassData(source.data);
        setClassDataPath(source.path);
        setClassDataFormat(source.format);
        setClassDataDir(targetDir);
        setStatusMessage(`Loaded classes (${source.format})`);
      } catch (error) {
        setErrorMessage(`Failed to load classes. ${String(error)}`);
      } finally {
        setIsBusy(false);
      }
    },
    [
      dataDirectory,
      projectConfig,
      repository,
      setClassData,
      setClassDataDir,
      setClassDataFormat,
      setClassDataPath,
      setErrorMessage,
      setIsBusy,
      setStatusMessage
    ]
  );

  const handleSaveClassesData = useCallback(async () => {
    if (!classData) {
      setErrorMessage("No classes loaded to save.");
      return;
    }
    if (!dataDirectory) {
      setErrorMessage("No data directory set for classes.");
      return;
    }
    setErrorMessage(null);
    setIsBusy(true);
    try {
      const format = classDataFormat ?? projectConfig?.defaultFormat ?? "json";
      const classesFile = projectConfig?.dataFiles.classes;
      const path = await repository.saveClassesData(
        dataDirectory,
        classData,
        format,
        classesFile
      );
      setClassDataPath(path);
      setClassDataFormat(format);
      setClassDataDir(dataDirectory);
      setStatusMessage(`Saved classes (${format})`);
    } catch (error) {
      setErrorMessage(`Failed to save classes. ${String(error)}`);
    } finally {
      setIsBusy(false);
    }
  }, [
    classData,
    classDataFormat,
    dataDirectory,
    projectConfig,
    repository,
    setClassDataDir,
    setClassDataFormat,
    setClassDataPath,
    setErrorMessage,
    setIsBusy,
    setStatusMessage
  ]);

  const handleLoadRacesData = useCallback(
    async (overrideDir?: string) => {
      const targetDir =
        typeof overrideDir === "string" ? overrideDir : dataDirectory;
      if (!targetDir) {
        setErrorMessage("No data directory set for races.");
        return;
      }
      const racesFile = projectConfig?.dataFiles.races;
      const defaultFormat = projectConfig?.defaultFormat;
      setErrorMessage(null);
      setIsBusy(true);
      try {
        const source = await repository.loadRacesData(
          targetDir,
          racesFile,
          defaultFormat
        );
        setRaceData(source.data);
        setRaceDataPath(source.path);
        setRaceDataFormat(source.format);
        setRaceDataDir(targetDir);
        setStatusMessage(`Loaded races (${source.format})`);
      } catch (error) {
        setErrorMessage(`Failed to load races. ${String(error)}`);
      } finally {
        setIsBusy(false);
      }
    },
    [
      dataDirectory,
      projectConfig,
      repository,
      setErrorMessage,
      setIsBusy,
      setRaceData,
      setRaceDataDir,
      setRaceDataFormat,
      setRaceDataPath,
      setStatusMessage
    ]
  );

  const handleSaveRacesData = useCallback(async () => {
    if (!raceData) {
      setErrorMessage("No races loaded to save.");
      return;
    }
    if (!dataDirectory) {
      setErrorMessage("No data directory set for races.");
      return;
    }
    setErrorMessage(null);
    setIsBusy(true);
    try {
      const format = raceDataFormat ?? projectConfig?.defaultFormat ?? "json";
      const racesFile = projectConfig?.dataFiles.races;
      const classNames = referenceData?.classes ?? [];
      const path = await repository.saveRacesData(
        dataDirectory,
        raceData,
        format,
        racesFile,
        classNames
      );
      setRaceDataPath(path);
      setRaceDataFormat(format);
      setRaceDataDir(dataDirectory);
      setStatusMessage(`Saved races (${format})`);
    } catch (error) {
      setErrorMessage(`Failed to save races. ${String(error)}`);
    } finally {
      setIsBusy(false);
    }
  }, [
    dataDirectory,
    projectConfig,
    raceData,
    raceDataFormat,
    referenceData,
    repository,
    setErrorMessage,
    setIsBusy,
    setRaceDataDir,
    setRaceDataFormat,
    setRaceDataPath,
    setStatusMessage
  ]);

  const handleLoadSkillsData = useCallback(
    async (overrideDir?: string) => {
      const targetDir =
        typeof overrideDir === "string" ? overrideDir : dataDirectory;
      if (!targetDir) {
        setErrorMessage("No data directory set for skills.");
        return;
      }
      const skillsFile = projectConfig?.dataFiles.skills;
      const defaultFormat = projectConfig?.defaultFormat;
      setErrorMessage(null);
      setIsBusy(true);
      try {
        const source = await repository.loadSkillsData(
          targetDir,
          skillsFile,
          defaultFormat
        );
        setSkillData(source.data);
        setSkillDataPath(source.path);
        setSkillDataFormat(source.format);
        setSkillDataDir(targetDir);
        setStatusMessage(`Loaded skills (${source.format})`);
      } catch (error) {
        setErrorMessage(`Failed to load skills. ${String(error)}`);
      } finally {
        setIsBusy(false);
      }
    },
    [
      dataDirectory,
      projectConfig,
      repository,
      setErrorMessage,
      setIsBusy,
      setSkillData,
      setSkillDataDir,
      setSkillDataFormat,
      setSkillDataPath,
      setStatusMessage
    ]
  );

  const handleSaveSkillsData = useCallback(async () => {
    if (!skillData) {
      setErrorMessage("No skills loaded to save.");
      return;
    }
    if (!dataDirectory) {
      setErrorMessage("No data directory set for skills.");
      return;
    }
    setErrorMessage(null);
    setIsBusy(true);
    try {
      const format = skillDataFormat ?? projectConfig?.defaultFormat ?? "json";
      const skillsFile = projectConfig?.dataFiles.skills;
      const classNames = referenceData?.classes ?? [];
      const path = await repository.saveSkillsData(
        dataDirectory,
        skillData,
        format,
        skillsFile,
        classNames
      );
      setSkillDataPath(path);
      setSkillDataFormat(format);
      setSkillDataDir(dataDirectory);
      setStatusMessage(`Saved skills (${format})`);
    } catch (error) {
      setErrorMessage(`Failed to save skills. ${String(error)}`);
    } finally {
      setIsBusy(false);
    }
  }, [
    dataDirectory,
    projectConfig,
    referenceData,
    repository,
    setErrorMessage,
    setIsBusy,
    setSkillDataDir,
    setSkillDataFormat,
    setSkillDataPath,
    setStatusMessage,
    skillData,
    skillDataFormat
  ]);

  const handleLoadGroupsData = useCallback(
    async (overrideDir?: string) => {
      const targetDir =
        typeof overrideDir === "string" ? overrideDir : dataDirectory;
      if (!targetDir) {
        setErrorMessage("No data directory set for groups.");
        return;
      }
      const groupsFile = projectConfig?.dataFiles.groups;
      const defaultFormat = projectConfig?.defaultFormat;
      setErrorMessage(null);
      setIsBusy(true);
      try {
        const source = await repository.loadGroupsData(
          targetDir,
          groupsFile,
          defaultFormat
        );
        setGroupData(source.data);
        setGroupDataPath(source.path);
        setGroupDataFormat(source.format);
        setGroupDataDir(targetDir);
        setStatusMessage(`Loaded groups (${source.format})`);
      } catch (error) {
        setErrorMessage(`Failed to load groups. ${String(error)}`);
      } finally {
        setIsBusy(false);
      }
    },
    [
      dataDirectory,
      projectConfig,
      repository,
      setErrorMessage,
      setGroupData,
      setGroupDataDir,
      setGroupDataFormat,
      setGroupDataPath,
      setIsBusy,
      setStatusMessage
    ]
  );

  const handleSaveGroupsData = useCallback(async () => {
    if (!groupData) {
      setErrorMessage("No groups loaded to save.");
      return;
    }
    if (!dataDirectory) {
      setErrorMessage("No data directory set for groups.");
      return;
    }
    setErrorMessage(null);
    setIsBusy(true);
    try {
      const format = groupDataFormat ?? projectConfig?.defaultFormat ?? "json";
      const groupsFile = projectConfig?.dataFiles.groups;
      const classNames = referenceData?.classes ?? [];
      const path = await repository.saveGroupsData(
        dataDirectory,
        groupData,
        format,
        groupsFile,
        classNames
      );
      setGroupDataPath(path);
      setGroupDataFormat(format);
      setGroupDataDir(dataDirectory);
      setStatusMessage(`Saved groups (${format})`);
    } catch (error) {
      setErrorMessage(`Failed to save groups. ${String(error)}`);
    } finally {
      setIsBusy(false);
    }
  }, [
    dataDirectory,
    groupData,
    groupDataFormat,
    projectConfig,
    referenceData,
    repository,
    setErrorMessage,
    setGroupDataDir,
    setGroupDataFormat,
    setGroupDataPath,
    setIsBusy,
    setStatusMessage
  ]);

  const handleLoadCommandsData = useCallback(
    async (overrideDir?: string) => {
      const targetDir =
        typeof overrideDir === "string" ? overrideDir : dataDirectory;
      if (!targetDir) {
        setErrorMessage("No data directory set for commands.");
        return;
      }
      const commandsFile = projectConfig?.dataFiles.commands;
      const defaultFormat = projectConfig?.defaultFormat;
      setErrorMessage(null);
      setIsBusy(true);
      try {
        const source = await repository.loadCommandsData(
          targetDir,
          commandsFile,
          defaultFormat
        );
        setCommandData(source.data);
        setCommandDataPath(source.path);
        setCommandDataFormat(source.format);
        setCommandDataDir(targetDir);
        setStatusMessage(`Loaded commands (${source.format})`);
      } catch (error) {
        setErrorMessage(`Failed to load commands. ${String(error)}`);
      } finally {
        setIsBusy(false);
      }
    },
    [
      dataDirectory,
      projectConfig,
      repository,
      setCommandData,
      setCommandDataDir,
      setCommandDataFormat,
      setCommandDataPath,
      setErrorMessage,
      setIsBusy,
      setStatusMessage
    ]
  );

  const handleSaveCommandsData = useCallback(async () => {
    if (!commandData) {
      setErrorMessage("No commands loaded to save.");
      return;
    }
    if (!dataDirectory) {
      setErrorMessage("No data directory set for commands.");
      return;
    }
    setErrorMessage(null);
    setIsBusy(true);
    try {
      const format =
        commandDataFormat ?? projectConfig?.defaultFormat ?? "json";
      const commandsFile = projectConfig?.dataFiles.commands;
      const path = await repository.saveCommandsData(
        dataDirectory,
        commandData,
        format,
        commandsFile
      );
      setCommandDataPath(path);
      setCommandDataFormat(format);
      setCommandDataDir(dataDirectory);
      setStatusMessage(`Saved commands (${format})`);
    } catch (error) {
      setErrorMessage(`Failed to save commands. ${String(error)}`);
    } finally {
      setIsBusy(false);
    }
  }, [
    commandData,
    commandDataFormat,
    dataDirectory,
    projectConfig,
    repository,
    setCommandDataDir,
    setCommandDataFormat,
    setCommandDataPath,
    setErrorMessage,
    setIsBusy,
    setStatusMessage
  ]);

  const handleLoadSocialsData = useCallback(
    async (overrideDir?: string) => {
      const targetDir =
        typeof overrideDir === "string" ? overrideDir : dataDirectory;
      if (!targetDir) {
        setErrorMessage("No data directory set for socials.");
        return;
      }
      const socialsFile = projectConfig?.dataFiles.socials;
      const defaultFormat = projectConfig?.defaultFormat;
      setErrorMessage(null);
      setIsBusy(true);
      try {
        const source = await repository.loadSocialsData(
          targetDir,
          socialsFile,
          defaultFormat
        );
        setSocialData(source.data);
        setSocialDataPath(source.path);
        setSocialDataFormat(source.format);
        setSocialDataDir(targetDir);
        setStatusMessage(`Loaded socials (${source.format})`);
      } catch (error) {
        setErrorMessage(`Failed to load socials. ${String(error)}`);
      } finally {
        setIsBusy(false);
      }
    },
    [
      dataDirectory,
      projectConfig,
      repository,
      setErrorMessage,
      setIsBusy,
      setSocialData,
      setSocialDataDir,
      setSocialDataFormat,
      setSocialDataPath,
      setStatusMessage
    ]
  );

  const handleSaveSocialsData = useCallback(async () => {
    if (!socialData) {
      setErrorMessage("No socials loaded to save.");
      return;
    }
    if (!dataDirectory) {
      setErrorMessage("No data directory set for socials.");
      return;
    }
    setErrorMessage(null);
    setIsBusy(true);
    try {
      const format = socialDataFormat ?? projectConfig?.defaultFormat ?? "json";
      const socialsFile = projectConfig?.dataFiles.socials;
      const path = await repository.saveSocialsData(
        dataDirectory,
        socialData,
        format,
        socialsFile
      );
      setSocialDataPath(path);
      setSocialDataFormat(format);
      setSocialDataDir(dataDirectory);
      setStatusMessage(`Saved socials (${format})`);
    } catch (error) {
      setErrorMessage(`Failed to save socials. ${String(error)}`);
    } finally {
      setIsBusy(false);
    }
  }, [
    dataDirectory,
    projectConfig,
    repository,
    setErrorMessage,
    setIsBusy,
    setSocialDataDir,
    setSocialDataFormat,
    setSocialDataPath,
    setStatusMessage,
    socialData,
    socialDataFormat
  ]);

  const handleLoadTutorialsData = useCallback(
    async (overrideDir?: string) => {
      const targetDir =
        typeof overrideDir === "string" ? overrideDir : dataDirectory;
      if (!targetDir) {
        setErrorMessage("No data directory set for tutorials.");
        return;
      }
      const tutorialsFile = projectConfig?.dataFiles.tutorials;
      const defaultFormat = projectConfig?.defaultFormat;
      setErrorMessage(null);
      setIsBusy(true);
      try {
        const source = await repository.loadTutorialsData(
          targetDir,
          tutorialsFile,
          defaultFormat
        );
        setTutorialData(source.data);
        setTutorialDataPath(source.path);
        setTutorialDataFormat(source.format);
        setTutorialDataDir(targetDir);
        setStatusMessage(`Loaded tutorials (${source.format})`);
      } catch (error) {
        setErrorMessage(`Failed to load tutorials. ${String(error)}`);
      } finally {
        setIsBusy(false);
      }
    },
    [
      dataDirectory,
      projectConfig,
      repository,
      setErrorMessage,
      setIsBusy,
      setStatusMessage,
      setTutorialData,
      setTutorialDataDir,
      setTutorialDataFormat,
      setTutorialDataPath
    ]
  );

  const handleSaveTutorialsData = useCallback(async () => {
    if (!tutorialData) {
      setErrorMessage("No tutorials loaded to save.");
      return;
    }
    if (!dataDirectory) {
      setErrorMessage("No data directory set for tutorials.");
      return;
    }
    setErrorMessage(null);
    setIsBusy(true);
    try {
      const format =
        tutorialDataFormat ?? projectConfig?.defaultFormat ?? "json";
      const tutorialsFile = projectConfig?.dataFiles.tutorials;
      const path = await repository.saveTutorialsData(
        dataDirectory,
        tutorialData,
        format,
        tutorialsFile
      );
      setTutorialDataPath(path);
      setTutorialDataFormat(format);
      setTutorialDataDir(dataDirectory);
      setStatusMessage(`Saved tutorials (${format})`);
    } catch (error) {
      setErrorMessage(`Failed to save tutorials. ${String(error)}`);
    } finally {
      setIsBusy(false);
    }
  }, [
    dataDirectory,
    projectConfig,
    repository,
    setErrorMessage,
    setIsBusy,
    setStatusMessage,
    setTutorialDataDir,
    setTutorialDataFormat,
    setTutorialDataPath,
    tutorialData,
    tutorialDataFormat
  ]);

  const handleLoadLootData = useCallback(
    async (overrideDir?: string) => {
      const targetDir =
        typeof overrideDir === "string" ? overrideDir : dataDirectory;
      if (!targetDir) {
        setErrorMessage("No data directory set for loot.");
        return;
      }
      const lootFile = projectConfig?.dataFiles.loot;
      const defaultFormat = projectConfig?.defaultFormat;
      setErrorMessage(null);
      setIsBusy(true);
      try {
        const source = await repository.loadLootData(
          targetDir,
          lootFile,
          defaultFormat
        );
        setLootData(source.data);
        setLootDataPath(source.path);
        setLootDataFormat(source.format);
        setLootDataDir(targetDir);
        setStatusMessage(`Loaded loot (${source.format})`);
      } catch (error) {
        setErrorMessage(`Failed to load loot. ${String(error)}`);
      } finally {
        setIsBusy(false);
      }
    },
    [
      dataDirectory,
      projectConfig,
      repository,
      setErrorMessage,
      setIsBusy,
      setLootData,
      setLootDataDir,
      setLootDataFormat,
      setLootDataPath,
      setStatusMessage
    ]
  );

  const handleSaveLootData = useCallback(async () => {
    if (!lootData) {
      setErrorMessage("No loot loaded to save.");
      return;
    }
    if (!dataDirectory) {
      setErrorMessage("No data directory set for loot.");
      return;
    }
    setErrorMessage(null);
    setIsBusy(true);
    try {
      const format = lootDataFormat ?? projectConfig?.defaultFormat ?? "json";
      const lootFile = projectConfig?.dataFiles.loot;
      const path = await repository.saveLootData(
        dataDirectory,
        lootData,
        format,
        lootFile
      );
      setLootDataPath(path);
      setLootDataFormat(format);
      setLootDataDir(dataDirectory);
      setStatusMessage(`Saved loot (${format})`);
    } catch (error) {
      setErrorMessage(`Failed to save loot. ${String(error)}`);
    } finally {
      setIsBusy(false);
    }
  }, [
    dataDirectory,
    lootData,
    lootDataFormat,
    projectConfig,
    repository,
    setErrorMessage,
    setIsBusy,
    setLootDataDir,
    setLootDataFormat,
    setLootDataPath,
    setStatusMessage
  ]);

  return {
    handleLoadClassesData,
    handleSaveClassesData,
    handleLoadRacesData,
    handleSaveRacesData,
    handleLoadSkillsData,
    handleSaveSkillsData,
    handleLoadGroupsData,
    handleSaveGroupsData,
    handleLoadCommandsData,
    handleSaveCommandsData,
    handleLoadSocialsData,
    handleSaveSocialsData,
    handleLoadTutorialsData,
    handleSaveTutorialsData,
    handleLoadLootData,
    handleSaveLootData
  };
}
