import { useEditorUiStore } from "../stores/useEditorUiStore";

type ViewTab = {
  id: string;
};

type ViewTabsProps = {
  tabs: ViewTab[];
};

export function ViewTabs({ tabs }: ViewTabsProps) {
  const { activeTab, setActiveTab } = useEditorUiStore();
  
  return (
    <nav className="tabs" aria-label="View tabs">
      {tabs.map((tab) => (
        <button
          key={tab.id}
          type="button"
          className={`tab${tab.id === activeTab ? " tab--active" : ""}`}
          aria-pressed={tab.id === activeTab}
          onClick={() => setActiveTab(tab.id)}
        >
          {tab.id}
        </button>
      ))}
    </nav>
  );
}
