import type { MutableRefObject } from "react";
import type { ColDef, GridApi } from "ag-grid-community";
import { AgGridReact } from "ag-grid-react";

type RaceTableViewProps = {
  rows: Array<Record<string, unknown>>;
  columns: ColDef[];
  defaultColDef: ColDef;
  onSelectRace: (index: number | null) => void;
  gridApiRef: MutableRefObject<GridApi | null>;
};

export function RaceTableView({
  rows,
  columns,
  defaultColDef,
  onSelectRace,
  gridApiRef
}: RaceTableViewProps) {
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
            getRowId={(params) => String(params.data.index)}
            domLayout="autoHeight"
            onRowClicked={(event) =>
              onSelectRace(event.data?.index ?? null)
            }
            onGridReady={(event) => {
              gridApiRef.current = event.api;
            }}
          />
        </div>
      ) : (
        <div className="entity-table__empty">
          <h3>Races will appear here</h3>
          <p>Load a races file from the data directory to get started.</p>
        </div>
      )}
    </div>
  );
}
