import { useEffect, useState } from "react";
import type {
  Edge,
  EdgeTypes,
  Node,
  NodeChange,
  ReactFlowInstance
} from "reactflow";
import ReactFlow, { Background, Controls } from "reactflow";
import { areaNodeTypes, type AreaGraphNodeData } from "../map/areaNodes";

type AreaGraphViewProps = {
  nodes: Node<AreaGraphNodeData>[];
  edges: Edge[];
  edgeTypes?: EdgeTypes;
  nodesDraggable?: boolean;
  dirtyCount?: number;
  selectedNodeLocked?: boolean;
  hasLayout?: boolean;
  preferCardinalLayout?: boolean;
  filterValue: string;
  vnumQuery: string;
  matchLabel: string | null;
  onFilterChange: (value: string) => void;
  onVnumQueryChange: (value: string) => void;
  onNodeClick: (node: Node<AreaGraphNodeData>) => void;
  onNodesChange?: (changes: NodeChange[]) => void;
  onNodeDragStop?: (event: unknown, node: Node<AreaGraphNodeData>) => void;
  onTogglePreferGrid?: (value: boolean) => void;
  onRelayout?: () => void;
  onLockSelected?: () => void;
  onUnlockSelected?: () => void;
  onLockDirty?: () => void;
  onClearLayout?: () => void;
};

export function AreaGraphView({
  nodes,
  edges,
  edgeTypes,
  nodesDraggable = false,
  dirtyCount = 0,
  selectedNodeLocked = false,
  hasLayout = false,
  preferCardinalLayout,
  filterValue,
  vnumQuery,
  matchLabel,
  onFilterChange,
  onVnumQueryChange,
  onNodeClick,
  onNodesChange,
  onNodeDragStop,
  onTogglePreferGrid,
  onRelayout,
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
          {typeof preferCardinalLayout === "boolean" && onTogglePreferGrid ? (
            <label className="map-toggle">
              <input
                type="checkbox"
                checked={preferCardinalLayout}
                onChange={(event) => onTogglePreferGrid(event.target.checked)}
              />
              <span>Prefer Grid</span>
            </label>
          ) : null}
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
              onClick={onRelayout}
              disabled={!nodeCount}
            >
              Relayout
            </button>
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
          nodeTypes={areaNodeTypes}
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
