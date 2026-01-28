/**************************************************************************
 *  File: olc_save.c                                                       *
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
/* OLC_SAVE.C
 * This takes care of saving all the .are information.
 * Notes:
 * -If a good syntax checker is used for setting vnum ranges of areas
 *  then it would become possible to just cycle through vnums instead
 *  of using the hash stuff and checking that the room or reset or
 *  mob etc is part of that area.
 */

#include "olc.h"
#include "olc_save.h"

#include <persist/area/area_persist.h>
#include <persist/persist_io_adapters.h>
#include <persist/area/rom-olc/area_persist_rom_olc.h>

#include <craft/gather.h>

#include <comm.h>
#include <config.h>
#include <db.h>
#include <handler.h>
#include <lookup.h>
#include <mob_cmds.h>
#include <skills.h>
#include <special.h>
#include <tables.h>
#include <tablesave.h>
#include <lox/lox.h>

#include <entities/descriptor.h>
#include <entities/event.h>
#include <entities/faction.h>
#include <entities/room.h>
#include <entities/object.h>
#include <entities/player_data.h>

#include <data/mobile_data.h>
#include <entities/quest.h>
#include <data/race.h>
#include <data/skill.h>
#include <data/loot.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#define DIF(a,b) (~((~a)|(b)))

/*
 *  Verbose writes reset data in plain english into the comments
 *  section of the resets.  It makes areas considerably larger but
 *  may aid in debugging.
 */

/* #define VERBOSE */

/*****************************************************************************
 Name:		fix_string
 Purpose:	Returns a string without \r and ~.
 ****************************************************************************/
char* fix_string(const char* str)
{
    static char strfix[MAX_STRING_LENGTH * 2];
    int i;
    int o;

    if (str == NULL)
        return "";

    for (o = i = 0; str[i + o] != '\0'; i++) {
        if (str[i + o] == '\r' || str[i + o] == '~')
            o++;
        strfix[i] = str[i + o];
    }
    strfix[i] = '\0';
    return strfix;
}

char* fix_lox_script(const char* str)
{
    static char script_fix[MAX_STRING_LENGTH * 4];
    size_t dst = 0;

    if (str == NULL)
        return "";

    for (size_t src = 0; str[src] != '\0'; ++src) {
        if (str[src] == '\r')
            continue;
        script_fix[dst++] = str[src];
        if (dst >= sizeof(script_fix) - 1)
            break;
    }

    script_fix[dst] = '\0';
    return script_fix;
}

bool area_changed()
{
    AreaData* area_data;

    FOR_EACH_AREA(area_data)
        if (IS_SET(area_data->area_flags, AREA_CHANGED))
            return true;

    return false;
}

void save_mobprogs(FILE* fp, AreaData* area_data)
{
    MobProgCode* pMprog;

    fprintf(fp, "#MOBPROGS\n");

    for (VNUM i = area_data->min_vnum; i <= area_data->max_vnum; i++) {
        if ((pMprog = get_mprog_index(i)) != NULL) {
            fprintf(fp, "#%"PRVNUM"\n", i);
            fprintf(fp, "%s~\n", fix_string(pMprog->code));
        }
    }

    fprintf(fp, "#0\n\n");
    return;
}

/*****************************************************************************
 Name:		save_area_list
 Purpose:	Saves the listing of files to be loaded at startup.
 Called by:	do_asave(olc_save.c).
 ****************************************************************************/
void save_area_list()
{
    FILE* fp;
    AreaData* area;
    HelpArea* ha;

    OPEN_OR_RETURN(fp = open_write_area_list());

    FOR_EACH(ha, help_area_list)
        if (ha->area_data == NULL)
            fprintf(fp, "%s\n", ha->filename);

    FOR_EACH_AREA(area) {
        fprintf(fp, "%s\n", area->file_name);
    }

    fprintf(fp, "$\n");
    
    close_file(fp);

    return;
}

/*
 * ROM OLC
 * Used in save_mobile and save_object below.  Writes
 * flags on the form fread_flag reads.
 *
 * buf[] must hold at least 32+1 characters.
 *
 * -- Hugin
 */
char* fwrite_flag(long flags, char buf[])
{
    char offset;
    char* cp;

    buf[0] = '\0';

    if (flags == 0) {
        strcpy(buf, "0");
        return buf;
    }

    /* 32 -- number of bits in a long */

    for (offset = 0, cp = buf; offset < 32; offset++)
        if (flags & ((long)1 << offset)) {
            if (offset <= 'Z' - 'A')
                *(cp++) = 'A' + offset;
            else
                *(cp++) = 'a' + offset - ('Z' - 'A' + 1);
        }

    *cp = '\0';

    return buf;
}

/*****************************************************************************
 Name:		save_mobile
 Purpose:	Save one mobile to file, new format -- Hugin
 Called by:	save_mobiles (below).
 ****************************************************************************/

static void save_faction_relations(FILE* fp, const char* keyword, ValueArray* relations)
{
    bool wrote = false;

    if (relations == NULL || relations->count == 0)
        return;

    for (int i = 0; i < relations->count; ++i) {
        Value entry = relations->values[i];
        if (!IS_INT(entry))
            continue;

        VNUM rel_vnum = (VNUM)AS_INT(entry);
        if (rel_vnum == 0)
            continue;

        if (!wrote) {
            fprintf(fp, "%s", keyword);
            wrote = true;
        }

        fprintf(fp, " %" PRVNUM, rel_vnum);
    }

    if (wrote)
        fprintf(fp, " 0\n");
}

void save_factions(FILE* fp, AreaData* area)
{
    if (faction_table.capacity == 0 || faction_table.entries == NULL)
        return;

    bool found = false;

    for (int idx = 0; idx < faction_table.capacity; ++idx) {
        Entry* entry = &faction_table.entries[idx];
        if (IS_NIL(entry->value) || !IS_FACTION(entry->value))
            continue;

        Faction* faction = AS_FACTION(entry->value);
        if (faction->area != area)
            continue;

        if (!found) {
            found = true;
            fprintf(fp, "#FACTIONS\n");
        }

        fprintf(fp, "#%" PRVNUM "\n", VNUM_FIELD(faction));
        fprintf(fp, "Name %s~\n", NAME_STR(faction));
        fprintf(fp, "DefaultStanding %d\n", faction->default_standing);
        save_faction_relations(fp, "Allies", &faction->allies);
        save_faction_relations(fp, "Opposing", &faction->enemies);
        fprintf(fp, "End\n");
    }

    if (!found)
        return;

    fprintf(fp, "#0\n\n");
}

