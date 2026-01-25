import type { ColDef } from "ag-grid-community";
import type { GroupDataFile, GroupDefinition } from "../../repository/types";
import {
  defaultSkillRating,
  groupSkillCount
} from "../../constants/globalDefaults";
import { crudFail, crudOk, type CrudResult } from "../types";

export type GroupRow = {
  index: number;
  name: string;
  ratingSummary: string;
  skills: number;
};

type GroupCrudCreateResult = {
  groupData: GroupDataFile;
  index: number;
  name: string;
};

type GroupCrudDeleteResult = {
  groupData: GroupDataFile;
  deletedName: string;
};

export const buildGroupRows = (groupData: GroupDataFile | null): GroupRow[] => {
  if (!groupData) {
    return [];
  }
  return groupData.groups.map((group, index) => ({
    index,
    name: group.name ?? "(unnamed)",
    ratingSummary: Array.isArray(group.ratings)
      ? `${group.ratings.length} ratings`
      : group.ratings
        ? `${Object.keys(group.ratings).length} ratings`
        : "default",
    skills: group.skills?.length ?? 0
  }));
};

export const groupColumns: ColDef<GroupRow>[] = [
  { headerName: "#", field: "index", width: 80, sort: "asc" },
  { headerName: "Name", field: "name", flex: 2, minWidth: 220 },
  { headerName: "Ratings", field: "ratingSummary", width: 140 },
  { headerName: "Skills", field: "skills", width: 110 }
];

export const createGroup = (
  groupData: GroupDataFile | null,
  classNames: string[]
): CrudResult<GroupCrudCreateResult> => {
  if (!groupData) {
    return crudFail("Load groups before creating groups.");
  }
  const nextIndex = groupData.groups.length;
  const nextClassNames = classNames ?? [];
  const ratings = Object.fromEntries(
    nextClassNames.map((name) => [name, defaultSkillRating])
  );
  const newGroup: GroupDefinition = {
    name: `New Group ${nextIndex + 1}`,
    ratings,
    skills: new Array(groupSkillCount).fill("")
  };
  const nextGroupData = {
    ...groupData,
    groups: [...groupData.groups, newGroup]
  };
  return crudOk({
    groupData: nextGroupData,
    index: nextIndex,
    name: newGroup.name
  });
};

export const deleteGroup = (
  groupData: GroupDataFile | null,
  selectedIndex: number | null
): CrudResult<GroupCrudDeleteResult> => {
  if (!groupData) {
    return crudFail("Load groups before deleting groups.");
  }
  if (selectedIndex === null) {
    return crudFail("Select a group to delete.");
  }
  const deletedName = groupData.groups[selectedIndex]?.name ?? "group";
  const nextGroups = groupData.groups.filter(
    (_, index) => index !== selectedIndex
  );
  return crudOk({
    groupData: { ...groupData, groups: nextGroups },
    deletedName
  });
};
