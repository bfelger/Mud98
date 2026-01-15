type MapToolbarProps = {
  autoLayoutEnabled: boolean;
  preferCardinalLayout: boolean;
  showVerticalEdges: boolean;
  dirtyRoomCount: number;
  selectedRoomNode: boolean;
  selectedRoomLocked: boolean;
  hasRoomLayout: boolean;
  onRelayout: () => void;
  onToggleAutoLayout: (value: boolean) => void;
  onTogglePreferGrid: (value: boolean) => void;
  onToggleVerticalEdges: (value: boolean) => void;
  onLockSelected: () => void;
  onUnlockSelected: () => void;
  onClearLayout: () => void;
};

export function MapToolbar({
  autoLayoutEnabled,
  preferCardinalLayout,
  showVerticalEdges,
  dirtyRoomCount,
  selectedRoomNode,
  selectedRoomLocked,
  hasRoomLayout,
  onRelayout,
  onToggleAutoLayout,
  onTogglePreferGrid,
  onToggleVerticalEdges,
  onLockSelected,
  onUnlockSelected,
  onClearLayout
}: MapToolbarProps) {
  return (
    <div className="map-toolbar">
      <button className="action-button" type="button" onClick={onRelayout}>
        Re-layout
      </button>
      <label className="map-toggle">
        <input
          type="checkbox"
          checked={autoLayoutEnabled}
          onChange={(event) => onToggleAutoLayout(event.target.checked)}
        />
        <span>Auto layout</span>
      </label>
      <label className="map-toggle">
        <input
          type="checkbox"
          checked={preferCardinalLayout}
          onChange={(event) => onTogglePreferGrid(event.target.checked)}
        />
        <span>Prefer grid layout</span>
      </label>
      <label className="map-toggle">
        <input
          type="checkbox"
          checked={showVerticalEdges}
          onChange={(event) => onToggleVerticalEdges(event.target.checked)}
        />
        <span>Vertical edges</span>
      </label>
      {dirtyRoomCount ? (
        <span
          className="map-dirty"
          title="Moved rooms are not locked or saved to meta yet."
        >
          Unpinned: {dirtyRoomCount}
        </span>
      ) : null}
      <button
        className="action-button"
        type="button"
        onClick={onLockSelected}
        disabled={!selectedRoomNode || selectedRoomLocked}
      >
        Lock selected
      </button>
      <button
        className="action-button"
        type="button"
        onClick={onUnlockSelected}
        disabled={!selectedRoomNode || !selectedRoomLocked}
      >
        Unlock selected
      </button>
      <button
        className="action-button"
        type="button"
        onClick={onClearLayout}
        disabled={!hasRoomLayout}
      >
        Clear layout
      </button>
    </div>
  );
}
