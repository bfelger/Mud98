export type AreaJson = Record<string, unknown>;

export type EditorMeta = {
  version: number;
  updatedAt: string;
  view: {
    activeTab: string;
  };
  selection: {
    entityType: string;
  };
  layout?: EditorLayout;
};

export type ReferenceData = {
  classes: string[];
  races: string[];
  skills: string[];
  groups: string[];
  commands: string[];
  socials: string[];
  tutorials: string[];
  sourceDir: string;
};

export type ClassDefinition = {
  name: string;
  whoName?: string;
  baseGroup?: string;
  defaultGroup?: string;
  weaponVnum?: number;
  armorProf?: string;
  guilds?: number[];
  primeStat?: string;
  skillCap?: number;
  thac0_00?: number;
  thac0_32?: number;
  hpMin?: number;
  hpMax?: number;
  manaUser?: boolean;
  startLoc?: number;
  titles?: string[][];
};

export type ClassDataFile = {
  formatVersion: number;
  classes: ClassDefinition[];
};

export type ClassDataSource = {
  path: string;
  format: "json" | "olc";
  data: ClassDataFile;
};

export type RaceStats = {
  str?: number;
  int?: number;
  wis?: number;
  dex?: number;
  con?: number;
};

export type RaceDefinition = {
  name: string;
  whoName?: string;
  pc?: boolean;
  points?: number;
  size?: string;
  stats?: RaceStats;
  maxStats?: RaceStats;
  actFlags?: string[];
  affectFlags?: string[];
  offFlags?: string[];
  immFlags?: string[];
  resFlags?: string[];
  vulnFlags?: string[];
  formFlags?: string[];
  partFlags?: string[];
  classMult?: Record<string, number> | number[];
  startLoc?: number;
  classStart?: Record<string, number> | number[];
  skills?: string[];
};

export type RaceDataFile = {
  formatVersion: number;
  races: RaceDefinition[];
};

export type RaceDataSource = {
  path: string;
  format: "json" | "olc";
  data: RaceDataFile;
};

export type SkillDefinition = {
  name: string;
  levels?: Record<string, number> | number[];
  ratings?: Record<string, number> | number[];
  spell?: string;
  loxSpell?: string;
  target?: string;
  minPosition?: string;
  gsn?: string;
  slot?: number;
  minMana?: number;
  beats?: number;
  nounDamage?: string;
  msgOff?: string;
  msgObj?: string;
};

export type SkillDataFile = {
  formatVersion: number;
  skills: SkillDefinition[];
};

export type SkillDataSource = {
  path: string;
  format: "json" | "olc";
  data: SkillDataFile;
};

export type GroupDefinition = {
  name: string;
  ratings?: Record<string, number> | number[];
  skills?: string[];
};

export type GroupDataFile = {
  formatVersion: number;
  groups: GroupDefinition[];
};

export type GroupDataSource = {
  path: string;
  format: "json" | "olc";
  data: GroupDataFile;
};

export type CommandDefinition = {
  name: string;
  function?: string;
  position?: string;
  level?: number;
  log?: string;
  category?: string;
  loxFunction?: string;
};

export type CommandDataFile = {
  formatVersion: number;
  commands: CommandDefinition[];
};

export type CommandDataSource = {
  path: string;
  format: "json" | "olc";
  data: CommandDataFile;
};

export type SocialDefinition = {
  name: string;
  charNoArg?: string;
  othersNoArg?: string;
  charFound?: string;
  othersFound?: string;
  victFound?: string;
  charAuto?: string;
  othersAuto?: string;
};

export type SocialDataFile = {
  formatVersion: number;
  socials: SocialDefinition[];
};

export type SocialDataSource = {
  path: string;
  format: "json" | "olc";
  data: SocialDataFile;
};

export type ProjectDataFiles = {
  classes: string;
  races: string;
  skills: string;
  groups: string;
  commands: string;
  socials: string;
  tutorials: string;
  themes: string;
  loot: string;
  lox: string;
};

export type ProjectConfig = {
  path: string;
  rootDir: string;
  areaDir: string | null;
  areaList: string;
  dataDir: string | null;
  defaultFormat: "json" | "olc";
  dataFiles: ProjectDataFiles;
};

export type RoomLayoutEntry = {
  x: number;
  y: number;
  locked?: boolean;
};

export type EditorLayout = {
  rooms?: Record<string, RoomLayoutEntry>;
};

export type AreaIndexEntry = {
  fileName: string;
  name: string;
  vnumRange: [number, number] | null;
};

export type AreaExitDirection = "north" | "east" | "south" | "west" | "up" | "down";

export type AreaGraphLink = {
  fromFile: string;
  toFile: string;
  count: number;
  directionCounts: Partial<Record<AreaExitDirection, number>>;
};
