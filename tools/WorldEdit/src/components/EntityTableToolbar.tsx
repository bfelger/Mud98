export type EntityTableToolbarConfig = {
  title: string;
  count: number;
  newLabel?: string;
  newLabelSecondary?: string;
  deleteLabel?: string;
  onCreate?: () => void;
  onCreateSecondary?: () => void;
  onDelete?: () => void;
  canCreate?: boolean;
  canCreateSecondary?: boolean;
  canDelete?: boolean;
};

export function EntityTableToolbar({
  title,
  count,
  newLabel = "New",
  newLabelSecondary,
  deleteLabel = "Delete",
  onCreate,
  onCreateSecondary,
  onDelete,
  canCreate,
  canCreateSecondary,
  canDelete
}: EntityTableToolbarConfig) {
  const allowCreate = canCreate ?? Boolean(onCreate);
  const allowCreateSecondary =
    canCreateSecondary ?? Boolean(onCreateSecondary);
  const allowDelete = canDelete ?? Boolean(onDelete);
  const disableCreate = !onCreate || !allowCreate;
  const disableCreateSecondary = !onCreateSecondary || !allowCreateSecondary;
  const disableDelete = !onDelete || !allowDelete;

  return (
    <div className="entity-table__toolbar">
      <div className="entity-table__toolbar-meta">
        <span className="entity-table__toolbar-title">{title}</span>
        <span className="entity-table__toolbar-count">{count} total</span>
      </div>
      <div className="entity-table__toolbar-actions">
        <button
          className="ghost-button"
          type="button"
          onClick={onCreate}
          disabled={disableCreate}
        >
          {newLabel}
        </button>
        {newLabelSecondary || onCreateSecondary ? (
          <button
            className="ghost-button"
            type="button"
            onClick={onCreateSecondary}
            disabled={disableCreateSecondary}
          >
            {newLabelSecondary ?? "New"}
          </button>
        ) : null}
        <button
          className="ghost-button ghost-button--danger"
          type="button"
          onClick={onDelete}
          disabled={disableDelete}
        >
          {deleteLabel}
        </button>
      </div>
    </div>
  );
}
