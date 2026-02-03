/**
 * Hook to reset a react-hook-form form when defaults change.
 * 
 * Replaces repetitive useEffect(() => form.reset(defaults), [form, defaults])
 * patterns with a single reusable hook.
 */

import { useEffect } from "react";
import type { UseFormReturn, FieldValues } from "react-hook-form";

/**
 * Resets a form whenever the default values change.
 * 
 * @param form - The react-hook-form form instance
 * @param defaults - The default values to reset to
 */
export function useFormResetOnDefaultsChange<T extends FieldValues>(
  form: UseFormReturn<T>,
  defaults: T
): void {
  useEffect(() => {
    form.reset(defaults);
  }, [form, defaults]);
}
