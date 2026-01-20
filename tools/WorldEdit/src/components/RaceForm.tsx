import type { FormEventHandler } from "react";
import type { UseFormRegister } from "react-hook-form";

type RaceFormProps = {
  onSubmit: FormEventHandler<HTMLFormElement>;
  register: UseFormRegister<any>;
  formState: {
    isDirty: boolean;
    errors: Record<string, { message?: string } | undefined>;
  };
  sizeOptions: string[];
  statKeys: string[];
  actFlags: string[];
  affectFlags: string[];
  offFlags: string[];
  immFlags: string[];
  resFlags: string[];
  vulnFlags: string[];
  formFlags: string[];
  partFlags: string[];
  classNames: string[];
  skillCount: number;
};

export function RaceForm({
  onSubmit,
  register,
  formState,
  sizeOptions,
  statKeys,
  actFlags,
  affectFlags,
  offFlags,
  immFlags,
  resFlags,
  vulnFlags,
  formFlags,
  partFlags,
  classNames,
  skillCount
}: RaceFormProps) {
  const errors = formState.errors as Record<string, any>;

  return (
    <div className="form-view">
      <form className="form-shell" onSubmit={onSubmit}>
        <div className="form-grid">
          <div className="form-field form-field--wide">
            <label className="form-label" htmlFor="race-name">
              Race Name
            </label>
            <input
              id="race-name"
              className="form-input"
              type="text"
              {...register("name")}
            />
            {errors.name?.message ? (
              <span className="form-error">{errors.name.message}</span>
            ) : null}
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="race-who">
              Who Name
            </label>
            <input
              id="race-who"
              className="form-input"
              type="text"
              {...register("whoName")}
            />
          </div>
          <div className="form-field">
            <label className="form-label">Playable</label>
            <label className="checkbox-pill">
              <input type="checkbox" {...register("pc")} />
              <span>PC Race</span>
            </label>
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="race-points">
              Points
            </label>
            <input
              id="race-points"
              className="form-input"
              type="number"
              {...register("points", { valueAsNumber: true })}
            />
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="race-size">
              Size
            </label>
            <select
              id="race-size"
              className="form-select"
              {...register("size")}
            >
              <option value="">Default (medium)</option>
              {sizeOptions.map((size) => (
                <option key={size} value={size}>
                  {size}
                </option>
              ))}
            </select>
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="race-start">
              Start Loc
            </label>
            <input
              id="race-start"
              className="form-input"
              type="number"
              {...register("startLoc", { valueAsNumber: true })}
            />
          </div>
        </div>

        <div className="form-field form-field--full">
          <div className="form-section-header">
            <div>
              <div className="form-label">Base Stats</div>
              <div className="form-hint">Racial stat adjustments.</div>
            </div>
          </div>
          <div className="form-grid">
            {statKeys.map((stat) => (
              <div className="form-field" key={`stat-${stat}`}>
                <label className="form-label" htmlFor={`race-stat-${stat}`}>
                  {stat.toUpperCase()}
                </label>
                <input
                  id={`race-stat-${stat}`}
                  className="form-input"
                  type="number"
                  {...register(`stats.${stat}`, { valueAsNumber: true })}
                />
              </div>
            ))}
          </div>
        </div>

        <div className="form-field form-field--full">
          <div className="form-section-header">
            <div>
              <div className="form-label">Max Stats</div>
              <div className="form-hint">Maximum racial stat caps.</div>
            </div>
          </div>
          <div className="form-grid">
            {statKeys.map((stat) => (
              <div className="form-field" key={`max-stat-${stat}`}>
                <label className="form-label" htmlFor={`race-max-${stat}`}>
                  {stat.toUpperCase()}
                </label>
                <input
                  id={`race-max-${stat}`}
                  className="form-input"
                  type="number"
                  {...register(`maxStats.${stat}`, { valueAsNumber: true })}
                />
              </div>
            ))}
          </div>
        </div>

        <div className="form-field form-field--full">
          <label className="form-label">Act Flags</label>
          <div className="form-checkboxes">
            {actFlags.map((flag) => (
              <label key={flag} className="checkbox-pill">
                <input type="checkbox" value={flag} {...register("actFlags")} />
                <span>{flag}</span>
              </label>
            ))}
          </div>
        </div>

        <div className="form-field form-field--full">
          <label className="form-label">Affect Flags</label>
          <div className="form-checkboxes">
            {affectFlags.map((flag) => (
              <label key={flag} className="checkbox-pill">
                <input
                  type="checkbox"
                  value={flag}
                  {...register("affectFlags")}
                />
                <span>{flag}</span>
              </label>
            ))}
          </div>
        </div>

        <div className="form-field form-field--full">
          <label className="form-label">Offense Flags</label>
          <div className="form-checkboxes">
            {offFlags.map((flag) => (
              <label key={flag} className="checkbox-pill">
                <input type="checkbox" value={flag} {...register("offFlags")} />
                <span>{flag}</span>
              </label>
            ))}
          </div>
        </div>

        <div className="form-field form-field--full">
          <label className="form-label">Immunity Flags</label>
          <div className="form-checkboxes">
            {immFlags.map((flag) => (
              <label key={flag} className="checkbox-pill">
                <input type="checkbox" value={flag} {...register("immFlags")} />
                <span>{flag}</span>
              </label>
            ))}
          </div>
        </div>

        <div className="form-field form-field--full">
          <label className="form-label">Resistance Flags</label>
          <div className="form-checkboxes">
            {resFlags.map((flag) => (
              <label key={flag} className="checkbox-pill">
                <input type="checkbox" value={flag} {...register("resFlags")} />
                <span>{flag}</span>
              </label>
            ))}
          </div>
        </div>

        <div className="form-field form-field--full">
          <label className="form-label">Vulnerability Flags</label>
          <div className="form-checkboxes">
            {vulnFlags.map((flag) => (
              <label key={flag} className="checkbox-pill">
                <input
                  type="checkbox"
                  value={flag}
                  {...register("vulnFlags")}
                />
                <span>{flag}</span>
              </label>
            ))}
          </div>
        </div>

        <div className="form-field form-field--full">
          <label className="form-label">Form Flags</label>
          <div className="form-checkboxes">
            {formFlags.map((flag) => (
              <label key={flag} className="checkbox-pill">
                <input type="checkbox" value={flag} {...register("formFlags")} />
                <span>{flag}</span>
              </label>
            ))}
          </div>
        </div>

        <div className="form-field form-field--full">
          <label className="form-label">Part Flags</label>
          <div className="form-checkboxes">
            {partFlags.map((flag) => (
              <label key={flag} className="checkbox-pill">
                <input type="checkbox" value={flag} {...register("partFlags")} />
                <span>{flag}</span>
              </label>
            ))}
          </div>
        </div>

        <div className="form-field form-field--full">
          <div className="form-section-header">
            <div>
              <div className="form-label">Class Multipliers</div>
              <div className="form-hint">Experience cost per class.</div>
            </div>
          </div>
          <div className="form-grid">
            {classNames.map((className) => (
              <div className="form-field" key={`class-mult-${className}`}>
                <label
                  className="form-label"
                  htmlFor={`race-class-mult-${className}`}
                >
                  {className}
                </label>
                <input
                  id={`race-class-mult-${className}`}
                  className="form-input"
                  type="number"
                  {...register(`classMult.${className}`, {
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
              <div className="form-label">Class Start Rooms</div>
              <div className="form-hint">Overrides for class start VNUMs.</div>
            </div>
          </div>
          <div className="form-grid">
            {classNames.map((className) => (
              <div className="form-field" key={`class-start-${className}`}>
                <label
                  className="form-label"
                  htmlFor={`race-class-start-${className}`}
                >
                  {className}
                </label>
                <input
                  id={`race-class-start-${className}`}
                  className="form-input"
                  type="number"
                  {...register(`classStart.${className}`, {
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
              <div className="form-hint">Up to {skillCount} racial skills.</div>
            </div>
          </div>
          <div className="form-grid">
            {Array.from({ length: skillCount }).map((_, index) => (
              <div className="form-field" key={`race-skill-${index}`}>
                <label
                  className="form-label"
                  htmlFor={`race-skill-${index}`}
                >
                  Skill {index + 1}
                </label>
                <input
                  id={`race-skill-${index}`}
                  className="form-input"
                  type="text"
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
