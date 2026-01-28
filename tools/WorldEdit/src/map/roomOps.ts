import type { AreaJson } from "../repository/types";

type RoomNodeSize = {
  width: number;
  height: number;
};

type RoomLayoutNode = {
  id: string;
  type: "room";
  position: { x: number; y: number };
  data: {
    vnum: number;
    label: string;
    sector: string;
    upExitTargets: number[];
    downExitTargets: number[];
  };
};

type RoomOpsHelpers = {
  parseVnum: (value: unknown) => number | null;
  getEntityList: (areaData: AreaJson, key: string) => unknown[];
  findByVnum: (list: unknown[], vnum: number) => Record<string, unknown> | null;
};

type DigRoomParams = RoomOpsHelpers & {
  getNextEntityVnum: (areaData: AreaJson, entity: string) => number | null;
  areaData: AreaJson | null;
  fromVnum: number;
  direction: string;
  autoLayoutEnabled: boolean;
  roomNodeSize: RoomNodeSize;
  sourceNodePosition: { x: number; y: number } | null;
};

type LinkRoomsParams = RoomOpsHelpers & {
  areaData: AreaJson | null;
  fromVnum: number;
  direction: string;
  targetVnumInput: string;
};

type DigRoomResult =
  | {
      ok: true;
      areaData: AreaJson;
      nextVnum: number;
      message: string;
      layoutNode?: RoomLayoutNode;
    }
  | { ok: false; message: string };

type LinkRoomsResult =
  | { ok: true; areaData: AreaJson; message: string }
  | { ok: false; message: string };

const oppositeDirectionMap: Record<string, string> = {
  north: "south",
  east: "west",
  south: "north",
  west: "east",
  up: "down",
  down: "up"
};

const normalizeDirectionKey = (value: unknown): string =>
  typeof value === "string" ? value.trim().toLowerCase() : "";

const hasExitDirection = (exits: unknown[], dirKey: string): boolean =>
  exits.some((exit) => {
    if (!exit || typeof exit !== "object") {
      return false;
    }
    const exitRecord = exit as Record<string, unknown>;
    return normalizeDirectionKey(exitRecord.dir) === dirKey;
  });

export const digRoom = (params: DigRoomParams): DigRoomResult => {
  const {
    areaData,
    fromVnum,
    direction,
    autoLayoutEnabled,
    roomNodeSize,
    sourceNodePosition,
    parseVnum,
    getEntityList,
    findByVnum,
    getNextEntityVnum
  } = params;
  if (!areaData) {
    return { ok: false, message: "Load an area before digging rooms." };
  }
  const dirKey = normalizeDirectionKey(direction);
  const backDir = oppositeDirectionMap[dirKey];
  if (!backDir) {
    return { ok: false, message: `Unknown direction: ${direction}` };
  }
  const rooms = getEntityList(areaData, "Rooms");
  const source = findByVnum(rooms, fromVnum);
  if (!source) {
    return { ok: false, message: `Room ${fromVnum} not found.` };
  }
  const sourceExits = Array.isArray(source.exits) ? source.exits : [];
  if (hasExitDirection(sourceExits, dirKey)) {
    return {
      ok: false,
      message: `Room ${fromVnum} already has a ${dirKey} exit.`
    };
  }
  const nextVnum = getNextEntityVnum(areaData, "Rooms");
  if (nextVnum === null) {
    return {
      ok: false,
      message: "No available room VNUMs in the area range."
    };
  }
  const areadata = (areaData as Record<string, unknown>).areadata;
  const areaSector =
    areadata && typeof areadata === "object"
      ? (areadata as Record<string, unknown>).sector
      : null;
  const sectorType =
    typeof areaSector === "string" && areaSector.trim().length
      ? areaSector
      : "inside";
  const newRoom: Record<string, unknown> = {
    vnum: nextVnum,
    name: "New Room",
    description: "An unfinished room.\n\r",
    sectorType,
    roomFlags: [],
    exits: [{ dir: backDir, toVnum: fromVnum }]
  };
  const nextSourceExits = [...sourceExits, { dir: dirKey, toVnum: nextVnum }];
  const currentRooms = Array.isArray((areaData as Record<string, unknown>).rooms)
    ? [...((areaData as Record<string, unknown>).rooms as unknown[])]
    : [];
  const nextRooms = currentRooms.map((room) => {
    if (!room || typeof room !== "object") {
      return room;
    }
    const record = room as Record<string, unknown>;
    const vnum = parseVnum(record.vnum);
    if (vnum !== fromVnum) {
      return record;
    }
    return {
      ...record,
      exits: nextSourceExits
    };
  });
  nextRooms.push(newRoom);
  const nextAreaData = {
    ...areaData,
    rooms: nextRooms
  } as AreaJson;

  let layoutNode: RoomLayoutNode | undefined;
  if (!autoLayoutEnabled && sourceNodePosition) {
    const stepX = roomNodeSize.width + 110;
    const stepY = roomNodeSize.height + 110;
    const offset =
      dirKey === "north" || dirKey === "up"
        ? { x: 0, y: -stepY }
        : dirKey === "south" || dirKey === "down"
          ? { x: 0, y: stepY }
          : dirKey === "east"
            ? { x: stepX, y: 0 }
            : dirKey === "west"
              ? { x: -stepX, y: 0 }
              : { x: 0, y: 0 };
    layoutNode = {
      id: String(nextVnum),
      type: "room",
      position: {
        x: sourceNodePosition.x + offset.x,
        y: sourceNodePosition.y + offset.y
      },
      data: {
        vnum: nextVnum,
        label: "New Room",
        sector: sectorType,
        upExitTargets: dirKey === "down" ? [fromVnum] : [],
        downExitTargets: dirKey === "up" ? [fromVnum] : []
      }
    };
  }

  return {
    ok: true,
    areaData: nextAreaData,
    nextVnum,
    message: `Dug ${dirKey} to room ${nextVnum} (unsaved)`,
    layoutNode
  };
};

