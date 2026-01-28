import type { ColDef } from "ag-grid-community";
import type { AreaJson } from "../../repository/types";
import { crudFail, crudOk, type CrudResult } from "../types";
import { getFirstString, getNextVnumFromList, parseVnum } from "./utils";

export type RecipeRow = {
  vnum: number;
  name: string;
  skill: string;
  inputs: string;
  output: string;
};

type RecipeCrudCreateResult = {
  areaData: AreaJson;
  vnum: number;
};

type RecipeCrudDeleteResult = {
  areaData: AreaJson;
  deletedVnum: number;
};

export const recipeColumns: ColDef<RecipeRow>[] = [
  { headerName: "VNUM", field: "vnum", width: 110, sort: "asc" },
  { headerName: "Name", field: "name", flex: 2, minWidth: 200 },
  { headerName: "Skill", field: "skill", width: 160 },
  { headerName: "Inputs", field: "inputs", width: 130 },
  { headerName: "Output", field: "output", width: 140 }
];

export const buildRecipeRows = (areaData: AreaJson | null): RecipeRow[] => {
  const recipes = Array.isArray((areaData as Record<string, unknown>)?.recipes)
    ? ((areaData as Record<string, unknown>).recipes as unknown[])
    : [];
  return recipes
    .filter((entry): entry is Record<string, unknown> =>
      Boolean(entry && typeof entry === "object")
    )
    .map((record) => {
      const vnum = parseVnum(record.vnum) ?? 0;
      const inputs = Array.isArray(record.inputs) ? record.inputs : [];
      const outputVnum = parseVnum(record.outputVnum);
      const outputQty = parseVnum(record.outputQuantity) ?? 1;
      return {
        vnum,
        name: getFirstString(record.name, "(unnamed recipe)"),
        skill: getFirstString(record.skill, "—"),
        inputs: `${inputs.length} input${inputs.length === 1 ? "" : "s"}`,
        output:
          outputVnum !== null
            ? `${outputVnum}${outputQty > 1 ? ` x${outputQty}` : ""}`
            : "—"
      };
    });
};

export const createRecipe = (
  areaData: AreaJson | null
): CrudResult<RecipeCrudCreateResult> => {
  if (!areaData) {
    return crudFail("Load an area before creating recipes.");
  }
  const recipes = Array.isArray((areaData as Record<string, unknown>).recipes)
    ? ((areaData as Record<string, unknown>).recipes as unknown[])
    : [];
  const nextVnum = getNextVnumFromList(areaData, recipes);
  if (nextVnum === null) {
    return crudFail("No available recipe VNUMs in the area range.");
  }
  const newRecipe: Record<string, unknown> = {
    vnum: nextVnum,
    name: "New Recipe",
    skill: "",
    minSkill: 0,
    minLevel: 1,
    stationType: [],
    inputs: [],
    outputs: []
  };
  const nextRecipes = [...recipes, newRecipe];
  const nextAreaData = {
    ...(areaData as Record<string, unknown>),
    recipes: nextRecipes
  } as AreaJson;
  return crudOk({ areaData: nextAreaData, vnum: nextVnum });
};

export const deleteRecipe = (
  areaData: AreaJson | null,
  selectedVnum: number | null
): CrudResult<RecipeCrudDeleteResult> => {
  if (!areaData) {
    return crudFail("Load an area before deleting recipes.");
  }
  if (selectedVnum === null) {
    return crudFail("Select a recipe to delete.");
  }
  const recipes = Array.isArray((areaData as Record<string, unknown>).recipes)
    ? ((areaData as Record<string, unknown>).recipes as unknown[])
    : [];
  const nextRecipes = recipes.filter((recipe) => {
    if (!recipe || typeof recipe !== "object") {
      return true;
    }
    const vnum = parseVnum((recipe as Record<string, unknown>).vnum);
    return vnum !== selectedVnum;
  });
  const nextAreaData = {
    ...(areaData as Record<string, unknown>),
    recipes: nextRecipes
  } as AreaJson;
  return crudOk({ areaData: nextAreaData, deletedVnum: selectedVnum });
};
