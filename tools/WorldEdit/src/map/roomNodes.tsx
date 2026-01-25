import type { MouseEvent } from "react";
import type { Node, NodeProps } from "reactflow";
import { Handle, Position } from "reactflow";
import type { AreaJson, EditorLayout, RoomLayoutEntry } from "../repository/types";
import { getFirstString, parseVnum } from "../crud/area/utils";

export type RoomNodeData = {
  vnum: number;
  label: string;
  sector: string;
  dirty?: boolean;
  locked?: boolean;
  upExitTargets?: number[];
  downExitTargets?: number[];
  onNavigate?: (vnum: number) => void;
  onContextMenu?: (event: MouseEvent<HTMLDivElement>, vnum: number) => void;
  grid?: {
    x: number;
    y: number;
  };
};

export type RoomLayoutMap = Record<string, RoomLayoutEntry>;

export const RoomNode = ({ data, selected }: NodeProps<RoomNodeData>) => {
  const upTargets = data.upExitTargets ?? [];
  const downTargets = data.downExitTargets ?? [];
  const handleNavigate =
    (targets: number[]) => (event: MouseEvent<HTMLButtonElement>) => {
      event.stopPropagation();
      if (!targets.length) {
        return;
      }
      data.onNavigate?.(targets[0]);
    };
  const handleContextMenu = (event: MouseEvent<HTMLDivElement>) => {
    event.preventDefault();
    event.stopPropagation();
    data.onContextMenu?.(event, data.vnum);
  };
  const buildTitle = (label: string, targets: number[]) =>
    targets.length
      ? `${label} to ${targets.join(", ")}`
      : `${label} exit`;
  return (
    <div
      className={`room-node${selected ? " room-node--selected" : ""}`}
      onContextMenu={handleContextMenu}
    >
      {data.dirty ? (
        <div className="room-node__dirty" title="Position moved but not locked">
          Dirty
        </div>
      ) : null}
      <Handle
        id="north-out"
        type="source"
        position={Position.Top}
        className="room-node__handle"
      />
      <Handle
        id="north-in"
        type="target"
        position={Position.Top}
        className="room-node__handle"
      />
      <Handle
        id="east-out"
        type="source"
        position={Position.Right}
        className="room-node__handle"
      />
      <Handle
        id="east-in"
        type="target"
        position={Position.Right}
        className="room-node__handle"
      />
      <Handle
        id="south-out"
        type="source"
        position={Position.Bottom}
        className="room-node__handle"
      />
      <Handle
        id="south-in"
        type="target"
        position={Position.Bottom}
        className="room-node__handle"
      />
      <Handle
        id="west-out"
        type="source"
        position={Position.Left}
        className="room-node__handle"
      />
      <Handle
        id="west-in"
        type="target"
        position={Position.Left}
        className="room-node__handle"
      />
      {upTargets.length || downTargets.length ? (
        <div className="room-node__exits">
          {upTargets.length ? (
            <button
              className="room-node__exit-button room-node__exit-button--up"
              type="button"
              onClick={handleNavigate(upTargets)}
              title={buildTitle("Up exit", upTargets)}
            >
              <span className="room-node__exit-icon">⬆</span>
              {upTargets.length > 1 ? (
                <span className="room-node__exit-count">{upTargets.length}</span>
              ) : null}
            </button>
          ) : null}
          {downTargets.length ? (
            <button
              className="room-node__exit-button room-node__exit-button--down"
              type="button"
              onClick={handleNavigate(downTargets)}
              title={buildTitle("Down exit", downTargets)}
            >
              <span className="room-node__exit-icon">⬇</span>
              {downTargets.length > 1 ? (
                <span className="room-node__exit-count">{downTargets.length}</span>
              ) : null}
            </button>
          ) : null}
        </div>
      ) : null}
      <div className="room-node__vnum">{data.vnum}</div>
      <div className="room-node__name">{data.label}</div>
      <div className="room-node__sector">{data.sector}</div>
    </div>
  );
};

export const roomNodeTypes = { room: RoomNode };

