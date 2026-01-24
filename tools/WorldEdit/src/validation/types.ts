import type { AreaIndexEntry, AreaJson } from "../repository/types";

export type ValidationSeverity = "error" | "warning";

export type ValidationSource = "core" | "plugin";

export type ValidationIssueBase = {
  id: string;
  severity: ValidationSeverity;
  entityType: string;
  message: string;
  vnum?: number;
  resetIndex?: number;
};

export type ValidationIssue = ValidationIssueBase & {
  ruleId: string;
  source: ValidationSource;
};

export type ValidationContext = {
  areaData: AreaJson | null;
  areaIndex: AreaIndexEntry[];
};

export type ValidationRule = {
  id: string;
  label: string;
  description?: string;
  source: ValidationSource;
  run: (context: ValidationContext) => ValidationIssueBase[];
};

export type ValidationConfig = {
  disabledRuleIds?: string[];
};
