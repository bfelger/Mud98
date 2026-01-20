import type { FormEventHandler } from "react";
import type { Control, FieldPath, UseFormRegister } from "react-hook-form";
import { VnumPicker, type VnumOption } from "./VnumPicker";

type ShopFormProps = {
  onSubmit: FormEventHandler<HTMLFormElement>;
  register: UseFormRegister<any>;
  control: Control<any>;
  formState: {
    isDirty: boolean;
    errors: Record<string, { message?: string } | undefined>;
  };
  mobileVnumOptions: VnumOption[];
};

export function ShopForm({
  onSubmit,
  register,
  control,
  formState,
  mobileVnumOptions
}: ShopFormProps) {
  const errors = formState.errors as Record<string, any>;

  return (
    <div className="form-view">
      <form className="form-shell" onSubmit={onSubmit}>
        <div className="form-grid">
          <VnumPicker<any>
            id="shop-keeper"
            label="Keeper VNUM"
            name={"keeper" as FieldPath<any>}
            register={register}
            control={control}
            options={mobileVnumOptions}
            error={errors.keeper?.message}
          />
          <div className="form-field">
            <label className="form-label" htmlFor="shop-profit-buy">
              Profit Buy
            </label>
            <input
              id="shop-profit-buy"
              className="form-input"
              type="number"
              {...register("profitBuy")}
            />
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="shop-profit-sell">
              Profit Sell
            </label>
            <input
              id="shop-profit-sell"
              className="form-input"
              type="number"
              {...register("profitSell")}
            />
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="shop-open-hour">
              Open Hour
            </label>
            <input
              id="shop-open-hour"
              className="form-input"
              type="number"
              {...register("openHour")}
            />
          </div>
          <div className="form-field">
            <label className="form-label" htmlFor="shop-close-hour">
              Close Hour
            </label>
            <input
              id="shop-close-hour"
              className="form-input"
              type="number"
              {...register("closeHour")}
            />
          </div>
        </div>
        <div className="form-field form-field--full">
          <label className="form-label">Buy Types</label>
          <div className="block-grid">
            {Array.from({ length: 5 }).map((_, index) => (
              <div className="form-field" key={`buy-type-${index}`}>
                <label className="form-label" htmlFor={`shop-buy-${index}`}>
                  Type {index + 1}
                </label>
                <input
                  id={`shop-buy-${index}`}
                  className="form-input"
                  type="number"
                  {...register(`buyTypes.${index}` as FieldPath<any>)}
                />
              </div>
            ))}
          </div>
        </div>
        <div className="form-actions">
          <button className="action-button" type="submit">
            Save Shop
          </button>
        </div>
      </form>
    </div>
  );
}
