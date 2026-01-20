import type { MutableRefObject } from "react";
import type { ColDef, GridApi } from "ag-grid-community";
import { AgGridReact } from "ag-grid-react";

type LootTableViewProps = {
  rows: Array<Record<string, unknown>>;
  columns: ColDef[];
  defaultColDef: ColDef;
  onSelectLoot: (kind: "group" | "table" | null, index: number | null) => void;
  gridApiRef: MutableRefObject<GridApi | null>;
};

export function LootTableView({
  rows,
  columns,
  defaultColDef,
  onSelectLoot,
  gridApiRef
}: LootTableViewProps) {
  return (
    <div className="entity-table">
      {rows.length ? (
        <div className="ag-theme-quartz worldedit-grid">
          <AgGridReact
            rowData={rows}
            columnDefs={columns}
            defaultColDef={defaultColDef}
            animateRows
            rowSelection="single"
            getRowId={(params) => String(params.data.id)}
            domLayout="autoHeight"
            onRowClicked={(event) =>
              onSelectLoot(event.data?.kind ?? null, event.data?.index ?? null)
            }
            onGridReady={(event) => {
              gridApiRef.current = event.api;
            }}
          />
        </div>
      ) : (
        <div className="entity-table__empty">
          <h3>Loot entries will appear here</h3>
          <p>Load the loot file from the data directory to get started.</p>
        </div>
      )}
    </div>
  );
}
