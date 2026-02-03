/**
 * Area data store using Zustand.
 *
 * Manages state for the currently loaded area:
 * - Area file path and data
 * - Area directory (for file dialogs)
 * - Area index (list of all areas for cross-references)
 * - Area graph links (connections between areas)
 */

import { create } from "zustand";
import { devtools } from "zustand/middleware";
import type { AreaGraphLink, AreaIndexEntry, AreaJson } from "../repository/types";

/** Area store state interface. */
export interface AreaState {
  // Current area
  areaPath: string | null;
  areaData: AreaJson | null;

  // Directory for file dialogs (persisted to localStorage)
  areaDirectory: string | null;

  // Area index for cross-references
  areaIndex: AreaIndexEntry[];

  // Graph links between areas
  areaGraphLinks: AreaGraphLink[];

  // Actions
  setAreaPath: (path: string | null) => void;
  setAreaData: (data: AreaJson | null) => void;
  setAreaDirectory: (dir: string | null) => void;
  setAreaIndex: (index: AreaIndexEntry[]) => void;
  setAreaGraphLinks: (links: AreaGraphLink[]) => void;

  // Batch actions
  loadArea: (path: string, data: AreaJson) => void;
  clearArea: () => void;
}

/** LocalStorage key for area directory. */
const AREA_DIR_KEY = "worldedit.areaDir";

/**
 * Zustand store for area data.
 */
export const useAreaStore = create<AreaState>()(devtools(
  (set) => ({
  // Initial state
  areaPath: null,
  areaData: null,
  areaDirectory: localStorage.getItem(AREA_DIR_KEY),
  areaIndex: [],
  areaGraphLinks: [],

  // Actions
  setAreaPath: (path) => set({ areaPath: path }),
  setAreaData: (data) => set({ areaData: data }),
  setAreaDirectory: (dir) => {
    if (dir) {
      localStorage.setItem(AREA_DIR_KEY, dir);
    } else {
      localStorage.removeItem(AREA_DIR_KEY);
    }
    set({ areaDirectory: dir });
  },
  setAreaIndex: (index) => set({ areaIndex: index }),
  setAreaGraphLinks: (links) => set({ areaGraphLinks: links }),

  // Batch actions
  loadArea: (path, data) =>
    set({
      areaPath: path,
      areaData: data
    }),

  clearArea: () =>
    set({
      areaPath: null,
      areaData: null
    })
}), { name: "AreaStore" }));
