type PlaceholderTab = {
  id: string;
  title: string;
  description: string;
};

type PlaceholderGridProps = {
  tabs: PlaceholderTab[];
  activeTab: string;
};

export function PlaceholderGrid({ tabs, activeTab }: PlaceholderGridProps) {
  return (
    <div className="placeholder-grid">
      {tabs.map((tab) => (
        <div
          className={`placeholder-block${
            tab.id === activeTab ? " placeholder-block--active" : ""
          }`}
          key={tab.id}
        >
          <div className="placeholder-title">{tab.title}</div>
          <p>{tab.description}</p>
        </div>
      ))}
    </div>
  );
}
