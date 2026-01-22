import ELK from "elkjs/lib/elk.bundled.js";
import type { Edge, Node } from "reactflow";

type GridPosition = { x: number; y: number };

const elk = new ELK();

export const roomNodeSize = {
  width: 180,
  height: 180
};

export const areaNodeSize = {
  width: 180,
  height: 180
};

export const roomPortDefinitions = [
  { id: "north-in", side: "NORTH" },
  { id: "north-out", side: "NORTH" },
  { id: "east-in", side: "EAST" },
  { id: "east-out", side: "EAST" },
  { id: "south-in", side: "SOUTH" },
  { id: "south-out", side: "SOUTH" },
  { id: "west-in", side: "WEST" },
  { id: "west-out", side: "WEST" }
] as const;

export const areaPortDefinitions = roomPortDefinitions;

const directionOffsets: Record<string, GridPosition> = {
  north: { x: 0, y: -1 },
  east: { x: 1, y: 0 },
  south: { x: 0, y: 1 },
  west: { x: -1, y: 0 }
};

const oppositeDirections: Record<string, string> = {
  north: "south",
  east: "west",
  south: "north",
  west: "east"
};

function getEdgeDirKey(edge: Edge): string | null {
  if (
    typeof edge.data === "object" &&
    edge.data &&
    "dirKey" in edge.data &&
    typeof (edge.data as { dirKey?: unknown }).dirKey === "string"
  ) {
    return String((edge.data as { dirKey?: string }).dirKey);
  }
  if (typeof edge.label === "string") {
    const labelKey = edge.label.trim().toLowerCase();
    return labelKey.length ? labelKey : null;
  }
  return null;
}

function getAreaGridDirection(dirKey: string | null): string | null {
  if (!dirKey) {
    return null;
  }
  if (dirKey in directionOffsets) {
    return dirKey;
  }
  if (dirKey === "up") {
    return "north";
  }
  if (dirKey === "down") {
    return "south";
  }
  return null;
}

function findOpenGridCoord(
  start: GridPosition,
  occupied: Map<string, string>
): GridPosition {
  const keyFor = (pos: GridPosition) => `${pos.x},${pos.y}`;
  if (!occupied.has(keyFor(start))) {
    return start;
  }
  for (let radius = 1; radius <= 12; radius += 1) {
    for (let dx = -radius; dx <= radius; dx += 1) {
      for (let dy = -radius; dy <= radius; dy += 1) {
        if (Math.abs(dx) !== radius && Math.abs(dy) !== radius) {
          continue;
        }
        const candidate = { x: start.x + dx, y: start.y + dy };
        if (!occupied.has(keyFor(candidate))) {
          return candidate;
        }
      }
    }
  }
  return start;
}

export function layoutAreaGraphNodesGrid<T>(
  nodes: Node<T>[],
  edges: Edge[]
): Node<T>[] {
  if (!nodes.length) {
    return [];
  }
  const nodeIds = new Set(nodes.map((node) => node.id));
  const adjacency = new Map<string, Array<{ id: string; dir: string }>>();
  const addAdjacency = (from: string, to: string, dir: string) => {
    if (!adjacency.has(from)) {
      adjacency.set(from, []);
    }
    adjacency.get(from)?.push({ id: to, dir });
  };
  edges.forEach((edge) => {
    const dirKey = getAreaGridDirection(getEdgeDirKey(edge));
    if (!dirKey || !(dirKey in directionOffsets)) {
      return;
    }
    if (!nodeIds.has(edge.source) || !nodeIds.has(edge.target)) {
      return;
    }
    addAdjacency(edge.source, edge.target, dirKey);
    const opposite = oppositeDirections[dirKey];
    if (opposite) {
      addAdjacency(edge.target, edge.source, opposite);
    }
  });
  const sortedNodeIds = [...nodeIds].sort((a, b) => {
    const aNum = Number.parseInt(a, 10);
    const bNum = Number.parseInt(b, 10);
    if (Number.isNaN(aNum) || Number.isNaN(bNum)) {
      return a.localeCompare(b);
    }
    return aNum - bNum;
  });
  const positions = new Map<string, GridPosition>();
  const gridSpacingX = areaNodeSize.width + 110;
  const gridSpacingY = areaNodeSize.height + 110;
  const componentGap = 2;
  let componentOffsetX = 0;

  sortedNodeIds.forEach((startId) => {
    if (positions.has(startId)) {
      return;
    }
    const queue: string[] = [startId];
    const componentPositions = new Map<string, GridPosition>();
    const occupied = new Map<string, string>();
    const startPos = { x: 0, y: 0 };
    componentPositions.set(startId, startPos);
    occupied.set(`${startPos.x},${startPos.y}`, startId);

    while (queue.length) {
      const current = queue.shift();
      if (!current) {
        break;
      }
      const currentPos = componentPositions.get(current);
      if (!currentPos) {
        continue;
      }
      const neighbors = adjacency.get(current) ?? [];
      neighbors.forEach((neighbor) => {
        if (componentPositions.has(neighbor.id)) {
          return;
        }
        const offset = directionOffsets[neighbor.dir];
        if (!offset) {
          return;
        }
        let nextPos = {
          x: currentPos.x + offset.x,
          y: currentPos.y + offset.y
        };
        if (occupied.has(`${nextPos.x},${nextPos.y}`)) {
          nextPos = findOpenGridCoord(nextPos, occupied);
        }
        componentPositions.set(neighbor.id, nextPos);
        occupied.set(`${nextPos.x},${nextPos.y}`, neighbor.id);
        queue.push(neighbor.id);
      });
    }

    let minX = 0;
    let maxX = 0;
    let minY = 0;
    let maxY = 0;
    componentPositions.forEach((pos) => {
      minX = Math.min(minX, pos.x);
      maxX = Math.max(maxX, pos.x);
      minY = Math.min(minY, pos.y);
      maxY = Math.max(maxY, pos.y);
    });
    const shiftX = componentOffsetX - minX;
    const shiftY = -minY;
    componentPositions.forEach((pos, nodeId) => {
      positions.set(nodeId, {
        x: pos.x + shiftX,
        y: pos.y + shiftY
      });
    });
    componentOffsetX += maxX - minX + 1 + componentGap;
  });

  return nodes.map((node) => {
    const position = positions.get(node.id);
    if (!position) {
      return node;
    }
    return {
      ...node,
      position: {
        x: position.x * gridSpacingX,
        y: position.y * gridSpacingY
      }
    };
  });
}

