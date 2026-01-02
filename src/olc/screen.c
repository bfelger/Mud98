////////////////////////////////////////////////////////////////////////////////
// screen.c
////////////////////////////////////////////////////////////////////////////////

#include "merc.h"

#include "act_move.h"
#include "bit.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "mob_cmds.h"
#include "olc.h"
#include "recycle.h"
#include "screen.h"
#include "special.h"
#include "tables.h"
#include "vt.h"

#include "entities/mobile.h"
#include "entities/descriptor.h"
#include "entities/object.h"

#include "data/mobile_data.h"
#include "data/race.h"

char* areaname(void* point)
{
    AreaData* area = *(AreaData**)point;

    return NAME_STR(area);
}

char* clan2str(void* point)
{
    int16_t clan = *(int16_t*)point;

    return clan_table[clan].name;
}

char* wealth2str(void* point)
{
    static char buf[64];
    int wealth = *(int*)point;
    int16_t gold = 0, silver = 0, copper = 0;
    convert_copper_to_money(wealth, &gold, &silver, &copper);
    format_money_string(buf, sizeof(buf), gold, silver, copper, true);
    return buf;
}

char* extradescr2str(void* point)
{
    static	char buf[MIL];
    ExtraDesc* ed = *(ExtraDesc**)point;

    buf[0] = '\0';
    for (; ed; NEXT_LINK(ed)) {
        strcat(buf, ed->keyword);
        if (ed->next)
            strcat(buf, " ");
    }
    buf[67] = '\0'; // para lineas de 80 carac
    return buf;
}

char* progs(void* point)
{
    static	char buf[MSL];
    char tmpbuf[MIL];
    int cnt;
    MobProg* list = *(MobProg**)point;

    buf[0] = '\0';
    strcat(buf, "Progs:\n\r");

    for (cnt = 0; list; NEXT_LINK(list)) {
        if (cnt == 0)
            strcat(buf, "#Num  Vnum  Trigger Phrase     " COLOR_EOL);

        sprintf(tmpbuf, "%3d %5d %7.7s %s\n\r", cnt,
            list->vnum, event_trigger_name(list->trig_type),
            list->trig_phrase);
        strcat(buf, tmpbuf);
        cnt++;
    }

    return buf;
}

char* sex2str(void* point)
{
    int16_t sex = *(int16_t*)point;
    static	char tmpch[2];

    if (sex == SEX_FEMALE)
        tmpch[0] = 'F';
    else
        if (sex == SEX_MALE)
            tmpch[0] = 'M';
        else
            tmpch[0] = 'O';

    return tmpch;
}

char* damtype2str(void* point)
{
    int16_t dtype = *(int16_t*)point;

    return attack_table[dtype].name;
}

const char* size2str(void* point)
{
    MobSize siz = *(MobSize*)point;

    return mob_size_table[siz].name;
}

char* ac2str(void* point)
{
    int16_t* ac = (int16_t*)point;
    static	char buf[40];

    sprintf(buf, "Pi:%d Ba:%d Sl:%d Ma:%d",
        ac[AC_PIERCE], ac[AC_BASH],
        ac[AC_SLASH], ac[AC_EXOTIC]);

    return buf;
}

char* dice2str(void* point)
{
    int16_t* dado = (int16_t*)point;
    static	char buf[30];

    sprintf(buf, "%dd%d+%d", dado[DICE_NUMBER], dado[DICE_TYPE], dado[DICE_BONUS]);

    return buf;
}

char* race2str(void* point)
{
    int16_t race = *(int16_t*)point;

    return race_table[race].name;
}

const char* pos2str(void* point)
{
    Position posic = *(Position*)point;

    return position_table[posic].short_name;
}

