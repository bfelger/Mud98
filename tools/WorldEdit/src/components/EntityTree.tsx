import { useSelectionStore } from "../stores/useSelectionStore";
import type { EntityKey, GlobalEntityKey } from "../constants/appConfig";

type EntityTreeItem = {
  key: string;
  count: number;
};

type EntityTreeProps = {
  /** Items to display - computed from areaData/globalData in parent */
  items: EntityTreeItem[];
};

export function EntityTree({ items }: EntityTreeProps) {
  // Get mode and selection state from store
  const {
    editorMode,
    setEditorMode,
    selectedEntity,
    setSelectedEntity,
    selectedGlobalEntity,
    setSelectedGlobalEntity
  } = useSelectionStore();

  // Determine which entity is selected based on mode
  const currentSelectedEntity =
    editorMode === "Area" ? selectedEntity : selectedGlobalEntity;

  // Handle entity selection
  const handleSelectEntity = (entity: string) => {
    if (editorMode === "Area") {
      setSelectedEntity(entity as EntityKey);
    } else {
      setSelectedGlobalEntity(entity as GlobalEntityKey);
    }
  };

  return (
    <aside className="sidebar">
      <div className="panel-header">
        <h2>Entity Tree</h2>
      </div>
      <div className="tree-mode">
        <div className="tree-mode__label">Mode</div>
        <div className="tree-mode__buttons" role="group" aria-label="Editor mode">
          <button
            className={`tree-mode__button${
              editorMode === "Area" ? " tree-mode__button--active" : ""
            }`}
            type="button"
            aria-pressed={editorMode === "Area"}
            onClick={() => setEditorMode("Area")}
          >
            Area
          </button>
          <button
            className={`tree-mode__button${
              editorMode === "Global" ? " tree-mode__button--active" : ""
            }`}
            type="button"
            aria-pressed={editorMode === "Global"}
            onClick={() => setEditorMode("Global")}
          >
            Global
          </button>
        </div>
      </div>
      <div className="tree">
        <div className="tree-group">
          <div className="tree-group__label">
            {editorMode === "Area" ? "Area" : "Global Data"}
          </div>
          <ul className="tree-list">
            {items.map((child) => (
              <li key={child.key}>
                <button
                  className={`tree-item${
                    child.key === currentSelectedEntity ? " tree-item--active" : ""
                  }`}
                  type="button"
                  aria-pressed={child.key === currentSelectedEntity}
                  onClick={() => handleSelectEntity(child.key)}
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
