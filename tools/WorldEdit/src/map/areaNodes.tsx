import type { Node, NodeProps, NodeTypes } from "reactflow";
import { Handle, Position } from "reactflow";
import type { AreaLayoutEntry, EditorLayout } from "../repository/types";

export type AreaGraphNodeData = {
  label: string;
  range: string;
  isCurrent?: boolean;
  isMatch?: boolean;
  locked?: boolean;
  dirty?: boolean;
};

export type AreaLayoutMap = Record<string, AreaLayoutEntry>;

export const AreaGraphNode = ({ data, selected }: NodeProps<AreaGraphNodeData>) => {
  const classes = [
    "area-node",
    data.isCurrent ? "area-node--current" : "",
    data.isMatch ? "area-node--match" : "",
    data.locked ? "area-node--locked" : "",
    data.dirty ? "area-node--dirty" : "",
    selected ? "area-node--selected" : ""
  ]
    .filter(Boolean)
    .join(" ");
  return (
    <div className={classes}>
      <Handle
        type="target"
        id="north-in"
        position={Position.Top}
        className="area-node__handle"
      />
      <Handle
        type="source"
        id="north-out"
        position={Position.Top}
        className="area-node__handle"
      />
      <Handle
        type="target"
        id="east-in"
        position={Position.Right}
        className="area-node__handle"
      />
      <Handle
        type="source"
        id="east-out"
        position={Position.Right}
        className="area-node__handle"
      />
      <div className="area-node__title">{data.label}</div>
      <div className="area-node__range">{data.range}</div>
      <Handle
        type="source"
        id="south-out"
        position={Position.Bottom}
        className="area-node__handle"
      />
      <Handle
        type="target"
        id="south-in"
        position={Position.Bottom}
        className="area-node__handle"
      />
      <Handle
        type="target"
        id="west-in"
        position={Position.Left}
        className="area-node__handle"
      />
      <Handle
        type="source"
        id="west-out"
        position={Position.Left}
        className="area-node__handle"
      />
    </div>
  );
};

export const areaNodeTypes: NodeTypes = { area: AreaGraphNode };

export const extractAreaLayout = (
  layout: EditorLayout | null | undefined
): AreaLayoutMap => {
  const areas =
    layout && typeof layout === "object" && "areas" in layout
      ? layout.areas
      : undefined;
  if (!areas || typeof areas !== "object") {
    return {};
  }
  const entries: AreaLayoutMap = {};
  Object.entries(areas).forEach(([key, value]) => {
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

export const applyAreaLayoutOverrides = (
  nodes: Node<AreaGraphNodeData>[],
  layout: AreaLayoutMap,
  dirtyNodes: Set<string>
): Node<AreaGraphNodeData>[] =>
  nodes.map((node) => {
    const override = layout[node.id];
    const isLocked = override?.locked === true;
    const isDirty = dirtyNodes.has(node.id);
    return {
      ...node,
      position: isLocked
        ? {
            x: override.x,
            y: override.y
          }
        : node.position,
      draggable: !isLocked,
      data: {
        ...node.data,
        locked: isLocked,
        dirty: isDirty
      }
    };
  });
