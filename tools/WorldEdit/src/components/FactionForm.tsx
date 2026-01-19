import type { FormEventHandler } from "react";
import type { UseFormRegister } from "react-hook-form";

type FactionFormProps = {
  onSubmit: FormEventHandler<HTMLFormElement>;
  register: UseFormRegister<any>;
  formState: {
    isDirty: boolean;
    errors: Record<string, { message?: string } | undefined>;
  };
};

export function FactionForm({ onSubmit, register, formState }: FactionFormProps) {
  const errors = formState.errors as Record<string, any>;

  return (
    <div className="form-view">
      <form className="form-shell" onSubmit={onSubmit}>
        <div className="form-grid">
          <div className="form-field">
            <label className="form-label" htmlFor="faction-vnum">
              VNUM
            </label>
            <input
              id="faction-vnum"
              className="form-input"
              type="number"
              readOnly
              {...register("vnum", { valueAsNumber: true })}
            />
          </div>
          <div className="form-field form-field--wide">
            <label className="form-label" htmlFor="faction-name">
              Name
            </label>
            <input
              id="faction-name"
              className="form-input"
              type="text"
              {...register("name")}
            />
            {errors.name?.message ? (
              <span className="form-error">{errors.name.message}</span>
            ) : null}
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="faction-default">
              Default Standing
            </label>
            <input
              id="faction-default"
              className="form-input"
              type="number"
              {...register("defaultStanding")}
            />
          </div>
        </div>
        <div className="form-field form-field--full">
          <label className="form-label" htmlFor="faction-allies">
            Allies (VNUM list)
          </label>
          <input
            id="faction-allies"
            className="form-input"
            type="text"
            placeholder="e.g. 1001, 1002"
            {...register("alliesCsv")}
          />
        </div>
        <div className="form-field form-field--full">
          <label className="form-label" htmlFor="faction-opposing">
            Opposing (VNUM list)
          </label>
          <input
            id="faction-opposing"
            className="form-input"
            type="text"
            placeholder="e.g. 2001, 2002"
            {...register("opposingCsv")}
          />
        </div>
        <div className="form-actions">
          <button className="action-button" type="submit">
            Save Faction
          </button>
        </div>
      </form>
    </div>
  );
}
