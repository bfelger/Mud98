////////////////////////////////////////////////////////////////////////////////
// persist/player/rom-olc/player_persist_rom_olc.c
// ROM-OLC player persistence implementation (legacy player files).
////////////////////////////////////////////////////////////////////////////////

#include <persist/player/player_persist.h>

#include <color.h>
#include <comm.h>
#include <config.h>
#include <db.h>
#include <digest.h>
#include <handler.h>
#include <lookup.h>
#include <magic.h>
#include <recycle.h>
#include <skills.h>
#include <stringutils.h>
#include <tables.h>
#include <vt.h>

#include <persist/player/player_persist.h>
#include <persist/theme/rom-olc/theme_rom_olc_io.h>

#include <data/mobile_data.h>
#include <data/player.h>
#include <data/race.h>
#include <data/skill.h>
#include <data/tutorial.h>

#include <entities/faction.h>
#include <entities/object.h>

#include <lox/lox.h>

#include <ctype.h>
#include <errno.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

extern bool test_output_enabled;

#define MAX_NEST 100
static Object* rgObjNest[MAX_NEST];

static void fwrite_char(Mobile* ch, FILE* fp);
static void fwrite_obj(Mobile* ch, Object* obj, FILE* fp, int iNest);
static void fwrite_pet(Mobile* pet, FILE* fp);
static void fwrite_themes(Mobile* ch, FILE* fp);
static void fwrite_quests(Mobile* ch, FILE* fp);

static void fread_char(Mobile* ch, FILE* fp);
static void fread_pet(Mobile* ch, FILE* fp);
static void fread_obj(Mobile* ch, FILE* fp);
static void fread_theme(Mobile* ch, FILE* fp);
static void fread_quests(Mobile* ch, FILE* fp);

PersistResult player_persist_rom_olc_save(const PlayerPersistSaveParams* params)
{
    if (!params || !params->fp || !params->ch)
        return (PersistResult){ PERSIST_ERR_INTERNAL, "player ROM save: invalid params", -1 };

    Mobile* ch = params->ch;
    FILE* fp = params->fp;

    fwrite_char(ch, fp);
    for (Node* node = ch->objects.back; node != NULL; node = node->prev) {
        Object* obj = AS_OBJECT(node->value);
        fwrite_obj(ch, obj, fp, 0);
    }
    if (ch->pet != NULL && ch->pet->in_room == ch->in_room)
        fwrite_pet(ch->pet, fp);
    fwrite_themes(ch, fp);
    fwrite_quests(ch, fp);
    fprintf(fp, "#END\n");

    return (PersistResult){ PERSIST_OK, NULL, -1 };
}

PersistResult player_persist_rom_olc_load(const PlayerPersistLoadParams* params)
{
    if (!params || !params->fp || !params->ch)
        return (PersistResult){ PERSIST_ERR_INTERNAL, "player ROM load: invalid params", -1 };

    Mobile* ch = params->ch;
    FILE* fp = params->fp;

    for (int iNest = 0; iNest < MAX_NEST; iNest++)
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
            bug("player_persist_rom_olc_load: # not found.", 0);
            break;
        }

        word = fread_word(fp);
        if (!str_cmp(word, "PLAYER"))
            fread_char(ch, fp);
        else if (!str_cmp(word, "OBJECT") || !str_cmp(word, "O"))
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
            bug("player_persist_rom_olc_load: bad section.", 0);
            break;
        }
    }

    return (PersistResult){ PERSIST_OK, NULL, -1 };
}

