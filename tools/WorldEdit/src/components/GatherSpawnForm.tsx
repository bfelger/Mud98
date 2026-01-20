import type { FormEventHandler } from "react";
import type { Control, UseFormRegister } from "react-hook-form";
import { VnumPicker, type VnumOption } from "./VnumPicker";

type GatherSpawnFormProps = {
  onSubmit: FormEventHandler<HTMLFormElement>;
  register: UseFormRegister<any>;
  control: Control<any>;
  formState: {
    isDirty: boolean;
    errors: Record<string, { message?: string } | undefined>;
  };
  sectors: string[];
  objectVnumOptions: VnumOption[];
};

export function GatherSpawnForm({
  onSubmit,
  register,
  control,
  formState,
  sectors,
  objectVnumOptions
}: GatherSpawnFormProps) {
  const errors = formState.errors as Record<string, any>;

  return (
    <div className="form-view">
      <form className="form-shell" onSubmit={onSubmit}>
        <div className="form-grid">
          <div className="form-field">
            <label className="form-label" htmlFor="gather-sector">
              Spawn Sector
            </label>
            <select
              id="gather-sector"
              className="form-select"
              {...register("spawnSector")}
            >
              <option value="">Default</option>
              {sectors.map((sector) => (
                <option key={sector} value={sector}>
                  {sector}
                </option>
              ))}
            </select>
          </div>
          <VnumPicker<any>
            id="gather-vnum"
            label="Object VNUM"
            name="vnum"
            register={register}
            control={control}
            options={objectVnumOptions}
            entityLabel="Object"
            error={errors.vnum?.message}
          />
          <div className="form-field">
            <label className="form-label" htmlFor="gather-qty">
              Quantity
            </label>
            <input
              id="gather-qty"
              className="form-input"
              type="number"
              {...register("quantity", { valueAsNumber: true })}
            />
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="gather-respawn">
              Respawn Timer
            </label>
            <input
              id="gather-respawn"
              className="form-input"
              type="number"
              {...register("respawnTimer", { valueAsNumber: true })}
            />
          </div>
        </div>

        <div className="form-actions">
          <button
            className="action-button action-button--primary"
            type="submit"
            disabled={!formState.isDirty}
          >
            Save Gather Spawn
          </button>
          <span className="form-hint">
            {formState.isDirty ? "Unsaved changes" : "Up to date"}
          </span>
        </div>
      </form>
    </div>
  );
}
