////////////////////////////////////////////////////////////////////////////////
// entities/shop_data.h
////////////////////////////////////////////////////////////////////////////////

typedef struct shop_data_t ShopData;

#pragma once
#ifndef MUD98__ENTITITES__SHOP_DATA_H
#define MUD98__ENTITITES__SHOP_DATA_H

#include <data/item.h>

#include <stdint.h>

#define MAX_TRADE 5

typedef struct shop_data_t {
    ShopData* next;                     // Next shop in list
    VNUM keeper;                        // Vnum of shop keeper mob
    ItemType buy_type[MAX_TRADE];       // Item types shop will buy
    int16_t profit_buy;                 // Cost multiplier for buying
    int16_t profit_sell;                // Cost multiplier for selling
    int16_t open_hour;                  // First opening hour
    int16_t close_hour;                 // First closing hour
} ShopData;

ShopData* new_shop_data(void);
void free_shop_data(ShopData* pShop);

extern ShopData* shop_first;
extern ShopData* shop_last;

extern int shop_count;
extern int shop_perm_count;

#endif // !MUD98__ENTITITES__SHOP_DATA_H