void save_mobile(FILE* fp, MobPrototype* p_mob_proto)
{
    int16_t race = p_mob_proto->race;
    long temp;
    char buf[MAX_STRING_LENGTH];
    MobProg* pMprog;

    fprintf(fp, "#%"PRVNUM"\n", VNUM_FIELD(p_mob_proto));
    fprintf(fp, "%s~\n", NAME_STR(p_mob_proto));
    fprintf(fp, "%s~\n", p_mob_proto->short_descr);
    fprintf(fp, "%s~\n", fix_string(p_mob_proto->long_descr));
    fprintf(fp, "%s~\n", fix_string(p_mob_proto->description));
    fprintf(fp, "%s~\n", race_table[race].name);

    temp = DIF(p_mob_proto->act_flags, race_table[race].act_flags);
    fprintf(fp, "%s ", fwrite_flag(temp, buf));

    temp = DIF(p_mob_proto->affect_flags, race_table[race].aff);
    fprintf(fp, "%s ", fwrite_flag(temp, buf));

    fprintf(fp, "%d %d\n", p_mob_proto->alignment, p_mob_proto->group);
    fprintf(fp, "%d ", p_mob_proto->level);
    fprintf(fp, "%d ", p_mob_proto->hitroll);
    fprintf(fp, "%dd%d+%d ", p_mob_proto->hit[DICE_NUMBER],
        p_mob_proto->hit[DICE_TYPE],
        p_mob_proto->hit[DICE_BONUS]);
    fprintf(fp, "%dd%d+%d ", p_mob_proto->mana[DICE_NUMBER],
        p_mob_proto->mana[DICE_TYPE],
        p_mob_proto->mana[DICE_BONUS]);
    fprintf(fp, "%dd%d+%d ", p_mob_proto->damage[DICE_NUMBER],
        p_mob_proto->damage[DICE_TYPE],
        p_mob_proto->damage[DICE_BONUS]);
    fprintf(fp, "%s\n", attack_table[p_mob_proto->dam_type].name);
    fprintf(fp, "%d %d %d %d\n", p_mob_proto->ac[AC_PIERCE] / 10,
        p_mob_proto->ac[AC_BASH] / 10,
        p_mob_proto->ac[AC_SLASH] / 10,
        p_mob_proto->ac[AC_EXOTIC] / 10);

    temp = DIF(p_mob_proto->atk_flags, race_table[race].off);
    fprintf(fp, "%s ", fwrite_flag(temp, buf));

    temp = DIF(p_mob_proto->imm_flags, race_table[race].imm);
    fprintf(fp, "%s ", fwrite_flag(temp, buf));

    temp = DIF(p_mob_proto->res_flags, race_table[race].res);
    fprintf(fp, "%s ", fwrite_flag(temp, buf));

    temp = DIF(p_mob_proto->vuln_flags, race_table[race].vuln);
    fprintf(fp, "%s\n", fwrite_flag(temp, buf));

    fprintf(fp, "%s %s %s %d\n",
        position_table[p_mob_proto->start_pos].short_name,
        position_table[p_mob_proto->default_pos].short_name,
        sex_table[p_mob_proto->sex].name,
        p_mob_proto->wealth);

    temp = DIF(p_mob_proto->form, race_table[race].form);
    fprintf(fp, "%s ", fwrite_flag(temp, buf));

    temp = DIF(p_mob_proto->parts, race_table[race].parts);
    fprintf(fp, "%s ", fwrite_flag(temp, buf));

    fprintf(fp, "%s ", mob_size_table[p_mob_proto->size].name);
    fprintf(fp, "'%s'\n", ((p_mob_proto->material[0] != '\0') ? p_mob_proto->material : "unknown"));

    if ((temp = DIF(race_table[race].act_flags, p_mob_proto->act_flags)))
        fprintf(fp, "F act %s\n", fwrite_flag(temp, buf));

    if ((temp = DIF(race_table[race].aff, p_mob_proto->affect_flags)))
        fprintf(fp, "F aff %s\n", fwrite_flag(temp, buf));

    if ((temp = DIF(race_table[race].off, p_mob_proto->atk_flags)))
        fprintf(fp, "F off %s\n", fwrite_flag(temp, buf));

    if ((temp = DIF(race_table[race].imm, p_mob_proto->imm_flags)))
        fprintf(fp, "F imm %s\n", fwrite_flag(temp, buf));

    if ((temp = DIF(race_table[race].res, p_mob_proto->res_flags)))
        fprintf(fp, "F res %s\n", fwrite_flag(temp, buf));

    if ((temp = DIF(race_table[race].vuln, p_mob_proto->vuln_flags)))
        fprintf(fp, "F vul %s\n", fwrite_flag(temp, buf));

    if ((temp = DIF(race_table[race].form, p_mob_proto->form)))
        fprintf(fp, "F for %s\n", fwrite_flag(temp, buf));

    if ((temp = DIF(race_table[race].parts, p_mob_proto->parts)))
        fprintf(fp, "F par %s\n", fwrite_flag(temp, buf));

    if (p_mob_proto->faction_vnum != 0)
        fprintf(fp, "Faction %" PRVNUM "\n", p_mob_proto->faction_vnum);

    if (!IS_NULLSTR(p_mob_proto->loot_table))
        fprintf(fp, "LootTable %s~\n", p_mob_proto->loot_table);

    if (p_mob_proto->craft_mats != NULL && p_mob_proto->craft_mat_count > 0) {
        fprintf(fp, "CraftMats");
        for (int i = 0; i < p_mob_proto->craft_mat_count; i++) {
            fprintf(fp, " %" PRVNUM, p_mob_proto->craft_mats[i]);
        }
        fprintf(fp, " 0\n");
    }

    FOR_EACH(pMprog, p_mob_proto->mprogs) {
        fprintf(fp, "M '%s' %"PRVNUM" %s~\n",
            event_trigger_name(pMprog->trig_type), pMprog->vnum,
            pMprog->trig_phrase);
    }

    save_events(fp, &p_mob_proto->header);

    if (p_mob_proto->header.script != NULL) {
        fprintf(fp, "L\n%s~\n", fix_lox_script(p_mob_proto->header.script->chars));
    }

    return;
}

