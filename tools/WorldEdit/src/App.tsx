import { useMemo, useState } from "react";
import { LocalFileRepository } from "./repository/localFileRepository";
import type { AreaJson } from "./repository/types";

const tabs = [
  {
    id: "Table",
    title: "Table View",
    description: "Filterable grid for Rooms, Mobiles, Objects, Resets."
  },
  {
    id: "Form",
    title: "Form View",
    description: "Schema-driven editor with validation and pickers."
  },
  {
    id: "Map",
    title: "Map View",
    description: "Orthogonal room layout with exit routing."
  },
  {
    id: "Script",
    title: "Script View",
    description: "Lox script editing with events panel."
  }
] as const;

const entityItems = [
  { key: "Rooms", count: 42 },
  { key: "Mobiles", count: 18 },
  { key: "Objects", count: 26 },
  { key: "Resets", count: 31 },
  { key: "Shops", count: 2 },
  { key: "Quests", count: 1 },
  { key: "Factions", count: 3 },
  { key: "Helps", count: 9 }
] as const;

type TabId = (typeof tabs)[number]["id"];
type EntityKey = (typeof entityItems)[number]["key"];

const entityDetails: Record<
  EntityKey,
  {
    kindLabel: string;
    selectionLabel: string;
    vnumRange: string;
    lastSave: string;
    flags: string[];
    exits: string;
    validation: string;
  }
> = {
  Rooms: {
    kindLabel: "Room",
    selectionLabel: "3001 - Hall of the Wind",
    vnumRange: "3000-3099",
    lastSave: "2 min ago",
    flags: ["indoors", "safe", "no_mob"],
    exits: "north, east, down",
    validation: "2 warnings, 0 errors"
  },
  Mobiles: {
    kindLabel: "Mobile",
    selectionLabel: "3005 - Alia the Warden",
    vnumRange: "3100-3199",
    lastSave: "8 min ago",
    flags: ["sentinel", "train", "practice"],
    exits: "n/a",
    validation: "0 warnings, 0 errors"
  },
  Objects: {
    kindLabel: "Object",
    selectionLabel: "3021 - Obsidian Lantern",
    vnumRange: "3200-3299",
    lastSave: "5 min ago",
    flags: ["take", "light", "glow"],
    exits: "n/a",
    validation: "1 warning, 0 errors"
  },
  Resets: {
    kindLabel: "Reset",
    selectionLabel: "R: 3001 -> 3021",
    vnumRange: "n/a",
    lastSave: "12 min ago",
    flags: ["room", "object"],
    exits: "n/a",
    validation: "0 warnings, 1 error"
  },
  Shops: {
    kindLabel: "Shop",
    selectionLabel: "Keeper 3012",
    vnumRange: "n/a",
    lastSave: "1 hour ago",
    flags: ["weapons", "armor"],
    exits: "n/a",
    validation: "0 warnings, 0 errors"
  },
  Quests: {
    kindLabel: "Quest",
    selectionLabel: "Quest 1 - Fog of Glass",
    vnumRange: "n/a",
    lastSave: "3 hours ago",
    flags: ["deliver", "reward"],
    exits: "n/a",
    validation: "0 warnings, 0 errors"
  },
  Factions: {
    kindLabel: "Faction",
    selectionLabel: "Faction 12 - Seaborn",
    vnumRange: "n/a",
    lastSave: "Yesterday",
    flags: ["ally: wind", "opposed: ember"],
    exits: "n/a",
    validation: "1 warning, 0 errors"
  },
  Helps: {
    kindLabel: "Help",
    selectionLabel: "help: sanctuary",
    vnumRange: "n/a",
    lastSave: "4 days ago",
    flags: ["level 5", "keywords 3"],
    exits: "n/a",
    validation: "0 warnings, 0 errors"
  }
};

function fileNameFromPath(path: string | null): string {
  if (!path) {
    return "No area loaded";
  }
  const normalized = path.replace(/[/\\\\]+/g, "/");
  return normalized.split("/").pop() ?? path;
}

