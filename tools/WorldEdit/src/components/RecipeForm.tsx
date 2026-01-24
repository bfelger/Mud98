import type { FormEventHandler } from "react";
import type { Control, UseFormRegister } from "react-hook-form";
import { VnumPicker, type VnumOption } from "./VnumPicker";

type RecipeFormProps = {
  onSubmit: FormEventHandler<HTMLFormElement>;
  register: UseFormRegister<any>;
  control: Control<any>;
  formState: {
    isDirty: boolean;
    errors: Record<string, { message?: string } | undefined>;
  };
  inputFields: Array<{ id: string }>;
  appendInput: (value: Record<string, unknown>) => void;
  removeInput: (index: number) => void;
  moveInput: (from: number, to: number) => void;
  workstationTypeOptions: string[];
  discoveryOptions: string[];
  objectVnumOptions: VnumOption[];
};

export function RecipeForm({
  onSubmit,
  register,
  control,
  formState,
  inputFields,
  appendInput,
  removeInput,
  moveInput,
  workstationTypeOptions,
  discoveryOptions,
  objectVnumOptions
}: RecipeFormProps) {
  const errors = formState.errors as Record<string, any>;

  return (
    <div className="form-view">
      <form className="form-shell" onSubmit={onSubmit}>
        <div className="form-grid">
          <div className="form-field">
            <label className="form-label" htmlFor="recipe-vnum">
              VNUM
            </label>
            <input
              id="recipe-vnum"
              className="form-input"
              type="number"
              readOnly
              {...register("vnum", { valueAsNumber: true })}
            />
          </div>
          <div className="form-field form-field--wide">
            <label className="form-label" htmlFor="recipe-name">
              Name
            </label>
            <input
              id="recipe-name"
              className="form-input"
              type="text"
              {...register("name")}
            />
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="recipe-skill">
              Skill
            </label>
            <input
              id="recipe-skill"
              className="form-input"
              type="text"
              {...register("skill")}
            />
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="recipe-min-skill">
              Min Skill
            </label>
            <input
              id="recipe-min-skill"
              className="form-input"
              type="number"
              {...register("minSkill", { valueAsNumber: true })}
            />
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="recipe-min-skill-pct">
              Min Skill %
            </label>
            <input
              id="recipe-min-skill-pct"
              className="form-input"
              type="number"
              {...register("minSkillPct", { valueAsNumber: true })}
            />
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="recipe-min-level">
              Min Level
            </label>
            <input
              id="recipe-min-level"
              className="form-input"
              type="number"
              {...register("minLevel", { valueAsNumber: true })}
            />
          </div>
          <div className="form-field form-field--wide">
            <label className="form-label" htmlFor="recipe-station-type">
              Station Types
            </label>
            <select
              id="recipe-station-type"
              className="form-select"
              multiple
              {...register("stationType")}
            >
              {workstationTypeOptions.map((option) => (
                <option key={option} value={option}>
                  {option}
                </option>
              ))}
            </select>
            <div className="form-hint">Hold Ctrl/Cmd to select multiple.</div>
          </div>
          <VnumPicker<any>
            id="recipe-station-vnum"
            label="Station VNUM"
            name="stationVnum"
            register={register}
            control={control}
            options={objectVnumOptions}
            entityLabel="Object"
          />
          <div className="form-field">
            <label className="form-label" htmlFor="recipe-discovery">
              Discovery
            </label>
            <select
              id="recipe-discovery"
              className="form-select"
              {...register("discovery")}
            >
              <option value="">Default</option>
              {discoveryOptions.map((option) => (
                <option key={option} value={option}>
                  {option}
                </option>
              ))}
            </select>
          </div>
          <VnumPicker<any>
            id="recipe-output-vnum"
            label="Output VNUM"
            name="outputVnum"
            register={register}
            control={control}
            options={objectVnumOptions}
            entityLabel="Object"
            error={errors.outputVnum?.message}
          />
          <div className="form-field">
            <label className="form-label" htmlFor="recipe-output-qty">
              Output Qty
            </label>
            <input
              id="recipe-output-qty"
              className="form-input"
              type="number"
              {...register("outputQuantity", { valueAsNumber: true })}
            />
          </div>
        </div>

        <div className="form-field form-field--full">
          <div className="form-section-header">
            <div>
              <div className="form-label">Inputs</div>
              <div className="form-hint">
                {inputFields.length
                  ? `${inputFields.length} input${
                      inputFields.length === 1 ? "" : "s"
                    }`
                  : "No inputs yet"}
              </div>
            </div>
            <button
              className="ghost-button"
              type="button"
              onClick={() =>
                appendInput({
                  vnum: 0,
                  quantity: 1
                })
              }
            >
              Add Input
            </button>
          </div>
          <div className="block-list">
            {inputFields.map((field, index) => (
              <div className="block-card" key={field.id}>
                <div className="block-card__header">
                  <span>Input {index + 1}</span>
                  <div className="block-actions">
                    <button
                      className="ghost-button"
                      type="button"
                      onClick={() => moveInput(index, index - 1)}
                      disabled={index === 0}
                    >
                      Up
                    </button>
                    <button
                      className="ghost-button"
                      type="button"
                      onClick={() => moveInput(index, index + 1)}
                      disabled={index === inputFields.length - 1}
                    >
                      Down
                    </button>
                    <button
                      className="ghost-button"
                      type="button"
                      onClick={() => removeInput(index)}
                    >
                      Remove
                    </button>
                  </div>
                </div>
                <div className="block-grid">
                  <VnumPicker<any>
                    id={`recipe-input-vnum-${index}`}
                    label="Item VNUM"
                    name={`inputs.${index}.vnum`}
                    register={register}
                    control={control}
                    options={objectVnumOptions}
                    entityLabel="Object"
                    error={errors.inputs?.[index]?.vnum?.message}
                  />
                  <div className="form-field">
                    <label
                      className="form-label"
                      htmlFor={`recipe-input-qty-${index}`}
                    >
                      Quantity
                    </label>
                    <input
                      id={`recipe-input-qty-${index}`}
                      className="form-input"
                      type="number"
                      {...register(`inputs.${index}.quantity`, {
                        valueAsNumber: true
                      })}
                    />
                    {errors.inputs?.[index]?.quantity?.message ? (
                      <span className="form-error">
                        {errors.inputs[index].quantity.message}
                      </span>
                    ) : null}
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
            Save Recipe
          </button>
          <span className="form-hint">
            {formState.isDirty ? "Unsaved changes" : "Up to date"}
          </span>
        </div>
      </form>
    </div>
  );
}
