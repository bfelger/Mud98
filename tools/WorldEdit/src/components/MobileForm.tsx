import type { FormEventHandler } from "react";
import type { UseFormRegister } from "react-hook-form";
import { EventBindingsView } from "./EventBindingsView";
import type { EventBinding, EventEntityKey } from "../data/eventTypes";

type MobileFormProps = {
  onSubmit: FormEventHandler<HTMLFormElement>;
  register: UseFormRegister<any>;
  formState: {
    isDirty: boolean;
    errors: Record<string, { message?: string } | undefined>;
  };
  positions: string[];
  sexes: string[];
  sizes: string[];
  damageTypes: string[];
  mobActFlags: string[];
  attackFlags: string[];
  formFlagsOptions: string[];
  partFlagsOptions: string[];
  overrideFormFlags: boolean;
  overridePartFlags: boolean;
  onEnableFormFlagsOverride: () => void;
  onDisableFormFlagsOverride: () => void;
  onEnablePartFlagsOverride: () => void;
  onDisablePartFlagsOverride: () => void;
  raceOptions: string[];
  canEditScript: boolean;
  scriptEventEntity: EventEntityKey | null;
  eventBindings: EventBinding[];
  scriptValue: string;
  onEventBindingsChange: (events: EventBinding[]) => void;
  lootTableOptions: string[];
};

