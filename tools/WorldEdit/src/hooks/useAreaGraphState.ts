import { useCallback, useEffect, useMemo, useState } from "react";
import { applyNodeChanges, type Node, type NodeChange } from "reactflow";
import type {
  AreaGraphLink,
  AreaIndexEntry,
  AreaJson
} from "../repository/types";
import {
  applyAreaLayoutOverrides,
  type AreaLayoutMap,
  type AreaGraphNodeData
} from "../map/areaNodes";
import { layoutAreaGraphNodes } from "../map/graphLayout";
import {
  buildAreaGraphContext,
  buildAreaGraphEdges,
  buildAreaGraphFilteredEntries,
  buildAreaGraphMatch,
  buildAreaGraphMatchLabel,
  buildAreaGraphNodes
} from "../map/areaGraphBuilder";
import type { ExternalExit } from "../map/roomEdges";

type UseAreaGraphStateArgs = {
  areaIndex: AreaIndexEntry[];
  areaData: AreaJson | null;
  areaPath: string | null;
  areaGraphLinks: AreaGraphLink[];
  externalExits: ExternalExit[];
  areaDirectionHandleMap: Record<string, { source: string; target: string }>;
  getAreaVnumBounds: (areaData: AreaJson | null) => { min: number; max: number } | null;
  getFirstString: (value: unknown, fallback: string) => string;
  fileNameFromPath: (path: string | null) => string;
  parseVnum: (value: unknown) => number | null;
  formatVnumRange: (range: [number, number] | null) => string;
  getDominantExitDirection: (
    directionCounts: Partial<Record<string, number>> | null | undefined
  ) => string | null;
  findAreaForVnum: (areaIndex: AreaIndexEntry[], vnum: number) => AreaIndexEntry | null;
};

