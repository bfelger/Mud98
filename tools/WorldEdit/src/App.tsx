import {
  useCallback,
  useEffect,
  useMemo,
  useRef,
  useState,
  type MouseEvent
} from "react";
import type { ColDef, GridApi } from "ag-grid-community";
import {
  type Edge,
  type Node
} from "reactflow";
import { zodResolver } from "@hookform/resolvers/zod";
import {
  useFieldArray,
  useForm,
  useWatch,
  type FieldPath,
} from "react-hook-form";
import { z } from "zod";
import { useSelectionStore } from "./stores/useSelectionStore";
import { useEditorUiStore } from "./stores/useEditorUiStore";
import { useGlobalDataStore } from "./stores/useGlobalDataStore";
import { useAreaStore } from "./stores/useAreaStore";
import { ScriptView } from "./components/ScriptView";
import { AreaForm } from "./components/AreaForm";
import { AreaGraphView } from "./components/AreaGraphView";
import { RoomForm } from "./components/RoomForm";
import { MobileForm } from "./components/MobileForm";
import { ObjectForm } from "./components/ObjectForm";
import { ResetForm } from "./components/ResetForm";
import { ResetEditorFields } from "./components/ResetEditorFields";
import { ShopForm } from "./components/ShopForm";
import { QuestForm } from "./components/QuestForm";
import { FactionForm } from "./components/FactionForm";
import { ClassForm } from "./components/ClassForm";
import { RaceForm } from "./components/RaceForm";
import { SkillForm } from "./components/SkillForm";
import { GroupForm } from "./components/GroupForm";
import { CommandForm } from "./components/CommandForm";
import { SocialForm } from "./components/SocialForm";
import { TutorialForm } from "./components/TutorialForm";
import { RecipeForm } from "./components/RecipeForm";
import { GatherSpawnForm } from "./components/GatherSpawnForm";
import { LootForm } from "./components/LootForm";
import { GlobalFormActions } from "./components/GlobalFormActions";
import { TableView } from "./components/TableView";
import { ClassTableView } from "./components/ClassTableView";
import { RaceTableView } from "./components/RaceTableView";
import { SkillTableView } from "./components/SkillTableView";
import { GroupTableView } from "./components/GroupTableView";
import { CommandTableView } from "./components/CommandTableView";
import { SocialTableView } from "./components/SocialTableView";
import { TutorialTableView } from "./components/TutorialTableView";
import { LootTableView } from "./components/LootTableView";
import { InspectorPanel } from "./components/InspectorPanel";
import { EntityTree } from "./components/EntityTree";
import { Topbar } from "./components/Topbar";
import { Statusbar } from "./components/Statusbar";
import { ViewCardHeader } from "./components/ViewCardHeader";
import { ViewTabs } from "./components/ViewTabs";
import { MapView } from "./components/MapView";
import { ViewBody } from "./components/ViewBody";
import {
  classTitleCount,
  defaultSkillLevel,
  defaultSkillRating,
  groupSkillCount,
  raceSkillCount,
  raceStatKeys
} from "./constants/globalDefaults";
import {
  entityDataKeys,
  entityKindLabels
} from "./constants/appConfig";
import {
  parseVnum,
  parseNumberList,
  formatNumberList,
  formatVnumLabel,
  formatVnumRange,
  normalizeOptionalText,
  normalizeLineEndingsForDisplay,
  normalizeLineEndingsForMud,
  normalizeOptionalMudText,
  normalizeOptionalScript,
  normalizeDice,
  normalizeChecklistStatus,
  getEntityList,
  findByVnum,
  findShopByKeeper,
  getFirstString,
  getAreaVnumRange,
  getAreaVnumBounds,
  getNextEntityVnum,
  getFirstEntityVnum,
  getDefaultSelection,
  resolveResetCommand,
  isResetForRoom,
  formatResetSummary,
  itemTypeBlockMap,
  type DiceFormInput,
  type ObjectBlockKey
} from "./utils/areaUtils";
import {
  validateVnumRanges,
  validateDuplicateVnums,
  validateResetReferences,
  validateOneWayExits,
  validateOrphanRooms,
  buildValidationSummary,
  buildAreaDebugSummary
} from "./validation/areaValidation";
import {
  buildRoomRows,
  roomColumns as roomColumnsDef
} from "./crud/area/roomsCrud";
import {
  buildMobileRows,
  mobileColumns as mobileColumnsDef
} from "./crud/area/mobilesCrud";
import {
  buildObjectRows,
  objectColumns as objectColumnsDef
} from "./crud/area/objectsCrud";
import {
  buildResetRows,
  createReset,
  deleteReset,
  resetColumns as resetColumnsDef
} from "./crud/area/resetsCrud";
import {
  buildShopRows,
  shopColumns as shopColumnsDef
} from "./crud/area/shopsCrud";
import {
  buildFactionRows,
  factionColumns as factionColumnsDef
} from "./crud/area/factionsCrud";
import {
  buildQuestRows,
  questColumns as questColumnsDef
} from "./crud/area/questsCrud";
import {
  buildRecipeRows,
  recipeColumns as recipeColumnsDef
} from "./crud/area/recipesCrud";
import {
  buildGatherSpawnRows,
  gatherSpawnColumns as gatherSpawnColumnsDef
} from "./crud/area/gatherSpawnsCrud";
import { extractAreaLootData } from "./crud/area/areaLootCrud";
import { buildLootRows, lootColumns as lootColumnsDef } from "./crud/loot/lootRows";
import {
  buildClassRows,
  classColumns as classColumnsDef
} from "./crud/global/classesCrud";
import {
  buildRaceRows,
  raceColumns as raceColumnsDef
} from "./crud/global/racesCrud";
import {
  buildSkillRows,
  skillColumns as skillColumnsDef
} from "./crud/global/skillsCrud";
import {
  buildGroupRows,
  groupColumns as groupColumnsDef
} from "./crud/global/groupsCrud";
import {
  buildCommandRows,
  commandColumns as commandColumnsDef
} from "./crud/global/commandsCrud";
import {
  buildSocialRows,
  socialColumns as socialColumnsDef
} from "./crud/global/socialsCrud";
import {
  buildTutorialRows,
  tutorialColumns as tutorialColumnsDef
} from "./crud/global/tutorialsCrud";
import {
  buildTitlePairs,
  normalizeGroupSkills,
  normalizeRaceClassMap,
  normalizeRaceSkills,
  normalizeRaceStats,
  titlesToText
} from "./utils/globalNormalizers";
import { digRoom, linkRooms } from "./map/roomOps";
import {
  roomNodeTypes,
  type RoomLayoutMap,
  type RoomNodeData
} from "./map/roomNodes";
import { type AreaLayoutMap } from "./map/areaNodes";
import { roomEdgeTypes, areaEdgeTypes } from "./map/edgeComponents";
import { buildExitTargetValidation, findAreaForVnum } from "./map/roomEdges";
import type { VnumOption } from "./components/VnumPicker";
import type { EventBinding } from "./data/eventTypes";
import type {
  AreaGraphLink,
  AreaIndexEntry,
  AreaJson,
  ClassDataFile,
  ClassDefinition,
  CommandDataFile,
  CommandDefinition,
  EditorLayout,
  EditorMeta,
  GroupDataFile,
  GroupDefinition,
  LootDataFile,
  LootEntry,
  LootGroup,
  LootOp,
  LootTable,
  ProjectConfig,
  RaceDataFile,
  RaceDefinition,
  RecipeDefinition,
  GatherSpawnDefinition,
  SocialDataFile,
  SocialDefinition,
  TutorialDataFile,
  TutorialDefinition,
  SkillDataFile,
  SkillDefinition,
  ReferenceData
} from "./repository/types";
import type { WorldRepository } from "./repository/worldRepository";
import type {
  ValidationIssue,
  ValidationIssueBase,
  ValidationRule,
  ValidationSeverity
} from "./validation/types";
import {
  buildValidationIssues,
  loadValidationConfig
} from "./validation/registry";
import { loadPluginRules } from "./validation/plugins";
import { areaNodeSize, roomNodeSize } from "./map/graphLayout";
import { useAreaGraphState } from "./hooks/useAreaGraphState";
import { useRoomMapState, type RoomContextMenuState } from "./hooks/useRoomMapState";
import { useAreaCrudHandlers } from "./hooks/useAreaCrudHandlers";
import { useGlobalCrudHandlers } from "./hooks/useGlobalCrudHandlers";
import { useAreaIoHandlers } from "./hooks/useAreaIoHandlers";
import { useGlobalIoHandlers } from "./hooks/useGlobalIoHandlers";
import { useGlobalDataLazyLoad } from "./hooks/useGlobalDataLazyLoad";
import { useGlobalIndexClamp } from "./hooks/useGlobalIndexClamp";
import {
  containerFlagEnum,
  containerFlags,
  damageTypeEnum,
  damageTypes,
  directionEnum,
  directions,
  exitFlagEnum,
  exitFlags,
  actFlags,
  affectFlags,
  offFlags,
  immFlags,
  resFlags,
  vulnFlags,
  skillTargets,
  logFlags,
  showFlags,
  actFlagEnum,
  extraFlagEnum,
  extraFlags,
  furnitureFlagEnum,
  furnitureFlags,
  formFlagEnum,
  formFlags,
  liquidEnum,
  liquids,
  portalFlagEnum,
  portalFlags,
  positionEnum,
  positions,
  partFlagEnum,
  partFlags,
  offFlagEnum,
  roomFlagEnum,
  roomFlags as roomFlagOptions,
  sectorEnum,
  sectors,
  sexEnum,
  sexes,
  sizeEnum,
  sizes,
  wearLocationEnum,
  wearLocations,
  wearFlagEnum,
  wearFlags,
  weaponClassEnum,
  weaponClasses,
  weaponFlagEnum,
  weaponFlags,
  workstationTypes,
  discoveryTypes
} from "./schemas/enums";
import {
  optionalIntSchema,
  optionalEnumSchema,
  optionalSectorSchema,
  optionalPositionSchema,
  optionalSexSchema,
  optionalSizeSchema,
  optionalDamageTypeSchema,
  optionalDirectionSchema,
  optionalWearLocationSchema,
  checklistStatusOptions,
  defaultDaycyclePeriods,
  areaFormSchema,
  exitFormSchema,
  daycyclePeriodFormSchema,
  diceFormSchema,
  mobileFormSchema,
  weaponFormSchema,
  armorFormSchema,
  containerFormSchema,
  lightFormSchema,
  drinkFormSchema,
  foodFormSchema,
  moneyFormSchema,
  wandFormSchema,
  spellsFormSchema,
  portalFormSchema,
  furnitureFormSchema,
  objectFormSchema,
  resetFormSchema,
  roomFormSchema,
  shopFormSchema,
  questFormSchema,
  factionFormSchema,
  classFormSchema,
  raceFormSchema,
  skillFormSchema,
  groupFormSchema,
  commandFormSchema,
  socialFormSchema,
  tutorialFormSchema,
  lootEntryFormSchema,
  lootOpFormSchema,
  lootFormSchema,
  recipeInputFormSchema,
  recipeFormSchema,
  gatherSpawnFormSchema
} from "./schemas/formSchemas";
import type {
  RoomFormValues,
  AreaFormValues,
  MobileFormValues,
  ObjectFormValues,
  ResetFormValues,
  RoomResetItem,
  ShopFormValues,
  QuestFormValues,
  FactionFormValues,
  ClassFormValues,
  RaceFormValues,
  SkillFormValues,
  GroupFormValues,
  CommandFormValues,
  SocialFormValues,
  TutorialFormValues,
  LootFormValues,
  RecipeFormValues,
  GatherSpawnFormValues
} from "./schemas/formTypes";
import {
  tabs,
  entityOrder,
  globalEntityOrder,
  primeStatOptions,
  armorProfOptions,
  classGuildCount,
  lootEntryTypeOptions,
  lootOpTypeOptions,
  itemTypeOptions,
  resetCommandOptions,
  questTypeOptions,
  type ResetCommand,
  resetCommandMap,
  resetCommandLabels,
  areaDirectionHandleMap,
  externalDirectionLabels,
  type EntityKey,
  type GlobalEntityKey,
  type EditorMode
} from "./constants/appConfig";

type AppProps = {
  repository: WorldRepository;
};

type TabId = (typeof tabs)[number]["id"];

type RoomContextMenuState = {
  vnum: number;
  x: number;
  y: number;
};


function fileNameFromPath(path: string | null): string {
  if (!path) {
    return "No area loaded";
  }
  const normalized = path.replace(/[/\\\\]+/g, "/");
  return normalized.split("/").pop() ?? path;
}

function buildEditorMeta(
  existing: EditorMeta | null,
  activeTab: string,
  selectedEntity: string,
  roomLayout: RoomLayoutMap,
  areaLayout: AreaLayoutMap
): EditorMeta {
  const now = new Date().toISOString();
  const base: EditorMeta = existing ?? {
    version: 1,
    updatedAt: now,
    view: { activeTab },
    selection: { entityType: selectedEntity },
    layout: {}
  };
  const nextLayout: EditorLayout = {
    ...(base.layout ?? {}),
    rooms: roomLayout,
    areas: areaLayout
  };

  return {
    ...base,
    version: 1,
    updatedAt: now,
    view: {
      ...base.view,
      activeTab
    },
    selection: {
      ...base.selection,
      entityType: selectedEntity
    },
    layout: nextLayout
  };
}


function buildVnumOptions(
  areaData: AreaJson | null,
  entity: "Rooms" | "Mobiles" | "Objects"
): VnumOption[] {
  if (!areaData) {
    return [];
  }
  const list = getEntityList(areaData, entity);
  const options: VnumOption[] = [];
  const entityLabel =
    entity === "Rooms" ? "Room" : entity === "Mobiles" ? "Mobile" : "Object";
  for (const entry of list) {
    if (!entry || typeof entry !== "object") {
      continue;
    }
    const record = entry as Record<string, unknown>;
    const vnum = parseVnum(record.vnum);
    if (vnum === null) {
      continue;
    }
    let name = "VNUM";
    if (entity === "Rooms") {
      name = getFirstString(record.name, "(unnamed room)");
    } else if (entity === "Mobiles") {
      name = getFirstString(record.shortDescr, "(unnamed mobile)");
    } else {
      name = getFirstString(record.shortDescr, "(unnamed object)");
    }
    options.push({
      vnum,
      label: `${entityLabel} · ${name}`,
      entityType: entityLabel,
      name
    });
  }
  return options;
}

function buildSelectionSummary(
  selectedEntity: EntityKey,
  areaData: AreaJson | null,
  selectedVnums: Partial<Record<EntityKey, number | null>>,
  lootContext?: {
    data: LootDataFile | null;
    kind: "group" | "table" | null;
    index: number | null;
  }
) {
  const list = getEntityList(areaData, selectedEntity);
  const count = list.length;
  const selectedValue =
    selectedVnums[selectedEntity] !== null &&
    selectedVnums[selectedEntity] !== undefined
      ? (selectedVnums[selectedEntity] as number)
      : null;
  const selected =
    selectedValue !== null
      ? selectedEntity === "Resets"
        ? (list[selectedValue] as Record<string, unknown> | undefined) ?? null
        : selectedEntity === "Shops"
          ? findShopByKeeper(list, selectedValue)
          : findByVnum(list, selectedValue)
      : null;
  const first = (selected ?? list[0] ?? {}) as Record<string, unknown>;
  const vnumRange = getAreaVnumRange(areaData);
  const vnum = parseVnum(first.vnum);
  const emptyLabel = `No ${selectedEntity.toLowerCase()} loaded`;

  let selectionLabel = emptyLabel;
  let flags: string[] = [];
  let exits = "n/a";
  const lootData = lootContext?.data ?? null;
  const lootKind = lootContext?.kind ?? null;
  const lootIndex = lootContext?.index ?? null;

  switch (selectedEntity) {
    case "Area": {
      selectionLabel = count
        ? getFirstString(first.name, "Area data")
        : emptyLabel;
      exits = "n/a";
      break;
    }
    case "Rooms": {
      selectionLabel =
        vnum !== null
          ? `${vnum} - ${getFirstString(first.name, "(unnamed room)")}`
          : emptyLabel;
      flags = Array.isArray(first.roomFlags)
        ? first.roomFlags.filter((flag) => typeof flag === "string")
        : [];
      const exitList = Array.isArray(first.exits) ? first.exits : [];
      exits =
        exitList.length > 0
          ? exitList
              .map((exit) =>
                typeof exit === "object" && exit
                  ? (exit as Record<string, unknown>).dir
                  : null
              )
              .filter((dir): dir is string => typeof dir === "string")
              .join(", ")
          : "none";
      break;
    }
    case "Mobiles": {
      selectionLabel =
        vnum !== null
          ? `${vnum} - ${getFirstString(first.shortDescr, "(unnamed mobile)")}`
          : emptyLabel;
      flags = Array.isArray(first.actFlags)
        ? first.actFlags.filter((flag) => typeof flag === "string")
        : [];
      break;
    }
    case "Objects": {
      selectionLabel =
        vnum !== null
          ? `${vnum} - ${getFirstString(first.shortDescr, "(unnamed object)")}`
          : emptyLabel;
      flags = Array.isArray(first.wearFlags)
        ? first.wearFlags.filter((flag) => typeof flag === "string")
        : [];
      break;
    }
    case "Resets": {
      const commandName = getFirstString(first.commandName, "reset");
      const roomVnum = parseVnum(first.roomVnum);
      selectionLabel =
        roomVnum !== null
          ? `${commandName} -> room ${roomVnum}`
          : count
            ? commandName
            : emptyLabel;
      break;
    }
    case "Shops": {
      const keeper = parseVnum(first.keeper);
      selectionLabel =
        keeper !== null ? `Keeper ${keeper}` : count ? "Shop entry" : emptyLabel;
      break;
    }
    case "Quests": {
      selectionLabel =
        vnum !== null
          ? `${vnum} - ${getFirstString(first.name, "Quest")}`
          : count
            ? "Quest entry"
            : emptyLabel;
      break;
    }
    case "Factions": {
      selectionLabel =
        vnum !== null
          ? `${vnum} - ${getFirstString(first.name, "Faction")}`
          : count
            ? "Faction entry"
            : emptyLabel;
      break;
    }
    case "Loot": {
      if (!lootData || lootKind === null || lootIndex === null) {
        selectionLabel = count ? "Loot entry" : emptyLabel;
        break;
      }
      if (lootKind === "group") {
        const group = lootData.groups[lootIndex];
        selectionLabel = group
          ? `Group: ${getFirstString(group.name, "(unnamed)")}`
          : emptyLabel;
        break;
      }
      const table = lootData.tables[lootIndex];
      selectionLabel = table
        ? `Table: ${getFirstString(table.name, "(unnamed)")}`
        : emptyLabel;
      break;
    }
    case "Recipes": {
      selectionLabel =
        vnum !== null
          ? `${vnum} - ${getFirstString(first.name, "Recipe")}`
          : count
            ? "Recipe entry"
            : emptyLabel;
      break;
    }
    case "Gather Spawns": {
      selectionLabel =
        vnum !== null
          ? `${vnum} - ${getFirstString(first.spawnSector, "spawn")}`
          : count
            ? "Gather spawn entry"
            : emptyLabel;
      break;
    }
    case "Helps": {
      const keyword = getFirstString(first.keyword, "help");
      selectionLabel = count ? `help: ${keyword}` : emptyLabel;
      break;
    }
    default:
      break;
  }

  const range =
    selectedEntity === "Rooms" ||
    selectedEntity === "Mobiles" ||
    selectedEntity === "Objects" ||
    selectedEntity === "Area"
      ? vnumRange ?? "n/a"
      : "n/a";

  return {
    kindLabel: entityKindLabels[selectedEntity],
    selectionLabel,
    vnumRange: range,
    lastSave: areaData ? "loaded" : "n/a",
    flags,
    exits,
    validation: "validation not run"
  };
}

function getDominantExitDirection(
  directionCounts: Partial<Record<string, number>> | null | undefined
): string | null {
  if (!directionCounts) {
    return null;
  }
  const order = ["north", "east", "south", "west", "up", "down"];
  let best: string | null = null;
  let bestCount = -1;
  order.forEach((dir) => {
    const count = directionCounts[dir];
    if (typeof count !== "number") {
      return;
    }
    if (count > bestCount) {
      best = dir;
      bestCount = count;
    }
  });
  return best;
}

function cleanOptionalString(value: string | undefined): string | undefined {
  if (!value) {
    return undefined;
  }
  const trimmed = value.trim();
  return trimmed.length ? trimmed : undefined;
}

function syncGridSelection(
  api: GridApi | null,
  rowId: number | string | null
) {
  if (!api) {
    return;
  }
  api.deselectAll();
  if (rowId === null) {
    return;
  }
  const node = api.getRowNode(String(rowId));
  if (node) {
    node.setSelected(true);
  }
}

