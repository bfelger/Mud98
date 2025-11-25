# Legacy rom-olc backend

- Implements the current text `.are` format using the `AreaPersistFormat` API (selected by default by `area_persist_select_format` unless a `.json` extension is seen and JSON is enabled).
- Supports FILE-backed readers/writers (direct) and buffer-backed readers/writers (staged through tmpfile) via `persist_io_adapters`.
- Load/save are bridged to the legacy section parsers and writers used by `boot_db`/`save_area`, preserving globals and single-instance behavior.
- Loader guard converts fatal loader exits into `PERSIST_ERR_FORMAT` for persistence callers.
- Optional exhaustive test (guarded by `PERSIST_EXHAUSTIVE_PERSIST_TEST`) round-trips every area in `area.lst` to verify deterministic output.
