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

#include "save.h"

#include "benchmark.h"
#include "color.h"
#include "comm.h"
#include "config.h"
#include "db.h"
#include "digest.h"
#include "handler.h"
#include "lookup.h"
#include "magic.h"
#include "recycle.h"
#include "skills.h"
#include "stringutils.h"
#include "tables.h"
#include "vt.h"

#include "entities/descriptor.h"
#include "entities/object.h"

#include "data/mobile_data.h"
#include "data/player.h"
#include "data/race.h"
#include "data/skill.h"

#include "lox/lox.h"

#include <ctype.h>
#include <errno.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

extern int _filbuf args((FILE*));

char* print_flags(FLAGS flag)
{
    int count, pos = 0;
    static char buf[52];

    for (count = 0; count < 32; count++) {
        if (IS_SET(flag, 1 << count)) {
            if (count < 26)
                buf[pos] = (char)('A' + count);
            else
                buf[pos] = (char)('a' + (count - 26));
            pos++;
        }
    }

    if (pos == 0) {
        buf[pos] = '0';
        pos++;
    }

    buf[pos] = '\0';

    return buf;
}

// Array of containers read for proper re-nesting of objects.
#define MAX_NEST 100
static Object* rgObjNest[MAX_NEST];

// Local functions.
void fwrite_char(Mobile* ch, FILE* fp);
void fwrite_obj(Mobile* ch, Object* obj, FILE* fp, int iNest);
void fwrite_pet(Mobile* pet, FILE* fp);
void fwrite_themes(Mobile* ch, FILE* fp);
void fwrite_quests(Mobile* ch, FILE* fp);

void fread_char(Mobile* ch, FILE* fp);
void fread_pet(Mobile* ch, FILE* fp);
void fread_obj(Mobile* ch, FILE* fp);
void fread_theme(Mobile* ch, FILE* fp);
void fread_quests(Mobile* ch, FILE* fp);

void save_char_obj(Mobile* ch)
{
    char strsave[MAX_INPUT_LENGTH];
    FILE* fp;

    if (IS_NPC(ch)) 
        return;

    if (ch->desc != NULL && ch->desc->original != NULL) 
        ch = ch->desc->original;

    /* create god log */
    if (IS_IMMORTAL(ch) || ch->level >= LEVEL_IMMORTAL) {
        sprintf(strsave, "%s%s", cfg_get_gods_dir(), capitalize(NAME_STR(ch)));
        OPEN_OR_RETURN(fp = open_write_file(strsave));
        fprintf(fp, "Lev %2d Trust %2d  %s%s\n", ch->level, get_trust(ch),
            NAME_STR(ch), ch->pcdata->title);
        close_file(fp);
    }

    sprintf(strsave, "%s%s", cfg_get_player_dir(), capitalize(NAME_STR(ch)));
    OPEN_OR_RETURN(fp = open_write_file(strsave));

    fwrite_char(ch, fp);
    if (ch->carrying != NULL) 
        fwrite_obj(ch, ch->carrying, fp, 0);
    /* save the pets */
    if (ch->pet != NULL && ch->pet->in_room == ch->in_room)
        fwrite_pet(ch->pet, fp);
    fwrite_themes(ch, fp);
    fwrite_quests(ch, fp);
    
    fprintf(fp, "#END\n");

    close_file(fp);
}

// Write the char.
void fwrite_char(Mobile* ch, FILE* fp)
{
    Affect* affect;
    int sn, gn, pos;

    fprintf(fp, "#%s\n", IS_NPC(ch) ? "MOB" : "PLAYER");

    fprintf(fp, "Name %s~\n", NAME_STR(ch));
    fprintf(fp, "Id   %d\n", ch->id);
    fprintf(fp, "LogO " TIME_FMT "\n", current_time);
    fprintf(fp, "Vers %d\n", 5);
    if (ch->short_descr[0] != '\0')fprintf(fp, "ShD  %s~\n", ch->short_descr);
    if (ch->long_descr[0] != '\0') fprintf(fp, "LnD  %s~\n", ch->long_descr);
    if (ch->description[0] != '\0') fprintf(fp, "Desc %s~\n", ch->description);
    if (ch->prompt != NULL || !str_cmp(ch->prompt, "<%hhp %mm %vmv> ")
        || !str_cmp(ch->prompt, "{c<%hhp %mm %vmv>{x "))
        fprintf(fp, "Prom %s~\n", ch->prompt);
    fprintf(fp, "Race %s~\n", race_table[ch->race].name);
    if (ch->clan) fprintf(fp, "Clan %s~\n", clan_table[ch->clan].name);
    fprintf(fp, "Sex  %d\n", ch->sex);
    fprintf(fp, "Cla  %d\n", ch->ch_class);
    fprintf(fp, "Levl %d\n", ch->level);
    if (ch->trust != 0)
        fprintf(fp, "Tru  %d\n", ch->trust);
    fprintf(fp, "Sec  %d\n", ch->pcdata->security);	// OLC
    fprintf(fp, "Plyd %d\n", (int)(ch->played + (current_time - ch->logon)));
    fprintf(fp, "Not  "TIME_FMT" "TIME_FMT" "TIME_FMT" "TIME_FMT" "TIME_FMT"\n", 
            ch->pcdata->last_note, ch->pcdata->last_idea, ch->pcdata->last_penalty,
            ch->pcdata->last_news, ch->pcdata->last_changes);
    fprintf(fp, "Scro %d\n", ch->lines);
    fprintf(fp, "Recall %d\n", ch->pcdata->recall);
    fprintf(fp, "Room %d\n",
            (ch->in_room == get_room(NULL, ROOM_VNUM_LIMBO)
             && ch->was_in_room != NULL)
                ? ch->was_in_room->vnum
            : ch->in_room == NULL ? ch->pcdata->recall
                                  : ch->in_room->vnum);

    fprintf(fp, "HMV  %d %d %d %d %d %d\n", ch->hit, ch->max_hit, ch->mana,
            ch->max_mana, ch->move, ch->max_move);
    fprintf(fp, "Gold %d\n", ch->gold);
    fprintf(fp, "Silv %d\n", ch->silver);
    fprintf(fp, "Exp  %d\n", ch->exp);
    if (ch->act_flags != 0) fprintf(fp, "Act  %s\n", print_flags(ch->act_flags));
    if (ch->affect_flags != 0)
        fprintf(fp, "AfBy %s\n", print_flags(ch->affect_flags));
    fprintf(fp, "Comm %s\n", print_flags(ch->comm_flags));
    if (ch->wiznet) fprintf(fp, "Wizn %s\n", print_flags(ch->wiznet));
    if (ch->invis_level) fprintf(fp, "Invi %d\n", ch->invis_level);
    if (ch->incog_level) fprintf(fp, "Inco %d\n", ch->incog_level);
    fprintf(fp, "Pos  %d\n",
            ch->position == POS_FIGHTING ? POS_STANDING : ch->position);
    if (ch->practice != 0) fprintf(fp, "Prac %d\n", ch->practice);
    if (ch->train != 0) fprintf(fp, "Trai %d\n", ch->train);
    if (ch->saving_throw != 0) fprintf(fp, "Save  %d\n", ch->saving_throw);
    fprintf(fp, "Alig  %d\n", ch->alignment);
    if (ch->hitroll != 0) fprintf(fp, "Hit   %d\n", ch->hitroll);
    if (ch->damroll != 0) fprintf(fp, "Dam   %d\n", ch->damroll);
    fprintf(fp, "ACs %d %d %d %d\n", ch->armor[0], ch->armor[1], ch->armor[2],
            ch->armor[3]);
    if (ch->wimpy != 0) fprintf(fp, "Wimp  %d\n", ch->wimpy);
    fprintf(fp, "Attr %d %d %d %d %d\n", ch->perm_stat[STAT_STR],
            ch->perm_stat[STAT_INT], ch->perm_stat[STAT_WIS],
            ch->perm_stat[STAT_DEX], ch->perm_stat[STAT_CON]);

    fprintf(fp, "AMod %d %d %d %d %d\n", ch->mod_stat[STAT_STR],
            ch->mod_stat[STAT_INT], ch->mod_stat[STAT_WIS],
            ch->mod_stat[STAT_DEX], ch->mod_stat[STAT_CON]);

    if (IS_NPC(ch)) { 
        fprintf(fp, "Vnum %"PRVNUM"\n", ch->prototype->vnum); 
    }
    else {
        char digest_buf[256];
        bin_to_hex(digest_buf, ch->pcdata->pwd_digest, ch->pcdata->pwd_digest_len);
        fprintf(fp, "PwdDigest %s~\n", digest_buf);

        if (ch->pcdata->bamfin[0] != '\0')
            fprintf(fp, "Bin  %s~\n", ch->pcdata->bamfin);
        if (ch->pcdata->bamfout[0] != '\0')
            fprintf(fp, "Bout %s~\n", ch->pcdata->bamfout);
        fprintf(fp, "Titl %s~\n", ch->pcdata->title);
        fprintf(fp, "Pnts %d\n", ch->pcdata->points);
        fprintf(fp, "TSex %d\n", ch->pcdata->true_sex);
        fprintf(fp, "LLev %d\n", ch->pcdata->last_level);
        fprintf(fp, "HMVP %d %d %d\n", ch->pcdata->perm_hit,
                ch->pcdata->perm_mana, ch->pcdata->perm_move);
        fprintf(fp, "Cnd  %d %d %d %d\n", ch->pcdata->condition[0],
                ch->pcdata->condition[1], ch->pcdata->condition[2],
                ch->pcdata->condition[3]);
        if (ch->pcdata->theme_config.current_theme_name)
            fprintf(fp, "Theme %s~\n", ch->pcdata->theme_config.current_theme_name);
        fprintf(fp, "ThemeConfig %d %d %d %d %d\n",
            ch->pcdata->theme_config.hide_24bit,
            ch->pcdata->theme_config.hide_256,
            ch->pcdata->theme_config.xterm,
            ch->pcdata->theme_config.hide_rgb_help,
            0); // Reserved

        /* write alias */
        for (pos = 0; pos < MAX_ALIAS; pos++) {
            if (ch->pcdata->alias[pos] == NULL
                || ch->pcdata->alias_sub[pos] == NULL)
                break;

            fprintf(fp, "Alias %s %s~\n", ch->pcdata->alias[pos],
                    ch->pcdata->alias_sub[pos]);
        }

        for (sn = 0; sn < skill_count; sn++) {
            if (skill_table[sn].name != NULL && ch->pcdata->learned[sn] > 0) {
                fprintf(fp, "Sk %d '%s'\n", ch->pcdata->learned[sn],
                        skill_table[sn].name);
            }
        }

        for (gn = 0; gn < skill_group_count; gn++) {
            if (skill_group_table[gn].name != NULL && ch->pcdata->group_known[gn]) {
                fprintf(fp, "Gr '%s'\n", skill_group_table[gn].name);
            }
        }
    }

    FOR_EACH(affect, ch->affected) {
        if (affect->type < 0 || affect->type >= skill_count)
            continue;

        fprintf(fp, "Affc '%s' %3d %3d %3d %3d %3d %10d\n",
                skill_table[affect->type].name, affect->where, affect->level,
                affect->duration, affect->modifier, affect->location, affect->bitvector);
    }

    fprintf(fp, "End\n\n");
    return;
}

