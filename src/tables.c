/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
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

#include "merc.h"
#include "tables.h"

#include <stdio.h>
#include <sys/types.h>
#include <time.h>

/* for clans */
const struct clan_type clan_table[MAX_CLAN] = {
    /*  name,		who entry,	death-transfer room,	independent */
    /* independent should be false if is a real clan */
    {"", "", ROOM_VNUM_ALTAR, true},
    {"loner", "[ Loner ] ", ROOM_VNUM_ALTAR, true},
    {"rom", "[  ROM  ] ", ROOM_VNUM_ALTAR, false}};

/* for position */
const struct position_type position_table[]
    = {{"dead", "dead"},           {"mortally wounded", "mort"},
       {"incapacitated", "incap"}, {"stunned", "stun"},
       {"sleeping", "sleep"},      {"resting", "rest"},
       {"sitting", "sit"},         {"fighting", "fight"},
       {"standing", "stand"},      {NULL, NULL}};

/* for sex */
const struct sex_type sex_table[]
    = {{"none"}, {"male"}, {"female"}, {"either"}, {NULL}};

/* for sizes */
const struct size_type size_table[] = {{"tiny"},
                                       {"small"},
                                       {"medium"},
                                       {"large"},
                                       {
                                           "huge",
                                       },
                                       {"giant"},
                                       {NULL}};

/* various flag tables */
const struct flag_type act_flags[] = {{"npc", A, false},
                                      {"sentinel", B, true},
                                      {"scavenger", C, true},
                                      {"aggressive", F, true},
                                      {"stay_area", G, true},
                                      {"wimpy", H, true},
                                      {"pet", I, true},
                                      {"train", J, true},
                                      {"practice", K, true},
                                      {"undead", O, true},
                                      {"cleric", Q, true},
                                      {"mage", R, true},
                                      {"thief", S, true},
                                      {"warrior", T, true},
                                      {"noalign", U, true},
                                      {"nopurge", V, true},
                                      {"outdoors", W, true},
                                      {"indoors", Y, true},
                                      {"healer", aa, true},
                                      {"gain", bb, true},
                                      {"update_always", cc, true},
                                      {"changer", dd, true},
                                      {NULL, 0, false}};

const struct flag_type plr_flags[] = {{"npc", A, false},
                                      {"autoassist", C, false},
                                      {"autoexit", D, false},
                                      {"autoloot", E, false},
                                      {"autosac", F, false},
                                      {"autogold", G, false},
                                      {"autosplit", H, false},
                                      {"holylight", N, false},
                                      {"can_loot", P, false},
                                      {"nosummon", Q, false},
                                      {"nofollow", R, false},
                                      {"colour", T, false},
                                      {"permit", U, true},
                                      {"log", W, false},
                                      {"deny", X, false},
                                      {"freeze", Y, false},
                                      {"thief", Z, false},
                                      {"killer", aa, false},
                                      {NULL, 0, 0}};

const struct flag_type affect_flags[]
    = {{"blind", A, true},        {"invisible", B, true},
       {"detect_evil", C, true},  {"detect_invis", D, true},
       {"detect_magic", E, true}, {"detect_hidden", F, true},
       {"detect_good", G, true},  {"sanctuary", H, true},
       {"faerie_fire", I, true},  {"infrared", J, true},
       {"curse", K, true},        {"poison", M, true},
       {"protect_evil", N, true}, {"protect_good", O, true},
       {"sneak", P, true},        {"hide", Q, true},
       {"sleep", R, true},        {"charm", S, true},
       {"flying", T, true},       {"pass_door", U, true},
       {"haste", V, true},        {"calm", W, true},
       {"plague", X, true},       {"weaken", Y, true},
       {"dark_vision", Z, true},  {"berserk", aa, true},
       {"swim", bb, true},        {"regeneration", cc, true},
       {"slow", dd, true},        {NULL, 0, 0}};

const struct flag_type off_flags[]
    = {{"area_attack", A, true},    {"backstab", B, true},
       {"bash", C, true},           {"berserk", D, true},
       {"disarm", E, true},         {"dodge", F, true},
       {"fade", G, true},           {"fast", H, true},
       {"kick", I, true},           {"dirt_kick", J, true},
       {"parry", K, true},          {"rescue", L, true},
       {"tail", M, true},           {"trip", N, true},
       {"crush", O, true},          {"assist_all", P, true},
       {"assist_align", Q, true},   {"assist_race", R, true},
       {"assist_players", S, true}, {"assist_guard", T, true},
       {"assist_vnum", U, true},    {NULL, 0, 0}};