/*****************************************************************************
 Name:		save_mobiles
 Purpose:	Save #MOBILES secion of an area file.
 Called by:	save_area(olc_save.c).
 Notes:         Changed for ROM OLC.
 ****************************************************************************/
void save_mobiles(FILE* fp, AreaData* area)
{
    MobPrototype* pMob;

    fprintf(fp, "#MOBILES\n");

    for (VNUM i = area->min_vnum; i <= area->max_vnum; i++) {
        if ((pMob = get_mob_prototype(i)))
            save_mobile(fp, pMob);
    }

    fprintf(fp, "#0\n\n\n\n");
    return;
}

/*****************************************************************************
 Name:		save_object
 Purpose:	Save one object to file.
                new ROM format saving -- Hugin
 Called by:	save_objects (below).
 ****************************************************************************/
void save_object(FILE* fp, ObjPrototype* obj_proto)
{
    char letter;
    Affect* pAf;
    ExtraDesc* pEd;
    char buf[MAX_STRING_LENGTH];

    fprintf(fp, "#%"PRVNUM"\n", VNUM_FIELD(obj_proto));
    fprintf(fp, "%s~\n", NAME_STR(obj_proto));
    fprintf(fp, "%s~\n", obj_proto->short_descr);
    fprintf(fp, "%s~\n", fix_string(obj_proto->description));
    fprintf(fp, "%s~\n", obj_proto->material);
    fprintf(fp, "%s ", item_type_table[obj_proto->item_type].name);
    fprintf(fp, "%s ", fwrite_flag(obj_proto->extra_flags, buf));
    fprintf(fp, "%s\n", fwrite_flag(obj_proto->wear_flags, buf));

/*
 *  Using fwrite_flag to write most values gives a strange
 *  looking area file, consider making a case for each
 *  item type later.
 */

    switch (obj_proto->item_type) {
    default:
        fprintf(fp, "%s ", fwrite_flag(obj_proto->value[0], buf));
        fprintf(fp, "%s ", fwrite_flag(obj_proto->value[1], buf));
        fprintf(fp, "%s ", fwrite_flag(obj_proto->value[2], buf));
        fprintf(fp, "%s ", fwrite_flag(obj_proto->value[3], buf));
        fprintf(fp, "%s\n", fwrite_flag(obj_proto->value[4], buf));
        break;

    case ITEM_LIGHT:
        fprintf(fp, "0 0 %d 0 0\n",
            obj_proto->light.hours < 1 ? 999  /* infinite */
            : obj_proto->light.hours);
        break;

    case ITEM_MONEY:
        fprintf(fp, "%d %d %d 0 0\n",
            obj_proto->money.copper,
            obj_proto->money.silver,
            obj_proto->money.gold);
        break;

    case ITEM_DRINK_CON:
        fprintf(fp, "%d %d '%s' %d 0\n",
            obj_proto->drink_con.capacity,
            obj_proto->drink_con.current,
            liquid_table[obj_proto->drink_con.liquid_type].name,
            obj_proto->drink_con.poisoned);
        break;

    case ITEM_FOUNTAIN:
        fprintf(fp, "%d %d '%s' 0 0\n",
            obj_proto->fountain.capacity,
            obj_proto->fountain.current,
            liquid_table[obj_proto->fountain.liquid_type].name);
        break;

    case ITEM_CONTAINER:
        fprintf(fp, "%d %s %d %d %d\n",
            obj_proto->container.capacity,
            fwrite_flag(obj_proto->container.flags, buf),
            obj_proto->container.key_vnum,
            obj_proto->container.max_item_weight,
            obj_proto->container.weight_mult);
        break;

    case ITEM_FOOD:
        fprintf(fp, "%d %d 0 %s 0\n",
            obj_proto->food.hours_full,
            obj_proto->food.hours_hunger,
            fwrite_flag(obj_proto->food.poisoned, buf));
        break;

    case ITEM_PORTAL:
        fprintf(fp, "%d %s %s %d %d\n",
            obj_proto->portal.charges,
            fwrite_flag(obj_proto->portal.gate_flags, buf),
            fwrite_flag(obj_proto->portal.key_vnum, buf),
            obj_proto->portal.destination,
            obj_proto->portal.key_vnum);
        break;

    case ITEM_FURNITURE:
        fprintf(fp, "%d %d %s %d %d\n",
            obj_proto->furniture.max_people,
            obj_proto->furniture.max_weight,
            fwrite_flag(obj_proto->furniture.flags, buf),
            obj_proto->furniture.heal_rate,
            obj_proto->furniture.mana_rate);
        break;

    case ITEM_WEAPON:
        fprintf(fp, "%s %d %d '%s' %s\n",
            weapon_table[obj_proto->weapon.weapon_type].name,
            obj_proto->weapon.num_dice,
            obj_proto->weapon.size_dice,
            attack_table[obj_proto->weapon.damage_type].name,
            fwrite_flag(obj_proto->weapon.flags, buf));
        break;

    case ITEM_ARMOR:
        fprintf(fp, "%d %d %d %d %d\n",
            obj_proto->armor.ac_pierce,
            obj_proto->armor.ac_bash,
            obj_proto->armor.ac_slash,
            obj_proto->armor.ac_exotic,
            obj_proto->armor.armor_type);
        break;

    case ITEM_PILL:
    case ITEM_POTION:
    case ITEM_SCROLL:
        fprintf(fp, "%d '%s' '%s' '%s' '%s'\n",
            obj_proto->value[0] > 0 ? /* no negative numbers */
            obj_proto->value[0]
            : 0,
            obj_proto->value[1] > 0 ?
            skill_table[obj_proto->value[1]].name
            : "",
            obj_proto->value[2] > 0 ?
            skill_table[obj_proto->value[2]].name
            : "",
            obj_proto->value[3] > 0 ?
            skill_table[obj_proto->value[3]].name
            : "",
            obj_proto->value[4] > 0 ?
            skill_table[obj_proto->value[4]].name
            : "");
        break;

    case ITEM_STAFF:
    case ITEM_WAND:
        fprintf(fp, "%d ", obj_proto->value[0]);
        fprintf(fp, "%d ", obj_proto->value[1]);
        fprintf(fp, "%d '%s' 0\n",
            obj_proto->value[2],
            obj_proto->value[3] > 0 ?
            skill_table[obj_proto->value[3]].name
            : "");
        break;

    case ITEM_MAT:
        // Crafting material: mat_type amount quality unused unused
        fprintf(fp, "%d %d %d %d %d\n",
            obj_proto->craft_mat.mat_type,
            obj_proto->craft_mat.amount,
            obj_proto->craft_mat.quality,
            obj_proto->craft_mat.unused3,
            obj_proto->craft_mat.unused4);
        break;

    case ITEM_GATHER:
        // Gather: gather_type mat_vnum quantity min_skill unused
        fprintf(fp, "%s %d %d %d %d\n",
            gather_type_name(obj_proto->gather.gather_type),
            obj_proto->gather.mat_vnum,
            obj_proto->gather.quantity,
            obj_proto->gather.min_skill,
            obj_proto->gather.unused4);
        break;

    case ITEM_WORKSTATION:
        // Workstation: station_flags bonus min_skill unused unused
        fprintf(fp, "%s %d %d %d %d\n",
            fwrite_flag(obj_proto->workstation.station_flags, buf),
            obj_proto->workstation.bonus,
            obj_proto->workstation.min_skill,
            obj_proto->workstation.unused3,
            obj_proto->workstation.unused4);
        break;
    }

    fprintf(fp, "%d ", obj_proto->level);
    fprintf(fp, "%d ", obj_proto->weight);
    fprintf(fp, "%d ", obj_proto->cost);

    if (obj_proto->condition > 90) letter = 'P';
    else if (obj_proto->condition > 75) letter = 'G';
    else if (obj_proto->condition > 50) letter = 'A';
    else if (obj_proto->condition > 25) letter = 'W';
    else if (obj_proto->condition > 10) letter = 'D';
    else if (obj_proto->condition > 0) letter = 'B';
    else                                   letter = 'R';

    fprintf(fp, "%c\n", letter);

    FOR_EACH(pAf, obj_proto->affected) {
        if (pAf->where == TO_OBJECT || pAf->bitvector == 0)
            fprintf(fp, "A\n%d %d\n", pAf->location, pAf->modifier);
        else {
            fprintf(fp, "F\n");

            switch (pAf->where) {
            case TO_AFFECTS:
                fprintf(fp, "A ");
                break;
            case TO_IMMUNE:
                fprintf(fp, "I ");
                break;
            case TO_RESIST:
                fprintf(fp, "R ");
                break;
            case TO_VULN:
                fprintf(fp, "V ");
                break;
            case TO_WEAPON:
            case TO_OBJECT:
            default:
                bug("olc_save: Invalid Affect->where", 0);
                break;
            }

            fprintf(fp, "%d %d %s\n", pAf->location, pAf->modifier,
                fwrite_flag(pAf->bitvector, buf));
        }
    }

    FOR_EACH(pEd, obj_proto->extra_desc) {
        fprintf(fp, "E\n%s~\n%s~\n", pEd->keyword,
            fix_string(pEd->description));
    }

    save_events(fp, &obj_proto->header);

    if (obj_proto->salvage_mats != NULL && obj_proto->salvage_mat_count > 0) {
        fprintf(fp, "SalvageMats");
        for (int i = 0; i < obj_proto->salvage_mat_count; i++) {
            fprintf(fp, " %" PRVNUM, obj_proto->salvage_mats[i]);
        }
        fprintf(fp, " 0\n");
    }

    if (obj_proto->header.script != NULL) {
        fprintf(fp, "L\n%s~\n", fix_lox_script(obj_proto->header.script->chars));
    }

    return;
}

