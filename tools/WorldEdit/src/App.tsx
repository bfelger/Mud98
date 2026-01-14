import { useEffect, useMemo, useState } from "react";
import type { ColDef } from "ag-grid-community";
import { AgGridReact } from "ag-grid-react";
import { LocalFileRepository } from "./repository/localFileRepository";
import type { AreaJson, EditorMeta, ReferenceData } from "./repository/types";

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

const entityOrder = [
  "Rooms",
  "Mobiles",
  "Objects",
  "Resets",
  "Shops",
  "Quests",
  "Factions",
  "Helps"
] as const;

type TabId = (typeof tabs)[number]["id"];
type EntityKey = (typeof entityOrder)[number];

type RoomRow = {
  vnum: number;
  name: string;
  sector: string;
  exits: number;
  flags: string;
};

const entityKindLabels: Record<EntityKey, string> = {
  Rooms: "Room",
  Mobiles: "Mobile",
  Objects: "Object",
  Resets: "Reset",
  Shops: "Shop",
  Quests: "Quest",
  Factions: "Faction",
  Helps: "Help"
};

const entityDataKeys: Record<EntityKey, string> = {
  Rooms: "rooms",
  Mobiles: "mobiles",
  Objects: "objects",
  Resets: "resets",
  Shops: "shops",
  Quests: "quests",
  Factions: "factions",
  Helps: "helps"
};

function fileNameFromPath(path: string | null): string {
  if (!path) {
    return "No area loaded";
  }
  const normalized = path.replace(/[/\\\\]+/g, "/");
  return normalized.split("/").pop() ?? path;
}

function buildEditorMeta(
  existing: EditorMeta | null,
  activeTab: string,
  selectedEntity: string
): EditorMeta {
  const now = new Date().toISOString();
  const base: EditorMeta = existing ?? {
    version: 1,
    updatedAt: now,
    view: { activeTab },
    selection: { entityType: selectedEntity },
    layout: {}
  };

  return {
    ...base,
    version: 1,
    updatedAt: now,
    view: {
      ...base.view,
      activeTab
    },
    selection: {
      ...base.selection,
      entityType: selectedEntity
    }
  };
}

function parseVnum(value: unknown): number | null {
  if (typeof value === "number") {
    return value;
  }
  if (typeof value === "string" && value.trim().length) {
    const parsed = Number(value);
    return Number.isFinite(parsed) ? parsed : null;
  }
  return null;
}

function findByVnum(list: unknown[], vnum: number): Record<string, unknown> | null {
  for (const entry of list) {
    if (!entry || typeof entry !== "object") {
      continue;
    }
    const record = entry as Record<string, unknown>;
    const entryVnum = parseVnum(record.vnum);
    if (entryVnum === vnum) {
      return record;
    }
  }
  return null;
}

function getEntityList(areaData: AreaJson | null, key: EntityKey): unknown[] {
  if (!areaData) {
    return [];
  }
  const list = (areaData as Record<string, unknown>)[entityDataKeys[key]];
  return Array.isArray(list) ? list : [];
}

function getAreaVnumRange(areaData: AreaJson | null): string | null {
  if (!areaData) {
    return null;
  }
  const areadata = (areaData as Record<string, unknown>).areadata;
  if (!areadata || typeof areadata !== "object") {
    return null;
  }
  const vnumRange = (areadata as Record<string, unknown>).vnumRange;
  if (
    Array.isArray(vnumRange) &&
    vnumRange.length === 2 &&
    typeof vnumRange[0] === "number" &&
    typeof vnumRange[1] === "number"
  ) {
    return `${vnumRange[0]}-${vnumRange[1]}`;
  }
  return null;
}

function getFirstString(value: unknown, fallback: string): string {
  return typeof value === "string" && value.trim().length ? value : fallback;
}

