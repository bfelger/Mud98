import type { LootDataFile, LootGroup, LootTable } from "../../repository/types";

export type LootRow = {
  id: string;
  kind: "group" | "table";
  index: number;
  name: string;
  details: string;
};

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