static void fwrite_char(Mobile* ch, FILE* fp)
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
    if (ch->prompt != NULL
        && str_cmp(ch->prompt, "<%hhp %mm %vmv> ")
        && str_cmp(ch->prompt, "^p<%hhp %mm %vmv>" COLOR_CLEAR " "))
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
                ? VNUM_FIELD(ch->was_in_room)
            : ch->in_room == NULL ? ch->pcdata->recall
                                  : VNUM_FIELD(ch->in_room));

    fprintf(fp, "HMV  %d %d %d %d %d %d\n", ch->hit, ch->max_hit, ch->mana,
            ch->max_mana, ch->move, ch->max_move);
    fprintf(fp, "Gold %d\n", ch->gold);
    fprintf(fp, "Silv %d\n", ch->silver);
    fprintf(fp, "Copp %d\n", ch->copper);
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
        fprintf(fp, "Vnum %"PRVNUM"\n", VNUM_FIELD(ch->prototype));
    }
    else {
        char digest_buf[256];
        if (ch->pcdata->pwd_digest_hex != NULL)
            strcpy(digest_buf, ch->pcdata->pwd_digest_hex);
        else
            bin_to_hex(digest_buf, ch->pcdata->pwd_digest, ch->pcdata->pwd_digest_len);
        fprintf(fp, "PwdDigest %s~\n", digest_buf);

        if (ch->pcdata->bamfin != NULL && ch->pcdata->bamfin[0] != '\0')
            fprintf(fp, "Bin  %s~\n", ch->pcdata->bamfin);
        if (ch->pcdata->bamfout != NULL && ch->pcdata->bamfout[0] != '\0')
            fprintf(fp, "Bout %s~\n", ch->pcdata->bamfout);
        if (ch->pcdata->title != NULL)
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
        if (ch->pcdata->tutorial != NULL) {
            fprintf(fp, "Tut %s~ %d\n", ch->pcdata->tutorial->name, ch->pcdata->tutorial_step);
        }

        for (pos = 0; pos < MAX_ALIAS; pos++) {
            if (ch->pcdata->alias[pos] == NULL
                || ch->pcdata->alias_sub[pos] == NULL)
                break;

            fprintf(fp, "Alias %s %s~\n", ch->pcdata->alias[pos],
                    ch->pcdata->alias_sub[pos]);
        }

        FactionReputationList* reps = &ch->pcdata->reputations;
        if (reps->entries != NULL) {
            for (size_t rep = 0; rep < reps->count; ++rep) {
                FactionReputation* entry = &reps->entries[rep];
                if (entry->vnum != 0)
                    fprintf(fp, "Faction %" PRVNUM " %d\n", entry->vnum, entry->value);
            }
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
}

static void fwrite_themes(Mobile* ch, FILE* fp)
{
    for (int i = 0; i < MAX_THEMES; ++i) {
        if (ch->pcdata->color_themes[i] == NULL)
            continue;

        ColorTheme* theme = ch->pcdata->color_themes[i];
        theme_rom_olc_write_theme(fp, theme, "End");
    }
}

static void fwrite_pet(Mobile* pet, FILE* fp)
{
    Affect* affect;

    fprintf(fp, "#PET\n");

    fprintf(fp, "Vnum %"PRVNUM"\n", VNUM_FIELD(pet->prototype));

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
    if (pet->copper > 0) fprintf(fp, "Copp %d\n", pet->copper);
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
}

static void fwrite_obj(Mobile* ch, Object* obj, FILE* fp, int iNest)
{
    ExtraDesc* ed;
    Affect* affect;

    if ((ch->level < obj->level - 2 && obj->item_type != ITEM_CONTAINER)
        || obj->item_type == ITEM_KEY
        || (obj->item_type == ITEM_MAP && !obj->value[0]))
        return;

    fprintf(fp, "#O\n");
    fprintf(fp, "Vnum %"PRVNUM"\n", VNUM_FIELD(obj->prototype));
    if (obj->enchanted)
        fprintf(fp, "Enchanted\n");
    fprintf(fp, "Nest %d\n", iNest);

    if (!lox_streq(NAME_FIELD(obj), NAME_FIELD(obj->prototype)))
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

    for (Node* node = obj->objects.back; node != NULL; node = node->prev) {
        Object* content = AS_OBJECT(node->value);
        fwrite_obj(ch, content, fp, iNest + 1);
    }
}

static void fwrite_quests(Mobile* ch, FILE* fp)
{
    if (ch->pcdata == NULL || ch->pcdata->quest_log == NULL)
        return;

    fprintf(fp, "#QUESTLOG\n");

    QuestStatus* qs;

    FOR_EACH(qs, ch->pcdata->quest_log->quests) {
        fprintf(fp, "%d %d %d  ", qs->vnum, qs->progress, qs->state);
    }

    fprintf(fp, "\nEnd\n\n");
}

#if defined(KEY)
#undef KEY
#endif

#define KEY(literal, field, value)                                             \
    if (!str_cmp(word, literal)) {                                             \
        field = value;                                                         \
        fMatch = true;                                                         \
        break;                                                                 \
    }

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

static void fread_char(Mobile* ch, FILE* fp)
{
    char buf[MAX_STRING_LENGTH];
    char* word;
    bool fMatch;
    int count = 0;
    time_t lastlogoff = current_time;
    int16_t percent;

    if (!test_output_enabled)
        printf_log("Loading %s.", NAME_STR(ch));

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
            KEY("Copp", ch->copper, (int16_t)fread_number(fp));

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

        case 'F':
            if (!str_cmp(word, "Faction")) {
                VNUM faction_vnum = fread_number(fp);
                int value = fread_number(fp);
                faction_set(ch->pcdata, faction_vnum, value);
                fMatch = true;
                break;
            }
            break;

        case 'G':
            KEY("Gold", ch->gold, (int16_t)fread_number(fp));
            if (!str_cmp(word, "Group") || !str_cmp(word, "Gr")) {
                SKNUM gn;
                char* temp;

                temp = fread_word(fp);
                gn = group_lookup(temp);
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
                if (room_data) {
                    Area* area = get_area_for_player(ch, room_data->area_data);
                    if (area || room_data->area_data->low_range == 1) {
                        ch->in_room = get_room_for_player(ch, VNUM_FIELD(room_data));
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
                fread_number(fp);
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

            if (!str_cmp(word, "Tut")) {
                char* n = fread_string(fp);
                Tutorial* t = get_tutorial(n);
                int s = fread_number(fp);
                if (t != NULL) {
                    ch->pcdata->tutorial = t;
                    ch->pcdata->tutorial_step = s;
                }
                else {
                    bug("fread_char: unknown tutorial '%s'. ", n);
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

static void fread_pet(Mobile* ch, FILE* fp)
{
    char* word;
    Mobile* pet;
    bool fMatch;
    time_t lastlogoff = current_time;
    int16_t percent;

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
            KEY("Copp", pet->copper, (int16_t)fread_number(fp));
            break;
        }

        if (!fMatch) {
            bug("Fread_pet: no match.", 0);
            fread_to_eol(fp);
        }
    }
}

static void fread_obj(Mobile* ch, FILE* fp)
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
    first = true;

    word = feof(fp) ? "End" : fread_word(fp);
    if (!str_cmp(word, "Vnum")) {
        VNUM vnum;
        first = false;

        vnum = fread_vnum(fp);
        if (get_object_prototype(vnum) == NULL) {
            bug("Fread_obj: bad vnum %"PRVNUM".", vnum);
        }
        else {
            obj = create_object(get_object_prototype(vnum), -1);
        }
    }

    if (obj == NULL)
    {
        obj = new_object();
        SET_NAME(obj, lox_empty_string);
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

static void fread_theme(Mobile* ch, FILE* fp)
{
    ColorTheme* theme = theme_rom_olc_read_theme(fp, "End", NAME_STR(ch));
    if (!theme)
        return;

    for (int i = 0; i < MAX_THEMES; ++i) {
        if (!ch->pcdata->color_themes[i]) {
            ch->pcdata->color_themes[i] = theme;
            return;
        }
    }

    bugf("Could not find a free color theme slot for %s.", NAME_STR(ch));
    free_color_theme(theme);
}

static void fread_quests(Mobile* ch, FILE* fp)
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
            word = feof(fp) ? "End" : fread_word(fp);
            continue;
        }
        add_quest_to_log(ch->pcdata->quest_log, quest, state, progress);
        word = feof(fp) ? "End" : fread_word(fp);
    }
}