function buildSelectionSummary(
  selectedEntity: EntityKey,
  areaData: AreaJson | null,
  selectedRoomVnum: number | null
) {
  const list = getEntityList(areaData, selectedEntity);
  const count = list.length;
  const selected =
    selectedEntity === "Rooms" && selectedRoomVnum !== null
      ? findByVnum(list, selectedRoomVnum)
      : null;
  const first = (selected ?? list[0] ?? {}) as Record<string, unknown>;
  const vnumRange = getAreaVnumRange(areaData);
  const vnum = parseVnum(first.vnum);
  const emptyLabel = `No ${selectedEntity.toLowerCase()} loaded`;

  let selectionLabel = emptyLabel;
  let flags: string[] = [];
  let exits = "n/a";

  switch (selectedEntity) {
    case "Rooms": {
      selectionLabel =
        vnum !== null
          ? `${vnum} - ${getFirstString(first.name, "(unnamed room)")}`
          : emptyLabel;
      flags = Array.isArray(first.roomFlags)
        ? first.roomFlags.filter((flag) => typeof flag === "string")
        : [];
      const exitList = Array.isArray(first.exits) ? first.exits : [];
      exits =
        exitList.length > 0
          ? exitList
              .map((exit) =>
                typeof exit === "object" && exit
                  ? (exit as Record<string, unknown>).dir
                  : null
              )
              .filter((dir): dir is string => typeof dir === "string")
              .join(", ")
          : "none";
      break;
    }
    case "Mobiles": {
      selectionLabel =
        vnum !== null
          ? `${vnum} - ${getFirstString(first.shortDescr, "(unnamed mobile)")}`
          : emptyLabel;
      flags = Array.isArray(first.actFlags)
        ? first.actFlags.filter((flag) => typeof flag === "string")
        : [];
      break;
    }
    case "Objects": {
      selectionLabel =
        vnum !== null
          ? `${vnum} - ${getFirstString(first.shortDescr, "(unnamed object)")}`
          : emptyLabel;
      flags = Array.isArray(first.wearFlags)
        ? first.wearFlags.filter((flag) => typeof flag === "string")
        : [];
      break;
    }
    case "Resets": {
      const commandName = getFirstString(first.commandName, "reset");
      const roomVnum = parseVnum(first.roomVnum);
      selectionLabel =
        roomVnum !== null
          ? `${commandName} -> room ${roomVnum}`
          : count
            ? commandName
            : emptyLabel;
      break;
    }
    case "Shops": {
      const keeper = parseVnum(first.keeper);
      selectionLabel =
        keeper !== null ? `Keeper ${keeper}` : count ? "Shop entry" : emptyLabel;
      break;
    }
    case "Quests": {
      selectionLabel =
        vnum !== null
          ? `${vnum} - ${getFirstString(first.name, "Quest")}`
          : count
            ? "Quest entry"
            : emptyLabel;
      break;
    }
    case "Factions": {
      selectionLabel =
        vnum !== null
          ? `${vnum} - ${getFirstString(first.name, "Faction")}`
          : count
            ? "Faction entry"
            : emptyLabel;
      break;
    }
    case "Helps": {
      const keyword = getFirstString(first.keyword, "help");
      selectionLabel = count ? `help: ${keyword}` : emptyLabel;
      break;
    }
    default:
      break;
  }

  const range =
    selectedEntity === "Rooms" ||
    selectedEntity === "Mobiles" ||
    selectedEntity === "Objects"
      ? vnumRange ?? "n/a"
      : "n/a";

  return {
    kindLabel: entityKindLabels[selectedEntity],
    selectionLabel,
    vnumRange: range,
    lastSave: areaData ? "loaded" : "n/a",
    flags,
    exits,
    validation: "validation not run"
  };
}

function buildAreaDebugSummary(areaData: AreaJson | null): {
  keys: string[];
  arrayCounts: Array<{ key: string; count: number }>;
} {
  if (!areaData || typeof areaData !== "object") {
    return { keys: [], arrayCounts: [] };
  }
  const keys = Object.keys(areaData as Record<string, unknown>).sort();
  const arrayCounts = keys
    .map((key) => {
      const value = (areaData as Record<string, unknown>)[key];
      return Array.isArray(value) ? { key, count: value.length } : null;
    })
    .filter((entry): entry is { key: string; count: number } => entry !== null);
  return { keys, arrayCounts };
}

