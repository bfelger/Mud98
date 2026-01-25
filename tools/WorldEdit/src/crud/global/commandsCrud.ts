import type { ColDef } from "ag-grid-community";
import type { CommandDataFile, CommandDefinition } from "../../repository/types";
import { crudFail, crudOk, type CrudResult } from "../types";

export type CommandRow = {
  index: number;
  name: string;
  function: string;
  position: string;
  level: number;
  log: string;
  category: string;
  loxFunction: string;
};

type CommandCrudCreateResult = {
  commandData: CommandDataFile;
  index: number;
  name: string;
};

type CommandCrudDeleteResult = {
  commandData: CommandDataFile;
  deletedName: string;
};

export const buildCommandRows = (
  commandData: CommandDataFile | null
): CommandRow[] => {
  if (!commandData) {
    return [];
  }
  return commandData.commands.map((command, index) => ({
    index,
    name: command.name ?? "(unnamed)",
    function: command.function ?? "do_nothing",
    position: command.position ?? "dead",
    level: command.level ?? 0,
    log: command.log ?? "log_normal",
    category: command.category ?? "undef",
    loxFunction: command.loxFunction ?? ""
  }));
};

export const commandColumns: ColDef<CommandRow>[] = [
  { headerName: "#", field: "index", width: 80, sort: "asc" },
  { headerName: "Name", field: "name", flex: 1, minWidth: 180 },
  { headerName: "Function", field: "function", flex: 1, minWidth: 180 },
  { headerName: "Position", field: "position", width: 140 },
  { headerName: "Level", field: "level", width: 110 },
  { headerName: "Log", field: "log", width: 130 },
  { headerName: "Category", field: "category", width: 140 },
  { headerName: "Lox", field: "loxFunction", flex: 1, minWidth: 160 }
];

export const createCommand = (
  commandData: CommandDataFile | null
): CrudResult<CommandCrudCreateResult> => {
  if (!commandData) {
    return crudFail("Load commands before creating commands.");
  }
  const nextIndex = commandData.commands.length;
  const newCommand: CommandDefinition = {
    name: `newcommand${nextIndex + 1}`,
    function: "",
    position: "",
    level: 0,
    log: "",
    category: "",
    loxFunction: ""
  };
  const nextCommandData = {
    ...commandData,
    commands: [...commandData.commands, newCommand]
  };
  return crudOk({
    commandData: nextCommandData,
    index: nextIndex,
    name: newCommand.name
  });
};

export const deleteCommand = (
  commandData: CommandDataFile | null,
  selectedIndex: number | null
): CrudResult<CommandCrudDeleteResult> => {
  if (!commandData) {
    return crudFail("Load commands before deleting commands.");
  }
  if (selectedIndex === null) {
    return crudFail("Select a command to delete.");
  }
  const deletedName =
    commandData.commands[selectedIndex]?.name ?? "command";
  const nextCommands = commandData.commands.filter(
    (_, index) => index !== selectedIndex
  );
  return crudOk({
    commandData: { ...commandData, commands: nextCommands },
    deletedName
  });
};
