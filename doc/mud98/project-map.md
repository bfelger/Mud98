# Mud98 Project Map

This is the quick orientation for the repo when opened at the root (`Mud98/`).

- **Build & configure**
  - CMake presets live at the repo root. Typical flow: `cmake --preset ninja-multi`, then `cmake --build --preset debug` (or `release`).
  - Shell scripts work from root: `./config` (configure), `./compile` (build).
- **Run**
  - Binaries land in `bin/<Config>/Mud98`. Launch from the repo root so `mud98.cfg`, `area/`, and `data/` resolve.
  - Helper: `./run [debug|release|relwithdebinfo] [test|bench]`.
- **Tests/benchmarks**
  - Tests are in a separate `Mud98Tests` executable (no longer baked into main binary).
  - `./run test` launches the test executable.
  - `ctest --output-on-failure` from the build directory uses CTest integration.
  - Benchmarks are in a separate `Mud98Benchmarks` executable.
  - `./run bench` launches benchmarks.
- **Key directories**
  - `src/` — source and CMakeLists.txt.
  - `bin/` — build outputs (per-config).
  - `area/` — game areas.
  - `data/` — data tables.
  - `doc/mud98/` — project-specific docs.
  - `keys/` — TLS materials (see `doc/mud98/wb-01-getting-started.md`).
  - `mud98.cfg` — runtime configuration (read when running from repo root).

See `doc/mud98/coding-guide.md` for coding standards and `doc/mud98/glossary.md` for domain terms. Add other guides here as they grow.
