/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St�rfeldt, Tom Madsen, and Katja Nyboe.   *
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

#include "effects.h"

#include "comm.h"
#include "db.h"
#include "handler.h"
#include "magic.h"
#include "recycle.h"
#include "update.h"

#include "entities/mobile.h"
#include "entities/descriptor.h"
#include "entities/object.h"

#include "data/mobile_data.h"
#include "data/skill.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

void acid_effect(void* vo, LEVEL level, int dam, SpellTarget target)
{
    if (target == SPELL_TARGET_ROOM) /* nail objects on the floor */
    {
        Room* room = (Room*)vo;
        Object* obj;

        FOR_EACH_ROOM_OBJ(obj, room) {
            acid_effect(obj, level, dam, SPELL_TARGET_OBJ);
        }
        return;
    }

    if (target == SPELL_TARGET_CHAR) /* do the effect on a victim */
    {
        Mobile* victim = (Mobile*)vo;
        Object* obj;

        /* let's toast some gear */
        FOR_EACH_MOB_OBJ(obj, victim) {
            acid_effect(obj, level, dam, SPELL_TARGET_OBJ);
        }
        return;
    }

    if (target == SPELL_TARGET_OBJ) /* toast an object */
    {
        Object* obj = (Object*)vo;
        Object* t_obj;
        int chance;
        char* msg;

        if (IS_OBJ_STAT(obj, ITEM_BURN_PROOF) || IS_OBJ_STAT(obj, ITEM_NOPURGE)
            || number_range(0, 4) == 0)
            return;

        chance = level / 4 + dam / 10;

        if (chance > 25) 
            chance = (chance - 25) / 2 + 25;
        if (chance > 50) 
            chance = (chance - 50) / 2 + 50;

        if (IS_OBJ_STAT(obj, ITEM_BLESS)) 
            chance -= 5;

        chance -= obj->level * 2;

        switch (obj->item_type) {
        default:
            return;
        case ITEM_CONTAINER:
        case ITEM_CORPSE_PC:
        case ITEM_CORPSE_NPC:
            msg = "$p fumes and dissolves.";
            break;
        case ITEM_ARMOR:
            msg = "$p is pitted and etched.";
            break;
        case ITEM_CLOTHING:
            msg = "$p is corroded into scrap.";
            break;
        case ITEM_STAFF:
        case ITEM_WAND:
            chance -= 10;
            msg = "$p corrodes and breaks.";
            break;
        case ITEM_SCROLL:
            chance += 10;
            msg = "$p is burned into waste.";
            break;
        }

        chance = URANGE(5, chance, 95);

        if (number_percent() > chance) 
            return;

        if (obj->carried_by != NULL)
            act(msg, obj->carried_by, obj, NULL, TO_ALL);
        else if (obj->in_room != NULL && ROOM_HAS_MOBS(obj->in_room))
            act(msg, obj->in_room, obj, NULL, TO_ALL);

        if (obj->item_type == ITEM_ARMOR) {
            /* etch it */
            Affect* affect;
            Affect* paf_next = NULL;
            bool af_found = false;
            int i;

            affect_enchant(obj);

            for (affect = obj->affected; affect != NULL; affect = paf_next) {
                paf_next = affect->next;
                if (affect->location == APPLY_AC) {
                    af_found = true;
                    affect->type = -1;
                    affect->modifier += 1;
                    affect->level = UMAX(affect->level, level);
                    break;
                }
            }

            if (!af_found)
            /* needs a new affect */
            {
                affect = new_affect();

                affect->type = -1;
                affect->level = level;
                affect->duration = -1;
                affect->location = APPLY_AC;
                affect->modifier = 1;
                affect->bitvector = 0;
                affect->next = obj->affected;
                obj->affected = affect;
            }

            if (obj->carried_by != NULL && obj->wear_loc != WEAR_UNHELD)
                for (i = 0; i < 4; i++) 
                    obj->carried_by->armor[i] += 1;
            return;
        }

        /* get rid of the object */
        if (OBJ_HAS_OBJS(obj)) {
            /* dump contents */
            FOR_EACH_OBJ_CONTENT(t_obj, obj) {
                obj_from_obj(t_obj);
                if (obj->in_room != NULL)
                    obj_to_room(t_obj, obj->in_room);
                else if (obj->carried_by != NULL)
                    obj_to_room(t_obj, obj->carried_by->in_room);
                else {
                    extract_obj(t_obj);
                    continue;
                }

                acid_effect(t_obj, level / 2, dam / 2, SPELL_TARGET_OBJ);
            }
        }

        extract_obj(obj);
        return;
    }
}

