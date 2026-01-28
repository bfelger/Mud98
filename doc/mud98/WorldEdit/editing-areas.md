# Editing areas

## General flow
1. Load a project config.
2. Load an area file.
4. Use the Table tab to browse entities.
5. Use the Form tab to edit the selected entity.

## Room creation
- Use the Rooms table toolbar "New" button.
- WorldEdit picks the next available VNUM in the area range.
- New rooms default their sector type to the Area's sector.

### Map-Based Creation
- Right-click Rooms in the Map view to quickly build out cardinally-adjacent rooms in with two-way links.

## Resets
- Resets can be edited either from the Resets table or directly on a Room form.
- The Room form shows resets for that room and allows inline add/delete/edit.

## Field hints and VNUM context
Where possible, VNUM fields show type/name hints. This reduces "magic number" mistakes and helps validation.

## Saving
- Save: writes the area JSON.
- Save Meta: writes layout/editor metadata to `area/.worldedit/`.
- Save As: writes a new area file.

