import { useSelectionStore } from "../stores/useSelectionStore";
import { useEditorUiStore } from "../stores/useEditorUiStore";
import { useGlobalDataStore } from "../stores/useGlobalDataStore";
import { useAreaStore } from "../stores/useAreaStore";

type StatusbarProps = {
  /** Path to project config file */
  projectPath: string | null;
  /** Summary of loaded reference data */
  referenceSummary: string;
};

export function Statusbar({ projectPath, referenceSummary }: StatusbarProps) {
  // Get state from stores
  const { editorMode, selectedEntity, selectedGlobalEntity } = useSelectionStore();
  const { statusMessage, errorMessage } = useEditorUiStore();
  const { editorMetaPath, dataDirectory } = useGlobalDataStore();
  const { areaPath, areaDirectory } = useAreaStore();

  // Determine which entity to show based on mode
  const currentEntity = editorMode === "Area" ? selectedEntity : selectedGlobalEntity;

  return (
    <footer className="statusbar">
      <span>Status: {errorMessage ?? statusMessage}</span>
      <span>Config: {projectPath ?? "Not set"}</span>
      <span>Area file: {areaPath ?? "Not loaded"}</span>
      <span>Meta file: {editorMetaPath ?? "Not loaded"}</span>
      <span>Area dir: {areaDirectory ?? "Not set"}</span>
      <span>Data dir: {dataDirectory ?? "Not set"}</span>
      <span>Selection: {currentEntity}</span>
      <span>Reference: {referenceSummary}</span>
    </footer>
  );
}