void cold_effect(void* vo, LEVEL level, int dam, SpellTarget target)
{
    if (target == SPELL_TARGET_ROOM) /* nail objects on the floor */
    {
        Room* room = (Room*)vo;
        Object* obj;

        FOR_EACH_ROOM_OBJ(obj, room) {
            cold_effect(obj, level, dam, SPELL_TARGET_OBJ);
        }
        return;
    }

    if (target == SPELL_TARGET_CHAR) /* whack a character */
    {
        Mobile* victim = (Mobile*)vo;
        Object* obj;

        /* chill touch effect */
        if (!saves_spell(level / 4 + (LEVEL)(dam / 20), victim, DAM_COLD)) {
            Affect af = { 0 };

            act("$n turns blue and shivers.", victim, NULL, NULL, TO_ROOM);
            act("A chill sinks deep into your bones.", victim, NULL, NULL,
                TO_CHAR);
            af.where = TO_AFFECTS;
            af.type = skill_lookup("chill touch");
            af.level = (int16_t)level;
            af.duration = 6;
            af.location = APPLY_STR;
            af.modifier = -1;
            af.bitvector = 0;
            affect_join(victim, &af);
        }

        /* hunger! (warmth sucked out */
        if (!IS_NPC(victim)) 
            gain_condition(victim, COND_HUNGER, dam / 20);

        /* let's toast some gear */
        FOR_EACH_MOB_OBJ(obj, victim) {
            cold_effect(obj, level, dam, SPELL_TARGET_OBJ);
        }
        return;
    }

    if (target == SPELL_TARGET_OBJ) /* toast an object */
    {
        Object* obj = (Object*)vo;
        int chance;
        char* msg;

        if (IS_OBJ_STAT(obj, ITEM_BURN_PROOF) || IS_OBJ_STAT(obj, ITEM_NOPURGE)
            || number_range(0, 4) == 0)
            return;

        chance = level / 4 + dam / 10;

        if (chance > 25) chance = (chance - 25) / 2 + 25;
        if (chance > 50) chance = (chance - 50) / 2 + 50;

        if (IS_OBJ_STAT(obj, ITEM_BLESS))
            chance -= 5;

        chance -= obj->level * 2;

        switch (obj->item_type) {
        default:
            return;
        case ITEM_POTION:
            msg = "$p freezes and shatters!";
            chance += 25;
            break;
        case ITEM_DRINK_CON:
            msg = "$p freezes and shatters!";
            chance += 5;
            break;
        }

        chance = URANGE(5, chance, 95);

        if (number_percent() > chance) 
            return;

        if (obj->carried_by != NULL)
            act(msg, obj->carried_by->in_room, obj, NULL, TO_ALL);
        else if (obj->in_room != NULL)
            act(msg, obj->in_room, obj, NULL, TO_ALL);

        extract_obj(obj);
        return;
    }
}

