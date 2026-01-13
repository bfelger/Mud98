function sortJsonValue(value: unknown): unknown {
  if (Array.isArray(value)) {
    return value.map(sortJsonValue);
  }

  if (value && typeof value === "object") {
    const entries = Object.entries(value as Record<string, unknown>).sort(
      ([left], [right]) => left.localeCompare(right)
    );
    const sorted: Record<string, unknown> = {};
    for (const [key, entryValue] of entries) {
      sorted[key] = sortJsonValue(entryValue);
    }
    return sorted;
  }

  return value;
}

export function canonicalStringify(value: unknown): string {
  return JSON.stringify(sortJsonValue(value), null, 4);
}
