type ExternalExit = {
  fromVnum: number;
  fromName: string;
  direction: string;
  toVnum: number;
  areaName: string | null;
};

type MapExternalExitsPanelProps = {
  externalExits: ExternalExit[];
  onNavigate: (vnum: number) => void;
};

export function MapExternalExitsPanel({
  externalExits,
  onNavigate
}: MapExternalExitsPanelProps) {
  return (
    <div className="map-panel">
      <div className="map-panel__header">
        <span>External exits</span>
        <span className="map-panel__count">{externalExits.length}</span>
      </div>
      {externalExits.length ? (
        <ul className="map-panel__list">
          {externalExits.map((exit) => (
            <li
              key={`${exit.fromVnum}-${exit.direction}-${exit.toVnum}`}
              className="map-panel__item"
            >
              <button
                type="button"
                className="map-panel__link"
                onClick={() => onNavigate(exit.fromVnum)}
                title={`${exit.fromName} (${exit.fromVnum})`}
              >
                {exit.fromVnum}
              </button>
              <span className="map-panel__dir">
                {exit.direction.toUpperCase()}
              </span>
              <span className="map-panel__target">
                -&gt; {exit.toVnum}
                <span className="map-panel__area">
                  {exit.areaName ?? "Unknown area"}
                </span>
              </span>
            </li>
          ))}
        </ul>
      ) : (
        <div className="map-panel__empty">No external exits detected.</div>
      )}
    </div>
  );
}