static void fwrite_palette(Color* color, int index, FILE* fp)
{
    // Extra reserved value for future use.
    fprintf(fp, "Palette %d %u %u %u %u 0\n", index, color->mode, color->code[0], 
        color->code[1], color->code[2]);
}

static void fwrite_channel(Color* color, const char* channel, FILE* fp)
{
    // Extra reserved value for future use.
    fprintf(fp, "Channel %s %u %u %u %u 0\n", channel, color->mode, color->code[0],
        color->code[1], color->code[2]);
}

void fwrite_themes(Mobile* ch, FILE* fp)
{
    for (int i = 0; i < MAX_THEMES; ++i) {
        if (ch->pcdata->color_themes[i] == NULL)
            continue;

        ColorTheme* theme = ch->pcdata->color_themes[i];
        fprintf(fp, "#THEME\n");
        fprintf(fp, "Name %s~\n", theme->name);
        fprintf(fp, "Banner %s~\n", theme->banner);
        fprintf(fp, "Info %d %d %d %d\n", theme->type, theme->mode, 
            theme->palette_max, theme->is_public);
        
        for (int j = 0; j < theme->palette_max; ++j)
            fwrite_palette(&theme->palette[j], j, fp);

        for (int j = 0; j < SLOT_MAX; ++j)
            fwrite_channel(&theme->channels[j], color_slot_entries[j].name, fp);

        fprintf(fp, "End\n\n");
    }
}

/* write a pet */
void fwrite_pet(Mobile* pet, FILE* fp)
{
    Affect* affect;

    fprintf(fp, "#PET\n");

    fprintf(fp, "Vnum %"PRVNUM"\n", pet->prototype->vnum);

    fprintf(fp, "Name %s~\n", NAME_STR(pet));
    fprintf(fp, "LogO " TIME_FMT "\n", current_time);
    if (pet->short_descr != pet->prototype->short_descr)
        fprintf(fp, "ShD  %s~\n", pet->short_descr);
    if (pet->long_descr != pet->prototype->long_descr)
        fprintf(fp, "LnD  %s~\n", pet->long_descr);
    if (pet->description != pet->prototype->description)
        fprintf(fp, "Desc %s~\n", pet->description);
    if (pet->race != pet->prototype->race)
        fprintf(fp, "Race %s~\n", race_table[pet->race].name);
    if (pet->clan) fprintf(fp, "Clan %s~\n", clan_table[pet->clan].name);
    fprintf(fp, "Sex  %d\n", pet->sex);
    if (pet->level != pet->prototype->level)
        fprintf(fp, "Levl %d\n", pet->level);
    fprintf(fp, "HMV  %d %d %d %d %d %d\n", pet->hit, pet->max_hit, pet->mana,
            pet->max_mana, pet->move, pet->max_move);
    if (pet->gold > 0) fprintf(fp, "Gold %d\n", pet->gold);
    if (pet->silver > 0) fprintf(fp, "Silv %d\n", pet->silver);
    if (pet->exp > 0) fprintf(fp, "Exp  %d\n", pet->exp);
    if (pet->act_flags != pet->prototype->act_flags)
        fprintf(fp, "Act  %s\n", print_flags(pet->act_flags));
    if (pet->affect_flags != pet->prototype->affect_flags)
        fprintf(fp, "AfBy %s\n", print_flags(pet->affect_flags));
    if (pet->comm_flags != 0) fprintf(fp, "Comm %s\n", print_flags(pet->comm_flags));
    fprintf(fp, "Pos  %d\n",
            pet->position = POS_FIGHTING ? POS_STANDING : pet->position);
    if (pet->saving_throw != 0) fprintf(fp, "Save %d\n", pet->saving_throw);
    if (pet->alignment != pet->prototype->alignment)
        fprintf(fp, "Alig %d\n", pet->alignment);
    if (pet->hitroll != pet->prototype->hitroll)
        fprintf(fp, "Hit  %d\n", pet->hitroll);
    if (pet->damroll != pet->prototype->damage[DICE_BONUS])
        fprintf(fp, "Dam  %d\n", pet->damroll);
    fprintf(fp, "ACs  %d %d %d %d\n", pet->armor[0], pet->armor[1],
            pet->armor[2], pet->armor[3]);
    fprintf(fp, "Attr %d %d %d %d %d\n", pet->perm_stat[STAT_STR],
            pet->perm_stat[STAT_INT], pet->perm_stat[STAT_WIS],
            pet->perm_stat[STAT_DEX], pet->perm_stat[STAT_CON]);
    fprintf(fp, "AMod %d %d %d %d %d\n", pet->mod_stat[STAT_STR],
            pet->mod_stat[STAT_INT], pet->mod_stat[STAT_WIS],
            pet->mod_stat[STAT_DEX], pet->mod_stat[STAT_CON]);

    FOR_EACH(affect, pet->affected) {
        if (affect->type < 0 || affect->type >= skill_count) continue;

        fprintf(fp, "Affc '%s' %3d %3d %3d %3d %3d %10d\n",
                skill_table[affect->type].name, affect->where, affect->level,
                affect->duration, affect->modifier, affect->location, affect->bitvector);
    }

    fprintf(fp, "End\n");
    return;
}

