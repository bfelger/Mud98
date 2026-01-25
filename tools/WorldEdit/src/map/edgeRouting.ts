import { Position } from "reactflow";

export type Point = { x: number; y: number };
export type NodeBounds = {
  id: string;
  left: number;
  right: number;
  top: number;
  bottom: number;
};

type OrthogonalPath = {
  points: Point[];
  orientation: "horizontal" | "vertical";
};

export const offsetPoint = (
  point: Point,
  direction: Position,
  distance: number
): Point => {
  switch (direction) {
    case Position.Left:
      return { x: point.x - distance, y: point.y };
    case Position.Right:
      return { x: point.x + distance, y: point.y };
    case Position.Top:
      return { x: point.x, y: point.y - distance };
    case Position.Bottom:
      return { x: point.x, y: point.y + distance };
    default:
      return point;
  }
};

const pointsToPath = (points: Point[]): string => {
  if (!points.length) {
    return "";
  }
  return points
    .map((point, index) =>
      `${index === 0 ? "M" : "L"}${point.x} ${point.y}`
    )
    .join(" ");
};

const getPathLength = (points: Point[]): number => {
  let length = 0;
  for (let index = 1; index < points.length; index += 1) {
    const prev = points[index - 1];
    const next = points[index];
    length += Math.abs(next.x - prev.x) + Math.abs(next.y - prev.y);
  }
  return length;
};

const getPathMidpoint = (points: Point[]): Point => {
  if (!points.length) {
    return { x: 0, y: 0 };
  }
  const total = getPathLength(points);
  if (total === 0) {
    return points[0];
  }
  let remaining = total / 2;
  for (let index = 1; index < points.length; index += 1) {
    const prev = points[index - 1];
    const next = points[index];
    const segmentLength = Math.abs(next.x - prev.x) + Math.abs(next.y - prev.y);
    if (segmentLength >= remaining) {
      const ratio = segmentLength === 0 ? 0 : remaining / segmentLength;
      return {
        x: prev.x + (next.x - prev.x) * ratio,
        y: prev.y + (next.y - prev.y) * ratio
      };
    }
    remaining -= segmentLength;
  }
  return points[points.length - 1];
};

const segmentIntersectsRect = (
  start: Point,
  end: Point,
  rect: NodeBounds,
  padding: number
): boolean => {
  const left = rect.left - padding;
  const right = rect.right + padding;
  const top = rect.top - padding;
  const bottom = rect.bottom + padding;
  if (start.x === end.x) {
    const x = start.x;
    const minY = Math.min(start.y, end.y);
    const maxY = Math.max(start.y, end.y);
    return x >= left && x <= right && maxY >= top && minY <= bottom;
  }
  if (start.y === end.y) {
    const y = start.y;
    const minX = Math.min(start.x, end.x);
    const maxX = Math.max(start.x, end.x);
    return y >= top && y <= bottom && maxX >= left && minX <= right;
  }
  return false;
};

const countPathIntersections = (
  points: Point[],
  obstacles: NodeBounds[],
  padding: number
): number => {
  let count = 0;
  for (let index = 1; index < points.length; index += 1) {
    const start = points[index - 1];
    const end = points[index];
    for (const obstacle of obstacles) {
      if (segmentIntersectsRect(start, end, obstacle, padding)) {
        count += 1;
        break;
      }
    }
  }
  return count;
};

const createOrthogonalPath = (
  start: Point,
  end: Point,
  sourceDirection: Position,
  targetDirection: Position,
  orientation: "horizontal" | "vertical",
  edgeStubSize: number
): OrthogonalPath => {
  const sourceStub = offsetPoint(start, sourceDirection, edgeStubSize);
  const targetStub = offsetPoint(end, targetDirection, edgeStubSize);
  if (orientation === "horizontal") {
    const midX = (sourceStub.x + targetStub.x) / 2;
    return {
      orientation,
      points: [
        start,
        sourceStub,
        { x: midX, y: sourceStub.y },
        { x: midX, y: targetStub.y },
        targetStub,
        end
      ]
    };
  }
  const midY = (sourceStub.y + targetStub.y) / 2;
  return {
    orientation,
    points: [
      start,
      sourceStub,
      { x: sourceStub.x, y: midY },
      { x: targetStub.x, y: midY },
      targetStub,
      end
    ]
  };
};

