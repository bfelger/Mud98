/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik Stærfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

/***************************************************************************
 *  ROM 2.4 is copyright 1993-1998 Russ Taylor                             *
 *  ROM has been brought to you by the ROM consortium                      *
 *      Russ Taylor (rtaylor@hypercube.org)                                *
 *      Gabrielle Taylor (gtaylor@hypercube.org)                           *
 *      Brian Moore (zump@rom.org)                                         *
 *  By using this code, you have agreed to follow the terms of the         *
 *  ROM license, in the file Rom24/doc/rom.license                         *
 ***************************************************************************/

////////////////////////////////////////////////////////////////////////////////
// rng.c
// Production RNG implementation using PCG (Permuted Congruential Generator).
//
// This file contains the ROM legacy RNG functions adapted to use the modern
// PCG random number generator while preserving the original API and behavior.
////////////////////////////////////////////////////////////////////////////////

#include "merc.h"
#include "rng.h"
#include "pcg_basic.h"

#include <stdio.h>

// Production RNG implementations using PCG
static int prod_number_range(int from, int to);
static int prod_number_percent(void);
static int prod_number_bits(int width);
static Direction prod_number_door(void);
static long prod_number_mm(void);
static int prod_dice(int number, int size);
static int prod_number_fuzzy(int number);

// Production RNG ops table
RngOps rng_production = {
    .number_range = prod_number_range,
    .number_percent = prod_number_percent,
    .number_bits = prod_number_bits,
    .number_door = prod_number_door,
    .number_mm = prod_number_mm,
    .dice = prod_dice,
    .number_fuzzy = prod_number_fuzzy,
};

// Global RNG pointer - defaults to production
RngOps* rng = &rng_production;

// Generate a random number in range [from, to] inclusive
static int prod_number_range(int from, int to)
{
    int power;
    int number;

    if (from == 0 && to == 0) 
        return 0;

    if ((to = to - from + 1) <= 1) 
        return from;

    for (power = 2; power < to; power <<= 1)
        ;

    while ((number = prod_number_mm() & (power - 1)) >= to)
        ;

    return from + number;
}

// Generate a percentile roll [1, 100]
static int prod_number_percent(void)
{
    int percent;

    while ((percent = prod_number_mm() & (128 - 1)) > 99)
        ;

    return 1 + percent;
}

// Generate a random number in range [0, 2^width - 1]
static int prod_number_bits(int width)
{
    return prod_number_mm() & ((1 << width) - 1);
}

// Generate a random door direction [0, 5]
static Direction prod_number_door(void)
{
    Direction door;

    while ((door = prod_number_mm() & (8 - 1)) > 5)
        ;

    return door;
}

// Core PCG random number generator
static long prod_number_mm(void)
{
    return pcg32_random();
}

// Roll dice: returns sum of 'number' d'size' rolls
static int prod_dice(int number, int size)
{
    int idice;
    int sum;

    switch (size) {
    case 0:
        return 0;
    case 1:
        return number;
    }

    for (idice = 0, sum = 0; idice < number; idice++)
        sum += prod_number_range(1, size);

    return sum;
}

// Add slight random variance to a number
static int prod_number_fuzzy(int number)
{
    switch (prod_number_bits(2)) {
    case 0:
        number -= 1;
        break;
    case 3:
        number += 1;
        break;
    }

    return UMAX(1, number);
}