char* exits2str(void* point)
{
    RoomExitData** pexitarray = (RoomExitData**)point;
    RoomExitData* room_exit;
    static char buf[MSL];
    char word[MIL], reset_state[MIL], tmpbuf[MIL];
    char* state;
    size_t length;

    buf[0] = '\0';

    for (int j = 0; j < DIR_MAX; j++) {
        if ((room_exit = pexitarray[j]) == NULL)
            continue;

        sprintf(tmpbuf, "-%-5.5s to [%5d] ",
            capitalize(dir_list[j].name),
            room_exit->to_room ? VNUM_FIELD(room_exit->to_room) : 0);
        strcat(buf, tmpbuf);

        if (room_exit->key > 0) {
            sprintf(tmpbuf, "Key:%d ", room_exit->key);
            strcat(buf, tmpbuf);
        }

        /*
         * Format up the exit info.
         * Capitalize all flags that are not part of the reset info.
         */
        strcpy(reset_state, flag_string(exit_flag_table, room_exit->exit_reset_flags));
        //state = flag_string(exit_flag_table, room_exit->);
        state = reset_state;
        //strcat(buf, "Flags: [");
        strcat(buf, "Flags: [");
        for (; ;) {
            state = one_argument(state, word);

            if (word[0] == '\0') {
                size_t end;

                end = strlen(buf) - 1;
                buf[end] = ']';
                if (room_exit->keyword)
                    strcat(buf, "(K)");
                if (room_exit->description)
                    strcat(buf, "(D)");
                strcat(buf, "\n\r");
                break;
            }

            //if (str_infix(word, reset_state)) {
                length = strlen(word);
                for (size_t i = 0; i < length; i++)
                    word[i] = UPPER(word[i]);
            //}

            strcat(buf, word);
            strcat(buf, " ");
        }

	    if (room_exit->keyword && room_exit->keyword[0] != '\0') {
            sprintf(tmpbuf, "Kwds: [%s]\n\r", room_exit->keyword);
            strcat(buf, tmpbuf);
        }
        if (room_exit->description && room_exit->description[0] != '\0') {
            sprintf(tmpbuf, "%s", room_exit->description);
            strcat(buf, tmpbuf);
        } 
    }

    return buf;
}

char* spec2str(void* point)
{
    SpecFunc* spec = *(SpecFunc**)point;

    return spec_name(spec);
}

char* shop2str(void* point)
{
    ShopData* pShop = *(ShopData**)point;
    int iTrade;
    static	char buf[MSL];
    char tmpbuf[MIL];

    sprintf(buf,
        "Markup when buying: %d%%\n\r"
        "Markup when selling: %d%%\n\r"
        "Opens: %d   Closes: %d\n\r",
        pShop->profit_buy, pShop->profit_sell,
        pShop->open_hour, pShop->close_hour);

    for (iTrade = 0; iTrade < MAX_TRADE; iTrade++) {
        if (pShop->buy_type[iTrade] != 0) {
            if (iTrade == 0)
                strcat(buf, "N Type\n\r");
            sprintf(tmpbuf, "%d %s\n\r", iTrade,
                flag_string(type_flag_table, pShop->buy_type[iTrade]));
            strcat(buf, tmpbuf);
        }
    }

    return buf;
}

#ifdef U
#define OLD_U U
#endif
#define U(x)    (uintptr_t)(x)

const struct olc_show_table_type redit_olc_show_table[] = {
    {
        "name",	U(&xRoom.header.name), "Name:", OLCS_LOX_STRING,
        1, 1, 40, 1, 1, 0
    },
    {
        "vnum",	U(&xRoom.header.vnum), "Vnum:", OLCS_VNUM,
        49, 1, 5, 1, 1, 0
    },
    {
        "area", U(&xRoom.area_data), "Area:", OLCS_STRFUNC,
        60, 1, 15, 1, 1, U(areaname)
    },
    {
        "heal", U(&xRoom.heal_rate), "Heal:", OLCS_INT16,
        1, 2, 3, 1, 1, 0
    },
    {
        "mana", U(&xRoom.mana_rate), "Mana:", OLCS_INT16,
        10, 2, 3, 1, 1, 0
    },
    {
        "clan", U(&xRoom.clan), "Clan:", OLCS_STRFUNC,
        19, 2, 10, 1, 1, U(clan2str)
    },
    {
        "sector", U(&xRoom.sector_type), "Sector:", OLCS_FLAGSTR_INT16,
        35, 2, 10, 1, 1, U(sector_flag_table)
    },
    {
        "roomflags", U(&xRoom.room_flags), "Flags:", OLCS_FLAGSTR_INT,
        1, 3, 73, 1, 1, U(room_flag_table)
    },
    {
        "extradesc", U(&xRoom.extra_desc), "Extra desc:", OLCS_STRFUNC,
        1, 4, 67, 1, 1, U(extradescr2str)
    },
    {
        "desctag", 0, "Description:", OLCS_TAG,
        1, 5, -1, -1, 1, 0
    },
    {
        "desc", U(&xRoom.description), "", OLCS_STRING,
        1, 6, -1, 6, 1, 0
    },
    {
        "exitstag", 0, "Exits:", OLCS_TAG,
        1, 12, -1, -1, 1, 0
    },
    {
        "exits", U(&xRoom.exit_data), "", OLCS_STRFUNC,
        1, 13, -1, -1, 1, U(exits2str)
    },
    // page 2
    {
        "owner", U(&xRoom.owner), "Owner:", OLCS_STRING,
        1, 1, 10, 1, 2, 0
    },
    {
        NULL, 0, NULL, 0, 0, 0, 0, 0, 0, 0
    }
};

