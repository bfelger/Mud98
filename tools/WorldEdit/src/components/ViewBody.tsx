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
  socialCount: number;
  tutorialCount: number;
  lootCount: number;
  classActions: ReactNode;
  raceActions: ReactNode;
  skillActions: ReactNode;
  groupActions: ReactNode;
  commandActions: ReactNode;
  socialActions: ReactNode;
  tutorialActions: ReactNode;
  lootActions: ReactNode;
  areaLootCount: number;
  recipeCount: number;
  gatherSpawnCount: number;
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
  socialForm: ReactNode;
  tutorialForm: ReactNode;
  lootForm: ReactNode;
  areaLootForm: ReactNode;
  recipeForm: ReactNode;
  gatherSpawnForm: ReactNode;
  tableView: ReactNode;
  classTableView: ReactNode;
  raceTableView: ReactNode;
  skillTableView: ReactNode;
  groupTableView: ReactNode;
  commandTableView: ReactNode;
  socialTableView: ReactNode;
  tutorialTableView: ReactNode;
  lootTableView: ReactNode;
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
  socialCount,
  tutorialCount,
  lootCount,
  classActions,
  raceActions,
  skillActions,
  groupActions,
  commandActions,
  socialActions,
  tutorialActions,
  lootActions,
  areaLootCount,
  recipeCount,
  gatherSpawnCount,
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
  socialForm,
  tutorialForm,
  lootForm,
  areaLootForm,
  recipeForm,
  gatherSpawnForm,
  tableView,
  classTableView,
  raceTableView,
  skillTableView,
  groupTableView,
  commandTableView,
  socialTableView,
  tutorialTableView,
  lootTableView,
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
          <>
            {classActions}
            {classForm}
          </>
        ) : (
          <>
            {classActions}
            <div className="entity-table__empty">
              <h3>No classes loaded</h3>
              <p>Load the classes file from the data directory.</p>
            </div>
          </>
        );
      }
      if (activeTab === "Table") {
        return <>{classTableView}</>;
      }
    }
    if (selectedGlobalEntity === "Races") {
      if (activeTab === "Form") {
        return raceCount ? (
          <>
            {raceActions}
            {raceForm}
          </>
        ) : (
          <>
            {raceActions}
            <div className="entity-table__empty">
              <h3>No races loaded</h3>
              <p>Load the races file from the data directory.</p>
            </div>
          </>
        );
      }
      if (activeTab === "Table") {
        return <>{raceTableView}</>;
      }
    }
    if (selectedGlobalEntity === "Skills") {
      if (activeTab === "Form") {
        return skillCount ? (
          <>
            {skillActions}
            {skillForm}
          </>
        ) : (
          <>
            {skillActions}
            <div className="entity-table__empty">
              <h3>No skills loaded</h3>
              <p>Load the skills file from the data directory.</p>
            </div>
          </>
        );
      }
      if (activeTab === "Table") {
        return <>{skillTableView}</>;
      }
    }
    if (selectedGlobalEntity === "Groups") {
      if (activeTab === "Form") {
        return groupCount ? (
          <>
            {groupActions}
            {groupForm}
          </>
        ) : (
          <>
            {groupActions}
            <div className="entity-table__empty">
              <h3>No groups loaded</h3>
              <p>Load the groups file from the data directory.</p>
            </div>
          </>
        );
      }
      if (activeTab === "Table") {
        return <>{groupTableView}</>;
      }
    }
    if (selectedGlobalEntity === "Commands") {
      if (activeTab === "Form") {
        return commandCount ? (
          <>
            {commandActions}
            {commandForm}
          </>
        ) : (
          <>
            {commandActions}
            <div className="entity-table__empty">
              <h3>No commands loaded</h3>
              <p>Load the commands file from the data directory.</p>
            </div>
          </>
        );
      }
      if (activeTab === "Table") {
        return <>{commandTableView}</>;
      }
    }
    if (selectedGlobalEntity === "Socials") {
      if (activeTab === "Form") {
        return socialCount ? (
          <>
            {socialActions}
            {socialForm}
          </>
        ) : (
          <>
            {socialActions}
            <div className="entity-table__empty">
              <h3>No socials loaded</h3>
              <p>Load the socials file from the data directory.</p>
            </div>
          </>
        );
      }
      if (activeTab === "Table") {
        return <>{socialTableView}</>;
      }
    }
    if (selectedGlobalEntity === "Tutorials") {
      if (activeTab === "Form") {
        return tutorialCount ? (
          <>
            {tutorialActions}
            {tutorialForm}
          </>
        ) : (
          <>
            {tutorialActions}
            <div className="entity-table__empty">
              <h3>No tutorials loaded</h3>
              <p>Load the tutorials file from the data directory.</p>
            </div>
          </>
        );
      }
      if (activeTab === "Table") {
        return <>{tutorialTableView}</>;
      }
    }
    if (selectedGlobalEntity === "Loot") {
      if (activeTab === "Form") {
        return lootCount ? (
          <>
            {lootActions}
            {lootForm}
          </>
        ) : (
          <>
            {lootActions}
            <div className="entity-table__empty">
              <h3>No loot loaded</h3>
              <p>Load the loot file from the data directory.</p>
            </div>
          </>
        );
      }
      if (activeTab === "Table") {
        return <>{lootTableView}</>;
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

  if (activeTab === "Form" && selectedEntity === "Loot") {
    return areaLootCount ? (
      <>{areaLootForm}</>
    ) : (
      <div className="entity-table__empty">
        <h3>No loot available</h3>
        <p>Load an area JSON file to edit loot groups and tables.</p>
      </div>
    );
  }

  if (activeTab === "Form" && selectedEntity === "Recipes") {
    return recipeCount ? (
      <>{recipeForm}</>
    ) : (
      <div className="entity-table__empty">
        <h3>No recipes available</h3>
        <p>Load an area JSON file to edit recipes.</p>
      </div>
    );
  }

  if (activeTab === "Form" && selectedEntity === "Gather Spawns") {
    return gatherSpawnCount ? (
      <>{gatherSpawnForm}</>
    ) : (
      <div className="entity-table__empty">
        <h3>No gather spawns available</h3>
        <p>Load an area JSON file to edit gather spawns.</p>
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
    selectedEntity === "Factions" ||
    selectedEntity === "Loot" ||
    selectedEntity === "Recipes" ||
    selectedEntity === "Gather Spawns";

  if (activeTab === "Table" && isTableEntity) {
    return <>{tableView}</>;
  }

  return <PlaceholderGrid tabs={tabs} activeTab={activeTab} />;
}
