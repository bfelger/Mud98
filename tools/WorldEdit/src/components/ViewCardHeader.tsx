type ViewCardHeaderProps = {
  title: string;
  meta: string[];
};

export function ViewCardHeader({ title, meta }: ViewCardHeaderProps) {
  return (
    <div className="view-card__header">
      <h2>{title}</h2>
      <div className="view-card__meta">
        {meta.map((item, index) => (
          <span key={`${item}-${index}`}>{item}</span>
        ))}
      </div>
    </div>
  );
}