const adjustOrthogonalPath = (
  path: OrthogonalPath,
  obstacles: NodeBounds[],
  edgeClearance: number
): OrthogonalPath => {
  if (path.points.length < 4) {
    return path;
  }
  const points = [...path.points];
  if (path.orientation === "horizontal") {
    const start = points[2];
    const end = points[3];
    const blockers = obstacles.filter((obstacle) =>
      segmentIntersectsRect(start, end, obstacle, edgeClearance)
    );
    if (!blockers.length) {
      return path;
    }
    const candidateXs = new Set<number>([start.x]);
    blockers.forEach((blocker) => {
      candidateXs.add(blocker.left - edgeClearance * 2);
      candidateXs.add(blocker.right + edgeClearance * 2);
    });
    const sortedCandidates = [...candidateXs].sort(
      (a, b) => Math.abs(a - start.x) - Math.abs(b - start.x)
    );
    for (const candidate of sortedCandidates) {
      points[2] = { x: candidate, y: points[2].y };
      points[3] = { x: candidate, y: points[3].y };
      if (!countPathIntersections(points, obstacles, edgeClearance)) {
        return { ...path, points };
      }
    }
    return path;
  }
  const start = points[2];
  const end = points[3];
  const blockers = obstacles.filter((obstacle) =>
    segmentIntersectsRect(start, end, obstacle, edgeClearance)
  );
  if (!blockers.length) {
    return path;
  }
  const candidateYs = new Set<number>([start.y]);
  blockers.forEach((blocker) => {
    candidateYs.add(blocker.top - edgeClearance * 2);
    candidateYs.add(blocker.bottom + edgeClearance * 2);
  });
  const sortedCandidates = [...candidateYs].sort(
    (a, b) => Math.abs(a - start.y) - Math.abs(b - start.y)
  );
  for (const candidate of sortedCandidates) {
    points[2] = { x: points[2].x, y: candidate };
    points[3] = { x: points[3].x, y: candidate };
    if (!countPathIntersections(points, obstacles, edgeClearance)) {
      return { ...path, points };
    }
  }
  return path;
};

type EdgeDirectionPriority = Partial<
  Record<Position, "horizontal" | "vertical">
>;

type OrthogonalEdgePathParams = {
  start: Point;
  end: Point;
  sourceDirection: Position;
  targetDirection: Position;
  obstacles: NodeBounds[];
  edgeStubSize: number;
  edgeClearance: number;
  edgeDirectionPriority: EdgeDirectionPriority;
};

export const buildOrthogonalEdgePath = (
  params: OrthogonalEdgePathParams
): { path: string; labelPoint: Point } => {
  const {
    start,
    end,
    sourceDirection,
    targetDirection,
    obstacles,
    edgeStubSize,
    edgeClearance,
    edgeDirectionPriority
  } = params;
  const preferred = edgeDirectionPriority[sourceDirection] ?? "horizontal";
  const alternate = preferred === "horizontal" ? "vertical" : "horizontal";
  const primaryPath = createOrthogonalPath(
    start,
    end,
    sourceDirection,
    targetDirection,
    preferred,
    edgeStubSize
  );
  const secondaryPath = createOrthogonalPath(
    start,
    end,
    sourceDirection,
    targetDirection,
    alternate,
    edgeStubSize
  );
  const primaryScore = countPathIntersections(
    primaryPath.points,
    obstacles,
    edgeClearance
  );
  const secondaryScore = countPathIntersections(
    secondaryPath.points,
    obstacles,
    edgeClearance
  );
  let chosen =
    primaryScore < secondaryScore
      ? primaryPath
      : secondaryScore < primaryScore
        ? secondaryPath
        : getPathLength(primaryPath.points) <=
            getPathLength(secondaryPath.points)
          ? primaryPath
          : secondaryPath;
  chosen = adjustOrthogonalPath(chosen, obstacles, edgeClearance);
  return {
    path: pointsToPath(chosen.points),
    labelPoint: getPathMidpoint(chosen.points)
  };
};
