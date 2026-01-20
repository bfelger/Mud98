import type { ReactNode } from "react";
import { PlaceholderGrid } from "./PlaceholderGrid";

type ViewBodyProps = {
  activeTab: string;
  editorMode: "Area" | "Global";
  selectedEntity: string;
  selectedGlobalEntity: string;
  tabs: Array<{ id: string; title: string; description: string }>;
  hasAreaData: boolean;
  classCount: number;
  raceCount: number;
  skillCount: number;
  groupCount: number;
  commandCount: number;
  roomCount: number;
  mobileCount: number;
  objectCount: number;
  resetCount: number;
  shopCount: number;
  questCount: number;
  factionCount: number;
  mapHasRooms: boolean;
  areaForm: ReactNode;
  roomForm: ReactNode;
  mobileForm: ReactNode;
  objectForm: ReactNode;
  resetForm: ReactNode;
  shopForm: ReactNode;
  questForm: ReactNode;
  factionForm: ReactNode;
  classForm: ReactNode;
  raceForm: ReactNode;
  skillForm: ReactNode;
  groupForm: ReactNode;
  commandForm: ReactNode;
  tableView: ReactNode;
  classTableView: ReactNode;
  raceTableView: ReactNode;
  skillTableView: ReactNode;
  groupTableView: ReactNode;
  commandTableView: ReactNode;
  mapView: ReactNode;
  worldView: ReactNode;
  scriptView: ReactNode;
};

