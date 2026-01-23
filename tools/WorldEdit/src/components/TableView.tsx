import type { MutableRefObject } from "react";
import type { ColDef, GridApi } from "ag-grid-community";
import { AgGridReact } from "ag-grid-react";
import {
  EntityTableToolbar,
  type EntityTableToolbarConfig
} from "./EntityTableToolbar";

type TableViewProps = {
  selectedEntity: string;
  roomRows: Array<Record<string, unknown>>;
  mobileRows: Array<Record<string, unknown>>;
  objectRows: Array<Record<string, unknown>>;
  resetRows: Array<Record<string, unknown>>;
  shopRows: Array<Record<string, unknown>>;
  questRows: Array<Record<string, unknown>>;
  factionRows: Array<Record<string, unknown>>;
  lootRows: Array<Record<string, unknown>>;
  recipeRows: Array<Record<string, unknown>>;
  gatherSpawnRows: Array<Record<string, unknown>>;
  roomColumns: ColDef[];
  mobileColumns: ColDef[];
  objectColumns: ColDef[];
  resetColumns: ColDef[];
  shopColumns: ColDef[];
  questColumns: ColDef[];
  factionColumns: ColDef[];
  lootColumns: ColDef[];
  recipeColumns: ColDef[];
  gatherSpawnColumns: ColDef[];
  roomDefaultColDef: ColDef;
  mobileDefaultColDef: ColDef;
  objectDefaultColDef: ColDef;
  resetDefaultColDef: ColDef;
  shopDefaultColDef: ColDef;
  questDefaultColDef: ColDef;
  factionDefaultColDef: ColDef;
  lootDefaultColDef: ColDef;
  recipeDefaultColDef: ColDef;
  gatherSpawnDefaultColDef: ColDef;
  toolbar?: EntityTableToolbarConfig | null;
  onSelectRoom: (vnum: number | null) => void;
  onSelectMobile: (vnum: number | null) => void;
  onSelectObject: (vnum: number | null) => void;
  onSelectReset: (index: number | null) => void;
  onSelectShop: (keeper: number | null) => void;
  onSelectQuest: (vnum: number | null) => void;
  onSelectFaction: (vnum: number | null) => void;
  onSelectLoot: (kind: "group" | "table" | null, index: number | null) => void;
  onSelectRecipe: (vnum: number | null) => void;
  onSelectGatherSpawn: (vnum: number | null) => void;
  roomGridApiRef: MutableRefObject<GridApi | null>;
  mobileGridApiRef: MutableRefObject<GridApi | null>;
  objectGridApiRef: MutableRefObject<GridApi | null>;
  resetGridApiRef: MutableRefObject<GridApi | null>;
  shopGridApiRef: MutableRefObject<GridApi | null>;
  questGridApiRef: MutableRefObject<GridApi | null>;
  factionGridApiRef: MutableRefObject<GridApi | null>;
  lootGridApiRef: MutableRefObject<GridApi | null>;
  recipeGridApiRef: MutableRefObject<GridApi | null>;
  gatherSpawnGridApiRef: MutableRefObject<GridApi | null>;
};

