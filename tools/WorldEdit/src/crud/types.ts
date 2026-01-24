export type CrudResult<T> = { ok: true; data: T } | { ok: false; message: string };

export const crudOk = <T>(data: T): CrudResult<T> => ({ ok: true, data });

export const crudFail = <T>(message: string): CrudResult<T> => ({
  ok: false,
  message
});