// Write an object and its contents.
void fwrite_obj(Mobile* ch, Object* obj, FILE* fp, int iNest)
{
    ExtraDesc* ed;
    Affect* affect;

    /*
     * Slick recursion to write lists backwards,
     *   so loading them will load in forwards order.
     */
    if (obj->next_content != NULL) 
        fwrite_obj(ch, obj->next_content, fp, iNest);

    // Castrate storage characters.
    if ((ch->level < obj->level - 2 && obj->item_type != ITEM_CONTAINER)
        || obj->item_type == ITEM_KEY
        || (obj->item_type == ITEM_MAP && !obj->value[0]))
        return;

    fprintf(fp, "#O\n");
    fprintf(fp, "Vnum %"PRVNUM"\n", obj->prototype->vnum);
    if (obj->enchanted) 
        fprintf(fp, "Enchanted\n");
    fprintf(fp, "Nest %d\n", iNest);

    /* these data are only used if they do not match the defaults */

    if (!lox_streq(NAME_FIELD(obj), obj->prototype->name))
        fprintf(fp, "Name %s~\n", NAME_STR(obj));
    if (obj->short_descr != obj->prototype->short_descr)
        fprintf(fp, "ShD  %s~\n", obj->short_descr);
    if (obj->description != obj->prototype->description)
        fprintf(fp, "Desc %s~\n", obj->description);
    if (obj->extra_flags != obj->prototype->extra_flags)
        fprintf(fp, "ExtF %d\n", obj->extra_flags);
    if (obj->wear_flags != obj->prototype->wear_flags)
        fprintf(fp, "WeaF %d\n", obj->wear_flags);
    if (obj->item_type != obj->prototype->item_type)
        fprintf(fp, "Ityp %d\n", obj->item_type);
    if (obj->weight != obj->prototype->weight)
        fprintf(fp, "Wt   %d\n", obj->weight);
    if (obj->condition != obj->prototype->condition)
        fprintf(fp, "Cond %d\n", obj->condition);

    /* variable data */

    fprintf(fp, "Wear %d\n", obj->wear_loc);
    if (obj->level != obj->prototype->level)
        fprintf(fp, "Lev  %d\n", obj->level);
    if (obj->timer != 0) 
        fprintf(fp, "Time %d\n", obj->timer);
    fprintf(fp, "Cost %d\n", obj->cost);
    if (obj->value[0] != obj->prototype->value[0]
        || obj->value[1] != obj->prototype->value[1]
        || obj->value[2] != obj->prototype->value[2]
        || obj->value[3] != obj->prototype->value[3]
        || obj->value[4] != obj->prototype->value[4])
        fprintf(fp, "Val  %d %d %d %d %d\n", obj->value[0], obj->value[1],
                obj->value[2], obj->value[3], obj->value[4]);

    switch (obj->item_type) {
    case ITEM_POTION:
    case ITEM_SCROLL:
    case ITEM_PILL:
        if (obj->value[1] > 0) {
            fprintf(fp, "Spell 1 '%s'\n", skill_table[obj->value[1]].name);
        }

        if (obj->value[2] > 0) {
            fprintf(fp, "Spell 2 '%s'\n", skill_table[obj->value[2]].name);
        }

        if (obj->value[3] > 0) {
            fprintf(fp, "Spell 3 '%s'\n", skill_table[obj->value[3]].name);
        }
        break;

    case ITEM_STAFF:
    case ITEM_WAND:
        if (obj->value[3] > 0) {
            fprintf(fp, "Spell 3 '%s'\n", skill_table[obj->value[3]].name);
        }
        break;

    default:
        break;
    }

    FOR_EACH(affect, obj->affected) {
        if (affect->type < 0 || affect->type >= skill_count) 
            continue;
        fprintf(fp, "Affc '%s' %3d %3d %3d %3d %3d %10d\n",
                skill_table[affect->type].name, affect->where, affect->level,
                affect->duration, affect->modifier, affect->location, affect->bitvector);
    }

    FOR_EACH(ed, obj->extra_desc) {
        fprintf(fp, "ExDe %s~ %s~\n", ed->keyword, ed->description);
    }

    fprintf(fp, "End\n\n");

    if (obj->contains != NULL) 
        fwrite_obj(ch, obj->contains, fp, iNest + 1);

    return;
}


void fwrite_quests(Mobile* ch, FILE* fp)
{
    fprintf(fp, "#QUESTLOG\n");

    QuestStatus* qs;

    FOR_EACH(qs, ch->pcdata->quest_log->quests) {
        fprintf(fp, "%d %d %d  ", qs->vnum, qs->progress, qs->state);
    }

    fprintf(fp, "\nEnd\n\n");
}

