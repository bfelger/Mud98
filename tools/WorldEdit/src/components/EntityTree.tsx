type EntityTreeItem = {
  key: string;
  count: number;
};

type EntityTreeProps = {
  items: EntityTreeItem[];
  selectedEntity: string;
  onSelectEntity: (entity: string) => void;
};

export function EntityTree({
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
      <div className="tree">
        <div className="tree-group">
          <div className="tree-group__label">Area</div>
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
