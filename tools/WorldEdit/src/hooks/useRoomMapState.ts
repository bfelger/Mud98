import {
  useCallback,
  useEffect,
  useMemo,
  useRef,
  useState,
  type MouseEvent
} from "react";
import { applyNodeChanges, type Edge, type Node, type NodeChange } from "reactflow";
import { directions } from "../schemas/enums";
import type { AreaIndexEntry, AreaJson } from "../repository/types";
import { layoutRoomNodes } from "../map/graphLayout";
import {
  applyRoomLayoutOverrides,
  applyRoomSelection,
  buildRoomNodes,
  type RoomLayoutMap,
  type RoomNodeData
} from "../map/roomNodes";
import { buildExternalExits, buildRoomEdges, type ExternalExit } from "../map/roomEdges";

export type RoomContextMenuState = {
  vnum: number;
  x: number;
  y: number;
};

type UseRoomMapStateArgs = {
  areaData: AreaJson | null;
  areaIndex: AreaIndexEntry[];
  exitInvalidEdgeIds: Set<string>;
  activeTab: string;
  selectedEntity: string;
  setSelectedEntity: (entity: string) => void;
  parseVnum: (value: unknown) => number | null;
};

export const useRoomMapState = ({
  areaData,
  areaIndex,
  exitInvalidEdgeIds,
  activeTab,
  selectedEntity,
  setSelectedEntity,
  parseVnum
}: UseRoomMapStateArgs) => {
  const [selectedRoomVnum, setSelectedRoomVnum] = useState<number | null>(null);
  const [roomContextMenu, setRoomContextMenu] =
    useState<RoomContextMenuState | null>(null);
  const [roomLinkPanel, setRoomLinkPanel] =
    useState<RoomContextMenuState | null>(null);
  const [roomLinkDirection, setRoomLinkDirection] = useState<string>(
    directions[0]
  );
  const [roomLinkTarget, setRoomLinkTarget] = useState<string>("");
  const [layoutNodes, setLayoutNodes] = useState<Node<RoomNodeData>[]>([]);
  const [roomLayout, setRoomLayout] = useState<RoomLayoutMap>({});
  const [dirtyRoomNodes, setDirtyRoomNodes] = useState<Set<string>>(
    () => new Set()
  );
  const [autoLayoutEnabled, setAutoLayoutEnabled] = useState(true);
  const [preferCardinalLayout, setPreferCardinalLayout] = useState(() => {
    const stored = localStorage.getItem("worldedit.preferCardinalLayout");
    return stored ? stored === "true" : true;
  });
  const [showVerticalEdges, setShowVerticalEdges] = useState(() => {
    const stored = localStorage.getItem("worldedit.showVerticalEdges");
    return stored ? stored === "true" : false;
  });
  const [layoutNonce, setLayoutNonce] = useState(0);
  const roomLayoutRef = useRef<RoomLayoutMap>({});
  const roomNodesWithLayoutRef = useRef<Node<RoomNodeData>[]>([]);

  const baseRoomNodes = useMemo(() => buildRoomNodes(areaData), [areaData]);
  const layoutSourceNodes = useMemo(
    () => (layoutNodes.length ? layoutNodes : baseRoomNodes),
    [layoutNodes, baseRoomNodes]
  );
  const roomNodesWithLayout = useMemo(
    () => applyRoomLayoutOverrides(layoutSourceNodes, roomLayout),
    [layoutSourceNodes, roomLayout]
  );
  const roomEdges = useMemo(
    () => buildRoomEdges(areaData, showVerticalEdges, true, exitInvalidEdgeIds),
    [areaData, showVerticalEdges, exitInvalidEdgeIds]
  );
  const externalExits = useMemo(
    () => buildExternalExits(areaData, areaIndex),
    [areaData, areaIndex]
  );
  const closeRoomContextOverlays = useCallback(() => {
    setRoomContextMenu(null);
    setRoomLinkPanel(null);
  }, []);
  const handleRoomContextMenu = useCallback(
    (event: MouseEvent<HTMLDivElement>, vnum: number) => {
      setRoomContextMenu({ vnum, x: event.clientX, y: event.clientY });
      setRoomLinkPanel(null);
      setSelectedRoomVnum(vnum);
      setSelectedEntity("Rooms");
    },
    [setSelectedEntity]
  );
  const handleMapNavigate = useCallback(
    (vnum: number) => {
      closeRoomContextOverlays();
      setSelectedRoomVnum(vnum);
      setSelectedEntity("Rooms");
    },
    [closeRoomContextOverlays, setSelectedEntity]
  );
  const handleMapNodeClick = useCallback(
    (node: Node<RoomNodeData>) => {
      closeRoomContextOverlays();
      const vnum =
        typeof node.data?.vnum === "number"
          ? node.data.vnum
          : parseVnum(node.id);
      if (vnum !== null && vnum >= 0) {
        setSelectedRoomVnum(vnum);
        setSelectedEntity("Rooms");
      }
    },
    [closeRoomContextOverlays, parseVnum, setSelectedEntity]
  );
  const handleRelayout = useCallback(
    () => setLayoutNonce((value) => value + 1),
    []
  );
  const handleTogglePreferGrid = useCallback(
    (nextValue: boolean) => {
      setPreferCardinalLayout(nextValue);
      if (!autoLayoutEnabled) {
        setLayoutNonce((value) => value + 1);
      }
    },
    [autoLayoutEnabled]
  );
  const roomNodesWithHandlers = useMemo(
    () =>
      roomNodesWithLayout.map((node) => ({
        ...node,
        data: {
          ...node.data,
          onNavigate: handleMapNavigate,
          onContextMenu: handleRoomContextMenu,
          dirty: dirtyRoomNodes.has(node.id)
        }
      })),
    [roomNodesWithLayout, handleMapNavigate, handleRoomContextMenu, dirtyRoomNodes]
  );
  const mapNodes = useMemo(
    () => applyRoomSelection(roomNodesWithHandlers, selectedRoomVnum),
    [roomNodesWithHandlers, selectedRoomVnum]
  );
  const selectedRoomNode = useMemo(() => {
    if (selectedRoomVnum === null) {
      return null;
    }
    return mapNodes.find((node) => node.data.vnum === selectedRoomVnum) ?? null;
  }, [mapNodes, selectedRoomVnum]);
  const selectedRoomLocked = Boolean(selectedRoomNode?.data.locked);
  const dirtyRoomCount = dirtyRoomNodes.size;
  const hasRoomLayout = Object.keys(roomLayout).length > 0;

  const handleLockSelectedRoom = useCallback(() => {
    if (!selectedRoomNode) {
      return;
    }
    setRoomLayout((current) => ({
      ...current,
      [selectedRoomNode.id]: {
        x: selectedRoomNode.position.x,
        y: selectedRoomNode.position.y,
        locked: true
      }
    }));
    setDirtyRoomNodes((current) => {
      if (!current.has(selectedRoomNode.id)) {
        return current;
      }
      const next = new Set(current);
      next.delete(selectedRoomNode.id);
      return next;
    });
  }, [selectedRoomNode]);
  const handleUnlockSelectedRoom = useCallback(() => {
    if (!selectedRoomNode) {
      return;
    }
    setLayoutNodes((current) => {
      const source = current.length ? current : roomNodesWithLayout;
      return source.map((node) =>
        node.id === selectedRoomNode.id
          ? { ...node, position: selectedRoomNode.position }
          : node
      );
    });
    setRoomLayout((current) => {
      if (!current[selectedRoomNode.id]) {
        return current;
      }
      const next = { ...current };
      delete next[selectedRoomNode.id];
      return next;
    });
    setDirtyRoomNodes((current) => {
      if (current.has(selectedRoomNode.id)) {
        return current;
      }
      const next = new Set(current);
      next.add(selectedRoomNode.id);
      return next;
    });
  }, [selectedRoomNode, roomNodesWithLayout]);
  const handleClearRoomLayout = useCallback(() => {
    setRoomLayout({});
    setLayoutNodes([]);
    setDirtyRoomNodes(new Set());
    if (autoLayoutEnabled) {
      setLayoutNonce((value) => value + 1);
    }
  }, [autoLayoutEnabled]);
  const handleNodesChange = useCallback(
    (changes: NodeChange[]) => {
      setLayoutNodes((current) => {
        const source = current.length ? current : roomNodesWithLayout;
        return applyNodeChanges(changes, source);
      });
      const movedIds = changes
        .filter((change) => change.type === "position")
        .map((change) => change.id);
      if (movedIds.length) {
        setDirtyRoomNodes((current) => {
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
    [roomNodesWithLayout]
  );
  const handleNodeDragStop = useCallback(
    (_: unknown, node: Node<RoomNodeData>) => {
      setLayoutNodes((current) => {
        const source = current.length ? current : roomNodesWithLayout;
        return source.map((entry) =>
          entry.id === node.id ? { ...entry, position: node.position } : entry
        );
      });
      setDirtyRoomNodes((current) => {
        if (current.has(node.id)) {
          return current;
        }
        const next = new Set(current);
        next.add(node.id);
        return next;
      });
    },
    [roomNodesWithLayout]
  );
  const runRoomLayout = useCallback(
    async (cancelRef?: { current: boolean }) => {
      if (activeTab !== "Map") {
        return;
      }
      if (!baseRoomNodes.length) {
        if (!cancelRef?.current) {
          setLayoutNodes([]);
          setDirtyRoomNodes(new Set());
        }
        return;
      }
      try {
        const nextNodes = await layoutRoomNodes(
          baseRoomNodes,
          roomEdges,
          preferCardinalLayout
        );
        if (!cancelRef?.current) {
          setLayoutNodes(
            applyRoomLayoutOverrides(nextNodes, roomLayoutRef.current)
          );
          setDirtyRoomNodes(new Set());
        }
      } catch {
        if (!cancelRef?.current) {
          setLayoutNodes(
            applyRoomLayoutOverrides(baseRoomNodes, roomLayoutRef.current)
          );
          setDirtyRoomNodes(new Set());
        }
      }
    },
    [activeTab, baseRoomNodes, preferCardinalLayout, roomEdges]
  );

  useEffect(() => {
    roomLayoutRef.current = roomLayout;
  }, [roomLayout]);

  useEffect(() => {
    roomNodesWithLayoutRef.current = roomNodesWithLayout;
  }, [roomNodesWithLayout]);

  useEffect(() => {
    localStorage.setItem(
      "worldedit.preferCardinalLayout",
      String(preferCardinalLayout)
    );
  }, [preferCardinalLayout]);

  useEffect(() => {
    localStorage.setItem(
      "worldedit.showVerticalEdges",
      String(showVerticalEdges)
    );
  }, [showVerticalEdges]);

  useEffect(() => {
    if (activeTab === "Map" && selectedEntity !== "Rooms") {
      setSelectedEntity("Rooms");
    }
  }, [activeTab, selectedEntity, setSelectedEntity]);

  useEffect(() => {
    if (!autoLayoutEnabled) {
      return;
    }
    const cancelRef = { current: false };
    runRoomLayout(cancelRef);
    return () => {
      cancelRef.current = true;
    };
  }, [autoLayoutEnabled, runRoomLayout]);

  useEffect(() => {
    if (layoutNonce === 0) {
      return;
    }
    const cancelRef = { current: false };
    runRoomLayout(cancelRef);
    return () => {
      cancelRef.current = true;
    };
  }, [layoutNonce, runRoomLayout]);

  const appendLayoutNode = useCallback(
    (node: Node<RoomNodeData>) => {
      setLayoutNodes((current) => {
        const source =
          current.length ? current : roomNodesWithLayoutRef.current;
        return [...source, node];
      });
      setDirtyRoomNodes((current) => {
        const next = new Set(current);
        next.add(node.id);
        return next;
      });
    },
    []
  );
  const removeLayoutNode = useCallback((nodeId: string) => {
    setRoomLayout((current) => {
      if (!current[nodeId]) {
        return current;
      }
      const next = { ...current };
      delete next[nodeId];
      return next;
    });
    setLayoutNodes((current) => current.filter((node) => node.id !== nodeId));
    setDirtyRoomNodes((current) => {
      if (!current.has(nodeId)) {
        return current;
      }
      const next = new Set(current);
      next.delete(nodeId);
      return next;
    });
  }, []);
  const applyLoadedRoomLayout = useCallback((layout: RoomLayoutMap) => {
    setRoomLayout(layout);
    setLayoutNodes([]);
    setDirtyRoomNodes(new Set());
  }, []);

  return {
    selectedRoomVnum,
    setSelectedRoomVnum,
    roomContextMenu,
    setRoomContextMenu,
    roomLinkPanel,
    setRoomLinkPanel,
    roomLinkDirection,
    setRoomLinkDirection,
    roomLinkTarget,
    setRoomLinkTarget,
    mapNodes,
    roomEdges,
    externalExits,
    selectedRoomNode,
    selectedRoomLocked,
    dirtyRoomCount,
    hasRoomLayout,
    roomLayout,
    roomNodesWithLayoutRef,
    autoLayoutEnabled,
    setAutoLayoutEnabled,
    preferCardinalLayout,
    setPreferCardinalLayout,
    showVerticalEdges,
    setShowVerticalEdges,
    handleRelayout,
    handleTogglePreferGrid,
    handleMapNavigate,
    handleMapNodeClick,
    handleRoomContextMenu,
    closeRoomContextOverlays,
    handleClearRoomLayout,
    handleLockSelectedRoom,
    handleUnlockSelectedRoom,
    handleNodesChange,
    handleNodeDragStop,
    appendLayoutNode,
    removeLayoutNode,
    applyLoadedRoomLayout
  };
};
