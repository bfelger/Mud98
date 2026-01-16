export type EventEntityKey = "Rooms" | "Mobiles" | "Objects";

export type EventCriteriaKind = "none" | "int" | "string" | "intOrString";

export type EventBinding = {
  trigger: string;
  callback: string;
  criteria?: string | number;
};

export type EventTypeInfo = {
  trigger: string;
  defaultCallback: string;
  criteria: EventCriteriaKind;
  defaultCriteria?: string;
  validEntities: EventEntityKey[];
  criteriaHint?: string;
};

const allEntities: EventEntityKey[] = ["Rooms", "Mobiles", "Objects"];
const roomEntities: EventEntityKey[] = ["Rooms"];
const mobEntities: EventEntityKey[] = ["Mobiles"];
const objEntities: EventEntityKey[] = ["Objects"];
const placeEntities: EventEntityKey[] = ["Rooms"];

export const eventTypes: EventTypeInfo[] = [
  {
    trigger: "act",
    defaultCallback: "on_act",
    criteria: "string",
    validEntities: allEntities,
    criteriaHint: "Keyword filter"
  },
  {
    trigger: "attacked",
    defaultCallback: "on_attacked",
    criteria: "int",
    defaultCriteria: "100",
    validEntities: mobEntities,
    criteriaHint: "Percent chance"
  },
  {
    trigger: "bribe",
    defaultCallback: "on_bribe",
    criteria: "int",
    defaultCriteria: "0",
    validEntities: mobEntities,
    criteriaHint: "Minimum amount"
  },
  {
    trigger: "death",
    defaultCallback: "on_death",
    criteria: "none",
    validEntities: mobEntities
  },
  {
    trigger: "entry",
    defaultCallback: "on_entry",
    criteria: "int",
    defaultCriteria: "100",
    validEntities: mobEntities,
    criteriaHint: "Percent chance"
  },
  {
    trigger: "fight",
    defaultCallback: "on_fight",
    criteria: "int",
    defaultCriteria: "100",
    validEntities: mobEntities,
    criteriaHint: "Percent chance"
  },
  {
    trigger: "give",
    defaultCallback: "on_give",
    criteria: "intOrString",
    validEntities: mobEntities,
    criteriaHint: "Item vnum or keyword"
  },
  {
    trigger: "greet",
    defaultCallback: "on_greet",
    criteria: "none",
    validEntities: allEntities
  },
  {
    trigger: "grall",
    defaultCallback: "on_greet",
    criteria: "none",
    validEntities: mobEntities
  },
  {
    trigger: "hpcnt",
    defaultCallback: "on_hpcnt",
    criteria: "int",
    defaultCriteria: "100",
    validEntities: mobEntities,
    criteriaHint: "Percent threshold"
  },
  {
    trigger: "random",
    defaultCallback: "on_random",
    criteria: "int",
    defaultCriteria: "100",
    validEntities: mobEntities,
    criteriaHint: "Percent chance"
  },
  {
    trigger: "speech",
    defaultCallback: "on_speech",
    criteria: "string",
    validEntities: mobEntities,
    criteriaHint: "Keyword filter"
  },
  {
    trigger: "exit",
    defaultCallback: "on_exit",
    criteria: "int",
    validEntities: mobEntities,
    criteriaHint: "Direction index"
  },
  {
    trigger: "exall",
    defaultCallback: "on_exit",
    criteria: "int",
    validEntities: mobEntities,
    criteriaHint: "Direction index"
  },
  {
    trigger: "delay",
    defaultCallback: "on_delay",
    criteria: "int",
    defaultCriteria: "1",
    validEntities: mobEntities,
    criteriaHint: "Delay ticks"
  },
  {
    trigger: "surr",
    defaultCallback: "on_surr",
    criteria: "int",
    defaultCriteria: "100",
    validEntities: mobEntities,
    criteriaHint: "Percent chance"
  },
  {
    trigger: "login",
    defaultCallback: "on_login",
    criteria: "none",
    validEntities: roomEntities
  },
  {
    trigger: "given",
    defaultCallback: "on_given",
    criteria: "none",
    validEntities: objEntities
  },
  {
    trigger: "taken",
    defaultCallback: "on_taken",
    criteria: "none",
    validEntities: objEntities
  },
  {
    trigger: "dropped",
    defaultCallback: "on_dropped",
    criteria: "none",
    validEntities: objEntities
  },
  {
    trigger: "prdstart",
    defaultCallback: "on_prdstart",
    criteria: "string",
    validEntities: placeEntities,
    criteriaHint: "Period name"
  },
  {
    trigger: "prdstop",
    defaultCallback: "on_prdstop",
    criteria: "string",
    validEntities: placeEntities,
    criteriaHint: "Period name"
  }
];

export const eventTypeByTrigger = new Map(
  eventTypes.map((info) => [info.trigger, info])
);

export function getEventTypesForEntity(
  entity: EventEntityKey | null
): EventTypeInfo[] {
  if (!entity) {
    return [];
  }
  return eventTypes.filter((info) => info.validEntities.includes(entity));
}
