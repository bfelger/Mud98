import type { ColDef } from "ag-grid-community";
import type { AreaJson } from "../../repository/types";
import { crudFail, crudOk, type CrudResult } from "../types";
import { getFirstString, getNextVnumFromList, parseVnum } from "./utils";

export type ObjectRow = {
  vnum: number;
  name: string;
  itemType: string;
  level: number;
};

type ObjectCrudCreateResult = {
  areaData: AreaJson;
  vnum: number;
};

type ObjectCrudDeleteResult = {
  areaData: AreaJson;
  deletedVnum: number;
};

export const buildObjectRows = (areaData: AreaJson | null): ObjectRow[] => {
  if (!areaData) {
    return [];
  }
  const objects = (areaData as { objects?: unknown }).objects;
  if (!Array.isArray(objects)) {
    return [];
  }
  return objects.map((obj) => {
    const data = obj as Record<string, unknown>;
    const vnum = parseVnum(data.vnum) ?? -1;
    const name = getFirstString(data.shortDescr, "(unnamed object)");
    const itemType = getFirstString(data.itemType, "unknown");
    const level = parseVnum(data.level) ?? 0;
    return { vnum, name, itemType, level };
  });
};

export const objectColumns: ColDef<ObjectRow>[] = [
  { headerName: "VNUM", field: "vnum", width: 110, sort: "asc" },
  { headerName: "Name", field: "name", flex: 2, minWidth: 200 },
  { headerName: "Item Type", field: "itemType", flex: 1, minWidth: 160 },
  { headerName: "Level", field: "level", width: 110 }
];

export const createObject = (
  areaData: AreaJson | null
): CrudResult<ObjectCrudCreateResult> => {
  if (!areaData) {
    return crudFail("Load an area before creating objects.");
  }
  const objects = Array.isArray((areaData as Record<string, unknown>).objects)
    ? ((areaData as Record<string, unknown>).objects as unknown[])
    : [];
  const nextVnum = getNextVnumFromList(areaData, objects);
  if (nextVnum === null) {
    return crudFail("No available object VNUMs in the area range.");
  }
  const newObject: Record<string, unknown> = {
    vnum: nextVnum,
    name: "new object",
    shortDescr: "a new object",
    description: "An unfinished object sits here.\n\r",
    material: "wood",
    itemType: "trash",
    level: 1,
    weight: 1,
    cost: 0,
    extraFlags: [],
    wearFlags: []
  };
  const nextObjects = [...objects, newObject];
  const nextAreaData = {
    ...(areaData as Record<string, unknown>),
    objects: nextObjects
  } as AreaJson;
  return crudOk({ areaData: nextAreaData, vnum: nextVnum });
};

export const deleteObject = (
  areaData: AreaJson | null,
  selectedVnum: number | null
): CrudResult<ObjectCrudDeleteResult> => {
  if (!areaData) {
    return crudFail("Load an area before deleting objects.");
  }
  if (selectedVnum === null) {
    return crudFail("Select an object to delete.");
  }
  const objects = Array.isArray((areaData as Record<string, unknown>).objects)
    ? ((areaData as Record<string, unknown>).objects as unknown[])
    : [];
  const nextObjects = objects.filter((obj) => {
    if (!obj || typeof obj !== "object") {
      return true;
    }
    const vnum = parseVnum((obj as Record<string, unknown>).vnum);
    return vnum !== selectedVnum;
  });
  const nextAreaData = {
    ...(areaData as Record<string, unknown>),
    objects: nextObjects
  } as AreaJson;
  return crudOk({ areaData: nextAreaData, deletedVnum: selectedVnum });
};
