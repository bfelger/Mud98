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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

void free_mprog args((MPROG_LIST* mp));

/*
 * Globals
 */
extern int top_reset;
extern int top_area;
extern int top_exit;
extern int top_ed;
extern int top_room;

AREA_DATA* area_free;
EXTRA_DESCR_DATA* extra_descr_free;
EXIT_DATA* exit_free;
SHOP_DATA* shop_free;
RESET_DATA* reset_free;
HELP_DATA* help_free = NULL;

HELP_DATA* help_last;

void free_extra_descr(EXTRA_DESCR_DATA* pExtra);
void free_affect(AFFECT_DATA* af);

RESET_DATA* new_reset_data(void)
{
    RESET_DATA* pReset;

    if (!reset_free) {
        pReset = alloc_perm(sizeof(*pReset));
        top_reset++;
    }
    else {
        pReset = reset_free;
        reset_free = reset_free->next;
    }

    pReset->next = NULL;
    pReset->command = 'X';
    pReset->arg1 = 0;
    pReset->arg2 = 0;
    pReset->arg3 = 0;
    pReset->arg4 = 0;

    return pReset;
}

void free_reset_data(RESET_DATA* pReset)
{
    pReset->next = reset_free;
    reset_free = pReset;
    return;
}

AREA_DATA* new_area(void)
{
    AREA_DATA* pArea;
    char buf[MAX_INPUT_LENGTH];

    if (!area_free) {
        pArea = alloc_perm(sizeof(*pArea));
        top_area++;
    }
    else {
        pArea = area_free;
        area_free = area_free->next;
    }

    pArea->next = NULL;
    pArea->name = str_dup("New area");
/*    pArea->recall           =   ROOM_VNUM_TEMPLE;      ROM OLC */
    pArea->area_flags = AREA_ADDED;
    pArea->security = 1;
    pArea->builders = str_dup("None");
    pArea->credits = str_dup("None");
    pArea->min_vnum = 0;
    pArea->max_vnum = 0;
    pArea->age = 0;
    pArea->nplayer = 0;
    pArea->empty = true;              /* ROM patch */
    pArea->vnum = top_area - 1;
    sprintf(buf, "area%"PRVNUM".are", pArea->vnum);
    pArea->file_name = str_dup(buf);

    return pArea;
}

void free_area(AREA_DATA* pArea)
{
    free_string(pArea->name);
    free_string(pArea->file_name);
    free_string(pArea->builders);
    free_string(pArea->credits);

    pArea->next = area_free->next;
    area_free = pArea;
    return;
}

EXIT_DATA* new_exit(void)
{
    EXIT_DATA* pExit;

    if (!exit_free) {
        pExit = alloc_perm(sizeof(*pExit));
        top_exit++;
    }
    else {
        pExit = exit_free;
        exit_free = exit_free->next;
    }

    pExit->u1.to_room = NULL;                  /* ROM OLC */
    pExit->next = NULL;
/*  pExit->vnum         =   0;                        ROM OLC */
    pExit->exit_info = 0;
    pExit->key = 0;
    pExit->keyword = &str_empty[0];
    pExit->description = &str_empty[0];
    pExit->rs_flags = 0;

    return pExit;
}

void free_exit(EXIT_DATA* pExit)
{
    if (pExit == NULL)
        return;

    free_string(pExit->keyword);
    free_string(pExit->description);

    pExit->next = exit_free;
    exit_free = pExit;

    return;
}

SHOP_DATA* new_shop(void)
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

MPROG_CODE* new_mpcode(void)
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
