# Troubleshooting

## "libEGL warning: failed to open /dev/dri/renderD128: Permission denied"
This warning is common on systems without GPU permissions. It typically does not affect WorldEdit.

## No widgets appear (blank gradient)
This usually indicates a runtime error in the UI. Check the dev console for errors and fix the referenced file/line.

## Permission errors for editor metadata
WorldEdit writes to `area/.worldedit/`. Ensure the folder exists and that Tauri has permission for:
- read/write text file
- create directory