/*****************************************************************************
 Name:		save_objects
 Purpose:	Save #OBJECTS section of an area file.
 Called by:	save_area(olc_save.c).
 Notes:         Changed for ROM OLC.
 ****************************************************************************/
void save_objects(FILE* fp, AreaData* area)
{
    ObjPrototype* pObj;

    fprintf(fp, "#OBJECTS\n");

    for (VNUM i = area->min_vnum; i <= area->max_vnum; i++) {
        if ((pObj = get_object_prototype(i)))
            save_object(fp, pObj);
    }

    fprintf(fp, "#0\n\n\n\n");
    return;
}

/*****************************************************************************
 Name:		save_rooms
 Purpose:	Save #ROOMS section of an area file.
 Called by:	save_area(olc_save.c).
 ****************************************************************************/
void save_rooms(FILE* fp, AreaData* area)
{
    RoomData* pRoomIndex;
    ExtraDesc* pEd;
    RoomExitData* room_exit;
    char buf[MSL];
    int locks;

    fprintf(fp, "#ROOMS\n");

    FOR_EACH_GLOBAL_ROOM(pRoomIndex) {
        if (pRoomIndex->area_data == area) {
            fprintf(fp, "#%"PRVNUM"\n", VNUM_FIELD(pRoomIndex));
            fprintf(fp, "%s~\n", NAME_STR(pRoomIndex));
            fprintf(fp, "%s~\n0\n", fix_string(pRoomIndex->description));
            fprintf(fp, "%s ", fwrite_flag(pRoomIndex->room_flags, buf));
            fprintf(fp, "%d\n", pRoomIndex->sector_type);

            FOR_EACH(pEd, pRoomIndex->extra_desc) {
                fprintf(fp, "E\n%s~\n%s~\n", pEd->keyword,
                    fix_string(pEd->description));
            }

            for (DayCyclePeriod* period = pRoomIndex->periods; period != NULL; period = period->next) {
                const char* name = (period->name && period->name[0] != '\0') ? period->name : "period";
                fprintf(fp, "P %s %d %d\n", name, period->start_hour, period->end_hour);
                fprintf(fp, "%s~\n", fix_string(period->description));
                if (period->enter_message && period->enter_message[0] != '\0') {
                    fprintf(fp, "B %s\n", name);
                    fprintf(fp, "%s~\n", fix_string(period->enter_message));
                }
                if (period->exit_message && period->exit_message[0] != '\0') {
                    fprintf(fp, "A %s\n", name);
                    fprintf(fp, "%s~\n", fix_string(period->exit_message));
                }
            }

            // Put randomized rooms back in their original place to reduce
            // file deltas on save.
            RoomExitData* ex_to_save[DIR_MAX] = { NULL };
            for (int i = 0; i < DIR_MAX; ++i) {
                if ((room_exit = pRoomIndex->exit_data[i]) == NULL)
                    continue;
                ex_to_save[room_exit->orig_dir] = room_exit;
            }

            for (int i = 0; i < DIR_MAX; i++) {
                if ((room_exit = ex_to_save[i]) == NULL)
                    continue;

                if (room_exit->to_room) {
                    locks = 0;

                    if (IS_SET(room_exit->exit_reset_flags, EX_CLOSED)
                        || IS_SET(room_exit->exit_reset_flags, EX_LOCKED)
                        || IS_SET(room_exit->exit_reset_flags, EX_PICKPROOF)
                        || IS_SET(room_exit->exit_reset_flags, EX_NOPASS)
                        || IS_SET(room_exit->exit_reset_flags, EX_EASY)
                        || IS_SET(room_exit->exit_reset_flags, EX_HARD)
                        || IS_SET(room_exit->exit_reset_flags, EX_INFURIATING)
                        || IS_SET(room_exit->exit_reset_flags, EX_NOCLOSE)
                        || IS_SET(room_exit->exit_reset_flags, EX_NOLOCK))
                        SET_BIT(room_exit->exit_reset_flags, EX_ISDOOR);

                    if (IS_SET(room_exit->exit_reset_flags, EX_ISDOOR))
                        locks = IS_SET(room_exit->exit_reset_flags, EX_LOCKED) ? 2 : 1;

                    fprintf(fp, "D%d\n", room_exit->orig_dir);
                    fprintf(fp, "%s~\n", fix_string(room_exit->description));
                    fprintf(fp, "%s~\n", room_exit->keyword);
                    fprintf(fp, "%d %d %"PRVNUM"\n", 
                        locks,
                        room_exit->key,
                        VNUM_FIELD(room_exit->to_room));
                }
            }

            save_events(fp, &pRoomIndex->header);

            if (pRoomIndex->header.script != NULL) {
                fprintf(fp, "L\n%s~\n", fix_lox_script(pRoomIndex->header.script->chars));
            }

            if (pRoomIndex->suppress_daycycle_messages)
                fprintf(fp, "W 1\n");

            fprintf(fp, "S\n");
        }
    }

    fprintf(fp, "#0\n\n\n\n");
    return;
}

