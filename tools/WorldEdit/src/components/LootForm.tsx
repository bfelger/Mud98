import type { FormEventHandler } from "react";
import type { Control, UseFormRegister } from "react-hook-form";
import { VnumPicker, type VnumOption } from "./VnumPicker";

type LootFormProps = {
  onSubmit: FormEventHandler<HTMLFormElement>;
  register: UseFormRegister<any>;
  control: Control<any>;
  formState: {
    isDirty: boolean;
    errors: Record<string, { message?: string } | undefined>;
  };
  kind: "group" | "table";
  entryFields: Array<{ id: string }>;
  opFields: Array<{ id: string }>;
  appendEntry: (value: Record<string, unknown>) => void;
  removeEntry: (index: number) => void;
  moveEntry: (from: number, to: number) => void;
  appendOp: (value: Record<string, unknown>) => void;
  removeOp: (index: number) => void;
  moveOp: (from: number, to: number) => void;
  entryTypeOptions: string[];
  opTypeOptions: string[];
  vnumOptions: VnumOption[];
};

export function LootForm({
  onSubmit,
  register,
  control,
  formState,
  kind,
  entryFields,
  opFields,
  appendEntry,
  removeEntry,
  moveEntry,
  appendOp,
  removeOp,
  moveOp,
  entryTypeOptions,
  opTypeOptions,
  vnumOptions
}: LootFormProps) {
  const errors = formState.errors as Record<string, any>;

  return (
    <div className="form-view">
      <form className="form-shell" onSubmit={onSubmit}>
        <div className="form-grid">
          <div className="form-field">
            <label className="form-label" htmlFor="loot-kind">
              Kind
            </label>
            <input
              id="loot-kind"
              className="form-input"
              type="text"
              value={kind}
              readOnly
            />
          </div>
          <div className="form-field form-field--wide">
            <label className="form-label" htmlFor="loot-name">
              Name
            </label>
            <input
              id="loot-name"
              className="form-input"
              type="text"
              {...register("name")}
            />
            {errors.name?.message ? (
              <span className="form-error">{errors.name.message}</span>
            ) : null}
          </div>
          {kind === "group" ? (
            <div className="form-field">
              <label className="form-label" htmlFor="loot-rolls">
                Rolls
              </label>
              <input
                id="loot-rolls"
                className="form-input"
                type="number"
                {...register("rolls", { valueAsNumber: true })}
              />
            </div>
          ) : (
            <div className="form-field">
              <label className="form-label" htmlFor="loot-parent">
                Parent
              </label>
              <input
                id="loot-parent"
                className="form-input"
                type="text"
                {...register("parent")}
              />
            </div>
          )}
        </div>

        {kind === "group" ? (
          <div className="form-field form-field--full">
            <div className="form-section-header">
              <div>
                <div className="form-label">Entries</div>
                <div className="form-hint">
                  {entryFields.length
                    ? `${entryFields.length} entry${
                        entryFields.length === 1 ? "" : "ies"
                      }`
                    : "No entries yet"}
                </div>
              </div>
              <button
                className="ghost-button"
                type="button"
                onClick={() =>
                  appendEntry({
                    type: "item",
                    vnum: 0,
                    minQty: 1,
                    maxQty: 1,
                    weight: 100
                  })
                }
              >
                Add Entry
              </button>
            </div>
            <div className="block-list">
              {entryFields.map((field, index) => (
                <div className="block-card" key={field.id}>
                  <div className="block-card__header">
                    <span>Entry {index + 1}</span>
                    <div className="block-actions">
                      <button
                        className="ghost-button"
                        type="button"
                        onClick={() => moveEntry(index, index - 1)}
                        disabled={index === 0}
                      >
                        Up
                      </button>
                      <button
                        className="ghost-button"
                        type="button"
                        onClick={() => moveEntry(index, index + 1)}
                        disabled={index === entryFields.length - 1}
                      >
                        Down
                      </button>
                      <button
                        className="ghost-button"
                        type="button"
                        onClick={() => removeEntry(index)}
                      >
                        Remove
                      </button>
                    </div>
                  </div>
                  <div className="block-grid">
                    <div className="form-field">
                      <label
                        className="form-label"
                        htmlFor={`loot-entry-type-${index}`}
                      >
                        Type
                      </label>
                      <select
                        id={`loot-entry-type-${index}`}
                        className="form-select"
                        {...register(`entries.${index}.type`)}
                      >
                        {entryTypeOptions.map((option) => (
                          <option key={option} value={option}>
                            {option}
                          </option>
                        ))}
                      </select>
                    </div>
                    <VnumPicker<any>
                      id={`loot-entry-vnum-${index}`}
                      label="Vnum"
                      name={`entries.${index}.vnum`}
                      register={register}
                      control={control}
                      options={vnumOptions}
                      entityLabel="Object"
                    />
                    <div className="form-field">
                      <label
                        className="form-label"
                        htmlFor={`loot-entry-min-${index}`}
                      >
                        Min Qty
                      </label>
                      <input
                        id={`loot-entry-min-${index}`}
                        className="form-input"
                        type="number"
                        {...register(`entries.${index}.minQty`, {
                          valueAsNumber: true
                        })}
                      />
                    </div>
                    <div className="form-field">
                      <label
                        className="form-label"
                        htmlFor={`loot-entry-max-${index}`}
                      >
                        Max Qty
                      </label>
                      <input
                        id={`loot-entry-max-${index}`}
                        className="form-input"
                        type="number"
                        {...register(`entries.${index}.maxQty`, {
                          valueAsNumber: true
                        })}
                      />
                    </div>
                    <div className="form-field">
                      <label
                        className="form-label"
                        htmlFor={`loot-entry-weight-${index}`}
                      >
                        Weight
                      </label>
                      <input
                        id={`loot-entry-weight-${index}`}
                        className="form-input"
                        type="number"
                        {...register(`entries.${index}.weight`, {
                          valueAsNumber: true
                        })}
                      />
                    </div>
                  </div>
                </div>
              ))}
            </div>
          </div>
        ) : (
          <div className="form-field form-field--full">
            <div className="form-section-header">
              <div>
                <div className="form-label">Operations</div>
                <div className="form-hint">
                  {opFields.length
                    ? `${opFields.length} op${
                        opFields.length === 1 ? "" : "s"
                      }`
                    : "No operations yet"}
                </div>
              </div>
              <button
                className="ghost-button"
                type="button"
                onClick={() =>
                  appendOp({
                    op: "use_group",
                    group: "",
                    chance: 100
                  })
                }
              >
                Add Op
              </button>
            </div>
            <div className="block-list">
              {opFields.map((field, index) => (
                <div className="block-card" key={field.id}>
                  <div className="block-card__header">
                    <span>Op {index + 1}</span>
                    <div className="block-actions">
                      <button
                        className="ghost-button"
                        type="button"
                        onClick={() => moveOp(index, index - 1)}
                        disabled={index === 0}
                      >
                        Up
                      </button>
                      <button
                        className="ghost-button"
                        type="button"
                        onClick={() => moveOp(index, index + 1)}
                        disabled={index === opFields.length - 1}
                      >
                        Down
                      </button>
                      <button
                        className="ghost-button"
                        type="button"
                        onClick={() => removeOp(index)}
                      >
                        Remove
                      </button>
                    </div>
                  </div>
                  <div className="block-grid">
                    <div className="form-field">
                      <label
                        className="form-label"
                        htmlFor={`loot-op-type-${index}`}
                      >
                        Op
                      </label>
                      <select
                        id={`loot-op-type-${index}`}
                        className="form-select"
                        {...register(`ops.${index}.op`)}
                      >
                        {opTypeOptions.map((option) => (
                          <option key={option} value={option}>
                            {option}
                          </option>
                        ))}
                      </select>
                    </div>
                    <div className="form-field">
                      <label
                        className="form-label"
                        htmlFor={`loot-op-group-${index}`}
                      >
                        Group
                      </label>
                      <input
                        id={`loot-op-group-${index}`}
                        className="form-input"
                        type="text"
                        {...register(`ops.${index}.group`)}
                      />
                    </div>
                    <VnumPicker<any>
                      id={`loot-op-vnum-${index}`}
                      label="Vnum"
                      name={`ops.${index}.vnum`}
                      register={register}
                      control={control}
                      options={vnumOptions}
                      entityLabel="Object"
                    />
                    <div className="form-field">
                      <label
                        className="form-label"
                        htmlFor={`loot-op-chance-${index}`}
                      >
                        Chance
                      </label>
                      <input
                        id={`loot-op-chance-${index}`}
                        className="form-input"
                        type="number"
                        {...register(`ops.${index}.chance`, {
                          valueAsNumber: true
                        })}
                      />
                    </div>
                    <div className="form-field">
                      <label
                        className="form-label"
                        htmlFor={`loot-op-min-${index}`}
                      >
                        Min Qty
                      </label>
                      <input
                        id={`loot-op-min-${index}`}
                        className="form-input"
                        type="number"
                        {...register(`ops.${index}.minQty`, {
                          valueAsNumber: true
                        })}
                      />
                    </div>
                    <div className="form-field">
                      <label
                        className="form-label"
                        htmlFor={`loot-op-max-${index}`}
                      >
                        Max Qty
                      </label>
                      <input
                        id={`loot-op-max-${index}`}
                        className="form-input"
                        type="number"
                        {...register(`ops.${index}.maxQty`, {
                          valueAsNumber: true
                        })}
                      />
                    </div>
                    <div className="form-field">
                      <label
                        className="form-label"
                        htmlFor={`loot-op-mult-${index}`}
                      >
                        Multiplier
                      </label>
                      <input
                        id={`loot-op-mult-${index}`}
                        className="form-input"
                        type="number"
                        {...register(`ops.${index}.multiplier`, {
                          valueAsNumber: true
                        })}
                      />
                    </div>
                  </div>
                </div>
              ))}
            </div>
          </div>
        )}

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
