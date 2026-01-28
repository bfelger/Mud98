import type { LootDataFile } from "../../repository/types";
import { crudFail, crudOk, type CrudResult } from "../types";

type LootCrudCreateResult = {
  lootData: LootDataFile;
  kind: "group" | "table";
  index: number;
};

type LootCrudDeleteResult = {
  lootData: LootDataFile;
  kind: "group" | "table";
  index: number;
};

export const createLoot = (
  lootData: LootDataFile | null,
  kind: "group" | "table"
): CrudResult<LootCrudCreateResult> => {
  if (!lootData) {
    return crudFail("Load loot before creating loot entries.");
  }
  const nextGroups = [...lootData.groups];
  const nextTables = [...lootData.tables];
  let nextIndex = 0;
  if (kind === "group") {
    nextGroups.push({
      name: "New Loot Group",
      rolls: 1,
      entries: []
    });
    nextIndex = nextGroups.length - 1;
  } else {
    nextTables.push({
      name: "New Loot Table",
      parent: "",
      ops: []
    });
    nextIndex = nextTables.length - 1;
  }
  return crudOk({
    lootData: {
      ...lootData,
      groups: nextGroups,
      tables: nextTables
    },
    kind,
    index: nextIndex
  });
};

export const deleteLoot = (
  lootData: LootDataFile | null,
  kind: "group" | "table" | null,
  index: number | null
): CrudResult<LootCrudDeleteResult> => {
  if (!lootData) {
    return crudFail("Load loot before deleting loot entries.");
  }
  if (kind === null || index === null) {
    return crudFail("Select a loot entry to delete.");
  }
  const nextGroups = [...lootData.groups];
  const nextTables = [...lootData.tables];
  if (kind === "group") {
    nextGroups.splice(index, 1);
  } else {
    nextTables.splice(index, 1);
  }
  return crudOk({
    lootData: {
      ...lootData,
      groups: nextGroups,
      tables: nextTables
    },
    kind,
    index
  });
};
