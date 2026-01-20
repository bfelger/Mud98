import { useMemo } from "react";
import type {
  Control,
  FieldPath,
  FieldValues,
  UseFormRegister
} from "react-hook-form";
import { useWatch } from "react-hook-form";

export type VnumOption = {
  vnum: number;
  label: string;
  entityType?: string;
  name?: string;
};

type VnumPickerProps<TFieldValues extends FieldValues> = {
  id: string;
  label: string;
  name: FieldPath<TFieldValues>;
  register: UseFormRegister<TFieldValues>;
  control: Control<TFieldValues>;
  options: VnumOption[];
  error?: string;
  entityLabel?: string;
};

export function VnumPicker<TFieldValues extends FieldValues>({
  id,
  label,
  name,
  register,
  control,
  options,
  error,
  entityLabel
}: VnumPickerProps<TFieldValues>) {
  const listId = `${id}-options`;
  const optionMap = useMemo(() => {
    const map = new Map<number, VnumOption>();
    options.forEach((option) => {
      map.set(option.vnum, option);
    });
    return map;
  }, [options]);
  const watchedValue = useWatch({
    control,
    name
  });
  const parsedVnum =
    typeof watchedValue === "number"
      ? watchedValue
      : typeof watchedValue === "string"
        ? Number.parseInt(watchedValue, 10)
        : Number.NaN;
  const resolvedOption = Number.isFinite(parsedVnum)
    ? optionMap.get(parsedVnum)
    : undefined;
  const fallbackText =
    Number.isFinite(parsedVnum) && entityLabel
      ? `${entityLabel} · #${parsedVnum}`
      : null;
  const helperText =
    resolvedOption?.entityType && resolvedOption?.name
      ? `${resolvedOption.entityType} · ${resolvedOption.name}`
      : resolvedOption?.label ?? fallbackText;
  return (
    <div className="form-field">
      <label className="form-label" htmlFor={id}>
        {label}
      </label>
      <input
        id={id}
        className="form-input"
        type="text"
        inputMode="numeric"
        list={listId}
        {...register(name)}
      />
      <datalist id={listId}>
        {options.map((option) => (
          <option
            key={option.vnum}
            value={String(option.vnum)}
            label={option.label}
          >
            {option.label}
          </option>
        ))}
      </datalist>
      {helperText ? (
        <div className="form-hint" title={helperText}>
          {helperText}
        </div>
      ) : null}
      {error ? <span className="form-error">{error}</span> : null}
    </div>
  );
}
