export type EntityTableToolbarConfig = {
  title: string;
  count: number;
  newLabel?: string;
  deleteLabel?: string;
  onCreate?: () => void;
  onDelete?: () => void;
  canCreate?: boolean;
  canDelete?: boolean;
};

export function EntityTableToolbar({
  title,
  count,
  newLabel = "New",
  deleteLabel = "Delete",
  onCreate,
  onDelete,
  canCreate,
  canDelete
}: EntityTableToolbarConfig) {
  const allowCreate = canCreate ?? Boolean(onCreate);
  const allowDelete = canDelete ?? Boolean(onDelete);
  const disableCreate = !onCreate || !allowCreate;
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
