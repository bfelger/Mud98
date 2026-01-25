import type { ColDef } from "ag-grid-community";
import type { AreaJson } from "../../repository/types";
import { crudFail, crudOk, type CrudResult } from "../types";
import { getFirstString, getNextVnumFromList, parseVnum } from "./utils";

export type FactionRow = {
  vnum: number;
  name: string;
  defaultStanding: number;
  allies: string;
  opposing: string;
};

type FactionCrudCreateResult = {
  areaData: AreaJson;
  vnum: number;
};

type FactionCrudDeleteResult = {
  areaData: AreaJson;
  deletedVnum: number;
};

export const factionColumns: ColDef<FactionRow>[] = [
  { headerName: "VNUM", field: "vnum", width: 110, sort: "asc" },
  { headerName: "Name", field: "name", flex: 2, minWidth: 200 },
  { headerName: "Default", field: "defaultStanding", width: 120 },
  { headerName: "Allies", field: "allies", width: 110 },
  { headerName: "Opposing", field: "opposing", width: 120 }
];

export const buildFactionRows = (areaData: AreaJson | null): FactionRow[] => {
  if (!areaData) {
    return [];
  }
  const factions = Array.isArray((areaData as Record<string, unknown>).factions)
    ? ((areaData as Record<string, unknown>).factions as unknown[])
    : [];
  if (!factions.length) {
    return [];
  }
  return factions.map((faction) => {
    const record = faction as Record<string, unknown>;
    const vnum = parseVnum(record.vnum) ?? -1;
    const name = getFirstString(record.name, "(unnamed faction)");
    const defaultStanding = parseVnum(record.defaultStanding) ?? 0;
    const allies = Array.isArray(record.allies) ? String(record.allies.length) : "0";
    const opposing = Array.isArray(record.opposing)
      ? String(record.opposing.length)
      : "0";
    return {
      vnum,
      name,
      defaultStanding,
      allies,
      opposing
    };
  });
};

export const createFaction = (
  areaData: AreaJson | null
): CrudResult<FactionCrudCreateResult> => {
  if (!areaData) {
    return crudFail("Load an area before creating factions.");
  }
  const factions = Array.isArray((areaData as Record<string, unknown>).factions)
    ? ((areaData as Record<string, unknown>).factions as unknown[])
    : [];
  const nextVnum = getNextVnumFromList(areaData, factions);
  if (nextVnum === null) {
    return crudFail("No available faction VNUMs in the area range.");
  }
  const newFaction: Record<string, unknown> = {
    vnum: nextVnum,
    name: "New Faction",
    defaultStanding: 0,
    allies: [],
    opposing: []
  };
  const nextFactions = [...factions, newFaction];
  const nextAreaData = {
    ...(areaData as Record<string, unknown>),
    factions: nextFactions
  } as AreaJson;
  return crudOk({ areaData: nextAreaData, vnum: nextVnum });
};

export const deleteFaction = (
  areaData: AreaJson | null,
  selectedVnum: number | null
): CrudResult<FactionCrudDeleteResult> => {
  if (!areaData) {
    return crudFail("Load an area before deleting factions.");
  }
  if (selectedVnum === null) {
    return crudFail("Select a faction to delete.");
  }
  const factions = Array.isArray((areaData as Record<string, unknown>).factions)
    ? ((areaData as Record<string, unknown>).factions as unknown[])
    : [];
  const nextFactions = factions.filter((faction) => {
    if (!faction || typeof faction !== "object") {
      return true;
    }
    const vnum = parseVnum((faction as Record<string, unknown>).vnum);
    return vnum !== selectedVnum;
  });
  const nextAreaData = {
    ...(areaData as Record<string, unknown>),
    factions: nextFactions
  } as AreaJson;
  return crudOk({ areaData: nextAreaData, deletedVnum: selectedVnum });
};
