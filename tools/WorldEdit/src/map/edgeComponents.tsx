/**
 * Custom edge components for React Flow map visualization.
 *
 * Provides specialized edge rendering for:
 * - Room-to-room connections with obstacle avoidance
 * - Vertical (up/down) connections
 * - External stub edges pointing to other areas
 */

import { useMemo } from "react";
import {
  BaseEdge,
  EdgeLabelRenderer,
  Position,
  type EdgeProps,
  useStore
} from "reactflow";
import { buildOrthogonalEdgePath, offsetPoint } from "./edgeRouting";
import { areaNodeSize, roomNodeSize } from "./graphLayout";
import {
  edgeStubSize,
  edgeClearance,
  externalStubSize,
  edgeDirectionPriority,
  externalDirectionMap,
  externalDirectionLabels
} from "../constants/appConfig";

/** Data payload for room edges. */
export type RoomEdgeData = {
  dirKey?: string;
  exitFlags?: string[];
  invalid?: boolean;
  nodeType?: string;
  nodeSize?: { width: number; height: number };
};

/** Data payload for external stub edges. */
export type ExternalStubEdgeData = {
  dirKey?: string;
  targetVnum?: number;
  invalid?: boolean;
};

/** Data payload for vertical edges. */
export type VerticalEdgeData = {
  dirKey?: string;
};

/**
 * Room edge component with orthogonal path routing and obstacle avoidance.
 * Renders doors/locked exits with different styles.
 */
export function RoomEdge({
  id,
  sourceX,
  sourceY,
  targetX,
  targetY,
  source,
  target,
  sourcePosition,
  targetPosition,
  data,
  label,
  selected,
  markerEnd,
  interactionWidth
}: EdgeProps<RoomEdgeData>) {
  const nodeInternals = useStore((state) => state.nodeInternals);
  const nodeType =
    data?.nodeType && typeof data.nodeType === "string"
      ? data.nodeType
      : "room";
  const fallbackSize =
    data?.nodeSize ??
    (nodeType === "area" ? areaNodeSize : roomNodeSize);
  const obstacles = useMemo(() => {
    const entries = Array.from(nodeInternals.values());
    return entries
      .filter((node) => node.type === nodeType)
      .map((node) => {
        const width = node.width ?? fallbackSize.width;
        const height = node.height ?? fallbackSize.height;
        const position = node.positionAbsolute ?? node.position;
        return {
          id: node.id,
          left: position.x,
          right: position.x + width,
          top: position.y,
          bottom: position.y + height
        };
      });
  }, [nodeInternals, nodeType, fallbackSize]);
  const filteredObstacles = useMemo(
    () =>
      obstacles.filter(
        (obstacle) => obstacle.id !== source && obstacle.id !== target
      ),
    [obstacles, source, target]
  );
  const safeSourcePosition = sourcePosition ?? Position.Right;
  const safeTargetPosition = targetPosition ?? Position.Left;
  const { path, labelPoint } = useMemo(
    () =>
      buildOrthogonalEdgePath({
        start: { x: sourceX, y: sourceY },
        end: { x: targetX, y: targetY },
        sourceDirection: safeSourcePosition,
        targetDirection: safeTargetPosition,
        obstacles: filteredObstacles,
        edgeStubSize,
        edgeClearance,
        edgeDirectionPriority
      }),
    [
      sourceX,
      sourceY,
      targetX,
      targetY,
      safeSourcePosition,
      safeTargetPosition,
      filteredObstacles
    ]
  );
  const exitFlags = Array.isArray(data?.exitFlags) ? data.exitFlags : [];
  const isInvalid = data?.invalid === true;
  const isLocked = exitFlags.includes("locked");
  const isDoor = isLocked || exitFlags.includes("door") || exitFlags.includes("closed");
  const stroke = isInvalid
    ? "rgba(167, 60, 60, 0.9)"
    : isLocked
      ? "rgba(167, 60, 60, 0.85)"
      : isDoor
        ? "rgba(138, 92, 34, 0.75)"
        : "rgba(47, 108, 106, 0.55)";
  const edgeStyle = {
    stroke,
    strokeWidth: selected
      ? 2.4
      : isInvalid
        ? 2.2
        : isLocked
          ? 2.1
          : isDoor
            ? 1.8
            : 1.4,
    strokeDasharray: isInvalid
      ? "3 3"
      : isLocked
        ? "4 3"
        : isDoor
          ? "6 4"
          : undefined
  };
  const labelClass = isInvalid
    ? "room-edge__label room-edge__label--invalid"
    : isLocked
      ? "room-edge__label room-edge__label--locked"
      : isDoor
        ? "room-edge__label room-edge__label--door"
        : "room-edge__label";
  return (
    <>
      <BaseEdge
        id={id}
        path={path}
        markerEnd={markerEnd}
        style={edgeStyle}
        interactionWidth={interactionWidth}
      />
      {typeof label === "string" && label.length ? (
        <EdgeLabelRenderer>
          <div
            className={labelClass}
            style={{
              transform: `translate(-50%, -50%) translate(${labelPoint.x}px, ${labelPoint.y}px)`
            }}
          >
            {label}
          </div>
        </EdgeLabelRenderer>
      ) : null}
    </>
  );
}