export function ViewBody({
  activeTab,
  editorMode,
  selectedEntity,
  selectedGlobalEntity,
  tabs,
  hasAreaData,
  classCount,
  raceCount,
  skillCount,
  groupCount,
  commandCount,
  roomCount,
  mobileCount,
  objectCount,
  resetCount,
  shopCount,
  questCount,
  factionCount,
  mapHasRooms,
  areaForm,
  roomForm,
  mobileForm,
  objectForm,
  resetForm,
  shopForm,
  questForm,
  factionForm,
  classForm,
  raceForm,
  skillForm,
  groupForm,
  commandForm,
  tableView,
  classTableView,
  raceTableView,
  skillTableView,
  groupTableView,
  commandTableView,
  mapView,
  worldView,
  scriptView
}: ViewBodyProps) {
  if (editorMode === "Global") {
    const isSupportedTab = activeTab === "Table" || activeTab === "Form";
    const label = selectedGlobalEntity || "Global data";
    if (selectedGlobalEntity === "Classes") {
      if (activeTab === "Form") {
        return classCount ? (
          <>{classForm}</>
        ) : (
          <div className="entity-table__empty">
            <h3>No classes loaded</h3>
            <p>Load the classes file from the data directory.</p>
          </div>
        );
      }
      if (activeTab === "Table") {
        return <>{classTableView}</>;
      }
    }
    if (selectedGlobalEntity === "Races") {
      if (activeTab === "Form") {
        return raceCount ? (
          <>{raceForm}</>
        ) : (
          <div className="entity-table__empty">
            <h3>No races loaded</h3>
            <p>Load the races file from the data directory.</p>
          </div>
        );
      }
      if (activeTab === "Table") {
        return <>{raceTableView}</>;
      }
    }
    if (selectedGlobalEntity === "Skills") {
      if (activeTab === "Form") {
        return skillCount ? (
          <>{skillForm}</>
        ) : (
          <div className="entity-table__empty">
            <h3>No skills loaded</h3>
            <p>Load the skills file from the data directory.</p>
          </div>
        );
      }
      if (activeTab === "Table") {
        return <>{skillTableView}</>;
      }
    }
    if (selectedGlobalEntity === "Groups") {
      if (activeTab === "Form") {
        return groupCount ? (
          <>{groupForm}</>
        ) : (
          <div className="entity-table__empty">
            <h3>No groups loaded</h3>
            <p>Load the groups file from the data directory.</p>
          </div>
        );
      }
      if (activeTab === "Table") {
        return <>{groupTableView}</>;
      }
    }
    if (selectedGlobalEntity === "Commands") {
      if (activeTab === "Form") {
        return commandCount ? (
          <>{commandForm}</>
        ) : (
          <div className="entity-table__empty">
            <h3>No commands loaded</h3>
            <p>Load the commands file from the data directory.</p>
          </div>
        );
      }
      if (activeTab === "Table") {
        return <>{commandTableView}</>;
      }
    }
    return (
      <div className="entity-table__empty">
        <h3>
          {isSupportedTab
            ? `${label} editor coming soon`
            : `${label} not available in ${activeTab} view`}
        </h3>
        <p>
          {isSupportedTab
            ? "Global data editors will live here once Issue F4 lands."
            : "Switch to the Form or Table tab to prepare global data editors."}
        </p>
      </div>
    );
  }
  if (activeTab === "Form" && selectedEntity === "Area") {
    return hasAreaData ? (
      <>{areaForm}</>
    ) : (
      <div className="entity-table__empty">
        <h3>No area loaded</h3>
        <p>Load an area JSON file to edit area data.</p>
      </div>
    );
  }

  if (activeTab === "Form" && selectedEntity === "Rooms") {
    return roomCount ? (
      <>{roomForm}</>
    ) : (
      <div className="entity-table__empty">
        <h3>No rooms available</h3>
        <p>Load an area JSON file to edit room data.</p>
      </div>
    );
  }

  if (activeTab === "Form" && selectedEntity === "Mobiles") {
    return mobileCount ? (
      <>{mobileForm}</>
    ) : (
      <div className="entity-table__empty">
        <h3>No mobiles available</h3>
        <p>Load an area JSON file to edit mobile data.</p>
      </div>
    );
  }

  if (activeTab === "Form" && selectedEntity === "Objects") {
    return objectCount ? (
      <>{objectForm}</>
    ) : (
      <div className="entity-table__empty">
        <h3>No objects available</h3>
        <p>Load an area JSON file to edit object data.</p>
      </div>
    );
  }

  if (activeTab === "Form" && selectedEntity === "Resets") {
    return resetCount ? (
      <>{resetForm}</>
    ) : (
      <div className="entity-table__empty">
        <h3>No resets available</h3>
        <p>Load an area JSON file to edit reset data.</p>
      </div>
    );
  }

  if (activeTab === "Form" && selectedEntity === "Shops") {
    return shopCount ? (
      <>{shopForm}</>
    ) : (
      <div className="entity-table__empty">
        <h3>No shops available</h3>
        <p>Load an area JSON file to edit shop data.</p>
      </div>
    );
  }

  if (activeTab === "Form" && selectedEntity === "Quests") {
    return questCount ? (
      <>{questForm}</>
    ) : (
      <div className="entity-table__empty">
        <h3>No quests available</h3>
        <p>Load an area JSON file to edit quest data.</p>
      </div>
    );
  }

  if (activeTab === "Form" && selectedEntity === "Factions") {
    return factionCount ? (
      <>{factionForm}</>
    ) : (
      <div className="entity-table__empty">
        <h3>No factions available</h3>
        <p>Load an area JSON file to edit faction data.</p>
      </div>
    );
  }

  if (activeTab === "Map") {
    return mapHasRooms ? (
      <>{mapView}</>
    ) : (
      <div className="entity-table__empty">
        <h3>No rooms available</h3>
        <p>Load an area JSON file to view the room map.</p>
      </div>
    );
  }

  if (activeTab === "World") {
    return <>{worldView}</>;
  }

  if (activeTab === "Script") {
    return <>{scriptView}</>;
  }

  const isTableEntity =
    selectedEntity === "Rooms" ||
    selectedEntity === "Mobiles" ||
    selectedEntity === "Objects" ||
    selectedEntity === "Resets" ||
    selectedEntity === "Shops" ||
    selectedEntity === "Quests" ||
    selectedEntity === "Factions";

  if (activeTab === "Table" && isTableEntity) {
    return <>{tableView}</>;
  }

  return <PlaceholderGrid tabs={tabs} activeTab={activeTab} />;
}
