////////////////////////////////////////////////////////////////////////////////
// mem.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__MEM_H
#define MUD98__MEM_H

#include "merc.h"

SHOP_DATA* new_shop();
void free_shop(SHOP_DATA* pShop);

MPROG_CODE* new_mpcode();

#endif // !MUD98__MEM_H