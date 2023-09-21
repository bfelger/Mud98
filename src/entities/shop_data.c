////////////////////////////////////////////////////////////////////////////////
// shop_data.c
////////////////////////////////////////////////////////////////////////////////

#include "shop_data.h"

#include "db.h"

ShopData* shop_first;
ShopData* shop_last;
ShopData* shop_free;

int top_shop;

ShopData* new_shop_data()
{
    ShopData* pShop;
    int buy;

    if (!shop_free) {
        pShop = alloc_perm(sizeof(*pShop));
        top_shop++;
    }
    else {
        pShop = shop_free;
        shop_free = shop_free->next;
    }

    pShop->next = NULL;
    pShop->keeper = 0;

    for (buy = 0; buy < MAX_TRADE; buy++)
        pShop->buy_type[buy] = 0;

    pShop->profit_buy = 100;
    pShop->profit_sell = 100;
    pShop->open_hour = 0;
    pShop->close_hour = 23;

    return pShop;
}

void free_shop_data(ShopData* pShop)
{
    pShop->next = shop_free;
    shop_free = pShop;
    return;
}