const struct flag_type imm_flags[]
    = {{"summon", A, true},    {"charm", B, true},   {"magic", C, true},
       {"weapon", D, true},    {"bash", E, true},    {"pierce", F, true},
       {"slash", G, true},     {"fire", H, true},    {"cold", I, true},
       {"lightning", J, true}, {"acid", K, true},    {"poison", L, true},
       {"negative", M, true},  {"holy", N, true},    {"energy", O, true},
       {"mental", P, true},    {"disease", Q, true}, {"drowning", R, true},
       {"light", S, true},     {"sound", T, true},   {"wood", X, true},
       {"silver", Y, true},    {"iron", Z, true},    {NULL, 0, 0}};

const struct flag_type form_flags[]
    = {{"edible", FORM_EDIBLE, true},
       {"poison", FORM_POISON, true},
       {"magical", FORM_MAGICAL, true},
       {"instant_decay", FORM_INSTANT_DECAY, true},
       {"other", FORM_OTHER, true},
       {"animal", FORM_ANIMAL, true},
       {"sentient", FORM_SENTIENT, true},
       {"undead", FORM_UNDEAD, true},
       {"construct", FORM_CONSTRUCT, true},
       {"mist", FORM_MIST, true},
       {"intangible", FORM_INTANGIBLE, true},
       {"biped", FORM_BIPED, true},
       {"centaur", FORM_CENTAUR, true},
       {"insect", FORM_INSECT, true},
       {"spider", FORM_SPIDER, true},
       {"crustacean", FORM_CRUSTACEAN, true},
       {"worm", FORM_WORM, true},
       {"blob", FORM_BLOB, true},
       {"mammal", FORM_MAMMAL, true},
       {"bird", FORM_BIRD, true},
       {"reptile", FORM_REPTILE, true},
       {"snake", FORM_SNAKE, true},
       {"dragon", FORM_DRAGON, true},
       {"amphibian", FORM_AMPHIBIAN, true},
       {"fish", FORM_FISH, true},
       {"cold_blood", FORM_COLD_BLOOD, true},
       {NULL, 0, 0}};

const struct flag_type part_flags[] = {{"head", PART_HEAD, true},
                                       {"arms", PART_ARMS, true},
                                       {"legs", PART_LEGS, true},
                                       {"heart", PART_HEART, true},
                                       {"brains", PART_BRAINS, true},
                                       {"guts", PART_GUTS, true},
                                       {"hands", PART_HANDS, true},
                                       {"feet", PART_FEET, true},
                                       {"fingers", PART_FINGERS, true},
                                       {"ear", PART_EAR, true},
                                       {"eye", PART_EYE, true},
                                       {"long_tongue", PART_LONG_TONGUE, true},
                                       {"eyestalks", PART_EYESTALKS, true},
                                       {"tentacles", PART_TENTACLES, true},
                                       {"fins", PART_FINS, true},
                                       {"wings", PART_WINGS, true},
                                       {"tail", PART_TAIL, true},
                                       {"claws", PART_CLAWS, true},
                                       {"fangs", PART_FANGS, true},
                                       {"horns", PART_HORNS, true},
                                       {"scales", PART_SCALES, true},
                                       {"tusks", PART_TUSKS, true},
                                       {NULL, 0, 0}};

const struct flag_type comm_flags[]
    = {{"quiet", COMM_QUIET, true},
       {"deaf", COMM_DEAF, true},
       {"nowiz", COMM_NOWIZ, true},
       {"noclangossip", COMM_NOAUCTION, true},
       {"nogossip", COMM_NOGOSSIP, true},
       {"noquestion", COMM_NOQUESTION, true},
       {"nomusic", COMM_NOMUSIC, true},
       {"noclan", COMM_NOCLAN, true},
       {"noquote", COMM_NOQUOTE, true},
       {"shoutsoff", COMM_SHOUTSOFF, true},
       {"compact", COMM_COMPACT, true},
       {"brief", COMM_BRIEF, true},
       {"prompt", COMM_PROMPT, true},
       {"combine", COMM_COMBINE, true},
       {"telnet_ga", COMM_TELNET_GA, true},
       {"show_affects", COMM_SHOW_AFFECTS, true},
       {"nograts", COMM_NOGRATS, true},
       {"noemote", COMM_NOEMOTE, false},
       {"noshout", COMM_NOSHOUT, false},
       {"notell", COMM_NOTELL, false},
       {"nochannels", COMM_NOCHANNELS, false},
       {"snoop_proof", COMM_SNOOP_PROOF, false},
       {"afk", COMM_AFK, true},
       {NULL, 0, 0}};
