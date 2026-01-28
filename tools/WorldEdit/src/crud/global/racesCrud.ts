import type { ColDef } from "ag-grid-community";
import type { RaceDataFile, RaceDefinition } from "../../repository/types";
import { raceSkillCount } from "../../constants/globalDefaults";
import { crudFail, crudOk, type CrudResult } from "../types";

export type RaceRow = {
  index: number;
  name: string;
  whoName: string;
  pc: string;
  points: number;
  size: string;
  startLoc: number;
};

type RaceCrudCreateResult = {
  raceData: RaceDataFile;
  index: number;
  name: string;
};

type RaceCrudDeleteResult = {
  raceData: RaceDataFile;
  deletedName: string;
};

export const buildRaceRows = (raceData: RaceDataFile | null): RaceRow[] => {
  if (!raceData) {
    return [];
  }
  return raceData.races.map((race, index) => ({
    index,
    name: race.name ?? "(unnamed)",
    whoName: race.whoName ?? "",
    pc: race.pc ? "yes" : "no",
    points: race.points ?? 0,
    size: race.size ?? "medium",
    startLoc: race.startLoc ?? 0
  }));
};

export const raceColumns: ColDef<RaceRow>[] = [
  { headerName: "#", field: "index", width: 80, sort: "asc" },
  { headerName: "Name", field: "name", flex: 2, minWidth: 200 },
  { headerName: "Who", field: "whoName", width: 120 },
  { headerName: "PC", field: "pc", width: 80 },
  { headerName: "Points", field: "points", width: 110 },
  { headerName: "Size", field: "size", width: 110 },
  { headerName: "Start", field: "startLoc", width: 110 }
];

export const createRace = (
  raceData: RaceDataFile | null,
  classNames: string[]
): CrudResult<RaceCrudCreateResult> => {
  if (!raceData) {
    return crudFail("Load races before creating races.");
  }
  const nextIndex = raceData.races.length;
  const nextClassNames = classNames ?? [];
  const classMult = Object.fromEntries(
    nextClassNames.map((name) => [name, 100])
  );
  const classStart = Object.fromEntries(
    nextClassNames.map((name) => [name, 0])
  );
  const newRace: RaceDefinition = {
    name: `New Race ${nextIndex + 1}`,
    whoName: "",
    pc: false,
    points: 0,
    size: "",
    startLoc: 0,
    stats: {
      str: 0,
      int: 0,
      wis: 0,
      dex: 0,
      con: 0
    },
    maxStats: {
      str: 0,
      int: 0,
      wis: 0,
      dex: 0,
      con: 0
    },
    actFlags: [],
    affectFlags: [],
    offFlags: [],
    immFlags: [],
    resFlags: [],
    vulnFlags: [],
    formFlags: [],
    partFlags: [],
    classMult,
    classStart,
    skills: new Array(raceSkillCount).fill("")
  };
  const nextRaceData = {
    ...raceData,
    races: [...raceData.races, newRace]
  };
  return crudOk({
    raceData: nextRaceData,
    index: nextIndex,
    name: newRace.name
  });
};

export const deleteRace = (
  raceData: RaceDataFile | null,
  selectedIndex: number | null
): CrudResult<RaceCrudDeleteResult> => {
  if (!raceData) {
    return crudFail("Load races before deleting races.");
  }
  if (selectedIndex === null) {
    return crudFail("Select a race to delete.");
  }
  const deletedName = raceData.races[selectedIndex]?.name ?? "race";
  const nextRaces = raceData.races.filter((_, index) => index !== selectedIndex);
  return crudOk({
    raceData: { ...raceData, races: nextRaces },
    deletedName
  });
};
