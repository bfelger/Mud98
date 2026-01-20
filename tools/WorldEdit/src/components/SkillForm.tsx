import type { FormEventHandler } from "react";
import type { UseFormRegister } from "react-hook-form";

type SkillFormProps = {
  onSubmit: FormEventHandler<HTMLFormElement>;
  register: UseFormRegister<any>;
  formState: {
    isDirty: boolean;
    errors: Record<string, { message?: string } | undefined>;
  };
  classNames: string[];
  targetOptions: string[];
  positionOptions: string[];
};

export function SkillForm({
  onSubmit,
  register,
  formState,
  classNames,
  targetOptions,
  positionOptions
}: SkillFormProps) {
  const errors = formState.errors as Record<string, any>;

  return (
    <div className="form-view">
      <form className="form-shell" onSubmit={onSubmit}>
        <div className="form-grid">
          <div className="form-field form-field--wide">
            <label className="form-label" htmlFor="skill-name">
              Skill Name
            </label>
            <input
              id="skill-name"
              className="form-input"
              type="text"
              {...register("name")}
            />
            {errors.name?.message ? (
              <span className="form-error">{errors.name.message}</span>
            ) : null}
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="skill-spell">
              Spell Func
            </label>
            <input
              id="skill-spell"
              className="form-input"
              type="text"
              {...register("spell")}
            />
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="skill-lox">
              Lox Spell
            </label>
            <input
              id="skill-lox"
              className="form-input"
              type="text"
              {...register("loxSpell")}
            />
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="skill-target">
              Target
            </label>
            <select
              id="skill-target"
              className="form-select"
              {...register("target")}
            >
              <option value="">Default (tar_ignore)</option>
              {targetOptions.map((option) => (
                <option key={option} value={option}>
                  {option}
                </option>
              ))}
            </select>
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="skill-position">
              Minimum Position
            </label>
            <select
              id="skill-position"
              className="form-select"
              {...register("minPosition")}
            >
              <option value="">Default (dead)</option>
              {positionOptions.map((option) => (
                <option key={option} value={option}>
                  {option}
                </option>
              ))}
            </select>
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="skill-gsn">
              GSN
            </label>
            <input
              id="skill-gsn"
              className="form-input"
              type="text"
              {...register("gsn")}
            />
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="skill-slot">
              Slot
            </label>
            <input
              id="skill-slot"
              className="form-input"
              type="number"
              {...register("slot", { valueAsNumber: true })}
            />
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="skill-mana">
              Min Mana
            </label>
            <input
              id="skill-mana"
              className="form-input"
              type="number"
              {...register("minMana", { valueAsNumber: true })}
            />
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="skill-beats">
              Beats
            </label>
            <input
              id="skill-beats"
              className="form-input"
              type="number"
              {...register("beats", { valueAsNumber: true })}
            />
          </div>
        </div>

        <div className="form-field form-field--full">
          <div className="form-section-header">
            <div>
              <div className="form-label">Class Levels</div>
              <div className="form-hint">
                Level required to learn the skill.
              </div>
            </div>
          </div>
          <div className="form-grid">
            {classNames.map((className) => (
              <div className="form-field" key={`skill-level-${className}`}>
                <label
                  className="form-label"
                  htmlFor={`skill-level-${className}`}
                >
                  {className}
                </label>
                <input
                  id={`skill-level-${className}`}
                  className="form-input"
                  type="number"
                  {...register(`levels.${className}`, {
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
              <div className="form-label">Class Ratings</div>
              <div className="form-hint">Training costs by class.</div>
            </div>
          </div>
          <div className="form-grid">
            {classNames.map((className) => (
              <div className="form-field" key={`skill-rating-${className}`}>
                <label
                  className="form-label"
                  htmlFor={`skill-rating-${className}`}
                >
                  {className}
                </label>
                <input
                  id={`skill-rating-${className}`}
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

        <div className="form-grid">
          <div className="form-field form-field--wide">
            <label className="form-label" htmlFor="skill-noun">
              Noun Damage
            </label>
            <input
              id="skill-noun"
              className="form-input"
              type="text"
              {...register("nounDamage")}
            />
          </div>
          <div className="form-field form-field--full">
            <label className="form-label" htmlFor="skill-msg-off">
              Message Off
            </label>
            <textarea
              id="skill-msg-off"
              className="form-textarea"
              rows={3}
              {...register("msgOff")}
            />
          </div>
          <div className="form-field form-field--full">
            <label className="form-label" htmlFor="skill-msg-obj">
              Message Object
            </label>
            <textarea
              id="skill-msg-obj"
              className="form-textarea"
              rows={3}
              {...register("msgObj")}
            />
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
