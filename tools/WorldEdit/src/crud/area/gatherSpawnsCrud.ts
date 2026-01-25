import type { ColDef } from "ag-grid-community";
import type { AreaJson } from "../../repository/types";
import { crudFail, crudOk, type CrudResult } from "../types";
import { getFirstString, getNextVnumFromList, parseVnum } from "./utils";

export type GatherSpawnRow = {
  index: number;
  vnum: number;
  sector: string;
  quantity: number;
  respawnTimer: number;
};

type GatherSpawnCrudCreateResult = {
  areaData: AreaJson;
  vnum: number;
};

type GatherSpawnCrudDeleteResult = {
  areaData: AreaJson;
  deletedVnum: number;
};

export const gatherSpawnColumns: ColDef<GatherSpawnRow>[] = [
  { headerName: "#", field: "index", width: 80, sort: "asc" },
  { headerName: "VNUM", field: "vnum", width: 120 },
  { headerName: "Sector", field: "sector", width: 140 },
  { headerName: "Qty", field: "quantity", width: 110 },
  { headerName: "Respawn", field: "respawnTimer", width: 120 }
];

export const buildGatherSpawnRows = (
  areaData: AreaJson | null
): GatherSpawnRow[] => {
  const spawns = Array.isArray(
    (areaData as Record<string, unknown>)?.gatherSpawns
  )
    ? ((areaData as Record<string, unknown>).gatherSpawns as unknown[])
    : [];
  return spawns
    .filter((entry): entry is Record<string, unknown> =>
      Boolean(entry && typeof entry === "object")
    )
    .map((record, index) => {
      const vnum = parseVnum(record.vnum) ?? 0;
      const sector =
        typeof record.spawnSector === "string" ? record.spawnSector : "—";
      return {
        index,
        vnum,
        sector,
        quantity: parseVnum(record.quantity) ?? 0,
        respawnTimer: parseVnum(record.respawnTimer) ?? 0
      };
    });
};

export const createGatherSpawn = (
  areaData: AreaJson | null
): CrudResult<GatherSpawnCrudCreateResult> => {
  if (!areaData) {
    return crudFail("Load an area before creating gather spawns.");
  }
  const spawns = Array.isArray((areaData as Record<string, unknown>).gatherSpawns)
    ? ((areaData as Record<string, unknown>).gatherSpawns as unknown[])
    : [];
  const nextVnum = getNextVnumFromList(areaData, spawns);
  if (nextVnum === null) {
    return crudFail("No available gather spawn VNUMs in the area range.");
  }
  const rooms = Array.isArray((areaData as Record<string, unknown>).rooms)
    ? ((areaData as Record<string, unknown>).rooms as unknown[])
    : [];
  const firstRoom = rooms.find((room) => room && typeof room === "object");
  const roomVnum = firstRoom
    ? parseVnum((firstRoom as Record<string, unknown>).vnum) ?? 0
    : 0;
  const newSpawn: Record<string, unknown> = {
    vnum: nextVnum,
    name: "New Gather Spawn",
    skill: "",
    roomVnum,
    successTable: "",
    failureTable: "",
    minSkill: 0,
    minLevel: 1
  };
  const nextSpawns = [...spawns, newSpawn];
  const nextAreaData = {
    ...(areaData as Record<string, unknown>),
    gatherSpawns: nextSpawns
  } as AreaJson;
  return crudOk({ areaData: nextAreaData, vnum: nextVnum });
};

export const deleteGatherSpawn = (
  areaData: AreaJson | null,
  selectedVnum: number | null
): CrudResult<GatherSpawnCrudDeleteResult> => {
  if (!areaData) {
    return crudFail("Load an area before deleting gather spawns.");
  }
  if (selectedVnum === null) {
    return crudFail("Select a gather spawn to delete.");
  }
  const spawns = Array.isArray((areaData as Record<string, unknown>).gatherSpawns)
    ? ((areaData as Record<string, unknown>).gatherSpawns as unknown[])
    : [];
  const nextSpawns = spawns.filter((spawn) => {
    if (!spawn || typeof spawn !== "object") {
      return true;
    }
    const vnum = parseVnum((spawn as Record<string, unknown>).vnum);
    return vnum !== selectedVnum;
  });
  const nextAreaData = {
    ...(areaData as Record<string, unknown>),
    gatherSpawns: nextSpawns
  } as AreaJson;
  return crudOk({ areaData: nextAreaData, deletedVnum: selectedVnum });
};
