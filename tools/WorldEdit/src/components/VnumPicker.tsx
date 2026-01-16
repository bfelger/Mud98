import type {
  FieldPath,
  FieldValues,
  UseFormRegister
} from "react-hook-form";

export type VnumOption = {
  vnum: number;
  label: string;
};

type VnumPickerProps<TFieldValues extends FieldValues> = {
  id: string;
  label: string;
  name: FieldPath<TFieldValues>;
  register: UseFormRegister<TFieldValues>;
  options: VnumOption[];
  error?: string;
};

export function VnumPicker<TFieldValues extends FieldValues>({
  id,
  label,
  name,
  register,
  options,
  error
}: VnumPickerProps<TFieldValues>) {
  const listId = `${id}-options`;
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
          <option key={option.vnum} value={String(option.vnum)}>
            {option.label}
          </option>
        ))}
      </datalist>
      {error ? <span className="form-error">{error}</span> : null}
    </div>
  );
}
