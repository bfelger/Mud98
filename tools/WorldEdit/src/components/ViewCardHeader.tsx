type ViewCardHeaderProps = {
  title: string;
  vnumRange: string;
  lastSave: string;
  activeTab: string;
};

export function ViewCardHeader({
  title,
  vnumRange,
  lastSave,
  activeTab
}: ViewCardHeaderProps) {
  return (
    <div className="view-card__header">
      <h2>{title}</h2>
      <div className="view-card__meta">
        <span>VNUM range {vnumRange}</span>
        <span>Last save {lastSave}</span>
        <span>Active view {activeTab}</span>
      </div>
    </div>
  );
}
