#!/usr/bin/env node
import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";

const scriptDir = path.dirname(fileURLToPath(import.meta.url));
const repoRoot = path.resolve(scriptDir, "..", "..", "..");

const read = (relPath) =>
  fs.readFileSync(path.join(repoRoot, relPath), "utf8");

const enumsSource = read("tools/WorldEdit/src/schemas/enums.ts");

const extractBlock = (source, regex, label) => {
  const match = source.match(regex);
  if (!match) {
    throw new Error(`Could not find ${label}.`);
  }
  return match[1];
};

const stripCComments = (source) =>
  source
    .replace(/\/\*[\s\S]*?\*\//g, "")
    .replace(/\/\/.*$/gm, "");

const parseTsArray = (name, stack = []) => {
  if (stack.includes(name)) {
    throw new Error(`Circular enum reference: ${[...stack, name].join(" -> ")}`);
  }
  const block = extractBlock(
    enumsSource,
    new RegExp(
      `export\\s+const\\s+${name}\\s*=\\s*\\[([\\s\\S]*?)\\]\\s*as\\s+const;`
    ),
    `TypeScript enum array "${name}"`
  );
  const values = [];
  const itemRegex = /(\.\.\.\s*[A-Za-z_][A-Za-z0-9_]*)|"([^"]*)"/g;
  let match = null;
  while ((match = itemRegex.exec(block))) {
    if (match[2] !== undefined) {
      values.push(match[2]);
    } else if (match[1]) {
      const refName = match[1].replace(/\.\.\.\s*/, "");
      values.push(...parseTsArray(refName, [...stack, name]));
    }
  }
  return values;
};

const parseFlagTable = (source, tableName) => {
  const block = extractBlock(
    source,
    new RegExp(
      `const\\s+struct\\s+flag_type\\s+${tableName}\\s*(?:\\[[^\\]]*\\])?\\s*=\\s*\\{([\\s\\S]*?)\\};`
    ),
    `flag table "${tableName}"`
  );
  const values = [];
  const itemRegex = /\{\s*"([^"]+)"\s*,/g;
  let match = null;
  while ((match = itemRegex.exec(block))) {
    values.push(match[1]);
  }
  return values;
};

const parseAttackTable = (source) => {
  const block = extractBlock(
    source,
    /const\s+AttackInfo\s+attack_table\s*\[[^\]]*\]\s*=\s*\{([\s\S]*?)\};/,
    "attack_table"
  );
  const values = [];
  const itemRegex = /\{\s*"([^"]+)"\s*,/g;
  let match = null;
  while ((match = itemRegex.exec(block))) {
    values.push(match[1]);
  }
  return values;
};

const parseLiquidTable = (source) => {
  const block = extractBlock(
    source,
    /const\s+LiquidInfo\s+liquid_table\s*\[[^\]]*\]\s*=\s*\{([\s\S]*?)\};/,
    "liquid_table"
  );
  const values = [];
  const itemRegex = /\{\s*"([^"]+)"\s*,/g;
  let match = null;
  while ((match = itemRegex.exec(block))) {
    values.push(match[1]);
  }
  return values;
};

const parsePositionTable = (source) => {
  const block = extractBlock(
    source,
    /const\s+PositionInfo\s+position_table\s*\[[^\]]*\]\s*=\s*\{([\s\S]*?)\};/,
    "position_table"
  );
  const values = [];
  const itemRegex = /\{\s*POS_[^,]+,\s*"([^"]+)"/g;
  let match = null;
  while ((match = itemRegex.exec(block))) {
    values.push(match[1]);
  }
  return values;
};

const parseSexTable = (source) => {
  const block = extractBlock(
    source,
    /const\s+SexInfo\s+sex_table\s*\[[^\]]*\]\s*=\s*\{([\s\S]*?)\};/,
    "sex_table"
  );
  const values = [];
  const itemRegex = /\{\s*SEX_[^,]+,\s*"([^"]+)"/g;
  let match = null;
  while ((match = itemRegex.exec(block))) {
    values.push(match[1]);
  }
  return values;
};

const parseSizeTable = (source) => {
  const block = extractBlock(
    source,
    /const\s+MobSizeInfo\s+mob_size_table\s*\[[^\]]*\]\s*=\s*\{([\s\S]*?)\};/,
    "mob_size_table"
  );
  const values = [];
  const itemRegex = /\{\s*SIZE_[^,]+,\s*"([^"]+)"/g;
  let match = null;
  while ((match = itemRegex.exec(block))) {
    values.push(match[1]);
  }
  return values;
};

