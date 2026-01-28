# Maps

WorldEdit provides two map views:

## Area Map
- Graph view of rooms in the selected area.
- Orthogonal layout with cardinal anchors.
- Nodes are square and labels wrap.
- Supports manual dragging and lock/save metadata for persistent placement.

## World Map
- Graph view of all areas based on inter-area exits.
- Uses the same layout pipeline as Area Map for consistency.
- Area nodes can be locked and saved in editor metadata.

## Map metadata
- Layout and locks are stored in `area/.worldedit/*.editor.json`.
- Metadata is saved per area and does not affect gameplay data.