export const useAreaGraphState = ({
  areaIndex,
  areaData,
  areaPath,
  areaGraphLinks,
  externalExits,
  areaDirectionHandleMap,
  getAreaVnumBounds,
  getFirstString,
  fileNameFromPath,
  parseVnum,
  formatVnumRange,
  getDominantExitDirection,
  findAreaForVnum
}: UseAreaGraphStateArgs) => {
  const [areaGraphFilter, setAreaGraphFilter] = useState("");
  const [areaGraphVnumQuery, setAreaGraphVnumQuery] = useState("");
  const [areaGraphLayoutNodes, setAreaGraphLayoutNodes] = useState<
    Node<AreaGraphNodeData>[]
  >([]);
  const [areaLayout, setAreaLayout] = useState<AreaLayoutMap>({});
  const [dirtyAreaNodes, setDirtyAreaNodes] = useState<Set<string>>(
    () => new Set()
  );
  const [preferAreaCardinalLayout, setPreferAreaCardinalLayout] = useState(
    () => {
      const stored = localStorage.getItem("worldedit.preferAreaCardinalLayout");
      return stored ? stored === "true" : true;
    }
  );

  const areaGraphContext = useMemo(
    () =>
      buildAreaGraphContext({
        areaIndex,
        areaData,
        areaPath,
        getAreaVnumBounds,
        getFirstString,
        fileNameFromPath
      }),
    [areaIndex, areaData, areaPath, getAreaVnumBounds, getFirstString, fileNameFromPath]
  );
  const areaGraphFilteredEntries = useMemo(
    () => buildAreaGraphFilteredEntries(areaGraphContext.entries, areaGraphFilter),
    [areaGraphContext.entries, areaGraphFilter]
  );
  const areaGraphMatch = useMemo(
    () => buildAreaGraphMatch(areaGraphContext.entries, areaGraphVnumQuery, parseVnum),
    [areaGraphContext.entries, areaGraphVnumQuery, parseVnum]
  );
  const areaGraphMatchLabel = useMemo(
    () => buildAreaGraphMatchLabel(areaGraphMatch, formatVnumRange),
    [areaGraphMatch, formatVnumRange]
  );
  const areaGraphNodes = useMemo(
    () =>
      buildAreaGraphNodes({
        areaGraphFilteredEntries,
        areaGraphContext,
        areaGraphMatch,
        formatVnumRange
      }),
    [areaGraphFilteredEntries, areaGraphContext, areaGraphMatch, formatVnumRange]
  );
  const areaGraphEdges = useMemo(
    () =>
      buildAreaGraphEdges({
        areaGraphContext,
        areaGraphFilteredEntries,
        areaGraphLinks,
        areaIndex,
        externalExits,
        areaDirectionHandleMap,
        getDominantExitDirection,
        findAreaForVnum
      }),
    [
      areaGraphContext,
      areaGraphFilteredEntries,
      areaGraphLinks,
      areaIndex,
      externalExits,
      areaDirectionHandleMap,
      getDominantExitDirection,
      findAreaForVnum
    ]
  );
  const worldMapNodes = useMemo(
    () =>
      applyAreaLayoutOverrides(
        areaGraphLayoutNodes.length ? areaGraphLayoutNodes : areaGraphNodes,
        areaLayout,
        dirtyAreaNodes
      ),
    [areaGraphLayoutNodes, areaGraphNodes, areaLayout, dirtyAreaNodes]
  );
  const selectedAreaNode = useMemo(
    () => worldMapNodes.find((node) => node.selected) ?? null,
    [worldMapNodes]
  );
  const selectedAreaLocked = Boolean(
    selectedAreaNode && areaLayout[selectedAreaNode.id]?.locked
  );
  const dirtyAreaCount = dirtyAreaNodes.size;
  const hasAreaLayout = Object.keys(areaLayout).length > 0;

  const runAreaGraphLayout = useCallback(
    async (cancelRef?: { current: boolean }) => {
      if (!areaGraphNodes.length) {
        if (!cancelRef?.current) {
          setAreaGraphLayoutNodes([]);
        }
        return;
      }
      try {
        const nextNodes = await layoutAreaGraphNodes(
          areaGraphNodes,
          areaGraphEdges,
          preferAreaCardinalLayout
        );
        if (!cancelRef?.current) {
          setAreaGraphLayoutNodes(nextNodes);
          setDirtyAreaNodes(new Set());
        }
      } catch {
        if (!cancelRef?.current) {
          setAreaGraphLayoutNodes(areaGraphNodes);
          setDirtyAreaNodes(new Set());
        }
      }
    },
    [areaGraphNodes, areaGraphEdges, preferAreaCardinalLayout]
  );
  const handleTogglePreferAreaGrid = useCallback((nextValue: boolean) => {
    setPreferAreaCardinalLayout(nextValue);
  }, []);
  const handleLockSelectedArea = useCallback(() => {
    if (!selectedAreaNode) {
      return;
    }
    setAreaLayout((current) => ({
      ...current,
      [selectedAreaNode.id]: {
        x: selectedAreaNode.position.x,
        y: selectedAreaNode.position.y,
        locked: true
      }
    }));
    setDirtyAreaNodes((current) => {
      if (!current.has(selectedAreaNode.id)) {
        return current;
      }
      const next = new Set(current);
      next.delete(selectedAreaNode.id);
      return next;
    });
  }, [selectedAreaNode]);
  const handleUnlockSelectedArea = useCallback(() => {
    if (!selectedAreaNode) {
      return;
    }
    setAreaGraphLayoutNodes((current) => {
      const source = current.length ? current : areaGraphNodes;
      return source.map((node) =>
        node.id === selectedAreaNode.id
          ? { ...node, position: selectedAreaNode.position }
          : node
      );
    });
    setAreaLayout((current) => {
      if (!current[selectedAreaNode.id]) {
        return current;
      }
      const next = { ...current };
      delete next[selectedAreaNode.id];
      return next;
    });
    setDirtyAreaNodes((current) => {
      if (current.has(selectedAreaNode.id)) {
        return current;
      }
      const next = new Set(current);
      next.add(selectedAreaNode.id);
      return next;
    });
  }, [selectedAreaNode, areaGraphNodes]);
  const handleLockDirtyAreas = useCallback(() => {
    if (!dirtyAreaNodes.size) {
      return;
    }
    setAreaLayout((current) => {
      const next = { ...current };
      dirtyAreaNodes.forEach((id) => {
        const node = worldMapNodes.find((entry) => entry.id === id);
        if (!node) {
          return;
        }
        next[id] = {
          x: node.position.x,
          y: node.position.y,
          locked: true
        };
      });
      return next;
    });
    setDirtyAreaNodes(new Set());
  }, [dirtyAreaNodes, worldMapNodes]);
  const handleClearAreaLayout = useCallback(() => {
    setAreaLayout({});
    setDirtyAreaNodes(new Set());
    void runAreaGraphLayout();
  }, [runAreaGraphLayout]);
  const handleRelayoutArea = useCallback(() => {
    void runAreaGraphLayout();
  }, [runAreaGraphLayout]);
  const handleAreaGraphNodesChange = useCallback(
    (changes: NodeChange[]) => {
      setAreaGraphLayoutNodes((current) => {
        const source = current.length ? current : areaGraphNodes;
        return applyNodeChanges(changes, source);
      });
      const movedIds = changes
        .filter((change) => change.type === "position")
        .map((change) => change.id);
      if (movedIds.length) {
        setDirtyAreaNodes((current) => {
          let changed = false;
          const next = new Set(current);
          movedIds.forEach((id) => {
            if (!next.has(id)) {
              next.add(id);
              changed = true;
            }
          });
          return changed ? next : current;
        });
      }
    },
    [areaGraphNodes]
  );
  const handleAreaGraphNodeDragStop = useCallback(
    (_: unknown, node: Node<AreaGraphNodeData>) => {
      setAreaGraphLayoutNodes((current) => {
        const source = current.length ? current : areaGraphNodes;
        return source.map((entry) =>
          entry.id === node.id ? { ...entry, position: node.position } : entry
        );
      });
      setDirtyAreaNodes((current) => {
        if (current.has(node.id)) {
          return current;
        }
        const next = new Set(current);
        next.add(node.id);
        return next;
      });
    },
    [areaGraphNodes]
  );
  const applyLoadedAreaLayout = useCallback((layout: AreaLayoutMap) => {
    setAreaLayout(layout);
    setAreaGraphLayoutNodes([]);
    setDirtyAreaNodes(new Set());
  }, []);

  useEffect(() => {
    const cancelRef = { current: false };
    runAreaGraphLayout(cancelRef);
    return () => {
      cancelRef.current = true;
    };
  }, [runAreaGraphLayout]);

  useEffect(() => {
    localStorage.setItem(
      "worldedit.preferAreaCardinalLayout",
      String(preferAreaCardinalLayout)
    );
  }, [preferAreaCardinalLayout]);

  return {
    areaGraphFilter,
    setAreaGraphFilter,
    areaGraphVnumQuery,
    setAreaGraphVnumQuery,
    areaGraphMatchLabel,
    areaGraphNodes,
    areaGraphEdges,
    worldMapNodes,
    selectedAreaNode,
    selectedAreaLocked,
    dirtyAreaCount,
    hasAreaLayout,
    areaLayout,
    preferAreaCardinalLayout,
    setPreferAreaCardinalLayout,
    applyLoadedAreaLayout,
    handleTogglePreferAreaGrid,
    handleLockSelectedArea,
    handleUnlockSelectedArea,
    handleLockDirtyAreas,
    handleClearAreaLayout,
    handleRelayoutArea,
    handleAreaGraphNodesChange,
    handleAreaGraphNodeDragStop
  };
};
