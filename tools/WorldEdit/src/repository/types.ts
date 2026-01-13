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
  layout?: Record<string, unknown>;
};

export type ReferenceData = {
  classes: string[];
  races: string[];
  skills: string[];
  commands: string[];
  sourceDir: string;
};
