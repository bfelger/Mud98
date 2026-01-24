import type { ValidationRule } from "./types";

type ValidationModule = {
  rule?: ValidationRule;
  rules?: ValidationRule[];
  default?: ValidationRule | ValidationRule[];
};

const modules = import.meta.glob<ValidationModule>("./plugins/*.ts", {
  eager: true
});

export function loadPluginRules(): ValidationRule[] {
  const rules: ValidationRule[] = [];

  for (const module of Object.values(modules)) {
    if (!module) {
      continue;
    }
    if (module.rule) {
      rules.push(module.rule);
    }
    if (Array.isArray(module.rules)) {
      rules.push(...module.rules);
    }
    if (module.default) {
      if (Array.isArray(module.default)) {
        rules.push(...module.default);
      } else {
        rules.push(module.default);
      }
    }
  }

  return rules;
}