const struct olc_show_table_type medit_olc_show_table[] =
{
    {
        "name", U(&xMob.header.name), "Name:", OLCS_LOX_STRING,
        1, 1, 31, 1, 1, 0
    },
    {
        "area", U(&xMob.area), "Area:", OLCS_STRFUNC,
        40, 1, 12, 1, 1, U(areaname)
    },
    {
        "vnum",	U(&xMob.header.vnum), "Vnum:", OLCS_VNUM,
        70, 1, 5, 1, 1, 0
    },
    {
        "level", U(&xMob.level), "Level:", OLCS_INT16,
        1, 2, 2, 1, 1, 0
    },
    {
        "align", U(&xMob.alignment), "Align:", OLCS_INT16,
        11, 2, 5, 1, 1, 0
    },
    {
        "damtype", U(&xMob.dam_type), "Damtype:", OLCS_STRFUNC,
        37, 2, 12, 1, 1, U(damtype2str)
    },
    {
        "sex", U(&xMob.sex), "Sex:", OLCS_STRFUNC,
        58, 2, 1, 1, 1, U(sex2str)
    },
    {
        "group", U(&xMob.group), "Group:", OLCS_INT16,
        70, 2, 5, 1, 1, 0
    },
    {
        "race", U(&xMob.race), "Race:", OLCS_STRFUNC,
        1, 3, 10, 1, 1, U(race2str)
    },
    {
        "size", U(&xMob.size), "Size:", OLCS_STRFUNC,
        17, 3, 5, 1, 1, U(size2str)
    },
    {
        "material", U(&xMob.material), "Mat:", OLCS_STRING,
        27, 3, 10, 1, 1, 0
    },
    {
        "wealth", U(&xMob.wealth), "$:", OLCS_STRFUNC,
        43, 3, 12, 1, 1, U(wealth2str)
    },
    {
        "start_pos", U(&xMob.start_pos), "Pos start:", OLCS_STRFUNC,
        58, 3, 4, 1, 1, U(pos2str)
    },
    {
        "default_pos", U(&xMob.default_pos), "def:", OLCS_STRFUNC,
        72, 3, 4, 1, 1, U(pos2str)
    },
    {
        "hit", U(&xMob.hit), "HitD:", OLCS_STRFUNC,
        1, 4, 12, 1, 1, U(dice2str)
    },
    {
        "damage", U(&xMob.damage), "DamD:", OLCS_STRFUNC,
        22, 4, 12, 1, 1, U(dice2str)
    },
    {
        "mana", U(&xMob.mana), "ManaD:", OLCS_STRFUNC,
        39, 4, 12, 1, 1, U(dice2str)
    },
    {
        "hitroll", U(&xMob.hitroll), "Hitr:", OLCS_INT16,
        71, 4, 2, 1, 1, 0
    },
    {
        "spec_fun", U(&xMob.spec_fun), "Spec:", OLCS_STRFUNC,
        1, 5, 15, 1, 1, U(spec2str)
    },
    {
        "ac", U(&xMob.ac), "AC:", OLCS_STRFUNC,
        39, 5, 38, 1, 1, U(ac2str)
    },
    {
        "act", U(&xMob.act_flags), "Act :", OLCS_FLAGSTR_INT,
        1, 6, 74, 1, 1, U(act_flag_table)
    },
    {
        "aff", U(&xMob.affect_flags), "Aff :", OLCS_FLAGSTR_INT,
        1, 7, 74, 1, 1, U(affect_flag_table)
    },
    {
        "form", U(&xMob.form), "Form:", OLCS_FLAGSTR_INT,
        1, 9, 74, 1, 1, U(form_flag_table)
    },
    {
        "parts", U(&xMob.parts), "Part:", OLCS_FLAGSTR_INT,
        1, 10, 74, 1, 1, U(part_flag_table)
    },
    {
        "imm", U(&xMob.imm_flags), "Imm:", OLCS_FLAGSTR_INT,
        1, 11, 74, 1, 1, U(imm_flag_table)
    },
    {
        "res", U(&xMob.res_flags), "Res:", OLCS_FLAGSTR_INT,
        1, 12, 74, 1, 1, U(res_flag_table)
    },
    {
        "vuln", U(&xMob.vuln_flags), "Vuln:", OLCS_FLAGSTR_INT,
        1, 13, 74, 1, 1, U(vuln_flag_table)
    },
    {
        "off", U(&xMob.atk_flags), "Off:", OLCS_FLAGSTR_INT,
        1, 14, 74, 1, 1, U(off_flag_table)
    },
    {
        "short_descr", U(&xMob.short_descr), "ShDe:", OLCS_STRING,
        1, 15, 74, 1, 1, 0
    },
    {
        "long_descr", U(&xMob.long_descr), "LoDe:", OLCS_STRING,
        1, 16, 74, 1, 1, 0
    },
    // page 2
    {
        "shop", U(&xMob.pShop), "", OLCS_STRFUNC,
        1, 2, 25, 4 + MAX_TRADE, 2, U(shop2str)
    },
    {
        NULL, 0, NULL, 0,
        0, 0, 0, 0, 0, 0
    }
};

