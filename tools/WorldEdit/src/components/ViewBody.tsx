import type { ReactNode } from "react";
import { PlaceholderGrid } from "./PlaceholderGrid";

type ViewBodyProps = {
  activeTab: string;
  selectedEntity: string;
  tabs: Array<{ id: string; title: string; description: string }>;
  hasAreaData: boolean;
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
  tableView: ReactNode;
  mapView: ReactNode;
  worldView: ReactNode;
  scriptView: ReactNode;
};

export function ViewBody({
  activeTab,
  selectedEntity,
  tabs,
  hasAreaData,
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
  tableView,
  mapView,
  worldView,
  scriptView
}: ViewBodyProps) {
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
