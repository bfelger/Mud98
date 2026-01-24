import type { FormEventHandler } from "react";
import type { Control, FieldPath, UseFormRegister } from "react-hook-form";
import { VnumPicker, type VnumOption } from "./VnumPicker";

type QuestFormProps = {
  onSubmit: FormEventHandler<HTMLFormElement>;
  register: UseFormRegister<any>;
  control: Control<any>;
  formState: {
    isDirty: boolean;
    errors: Record<string, { message?: string } | undefined>;
  };
  questTypeOptions: string[];
  objectVnumOptions: VnumOption[];
};

export function QuestForm({
  onSubmit,
  register,
  control,
  formState,
  questTypeOptions,
  objectVnumOptions
}: QuestFormProps) {
  const errors = formState.errors as Record<string, any>;

  return (
    <div className="form-view">
      <form className="form-shell" onSubmit={onSubmit}>
        <div className="form-grid">
          <div className="form-field">
            <label className="form-label" htmlFor="quest-vnum">
              VNUM
            </label>
            <input
              id="quest-vnum"
              className="form-input"
              type="number"
              readOnly
              {...register("vnum", { valueAsNumber: true })}
            />
          </div>
          <div className="form-field form-field--wide">
            <label className="form-label" htmlFor="quest-name">
              Name
            </label>
            <input
              id="quest-name"
              className="form-input"
              type="text"
              {...register("name")}
            />
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="quest-type">
              Type
            </label>
            <select
              id="quest-type"
              className="form-select"
              {...register("type")}
            >
              <option value="">Select</option>
              {questTypeOptions.map((type) => (
                <option key={type} value={type}>
                  {type}
                </option>
              ))}
            </select>
          </div>
        </div>
        <div className="form-field form-field--full">
          <label className="form-label" htmlFor="quest-entry">
            Entry
          </label>
          <textarea
            id="quest-entry"
            className="form-textarea"
            rows={4}
            {...register("entry")}
          />
        </div>
        <div className="form-grid">
          <div className="form-field">
            <label className="form-label" htmlFor="quest-xp">
              XP
            </label>
            <input
              id="quest-xp"
              className="form-input"
              type="number"
              {...register("xp")}
            />
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="quest-level">
              Level
            </label>
            <input
              id="quest-level"
              className="form-input"
              type="number"
              {...register("level")}
            />
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="quest-count">
              Count
            </label>
            <input
              id="quest-count"
              className="form-input"
              type="number"
              {...register("count")}
            />
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="quest-target">
              Target VNUM
            </label>
            <input
              id="quest-target"
              className="form-input"
              type="number"
              {...register("target")}
            />
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="quest-upper">
              Target Upper
            </label>
            <input
              id="quest-upper"
              className="form-input"
              type="number"
              {...register("upper")}
            />
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="quest-end">
              End VNUM
            </label>
            <input
              id="quest-end"
              className="form-input"
              type="number"
              {...register("end")}
            />
          </div>
        </div>
        <div className="form-field form-field--full">
          <div className="form-section-header">
            <div>
              <div className="form-label">Rewards</div>
              <div className="form-hint">Gold, reputation, and item rewards.</div>
            </div>
          </div>
          <div className="block-list">
            <div className="block-card">
              <div className="block-card__header">
                <span>Currency + Reputation</span>
              </div>
              <div className="block-grid">
                <div className="form-field">
                  <label className="form-label" htmlFor="quest-reward-gold">
                    Gold
                  </label>
                  <input
                    id="quest-reward-gold"
                    className="form-input"
                    type="number"
                    {...register("rewardGold")}
                  />
                </div>
                <div className="form-field">
                  <label className="form-label" htmlFor="quest-reward-silver">
                    Silver
                  </label>
                  <input
                    id="quest-reward-silver"
                    className="form-input"
                    type="number"
                    {...register("rewardSilver")}
                  />
                </div>
                <div className="form-field">
                  <label className="form-label" htmlFor="quest-reward-copper">
                    Copper
                  </label>
                  <input
                    id="quest-reward-copper"
                    className="form-input"
                    type="number"
                    {...register("rewardCopper")}
                  />
                </div>
                <div className="form-field">
                  <label className="form-label" htmlFor="quest-reward-faction">
                    Faction VNUM
                  </label>
                  <input
                    id="quest-reward-faction"
                    className="form-input"
                    type="number"
                    {...register("rewardFaction")}
                  />
                </div>
                <div className="form-field">
                  <label className="form-label" htmlFor="quest-reward-rep">
                    Reputation
                  </label>
                  <input
                    id="quest-reward-rep"
                    className="form-input"
                    type="number"
                    {...register("rewardReputation")}
                  />
                </div>
              </div>
            </div>
            <div className="block-card">
              <div className="block-card__header">
                <span>Reward Items</span>
              </div>
              <div className="block-grid">
                {Array.from({ length: 3 }).map((_, index) => (
                  <VnumPicker<any>
                    key={`quest-reward-obj-${index}`}
                    id={`quest-reward-obj-${index}`}
                    label={`Item ${index + 1}`}
                    name={`rewardObjs.${index}` as FieldPath<any>}
                    register={register}
                    control={control}
                    options={objectVnumOptions}
                    error={errors.rewardObjs?.[index]?.message}
                  />
                ))}
                {Array.from({ length: 3 }).map((_, index) => (
                  <div className="form-field" key={`quest-reward-count-${index}`}>
                    <label
                      className="form-label"
                      htmlFor={`quest-reward-count-${index}`}
                    >
                      Count {index + 1}
                    </label>
                    <input
                      id={`quest-reward-count-${index}`}
                      className="form-input"
                      type="number"
                      {...register(`rewardCounts.${index}` as FieldPath<any>)}
                    />
                  </div>
                ))}
              </div>
            </div>
          </div>
        </div>
        <div className="form-actions">
          <button className="action-button" type="submit">
            Save Quest
          </button>
        </div>
      </form>
    </div>
  );
}
