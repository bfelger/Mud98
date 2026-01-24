import type {
  ValidationConfig,
  ValidationContext,
  ValidationIssue,
  ValidationRule
} from "./types";

// LocalStorage contains a JSON array of disabled rule IDs.
export const VALIDATION_CONFIG_KEY = "worldedit.validation.disabledRules";

export function loadValidationConfig(): ValidationConfig {
  if (typeof localStorage === "undefined") {
    return {};
  }
  const raw = localStorage.getItem(VALIDATION_CONFIG_KEY);
  if (!raw) {
    return {};
  }
  try {
    const parsed = JSON.parse(raw);
    if (Array.isArray(parsed)) {
      return {
        disabledRuleIds: parsed.map((entry) => String(entry))
      };
    }
  } catch {
    return {};
  }
  return {};
}

export function buildValidationIssues(
  rules: ValidationRule[],
  context: ValidationContext,
  config: ValidationConfig
): ValidationIssue[] {
  const disabled = new Set(config.disabledRuleIds ?? []);
  const issues: ValidationIssue[] = [];

  for (const rule of rules) {
    if (disabled.has(rule.id)) {
      continue;
    }
    const nextIssues = rule.run(context);
    for (const issue of nextIssues) {
      issues.push({
        ...issue,
        ruleId: rule.id,
        source: rule.source
      });
    }
  }

  return issues;
}
