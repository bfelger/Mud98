import type { ColDef } from "ag-grid-community";
import type { AreaJson } from "../../repository/types";
import { crudFail, crudOk, type CrudResult } from "../types";
import { getFirstString, parseVnum } from "./utils";

export type ShopRow = {
  keeper: number;
  buyTypes: string;
  profitBuy: number;
  profitSell: number;
  hours: string;
};

type ShopCrudCreateResult = {
  areaData: AreaJson;
  keeper: number;
};

type ShopCrudDeleteResult = {
  areaData: AreaJson;
  deletedKeeper: number;
};

export const shopColumns: ColDef<ShopRow>[] = [
  { headerName: "Keeper", field: "keeper", width: 120, sort: "asc" },
  { headerName: "Buy Types", field: "buyTypes", flex: 2, minWidth: 220 },
  { headerName: "Profit Buy", field: "profitBuy", width: 120 },
  { headerName: "Profit Sell", field: "profitSell", width: 120 },
  { headerName: "Hours", field: "hours", width: 120 }
];

export const buildShopRows = (areaData: AreaJson | null): ShopRow[] => {
  if (!areaData) {
    return [];
  }
  const shops = Array.isArray((areaData as Record<string, unknown>).shops)
    ? ((areaData as Record<string, unknown>).shops as unknown[])
    : [];
  if (!shops.length) {
    return [];
  }
  return shops.map((shop) => {
    const record = shop as Record<string, unknown>;
    const keeper = parseVnum(record.keeper) ?? -1;
    const buyTypes = Array.isArray(record.buyTypes)
      ? record.buyTypes
          .map((value) => parseVnum(value))
          .filter((value): value is number => value !== null)
          .join(", ")
      : "—";
    const profitBuy = parseVnum(record.profitBuy) ?? 0;
    const profitSell = parseVnum(record.profitSell) ?? 0;
    const openHour = parseVnum(record.openHour);
    const closeHour = parseVnum(record.closeHour);
    const hours =
      openHour !== null && closeHour !== null
        ? `${openHour}-${closeHour}`
        : "—";
    return {
      keeper,
      buyTypes,
      profitBuy,
      profitSell,
      hours
    };
  });
};

const findFirstVnum = (entries: unknown[]): number | null => {
  const first = entries.find((entry) => entry && typeof entry === "object");
  return first ? parseVnum((first as Record<string, unknown>).vnum) : null;
};

export const createShop = (
  areaData: AreaJson | null
): CrudResult<ShopCrudCreateResult> => {
  if (!areaData) {
    return crudFail("Load an area before creating shops.");
  }
  const mobiles = Array.isArray((areaData as Record<string, unknown>).mobiles)
    ? ((areaData as Record<string, unknown>).mobiles as unknown[])
    : [];
  const shops = Array.isArray((areaData as Record<string, unknown>).shops)
    ? ((areaData as Record<string, unknown>).shops as unknown[])
    : [];
  const usedKeepers = new Set<number>();
  shops.forEach((shop) => {
    if (!shop || typeof shop !== "object") {
      return;
    }
    const keeper = parseVnum((shop as Record<string, unknown>).keeper);
    if (keeper !== null) {
      usedKeepers.add(keeper);
    }
  });
  let keeper: number | null = null;
  for (const mob of mobiles) {
    if (!mob || typeof mob !== "object") {
      continue;
    }
    const vnum = parseVnum((mob as Record<string, unknown>).vnum);
    if (vnum !== null && !usedKeepers.has(vnum)) {
      keeper = vnum;
      break;
    }
  }
  if (keeper === null && mobiles.length) {
    keeper = findFirstVnum(mobiles);
  }
  if (keeper === null) {
    keeper = 0;
  }
  const newShop: Record<string, unknown> = {
    keeper,
    buyTypes: [0, 0, 0, 0, 0],
    profitBuy: 100,
    profitSell: 100,
    openHour: 0,
    closeHour: 23
  };
  const nextShops = [...shops, newShop];
  const nextAreaData = {
    ...(areaData as Record<string, unknown>),
    shops: nextShops
  } as AreaJson;
  return crudOk({ areaData: nextAreaData, keeper });
};

export const deleteShop = (
  areaData: AreaJson | null,
  selectedKeeper: number | null
): CrudResult<ShopCrudDeleteResult> => {
  if (!areaData) {
    return crudFail("Load an area before deleting shops.");
  }
  if (selectedKeeper === null) {
    return crudFail("Select a shop to delete.");
  }
  const shops = Array.isArray((areaData as Record<string, unknown>).shops)
    ? ((areaData as Record<string, unknown>).shops as unknown[])
    : [];
  const nextShops = shops.filter((shop) => {
    if (!shop || typeof shop !== "object") {
      return true;
    }
    const keeper = parseVnum((shop as Record<string, unknown>).keeper);
    return keeper !== selectedKeeper;
  });
  const nextAreaData = {
    ...(areaData as Record<string, unknown>),
    shops: nextShops
  } as AreaJson;
  return crudOk({ areaData: nextAreaData, deletedKeeper: selectedKeeper });
};
