# Mud98 Engineering Notes

Quick pointers to keep changes safe and easy to implement.

## Build & Test Quickly
  - Configure with `cmake --preset ninja-multi`; build with `cmake --build --preset debug`.
  - Binaries land in `bin/<Config>/Mud98`; run from repo root so `mud98.cfg`, `area/`, and `data/` resolve.
  - `./src/run unittest` runs unit tests (`-U` flag); `./src/run bench` runs benchmarks.

## Highly-Sensitivity Areas
  - Networking (`comm.*`, `mth/*`, TLS setup) and save/load paths are easy to break; mirror existing patterns.
  - Player/area persistence and protocol messages must stay backward compatible with published data unless explicitly migrating.
- **When adding features**
  - Update `src/CMakeLists.txt` with new sources.
  - Add/adjust docs in `doc/mud98/`; keep a brief note of intent and invariants.
  - Prefer small, testable, but _comprehensive_ changes, seeking 100% code coverage.
    - Add unit tests to cover behavioral changes.

## Configuration
  - Default runtime config is `mud98.cfg` in the repo root; keep new settings documented there.
  - TLS keys live under `keys/`; for local dev, see `keys/ssl_gen_keys`.

## AI Usage
In order to work effectively and not produce hallucinogenic results, provide generative coding assistants with immediate "context" by pointing them at documentation.
  - Point assistants at `doc/mud98/project-map.md`, `coding-guide.md`, and `glossary.md` for quick context.
  - Mention the build preset and target config you’re using (`ninja-multi`, `Debug/Release`) to avoid confusion about paths.