export default function App({ repository }: AppProps) {
  // UI state from Zustand store
  const {
    activeTab,
    setActiveTab,
    statusMessage,
    setStatusMessage,
    errorMessage,
    setErrorMessage,
    isBusy,
    setIsBusy,
    diagnosticFilter,
    setDiagnosticFilter
  } = useEditorUiStore();

  // Selection state from Zustand store
  const {
    editorMode,
    setEditorMode,
    selectedEntity,
    setSelectedEntity,
    selectedGlobalEntity,
    setSelectedGlobalEntity,
    selectedClassIndex,
    setSelectedClassIndex,
    selectedRaceIndex,
    setSelectedRaceIndex,
    selectedSkillIndex,
    setSelectedSkillIndex,
    selectedGroupIndex,
    setSelectedGroupIndex,
    selectedCommandIndex,
    setSelectedCommandIndex,
    selectedSocialIndex,
    setSelectedSocialIndex,
    selectedTutorialIndex,
    setSelectedTutorialIndex,
    selectedGlobalLoot,
    setSelectedGlobalLoot,
    selectedMobileVnum,
    setSelectedMobileVnum,
    selectedObjectVnum,
    setSelectedObjectVnum,
    selectedResetIndex,
    setSelectedResetIndex,
    selectedShopKeeper,
    setSelectedShopKeeper,
    selectedQuestVnum,
    setSelectedQuestVnum,
    selectedFactionVnum,
    setSelectedFactionVnum,
    selectedAreaLoot,
    setSelectedAreaLoot,
    selectedRecipeVnum,
    setSelectedRecipeVnum,
    selectedGatherVnum,
    setSelectedGatherVnum,
    clearAreaSelections
  } = useSelectionStore();

  // Global data state from Zustand store
  const {
    classData,
    setClassData,
    classDataPath,
    setClassDataPath,
    classDataFormat,
    setClassDataFormat,
    classDataDir,
    setClassDataDir,
    raceData,
    setRaceData,
    raceDataPath,
    setRaceDataPath,
    raceDataFormat,
    setRaceDataFormat,
    raceDataDir,
    setRaceDataDir,
    skillData,
    setSkillData,
    skillDataPath,
    setSkillDataPath,
    skillDataFormat,
    setSkillDataFormat,
    skillDataDir,
    setSkillDataDir,
    groupData,
    setGroupData,
    groupDataPath,
    setGroupDataPath,
    groupDataFormat,
    setGroupDataFormat,
    groupDataDir,
    setGroupDataDir,
    commandData,
    setCommandData,
    commandDataPath,
    setCommandDataPath,
    commandDataFormat,
    setCommandDataFormat,
    commandDataDir,
    setCommandDataDir,
    socialData,
    setSocialData,
    socialDataPath,
    setSocialDataPath,
    socialDataFormat,
    setSocialDataFormat,
    socialDataDir,
    setSocialDataDir,
    tutorialData,
    setTutorialData,
    tutorialDataPath,
    setTutorialDataPath,
    tutorialDataFormat,
    setTutorialDataFormat,
    tutorialDataDir,
    setTutorialDataDir,
    lootData,
    setLootData,
    lootDataPath,
    setLootDataPath,
    lootDataFormat,
    setLootDataFormat,
    lootDataDir,
    setLootDataDir,
    referenceData,
    setReferenceData,
    projectConfig,
    setProjectConfig,
    editorMeta,
    setEditorMeta,
    editorMetaPath,
    setEditorMetaPath,
    dataDirectory,
    setDataDirectory
  } = useGlobalDataStore();

  // Aliases for backward compatibility with existing code
  const selectedLootKind = selectedGlobalLoot.kind;
  const selectedLootIndex = selectedGlobalLoot.index;
  const setSelectedLootKind = (kind: "group" | "table" | null) =>
    setSelectedGlobalLoot({ ...selectedGlobalLoot, kind });
  const setSelectedLootIndex = (index: number | null) =>
    setSelectedGlobalLoot({ ...selectedGlobalLoot, index });
  const selectedAreaLootKind = selectedAreaLoot.kind;
  const selectedAreaLootIndex = selectedAreaLoot.index;
  const setSelectedAreaLootKind = (kind: "group" | "table" | null) =>
    setSelectedAreaLoot({ ...selectedAreaLoot, kind });
  const setSelectedAreaLootIndex = (index: number | null) =>
    setSelectedAreaLoot({ ...selectedAreaLoot, index });

  // Area data from Zustand store
  const {
    areaPath,
    setAreaPath,
    areaData,
    setAreaData,
    areaDirectory,
    setAreaDirectory,
    areaIndex,
    setAreaIndex,
    areaGraphLinks,
    setAreaGraphLinks
  } = useAreaStore();

  const roomGridApi = useRef<GridApi | null>(null);
  const mobileGridApi = useRef<GridApi | null>(null);
  const objectGridApi = useRef<GridApi | null>(null);
  const resetGridApi = useRef<GridApi | null>(null);
  const shopGridApi = useRef<GridApi | null>(null);
  const questGridApi = useRef<GridApi | null>(null);
  const factionGridApi = useRef<GridApi | null>(null);
  const areaLootGridApi = useRef<GridApi | null>(null);
  const recipeGridApi = useRef<GridApi | null>(null);
  const gatherGridApi = useRef<GridApi | null>(null);
  const classGridApi = useRef<GridApi | null>(null);
  const raceGridApi = useRef<GridApi | null>(null);
  const skillGridApi = useRef<GridApi | null>(null);
  const groupGridApi = useRef<GridApi | null>(null);
  const commandGridApi = useRef<GridApi | null>(null);
  const socialGridApi = useRef<GridApi | null>(null);
  const tutorialGridApi = useRef<GridApi | null>(null);
  const lootGridApi = useRef<GridApi | null>(null);
  const exitValidation = useMemo(
    () => buildExitTargetValidation(areaData, areaIndex),
    [areaData, areaIndex]
  );
  const {
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
    applyLoadedRoomLayout,
    autoLayoutEnabled,
    setAutoLayoutEnabled,
    preferCardinalLayout,
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
    removeLayoutNode: removeRoomLayout
  } = useRoomMapState({
    areaData,
    areaIndex,
    exitInvalidEdgeIds: exitValidation.invalidEdgeIds,
    activeTab,
    selectedEntity,
    setSelectedEntity,
    parseVnum
  });
  const {
    areaGraphFilter,
    setAreaGraphFilter,
    areaGraphVnumQuery,
    setAreaGraphVnumQuery,
    areaGraphMatchLabel,
    areaGraphEdges,
    worldMapNodes,
    selectedAreaNode,
    selectedAreaLocked,
    dirtyAreaCount,
    hasAreaLayout,
    areaLayout,
    preferAreaCardinalLayout,
    applyLoadedAreaLayout,
    handleTogglePreferAreaGrid,
    handleLockSelectedArea,
    handleUnlockSelectedArea,
    handleLockDirtyAreas,
    handleClearAreaLayout,
    handleRelayoutArea,
    handleAreaGraphNodesChange,
    handleAreaGraphNodeDragStop
  } = useAreaGraphState({
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
  });
  const {
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
  } = useAreaCrudHandlers({
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
  });
  const {
    handleCreateClass,
    handleDeleteClass,
    handleCreateRace,
    handleDeleteRace,
    handleCreateSkill,
    handleDeleteSkill,
    handleCreateGroup,
    handleDeleteGroup,
    handleCreateCommand,
    handleDeleteCommand,
    handleCreateSocial,
    handleDeleteSocial,
    handleCreateTutorial,
    handleDeleteTutorial,
    handleCreateLoot,
    handleDeleteLoot
  } = useGlobalCrudHandlers({
    classData,
    raceData,
    skillData,
    groupData,
    commandData,
    socialData,
    tutorialData,
    lootData,
    referenceData,
    selectedClassIndex,
    selectedRaceIndex,
    selectedSkillIndex,
    selectedGroupIndex,
    selectedCommandIndex,
    selectedSocialIndex,
    selectedTutorialIndex,
    selectedLootKind,
    selectedLootIndex,
    setClassData,
    setRaceData,
    setSkillData,
    setGroupData,
    setCommandData,
    setSocialData,
    setTutorialData,
    setLootData,
    setSelectedGlobalEntity,
    setSelectedClassIndex,
    setSelectedRaceIndex,
    setSelectedSkillIndex,
    setSelectedGroupIndex,
    setSelectedCommandIndex,
    setSelectedSocialIndex,
    setSelectedTutorialIndex,
    setSelectedLootKind,
    setSelectedLootIndex,
    setActiveTab,
    setStatusMessage
  });
  const {
    handleLoadClassesData,
    handleSaveClassesData,
    handleLoadRacesData,
    handleSaveRacesData,
    handleLoadSkillsData,
    handleSaveSkillsData,
    handleLoadGroupsData,
    handleSaveGroupsData,
    handleLoadCommandsData,
    handleSaveCommandsData,
    handleLoadSocialsData,
    handleSaveSocialsData,
    handleLoadTutorialsData,
    handleSaveTutorialsData,
    handleLoadLootData,
    handleSaveLootData
  } = useGlobalIoHandlers({
    dataDirectory,
    projectConfig,
    referenceData,
    repository,
    classData,
    classDataFormat,
    raceData,
    raceDataFormat,
    skillData,
    skillDataFormat,
    groupData,
    groupDataFormat,
    commandData,
    commandDataFormat,
    socialData,
    socialDataFormat,
    tutorialData,
    tutorialDataFormat,
    lootData,
    lootDataFormat,
    setClassData,
    setClassDataPath,
    setClassDataFormat,
    setClassDataDir,
    setRaceData,
    setRaceDataPath,
    setRaceDataFormat,
    setRaceDataDir,
    setSkillData,
    setSkillDataPath,
    setSkillDataFormat,
    setSkillDataDir,
    setGroupData,
    setGroupDataPath,
    setGroupDataFormat,
    setGroupDataDir,
    setCommandData,
    setCommandDataPath,
    setCommandDataFormat,
    setCommandDataDir,
    setSocialData,
    setSocialDataPath,
    setSocialDataFormat,
    setSocialDataDir,
    setTutorialData,
    setTutorialDataPath,
    setTutorialDataFormat,
    setTutorialDataDir,
    setLootData,
    setLootDataPath,
    setLootDataFormat,
    setLootDataDir,
    setStatusMessage,
    setErrorMessage,
    setIsBusy
  });
  const {
    handleOpenProject,
    handleOpenArea,
    handleSaveArea,
    handleSaveAreaAs,
    handleSaveEditorMeta,
    handleSetAreaDirectory
  } = useAreaIoHandlers({
    areaDirectory,
    dataDirectory,
    projectConfig,
    areaPath,
    areaData,
    editorMeta,
    activeTab,
    selectedEntity,
    roomLayout,
    areaLayout,
    repository,
    setProjectConfig,
    setAreaDirectory,
    setDataDirectory,
    setReferenceData,
    setAreaPath,
    setAreaData,
    setEditorMeta,
    setEditorMetaPath,
    setStatusMessage,
    setErrorMessage,
    setIsBusy,
    applyLoadedRoomLayout,
    applyLoadedAreaLayout,
    buildEditorMeta,
    fileNameFromPath
  });
  const areaForm = useForm<AreaFormValues>({
    resolver: zodResolver(areaFormSchema),
    defaultValues: {
      name: "",
      version: 1,
      vnumRangeStart: 0,
      vnumRangeEnd: 0,
      builders: "",
      credits: "",
      security: undefined,
      sector: undefined,
      lowLevel: undefined,
      highLevel: undefined,
      reset: undefined,
      alwaysReset: false,
      instType: "single",
      daycycleSuppress: false,
      daycyclePeriods: [],
      storyBeats: [],
      checklist: []
    }
  });
  const {
    register: registerArea,
    handleSubmit: handleAreaSubmitForm,
    formState: areaFormState,
    control: areaFormControl
  } = areaForm;
  const {
    fields: storyBeatFields,
    append: appendStoryBeat,
    remove: removeStoryBeat,
    move: moveStoryBeat
  } = useFieldArray({
    control: areaFormControl,
    name: "storyBeats"
  });
  const {
    fields: checklistFields,
    append: appendChecklist,
    remove: removeChecklist,
    move: moveChecklist
  } = useFieldArray({
    control: areaFormControl,
    name: "checklist"
  });
  const {
    fields: daycycleFields,
    append: appendDaycycle,
    remove: removeDaycycle,
    move: moveDaycycle
  } = useFieldArray({
    control: areaFormControl,
    name: "daycyclePeriods"
  });
  const roomForm = useForm<RoomFormValues>({
    resolver: zodResolver(roomFormSchema),
    defaultValues: {
      vnum: 0,
      name: "",
      description: "",
      sectorType: undefined,
      roomFlags: [],
      suppressDaycycleMessages: false,
      timePeriods: [],
      exits: []
    }
  });
  const {
    register: registerRoom,
    handleSubmit: handleRoomSubmitForm,
    formState: roomFormState,
    control: roomFormControl
  } = roomForm;
  const {
    fields: exitFields,
    append: appendExit,
    remove: removeExit
  } = useFieldArray({
    control: roomFormControl,
    name: "exits"
  });
  const {
    fields: timePeriodFields,
    append: appendTimePeriod,
    remove: removeTimePeriod,
    move: moveTimePeriod
  } = useFieldArray({
    control: roomFormControl,
    name: "timePeriods"
  });
  const handleAddDefaultAreaPeriods = () => {
    appendDaycycle(defaultDaycyclePeriods as unknown as Record<string, unknown>[]);
  };
  const handleAddDefaultRoomPeriods = () => {
    appendTimePeriod(
      defaultDaycyclePeriods as unknown as Record<string, unknown>[]
    );
  };
  const mobileForm = useForm<MobileFormValues>({
    resolver: zodResolver(mobileFormSchema),
    defaultValues: {
      vnum: 0,
      name: "",
      shortDescr: "",
      longDescr: "",
      description: "",
      race: "",
      level: undefined,
      hitroll: undefined,
      alignment: undefined,
      damType: undefined,
      startPos: undefined,
      defaultPos: undefined,
      sex: undefined,
      size: undefined,
      material: "",
      factionVnum: undefined,
      damageNoun: "",
      offensiveSpell: "",
      hitDice: { number: undefined, type: undefined, bonus: undefined },
      manaDice: { number: undefined, type: undefined, bonus: undefined },
      damageDice: { number: undefined, type: undefined, bonus: undefined }
    }
  });
  const {
    register: registerMobile,
    handleSubmit: handleMobileSubmitForm,
    formState: mobileFormState,
    setValue: setMobileFormValue,
    getValues: getMobileFormValues
  } = mobileForm;
  const [overrideMobileFormFlags, setOverrideMobileFormFlags] = useState(false);
  const [overrideMobilePartFlags, setOverrideMobilePartFlags] = useState(false);
  const objectForm = useForm<ObjectFormValues>({
    resolver: zodResolver(objectFormSchema),
    defaultValues: {
      vnum: 0,
      name: "",
      shortDescr: "",
      description: "",
      material: "",
      itemType: "",
      extraFlags: [],
      wearFlags: [],
      level: undefined,
      weight: undefined,
      cost: undefined,
      condition: undefined,
      weapon: {
        class: undefined,
        diceNumber: undefined,
        diceType: undefined,
        damageType: undefined,
        flags: []
      },
      armor: {
        acPierce: undefined,
        acBash: undefined,
        acSlash: undefined,
        acExotic: undefined
      },
      container: {
        capacity: undefined,
        flags: [],
        keyVnum: undefined,
        maxWeight: undefined,
        weightMult: undefined
      },
      light: {
        hours: undefined
      },
      drink: {
        capacity: undefined,
        remaining: undefined,
        liquid: undefined,
        poisoned: undefined
      },
      fountain: {
        capacity: undefined,
        remaining: undefined,
        liquid: undefined,
        poisoned: undefined
      },
      food: {
        foodHours: undefined,
        fullHours: undefined,
        poisoned: undefined
      },
      money: {
        gold: undefined,
        silver: undefined
      },
      wand: {
        level: undefined,
        totalCharges: undefined,
        chargesLeft: undefined,
        spell: ""
      },
      staff: {
        level: undefined,
        totalCharges: undefined,
        chargesLeft: undefined,
        spell: ""
      },
      spells: {
        level: undefined,
        spell1: "",
        spell2: "",
        spell3: "",
        spell4: ""
      },
      portal: {
        charges: undefined,
        exitFlags: [],
        portalFlags: [],
        toVnum: undefined
      },
      furniture: {
        slots: undefined,
        weight: undefined,
        flags: [],
        healBonus: undefined,
        manaBonus: undefined,
        maxPeople: undefined
      }
    }
  });
  const {
    register: registerObject,
    handleSubmit: handleObjectSubmitForm,
    formState: objectFormState,
    control: objectFormControl
  } = objectForm;
  const resetForm = useForm<ResetFormValues>({
    resolver: zodResolver(resetFormSchema),
    defaultValues: {
      index: 0,
      commandName: resetCommandOptions[0],
      mobVnum: undefined,
      objVnum: undefined,
      roomVnum: undefined,
      containerVnum: undefined,
      maxInArea: undefined,
      maxInRoom: undefined,
      count: undefined,
      maxInContainer: undefined,
      wearLoc: undefined,
      direction: undefined,
      state: undefined,
      exits: undefined
    }
  });
  const {
    register: registerReset,
    handleSubmit: handleResetSubmitForm,
    formState: resetFormState,
    control: resetFormControl
  } = resetForm;
  const shopForm = useForm<ShopFormValues>({
    resolver: zodResolver(shopFormSchema),
    defaultValues: {
      keeper: undefined,
      buyTypes: [0, 0, 0, 0, 0],
      profitBuy: undefined,
      profitSell: undefined,
      openHour: undefined,
      closeHour: undefined
    }
  });
  const {
    register: registerShop,
    handleSubmit: handleShopSubmitForm,
    formState: shopFormState,
    control: shopFormControl
  } = shopForm;
  const questForm = useForm<QuestFormValues>({
    resolver: zodResolver(questFormSchema),
    defaultValues: {
      vnum: 0,
      name: "",
      entry: "",
      type: "",
      xp: undefined,
      level: undefined,
      end: undefined,
      target: undefined,
      upper: undefined,
      count: undefined,
      rewardFaction: undefined,
      rewardReputation: undefined,
      rewardGold: undefined,
      rewardSilver: undefined,
      rewardCopper: undefined,
      rewardObjs: [0, 0, 0],
      rewardCounts: [0, 0, 0]
    }
  });
  const {
    register: registerQuest,
    handleSubmit: handleQuestSubmitForm,
    formState: questFormState,
    control: questFormControl
  } = questForm;
  const factionForm = useForm<FactionFormValues>({
    resolver: zodResolver(factionFormSchema),
    defaultValues: {
      vnum: 0,
      name: "",
      defaultStanding: undefined,
      alliesCsv: "",
      opposingCsv: ""
    }
  });
  const {
    register: registerFaction,
    handleSubmit: handleFactionSubmitForm,
    formState: factionFormState
  } = factionForm;
  const recipeForm = useForm<RecipeFormValues>({
    resolver: zodResolver(recipeFormSchema),
    defaultValues: {
      vnum: 0,
      name: "",
      skill: "",
      minSkill: undefined,
      minSkillPct: undefined,
      minLevel: undefined,
      stationType: [],
      stationVnum: undefined,
      discovery: undefined,
      inputs: [],
      outputVnum: undefined,
      outputQuantity: undefined
    }
  });
  const {
    register: registerRecipe,
    handleSubmit: handleRecipeSubmitForm,
    formState: recipeFormState,
    reset: resetRecipeForm,
    control: recipeFormControl
  } = recipeForm;
  const {
    fields: recipeInputFields,
    append: appendRecipeInput,
    remove: removeRecipeInput,
    move: moveRecipeInput
  } = useFieldArray({
    control: recipeFormControl,
    name: "inputs"
  });
  const gatherSpawnForm = useForm<GatherSpawnFormValues>({
    resolver: zodResolver(gatherSpawnFormSchema),
    defaultValues: {
      spawnSector: undefined,
      vnum: undefined,
      quantity: undefined,
      respawnTimer: undefined
    }
  });
  const {
    register: registerGatherSpawn,
    handleSubmit: handleGatherSpawnSubmitForm,
    formState: gatherSpawnFormState,
    reset: resetGatherSpawnForm,
    control: gatherSpawnFormControl
  } = gatherSpawnForm;
  const classFormDefaults = useMemo<ClassFormValues>(
    () => ({
      name: "",
      whoName: "",
      baseGroup: "",
      defaultGroup: "",
      weaponVnum: 0,
      armorProf: "",
      guilds: new Array(classGuildCount).fill(0),
      primeStat: "",
      skillCap: 0,
      thac0_00: 0,
      thac0_32: 0,
      hpMin: 0,
      hpMax: 0,
      manaUser: false,
      startLoc: 0,
      titlesMale: "",
      titlesFemale: ""
    }),
    []
  );
  const classForm = useForm<ClassFormValues>({
    resolver: zodResolver(classFormSchema),
    defaultValues: classFormDefaults
  });
  const {
    register: registerClass,
    handleSubmit: handleClassSubmitForm,
    formState: classFormState,
    reset: resetClassForm
  } = classForm;
  const raceClassNames = referenceData?.classes ?? [];
  const raceFormDefaults = useMemo<RaceFormValues>(
    () => ({
      name: "",
      whoName: "",
      pc: false,
      points: 0,
      size: "",
      startLoc: 0,
      stats: {
        str: 0,
        int: 0,
        wis: 0,
        dex: 0,
        con: 0
      },
      maxStats: {
        str: 0,
        int: 0,
        wis: 0,
        dex: 0,
        con: 0
      },
      actFlags: [],
      affectFlags: [],
      offFlags: [],
      immFlags: [],
      resFlags: [],
      vulnFlags: [],
      formFlags: [],
      partFlags: [],
      classMult: Object.fromEntries(
        raceClassNames.map((name) => [name, 100])
      ),
      classStart: Object.fromEntries(
        raceClassNames.map((name) => [name, 0])
      ),
      skills: new Array(raceSkillCount).fill("")
    }),
    [raceClassNames]
  );
  const raceForm = useForm<RaceFormValues>({
    resolver: zodResolver(raceFormSchema),
    defaultValues: raceFormDefaults
  });
  const {
    register: registerRace,
    handleSubmit: handleRaceSubmitForm,
    formState: raceFormState,
    reset: resetRaceForm
  } = raceForm;
  const skillClassNames = referenceData?.classes ?? [];
  const skillFormDefaults = useMemo<SkillFormValues>(
    () => ({
      name: "",
      levels: Object.fromEntries(
        skillClassNames.map((name) => [name, defaultSkillLevel])
      ),
      ratings: Object.fromEntries(
        skillClassNames.map((name) => [name, defaultSkillRating])
      ),
      spell: "",
      loxSpell: "",
      target: "",
      minPosition: "",
      gsn: "",
      slot: 0,
      minMana: 0,
      beats: 0,
      nounDamage: "",
      msgOff: "",
      msgObj: ""
    }),
    [skillClassNames]
  );
  const skillForm = useForm<SkillFormValues>({
    resolver: zodResolver(skillFormSchema),
    defaultValues: skillFormDefaults
  });
  const {
    register: registerSkill,
    handleSubmit: handleSkillSubmitForm,
    formState: skillFormState,
    reset: resetSkillForm
  } = skillForm;
  const groupClassNames = referenceData?.classes ?? [];
  const groupSkillOptions = referenceData?.skills ?? [];
  const groupFormDefaults = useMemo<GroupFormValues>(
    () => ({
      name: "",
      ratings: Object.fromEntries(
        groupClassNames.map((name) => [name, defaultSkillRating])
      ),
      skills: new Array(groupSkillCount).fill("")
    }),
    [groupClassNames]
  );
  const groupForm = useForm<GroupFormValues>({
    resolver: zodResolver(groupFormSchema),
    defaultValues: groupFormDefaults
  });
  const {
    register: registerGroup,
    handleSubmit: handleGroupSubmitForm,
    formState: groupFormState,
    reset: resetGroupForm
  } = groupForm;
  const commandFormDefaults = useMemo<CommandFormValues>(
    () => ({
      name: "",
      function: "",
      position: "",
      level: 0,
      log: "",
      category: "",
      loxFunction: ""
    }),
    []
  );
  const commandForm = useForm<CommandFormValues>({
    resolver: zodResolver(commandFormSchema),
    defaultValues: commandFormDefaults
  });
  const {
    register: registerCommand,
    handleSubmit: handleCommandSubmitForm,
    formState: commandFormState,
    reset: resetCommandForm
  } = commandForm;
  const socialFormDefaults = useMemo<SocialFormValues>(
    () => ({
      name: "",
      charNoArg: "",
      othersNoArg: "",
      charFound: "",
      othersFound: "",
      victFound: "",
      charAuto: "",
      othersAuto: ""
    }),
    []
  );
  const socialForm = useForm<SocialFormValues>({
    resolver: zodResolver(socialFormSchema),
    defaultValues: socialFormDefaults
  });
  const {
    register: registerSocial,
    handleSubmit: handleSocialSubmitForm,
    formState: socialFormState,
    reset: resetSocialForm
  } = socialForm;
  const tutorialFormDefaults = useMemo<TutorialFormValues>(
    () => ({
      name: "",
      blurb: "",
      finish: "",
      minLevel: 0,
      steps: []
    }),
    []
  );
  const tutorialForm = useForm<TutorialFormValues>({
    resolver: zodResolver(tutorialFormSchema),
    defaultValues: tutorialFormDefaults
  });
  const {
    register: registerTutorial,
    handleSubmit: handleTutorialSubmitForm,
    formState: tutorialFormState,
    reset: resetTutorialForm,
    control: tutorialFormControl
  } = tutorialForm;
  const {
    fields: tutorialStepFields,
    append: appendTutorialStep,
    remove: removeTutorialStep,
    move: moveTutorialStep
  } = useFieldArray({
    control: tutorialFormControl,
    name: "steps"
  });
  const lootFormDefaults = useMemo<LootFormValues>(
    () => ({
      kind: "group",
      name: "",
      rolls: 1,
      entries: [],
      parent: "",
      ops: []
    }),
    []
  );
  const areaLootForm = useForm<LootFormValues>({
    resolver: zodResolver(lootFormSchema),
    defaultValues: lootFormDefaults
  });
  const {
    register: registerAreaLoot,
    handleSubmit: handleAreaLootSubmitForm,
    formState: areaLootFormState,
    reset: resetAreaLootForm,
    control: areaLootFormControl
  } = areaLootForm;
  const {
    fields: areaLootEntryFields,
    append: appendAreaLootEntry,
    remove: removeAreaLootEntry,
    move: moveAreaLootEntry
  } = useFieldArray({
    control: areaLootFormControl,
    name: "entries"
  });
  const {
    fields: areaLootOpFields,
    append: appendAreaLootOp,
    remove: removeAreaLootOp,
    move: moveAreaLootOp
  } = useFieldArray({
    control: areaLootFormControl,
    name: "ops"
  });
  const lootForm = useForm<LootFormValues>({
    resolver: zodResolver(lootFormSchema),
    defaultValues: lootFormDefaults
  });
  const {
    register: registerLoot,
    handleSubmit: handleLootSubmitForm,
    formState: lootFormState,
    reset: resetLootForm,
    control: lootFormControl
  } = lootForm;
  const {
    fields: lootEntryFields,
    append: appendLootEntry,
    remove: removeLootEntry,
    move: moveLootEntry
  } = useFieldArray({
    control: lootFormControl,
    name: "entries"
  });
  const {
    fields: lootOpFields,
    append: appendLootOp,
    remove: removeLootOp,
    move: moveLootOp
  } = useFieldArray({
    control: lootFormControl,
    name: "ops"
  });
  const watchedResetCommand = useWatch({
    control: resetForm.control,
    name: "commandName"
  });
  const watchedObjectItemType = useWatch({
    control: objectForm.control,
    name: "itemType"
  });

  useEffect(() => {
    const nextRoom = getDefaultSelection(areaData, "Rooms", selectedRoomVnum);
    if (selectedRoomVnum !== nextRoom) {
      setSelectedRoomVnum(nextRoom);
    }
    const nextMobile = getDefaultSelection(
      areaData,
      "Mobiles",
      selectedMobileVnum
    );
    if (selectedMobileVnum !== nextMobile) {
      setSelectedMobileVnum(nextMobile);
    }
    const nextObject = getDefaultSelection(
      areaData,
      "Objects",
      selectedObjectVnum
    );
    if (selectedObjectVnum !== nextObject) {
      setSelectedObjectVnum(nextObject);
    }
    const nextReset = getDefaultSelection(
      areaData,
      "Resets",
      selectedResetIndex
    );
    if (selectedResetIndex !== nextReset) {
      setSelectedResetIndex(nextReset);
    }
    const nextShop = getDefaultSelection(
      areaData,
      "Shops",
      selectedShopKeeper
    );
    if (selectedShopKeeper !== nextShop) {
      setSelectedShopKeeper(nextShop);
    }
    const nextQuest = getDefaultSelection(
      areaData,
      "Quests",
      selectedQuestVnum
    );
    if (selectedQuestVnum !== nextQuest) {
      setSelectedQuestVnum(nextQuest);
    }
    const nextFaction = getDefaultSelection(
      areaData,
      "Factions",
      selectedFactionVnum
    );
    if (selectedFactionVnum !== nextFaction) {
      setSelectedFactionVnum(nextFaction);
    }
    const nextRecipe = getDefaultSelection(
      areaData,
      "Recipes",
      selectedRecipeVnum
    );
    if (selectedRecipeVnum !== nextRecipe) {
      setSelectedRecipeVnum(nextRecipe);
    }
    const nextGather = getDefaultSelection(
      areaData,
      "Gather Spawns",
      selectedGatherVnum
    );
    if (selectedGatherVnum !== nextGather) {
      setSelectedGatherVnum(nextGather);
    }
  }, [
    areaData,
    selectedRoomVnum,
    selectedMobileVnum,
    selectedObjectVnum,
    selectedResetIndex,
    selectedShopKeeper,
    selectedQuestVnum,
    selectedFactionVnum,
    selectedRecipeVnum,
    selectedGatherVnum
  ]);

  const areaLootData = useMemo(
    () => extractAreaLootData(areaData),
    [areaData]
  );

  useEffect(() => {
    const groupCount = areaLootData?.groups.length ?? 0;
    const tableCount = areaLootData?.tables.length ?? 0;
    if (!areaLootData || (groupCount === 0 && tableCount === 0)) {
      if (selectedAreaLootKind !== null || selectedAreaLootIndex !== null) {
        setSelectedAreaLootKind(null);
        setSelectedAreaLootIndex(null);
      }
      return;
    }
    if (selectedAreaLootKind === "group") {
      if (selectedAreaLootIndex === null || selectedAreaLootIndex >= groupCount) {
        setSelectedAreaLootKind(groupCount ? "group" : "table");
        setSelectedAreaLootIndex(0);
      }
      return;
    }
    if (selectedAreaLootKind === "table") {
      if (selectedAreaLootIndex === null || selectedAreaLootIndex >= tableCount) {
        setSelectedAreaLootKind(groupCount ? "group" : "table");
        setSelectedAreaLootIndex(0);
      }
      return;
    }
    setSelectedAreaLootKind(groupCount ? "group" : "table");
    setSelectedAreaLootIndex(0);
  }, [areaLootData, selectedAreaLootKind, selectedAreaLootIndex]);

  useEffect(() => {
    setSelectedRoomVnum(null);
    setSelectedMobileVnum(null);
    setSelectedObjectVnum(null);
    setSelectedResetIndex(null);
    setSelectedShopKeeper(null);
    setSelectedQuestVnum(null);
    setSelectedFactionVnum(null);
    setSelectedRecipeVnum(null);
    setSelectedGatherVnum(null);
    setSelectedAreaLootKind(null);
    setSelectedAreaLootIndex(null);
  }, [areaPath]);

  useEffect(() => {
    if (
      editorMode === "Global" &&
      activeTab !== "Table" &&
      activeTab !== "Form"
    ) {
      setActiveTab("Table");
    }
  }, [editorMode, activeTab]);

  // Lazy-load global data when entity is selected
  useGlobalDataLazyLoad({
    handleLoadClassesData,
    handleLoadRacesData,
    handleLoadSkillsData,
    handleLoadGroupsData,
    handleLoadCommandsData,
    handleLoadSocialsData,
    handleLoadTutorialsData,
    handleLoadLootData
  });

  // Auto-clamp selection indices when global data changes
  useGlobalIndexClamp();

  useEffect(() => {
    if (!classData || selectedClassIndex === null) {
      resetClassForm(classFormDefaults);
      return;
    }
    const cls = classData.classes[selectedClassIndex];
    resetClassForm({
      name: cls.name ?? "",
      whoName: cls.whoName ?? "",
      baseGroup: cls.baseGroup ?? "",
      defaultGroup: cls.defaultGroup ?? "",
      weaponVnum: cls.weaponVnum ?? 0,
      armorProf: cls.armorProf ?? "",
      guilds: Array.from({ length: classGuildCount }).map(
        (_, index) => cls.guilds?.[index] ?? 0
      ),
      primeStat: cls.primeStat ?? "",
      skillCap: cls.skillCap ?? 0,
      thac0_00: cls.thac0_00 ?? 0,
      thac0_32: cls.thac0_32 ?? 0,
      hpMin: cls.hpMin ?? 0,
      hpMax: cls.hpMax ?? 0,
      manaUser: cls.manaUser ?? false,
      startLoc: cls.startLoc ?? 0,
      titlesMale: titlesToText(cls.titles, 0),
      titlesFemale: titlesToText(cls.titles, 1)
    });
  }, [classData, classFormDefaults, resetClassForm, selectedClassIndex]);

  useEffect(() => {
    if (!raceData || selectedRaceIndex === null) {
      resetRaceForm(raceFormDefaults);
      return;
    }
    const race = raceData.races[selectedRaceIndex];
    const classNames = referenceData?.classes ?? [];
    resetRaceForm({
      name: race.name ?? "",
      whoName: race.whoName ?? "",
      pc: race.pc ?? false,
      points: race.points ?? 0,
      size: race.size ?? "",
      startLoc: race.startLoc ?? 0,
      stats: normalizeRaceStats(race.stats),
      maxStats: normalizeRaceStats(race.maxStats),
      actFlags: race.actFlags ?? [],
      affectFlags: race.affectFlags ?? [],
      offFlags: race.offFlags ?? [],
      immFlags: race.immFlags ?? [],
      resFlags: race.resFlags ?? [],
      vulnFlags: race.vulnFlags ?? [],
      formFlags: race.formFlags ?? [],
      partFlags: race.partFlags ?? [],
      classMult: normalizeRaceClassMap(race.classMult, classNames, 100),
      classStart: normalizeRaceClassMap(race.classStart, classNames, 0),
      skills: normalizeRaceSkills(race.skills)
    });
  }, [
    raceData,
    selectedRaceIndex,
    raceFormDefaults,
    referenceData,
    resetRaceForm
  ]);

  useEffect(() => {
    if (!skillData || selectedSkillIndex === null) {
      resetSkillForm(skillFormDefaults);
      return;
    }
    const skill = skillData.skills[selectedSkillIndex];
    const classNames = referenceData?.classes ?? [];
    resetSkillForm({
      name: skill.name ?? "",
      levels: normalizeRaceClassMap(
        skill.levels as Record<string, number> | number[] | undefined,
        classNames,
        defaultSkillLevel
      ),
      ratings: normalizeRaceClassMap(
        skill.ratings as Record<string, number> | number[] | undefined,
        classNames,
        defaultSkillRating
      ),
      spell: skill.spell ?? "",
      loxSpell: skill.loxSpell ?? "",
      target: skill.target ?? "",
      minPosition: skill.minPosition ?? "",
      gsn: skill.gsn ?? "",
      slot: skill.slot ?? 0,
      minMana: skill.minMana ?? 0,
      beats: skill.beats ?? 0,
      nounDamage: skill.nounDamage ?? "",
      msgOff: skill.msgOff ?? "",
      msgObj: skill.msgObj ?? ""
    });
  }, [
    skillData,
    selectedSkillIndex,
    skillFormDefaults,
    referenceData,
    resetSkillForm
  ]);

  useEffect(() => {
    if (!groupData || selectedGroupIndex === null) {
      resetGroupForm(groupFormDefaults);
      return;
    }
    const group = groupData.groups[selectedGroupIndex];
    const classNames = referenceData?.classes ?? [];
    resetGroupForm({
      name: group.name ?? "",
      ratings: normalizeRaceClassMap(
        group.ratings as Record<string, number> | number[] | undefined,
        classNames,
        defaultSkillRating
      ),
      skills: normalizeGroupSkills(group.skills)
    });
  }, [
    groupData,
    selectedGroupIndex,
    groupFormDefaults,
    referenceData,
    resetGroupForm
  ]);

  useEffect(() => {
    if (!commandData || selectedCommandIndex === null) {
      resetCommandForm(commandFormDefaults);
      return;
    }
    const command = commandData.commands[selectedCommandIndex];
    resetCommandForm({
      name: command.name ?? "",
      function: command.function ?? "",
      position: command.position ?? "",
      level: command.level ?? 0,
      log: command.log ?? "",
      category: command.category ?? "",
      loxFunction: command.loxFunction ?? ""
    });
  }, [
    commandData,
    selectedCommandIndex,
    commandFormDefaults,
    resetCommandForm
  ]);

  useEffect(() => {
    if (!socialData || selectedSocialIndex === null) {
      resetSocialForm(socialFormDefaults);
      return;
    }
    const social = socialData.socials[selectedSocialIndex];
    resetSocialForm({
      name: social.name ?? "",
      charNoArg: normalizeLineEndingsForDisplay(social.charNoArg),
      othersNoArg: normalizeLineEndingsForDisplay(social.othersNoArg),
      charFound: normalizeLineEndingsForDisplay(social.charFound),
      othersFound: normalizeLineEndingsForDisplay(social.othersFound),
      victFound: normalizeLineEndingsForDisplay(social.victFound),
      charAuto: normalizeLineEndingsForDisplay(social.charAuto),
      othersAuto: normalizeLineEndingsForDisplay(social.othersAuto)
    });
  }, [
    socialData,
    selectedSocialIndex,
    socialFormDefaults,
    resetSocialForm
  ]);

  useEffect(() => {
    if (!tutorialData || selectedTutorialIndex === null) {
      resetTutorialForm(tutorialFormDefaults);
      return;
    }
    const tutorial = tutorialData.tutorials[selectedTutorialIndex];
    resetTutorialForm({
      name: tutorial.name ?? "",
      blurb: normalizeLineEndingsForDisplay(tutorial.blurb),
      finish: normalizeLineEndingsForDisplay(tutorial.finish),
      minLevel: tutorial.minLevel ?? 0,
      steps: (tutorial.steps ?? []).map((step) => ({
        prompt: normalizeLineEndingsForDisplay(step.prompt),
        match: step.match ?? ""
      }))
    });
  }, [
    tutorialData,
    selectedTutorialIndex,
    tutorialFormDefaults,
    resetTutorialForm
  ]);

  useEffect(() => {
    if (
      !areaLootData ||
      selectedAreaLootKind === null ||
      selectedAreaLootIndex === null
    ) {
      resetAreaLootForm(lootFormDefaults);
      return;
    }
    if (selectedAreaLootKind === "group") {
      const group = areaLootData.groups[selectedAreaLootIndex];
      if (!group) {
        resetAreaLootForm(lootFormDefaults);
        return;
      }
      resetAreaLootForm({
        kind: "group",
        name: group.name ?? "",
        rolls: group.rolls ?? 1,
        entries: (group.entries ?? []).map((entry) => ({
          type: entry.type === "cp" ? "cp" : "item",
          vnum: entry.vnum ?? 0,
          minQty: entry.minQty ?? 1,
          maxQty: entry.maxQty ?? 1,
          weight: entry.weight ?? 100
        })),
        parent: "",
        ops: []
      });
      return;
    }
    const table = areaLootData.tables[selectedAreaLootIndex];
    if (!table) {
      resetAreaLootForm(lootFormDefaults);
      return;
    }
    resetAreaLootForm({
      kind: "table",
      name: table.name ?? "",
      parent: table.parent ?? "",
      rolls: 1,
      entries: [],
      ops: (table.ops ?? []).map((op) => ({
        op: op.op,
        group: op.group ?? "",
        vnum: op.vnum ?? 0,
        chance: op.chance ?? 100,
        minQty: op.minQty ?? 1,
        maxQty: op.maxQty ?? 1,
        multiplier: op.multiplier ?? 100
      }))
    });
  }, [
    areaLootData,
    selectedAreaLootKind,
    selectedAreaLootIndex,
    lootFormDefaults,
    resetAreaLootForm
  ]);

  useEffect(() => {
    if (
      !lootData ||
      selectedLootKind === null ||
      selectedLootIndex === null
    ) {
      resetLootForm(lootFormDefaults);
      return;
    }
    if (selectedLootKind === "group") {
      const group = lootData.groups[selectedLootIndex];
      if (!group) {
        resetLootForm(lootFormDefaults);
        return;
      }
      resetLootForm({
        kind: "group",
        name: group.name ?? "",
        rolls: group.rolls ?? 1,
        entries: (group.entries ?? []).map((entry) => ({
          type: entry.type === "cp" ? "cp" : "item",
          vnum: entry.vnum ?? 0,
          minQty: entry.minQty ?? 1,
          maxQty: entry.maxQty ?? 1,
          weight: entry.weight ?? 100
        })),
        parent: "",
        ops: []
      });
      return;
    }
    const table = lootData.tables[selectedLootIndex];
    if (!table) {
      resetLootForm(lootFormDefaults);
      return;
    }
    resetLootForm({
      kind: "table",
      name: table.name ?? "",
      parent: table.parent ?? "",
      rolls: 1,
      entries: [],
      ops: (table.ops ?? []).map((op) => ({
        op: op.op,
        group: op.group ?? "",
        vnum: op.vnum ?? 0,
        chance: op.chance ?? 100,
        minQty: op.minQty ?? 1,
        maxQty: op.maxQty ?? 1,
        multiplier: op.multiplier ?? 100
      }))
    });
  }, [
    lootData,
    selectedLootKind,
    selectedLootIndex,
    lootFormDefaults,
    resetLootForm
  ]);

  const selection = useMemo(
    () =>
      buildSelectionSummary(selectedEntity, areaData, {
        Rooms: selectedRoomVnum,
        Mobiles: selectedMobileVnum,
        Objects: selectedObjectVnum,
        Resets: selectedResetIndex,
        Shops: selectedShopKeeper,
        Quests: selectedQuestVnum,
        Factions: selectedFactionVnum,
        Recipes: selectedRecipeVnum,
        "Gather Spawns": selectedGatherVnum
      },
      {
        data: areaLootData,
        kind: selectedAreaLootKind,
        index: selectedAreaLootIndex
      }),
    [
      areaData,
      selectedEntity,
      selectedRoomVnum,
      selectedMobileVnum,
      selectedObjectVnum,
      selectedResetIndex,
      selectedShopKeeper,
      selectedQuestVnum,
      selectedFactionVnum,
      selectedRecipeVnum,
      selectedGatherVnum,
      areaLootData,
      selectedAreaLootKind,
      selectedAreaLootIndex
    ]
  );
  const selectedEntityVnum = useMemo(() => {
    if (selectedEntity === "Rooms") {
      return selectedRoomVnum;
    }
    if (selectedEntity === "Mobiles") {
      return selectedMobileVnum;
    }
    if (selectedEntity === "Objects") {
      return selectedObjectVnum;
    }
    if (selectedEntity === "Recipes") {
      return selectedRecipeVnum;
    }
    if (selectedEntity === "Gather Spawns") {
      return selectedGatherVnum;
    }
    return null;
  }, [
    selectedEntity,
    selectedRoomVnum,
    selectedMobileVnum,
    selectedObjectVnum,
    selectedRecipeVnum,
    selectedGatherVnum
  ]);
  const validationConfig = useMemo(() => loadValidationConfig(), []);
  const pluginValidationRules = useMemo(() => loadPluginRules(), []);
  const coreValidationRules = useMemo<ValidationRule[]>(
    () => [
      {
        id: "vnum-range",
        label: "VNUM range",
        source: "core",
        run: ({ areaData: data }) => validateVnumRanges(data)
      },
      {
        id: "duplicate-vnum",
        label: "Duplicate VNUMs",
        source: "core",
        run: ({ areaData: data }) => validateDuplicateVnums(data)
      },
      {
        id: "exit-targets",
        label: "Exit targets",
        source: "core",
        run: () => exitValidation.issues
      },
      {
        id: "reset-references",
        label: "Reset references",
        source: "core",
        run: ({ areaData: data }) => validateResetReferences(data)
      },
      {
        id: "one-way-exits",
        label: "One-way exits",
        source: "core",
        run: ({ areaData: data }) => validateOneWayExits(data)
      },
      {
        id: "orphan-rooms",
        label: "Orphan rooms",
        source: "core",
        run: ({ areaData: data }) => validateOrphanRooms(data)
      }
    ],
    [exitValidation]
  );
  const validationIssues = useMemo(
    () =>
      buildValidationIssues(
        [...coreValidationRules, ...pluginValidationRules],
        { areaData, areaIndex },
        validationConfig
      ),
    [areaData, areaIndex, coreValidationRules, pluginValidationRules, validationConfig]
  );
  const validationSummary = useMemo(
    () =>
      buildValidationSummary(
        validationIssues,
        selectedEntity,
        selectedEntityVnum,
        selectedResetIndex
      ),
    [validationIssues, selectedEntity, selectedEntityVnum, selectedResetIndex]
  );
  const diagnosticsList = useMemo(() => {
    const filtered =
      diagnosticFilter === "All"
        ? validationIssues
        : validationIssues.filter(
            (issue) => issue.entityType === diagnosticFilter
          );
    const priority: Record<ValidationSeverity, number> = {
      error: 0,
      warning: 1
    };
    return [...filtered].sort((a, b) => {
      const severityOrder = priority[a.severity] - priority[b.severity];
      if (severityOrder !== 0) {
        return severityOrder;
      }
      if (a.entityType !== b.entityType) {
        return a.entityType.localeCompare(b.entityType);
      }
      const aKey = a.resetIndex ?? a.vnum ?? 0;
      const bKey = b.resetIndex ?? b.vnum ?? 0;
      return aKey - bKey;
    });
  }, [validationIssues, diagnosticFilter]);
  const diagnosticsCount = diagnosticsList.length;
  const handleDiagnosticsClick = useCallback(
    (issue: ValidationIssue) => {
      if (issue.entityType === "Rooms" && typeof issue.vnum === "number") {
        setSelectedEntity("Rooms");
        setSelectedRoomVnum(issue.vnum);
        return;
      }
      if (issue.entityType === "Mobiles" && typeof issue.vnum === "number") {
        setSelectedEntity("Mobiles");
        setSelectedMobileVnum(issue.vnum);
        return;
      }
      if (issue.entityType === "Objects" && typeof issue.vnum === "number") {
        setSelectedEntity("Objects");
        setSelectedObjectVnum(issue.vnum);
        return;
      }
      if (
        issue.entityType === "Resets" &&
        typeof issue.resetIndex === "number"
      ) {
        setSelectedEntity("Resets");
        setSelectedResetIndex(issue.resetIndex);
      }
    },
    [
      setSelectedEntity,
      setSelectedRoomVnum,
      setSelectedMobileVnum,
      setSelectedObjectVnum,
      setSelectedResetIndex
    ]
  );
  const areaName = fileNameFromPath(areaPath);
  const areaDebug = useMemo(() => buildAreaDebugSummary(areaData), [areaData]);
  const areaVnumRange = useMemo(() => getAreaVnumRange(areaData), [areaData]);
  const entityItems = useMemo(
    () =>
      entityOrder.map((key) => ({
        key,
        count: getEntityList(areaData, key).length
      })),
    [areaData]
  );
  const lootCount = lootData
    ? lootData.groups.length + lootData.tables.length
    : 0;
  const globalItems = useMemo(() => {
    const counts: Record<GlobalEntityKey, number> = {
      Classes: referenceData?.classes.length ?? 0,
      Races: referenceData?.races.length ?? 0,
      Skills: referenceData?.skills.length ?? 0,
      Groups: referenceData?.groups.length ?? 0,
      Commands: referenceData?.commands.length ?? 0,
      Socials: referenceData?.socials.length ?? 0,
      Tutorials: referenceData?.tutorials.length ?? 0,
      Loot: lootCount
    };
    return globalEntityOrder.map((key) => ({
      key,
      count: counts[key] ?? 0
    }));
  }, [lootCount, referenceData]);
  const classRows = useMemo(() => buildClassRows(classData), [classData]);
  const raceRows = useMemo(() => buildRaceRows(raceData), [raceData]);
  const skillRows = useMemo(() => buildSkillRows(skillData), [skillData]);
  const groupRows = useMemo(() => buildGroupRows(groupData), [groupData]);
  const commandRows = useMemo(
    () => buildCommandRows(commandData),
    [commandData]
  );
  const socialRows = useMemo(
    () => buildSocialRows(socialData),
    [socialData]
  );
  const tutorialRows = useMemo(
    () => buildTutorialRows(tutorialData),
    [tutorialData]
  );
  const areaLootRows = useMemo(
    () => buildLootRows(areaLootData),
    [areaLootData]
  );
  const recipeRows = useMemo(() => buildRecipeRows(areaData), [areaData]);
  const gatherSpawnRows = useMemo(
    () => buildGatherSpawnRows(areaData),
    [areaData]
  );
  const lootRows = useMemo(() => buildLootRows(lootData), [lootData]);
  const roomRows = useMemo(() => buildRoomRows(areaData), [areaData]);
  const mobileRows = useMemo(() => buildMobileRows(areaData), [areaData]);
  const objectRows = useMemo(() => buildObjectRows(areaData), [areaData]);
  const resetRows = useMemo(() => buildResetRows(areaData), [areaData]);
  const shopRows = useMemo(() => buildShopRows(areaData), [areaData]);
  const questRows = useMemo(() => buildQuestRows(areaData), [areaData]);
  const factionRows = useMemo(() => buildFactionRows(areaData), [areaData]);

  useEffect(() => {
    let cancelled = false;
    const loadIndex = async () => {
      const baseDir =
        areaDirectory ??
        (areaPath ? await repository.resolveAreaDirectory(areaPath) : null);
      if (!baseDir) {
        if (!cancelled) {
          setAreaIndex([]);
        }
        return;
      }
      try {
        const entries = await repository.loadAreaIndex(
          baseDir,
          projectConfig?.areaList
        );
        if (!cancelled) {
          setAreaIndex(entries);
        }
      } catch {
        if (!cancelled) {
          setAreaIndex([]);
        }
      }
    };
    loadIndex();
    return () => {
      cancelled = true;
    };
  }, [areaDirectory, areaPath, projectConfig, repository]);
  useEffect(() => {
    let cancelled = false;
    const loadLinks = async () => {
      if (!areaIndex.length) {
        if (!cancelled) {
          setAreaGraphLinks([]);
        }
        return;
      }
      const baseDir =
        areaDirectory ??
        (areaPath ? await repository.resolveAreaDirectory(areaPath) : null);
      if (!baseDir) {
        if (!cancelled) {
          setAreaGraphLinks([]);
        }
        return;
      }
      try {
        const links = await repository.loadAreaGraphLinks(baseDir, areaIndex);
        if (!cancelled) {
          setAreaGraphLinks(links);
        }
      } catch {
        if (!cancelled) {
          setAreaGraphLinks([]);
        }
      }
    };
    loadLinks();
    return () => {
      cancelled = true;
    };
  }, [areaDirectory, areaPath, areaIndex, repository]);
  const selectedRoomRecord = useMemo(() => {
    if (!areaData || selectedRoomVnum === null) {
      return null;
    }
    return findByVnum(getEntityList(areaData, "Rooms"), selectedRoomVnum);
  }, [areaData, selectedRoomVnum]);
  const selectedMobileRecord = useMemo(() => {
    if (!areaData || selectedMobileVnum === null) {
      return null;
    }
    return findByVnum(getEntityList(areaData, "Mobiles"), selectedMobileVnum);
  }, [areaData, selectedMobileVnum]);
  const selectedObjectRecord = useMemo(() => {
    if (!areaData || selectedObjectVnum === null) {
      return null;
    }
    return findByVnum(getEntityList(areaData, "Objects"), selectedObjectVnum);
  }, [areaData, selectedObjectVnum]);
  const selectedResetRecord = useMemo(() => {
    if (!areaData || selectedResetIndex === null) {
      return null;
    }
    const resets = getEntityList(areaData, "Resets");
    if (selectedResetIndex < 0 || selectedResetIndex >= resets.length) {
      return null;
    }
    const record = resets[selectedResetIndex];
    return record && typeof record === "object"
      ? (record as Record<string, unknown>)
      : null;
  }, [areaData, selectedResetIndex]);
  const selectedShopRecord = useMemo(() => {
    if (!areaData || selectedShopKeeper === null) {
      return null;
    }
    const shops = getEntityList(areaData, "Shops");
    return findShopByKeeper(shops, selectedShopKeeper);
  }, [areaData, selectedShopKeeper]);
  const selectedQuestRecord = useMemo(() => {
    if (!areaData || selectedQuestVnum === null) {
      return null;
    }
    return findByVnum(getEntityList(areaData, "Quests"), selectedQuestVnum);
  }, [areaData, selectedQuestVnum]);
  const selectedFactionRecord = useMemo(() => {
    if (!areaData || selectedFactionVnum === null) {
      return null;
    }
    return findByVnum(getEntityList(areaData, "Factions"), selectedFactionVnum);
  }, [areaData, selectedFactionVnum]);
  const selectedRecipeRecord = useMemo(() => {
    if (!areaData || selectedRecipeVnum === null) {
      return null;
    }
    return findByVnum(getEntityList(areaData, "Recipes"), selectedRecipeVnum);
  }, [areaData, selectedRecipeVnum]);
  const selectedGatherRecord = useMemo(() => {
    if (!areaData || selectedGatherVnum === null) {
      return null;
    }
    return findByVnum(
      getEntityList(areaData, "Gather Spawns"),
      selectedGatherVnum
    );
  }, [areaData, selectedGatherVnum]);
  const selectedLootRecord = useMemo(() => {
    if (!lootData || selectedLootKind === null || selectedLootIndex === null) {
      return null;
    }
    if (selectedLootKind === "group") {
      return lootData.groups[selectedLootIndex] ?? null;
    }
    return lootData.tables[selectedLootIndex] ?? null;
  }, [lootData, selectedLootKind, selectedLootIndex]);
  const scriptContext = useMemo(() => {
    if (selectedEntity === "Area") {
      const areadata =
        areaData && typeof (areaData as Record<string, unknown>).areadata === "object"
          ? ((areaData as Record<string, unknown>).areadata as Record<string, unknown>)
          : null;
      return {
        entity: "Area" as const,
        vnum: null,
        record: areadata
      };
    }
    if (selectedEntity === "Rooms") {
      return {
        entity: "Rooms" as const,
        vnum: selectedRoomVnum,
        record: selectedRoomRecord
      };
    }
    if (selectedEntity === "Mobiles") {
      return {
        entity: "Mobiles" as const,
        vnum: selectedMobileVnum,
        record: selectedMobileRecord
      };
    }
    if (selectedEntity === "Objects") {
      return {
        entity: "Objects" as const,
        vnum: selectedObjectVnum,
        record: selectedObjectRecord
      };
    }
    return null;
  }, [
    selectedEntity,
    areaData,
    selectedRoomVnum,
    selectedRoomRecord,
    selectedMobileVnum,
    selectedMobileRecord,
    selectedObjectVnum,
    selectedObjectRecord
  ]);
  const scriptValue = useMemo(() => {
    if (!scriptContext?.record) {
      return "";
    }
    const raw = (scriptContext.record as Record<string, unknown>).loxScript;
    return typeof raw === "string" ? normalizeLineEndingsForDisplay(raw) : "";
  }, [scriptContext]);
  const scriptTitle = useMemo(() => {
    if (!scriptContext) {
      return "Script View";
    }
    if (scriptContext.entity === "Area") {
      const record = scriptContext.record as Record<string, unknown> | null;
      const name = record && typeof record.name === "string" ? record.name : "";
      return name ? `Area: ${name}` : "Area";
    }
    if (scriptContext.vnum === null) {
      return entityKindLabels[scriptContext.entity];
    }
    return `${entityKindLabels[scriptContext.entity]} ${scriptContext.vnum}`;
  }, [scriptContext]);
  const scriptSubtitle = useMemo(() => {
    if (!scriptContext?.record) {
      return "";
    }
    const record = scriptContext.record as Record<string, unknown>;
    if (scriptContext.entity === "Rooms") {
      return getFirstString(record.name, "");
    }
    if (scriptContext.entity === "Mobiles") {
      return getFirstString(record.shortDescr, "");
    }
    if (scriptContext.entity === "Objects") {
      return getFirstString(record.shortDescr, "");
    }
    if (scriptContext.entity === "Area") {
      return getFirstString(record.credits, "");
    }
    return "";
  }, [scriptContext]);
  const eventBindings = useMemo<EventBinding[]>(() => {
    if (!scriptContext?.record) {
      return [];
    }
    const raw = (scriptContext.record as Record<string, unknown>).events;
    if (!Array.isArray(raw)) {
      return [];
    }
    return raw
      .filter((event) => event && typeof event === "object")
      .map((event) => {
        const record = event as Record<string, unknown>;
        const trigger = typeof record.trigger === "string" ? record.trigger : "";
        const callback =
          typeof record.callback === "string" ? record.callback : "";
        const criteria =
          typeof record.criteria === "string" || typeof record.criteria === "number"
            ? record.criteria
            : undefined;
        return {
          trigger,
          callback,
          criteria
        };
      });
  }, [scriptContext]);
  const canEditScript = Boolean(
    scriptContext?.record &&
      (scriptContext.entity === "Area" || scriptContext?.vnum !== null)
  );
  const scriptEventEntity = scriptContext?.entity ?? null;
  const handleApplyScript = useCallback(
    (nextScript: string) => {
      if (!scriptContext) {
        return;
      }
      const normalized = normalizeOptionalScript(nextScript);
      setAreaData((current) => {
        if (!current) {
          return current;
        }
        if (scriptContext.entity === "Area") {
          const areadata =
            typeof (current as Record<string, unknown>).areadata === "object"
              ? ((current as Record<string, unknown>)
                  .areadata as Record<string, unknown>)
              : {};
          const nextArea: Record<string, unknown> = { ...areadata };
          if (normalized) {
            nextArea.loxScript = normalized;
          } else {
            delete nextArea.loxScript;
          }
          return {
            ...(current as Record<string, unknown>),
            areadata: nextArea
          };
        }
        const list = getEntityList(current, scriptContext.entity);
        if (!list.length) {
          return current;
        }
        const nextList = list.map((entry) => {
          if (!entry || typeof entry !== "object") {
            return entry;
          }
          const record = entry as Record<string, unknown>;
          const vnum = parseVnum(record.vnum);
          if (vnum !== scriptContext.vnum) {
            return record;
          }
          const nextRecord: Record<string, unknown> = { ...record };
          if (normalized) {
            nextRecord.loxScript = normalized;
          } else {
            delete nextRecord.loxScript;
          }
          return nextRecord;
        });
        return {
          ...(current as Record<string, unknown>),
          [entityDataKeys[scriptContext.entity]]: nextList
        };
      });
      const statusTarget =
        scriptContext.entity === "Area"
          ? "area"
          : `${entityKindLabels[scriptContext.entity].toLowerCase()} ${
              scriptContext.vnum
            }`;
      setStatusMessage(`Updated ${statusTarget} script (unsaved)`);
    },
    [scriptContext]
  );
  const handleEventBindingsChange = useCallback(
    (nextEvents: EventBinding[]) => {
      if (!scriptContext) {
        return;
      }
      setAreaData((current) => {
        if (!current) {
          return current;
        }
        if (scriptContext.entity === "Area") {
          const areadata =
            typeof (current as Record<string, unknown>).areadata === "object"
              ? ((current as Record<string, unknown>)
                  .areadata as Record<string, unknown>)
              : {};
          const nextArea: Record<string, unknown> = { ...areadata };
          if (nextEvents.length) {
            nextArea.events = nextEvents.map((event) => {
              const trigger = event.trigger.trim();
              const callback = event.callback.trim();
              const nextEvent: Record<string, unknown> = {
                trigger,
                callback
              };
              if (event.criteria !== undefined && event.criteria !== "") {
                nextEvent.criteria = event.criteria;
              }
              return nextEvent;
            });
          } else {
            delete nextArea.events;
          }
          return {
            ...(current as Record<string, unknown>),
            areadata: nextArea
          };
        }
        const list = getEntityList(current, scriptContext.entity);
        if (!list.length) {
          return current;
        }
        const nextList = list.map((entry) => {
          if (!entry || typeof entry !== "object") {
            return entry;
          }
          const record = entry as Record<string, unknown>;
          const vnum = parseVnum(record.vnum);
          if (vnum !== scriptContext.vnum) {
            return record;
          }
          const nextRecord: Record<string, unknown> = { ...record };
          if (nextEvents.length) {
            nextRecord.events = nextEvents.map((event) => {
              const trigger = event.trigger.trim();
              const callback = event.callback.trim();
              const nextEvent: Record<string, unknown> = {
                trigger,
                callback
              };
              if (event.criteria !== undefined && event.criteria !== "") {
                nextEvent.criteria = event.criteria;
              }
              return nextEvent;
            });
          } else {
            delete nextRecord.events;
          }
          return nextRecord;
        });
        return {
          ...(current as Record<string, unknown>),
          [entityDataKeys[scriptContext.entity]]: nextList
        };
      });
      const statusTarget =
        scriptContext.entity === "Area"
          ? "area"
          : `${entityKindLabels[scriptContext.entity].toLowerCase()} ${
              scriptContext.vnum
            }`;
      setStatusMessage(`Updated ${statusTarget} events (unsaved)`);
    },
    [scriptContext]
  );
  const areaFormDefaults = useMemo<AreaFormValues>(() => {
    const areadata =
      areaData && typeof (areaData as Record<string, unknown>).areadata === "object"
        ? ((areaData as Record<string, unknown>).areadata as Record<string, unknown>)
        : {};
    const daycycle =
      areaData && typeof (areaData as Record<string, unknown>).daycycle === "object"
        ? ((areaData as Record<string, unknown>).daycycle as Record<string, unknown>)
        : {};
    const vnumRange = Array.isArray(areadata.vnumRange) ? areadata.vnumRange : [];
    const vnumRangeStart = parseVnum(vnumRange[0]) ?? 0;
    const vnumRangeEnd = parseVnum(vnumRange[1]) ?? 0;
    const storyBeats = Array.isArray((areaData as Record<string, unknown>)?.storyBeats)
      ? ((areaData as Record<string, unknown>).storyBeats as unknown[])
          .filter((entry) => entry && typeof entry === "object")
          .map((entry) => {
            const record = entry as Record<string, unknown>;
            return {
              title: typeof record.title === "string" ? record.title : "",
              description:
                typeof record.description === "string"
                  ? normalizeLineEndingsForDisplay(record.description)
                  : ""
            };
          })
      : [];
    const checklist = Array.isArray((areaData as Record<string, unknown>)?.checklist)
      ? ((areaData as Record<string, unknown>).checklist as unknown[])
          .filter((entry) => entry && typeof entry === "object")
          .map((entry) => {
            const record = entry as Record<string, unknown>;
            return {
              title: typeof record.title === "string" ? record.title : "",
              status: normalizeChecklistStatus(record.status),
              description:
                typeof record.description === "string"
                  ? normalizeLineEndingsForDisplay(record.description)
                  : ""
            };
          })
      : [];
    const daycyclePeriods = Array.isArray(daycycle.periods)
      ? (daycycle.periods as unknown[])
          .filter((entry) => entry && typeof entry === "object")
          .map((entry) => {
            const record = entry as Record<string, unknown>;
            return {
              name: typeof record.name === "string" ? record.name : "",
              fromHour: parseVnum(record.fromHour) ?? undefined,
              toHour: parseVnum(record.toHour) ?? undefined,
              description:
                typeof record.description === "string"
                  ? normalizeLineEndingsForDisplay(record.description)
                  : "",
              enterMessage:
                typeof record.enterMessage === "string"
                  ? normalizeLineEndingsForDisplay(record.enterMessage)
                  : "",
              exitMessage:
                typeof record.exitMessage === "string"
                  ? normalizeLineEndingsForDisplay(record.exitMessage)
                  : ""
            };
          })
      : [];
    return {
      name: typeof areadata.name === "string" ? areadata.name : "",
      version: parseVnum(areadata.version) ?? 1,
      vnumRangeStart,
      vnumRangeEnd,
      builders: typeof areadata.builders === "string" ? areadata.builders : "",
      credits: typeof areadata.credits === "string" ? areadata.credits : "",
      lootTable:
        typeof areadata.lootTable === "string" ? areadata.lootTable : "",
      security: parseVnum(areadata.security) ?? undefined,
      sector: typeof areadata.sector === "string" ? areadata.sector : undefined,
      lowLevel: parseVnum(areadata.lowLevel) ?? undefined,
      highLevel: parseVnum(areadata.highLevel) ?? undefined,
      reset: parseVnum(areadata.reset) ?? undefined,
      alwaysReset:
        typeof areadata.alwaysReset === "boolean" ? areadata.alwaysReset : false,
      instType: areadata.instType === "multi" ? "multi" : "single",
      daycycleSuppress:
        typeof daycycle.suppressDaycycleMessages === "boolean"
          ? daycycle.suppressDaycycleMessages
          : false,
      daycyclePeriods,
      storyBeats,
      checklist
    };
  }, [areaData]);
  const roomFormDefaults = useMemo<RoomFormValues>(() => {
    const record = selectedRoomRecord;
    const vnum =
      selectedRoomVnum ?? parseVnum(record?.vnum) ?? 0;
    const sectorType =
      typeof record?.sectorType === "string"
        ? record.sectorType
        : typeof record?.sector === "string"
          ? record.sector
          : undefined;
    const roomFlags = Array.isArray(record?.roomFlags)
      ? record.roomFlags.filter((flag) => typeof flag === "string")
      : [];
    const timePeriods = Array.isArray(record?.timePeriods)
      ? record.timePeriods
          .filter((entry: unknown) => entry && typeof entry === "object")
          .map((entry: Record<string, unknown>) => ({
            name: typeof entry.name === "string" ? entry.name : "",
            fromHour: parseVnum(entry.fromHour) ?? undefined,
            toHour: parseVnum(entry.toHour) ?? undefined,
            description:
              typeof entry.description === "string"
                ? normalizeLineEndingsForDisplay(entry.description)
                : "",
            enterMessage:
              typeof entry.enterMessage === "string"
                ? normalizeLineEndingsForDisplay(entry.enterMessage)
                : "",
            exitMessage:
              typeof entry.exitMessage === "string"
                ? normalizeLineEndingsForDisplay(entry.exitMessage)
                : ""
          }))
      : [];
    return {
      vnum,
      name: typeof record?.name === "string" ? record.name : "",
      description:
        typeof record?.description === "string"
          ? normalizeLineEndingsForDisplay(record.description)
          : "",
      sectorType,
      roomFlags,
      suppressDaycycleMessages:
        typeof record?.suppressDaycycleMessages === "boolean"
          ? record.suppressDaycycleMessages
          : false,
      timePeriods,
      exits: Array.isArray(record?.exits)
        ? record.exits
            .filter((exit): exit is Record<string, unknown> =>
              Boolean(exit && typeof exit === "object")
            )
            .map((exit) => ({
              dir:
                typeof exit.dir === "string"
                  ? exit.dir
                  : directions[0],
              toVnum: parseVnum(exit.toVnum) ?? 0,
              key: parseVnum(exit.key) ?? undefined,
              flags: Array.isArray(exit.flags)
                ? exit.flags.filter((flag) => typeof flag === "string")
                : [],
              description:
                typeof exit.description === "string"
                  ? normalizeLineEndingsForDisplay(exit.description)
                  : "",
              keyword: typeof exit.keyword === "string" ? exit.keyword : ""
            }))
        : []
    };
  }, [selectedRoomRecord, selectedRoomVnum]);
  const mobileFormDefaults = useMemo<MobileFormValues>(() => {
    const record = selectedMobileRecord;
    const vnum = selectedMobileVnum ?? parseVnum(record?.vnum) ?? 0;
    const resolveDice = (value: unknown) => {
      if (!value || typeof value !== "object") {
        return { number: undefined, type: undefined, bonus: undefined };
      }
      const dice = value as Record<string, unknown>;
      return {
        number: parseVnum(dice.number) ?? undefined,
        type: parseVnum(dice.type) ?? undefined,
        bonus: parseVnum(dice.bonus) ?? undefined
      };
    };
    return {
      vnum,
      name: typeof record?.name === "string" ? record.name : "",
      shortDescr:
        typeof record?.shortDescr === "string" ? record.shortDescr : "",
      longDescr:
        typeof record?.longDescr === "string"
          ? normalizeLineEndingsForDisplay(record.longDescr)
          : "",
      description:
        typeof record?.description === "string"
          ? normalizeLineEndingsForDisplay(record.description)
          : "",
      race: typeof record?.race === "string" ? record.race : "",
      level: parseVnum(record?.level) ?? undefined,
      hitroll: parseVnum(record?.hitroll) ?? undefined,
      alignment: parseVnum(record?.alignment) ?? undefined,
      damType: typeof record?.damType === "string" ? record.damType : undefined,
      startPos:
        typeof record?.startPos === "string" ? record.startPos : undefined,
      defaultPos:
        typeof record?.defaultPos === "string" ? record.defaultPos : undefined,
      sex: typeof record?.sex === "string" ? record.sex : undefined,
      size: typeof record?.size === "string" ? record.size : undefined,
      material: typeof record?.material === "string" ? record.material : "",
      factionVnum: parseVnum(record?.factionVnum) ?? undefined,
      damageNoun:
        typeof record?.damageNoun === "string" ? record.damageNoun : "",
      offensiveSpell:
        typeof record?.offensiveSpell === "string"
          ? record.offensiveSpell
          : "",
      lootTable:
        typeof record?.lootTable === "string" ? record.lootTable : "",
      actFlags: Array.isArray(record?.actFlags)
        ? record.actFlags.filter((flag) => typeof flag === "string")
        : [],
      atkFlags: Array.isArray(record?.atkFlags)
        ? record.atkFlags.filter((flag) => typeof flag === "string")
        : [],
      formFlags: Array.isArray(record?.formFlags)
        ? record.formFlags.filter((flag) => typeof flag === "string")
        : [],
      partFlags: Array.isArray(record?.partFlags)
        ? record.partFlags.filter((flag) => typeof flag === "string")
        : [],
      hitDice: resolveDice(record?.hitDice),
      manaDice: resolveDice(record?.manaDice),
      damageDice: resolveDice(record?.damageDice)
    };
  }, [selectedMobileRecord, selectedMobileVnum]);
  const objectFormDefaults = useMemo<ObjectFormValues>(() => {
    const record = selectedObjectRecord;
    const vnum = selectedObjectVnum ?? parseVnum(record?.vnum) ?? 0;
    const resolveWeapon = (value: unknown) => {
      if (!value || typeof value !== "object") {
        return {
          class: undefined,
          diceNumber: undefined,
          diceType: undefined,
          damageType: undefined,
          flags: []
        };
      }
      const weapon = value as Record<string, unknown>;
      const dice = Array.isArray(weapon.dice) ? weapon.dice : [];
      return {
        class: typeof weapon.class === "string" ? weapon.class : undefined,
        diceNumber: parseVnum(dice[0]) ?? undefined,
        diceType: parseVnum(dice[1]) ?? undefined,
        damageType:
          typeof weapon.damageType === "string" ? weapon.damageType : undefined,
        flags: Array.isArray(weapon.flags)
          ? weapon.flags.filter((flag) => typeof flag === "string")
          : []
      };
    };
    const resolveArmor = (value: unknown) => {
      if (!value || typeof value !== "object") {
        return {
          acPierce: undefined,
          acBash: undefined,
          acSlash: undefined,
          acExotic: undefined
        };
      }
      const armor = value as Record<string, unknown>;
      return {
        acPierce: parseVnum(armor.acPierce) ?? undefined,
        acBash: parseVnum(armor.acBash) ?? undefined,
        acSlash: parseVnum(armor.acSlash) ?? undefined,
        acExotic: parseVnum(armor.acExotic) ?? undefined
      };
    };
    const resolveContainer = (value: unknown) => {
      if (!value || typeof value !== "object") {
        return {
          capacity: undefined,
          flags: [],
          keyVnum: undefined,
          maxWeight: undefined,
          weightMult: undefined
        };
      }
      const container = value as Record<string, unknown>;
      return {
        capacity: parseVnum(container.capacity) ?? undefined,
        flags: Array.isArray(container.flags)
          ? container.flags.filter((flag) => typeof flag === "string")
          : [],
        keyVnum: parseVnum(container.keyVnum) ?? undefined,
        maxWeight: parseVnum(container.maxWeight) ?? undefined,
        weightMult: parseVnum(container.weightMult) ?? undefined
      };
    };
    const resolveLight = (value: unknown) => {
      if (!value || typeof value !== "object") {
        return { hours: undefined };
      }
      const light = value as Record<string, unknown>;
      return { hours: parseVnum(light.hours) ?? undefined };
    };
    const resolveDrink = (value: unknown) => {
      if (!value || typeof value !== "object") {
        return {
          capacity: undefined,
          remaining: undefined,
          liquid: undefined,
          poisoned: undefined
        };
      }
      const drink = value as Record<string, unknown>;
      return {
        capacity: parseVnum(drink.capacity) ?? undefined,
        remaining: parseVnum(drink.remaining) ?? undefined,
        liquid: typeof drink.liquid === "string" ? drink.liquid : undefined,
        poisoned: typeof drink.poisoned === "boolean" ? drink.poisoned : undefined
      };
    };
    const resolveFood = (value: unknown) => {
      if (!value || typeof value !== "object") {
        return { foodHours: undefined, fullHours: undefined, poisoned: undefined };
      }
      const food = value as Record<string, unknown>;
      return {
        foodHours: parseVnum(food.foodHours) ?? undefined,
        fullHours: parseVnum(food.fullHours) ?? undefined,
        poisoned: typeof food.poisoned === "boolean" ? food.poisoned : undefined
      };
    };
    const resolveMoney = (value: unknown) => {
      if (!value || typeof value !== "object") {
        return { gold: undefined, silver: undefined };
      }
      const money = value as Record<string, unknown>;
      return {
        gold: parseVnum(money.gold) ?? undefined,
        silver: parseVnum(money.silver) ?? undefined
      };
    };
    const resolveWand = (value: unknown) => {
      if (!value || typeof value !== "object") {
        return {
          level: undefined,
          totalCharges: undefined,
          chargesLeft: undefined,
          spell: ""
        };
      }
      const wand = value as Record<string, unknown>;
      return {
        level: parseVnum(wand.level) ?? undefined,
        totalCharges: parseVnum(wand.totalCharges) ?? undefined,
        chargesLeft: parseVnum(wand.chargesLeft) ?? undefined,
        spell: typeof wand.spell === "string" ? wand.spell : ""
      };
    };
    const resolveSpells = (value: unknown) => {
      if (!value || typeof value !== "object") {
        return { level: undefined, spell1: "", spell2: "", spell3: "", spell4: "" };
      }
      const spells = value as Record<string, unknown>;
      return {
        level: parseVnum(spells.level) ?? undefined,
        spell1: typeof spells.spell1 === "string" ? spells.spell1 : "",
        spell2: typeof spells.spell2 === "string" ? spells.spell2 : "",
        spell3: typeof spells.spell3 === "string" ? spells.spell3 : "",
        spell4: typeof spells.spell4 === "string" ? spells.spell4 : ""
      };
    };
    const resolvePortal = (value: unknown) => {
      if (!value || typeof value !== "object") {
        return { charges: undefined, exitFlags: [], portalFlags: [], toVnum: undefined };
      }
      const portal = value as Record<string, unknown>;
      return {
        charges: parseVnum(portal.charges) ?? undefined,
        exitFlags: Array.isArray(portal.exitFlags)
          ? portal.exitFlags.filter((flag) => typeof flag === "string")
          : [],
        portalFlags: Array.isArray(portal.portalFlags)
          ? portal.portalFlags.filter((flag) => typeof flag === "string")
          : [],
        toVnum: parseVnum(portal.toVnum) ?? undefined
      };
    };
    const resolveFurniture = (value: unknown) => {
      if (!value || typeof value !== "object") {
        return {
          slots: undefined,
          weight: undefined,
          flags: [],
          healBonus: undefined,
          manaBonus: undefined,
          maxPeople: undefined
        };
      }
      const furniture = value as Record<string, unknown>;
      return {
        slots: parseVnum(furniture.slots) ?? undefined,
        weight: parseVnum(furniture.weight) ?? undefined,
        flags: Array.isArray(furniture.flags)
          ? furniture.flags.filter((flag) => typeof flag === "string")
          : [],
        healBonus: parseVnum(furniture.healBonus) ?? undefined,
        manaBonus: parseVnum(furniture.manaBonus) ?? undefined,
        maxPeople: parseVnum(furniture.maxPeople) ?? undefined
      };
    };
    return {
      vnum,
      name: typeof record?.name === "string" ? record.name : "",
      shortDescr:
        typeof record?.shortDescr === "string" ? record.shortDescr : "",
      description:
        typeof record?.description === "string"
          ? normalizeLineEndingsForDisplay(record.description)
          : "",
      material: typeof record?.material === "string" ? record.material : "",
      itemType: typeof record?.itemType === "string" ? record.itemType : "",
      extraFlags: Array.isArray(record?.extraFlags)
        ? record.extraFlags.filter((flag) => typeof flag === "string")
        : [],
      wearFlags: Array.isArray(record?.wearFlags)
        ? record.wearFlags.filter((flag) => typeof flag === "string")
        : [],
      level: parseVnum(record?.level) ?? undefined,
      weight: parseVnum(record?.weight) ?? undefined,
      cost: parseVnum(record?.cost) ?? undefined,
      condition: parseVnum(record?.condition) ?? undefined,
      weapon: resolveWeapon(record?.weapon),
      armor: resolveArmor(record?.armor),
      container: resolveContainer(record?.container),
      light: resolveLight(record?.light),
      drink: resolveDrink(record?.drink),
      fountain: resolveDrink(record?.fountain),
      food: resolveFood(record?.food),
      money: resolveMoney(record?.money),
      wand: resolveWand(record?.wand),
      staff: resolveWand(record?.staff),
      spells: resolveSpells(record?.spells),
      portal: resolvePortal(record?.portal),
      furniture: resolveFurniture(record?.furniture)
    };
  }, [selectedObjectRecord, selectedObjectVnum]);
  const resetFormDefaults = useMemo<ResetFormValues>(() => {
    const record = selectedResetRecord;
    const index = selectedResetIndex ?? 0;
    const commandName = resolveResetCommand(record);
    return {
      index,
      commandName: commandName || resetCommandOptions[0],
      mobVnum: parseVnum(record?.mobVnum) ?? undefined,
      objVnum: parseVnum(record?.objVnum) ?? undefined,
      roomVnum: parseVnum(record?.roomVnum) ?? undefined,
      containerVnum: parseVnum(record?.containerVnum) ?? undefined,
      maxInArea: parseVnum(record?.maxInArea) ?? undefined,
      maxInRoom: parseVnum(record?.maxInRoom) ?? undefined,
      count: parseVnum(record?.count) ?? undefined,
      maxInContainer: parseVnum(record?.maxInContainer) ?? undefined,
      wearLoc: typeof record?.wearLoc === "string" ? record.wearLoc : undefined,
      direction:
        typeof record?.direction === "string" ? record.direction : undefined,
      state: parseVnum(record?.state) ?? undefined,
      exits: parseVnum(record?.exits) ?? undefined
    };
  }, [selectedResetIndex, selectedResetRecord]);
  const shopFormDefaults = useMemo<ShopFormValues>(() => {
    const record = selectedShopRecord ?? {};
    const buyTypes = Array.isArray(record.buyTypes) ? record.buyTypes : [];
    return {
      keeper: parseVnum(record.keeper) ?? undefined,
      buyTypes: Array.from({ length: 5 }).map(
        (_, index) => parseVnum(buyTypes[index]) ?? 0
      ),
      profitBuy: parseVnum(record.profitBuy) ?? undefined,
      profitSell: parseVnum(record.profitSell) ?? undefined,
      openHour: parseVnum(record.openHour) ?? undefined,
      closeHour: parseVnum(record.closeHour) ?? undefined
    };
  }, [selectedShopRecord]);
  const questFormDefaults = useMemo<QuestFormValues>(() => {
    const record = selectedQuestRecord ?? {};
    const rewardObjs = Array.isArray(record.rewardObjs) ? record.rewardObjs : [];
    const rewardCounts = Array.isArray(record.rewardCounts)
      ? record.rewardCounts
      : [];
    return {
      vnum: parseVnum(record.vnum) ?? 0,
      name: getFirstString(record.name, ""),
      entry:
        typeof record.entry === "string"
          ? normalizeLineEndingsForDisplay(record.entry)
          : "",
      type: getFirstString(record.type, ""),
      xp: parseVnum(record.xp) ?? undefined,
      level: parseVnum(record.level) ?? undefined,
      end: parseVnum(record.end) ?? undefined,
      target: parseVnum(record.target) ?? undefined,
      upper: parseVnum(record.upper) ?? undefined,
      count: parseVnum(record.count) ?? undefined,
      rewardFaction: parseVnum(record.rewardFaction) ?? undefined,
      rewardReputation: parseVnum(record.rewardReputation) ?? undefined,
      rewardGold: parseVnum(record.rewardGold) ?? undefined,
      rewardSilver: parseVnum(record.rewardSilver) ?? undefined,
      rewardCopper: parseVnum(record.rewardCopper) ?? undefined,
      rewardObjs: Array.from({ length: 3 }).map(
        (_, index) => parseVnum(rewardObjs[index]) ?? 0
      ),
      rewardCounts: Array.from({ length: 3 }).map(
        (_, index) => parseVnum(rewardCounts[index]) ?? 0
      )
    };
  }, [selectedQuestRecord]);
  const factionFormDefaults = useMemo<FactionFormValues>(() => {
    const record = selectedFactionRecord ?? {};
    return {
      vnum: parseVnum(record.vnum) ?? 0,
      name: getFirstString(record.name, ""),
      defaultStanding: parseVnum(record.defaultStanding) ?? undefined,
      alliesCsv: formatNumberList(record.allies),
      opposingCsv: formatNumberList(record.opposing)
    };
  }, [selectedFactionRecord]);
  const recipeFormDefaults = useMemo<RecipeFormValues>(() => {
    const record = selectedRecipeRecord ?? {};
    const vnum = selectedRecipeVnum ?? parseVnum(record.vnum) ?? 0;
    const stationType = Array.isArray(record.stationType)
      ? record.stationType.filter((value): value is string => typeof value === "string")
      : [];
    const inputs = Array.isArray(record.inputs) ? record.inputs : [];
    return {
      vnum,
      name: getFirstString(record.name, ""),
      skill: getFirstString(record.skill, ""),
      minSkill: parseVnum(record.minSkill) ?? undefined,
      minSkillPct: parseVnum(record.minSkillPct) ?? undefined,
      minLevel: parseVnum(record.minLevel) ?? undefined,
      stationType,
      stationVnum: parseVnum(record.stationVnum) ?? undefined,
      discovery:
        typeof record.discovery === "string" ? record.discovery : undefined,
      inputs: inputs
        .filter(
          (input): input is Record<string, unknown> =>
            Boolean(input && typeof input === "object")
        )
        .map((input) => ({
          vnum: parseVnum(input.vnum) ?? 0,
          quantity: parseVnum(input.quantity) ?? 1
        })),
      outputVnum: parseVnum(record.outputVnum) ?? undefined,
      outputQuantity: parseVnum(record.outputQuantity) ?? undefined
    };
  }, [selectedRecipeRecord, selectedRecipeVnum]);
  const gatherSpawnFormDefaults = useMemo<GatherSpawnFormValues>(() => {
    const record = selectedGatherRecord ?? {};
    const vnum = selectedGatherVnum ?? parseVnum(record.vnum) ?? undefined;
    return {
      spawnSector:
        typeof record.spawnSector === "string"
          ? record.spawnSector
          : undefined,
      vnum,
      quantity: parseVnum(record.quantity) ?? undefined,
      respawnTimer: parseVnum(record.respawnTimer) ?? undefined
    };
  }, [selectedGatherRecord, selectedGatherVnum]);
  const activeResetCommand = (watchedResetCommand ??
    resetFormDefaults.commandName ??
    "").trim();
  const roomVnumOptions = useMemo(
    () => buildVnumOptions(areaData, "Rooms"),
    [areaData]
  );
  const roomVnumOptionMap = useMemo(() => {
    const map = new Map<number, VnumOption>();
    roomVnumOptions.forEach((option) => {
      map.set(option.vnum, option);
    });
    return map;
  }, [roomVnumOptions]);
  const mobileVnumOptions = useMemo(
    () => buildVnumOptions(areaData, "Mobiles"),
    [areaData]
  );
  const mobileVnumOptionMap = useMemo(() => {
    const map = new Map<number, VnumOption>();
    mobileVnumOptions.forEach((option) => {
      map.set(option.vnum, option);
    });
    return map;
  }, [mobileVnumOptions]);
  const objectVnumOptions = useMemo(
    () => buildVnumOptions(areaData, "Objects"),
    [areaData]
  );
  const objectVnumOptionMap = useMemo(() => {
    const map = new Map<number, VnumOption>();
    objectVnumOptions.forEach((option) => {
      map.set(option.vnum, option);
    });
    return map;
  }, [objectVnumOptions]);
  const roomResetItems = useMemo<RoomResetItem[]>(() => {
    if (!areaData || selectedRoomVnum === null) {
      return [];
    }
    const resets = getEntityList(areaData, "Resets");
    const items: RoomResetItem[] = [];
    resets.forEach((reset, index) => {
      if (!reset || typeof reset !== "object") {
        return;
      }
      const record = reset as Record<string, unknown>;
      if (!isResetForRoom(record, selectedRoomVnum)) {
        return;
      }
      const command = resolveResetCommand(record);
      const label = formatResetSummary(
        record,
        command,
        roomVnumOptionMap,
        mobileVnumOptionMap,
        objectVnumOptionMap
      );
      items.push({ index, command, label });
    });
    return items;
  }, [
    areaData,
    selectedRoomVnum,
    roomVnumOptionMap,
    mobileVnumOptionMap,
    objectVnumOptionMap
  ]);
  const selectedRoomResetIndex = useMemo(() => {
    if (selectedResetIndex === null) {
      return null;
    }
    return roomResetItems.some((item) => item.index === selectedResetIndex)
      ? selectedResetIndex
      : null;
  }, [roomResetItems, selectedResetIndex]);
  const lootTableOptions = useMemo(() => {
    if (!areaData) {
      return [];
    }
    const loot = (areaData as Record<string, unknown>).loot;
    if (!loot || typeof loot !== "object") {
      return [];
    }
    const tables = Array.isArray((loot as Record<string, unknown>).tables)
      ? ((loot as Record<string, unknown>).tables as unknown[])
      : [];
    return tables
      .map((table) => {
        if (!table || typeof table !== "object") {
          return "";
        }
        const name = (table as Record<string, unknown>).name;
        return typeof name === "string" ? name.trim() : "";
      })
      .filter((name) => name.length);
  }, [areaData]);
  const activeObjectBlock = useMemo(() => {
    const key = (
      watchedObjectItemType ??
      objectFormDefaults.itemType ??
      ""
    )
      .trim()
      .toLowerCase();
    return itemTypeBlockMap[key] ?? null;
  }, [objectFormDefaults.itemType, watchedObjectItemType]);
  const classColumns = classColumnsDef;
  const raceColumns = raceColumnsDef;
  const skillColumns = skillColumnsDef;
  const groupColumns = groupColumnsDef;
  const commandColumns = commandColumnsDef;
  const socialColumns = socialColumnsDef;
  const tutorialColumns = tutorialColumnsDef;
  const lootColumns = lootColumnsDef;
  const roomColumns = roomColumnsDef;
  const mobileColumns = mobileColumnsDef;
  const objectColumns = objectColumnsDef;
  const resetColumns = resetColumnsDef;
  const shopColumns = shopColumnsDef;
  const questColumns = questColumnsDef;
  const factionColumns = factionColumnsDef;
  const recipeColumns = recipeColumnsDef;
  const gatherSpawnColumns = gatherSpawnColumnsDef;
  const roomDefaultColDef = useMemo<ColDef>(
    () => ({
      sortable: true,
      resizable: true,
      filter: true
    }),
    []
  );
  const mobileDefaultColDef = roomDefaultColDef;
  const objectDefaultColDef = roomDefaultColDef;
  const classDefaultColDef = roomDefaultColDef;
  const raceDefaultColDef = roomDefaultColDef;
  const skillDefaultColDef = roomDefaultColDef;
  const groupDefaultColDef = roomDefaultColDef;
  const commandDefaultColDef = roomDefaultColDef;
  const socialDefaultColDef = roomDefaultColDef;
  const tutorialDefaultColDef = roomDefaultColDef;
  const lootDefaultColDef = roomDefaultColDef;
  const recipeDefaultColDef = roomDefaultColDef;
  const gatherSpawnDefaultColDef = roomDefaultColDef;

  useEffect(() => {
    areaForm.reset(areaFormDefaults);
  }, [areaForm, areaFormDefaults]);

  useEffect(() => {
    roomForm.reset(roomFormDefaults);
  }, [roomForm, roomFormDefaults]);

  useEffect(() => {
    mobileForm.reset(mobileFormDefaults);
  }, [mobileForm, mobileFormDefaults]);

  useEffect(() => {
    const record = selectedMobileRecord;
    setOverrideMobileFormFlags(Array.isArray(record?.formFlags));
    setOverrideMobilePartFlags(Array.isArray(record?.partFlags));
  }, [selectedMobileRecord]);

  useEffect(() => {
    objectForm.reset(objectFormDefaults);
  }, [objectForm, objectFormDefaults]);

  useEffect(() => {
    resetForm.reset(resetFormDefaults);
  }, [resetForm, resetFormDefaults]);

  useEffect(() => {
    shopForm.reset(shopFormDefaults);
  }, [shopForm, shopFormDefaults]);

  useEffect(() => {
    questForm.reset(questFormDefaults);
  }, [questForm, questFormDefaults]);

  useEffect(() => {
    factionForm.reset(factionFormDefaults);
  }, [factionForm, factionFormDefaults]);

  useEffect(() => {
    recipeForm.reset(recipeFormDefaults);
  }, [recipeForm, recipeFormDefaults]);

  useEffect(() => {
    gatherSpawnForm.reset(gatherSpawnFormDefaults);
  }, [gatherSpawnForm, gatherSpawnFormDefaults]);

  useEffect(() => {
    if (activeTab !== "Table" || selectedEntity !== "Rooms") {
      return;
    }
    syncGridSelection(roomGridApi.current, selectedRoomVnum);
  }, [activeTab, selectedEntity, selectedRoomVnum, roomRows]);

  useEffect(() => {
    if (activeTab !== "Table" || selectedEntity !== "Mobiles") {
      return;
    }
    syncGridSelection(mobileGridApi.current, selectedMobileVnum);
  }, [activeTab, selectedEntity, selectedMobileVnum, mobileRows]);

  useEffect(() => {
    if (activeTab !== "Table" || selectedEntity !== "Objects") {
      return;
    }
    syncGridSelection(objectGridApi.current, selectedObjectVnum);
  }, [activeTab, selectedEntity, selectedObjectVnum, objectRows]);

  useEffect(() => {
    if (activeTab !== "Table" || selectedEntity !== "Resets") {
      return;
    }
    syncGridSelection(resetGridApi.current, selectedResetIndex);
  }, [activeTab, selectedEntity, selectedResetIndex, resetRows]);

  useEffect(() => {
    if (activeTab !== "Table" || selectedEntity !== "Shops") {
      return;
    }
    syncGridSelection(shopGridApi.current, selectedShopKeeper);
  }, [activeTab, selectedEntity, selectedShopKeeper, shopRows]);

  useEffect(() => {
    if (activeTab !== "Table" || selectedEntity !== "Quests") {
      return;
    }
    syncGridSelection(questGridApi.current, selectedQuestVnum);
  }, [activeTab, selectedEntity, selectedQuestVnum, questRows]);

  useEffect(() => {
    if (activeTab !== "Table" || selectedEntity !== "Factions") {
      return;
    }
    syncGridSelection(factionGridApi.current, selectedFactionVnum);
  }, [activeTab, selectedEntity, selectedFactionVnum, factionRows]);

  useEffect(() => {
    if (activeTab !== "Table" || selectedEntity !== "Loot") {
      return;
    }
    const rowId =
      selectedAreaLootKind && selectedAreaLootIndex !== null
        ? `${selectedAreaLootKind}:${selectedAreaLootIndex}`
        : null;
    syncGridSelection(areaLootGridApi.current, rowId);
  }, [
    activeTab,
    selectedEntity,
    selectedAreaLootKind,
    selectedAreaLootIndex,
    areaLootRows
  ]);

  useEffect(() => {
    if (activeTab !== "Table" || selectedEntity !== "Recipes") {
      return;
    }
    syncGridSelection(recipeGridApi.current, selectedRecipeVnum);
  }, [activeTab, selectedEntity, selectedRecipeVnum, recipeRows]);

  useEffect(() => {
    if (activeTab !== "Table" || selectedEntity !== "Gather Spawns") {
      return;
    }
    syncGridSelection(gatherGridApi.current, selectedGatherVnum);
  }, [activeTab, selectedEntity, selectedGatherVnum, gatherSpawnRows]);

  useEffect(() => {
    if (
      editorMode !== "Global" ||
      selectedGlobalEntity !== "Classes" ||
      activeTab !== "Table"
    ) {
      return;
    }
    syncGridSelection(classGridApi.current, selectedClassIndex);
  }, [
    editorMode,
    selectedGlobalEntity,
    activeTab,
    selectedClassIndex,
    classRows
  ]);

  useEffect(() => {
    if (
      editorMode !== "Global" ||
      selectedGlobalEntity !== "Races" ||
      activeTab !== "Table"
    ) {
      return;
    }
    syncGridSelection(raceGridApi.current, selectedRaceIndex);
  }, [
    editorMode,
    selectedGlobalEntity,
    activeTab,
    selectedRaceIndex,
    raceRows
  ]);

  useEffect(() => {
    if (
      editorMode !== "Global" ||
      selectedGlobalEntity !== "Skills" ||
      activeTab !== "Table"
    ) {
      return;
    }
    syncGridSelection(skillGridApi.current, selectedSkillIndex);
  }, [
    editorMode,
    selectedGlobalEntity,
    activeTab,
    selectedSkillIndex,
    skillRows
  ]);

  useEffect(() => {
    if (
      editorMode !== "Global" ||
      selectedGlobalEntity !== "Groups" ||
      activeTab !== "Table"
    ) {
      return;
    }
    syncGridSelection(groupGridApi.current, selectedGroupIndex);
  }, [
    editorMode,
    selectedGlobalEntity,
    activeTab,
    selectedGroupIndex,
    groupRows
  ]);

  useEffect(() => {
    if (
      editorMode !== "Global" ||
      selectedGlobalEntity !== "Commands" ||
      activeTab !== "Table"
    ) {
      return;
    }
    syncGridSelection(commandGridApi.current, selectedCommandIndex);
  }, [
    editorMode,
    selectedGlobalEntity,
    activeTab,
    selectedCommandIndex,
    commandRows
  ]);

  useEffect(() => {
    if (
      editorMode !== "Global" ||
      selectedGlobalEntity !== "Socials" ||
      activeTab !== "Table"
    ) {
      return;
    }
    syncGridSelection(socialGridApi.current, selectedSocialIndex);
  }, [
    editorMode,
    selectedGlobalEntity,
    activeTab,
    selectedSocialIndex,
    socialRows
  ]);

  useEffect(() => {
    if (
      editorMode !== "Global" ||
      selectedGlobalEntity !== "Tutorials" ||
      activeTab !== "Table"
    ) {
      return;
    }
    syncGridSelection(tutorialGridApi.current, selectedTutorialIndex);
  }, [
    editorMode,
    selectedGlobalEntity,
    activeTab,
    selectedTutorialIndex,
    tutorialRows
  ]);

  useEffect(() => {
    if (
      editorMode !== "Global" ||
      selectedGlobalEntity !== "Loot" ||
      activeTab !== "Table"
    ) {
      return;
    }
    const rowId =
      selectedLootKind && selectedLootIndex !== null
        ? `${selectedLootKind}:${selectedLootIndex}`
        : null;
    syncGridSelection(lootGridApi.current, rowId);
  }, [
    editorMode,
    selectedGlobalEntity,
    activeTab,
    selectedLootKind,
    selectedLootIndex,
    lootRows
  ]);

  const handleOpenRoomLinkPanel = useCallback(
    (menu: RoomContextMenuState) => {
      setRoomLinkPanel({
        vnum: menu.vnum,
        x: menu.x + 8,
        y: menu.y + 8
      });
      setRoomLinkDirection(directions[0]);
      setRoomLinkTarget("");
      setRoomContextMenu(null);
    },
    []
  );

  const handleSelectRoomReset = useCallback(
    (index: number) => {
      setSelectedResetIndex(index);
    },
    [setSelectedResetIndex]
  );

  const handleCreateRoomReset = useCallback(() => {
    if (!areaData) {
      setStatusMessage("Load an area before creating resets.");
      return;
    }
    if (selectedRoomVnum === null) {
      setStatusMessage("Select a room to add a reset.");
      return;
    }
    const result = createReset(areaData);
    if (!result.ok) {
      setStatusMessage(result.message);
      return;
    }
    const resets = getEntityList(result.data.areaData, "Resets");
    const nextResets = resets.map((reset, index) => {
      if (index !== result.data.index) {
        return reset as Record<string, unknown>;
      }
      return {
        ...(reset as Record<string, unknown>),
        roomVnum: selectedRoomVnum
      };
    });
    const nextAreaData = {
      ...(result.data.areaData as Record<string, unknown>),
      resets: nextResets
    } as AreaJson;
    setAreaData(nextAreaData);
    setSelectedResetIndex(result.data.index);
    setStatusMessage(
      `Created reset #${result.data.index + 1} for room ${selectedRoomVnum} (unsaved)`
    );
  }, [areaData, selectedRoomVnum, setAreaData, setSelectedResetIndex, setStatusMessage]);

  const handleDeleteRoomReset = useCallback(() => {
    if (!areaData) {
      setStatusMessage("Load an area before deleting resets.");
      return;
    }
    if (selectedRoomResetIndex === null) {
      setStatusMessage("Select a room reset to delete.");
      return;
    }
    const result = deleteReset(areaData, selectedRoomResetIndex);
    if (!result.ok) {
      setStatusMessage(result.message);
      return;
    }
    setAreaData(result.data.areaData);
    setSelectedResetIndex(null);
    setStatusMessage(`Deleted reset #${result.data.deletedIndex + 1} (unsaved)`);
  }, [areaData, selectedRoomResetIndex, setAreaData, setSelectedResetIndex, setStatusMessage]);

  const handleDigRoom = useCallback(
    (fromVnum: number, direction: string) => {
      const sourceNode =
        roomNodesWithLayoutRef.current.find(
          (node) => node.data.vnum === fromVnum
        ) ?? null;
      const result = digRoom({
        areaData,
        fromVnum,
        direction,
        autoLayoutEnabled,
        roomNodeSize,
        sourceNodePosition: sourceNode ? sourceNode.position : null,
        parseVnum,
        getEntityList,
        findByVnum,
        getNextEntityVnum
      });
      if (!result.ok) {
        setStatusMessage(result.message);
        return;
      }
      setAreaData(result.areaData);
      if (result.layoutNode) {
        appendLayoutNode(result.layoutNode);
      }
      setSelectedRoomVnum(result.nextVnum);
      setSelectedEntity("Rooms");
      setStatusMessage(result.message);
    },
    [
      areaData,
      autoLayoutEnabled,
      roomNodeSize,
      parseVnum,
      getEntityList,
      findByVnum,
      getNextEntityVnum,
      setAreaData,
      appendLayoutNode,
      setSelectedEntity,
      setSelectedRoomVnum,
      setStatusMessage,
      digRoom
    ]
  );

  const handleLinkRooms = useCallback(() => {
    if (!roomLinkPanel) {
      return;
    }
    const result = linkRooms({
      areaData,
      fromVnum: roomLinkPanel.vnum,
      direction: roomLinkDirection,
      targetVnumInput: roomLinkTarget,
      parseVnum,
      getEntityList,
      findByVnum
    });
    if (!result.ok) {
      setStatusMessage(result.message);
      return;
    }
    setAreaData(result.areaData);
    setSelectedRoomVnum(roomLinkPanel.vnum);
    setSelectedEntity("Rooms");
    setRoomLinkPanel(null);
    setRoomLinkTarget("");
    setStatusMessage(result.message);
  }, [
    roomLinkPanel,
    areaData,
    roomLinkDirection,
    roomLinkTarget,
    parseVnum,
    getEntityList,
    findByVnum,
    setAreaData,
    setSelectedEntity,
    setSelectedRoomVnum,
    setStatusMessage,
    setRoomLinkPanel,
    setRoomLinkTarget,
    linkRooms
  ]);

  const handleAreaSubmit = (data: AreaFormValues) => {
    if (!areaData) {
      return;
    }
    const nextDaycyclePeriods = (data.daycyclePeriods ?? [])
      .map((period) => {
        const name = (period.name ?? "").trim();
        const description = normalizeOptionalMudText(period.description);
        const enterMessage = normalizeOptionalMudText(period.enterMessage);
        const exitMessage = normalizeOptionalMudText(period.exitMessage);
        const nextPeriod: Record<string, unknown> = {
          name,
          fromHour: period.fromHour ?? 0,
          toHour: period.toHour ?? period.fromHour ?? 0
        };
        if (description) {
          nextPeriod.description = description;
        }
        if (enterMessage) {
          nextPeriod.enterMessage = enterMessage;
        }
        if (exitMessage) {
          nextPeriod.exitMessage = exitMessage;
        }
        return nextPeriod;
      })
      .filter((period) => Boolean(period.name));
    const nextStoryBeats = (data.storyBeats ?? [])
      .map((beat) => {
        const title = beat.title.trim();
        const description = normalizeOptionalText(beat.description);
        const nextBeat: Record<string, unknown> = {
          title
        };
        if (description) {
          nextBeat.description = normalizeLineEndingsForMud(description);
        }
        return nextBeat;
      })
      .filter((beat) => Boolean(beat.title));
    const nextChecklist = (data.checklist ?? [])
      .map((item) => {
        const title = item.title.trim();
        const description = normalizeOptionalText(item.description);
        const nextItem: Record<string, unknown> = {
          title,
          status: item.status
        };
        if (description) {
          nextItem.description = normalizeLineEndingsForMud(description);
        }
        return nextItem;
      })
      .filter((item) => Boolean(item.title));
    setAreaData((current) => {
      if (!current) {
        return current;
      }
      const record = current as Record<string, unknown>;
      const areadata =
        record.areadata && typeof record.areadata === "object"
          ? (record.areadata as Record<string, unknown>)
          : {};
      const nextAreadata: Record<string, unknown> = {
        ...areadata,
        version: data.version,
        name: data.name,
        vnumRange: [data.vnumRangeStart, data.vnumRangeEnd],
        builders: normalizeOptionalText(data.builders),
        credits: normalizeOptionalText(data.credits),
        lootTable: cleanOptionalString(data.lootTable),
        security: data.security ?? undefined,
        sector: data.sector ?? undefined,
        lowLevel: data.lowLevel ?? undefined,
        highLevel: data.highLevel ?? undefined,
        reset: data.reset ?? undefined,
        alwaysReset: data.alwaysReset ? true : undefined,
        instType: data.instType === "multi" ? "multi" : undefined
      };
      const daycycle =
        data.daycycleSuppress || nextDaycyclePeriods.length
          ? {
              suppressDaycycleMessages: data.daycycleSuppress ? true : undefined,
              periods: nextDaycyclePeriods.length
                ? nextDaycyclePeriods
                : undefined
            }
          : undefined;
      return {
        ...record,
        areadata: nextAreadata,
        daycycle,
        storyBeats: nextStoryBeats.length ? nextStoryBeats : undefined,
        checklist: nextChecklist.length ? nextChecklist : undefined
      };
    });
    setStatusMessage("Updated area data (unsaved)");
  };

  const handleRoomSubmit = (data: RoomFormValues) => {
    if (!areaData || selectedRoomVnum === null) {
      return;
    }
    const normalizedTimePeriods = (data.timePeriods ?? [])
      .map((period) => {
        const name = (period.name ?? "").trim();
        const description = normalizeOptionalMudText(period.description);
        const enterMessage = normalizeOptionalMudText(period.enterMessage);
        const exitMessage = normalizeOptionalMudText(period.exitMessage);
        const nextPeriod: Record<string, unknown> = {
          name,
          fromHour: period.fromHour ?? 0,
          toHour: period.toHour ?? period.fromHour ?? 0
        };
        if (description) {
          nextPeriod.description = description;
        }
        if (enterMessage) {
          nextPeriod.enterMessage = enterMessage;
        }
        if (exitMessage) {
          nextPeriod.exitMessage = exitMessage;
        }
        return nextPeriod;
      })
      .filter((period) => Boolean(period.name));
    const normalizedExits = (data.exits ?? []).map((exit) => {
      const nextExit: Record<string, unknown> = {
        dir: exit.dir,
        toVnum: exit.toVnum
      };
      if (exit.key !== undefined) {
        nextExit.key = exit.key;
      }
      if (exit.flags && exit.flags.length) {
        nextExit.flags = exit.flags;
      }
      const description = normalizeOptionalMudText(exit.description);
      if (description) {
        nextExit.description = description;
      }
      if (exit.keyword && exit.keyword.trim().length) {
        nextExit.keyword = exit.keyword;
      }
      return nextExit;
    });
    setAreaData((current) => {
      if (!current) {
        return current;
      }
      const rooms = getEntityList(current, "Rooms");
      if (!rooms.length) {
        return current;
      }
      const nextRooms = rooms.map((room) => {
        const record = room as Record<string, unknown>;
        const roomVnum = parseVnum(record.vnum);
        if (roomVnum !== selectedRoomVnum) {
          return record;
        }
        const nextRoom: Record<string, unknown> = {
          ...record,
          name: data.name,
          description: normalizeLineEndingsForMud(data.description),
          sectorType: data.sectorType ?? undefined,
          roomFlags: data.roomFlags?.length ? data.roomFlags : undefined,
          suppressDaycycleMessages: data.suppressDaycycleMessages
            ? true
            : undefined,
          timePeriods: normalizedTimePeriods.length
            ? normalizedTimePeriods
            : undefined,
          manaRate: undefined,
          healRate: undefined,
          clan: undefined,
          owner: undefined,
          exits: normalizedExits.length ? normalizedExits : undefined
        };
        if (!("sectorType" in record) && "sector" in record) {
          nextRoom.sector = data.sectorType ?? undefined;
        }
        return nextRoom;
      });
      return {
        ...(current as Record<string, unknown>),
        rooms: nextRooms
      };
    });
    setStatusMessage(`Updated room ${data.vnum} (unsaved)`);
  };

  const handleMobileSubmit = (data: MobileFormValues) => {
    if (!areaData || selectedMobileVnum === null) {
      return;
    }
    const hitDice = normalizeDice(data.hitDice);
    const manaDice = normalizeDice(data.manaDice);
    const damageDice = normalizeDice(data.damageDice);
    setAreaData((current) => {
      if (!current) {
        return current;
      }
      const mobiles = getEntityList(current, "Mobiles");
      if (!mobiles.length) {
        return current;
      }
      const nextMobiles = mobiles.map((mob) => {
        const record = mob as Record<string, unknown>;
        const mobVnum = parseVnum(record.vnum);
        if (mobVnum !== selectedMobileVnum) {
          return record;
        }
        const nextMobile: Record<string, unknown> = {
          ...record,
          name: data.name,
          shortDescr: data.shortDescr,
          longDescr: normalizeLineEndingsForMud(data.longDescr),
          description: normalizeLineEndingsForMud(data.description),
          race: data.race,
          level: data.level ?? undefined,
          hitroll: data.hitroll ?? undefined,
          alignment: data.alignment ?? undefined,
          damType: data.damType ?? undefined,
          startPos: data.startPos ?? undefined,
          defaultPos: data.defaultPos ?? undefined,
          sex: data.sex ?? undefined,
          size: data.size ?? undefined,
          material: normalizeOptionalText(data.material),
          factionVnum: data.factionVnum ?? undefined,
          damageNoun: normalizeOptionalText(data.damageNoun),
          offensiveSpell: normalizeOptionalText(data.offensiveSpell),
          lootTable: cleanOptionalString(data.lootTable),
          actFlags: data.actFlags?.length ? data.actFlags : undefined,
          atkFlags: data.atkFlags?.length ? data.atkFlags : undefined,
          formFlags: overrideMobileFormFlags
            ? data.formFlags ?? []
            : undefined,
          partFlags: overrideMobilePartFlags
            ? data.partFlags ?? []
            : undefined,
          hitDice,
          manaDice,
          damageDice
        };
        return nextMobile;
      });
      return {
        ...(current as Record<string, unknown>),
        mobiles: nextMobiles
      };
    });
    setStatusMessage(`Updated mobile ${data.vnum} (unsaved)`);
  };

  const handleObjectSubmit = (data: ObjectFormValues) => {
    if (!areaData || selectedObjectVnum === null) {
      return;
    }
    const itemTypeKey = data.itemType.trim().toLowerCase();
    const activeBlock = itemTypeBlockMap[itemTypeKey] ?? null;
    const buildWeapon = () => {
      if (!data.weapon) {
        return undefined;
      }
      const next: Record<string, unknown> = {};
      if (data.weapon.class) {
        next.class = data.weapon.class;
      }
      if (
        data.weapon.diceNumber !== undefined &&
        data.weapon.diceType !== undefined
      ) {
        next.dice = [data.weapon.diceNumber, data.weapon.diceType];
      }
      if (data.weapon.damageType) {
        next.damageType = data.weapon.damageType;
      }
      if (data.weapon.flags && data.weapon.flags.length) {
        next.flags = data.weapon.flags;
      }
      return Object.keys(next).length ? next : undefined;
    };
    const buildArmor = () => {
      if (!data.armor) {
        return undefined;
      }
      const next: Record<string, unknown> = {};
      if (data.armor.acPierce !== undefined) next.acPierce = data.armor.acPierce;
      if (data.armor.acBash !== undefined) next.acBash = data.armor.acBash;
      if (data.armor.acSlash !== undefined) next.acSlash = data.armor.acSlash;
      if (data.armor.acExotic !== undefined) next.acExotic = data.armor.acExotic;
      return Object.keys(next).length ? next : undefined;
    };
    const buildContainer = () => {
      if (!data.container) {
        return undefined;
      }
      const next: Record<string, unknown> = {};
      if (data.container.capacity !== undefined)
        next.capacity = data.container.capacity;
      if (data.container.flags && data.container.flags.length)
        next.flags = data.container.flags;
      if (data.container.keyVnum !== undefined)
        next.keyVnum = data.container.keyVnum;
      if (data.container.maxWeight !== undefined)
        next.maxWeight = data.container.maxWeight;
      if (data.container.weightMult !== undefined)
        next.weightMult = data.container.weightMult;
      return Object.keys(next).length ? next : undefined;
    };
    const buildLight = () => {
      if (!data.light) {
        return undefined;
      }
      const next: Record<string, unknown> = {};
      if (data.light.hours !== undefined) next.hours = data.light.hours;
      return Object.keys(next).length ? next : undefined;
    };
    const buildDrink = (block?: ObjectFormValues["drink"]) => {
      if (!block) {
        return undefined;
      }
      const next: Record<string, unknown> = {};
      if (block.capacity !== undefined) next.capacity = block.capacity;
      if (block.remaining !== undefined) next.remaining = block.remaining;
      if (block.liquid) next.liquid = block.liquid;
      if (block.poisoned) next.poisoned = true;
      return Object.keys(next).length ? next : undefined;
    };
    const buildFood = () => {
      if (!data.food) {
        return undefined;
      }
      const next: Record<string, unknown> = {};
      if (data.food.foodHours !== undefined)
        next.foodHours = data.food.foodHours;
      if (data.food.fullHours !== undefined)
        next.fullHours = data.food.fullHours;
      if (data.food.poisoned) next.poisoned = true;
      return Object.keys(next).length ? next : undefined;
    };
    const buildMoney = () => {
      if (!data.money) {
        return undefined;
      }
      const next: Record<string, unknown> = {};
      if (data.money.gold !== undefined) next.gold = data.money.gold;
      if (data.money.silver !== undefined) next.silver = data.money.silver;
      return Object.keys(next).length ? next : undefined;
    };
    const buildWand = (block?: ObjectFormValues["wand"]) => {
      if (!block) {
        return undefined;
      }
      const next: Record<string, unknown> = {};
      if (block.level !== undefined) next.level = block.level;
      if (block.totalCharges !== undefined)
        next.totalCharges = block.totalCharges;
      if (block.chargesLeft !== undefined)
        next.chargesLeft = block.chargesLeft;
      const spell = normalizeOptionalText(block.spell);
      if (spell) next.spell = spell;
      return Object.keys(next).length ? next : undefined;
    };
    const buildSpells = () => {
      if (!data.spells) {
        return undefined;
      }
      const next: Record<string, unknown> = {};
      if (data.spells.level !== undefined) next.level = data.spells.level;
      const spell1 = normalizeOptionalText(data.spells.spell1);
      const spell2 = normalizeOptionalText(data.spells.spell2);
      const spell3 = normalizeOptionalText(data.spells.spell3);
      const spell4 = normalizeOptionalText(data.spells.spell4);
      if (spell1) next.spell1 = spell1;
      if (spell2) next.spell2 = spell2;
      if (spell3) next.spell3 = spell3;
      if (spell4) next.spell4 = spell4;
      return Object.keys(next).length ? next : undefined;
    };
    const buildPortal = () => {
      if (!data.portal) {
        return undefined;
      }
      const next: Record<string, unknown> = {};
      if (data.portal.charges !== undefined) next.charges = data.portal.charges;
      if (data.portal.exitFlags && data.portal.exitFlags.length)
        next.exitFlags = data.portal.exitFlags;
      if (data.portal.portalFlags && data.portal.portalFlags.length)
        next.portalFlags = data.portal.portalFlags;
      if (data.portal.toVnum !== undefined) next.toVnum = data.portal.toVnum;
      return Object.keys(next).length ? next : undefined;
    };
    const buildFurniture = () => {
      if (!data.furniture) {
        return undefined;
      }
      const next: Record<string, unknown> = {};
      if (data.furniture.slots !== undefined) next.slots = data.furniture.slots;
      if (data.furniture.weight !== undefined)
        next.weight = data.furniture.weight;
      if (data.furniture.flags && data.furniture.flags.length)
        next.flags = data.furniture.flags;
      if (data.furniture.healBonus !== undefined)
        next.healBonus = data.furniture.healBonus;
      if (data.furniture.manaBonus !== undefined)
        next.manaBonus = data.furniture.manaBonus;
      if (data.furniture.maxPeople !== undefined)
        next.maxPeople = data.furniture.maxPeople;
      return Object.keys(next).length ? next : undefined;
    };

    setAreaData((current) => {
      if (!current) {
        return current;
      }
      const objects = getEntityList(current, "Objects");
      if (!objects.length) {
        return current;
      }
      const nextObjects = objects.map((obj) => {
        const record = obj as Record<string, unknown>;
        const objVnum = parseVnum(record.vnum);
        if (objVnum !== selectedObjectVnum) {
          return record;
        }
        const nextObject: Record<string, unknown> = {
          ...record,
          name: data.name,
          shortDescr: data.shortDescr,
          description: normalizeLineEndingsForMud(data.description),
          material: data.material,
          itemType: data.itemType,
          extraFlags: data.extraFlags?.length ? data.extraFlags : undefined,
          wearFlags: data.wearFlags?.length ? data.wearFlags : undefined,
          level: data.level ?? undefined,
          weight: data.weight ?? undefined,
          cost: data.cost ?? undefined,
          condition: data.condition ?? undefined,
          weapon: activeBlock === "weapon" ? buildWeapon() : undefined,
          armor: activeBlock === "armor" ? buildArmor() : undefined,
          container: activeBlock === "container" ? buildContainer() : undefined,
          light: activeBlock === "light" ? buildLight() : undefined,
          drink: activeBlock === "drink" ? buildDrink(data.drink) : undefined,
          fountain:
            activeBlock === "fountain" ? buildDrink(data.fountain) : undefined,
          food: activeBlock === "food" ? buildFood() : undefined,
          money: activeBlock === "money" ? buildMoney() : undefined,
          wand: activeBlock === "wand" ? buildWand(data.wand) : undefined,
          staff: activeBlock === "staff" ? buildWand(data.staff) : undefined,
          spells: activeBlock === "spells" ? buildSpells() : undefined,
          portal: activeBlock === "portal" ? buildPortal() : undefined,
          furniture: activeBlock === "furniture" ? buildFurniture() : undefined
        };
        return nextObject;
      });
      return {
        ...(current as Record<string, unknown>),
        objects: nextObjects
      };
    });
    setStatusMessage(`Updated object ${data.vnum} (unsaved)`);
  };

  const handleResetSubmit = (data: ResetFormValues) => {
    if (!areaData || selectedResetIndex === null) {
      return;
    }
    const commandName = data.commandName.trim();
    const isKnown = resetCommandOptions.includes(commandName as ResetCommand);
    setAreaData((current) => {
      if (!current) {
        return current;
      }
      const resets = getEntityList(current, "Resets");
      if (!resets.length) {
        return current;
      }
      const nextResets = resets.map((reset, index) => {
        if (index !== selectedResetIndex) {
          return reset as Record<string, unknown>;
        }
        const record = reset as Record<string, unknown>;
        if (!isKnown) {
          return {
            ...record,
            commandName,
            command: undefined
          };
        }
        const nextReset: Record<string, unknown> = {
          ...record,
          commandName,
          command: undefined,
          mobVnum: undefined,
          objVnum: undefined,
          roomVnum: undefined,
          containerVnum: undefined,
          maxInArea: undefined,
          maxInRoom: undefined,
          count: undefined,
          maxInContainer: undefined,
          wearLoc: undefined,
          direction: undefined,
          state: undefined,
          exits: undefined,
          arg1: undefined,
          arg2: undefined,
          arg3: undefined,
          arg4: undefined
        };
        if (commandName === "loadMob") {
          nextReset.mobVnum = data.mobVnum ?? undefined;
          nextReset.roomVnum = data.roomVnum ?? undefined;
          nextReset.maxInArea = data.maxInArea ?? undefined;
          nextReset.maxInRoom = data.maxInRoom ?? undefined;
        } else if (commandName === "placeObj") {
          nextReset.objVnum = data.objVnum ?? undefined;
          nextReset.roomVnum = data.roomVnum ?? undefined;
        } else if (commandName === "putObj") {
          nextReset.objVnum = data.objVnum ?? undefined;
          nextReset.containerVnum = data.containerVnum ?? undefined;
          nextReset.count = data.count ?? undefined;
          nextReset.maxInContainer = data.maxInContainer ?? undefined;
        } else if (commandName === "giveObj") {
          nextReset.objVnum = data.objVnum ?? undefined;
        } else if (commandName === "equipObj") {
          nextReset.objVnum = data.objVnum ?? undefined;
          nextReset.wearLoc = data.wearLoc ?? undefined;
        } else if (commandName === "setDoor") {
          nextReset.roomVnum = data.roomVnum ?? undefined;
          nextReset.direction = data.direction ?? undefined;
          nextReset.state = data.state ?? undefined;
        } else if (commandName === "randomizeExits") {
          nextReset.roomVnum = data.roomVnum ?? undefined;
          nextReset.exits = data.exits ?? undefined;
        }
        return nextReset;
      });
      return {
        ...(current as Record<string, unknown>),
        resets: nextResets
      };
    });
    setStatusMessage(`Updated reset #${data.index} (unsaved)`);
  };

  const handleApplyRoomReset = useCallback(() => {
    void handleResetSubmitForm(handleResetSubmit)();
  }, [handleResetSubmitForm, handleResetSubmit]);

  const handleShopSubmit = (data: ShopFormValues) => {
    if (!areaData || selectedShopKeeper === null) {
      return;
    }
    const keeper =
      data.keeper !== undefined ? data.keeper : selectedShopKeeper;
    const buyTypes = Array.from({ length: 5 }).map(
      (_, index) => data.buyTypes?.[index] ?? 0
    );
    setAreaData((current) => {
      if (!current) {
        return current;
      }
      const shops = getEntityList(current, "Shops");
      if (!shops.length) {
        return current;
      }
      const nextShops = shops.map((shop) => {
        const record = shop as Record<string, unknown>;
        const shopKeeper = parseVnum(record.keeper);
        if (shopKeeper !== selectedShopKeeper) {
          return record;
        }
        return {
          ...record,
          keeper,
          buyTypes,
          profitBuy: data.profitBuy ?? 0,
          profitSell: data.profitSell ?? 0,
          openHour: data.openHour ?? 0,
          closeHour: data.closeHour ?? 0
        };
      });
      return {
        ...(current as Record<string, unknown>),
        shops: nextShops
      };
    });
    setSelectedShopKeeper(keeper);
    setStatusMessage(`Updated shop ${keeper} (unsaved)`);
  };

  const handleQuestSubmit = (data: QuestFormValues) => {
    if (!areaData || selectedQuestVnum === null) {
      return;
    }
    const rewardObjs = Array.from({ length: 3 }).map(
      (_, index) => data.rewardObjs?.[index] ?? 0
    );
    const rewardCounts = Array.from({ length: 3 }).map(
      (_, index) => data.rewardCounts?.[index] ?? 0
    );
    setAreaData((current) => {
      if (!current) {
        return current;
      }
      const quests = getEntityList(current, "Quests");
      if (!quests.length) {
        return current;
      }
      const nextQuests = quests.map((quest) => {
        const record = quest as Record<string, unknown>;
        const vnum = parseVnum(record.vnum);
        if (vnum !== selectedQuestVnum) {
          return record;
        }
        return {
          ...record,
          vnum: data.vnum,
          name: data.name ?? "",
          entry: normalizeLineEndingsForMud(data.entry ?? ""),
          type: data.type ?? "",
          xp: data.xp ?? 0,
          level: data.level ?? 0,
          end: data.end ?? 0,
          target: data.target ?? 0,
          upper: data.upper ?? 0,
          count: data.count ?? 0,
          rewardFaction: data.rewardFaction ?? 0,
          rewardReputation: data.rewardReputation ?? 0,
          rewardGold: data.rewardGold ?? 0,
          rewardSilver: data.rewardSilver ?? 0,
          rewardCopper: data.rewardCopper ?? 0,
          rewardObjs,
          rewardCounts
        };
      });
      return {
        ...(current as Record<string, unknown>),
        quests: nextQuests
      };
    });
    setStatusMessage(`Updated quest ${data.vnum} (unsaved)`);
  };

  const handleFactionSubmit = (data: FactionFormValues) => {
    if (!areaData || selectedFactionVnum === null) {
      return;
    }
    const allies = parseNumberList(data.alliesCsv);
    const opposing = parseNumberList(data.opposingCsv);
    setAreaData((current) => {
      if (!current) {
        return current;
      }
      const factions = getEntityList(current, "Factions");
      if (!factions.length) {
        return current;
      }
      const nextFactions = factions.map((faction) => {
        const record = faction as Record<string, unknown>;
        const vnum = parseVnum(record.vnum);
        if (vnum !== selectedFactionVnum) {
          return record;
        }
        const nextFaction: Record<string, unknown> = {
          ...record,
          vnum: data.vnum,
          name: data.name ?? "",
          defaultStanding: data.defaultStanding ?? 0
        };
        if (allies.length) {
          nextFaction.allies = allies;
        } else {
          nextFaction.allies = undefined;
        }
        if (opposing.length) {
          nextFaction.opposing = opposing;
        } else {
          nextFaction.opposing = undefined;
        }
        return nextFaction;
      });
      return {
        ...(current as Record<string, unknown>),
        factions: nextFactions
      };
    });
    setStatusMessage(`Updated faction ${data.vnum} (unsaved)`);
  };

  const handleAreaLootSubmit = (data: LootFormValues) => {
    if (
      !areaData ||
      selectedAreaLootKind === null ||
      selectedAreaLootIndex === null
    ) {
      return;
    }
    const toInt = (value: number | undefined, fallback: number) => {
      if (typeof value !== "number" || !Number.isFinite(value)) {
        return fallback;
      }
      return Math.trunc(value);
    };
    const toOptionalInt = (value: number | undefined) => {
      if (typeof value !== "number" || !Number.isFinite(value)) {
        return undefined;
      }
      return Math.trunc(value);
    };
    const name = data.name.trim() || "unnamed";
    setAreaData((current) => {
      if (!current) {
        return current;
      }
      const record = current as Record<string, unknown>;
      const loot =
        record.loot && typeof record.loot === "object"
          ? (record.loot as Record<string, unknown>)
          : {};
      const groups = Array.isArray(loot.groups)
        ? [...(loot.groups as LootGroup[])]
        : [];
      const tables = Array.isArray(loot.tables)
        ? [...(loot.tables as LootTable[])]
        : [];
      if (selectedAreaLootKind === "group") {
        const entries = (data.entries ?? []).map((entry) => {
          const type = entry.type === "cp" ? "cp" : "item";
          const nextEntry: LootEntry = {
            type,
            minQty: toInt(entry.minQty, 1),
            maxQty: toInt(entry.maxQty, 1),
            weight: toInt(entry.weight, 100)
          };
          if (type === "item") {
            nextEntry.vnum = toInt(entry.vnum, 0);
          }
          return nextEntry;
        });
        const nextRecord: LootGroup = {
          name,
          rolls: toInt(data.rolls, 1),
          entries
        };
        if (
          selectedAreaLootIndex < 0 ||
          selectedAreaLootIndex >= groups.length
        ) {
          return current;
        }
        groups[selectedAreaLootIndex] = nextRecord;
        return { ...record, loot: { ...loot, groups, tables } };
      }
      const ops = (data.ops ?? []).map((op) => {
        const nextOp: LootOp = { op: op.op };
        const group = cleanOptionalString(op.group);
        const vnum = toOptionalInt(op.vnum);
        const chance = toOptionalInt(op.chance);
        const minQty = toOptionalInt(op.minQty);
        const maxQty = toOptionalInt(op.maxQty);
        const multiplier = toOptionalInt(op.multiplier);
        switch (op.op) {
          case "use_group":
            if (group) nextOp.group = group;
            if (chance !== undefined) nextOp.chance = chance;
            break;
          case "add_item":
            if (vnum !== undefined) nextOp.vnum = vnum;
            if (chance !== undefined) nextOp.chance = chance;
            if (minQty !== undefined) nextOp.minQty = minQty;
            if (maxQty !== undefined) nextOp.maxQty = maxQty;
            break;
          case "add_cp":
            if (chance !== undefined) nextOp.chance = chance;
            if (minQty !== undefined) nextOp.minQty = minQty;
            if (maxQty !== undefined) nextOp.maxQty = maxQty;
            break;
          case "mul_cp":
          case "mul_all_chances":
            if (multiplier !== undefined) nextOp.multiplier = multiplier;
            break;
          case "remove_item":
            if (vnum !== undefined) nextOp.vnum = vnum;
            break;
          case "remove_group":
            if (group) nextOp.group = group;
            break;
          default:
            break;
        }
        return nextOp;
      });
      const nextRecord: LootTable = {
        name,
        parent: cleanOptionalString(data.parent),
        ops
      };
      if (
        selectedAreaLootIndex < 0 ||
        selectedAreaLootIndex >= tables.length
      ) {
        return current;
      }
      tables[selectedAreaLootIndex] = nextRecord;
      return { ...record, loot: { ...loot, groups, tables } };
    });
    setStatusMessage(
      `Updated loot ${selectedAreaLootKind} ${name} (unsaved)`
    );
  };

  const handleRecipeSubmit = (data: RecipeFormValues) => {
    if (!areaData || selectedRecipeVnum === null) {
      return;
    }
    const inputs = (data.inputs ?? [])
      .map((input) => ({
        vnum: input.vnum ?? 0,
        quantity: input.quantity ?? 1
      }))
      .filter((input) => input.vnum > 0);
    const nextRecord: RecipeDefinition = {
      vnum: data.vnum,
      name: cleanOptionalString(data.name),
      skill: cleanOptionalString(data.skill),
      minSkill: data.minSkill ?? undefined,
      minSkillPct: data.minSkillPct ?? undefined,
      minLevel: data.minLevel ?? undefined,
      stationType:
        data.stationType && data.stationType.length
          ? data.stationType
          : undefined,
      stationVnum: data.stationVnum ?? undefined,
      discovery: cleanOptionalString(data.discovery),
      inputs: inputs.length ? inputs : undefined,
      outputVnum: data.outputVnum ?? undefined,
      outputQuantity: data.outputQuantity ?? undefined
    };
    setAreaData((current) => {
      if (!current) {
        return current;
      }
      const recipes = getEntityList(current, "Recipes");
      if (!recipes.length) {
        return current;
      }
      const nextRecipes = recipes.map((recipe) => {
        const record = recipe as Record<string, unknown>;
        const vnum = parseVnum(record.vnum);
        if (vnum !== selectedRecipeVnum) {
          return record;
        }
        return nextRecord;
      });
      return {
        ...(current as Record<string, unknown>),
        recipes: nextRecipes
      };
    });
    setStatusMessage(`Updated recipe ${data.vnum} (unsaved)`);
  };

  const handleGatherSpawnSubmit = (data: GatherSpawnFormValues) => {
    if (!areaData || selectedGatherVnum === null) {
      return;
    }
    const nextRecord: GatherSpawnDefinition = {
      spawnSector: data.spawnSector ?? undefined,
      vnum: data.vnum ?? 0,
      quantity: data.quantity ?? undefined,
      respawnTimer: data.respawnTimer ?? undefined
    };
    setAreaData((current) => {
      if (!current) {
        return current;
      }
      const spawns = getEntityList(current, "Gather Spawns");
      if (!spawns.length) {
        return current;
      }
      const nextSpawns = spawns.map((spawn) => {
        const record = spawn as Record<string, unknown>;
        const vnum = parseVnum(record.vnum);
        if (vnum !== selectedGatherVnum) {
          return record;
        }
        return nextRecord;
      });
      return {
        ...(current as Record<string, unknown>),
        gatherSpawns: nextSpawns
      };
    });
    setStatusMessage(`Updated gather spawn ${nextRecord.vnum} (unsaved)`);
  };

  const handleClassSubmit = (data: ClassFormValues) => {
    if (!classData || selectedClassIndex === null) {
      return;
    }
    const guilds = Array.from({ length: classGuildCount }).map(
      (_, index) => data.guilds?.[index] ?? 0
    );
    const titles = buildTitlePairs(data.titlesMale ?? "", data.titlesFemale ?? "");
    const nextRecord: ClassDefinition = {
      name: data.name,
      whoName: cleanOptionalString(data.whoName),
      baseGroup: cleanOptionalString(data.baseGroup),
      defaultGroup: cleanOptionalString(data.defaultGroup),
      weaponVnum: data.weaponVnum,
      armorProf: cleanOptionalString(data.armorProf),
      guilds,
      primeStat: cleanOptionalString(data.primeStat),
      skillCap: data.skillCap,
      thac0_00: data.thac0_00,
      thac0_32: data.thac0_32,
      hpMin: data.hpMin,
      hpMax: data.hpMax,
      manaUser: data.manaUser,
      startLoc: data.startLoc,
      titles
    };
    setClassData((current) => {
      if (!current) {
        return current;
      }
      const nextClasses = [...current.classes];
      nextClasses[selectedClassIndex] = nextRecord;
      return { ...current, classes: nextClasses };
    });
    setStatusMessage(`Updated class ${data.name} (unsaved)`);
  };

  const handleRaceSubmit = (data: RaceFormValues) => {
    if (!raceData || selectedRaceIndex === null) {
      return;
    }
    const classNames = referenceData?.classes ?? [];
    const classMult: Record<string, number> = {};
    const classStart: Record<string, number> = {};
    classNames.forEach((name) => {
      const mult = Number(data.classMult?.[name] ?? 100);
      classMult[name] = Number.isFinite(mult) ? Math.trunc(mult) : 100;
      const start = Number(data.classStart?.[name] ?? 0);
      classStart[name] = Number.isFinite(start) ? Math.trunc(start) : 0;
    });
    const skills = (data.skills ?? [])
      .map((entry) => entry.trim())
      .filter((entry) => entry.length);
    const nextRecord: RaceDefinition = {
      name: data.name,
      whoName: cleanOptionalString(data.whoName),
      pc: data.pc,
      points: data.points,
      size: cleanOptionalString(data.size),
      startLoc: data.startLoc,
      stats: data.stats,
      maxStats: data.maxStats,
      actFlags: data.actFlags?.length ? data.actFlags : undefined,
      affectFlags: data.affectFlags?.length ? data.affectFlags : undefined,
      offFlags: data.offFlags?.length ? data.offFlags : undefined,
      immFlags: data.immFlags?.length ? data.immFlags : undefined,
      resFlags: data.resFlags?.length ? data.resFlags : undefined,
      vulnFlags: data.vulnFlags?.length ? data.vulnFlags : undefined,
      formFlags: data.formFlags?.length ? data.formFlags : undefined,
      partFlags: data.partFlags?.length ? data.partFlags : undefined,
      classMult: classNames.length ? classMult : undefined,
      classStart: classNames.length ? classStart : undefined,
      skills
    };
    setRaceData((current) => {
      if (!current) {
        return current;
      }
      const nextRaces = [...current.races];
      nextRaces[selectedRaceIndex] = nextRecord;
      return { ...current, races: nextRaces };
    });
    setStatusMessage(`Updated race ${data.name} (unsaved)`);
  };

  const handleSkillSubmit = (data: SkillFormValues) => {
    if (!skillData || selectedSkillIndex === null) {
      return;
    }
    const classNames = referenceData?.classes ?? [];
    const levels: Record<string, number> = {};
    const ratings: Record<string, number> = {};
    classNames.forEach((name) => {
      const level = Number(data.levels?.[name] ?? defaultSkillLevel);
      levels[name] = Number.isFinite(level) ? Math.trunc(level) : defaultSkillLevel;
      const rating = Number(data.ratings?.[name] ?? defaultSkillRating);
      ratings[name] = Number.isFinite(rating) ? Math.trunc(rating) : defaultSkillRating;
    });
    const nextRecord: SkillDefinition = {
      name: data.name,
      levels: classNames.length ? levels : undefined,
      ratings: classNames.length ? ratings : undefined,
      spell: cleanOptionalString(data.spell),
      loxSpell: cleanOptionalString(data.loxSpell),
      target: cleanOptionalString(data.target),
      minPosition: cleanOptionalString(data.minPosition),
      gsn: cleanOptionalString(data.gsn),
      slot: data.slot,
      minMana: data.minMana,
      beats: data.beats,
      nounDamage: cleanOptionalString(data.nounDamage),
      msgOff: cleanOptionalString(data.msgOff),
      msgObj: cleanOptionalString(data.msgObj)
    };
    setSkillData((current) => {
      if (!current) {
        return current;
      }
      const nextSkills = [...current.skills];
      nextSkills[selectedSkillIndex] = nextRecord;
      return { ...current, skills: nextSkills };
    });
    setStatusMessage(`Updated skill ${data.name} (unsaved)`);
  };

  const handleGroupSubmit = (data: GroupFormValues) => {
    if (!groupData || selectedGroupIndex === null) {
      return;
    }
    const classNames = referenceData?.classes ?? [];
    const ratings: Record<string, number> = {};
    classNames.forEach((name) => {
      const rating = Number(data.ratings?.[name] ?? defaultSkillRating);
      ratings[name] = Number.isFinite(rating)
        ? Math.trunc(rating)
        : defaultSkillRating;
    });
    const skills = (data.skills ?? [])
      .map((entry) => entry.trim())
      .filter((entry) => entry.length);
    const nextRecord: GroupDefinition = {
      name: data.name,
      ratings: classNames.length ? ratings : undefined,
      skills
    };
    setGroupData((current) => {
      if (!current) {
        return current;
      }
      const nextGroups = [...current.groups];
      nextGroups[selectedGroupIndex] = nextRecord;
      return { ...current, groups: nextGroups };
    });
    setStatusMessage(`Updated group ${data.name} (unsaved)`);
  };

  const handleCommandSubmit = (data: CommandFormValues) => {
    if (!commandData || selectedCommandIndex === null) {
      return;
    }
    const nextRecord: CommandDefinition = {
      name: data.name,
      function: cleanOptionalString(data.function),
      position: cleanOptionalString(data.position),
      level: data.level,
      log: cleanOptionalString(data.log),
      category: cleanOptionalString(data.category),
      loxFunction: cleanOptionalString(data.loxFunction)
    };
    setCommandData((current) => {
      if (!current) {
        return current;
      }
      const nextCommands = [...current.commands];
      nextCommands[selectedCommandIndex] = nextRecord;
      return { ...current, commands: nextCommands };
    });
    setStatusMessage(`Updated command ${data.name} (unsaved)`);
  };

  const handleSocialSubmit = (data: SocialFormValues) => {
    if (!socialData || selectedSocialIndex === null) {
      return;
    }
    const nextRecord: SocialDefinition = {
      name: data.name,
      charNoArg: normalizeOptionalMudText(data.charNoArg),
      othersNoArg: normalizeOptionalMudText(data.othersNoArg),
      charFound: normalizeOptionalMudText(data.charFound),
      othersFound: normalizeOptionalMudText(data.othersFound),
      victFound: normalizeOptionalMudText(data.victFound),
      charAuto: normalizeOptionalMudText(data.charAuto),
      othersAuto: normalizeOptionalMudText(data.othersAuto)
    };
    setSocialData((current) => {
      if (!current) {
        return current;
      }
      const nextSocials = [...current.socials];
      nextSocials[selectedSocialIndex] = nextRecord;
      return { ...current, socials: nextSocials };
    });
    setStatusMessage(`Updated social ${data.name} (unsaved)`);
  };

  const handleTutorialSubmit = (data: TutorialFormValues) => {
    if (!tutorialData || selectedTutorialIndex === null) {
      return;
    }
    const steps = (data.steps ?? [])
      .map((step) => {
        const prompt = normalizeOptionalMudText(step.prompt);
        if (!prompt) {
          return null;
        }
        const match = cleanOptionalString(step.match);
        return {
          prompt,
          match
        };
      })
      .filter(
        (
          step
        ): step is {
          prompt: string;
          match?: string;
        } => Boolean(step)
      );
    const nextRecord: TutorialDefinition = {
      name: data.name,
      blurb: normalizeOptionalMudText(data.blurb),
      finish: normalizeOptionalMudText(data.finish),
      minLevel: data.minLevel ?? 0,
      steps
    };
    setTutorialData((current) => {
      if (!current) {
        return current;
      }
      const nextTutorials = [...current.tutorials];
      nextTutorials[selectedTutorialIndex] = nextRecord;
      return { ...current, tutorials: nextTutorials };
    });
    setStatusMessage(`Updated tutorial ${data.name} (unsaved)`);
  };

  const handleLootSubmit = (data: LootFormValues) => {
    if (!lootData || selectedLootKind === null || selectedLootIndex === null) {
      return;
    }
    const toInt = (value: number | undefined, fallback: number) => {
      if (typeof value !== "number" || !Number.isFinite(value)) {
        return fallback;
      }
      return Math.trunc(value);
    };
    const toOptionalInt = (value: number | undefined) => {
      if (typeof value !== "number" || !Number.isFinite(value)) {
        return undefined;
      }
      return Math.trunc(value);
    };
    const name = data.name.trim() || "unnamed";
    if (selectedLootKind === "group") {
      const entries = (data.entries ?? []).map((entry) => {
        const type = entry.type === "cp" ? "cp" : "item";
        const nextEntry: LootEntry = {
          type,
          minQty: toInt(entry.minQty, 1),
          maxQty: toInt(entry.maxQty, 1),
          weight: toInt(entry.weight, 100)
        };
        if (type === "item") {
          nextEntry.vnum = toInt(entry.vnum, 0);
        }
        return nextEntry;
      });
      const nextRecord: LootGroup = {
        name,
        rolls: toInt(data.rolls, 1),
        entries
      };
      setLootData((current) => {
        if (!current) {
          return current;
        }
        const nextGroups = [...current.groups];
        if (
          selectedLootIndex < 0 ||
          selectedLootIndex >= nextGroups.length
        ) {
          return current;
        }
        nextGroups[selectedLootIndex] = nextRecord;
        return { ...current, groups: nextGroups };
      });
      setStatusMessage(`Updated loot group ${name} (unsaved)`);
      return;
    }
    const ops = (data.ops ?? []).map((op) => {
      const nextOp: LootOp = { op: op.op };
      const group = cleanOptionalString(op.group);
      const vnum = toOptionalInt(op.vnum);
      const chance = toOptionalInt(op.chance);
      const minQty = toOptionalInt(op.minQty);
      const maxQty = toOptionalInt(op.maxQty);
      const multiplier = toOptionalInt(op.multiplier);
      switch (op.op) {
        case "use_group":
          if (group) nextOp.group = group;
          if (chance !== undefined) nextOp.chance = chance;
          break;
        case "add_item":
          if (vnum !== undefined) nextOp.vnum = vnum;
          if (chance !== undefined) nextOp.chance = chance;
          if (minQty !== undefined) nextOp.minQty = minQty;
          if (maxQty !== undefined) nextOp.maxQty = maxQty;
          break;
        case "add_cp":
          if (chance !== undefined) nextOp.chance = chance;
          if (minQty !== undefined) nextOp.minQty = minQty;
          if (maxQty !== undefined) nextOp.maxQty = maxQty;
          break;
        case "mul_cp":
        case "mul_all_chances":
          if (multiplier !== undefined) nextOp.multiplier = multiplier;
          break;
        case "remove_item":
          if (vnum !== undefined) nextOp.vnum = vnum;
          break;
        case "remove_group":
          if (group) nextOp.group = group;
          break;
        default:
          break;
      }
      return nextOp;
    });
    const nextRecord: LootTable = {
      name,
      parent: cleanOptionalString(data.parent),
      ops
    };
    setLootData((current) => {
      if (!current) {
        return current;
      }
      const nextTables = [...current.tables];
      if (
        selectedLootIndex < 0 ||
        selectedLootIndex >= nextTables.length
      ) {
        return current;
      }
      nextTables[selectedLootIndex] = nextRecord;
      return { ...current, tables: nextTables };
    });
    setStatusMessage(`Updated loot table ${name} (unsaved)`);
  };

  const areaFormNode = (
    <AreaForm
      onSubmit={handleAreaSubmitForm(handleAreaSubmit)}
      register={registerArea}
      formState={areaFormState}
      sectors={sectors}
      storyBeatFields={storyBeatFields}
      appendStoryBeat={appendStoryBeat}
      removeStoryBeat={removeStoryBeat}
      moveStoryBeat={moveStoryBeat}
      checklistFields={checklistFields}
      appendChecklist={appendChecklist}
      removeChecklist={removeChecklist}
      moveChecklist={moveChecklist}
      daycycleFields={daycycleFields}
      appendDaycycle={appendDaycycle}
      removeDaycycle={removeDaycycle}
      moveDaycycle={moveDaycycle}
      onAddDefaultDaycycle={handleAddDefaultAreaPeriods}
      lootTableOptions={lootTableOptions}
      eventBindings={eventBindings}
      scriptValue={scriptValue}
      canEditScript={canEditScript}
      scriptEventEntity={scriptEventEntity}
      onEventBindingsChange={handleEventBindingsChange}
    />
  );
  const roomResetEditorNode =
    selectedRoomResetIndex === null ? (
      <div className="placeholder-block">
        <div className="placeholder-title">Select a reset to edit</div>
        <p>Choose a reset above to edit its details.</p>
      </div>
    ) : (
      <div className="block-card">
        <div className="block-card__header">
          <span>Reset #{selectedRoomResetIndex + 1}</span>
        </div>
        <ResetEditorFields
          idPrefix="room-reset"
          register={resetForm.register}
          control={resetFormControl}
          errors={resetFormState.errors}
          activeResetCommand={activeResetCommand}
          resetCommandOptions={resetCommandOptions}
          resetCommandLabels={resetCommandLabels}
          directions={directions}
          wearLocations={wearLocations}
          roomVnumOptions={roomVnumOptions}
          objectVnumOptions={objectVnumOptions}
          mobileVnumOptions={mobileVnumOptions}
        />
        <div className="form-actions">
          <button
            className="action-button action-button--primary"
            type="button"
            onClick={handleApplyRoomReset}
            disabled={!resetFormState.isDirty}
          >
            Apply Reset Changes
          </button>
          <span className="form-hint">
            {resetFormState.isDirty ? "Unsaved changes" : "Up to date"}
          </span>
        </div>
      </div>
    );
  const roomFormNode = (
    <RoomForm
      onSubmit={handleRoomSubmitForm(handleRoomSubmit)}
      register={registerRoom}
      control={roomFormControl}
      formState={roomFormState}
      exitFields={exitFields}
      appendExit={appendExit}
      removeExit={removeExit}
      directions={directions}
      sectors={sectors}
      roomFlags={roomFlagOptions}
      exitFlags={exitFlags}
      roomVnumOptions={roomVnumOptions}
      objectVnumOptions={objectVnumOptions}
      timePeriodFields={timePeriodFields}
      appendTimePeriod={appendTimePeriod}
      removeTimePeriod={removeTimePeriod}
      moveTimePeriod={moveTimePeriod}
      onAddDefaultPeriods={handleAddDefaultRoomPeriods}
      roomResets={roomResetItems}
      selectedRoomResetIndex={selectedRoomResetIndex}
      onSelectRoomReset={handleSelectRoomReset}
      onCreateRoomReset={handleCreateRoomReset}
      onDeleteRoomReset={handleDeleteRoomReset}
      canDeleteRoomReset={selectedRoomResetIndex !== null}
      roomResetEditor={roomResetEditorNode}
      canEditScript={canEditScript}
      scriptEventEntity={scriptEventEntity}
      eventBindings={eventBindings}
      scriptValue={scriptValue}
      onEventBindingsChange={handleEventBindingsChange}
    />
  );
  const mobileFormNode = (
    <MobileForm
      onSubmit={handleMobileSubmitForm(handleMobileSubmit)}
      register={registerMobile}
      formState={mobileFormState}
      positions={positions}
      sexes={sexes}
      sizes={sizes}
      damageTypes={damageTypes}
      mobActFlags={[...actFlags]}
      attackFlags={[...offFlags]}
      formFlagsOptions={[...formFlags]}
      partFlagsOptions={[...partFlags]}
      overrideFormFlags={overrideMobileFormFlags}
      overridePartFlags={overrideMobilePartFlags}
      onEnableFormFlagsOverride={() => {
        setOverrideMobileFormFlags(true);
        if (!getMobileFormValues("formFlags")?.length) {
          setMobileFormValue("formFlags", []);
        }
      }}
      onDisableFormFlagsOverride={() => {
        setOverrideMobileFormFlags(false);
        setMobileFormValue("formFlags", []);
      }}
      onEnablePartFlagsOverride={() => {
        setOverrideMobilePartFlags(true);
        if (!getMobileFormValues("partFlags")?.length) {
          setMobileFormValue("partFlags", []);
        }
      }}
      onDisablePartFlagsOverride={() => {
        setOverrideMobilePartFlags(false);
        setMobileFormValue("partFlags", []);
      }}
      raceOptions={referenceData?.races ?? []}
      canEditScript={canEditScript}
      scriptEventEntity={scriptEventEntity}
      eventBindings={eventBindings}
      scriptValue={scriptValue}
      onEventBindingsChange={handleEventBindingsChange}
      lootTableOptions={lootTableOptions}
    />
  );
  const objectFormNode = (
    <ObjectForm
      onSubmit={handleObjectSubmitForm(handleObjectSubmit)}
      register={registerObject}
      control={objectFormControl}
      formState={objectFormState}
      itemTypeOptions={itemTypeOptions}
      wearFlags={wearFlags}
      extraFlags={extraFlags}
      weaponClasses={weaponClasses}
      weaponFlags={weaponFlags}
      damageTypes={damageTypes}
      liquids={liquids}
      exitFlags={exitFlags}
      portalFlags={portalFlags}
      containerFlags={containerFlags}
      furnitureFlags={furnitureFlags}
      activeObjectBlock={activeObjectBlock}
      objectVnumOptions={objectVnumOptions}
      roomVnumOptions={roomVnumOptions}
      canEditScript={canEditScript}
      scriptEventEntity={scriptEventEntity}
      eventBindings={eventBindings}
      scriptValue={scriptValue}
      onEventBindingsChange={handleEventBindingsChange}
    />
  );
  const resetFormNode = (
    <ResetForm
      onSubmit={handleResetSubmitForm(handleResetSubmit)}
      register={registerReset}
      control={resetFormControl}
      formState={resetFormState}
      activeResetCommand={activeResetCommand}
      resetCommandOptions={resetCommandOptions}
      resetCommandLabels={resetCommandLabels}
      directions={directions}
      wearLocations={wearLocations}
      roomVnumOptions={roomVnumOptions}
      objectVnumOptions={objectVnumOptions}
      mobileVnumOptions={mobileVnumOptions}
    />
  );
  const shopFormNode = (
    <ShopForm
      onSubmit={handleShopSubmitForm(handleShopSubmit)}
      register={registerShop}
      control={shopFormControl}
      formState={shopFormState}
      mobileVnumOptions={mobileVnumOptions}
    />
  );
  const questFormNode = (
    <QuestForm
      onSubmit={handleQuestSubmitForm(handleQuestSubmit)}
      register={registerQuest}
      control={questFormControl}
      formState={questFormState}
      questTypeOptions={[...questTypeOptions]}
      objectVnumOptions={objectVnumOptions}
    />
  );
  const factionFormNode = (
    <FactionForm
      onSubmit={handleFactionSubmitForm(handleFactionSubmit)}
      register={registerFaction}
      formState={factionFormState}
    />
  );
  const activeAreaLootKind =
    selectedAreaLootKind ??
    (areaLootData && areaLootData.groups.length === 0 ? "table" : "group");
  const areaLootFormNode = (
    <LootForm
      onSubmit={handleAreaLootSubmitForm(handleAreaLootSubmit)}
      register={registerAreaLoot}
      control={areaLootFormControl}
      formState={areaLootFormState}
      kind={activeAreaLootKind}
      entryFields={areaLootEntryFields}
      opFields={areaLootOpFields}
      appendEntry={appendAreaLootEntry}
      removeEntry={removeAreaLootEntry}
      moveEntry={moveAreaLootEntry}
      appendOp={appendAreaLootOp}
      removeOp={removeAreaLootOp}
      moveOp={moveAreaLootOp}
      entryTypeOptions={[...lootEntryTypeOptions]}
      opTypeOptions={[...lootOpTypeOptions]}
      vnumOptions={objectVnumOptions}
    />
  );
  const recipeFormNode = (
    <RecipeForm
      onSubmit={handleRecipeSubmitForm(handleRecipeSubmit)}
      register={registerRecipe}
      control={recipeFormControl}
      formState={recipeFormState}
      inputFields={recipeInputFields}
      appendInput={appendRecipeInput}
      removeInput={removeRecipeInput}
      moveInput={moveRecipeInput}
      workstationTypeOptions={[...workstationTypes]}
      discoveryOptions={[...discoveryTypes]}
      objectVnumOptions={objectVnumOptions}
    />
  );
  const gatherSpawnFormNode = (
    <GatherSpawnForm
      onSubmit={handleGatherSpawnSubmitForm(handleGatherSpawnSubmit)}
      register={registerGatherSpawn}
      control={gatherSpawnFormControl}
      formState={gatherSpawnFormState}
      sectors={[...sectors]}
      objectVnumOptions={objectVnumOptions}
    />
  );
  const classFormNode = (
    <ClassForm
      onSubmit={handleClassSubmitForm(handleClassSubmit)}
      register={registerClass}
      formState={classFormState}
      primeStatOptions={[...primeStatOptions]}
      armorProfOptions={[...armorProfOptions]}
      titleCount={classTitleCount}
    />
  );
  const raceFormNode = (
    <RaceForm
      onSubmit={handleRaceSubmitForm(handleRaceSubmit)}
      register={registerRace}
      formState={raceFormState}
      sizeOptions={[...sizes]}
      statKeys={[...raceStatKeys]}
      actFlags={[...actFlags]}
      affectFlags={[...affectFlags]}
      offFlags={[...offFlags]}
      immFlags={[...immFlags]}
      resFlags={[...resFlags]}
      vulnFlags={[...vulnFlags.filter((flag) => flag !== "none")]}
      formFlags={[...formFlags]}
      partFlags={[...partFlags]}
      classNames={raceClassNames}
      skillCount={raceSkillCount}
    />
  );
  const skillFormNode = (
    <SkillForm
      onSubmit={handleSkillSubmitForm(handleSkillSubmit)}
      register={registerSkill}
      formState={skillFormState}
      classNames={skillClassNames}
      targetOptions={[...skillTargets]}
      positionOptions={[...positions]}
    />
  );
  const groupFormNode = (
    <GroupForm
      onSubmit={handleGroupSubmitForm(handleGroupSubmit)}
      register={registerGroup}
      formState={groupFormState}
      classNames={groupClassNames}
      skillOptions={groupSkillOptions}
      skillCount={groupSkillCount}
    />
  );
  const commandFormNode = (
    <CommandForm
      onSubmit={handleCommandSubmitForm(handleCommandSubmit)}
      register={registerCommand}
      formState={commandFormState}
      positionOptions={[...positions]}
      logOptions={[...logFlags]}
      categoryOptions={[...showFlags]}
    />
  );
  const socialFormNode = (
    <SocialForm
      onSubmit={handleSocialSubmitForm(handleSocialSubmit)}
      register={registerSocial}
      formState={socialFormState}
    />
  );
  const tutorialFormNode = (
    <TutorialForm
      onSubmit={handleTutorialSubmitForm(handleTutorialSubmit)}
      register={registerTutorial}
      formState={tutorialFormState}
      stepFields={tutorialStepFields}
      appendStep={appendTutorialStep}
      removeStep={removeTutorialStep}
      moveStep={moveTutorialStep}
    />
  );
  const activeLootKind =
    selectedLootKind ??
    (lootData && lootData.groups.length === 0 ? "table" : "group");
  const lootFormNode = (
    <LootForm
      onSubmit={handleLootSubmitForm(handleLootSubmit)}
      register={registerLoot}
      control={lootFormControl}
      formState={lootFormState}
      kind={activeLootKind}
      entryFields={lootEntryFields}
      opFields={lootOpFields}
      appendEntry={appendLootEntry}
      removeEntry={removeLootEntry}
      moveEntry={moveLootEntry}
      appendOp={appendLootOp}
      removeOp={removeLootOp}
      moveOp={moveLootOp}
      entryTypeOptions={[...lootEntryTypeOptions]}
      opTypeOptions={[...lootOpTypeOptions]}
      vnumOptions={objectVnumOptions}
    />
  );
  const canCreateRoom = Boolean(areaData);
  const tableToolbar = useMemo(() => {
    if (selectedEntity === "Rooms") {
      return {
        title: "Rooms",
        count: roomRows.length,
        newLabel: "New Room",
        deleteLabel: "Delete Room",
        canCreate: canCreateRoom,
        canDelete: selectedRoomVnum !== null,
        onCreate: handleCreateRoom,
        onDelete: handleDeleteRoom
      };
    }
    if (selectedEntity === "Mobiles") {
      return {
        title: "Mobiles",
        count: mobileRows.length,
        newLabel: "New Mobile",
        deleteLabel: "Delete Mobile",
        canCreate: Boolean(areaData),
        canDelete: selectedMobileVnum !== null,
        onCreate: handleCreateMobile,
        onDelete: handleDeleteMobile
      };
    }
    if (selectedEntity === "Objects") {
      return {
        title: "Objects",
        count: objectRows.length,
        newLabel: "New Object",
        deleteLabel: "Delete Object",
        canCreate: Boolean(areaData),
        canDelete: selectedObjectVnum !== null,
        onCreate: handleCreateObject,
        onDelete: handleDeleteObject
      };
    }
    if (selectedEntity === "Resets") {
      return {
        title: "Resets",
        count: resetRows.length,
        newLabel: "New Reset",
        deleteLabel: "Delete Reset",
        canCreate: Boolean(areaData),
        canDelete: selectedResetIndex !== null,
        onCreate: handleCreateReset,
        onDelete: handleDeleteReset
      };
    }
    if (selectedEntity === "Shops") {
      return {
        title: "Shops",
        count: shopRows.length,
        newLabel: "New Shop",
        deleteLabel: "Delete Shop",
        canCreate: Boolean(areaData),
        canDelete: selectedShopKeeper !== null,
        onCreate: handleCreateShop,
        onDelete: handleDeleteShop
      };
    }
    if (selectedEntity === "Quests") {
      return {
        title: "Quests",
        count: questRows.length,
        newLabel: "New Quest",
        deleteLabel: "Delete Quest",
        canCreate: Boolean(areaData),
        canDelete: selectedQuestVnum !== null,
        onCreate: handleCreateQuest,
        onDelete: handleDeleteQuest
      };
    }
    if (selectedEntity === "Factions") {
      return {
        title: "Factions",
        count: factionRows.length,
        newLabel: "New Faction",
        deleteLabel: "Delete Faction",
        canCreate: Boolean(areaData),
        canDelete: selectedFactionVnum !== null,
        onCreate: handleCreateFaction,
        onDelete: handleDeleteFaction
      };
    }
    if (selectedEntity === "Loot") {
      const hasSelection =
        selectedAreaLootKind !== null && selectedAreaLootIndex !== null;
      const kindLabel =
        selectedAreaLootKind === "table" ? "Table" : "Group";
      return {
        title: "Loot",
        count: areaLootRows.length,
        newLabel: "New Group",
        newLabelSecondary: "New Table",
        deleteLabel: `Delete ${kindLabel}`,
        canCreate: Boolean(areaData),
        canDelete: hasSelection,
        onCreate: () => handleCreateAreaLoot("group"),
        onCreateSecondary: () => handleCreateAreaLoot("table"),
        onDelete: handleDeleteAreaLoot
      };
    }
    if (selectedEntity === "Recipes") {
      return {
        title: "Recipes",
        count: recipeRows.length,
        newLabel: "New Recipe",
        deleteLabel: "Delete Recipe",
        canCreate: Boolean(areaData),
        canDelete: selectedRecipeVnum !== null,
        onCreate: handleCreateRecipe,
        onDelete: handleDeleteRecipe
      };
    }
    if (selectedEntity === "Gather Spawns") {
      return {
        title: "Gather Spawns",
        count: gatherSpawnRows.length,
        newLabel: "New Gather Spawn",
        deleteLabel: "Delete Gather Spawn",
        canCreate: Boolean(areaData),
        canDelete: selectedGatherVnum !== null,
        onCreate: handleCreateGatherSpawn,
        onDelete: handleDeleteGatherSpawn
      };
    }
    return null;
  }, [
    selectedEntity,
    roomRows.length,
    mobileRows.length,
    objectRows.length,
    resetRows.length,
    shopRows.length,
    questRows.length,
    factionRows.length,
    areaLootRows.length,
    recipeRows.length,
    gatherSpawnRows.length,
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
    canCreateRoom,
    areaData,
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
  ]);
  const tableViewNode = (
    <TableView
      selectedEntity={selectedEntity}
      roomRows={roomRows}
      mobileRows={mobileRows}
      objectRows={objectRows}
      resetRows={resetRows}
      shopRows={shopRows}
      questRows={questRows}
      factionRows={factionRows}
      lootRows={areaLootRows}
      recipeRows={recipeRows}
      gatherSpawnRows={gatherSpawnRows}
      roomColumns={roomColumns}
      mobileColumns={mobileColumns}
      objectColumns={objectColumns}
      resetColumns={resetColumns}
      shopColumns={shopColumns}
      questColumns={questColumns}
      factionColumns={factionColumns}
      lootColumns={lootColumns}
      recipeColumns={recipeColumns}
      gatherSpawnColumns={gatherSpawnColumns}
      roomDefaultColDef={roomDefaultColDef}
      mobileDefaultColDef={mobileDefaultColDef}
      objectDefaultColDef={objectDefaultColDef}
      resetDefaultColDef={roomDefaultColDef}
      shopDefaultColDef={roomDefaultColDef}
      questDefaultColDef={roomDefaultColDef}
      factionDefaultColDef={roomDefaultColDef}
      lootDefaultColDef={lootDefaultColDef}
      recipeDefaultColDef={recipeDefaultColDef}
      gatherSpawnDefaultColDef={gatherSpawnDefaultColDef}
      toolbar={tableToolbar}
      onSelectRoom={setSelectedRoomVnum}
      onSelectMobile={setSelectedMobileVnum}
      onSelectObject={setSelectedObjectVnum}
      onSelectReset={setSelectedResetIndex}
      onSelectShop={setSelectedShopKeeper}
      onSelectQuest={setSelectedQuestVnum}
      onSelectFaction={setSelectedFactionVnum}
      onSelectLoot={(kind, index) => {
        setSelectedAreaLootKind(kind);
        setSelectedAreaLootIndex(index);
      }}
      onSelectRecipe={setSelectedRecipeVnum}
      onSelectGatherSpawn={setSelectedGatherVnum}
      roomGridApiRef={roomGridApi}
      mobileGridApiRef={mobileGridApi}
      objectGridApiRef={objectGridApi}
      resetGridApiRef={resetGridApi}
      shopGridApiRef={shopGridApi}
      questGridApiRef={questGridApi}
      factionGridApiRef={factionGridApi}
      lootGridApiRef={areaLootGridApi}
      recipeGridApiRef={recipeGridApi}
      gatherSpawnGridApiRef={gatherGridApi}
    />
  );
  const classTableToolbar = useMemo(
    () => ({
      title: "Classes",
      count: classRows.length,
      newLabel: "New Class",
      deleteLabel: "Delete Class",
      canCreate: Boolean(classData),
      canDelete: selectedClassIndex !== null,
      onCreate: handleCreateClass,
      onDelete: handleDeleteClass
    }),
    [
      classRows.length,
      classData,
      selectedClassIndex,
      handleCreateClass,
      handleDeleteClass
    ]
  );
  const classTableViewNode = (
    <ClassTableView
      rows={classRows}
      columns={classColumns}
      defaultColDef={classDefaultColDef}
      toolbar={classTableToolbar}
      onSelectClass={setSelectedClassIndex}
      gridApiRef={classGridApi}
    />
  );
  const raceTableToolbar = useMemo(
    () => ({
      title: "Races",
      count: raceRows.length,
      newLabel: "New Race",
      deleteLabel: "Delete Race",
      canCreate: Boolean(raceData),
      canDelete: selectedRaceIndex !== null,
      onCreate: handleCreateRace,
      onDelete: handleDeleteRace
    }),
    [
      raceRows.length,
      raceData,
      selectedRaceIndex,
      handleCreateRace,
      handleDeleteRace
    ]
  );
  const raceTableViewNode = (
    <RaceTableView
      rows={raceRows}
      columns={raceColumns}
      defaultColDef={raceDefaultColDef}
      toolbar={raceTableToolbar}
      onSelectRace={setSelectedRaceIndex}
      gridApiRef={raceGridApi}
    />
  );
  const skillTableToolbar = useMemo(
    () => ({
      title: "Skills",
      count: skillRows.length,
      newLabel: "New Skill",
      deleteLabel: "Delete Skill",
      canCreate: Boolean(skillData),
      canDelete: selectedSkillIndex !== null,
      onCreate: handleCreateSkill,
      onDelete: handleDeleteSkill
    }),
    [
      skillRows.length,
      skillData,
      selectedSkillIndex,
      handleCreateSkill,
      handleDeleteSkill
    ]
  );
  const skillTableViewNode = (
    <SkillTableView
      rows={skillRows}
      columns={skillColumns}
      defaultColDef={skillDefaultColDef}
      toolbar={skillTableToolbar}
      onSelectSkill={setSelectedSkillIndex}
      gridApiRef={skillGridApi}
    />
  );
  const groupTableToolbar = useMemo(
    () => ({
      title: "Groups",
      count: groupRows.length,
      newLabel: "New Group",
      deleteLabel: "Delete Group",
      canCreate: Boolean(groupData),
      canDelete: selectedGroupIndex !== null,
      onCreate: handleCreateGroup,
      onDelete: handleDeleteGroup
    }),
    [
      groupRows.length,
      groupData,
      selectedGroupIndex,
      handleCreateGroup,
      handleDeleteGroup
    ]
  );
  const groupTableViewNode = (
    <GroupTableView
      rows={groupRows}
      columns={groupColumns}
      defaultColDef={groupDefaultColDef}
      toolbar={groupTableToolbar}
      onSelectGroup={setSelectedGroupIndex}
      gridApiRef={groupGridApi}
    />
  );
  const commandTableToolbar = useMemo(
    () => ({
      title: "Commands",
      count: commandRows.length,
      newLabel: "New Command",
      deleteLabel: "Delete Command",
      canCreate: Boolean(commandData),
      canDelete: selectedCommandIndex !== null,
      onCreate: handleCreateCommand,
      onDelete: handleDeleteCommand
    }),
    [
      commandRows.length,
      commandData,
      selectedCommandIndex,
      handleCreateCommand,
      handleDeleteCommand
    ]
  );
  const commandTableViewNode = (
    <CommandTableView
      rows={commandRows}
      columns={commandColumns}
      defaultColDef={commandDefaultColDef}
      toolbar={commandTableToolbar}
      onSelectCommand={setSelectedCommandIndex}
      gridApiRef={commandGridApi}
    />
  );
  const socialTableToolbar = useMemo(
    () => ({
      title: "Socials",
      count: socialRows.length,
      newLabel: "New Social",
      deleteLabel: "Delete Social",
      canCreate: Boolean(socialData),
      canDelete: selectedSocialIndex !== null,
      onCreate: handleCreateSocial,
      onDelete: handleDeleteSocial
    }),
    [
      socialRows.length,
      socialData,
      selectedSocialIndex,
      handleCreateSocial,
      handleDeleteSocial
    ]
  );
  const socialTableViewNode = (
    <SocialTableView
      rows={socialRows}
      columns={socialColumns}
      defaultColDef={socialDefaultColDef}
      toolbar={socialTableToolbar}
      onSelectSocial={setSelectedSocialIndex}
      gridApiRef={socialGridApi}
    />
  );
  const tutorialTableToolbar = useMemo(
    () => ({
      title: "Tutorials",
      count: tutorialRows.length,
      newLabel: "New Tutorial",
      deleteLabel: "Delete Tutorial",
      canCreate: Boolean(tutorialData),
      canDelete: selectedTutorialIndex !== null,
      onCreate: handleCreateTutorial,
      onDelete: handleDeleteTutorial
    }),
    [
      tutorialRows.length,
      tutorialData,
      selectedTutorialIndex,
      handleCreateTutorial,
      handleDeleteTutorial
    ]
  );
  const tutorialTableViewNode = (
    <TutorialTableView
      rows={tutorialRows}
      columns={tutorialColumns}
      defaultColDef={tutorialDefaultColDef}
      toolbar={tutorialTableToolbar}
      onSelectTutorial={setSelectedTutorialIndex}
      gridApiRef={tutorialGridApi}
    />
  );
  const lootTableToolbar = useMemo(() => {
    const hasSelection =
      selectedLootKind !== null && selectedLootIndex !== null;
    const kindLabel = selectedLootKind === "table" ? "Table" : "Group";
    return {
      title: "Loot",
      count: lootRows.length,
      newLabel: "New Group",
      newLabelSecondary: "New Table",
      deleteLabel: `Delete ${kindLabel}`,
      canCreate: Boolean(lootData),
      canDelete: hasSelection,
      onCreate: () => handleCreateLoot("group"),
      onCreateSecondary: () => handleCreateLoot("table"),
      onDelete: handleDeleteLoot
    };
  }, [
    lootRows.length,
    lootData,
    selectedLootKind,
    selectedLootIndex,
    handleCreateLoot,
    handleDeleteLoot
  ]);
  const lootTableViewNode = (
    <LootTableView
      rows={lootRows}
      columns={lootColumns}
      defaultColDef={lootDefaultColDef}
      toolbar={lootTableToolbar}
      onSelectLoot={(kind, index) => {
        setSelectedLootKind(kind);
        setSelectedLootIndex(index);
      }}
      gridApiRef={lootGridApi}
    />
  );
  const roomContextMenuNode = roomContextMenu ? (
    <div
      className="map-context-menu"
      style={{ left: roomContextMenu.x, top: roomContextMenu.y }}
      onClick={(event) => event.stopPropagation()}
      onContextMenu={(event) => event.preventDefault()}
    >
      <div className="map-context-menu__title">
        Room {roomContextMenu.vnum}
        {(() => {
          if (!areaData) {
            return "";
          }
          const room = findByVnum(
            getEntityList(areaData, "Rooms"),
            roomContextMenu.vnum
          );
          const name = room ? getFirstString(room.name, "") : "";
          return name ? ` · ${name}` : "";
        })()}
      </div>
      <div className="map-context-menu__section">Dig</div>
      <div className="map-context-menu__grid">
        {directions.map((dir) => (
          <button
            key={dir}
            className="map-context-menu__button"
            type="button"
            onClick={() => {
              handleDigRoom(roomContextMenu.vnum, dir);
              setRoomContextMenu(null);
            }}
          >
            {externalDirectionLabels[dir] ?? dir}
          </button>
        ))}
      </div>
      <div className="map-context-menu__section">Link</div>
      <button
        className="map-context-menu__action"
        type="button"
        onClick={() => handleOpenRoomLinkPanel(roomContextMenu)}
      >
        Link...
      </button>
    </div>
  ) : null;
  const roomLinkPanelNode = roomLinkPanel ? (
    <div
      className="map-context-menu map-context-menu--panel"
      style={{ left: roomLinkPanel.x, top: roomLinkPanel.y }}
      onClick={(event) => event.stopPropagation()}
      onContextMenu={(event) => event.preventDefault()}
    >
      <div className="map-context-menu__title">
        Link from room {roomLinkPanel.vnum}
      </div>
      <div className="map-context-menu__field">
        <label className="map-context-menu__label" htmlFor="room-link-direction">
          Direction
        </label>
        <select
          id="room-link-direction"
          className="form-select"
          value={roomLinkDirection}
          onChange={(event) => setRoomLinkDirection(event.target.value)}
        >
          {directions.map((dir) => (
            <option key={dir} value={dir}>
              {dir}
            </option>
          ))}
        </select>
      </div>
      <div className="map-context-menu__field">
        <label className="map-context-menu__label" htmlFor="room-link-target">
          Target VNUM
        </label>
        <input
          id="room-link-target"
          className="form-input"
          type="text"
          inputMode="numeric"
          list="room-link-target-options"
          value={roomLinkTarget}
          onChange={(event) => setRoomLinkTarget(event.target.value)}
        />
        <datalist id="room-link-target-options">
          {roomVnumOptions.map((option) => (
            <option
              key={option.vnum}
              value={String(option.vnum)}
              label={option.label}
            >
              {option.label}
            </option>
          ))}
        </datalist>
        {(() => {
          const parsed = Number.parseInt(roomLinkTarget, 10);
          const option = Number.isFinite(parsed)
            ? roomVnumOptionMap.get(parsed)
            : undefined;
          const hint =
            option?.entityType && option?.name
              ? `${option.entityType} · ${option.name}`
              : option?.label ?? null;
          return hint ? (
            <div className="map-context-menu__hint" title={hint}>
              {hint}
            </div>
          ) : null;
        })()}
      </div>
      <div className="map-context-menu__actions">
        <button
          className="map-context-menu__action"
          type="button"
          onClick={handleLinkRooms}
        >
          Create Link
        </button>
        <button
          className="map-context-menu__button"
          type="button"
          onClick={() => {
            setRoomLinkPanel(null);
            setRoomLinkTarget("");
          }}
        >
          Cancel
        </button>
      </div>
    </div>
  ) : null;
  const mapViewNode = (
    <MapView
      mapNodes={mapNodes}
      roomEdges={roomEdges}
      roomNodeTypes={roomNodeTypes}
      roomEdgeTypes={roomEdgeTypes}
      areaVnumRange={areaVnumRange}
      externalExits={externalExits}
      onNavigateExit={handleMapNavigate}
      onNodeClick={handleMapNodeClick}
      onNodesChange={handleNodesChange}
      onNodeDragStop={handleNodeDragStop}
      autoLayoutEnabled={autoLayoutEnabled}
      preferCardinalLayout={preferCardinalLayout}
      showVerticalEdges={showVerticalEdges}
      dirtyRoomCount={dirtyRoomCount}
      selectedRoomNode={Boolean(selectedRoomNode)}
      selectedRoomLocked={selectedRoomLocked}
      hasRoomLayout={hasRoomLayout}
      onRelayout={handleRelayout}
      onToggleAutoLayout={setAutoLayoutEnabled}
      onTogglePreferGrid={handleTogglePreferGrid}
      onToggleVerticalEdges={setShowVerticalEdges}
      onLockSelected={handleLockSelectedRoom}
      onUnlockSelected={handleUnlockSelectedRoom}
      onClearLayout={handleClearRoomLayout}
      contextMenu={
        roomContextMenuNode || roomLinkPanelNode ? (
          <>
            {roomContextMenuNode}
            {roomLinkPanelNode}
          </>
        ) : null
      }
      onCloseContextMenu={closeRoomContextOverlays}
    />
  );
  const worldViewNode = (
    <AreaGraphView
      nodes={worldMapNodes}
      edges={areaGraphEdges}
      edgeTypes={areaEdgeTypes}
      nodesDraggable={true}
      dirtyCount={dirtyAreaCount}
      selectedNodeLocked={selectedAreaLocked}
      hasLayout={hasAreaLayout}
      preferCardinalLayout={preferAreaCardinalLayout}
      filterValue={areaGraphFilter}
      vnumQuery={areaGraphVnumQuery}
      matchLabel={areaGraphMatchLabel}
      onFilterChange={setAreaGraphFilter}
      onVnumQueryChange={setAreaGraphVnumQuery}
      onNodeClick={() => null}
      onNodesChange={handleAreaGraphNodesChange}
      onNodeDragStop={handleAreaGraphNodeDragStop}
      onTogglePreferGrid={handleTogglePreferAreaGrid}
      onRelayout={handleRelayoutArea}
      onLockSelected={handleLockSelectedArea}
      onUnlockSelected={handleUnlockSelectedArea}
      onLockDirty={handleLockDirtyAreas}
      onClearLayout={handleClearAreaLayout}
    />
  );
  const scriptViewNode = (
    <ScriptView
      title={scriptTitle}
      subtitle={scriptSubtitle}
      value={scriptValue}
      canEdit={canEditScript}
      emptyState="Select an area, room, mobile, or object to edit its Lox script."
      onApply={handleApplyScript}
    />
  );
  const referenceSummary = referenceData
    ? `classes ${referenceData.classes.length}, races ${referenceData.races.length}, skills ${referenceData.skills.length}, groups ${referenceData.groups.length}, commands ${referenceData.commands.length}, socials ${referenceData.socials.length}, tutorials ${referenceData.tutorials.length}`
    : "not loaded";
  const classFileLabel = classDataPath
    ? fileNameFromPath(classDataPath)
    : "not loaded";
  const raceFileLabel = raceDataPath
    ? fileNameFromPath(raceDataPath)
    : "not loaded";
  const skillFileLabel = skillDataPath
    ? fileNameFromPath(skillDataPath)
    : "not loaded";
  const groupFileLabel = groupDataPath
    ? fileNameFromPath(groupDataPath)
    : "not loaded";
  const commandFileLabel = commandDataPath
    ? fileNameFromPath(commandDataPath)
    : "not loaded";
  const socialFileLabel = socialDataPath
    ? fileNameFromPath(socialDataPath)
    : "not loaded";
  const tutorialFileLabel = tutorialDataPath
    ? fileNameFromPath(tutorialDataPath)
    : "not loaded";
  const lootFileLabel = lootDataPath
    ? fileNameFromPath(lootDataPath)
    : "not loaded";
  const globalFileLabels: Record<GlobalEntityKey, string> = {
    Classes: classFileLabel,
    Races: raceFileLabel,
    Skills: skillFileLabel,
    Groups: groupFileLabel,
    Commands: commandFileLabel,
    Socials: socialFileLabel,
    Tutorials: tutorialFileLabel,
    Loot: lootFileLabel
  };
  const dataFileLabel = globalFileLabels[selectedGlobalEntity] ?? "not loaded";
  const classActionsNode = (
    <GlobalFormActions
      label="Classes"
      dataFileLabel={classFileLabel}
      loadDisabled={!dataDirectory || isBusy}
      saveDisabled={!classData || isBusy}
      onLoad={handleLoadClassesData}
      onSave={handleSaveClassesData}
    />
  );
  const raceActionsNode = (
    <GlobalFormActions
      label="Races"
      dataFileLabel={raceFileLabel}
      loadDisabled={!dataDirectory || isBusy}
      saveDisabled={!raceData || isBusy}
      onLoad={handleLoadRacesData}
      onSave={handleSaveRacesData}
    />
  );
  const skillActionsNode = (
    <GlobalFormActions
      label="Skills"
      dataFileLabel={skillFileLabel}
      loadDisabled={!dataDirectory || isBusy}
      saveDisabled={!skillData || isBusy}
      onLoad={handleLoadSkillsData}
      onSave={handleSaveSkillsData}
    />
  );
  const groupActionsNode = (
    <GlobalFormActions
      label="Groups"
      dataFileLabel={groupFileLabel}
      loadDisabled={!dataDirectory || isBusy}
      saveDisabled={!groupData || isBusy}
      onLoad={handleLoadGroupsData}
      onSave={handleSaveGroupsData}
    />
  );
  const commandActionsNode = (
    <GlobalFormActions
      label="Commands"
      dataFileLabel={commandFileLabel}
      loadDisabled={!dataDirectory || isBusy}
      saveDisabled={!commandData || isBusy}
      onLoad={handleLoadCommandsData}
      onSave={handleSaveCommandsData}
    />
  );
  const socialActionsNode = (
    <GlobalFormActions
      label="Socials"
      dataFileLabel={socialFileLabel}
      loadDisabled={!dataDirectory || isBusy}
      saveDisabled={!socialData || isBusy}
      onLoad={handleLoadSocialsData}
      onSave={handleSaveSocialsData}
    />
  );
  const tutorialActionsNode = (
    <GlobalFormActions
      label="Tutorials"
      dataFileLabel={tutorialFileLabel}
      loadDisabled={!dataDirectory || isBusy}
      saveDisabled={!tutorialData || isBusy}
      onLoad={handleLoadTutorialsData}
      onSave={handleSaveTutorialsData}
    />
  );
  const lootActionsNode = (
    <GlobalFormActions
      label="Loot"
      dataFileLabel={lootFileLabel}
      loadDisabled={!dataDirectory || isBusy}
      saveDisabled={!lootData || isBusy}
      onLoad={handleLoadLootData}
      onSave={handleSaveLootData}
    />
  );
  const viewTitle =
    editorMode === "Area" ? selectedEntity : selectedGlobalEntity;
  const viewMeta =
    editorMode === "Area"
      ? [
          `VNUM range ${selection.vnumRange}`,
          `Last save ${selection.lastSave}`,
          `Active view ${activeTab}`
        ]
      : [
          `Data dir ${dataDirectory ?? "not set"}`,
          `Data file ${dataFileLabel}`,
          `Reference ${referenceSummary}`,
          `Active view ${activeTab}`
        ];
  const visibleTabs =
    editorMode === "Area"
      ? tabs
      : tabs.filter((tab) => tab.id === "Table" || tab.id === "Form");
  const treeItems = editorMode === "Area" ? entityItems : globalItems;
  const treeSelection =
    editorMode === "Area" ? selectedEntity : selectedGlobalEntity;
  const statusSelection =
    editorMode === "Area"
      ? selectedEntity
      : `Global · ${selectedGlobalEntity}`;
  const handleTreeSelect = (entity: string) => {
    if (editorMode === "Area") {
      setSelectedEntity(entity as EntityKey);
    } else {
      setSelectedGlobalEntity(entity as GlobalEntityKey);
    }
  };

  return (
    <div className="app-shell">
      <Topbar
        onOpenProject={handleOpenProject}
        onOpenArea={handleOpenArea}
        onSetAreaDirectory={handleSetAreaDirectory}
        onSaveArea={handleSaveArea}
        onSaveEditorMeta={handleSaveEditorMeta}
        onSaveAreaAs={handleSaveAreaAs}
      />

      <section className="workspace">
        <EntityTree items={treeItems} />

        <main className="center">
          <ViewTabs tabs={visibleTabs} />

          <div className="view-card">
            <ViewCardHeader
              title={viewTitle}
              meta={viewMeta}
            />
            <div className="view-card__body">
              <ViewBody
                activeTab={activeTab}
                editorMode={editorMode}
                selectedEntity={selectedEntity}
                selectedGlobalEntity={selectedGlobalEntity}
                tabs={visibleTabs}
                hasAreaData={Boolean(areaData)}
                classCount={classRows.length}
                raceCount={raceRows.length}
                skillCount={skillRows.length}
                groupCount={groupRows.length}
                commandCount={commandRows.length}
                socialCount={socialRows.length}
                tutorialCount={tutorialRows.length}
                lootCount={lootCount}
                classActions={classActionsNode}
                raceActions={raceActionsNode}
                skillActions={skillActionsNode}
                groupActions={groupActionsNode}
                commandActions={commandActionsNode}
                socialActions={socialActionsNode}
                tutorialActions={tutorialActionsNode}
                lootActions={lootActionsNode}
                areaLootCount={areaLootRows.length}
                recipeCount={recipeRows.length}
                gatherSpawnCount={gatherSpawnRows.length}
                roomCount={roomRows.length}
                mobileCount={mobileRows.length}
                objectCount={objectRows.length}
                resetCount={resetRows.length}
                shopCount={shopRows.length}
                questCount={questRows.length}
                factionCount={factionRows.length}
                mapHasRooms={mapNodes.length > 0}
                areaForm={areaFormNode}
                roomForm={roomFormNode}
                mobileForm={mobileFormNode}
                objectForm={objectFormNode}
                resetForm={resetFormNode}
                shopForm={shopFormNode}
                questForm={questFormNode}
                factionForm={factionFormNode}
                classForm={classFormNode}
                raceForm={raceFormNode}
                skillForm={skillFormNode}
                groupForm={groupFormNode}
                commandForm={commandFormNode}
                socialForm={socialFormNode}
                tutorialForm={tutorialFormNode}
                lootForm={lootFormNode}
                areaLootForm={areaLootFormNode}
                recipeForm={recipeFormNode}
                gatherSpawnForm={gatherSpawnFormNode}
                tableView={tableViewNode}
                classTableView={classTableViewNode}
                raceTableView={raceTableViewNode}
                skillTableView={skillTableViewNode}
                groupTableView={groupTableViewNode}
                commandTableView={commandTableViewNode}
                socialTableView={socialTableViewNode}
                tutorialTableView={tutorialTableViewNode}
                lootTableView={lootTableViewNode}
                mapView={mapViewNode}
                worldView={worldViewNode}
                scriptView={scriptViewNode}
              />
            </div>
          </div>
        </main>

        <InspectorPanel
          selection={selection}
          validationSummary={validationSummary}
          diagnosticsCount={diagnosticsCount}
          diagnosticFilter={diagnosticFilter}
          entityOrder={entityOrder}
          diagnosticsList={diagnosticsList}
          entityKindLabels={entityKindLabels}
          onDiagnosticFilterChange={(value) =>
            setDiagnosticFilter(value as "All" | EntityKey)
          }
          onDiagnosticsClick={handleDiagnosticsClick}
          areaDebug={areaDebug}
        />
      </section>

      <Statusbar
        projectPath={projectConfig?.path ?? null}
        referenceSummary={referenceSummary}
      />
    </div>
  );
}