function buildRoomRows(areaData: AreaJson | null): RoomRow[] {
  if (!areaData) {
    return [];
  }
  const rooms = (areaData as { rooms?: unknown }).rooms;
  if (!Array.isArray(rooms)) {
    return [];
  }
  return rooms.map((room) => {
    const data = room as Record<string, unknown>;
    const vnum =
      typeof data.vnum === "number"
        ? data.vnum
        : Number.isFinite(Number(data.vnum))
          ? Number(data.vnum)
          : -1;
    const name = typeof data.name === "string" ? data.name : "(unnamed room)";
    const sectorType =
      typeof data.sectorType === "string"
        ? data.sectorType
        : typeof data.sector === "string"
          ? data.sector
          : "inside";
    const roomFlags = Array.isArray(data.roomFlags)
      ? data.roomFlags.filter((flag) => typeof flag === "string")
      : [];
    const exits = Array.isArray(data.exits) ? data.exits.length : 0;
    return {
      vnum,
      name,
      sector: sectorType,
      exits,
      flags: roomFlags.length ? roomFlags.join(", ") : "—"
    };
  });
}

export default function App() {
  const repository = useMemo(() => new LocalFileRepository(), []);
  const [activeTab, setActiveTab] = useState<TabId>(tabs[0].id);
  const [selectedEntity, setSelectedEntity] = useState<EntityKey>(
    entityOrder[0]
  );
  const [selectedRoomVnum, setSelectedRoomVnum] = useState<number | null>(null);
  const [areaPath, setAreaPath] = useState<string | null>(null);
  const [areaData, setAreaData] = useState<AreaJson | null>(null);
  const [editorMeta, setEditorMeta] = useState<EditorMeta | null>(null);
  const [editorMetaPath, setEditorMetaPath] = useState<string | null>(null);
  const [referenceData, setReferenceData] = useState<ReferenceData | null>(null);
  const [dataDirectory, setDataDirectory] = useState<string | null>(null);
  const [statusMessage, setStatusMessage] = useState("No area loaded");
  const [errorMessage, setErrorMessage] = useState<string | null>(null);
  const [isBusy, setIsBusy] = useState(false);
  const [areaDirectory, setAreaDirectory] = useState<string | null>(() =>
    localStorage.getItem("worldedit.areaDir")
  );

  useEffect(() => {
    setSelectedRoomVnum(null);
  }, [areaData]);

  const selection = useMemo(
    () => buildSelectionSummary(selectedEntity, areaData, selectedRoomVnum),
    [areaData, selectedEntity, selectedRoomVnum]
  );
  const areaName = fileNameFromPath(areaPath);
  const areaDebug = useMemo(() => buildAreaDebugSummary(areaData), [areaData]);
  const entityItems = useMemo(
    () =>
      entityOrder.map((key) => ({
        key,
        count: getEntityList(areaData, key).length
      })),
    [areaData]
  );
  const roomRows = useMemo(() => buildRoomRows(areaData), [areaData]);
  const roomColumns = useMemo<ColDef<RoomRow>[]>(
    () => [
      { headerName: "VNUM", field: "vnum", width: 110, sort: "asc" },
      { headerName: "Name", field: "name", flex: 2, minWidth: 200 },
      { headerName: "Sector", field: "sector", flex: 1, minWidth: 140 },
      { headerName: "Exits", field: "exits", width: 110 },
      { headerName: "Room Flags", field: "flags", flex: 2, minWidth: 220 }
    ],
    []
  );
  const roomDefaultColDef = useMemo<ColDef>(
    () => ({
      sortable: true,
      resizable: true,
      filter: true
    }),
    []
  );

  const handleOpenArea = async () => {
    setErrorMessage(null);
    setIsBusy(true);
    try {
      const path = await repository.pickAreaFile(areaDirectory);
      if (!path) {
        return;
      }
      const loaded = await repository.loadArea(path);
      const metaPath = repository.editorMetaPathForArea(path);
      let loadedMeta: EditorMeta | null = null;
      setAreaPath(path);
      setAreaData(loaded);
      setEditorMetaPath(metaPath);
      try {
        loadedMeta = await repository.loadEditorMeta(metaPath);
      } catch (error) {
        setErrorMessage(`Failed to load editor meta. ${String(error)}`);
      }
      setEditorMeta(loadedMeta);
      setStatusMessage(
        loadedMeta
          ? `Loaded ${fileNameFromPath(path)} + editor meta`
          : `Loaded ${fileNameFromPath(path)}`
      );
      try {
        const resolvedDataDir = await repository.resolveDataDirectory(
          path,
          areaDirectory
        );
        if (resolvedDataDir) {
          const refs = await repository.loadReferenceData(resolvedDataDir);
          setReferenceData(refs);
          setDataDirectory(resolvedDataDir);
          setStatusMessage(
            `Loaded ${fileNameFromPath(path)} + reference data`
          );
        }
      } catch (error) {
        setErrorMessage(`Failed to load reference data. ${String(error)}`);
      }
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

  const saveEditorMetaToPath = async (path: string) => {
    const metaPath = repository.editorMetaPathForArea(path);
    const nextMeta = buildEditorMeta(editorMeta, activeTab, selectedEntity);
    await repository.saveEditorMeta(metaPath, nextMeta);
    setEditorMeta(nextMeta);
    setEditorMetaPath(metaPath);
    setStatusMessage(`Saved editor meta ${fileNameFromPath(metaPath)}`);
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
      setEditorMetaPath(repository.editorMetaPathForArea(targetPath));
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
      setEditorMetaPath(repository.editorMetaPathForArea(targetPath));
      await saveToPath(targetPath);
    } catch (error) {
      setErrorMessage(`Failed to save area JSON. ${String(error)}`);
    } finally {
      setIsBusy(false);
    }
  };

  const handleSaveEditorMeta = async () => {
    if (!areaPath) {
      setErrorMessage("No area loaded to save editor metadata.");
      return;
    }
    setErrorMessage(null);
    setIsBusy(true);
    try {
      await saveEditorMetaToPath(areaPath);
    } catch (error) {
      setErrorMessage(`Failed to save editor meta. ${String(error)}`);
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

  const handleLoadReferenceData = async () => {
    setErrorMessage(null);
    setIsBusy(true);
    try {
      const resolvedDataDir = await repository.resolveDataDirectory(
        areaPath,
        areaDirectory
      );
      if (!resolvedDataDir) {
        setErrorMessage("Unable to resolve data directory.");
        return;
      }
      const refs = await repository.loadReferenceData(resolvedDataDir);
      setReferenceData(refs);
      setDataDirectory(resolvedDataDir);
      setStatusMessage(`Loaded reference data from ${resolvedDataDir}`);
    } catch (error) {
      setErrorMessage(`Failed to load reference data. ${String(error)}`);
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
            className="action-button"
            type="button"
            onClick={handleLoadReferenceData}
            disabled={isBusy}
          >
            Load Ref Data
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
            onClick={handleSaveEditorMeta}
            disabled={!areaPath || isBusy}
          >
            Save Meta
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
              {activeTab === "Table" && selectedEntity === "Rooms" ? (
                <div className="room-table">
                  {roomRows.length ? (
                    <div className="ag-theme-quartz worldedit-grid">
                      <AgGridReact<RoomRow>
                        rowData={roomRows}
                        columnDefs={roomColumns}
                        defaultColDef={roomDefaultColDef}
                        animateRows
                        rowSelection="single"
                        getRowId={(params) => String(params.data.vnum)}
                        domLayout="autoHeight"
                        onRowClicked={(event) =>
                          setSelectedRoomVnum(event.data?.vnum ?? null)
                        }
                      />
                    </div>
                  ) : (
                    <div className="room-table__empty">
                      <h3>Rooms will appear here</h3>
                      <p>Load an area JSON file to populate the room table.</p>
                    </div>
                  )}
                </div>
              ) : (
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
              )}
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
              {selection.validation}
            </div>
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
      </section>

      <footer className="statusbar">
        <span>Status: {errorMessage ?? statusMessage}</span>
        <span>Area file: {areaPath ?? "Not loaded"}</span>
        <span>Meta file: {editorMetaPath ?? "Not loaded"}</span>
        <span>Area dir: {areaDirectory ?? "Not set"}</span>
        <span>Data dir: {dataDirectory ?? "Not set"}</span>
        <span>Selection: {selectedEntity}</span>
        <span>
          Reference:{" "}
          {referenceData
            ? `classes ${referenceData.classes.length}, races ${referenceData.races.length}, skills ${referenceData.skills.length}, commands ${referenceData.commands.length}`
            : "not loaded"}
        </span>
      </footer>
    </div>
  );
}
