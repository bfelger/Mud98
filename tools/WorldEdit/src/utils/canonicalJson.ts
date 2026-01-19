type OrderMap = Record<string, string[]>;

const ROOT_ORDER = [
  "formatVersion",
  "areadata",
  "storyBeats",
  "checklist",
  "daycycle",
  "rooms",
  "mobiles",
  "objects",
  "shops",
  "specials",
  "mobprogs",
  "resets",
  "factions",
  "helps",
  "quests",
  "loot",
  "recipes",
  "gatherSpawns"
];

const ORDER_MAP: OrderMap = {
  areadata: [
    "version",
    "name",
    "builders",
    "vnumRange",
    "credits",
    "security",
    "sector",
    "lowLevel",
    "highLevel",
    "reset",
    "alwaysReset",
    "instType",
    "lootTable"
  ],
  storyBeats: ["title", "description"],
  checklist: ["title", "description", "status", "statusValue"],
  daycycle: ["suppressDaycycleMessages", "periods"],
  periods: ["name", "fromHour", "toHour", "description", "enterMessage", "exitMessage"],
  timePeriods: [
    "name",
    "fromHour",
    "toHour",
    "description",
    "enterMessage",
    "exitMessage"
  ],
  rooms: [
    "vnum",
    "name",
    "description",
    "roomFlags",
    "roomFlagsValue",
    "sectorType",
    "suppressDaycycleMessages",
    "timePeriods",
    "exits",
    "extraDescs",
    "loxScript",
    "events"
  ],
  exits: ["dir", "toVnum", "key", "flags", "description", "keyword"],
  extraDescs: ["keyword", "description"],
  events: ["trigger", "triggerValue", "callback", "criteria"],
  mobiles: [
    "vnum",
    "name",
    "shortDescr",
    "longDescr",
    "description",
    "race",
    "actFlags",
    "actFlagsValue",
    "affectFlags",
    "atkFlags",
    "immFlags",
    "resFlags",
    "vulnFlags",
    "formFlags",
    "partFlags",
    "alignment",
    "group",
    "level",
    "hitroll",
    "hitDice",
    "manaDice",
    "damageDice",
    "damType",
    "ac",
    "startPos",
    "defaultPos",
    "sex",
    "wealth",
    "size",
    "material",
    "factionVnum",
    "lootTable",
    "craftMats",
    "loxScript",
    "events"
  ],
  hitDice: ["number", "type", "bonus"],
  manaDice: ["number", "type", "bonus"],
  damageDice: ["number", "type", "bonus"],
  ac: ["pierce", "bash", "slash", "exotic"],
  objects: [
    "vnum",
    "name",
    "shortDescr",
    "description",
    "material",
    "itemType",
    "extraFlags",
    "wearFlags",
    "weapon",
    "container",
    "light",
    "armor",
    "drink",
    "fountain",
    "food",
    "money",
    "wand",
    "staff",
    "spells",
    "portal",
    "furniture",
    "values",
    "level",
    "weight",
    "cost",
    "condition",
    "extraDescs",
    "affects",
    "salvageMats",
    "loxScript",
    "events"
  ],
  weapon: ["class", "dice", "damageType", "flags"],
  container: ["capacity", "flags", "keyVnum", "maxWeight", "weightMult"],
  light: ["hours"],
  armor: ["acPierce", "acBash", "acSlash", "acExotic", "armorType"],
  drink: ["capacity", "remaining", "liquid", "poisoned"],
  fountain: ["capacity", "remaining", "liquid"],
  food: ["foodHours", "fullHours", "poisoned"],
  money: ["copper", "silver", "gold"],
  wand: ["level", "chargesTotal", "chargesRemaining", "spell"],
  staff: ["level", "chargesTotal", "chargesRemaining", "spell"],
  spells: ["level", "spells"],
  portal: ["charges", "exitFlags", "portalFlags", "toVnum", "keyVnum"],
  furniture: ["maxPeople", "maxWeight", "flags", "healBonus", "manaBonus"],
  affects: [
    "type",
    "where",
    "location",
    "level",
    "duration",
    "modifier",
    "bitvector",
    "bitvectorValue"
  ],
  shops: ["keeper", "buyTypes", "profitBuy", "profitSell", "openHour", "closeHour"],
  specials: ["mobVnum", "spec"],
  mobprogs: ["vnum", "code"],
  factions: ["vnum", "name", "defaultStanding", "allies", "opposing"],
  helps: ["level", "keyword", "text"],
  quests: [
    "vnum",
    "name",
    "entry",
    "type",
    "xp",
    "level",
    "end",
    "target",
    "upper",
    "count",
    "rewardFaction",
    "rewardReputation",
    "rewardGold",
    "rewardSilver",
    "rewardCopper",
    "rewardObjs",
    "rewardCounts"
  ],
  gatherSpawns: ["spawnSector", "vnum", "quantity", "respawnTimer"]
};

const RESET_ORDER_BY_COMMAND: Record<string, string[]> = {
  loadMob: ["mobVnum", "maxInArea", "roomVnum", "maxInRoom", "commandName"],
  placeObj: ["objVnum", "roomVnum", "commandName"],
  putObj: [
    "objVnum",
    "count",
    "containerVnum",
    "maxInContainer",
    "roomVnum",
    "commandName"
  ],
  giveObj: ["objVnum", "roomVnum", "commandName"],
  equipObj: ["objVnum", "wearLoc", "roomVnum", "commandName"],
  setDoor: ["roomVnum", "direction", "state", "commandName"],
  randomizeExits: ["roomVnum", "exits", "commandName"]
};

function getResetOrder(value: Record<string, unknown>): string[] {
  const commandName =
    typeof value.commandName === "string" ? value.commandName.trim() : "";
  if (commandName && RESET_ORDER_BY_COMMAND[commandName]) {
    return RESET_ORDER_BY_COMMAND[commandName];
  }
  return ["roomVnum", "command", "arg1", "arg2", "arg3", "arg4", "commandName"];
}

function getOrderForKey(
  key: string | null,
  value: Record<string, unknown>
): string[] | null {
  if (!key) {
    return ROOT_ORDER;
  }
  if (key === "resets") {
    return getResetOrder(value);
  }
  return ORDER_MAP[key] ?? null;
}

function sortJsonValue(value: unknown, contextKey: string | null): unknown {
  if (Array.isArray(value)) {
    return value.map((entry) => sortJsonValue(entry, contextKey));
  }

  if (value && typeof value === "object") {
    const record = value as Record<string, unknown>;
    const order = getOrderForKey(contextKey, record);
    const keys = Object.keys(record);
    const orderedKeys = order ? order.filter((key) => key in record) : [];
    const seen = new Set(orderedKeys);
    const remainder = keys.filter((key) => !seen.has(key));
    const nextKeys = order ? [...orderedKeys, ...remainder] : keys;
    const sorted: Record<string, unknown> = {};
    for (const key of nextKeys) {
      sorted[key] = sortJsonValue(record[key], key);
    }
    return sorted;
  }

  return value;
}

export function canonicalStringify(value: unknown): string {
  return JSON.stringify(sortJsonValue(value, null), null, 2);
}