/*****************************************************************************
 Name:		save_specials
 Purpose:	Save #SPECIALS section of area file.
 Called by:	save_area(olc_save.c).
 ****************************************************************************/
void save_specials(FILE* fp, AreaData* area)
{
    MobPrototype* p_mob_proto;

    fprintf(fp, "#SPECIALS\n");

    FOR_EACH_MOB_PROTO(p_mob_proto) {
        if (p_mob_proto && p_mob_proto->area == area && p_mob_proto->spec_fun) {
#if defined( VERBOSE )
            fprintf(fp, "M %"PRVNUM" %s Load to: %s\n", VNUM_FIELD(p_mob_proto),
                spec_name(p_mob_proto->spec_fun),
                p_mob_proto->short_descr);
#else
            fprintf(fp, "M %"PRVNUM" %s\n", VNUM_FIELD(p_mob_proto),
                spec_name(p_mob_proto->spec_fun));
#endif
        }
    }

    fprintf(fp, "S\n\n\n\n");
    return;
}

/*
 * This function is obsolete.  It it not needed but has been left here
 * for historical reasons.  It is used currently for the same reason.
 *
 * I don't think it's obsolete in ROM -- Hugin.
 */
void save_door_resets(FILE* fp, AreaData* area)
{
    int i;
    RoomData* pRoomIndex;
    RoomExitData* room_exit;

    FOR_EACH_GLOBAL_ROOM(pRoomIndex) {
        if (pRoomIndex->area_data == area) {
            for (i = 0; i < DIR_MAX; i++) {
                if ((room_exit = pRoomIndex->exit_data[i]) == NULL)
                    continue;

                if (room_exit->to_room
                    && (IS_SET(room_exit->exit_reset_flags, EX_CLOSED)
                        || IS_SET(room_exit->exit_reset_flags, EX_LOCKED))) {
#if defined( VERBOSE )
                    fprintf(fp, "D 0 %"PRVNUM" %d %d The %s door of %s is %s\n",
                        VNUM_FIELD(pRoomIndex),
                        room_exit->orig_dir,
                        IS_SET(room_exit->exit_reset_flags, EX_LOCKED) ? 2 : 1,
                        dir_list[room_exit->orig_dir].name,
                        pRoomIndex->name,
                        IS_SET(room_exit->exit_reset_flags, EX_LOCKED) ? "closed and locked"
                        : "closed");
#else
                    fprintf(fp, "D 0 %"PRVNUM" %d %d\n",
                        VNUM_FIELD(pRoomIndex),
                        room_exit->orig_dir,
                        IS_SET(room_exit->exit_reset_flags, EX_LOCKED) ? 2 : 1);

#endif
                }
            }
        }
    }

    return;
}

