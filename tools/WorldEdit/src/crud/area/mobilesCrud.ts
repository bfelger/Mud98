import type { ColDef } from "ag-grid-community";
import type { AreaJson } from "../../repository/types";
import { crudFail, crudOk, type CrudResult } from "../types";
import { getFirstString, getNextVnumFromList, parseVnum } from "./utils";

export type MobileRow = {
  vnum: number;
  name: string;
  level: number;
  race: string;
  alignment: number;
};

type MobileCrudCreateResult = {
  areaData: AreaJson;
  vnum: number;
};

type MobileCrudDeleteResult = {
  areaData: AreaJson;
  deletedVnum: number;
};

const defaultMobileRace = "human";

export const buildMobileRows = (areaData: AreaJson | null): MobileRow[] => {
  if (!areaData) {
    return [];
  }
  const mobiles = (areaData as { mobiles?: unknown }).mobiles;
  if (!Array.isArray(mobiles)) {
    return [];
  }
  return mobiles.map((mob) => {
    const data = mob as Record<string, unknown>;
    const vnum = parseVnum(data.vnum) ?? -1;
    const name = getFirstString(data.shortDescr, "(unnamed mobile)");
    const level = parseVnum(data.level) ?? 0;
    const race = getFirstString(data.race, "unknown");
    const alignment = parseVnum(data.alignment) ?? 0;
    return { vnum, name, level, race, alignment };
  });
};

export const mobileColumns: ColDef<MobileRow>[] = [
  { headerName: "VNUM", field: "vnum", width: 110, sort: "asc" },
  { headerName: "Name", field: "name", flex: 2, minWidth: 200 },
  { headerName: "Level", field: "level", width: 110 },
  { headerName: "Race", field: "race", flex: 1, minWidth: 140 },
  { headerName: "Alignment", field: "alignment", width: 130 }
];

export const createMobile = (
  areaData: AreaJson | null,
  defaultRace?: string
): CrudResult<MobileCrudCreateResult> => {
  if (!areaData) {
    return crudFail("Load an area before creating mobiles.");
  }
  const mobiles = Array.isArray((areaData as Record<string, unknown>).mobiles)
    ? ((areaData as Record<string, unknown>).mobiles as unknown[])
    : [];
  const nextVnum = getNextVnumFromList(areaData, mobiles);
  if (nextVnum === null) {
    return crudFail("No available mobile VNUMs in the area range.");
  }
  const race =
    typeof defaultRace === "string" && defaultRace.trim().length
      ? defaultRace
      : defaultMobileRace;
  const newMobile: Record<string, unknown> = {
    vnum: nextVnum,
    name: "new mobile",
    shortDescr: "a new mobile",
    longDescr: "A new mobile is here.\n\r",
    description: "It looks unfinished.\n\r",
    race,
    level: 1,
    alignment: 0
  };
  const nextMobiles = [...mobiles, newMobile];
  const nextAreaData = {
    ...(areaData as Record<string, unknown>),
    mobiles: nextMobiles
  } as AreaJson;
  return crudOk({ areaData: nextAreaData, vnum: nextVnum });
};

export const deleteMobile = (
  areaData: AreaJson | null,
  selectedVnum: number | null
): CrudResult<MobileCrudDeleteResult> => {
  if (!areaData) {
    return crudFail("Load an area before deleting mobiles.");
  }
  if (selectedVnum === null) {
    return crudFail("Select a mobile to delete.");
  }
  const mobiles = Array.isArray((areaData as Record<string, unknown>).mobiles)
    ? ((areaData as Record<string, unknown>).mobiles as unknown[])
    : [];
  const nextMobiles = mobiles.filter((mob) => {
    if (!mob || typeof mob !== "object") {
      return true;
    }
    const vnum = parseVnum((mob as Record<string, unknown>).vnum);
    return vnum !== selectedVnum;
  });
  const nextAreaData = {
    ...(areaData as Record<string, unknown>),
    mobiles: nextMobiles
  } as AreaJson;
  return crudOk({ areaData: nextAreaData, deletedVnum: selectedVnum });
};
