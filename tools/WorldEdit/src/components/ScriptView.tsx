import { useEffect, useMemo, useState } from "react";
import Editor from "@monaco-editor/react";
import { registerLoxLanguage } from "../monaco/loxLanguage";

type ScriptViewProps = {
  title: string;
  subtitle?: string;
  value: string;
  canEdit: boolean;
  emptyState: string;
  onApply: (value: string) => void;
};

const editorOptions = {
  minimap: { enabled: false },
  scrollBeyondLastLine: false,
  tabSize: 2,
  fontSize: 13,
  fontFamily: "\"IBM Plex Mono\", \"Fira Mono\", \"Menlo\", monospace"
};

export function ScriptView({
  title,
  subtitle,
  value,
  canEdit,
  emptyState,
  onApply
}: ScriptViewProps) {
  const [draft, setDraft] = useState(value);

  useEffect(() => {
    setDraft(value);
  }, [value]);

  const isDirty = draft !== value;
  const footerLabel = useMemo(
    () => (isDirty ? "Unsaved changes" : "Up to date"),
    [isDirty]
  );

  if (!canEdit) {
    return (
      <div className="script-view script-view__empty">
        <h3>No script target selected</h3>
        <p>{emptyState}</p>
      </div>
    );
  }

  return (
    <div className="script-view">
      <div className="script-view__header">
        <div>
          <div className="script-view__title">{title}</div>
          {subtitle ? (
            <div className="script-view__meta">{subtitle}</div>
          ) : null}
        </div>
        <div className="script-view__badge">Lox Script</div>
      </div>
      <div className="script-view__editor">
        <Editor
          beforeMount={registerLoxLanguage}
          language="lox"
          theme="vs"
          value={draft}
          height="100%"
          onChange={(nextValue) => setDraft(nextValue ?? "")}
          options={editorOptions}
        />
      </div>
      <div className="script-view__footer">
        <div className="script-view__actions">
          <button
            className="action-button action-button--primary"
            type="button"
            onClick={() => onApply(draft)}
            disabled={!isDirty}
          >
            Apply Script
          </button>
          <button
            className="ghost-button"
            type="button"
            onClick={() => setDraft(value)}
            disabled={!isDirty}
          >
            Reset
          </button>
        </div>
        <span className="form-hint">{footerLabel}</span>
      </div>
    </div>
  );
}
