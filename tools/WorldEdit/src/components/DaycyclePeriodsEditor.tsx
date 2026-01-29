import type { UseFormRegister } from "react-hook-form";

type DaycyclePeriodsEditorProps = {
  title: string;
  fields: Array<{ id: string }>;
  register: UseFormRegister<any>;
  namePrefix: string;
  appendPeriod: (value: Record<string, unknown>) => void;
  removePeriod: (index: number) => void;
  movePeriod?: (from: number, to: number) => void;
  suppressField?: string;
  suppressLabel?: string;
  onAddDefaults?: () => void;
  showDescription?: boolean;
};

export function DaycyclePeriodsEditor({
  title,
  fields,
  register,
  namePrefix,
  appendPeriod,
  removePeriod,
  movePeriod,
  suppressField,
  suppressLabel,
  onAddDefaults,
  showDescription = true
}: DaycyclePeriodsEditorProps) {
  const handleAdd = () =>
    appendPeriod({
      name: "",
      fromHour: 0,
      toHour: 0,
      description: "",
      enterMessage: "",
      exitMessage: ""
    });

  return (
    <div className="form-field form-field--full">
      <div className="form-section-header">
        <div>
          <div className="form-label">{title}</div>
          <div className="form-hint">
            {fields.length
              ? `${fields.length} period${fields.length === 1 ? "" : "s"}`
              : "No periods defined"}
          </div>
        </div>
        <div className="block-actions">
          {fields.length === 0 && onAddDefaults ? (
            <button className="ghost-button" type="button" onClick={onAddDefaults}>
              Add Default Periods
            </button>
          ) : null}
          <button className="ghost-button" type="button" onClick={handleAdd}>
            Add Period
          </button>
        </div>
      </div>
      {suppressField ? (
        <label className="checkbox-pill">
          <input type="checkbox" {...register(suppressField)} />
          <span>{suppressLabel ?? "Suppress base daycycle messages"}</span>
        </label>
      ) : null}
      <div className="block-list">
        {fields.map((field, index) => (
          <div className="block-card" key={field.id}>
            <div className="block-card__header">
              <span>Period {index + 1}</span>
              <div className="block-actions">
                {movePeriod ? (
                  <>
                    <button
                      className="ghost-button"
                      type="button"
                      onClick={() => movePeriod(index, index - 1)}
                      disabled={index === 0}
                    >
                      Up
                    </button>
                    <button
                      className="ghost-button"
                      type="button"
                      onClick={() => movePeriod(index, index + 1)}
                      disabled={index === fields.length - 1}
                    >
                      Down
                    </button>
                  </>
                ) : null}
                <button
                  className="ghost-button"
                  type="button"
                  onClick={() => removePeriod(index)}
                >
                  Remove
                </button>
              </div>
            </div>
            <div className="block-grid">
              <div className="form-field form-field--wide">
                <label
                  className="form-label"
                  htmlFor={`period-name-${field.id}`}
                >
                  Name
                </label>
                <input
                  id={`period-name-${field.id}`}
                  className="form-input"
                  type="text"
                  {...register(`${namePrefix}.${index}.name`)}
                />
              </div>
              <div className="form-field">
                <label
                  className="form-label"
                  htmlFor={`period-from-${field.id}`}
                >
                  From Hour
                </label>
                <input
                  id={`period-from-${field.id}`}
                  className="form-input"
                  type="number"
                  {...register(`${namePrefix}.${index}.fromHour`, {
                    valueAsNumber: true
                  })}
                />
              </div>
              <div className="form-field">
                <label
                  className="form-label"
                  htmlFor={`period-to-${field.id}`}
                >
                  To Hour
                </label>
                <input
                  id={`period-to-${field.id}`}
                  className="form-input"
                  type="number"
                  {...register(`${namePrefix}.${index}.toHour`, {
                    valueAsNumber: true
                  })}
                />
              </div>
              {showDescription ? (
                <div className="form-field form-field--full">
                  <label
                    className="form-label"
                    htmlFor={`period-desc-${field.id}`}
                  >
                    Description
                  </label>
                  <textarea
                    id={`period-desc-${field.id}`}
                    className="form-textarea"
                    rows={3}
                    {...register(`${namePrefix}.${index}.description`)}
                  />
                </div>
              ) : null}
              <div className="form-field form-field--full">
                <label
                  className="form-label"
                  htmlFor={`period-enter-${field.id}`}
                >
                  Enter Message
                </label>
                <textarea
                  id={`period-enter-${field.id}`}
                  className="form-textarea"
                  rows={3}
                  {...register(`${namePrefix}.${index}.enterMessage`)}
                />
              </div>
              <div className="form-field form-field--full">
                <label
                  className="form-label"
                  htmlFor={`period-exit-${field.id}`}
                >
                  Exit Message
                </label>
                <textarea
                  id={`period-exit-${field.id}`}
                  className="form-textarea"
                  rows={3}
                  {...register(`${namePrefix}.${index}.exitMessage`)}
                />
              </div>
            </div>
          </div>
        ))}
      </div>
    </div>
  );
}