export default function App() {
  const repository = useMemo(() => new LocalFileRepository(), []);
  const [activeTab, setActiveTab] = useState<TabId>(tabs[0].id);
  const [selectedEntity, setSelectedEntity] = useState<EntityKey>(
    entityItems[0].key
  );
  const [areaPath, setAreaPath] = useState<string | null>(null);
  const [areaData, setAreaData] = useState<AreaJson | null>(null);
  const [statusMessage, setStatusMessage] = useState("No area loaded");
  const [errorMessage, setErrorMessage] = useState<string | null>(null);
  const [isBusy, setIsBusy] = useState(false);
  const [areaDirectory, setAreaDirectory] = useState<string | null>(() =>
    localStorage.getItem("worldedit.areaDir")
  );

  const selection = entityDetails[selectedEntity];
  const areaName = fileNameFromPath(areaPath);

  const handleOpenArea = async () => {
    setErrorMessage(null);
    setIsBusy(true);
    try {
      const path = await repository.pickAreaFile(areaDirectory);
      if (!path) {
        return;
      }
      const loaded = await repository.loadArea(path);
      setAreaPath(path);
      setAreaData(loaded);
      setStatusMessage(`Loaded ${fileNameFromPath(path)}`);
    } catch (error) {
      setErrorMessage(`Failed to load area JSON. ${String(error)}`);
    } finally {
      setIsBusy(false);
    }
  };

  const saveToPath = async (path: string) => {
    if (!areaData) {
      return;
    }
    await repository.saveArea(path, areaData);
    setStatusMessage(`Saved ${fileNameFromPath(path)}`);
  };

  const handleSaveArea = async () => {
    if (!areaData) {
      setErrorMessage("No area loaded to save.");
      return;
    }
    setErrorMessage(null);
    setIsBusy(true);
    try {
      let targetPath = areaPath;
      if (!targetPath) {
        targetPath = await repository.pickSaveFile(areaDirectory);
      }
      if (!targetPath) {
        return;
      }
      setAreaPath(targetPath);
      await saveToPath(targetPath);
    } catch (error) {
      setErrorMessage(`Failed to save area JSON. ${String(error)}`);
    } finally {
      setIsBusy(false);
    }
  };

  const handleSaveAreaAs = async () => {
    if (!areaData) {
      setErrorMessage("No area loaded to save.");
      return;
    }
    setErrorMessage(null);
    setIsBusy(true);
    try {
      const targetPath = await repository.pickSaveFile(areaPath ?? areaDirectory);
      if (!targetPath) {
        return;
      }
      setAreaPath(targetPath);
      await saveToPath(targetPath);
    } catch (error) {
      setErrorMessage(`Failed to save area JSON. ${String(error)}`);
    } finally {
      setIsBusy(false);
    }
  };

  const handleSetAreaDirectory = async () => {
    setErrorMessage(null);
    setIsBusy(true);
    try {
      const selected = await repository.pickAreaDirectory(areaDirectory);
      if (!selected) {
        return;
      }
      setAreaDirectory(selected);
      localStorage.setItem("worldedit.areaDir", selected);
      setStatusMessage(`Area directory set to ${selected}`);
    } catch (error) {
      setErrorMessage(`Failed to set area directory. ${String(error)}`);
    } finally {
      setIsBusy(false);
    }
  };

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
            onClick={handleOpenArea}
            disabled={isBusy}
          >
            Open Area
          </button>
          <button
            className="action-button"
            type="button"
            onClick={handleSetAreaDirectory}
            disabled={isBusy}
          >
            Set Area Dir
          </button>
          <button
            className="action-button action-button--primary"
            type="button"
            onClick={handleSaveArea}
            disabled={!areaData || isBusy}
          >
            Save
          </button>
          <button
            className="action-button"
            type="button"
            onClick={handleSaveAreaAs}
            disabled={!areaData || isBusy}
          >
            Save As
          </button>
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
            <div className="tree-group">
              <div className="tree-group__label">Area</div>
              <ul className="tree-list">
                {entityItems.map((child) => (
                  <li key={child.key}>
                    <button
                      className={`tree-item${
                        child.key === selectedEntity
                          ? " tree-item--active"
                          : ""
                      }`}
                      type="button"
                      aria-pressed={child.key === selectedEntity}
                      onClick={() => setSelectedEntity(child.key)}
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

        <main className="center">
          <nav className="tabs" aria-label="View tabs">
            {tabs.map((tab) => (
              <button
                key={tab.id}
                type="button"
                className={`tab${tab.id === activeTab ? " tab--active" : ""}`}
                aria-pressed={tab.id === activeTab}
                onClick={() => setActiveTab(tab.id)}
              >
                {tab.id}
              </button>
            ))}
          </nav>

          <div className="view-card">
            <div className="view-card__header">
              <h2>{selectedEntity}</h2>
              <div className="view-card__meta">
                <span>VNUM range {selection.vnumRange}</span>
                <span>Last save {selection.lastSave}</span>
                <span>Active view {activeTab}</span>
              </div>
            </div>
            <div className="view-card__body">
              <div className="placeholder-grid">
                {tabs.map((tab) => (
                  <div
                    className={`placeholder-block${
                      tab.id === activeTab ? " placeholder-block--active" : ""
                    }`}
                    key={tab.id}
                  >
                    <div className="placeholder-title">{tab.title}</div>
                    <p>{tab.description}</p>
                  </div>
                ))}
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
            <div className="inspector__label">Selected {selection.kindLabel}</div>
            <div className="inspector__value">{selection.selectionLabel}</div>
          </div>
          <div className="inspector__section">
            <div className="inspector__label">Flags</div>
            <div className="inspector__tags">
              {selection.flags.map((flag) => (
                <span key={flag}>{flag}</span>
              ))}
            </div>
          </div>
          <div className="inspector__section">
            <div className="inspector__label">Exits</div>
            <div className="inspector__value">{selection.exits}</div>
          </div>
          <div className="inspector__section">
            <div className="inspector__label">Validation</div>
            <div className="inspector__value inspector__value--warn">
              {selection.validation}
            </div>
          </div>
        </aside>
      </section>

      <footer className="statusbar">
        <span>Status: {errorMessage ?? statusMessage}</span>
        <span>Area file: {areaPath ?? "Not loaded"}</span>
        <span>Area dir: {areaDirectory ?? "Not set"}</span>
        <span>Selection: {selectedEntity}</span>
      </footer>
    </div>
  );
}