////////////////////////////////////////////////////////////////////////////////
// Name: save_resets
// Purpose:	Saves the #RESETS section of an area file.
// Called by: save_area(olc_save.c)
////////////////////////////////////////////////////////////////////////////////
void save_resets(FILE* fp, AreaData* area)
{
    Reset* reset;
    MobPrototype* pLastMob = NULL;
#ifdef VERBOSE
    ObjPrototype* pLastObj;
#endif
    RoomData* pRoom;
    char buf[MAX_STRING_LENGTH];

    fprintf(fp, "#RESETS\n");

    save_door_resets(fp, area);

    FOR_EACH_GLOBAL_ROOM(pRoom) {
        if (pRoom->area_data == area) {
            FOR_EACH(reset, pRoom->reset_first) {
                switch (reset->command) {
                default:
                    bug("Save_resets: bad command %c.", reset->command);
                    break;

#ifdef VERBOSE
                case 'M':
                    pLastMob = get_mob_prototype(reset->arg1);
                    fprintf(fp, "M 0 %d %d %d %d Load %s\n",
                        reset->arg1,
                        reset->arg2,
                        reset->arg3,
                        reset->arg4,
                        pLastMob->short_descr);
                    break;

                case 'O':
                    pLastObj = get_object_prototype(reset->arg1);
                    pRoom = get_room(reset->arg3);
                    fprintf(fp, "O 0 %d 0 %d %s loaded to %s\n",
                        reset->arg1,
                        reset->arg3,
                        capitalize(pLastObj->short_descr),
                        pRoom->name);
                    break;

                case 'P':
                    pLastObj = get_object_prototype(reset->arg1);
                    fprintf(fp, "P 0 %d %d %d %d %s put inside %s\n",
                        reset->arg1,
                        reset->arg2,
                        reset->arg3,
                        reset->arg4,
                        capitalize(get_object_prototype(reset->arg1)->short_descr),
                        pLastObj->short_descr);
                    break;

                case 'G':
                    fprintf(fp, "G 0 %d 0 %s is given to %s\n",
                        reset->arg1,
                        capitalize(get_object_prototype(reset->arg1)->short_descr),
                        pLastMob ? pLastMob->short_descr : "!NO_MOB!");
                    if (!pLastMob) {
                        sprintf(buf, "Save_resets: !NO_MOB! in [%s]", area->file_name);
                        bug(buf, 0);
                    }
                    break;

                case 'E':
                    fprintf(fp, "E 0 %d 0 %d %s is loaded %s of %s\n",
                        reset->arg1,
                        reset->arg3,
                        capitalize(get_object_prototype(reset->arg1)->short_descr),
                        flag_string(wear_loc_strings, reset->arg3),
                        pLastMob ? pLastMob->short_descr : "!NO_MOB!");
                    if (!pLastMob) {
                        sprintf(buf, "Save_resets: !NO_MOB! in [%s]", area->file_name);
                        bug(buf, 0);
                    }
                    break;

                case 'D':
                    break;

                case 'R':
                    pRoom = get_room(reset->arg1);
                    fprintf(fp, "R 0 %d %d Randomize %s\n",
                        reset->arg1,
                        reset->arg2,
                        pRoom->name);
                    break;
                }
#else
        case 'M':
            pLastMob = get_mob_prototype(reset->arg1);
            fprintf(fp, "M 0 %d %d %d %d\n",
                reset->arg1,
                reset->arg2,
                reset->arg3,
                reset->arg4);
            break;

        case 'O':
#ifdef VERBOSE
            pLastObj = get_object_prototype(reset->arg1);
#endif
            pRoom = get_room_data(reset->arg3);
            fprintf(fp, "O 0 %d 0 %d\n",
                reset->arg1,
                reset->arg3);
            break;

        case 'P':
#ifdef VERBOSE
            pLastObj = get_object_prototype(reset->arg1);
#endif
            fprintf(fp, "P 0 %d %d %d %d\n",
                reset->arg1,
                reset->arg2,
                reset->arg3,
                reset->arg4);
            break;

        case 'G':
            fprintf(fp, "G 0 %d 0\n", reset->arg1);
            if (!pLastMob) {
                sprintf(buf,
                    "Save_resets: !NO_MOB! in [%s] reset G arg1=%d arg2=%d arg3=%d arg4=%d",
                    area->file_name, reset->arg1, reset->arg2, reset->arg3, reset->arg4);
                bug(buf, 0);
            }
            break;

        case 'E':
            fprintf(fp, "E 0 %d 0 %d\n",
                reset->arg1,
                reset->arg3);
            if (!pLastMob) {
                sprintf(buf,
                    "Save_resets: !NO_MOB! in [%s] reset E arg1=%d arg2=%d arg3=%d arg4=%d",
                    area->file_name, reset->arg1, reset->arg2, reset->arg3, reset->arg4);
                bug(buf, 0);
            }
            break;

        case 'D':
            break;

        case 'R':
            pRoom = get_room_data(reset->arg1);
            fprintf(fp, "R 0 %d %d\n",
                reset->arg1,
                reset->arg2);
            break;
                }
#endif
            }
        }	/* End if correct area */
    }	/* End for pRoom */

    fprintf(fp, "S\n\n\n\n");
    return;
}



/*****************************************************************************
 Name:		save_shops
 Purpose:	Saves the #SHOPS section of an area file.
 Called by:	save_area(olc_save.c)
 ****************************************************************************/
void save_shops(FILE* fp, AreaData* area)
{
    ShopData* pShopIndex;
    MobPrototype* p_mob_proto;
    int iTrade;

    fprintf(fp, "#SHOPS\n");

    FOR_EACH_MOB_PROTO(p_mob_proto) {
        if (p_mob_proto && p_mob_proto->area == area && p_mob_proto->pShop) {
            pShopIndex = p_mob_proto->pShop;

            fprintf(fp, "%d ", pShopIndex->keeper);
            for (iTrade = 0; iTrade < MAX_TRADE; iTrade++) {
                if (pShopIndex->buy_type[iTrade] != 0) {
                    fprintf(fp, "%d ", pShopIndex->buy_type[iTrade]);
                }
                else
                    fprintf(fp, "0 ");
            }
            fprintf(fp, "%d %d ", pShopIndex->profit_buy, pShopIndex->profit_sell);
            fprintf(fp, "%d %d\n", pShopIndex->open_hour, pShopIndex->close_hour);
        }
    }

    fprintf(fp, "0\n\n\n\n");
    return;
}