const struct olc_show_table_type oedit_olc_show_table[] =
{
    {
        "name", U(&xObj.header.name), "Name:", OLCS_LOX_STRING,
        1, 1, 32, 1, 1, 0
    },
    {
        "area", U(&xObj.area), "Area:", OLCS_STRFUNC,
        40, 1, 14, 1, 1, U(areaname)
    },
    {
        "vnum", U(&xObj.header.vnum), "Vnum:", OLCS_VNUM,
        68, 1, 5, 1, 1, 0
    },
    {
        "type", U(&xObj.item_type), "Type:", OLCS_FLAGSTR_INT16,
        1, 2, 11, 1, 1, U(type_flag_table)
    },
    {
        "level", U(&xObj.level), "Level:", OLCS_INT16,
        20, 2, 2, 1, 1, 0
    },
    {
        "condition", U(&xObj.condition), "Cond:", OLCS_INT16,
        29, 2, 3, 1, 1, 0
    },
    {
        "weight", U(&xObj.weight), "Weight:", OLCS_INT16,
        56, 2, 5, 1, 1, 0
    },
    {
        "cost",	U(&xObj.cost), "Cost:", OLCS_INT,
        68, 2, -1, 1, 1, 0
    },
    {
        "wear", U(&xObj.wear_flags), "Wear F:", OLCS_FLAGSTR_INT,
        1, 3, 72, 1, 1, U(wear_flag_table)
    },
    {
        "extra", U(&xObj.extra_flags), "Extra :", OLCS_FLAGSTR_INT,
        1, 4, 72, 1, 1, U(extra_flag_table)
    },
    {
        "short_descr", U(&xObj.short_descr), "ShDesc:", OLCS_STRING,
        1, 5, 72, 1, 1, 0
    },
    {
        "extra_desc", U(&xObj.extra_desc), "ExDesc:", OLCS_STRFUNC,
        1, 6, 72, 1, 1, U(extradescr2str)
    },
    {
        "description", U(&xObj.description), "Desc  :", OLCS_STRING,
        1, 7, 72, 6, 1, 0
    },
    {
        NULL, 0, NULL, 0,
        0, 0, 0, 0, 0, 0
    }
};

void InitScreen(Descriptor* d)
{
    char buf[MIL];
    int size;
    Mobile* ch = d->character;

    if (!IS_SET(ch->comm_flags, COMM_OLCX))
        return;

    size = IS_NPC(ch) ? PAGELEN : (ch->lines + 3);

    send_to_char(VT_HOMECLR, ch);

    if (get_editor(d) != ED_NONE && IS_SET(ch->comm_flags, COMM_OLCX)) {
        InitScreenMap(d);
        sprintf(buf, VT_MARGSET, size - 4, size);
        send_to_char(buf, ch);
        sprintf(buf, VT_CURSPOS "OK!\n\r", size - 4, 0);
        send_to_char(buf, ch);
    }
}

void InitScreenMap(Descriptor* d)
{
    if (d->screenmap == NULL)
        d->screenmap = calloc(80 * ((size_t)(d->character->lines) - 3) + 1, sizeof(char));
    if (d->oldscreenmap == NULL)
        d->oldscreenmap = calloc(80 * ((size_t)(d->character->lines) - 3) + 1, sizeof(char));
    if (d->oldscreenmap == NULL || d->screenmap == NULL) {
        perror("InitScreenMap(): calloc failed!");
        exit(-1);
    }
    for (size_t i = 0; i < 80 * (size_t)(d->character->lines - 3); i++)
        d->screenmap[i] = d->oldscreenmap[i] = ' ';
}