// Load a char and inventory into a new ch structure.
bool load_char_obj(Descriptor* d, char* name)
{
    char strsave[MAX_INPUT_LENGTH] = { 0 };
    Mobile* ch;
    FILE* fp;
    int stat;

    ch = new_mobile();
    ch->pcdata = new_player_data();
    ch->pcdata->ch = ch;
    d->character = ch;
    ch->desc = d;
    NAME_FIELD(ch) = lox_string(name);
    ch->id = get_pc_id();
    ch->race = race_lookup("human");
    ch->act_flags = PLR_NOSUMMON;
    ch->comm_flags = COMM_COMBINE | COMM_PROMPT;
    ch->prompt = str_dup("<%hhp %mm %vmv> ");
    ch->pcdata->confirm_delete = false;
    ch->pcdata->pwd_digest = NULL;
    ch->pcdata->pwd_digest_len = 0;
    ch->pcdata->bamfin = str_dup("");
    ch->pcdata->bamfout = str_dup("");
    ch->pcdata->title = str_dup("");
    for (stat = 0; stat < STAT_COUNT; stat++)
        ch->perm_stat[stat] = 13;
    ch->pcdata->condition[COND_THIRST] = 48;
    ch->pcdata->condition[COND_FULL] = 48;
    ch->pcdata->condition[COND_HUNGER] = 48;
    ch->pcdata->security = 0;   // OLC

    // Set the default color theme.
    SET_BIT(ch->act_flags, PLR_COLOUR);
    set_default_colors(ch);
    for (int i = 0; i < MAX_THEMES; ++i) {
        ch->pcdata->color_themes[i] = NULL;
    }

#ifndef _MSC_VER
    char buf[MAX_INPUT_LENGTH + 50] = { 0 };    // To handle strsave + format
    /* decompress if .gz file exists */
    sprintf(strsave, "%s%s%s", cfg_get_player_dir(), capitalize(name), ".gz");
    if (file_exists(strsave)) {
        sprintf(buf, "gzip -dfq %s", strsave);
        if (!system(buf)) {
            sprintf(buf, "ERROR: Failed to zip %s (Error Code: %d).", strsave, errno);
            log_string(buf);
        }
    }
#endif

    sprintf(strsave, "%s%s", cfg_get_player_dir(), capitalize(name));

    if (!file_exists(strsave))
        return false;

    OPEN_OR_RETURN_FALSE(fp = open_read_file(strsave));

    int iNest;

    for (iNest = 0; iNest < MAX_NEST; iNest++)
        rgObjNest[iNest] = NULL;

    for (;;) {
        char letter;
        char* word;

        letter = fread_letter(fp);
        if (letter == '*') {
            fread_to_eol(fp);
            continue;
        }

        if (letter != '#') {
            bug("Load_char_obj: # not found.", 0);
            break;
        }

        word = fread_word(fp);
        if (!str_cmp(word, "PLAYER"))
            fread_char(ch, fp);
        else if (!str_cmp(word, "OBJECT"))
            fread_obj(ch, fp);
        else if (!str_cmp(word, "O"))
            fread_obj(ch, fp);
        else if (!str_cmp(word, "PET"))
            fread_pet(ch, fp);
        else if (!str_cmp(word, "THEME"))
            fread_theme(ch, fp);
        else if (!str_cmp(word, "QUESTLOG"))
            fread_quests(ch, fp);
        else if (!str_cmp(word, "END"))
            break;
        else {
            bug("Load_char_obj: bad section.", 0);
            break;
        }
    }
    close_file(fp);

    /* initialize race */
    int i;

    if (ch->race == 0) 
        ch->race = race_lookup("human");

    ch->size = race_table[ch->race].size;
    ch->dam_type = DAM_BASH;

    for (i = 0; i < RACE_NUM_SKILLS; i++) {
        if (race_table[ch->race].skills[i] == NULL)
            break;
        group_add(ch, race_table[ch->race].skills[i], false);
    }
    ch->affect_flags = ch->affect_flags | race_table[ch->race].aff;
    ch->imm_flags = ch->imm_flags | race_table[ch->race].imm;
    ch->res_flags = ch->res_flags | race_table[ch->race].res;
    ch->vuln_flags = ch->vuln_flags | race_table[ch->race].vuln;
    ch->form = race_table[ch->race].form;
    ch->parts = race_table[ch->race].parts;


    if (ch->version < 2) /* need to add the new skills */
    {
        group_add(ch, "rom basics", false);
        group_add(ch, class_table[ch->ch_class].base_group, false);
        group_add(ch, class_table[ch->ch_class].default_group, true);
        ch->pcdata->learned[gsn_recall] = 50;
    }

    /* fix levels */
    if (ch->version < 3 && (ch->level > 35 || ch->trust > 35)) {
        switch (ch->level) {
        case (40):
            ch->level = 60;
            break; /* imp -> imp */
        case (39):
            ch->level = 58;
            break; /* god -> supreme */
        case (38):
            ch->level = 56;
            break; /* deity -> god */
        case (37):
            ch->level = 53;
            break; /* angel -> demigod */
        }

        switch (ch->trust) {
        case (40):
            ch->trust = 60;
            break; /* imp -> imp */
        case (39):
            ch->trust = 58;
            break; /* god -> supreme */
        case (38):
            ch->trust = 56;
            break; /* deity -> god */
        case (37):
            ch->trust = 53;
            break; /* angel -> demigod */
        case (36):
            ch->trust = 51;
            break; /* hero -> hero */
        }
    }

    char* theme_name;
    if ((theme_name = ch->pcdata->theme_config.current_theme_name)) {
        ColorTheme* theme = lookup_color_theme(ch, theme_name);
        if (!theme) {
            free_string(theme_name);
            theme = (ColorTheme*)system_color_themes[SYSTEM_COLOR_THEME_LOPE];
            ch->pcdata->theme_config.current_theme_name = str_dup(theme->name);
        }
        if (ch->pcdata->current_theme)
            free_color_theme(ch->pcdata->current_theme);
        ch->pcdata->current_theme = dup_color_theme(theme);
        set_default_colors(ch);
    }

    /* ream gold */
    if (ch->version < 4) { 
        ch->gold /= 100; 
    }

    return true;
}

// Read in a char.

#if defined(KEY)
#undef KEY
#endif

#define KEY(literal, field, value)                                             \
    if (!str_cmp(word, literal)) {                                             \
        field = value;                                                         \
        fMatch = true;                                                         \
        break;                                                                 \
    }

/* provided to free strings */
#if defined(KEYS)
#undef KEYS
#endif

#define KEYS(literal, field, value)                                            \
    if (!str_cmp(word, literal)) {                                             \
        free_string(field);                                                    \
        field = value;                                                         \
        fMatch = true;                                                         \
        break;                                                                 \
    }

#define KEYLS(literal, entity, field, value)                                   \
    if (!str_cmp(word, literal)) {                                             \
        (entity)->header.field = (value);                                      \
        SET_LOX_FIELD(&((entity)->header), (entity)->header.field, field);     \
        fMatch = true;                                                         \
        break;                                                                 \
    }

