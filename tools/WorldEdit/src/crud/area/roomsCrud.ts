import type { ColDef } from "ag-grid-community";
import type { AreaJson } from "../../repository/types";
import { crudFail, crudOk, type CrudResult } from "../types";
import { getFirstString, getNextVnumFromList, parseVnum } from "./utils";

export type RoomRow = {
  vnum: number;
  name: string;
  sector: string;
  exits: number;
  flags: string;
};

type RoomCrudCreateResult = {
  areaData: AreaJson;
  vnum: number;
};

type RoomCrudDeleteResult = {
  areaData: AreaJson;
  roomId: string;
  deletedVnum: number;
};

const fallbackSector = "inside";

export const buildRoomRows = (areaData: AreaJson | null): RoomRow[] => {
  if (!areaData) {
    return [];
  }
  const rooms = (areaData as { rooms?: unknown }).rooms;
  if (!Array.isArray(rooms)) {
    return [];
  }
  return rooms.map((room) => {
    const data = room as Record<string, unknown>;
    const vnum = parseVnum(data.vnum) ?? -1;
    const name = getFirstString(data.name, "(unnamed room)");
    const sectorType =
      typeof data.sectorType === "string"
        ? data.sectorType
        : typeof data.sector === "string"
          ? data.sector
          : fallbackSector;
    const roomFlags = Array.isArray(data.roomFlags)
      ? data.roomFlags.filter((flag) => typeof flag === "string")
      : [];
    const exits = Array.isArray(data.exits) ? data.exits.length : 0;
    return {
      vnum,
      name,
      sector: sectorType,
      exits,
      flags: roomFlags.length ? roomFlags.join(", ") : "—"
    };
  });
};

export const roomColumns: ColDef<RoomRow>[] = [
  { headerName: "VNUM", field: "vnum", width: 110, sort: "asc" },
  { headerName: "Name", field: "name", flex: 2, minWidth: 200 },
  { headerName: "Sector", field: "sector", flex: 1, minWidth: 140 },
  { headerName: "Exits", field: "exits", width: 110 },
  { headerName: "Room Flags", field: "flags", flex: 2, minWidth: 220 }
];

export const createRoom = (
  areaData: AreaJson | null
): CrudResult<RoomCrudCreateResult> => {
  if (!areaData) {
    return crudFail("Load an area before creating rooms.");
  }
  const rooms = Array.isArray((areaData as Record<string, unknown>).rooms)
    ? ((areaData as Record<string, unknown>).rooms as unknown[])
    : [];
  const nextVnum = getNextVnumFromList(areaData, rooms);
  if (nextVnum === null) {
    return crudFail("No available room VNUMs in the area range.");
  }
  const areadata = (areaData as Record<string, unknown>).areadata;
  const areaSector =
    areadata && typeof areadata === "object"
      ? (areadata as Record<string, unknown>).sector
      : null;
  const sectorType =
    typeof areaSector === "string" && areaSector.trim().length
      ? areaSector
      : fallbackSector;
  const newRoom: Record<string, unknown> = {
    vnum: nextVnum,
    name: "New Room",
    description: "An unfinished room.\n\r",
    sectorType,
    roomFlags: [],
    exits: []
  };
  const nextRooms = [...rooms, newRoom];
  const nextAreaData = {
    ...(areaData as Record<string, unknown>),
    rooms: nextRooms
  } as AreaJson;
  return crudOk({ areaData: nextAreaData, vnum: nextVnum });
};

export const deleteRoom = (
  areaData: AreaJson | null,
  selectedRoomVnum: number | null
): CrudResult<RoomCrudDeleteResult> => {
  if (!areaData) {
    return crudFail("Load an area before deleting rooms.");
  }
  if (selectedRoomVnum === null) {
    return crudFail("Select a room to delete.");
  }
  const rooms = Array.isArray((areaData as Record<string, unknown>).rooms)
    ? ((areaData as Record<string, unknown>).rooms as unknown[])
    : [];
  const nextRooms = rooms.filter((room) => {
    if (!room || typeof room !== "object") {
      return true;
    }
    const vnum = parseVnum((room as Record<string, unknown>).vnum);
    return vnum !== selectedRoomVnum;
  });
  const nextAreaData = {
    ...(areaData as Record<string, unknown>),
    rooms: nextRooms
  } as AreaJson;
  return crudOk({
    areaData: nextAreaData,
    roomId: String(selectedRoomVnum),
    deletedVnum: selectedRoomVnum
  });
};
