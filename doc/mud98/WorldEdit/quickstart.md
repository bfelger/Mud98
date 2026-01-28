# Quickstart

This guide assumes you are running WorldEdit locally from the Mud98 repo.

## Prerequisites
- Node.js + npm
- Rust toolchain (for Tauri)
- OS-specific Tauri dependencies (see the official Tauri prerequisites for your platform)

## Run (dev)
From the repo root:

```sh
cd tools/WorldEdit
npm install
npm run tauri dev
```

## Build (release)
```sh
cd tools/WorldEdit
npm install
npm run build
npm run tauri build
```

## First launch
1. Click "Open Config" and select a `mud98.cfg` (for example `mud98.cfg` or `new/mud98.cfg`).
2. WorldEdit automatically loads the area directory and global data files from that config.
3. Use the Entity Tree to select Areas/Rooms/etc.

## Notes
- If you are working in a new sandbox, create a copy of your area/data files first.
- WorldEdit stores map layout and editor metadata separately from area JSON.

