import type { Edge, EdgeTypes, Node, NodeChange, NodeTypes } from "reactflow";
import ReactFlow, { Background, Controls } from "reactflow";
import { MapToolbar } from "./MapToolbar";
import { MapOverlay } from "./MapOverlay";
import { MapExternalExitsPanel } from "./MapExternalExitsPanel";

type MapViewProps = {
  mapNodes: Node[];
  roomEdges: Edge[];
  roomNodeTypes: NodeTypes;
  roomEdgeTypes: EdgeTypes;
  areaVnumRange: string | null;
  externalExits: Array<{
    fromVnum: number;
    fromName: string;
    direction: string;
    toVnum: number;
    areaName: string | null;
  }>;
  onNavigateExit: (vnum: number) => void;
  onNodeClick: (node: Node) => void;
  onNodesChange: (changes: NodeChange[]) => void;
  onNodeDragStop: (event: unknown, node: Node) => void;
  autoLayoutEnabled: boolean;
  preferCardinalLayout: boolean;
  showVerticalEdges: boolean;
  dirtyRoomCount: number;
  selectedRoomNode: boolean;
  selectedRoomLocked: boolean;
  hasRoomLayout: boolean;
  onRelayout: () => void;
  onToggleAutoLayout: (value: boolean) => void;
  onTogglePreferGrid: (value: boolean) => void;
  onToggleVerticalEdges: (value: boolean) => void;
  onLockSelected: () => void;
  onUnlockSelected: () => void;
  onClearLayout: () => void;
};

export function MapView({
  mapNodes,
  roomEdges,
  roomNodeTypes,
  roomEdgeTypes,
  areaVnumRange,
  externalExits,
  onNavigateExit,
  onNodeClick,
  onNodesChange,
  onNodeDragStop,
  autoLayoutEnabled,
  preferCardinalLayout,
  showVerticalEdges,
  dirtyRoomCount,
  selectedRoomNode,
  selectedRoomLocked,
  hasRoomLayout,
  onRelayout,
  onToggleAutoLayout,
  onTogglePreferGrid,
  onToggleVerticalEdges,
  onLockSelected,
  onUnlockSelected,
  onClearLayout
}: MapViewProps) {
  return (
    <div className="map-shell">
      <div className="map-canvas">
        <MapToolbar
          autoLayoutEnabled={autoLayoutEnabled}
          preferCardinalLayout={preferCardinalLayout}
          showVerticalEdges={showVerticalEdges}
          dirtyRoomCount={dirtyRoomCount}
          selectedRoomNode={selectedRoomNode}
          selectedRoomLocked={selectedRoomLocked}
          hasRoomLayout={hasRoomLayout}
          onRelayout={onRelayout}
          onToggleAutoLayout={onToggleAutoLayout}
          onTogglePreferGrid={onTogglePreferGrid}
          onToggleVerticalEdges={onToggleVerticalEdges}
          onLockSelected={onLockSelected}
          onUnlockSelected={onUnlockSelected}
          onClearLayout={onClearLayout}
        />
        <ReactFlow
          id="area-map"
          nodes={mapNodes}
          edges={roomEdges}
          nodeTypes={roomNodeTypes}
          edgeTypes={roomEdgeTypes}
          fitView
          nodesDraggable
          nodesConnectable={false}
          onNodesChange={onNodesChange}
          onNodeDragStop={onNodeDragStop}
          onNodeClick={(_, node) => onNodeClick(node)}
          panOnScroll
        >
          <Background gap={24} size={1} />
          <Controls showInteractive={false} />
        </ReactFlow>
        <MapOverlay areaVnumRange={areaVnumRange} />
      </div>
      <MapExternalExitsPanel
        externalExits={externalExits}
        onNavigate={onNavigateExit}
      />
    </div>
  );
}
