# Persistence modules

- `persist_io.h` / `persist_result.h` hold shared stream abstractions and status/result helpers usable across persisted domains (areas, classes, commands, etc.).
- `persist_io_adapters.h/.c` provide common reader/writer adapters for FILE*, in-memory readers, and in-memory writers.
- `area_persist.h` binds those primitives to area-specific params; other domains can add their own `*_persist.h` without duplicating the core types.
- Format implementations live in subfolders (e.g., `rom-olc/` for the current text `.are` grammar, `json/` stub backed by jansson when enabled).
- Callers own path resolution, temp/rename handling, and format selection; backends focus on parsing/serializing via the provided stream ops.
