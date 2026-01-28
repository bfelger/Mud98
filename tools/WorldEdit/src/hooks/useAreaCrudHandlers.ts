import { useCallback } from "react";
import { createRoom, deleteRoom } from "../crud/area/roomsCrud";
import { createMobile, deleteMobile } from "../crud/area/mobilesCrud";
import { createObject, deleteObject } from "../crud/area/objectsCrud";
import { createReset, deleteReset } from "../crud/area/resetsCrud";
import { createShop, deleteShop } from "../crud/area/shopsCrud";
import { createQuest, deleteQuest } from "../crud/area/questsCrud";
import { createFaction, deleteFaction } from "../crud/area/factionsCrud";
import { createAreaLoot, deleteAreaLoot } from "../crud/area/areaLootCrud";
import { createRecipe, deleteRecipe } from "../crud/area/recipesCrud";
import { createGatherSpawn, deleteGatherSpawn } from "../crud/area/gatherSpawnsCrud";
import type { AreaJson, ReferenceData } from "../repository/types";

type UseAreaCrudHandlersArgs = {
  areaData: AreaJson | null;
  referenceData: ReferenceData | null;
  selectedRoomVnum: number | null;
  selectedMobileVnum: number | null;
  selectedObjectVnum: number | null;
  selectedResetIndex: number | null;
  selectedShopKeeper: number | null;
  selectedQuestVnum: number | null;
  selectedFactionVnum: number | null;
  selectedAreaLootKind: "group" | "table" | null;
  selectedAreaLootIndex: number | null;
  selectedRecipeVnum: number | null;
  selectedGatherVnum: number | null;
  setAreaData: (data: AreaJson | null) => void;
  setSelectedEntity: (entity: string) => void;
  setSelectedRoomVnum: (vnum: number | null) => void;
  setSelectedMobileVnum: (vnum: number | null) => void;
  setSelectedObjectVnum: (vnum: number | null) => void;
  setSelectedResetIndex: (index: number | null) => void;
  setSelectedShopKeeper: (vnum: number | null) => void;
  setSelectedQuestVnum: (vnum: number | null) => void;
  setSelectedFactionVnum: (vnum: number | null) => void;
  setSelectedAreaLootKind: (kind: "group" | "table" | null) => void;
  setSelectedAreaLootIndex: (index: number | null) => void;
  setSelectedRecipeVnum: (vnum: number | null) => void;
  setSelectedGatherVnum: (vnum: number | null) => void;
  setActiveTab: (tab: "Form" | "Table" | "Map" | "Script" | "World" | "Inspect") => void;
  setStatusMessage: (message: string) => void;
  removeRoomLayout: (roomId: string) => void;
};

