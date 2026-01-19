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
  mapHasRooms: boolean;
  areaForm: ReactNode;
  roomForm: ReactNode;
  mobileForm: ReactNode;
  objectForm: ReactNode;
  resetForm: ReactNode;
  tableView: ReactNode;
  mapView: ReactNode;
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
  mapHasRooms,
  areaForm,
  roomForm,
  mobileForm,
  objectForm,
  resetForm,
  tableView,
  mapView,
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

  if (activeTab === "Script") {
    return <>{scriptView}</>;
  }

  const isTableEntity =
    selectedEntity === "Rooms" ||
    selectedEntity === "Mobiles" ||
    selectedEntity === "Objects" ||
    selectedEntity === "Resets";

  if (activeTab === "Table" && isTableEntity) {
    return <>{tableView}</>;
  }

  return <PlaceholderGrid tabs={tabs} activeTab={activeTab} />;
}
