import type { FormEventHandler } from "react";
import type { UseFormRegister } from "react-hook-form";

type ClassFormProps = {
  onSubmit: FormEventHandler<HTMLFormElement>;
  register: UseFormRegister<any>;
  formState: {
    isDirty: boolean;
    errors: Record<string, { message?: string } | undefined>;
  };
  primeStatOptions: string[];
  armorProfOptions: string[];
  titleCount: number;
};

export function ClassForm({
  onSubmit,
  register,
  formState,
  primeStatOptions,
  armorProfOptions,
  titleCount
}: ClassFormProps) {
  return (
    <div className="form-view">
      <form className="form-shell" onSubmit={onSubmit}>
        <div className="form-grid">
          <div className="form-field form-field--wide">
            <label className="form-label" htmlFor="class-name">
              Class Name
            </label>
            <input
              id="class-name"
              className="form-input"
              type="text"
              {...register("name")}
            />
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="class-who">
              Who Name
            </label>
            <input
              id="class-who"
              className="form-input"
              type="text"
              {...register("whoName")}
            />
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="class-prime">
              Prime Stat
            </label>
            <select
              id="class-prime"
              className="form-select"
              {...register("primeStat")}
            >
              <option value="">Default</option>
              {primeStatOptions.map((stat) => (
                <option key={stat} value={stat}>
                  {stat}
                </option>
              ))}
            </select>
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="class-armor">
              Armor Prof
            </label>
            <select
              id="class-armor"
              className="form-select"
              {...register("armorProf")}
            >
              <option value="">Default</option>
              {armorProfOptions.map((armor) => (
                <option key={armor} value={armor}>
                  {armor}
                </option>
              ))}
            </select>
          </div>
          <div className="form-field form-field--wide">
            <label className="form-label" htmlFor="class-base-group">
              Base Group
            </label>
            <input
              id="class-base-group"
              className="form-input"
              type="text"
              {...register("baseGroup")}
            />
          </div>
          <div className="form-field form-field--wide">
            <label className="form-label" htmlFor="class-default-group">
              Default Group
            </label>
            <input
              id="class-default-group"
              className="form-input"
              type="text"
              {...register("defaultGroup")}
            />
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="class-weapon">
              Weapon VNUM
            </label>
            <input
              id="class-weapon"
              className="form-input"
              type="number"
              {...register("weaponVnum", { valueAsNumber: true })}
            />
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="class-start-loc">
              Start Loc
            </label>
            <input
              id="class-start-loc"
              className="form-input"
              type="number"
              {...register("startLoc", { valueAsNumber: true })}
            />
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="class-guild-1">
              Guild VNUM 1
            </label>
            <input
              id="class-guild-1"
              className="form-input"
              type="number"
              {...register("guilds.0", { valueAsNumber: true })}
            />
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="class-guild-2">
              Guild VNUM 2
            </label>
            <input
              id="class-guild-2"
              className="form-input"
              type="number"
              {...register("guilds.1", { valueAsNumber: true })}
            />
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="class-skill-cap">
              Skill Cap
            </label>
            <input
              id="class-skill-cap"
              className="form-input"
              type="number"
              {...register("skillCap", { valueAsNumber: true })}
            />
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="class-thac0-00">
              Thac0 (00)
            </label>
            <input
              id="class-thac0-00"
              className="form-input"
              type="number"
              {...register("thac0_00", { valueAsNumber: true })}
            />
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="class-thac0-32">
              Thac0 (32)
            </label>
            <input
              id="class-thac0-32"
              className="form-input"
              type="number"
              {...register("thac0_32", { valueAsNumber: true })}
            />
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="class-hp-min">
              HP Min
            </label>
            <input
              id="class-hp-min"
              className="form-input"
              type="number"
              {...register("hpMin", { valueAsNumber: true })}
            />
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="class-hp-max">
              HP Max
            </label>
            <input
              id="class-hp-max"
              className="form-input"
              type="number"
              {...register("hpMax", { valueAsNumber: true })}
            />
          </div>
          <div className="form-field">
            <label className="form-label">Mana User</label>
            <label className="checkbox-pill">
              <input type="checkbox" {...register("manaUser")} />
              <span>Enabled</span>
            </label>
          </div>
          <div className="form-field form-field--full">
            <label className="form-label" htmlFor="class-titles-male">
              Titles (Male)
            </label>
            <textarea
              id="class-titles-male"
              className="form-textarea"
              rows={8}
              placeholder="One title per line."
              {...register("titlesMale")}
            />
          </div>
          <div className="form-field form-field--full">
            <label className="form-label" htmlFor="class-titles-female">
              Titles (Female)
            </label>
            <textarea
              id="class-titles-female"
              className="form-textarea"
              rows={8}
              placeholder="One title per line."
              {...register("titlesFemale")}
            />
            <span className="form-hint">
              Lines map to levels 0-{titleCount - 1}.
            </span>
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
            {formState.isDirty ? "Unsaved changes" : "Up to date"}
          </span>
        </div>
      </form>
    </div>
  );
}