export const useAreaCrudHandlers = ({
  areaData,
  referenceData,
  selectedRoomVnum,
  selectedMobileVnum,
  selectedObjectVnum,
  selectedResetIndex,
  selectedShopKeeper,
  selectedQuestVnum,
  selectedFactionVnum,
  selectedAreaLootKind,
  selectedAreaLootIndex,
  selectedRecipeVnum,
  selectedGatherVnum,
  setAreaData,
  setSelectedEntity,
  setSelectedRoomVnum,
  setSelectedMobileVnum,
  setSelectedObjectVnum,
  setSelectedResetIndex,
  setSelectedShopKeeper,
  setSelectedQuestVnum,
  setSelectedFactionVnum,
  setSelectedAreaLootKind,
  setSelectedAreaLootIndex,
  setSelectedRecipeVnum,
  setSelectedGatherVnum,
  setActiveTab,
  setStatusMessage,
  removeRoomLayout
}: UseAreaCrudHandlersArgs) => {
  const handleCreateRoom = useCallback(() => {
    const result = createRoom(areaData);
    if (!result.ok) {
      setStatusMessage(result.message);
      return;
    }
    setAreaData(result.data.areaData);
    setSelectedEntity("Rooms");
    setSelectedRoomVnum(result.data.vnum);
    setActiveTab("Form");
    setStatusMessage(`Created room ${result.data.vnum} (unsaved)`);
  }, [areaData, setStatusMessage, setSelectedEntity, setSelectedRoomVnum, setActiveTab, setAreaData]);

  const handleDeleteRoom = useCallback(() => {
    const result = deleteRoom(areaData, selectedRoomVnum);
    if (!result.ok) {
      setStatusMessage(result.message);
      return;
    }
    const { areaData: nextAreaData, roomId, deletedVnum } = result.data;
    setAreaData(nextAreaData);
    removeRoomLayout(roomId);
    setSelectedRoomVnum(null);
    setStatusMessage(`Deleted room ${deletedVnum} (unsaved)`);
  }, [areaData, selectedRoomVnum, setStatusMessage, setAreaData, removeRoomLayout, setSelectedRoomVnum]);

  const handleCreateMobile = useCallback(() => {
    const defaultRace =
      referenceData?.races?.[0] ? referenceData.races[0] : "human";
    const result = createMobile(areaData, defaultRace);
    if (!result.ok) {
      setStatusMessage(result.message);
      return;
    }
    setAreaData(result.data.areaData);
    setSelectedEntity("Mobiles");
    setSelectedMobileVnum(result.data.vnum);
    setActiveTab("Form");
    setStatusMessage(`Created mobile ${result.data.vnum} (unsaved)`);
  }, [areaData, referenceData, setStatusMessage, setSelectedEntity, setSelectedMobileVnum, setActiveTab, setAreaData]);

  const handleDeleteMobile = useCallback(() => {
    const result = deleteMobile(areaData, selectedMobileVnum);
    if (!result.ok) {
      setStatusMessage(result.message);
      return;
    }
    setAreaData(result.data.areaData);
    setSelectedMobileVnum(null);
    setStatusMessage(`Deleted mobile ${result.data.deletedVnum} (unsaved)`);
  }, [areaData, selectedMobileVnum, setStatusMessage, setAreaData, setSelectedMobileVnum]);

  const handleCreateObject = useCallback(() => {
    const result = createObject(areaData);
    if (!result.ok) {
      setStatusMessage(result.message);
      return;
    }
    setAreaData(result.data.areaData);
    setSelectedEntity("Objects");
    setSelectedObjectVnum(result.data.vnum);
    setActiveTab("Form");
    setStatusMessage(`Created object ${result.data.vnum} (unsaved)`);
  }, [areaData, setStatusMessage, setSelectedEntity, setSelectedObjectVnum, setActiveTab, setAreaData]);

  const handleDeleteObject = useCallback(() => {
    const result = deleteObject(areaData, selectedObjectVnum);
    if (!result.ok) {
      setStatusMessage(result.message);
      return;
    }
    setAreaData(result.data.areaData);
    setSelectedObjectVnum(null);
    setStatusMessage(`Deleted object ${result.data.deletedVnum} (unsaved)`);
  }, [areaData, selectedObjectVnum, setStatusMessage, setAreaData, setSelectedObjectVnum]);

  const handleCreateReset = useCallback(() => {
    const result = createReset(areaData);
    if (!result.ok) {
      setStatusMessage(result.message);
      return;
    }
    setAreaData(result.data.areaData);
    setSelectedEntity("Resets");
    setSelectedResetIndex(result.data.index);
    setActiveTab("Form");
    setStatusMessage(`Created reset #${result.data.index} (unsaved)`);
  }, [areaData, setStatusMessage, setSelectedEntity, setSelectedResetIndex, setActiveTab, setAreaData]);

  const handleDeleteReset = useCallback(() => {
    const result = deleteReset(areaData, selectedResetIndex);
    if (!result.ok) {
      setStatusMessage(result.message);
      return;
    }
    setAreaData(result.data.areaData);
    setSelectedResetIndex(null);
    setStatusMessage(`Deleted reset #${result.data.deletedIndex} (unsaved)`);
  }, [areaData, selectedResetIndex, setStatusMessage, setAreaData, setSelectedResetIndex]);

  const handleCreateShop = useCallback(() => {
    const result = createShop(areaData);
    if (!result.ok) {
      setStatusMessage(result.message);
      return;
    }
    setAreaData(result.data.areaData);
    setSelectedEntity("Shops");
    setSelectedShopKeeper(result.data.keeper);
    setActiveTab("Form");
    setStatusMessage(`Created shop ${result.data.keeper} (unsaved)`);
  }, [areaData, setStatusMessage, setSelectedEntity, setSelectedShopKeeper, setActiveTab, setAreaData]);

  const handleDeleteShop = useCallback(() => {
    const result = deleteShop(areaData, selectedShopKeeper);
    if (!result.ok) {
      setStatusMessage(result.message);
      return;
    }
    setAreaData(result.data.areaData);
    setSelectedShopKeeper(null);
    setStatusMessage(`Deleted shop ${result.data.deletedKeeper} (unsaved)`);
  }, [areaData, selectedShopKeeper, setStatusMessage, setAreaData, setSelectedShopKeeper]);

  const handleCreateQuest = useCallback(() => {
    const result = createQuest(areaData);
    if (!result.ok) {
      setStatusMessage(result.message);
      return;
    }
    setAreaData(result.data.areaData);
    setSelectedEntity("Quests");
    setSelectedQuestVnum(result.data.vnum);
    setActiveTab("Form");
    setStatusMessage(`Created quest ${result.data.vnum} (unsaved)`);
  }, [areaData, setStatusMessage, setSelectedEntity, setSelectedQuestVnum, setActiveTab, setAreaData]);

  const handleDeleteQuest = useCallback(() => {
    const result = deleteQuest(areaData, selectedQuestVnum);
    if (!result.ok) {
      setStatusMessage(result.message);
      return;
    }
    setAreaData(result.data.areaData);
    setSelectedQuestVnum(null);
    setStatusMessage(`Deleted quest ${result.data.deletedVnum} (unsaved)`);
  }, [areaData, selectedQuestVnum, setStatusMessage, setAreaData, setSelectedQuestVnum]);

  const handleCreateFaction = useCallback(() => {
    const result = createFaction(areaData);
    if (!result.ok) {
      setStatusMessage(result.message);
      return;
    }
    setAreaData(result.data.areaData);
    setSelectedEntity("Factions");
    setSelectedFactionVnum(result.data.vnum);
    setActiveTab("Form");
    setStatusMessage(`Created faction ${result.data.vnum} (unsaved)`);
  }, [areaData, setStatusMessage, setSelectedEntity, setSelectedFactionVnum, setActiveTab, setAreaData]);

  const handleDeleteFaction = useCallback(() => {
    const result = deleteFaction(areaData, selectedFactionVnum);
    if (!result.ok) {
      setStatusMessage(result.message);
      return;
    }
    setAreaData(result.data.areaData);
    setSelectedFactionVnum(null);
    setStatusMessage(`Deleted faction ${result.data.deletedVnum} (unsaved)`);
  }, [areaData, selectedFactionVnum, setStatusMessage, setAreaData, setSelectedFactionVnum]);

  const handleCreateAreaLoot = useCallback((kind: "group" | "table") => {
    const result = createAreaLoot(areaData, kind);
    if (!result.ok) {
      setStatusMessage(result.message);
      return;
    }
    setAreaData(result.data.areaData);
    setSelectedEntity("Loot");
    setSelectedAreaLootKind(result.data.kind);
    setSelectedAreaLootIndex(result.data.index);
    setActiveTab("Form");
    setStatusMessage(
      `Created loot ${result.data.kind} ${result.data.index} (unsaved)`
    );
  }, [areaData, setStatusMessage, setSelectedEntity, setSelectedAreaLootKind, setSelectedAreaLootIndex, setActiveTab, setAreaData]);

  const handleDeleteAreaLoot = useCallback(() => {
    const result = deleteAreaLoot(
      areaData,
      selectedAreaLootKind,
      selectedAreaLootIndex
    );
    if (!result.ok) {
      setStatusMessage(result.message);
      return;
    }
    setAreaData(result.data.areaData);
    setSelectedAreaLootKind(null);
    setSelectedAreaLootIndex(null);
    setStatusMessage(
      `Deleted loot ${result.data.kind} ${result.data.index} (unsaved)`
    );
  }, [areaData, selectedAreaLootKind, selectedAreaLootIndex, setStatusMessage, setAreaData, setSelectedAreaLootKind, setSelectedAreaLootIndex]);

  const handleCreateRecipe = useCallback(() => {
    const result = createRecipe(areaData);
    if (!result.ok) {
      setStatusMessage(result.message);
      return;
    }
    setAreaData(result.data.areaData);
    setSelectedEntity("Recipes");
    setSelectedRecipeVnum(result.data.vnum);
    setActiveTab("Form");
    setStatusMessage(`Created recipe ${result.data.vnum} (unsaved)`);
  }, [areaData, setStatusMessage, setSelectedEntity, setSelectedRecipeVnum, setActiveTab, setAreaData]);

  const handleDeleteRecipe = useCallback(() => {
    const result = deleteRecipe(areaData, selectedRecipeVnum);
    if (!result.ok) {
      setStatusMessage(result.message);
      return;
    }
    setAreaData(result.data.areaData);
    setSelectedRecipeVnum(null);
    setStatusMessage(`Deleted recipe ${result.data.deletedVnum} (unsaved)`);
  }, [areaData, selectedRecipeVnum, setStatusMessage, setAreaData, setSelectedRecipeVnum]);

  const handleCreateGatherSpawn = useCallback(() => {
    const result = createGatherSpawn(areaData);
    if (!result.ok) {
      setStatusMessage(result.message);
      return;
    }
    setAreaData(result.data.areaData);
    setSelectedEntity("Gather Spawns");
    setSelectedGatherVnum(result.data.vnum);
    setActiveTab("Form");
    setStatusMessage(`Created gather spawn ${result.data.vnum} (unsaved)`);
  }, [areaData, setStatusMessage, setSelectedEntity, setSelectedGatherVnum, setActiveTab, setAreaData]);

  const handleDeleteGatherSpawn = useCallback(() => {
    const result = deleteGatherSpawn(areaData, selectedGatherVnum);
    if (!result.ok) {
      setStatusMessage(result.message);
      return;
    }
    setAreaData(result.data.areaData);
    setSelectedGatherVnum(null);
    setStatusMessage(`Deleted gather spawn ${result.data.deletedVnum} (unsaved)`);
  }, [areaData, selectedGatherVnum, setStatusMessage, setAreaData, setSelectedGatherVnum]);

  return {
    handleCreateRoom,
    handleDeleteRoom,
    handleCreateMobile,
    handleDeleteMobile,
    handleCreateObject,
    handleDeleteObject,
    handleCreateReset,
    handleDeleteReset,
    handleCreateShop,
    handleDeleteShop,
    handleCreateQuest,
    handleDeleteQuest,
    handleCreateFaction,
    handleDeleteFaction,
    handleCreateAreaLoot,
    handleDeleteAreaLoot,
    handleCreateRecipe,
    handleDeleteRecipe,
    handleCreateGatherSpawn,
    handleDeleteGatherSpawn
  };
};