void UpdateOLCScreen(Descriptor* d)
{
    INIT_BUF(buf, MSL * 2);
    INIT_BUF(buf2, MSL * 2);
    void* point;
    const struct olc_show_table_type* table;
    int16_t x, y;
    size_t i;
    size_t j;
    uintptr_t blah;
    size_t size;
    STRFUNC* func;
    char* tmpstr;
    const struct flag_type* flagt;

    if (d->screenmap == NULL || d->oldscreenmap == NULL)
        InitScreenMap(d);

    switch (get_editor(d)) {
    default:	return;
    case ED_ROOM:	
        table = redit_olc_show_table;
        blah = U(&xRoom);
        break;
    case ED_MOBILE:	
        table = medit_olc_show_table;
        blah = U(&xMob);
        break;
    case ED_OBJECT:	
        table = oedit_olc_show_table;
        blah = U(&xObj);
        break;
    }

    write_to_buffer(d, VT_CURSAVE, 0);

    tmpstr = d->oldscreenmap;
    d->oldscreenmap = d->screenmap;
    d->screenmap = tmpstr;

    for (i = 0; table[i].name != NULL; i++) {
        point = (void*)(table[i].point - blah + get_pEdit(d));

        if (table[i].page != d->page)
            continue;

        switch (table[i].type) {
        default:
            break;
        case OLCS_STRING:
            SET_BUF(buf, *(char**)point);
            break;

        case OLCS_INT:
            SET_BUF(buf, itos(*(int*)point));
            break;

        case OLCS_INT16:
            SET_BUF(buf, itos(*(int16_t*)point));
            break;

        case OLCS_VNUM: {
            char vnum_buf[50]; // Overkill
            sprintf(vnum_buf, "%" PRVNUM, *(VNUM*)point);
            SET_BUF(buf, vnum_buf);
            break;
        }
        case OLCS_LOX_STRING: {
            String* str = *(String**)point;
            SET_BUF(buf, str->chars);
            break;
        }
        case OLCS_STRFUNC:
            func = (STRFUNC*)table[i].func;
            tmpstr = (*func) (point);
            SET_BUF(buf, tmpstr ? tmpstr : "");
            break;

        case OLCS_FLAGSTR_INT:
            flagt = (const struct flag_type*)table[i].func;
            SET_BUF(buf, flag_string(flagt, *(int*)point));
            break;

        case OLCS_FLAGSTR_INT16:
            flagt = (const struct flag_type*)table[i].func;
            SET_BUF(buf, flag_string(flagt, *(int16_t*)point));
            break;

        case OLCS_BOOL:
            SET_BUF(buf, *(bool*)point == true ? "S" : "N");
            break;

        case OLCS_TAG:
            BUF(buf)[0] = '\0';
            break;
        }

        SET_BUF(buf2, table[i].desc);
        add_buf(buf2, BUF(buf));
        size = strlen(BUF(buf2));
        x = table[i].x;
        y = table[i].y;
        for (j = 0; j < size; j++) {
            if (BUF(buf2)[j] == '\r') {
                x = table[i].x;
                continue;
            }
            if (BUF(buf2)[j] == '\n') {
                y++;
                continue;
            }
            if ((table[i].sizex < 1 && x > 79)
                || (table[i].sizex > 0 && x >= table[i].x + table[i].sizex + (int)strlen(table[i].desc))) {
                y++;
                x = table[i].x;
            }
            if ((table[i].sizey < 1 && y > d->character->lines - 3)
                || (table[i].sizey > 0 && y >= table[i].y + table[i].sizey))
                break;
            if (((y - 1) * 80 + (x - 1)) >= 80 * (d->character->lines - 3))
                break;
            d->screenmap[(y - 1) * 80 + (x++ - 1)] = BUF(buf2)[j];
        }
    }

    // Only send the differences
    size = strlen(d->screenmap);
    i = j = 0;
    BUF(buf)[0] = '\0';
    while (i < size) {
        if (d->screenmap[i] == d->oldscreenmap[i]) {
            i++;
            continue;
        }
        sprintf(BUF(buf2), VT_CURSPOS "%c", (int)(i / 80 + 1), (int)(i % 80), d->screenmap[i]);
        ++i;
        strcat(BUF(buf), BUF(buf2));
        j += strlen(BUF(buf2));
        while (i < size && d->screenmap && d->screenmap[i] != d->oldscreenmap[i])
            BUF(buf)[j++] = d->screenmap[i++];
        BUF(buf)[j] = '\0';
    }

    write_to_buffer(d, BUF(buf), j);
    write_to_buffer(d, VT_CURREST, 0);

    free_buf(buf);
    free_buf(buf2);
}

#undef U
#ifdef OLD_U
#define U OLD_U
#undef OLD_U
#endif
