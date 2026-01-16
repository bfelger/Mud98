import type { FormEventHandler } from "react";
import type { FieldPath, UseFormRegister } from "react-hook-form";
import { EventBindingsView } from "./EventBindingsView";
import { VnumPicker, type VnumOption } from "./VnumPicker";
import type { EventBinding, EventEntityKey } from "../data/eventTypes";

type ObjectFormProps = {
  onSubmit: FormEventHandler<HTMLFormElement>;
  register: UseFormRegister<any>;
  formState: {
    isDirty: boolean;
    errors: Record<string, { message?: string } | undefined>;
  };
  itemTypeOptions: string[];
  wearFlags: string[];
  extraFlags: string[];
  weaponClasses: string[];
  weaponFlags: string[];
  damageTypes: string[];
  liquids: string[];
  exitFlags: string[];
  portalFlags: string[];
  containerFlags: string[];
  furnitureFlags: string[];
  activeObjectBlock: string | null;
  objectVnumOptions: VnumOption[];
  roomVnumOptions: VnumOption[];
  canEditScript: boolean;
  scriptEventEntity: EventEntityKey | null;
  eventBindings: EventBinding[];
  scriptValue: string;
  onEventBindingsChange: (events: EventBinding[]) => void;
};

export function ObjectForm({
  onSubmit,
  register,
  formState,
  itemTypeOptions,
  wearFlags,
  extraFlags,
  weaponClasses,
  weaponFlags,
  damageTypes,
  liquids,
  exitFlags,
  portalFlags,
  containerFlags,
  furnitureFlags,
  activeObjectBlock,
  objectVnumOptions,
  roomVnumOptions,
  canEditScript,
  scriptEventEntity,
  eventBindings,
  scriptValue,
  onEventBindingsChange
}: ObjectFormProps) {
  const errors = formState.errors as Record<string, any>;

  return (
    <div className="form-view">
      <form className="form-shell" onSubmit={onSubmit}>
        <div className="form-grid">
          <div className="form-field">
            <label className="form-label" htmlFor="obj-vnum">
              VNUM
            </label>
            <input
              id="obj-vnum"
              className="form-input"
              type="number"
              readOnly
              {...register("vnum", { valueAsNumber: true })}
            />
          </div>
          <div className="form-field form-field--wide">
            <label className="form-label" htmlFor="obj-name">
              Name
            </label>
            <input
              id="obj-name"
              className="form-input"
              type="text"
              {...register("name")}
            />
            {errors.name ? (
              <span className="form-error">
                {errors.name.message}
              </span>
            ) : null}
          </div>
          <div className="form-field form-field--wide">
            <label className="form-label" htmlFor="obj-short">
              Short Description
            </label>
            <input
              id="obj-short"
              className="form-input"
              type="text"
              {...register("shortDescr")}
            />
            {errors.shortDescr ? (
              <span className="form-error">
                {errors.shortDescr.message}
              </span>
            ) : null}
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="obj-itemtype">
              Item Type
            </label>
            <select
              id="obj-itemtype"
              className="form-select"
              {...register("itemType")}
            >
              <option value="">Select</option>
              {itemTypeOptions.map((itemType) => (
                <option key={itemType} value={itemType}>
                  {itemType}
                </option>
              ))}
            </select>
            {errors.itemType ? (
              <span className="form-error">
                {errors.itemType.message}
              </span>
            ) : null}
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="obj-material">
              Material
            </label>
            <input
              id="obj-material"
              className="form-input"
              type="text"
              {...register("material")}
            />
            {errors.material ? (
              <span className="form-error">
                {errors.material.message}
              </span>
            ) : null}
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="obj-level">
              Level
            </label>
            <input
              id="obj-level"
              className="form-input"
              type="number"
              {...register("level")}
            />
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="obj-weight">
              Weight
            </label>
            <input
              id="obj-weight"
              className="form-input"
              type="number"
              {...register("weight")}
            />
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="obj-cost">
              Cost
            </label>
            <input
              id="obj-cost"
              className="form-input"
              type="number"
              {...register("cost")}
            />
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="obj-condition">
              Condition
            </label>
            <input
              id="obj-condition"
              className="form-input"
              type="number"
              {...register("condition")}
            />
          </div>
        </div>
        <div className="form-field form-field--full">
          <label
            className="form-label"
            htmlFor="obj-description"
          >
            Description
          </label>
          <textarea
            id="obj-description"
            className="form-textarea"
            rows={6}
            {...register("description")}
          />
          {errors.description ? (
            <span className="form-error">
              {errors.description.message}
            </span>
          ) : null}
        </div>
        <div className="form-field form-field--full">
          <label className="form-label">Wear Flags</label>
          <div className="form-checkboxes">
            {wearFlags.map((flag) => (
              <label key={flag} className="checkbox-pill">
                <input
                  type="checkbox"
                  value={flag}
                  {...register("wearFlags")}
                />
                <span>{flag}</span>
              </label>
            ))}
          </div>
        </div>
        <div className="form-field form-field--full">
          <label className="form-label">Extra Flags</label>
          <div className="form-checkboxes">
            {extraFlags.map((flag) => (
              <label key={flag} className="checkbox-pill">
                <input
                  type="checkbox"
                  value={flag}
                  {...register("extraFlags")}
                />
                <span>{flag}</span>
              </label>
            ))}
          </div>
        </div>
        <div className="form-field form-field--full">
          <div className="form-section-header">
            <div>
              <div className="form-label">Typed Block</div>
              <div className="form-hint">
                Determined by item type.
              </div>
            </div>
          </div>
          <div className="block-list">
            {!activeObjectBlock ? (
              <div className="placeholder-block">
                <div className="placeholder-title">
                  No typed block
                </div>
                <p>
                  Choose an item type with typed data to edit.
                </p>
              </div>
            ) : null}
            {activeObjectBlock === "weapon" ? (
              <div className="block-card">
                <div className="block-card__header">
                  <span>Weapon</span>
                </div>
                <div className="block-grid">
                  <div className="form-field">
                    <label className="form-label">
                      Weapon Class
                    </label>
                    <select
                      className="form-select"
                      {...register("weapon.class")}
                    >
                      <option value="">Select</option>
                      {weaponClasses.map((weaponClass) => (
                        <option
                          key={weaponClass}
                          value={weaponClass}
                        >
                          {weaponClass}
                        </option>
                      ))}
                    </select>
                  </div>
                  <div className="form-field">
                    <label className="form-label">Dice #</label>
                    <input
                      className="form-input"
                      type="number"
                      {...register("weapon.diceNumber")}
                    />
                  </div>
                  <div className="form-field">
                    <label className="form-label">Dice Type</label>
                    <input
                      className="form-input"
                      type="number"
                      {...register("weapon.diceType")}
                    />
                  </div>
                  <div className="form-field">
                    <label className="form-label">
                      Damage Type
                    </label>
                    <select
                      className="form-select"
                      {...register("weapon.damageType")}
                    >
                      <option value="">Select</option>
                      {damageTypes.map((damageType) => (
                        <option
                          key={damageType}
                          value={damageType}
                        >
                          {damageType}
                        </option>
                      ))}
                    </select>
                  </div>
                  <div className="form-field form-field--full">
                    <label className="form-label">
                      Weapon Flags
                    </label>
                    <div className="form-checkboxes">
                      {weaponFlags.map((flag) => (
                        <label
                          key={flag}
                          className="checkbox-pill"
                        >
                          <input
                            type="checkbox"
                            value={flag}
                            {...register("weapon.flags")}
                          />
                          <span>{flag}</span>
                        </label>
                      ))}
                    </div>
                  </div>
                </div>
              </div>
            ) : null}
            {activeObjectBlock === "armor" ? (
              <div className="block-card">
                <div className="block-card__header">
                  <span>Armor</span>
                </div>
                <div className="block-grid">
                  <div className="form-field">
                    <label className="form-label">AC Pierce</label>
                    <input
                      className="form-input"
                      type="number"
                      {...register("armor.acPierce")}
                    />
                  </div>
                  <div className="form-field">
                    <label className="form-label">AC Bash</label>
                    <input
                      className="form-input"
                      type="number"
                      {...register("armor.acBash")}
                    />
                  </div>
                  <div className="form-field">
                    <label className="form-label">AC Slash</label>
                    <input
                      className="form-input"
                      type="number"
                      {...register("armor.acSlash")}
                    />
                  </div>
                  <div className="form-field">
                    <label className="form-label">AC Exotic</label>
                    <input
                      className="form-input"
                      type="number"
                      {...register("armor.acExotic")}
                    />
                  </div>
                </div>
              </div>
            ) : null}
            {activeObjectBlock === "container" ? (
              <div className="block-card">
                <div className="block-card__header">
                  <span>Container</span>
                </div>
                <div className="block-grid">
                  <div className="form-field">
                    <label className="form-label">Capacity</label>
                    <input
                      className="form-input"
                      type="number"
                      {...register("container.capacity")}
                    />
                  </div>
                  <VnumPicker<any>
                    id="object-container-key"
                    label="Key VNUM"
                    name={
                      "container.keyVnum" as FieldPath<any>
                    }
                    register={register}
                    options={objectVnumOptions}
                    error={
                      errors.container?.keyVnum
                        ?.message
                    }
                  />
                  <div className="form-field">
                    <label className="form-label">Max Weight</label>
                    <input
                      className="form-input"
                      type="number"
                      {...register("container.maxWeight")}
                    />
                  </div>
                  <div className="form-field">
                    <label className="form-label">Weight Mult</label>
                    <input
                      className="form-input"
                      type="number"
                      {...register("container.weightMult")}
                    />
                  </div>
                  <div className="form-field form-field--full">
                    <label className="form-label">
                      Container Flags
                    </label>
                    <div className="form-checkboxes">
                      {containerFlags.map((flag) => (
                        <label
                          key={flag}
                          className="checkbox-pill"
                        >
                          <input
                            type="checkbox"
                            value={flag}
                            {...register("container.flags")}
                          />
                          <span>{flag}</span>
                        </label>
                      ))}
                    </div>
                  </div>
                </div>
              </div>
            ) : null}
            {activeObjectBlock === "light" ? (
              <div className="block-card">
                <div className="block-card__header">
                  <span>Light</span>
                </div>
                <div className="block-grid">
                  <div className="form-field">
                    <label className="form-label">Hours</label>
                    <input
                      className="form-input"
                      type="number"
                      {...register("light.hours")}
                    />
                  </div>
                </div>
              </div>
            ) : null}
            {activeObjectBlock === "drink" ? (
              <div className="block-card">
                <div className="block-card__header">
                  <span>Drink</span>
                </div>
                <div className="block-grid">
                  <div className="form-field">
                    <label className="form-label">Capacity</label>
                    <input
                      className="form-input"
                      type="number"
                      {...register("drink.capacity")}
                    />
                  </div>
                  <div className="form-field">
                    <label className="form-label">Remaining</label>
                    <input
                      className="form-input"
                      type="number"
                      {...register("drink.remaining")}
                    />
                  </div>
                  <div className="form-field">
                    <label className="form-label">Liquid</label>
                    <select
                      className="form-select"
                      {...register("drink.liquid")}
                    >
                      <option value="">Select</option>
                      {liquids.map((liquid) => (
                        <option key={liquid} value={liquid}>
                          {liquid}
                        </option>
                      ))}
                    </select>
                  </div>
                  <div className="form-field">
                    <label className="form-label">Poisoned</label>
                    <label className="checkbox-pill">
                      <input
                        type="checkbox"
                        {...register("drink.poisoned")}
                      />
                      <span>Poisoned</span>
                    </label>
                  </div>
                </div>
              </div>
            ) : null}
            {activeObjectBlock === "fountain" ? (
              <div className="block-card">
                <div className="block-card__header">
                  <span>Fountain</span>
                </div>
                <div className="block-grid">
                  <div className="form-field">
                    <label className="form-label">Capacity</label>
                    <input
                      className="form-input"
                      type="number"
                      {...register("fountain.capacity")}
                    />
                  </div>
                  <div className="form-field">
                    <label className="form-label">Remaining</label>
                    <input
                      className="form-input"
                      type="number"
                      {...register("fountain.remaining")}
                    />
                  </div>
                  <div className="form-field">
                    <label className="form-label">Liquid</label>
                    <select
                      className="form-select"
                      {...register("fountain.liquid")}
                    >
                      <option value="">Select</option>
                      {liquids.map((liquid) => (
                        <option key={liquid} value={liquid}>
                          {liquid}
                        </option>
                      ))}
                    </select>
                  </div>
                  <div className="form-field">
                    <label className="form-label">Poisoned</label>
                    <label className="checkbox-pill">
                      <input
                        type="checkbox"
                        {...register("fountain.poisoned")}
                      />
                      <span>Poisoned</span>
                    </label>
                  </div>
                </div>
              </div>
            ) : null}
            {activeObjectBlock === "food" ? (
              <div className="block-card">
                <div className="block-card__header">
                  <span>Food</span>
                </div>
                <div className="block-grid">
                  <div className="form-field">
                    <label className="form-label">Food Hours</label>
                    <input
                      className="form-input"
                      type="number"
                      {...register("food.foodHours")}
                    />
                  </div>
                  <div className="form-field">
                    <label className="form-label">Full Hours</label>
                    <input
                      className="form-input"
                      type="number"
                      {...register("food.fullHours")}
                    />
                  </div>
                  <div className="form-field">
                    <label className="form-label">Poisoned</label>
                    <label className="checkbox-pill">
                      <input
                        type="checkbox"
                        {...register("food.poisoned")}
                      />
                      <span>Poisoned</span>
                    </label>
                  </div>
                </div>
              </div>
            ) : null}
            {activeObjectBlock === "money" ? (
              <div className="block-card">
                <div className="block-card__header">
                  <span>Money</span>
                </div>
                <div className="block-grid">
                  <div className="form-field">
                    <label className="form-label">Gold</label>
                    <input
                      className="form-input"
                      type="number"
                      {...register("money.gold")}
                    />
                  </div>
                  <div className="form-field">
                    <label className="form-label">Silver</label>
                    <input
                      className="form-input"
                      type="number"
                      {...register("money.silver")}
                    />
                  </div>
                </div>
              </div>
            ) : null}
            {activeObjectBlock === "wand" ? (
              <div className="block-card">
                <div className="block-card__header">
                  <span>Wand</span>
                </div>
                <div className="block-grid">
                  <div className="form-field">
                    <label className="form-label">Level</label>
                    <input
                      className="form-input"
                      type="number"
                      {...register("wand.level")}
                    />
                  </div>
                  <div className="form-field">
                    <label className="form-label">
                      Total Charges
                    </label>
                    <input
                      className="form-input"
                      type="number"
                      {...register("wand.totalCharges")}
                    />
                  </div>
                  <div className="form-field">
                    <label className="form-label">
                      Charges Left
                    </label>
                    <input
                      className="form-input"
                      type="number"
                      {...register("wand.chargesLeft")}
                    />
                  </div>
                  <div className="form-field form-field--wide">
                    <label className="form-label">Spell</label>
                    <input
                      className="form-input"
                      type="text"
                      {...register("wand.spell")}
                    />
                  </div>
                </div>
              </div>
            ) : null}
            {activeObjectBlock === "staff" ? (
              <div className="block-card">
                <div className="block-card__header">
                  <span>Staff</span>
                </div>
                <div className="block-grid">
                  <div className="form-field">
                    <label className="form-label">Level</label>
                    <input
                      className="form-input"
                      type="number"
                      {...register("staff.level")}
                    />
                  </div>
                  <div className="form-field">
                    <label className="form-label">
                      Total Charges
                    </label>
                    <input
                      className="form-input"
                      type="number"
                      {...register("staff.totalCharges")}
                    />
                  </div>
                  <div className="form-field">
                    <label className="form-label">
                      Charges Left
                    </label>
                    <input
                      className="form-input"
                      type="number"
                      {...register("staff.chargesLeft")}
                    />
                  </div>
                  <div className="form-field form-field--wide">
                    <label className="form-label">Spell</label>
                    <input
                      className="form-input"
                      type="text"
                      {...register("staff.spell")}
                    />
                  </div>
                </div>
              </div>
            ) : null}
            {activeObjectBlock === "spells" ? (
              <div className="block-card">
                <div className="block-card__header">
                  <span>Spells</span>
                </div>
                <div className="block-grid">
                  <div className="form-field">
                    <label className="form-label">Level</label>
                    <input
                      className="form-input"
                      type="number"
                      {...register("spells.level")}
                    />
                  </div>
                  <div className="form-field">
                    <label className="form-label">Spell 1</label>
                    <input
                      className="form-input"
                      type="text"
                      {...register("spells.spell1")}
                    />
                  </div>
                  <div className="form-field">
                    <label className="form-label">Spell 2</label>
                    <input
                      className="form-input"
                      type="text"
                      {...register("spells.spell2")}
                    />
                  </div>
                  <div className="form-field">
                    <label className="form-label">Spell 3</label>
                    <input
                      className="form-input"
                      type="text"
                      {...register("spells.spell3")}
                    />
                  </div>
                  <div className="form-field">
                    <label className="form-label">Spell 4</label>
                    <input
                      className="form-input"
                      type="text"
                      {...register("spells.spell4")}
                    />
                  </div>
                </div>
              </div>
            ) : null}
            {activeObjectBlock === "portal" ? (
              <div className="block-card">
                <div className="block-card__header">
                  <span>Portal</span>
                </div>
                <div className="block-grid">
                  <div className="form-field">
                    <label className="form-label">Charges</label>
                    <input
                      className="form-input"
                      type="number"
                      {...register("portal.charges")}
                    />
                  </div>
                  <VnumPicker<any>
                    id="object-portal-to"
                    label="To VNUM"
                    name={
                      "portal.toVnum" as FieldPath<any>
                    }
                    register={register}
                    options={roomVnumOptions}
                    error={
                      errors.portal?.toVnum?.message
                    }
                  />
                  <div className="form-field form-field--full">
                    <label className="form-label">Exit Flags</label>
                    <div className="form-checkboxes">
                      {exitFlags.map((flag) => (
                        <label
                          key={flag}
                          className="checkbox-pill"
                        >
                          <input
                            type="checkbox"
                            value={flag}
                            {...register("portal.exitFlags")}
                          />
                          <span>{flag}</span>
                        </label>
                      ))}
                    </div>
                  </div>
                  <div className="form-field form-field--full">
                    <label className="form-label">
                      Portal Flags
                    </label>
                    <div className="form-checkboxes">
                      {portalFlags.map((flag) => (
                        <label
                          key={flag}
                          className="checkbox-pill"
                        >
                          <input
                            type="checkbox"
                            value={flag}
                            {...register(
                              "portal.portalFlags"
                            )}
                          />
                          <span>{flag}</span>
                        </label>
                      ))}
                    </div>
                  </div>
                </div>
              </div>
            ) : null}
            {activeObjectBlock === "furniture" ? (
              <div className="block-card">
                <div className="block-card__header">
                  <span>Furniture</span>
                </div>
                <div className="block-grid">
                  <div className="form-field">
                    <label className="form-label">Slots</label>
                    <input
                      className="form-input"
                      type="number"
                      {...register("furniture.slots")}
                    />
                  </div>
                  <div className="form-field">
                    <label className="form-label">Weight</label>
                    <input
                      className="form-input"
                      type="number"
                      {...register("furniture.weight")}
                    />
                  </div>
                  <div className="form-field">
                    <label className="form-label">Heal Bonus</label>
                    <input
                      className="form-input"
                      type="number"
                      {...register("furniture.healBonus")}
                    />
                  </div>
                  <div className="form-field">
                    <label className="form-label">Mana Bonus</label>
                    <input
                      className="form-input"
                      type="number"
                      {...register("furniture.manaBonus")}
                    />
                  </div>
                  <div className="form-field">
                    <label className="form-label">Max People</label>
                    <input
                      className="form-input"
                      type="number"
                      {...register("furniture.maxPeople")}
                    />
                  </div>
                  <div className="form-field form-field--full">
                    <label className="form-label">
                      Furniture Flags
                    </label>
                    <div className="form-checkboxes">
                      {furnitureFlags.map((flag) => (
                        <label
                          key={flag}
                          className="checkbox-pill"
                        >
                          <input
                            type="checkbox"
                            value={flag}
                            {...register("furniture.flags")}
                          />
                          <span>{flag}</span>
                        </label>
                      ))}
                    </div>
                  </div>
                </div>
              </div>
            ) : null}
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
            {formState.isDirty
              ? "Unsaved changes"
              : "Up to date"}
          </span>
        </div>
      </form>
    </div>
  );
}