export function MobileForm({
  onSubmit,
  register,
  formState,
  positions,
  sexes,
  sizes,
  damageTypes,
  mobActFlags,
  attackFlags,
  formFlagsOptions,
  partFlagsOptions,
  overrideFormFlags,
  overridePartFlags,
  onEnableFormFlagsOverride,
  onDisableFormFlagsOverride,
  onEnablePartFlagsOverride,
  onDisablePartFlagsOverride,
  raceOptions,
  canEditScript,
  scriptEventEntity,
  eventBindings,
  scriptValue,
  onEventBindingsChange,
  lootTableOptions
}: MobileFormProps) {
  const errors = formState.errors as Record<string, any>;
  const lootListId = "mob-loot-table-options";

  return (
    <div className="form-view">
      <form className="form-shell" onSubmit={onSubmit}>
        <div className="form-grid">
          <div className="form-field">
            <label className="form-label" htmlFor="mob-vnum">
              VNUM
            </label>
            <input
              id="mob-vnum"
              className="form-input"
              type="number"
              readOnly
              {...register("vnum", { valueAsNumber: true })}
            />
          </div>
          <div className="form-field form-field--wide">
            <label className="form-label" htmlFor="mob-name">
              Name
            </label>
            <input
              id="mob-name"
              className="form-input"
              type="text"
              {...register("name")}
            />
            {errors.name?.message ? (
              <span className="form-error">{errors.name.message}</span>
            ) : null}
          </div>
          <div className="form-field form-field--wide">
            <label className="form-label" htmlFor="mob-short">
              Short Description
            </label>
            <input
              id="mob-short"
              className="form-input"
              type="text"
              {...register("shortDescr")}
            />
            {errors.shortDescr?.message ? (
              <span className="form-error">{errors.shortDescr.message}</span>
            ) : null}
          </div>
          <div className="form-field form-field--wide">
            <label className="form-label" htmlFor="mob-long">
              Long Description
            </label>
            <input
              id="mob-long"
              className="form-input"
              type="text"
              {...register("longDescr")}
            />
            {errors.longDescr?.message ? (
              <span className="form-error">{errors.longDescr.message}</span>
            ) : null}
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="mob-race">
              Race
            </label>
            <select id="mob-race" className="form-select" {...register("race")}>
              <option value="">Select</option>
              {raceOptions.map((race) => (
                <option key={race} value={race}>
                  {race}
                </option>
              ))}
            </select>
            {errors.race?.message ? (
              <span className="form-error">{errors.race.message}</span>
            ) : null}
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="mob-level">
              Level
            </label>
            <input
              id="mob-level"
              className="form-input"
              type="number"
              {...register("level")}
            />
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="mob-hitroll">
              Hitroll
            </label>
            <input
              id="mob-hitroll"
              className="form-input"
              type="number"
              {...register("hitroll")}
            />
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="mob-alignment">
              Alignment
            </label>
            <input
              id="mob-alignment"
              className="form-input"
              type="number"
              {...register("alignment")}
            />
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="mob-damtype">
              Damage Type
            </label>
            <select
              id="mob-damtype"
              className="form-select"
              {...register("damType")}
            >
              <option value="">Default</option>
              {damageTypes.map((damageType) => (
                <option key={damageType} value={damageType}>
                  {damageType}
                </option>
              ))}
            </select>
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="mob-startpos">
              Start Pos
            </label>
            <select
              id="mob-startpos"
              className="form-select"
              {...register("startPos")}
            >
              <option value="">Default</option>
              {positions.map((position) => (
                <option key={position} value={position}>
                  {position}
                </option>
              ))}
            </select>
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="mob-defaultpos">
              Default Pos
            </label>
            <select
              id="mob-defaultpos"
              className="form-select"
              {...register("defaultPos")}
            >
              <option value="">Default</option>
              {positions.map((position) => (
                <option key={position} value={position}>
                  {position}
                </option>
              ))}
            </select>
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="mob-sex">
              Sex
            </label>
            <select id="mob-sex" className="form-select" {...register("sex")}>
              <option value="">Default</option>
              {sexes.map((sex) => (
                <option key={sex} value={sex}>
                  {sex}
                </option>
              ))}
            </select>
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="mob-size">
              Size
            </label>
            <select id="mob-size" className="form-select" {...register("size")}>
              <option value="">Default</option>
              {sizes.map((size) => (
                <option key={size} value={size}>
                  {size}
                </option>
              ))}
            </select>
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="mob-material">
              Material
            </label>
            <input
              id="mob-material"
              className="form-input"
              type="text"
              {...register("material")}
            />
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="mob-faction">
              Faction VNUM
            </label>
            <input
              id="mob-faction"
              className="form-input"
              type="number"
              {...register("factionVnum")}
            />
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="mob-damage-noun">
              Damage Noun
            </label>
            <input
              id="mob-damage-noun"
              className="form-input"
              type="text"
              {...register("damageNoun")}
            />
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="mob-offensive-spell">
              Offensive Spell
            </label>
            <input
              id="mob-offensive-spell"
              className="form-input"
              type="text"
              {...register("offensiveSpell")}
            />
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="mob-loot-table">
              Loot Table
            </label>
            <input
              id="mob-loot-table"
              className="form-input"
              type="text"
              list={lootListId}
              {...register("lootTable")}
            />
            <datalist id={lootListId}>
              {lootTableOptions.map((option) => (
                <option key={option} value={option}>
                  {option}
                </option>
              ))}
            </datalist>
          </div>
        </div>
        <div className="form-field form-field--full">
          <label className="form-label" htmlFor="mob-description">
            Description
          </label>
          <textarea
            id="mob-description"
            className="form-textarea"
            rows={6}
            {...register("description")}
          />
          {errors.description?.message ? (
            <span className="form-error">{errors.description.message}</span>
          ) : null}
        </div>
        <div className="form-field form-field--full">
          <div className="form-section-header">
            <div>
              <div className="form-label">Act Flags</div>
              <div className="form-hint">Mobile behavior flags.</div>
            </div>
          </div>
          <div className="form-checkboxes">
            {mobActFlags.map((flag) => (
              <label key={flag} className="checkbox-pill">
                <input
                  type="checkbox"
                  value={flag}
                  {...register("actFlags")}
                />
                <span>{flag}</span>
              </label>
            ))}
          </div>
        </div>
        <div className="form-field form-field--full">
          <div className="form-section-header">
            <div>
              <div className="form-label">Attack Flags</div>
              <div className="form-hint">Combat abilities and assists.</div>
            </div>
          </div>
          <div className="form-checkboxes">
            {attackFlags.map((flag) => (
              <label key={flag} className="checkbox-pill">
                <input
                  type="checkbox"
                  value={flag}
                  {...register("atkFlags")}
                />
                <span>{flag}</span>
              </label>
            ))}
          </div>
        </div>
        <div className="form-field form-field--full">
          <div className="form-section-header">
            <div>
              <div className="form-label">Form Flags</div>
              <div className="form-hint">
                Only needed when overriding race defaults.
              </div>
            </div>
            <div className="form-header-actions">
              {overrideFormFlags ? (
                <button
                  className="ghost-button"
                  type="button"
                  onClick={onDisableFormFlagsOverride}
                >
                  Use Race Default
                </button>
              ) : (
                <button
                  className="ghost-button"
                  type="button"
                  onClick={onEnableFormFlagsOverride}
                >
                  Override Race
                </button>
              )}
            </div>
          </div>
          {overrideFormFlags ? (
            <div className="form-checkboxes">
              {formFlagsOptions.map((flag) => (
                <label key={flag} className="checkbox-pill">
                  <input
                    type="checkbox"
                    value={flag}
                    {...register("formFlags")}
                  />
                  <span>{flag}</span>
                </label>
              ))}
            </div>
          ) : (
            <div className="placeholder-block">
              <div className="placeholder-title">Using race defaults</div>
              <p>Enable override to edit per-mobile form flags.</p>
            </div>
          )}
        </div>
        <div className="form-field form-field--full">
          <div className="form-section-header">
            <div>
              <div className="form-label">Part Flags</div>
              <div className="form-hint">
                Only needed when overriding race defaults.
              </div>
            </div>
            <div className="form-header-actions">
              {overridePartFlags ? (
                <button
                  className="ghost-button"
                  type="button"
                  onClick={onDisablePartFlagsOverride}
                >
                  Use Race Default
                </button>
              ) : (
                <button
                  className="ghost-button"
                  type="button"
                  onClick={onEnablePartFlagsOverride}
                >
                  Override Race
                </button>
              )}
            </div>
          </div>
          {overridePartFlags ? (
            <div className="form-checkboxes">
              {partFlagsOptions.map((flag) => (
                <label key={flag} className="checkbox-pill">
                  <input
                    type="checkbox"
                    value={flag}
                    {...register("partFlags")}
                  />
                  <span>{flag}</span>
                </label>
              ))}
            </div>
          ) : (
            <div className="placeholder-block">
              <div className="placeholder-title">Using race defaults</div>
              <p>Enable override to edit per-mobile part flags.</p>
            </div>
          )}
        </div>
        <div className="form-field form-field--full">
          <div className="form-section-header">
            <div>
              <div className="form-label">Dice</div>
              <div className="form-hint">
                Number, type, and optional bonus.
              </div>
            </div>
          </div>
          <div className="dice-grid">
            <div className="dice-card">
              <div className="dice-card__title">Hit Dice</div>
              <div className="dice-fields">
                <div className="form-field">
                  <label className="form-label">Number</label>
                  <input
                    className="form-input"
                    type="number"
                    {...register("hitDice.number")}
                  />
                </div>
                <div className="form-field">
                  <label className="form-label">Type</label>
                  <input
                    className="form-input"
                    type="number"
                    {...register("hitDice.type")}
                  />
                </div>
                <div className="form-field">
                  <label className="form-label">Bonus</label>
                  <input
                    className="form-input"
                    type="number"
                    {...register("hitDice.bonus")}
                  />
                </div>
              </div>
            </div>
            <div className="dice-card">
              <div className="dice-card__title">Mana Dice</div>
              <div className="dice-fields">
                <div className="form-field">
                  <label className="form-label">Number</label>
                  <input
                    className="form-input"
                    type="number"
                    {...register("manaDice.number")}
                  />
                </div>
                <div className="form-field">
                  <label className="form-label">Type</label>
                  <input
                    className="form-input"
                    type="number"
                    {...register("manaDice.type")}
                  />
                </div>
                <div className="form-field">
                  <label className="form-label">Bonus</label>
                  <input
                    className="form-input"
                    type="number"
                    {...register("manaDice.bonus")}
                  />
                </div>
              </div>
            </div>
            <div className="dice-card">
              <div className="dice-card__title">Damage Dice</div>
              <div className="dice-fields">
                <div className="form-field">
                  <label className="form-label">Number</label>
                  <input
                    className="form-input"
                    type="number"
                    {...register("damageDice.number")}
                  />
                </div>
                <div className="form-field">
                  <label className="form-label">Type</label>
                  <input
                    className="form-input"
                    type="number"
                    {...register("damageDice.type")}
                  />
                </div>
                <div className="form-field">
                  <label className="form-label">Bonus</label>
                  <input
                    className="form-input"
                    type="number"
                    {...register("damageDice.bonus")}
                  />
                </div>
              </div>
            </div>
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
