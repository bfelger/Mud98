# Project files and layout

## Project file (mud98.cfg)
WorldEdit uses `mud98.cfg` as the project file. It contains:
- Area directory
- Data directory
- Individual data file names (classes, races, skills, commands, etc.)

Opening a config loads both area and data in one step.

## Area JSON vs editor meta
WorldEdit keeps layout/editor metadata separate from area JSON:

- Area JSON: `area/*.json`
- Editor meta: `area/.worldedit/*.editor.json`

This keeps gameplay data clean while still preserving layout, map locks, and view state.

## Global data
Global data is loaded from the data folder described in the config file. These files back dropdowns and validation:
- Classes
- Races
- Skills
- Groups
- Commands
- Socials
- Tutorials
- Loot tables/groups