export const linkRooms = (params: LinkRoomsParams): LinkRoomsResult => {
  const {
    areaData,
    fromVnum,
    direction,
    targetVnumInput,
    parseVnum,
    getEntityList,
    findByVnum
  } = params;
  if (!areaData) {
    return { ok: false, message: "Load an area before linking rooms." };
  }
  const dirKey = normalizeDirectionKey(direction);
  const backDir = oppositeDirectionMap[dirKey];
  if (!backDir) {
    return { ok: false, message: `Unknown direction: ${direction}` };
  }
  const targetVnum = Number.parseInt(targetVnumInput, 10);
  if (!Number.isFinite(targetVnum)) {
    return { ok: false, message: "Enter a valid target room VNUM." };
  }
  if (targetVnum === fromVnum) {
    return { ok: false, message: "Target room must be different from source." };
  }
  const rooms = getEntityList(areaData, "Rooms");
  const source = findByVnum(rooms, fromVnum);
  const target = findByVnum(rooms, targetVnum);
  if (!source || !target) {
    return { ok: false, message: "Both source and target rooms must exist." };
  }
  const sourceExits = Array.isArray(source.exits) ? source.exits : [];
  const targetExits = Array.isArray(target.exits) ? target.exits : [];
  if (hasExitDirection(sourceExits, dirKey)) {
    return {
      ok: false,
      message: `Room ${fromVnum} already has a ${dirKey} exit.`
    };
  }
  if (hasExitDirection(targetExits, backDir)) {
    return {
      ok: false,
      message: `Room ${targetVnum} already has a ${backDir} exit.`
    };
  }
  const nextSourceExits = [...sourceExits, { dir: dirKey, toVnum: targetVnum }];
  const nextTargetExits = [...targetExits, { dir: backDir, toVnum: fromVnum }];
  const currentRooms = Array.isArray((areaData as Record<string, unknown>).rooms)
    ? [...((areaData as Record<string, unknown>).rooms as unknown[])]
    : [];
  const nextRooms = currentRooms.map((room) => {
    if (!room || typeof room !== "object") {
      return room;
    }
    const record = room as Record<string, unknown>;
    const vnum = parseVnum(record.vnum);
    if (vnum === fromVnum) {
      return {
        ...record,
        exits: nextSourceExits
      };
    }
    if (vnum === targetVnum) {
      return {
        ...record,
        exits: nextTargetExits
      };
    }
    return record;
  });
  const nextAreaData = {
    ...areaData,
    rooms: nextRooms
  } as AreaJson;
  return {
    ok: true,
    areaData: nextAreaData,
    message: `Linked room ${fromVnum} ${dirKey} to ${targetVnum} (unsaved)`
  };
};
