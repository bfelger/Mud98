type SelectionSummary = {
  kindLabel: string;
  selectionLabel: string;
  flags: string[];
  exits: string;
};

type ValidationSeverity = "error" | "warning";

type DiagnosticsIssue = {
  id: string;
  severity: ValidationSeverity;
  entityType: string;
  message: string;
  vnum?: number;
  resetIndex?: number;
};

type AreaDebugSummary = {
  keys: string[];
  arrayCounts: Array<{ key: string; count: number }>;
};

type InspectorPanelProps = {
  selection: SelectionSummary;
  validationSummary: string;
  diagnosticsCount: number;
  diagnosticFilter: string;
  entityOrder: readonly string[];
  diagnosticsList: DiagnosticsIssue[];
  entityKindLabels: Record<string, string>;
  onDiagnosticFilterChange: (value: string) => void;
  onDiagnosticsClick: (issue: DiagnosticsIssue) => void;
  areaDebug: AreaDebugSummary;
};

export function InspectorPanel({
  selection,
  validationSummary,
  diagnosticsCount,
  diagnosticFilter,
  entityOrder,
  diagnosticsList,
  entityKindLabels,
  onDiagnosticFilterChange,
  onDiagnosticsClick,
  areaDebug
}: InspectorPanelProps) {
  return (
    <aside className="inspector">
      <div className="panel-header">
        <h2>Inspector</h2>
        <button className="ghost-button" type="button">
          Pin
        </button>
      </div>
      <div className="inspector__section">
        <div className="inspector__label">Selected {selection.kindLabel}</div>
        <div className="inspector__value">{selection.selectionLabel}</div>
      </div>
      <div className="inspector__section">
        <div className="inspector__label">Flags</div>
        <div className="inspector__tags">
          {selection.flags.length ? (
            selection.flags.map((flag) => <span key={flag}>{flag}</span>)
          ) : (
            <span className="inspector__empty">No flags</span>
          )}
        </div>
      </div>
      <div className="inspector__section">
        <div className="inspector__label">Exits</div>
        <div className="inspector__value">{selection.exits}</div>
      </div>
      <div className="inspector__section">
        <div className="inspector__label">Validation</div>
        <div className="inspector__value inspector__value--warn">
          {validationSummary}
        </div>
      </div>
      <div className="inspector__section">
        <div className="diagnostics__header">
          <span className="inspector__label">Diagnostics</span>
          <span className="diagnostics__count">{diagnosticsCount}</span>
        </div>
        <div className="diagnostics__filters">
          <label className="diagnostics__filter-label" htmlFor="diag-filter">
            Entity
          </label>
          <select
            id="diag-filter"
            className="form-select diagnostics__select"
            value={diagnosticFilter}
            onChange={(event) => onDiagnosticFilterChange(event.target.value)}
          >
            <option value="All">All</option>
            {entityOrder.map((entity) => (
              <option key={entity} value={entity}>
                {entity}
              </option>
            ))}
          </select>
        </div>
        {diagnosticsCount ? (
          <ul className="diagnostics__list">
            {diagnosticsList.map((issue) => {
              const targetLabel =
                issue.entityType === "Resets" &&
                typeof issue.resetIndex === "number"
                  ? `Reset #${issue.resetIndex + 1}`
                  : typeof issue.vnum === "number"
                    ? `${entityKindLabels[issue.entityType]} ${issue.vnum}`
                    : issue.entityType;
              const canNavigate =
                (issue.entityType === "Resets" &&
                  typeof issue.resetIndex === "number") ||
                typeof issue.vnum === "number";
              return (
                <li key={issue.id}>
                  <button
                    type="button"
                    className={`diagnostics__item diagnostics__item--${issue.severity}`}
                    onClick={() => onDiagnosticsClick(issue)}
                    disabled={!canNavigate}
                  >
                    <span
                      className={`diagnostics__severity diagnostics__severity--${issue.severity}`}
                    >
                      {issue.severity}
                    </span>
                    <span className="diagnostics__message">
                      {issue.message}
                    </span>
                    <span className="diagnostics__meta">{targetLabel}</span>
                  </button>
                </li>
              );
            })}
          </ul>
        ) : (
          <div className="diagnostics__empty">No diagnostics.</div>
        )}
      </div>
      <div className="inspector__section">
        <div className="inspector__label">Area Data Debug</div>
        {areaDebug.keys.length ? (
          <>
            <div className="inspector__value">
              Top-level keys: {areaDebug.keys.join(", ")}
            </div>
            {areaDebug.arrayCounts.length ? (
              <div className="inspector__debug-list">
                {areaDebug.arrayCounts.map((entry) => (
                  <div key={entry.key}>
                    {entry.key}: {entry.count}
                  </div>
                ))}
              </div>
            ) : null}
          </>
        ) : (
          <div className="inspector__empty">No area data loaded.</div>
        )}
      </div>
    </aside>
  );
}
