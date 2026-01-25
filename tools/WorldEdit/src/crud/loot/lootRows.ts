import type { ColDef } from "ag-grid-community";
import type { LootDataFile } from "../../repository/types";

export type LootRow = {
  id: string;
  kind: "group" | "table";
  index: number;
  name: string;
  details: string;
};

export const lootColumns: ColDef<LootRow>[] = [
  { headerName: "Type", field: "kind", width: 110, sort: "asc" },
  { headerName: "Name", field: "name", flex: 2, minWidth: 220 },
  { headerName: "Details", field: "details", flex: 2, minWidth: 240 }
];

export const buildLootRows = (lootData: LootDataFile | null): LootRow[] => {
  if (!lootData) {
    return [];
  }
  const rows: LootRow[] = [];
  lootData.groups.forEach((group, index) => {
    rows.push({
      id: `group:${index}`,
      kind: "group",
      index,
      name: group.name ?? "(unnamed group)",
      details: `rolls ${group.rolls ?? 1} · entries ${
        group.entries?.length ?? 0
      }`
    });
  });
  lootData.tables.forEach((table, index) => {
    const parent = table.parent ? `parent ${table.parent}` : "no parent";
    rows.push({
      id: `table:${index}`,
      kind: "table",
      index,
      name: table.name ?? "(unnamed table)",
      details: `${parent} · ops ${table.ops?.length ?? 0}`
    });
  });
  return rows;
};