export const buildRoomNodes = (areaData: AreaJson | null): Node<RoomNodeData>[] => {
  if (!areaData) {
    return [];
  }
  const rooms = Array.isArray((areaData as Record<string, unknown>).rooms)
    ? ((areaData as Record<string, unknown>).rooms as unknown[])
    : [];
  if (!rooms.length) {
    return [];
  }
  const roomVnums = new Set<number>();
  rooms.forEach((room) => {
    if (!room || typeof room !== "object") {
      return;
    }
    const vnum = parseVnum((room as Record<string, unknown>).vnum);
    if (vnum !== null) {
      roomVnums.add(vnum);
    }
  });
  const columns = 6;
  const spacingX = 220;
  const spacingY = 140;
  return rooms.map((room, index) => {
    const record = room as Record<string, unknown>;
    const vnum = parseVnum(record.vnum);
    const name = getFirstString(record.name, "(unnamed room)");
    const sector =
      typeof record.sectorType === "string"
        ? record.sectorType
        : typeof record.sector === "string"
          ? record.sector
          : "unknown";
    const exits = Array.isArray(record.exits) ? record.exits : [];
    const upExitTargets: number[] = [];
    const downExitTargets: number[] = [];
    exits.forEach((exit) => {
      if (!exit || typeof exit !== "object") {
        return;
      }
      const exitRecord = exit as Record<string, unknown>;
      const dir = typeof exitRecord.dir === "string" ? exitRecord.dir : "";
      const dirKey = dir.trim().toLowerCase();
      if (dirKey !== "up" && dirKey !== "down") {
        return;
      }
      const targetVnum = parseVnum(exitRecord.toVnum);
      if (targetVnum === null || !roomVnums.has(targetVnum)) {
        return;
      }
      if (dirKey === "up") {
        upExitTargets.push(targetVnum);
      } else {
        downExitTargets.push(targetVnum);
      }
    });
    const col = index % columns;
    const row = Math.floor(index / columns);
    return {
      id: vnum !== null ? String(vnum) : `room-${index}`,
      type: "room",
      position: { x: col * spacingX, y: row * spacingY },
      data: {
        vnum: vnum ?? -1,
        label: name,
        sector,
        upExitTargets,
        downExitTargets
      }
    };
  });
};

export const applyRoomSelection = (
  nodes: Node<RoomNodeData>[],
  selectedVnum: number | null
): Node<RoomNodeData>[] => {
  if (selectedVnum === null) {
    return nodes.map((node) => ({ ...node, selected: false }));
  }
  return nodes.map((node) => ({
    ...node,
    selected: node.data.vnum === selectedVnum
  }));
};

export const extractRoomLayout = (
  layout: EditorLayout | null | undefined
): RoomLayoutMap => {
  const rooms =
    layout && typeof layout === "object" && "rooms" in layout
      ? layout.rooms
      : undefined;
  if (!rooms || typeof rooms !== "object") {
    return {};
  }
  const entries: RoomLayoutMap = {};
  Object.entries(rooms).forEach(([key, value]) => {
    if (!value || typeof value !== "object") {
      return;
    }
    const record = value as Record<string, unknown>;
    const x = record.x;
    const y = record.y;
    if (typeof x !== "number" || typeof y !== "number") {
      return;
    }
    if (!Number.isFinite(x) || !Number.isFinite(y)) {
      return;
    }
    if (!record.locked) {
      return;
    }
    entries[key] = {
      x,
      y,
      locked: true
    };
  });
  return entries;
};

export const applyRoomLayoutOverrides = (
  nodes: Node<RoomNodeData>[],
  layout: RoomLayoutMap
): Node<RoomNodeData>[] => {
  if (!Object.keys(layout).length) {
    return nodes.map((node) => ({
      ...node,
      draggable: true,
      data: {
        ...node.data,
        locked: false
      }
    }));
  }
  return nodes.map((node) => {
    const override = layout[node.id];
    const isLocked = override?.locked === true;
    return {
      ...node,
      position: isLocked ? { x: override.x, y: override.y } : node.position,
      draggable: !isLocked,
      data: {
        ...node.data,
        locked: isLocked
      }
    };
  });
};
