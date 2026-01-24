import type { MutableRefObject } from "react";
import type { ColDef, GridApi } from "ag-grid-community";
import { AgGridReact } from "ag-grid-react";
import {
  EntityTableToolbar,
  type EntityTableToolbarConfig
} from "./EntityTableToolbar";

type TutorialTableViewProps = {
  rows: Array<Record<string, unknown>>;
  columns: ColDef[];
  defaultColDef: ColDef;
  toolbar?: EntityTableToolbarConfig | null;
  onSelectTutorial: (index: number | null) => void;
  gridApiRef: MutableRefObject<GridApi | null>;
};

export function TutorialTableView({
  rows,
  columns,
  defaultColDef,
  toolbar,
  onSelectTutorial,
  gridApiRef
}: TutorialTableViewProps) {
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
              onSelectTutorial(event.data?.index ?? null)
            }
            onGridReady={(event) => {
              gridApiRef.current = event.api;
            }}
          />
        </div>
      ) : (
        <div className="entity-table__empty">
          <h3>Tutorials will appear here</h3>
          <p>Load a tutorials file from the data directory to get started.</p>
        </div>
      )}
    </div>
  );
}