/**
 * Vertical edge for up/down connections.
 * Renders as a simple dashed line.
 */
export function VerticalEdge({
  id,
  sourceX,
  sourceY,
  targetX,
  targetY,
  markerEnd,
  interactionWidth
}: EdgeProps<VerticalEdgeData>) {
  const path = `M${sourceX} ${sourceY} L${targetX} ${targetY}`;
  return (
    <BaseEdge
      id={id}
      path={path}
      markerEnd={markerEnd}
      interactionWidth={interactionWidth}
      style={{
        stroke: "rgba(47, 108, 106, 0.4)",
        strokeWidth: 1.2,
        strokeDasharray: "4 5"
      }}
    />
  );
}

/**
 * External stub edge for exits leading to other areas.
 * Renders as a short stub with a label showing the direction and target VNUM.
 */
export function ExternalStubEdge({
  id,
  sourceX,
  sourceY,
  sourcePosition,
  data,
  markerEnd,
  interactionWidth
}: EdgeProps<ExternalStubEdgeData>) {
  const dirKey = data?.dirKey ?? "";
  const rawLabel =
    externalDirectionLabels[dirKey] ?? dirKey.trim().toUpperCase();
  const labelCode = rawLabel.length ? rawLabel : "X";
  const labelSuffix =
    typeof data?.targetVnum === "number" ? ` ${data.targetVnum}` : "";
  const isInvalid = data?.invalid === true;
  const direction =
    externalDirectionMap[dirKey] ?? sourcePosition ?? Position.Right;
  const start = { x: sourceX, y: sourceY };
  const end = offsetPoint(start, direction, externalStubSize);
  const labelPoint = offsetPoint(start, direction, externalStubSize + 16);
  const path = `M${start.x} ${start.y} L${end.x} ${end.y}`;
  return (
    <>
      <BaseEdge
        id={id}
        path={path}
        markerEnd={markerEnd}
        interactionWidth={interactionWidth}
        style={{
          stroke: isInvalid ? "rgba(167, 60, 60, 0.7)" : "rgba(47, 108, 106, 0.35)",
          strokeWidth: isInvalid ? 1.6 : 1.1,
          strokeDasharray: isInvalid ? "3 4" : "2 6"
        }}
      />
      <EdgeLabelRenderer>
        <div
          className={`room-edge__label room-edge__label--external${
            isInvalid ? " room-edge__label--invalid" : ""
          }`}
          style={{
            transform: `translate(-50%, -50%) translate(${labelPoint.x}px, ${labelPoint.y}px)`
          }}
        >
          {labelCode}
          {labelSuffix}
        </div>
      </EdgeLabelRenderer>
    </>
  );
}

/** Edge type mapping for room maps. */
export const roomEdgeTypes = {
  room: RoomEdge,
  vertical: VerticalEdge,
  external: ExternalStubEdge
};

/** Edge type mapping for area graphs. */
export const areaEdgeTypes = {
  area: RoomEdge
};
