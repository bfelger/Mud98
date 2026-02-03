/**
 * Selection state store using Zustand.
 *
 * Manages all selection state for both area entities and global data entities.
 * This centralizes the ~20 selection-related useState calls from App.tsx.
 */

import { create } from "zustand";
import type { EntityKey, GlobalEntityKey } from "../constants/appConfig";

/** Selection state for loot editor (used in both area and global contexts). */
export type LootSelection = {
  kind: "group" | "table" | null;
  index: number | null;
};

/** Full selection state interface. */
export interface SelectionState {
  // Editor mode
  editorMode: "Area" | "Global";

  // Area entity selection
  selectedEntity: EntityKey;
  selectedRoomVnum: number | null;
  selectedMobileVnum: number | null;
  selectedObjectVnum: number | null;
  selectedResetIndex: number | null;
  selectedShopKeeper: number | null;
  selectedQuestVnum: number | null;
  selectedFactionVnum: number | null;
  selectedRecipeVnum: number | null;
  selectedGatherVnum: number | null;
  selectedAreaLoot: LootSelection;

  // Global entity selection
  selectedGlobalEntity: GlobalEntityKey;
  selectedClassIndex: number | null;
  selectedRaceIndex: number | null;
  selectedSkillIndex: number | null;
  selectedGroupIndex: number | null;
  selectedCommandIndex: number | null;
  selectedSocialIndex: number | null;
  selectedTutorialIndex: number | null;
  selectedGlobalLoot: LootSelection;

  // Actions - Editor mode
  setEditorMode: (mode: "Area" | "Global") => void;

  // Actions - Area selections
  setSelectedEntity: (entity: EntityKey) => void;
  setSelectedRoomVnum: (vnum: number | null) => void;
  setSelectedMobileVnum: (vnum: number | null) => void;
  setSelectedObjectVnum: (vnum: number | null) => void;
  setSelectedResetIndex: (index: number | null) => void;
  setSelectedShopKeeper: (keeper: number | null) => void;
  setSelectedQuestVnum: (vnum: number | null) => void;
  setSelectedFactionVnum: (vnum: number | null) => void;
  setSelectedRecipeVnum: (vnum: number | null) => void;
  setSelectedGatherVnum: (vnum: number | null) => void;
  setSelectedAreaLoot: (selection: LootSelection) => void;

  // Actions - Global selections
  setSelectedGlobalEntity: (entity: GlobalEntityKey) => void;
  setSelectedClassIndex: (index: number | null) => void;
  setSelectedRaceIndex: (index: number | null) => void;
  setSelectedSkillIndex: (index: number | null) => void;
  setSelectedGroupIndex: (index: number | null) => void;
  setSelectedCommandIndex: (index: number | null) => void;
  setSelectedSocialIndex: (index: number | null) => void;
  setSelectedTutorialIndex: (index: number | null) => void;
  setSelectedGlobalLoot: (selection: LootSelection) => void;

  // Batch actions
  clearAreaSelections: () => void;
  clearGlobalSelections: () => void;

  // Helper to get current VNUM for the selected entity type
  getSelectedVnum: () => number | null;
}

/** Default loot selection state. */
const defaultLootSelection: LootSelection = { kind: null, index: null };

/**
 * Zustand store for all selection state.
 *
 * Usage in components:
 * ```tsx
 * const selectedEntity = useSelectionStore((s) => s.selectedEntity);
 * const setSelectedEntity = useSelectionStore((s) => s.setSelectedEntity);
 * // or destructure multiple:
 * const { selectedEntity, setSelectedEntity } = useSelectionStore();
 * ```
 */
