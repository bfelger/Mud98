import type {
  AreaJson,
  LootDataFile,
  LootGroup,
  LootTable
} from "../../repository/types";
import { crudFail, crudOk, type CrudResult } from "../types";

type AreaLootCreateResult = {
  areaData: AreaJson;
  kind: "group" | "table";
  index: number;
};

type AreaLootDeleteResult = {
  areaData: AreaJson;
  kind: "group" | "table";
  index: number;
};

export const extractAreaLootData = (
  areaData: AreaJson | null
): LootDataFile | null => {
  if (!areaData) {
    return null;
  }
  const loot = (areaData as Record<string, unknown>).loot;
  if (!loot || typeof loot !== "object") {
    return null;
  }
  const record = loot as Record<string, unknown>;
  const groups = Array.isArray(record.groups)
    ? (record.groups as LootGroup[])
    : [];
  const tables = Array.isArray(record.tables)
    ? (record.tables as LootTable[])
    : [];
  return {
    formatVersion: 1,
    groups,
    tables
  };
};

export const createAreaLoot = (
  areaData: AreaJson | null,
  kind: "group" | "table"
): CrudResult<AreaLootCreateResult> => {
  if (!areaData) {
    return crudFail("Load an area before creating loot.");
  }
  const currentLoot = extractAreaLootData(areaData);
  const nextGroups = currentLoot ? [...currentLoot.groups] : [];
  const nextTables = currentLoot ? [...currentLoot.tables] : [];
  let nextIndex = 0;
  if (kind === "group") {
    nextGroups.push({
      name: "New Loot Group",
      rolls: 1,
      entries: [],
      ops: []
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
  const nextAreaData = {
    ...(areaData as Record<string, unknown>),
    loot: {
      groups: nextGroups,
      tables: nextTables
    }
  } as AreaJson;
  return crudOk({ areaData: nextAreaData, kind, index: nextIndex });
};

export const deleteAreaLoot = (
  areaData: AreaJson | null,
  kind: "group" | "table" | null,
  index: number | null
): CrudResult<AreaLootDeleteResult> => {
  if (!areaData) {
    return crudFail("Load an area before deleting loot.");
  }
  if (kind === null || index === null) {
    return crudFail("Select a loot entry to delete.");
  }
  const currentLoot = extractAreaLootData(areaData);
  if (!currentLoot) {
    return crudFail("No loot data loaded.");
  }
  const nextGroups = [...currentLoot.groups];
  const nextTables = [...currentLoot.tables];
  if (kind === "group") {
    nextGroups.splice(index, 1);
  } else {
    nextTables.splice(index, 1);
  }
  const nextAreaData = {
    ...(areaData as Record<string, unknown>),
    loot: {
      groups: nextGroups,
      tables: nextTables
    }
  } as AreaJson;
  return crudOk({ areaData: nextAreaData, kind, index });
};
