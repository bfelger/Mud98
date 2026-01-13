const tabs = ["Table", "Form", "Map", "Script"] as const;
const entities = [
  {
    label: "Area",
    children: [
      { label: "Rooms", count: 42 },
      { label: "Mobiles", count: 18 },
      { label: "Objects", count: 26 },
      { label: "Resets", count: 31 },
      { label: "Shops", count: 2 },
      { label: "Quests", count: 1 },
      { label: "Factions", count: 3 },
      { label: "Helps", count: 9 }
    ]
  }
];

export default function App() {
  return (
    <div className="app-shell">
      <header className="topbar">
        <div className="brand">
          <span className="brand__badge">Mud98</span>
          <div className="brand__text">
            <span className="brand__title">WorldEdit</span>
            <span className="brand__subtitle">Area JSON editor</span>
          </div>
        </div>
        <div className="topbar__status">
          <span className="status-pill status-pill--ok">Workspace loaded</span>
          <span className="status-pill">Area: Faladri</span>
        </div>
      </header>

      <section className="workspace">
        <aside className="sidebar">
          <div className="panel-header">
            <h2>Entity Tree</h2>
            <button className="ghost-button" type="button">
              New
            </button>
          </div>
          <div className="tree">
            {entities.map((group) => (
              <div className="tree-group" key={group.label}>
                <div className="tree-group__label">{group.label}</div>
                <ul className="tree-list">
                  {group.children.map((child) => (
                    <li className="tree-item" key={child.label}>
                      <span className="tree-item__label">{child.label}</span>
                      <span className="tree-item__count">{child.count}</span>
                    </li>
                  ))}
                </ul>
              </div>
            ))}
          </div>
        </aside>

        <main className="center">
          <nav className="tabs" aria-label="View tabs">
            {tabs.map((tab, index) => (
              <button
                key={tab}
                type="button"
                className={`tab${index === 0 ? " tab--active" : ""}`}
              >
                {tab}
              </button>
            ))}
          </nav>

          <div className="view-card">
            <div className="view-card__header">
              <h2>Rooms</h2>
              <div className="view-card__meta">
                <span>VNUM range 3000-3099</span>
                <span>Last save 2 min ago</span>
              </div>
            </div>
            <div className="view-card__body">
              <div className="placeholder-grid">
                <div className="placeholder-block">
                  <div className="placeholder-title">Table View</div>
                  <p>Filterable grid for Rooms, Mobiles, Objects, Resets.</p>
                </div>
                <div className="placeholder-block">
                  <div className="placeholder-title">Map View</div>
                  <p>Orthogonal room layout with exit routing.</p>
                </div>
                <div className="placeholder-block">
                  <div className="placeholder-title">Form View</div>
                  <p>Schema-driven editor with validation.</p>
                </div>
                <div className="placeholder-block">
                  <div className="placeholder-title">Script View</div>
                  <p>Lox script editing with events panel.</p>
                </div>
              </div>
            </div>
          </div>
        </main>

        <aside className="inspector">
          <div className="panel-header">
            <h2>Inspector</h2>
            <button className="ghost-button" type="button">
              Pin
            </button>
          </div>
          <div className="inspector__section">
            <div className="inspector__label">Selected Room</div>
            <div className="inspector__value">3001 - Hall of the Wind</div>
          </div>
          <div className="inspector__section">
            <div className="inspector__label">Flags</div>
            <div className="inspector__tags">
              <span>indoors</span>
              <span>safe</span>
              <span>no_mob</span>
            </div>
          </div>
          <div className="inspector__section">
            <div className="inspector__label">Exits</div>
            <div className="inspector__value">north, east, down</div>
          </div>
          <div className="inspector__section">
            <div className="inspector__label">Validation</div>
            <div className="inspector__value inspector__value--warn">
              2 warnings, 0 errors
            </div>
          </div>
        </aside>
      </section>

      <footer className="statusbar">
        <span>Schema: area-json-schema.md</span>
        <span>Repository: LocalFileRepository</span>
        <span>Autosave: On</span>
      </footer>
    </div>
  );
}