void save_story_beats(FILE* fp, AreaData* area)
{
    if (area->story_beats == NULL)
        return;

    fprintf(fp, "#STORYBEATS\n");
    for (StoryBeat* beat = area->story_beats; beat != NULL; beat = beat->next) {
        fprintf(fp, "B\n%s~\n%s~\n",
            fix_string(beat->title),
            fix_string(beat->description));
    }
    fprintf(fp, "S\n\n\n\n");
}

void save_checklist(FILE* fp, AreaData* area)
{
    if (area->checklist == NULL)
        return;

    fprintf(fp, "#CHECKLIST\n");
    for (ChecklistItem* item = area->checklist; item != NULL; item = item->next) {
        fprintf(fp, "C %d\n%s~\n",
            (int)item->status,
            fix_string(item->title));
        // Description optional; keep empty line for backward compatibility.
        fprintf(fp, "%s~\n", IS_NULLSTR(item->description) ? "" : fix_string(item->description));
    }
    fprintf(fp, "S\n\n\n\n");
}

void save_area_daycycle(FILE* fp, AreaData* area)
{
    bool has_flag = area->suppress_daycycle_messages;
    bool has_periods = area->periods != NULL;
    if (!has_flag && !has_periods)
        return;

    fprintf(fp, "#DAYCYCLE\n");
    if (area->suppress_daycycle_messages)
        fprintf(fp, "W 1\n");

    for (DayCyclePeriod* period = area->periods; period != NULL; period = period->next) {
        const char* name = (period->name && period->name[0] != '\0') ? period->name : "period";
        fprintf(fp, "P %s %d %d\n",
            name,
            period->start_hour,
            period->end_hour);
        fprintf(fp, "%s~\n", period->description ? fix_string(period->description) : "");
        if (period->enter_message && period->enter_message[0] != '\0') {
            fprintf(fp, "B %s\n", name);
            fprintf(fp, "%s~\n", fix_string(period->enter_message));
        }
        if (period->exit_message && period->exit_message[0] != '\0') {
            fprintf(fp, "A %s\n", name);
            fprintf(fp, "%s~\n", fix_string(period->exit_message));
        }
    }

    fprintf(fp, "S\n\n\n\n");
}

void save_area_loot(FILE* fp, AreaData* area)
{
    save_loot_section(fp, &area->header);
}

void save_helps(FILE* fp, HelpArea* ha)
{
    HelpData* help = ha->first;

    fprintf(fp, "#HELPS\n");

    for (; help; help = help->next_area) {
        fprintf(fp, "%d %s~\n", help->level, help->keyword);
        fprintf(fp, "%s~\n\n", fix_string(help->text));
    }

    fprintf(fp, "-1 $~\n\n");

    ha->changed = false;

    return;
}

void save_other_helps(Mobile* ch)
{
    HelpArea* ha;
    FILE* fp;
    char buf[MIL];

    FOR_EACH(ha, help_area_list)
        if (ha->changed == true
            && ha->area_data == NULL) {
            sprintf(buf, "%s%s", cfg_get_area_dir(), ha->filename);

            OPEN_OR_CONTINUE(fp = open_read_file(buf));

            save_helps(fp, ha);

            if (ch)
                printf_to_char(ch, "%s\n\r", ha->filename);

            fprintf(fp, "#$\n");
            
            close_file(fp);
        }

    return;
}

/*****************************************************************************
 Name:		save_area
 Purpose:	Save an area, note that this format is new.
 Called by:	do_asave(olc_save.c).
 ****************************************************************************/
void save_area(AreaData* area)
{
    FILE* fp;
    char tmp[MIL];
    char area_file[MIL];

    sprintf(tmp, "%s%s.tmp", cfg_get_area_dir(), area->file_name);

    sprintf(area_file, "%s%s", cfg_get_area_dir(), area->file_name);

    OPEN_OR_RETURN(fp = open_write_file(tmp));

    PersistWriter writer = persist_writer_from_file(fp, tmp);
    const AreaPersistFormat* fmt = area_persist_select_format(area->file_name);
    AreaPersistSaveParams params = {
        .writer = &writer,
        .area = area,
        .file_name = area->file_name,
    };

    PersistResult result = fmt->save(&params);

    close_file(fp);

    if (!persist_succeeded(result)) {
        bugf("save_area : persist save failed for %s (%s)", area->file_name, result.message ? result.message : "unknown error");
        return;
    }

#ifdef _MSC_VER
    if (!MoveFileExA(tmp, area_file, MOVEFILE_REPLACE_EXISTING)) {
        bugf("save_area : Could not rename %s to %s!", area_file, tmp);
        perror(area_file);
    }
#else
    if (rename(tmp, area_file) != 0) {
        bugf("save_area : Could not rename %s to %s!", tmp, area_file);
        perror(area_file);
    }
#endif

}

/*****************************************************************************
 Name:		do_asave
 Purpose:	Entry point for saving area data.
 Called by:	interpreter(interp.c)
 ****************************************************************************/