void fire_effect(void* vo, LEVEL level, int dam, SpellTarget target)
{
    if (target == SPELL_TARGET_ROOM) /* nail objects on the floor */
    {
        Room* room = (Room*)vo;
        Object* obj;

        FOR_EACH_ROOM_OBJ(obj, room) {
            fire_effect(obj, level, dam, SPELL_TARGET_OBJ);
        }
        return;
    }

    if (target == SPELL_TARGET_CHAR) /* do the effect on a victim */
    {
        Mobile* victim = (Mobile*)vo;
        Object* obj;

        /* chance of blindness */
        if (!IS_AFFECTED(victim, AFF_BLIND)
            && !saves_spell(level / 4 + (LEVEL)(dam / 20), victim, DAM_FIRE)) {
            Affect af = { 0 };
            act("$n is blinded by smoke!", victim, NULL, NULL, TO_ROOM);
            act("Your eyes tear up from smoke...you can't see a thing!", victim,
                NULL, NULL, TO_CHAR);

            af.where = TO_AFFECTS;
            af.type = skill_lookup("fire breath");
            af.level = (int16_t)level;
            af.duration = (int16_t)number_range(0, level / 10);
            af.location = APPLY_HITROLL;
            af.modifier = -4;
            af.bitvector = AFF_BLIND;

            affect_to_mob(victim, &af);
        }

        /* getting thirsty */
        if (!IS_NPC(victim)) gain_condition(victim, COND_THIRST, dam / 20);

        /* let's toast some gear! */
        FOR_EACH_MOB_OBJ(obj, victim) {
            fire_effect(obj, level, dam, SPELL_TARGET_OBJ);
        }
        return;
    }

    if (target == SPELL_TARGET_OBJ) /* toast an object */
    {
        Object* obj = (Object*)vo;
        int chance;
        char* msg;

        if (IS_OBJ_STAT(obj, ITEM_BURN_PROOF) || IS_OBJ_STAT(obj, ITEM_NOPURGE)
            || number_range(0, 4) == 0)
            return;

        chance = level / 4 + dam / 10;

        if (chance > 25) chance = (chance - 25) / 2 + 25;
        if (chance > 50) chance = (chance - 50) / 2 + 50;

        if (IS_OBJ_STAT(obj, ITEM_BLESS)) chance -= 5;
        chance -= obj->level * 2;

        switch (obj->item_type) {
        default:
            return;
        case ITEM_CONTAINER:
            msg = "$p ignites and burns!";
            break;
        case ITEM_POTION:
            chance += 25;
            msg = "$p bubbles and boils!";
            break;
        case ITEM_SCROLL:
            chance += 50;
            msg = "$p crackles and burns!";
            break;
        case ITEM_STAFF:
            chance += 10;
            msg = "$p smokes and chars!";
            break;
        case ITEM_WAND:
            msg = "$p sparks and sputters!";
            break;
        case ITEM_FOOD:
            msg = "$p blackens and crisps!";
            break;
        case ITEM_PILL:
            msg = "$p melts and drips!";
            break;
        }

        chance = URANGE(5, chance, 95);

        if (number_percent() > chance)
            return;

        if (obj->carried_by != NULL)
            act(msg, obj->carried_by, obj, NULL, TO_ALL);
        else if (obj->in_room != NULL && ROOM_HAS_MOBS(obj->in_room))
            act(msg, obj->in_room, obj, NULL, TO_ALL);

        if (OBJ_HAS_OBJS(obj)) {
            /* dump the contents */
            Object* t_obj;
            FOR_EACH_OBJ_CONTENT(t_obj, obj) {
                obj_from_obj(t_obj);
                if (obj->in_room != NULL)
                    obj_to_room(t_obj, obj->in_room);
                else if (obj->carried_by != NULL)
                    obj_to_room(t_obj, obj->carried_by->in_room);
                else {
                    extract_obj(t_obj);
                    continue;
                }
                fire_effect(t_obj, level / 2, dam / 2, SPELL_TARGET_OBJ);
            }
        }

        extract_obj(obj);
        return;
    }
}