export function layoutRoomNodesGrid<T extends { grid?: GridPosition }>(
  nodes: Node<T>[],
  edges: Edge[]
): Node<T>[] {
  if (!nodes.length) {
    return [];
  }
  const nodeIds = new Set(nodes.map((node) => node.id));
  const adjacency = new Map<string, Array<{ id: string; dir: string }>>();
  const addAdjacency = (from: string, to: string, dir: string) => {
    if (!adjacency.has(from)) {
      adjacency.set(from, []);
    }
    adjacency.get(from)?.push({ id: to, dir });
  };
  edges.forEach((edge) => {
    const dirKey = getEdgeDirKey(edge);
    if (!dirKey || !(dirKey in directionOffsets)) {
      return;
    }
    if (!nodeIds.has(edge.source) || !nodeIds.has(edge.target)) {
      return;
    }
    addAdjacency(edge.source, edge.target, dirKey);
    const opposite = oppositeDirections[dirKey];
    if (opposite) {
      addAdjacency(edge.target, edge.source, opposite);
    }
  });
  const sortedNodeIds = [...nodeIds].sort((a, b) => {
    const aNum = Number.parseInt(a, 10);
    const bNum = Number.parseInt(b, 10);
    if (Number.isNaN(aNum) || Number.isNaN(bNum)) {
      return a.localeCompare(b);
    }
    return aNum - bNum;
  });
  const positions = new Map<string, GridPosition>();
  const gridSpacingX = roomNodeSize.width + 110;
  const gridSpacingY = roomNodeSize.height + 110;
  const componentGap = 2;
  let componentOffsetX = 0;

  sortedNodeIds.forEach((startId) => {
    if (positions.has(startId)) {
      return;
    }
    const queue: string[] = [startId];
    const componentPositions = new Map<string, GridPosition>();
    const occupied = new Map<string, string>();
    const startPos = { x: 0, y: 0 };
    componentPositions.set(startId, startPos);
    occupied.set(`${startPos.x},${startPos.y}`, startId);

    while (queue.length) {
      const current = queue.shift();
      if (!current) {
        break;
      }
      const currentPos = componentPositions.get(current);
      if (!currentPos) {
        continue;
      }
      const neighbors = adjacency.get(current) ?? [];
      neighbors.forEach((neighbor) => {
        if (componentPositions.has(neighbor.id)) {
          return;
        }
        const offset = directionOffsets[neighbor.dir];
        if (!offset) {
          return;
        }
        let nextPos = {
          x: currentPos.x + offset.x,
          y: currentPos.y + offset.y
        };
        if (occupied.has(`${nextPos.x},${nextPos.y}`)) {
          nextPos = findOpenGridCoord(nextPos, occupied);
        }
        componentPositions.set(neighbor.id, nextPos);
        occupied.set(`${nextPos.x},${nextPos.y}`, neighbor.id);
        queue.push(neighbor.id);
      });
    }

    let minX = 0;
    let maxX = 0;
    let minY = 0;
    let maxY = 0;
    componentPositions.forEach((pos) => {
      minX = Math.min(minX, pos.x);
      maxX = Math.max(maxX, pos.x);
      minY = Math.min(minY, pos.y);
      maxY = Math.max(maxY, pos.y);
    });
    const shiftX = componentOffsetX - minX;
    const shiftY = -minY;
    componentPositions.forEach((pos, nodeId) => {
      positions.set(nodeId, {
        x: pos.x + shiftX,
        y: pos.y + shiftY
      });
    });
    componentOffsetX += maxX - minX + 1 + componentGap;
  });

  return nodes.map((node) => {
    const gridPos = positions.get(node.id);
    if (!gridPos) {
      return node;
    }
    return {
      ...node,
      data: {
        ...node.data,
        grid: gridPos
      },
      position: {
        x: gridPos.x * gridSpacingX,
        y: gridPos.y * gridSpacingY
      }
    };
  });
}

