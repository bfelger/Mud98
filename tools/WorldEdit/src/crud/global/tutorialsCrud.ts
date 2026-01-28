import type { ColDef } from "ag-grid-community";
import type {
  TutorialDataFile,
  TutorialDefinition
} from "../../repository/types";
import { crudFail, crudOk, type CrudResult } from "../types";

export type TutorialRow = {
  index: number;
  name: string;
  minLevel: number;
  steps: number;
};

type TutorialCrudCreateResult = {
  tutorialData: TutorialDataFile;
  index: number;
  name: string;
};

type TutorialCrudDeleteResult = {
  tutorialData: TutorialDataFile;
  deletedName: string;
};

export const buildTutorialRows = (
  tutorialData: TutorialDataFile | null
): TutorialRow[] => {
  if (!tutorialData) {
    return [];
  }
  return tutorialData.tutorials.map((tutorial, index) => ({
    index,
    name: tutorial.name ?? "(unnamed)",
    minLevel: tutorial.minLevel ?? 0,
    steps: tutorial.steps?.length ?? 0
  }));
};

export const tutorialColumns: ColDef<TutorialRow>[] = [
  { headerName: "#", field: "index", width: 80, sort: "asc" },
  { headerName: "Name", field: "name", flex: 2, minWidth: 220 },
  { headerName: "Min Level", field: "minLevel", width: 120 },
  { headerName: "Steps", field: "steps", width: 110 }
];

export const createTutorial = (
  tutorialData: TutorialDataFile | null
): CrudResult<TutorialCrudCreateResult> => {
  if (!tutorialData) {
    return crudFail("Load tutorials before creating tutorials.");
  }
  const nextIndex = tutorialData.tutorials.length;
  const newTutorial: TutorialDefinition = {
    name: `New Tutorial ${nextIndex + 1}`,
    blurb: "",
    finish: "",
    minLevel: 0,
    steps: []
  };
  const nextTutorialData = {
    ...tutorialData,
    tutorials: [...tutorialData.tutorials, newTutorial]
  };
  return crudOk({
    tutorialData: nextTutorialData,
    index: nextIndex,
    name: newTutorial.name
  });
};

export const deleteTutorial = (
  tutorialData: TutorialDataFile | null,
  selectedIndex: number | null
): CrudResult<TutorialCrudDeleteResult> => {
  if (!tutorialData) {
    return crudFail("Load tutorials before deleting tutorials.");
  }
  if (selectedIndex === null) {
    return crudFail("Select a tutorial to delete.");
  }
  const deletedName =
    tutorialData.tutorials[selectedIndex]?.name ?? "tutorial";
  const nextTutorials = tutorialData.tutorials.filter(
    (_, index) => index !== selectedIndex
  );
  return crudOk({
    tutorialData: { ...tutorialData, tutorials: nextTutorials },
    deletedName
  });
};
