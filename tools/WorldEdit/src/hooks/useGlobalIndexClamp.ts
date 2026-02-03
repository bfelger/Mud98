/**
 * Hook to automatically clamp global entity selection indices.
 * 
 * Consolidates 7+ nearly identical useEffects into a single data-driven approach.
 * For each global data type, ensures that:
 * - Index is null when data is empty
 * - Index is 0 when data loads and index is null/out of range
 * - Index stays valid when items are deleted
 */

import { useEffect } from "react";
import { useSelectionStore } from "../stores/useSelectionStore";
import { useGlobalDataStore } from "../stores/useGlobalDataStore";

/**
 * Hook that auto-clamps selection indices when global data changes.
 * 
 * Replaces 7 separate useEffects (~100 lines) with a single data-driven approach.
 * The Loot entity has special handling due to its group/table dual structure.
 */
export function useGlobalIndexClamp(): void {
  const {
    selectedClassIndex,
    selectedRaceIndex,
    selectedSkillIndex,
    selectedGroupIndex,
    selectedCommandIndex,
    selectedSocialIndex,
    selectedTutorialIndex,
    selectedGlobalLoot,
    setSelectedClassIndex,
    setSelectedRaceIndex,
    setSelectedSkillIndex,
    setSelectedGroupIndex,
    setSelectedCommandIndex,
    setSelectedSocialIndex,
    setSelectedTutorialIndex,
    setSelectedGlobalLoot
  } = useSelectionStore();

  const {
    classData,
    raceData,
    skillData,
    groupData,
    commandData,
    socialData,
    tutorialData,
    lootData
  } = useGlobalDataStore();

  // Helper to clamp a single index
  const clampIndex = (
    data: { length: number } | null | undefined,
    index: number | null,
    setIndex: (i: number | null) => void
  ) => {
    const length = data?.length ?? 0;
    if (length === 0) {
      if (index !== null) setIndex(null);
      return;
    }
    if (index === null || index >= length) {
      setIndex(0);
    }
  };

  // Classes
  useEffect(() => {
    clampIndex(classData?.classes, selectedClassIndex, setSelectedClassIndex);
  }, [classData, selectedClassIndex, setSelectedClassIndex]);

  // Races
  useEffect(() => {
    clampIndex(raceData?.races, selectedRaceIndex, setSelectedRaceIndex);
  }, [raceData, selectedRaceIndex, setSelectedRaceIndex]);

  // Skills
  useEffect(() => {
    clampIndex(skillData?.skills, selectedSkillIndex, setSelectedSkillIndex);
  }, [skillData, selectedSkillIndex, setSelectedSkillIndex]);

  // Groups
  useEffect(() => {
    clampIndex(groupData?.groups, selectedGroupIndex, setSelectedGroupIndex);
  }, [groupData, selectedGroupIndex, setSelectedGroupIndex]);

  // Commands
  useEffect(() => {
    clampIndex(commandData?.commands, selectedCommandIndex, setSelectedCommandIndex);
  }, [commandData, selectedCommandIndex, setSelectedCommandIndex]);

  // Socials
  useEffect(() => {
    clampIndex(socialData?.socials, selectedSocialIndex, setSelectedSocialIndex);
  }, [socialData, selectedSocialIndex, setSelectedSocialIndex]);

  // Tutorials
  useEffect(() => {
    clampIndex(tutorialData?.tutorials, selectedTutorialIndex, setSelectedTutorialIndex);
  }, [tutorialData, selectedTutorialIndex, setSelectedTutorialIndex]);

  // Loot (special handling for dual group/table structure)
  useEffect(() => {
    const groupCount = lootData?.groups.length ?? 0;
    const tableCount = lootData?.tables.length ?? 0;
    const { kind: selectedLootKind, index: selectedLootIndex } = selectedGlobalLoot;

    if (!lootData || (groupCount === 0 && tableCount === 0)) {
      if (selectedLootKind !== null || selectedLootIndex !== null) {
        setSelectedGlobalLoot({ kind: null, index: null });
      }
      return;
    }

    if (selectedLootKind === "group") {
      if (selectedLootIndex === null || selectedLootIndex >= groupCount) {
        setSelectedGlobalLoot({
          kind: groupCount ? "group" : "table",
          index: 0
        });
      }
      return;
    }

    if (selectedLootKind === "table") {
      if (selectedLootIndex === null || selectedLootIndex >= tableCount) {
        setSelectedGlobalLoot({
          kind: groupCount ? "group" : "table",
          index: 0
        });
      }
      return;
    }

    // No kind selected yet, pick first available
    setSelectedGlobalLoot({
      kind: groupCount ? "group" : "table",
      index: 0
    });
  }, [lootData, selectedGlobalLoot, setSelectedGlobalLoot]);
}
