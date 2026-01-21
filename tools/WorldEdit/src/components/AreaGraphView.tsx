import { useEffect, useState } from "react";
import type {
  Edge,
  EdgeTypes,
  Node,
  NodeChange,
  NodeProps,
  NodeTypes,
  ReactFlowInstance
} from "reactflow";
import ReactFlow, {
  Background,
  Controls,
  Handle,
  Position
} from "reactflow";

type AreaGraphNodeData = {
  label: string;
  range: string;
  isCurrent?: boolean;
  isMatch?: boolean;
  locked?: boolean;
  dirty?: boolean;
};

type AreaGraphViewProps = {
  nodes: Node<AreaGraphNodeData>[];
  edges: Edge[];
  edgeTypes?: EdgeTypes;
  nodesDraggable?: boolean;
  dirtyCount?: number;
  selectedNodeLocked?: boolean;
  hasLayout?: boolean;
  filterValue: string;
  vnumQuery: string;
  matchLabel: string | null;
  onFilterChange: (value: string) => void;
  onVnumQueryChange: (value: string) => void;
  onNodeClick: (node: Node<AreaGraphNodeData>) => void;
  onNodesChange?: (changes: NodeChange[]) => void;
  onNodeDragStop?: (event: unknown, node: Node<AreaGraphNodeData>) => void;
  onLockSelected?: () => void;
  onUnlockSelected?: () => void;
  onLockDirty?: () => void;
  onClearLayout?: () => void;
};

function AreaGraphNode({ data, selected }: NodeProps<AreaGraphNodeData>) {
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
}

const nodeTypes: NodeTypes = { area: AreaGraphNode };

export function AreaGraphView({
  nodes,
  edges,
  edgeTypes,
  nodesDraggable = false,
  dirtyCount = 0,
  selectedNodeLocked = false,
  hasLayout = false,
  filterValue,
  vnumQuery,
  matchLabel,
  onFilterChange,
  onVnumQueryChange,
  onNodeClick,
  onNodesChange,
  onNodeDragStop,
  onLockSelected,
  onUnlockSelected,
  onLockDirty,
  onClearLayout
}: AreaGraphViewProps) {
  const [flowInstance, setFlowInstance] = useState<ReactFlowInstance | null>(
    null
  );
  const safeNodes = Array.isArray(nodes) ? nodes : [];
  const safeEdges = Array.isArray(edges) ? edges : [];
  const nodeCount = safeNodes.length;
  const edgeCount = safeEdges.length;
  const hasSelection = safeNodes.some((node) => node.selected);
  useEffect(() => {
    if (!flowInstance || !nodeCount) {
      return;
    }
    const handle = requestAnimationFrame(() => {
      flowInstance.fitView({ padding: 0.2 });
    });
    return () => cancelAnimationFrame(handle);
  }, [flowInstance, nodeCount, edgeCount]);

  return (
    <div className="map-shell">
      <div className="map-canvas">
        <div className="map-toolbar area-graph-toolbar">
          <label className="map-filter">
            <span>Filter</span>
            <input
              className="form-input form-input--compact"
              type="text"
              value={filterValue}
              onChange={(event) => onFilterChange(event.target.value)}
              placeholder="Area name"
            />
          </label>
          <label className="map-filter">
            <span>VNUM</span>
            <input
              className="form-input form-input--compact"
              type="number"
              value={vnumQuery}
              onChange={(event) => onVnumQueryChange(event.target.value)}
              placeholder="Search"
            />
          </label>
          {vnumQuery ? (
            <span className="map-pill">
              {matchLabel ? `Found: ${matchLabel}` : "No match"}
            </span>
          ) : null}
          {dirtyCount > 0 ? (
            <span className="map-pill">{dirtyCount} dirty</span>
          ) : null}
          <div className="map-actions">
            <button
              className="ghost-button"
              type="button"
              onClick={onLockSelected}
              disabled={!hasSelection || selectedNodeLocked}
            >
              Lock
            </button>
            <button
              className="ghost-button"
              type="button"
              onClick={onUnlockSelected}
              disabled={!hasSelection || !selectedNodeLocked}
            >
              Unlock
            </button>
            <button
              className="ghost-button"
              type="button"
              onClick={onLockDirty}
              disabled={dirtyCount === 0}
            >
              Lock Dirty
            </button>
            <button
              className="ghost-button"
              type="button"
              onClick={onClearLayout}
              disabled={!hasLayout}
            >
              Clear Layout
            </button>
          </div>
          <span className="map-pill">
            {nodeCount} areas · {edgeCount} links
          </span>
        </div>
        <ReactFlow
          id="world-graph"
          nodes={safeNodes}
          edges={safeEdges}
          nodeTypes={nodeTypes}
          edgeTypes={edgeTypes}
          defaultEdgeOptions={{
            style: { stroke: "rgba(47, 108, 106, 0.55)", strokeWidth: 1.4 }
          }}
          fitView
          nodesDraggable={nodesDraggable}
          nodesConnectable={false}
          panOnScroll
          onInit={setFlowInstance}
          onNodeClick={(_, node) => onNodeClick(node)}
          onNodesChange={onNodesChange}
          onNodeDragStop={onNodeDragStop}
        >
          <Background gap={24} size={1} />
          <Controls showInteractive={false} />
        </ReactFlow>
      </div>
    </div>
  );
}
