import type { FormEventHandler } from "react";
import type { FieldPath, UseFormRegister } from "react-hook-form";
import { VnumPicker, type VnumOption } from "./VnumPicker";

type ResetFormProps = {
  onSubmit: FormEventHandler<HTMLFormElement>;
  register: UseFormRegister<any>;
  formState: {
    isDirty: boolean;
    errors: Record<string, { message?: string } | undefined>;
  };
  activeResetCommand: string;
  resetCommandOptions: string[];
  resetCommandLabels: Record<string, string>;
  directions: string[];
  wearLocations: string[];
  roomVnumOptions: VnumOption[];
  objectVnumOptions: VnumOption[];
  mobileVnumOptions: VnumOption[];
};

export function ResetForm({
  onSubmit,
  register,
  formState,
  activeResetCommand,
  resetCommandOptions,
  resetCommandLabels,
  directions,
  wearLocations,
  roomVnumOptions,
  objectVnumOptions,
  mobileVnumOptions
}: ResetFormProps) {
  const errors = formState.errors as Record<string, any>;

  return (
    <div className="form-view">
      <form className="form-shell" onSubmit={onSubmit}>
        <div className="form-grid">
          <div className="form-field">
            <label className="form-label" htmlFor="reset-index">
              Index
            </label>
            <input
              id="reset-index"
              className="form-input"
              type="number"
              readOnly
              {...register("index", { valueAsNumber: true })}
            />
          </div>
          <div className="form-field form-field--wide">
            <label className="form-label" htmlFor="reset-command">
              Command
            </label>
            <select
              id="reset-command"
              className="form-select"
              {...register("commandName")}
            >
              <option value="">Select</option>
              {activeResetCommand &&
              !resetCommandOptions.includes(
                activeResetCommand
              ) ? (
                <option value={activeResetCommand}>
                  {activeResetCommand}
                </option>
              ) : null}
              {resetCommandOptions.map((command) => (
                <option key={command} value={command}>
                  {command}
                </option>
              ))}
            </select>
            {errors.commandName ? (
              <span className="form-error">
                {errors.commandName.message}
              </span>
            ) : null}
          </div>
        </div>
        <div className="form-field form-field--full">
          <div className="form-section-header">
            <div>
              <div className="form-label">Reset Details</div>
              <div className="form-hint">
                {resetCommandOptions.includes(activeResetCommand)
                  ? resetCommandLabels[activeResetCommand] ?? ""
                  : "Select a reset command."}
              </div>
            </div>
          </div>
          <div className="block-list">
            {!resetCommandOptions.includes(
              activeResetCommand
            ) ? (
              <div className="placeholder-block">
                <div className="placeholder-title">
                  No reset selected
                </div>
                <p>
                  Choose a command to edit reset-specific
                  fields.
                </p>
              </div>
            ) : null}
            {activeResetCommand === "loadMob" ? (
              <div className="block-card">
                <div className="block-card__header">
                  <span>Load Mobile</span>
                </div>
                <div className="block-grid">
                  <VnumPicker<any>
                    id="reset-load-mob"
                    label="Mob VNUM"
                    name={"mobVnum" as FieldPath<any>}
                    register={register}
                    options={mobileVnumOptions}
                    error={errors.mobVnum?.message}
                  />
                  <VnumPicker<any>
                    id="reset-load-room"
                    label="Room VNUM"
                    name={"roomVnum" as FieldPath<any>}
                    register={register}
                    options={roomVnumOptions}
                    error={errors.roomVnum?.message}
                  />
                  <div className="form-field">
                    <label className="form-label">
                      Max In Area
                    </label>
                    <input
                      className="form-input"
                      type="number"
                      {...register("maxInArea")}
                    />
                  </div>
                  <div className="form-field">
                    <label className="form-label">
                      Max In Room
                    </label>
                    <input
                      className="form-input"
                      type="number"
                      {...register("maxInRoom")}
                    />
                  </div>
                </div>
              </div>
            ) : null}
            {activeResetCommand === "placeObj" ? (
              <div className="block-card">
                <div className="block-card__header">
                  <span>Place Object</span>
                </div>
                <div className="block-grid">
                  <VnumPicker<any>
                    id="reset-place-obj"
                    label="Object VNUM"
                    name={"objVnum" as FieldPath<any>}
                    register={register}
                    options={objectVnumOptions}
                    error={errors.objVnum?.message}
                  />
                  <VnumPicker<any>
                    id="reset-place-room"
                    label="Room VNUM"
                    name={"roomVnum" as FieldPath<any>}
                    register={register}
                    options={roomVnumOptions}
                    error={errors.roomVnum?.message}
                  />
                </div>
              </div>
            ) : null}
            {activeResetCommand === "putObj" ? (
              <div className="block-card">
                <div className="block-card__header">
                  <span>Put Object</span>
                </div>
                <div className="block-grid">
                  <VnumPicker<any>
                    id="reset-put-obj"
                    label="Object VNUM"
                    name={"objVnum" as FieldPath<any>}
                    register={register}
                    options={objectVnumOptions}
                    error={errors.objVnum?.message}
                  />
                  <VnumPicker<any>
                    id="reset-put-container"
                    label="Container VNUM"
                    name={
                      "containerVnum" as FieldPath<any>
                    }
                    register={register}
                    options={objectVnumOptions}
                    error={
                      errors.containerVnum?.message
                    }
                  />
                  <div className="form-field">
                    <label className="form-label">Count</label>
                    <input
                      className="form-input"
                      type="number"
                      {...register("count")}
                    />
                  </div>
                  <div className="form-field">
                    <label className="form-label">
                      Max In Container
                    </label>
                    <input
                      className="form-input"
                      type="number"
                      {...register("maxInContainer")}
                    />
                  </div>
                </div>
              </div>
            ) : null}
            {activeResetCommand === "giveObj" ? (
              <div className="block-card">
                <div className="block-card__header">
                  <span>Give Object</span>
                </div>
                <div className="block-grid">
                  <VnumPicker<any>
                    id="reset-give-obj"
                    label="Object VNUM"
                    name={"objVnum" as FieldPath<any>}
                    register={register}
                    options={objectVnumOptions}
                    error={errors.objVnum?.message}
                  />
                </div>
              </div>
            ) : null}
            {activeResetCommand === "equipObj" ? (
              <div className="block-card">
                <div className="block-card__header">
                  <span>Equip Object</span>
                </div>
                <div className="block-grid">
                  <VnumPicker<any>
                    id="reset-equip-obj"
                    label="Object VNUM"
                    name={"objVnum" as FieldPath<any>}
                    register={register}
                    options={objectVnumOptions}
                    error={errors.objVnum?.message}
                  />
                  <div className="form-field">
                    <label className="form-label">
                      Wear Location
                    </label>
                    <select
                      className="form-select"
                      {...register("wearLoc")}
                    >
                      <option value="">Select</option>
                      {wearLocations.map((loc) => (
                        <option key={loc} value={loc}>
                          {loc}
                        </option>
                      ))}
                    </select>
                    {errors.wearLoc ? (
                      <span className="form-error">
                        {errors.wearLoc.message}
                      </span>
                    ) : null}
                  </div>
                </div>
              </div>
            ) : null}
            {activeResetCommand === "setDoor" ? (
              <div className="block-card">
                <div className="block-card__header">
                  <span>Set Door</span>
                </div>
                <div className="block-grid">
                  <VnumPicker<any>
                    id="reset-door-room"
                    label="Room VNUM"
                    name={"roomVnum" as FieldPath<any>}
                    register={register}
                    options={roomVnumOptions}
                    error={errors.roomVnum?.message}
                  />
                  <div className="form-field">
                    <label className="form-label">
                      Direction
                    </label>
                    <select
                      className="form-select"
                      {...register("direction")}
                    >
                      <option value="">Select</option>
                      {directions.map((dir) => (
                        <option key={dir} value={dir}>
                          {dir}
                        </option>
                      ))}
                    </select>
                    {errors.direction ? (
                      <span className="form-error">
                        {
                          errors.direction.message
                        }
                      </span>
                    ) : null}
                  </div>
                  <div className="form-field">
                    <label className="form-label">State</label>
                    <input
                      className="form-input"
                      type="number"
                      {...register("state")}
                    />
                    {errors.state ? (
                      <span className="form-error">
                        {errors.state.message}
                      </span>
                    ) : null}
                  </div>
                </div>
              </div>
            ) : null}
            {activeResetCommand === "randomizeExits" ? (
              <div className="block-card">
                <div className="block-card__header">
                  <span>Randomize Exits</span>
                </div>
                <div className="block-grid">
                  <VnumPicker<any>
                    id="reset-random-room"
                    label="Room VNUM"
                    name={"roomVnum" as FieldPath<any>}
                    register={register}
                    options={roomVnumOptions}
                    error={errors.roomVnum?.message}
                  />
                  <div className="form-field">
                    <label className="form-label">
                      Exit Count
                    </label>
                    <input
                      className="form-input"
                      type="number"
                      {...register("exits")}
                    />
                    {errors.exits ? (
                      <span className="form-error">
                        {errors.exits.message}
                      </span>
                    ) : null}
                  </div>
                </div>
              </div>
            ) : null}
          </div>
        </div>
        <div className="form-actions">
          <button
            className="action-button action-button--primary"
            type="submit"
            disabled={!formState.isDirty}
          >
            Apply Changes
          </button>
          <span className="form-hint">
            {formState.isDirty
              ? "Unsaved changes"
              : "Up to date"}
          </span>
        </div>
      </form>
    </div>
  );
}