export async function layoutRoomNodes<T extends { grid?: GridPosition }>(
  nodes: Node<T>[],
  edges: Edge[],
  preferCardinalLayout: boolean
): Promise<Node<T>[]> {
  if (!nodes.length) {
    return [];
  }
  if (preferCardinalLayout) {
    return layoutRoomNodesGrid(nodes, edges);
  }
  const layoutEdges = edges.filter(
    (edge) => edge.sourceHandle && edge.targetHandle
  );
  const layoutOptions: Record<string, string> = {
    "elk.algorithm": "layered",
    "elk.direction": "RIGHT",
    "elk.spacing.nodeNode": "120",
    "elk.layered.spacing.nodeNodeBetweenLayers": "180",
    "elk.spacing.componentComponent": "160",
    "elk.spacing.edgeNode": "40",
    "elk.spacing.edgeEdge": "20",
    "elk.portConstraints": "FIXED_SIDE"
  };
  const graph = {
    id: "root",
    layoutOptions,
    children: nodes.map((node) => ({
      id: node.id,
      width: roomNodeSize.width,
      height: roomNodeSize.height,
      ports: roomPortDefinitions.map((port) => ({
        id: `${node.id}.${port.id}`,
        layoutOptions: {
          "elk.port.side": port.side
        }
      }))
    })),
    edges: layoutEdges.map((edge) => {
      const dirKey = getEdgeDirKey(edge);
      const directionPriority = dirKey === "east" ? "2" : "0";
      return {
        id: edge.id,
        sources: [
          edge.sourceHandle
            ? `${edge.source}.${edge.sourceHandle}`
            : edge.source
        ],
        targets: [
          edge.targetHandle
            ? `${edge.target}.${edge.targetHandle}`
            : edge.target
        ],
        layoutOptions: {
          "elk.layered.priority.direction": directionPriority
        }
      };
    })
  };
  const layout = await elk.layout(graph);
  const positions = new Map<string, GridPosition>();
  layout.children?.forEach((child) => {
    if (child.id && typeof child.x === "number" && typeof child.y === "number") {
      positions.set(child.id, { x: child.x, y: child.y });
    }
  });
  return nodes.map((node) => {
    const position = positions.get(node.id);
    if (!position) {
      return node;
    }
    return {
      ...node,
      position
    };
  });
}

export async function layoutAreaGraphNodes<T>(
  nodes: Node<T>[],
  edges: Edge[],
  preferCardinalLayout = true
): Promise<Node<T>[]> {
  if (!nodes.length) {
    return [];
  }
  if (preferCardinalLayout) {
    return layoutAreaGraphNodesGrid(nodes, edges);
  }
  const layoutEdges = edges.filter(
    (edge) => edge.sourceHandle && edge.targetHandle
  );
  if (!layoutEdges.length) {
    return layoutAreaGraphNodesGrid(nodes, edges);
  }
  const layoutOptions: Record<string, string> = {
    "elk.algorithm": "layered",
    "elk.direction": "RIGHT",
    "elk.spacing.nodeNode": "80",
    "elk.layered.spacing.nodeNodeBetweenLayers": "140",
    "elk.spacing.componentComponent": "120",
    "elk.portConstraints": "FIXED_SIDE"
  };
  const graph = {
    id: "root",
    layoutOptions,
    children: nodes.map((node) => ({
      id: node.id,
      width: areaNodeSize.width,
      height: areaNodeSize.height,
      ports: areaPortDefinitions.map((port) => ({
        id: `${node.id}.${port.id}`,
        layoutOptions: {
          "elk.port.side": port.side
        }
      }))
    })),
    edges: layoutEdges.map((edge) => {
      const dirKey = getEdgeDirKey(edge);
      const directionPriority = dirKey === "east" ? "2" : "0";
      return {
        id: edge.id,
        sources: [
          edge.sourceHandle
            ? `${edge.source}.${edge.sourceHandle}`
            : edge.source
        ],
        targets: [
          edge.targetHandle
            ? `${edge.target}.${edge.targetHandle}`
            : edge.target
        ],
        layoutOptions: {
          "elk.layered.priority.direction": directionPriority
        }
      };
    })
  };
  const layout = await elk.layout(graph);
  const positions = new Map<string, GridPosition>();
  layout.children?.forEach((child) => {
    if (child.id && typeof child.x === "number" && typeof child.y === "number") {
      positions.set(child.id, { x: child.x, y: child.y });
    }
  });
  return nodes.map((node) => {
    const position = positions.get(node.id);
    if (!position) {
      return node;
    }
    return {
      ...node,
      position
    };
  });
}
