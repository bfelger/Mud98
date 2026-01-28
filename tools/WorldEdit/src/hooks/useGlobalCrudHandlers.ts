import { useCallback } from "react";
import { createClass, deleteClass } from "../crud/global/classesCrud";
import { createRace, deleteRace } from "../crud/global/racesCrud";
import { createSkill, deleteSkill } from "../crud/global/skillsCrud";
import { createGroup, deleteGroup } from "../crud/global/groupsCrud";
import { createCommand, deleteCommand } from "../crud/global/commandsCrud";
import { createSocial, deleteSocial } from "../crud/global/socialsCrud";
import { createTutorial, deleteTutorial } from "../crud/global/tutorialsCrud";
import { createLoot, deleteLoot } from "../crud/global/lootCrud";
import type {
  ClassDataFile,
  RaceDataFile,
  SkillDataFile,
  GroupDataFile,
  CommandDataFile,
  SocialDataFile,
  TutorialDataFile,
  LootDataFile,
  ReferenceData
} from "../repository/types";

type UseGlobalCrudHandlersArgs = {
  classData: ClassDataFile | null;
  raceData: RaceDataFile | null;
  skillData: SkillDataFile | null;
  groupData: GroupDataFile | null;
  commandData: CommandDataFile | null;
  socialData: SocialDataFile | null;
  tutorialData: TutorialDataFile | null;
  lootData: LootDataFile | null;
  referenceData: ReferenceData | null;
  selectedClassIndex: number | null;
  selectedRaceIndex: number | null;
  selectedSkillIndex: number | null;
  selectedGroupIndex: number | null;
  selectedCommandIndex: number | null;
  selectedSocialIndex: number | null;
  selectedTutorialIndex: number | null;
  selectedLootKind: "group" | "table" | null;
  selectedLootIndex: number | null;
  setClassData: (data: ClassDataFile | null) => void;
  setRaceData: (data: RaceDataFile | null) => void;
  setSkillData: (data: SkillDataFile | null) => void;
  setGroupData: (data: GroupDataFile | null) => void;
  setCommandData: (data: CommandDataFile | null) => void;
  setSocialData: (data: SocialDataFile | null) => void;
  setTutorialData: (data: TutorialDataFile | null) => void;
  setLootData: (data: LootDataFile | null) => void;
  setSelectedGlobalEntity: (entity: string) => void;
  setSelectedClassIndex: (index: number | null) => void;
  setSelectedRaceIndex: (index: number | null) => void;
  setSelectedSkillIndex: (index: number | null) => void;
  setSelectedGroupIndex: (index: number | null) => void;
  setSelectedCommandIndex: (index: number | null) => void;
  setSelectedSocialIndex: (index: number | null) => void;
  setSelectedTutorialIndex: (index: number | null) => void;
  setSelectedLootKind: (kind: "group" | "table" | null) => void;
  setSelectedLootIndex: (index: number | null) => void;
  setActiveTab: (tab: "Form" | "Table" | "Map" | "Script" | "World" | "Inspect") => void;
  setStatusMessage: (message: string) => void;
};

