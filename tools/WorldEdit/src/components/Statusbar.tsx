type StatusbarProps = {
  errorMessage: string | null;
  statusMessage: string;
  areaPath: string | null;
  editorMetaPath: string | null;
  areaDirectory: string | null;
  dataDirectory: string | null;
  projectPath: string | null;
  selectedEntity: string;
  referenceSummary: string;
};

export function Statusbar({
  errorMessage,
  statusMessage,
  areaPath,
  editorMetaPath,
  areaDirectory,
  dataDirectory,
  projectPath,
  selectedEntity,
  referenceSummary
}: StatusbarProps) {
  return (
    <footer className="statusbar">
      <span>Status: {errorMessage ?? statusMessage}</span>
      <span>Config: {projectPath ?? "Not set"}</span>
      <span>Area file: {areaPath ?? "Not loaded"}</span>
      <span>Meta file: {editorMetaPath ?? "Not loaded"}</span>
      <span>Area dir: {areaDirectory ?? "Not set"}</span>
      <span>Data dir: {dataDirectory ?? "Not set"}</span>
      <span>Selection: {selectedEntity}</span>
      <span>Reference: {referenceSummary}</span>
    </footer>
  );
}
