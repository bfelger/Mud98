////////////////////////////////////////////////////////////////////////////////
// mobile.h
// Mobiles
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__ENTITIES__MOBILE_H
#define MUD98__ENTITIES__MOBILE_H

typedef struct mobile_t Mobile;

#include "merc.h"

#include "interp.h"
#include "note.h"

#include "area.h"
#include "descriptor.h"
#include "mob_memory.h"
#include "mob_prototype.h"
#include "object.h"
#include "player_data.h"
#include "room.h"

#include "data/class.h"
#include "data/damage.h"
#include "data/mobile_data.h"
#include "data/stats.h"

#include <stdbool.h>

typedef struct mobile_t {
    Mobile* next;
    Mobile* next_in_room;
    Mobile* master;
    Mobile* leader;
    Mobile* fighting;
    Mobile* reply;
    Mobile* pet;
    Mobile* mprog_target;
    MobMemory* memory;
    SpecFunc* spec_fun;
    MobPrototype* prototype;
    Descriptor* desc;
    Affect* affected;
    NoteData* pnote;
    Object* carrying;
    Object* on;
    Room* in_room;
    Room* was_in_room;
    Area* zone;
    PlayerData* pcdata;
    CharGenData* gen_data;
    char* name;
    char* material;
    char* short_descr;
    char* long_descr;
    char* description;
    char* prompt;
    char* prefix;
    time_t logon;
    time_t played;
    int id;
    int version;
    int lines; /* for the pager */
    int exp;
    FLAGS act_flags;
    FLAGS comm_flags;
    FLAGS wiznet;
    FLAGS imm_flags;
    FLAGS res_flags;
    FLAGS vuln_flags;
    FLAGS affect_flags;
    FLAGS form;
    FLAGS parts;
    FLAGS atk_flags;
    int wait;
    int16_t damage[3];
    DamageType dam_type;
    Position start_pos;
    Position default_pos;
    int16_t mprog_delay;
    int16_t group;
    int16_t clan;
    Sex sex;
    int16_t ch_class;
    int16_t race;
    LEVEL level;
    int16_t trust;
    MobSize size;
    Position position;
    int16_t practice;
    int16_t train;
    int16_t carry_weight;
    int16_t carry_number;
    int16_t saving_throw;
    int16_t alignment;
    int16_t hitroll;
    int16_t damroll;
    int16_t armor[AC_COUNT];
    int16_t wimpy;
    int16_t perm_stat[STAT_COUNT];
    int16_t mod_stat[STAT_COUNT];
    int16_t invis_level;
    int16_t incog_level;
    int16_t timer;
    int16_t daze;
    int16_t hit;
    int16_t max_hit;
    int16_t mana;
    int16_t max_mana;
    int16_t move;
    int16_t max_move;
    int16_t gold;
    int16_t silver;
    bool valid;
} Mobile;

#define IS_NPC(ch)            (IS_SET((ch)->act_flags, ACT_IS_NPC))
#define IS_IMMORTAL(ch)       (get_trust(ch) >= LEVEL_IMMORTAL)
#define IS_HERO(ch)           (get_trust(ch) >= LEVEL_HERO)
#define IS_TRUSTED(ch, level) (get_trust((ch)) >= (level))
#define IS_AFFECTED(ch, sn)   (IS_SET((ch)->affect_flags, (sn)))

#define GET_AGE(ch)                                                            \
    ((int)(17 + ((ch)->played + current_time - (ch)->logon) / 72000))

#define IS_GOOD(ch)    (ch->alignment >= 350)
#define IS_EVIL(ch)    (ch->alignment <= -350)
#define IS_NEUTRAL(ch) (!IS_GOOD(ch) && !IS_EVIL(ch))

#define IS_AWAKE(ch)   (ch->position > POS_SLEEPING)
#define GET_AC(ch, type)                                                       \
    ((ch)->armor[type]                                                         \
     + (IS_AWAKE(ch) ? dex_mod[get_curr_stat(ch, STAT_DEX)].defensive : 0))
#define GET_HITROLL(ch)                                                        \
    ((ch)->hitroll + str_mod[get_curr_stat(ch, STAT_STR)].tohit)
#define GET_DAMROLL(ch)                                                        \
    ((ch)->damroll + str_mod[get_curr_stat(ch, STAT_STR)].todam)

#define IS_OUTSIDE(ch)         (!IS_SET((ch)->in_room->room_flags, ROOM_INDOORS))

#define WAIT_STATE(ch, npulse) ((ch)->wait = UMAX((ch)->wait, (npulse)))
#define DAZE_STATE(ch, npulse) ((ch)->daze = UMAX((ch)->daze, (npulse)))
#define get_carry_weight(ch)                                                   \
    ((ch)->carry_weight + (ch)->silver / 10 + (ch)->gold * 2 / 5)

#define HAS_TRIGGER(ch, trig) (IS_SET((ch)->prototype->mprog_flags, (trig)))
#define IS_SWITCHED(ch) (ch->desc && ch->desc->original)
#define IS_BUILDER(ch, Area) (!IS_NPC(ch) && !IS_SWITCHED(ch) && \
                (ch->pcdata->security >= Area->security \
                || strstr(Area->builders, ch->name) \
                || strstr(Area->builders, "All")))
#define PERS(ch, looker)                                                       \
    (can_see(looker, (ch)) ? (IS_NPC(ch) ? (ch)->short_descr : (ch)->name)     \
                           : "someone")

// We have to cast to int because MSVC's static analyzer isn't very good at 
// mapping to int on its own. Many false positive for unbounded enum use even
// with checks.
#define GET_ARCH(ch)    (CHECK_ARCH((int)class_table[ch->ch_class].arch))

void free_mobile(Mobile* ch);
Mobile* new_mobile();

extern Mobile* mob_list;
extern Mobile* mob_free;

extern int mob_count;

#endif // !MUD98__ENTITIES__CHAR_DATA_H
