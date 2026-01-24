import type { FormEventHandler } from "react";
import type { UseFormRegister } from "react-hook-form";

type SocialFormProps = {
  onSubmit: FormEventHandler<HTMLFormElement>;
  register: UseFormRegister<any>;
  formState: {
    isDirty: boolean;
    errors: Record<string, { message?: string } | undefined>;
  };
};

export function SocialForm({ onSubmit, register, formState }: SocialFormProps) {
  const errors = formState.errors as Record<string, any>;

  return (
    <div className="form-view">
      <form className="form-shell" onSubmit={onSubmit}>
        <div className="form-grid">
          <div className="form-field form-field--wide">
            <label className="form-label" htmlFor="social-name">
              Social Name
            </label>
            <input
              id="social-name"
              className="form-input"
              type="text"
              {...register("name")}
            />
            {errors.name?.message ? (
              <span className="form-error">{errors.name.message}</span>
            ) : null}
          </div>
        </div>

        <div className="form-field form-field--full">
          <div className="form-section-header">
            <div>
              <div className="form-label">No Target</div>
              <div className="form-hint">Messages when no target is supplied.</div>
            </div>
          </div>
          <div className="form-grid">
            <div className="form-field form-field--full">
              <label className="form-label" htmlFor="social-char-no-arg">
                Actor Message
              </label>
              <textarea
                id="social-char-no-arg"
                className="form-textarea"
                rows={2}
                {...register("charNoArg")}
              />
            </div>
            <div className="form-field form-field--full">
              <label className="form-label" htmlFor="social-others-no-arg">
                Room Message
              </label>
              <textarea
                id="social-others-no-arg"
                className="form-textarea"
                rows={2}
                {...register("othersNoArg")}
              />
            </div>
          </div>
        </div>

        <div className="form-field form-field--full">
          <div className="form-section-header">
            <div>
              <div className="form-label">Targeted</div>
              <div className="form-hint">Messages when a target is found.</div>
            </div>
          </div>
          <div className="form-grid">
            <div className="form-field form-field--full">
              <label className="form-label" htmlFor="social-char-found">
                Actor Message
              </label>
              <textarea
                id="social-char-found"
                className="form-textarea"
                rows={2}
                {...register("charFound")}
              />
            </div>
            <div className="form-field form-field--full">
              <label className="form-label" htmlFor="social-others-found">
                Room Message
              </label>
              <textarea
                id="social-others-found"
                className="form-textarea"
                rows={2}
                {...register("othersFound")}
              />
            </div>
            <div className="form-field form-field--full">
              <label className="form-label" htmlFor="social-vict-found">
                Victim Message
              </label>
              <textarea
                id="social-vict-found"
                className="form-textarea"
                rows={2}
                {...register("victFound")}
              />
            </div>
          </div>
        </div>

        <div className="form-field form-field--full">
          <div className="form-section-header">
            <div>
              <div className="form-label">Self Target</div>
              <div className="form-hint">Messages when targeting yourself.</div>
            </div>
          </div>
          <div className="form-grid">
            <div className="form-field form-field--full">
              <label className="form-label" htmlFor="social-char-auto">
                Actor Message
              </label>
              <textarea
                id="social-char-auto"
                className="form-textarea"
                rows={2}
                {...register("charAuto")}
              />
            </div>
            <div className="form-field form-field--full">
              <label className="form-label" htmlFor="social-others-auto">
                Room Message
              </label>
              <textarea
                id="social-others-auto"
                className="form-textarea"
                rows={2}
                {...register("othersAuto")}
              />
            </div>
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
