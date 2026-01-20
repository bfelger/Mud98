import type { FormEventHandler } from "react";
import type { UseFormRegister } from "react-hook-form";

type CommandFormProps = {
  onSubmit: FormEventHandler<HTMLFormElement>;
  register: UseFormRegister<any>;
  formState: {
    isDirty: boolean;
    errors: Record<string, { message?: string } | undefined>;
  };
  positionOptions: string[];
  logOptions: string[];
  categoryOptions: string[];
};

export function CommandForm({
  onSubmit,
  register,
  formState,
  positionOptions,
  logOptions,
  categoryOptions
}: CommandFormProps) {
  const errors = formState.errors as Record<string, any>;

  return (
    <div className="form-view">
      <form className="form-shell" onSubmit={onSubmit}>
        <div className="form-grid">
          <div className="form-field form-field--wide">
            <label className="form-label" htmlFor="command-name">
              Command Name
            </label>
            <input
              id="command-name"
              className="form-input"
              type="text"
              {...register("name")}
            />
            {errors.name?.message ? (
              <span className="form-error">{errors.name.message}</span>
            ) : null}
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="command-func">
              Function
            </label>
            <input
              id="command-func"
              className="form-input"
              type="text"
              {...register("function")}
            />
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="command-lox">
              Lox Function
            </label>
            <input
              id="command-lox"
              className="form-input"
              type="text"
              {...register("loxFunction")}
            />
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="command-position">
              Position
            </label>
            <select
              id="command-position"
              className="form-select"
              {...register("position")}
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
            <label className="form-label" htmlFor="command-level">
              Level
            </label>
            <input
              id="command-level"
              className="form-input"
              type="number"
              {...register("level", { valueAsNumber: true })}
            />
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="command-log">
              Log
            </label>
            <select
              id="command-log"
              className="form-select"
              {...register("log")}
            >
              <option value="">Default (log_normal)</option>
              {logOptions.map((option) => (
                <option key={option} value={option}>
                  {option}
                </option>
              ))}
            </select>
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="command-category">
              Category
            </label>
            <select
              id="command-category"
              className="form-select"
              {...register("category")}
            >
              <option value="">Default (undef)</option>
              {categoryOptions.map((option) => (
                <option key={option} value={option}>
                  {option}
                </option>
              ))}
            </select>
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
