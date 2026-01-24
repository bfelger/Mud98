import type { FormEventHandler } from "react";
import type { UseFormRegister } from "react-hook-form";

type GroupFormProps = {
  onSubmit: FormEventHandler<HTMLFormElement>;
  register: UseFormRegister<any>;
  formState: {
    isDirty: boolean;
    errors: Record<string, { message?: string } | undefined>;
  };
  classNames: string[];
  skillOptions: string[];
  skillCount: number;
};

export function GroupForm({
  onSubmit,
  register,
  formState,
  classNames,
  skillOptions,
  skillCount
}: GroupFormProps) {
  const errors = formState.errors as Record<string, any>;

  return (
    <div className="form-view">
      <form className="form-shell" onSubmit={onSubmit}>
        <div className="form-grid">
          <div className="form-field form-field--wide">
            <label className="form-label" htmlFor="group-name">
              Group Name
            </label>
            <input
              id="group-name"
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
              <div className="form-label">Class Ratings</div>
              <div className="form-hint">Group costs per class.</div>
            </div>
          </div>
          <div className="form-grid">
            {classNames.map((className) => (
              <div className="form-field" key={`group-rating-${className}`}>
                <label
                  className="form-label"
                  htmlFor={`group-rating-${className}`}
                >
                  {className}
                </label>
                <input
                  id={`group-rating-${className}`}
                  className="form-input"
                  type="number"
                  {...register(`ratings.${className}`, {
                    valueAsNumber: true
                  })}
                />
              </div>
            ))}
          </div>
        </div>

        <div className="form-field form-field--full">
          <div className="form-section-header">
            <div>
              <div className="form-label">Skills</div>
              <div className="form-hint">Up to {skillCount} skills.</div>
            </div>
          </div>
          <datalist id="group-skill-options">
            {skillOptions.map((skill) => (
              <option key={skill} value={skill} />
            ))}
          </datalist>
          <div className="form-grid">
            {Array.from({ length: skillCount }).map((_, index) => (
              <div className="form-field" key={`group-skill-${index}`}>
                <label
                  className="form-label"
                  htmlFor={`group-skill-${index}`}
                >
                  Skill {index + 1}
                </label>
                <input
                  id={`group-skill-${index}`}
                  className="form-input"
                  type="text"
                  list="group-skill-options"
                  {...register(`skills.${index}`)}
                />
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