void fread_char(Mobile* ch, FILE* fp)
{
    char buf[MAX_STRING_LENGTH];
    char* word;
    bool fMatch;
    int count = 0;
    time_t lastlogoff = current_time;
    int16_t percent;

    sprintf(buf, "Loading %s.", NAME_STR(ch));
    log_string(buf);

    for (;;) {
        word = feof(fp) ? "End" : fread_word(fp);
        fMatch = false;

        switch (UPPER(word[0])) {
        case '*':
            fMatch = true;
            fread_to_eol(fp);
            break;

        case 'A':
            KEY("Act", ch->act_flags, fread_flag(fp));
            KEY("AffectedBy", ch->affect_flags, fread_flag(fp));
            KEY("AfBy", ch->affect_flags, fread_flag(fp));
            KEY("Alignment", ch->alignment, (int16_t)fread_number(fp));
            KEY("Alig", ch->alignment, (int16_t)fread_number(fp));

            if (!str_cmp(word, "Alia")) {
                if (count >= MAX_ALIAS) {
                    fread_to_eol(fp);
                    fMatch = true;
                    break;
                }

                ch->pcdata->alias[count] = str_dup(fread_word(fp));
                ch->pcdata->alias_sub[count] = str_dup(fread_word(fp));
                count++;
                fMatch = true;
                break;
            }

            if (!str_cmp(word, "Alias")) {
                if (count >= MAX_ALIAS) {
                    fread_to_eol(fp);
                    fMatch = true;
                    break;
                }

                ch->pcdata->alias[count] = str_dup(fread_word(fp));
                ch->pcdata->alias_sub[count] = fread_string(fp);
                count++;
                fMatch = true;
                break;
            }

            if (!str_cmp(word, "AC") || !str_cmp(word, "Armor")) {
                fread_to_eol(fp);
                fMatch = true;
                break;
            }

            if (!str_cmp(word, "ACs")) {
                int i;

                for (i = 0; i < 4; i++) 
                    ch->armor[i] = (int16_t)fread_number(fp);
                fMatch = true;
                break;
            }

            if (!str_cmp(word, "AffD")) {
                Affect* affect;
                SKNUM sn;

                affect = new_affect();

                sn = skill_lookup(fread_word(fp));
                if (sn < 0)
                    bug("Fread_char: unknown skill.", 0);
                else
                    affect->type = sn;

                affect->level = (LEVEL)fread_number(fp);
                affect->duration = (int16_t)fread_number(fp);
                affect->modifier = (int16_t)fread_number(fp);
                affect->location = (int16_t)fread_number(fp);
                affect->bitvector = fread_number(fp);
                affect->next = ch->affected;
                ch->affected = affect;
                fMatch = true;
                break;
            }

            if (!str_cmp(word, "Affc")) {
                Affect* affect;
                SKNUM sn;

                affect = new_affect();

                sn = skill_lookup(fread_word(fp));
                if (sn < 0)
                    bug("Fread_char: unknown skill.", 0);
                else
                    affect->type = sn;

                affect->where = (int16_t)fread_number(fp);
                affect->level = (LEVEL)fread_number(fp);
                affect->duration = (int16_t)fread_number(fp);
                affect->modifier = (int16_t)fread_number(fp);
                affect->location = (int16_t)fread_number(fp);
                affect->bitvector = fread_number(fp);
                affect->next = ch->affected;
                ch->affected = affect;
                fMatch = true;
                break;
            }

            if (!str_cmp(word, "AttrMod") || !str_cmp(word, "AMod")) {
                int stat;
                for (stat = 0; stat < STAT_COUNT; stat++)
                    ch->mod_stat[stat] = (int16_t)fread_number(fp);
                fMatch = true;
                break;
            }

            if (!str_cmp(word, "AttrPerm") || !str_cmp(word, "Attr")) {
                int stat;

                for (stat = 0; stat < STAT_COUNT; stat++)
                    ch->perm_stat[stat] = (int16_t)fread_number(fp);
                fMatch = true;
                break;
            }
            break;

        case 'B':
            KEY("Bamfin", ch->pcdata->bamfin, fread_string(fp));
            KEY("Bamfout", ch->pcdata->bamfout, fread_string(fp));
            KEY("Bin", ch->pcdata->bamfin, fread_string(fp));
            KEY("Bout", ch->pcdata->bamfout, fread_string(fp));
            break;

        case 'C':
            KEY("Class", ch->ch_class, (int16_t)fread_number(fp));
            KEY("Cla", ch->ch_class, (int16_t)fread_number(fp));
            KEY("Clan", ch->clan, (int16_t)clan_lookup(fread_string(fp)));
            KEY("Comm", ch->comm_flags, fread_flag(fp));

            if (!str_cmp(word, "Condition") || !str_cmp(word, "Cond")) {
                ch->pcdata->condition[0] = (int16_t)fread_number(fp);
                ch->pcdata->condition[1] = (int16_t)fread_number(fp);
                ch->pcdata->condition[2] = (int16_t)fread_number(fp);
                fMatch = true;
                break;
            }
            if (!str_cmp(word, "Cnd")) {
                ch->pcdata->condition[0] = (int16_t)fread_number(fp);
                ch->pcdata->condition[1] = (int16_t)fread_number(fp);
                ch->pcdata->condition[2] = (int16_t)fread_number(fp);
                ch->pcdata->condition[3] = (int16_t)fread_number(fp);
                fMatch = true;
                break;
            }
            break;

        case 'D':
            KEY("Damroll", ch->damroll, (int16_t)fread_number(fp));
            KEY("Dam", ch->damroll, (int16_t)fread_number(fp));
            KEY("Description", ch->description, fread_string(fp));
            KEY("Desc", ch->description, fread_string(fp));
            break;

        case 'E':
            if (!str_cmp(word, "End")) {
                /* adjust hp mana move up  -- here for speed's sake */
                percent = (int16_t)((int64_t)(current_time - lastlogoff) * 25 / ((int64_t)2 * 60 * 60));

                percent = UMIN(percent, 100);

                if (percent > 0 && !IS_AFFECTED(ch, AFF_POISON)
                    && !IS_AFFECTED(ch, AFF_PLAGUE)) {
                    ch->hit += (ch->max_hit - ch->hit) * percent / 100;
                    ch->mana += (ch->max_mana - ch->mana) * percent / 100;
                    ch->move += (ch->max_move - ch->move) * percent / 100;
                }

                if (ch->in_room == NULL) {
                    ch->in_room = get_room(NULL, ch->pcdata->recall);
                }
                return;
            }
            KEY("Exp", ch->exp, fread_number(fp));
            break;

        case 'G':
            KEY("Gold", ch->gold, (int16_t)fread_number(fp));
            if (!str_cmp(word, "Group") || !str_cmp(word, "Gr")) {
                SKNUM gn;
                char* temp;

                temp = fread_word(fp);
                gn = group_lookup(temp);
                /* gn    = group_lookup( fread_word( fp ) ); */
                if (gn < 0) {
                    fprintf(stderr, "%s", temp);
                    bug("Fread_char: unknown group. ", 0);
                }
                else
                    gn_add(ch, gn);
                fMatch = true;
            }
            break;

        case 'H':
            KEY("Hitroll", ch->hitroll, (int16_t)fread_number(fp));
            KEY("Hit", ch->hitroll, (int16_t)fread_number(fp));

            if (!str_cmp(word, "HpManaMove") || !str_cmp(word, "HMV")) {
                ch->hit = (int16_t)fread_number(fp);
                ch->max_hit = (int16_t)fread_number(fp);
                ch->mana = (int16_t)fread_number(fp);
                ch->max_mana = (int16_t)fread_number(fp);
                ch->move = (int16_t)fread_number(fp);
                ch->max_move = (int16_t)fread_number(fp);
                fMatch = true;
                break;
            }

            if (!str_cmp(word, "HpManaMovePerm") || !str_cmp(word, "HMVP")) {
                ch->pcdata->perm_hit = (int16_t)fread_number(fp);
                ch->pcdata->perm_mana = (int16_t)fread_number(fp);
                ch->pcdata->perm_move = (int16_t)fread_number(fp);
                fMatch = true;
                break;
            }

            break;

        case 'I':
            KEY("Id", ch->id, fread_number(fp));
            KEY("InvisLevel", ch->invis_level, (int16_t)fread_number(fp));
            KEY("Inco", ch->incog_level, (int16_t)fread_number(fp));
            KEY("Invi", ch->invis_level, (int16_t)fread_number(fp));
            break;

        case 'L':
            KEY("LastLevel", ch->pcdata->last_level, (int16_t)fread_number(fp));
            KEY("LLev", ch->pcdata->last_level, (int16_t)fread_number(fp));
            KEY("Level", ch->level, (int16_t)fread_number(fp));
            KEY("Lev", ch->level, (int16_t)fread_number(fp));
            KEY("Levl", ch->level, (int16_t)fread_number(fp));
            KEY("LogO", lastlogoff, fread_number(fp));
            KEY("LongDescr", ch->long_descr, fread_string(fp));
            KEY("LnD", ch->long_descr, fread_string(fp));
            break;

        case 'N':
            KEYLS("Name", ch, name, fread_lox_string(fp));
            KEY("Note", ch->pcdata->last_note, fread_number(fp));
            if (!str_cmp(word, "Not")) {
                ch->pcdata->last_note = fread_number(fp);
                ch->pcdata->last_idea = fread_number(fp);
                ch->pcdata->last_penalty = fread_number(fp);
                ch->pcdata->last_news = fread_number(fp);
                ch->pcdata->last_changes = fread_number(fp);
                fMatch = true;
                break;
            }
            break;

        case 'P':
#ifdef _MSC_VER
            // Legacy password support for platforms without crypt(). 
            // Read it, hash it, and throw it away.
            // Because Linux has crypt() available, this won't work for that
            // platform. 
            if (!str_cmp(word, "Password") || !str_cmp(word, "Pass")) {
                char* pwd = fread_string(fp);
                set_password(pwd, ch);
                free_string(pwd);
                fMatch = true;
                break;
            }
#endif

            if (!str_cmp(word, "PwdDigest")) {
                char* dig_text = fread_string(fp);
                decode_digest(ch->pcdata, dig_text);
                free_string(dig_text);
                fMatch = true;
                break;
            }
            KEY("Played", ch->played, fread_number(fp));
            KEY("Plyd", ch->played, fread_number(fp));
            KEY("Points", ch->pcdata->points, (int16_t)fread_number(fp));
            KEY("Pnts", ch->pcdata->points, (int16_t)fread_number(fp));
            KEY("Position", ch->position, (int16_t)fread_number(fp));
            KEY("Pos", ch->position, (int16_t)fread_number(fp));
            KEY("Practice", ch->practice, (int16_t)fread_number(fp));
            KEY("Prac", ch->practice, (int16_t)fread_number(fp));
            KEYS("Prompt", ch->prompt, fread_string(fp));
            KEY("Prom", ch->prompt, fread_string(fp));
            break;

        case 'R':
            KEY("Race", ch->race, race_lookup(fread_string(fp)));

            if (!str_cmp(word, "Room")) {
                RoomData* room_data = get_room_data(fread_number(fp));
                // Don't let them log back in to a deleted instance unless it's 
                // a newbie zone.
                if (room_data) {
                    Area* area = get_area_for_player(ch, room_data->area_data);
                    if (area || room_data->area_data->low_range == 1) {
                        ch->in_room = get_room_for_player(ch, room_data->vnum);
                    }
                }
                fMatch = true;
                break;
            }

            if (!str_cmp(word, "Recall")) {
                ch->pcdata->recall = fread_number(fp);
                fMatch = true;
                break;
            }

            break;

        case 'S':
            KEY("SavingThrow", ch->saving_throw, (int16_t)fread_number(fp));
            KEY("Save", ch->saving_throw, (int16_t)fread_number(fp));
            KEY("Scro", ch->lines, fread_number(fp));
            KEY("Sex", ch->sex, (int16_t)fread_number(fp));
            KEY("ShortDescr", ch->short_descr, fread_string(fp));
            KEY("ShD", ch->short_descr, fread_string(fp));
            KEY("Sec", ch->pcdata->security, fread_number(fp));	// OLC
            KEY("Silv", ch->silver, (int16_t)fread_number(fp));

            if (!str_cmp(word, "Skill") || !str_cmp(word, "Sk")) {
                SKNUM sn;
                int16_t value;
                char* skill;

                value = (int16_t)fread_number(fp);
                skill = fread_word(fp);
                sn = skill_lookup(skill);
                if (sn < 0) {
                    bug("fread_char: unknown skill '%s'. ", skill);
                }
                else
                    ch->pcdata->learned[sn] = value;
                fMatch = true;
            }

            break;

        case 'T':
            KEY("Theme", ch->pcdata->theme_config.current_theme_name, fread_string(fp));

            if (!str_cmp(word, "ThemeConfig")) {
                ch->pcdata->theme_config.hide_24bit = fread_number(fp);
                ch->pcdata->theme_config.hide_256 = fread_number(fp);
                ch->pcdata->theme_config.xterm = fread_number(fp);
                ch->pcdata->theme_config.hide_rgb_help = fread_number(fp);
                fread_number(fp); // reserved
                fMatch = true;
                break;
            }

            KEY("trueSex", ch->pcdata->true_sex, (int16_t)fread_number(fp));
            KEY("TSex", ch->pcdata->true_sex, (int16_t)fread_number(fp));
            KEY("Trai", ch->train, (int16_t)fread_number(fp));
            KEY("Trust", ch->trust, (int16_t)fread_number(fp));
            KEY("Tru", ch->trust, (int16_t)fread_number(fp));

            if (!str_cmp(word, "Title") || !str_cmp(word, "Titl")) {
                ch->pcdata->title = fread_string(fp);
                if (ch->pcdata->title[0] != '.' && ch->pcdata->title[0] != ','
                    && ch->pcdata->title[0] != '!'
                    && ch->pcdata->title[0] != '?') {
                    sprintf(buf, " %s", ch->pcdata->title);
                    free_string(ch->pcdata->title);
                    ch->pcdata->title = str_dup(buf);
                }
                fMatch = true;
                break;
            }
            break;

        case 'V':
            KEY("Version", ch->version, fread_number(fp));
            KEY("Vers", ch->version, fread_number(fp));
            if (!str_cmp(word, "Vnum")) {
                ch->prototype = get_mob_prototype(fread_vnum(fp));
                fMatch = true;
                break;
            }
            break;

        case 'W':
            KEY("Wimpy", ch->wimpy, (int16_t)fread_number(fp));
            KEY("Wimp", ch->wimpy, (int16_t)fread_number(fp));
            KEY("Wizn", ch->wiznet, fread_flag(fp));
            break;
        }

        if (!fMatch) {
            bug("Fread_char: no match.", 0);
            bug(word, 0);
            fread_to_eol(fp);
        }
    }
}

