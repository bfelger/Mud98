import type { FormEventHandler } from "react";
import type { Control, UseFormRegister } from "react-hook-form";
import { ResetEditorFields } from "./ResetEditorFields";
import type { VnumOption } from "./VnumPicker";

type ResetFormProps = {
  onSubmit: FormEventHandler<HTMLFormElement>;
  register: UseFormRegister<any>;
  control: Control<any>;
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
  control,
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
  return (
    <div className="form-view">
      <form className="form-shell" onSubmit={onSubmit}>
        <ResetEditorFields
          register={register}
          control={control}
          errors={formState.errors}
          activeResetCommand={activeResetCommand}
          resetCommandOptions={resetCommandOptions}
          resetCommandLabels={resetCommandLabels}
          directions={directions}
          wearLocations={wearLocations}
          roomVnumOptions={roomVnumOptions}
          objectVnumOptions={objectVnumOptions}
          mobileVnumOptions={mobileVnumOptions}
        />
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
