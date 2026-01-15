import type { FormEventHandler } from "react";
import type { FieldPath, UseFormRegister } from "react-hook-form";
import { EventBindingsView } from "./EventBindingsView";
import { VnumPicker, type VnumOption } from "./VnumPicker";
import type { EventBinding, EventEntityKey } from "../data/eventTypes";

type RoomFormProps = {
  onSubmit: FormEventHandler<HTMLFormElement>;
  register: UseFormRegister<any>;
  formState: {
    isDirty: boolean;
    errors: Record<string, { message?: string } | undefined>;
  };
  exitFields: Array<{ id: string }>;
  appendExit: (value: Record<string, unknown>) => void;
  removeExit: (index: number) => void;
  directions: string[];
  sectors: string[];
  roomFlags: string[];
  exitFlags: string[];
  roomVnumOptions: VnumOption[];
  objectVnumOptions: VnumOption[];
  canEditScript: boolean;
  scriptEventEntity: EventEntityKey | null;
  eventBindings: EventBinding[];
  scriptValue: string;
  onEventBindingsChange: (events: EventBinding[]) => void;
};

export function RoomForm({
  onSubmit,
  register,
  formState,
  exitFields,
  appendExit,
  removeExit,
  directions,
  sectors,
  roomFlags,
  exitFlags,
  roomVnumOptions,
  objectVnumOptions,
  canEditScript,
  scriptEventEntity,
  eventBindings,
  scriptValue,
  onEventBindingsChange
}: RoomFormProps) {
  const errors = formState.errors as Record<string, any>;

  return (
    <div className="form-view">
      <form className="form-shell" onSubmit={onSubmit}>
        <div className="form-grid">
          <div className="form-field">
            <label className="form-label" htmlFor="room-vnum">
              VNUM
            </label>
            <input
              id="room-vnum"
              className="form-input"
              type="number"
              readOnly
              {...register("vnum", { valueAsNumber: true })}
            />
          </div>
          <div className="form-field form-field--wide">
            <label className="form-label" htmlFor="room-name">
              Name
            </label>
            <input
              id="room-name"
              className="form-input"
              type="text"
              {...register("name")}
            />
            {errors.name?.message ? (
              <span className="form-error">{errors.name.message}</span>
            ) : null}
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="room-sector">
              Sector
            </label>
            <select
              id="room-sector"
              className="form-select"
              {...register("sectorType")}
            >
              <option value="">Default (inside)</option>
              {sectors.map((sector) => (
                <option key={sector} value={sector}>
                  {sector}
                </option>
              ))}
            </select>
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="room-mana">
              Mana Rate
            </label>
            <input
              id="room-mana"
              className="form-input"
              type="number"
              {...register("manaRate")}
            />
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="room-heal">
              Heal Rate
            </label>
            <input
              id="room-heal"
              className="form-input"
              type="number"
              {...register("healRate")}
            />
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="room-clan">
              Clan
            </label>
            <input
              id="room-clan"
              className="form-input"
              type="number"
              {...register("clan")}
            />
          </div>
          <div className="form-field form-field--wide">
            <label className="form-label" htmlFor="room-owner">
              Owner
            </label>
            <input
              id="room-owner"
              className="form-input"
              type="text"
              {...register("owner")}
            />
          </div>
        </div>
        <div className="form-field form-field--full">
          <label className="form-label" htmlFor="room-description">
            Description
          </label>
          <textarea
            id="room-description"
            className="form-textarea"
            rows={6}
            {...register("description")}
          />
          {errors.description?.message ? (
            <span className="form-error">{errors.description.message}</span>
          ) : null}
        </div>
        <div className="form-field form-field--full">
          <label className="form-label">Room Flags</label>
          <div className="form-checkboxes">
            {roomFlags.map((flag) => (
              <label key={flag} className="checkbox-pill">
                <input
                  type="checkbox"
                  value={flag}
                  {...register("roomFlags")}
                />
                <span>{flag}</span>
              </label>
            ))}
          </div>
        </div>
        <div className="form-field form-field--full">
          <div className="form-section-header">
            <div>
              <div className="form-label">Exits</div>
              <div className="form-hint">
                {exitFields.length
                  ? `${exitFields.length} exit${
                      exitFields.length === 1 ? "" : "s"
                    }`
                  : "No exits defined"}
              </div>
            </div>
            <button
              className="ghost-button"
              type="button"
              onClick={() =>
                appendExit({
                  dir: directions[0],
                  toVnum: 0,
                  key: undefined,
                  flags: [],
                  description: "",
                  keyword: ""
                })
              }
            >
              Add Exit
            </button>
          </div>
          <div className="exit-list">
            {exitFields.map((field, index) => (
              <div key={field.id} className="exit-card">
                <div className="exit-card__header">
                  <span>Exit {index + 1}</span>
                  <button
                    className="ghost-button"
                    type="button"
                    onClick={() => removeExit(index)}
                  >
                    Remove
                  </button>
                </div>
                <div className="exit-grid">
                  <div className="form-field">
                    <label
                      className="form-label"
                      htmlFor={`exit-dir-${field.id}`}
                    >
                      Direction
                    </label>
                    <select
                      id={`exit-dir-${field.id}`}
                      className="form-select"
                      {...register(`exits.${index}.dir` as FieldPath<FieldValues>)}
                    >
                      {directions.map((dir) => (
                        <option key={dir} value={dir}>
                          {dir}
                        </option>
                      ))}
                    </select>
                  </div>
                  <VnumPicker<any>
                    id={`exit-to-${field.id}`}
                    label="To VNUM"
                    name={`exits.${index}.toVnum` as FieldPath<any>}
                    register={register}
                    options={roomVnumOptions}
                    error={errors.exits?.[index]?.toVnum?.message}
                  />
                  <VnumPicker<any>
                    id={`exit-key-${field.id}`}
                    label="Key VNUM"
                    name={`exits.${index}.key` as FieldPath<any>}
                    register={register}
                    options={objectVnumOptions}
                  />
                  <div className="form-field">
                    <label
                      className="form-label"
                      htmlFor={`exit-keyword-${field.id}`}
                    >
                      Keyword
                    </label>
                    <input
                      id={`exit-keyword-${field.id}`}
                      className="form-input"
                      type="text"
                      {...register(`exits.${index}.keyword` as FieldPath<any>)}
                    />
                  </div>
                  <div className="form-field form-field--full">
                    <label
                      className="form-label"
                      htmlFor={`exit-description-${field.id}`}
                    >
                      Description
                    </label>
                    <textarea
                      id={`exit-description-${field.id}`}
                      className="form-textarea form-textarea--exit"
                      rows={3}
                      {...register(
                        `exits.${index}.description` as FieldPath<any>
                      )}
                    />
                  </div>
                  <div className="form-field form-field--full">
                    <label className="form-label">Exit Flags</label>
                    <div className="form-checkboxes">
                      {exitFlags.map((flag) => (
                        <label key={flag} className="checkbox-pill">
                          <input
                            type="checkbox"
                            value={flag}
                            {...register(`exits.${index}.flags` as FieldPath<any>)}
                          />
                          <span>{flag}</span>
                        </label>
                      ))}
                    </div>
                  </div>
                </div>
              </div>
            ))}
          </div>
        </div>
        <div className="form-field form-field--full">
          <EventBindingsView
            entityType={canEditScript ? scriptEventEntity : null}
            events={eventBindings}
            script={scriptValue}
            canEdit={canEditScript}
            onChange={onEventBindingsChange}
          />
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
            {formState.isDirty ? "Unsaved changes" : "Up to date"}
          </span>
        </div>
      </form>
    </div>
  );
}
