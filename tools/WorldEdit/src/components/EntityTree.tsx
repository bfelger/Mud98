type EntityTreeItem = {
  key: string;
  count: number;
};

type EntityTreeProps = {
  mode: "Area" | "Global";
  onModeChange: (mode: "Area" | "Global") => void;
  items: EntityTreeItem[];
  selectedEntity: string;
  onSelectEntity: (entity: string) => void;
};

export function EntityTree({
  mode,
  onModeChange,
  items,
  selectedEntity,
  onSelectEntity
}: EntityTreeProps) {
  return (
    <aside className="sidebar">
      <div className="panel-header">
        <h2>Entity Tree</h2>
        <button className="ghost-button" type="button">
          New
        </button>
      </div>
      <div className="tree-mode">
        <div className="tree-mode__label">Mode</div>
        <div className="tree-mode__buttons" role="group" aria-label="Editor mode">
          <button
            className={`tree-mode__button${
              mode === "Area" ? " tree-mode__button--active" : ""
            }`}
            type="button"
            aria-pressed={mode === "Area"}
            onClick={() => onModeChange("Area")}
          >
            Area
          </button>
          <button
            className={`tree-mode__button${
              mode === "Global" ? " tree-mode__button--active" : ""
            }`}
            type="button"
            aria-pressed={mode === "Global"}
            onClick={() => onModeChange("Global")}
          >
            Global
          </button>
        </div>
      </div>
      <div className="tree">
        <div className="tree-group">
          <div className="tree-group__label">
            {mode === "Area" ? "Area" : "Global Data"}
          </div>
          <ul className="tree-list">
            {items.map((child) => (
              <li key={child.key}>
                <button
                  className={`tree-item${
                    child.key === selectedEntity ? " tree-item--active" : ""
                  }`}
                  type="button"
                  aria-pressed={child.key === selectedEntity}
                  onClick={() => onSelectEntity(child.key)}
                >
                  <span className="tree-item__label">{child.key}</span>
                  <span className="tree-item__count">{child.count}</span>
                </button>
              </li>
            ))}
          </ul>
        </div>
      </div>
    </aside>
  );
}
