import type { ColDef } from "ag-grid-community";
import type { AreaJson } from "../../repository/types";
import { crudFail, crudOk, type CrudResult } from "../types";
import { getFirstString, getNextVnumFromList, parseVnum } from "./utils";

export type QuestRow = {
  vnum: number;
  name: string;
  type: string;
  level: number;
  target: string;
  rewards: string;
};

type QuestCrudCreateResult = {
  areaData: AreaJson;
  vnum: number;
};

type QuestCrudDeleteResult = {
  areaData: AreaJson;
  deletedVnum: number;
};

export const questColumns: ColDef<QuestRow>[] = [
  { headerName: "VNUM", field: "vnum", width: 110, sort: "asc" },
  { headerName: "Name", field: "name", flex: 2, minWidth: 200 },
  { headerName: "Type", field: "type", flex: 1, minWidth: 140 },
  { headerName: "Level", field: "level", width: 110 },
  { headerName: "Target", field: "target", flex: 1, minWidth: 160 },
  { headerName: "Rewards", field: "rewards", flex: 1, minWidth: 160 }
];

export const buildQuestRows = (areaData: AreaJson | null): QuestRow[] => {
  if (!areaData) {
    return [];
  }
  const quests = Array.isArray((areaData as Record<string, unknown>).quests)
    ? ((areaData as Record<string, unknown>).quests as unknown[])
    : [];
  if (!quests.length) {
    return [];
  }
  return quests.map((quest) => {
    const record = quest as Record<string, unknown>;
    const vnum = parseVnum(record.vnum) ?? -1;
    const name = getFirstString(record.name, "(unnamed quest)");
    const type = getFirstString(record.type, "—");
    const level = parseVnum(record.level) ?? 0;
    const target = parseVnum(record.target);
    const end = parseVnum(record.end);
    const targetLabel =
      target !== null && end !== null ? `${target} -> ${end}` : "—";
    const rewardGold = parseVnum(record.rewardGold) ?? 0;
    const rewardSilver = parseVnum(record.rewardSilver) ?? 0;
    const rewardCopper = parseVnum(record.rewardCopper) ?? 0;
    const rewards = `G:${rewardGold} S:${rewardSilver} C:${rewardCopper}`;
    return {
      vnum,
      name,
      type,
      level,
      target: targetLabel,
      rewards
    };
  });
};

export const createQuest = (
  areaData: AreaJson | null
): CrudResult<QuestCrudCreateResult> => {
  if (!areaData) {
    return crudFail("Load an area before creating quests.");
  }
  const quests = Array.isArray((areaData as Record<string, unknown>).quests)
    ? ((areaData as Record<string, unknown>).quests as unknown[])
    : [];
  const nextVnum = getNextVnumFromList(areaData, quests);
  if (nextVnum === null) {
    return crudFail("No available quest VNUMs in the area range.");
  }
  const newQuest: Record<string, unknown> = {
    vnum: nextVnum,
    name: "New Quest",
    entry: "",
    type: "",
    xp: 0,
    level: 1,
    rewardObjs: [0, 0, 0],
    rewardCounts: [0, 0, 0]
  };
  const nextQuests = [...quests, newQuest];
  const nextAreaData = {
    ...(areaData as Record<string, unknown>),
    quests: nextQuests
  } as AreaJson;
  return crudOk({ areaData: nextAreaData, vnum: nextVnum });
};

export const deleteQuest = (
  areaData: AreaJson | null,
  selectedVnum: number | null
): CrudResult<QuestCrudDeleteResult> => {
  if (!areaData) {
    return crudFail("Load an area before deleting quests.");
  }
  if (selectedVnum === null) {
    return crudFail("Select a quest to delete.");
  }
  const quests = Array.isArray((areaData as Record<string, unknown>).quests)
    ? ((areaData as Record<string, unknown>).quests as unknown[])
    : [];
  const nextQuests = quests.filter((quest) => {
    if (!quest || typeof quest !== "object") {
      return true;
    }
    const vnum = parseVnum((quest as Record<string, unknown>).vnum);
    return vnum !== selectedVnum;
  });
  const nextAreaData = {
    ...(areaData as Record<string, unknown>),
    quests: nextQuests
  } as AreaJson;
  return crudOk({ areaData: nextAreaData, deletedVnum: selectedVnum });
};
