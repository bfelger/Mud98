import type { FormEventHandler } from "react";
import type { UseFormRegister } from "react-hook-form";

type AreaFormProps = {
  onSubmit: FormEventHandler<HTMLFormElement>;
  register: UseFormRegister<any>;
  formState: {
    isDirty: boolean;
    errors: Record<string, { message?: string } | undefined>;
  };
  sectors: string[];
  storyBeatFields: Array<{ id: string }>;
  appendStoryBeat: (value: Record<string, unknown>) => void;
  removeStoryBeat: (index: number) => void;
  moveStoryBeat: (from: number, to: number) => void;
  checklistFields: Array<{ id: string }>;
  appendChecklist: (value: Record<string, unknown>) => void;
  removeChecklist: (index: number) => void;
  moveChecklist: (from: number, to: number) => void;
};

const checklistStatusOptions = ["todo", "inProgress", "done"] as const;

export function AreaForm({
  onSubmit,
  register,
  formState,
  sectors,
  storyBeatFields,
  appendStoryBeat,
  removeStoryBeat,
  moveStoryBeat,
  checklistFields,
  appendChecklist,
  removeChecklist,
  moveChecklist
}: AreaFormProps) {
  const errors = formState.errors as Record<string, any>;

  return (
    <div className="form-view">
      <form className="form-shell" onSubmit={onSubmit}>
        <div className="form-grid">
          <div className="form-field form-field--wide">
            <label className="form-label" htmlFor="area-name">
              Area Name
            </label>
            <input
              id="area-name"
              className="form-input"
              type="text"
              {...register("name")}
            />
            {errors.name?.message ? (
              <span className="form-error">{errors.name.message}</span>
            ) : null}
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="area-version">
              Version
            </label>
            <input
              id="area-version"
              className="form-input"
              type="number"
              {...register("version", { valueAsNumber: true })}
            />
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="area-vnum-start">
              VNUM Start
            </label>
            <input
              id="area-vnum-start"
              className="form-input"
              type="number"
              {...register("vnumRangeStart", { valueAsNumber: true })}
            />
            {errors.vnumRangeStart?.message ? (
              <span className="form-error">{errors.vnumRangeStart.message}</span>
            ) : null}
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="area-vnum-end">
              VNUM End
            </label>
            <input
              id="area-vnum-end"
              className="form-input"
              type="number"
              {...register("vnumRangeEnd", { valueAsNumber: true })}
            />
            {errors.vnumRangeEnd?.message ? (
              <span className="form-error">{errors.vnumRangeEnd.message}</span>
            ) : null}
          </div>
          <div className="form-field form-field--wide">
            <label className="form-label" htmlFor="area-builders">
              Builders
            </label>
            <input
              id="area-builders"
              className="form-input"
              type="text"
              {...register("builders")}
            />
          </div>
          <div className="form-field form-field--wide">
            <label className="form-label" htmlFor="area-credits">
              Credits
            </label>
            <input
              id="area-credits"
              className="form-input"
              type="text"
              {...register("credits")}
            />
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="area-security">
              Security
            </label>
            <input
              id="area-security"
              className="form-input"
              type="number"
              {...register("security", { valueAsNumber: true })}
            />
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="area-sector">
              Sector
            </label>
            <select
              id="area-sector"
              className="form-select"
              {...register("sector")}
            >
              <option value="">Default</option>
              {sectors.map((sector) => (
                <option key={sector} value={sector}>
                  {sector}
                </option>
              ))}
            </select>
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="area-low-level">
              Low Level
            </label>
            <input
              id="area-low-level"
              className="form-input"
              type="number"
              {...register("lowLevel", { valueAsNumber: true })}
            />
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="area-high-level">
              High Level
            </label>
            <input
              id="area-high-level"
              className="form-input"
              type="number"
              {...register("highLevel", { valueAsNumber: true })}
            />
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="area-reset">
              Reset Threshold
            </label>
            <input
              id="area-reset"
              className="form-input"
              type="number"
              {...register("reset", { valueAsNumber: true })}
            />
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="area-inst-type">
              Instance Type
            </label>
            <select
              id="area-inst-type"
              className="form-select"
              {...register("instType")}
            >
              <option value="single">Single</option>
              <option value="multi">Multi</option>
            </select>
          </div>
          <div className="form-field">
            <label className="form-label">Always Reset</label>
            <label className="checkbox-pill">
              <input type="checkbox" {...register("alwaysReset")} />
              <span>Enabled</span>
            </label>
          </div>
        </div>
        <div className="form-field form-field--full">
          <div className="form-section-header">
            <div>
              <div className="form-label">Story Beats</div>
              <div className="form-hint">
                {storyBeatFields.length
                  ? `${storyBeatFields.length} beat${
                      storyBeatFields.length === 1 ? "" : "s"
                    }`
                  : "No story beats yet"}
              </div>
            </div>
            <button
              className="ghost-button"
              type="button"
              onClick={() =>
                appendStoryBeat({
                  title: "",
                  description: ""
                })
              }
            >
              Add Beat
            </button>
          </div>
          <div className="block-list">
            {storyBeatFields.map((field, index) => (
              <div className="block-card" key={field.id}>
                <div className="block-card__header">
                  <span>Beat {index + 1}</span>
                  <div className="block-actions">
                    <button
                      className="ghost-button"
                      type="button"
                      onClick={() => moveStoryBeat(index, index - 1)}
                      disabled={index === 0}
                    >
                      Up
                    </button>
                    <button
                      className="ghost-button"
                      type="button"
                      onClick={() => moveStoryBeat(index, index + 1)}
                      disabled={index === storyBeatFields.length - 1}
                    >
                      Down
                    </button>
                    <button
                      className="ghost-button"
                      type="button"
                      onClick={() => removeStoryBeat(index)}
                    >
                      Remove
                    </button>
                  </div>
                </div>
                <div className="block-grid">
                  <div className="form-field form-field--wide">
                    <label className="form-label" htmlFor={`beat-title-${index}`}>
                      Title
                    </label>
                    <input
                      id={`beat-title-${index}`}
                      className="form-input"
                      type="text"
                      {...register(`storyBeats.${index}.title`)}
                    />
                  </div>
                  <div className="form-field form-field--full">
                    <label
                      className="form-label"
                      htmlFor={`beat-desc-${index}`}
                    >
                      Description
                    </label>
                    <textarea
                      id={`beat-desc-${index}`}
                      className="form-textarea"
                      rows={3}
                      {...register(`storyBeats.${index}.description`)}
                    />
                  </div>
                </div>
              </div>
            ))}
          </div>
        </div>
        <div className="form-field form-field--full">
          <div className="form-section-header">
            <div>
              <div className="form-label">Checklist</div>
              <div className="form-hint">
                {checklistFields.length
                  ? `${checklistFields.length} item${
                      checklistFields.length === 1 ? "" : "s"
                    }`
                  : "No checklist items yet"}
              </div>
            </div>
            <button
              className="ghost-button"
              type="button"
              onClick={() =>
                appendChecklist({
                  title: "",
                  status: "todo",
                  description: ""
                })
              }
            >
              Add Item
            </button>
          </div>
          <div className="block-list">
            {checklistFields.map((field, index) => (
              <div className="block-card" key={field.id}>
                <div className="block-card__header">
                  <span>Item {index + 1}</span>
                  <div className="block-actions">
                    <button
                      className="ghost-button"
                      type="button"
                      onClick={() => moveChecklist(index, index - 1)}
                      disabled={index === 0}
                    >
                      Up
                    </button>
                    <button
                      className="ghost-button"
                      type="button"
                      onClick={() => moveChecklist(index, index + 1)}
                      disabled={index === checklistFields.length - 1}
                    >
                      Down
                    </button>
                    <button
                      className="ghost-button"
                      type="button"
                      onClick={() => removeChecklist(index)}
                    >
                      Remove
                    </button>
                  </div>
                </div>
                <div className="block-grid">
                  <div className="form-field form-field--wide">
                    <label className="form-label" htmlFor={`check-title-${index}`}>
                      Title
                    </label>
                    <input
                      id={`check-title-${index}`}
                      className="form-input"
                      type="text"
                      {...register(`checklist.${index}.title`)}
                    />
                  </div>
                  <div className="form-field">
                    <label className="form-label" htmlFor={`check-status-${index}`}>
                      Status
                    </label>
                    <select
                      id={`check-status-${index}`}
                      className="form-select"
                      {...register(`checklist.${index}.status`)}
                    >
                      {checklistStatusOptions.map((status) => (
                        <option key={status} value={status}>
                          {status}
                        </option>
                      ))}
                    </select>
                  </div>
                  <div className="form-field form-field--full">
                    <label
                      className="form-label"
                      htmlFor={`check-desc-${index}`}
                    >
                      Description
                    </label>
                    <textarea
                      id={`check-desc-${index}`}
                      className="form-textarea"
                      rows={3}
                      {...register(`checklist.${index}.description`)}
                    />
                  </div>
                </div>
              </div>
            ))}
          </div>
        </div>
        <div className="form-actions">
          <button
            className="action-button action-button--primary"
            type="submit"
            disabled={!formState.isDirty}
          >
            Apply Changes
          </button>
          <span className="form-hint">
            {formState.isDirty ? "Unsaved changes" : "Up to date"}
          </span>
        </div>
      </form>
    </div>
  );
}
