import type { Control, FieldPath, UseFormRegister } from "react-hook-form";
import { VnumPicker, type VnumOption } from "./VnumPicker";

type ResetEditorFieldsProps = {
  register: UseFormRegister<any>;
  control: Control<any>;
  errors: Record<string, { message?: string } | undefined>;
  activeResetCommand: string;
  resetCommandOptions: string[];
  resetCommandLabels: Record<string, string>;
  directions: string[];
  wearLocations: string[];
  roomVnumOptions: VnumOption[];
  objectVnumOptions: VnumOption[];
  mobileVnumOptions: VnumOption[];
  showIndex?: boolean;
  idPrefix?: string;
};

export function ResetEditorFields({
  register,
  control,
  errors,
  activeResetCommand,
  resetCommandOptions,
  resetCommandLabels,
  directions,
  wearLocations,
  roomVnumOptions,
  objectVnumOptions,
  mobileVnumOptions,
  showIndex = true,
  idPrefix = "reset"
}: ResetEditorFieldsProps) {
  return (
    <>
      <div className="form-grid">
        {showIndex ? (
          <div className="form-field">
            <label className="form-label" htmlFor={`${idPrefix}-index`}>
              Index
            </label>
            <input
              id={`${idPrefix}-index`}
              className="form-input"
              type="number"
              readOnly
              {...register("index", { valueAsNumber: true })}
            />
          </div>
        ) : null}
        <div className="form-field form-field--wide">
          <label className="form-label" htmlFor={`${idPrefix}-command`}>
            Command
          </label>
          <select
            id={`${idPrefix}-command`}
            className="form-select"
            {...register("commandName")}
          >
            <option value="">Select</option>
            {activeResetCommand &&
            !resetCommandOptions.includes(activeResetCommand) ? (
              <option value={activeResetCommand}>{activeResetCommand}</option>
            ) : null}
            {resetCommandOptions.map((command) => (
              <option key={command} value={command}>
                {command}
              </option>
            ))}
          </select>
          {errors.commandName ? (
            <span className="form-error">{errors.commandName.message}</span>
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
          {!resetCommandOptions.includes(activeResetCommand) ? (
            <div className="placeholder-block">
              <div className="placeholder-title">No reset selected</div>
              <p>Choose a command to edit reset-specific fields.</p>
            </div>
          ) : null}
          {activeResetCommand === "loadMob" ? (
            <div className="block-card">
              <div className="block-card__header">
                <span>Load Mobile</span>
              </div>
              <div className="block-grid">
                <VnumPicker<any>
                  id={`${idPrefix}-load-mob`}
                  label="Mob VNUM"
                  name={"mobVnum" as FieldPath<any>}
                  register={register}
                  control={control}
                  options={mobileVnumOptions}
                  error={(errors as any).mobVnum?.message}
                />
                <VnumPicker<any>
                  id={`${idPrefix}-load-room`}
                  label="Room VNUM"
                  name={"roomVnum" as FieldPath<any>}
                  register={register}
                  control={control}
                  options={roomVnumOptions}
                  error={(errors as any).roomVnum?.message}
                />
                <div className="form-field">
                  <label className="form-label">Max In Area</label>
                  <input
                    className="form-input"
                    type="number"
                    {...register("maxInArea")}
                  />
                </div>
                <div className="form-field">
                  <label className="form-label">Max In Room</label>
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
                  id={`${idPrefix}-place-obj`}
                  label="Object VNUM"
                  name={"objVnum" as FieldPath<any>}
                  register={register}
                  control={control}
                  options={objectVnumOptions}
                  error={(errors as any).objVnum?.message}
                />
                <VnumPicker<any>
                  id={`${idPrefix}-place-room`}
                  label="Room VNUM"
                  name={"roomVnum" as FieldPath<any>}
                  register={register}
                  control={control}
                  options={roomVnumOptions}
                  error={(errors as any).roomVnum?.message}
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
                  id={`${idPrefix}-put-obj`}
                  label="Object VNUM"
                  name={"objVnum" as FieldPath<any>}
                  register={register}
                  control={control}
                  options={objectVnumOptions}
                  error={(errors as any).objVnum?.message}
                />
                <VnumPicker<any>
                  id={`${idPrefix}-put-container`}
                  label="Container VNUM"
                  name={"containerVnum" as FieldPath<any>}
                  register={register}
                  control={control}
                  options={objectVnumOptions}
                  error={(errors as any).containerVnum?.message}
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
                  <label className="form-label">Max In Container</label>
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
                  id={`${idPrefix}-give-obj`}
                  label="Object VNUM"
                  name={"objVnum" as FieldPath<any>}
                  register={register}
                  control={control}
                  options={objectVnumOptions}
                  error={(errors as any).objVnum?.message}
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
                  id={`${idPrefix}-equip-obj`}
                  label="Object VNUM"
                  name={"objVnum" as FieldPath<any>}
                  register={register}
                  control={control}
                  options={objectVnumOptions}
                  error={(errors as any).objVnum?.message}
                />
                <div className="form-field">
                  <label className="form-label">Wear Location</label>
                  <select className="form-select" {...register("wearLoc")}>
                    <option value="">Select</option>
                    {wearLocations.map((loc) => (
                      <option key={loc} value={loc}>
                        {loc}
                      </option>
                    ))}
                  </select>
                  {(errors as any).wearLoc ? (
                    <span className="form-error">
                      {(errors as any).wearLoc.message}
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
                  id={`${idPrefix}-door-room`}
                  label="Room VNUM"
                  name={"roomVnum" as FieldPath<any>}
                  register={register}
                  control={control}
                  options={roomVnumOptions}
                  error={(errors as any).roomVnum?.message}
                />
                <div className="form-field">
                  <label className="form-label">Direction</label>
                  <select className="form-select" {...register("direction")}>
                    <option value="">Select</option>
                    {directions.map((dir) => (
                      <option key={dir} value={dir}>
                        {dir}
                      </option>
                    ))}
                  </select>
                  {(errors as any).direction ? (
                    <span className="form-error">
                      {(errors as any).direction.message}
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
                  {(errors as any).state ? (
                    <span className="form-error">
                      {(errors as any).state.message}
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
                  id={`${idPrefix}-random-room`}
                  label="Room VNUM"
                  name={"roomVnum" as FieldPath<any>}
                  register={register}
                  control={control}
                  options={roomVnumOptions}
                  error={(errors as any).roomVnum?.message}
                />
                <div className="form-field">
                  <label className="form-label">Exit Count</label>
                  <input
                    className="form-input"
                    type="number"
                    {...register("exits")}
                  />
                  {(errors as any).exits ? (
                    <span className="form-error">
                      {(errors as any).exits.message}
                    </span>
                  ) : null}
                </div>
              </div>
            </div>
          ) : null}
        </div>
      </div>
    </>
  );
}
