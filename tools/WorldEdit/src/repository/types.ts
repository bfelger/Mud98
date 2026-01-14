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
  commands: string[];
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
