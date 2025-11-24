# Mud98 Coding Guide

Compact house rules to keep changes consistent. The codebase targets modern C (GNU2x/Clang C2x; MSVC uses `/std:c17`).

- **Warnings & style**
  - Treat warnings as errors; keep code warning-free under GCC/Clang/MSVC.
  - Prefer small, focused functions; keep file-level `static` where possible.
  - Use `const` for inputs, `bool` for flags, and size-appropriate unsigned types for counts/lengths.
  - Include headers with project-relative paths: `#include "entities/mobile.h"`.
- **Error handling**
  - Check allocation and file I/O results; return early on failure.
  - Use existing logging/notify patterns in the touched module; avoid `printf`-style debugging in committed code.
- **Threading & I/O**
  - Networking is multithreaded on non-Windows via Pthreads; guard shared data (see existing patterns in `comm.*` and `entities/*`).
  - Keep socket and telnet/TLS handling consistent with existing helpers; avoid ad-hoc blocking calls.
- **Data & serialization**
  - Follow existing table/save/load helpers for areas, config, and player data; align new records with the current file formats.
- **Tests & benches**
  - Unit tests live under `src/tests/`; add focused cases when changing behavior.
  - Benchmarks live under `src/benchmarks/`; keep them deterministic.
- **CMake**
  - Add new source files to `src/CMakeLists.txt` executable list.
  - Keep runtime outputs in `bin/<Config>`; avoid introducing system-wide install requirements.

When in doubt, mirror the closest existing code in the subsystem you’re touching.