export function TableView({
  selectedEntity,
  roomRows,
  mobileRows,
  objectRows,
  resetRows,
  shopRows,
  questRows,
  factionRows,
  lootRows,
  recipeRows,
  gatherSpawnRows,
  roomColumns,
  mobileColumns,
  objectColumns,
  resetColumns,
  shopColumns,
  questColumns,
  factionColumns,
  lootColumns,
  recipeColumns,
  gatherSpawnColumns,
  roomDefaultColDef,
  mobileDefaultColDef,
  objectDefaultColDef,
  resetDefaultColDef,
  shopDefaultColDef,
  questDefaultColDef,
  factionDefaultColDef,
  lootDefaultColDef,
  recipeDefaultColDef,
  gatherSpawnDefaultColDef,
  toolbar,
  onSelectRoom,
  onSelectMobile,
  onSelectObject,
  onSelectReset,
  onSelectShop,
  onSelectQuest,
  onSelectFaction,
  onSelectLoot,
  onSelectRecipe,
  onSelectGatherSpawn,
  roomGridApiRef,
  mobileGridApiRef,
  objectGridApiRef,
  resetGridApiRef,
  shopGridApiRef,
  questGridApiRef,
  factionGridApiRef,
  lootGridApiRef,
  recipeGridApiRef,
  gatherSpawnGridApiRef
}: TableViewProps) {
  return (
    <div className="entity-table">
      {toolbar ? <EntityTableToolbar {...toolbar} /> : null}
      {selectedEntity === "Rooms" ? (
        roomRows.length ? (
          <div className="ag-theme-quartz worldedit-grid">
            <AgGridReact
              rowData={roomRows}
              columnDefs={roomColumns}
              defaultColDef={roomDefaultColDef}
              animateRows
              rowSelection="single"
              getRowId={(params) => String(params.data.vnum)}
              domLayout="autoHeight"
              onRowClicked={(event) =>
                onSelectRoom(event.data?.vnum ?? null)
              }
              onGridReady={(event) => {
                roomGridApiRef.current = event.api;
              }}
            />
          </div>
        ) : (
          <div className="entity-table__empty">
            <h3>Rooms will appear here</h3>
            <p>Load an area JSON file to populate the room table.</p>
          </div>
        )
      ) : null}
      {selectedEntity === "Mobiles" ? (
        mobileRows.length ? (
          <div className="ag-theme-quartz worldedit-grid">
            <AgGridReact
              rowData={mobileRows}
              columnDefs={mobileColumns}
              defaultColDef={mobileDefaultColDef}
              animateRows
              rowSelection="single"
              getRowId={(params) => String(params.data.vnum)}
              domLayout="autoHeight"
              onRowClicked={(event) =>
                onSelectMobile(event.data?.vnum ?? null)
              }
              onGridReady={(event) => {
                mobileGridApiRef.current = event.api;
              }}
            />
          </div>
        ) : (
          <div className="entity-table__empty">
            <h3>Mobiles will appear here</h3>
            <p>Load an area JSON file to populate the mobile table.</p>
          </div>
        )
      ) : null}
      {selectedEntity === "Objects" ? (
        objectRows.length ? (
          <div className="ag-theme-quartz worldedit-grid">
            <AgGridReact
              rowData={objectRows}
              columnDefs={objectColumns}
              defaultColDef={objectDefaultColDef}
              animateRows
              rowSelection="single"
              getRowId={(params) => String(params.data.vnum)}
              domLayout="autoHeight"
              onRowClicked={(event) =>
                onSelectObject(event.data?.vnum ?? null)
              }
              onGridReady={(event) => {
                objectGridApiRef.current = event.api;
              }}
            />
          </div>
        ) : (
          <div className="entity-table__empty">
            <h3>Objects will appear here</h3>
            <p>Load an area JSON file to populate the object table.</p>
          </div>
        )
      ) : null}
      {selectedEntity === "Resets" ? (
        resetRows.length ? (
          <div className="ag-theme-quartz worldedit-grid">
            <AgGridReact
              rowData={resetRows}
              columnDefs={resetColumns}
              defaultColDef={resetDefaultColDef}
              animateRows
              rowSelection="single"
              getRowId={(params) => String(params.data.index)}
              domLayout="autoHeight"
              onRowClicked={(event) =>
                onSelectReset(event.data?.index ?? null)
              }
              onGridReady={(event) => {
                resetGridApiRef.current = event.api;
              }}
            />
          </div>
        ) : (
          <div className="entity-table__empty">
            <h3>Resets will appear here</h3>
            <p>Load an area JSON file to populate the reset table.</p>
          </div>
        )
      ) : null}
      {selectedEntity === "Shops" ? (
        shopRows.length ? (
          <div className="ag-theme-quartz worldedit-grid">
            <AgGridReact
              rowData={shopRows}
              columnDefs={shopColumns}
              defaultColDef={shopDefaultColDef}
              animateRows
              rowSelection="single"
              getRowId={(params) => String(params.data.keeper)}
              domLayout="autoHeight"
              onRowClicked={(event) =>
                onSelectShop(event.data?.keeper ?? null)
              }
              onGridReady={(event) => {
                shopGridApiRef.current = event.api;
              }}
            />
          </div>
        ) : (
          <div className="entity-table__empty">
            <h3>Shops will appear here</h3>
            <p>Load an area JSON file to populate the shop table.</p>
          </div>
        )
      ) : null}
      {selectedEntity === "Quests" ? (
        questRows.length ? (
          <div className="ag-theme-quartz worldedit-grid">
            <AgGridReact
              rowData={questRows}
              columnDefs={questColumns}
              defaultColDef={questDefaultColDef}
              animateRows
              rowSelection="single"
              getRowId={(params) => String(params.data.vnum)}
              domLayout="autoHeight"
              onRowClicked={(event) =>
                onSelectQuest(event.data?.vnum ?? null)
              }
              onGridReady={(event) => {
                questGridApiRef.current = event.api;
              }}
            />
          </div>
        ) : (
          <div className="entity-table__empty">
            <h3>Quests will appear here</h3>
            <p>Load an area JSON file to populate the quest table.</p>
          </div>
        )
      ) : null}
      {selectedEntity === "Factions" ? (
        factionRows.length ? (
          <div className="ag-theme-quartz worldedit-grid">
            <AgGridReact
              rowData={factionRows}
              columnDefs={factionColumns}
              defaultColDef={factionDefaultColDef}
              animateRows
              rowSelection="single"
              getRowId={(params) => String(params.data.vnum)}
              domLayout="autoHeight"
              onRowClicked={(event) =>
                onSelectFaction(event.data?.vnum ?? null)
              }
              onGridReady={(event) => {
                factionGridApiRef.current = event.api;
              }}
            />
          </div>
        ) : (
          <div className="entity-table__empty">
            <h3>Factions will appear here</h3>
            <p>Load an area JSON file to populate the faction table.</p>
          </div>
        )
      ) : null}
      {selectedEntity === "Loot" ? (
        lootRows.length ? (
          <div className="ag-theme-quartz worldedit-grid">
            <AgGridReact
              rowData={lootRows}
              columnDefs={lootColumns}
              defaultColDef={lootDefaultColDef}
              animateRows
              rowSelection="single"
              getRowId={(params) => String(params.data.id)}
              domLayout="autoHeight"
              onRowClicked={(event) =>
                onSelectLoot(event.data?.kind ?? null, event.data?.index ?? null)
              }
              onGridReady={(event) => {
                lootGridApiRef.current = event.api;
              }}
            />
          </div>
        ) : (
          <div className="entity-table__empty">
            <h3>Loot entries will appear here</h3>
            <p>Load an area JSON file to populate the loot table.</p>
          </div>
        )
      ) : null}
      {selectedEntity === "Recipes" ? (
        recipeRows.length ? (
          <div className="ag-theme-quartz worldedit-grid">
            <AgGridReact
              rowData={recipeRows}
              columnDefs={recipeColumns}
              defaultColDef={recipeDefaultColDef}
              animateRows
              rowSelection="single"
              getRowId={(params) => String(params.data.vnum)}
              domLayout="autoHeight"
              onRowClicked={(event) =>
                onSelectRecipe(event.data?.vnum ?? null)
              }
              onGridReady={(event) => {
                recipeGridApiRef.current = event.api;
              }}
            />
          </div>
        ) : (
          <div className="entity-table__empty">
            <h3>Recipes will appear here</h3>
            <p>Load an area JSON file to populate the recipe table.</p>
          </div>
        )
      ) : null}
      {selectedEntity === "Gather Spawns" ? (
        gatherSpawnRows.length ? (
          <div className="ag-theme-quartz worldedit-grid">
            <AgGridReact
              rowData={gatherSpawnRows}
              columnDefs={gatherSpawnColumns}
              defaultColDef={gatherSpawnDefaultColDef}
              animateRows
              rowSelection="single"
              getRowId={(params) => String(params.data.vnum)}
              domLayout="autoHeight"
              onRowClicked={(event) =>
                onSelectGatherSpawn(event.data?.vnum ?? null)
              }
              onGridReady={(event) => {
                gatherSpawnGridApiRef.current = event.api;
              }}
            />
          </div>
        ) : (
          <div className="entity-table__empty">
            <h3>Gather spawns will appear here</h3>
            <p>Load an area JSON file to populate the gather spawn table.</p>
          </div>
        )
      ) : null}
    </div>
  );
}