void poison_effect(void* vo, LEVEL level, int dam, SpellTarget target)
{
    if (target == SPELL_TARGET_ROOM) /* nail objects on the floor */
    {
        Room* room = (Room*)vo;
        Object* obj;

        FOR_EACH_ROOM_OBJ(obj, room) {
            poison_effect(obj, level, dam, SPELL_TARGET_OBJ);
        }
        return;
    }

    if (target == SPELL_TARGET_CHAR) /* do the effect on a victim */
    {
        Mobile* victim = (Mobile*)vo;
        Object* obj;

        /* chance of poisoning */
        if (!saves_spell(level / 4 + (LEVEL)(dam / 20), victim, DAM_POISON)) {
            Affect af = { 0 };

            send_to_char("You feel poison coursing through your veins.\n\r",
                         victim);
            act("$n looks very ill.", victim, NULL, NULL, TO_ROOM);

            af.where = TO_AFFECTS;
            af.type = gsn_poison;
            af.level = level;
            af.duration = level / 2;
            af.location = APPLY_STR;
            af.modifier = -1;
            af.bitvector = AFF_POISON;
            affect_join(victim, &af);
        }

        /* equipment */
        FOR_EACH_MOB_OBJ(obj, victim) {
            poison_effect(obj, level, dam, SPELL_TARGET_OBJ);
        }
        return;
    }

    if (target == SPELL_TARGET_OBJ) /* do some poisoning */
    {
        Object* obj = (Object*)vo;
        int chance;

        if (IS_OBJ_STAT(obj, ITEM_BURN_PROOF) || IS_OBJ_STAT(obj, ITEM_BLESS)
            || number_range(0, 4) == 0)
            return;

        chance = level / 4 + dam / 10;
        if (chance > 25) chance = (chance - 25) / 2 + 25;
        if (chance > 50) chance = (chance - 50) / 2 + 50;

        chance -= obj->level * 2;

        switch (obj->item_type) {
        default:
            return;
        case ITEM_FOOD:
            break;
        case ITEM_DRINK_CON:
            if (obj->drink_con.current == obj->drink_con.capacity)
                return;
            break;
        }

        chance = URANGE(5, chance, 95);

        if (number_percent() > chance) return;

        obj->drink_con.poisoned = 1;
        return;
    }
}

void shock_effect(void* vo, LEVEL level, int dam, SpellTarget target)
{
    if (target == SPELL_TARGET_ROOM) {
        Room* room = (Room*)vo;
        Object* obj;

        FOR_EACH_ROOM_OBJ(obj, room) {
            shock_effect(obj, level, dam, SPELL_TARGET_OBJ);
        }
        return;
    }

    if (target == SPELL_TARGET_CHAR) {
        Mobile* victim = (Mobile*)vo;
        Object* obj;

        /* daze and confused? */
        if (!saves_spell(level / 4 + (LEVEL)(dam / 20), victim, DAM_LIGHTNING)) {
            send_to_char("Your muscles stop responding.\n\r", victim);
            DAZE_STATE(victim, UMAX(12, (int16_t)((level / 4) + (dam / 20))));
        }

        /* toast some gear */
        FOR_EACH_MOB_OBJ(obj, victim) {
            shock_effect(obj, level, dam, SPELL_TARGET_OBJ);
        }
        return;
    }

    if (target == SPELL_TARGET_OBJ) {
        Object* obj = (Object*)vo;
        int chance;
        char* msg;

        if (IS_OBJ_STAT(obj, ITEM_BURN_PROOF) || IS_OBJ_STAT(obj, ITEM_NOPURGE)
            || number_range(0, 4) == 0)
            return;

        chance = level / 4 + dam / 10;

        if (chance > 25) 
            chance = (chance - 25) / 2 + 25;
        if (chance > 50) 
            chance = (chance - 50) / 2 + 50;

        if (IS_OBJ_STAT(obj, ITEM_BLESS)) 
            chance -= 5;

        chance -= obj->level * 2;

        switch (obj->item_type) {
        default:
            return;
        case ITEM_WAND:
        case ITEM_STAFF:
            chance += 10;
            msg = "$p overloads and explodes!";
            break;
        case ITEM_JEWELRY:
            chance -= 10;
            msg = "$p is fused into a worthless lump.";
        }

        chance = URANGE(5, chance, 95);

        if (number_percent() > chance)
            return;

        if (obj->carried_by != NULL)
            act(msg, obj->carried_by, obj, NULL, TO_ALL);
        else if (obj->in_room != NULL && ROOM_HAS_MOBS(obj->in_room))
            act(msg, obj->in_room, obj, NULL, TO_ALL);

        extract_obj(obj);
        return;
    }
}
