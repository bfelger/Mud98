import type { ColDef } from "ag-grid-community";
import type { SkillDataFile, SkillDefinition } from "../../repository/types";
import {
  defaultSkillLevel,
  defaultSkillRating
} from "../../constants/globalDefaults";
import { crudFail, crudOk, type CrudResult } from "../types";

export type SkillRow = {
  index: number;
  name: string;
  target: string;
  minPosition: string;
  spell: string;
  slot: number;
};

type SkillCrudCreateResult = {
  skillData: SkillDataFile;
  index: number;
  name: string;
};

type SkillCrudDeleteResult = {
  skillData: SkillDataFile;
  deletedName: string;
};

export const buildSkillRows = (skillData: SkillDataFile | null): SkillRow[] => {
  if (!skillData) {
    return [];
  }
  return skillData.skills.map((skill, index) => ({
    index,
    name: skill.name ?? "(unnamed)",
    target: skill.target ?? "tar_ignore",
    minPosition: skill.minPosition ?? "dead",
    spell: skill.spell ?? "",
    slot: skill.slot ?? 0
  }));
};

export const skillColumns: ColDef<SkillRow>[] = [
  { headerName: "#", field: "index", width: 80, sort: "asc" },
  { headerName: "Name", field: "name", flex: 2, minWidth: 220 },
  { headerName: "Target", field: "target", width: 150 },
  { headerName: "Position", field: "minPosition", width: 140 },
  { headerName: "Spell", field: "spell", flex: 1, minWidth: 160 },
  { headerName: "Slot", field: "slot", width: 110 }
];

export const createSkill = (
  skillData: SkillDataFile | null,
  classNames: string[]
): CrudResult<SkillCrudCreateResult> => {
  if (!skillData) {
    return crudFail("Load skills before creating skills.");
  }
  const nextIndex = skillData.skills.length;
  const nextClassNames = classNames ?? [];
  const levels = Object.fromEntries(
    nextClassNames.map((name) => [name, defaultSkillLevel])
  );
  const ratings = Object.fromEntries(
    nextClassNames.map((name) => [name, defaultSkillRating])
  );
  const newSkill: SkillDefinition = {
    name: `New Skill ${nextIndex + 1}`,
    levels,
    ratings,
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
  };
  const nextSkillData = {
    ...skillData,
    skills: [...skillData.skills, newSkill]
  };
  return crudOk({
    skillData: nextSkillData,
    index: nextIndex,
    name: newSkill.name
  });
};

export const deleteSkill = (
  skillData: SkillDataFile | null,
  selectedIndex: number | null
): CrudResult<SkillCrudDeleteResult> => {
  if (!skillData) {
    return crudFail("Load skills before deleting skills.");
  }
  if (selectedIndex === null) {
    return crudFail("Select a skill to delete.");
  }
  const deletedName = skillData.skills[selectedIndex]?.name ?? "skill";
  const nextSkills = skillData.skills.filter(
    (_, index) => index !== selectedIndex
  );
  return crudOk({
    skillData: { ...skillData, skills: nextSkills },
    deletedName
  });
};
