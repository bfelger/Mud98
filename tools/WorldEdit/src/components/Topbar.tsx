type TopbarProps = {
  areaName: string;
  errorMessage: string | null;
  statusMessage: string;
  isBusy: boolean;
  hasArea: boolean;
  hasAreaPath: boolean;
  onOpenProject: () => void;
  onOpenArea: () => void;
  onSetAreaDirectory: () => void;
  onSaveArea: () => void;
  onSaveEditorMeta: () => void;
  onSaveAreaAs: () => void;
};

export function Topbar({
  areaName,
  errorMessage,
  statusMessage,
  isBusy,
  hasArea,
  hasAreaPath,
  onOpenProject,
  onOpenArea,
  onSetAreaDirectory,
  onSaveArea,
  onSaveEditorMeta,
  onSaveAreaAs
}: TopbarProps) {
  return (
    <header className="topbar">
      <div className="brand">
        <span className="brand__badge">Mud98</span>
        <div className="brand__text">
          <span className="brand__title">WorldEdit</span>
          <span className="brand__subtitle">Area JSON editor</span>
        </div>
      </div>
      <div className="topbar__status">
        <span
          className={`status-pill ${
            errorMessage ? "status-pill--error" : "status-pill--ok"
          }`}
        >
          {errorMessage ?? statusMessage}
        </span>
        <span className="status-pill">Area: {areaName}</span>
      </div>
      <div className="topbar__actions">
        <button
          className="action-button"
          type="button"
          onClick={onOpenProject}
          disabled={isBusy}
        >
          Open Config
        </button>
        <button
          className="action-button"
          type="button"
          onClick={onOpenArea}
          disabled={isBusy}
        >
          Open Area
        </button>
        <button
          className="action-button"
          type="button"
          onClick={onSetAreaDirectory}
          disabled={isBusy}
        >
          Set Area Dir
        </button>
        <button
          className="action-button action-button--primary"
          type="button"
          onClick={onSaveArea}
          disabled={!hasArea || isBusy}
        >
          Save
        </button>
        <button
          className="action-button"
          type="button"
          onClick={onSaveEditorMeta}
          disabled={!hasAreaPath || isBusy}
        >
          Save Meta
        </button>
        <button
          className="action-button"
          type="button"
          onClick={onSaveAreaAs}
          disabled={!hasArea || isBusy}
        >
          Save As
        </button>
      </div>
    </header>
  );
}