export const useGlobalCrudHandlers = ({
  classData,
  raceData,
  skillData,
  groupData,
  commandData,
  socialData,
  tutorialData,
  lootData,
  referenceData,
  selectedClassIndex,
  selectedRaceIndex,
  selectedSkillIndex,
  selectedGroupIndex,
  selectedCommandIndex,
  selectedSocialIndex,
  selectedTutorialIndex,
  selectedLootKind,
  selectedLootIndex,
  setClassData,
  setRaceData,
  setSkillData,
  setGroupData,
  setCommandData,
  setSocialData,
  setTutorialData,
  setLootData,
  setSelectedGlobalEntity,
  setSelectedClassIndex,
  setSelectedRaceIndex,
  setSelectedSkillIndex,
  setSelectedGroupIndex,
  setSelectedCommandIndex,
  setSelectedSocialIndex,
  setSelectedTutorialIndex,
  setSelectedLootKind,
  setSelectedLootIndex,
  setActiveTab,
  setStatusMessage
}: UseGlobalCrudHandlersArgs) => {
  const handleCreateClass = useCallback(() => {
    const result = createClass(classData);
    if (!result.ok) {
      setStatusMessage(result.message);
      return;
    }
    setClassData(result.data.classData);
    setSelectedGlobalEntity("Classes");
    setSelectedClassIndex(result.data.index);
    setActiveTab("Form");
    setStatusMessage(`Created class ${result.data.name} (unsaved)`);
  }, [classData, setStatusMessage, setSelectedGlobalEntity, setSelectedClassIndex, setActiveTab, setClassData]);

  const handleDeleteClass = useCallback(() => {
    const result = deleteClass(classData, selectedClassIndex);
    if (!result.ok) {
      setStatusMessage(result.message);
      return;
    }
    setClassData(result.data.classData);
    setSelectedClassIndex(null);
    setStatusMessage(`Deleted ${result.data.deletedName} (unsaved)`);
  }, [classData, selectedClassIndex, setStatusMessage, setClassData, setSelectedClassIndex]);

  const handleCreateRace = useCallback(() => {
    const result = createRace(raceData, referenceData?.classes ?? []);
    if (!result.ok) {
      setStatusMessage(result.message);
      return;
    }
    setRaceData(result.data.raceData);
    setSelectedGlobalEntity("Races");
    setSelectedRaceIndex(result.data.index);
    setActiveTab("Form");
    setStatusMessage(`Created race ${result.data.name} (unsaved)`);
  }, [raceData, referenceData, setStatusMessage, setSelectedGlobalEntity, setSelectedRaceIndex, setActiveTab, setRaceData]);

  const handleDeleteRace = useCallback(() => {
    const result = deleteRace(raceData, selectedRaceIndex);
    if (!result.ok) {
      setStatusMessage(result.message);
      return;
    }
    setRaceData(result.data.raceData);
    setSelectedRaceIndex(null);
    setStatusMessage(`Deleted ${result.data.deletedName} (unsaved)`);
  }, [raceData, selectedRaceIndex, setStatusMessage, setRaceData, setSelectedRaceIndex]);

  const handleCreateSkill = useCallback(() => {
    const result = createSkill(skillData, referenceData?.classes ?? []);
    if (!result.ok) {
      setStatusMessage(result.message);
      return;
    }
    setSkillData(result.data.skillData);
    setSelectedGlobalEntity("Skills");
    setSelectedSkillIndex(result.data.index);
    setActiveTab("Form");
    setStatusMessage(`Created skill ${result.data.name} (unsaved)`);
  }, [skillData, referenceData, setStatusMessage, setSelectedGlobalEntity, setSelectedSkillIndex, setActiveTab, setSkillData]);

  const handleDeleteSkill = useCallback(() => {
    const result = deleteSkill(skillData, selectedSkillIndex);
    if (!result.ok) {
      setStatusMessage(result.message);
      return;
    }
    setSkillData(result.data.skillData);
    setSelectedSkillIndex(null);
    setStatusMessage(`Deleted ${result.data.deletedName} (unsaved)`);
  }, [skillData, selectedSkillIndex, setStatusMessage, setSkillData, setSelectedSkillIndex]);

  const handleCreateGroup = useCallback(() => {
    const result = createGroup(groupData, referenceData?.classes ?? []);
    if (!result.ok) {
      setStatusMessage(result.message);
      return;
    }
    setGroupData(result.data.groupData);
    setSelectedGlobalEntity("Groups");
    setSelectedGroupIndex(result.data.index);
    setActiveTab("Form");
    setStatusMessage(`Created group ${result.data.name} (unsaved)`);
  }, [groupData, referenceData, setStatusMessage, setSelectedGlobalEntity, setSelectedGroupIndex, setActiveTab, setGroupData]);

  const handleDeleteGroup = useCallback(() => {
    const result = deleteGroup(groupData, selectedGroupIndex);
    if (!result.ok) {
      setStatusMessage(result.message);
      return;
    }
    setGroupData(result.data.groupData);
    setSelectedGroupIndex(null);
    setStatusMessage(`Deleted ${result.data.deletedName} (unsaved)`);
  }, [groupData, selectedGroupIndex, setStatusMessage, setGroupData, setSelectedGroupIndex]);

  const handleCreateCommand = useCallback(() => {
    const result = createCommand(commandData);
    if (!result.ok) {
      setStatusMessage(result.message);
      return;
    }
    setCommandData(result.data.commandData);
    setSelectedGlobalEntity("Commands");
    setSelectedCommandIndex(result.data.index);
    setActiveTab("Form");
    setStatusMessage(`Created command ${result.data.name} (unsaved)`);
  }, [commandData, setStatusMessage, setSelectedGlobalEntity, setSelectedCommandIndex, setActiveTab, setCommandData]);

  const handleDeleteCommand = useCallback(() => {
    const result = deleteCommand(commandData, selectedCommandIndex);
    if (!result.ok) {
      setStatusMessage(result.message);
      return;
    }
    setCommandData(result.data.commandData);
    setSelectedCommandIndex(null);
    setStatusMessage(`Deleted ${result.data.deletedName} (unsaved)`);
  }, [commandData, selectedCommandIndex, setStatusMessage, setCommandData, setSelectedCommandIndex]);

  const handleCreateSocial = useCallback(() => {
    const result = createSocial(socialData);
    if (!result.ok) {
      setStatusMessage(result.message);
      return;
    }
    setSocialData(result.data.socialData);
    setSelectedGlobalEntity("Socials");
    setSelectedSocialIndex(result.data.index);
    setActiveTab("Form");
    setStatusMessage(`Created social ${result.data.name} (unsaved)`);
  }, [socialData, setStatusMessage, setSelectedGlobalEntity, setSelectedSocialIndex, setActiveTab, setSocialData]);

  const handleDeleteSocial = useCallback(() => {
    const result = deleteSocial(socialData, selectedSocialIndex);
    if (!result.ok) {
      setStatusMessage(result.message);
      return;
    }
    setSocialData(result.data.socialData);
    setSelectedSocialIndex(null);
    setStatusMessage(`Deleted ${result.data.deletedName} (unsaved)`);
  }, [socialData, selectedSocialIndex, setStatusMessage, setSocialData, setSelectedSocialIndex]);

  const handleCreateTutorial = useCallback(() => {
    const result = createTutorial(tutorialData);
    if (!result.ok) {
      setStatusMessage(result.message);
      return;
    }
    setTutorialData(result.data.tutorialData);
    setSelectedGlobalEntity("Tutorials");
    setSelectedTutorialIndex(result.data.index);
    setActiveTab("Form");
    setStatusMessage(`Created tutorial ${result.data.name} (unsaved)`);
  }, [tutorialData, setStatusMessage, setSelectedGlobalEntity, setSelectedTutorialIndex, setActiveTab, setTutorialData]);

  const handleDeleteTutorial = useCallback(() => {
    const result = deleteTutorial(tutorialData, selectedTutorialIndex);
    if (!result.ok) {
      setStatusMessage(result.message);
      return;
    }
    setTutorialData(result.data.tutorialData);
    setSelectedTutorialIndex(null);
    setStatusMessage(`Deleted ${result.data.deletedName} (unsaved)`);
  }, [tutorialData, selectedTutorialIndex, setStatusMessage, setTutorialData, setSelectedTutorialIndex]);

  const handleCreateLoot = useCallback((kind: "group" | "table") => {
    const result = createLoot(lootData, kind);
    if (!result.ok) {
      setStatusMessage(result.message);
      return;
    }
    setLootData(result.data.lootData);
    setSelectedGlobalEntity("Loot");
    setSelectedLootKind(result.data.kind);
    setSelectedLootIndex(result.data.index);
    setActiveTab("Form");
    setStatusMessage(
      `Created loot ${result.data.kind} ${result.data.index} (unsaved)`
    );
  }, [lootData, setStatusMessage, setSelectedGlobalEntity, setSelectedLootKind, setSelectedLootIndex, setActiveTab, setLootData]);

  const handleDeleteLoot = useCallback(() => {
    const result = deleteLoot(lootData, selectedLootKind, selectedLootIndex);
    if (!result.ok) {
      setStatusMessage(result.message);
      return;
    }
    setLootData(result.data.lootData);
    setSelectedLootKind(null);
    setSelectedLootIndex(null);
    setStatusMessage(
      `Deleted loot ${result.data.kind} ${result.data.index} (unsaved)`
    );
  }, [lootData, selectedLootKind, selectedLootIndex, setStatusMessage, setLootData, setSelectedLootKind, setSelectedLootIndex]);

  return {
    handleCreateClass,
    handleDeleteClass,
    handleCreateRace,
    handleDeleteRace,
    handleCreateSkill,
    handleDeleteSkill,
    handleCreateGroup,
    handleDeleteGroup,
    handleCreateCommand,
    handleDeleteCommand,
    handleCreateSocial,
    handleDeleteSocial,
    handleCreateTutorial,
    handleDeleteTutorial,
    handleCreateLoot,
    handleDeleteLoot
  };
};
