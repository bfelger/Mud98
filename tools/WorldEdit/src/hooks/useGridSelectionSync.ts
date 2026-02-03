import { useEffect, useMemo, type RefObject } from "react";
import type { GridApi } from "ag-grid-community";
import { syncGridSelection } from "../utils/appUtils";
import { useEditorUiStore } from "../stores/useEditorUiStore";
import { useSelectionStore } from "../stores/useSelectionStore";
import type { EntityKey, GlobalEntityKey } from "../constants/appConfig";

/**
 * Grid refs organized by entity type.
 */
export type AreaGridRefs = {
  Rooms: RefObject<GridApi | null>;
  Mobiles: RefObject<GridApi | null>;
  Objects: RefObject<GridApi | null>;
  Resets: RefObject<GridApi | null>;
  Shops: RefObject<GridApi | null>;
  Quests: RefObject<GridApi | null>;
  Factions: RefObject<GridApi | null>;
  Loot: RefObject<GridApi | null>;
  Recipes: RefObject<GridApi | null>;
  "Gather Spawns": RefObject<GridApi | null>;
};

export type GlobalGridRefs = {
  Classes: RefObject<GridApi | null>;
  Races: RefObject<GridApi | null>;
  Skills: RefObject<GridApi | null>;
  Groups: RefObject<GridApi | null>;
  Commands: RefObject<GridApi | null>;
  Socials: RefObject<GridApi | null>;
  Tutorials: RefObject<GridApi | null>;
  Loot: RefObject<GridApi | null>;
};

/**
 * Selection values for each area entity type.
 */
type AreaSelectionValues = {
  Rooms: number | null;
  Mobiles: number | null;
  Objects: number | null;
  Resets: number | null;
  Shops: number | null;
  Quests: number | null;
  Factions: number | null;
  Loot: string | null; // composite "kind:index"
  Recipes: number | null;
  "Gather Spawns": number | null;
};

type GlobalSelectionValues = {
  Classes: number | null;
  Races: number | null;
  Skills: number | null;
  Groups: number | null;
  Commands: number | null;
  Socials: number | null;
  Tutorials: number | null;
  Loot: string | null; // composite "kind:index"
};

type UseAreaGridSyncParams = {
  gridRefs: AreaGridRefs;
  selectionValues: AreaSelectionValues;
  /** Any values that should trigger re-sync (e.g., row data) */
  rowsDeps: unknown[];
};

type UseGlobalGridSyncParams = {
  gridRefs: GlobalGridRefs;
  selectionValues: GlobalSelectionValues;
  rowsDeps: unknown[];
};

/**
 * Syncs area entity grid selection when activeTab is "Table" and 
 * the selected entity matches the config.
 */
export function useAreaGridSync({
  gridRefs,
  selectionValues,
  rowsDeps
}: UseAreaGridSyncParams) {
  const { activeTab } = useEditorUiStore();
  const { selectedEntity } = useSelectionStore();

  // Memoize selection values to get stable reference
  const selections = useMemo(
    () => selectionValues,
    // eslint-disable-next-line react-hooks/exhaustive-deps
    Object.values(selectionValues)
  );

  useEffect(() => {
    if (activeTab !== "Table") {
      return;
    }

    // Only sync for entities that have grids (not Area)
    const ref = gridRefs[selectedEntity as keyof AreaGridRefs];
    if (!ref) {
      return;
    }

    const rowId = selections[selectedEntity as keyof AreaSelectionValues];
    syncGridSelection(ref.current, rowId);
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [activeTab, selectedEntity, selections, ...rowsDeps]);
}

/**
 * Syncs global entity grid selection when in Global mode, activeTab is "Table",
 * and the selected global entity matches the config.
 */
export function useGlobalGridSync({
  gridRefs,
  selectionValues,
  rowsDeps
}: UseGlobalGridSyncParams) {
  const { activeTab } = useEditorUiStore();
  const { editorMode, selectedGlobalEntity } = useSelectionStore();

  // Memoize selection values to get stable reference
  const selections = useMemo(
    () => selectionValues,
    // eslint-disable-next-line react-hooks/exhaustive-deps
    Object.values(selectionValues)
  );

  useEffect(() => {
    if (editorMode !== "Global" || activeTab !== "Table") {
      return;
    }

    const ref = gridRefs[selectedGlobalEntity as keyof GlobalGridRefs];
    if (!ref) {
      return;
    }

    const rowId = selections[selectedGlobalEntity as keyof GlobalSelectionValues];
    syncGridSelection(ref.current, rowId);
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [editorMode, activeTab, selectedGlobalEntity, selections, ...rowsDeps]);
}