/* load a pet from the forgotten reaches */
void fread_pet(Mobile* ch, FILE* fp)
{
    char* word;
    Mobile* pet;
    bool fMatch;
    time_t lastlogoff = current_time;
    int16_t percent;

    /* first entry had BETTER be the vnum or we barf */
    word = feof(fp) ? "END" : fread_word(fp);
    if (!str_cmp(word, "Vnum")) {
        VNUM vnum;

        vnum = fread_vnum(fp);
        if (get_mob_prototype(vnum) == NULL) {
            bug("Fread_pet: bad vnum %"PRVNUM".", vnum);
            pet = create_mobile(get_mob_prototype(MOB_VNUM_FIDO));
        }
        else
            pet = create_mobile(get_mob_prototype(vnum));
    }
    else {
        bug("Fread_pet: no vnum in file.", 0);
        pet = create_mobile(get_mob_prototype(MOB_VNUM_FIDO));
    }

    for (;;) {
        word = feof(fp) ? "END" : fread_word(fp);
        fMatch = false;

        switch (UPPER(word[0])) {
        case '*':
            fMatch = true;
            fread_to_eol(fp);
            break;

        case 'A':
            KEY("Act", pet->act_flags, fread_flag(fp));
            KEY("AfBy", pet->affect_flags, fread_flag(fp));
            KEY("Alig", pet->alignment, (int16_t)fread_number(fp));

            if (!str_cmp(word, "ACs")) {
                int i;

                for (i = 0; i < 4; i++) pet->armor[i] = (int16_t)fread_number(fp);
                fMatch = true;
                break;
            }

            if (!str_cmp(word, "AffD")) {
                Affect* affect;
                SKNUM sn;

                affect = new_affect();

                sn = skill_lookup(fread_word(fp));
                if (sn < 0)
                    bug("Fread_char: unknown skill.", 0);
                else
                    affect->type = sn;

                affect->level = (LEVEL)fread_number(fp);
                affect->duration = (int16_t)fread_number(fp);
                affect->modifier = (int16_t)fread_number(fp);
                affect->location = (int16_t)fread_number(fp);
                affect->bitvector = fread_number(fp);
                affect->next = pet->affected;
                pet->affected = affect;
                fMatch = true;
                break;
            }

            if (!str_cmp(word, "Affc")) {
                Affect* affect;
                SKNUM sn;

                affect = new_affect();

                sn = skill_lookup(fread_word(fp));
                if (sn < 0)
                    bug("Fread_char: unknown skill.", 0);
                else
                    affect->type = sn;

                affect->where = (int16_t)fread_number(fp);
                affect->level = (LEVEL)fread_number(fp);
                affect->duration = (int16_t)fread_number(fp);
                affect->modifier = (int16_t)fread_number(fp);
                affect->location = (int16_t)fread_number(fp);
                affect->bitvector = fread_number(fp);
                affect->next = pet->affected;
                pet->affected = affect;
                fMatch = true;
                break;
            }

            if (!str_cmp(word, "AMod")) {
                int stat;

                for (stat = 0; stat < STAT_COUNT; stat++)
                    pet->mod_stat[stat] = (int16_t)fread_number(fp);
                fMatch = true;
                break;
            }

            if (!str_cmp(word, "Attr")) {
                int stat;

                for (stat = 0; stat < STAT_COUNT; stat++)
                    pet->perm_stat[stat] = (int16_t)fread_number(fp);
                fMatch = true;
                break;
            }
            break;

        case 'C':
            KEY("Clan", pet->clan, (int16_t)clan_lookup(fread_string(fp)));
            KEY("Comm", pet->comm_flags, fread_flag(fp));
            break;

        case 'D':
            KEY("Dam", pet->damroll, (int16_t)fread_number(fp));
            KEY("Desc", pet->description, fread_string(fp));
            break;

        case 'E':
            if (!str_cmp(word, "End")) {
                pet->leader = ch;
                pet->master = ch;
                ch->pet = pet;
                /* adjust hp mana move up  -- here for speed's sake */
                percent = (int16_t)((int64_t)(current_time - lastlogoff) * 25 / ((int64_t)2 * 60 * 60));

                if (percent > 0 && !IS_AFFECTED(ch, AFF_POISON)
                    && !IS_AFFECTED(ch, AFF_PLAGUE)) {
                    percent = UMIN(percent, 100);
                    pet->hit += (pet->max_hit - pet->hit) * percent / 100;
                    pet->mana += (pet->max_mana - pet->mana) * percent / 100;
                    pet->move += (pet->max_move - pet->move) * percent / 100;
                }
                return;
            }
            KEY("Exp", pet->exp, fread_number(fp));
            break;

        case 'G':
            KEY("Gold", pet->gold, (int16_t)fread_number(fp));
            break;

        case 'H':
            KEY("Hit", pet->hitroll, (int16_t)fread_number(fp));

            if (!str_cmp(word, "HMV")) {
                pet->hit = (int16_t)fread_number(fp);
                pet->max_hit = (int16_t)fread_number(fp);
                pet->mana = (int16_t)fread_number(fp);
                pet->max_mana = (int16_t)fread_number(fp);
                pet->move = (int16_t)fread_number(fp);
                pet->max_move = (int16_t)fread_number(fp);
                fMatch = true;
                break;
            }
            break;

        case 'L':
            KEY("Levl", pet->level, (int16_t)fread_number(fp));
            KEY("LnD", pet->long_descr, fread_string(fp));
            KEY("LogO", lastlogoff, fread_number(fp));
            break;

        case 'N':
            KEYLS("Name", pet, name, fread_lox_string(fp));
            break;

        case 'P':
            KEY("Pos", pet->position, (int16_t)fread_number(fp));
            break;

        case 'R':
            KEY("Race", pet->race, (int16_t)race_lookup(fread_string(fp)));
            break;

        case 'S':
            KEY("Save", pet->saving_throw, (int16_t)fread_number(fp));
            KEY("Sex", pet->sex, (int16_t)fread_number(fp));
            KEY("ShD", pet->short_descr, fread_string(fp));
            KEY("Silv", pet->silver, (int16_t)fread_number(fp));
            break;
        }

        if (!fMatch) {
            bug("Fread_pet: no match.", 0);
            fread_to_eol(fp);
        }
    }
}

