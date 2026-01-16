type ViewTab = {
  id: string;
};

type ViewTabsProps = {
  tabs: ViewTab[];
  activeTab: string;
  onSelectTab: (tabId: string) => void;
};

export function ViewTabs({ tabs, activeTab, onSelectTab }: ViewTabsProps) {
  return (
    <nav className="tabs" aria-label="View tabs">
      {tabs.map((tab) => (
        <button
          key={tab.id}
          type="button"
          className={`tab${tab.id === activeTab ? " tab--active" : ""}`}
          aria-pressed={tab.id === activeTab}
          onClick={() => onSelectTab(tab.id)}
        >
          {tab.id}
        </button>
      ))}
    </nav>
  );
}
