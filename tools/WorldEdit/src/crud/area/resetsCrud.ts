import type { ColDef } from "ag-grid-community";
import type { AreaJson } from "../../repository/types";
import { crudFail, crudOk, type CrudResult } from "../types";
import { getFirstString, parseVnum } from "./utils";

export type ResetRow = {
  index: number;
  command: string;
  entityVnum: string;
  roomVnum: string;
  details: string;
};

type ResetCrudCreateResult = {
  areaData: AreaJson;
  index: number;
};

type ResetCrudDeleteResult = {
  areaData: AreaJson;
  deletedIndex: number;
};

export const resetColumns: ColDef<ResetRow>[] = [
  { headerName: "#", field: "index", width: 80, sort: "asc" },
  { headerName: "Type", field: "command", flex: 1, minWidth: 160 },
  { headerName: "Entity VNUM", field: "entityVnum", width: 140 },
  { headerName: "Room VNUM", field: "roomVnum", width: 140 },
  { headerName: "Details", field: "details", flex: 2, minWidth: 220 }
];

export const buildResetRows = (areaData: AreaJson | null): ResetRow[] => {
  if (!areaData) {
    return [];
  }
  const resets = (areaData as { resets?: unknown }).resets;
  if (!Array.isArray(resets)) {
    return [];
  }
  return resets.map((reset, index) => {
    const data = reset as Record<string, unknown>;
    const command = getFirstString(
      data.commandName,
      getFirstString(data.command, "reset")
    );
    const mobVnum = parseVnum(data.mobVnum);
    const objVnum = parseVnum(data.objVnum);
    const containerVnum = parseVnum(data.containerVnum);
    const entityVnum =
      mobVnum !== null
        ? String(mobVnum)
        : objVnum !== null
          ? String(objVnum)
          : containerVnum !== null
            ? String(containerVnum)
            : "—";
    const roomVnum = parseVnum(data.roomVnum);
    const toVnum = parseVnum(data.toVnum);
    const roomLabel =
      roomVnum !== null ? String(roomVnum) : toVnum !== null ? String(toVnum) : "—";
    const count = parseVnum(data.count);
    const maxInArea = parseVnum(data.maxInArea);
    const maxInRoom = parseVnum(data.maxInRoom);
    const maxInContainer = parseVnum(data.maxInContainer);
    const wearLoc = getFirstString(data.wearLoc, "");
    const direction = getFirstString(data.direction, "");
    const state = parseVnum(data.state);
    const exits = parseVnum(data.exits);
    const detailsParts: string[] = [];
    if (count !== null) {
      detailsParts.push(`count ${count}`);
    }
    if (maxInArea !== null) {
      detailsParts.push(`max area ${maxInArea}`);
    }
    if (maxInRoom !== null) {
      detailsParts.push(`max room ${maxInRoom}`);
    }
    if (maxInContainer !== null) {
      detailsParts.push(`max container ${maxInContainer}`);
    }
    if (wearLoc) {
      detailsParts.push(`wear ${wearLoc}`);
    }
    if (direction) {
      detailsParts.push(`dir ${direction}`);
    }
    if (state !== null) {
      detailsParts.push(`state ${state}`);
    }
    if (exits !== null) {
      detailsParts.push(`exits ${exits}`);
    }
    const details = detailsParts.length ? detailsParts.join(", ") : "—";
    return {
      index,
      command,
      entityVnum,
      roomVnum: roomLabel,
      details
    };
  });
};

export const createReset = (
  areaData: AreaJson | null
): CrudResult<ResetCrudCreateResult> => {
  if (!areaData) {
    return crudFail("Load an area before creating resets.");
  }
  const rooms = Array.isArray((areaData as Record<string, unknown>).rooms)
    ? ((areaData as Record<string, unknown>).rooms as unknown[])
    : [];
  const mobiles = Array.isArray((areaData as Record<string, unknown>).mobiles)
    ? ((areaData as Record<string, unknown>).mobiles as unknown[])
    : [];
  const roomVnum =
    parseVnum((rooms.find((room) => room && typeof room === "object") as Record<
      string,
      unknown
    >)?.vnum) ?? 0;
  const mobVnum =
    parseVnum((mobiles.find((mob) => mob && typeof mob === "object") as Record<
      string,
      unknown
    >)?.vnum) ?? 0;
  const resets = Array.isArray((areaData as Record<string, unknown>).resets)
    ? ((areaData as Record<string, unknown>).resets as unknown[])
    : [];
  const nextIndex = resets.length;
  const newReset: Record<string, unknown> = {
    commandName: "loadMob",
    mobVnum,
    roomVnum,
    maxInArea: 1,
    maxInRoom: 1
  };
  const nextResets = [...resets, newReset];
  const nextAreaData = {
    ...(areaData as Record<string, unknown>),
    resets: nextResets
  } as AreaJson;
  return crudOk({ areaData: nextAreaData, index: nextIndex });
};

export const deleteReset = (
  areaData: AreaJson | null,
  selectedIndex: number | null
): CrudResult<ResetCrudDeleteResult> => {
  if (!areaData) {
    return crudFail("Load an area before deleting resets.");
  }
  if (selectedIndex === null) {
    return crudFail("Select a reset to delete.");
  }
  const resets = Array.isArray((areaData as Record<string, unknown>).resets)
    ? ((areaData as Record<string, unknown>).resets as unknown[])
    : [];
  const nextResets = resets.filter((_, index) => index !== selectedIndex);
  const nextAreaData = {
    ...(areaData as Record<string, unknown>),
    resets: nextResets
  } as AreaJson;
  return crudOk({ areaData: nextAreaData, deletedIndex: selectedIndex });
};
