/**
 * Editor UI state store using Zustand.
 *
 * Manages UI-related state like:
 * - Active tab
 * - Status and error messages
 * - Busy state
 * - Diagnostic filter
 */

import { create } from "zustand";
import type { EntityKey } from "../constants/appConfig";
import { tabs } from "../constants/appConfig";

/** Tab identifier type. */
export type TabId = (typeof tabs)[number]["id"];

/** Diagnostic filter options. */
export type DiagnosticFilter = "All" | EntityKey;

/** UI state interface. */
export interface EditorUiState {
  // Active view tab
  activeTab: TabId;

  // Status/feedback
  statusMessage: string;
  errorMessage: string | null;
  isBusy: boolean;

  // Diagnostic panel filter
  diagnosticFilter: DiagnosticFilter;

  // Actions
  setActiveTab: (tab: TabId) => void;
  setStatusMessage: (message: string) => void;
  setErrorMessage: (message: string | null) => void;
  setIsBusy: (busy: boolean) => void;
  setDiagnosticFilter: (filter: DiagnosticFilter) => void;

  // Batch actions
  showError: (message: string) => void;
  clearError: () => void;
  withBusy: <T>(action: () => Promise<T>) => Promise<T>;
}

/**
 * Zustand store for editor UI state.
 */
export const useEditorUiStore = create<EditorUiState>((set, get) => ({
  // Initial state
  activeTab: tabs[0].id,
  statusMessage: "No area loaded",
  errorMessage: null,
  isBusy: false,
  diagnosticFilter: "All",

  // Actions
  setActiveTab: (tab) => set({ activeTab: tab }),
  setStatusMessage: (message) => set({ statusMessage: message }),
  setErrorMessage: (message) => set({ errorMessage: message }),
  setIsBusy: (busy) => set({ isBusy: busy }),
  setDiagnosticFilter: (filter) => set({ diagnosticFilter: filter }),

  // Batch actions
  showError: (message) => set({ errorMessage: message }),
  clearError: () => set({ errorMessage: null }),

  /**
   * Wrap an async action with busy state management.
   * Sets isBusy=true before, isBusy=false after (even on error).
   */
  withBusy: async <T>(action: () => Promise<T>): Promise<T> => {
    set({ isBusy: true });
    try {
      return await action();
    } finally {
      set({ isBusy: false });
    }
  }
}));
