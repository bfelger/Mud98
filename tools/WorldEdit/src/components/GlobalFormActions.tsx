type GlobalFormActionsProps = {
  label: string;
  dataFileLabel: string;
  loadDisabled?: boolean;
  saveDisabled?: boolean;
  onLoad: () => void;
  onSave: () => void;
};

export function GlobalFormActions({
  label,
  dataFileLabel,
  loadDisabled,
  saveDisabled,
  onLoad,
  onSave
}: GlobalFormActionsProps) {
  return (
    <div className="form-toolbar">
      <div className="form-toolbar__meta">
        <span className="form-toolbar__title">{label}</span>
        <span className="form-toolbar__subtitle">
          Data file {dataFileLabel}
        </span>
      </div>
      <div className="form-toolbar__actions">
        <button
          className="action-button"
          type="button"
          onClick={onLoad}
          disabled={loadDisabled}
        >
          Load {label}
        </button>
        <button
          className="action-button action-button--primary"
          type="button"
          onClick={onSave}
          disabled={saveDisabled}
        >
          Save {label}
        </button>
      </div>
    </div>
  );
}
