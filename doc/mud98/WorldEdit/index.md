# WorldEdit (Builder Guide)

WorldEdit is Mud98's desktop editor for area JSON and global data. It is intended for builders and content authors.

## Start here
- [Quickstart](quickstart.md)
- [Project files and layout](project-files.md)
- [Editing areas](editing-areas.md)
- [Maps (Room + World)](maps.md)
- [Troubleshooting](troubleshooting.md)

## What WorldEdit edits
- Areas: rooms, exits, mobiles, objects, resets, shops, factions, quests, recipes, loot, gather spawns, specials.
- Global data: classes, races, skills, groups, commands, socials, tutorials, loot tables/groups.

## Key concepts
- Project file: select `mud98.cfg` to load both area and data folders in one step.
- Editor meta: map/layout data is saved into `area/.worldedit/*.editor.json` and is separate from area JSON.
- Reference data: global data loads automatically when you open a project so fields like Race can be dropdowns.

## Where this fits in the workflow
WorldEdit is designed to sit alongside in-game OLC. It is ideal for bulk edits, map layout, and global data maintenance.