const parseDirList = (source) => {
  const block = extractBlock(
    source,
    /const\s+DirInfo\s+dir_list\s*\[[^\]]*\]\s*=\s*\{([\s\S]*?)\};/,
    "dir_list"
  );
  const values = [];
  const itemRegex = /\{\s*DIR_[^,]+,\s*DIR_[^,]+,\s*"([^"]+)"/g;
  let match = null;
  while ((match = itemRegex.exec(block))) {
    values.push(match[1]);
  }
  return values;
};

const parseEventTriggers = (source) => {
  const block = extractBlock(
    source,
    /const\s+EventTypeInfo\s+event_type_info_table\s*\[[^\]]*\]\s*=\s*\{([\s\S]*?)\};/,
    "event_type_info_table"
  );
  const values = [];
  const itemRegex = /\{\s*TRIG_[^,]+,\s*"([^"]+)"/g;
  let match = null;
  while ((match = itemRegex.exec(block))) {
    values.push(match[1]);
  }
  return values;
};

const tablesSource = stripCComments(read("src/tables.c"));
const mobileSource = stripCComments(read("src/data/mobile_data.c"));
const damageSource = stripCComments(read("src/data/damage.c"));
const itemSource = stripCComments(read("src/data/item.c"));
const directionSource = stripCComments(read("src/data/direction.c"));
const eventsSource = stripCComments(read("src/data/events.c"));

const expected = {
  sectors: parseFlagTable(tablesSource, "sector_flag_table"),
  roomFlags: parseFlagTable(tablesSource, "room_flag_table"),
  exitFlags: parseFlagTable(tablesSource, "exit_flag_table"),
  directions: parseDirList(directionSource),
  wearLocations: parseFlagTable(tablesSource, "wear_loc_strings"),
  wearFlags: parseFlagTable(tablesSource, "wear_flag_table"),
  extraFlags: parseFlagTable(tablesSource, "extra_flag_table"),
  weaponClasses: parseFlagTable(tablesSource, "weapon_class"),
  weaponFlags: parseFlagTable(tablesSource, "weapon_type2"),
  containerFlags: parseFlagTable(tablesSource, "container_flag_table"),
  portalFlags: parseFlagTable(tablesSource, "portal_flag_table"),
  furnitureFlags: parseFlagTable(tablesSource, "furniture_flag_table"),
  liquids: parseLiquidTable(itemSource),
  positions: parsePositionTable(mobileSource),
  sizes: parseSizeTable(mobileSource),
  sexes: parseSexTable(mobileSource),
  damageTypes: parseAttackTable(damageSource),
  actFlags: parseFlagTable(tablesSource, "act_flag_table"),
  affectFlags: parseFlagTable(tablesSource, "affect_flag_table"),
  offFlags: parseFlagTable(tablesSource, "off_flag_table"),
  immFlags: parseFlagTable(tablesSource, "imm_flag_table"),
  resFlags: parseFlagTable(tablesSource, "res_flag_table"),
  vulnFlags: parseFlagTable(tablesSource, "vuln_flag_table"),
  formFlags: [
    ...parseFlagTable(tablesSource, "form_flag_table"),
    ...parseFlagTable(tablesSource, "form_defaults_flag_table")
  ],
  partFlags: [
    ...parseFlagTable(tablesSource, "part_flag_table"),
    ...parseFlagTable(tablesSource, "part_defaults_flag_table")
  ],
  applyLocations: parseFlagTable(tablesSource, "apply_flag_table"),
  eventTriggers: parseEventTriggers(eventsSource)
};

const arraysEqual = (left, right) =>
  left.length === right.length && left.every((val, idx) => val === right[idx]);

const report = [];

for (const [name, expectedValues] of Object.entries(expected)) {
  const actualValues = parseTsArray(name);
  const expectedSet = new Set(expectedValues);
  const actualSet = new Set(actualValues);
  const missing = expectedValues.filter((val) => !actualSet.has(val));
  const extra = actualValues.filter((val) => !expectedSet.has(val));
  const orderMismatch = missing.length === 0 && extra.length === 0
    ? !arraysEqual(actualValues, expectedValues)
    : false;

  if (missing.length || extra.length || orderMismatch) {
    report.push({ name, missing, extra, orderMismatch });
  }
}

if (!report.length) {
  console.log("Enum check OK: enums.ts matches C tables.");
  process.exit(0);
}

console.log("Enum check failed:");
for (const entry of report) {
  console.log(`- ${entry.name}`);
  if (entry.missing.length) {
    console.log(`  missing in enums.ts: ${entry.missing.join(", ")}`);
  }
  if (entry.extra.length) {
    console.log(`  extra in enums.ts: ${entry.extra.join(", ")}`);
  }
  if (entry.orderMismatch) {
    console.log("  order differs from C tables.");
  }
}

process.exit(1);