void fread_obj(Mobile* ch, FILE* fp)
{
    Object* obj;
    char* word;
    int iNest;
    bool fMatch;
    bool fNest;
    bool fVnum;
    bool first;

    fVnum = false;
    obj = NULL;
    first = true; /* used to counter fp offset */

    word = feof(fp) ? "End" : fread_word(fp);
    if (!str_cmp(word, "Vnum")) {
        VNUM vnum;
        first = false; /* fp will be in right place */

        vnum = fread_vnum(fp);
        if (get_object_prototype(vnum) == NULL) {
            bug("Fread_obj: bad vnum %"PRVNUM".", vnum);
        }
        else {
            obj = create_object(get_object_prototype(vnum), -1);
        }
    }

    if (obj == NULL) /* either not found or old style */
    {
        obj = new_object();
        obj->short_descr = str_dup("");
        obj->description = str_dup("");
    }

    fNest = false;
    fVnum = true;
    iNest = 0;

    for (;;) {
        if (first)
            first = false;
        else
            word = feof(fp) ? "End" : fread_word(fp);
        fMatch = false;

        switch (UPPER(word[0])) {
        case '*':
            fMatch = true;
            fread_to_eol(fp);
            break;

        case 'A':
            if (!str_cmp(word, "AffD")) {
                Affect* affect;
                SKNUM sn;

                affect = new_affect();

                sn = skill_lookup(fread_word(fp));
                if (sn < 0)
                    bug("Fread_obj: unknown skill.", 0);
                else
                    affect->type = sn;

                affect->level = (LEVEL)fread_number(fp);
                affect->duration = (int16_t)fread_number(fp);
                affect->modifier = (int16_t)fread_number(fp);
                affect->location = (int16_t)fread_number(fp);
                affect->bitvector = fread_number(fp);
                affect->next = obj->affected;
                obj->affected = affect;
                fMatch = true;
                break;
            }
            if (!str_cmp(word, "Affc")) {
                Affect* affect;
                SKNUM sn;

                affect = new_affect();

                sn = skill_lookup(fread_word(fp));
                if (sn < 0)
                    bug("Fread_obj: unknown skill.", 0);
                else
                    affect->type = sn;

                affect->where = (int16_t)fread_number(fp);
                affect->level = (LEVEL)fread_number(fp);
                affect->duration = (int16_t)fread_number(fp);
                affect->modifier = (int16_t)fread_number(fp);
                affect->location = (int16_t)fread_number(fp);
                affect->bitvector = fread_number(fp);
                affect->next = obj->affected;
                obj->affected = affect;
                fMatch = true;
                break;
            }
            break;

        case 'C':
            KEY("Cond", obj->condition, (int16_t)fread_number(fp));
            KEY("Cost", obj->cost, fread_number(fp));
            break;

        case 'D':
            KEY("Description", obj->description, fread_string(fp));
            KEY("Desc", obj->description, fread_string(fp));
            break;

        case 'E':

            if (!str_cmp(word, "Enchanted")) {
                obj->enchanted = true;
                fMatch = true;
                break;
            }

            KEY("ExtraFlags", obj->extra_flags, fread_number(fp));
            KEY("ExtF", obj->extra_flags, fread_number(fp));

            if (!str_cmp(word, "ExtraDescr") || !str_cmp(word, "ExDe")) {
                ExtraDesc* ed;

                ed = new_extra_desc();

                ed->keyword = fread_string(fp);
                ed->description = fread_string(fp);
                ed->next = obj->extra_desc;
                obj->extra_desc = ed;
                fMatch = true;
            }

            if (!str_cmp(word, "End")) {
                if (!fNest || (fVnum && obj->prototype == NULL)) {
                    bug("Fread_obj: incomplete object.", 0);
                    free_object(obj);
                    return;
                }
                else {
                    if (!fVnum) {
                        free_object(obj);
                        obj = create_object(get_object_prototype(OBJ_VNUM_DUMMY), 0);
                    }

                    if (iNest == 0 || rgObjNest[iNest] == NULL)
                        obj_to_char(obj, ch);
                    else
                        obj_to_obj(obj, rgObjNest[iNest - 1]);
                    return;
                }
            }
            break;

        case 'I':
            KEY("ItemType", obj->item_type, (int16_t)fread_number(fp));
            KEY("Ityp", obj->item_type, (int16_t)fread_number(fp));
            break;

        case 'L':
            KEY("Level", obj->level, (LEVEL)fread_number(fp));
            KEY("Lev", obj->level, (LEVEL)fread_number(fp));
            break;

        case 'N':
            KEYLS("Name", obj, name, fread_lox_string(fp));

            if (!str_cmp(word, "Nest")) {
                iNest = fread_number(fp);
                if (iNest < 0 || iNest >= MAX_NEST) {
                    bug("Fread_obj: bad nest %d.", iNest);
                }
                else {
                    rgObjNest[iNest] = obj;
                    fNest = true;
                }
                fMatch = true;
            }
            break;

        case 'O':
            break;

        case 'S':
            KEY("ShortDescr", obj->short_descr, fread_string(fp));
            KEY("ShD", obj->short_descr, fread_string(fp));

            if (!str_cmp(word, "Spell")) {
                int iValue;
                SKNUM sn;

                iValue = fread_number(fp);
                sn = skill_lookup(fread_word(fp));
                if (iValue < 0 || iValue > 3) {
                    bug("Fread_obj: bad iValue %d.", iValue);
                }
                else if (sn < 0) {
                    bug("Fread_obj: unknown skill.", 0);
                }
                else {
                    obj->value[iValue] = sn;
                }
                fMatch = true;
                break;
            }

            break;

        case 'T':
            KEY("Timer", obj->timer, (int16_t)fread_number(fp));
            KEY("Time", obj->timer, (int16_t)fread_number(fp));
            break;

        case 'V':
            if (!str_cmp(word, "Values") || !str_cmp(word, "Vals")) {
                obj->value[0] = fread_number(fp);
                obj->value[1] = fread_number(fp);
                obj->value[2] = fread_number(fp);
                obj->value[3] = fread_number(fp);
                if (obj->item_type == ITEM_WEAPON && obj->value[0] == 0)
                    obj->value[0] = obj->prototype->value[0];
                fMatch = true;
                break;
            }

            if (!str_cmp(word, "Val")) {
                obj->value[0] = fread_number(fp);
                obj->value[1] = fread_number(fp);
                obj->value[2] = fread_number(fp);
                obj->value[3] = fread_number(fp);
                obj->value[4] = fread_number(fp);
                fMatch = true;
                break;
            }

            if (!str_cmp(word, "Vnum")) {
                VNUM vnum;

                vnum = fread_vnum(fp);
                if ((obj->prototype = get_object_prototype(vnum)) == NULL)
                    bug("Fread_obj: bad vnum %"PRVNUM".", vnum);
                else
                    fVnum = true;
                fMatch = true;
                break;
            }
            break;

        case 'W':
            KEY("WearFlags", obj->wear_flags, fread_number(fp));
            KEY("WeaF", obj->wear_flags, fread_number(fp));
            KEY("WearLoc", obj->wear_loc, (int16_t)fread_number(fp));
            KEY("Wear", obj->wear_loc, (int16_t)fread_number(fp));
            KEY("Weight", obj->weight, (int16_t)fread_number(fp));
            KEY("Wt", obj->weight, (int16_t)fread_number(fp));
            break;
        }

        if (!fMatch) {
            bug("Fread_obj: no match.", 0);
            fread_to_eol(fp);
        }
    }
}

