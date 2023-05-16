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
const struct flag_type act_flags[] = {{"npc", BIT(0), false},
                                      {"sentinel", BIT(1), true},
                                      {"scavenger", BIT(2), true},
                                      {"aggressive", BIT(5), true},
                                      {"stay_area", BIT(6), true},
                                      {"wimpy", BIT(7), true},
                                      {"pet", BIT(8), true},
                                      {"train", BIT(9), true},
                                      {"practice", BIT(10), true},
                                      {"undead", BIT(14), true},
                                      {"cleric", BIT(16), true},
                                      {"mage", BIT(17), true},
                                      {"thief", BIT(18), true},
                                      {"warrior", BIT(19), true},
                                      {"noalign", BIT(20), true},
                                      {"nopurge", BIT(21), true},
                                      {"outdoors", BIT(22), true},
                                      {"indoors", BIT(24), true},
                                      {"healer", BIT(26), true},
                                      {"gain", BIT(27), true},
                                      {"update_always", BIT(28), true},
                                      {"changer", BIT(29), true},
                                      {NULL, 0, false}};

const struct flag_type plr_flags[] = {{"npc", BIT(0), false},
                                      {"autoassist", BIT(2), false},
                                      {"autoexit", BIT(3),false},
                                      {"autoloot", BIT(4), false},
                                      {"autosac", BIT(5), false},
                                      {"autogold", BIT(6), false},
                                      {"autosplit", BIT(7), false},
                                      {"holylight", BIT(13), false},
                                      {"can_loot", BIT(15), false},
                                      {"nosummon", BIT(16), false},
                                      {"nofollow", BIT(17), false},
                                      {"colour", BIT(19), false},
                                      {"permit", BIT(20), true},
                                      {"log", BIT(22), false},
                                      {"deny", BIT(23), false},
                                      {"freeze", BIT(24), false},
                                      {"thief", BIT(25), false},
                                      {"killer", BIT(26), false},
                                      {NULL, 0, 0}};

const struct flag_type affect_flags[]
    = {{"blind", BIT(0), true},        {"invisible", BIT(1), true},
       {"detect_evil", BIT(2), true},  {"detect_invis", BIT(3),true},
       {"detect_magic", BIT(4), true}, {"detect_hidden", BIT(5), true},
       {"detect_good", BIT(6), true},  {"sanctuary", BIT(7), true},
       {"faerie_fire", BIT(8), true},  {"infrared", BIT(9), true},
       {"curse", BIT(10), true},        {"poison", BIT(12), true},
       {"protect_evil", BIT(13), true}, {"protect_good", BIT(14), true},
       {"sneak", BIT(15), true},        {"hide", BIT(16), true},
       {"sleep", BIT(17), true},        {"charm", BIT(18), true},
       {"flying", BIT(19), true},       {"pass_door", BIT(20), true},
       {"haste", BIT(21), true},        {"calm", BIT(22), true},
       {"plague", BIT(23), true},       {"weaken", BIT(24), true},
       {"dark_vision", BIT(25), true},  {"berserk", BIT(26), true},
       {"swim", BIT(27), true},        {"regeneration", BIT(28), true},
       {"slow", BIT(29), true},        {NULL, 0, 0}};

const struct flag_type off_flags[]
    = {{"area_attack", BIT(0), true},    {"backstab", BIT(1), true},
       {"bash", BIT(2), true},           {"berserk", BIT(3),true},
       {"disarm", BIT(4), true},         {"dodge", BIT(5), true},
       {"fade", BIT(6), true},           {"fast", BIT(7), true},
       {"kick", BIT(8), true},           {"dirt_kick", BIT(9), true},
       {"parry", BIT(10), true},          {"rescue", BIT(11), true},
       {"tail", BIT(12), true},           {"trip", BIT(13), true},
       {"crush", BIT(14), true},          {"assist_all", BIT(15), true},
       {"assist_align", BIT(16), true},   {"assist_race", BIT(17), true},
       {"assist_players", BIT(18), true}, {"assist_guard", BIT(19), true},
       {"assist_vnum", BIT(20), true},    {NULL, 0, 0}};

const struct flag_type imm_flags[]
    = {{"summon", BIT(0), true},    {"charm", BIT(1), true},   {"magic", BIT(2), true},
       {"weapon", BIT(3),true},    {"bash", BIT(4), true},    {"pierce", BIT(5), true},
       {"slash", BIT(6), true},     {"fire", BIT(7), true},    {"cold", BIT(8), true},
       {"lightning", BIT(9), true}, {"acid", BIT(10), true},    {"poison", BIT(11), true},
       {"negative", BIT(12), true},  {"holy", BIT(13), true},    {"energy", BIT(14), true},
       {"mental", BIT(15), true},    {"disease", BIT(16), true}, {"drowning", BIT(17), true},
       {"light", BIT(18), true},     {"sound", BIT(19), true},   {"wood", BIT(23), true},
       {"silver", BIT(24), true},    {"iron", BIT(25), true},    {NULL, 0, 0}};

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
