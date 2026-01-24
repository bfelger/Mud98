import type { ColDef } from "ag-grid-community";
import type { ClassDataFile, ClassDefinition } from "../../repository/types";
import { crudFail, crudOk, type CrudResult } from "../types";

export type ClassRow = {
  index: number;
  name: string;
  whoName: string;
  primeStat: string;
  armorProf: string;
  weaponVnum: number;
  startLoc: number;
};

type ClassCrudCreateResult = {
  classData: ClassDataFile;
  index: number;
  name: string;
};

type ClassCrudDeleteResult = {
  classData: ClassDataFile;
  deletedName: string;
};

const classGuildCount = 2;

export const buildClassRows = (classData: ClassDataFile | null): ClassRow[] => {
  if (!classData) {
    return [];
  }
  return classData.classes.map((cls, index) => ({
    index,
    name: cls.name ?? "(unnamed)",
    whoName: cls.whoName ?? "",
    primeStat: cls.primeStat ?? "default",
    armorProf: cls.armorProf ?? "default",
    weaponVnum: cls.weaponVnum ?? 0,
    startLoc: cls.startLoc ?? 0
  }));
};

export const classColumns: ColDef<ClassRow>[] = [
  { headerName: "#", field: "index", width: 80, sort: "asc" },
  { headerName: "Name", field: "name", flex: 2, minWidth: 200 },
  { headerName: "Who", field: "whoName", width: 110 },
  { headerName: "Prime", field: "primeStat", width: 100 },
  { headerName: "Armor", field: "armorProf", width: 120 },
  { headerName: "Weapon", field: "weaponVnum", width: 120 },
  { headerName: "Start", field: "startLoc", width: 110 }
];

export const createClass = (
  classData: ClassDataFile | null
): CrudResult<ClassCrudCreateResult> => {
  if (!classData) {
    return crudFail("Load classes before creating classes.");
  }
  const nextIndex = classData.classes.length;
  const newRecord: ClassDefinition = {
    name: `New Class ${nextIndex + 1}`,
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
    titles: []
  };
  const nextClassData = {
    ...classData,
    classes: [...classData.classes, newRecord]
  };
  return crudOk({
    classData: nextClassData,
    index: nextIndex,
    name: newRecord.name
  });
};

export const deleteClass = (
  classData: ClassDataFile | null,
  selectedIndex: number | null
): CrudResult<ClassCrudDeleteResult> => {
  if (!classData) {
    return crudFail("Load classes before deleting classes.");
  }
  if (selectedIndex === null) {
    return crudFail("Select a class to delete.");
  }
  const deletedName = classData.classes[selectedIndex]?.name ?? "class";
  const nextClasses = classData.classes.filter(
    (_, index) => index !== selectedIndex
  );
  return crudOk({
    classData: { ...classData, classes: nextClasses },
    deletedName
  });
};
