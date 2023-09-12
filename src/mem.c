/***************************************************************************
 *  File: mem.c                                                            *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 *                                                                         *
 *  This code was freely distributed with the The Isles 1.1 source code,   *
 *  and has been used here for OLC - OLC would not be what it is without   *
 *  all the previous coders who released their source code.                *
 *                                                                         *
 ***************************************************************************/


#include "merc.h"

#include "db.h"

#include "entities/exit_data.h"
#include "entities/reset_data.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

SHOP_DATA* shop_free;


SHOP_DATA* new_shop()
{
    SHOP_DATA* pShop;
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

void free_shop(SHOP_DATA* pShop)
{
    pShop->next = shop_free;
    shop_free = pShop;
    return;
}

MPROG_CODE* mpcode_free;

MPROG_CODE* new_mpcode()
{
    MPROG_CODE* NewCode;
    extern int top_mprog_index;

    if (!mpcode_free) {
        NewCode = alloc_perm(sizeof(*NewCode));
        top_mprog_index++;
    }
    else {
        NewCode = mpcode_free;
        mpcode_free = mpcode_free->next;
    }

    NewCode->vnum = 0;
    NewCode->code = str_dup("");
    NewCode->next = NULL;

    return NewCode;
}

void free_mpcode(MPROG_CODE* pMcode)
{
    free_string(pMcode->code);
    pMcode->next = mpcode_free;
    mpcode_free = pMcode;
    return;
}