void do_asave(Mobile* ch, char* argument)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    AreaData* area;
    VNUM value;
    const char* requested_ext = NULL;
    bool force_format = false;

    if (ch == NULL || ch->desc == NULL || IS_NPC(ch)) {
        save_lox_public_scripts_if_dirty();
        return;
    }

    if (!ch)       /* Do an autosave */
    {
        save_area_list();

        FOR_EACH_AREA(area)
            if (str_cmp(argument, "changed")
                || IS_SET(area->area_flags, AREA_CHANGED)) {
                save_area(area);
                REMOVE_BIT(area->area_flags, AREA_CHANGED);
            }

        save_progs(0, MAX_VNUM); // Just in case

        save_other_helps(NULL);

        save_lox_public_scripts_if_dirty();
        return;
    }

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (!str_cmp(arg2, "json")) {
        requested_ext = ".json";
        force_format = true;
    }
    else if (!str_cmp(arg2, "olc")) {
        requested_ext = ".are";
        force_format = true;
    }

    if (arg1[0] == '\0') {
        send_to_char("Syntax:\n\r", ch);
        send_to_char("  asave <vnum> [json|olc]  - saves a particular area\n\r", ch);
        send_to_char("  asave list               - saves the area.lst file\n\r", ch);
        send_to_char("  asave area [json|olc]    - saves the area being edited\n\r", ch);
        send_to_char("  asave changed [json|olc] - saves all changed zones\n\r", ch);
        send_to_char("  asave world [json|olc]   - saves the world! (db dump)\n\r", ch);
        send_to_char("\n\r", ch);
        save_lox_public_scripts_if_dirty();
        return;
    }

    /* Snarf the value (which need not be numeric). */
    value = STRTOVNUM(arg1);
    if ((area = get_area_data(value)) == NULL && is_number(arg1)) {
        send_to_char("That area does not exist.\n\r", ch);
        save_lox_public_scripts_if_dirty();
        return;
    }
    /* Save area of given vnum. */
    /* ------------------------ */

    if (is_number(arg1) && area) {
        if (ch && !IS_BUILDER(ch, area)) {
            send_to_char("You are not a builder for this area.\n\r", ch);
            save_lox_public_scripts_if_dirty();
            return;
        }

        if (force_format && requested_ext) {
            const char* ext = strrchr(area->file_name, '.');
            char newname[MIL];
            if (ext)
                sprintf(newname, "%.*s%s", (int)(ext - area->file_name), area->file_name, requested_ext);
            else
                sprintf(newname, "%s%s", area->file_name, requested_ext);
            free_string(area->file_name);
            area->file_name = str_dup(newname);
        }

        save_area_list();
        save_area(area);
        save_lox_public_scripts_if_dirty();
        return;
    }

    /* Save the world, only authorized areas. */
    /* -------------------------------------- */
    if (!str_cmp("world", arg1)) {
        save_area_list();
        FOR_EACH_AREA(area) {
            /* Builder must be assigned this area. */
            if (!IS_BUILDER(ch, area))
                continue;

            if (force_format && requested_ext) {
                const char* ext = strrchr(area->file_name, '.');
                char newname[MIL];
                if (ext)
                    sprintf(newname, "%.*s%s", (int)(ext - area->file_name), area->file_name, requested_ext);
                else
                    sprintf(newname, "%s%s", area->file_name, requested_ext);
                free_string(area->file_name);
                area->file_name = str_dup(newname);
            }

            save_area(area);
            REMOVE_BIT(area->area_flags, AREA_CHANGED);
            REMOVE_BIT(area->area_flags, AREA_ADDED);
        }

        save_other_helps(ch);

        send_to_char("You saved the world.\n\r", ch);
        save_lox_public_scripts_if_dirty();
        return;
    }

    /* Save changed areas, only authorized areas. */
    /* ------------------------------------------ */

    if (!str_cmp("changed", arg1)) {
        char buf[MAX_INPUT_LENGTH];

        save_area_list();

        send_to_char("Saved zones:\n\r", ch);
        sprintf(buf, "None.\n\r");

        FOR_EACH_AREA(area) {
            /* Builder must be assigned this area. */
            if (!IS_BUILDER(ch, area))
                continue;

                /* Save changed areas. */
            if (IS_SET(area->area_flags, AREA_CHANGED)) {
                if (force_format && requested_ext) {
                    const char* ext = strrchr(area->file_name, '.');
                    char newname[MIL];
                    if (ext)
                        sprintf(newname, "%.*s%s", (int)(ext - area->file_name), area->file_name, requested_ext);
                    else
                        sprintf(newname, "%s%s", area->file_name, requested_ext);
                    free_string(area->file_name);
                    area->file_name = str_dup(newname);
                }

                save_area(area);
                sprintf(buf, "%24s - '%s'\n\r", NAME_STR(area), area->file_name);
                send_to_char(buf, ch);
                REMOVE_BIT(area->area_flags, AREA_CHANGED);
            }
        }

        save_other_helps(ch);
        save_progs(0, MAX_VNUM);

        if (!str_cmp(buf, "None.\n\r"))
            send_to_char(buf, ch);
        save_lox_public_scripts_if_dirty();
        return;
    }

    /* Save the area.lst file. */
    /* ----------------------- */
    if (!str_cmp(arg1, "list")) {
        save_area_list();
        save_lox_public_scripts_if_dirty();
        return;
    }

    /* Save area being edited, if authorized. */
    /* -------------------------------------- */
    if (!str_cmp(arg1, "area")) {
    /* Is character currently editing. */
        if (get_editor(ch->desc) == ED_NONE) {
            send_to_char("You are not editing an area, "
                "therefore an area vnum is required.\n\r", ch);
            save_lox_public_scripts_if_dirty();
            return;
        }

        /* Find the area to save. */
        switch (get_editor(ch->desc)) {
        case ED_AREA:
            area = (AreaData*)get_pEdit(ch->desc);
            break;
        case ED_ROOM:
            area = ch->in_room->area->data;
            break;
        case ED_OBJECT:
            area = ((ObjPrototype*)get_pEdit(ch->desc))->area;
            break;
        case ED_MOBILE:
            area = ((MobPrototype*)get_pEdit(ch->desc))->area;
            break;
        default:
            area = ch->in_room->area->data;
            break;
        }

        if (!IS_BUILDER(ch, area)) {
            send_to_char("You are not a builder for this area.\n\r", ch);
            save_lox_public_scripts_if_dirty();
            return;
        }

        if (force_format && requested_ext) {
            const char* ext = strrchr(area->file_name, '.');
            char newname[MIL];
            if (ext)
                sprintf(newname, "%.*s%s", (int)(ext - area->file_name), area->file_name, requested_ext);
            else
                sprintf(newname, "%s%s", area->file_name, requested_ext);
            free_string(area->file_name);
            area->file_name = str_dup(newname);
        }

        save_area_list();
        save_area(area);
        REMOVE_BIT(area->area_flags, AREA_CHANGED);
        send_to_char("Area saved.\n\r", ch);
        save_lox_public_scripts_if_dirty();
        return;
    }

    /* Show correct syntax. */
    /* -------------------- */
    do_asave(ch, "");
    save_lox_public_scripts_if_dirty();
    return;
}