export const useSelectionStore = create<SelectionState>((set, get) => ({
  // Initial state - Editor mode
  editorMode: "Area",

  // Initial state - Area selections
  selectedEntity: "Rooms",
  selectedRoomVnum: null,
  selectedMobileVnum: null,
  selectedObjectVnum: null,
  selectedResetIndex: null,
  selectedShopKeeper: null,
  selectedQuestVnum: null,
  selectedFactionVnum: null,
  selectedRecipeVnum: null,
  selectedGatherVnum: null,
  selectedAreaLoot: defaultLootSelection,

  // Initial state - Global selections
  selectedGlobalEntity: "Classes",
  selectedClassIndex: null,
  selectedRaceIndex: null,
  selectedSkillIndex: null,
  selectedGroupIndex: null,
  selectedCommandIndex: null,
  selectedSocialIndex: null,
  selectedTutorialIndex: null,
  selectedGlobalLoot: defaultLootSelection,

  // Actions - Editor mode
  setEditorMode: (mode) => set({ editorMode: mode }),

  // Actions - Area selections
  setSelectedEntity: (entity) => set({ selectedEntity: entity }),
  setSelectedRoomVnum: (vnum) => set({ selectedRoomVnum: vnum }),
  setSelectedMobileVnum: (vnum) => set({ selectedMobileVnum: vnum }),
  setSelectedObjectVnum: (vnum) => set({ selectedObjectVnum: vnum }),
  setSelectedResetIndex: (index) => set({ selectedResetIndex: index }),
  setSelectedShopKeeper: (keeper) => set({ selectedShopKeeper: keeper }),
  setSelectedQuestVnum: (vnum) => set({ selectedQuestVnum: vnum }),
  setSelectedFactionVnum: (vnum) => set({ selectedFactionVnum: vnum }),
  setSelectedRecipeVnum: (vnum) => set({ selectedRecipeVnum: vnum }),
  setSelectedGatherVnum: (vnum) => set({ selectedGatherVnum: vnum }),
  setSelectedAreaLoot: (selection) => set({ selectedAreaLoot: selection }),

  // Actions - Global selections
  setSelectedGlobalEntity: (entity) => set({ selectedGlobalEntity: entity }),
  setSelectedClassIndex: (index) => set({ selectedClassIndex: index }),
  setSelectedRaceIndex: (index) => set({ selectedRaceIndex: index }),
  setSelectedSkillIndex: (index) => set({ selectedSkillIndex: index }),
  setSelectedGroupIndex: (index) => set({ selectedGroupIndex: index }),
  setSelectedCommandIndex: (index) => set({ selectedCommandIndex: index }),
  setSelectedSocialIndex: (index) => set({ selectedSocialIndex: index }),
  setSelectedTutorialIndex: (index) => set({ selectedTutorialIndex: index }),
  setSelectedGlobalLoot: (selection) => set({ selectedGlobalLoot: selection }),

  // Batch actions
  clearAreaSelections: () =>
    set({
      selectedRoomVnum: null,
      selectedMobileVnum: null,
      selectedObjectVnum: null,
      selectedResetIndex: null,
      selectedShopKeeper: null,
      selectedQuestVnum: null,
      selectedFactionVnum: null,
      selectedRecipeVnum: null,
      selectedGatherVnum: null,
      selectedAreaLoot: defaultLootSelection
    }),

  clearGlobalSelections: () =>
    set({
      selectedClassIndex: null,
      selectedRaceIndex: null,
      selectedSkillIndex: null,
      selectedGroupIndex: null,
      selectedCommandIndex: null,
      selectedSocialIndex: null,
      selectedTutorialIndex: null,
      selectedGlobalLoot: defaultLootSelection
    }),

  // Helper to get current VNUM based on selected entity
  getSelectedVnum: () => {
    const state = get();
    switch (state.selectedEntity) {
      case "Rooms":
        return state.selectedRoomVnum;
      case "Mobiles":
        return state.selectedMobileVnum;
      case "Objects":
        return state.selectedObjectVnum;
      case "Quests":
        return state.selectedQuestVnum;
      case "Factions":
        return state.selectedFactionVnum;
      case "Recipes":
        return state.selectedRecipeVnum;
      case "Gather Spawns":
        return state.selectedGatherVnum;
      default:
        return null;
    }
  }
}));

/**
 * Hook to get the selected VNUM map (for passing to components that need all vnums).
 * This provides backward compatibility with the old selectedVnums pattern.
 */
export function useSelectedVnums() {
  return useSelectionStore((state) => ({
    Rooms: state.selectedRoomVnum,
    Mobiles: state.selectedMobileVnum,
    Objects: state.selectedObjectVnum,
    Resets: state.selectedResetIndex,
    Shops: state.selectedShopKeeper,
    Quests: state.selectedQuestVnum,
    Factions: state.selectedFactionVnum,
    Recipes: state.selectedRecipeVnum,
    "Gather Spawns": state.selectedGatherVnum
  }));
}
