import { useEffect, useState } from "react";
import type {
  Edge,
  Node,
  NodeProps,
  NodeTypes,
  ReactFlowInstance
} from "reactflow";
import ReactFlow, { Background, Controls, Handle, Position } from "reactflow";

type AreaGraphNodeData = {
  label: string;
  range: string;
  isCurrent?: boolean;
  isMatch?: boolean;
};

type AreaGraphViewProps = {
  nodes: Node<AreaGraphNodeData>[];
  edges: Edge[];
  filterValue: string;
  vnumQuery: string;
  matchLabel: string | null;
  onFilterChange: (value: string) => void;
  onVnumQueryChange: (value: string) => void;
  onNodeClick: (node: Node<AreaGraphNodeData>) => void;
};

function AreaGraphNode({ data, selected }: NodeProps<AreaGraphNodeData>) {
  const classes = [
    "area-node",
    data.isCurrent ? "area-node--current" : "",
    data.isMatch ? "area-node--match" : "",
    selected ? "area-node--selected" : ""
  ]
    .filter(Boolean)
    .join(" ");
  return (
    <div className={classes}>
      <Handle
        type="target"
        position={Position.Left}
        className="area-node__handle"
      />
      <div className="area-node__title">{data.label}</div>
      <div className="area-node__range">{data.range}</div>
      <Handle
        type="source"
        position={Position.Right}
        className="area-node__handle"
      />
    </div>
  );
}

const nodeTypes: NodeTypes = { area: AreaGraphNode };

export function AreaGraphView({
  nodes,
  edges,
  filterValue,
  vnumQuery,
  matchLabel,
  onFilterChange,
  onVnumQueryChange,
  onNodeClick
}: AreaGraphViewProps) {
  const [flowInstance, setFlowInstance] = useState<ReactFlowInstance | null>(
    null
  );

  useEffect(() => {
    if (!flowInstance || !nodes.length) {
      return;
    }
    flowInstance.fitView({ padding: 0.2 });
  }, [flowInstance, nodes, edges]);

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
          <span className="map-pill">
            {nodes.length} areas · {edges.length} links
          </span>
        </div>
        <ReactFlow
          id="world-graph"
          nodes={nodes}
          edges={edges}
          nodeTypes={nodeTypes}
          defaultEdgeOptions={{
            style: { stroke: "rgba(47, 108, 106, 0.55)", strokeWidth: 1.4 }
          }}
          fitView
          nodesDraggable={false}
          nodesConnectable={false}
          panOnScroll
          onInit={setFlowInstance}
          onNodeClick={(_, node) => onNodeClick(node)}
        >
          <Background gap={24} size={1} />
          <Controls showInteractive={false} />
        </ReactFlow>
      </div>
    </div>
  );
}
