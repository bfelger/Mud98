type MapOverlayProps = {
  areaVnumRange: string | null;
};

export function MapOverlay({ areaVnumRange }: MapOverlayProps) {
  if (!areaVnumRange) {
    return null;
  }

  return <div className="map-overlay">VNUM range: {areaVnumRange}</div>;
}
