import type { FormEventHandler } from "react";
import type { UseFormRegister } from "react-hook-form";

type TutorialFormProps = {
  onSubmit: FormEventHandler<HTMLFormElement>;
  register: UseFormRegister<any>;
  formState: {
    isDirty: boolean;
    errors: Record<string, { message?: string } | undefined>;
  };
  stepFields: Array<{ id: string }>;
  appendStep: (value: Record<string, unknown>) => void;
  removeStep: (index: number) => void;
  moveStep: (from: number, to: number) => void;
};

export function TutorialForm({
  onSubmit,
  register,
  formState,
  stepFields,
  appendStep,
  removeStep,
  moveStep
}: TutorialFormProps) {
  const errors = formState.errors as Record<string, any>;

  return (
    <div className="form-view">
      <form className="form-shell" onSubmit={onSubmit}>
        <div className="form-grid">
          <div className="form-field form-field--wide">
            <label className="form-label" htmlFor="tutorial-name">
              Tutorial Name
            </label>
            <input
              id="tutorial-name"
              className="form-input"
              type="text"
              {...register("name")}
            />
            {errors.name?.message ? (
              <span className="form-error">{errors.name.message}</span>
            ) : null}
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="tutorial-min-level">
              Min Level
            </label>
            <input
              id="tutorial-min-level"
              className="form-input"
              type="number"
              {...register("minLevel", { valueAsNumber: true })}
            />
          </div>
          <div className="form-field form-field--full">
            <label className="form-label" htmlFor="tutorial-blurb">
              Blurb
            </label>
            <textarea
              id="tutorial-blurb"
              className="form-textarea"
              rows={3}
              {...register("blurb")}
            />
          </div>
          <div className="form-field form-field--full">
            <label className="form-label" htmlFor="tutorial-finish">
              Finish Message
            </label>
            <textarea
              id="tutorial-finish"
              className="form-textarea"
              rows={3}
              {...register("finish")}
            />
          </div>
        </div>

        <div className="form-field form-field--full">
          <div className="form-section-header">
            <div>
              <div className="form-label">Steps</div>
              <div className="form-hint">
                {stepFields.length
                  ? `${stepFields.length} step${
                      stepFields.length === 1 ? "" : "s"
                    }`
                  : "No steps yet"}
              </div>
            </div>
            <button
              className="ghost-button"
              type="button"
              onClick={() =>
                appendStep({
                  prompt: "",
                  match: ""
                })
              }
            >
              Add Step
            </button>
          </div>
          <div className="block-list">
            {stepFields.map((field, index) => (
              <div className="block-card" key={field.id}>
                <div className="block-card__header">
                  <span>Step {index + 1}</span>
                  <div className="block-actions">
                    <button
                      className="ghost-button"
                      type="button"
                      onClick={() => moveStep(index, index - 1)}
                      disabled={index === 0}
                    >
                      Up
                    </button>
                    <button
                      className="ghost-button"
                      type="button"
                      onClick={() => moveStep(index, index + 1)}
                      disabled={index === stepFields.length - 1}
                    >
                      Down
                    </button>
                    <button
                      className="ghost-button"
                      type="button"
                      onClick={() => removeStep(index)}
                    >
                      Remove
                    </button>
                  </div>
                </div>
                <div className="block-grid">
                  <div className="form-field form-field--full">
                    <label
                      className="form-label"
                      htmlFor={`tutorial-step-prompt-${index}`}
                    >
                      Prompt
                    </label>
                    <textarea
                      id={`tutorial-step-prompt-${index}`}
                      className="form-textarea"
                      rows={4}
                      {...register(`steps.${index}.prompt`)}
                    />
                  </div>
                  <div className="form-field form-field--full">
                    <label
                      className="form-label"
                      htmlFor={`tutorial-step-match-${index}`}
                    >
                      Match Pattern
                    </label>
                    <input
                      id={`tutorial-step-match-${index}`}
                      className="form-input"
                      type="text"
                      {...register(`steps.${index}.match`)}
                    />
                  </div>
                </div>
              </div>
            ))}
          </div>
        </div>

        <div className="form-actions">
          <button
            className="primary-button"
            type="submit"
            disabled={!formState.isDirty}
          >
            Save changes
          </button>
        </div>
      </form>
    </div>
  );
}
