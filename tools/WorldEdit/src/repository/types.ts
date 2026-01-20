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
