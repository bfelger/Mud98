////////////////////////////////////////////////////////////////////////////////
// native_methods.h
// Enumerate native methods for Mud98 Entities in Lox
////////////////////////////////////////////////////////////////////////////////

#include "native.h"

#include <act_comm.h>
#include <comm.h>

#include <entities/area.h>
#include <entities/faction.h>
#include <entities/entity.h>
#include <entities/mobile.h>
#include <entities/object.h>
#include <entities/room.h>

Table native_methods;

static Value send_lox(Value receiver, int arg_count, Value* args)
{
    if (arg_count == 0) {
        runtime_error("send() requires at least one argument.");
        return FALSE_VAL;
    }

    if (!IS_ENTITY(receiver)) {
        runtime_error("send() called from a non-entity.");
        return FALSE_VAL;
    }

    if (IS_MOBILE(receiver) && AS_MOBILE(receiver)->desc == NULL) {
        // Skip NPCs (for now)
        return TRUE_VAL;
    }

    if (IS_MOBILE(receiver)) {
        Mobile* mob = AS_MOBILE(receiver);

        // Skip NPCs (for now)
        if (mob->pcdata == NULL)
            return TRUE_VAL;

        for (int i = 0; i < arg_count; i++) {
            char* msg = string_value(args[i]);
            act(msg, mob, NULL, NULL, TO_CHAR);
        }
    }

    return TRUE_VAL;
}

static Value say_lox(Value receiver, int arg_count, Value* args)
{
    if (arg_count == 0) {
        runtime_error("say() requires at least one argument.");
        return FALSE_VAL;
    }

    if (!IS_ENTITY(receiver)) {
        runtime_error("say() called from a non-entity.");
        return FALSE_VAL;
    }

    int first_arg = 0;
    Entity* ch = AS_ENTITY(receiver);
    Entity* vch = NULL;

    if (IS_ENTITY(args[first_arg])) {
        vch = AS_ENTITY(args[first_arg++]);
    }

    for (int i = first_arg; i < arg_count; i++) {
        char* msg = string_value(args[i]);
        act(COLOR_SAY "$n says '" COLOR_SAY_TEXT "$T" COLOR_SAY "'" COLOR_CLEAR "", ch, vch, msg, TO_ROOM);
    }

    return TRUE_VAL;
}

static Value echo_lox(Value receiver, int arg_count, Value* args)
{
    if (arg_count == 0) {
        runtime_error("echo() requires at least one argument.");
        return FALSE_VAL;
    }

    if (!IS_ENTITY(receiver)) {
        runtime_error("echo() called from a non-entity.");
        return FALSE_VAL;
    }

    int first_arg = 0;
    Entity* ch = AS_ENTITY(receiver);
    Entity* vch = NULL;

    if (IS_ENTITY(args[first_arg])) {
        vch = AS_ENTITY(args[first_arg++]);
    }

    for (int i = first_arg; i < arg_count; i++) {
        char* msg = string_value(args[i]);
        act(msg, ch, NULL, vch, TO_ROOM);
    }

    return TRUE_VAL;
}

static Value grant_skill_lox(Value receiver, int arg_count, Value* args)
{
    if (arg_count != 2 || !IS_STRING(args[0]) || !IS_INT(args[1])) {
        runtime_error("grant_skill() requires two arguments: a skill name and skill level adjustment.");
        return FALSE_VAL;
    }

    if (!IS_MOBILE(receiver) || AS_MOBILE(receiver)->pcdata == NULL) {
        runtime_error("grant_skill() called from a non-player.");
        return FALSE_VAL;
    }

    Mobile* mob = AS_MOBILE(receiver);
    SKNUM sn = skill_lookup(string_value(args[0]));

    if (sn < 0) {
        runtime_error("grant_skill() called with invalid skill name.");
        return FALSE_VAL;
    }

    int level_adjustment = AS_INT(args[1]);

    mob->pcdata->learned[sn] += level_adjustment;
    return TRUE_VAL;
}

static Value has_skill_lox(Value receiver, int arg_count, Value* args)
{
    if (arg_count != 1 || !IS_STRING(args[0])) {
        runtime_error("has_skill() requires one argument: a skill name.");
        return FALSE_VAL;
    }

    if (!IS_MOBILE(receiver) || AS_MOBILE(receiver)->pcdata == NULL) {
        runtime_error("has_skill() called from a non-player.");
        return FALSE_VAL;
    }

    Mobile* mob = AS_MOBILE(receiver);
    SKNUM sn = skill_lookup(string_value(args[0]));

    if (sn < 0) {
        runtime_error("has_skill() called with invalid skill name.");
        return FALSE_VAL;
    }

    return BOOL_VAL(mob->pcdata->learned[sn] > 0);
}

static Value set_recall_lox(Value receiver, int arg_count, Value* args)
{
    if (arg_count != 1 || !IS_INT(args[0])) {
        runtime_error("set_recall() requires one argument: a room vnum.");
        return FALSE_VAL;
    }

    if (!IS_MOBILE(receiver) || AS_MOBILE(receiver)->pcdata == NULL) {
        runtime_error("set_recall() called from a non-player.");
        return FALSE_VAL;
    }

    Mobile* mob = AS_MOBILE(receiver);
    PlayerData* pcdata = mob->pcdata;
    VNUM recall_vnum = AS_INT(args[0]);

    pcdata->recall = recall_vnum;
    return TRUE_VAL;
}

const NativeMethodEntry native_method_entries[] = {
    { "is_area",            is_area_lox                     },
    { "is_area_data",       is_area_data_lox                },
    { "is_mob",             is_mob_lox                      },
    { "is_mob_proto",       is_mob_proto_lox                },
    { "is_obj",             is_obj_lox                      },
    { "is_obj_proto",       is_obj_proto_lox                },
    { "is_room",            is_room_lox                     },
    { "is_room_data",       is_room_data_lox                },
    { "can_finish_quest",   can_finish_quest_lox            },
    { "can_quest",          can_quest_lox                   },
    { "finish_quest",       finish_quest_lox                },
    { "grant_quest",        grant_quest_lox                 },
    { "has_quest",          has_quest_lox                   },
    { "echo",               echo_lox                        },
    { "send",               send_lox                        },
    { "say",                say_lox                         },
    { "get_reputation",     faction_get_reputation_lox      },
    { "adjust_reputation",  faction_adjust_reputation_lox   },
    { "set_reputation",     faction_set_reputation_lox      },
    { "is_enemy",           faction_is_enemy_lox            },
    { "is_ally",            faction_is_ally_lox             },
    { "grant_skill",        grant_skill_lox                 },
    { "has_skill",          has_skill_lox                   },
    { "set_recall",         set_recall_lox                  },
    { NULL,                 NULL                            },
};