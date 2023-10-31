////////////////////////////////////////////////////////////////////////////////
// shop_data.c
////////////////////////////////////////////////////////////////////////////////

#include "shop_data.h"

#include "db.h"

ShopData* shop_first;
ShopData* shop_last;
ShopData* shop_free;

int shop_count;
int shop_perm_count;

ShopData* new_shop_data()
{
    LIST_ALLOC_PERM(shop, ShopData);

    shop->profit_buy = 100;
    shop->profit_sell = 100;
    shop->close_hour = 23;

    return shop;
}

void free_shop_data(ShopData* shop)
{
    LIST_FREE(shop);
}
