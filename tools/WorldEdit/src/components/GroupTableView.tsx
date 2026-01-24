import type { MutableRefObject } from "react";
import type { ColDef, GridApi } from "ag-grid-community";
import { AgGridReact } from "ag-grid-react";
import {
  EntityTableToolbar,
  type EntityTableToolbarConfig
} from "./EntityTableToolbar";

type GroupTableViewProps = {
  rows: Array<Record<string, unknown>>;
  columns: ColDef[];
  defaultColDef: ColDef;
  toolbar?: EntityTableToolbarConfig | null;
  onSelectGroup: (index: number | null) => void;
  gridApiRef: MutableRefObject<GridApi | null>;
};

export function GroupTableView({
  rows,
  columns,
  defaultColDef,
  toolbar,
  onSelectGroup,
  gridApiRef
}: GroupTableViewProps) {
  return (
    <div className="entity-table">
      {toolbar ? <EntityTableToolbar {...toolbar} /> : null}
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
              onSelectGroup(event.data?.index ?? null)
            }
            onGridReady={(event) => {
              gridApiRef.current = event.api;
            }}
          />
        </div>
      ) : (
        <div className="entity-table__empty">
          <h3>Groups will appear here</h3>
          <p>Load a groups file from the data directory to get started.</p>
        </div>
      )}
    </div>
  );
}