void fread_theme(Mobile* ch, FILE* fp)
{
    ColorTheme theme = { 0 };
    bool fMatch;
    char* word;

    for (;;) {
        word = feof(fp) ? "End" : fread_word(fp);
        fMatch = false;
        switch (UPPER(word[0])) {
        case 'B':
            KEY("Banner", theme.banner, fread_string(fp));
            break;
        case 'C':
            if (!str_cmp(word, "Channel")) {
                char* chan = fread_word(fp);
                int mode = fread_number(fp);
                uint8_t code_0 = (uint8_t)fread_number(fp);
                uint8_t code_1 = (uint8_t)fread_number(fp);
                uint8_t code_2 = (uint8_t)fread_number(fp);
                fread_number(fp); // reserved
                int slot = -1;
                LOOKUP_COLOR_SLOT_NAME(slot, chan);
                if (slot < 0 || slot > SLOT_MAX) {
                    bugf("fread_theme(%s): bad channel name '%s'.", 
                        NAME_STR(ch), chan);
                    break;
                }
                theme.channels[slot] = (Color){ 
                    .mode = mode, 
                    .code = { code_0, code_1, code_2 }, 
                    .cache = NULL, 
                    .xterm = NULL 
                };
                fMatch = true;
                break;
            }
            break;
        case 'E':
            if (!str_cmp(word, "End")) {
                for (int i = 0; i < MAX_THEMES; ++i) {
                    if (!ch->pcdata->color_themes[i]) {
                        ch->pcdata->color_themes[i] = new_color_theme();
                        *ch->pcdata->color_themes[i] = theme;
                        return;
                    }
                }
                bugf("Could not find a free color theme slot for %s.",
                    NAME_STR(ch));
                return;
            }
            break;
        case 'I':
            if (!str_cmp(word, "Info")) {
                theme.type = fread_number(fp);
                theme.mode = fread_number(fp);
                theme.palette_max = fread_number(fp);
                theme.is_public = fread_number(fp);
                fMatch = true;
                break;
            }
            break;
        case 'P':
            if (!str_cmp(word, "Palette")) {
                int idx = fread_number(fp);
                int mode = fread_number(fp);
                uint8_t code_0 = (uint8_t)fread_number(fp);
                uint8_t code_1 = (uint8_t)fread_number(fp);
                uint8_t code_2 = (uint8_t)fread_number(fp);
                fread_number(fp); // reserved
                theme.palette[idx] = (Color){
                    .mode = mode,
                    .code = { code_0, code_1, code_2 },
                    .cache = NULL, 
                    .xterm = NULL
                };
                fMatch = true;
                break;
            }
            break;
        case 'N':
            KEY("Name", theme.name, fread_string(fp));
            break;
        default:
            break;
        }

        if (!fMatch) {
            bugf("Fread_theme: no match for '%'.", word);
            fread_to_eol(fp);
        }
    }

    
}

void fread_quests(Mobile* ch, FILE* fp)
{
    char* word = feof(fp) ? "End" : fread_word(fp);
    while (str_cmp(word, "End")) {
        VNUM vnum = STRTOVNUM(word);
        int progress = fread_number(fp);
        QuestState state = fread_number(fp);
        Quest* quest = get_quest(vnum);
        if (!quest) {
            bugf("fread_quests: %s has unknown quest VNUM %d.", NAME_STR(ch),
                vnum);
            continue;
        }
        add_quest_to_log(ch->pcdata->quest_log, quest, state, progress);
        word = feof(fp) ? "End" : fread_word(fp);
    }
}

