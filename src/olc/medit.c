////////////////////////////////////////////////////////////////////////////////
// medit.c
////////////////////////////////////////////////////////////////////////////////

#include <merc.h>

#include "bit.h"
#include "lox_edit.h"
#include "olc.h"

#include <comm.h>
#include <db.h>
#include <handler.h>
#include <lookup.h>
#include <magic.h>
#include <mob_cmds.h>
#include <recycle.h>
#include <save.h>
#include <tables.h>

#include <entities/event.h>
#include <entities/mob_prototype.h>

#define MEDIT(fun) bool fun( Mobile *ch, char *argument )

MobPrototype xMob;

#ifdef U
#define OLD_U U
#endif
#define U(x)    (uintptr_t)(x)

const OlcCmdEntry mob_olc_comm_table[] = {
    { "name",	    U(&xMob.header.name),   ed_line_lox_string, 0		        },
    { "short",	    U(&xMob.short_descr),	ed_line_string,		0		        },
    { "long",	    U(&xMob.long_descr),	ed_line_string,		0	            },
    { "material",	U(&xMob.material),	    ed_line_string,		0		        },
    { "desc",	    U(&xMob.description),	ed_desc,		    0		        },
    { "level",	    U(&xMob.level),		    ed_number_level,    0		        },
    { "align",	    U(&xMob.alignment),	    ed_number_align,	0		        },
    { "group",	    U(&xMob.group),		    ed_olded,		    U(medit_group)	},
    { "imm",	    U(&xMob.imm_flags),	    ed_flag_toggle,		U(imm_flag_table)	},
    { "res",	    U(&xMob.res_flags),	    ed_flag_toggle,		U(res_flag_table)	},
    { "vuln",	    U(&xMob.vuln_flags),	ed_flag_toggle,		U(vuln_flag_table)	},
    { "act",	    U(&xMob.act_flags),		ed_flag_toggle,		U(act_flag_table)	},
    { "affect",	    U(&xMob.affect_flags),	ed_flag_toggle,		U(affect_flag_table)},
    { "off",	    U(&xMob.atk_flags),	    ed_flag_toggle,		U(off_flag_table)	},
    { "form",	    U(&xMob.form),		    ed_flag_toggle,		U(form_flag_table)	},
    { "parts",	    U(&xMob.parts),		    ed_flag_toggle,		U(part_flag_table)	},
    { "shop",	    U(&xMob),			    ed_shop,		    0		        },
    { "create",	    0,				        ed_new_mob,		    0		        },
    { "spec",	    U(&xMob.spec_fun),	    ed_gamespec,		0		        },
    { "recval",	    U(&xMob),			    ed_recval,		    0		        },
    { "sex",	    U(&xMob.sex),		    ed_int16lookup,		U(sex_lookup)	},
    { "size",	    U(&xMob.size),		    ed_int16lookup,		U(size_lookup)	},
    { "startpos",	U(&xMob.start_pos),	    ed_int16lookup,		U(position_lookup)},
    { "defaultpos", U(&xMob.default_pos),	ed_int16lookup,		U(position_lookup)},
    { "damtype",	U(&xMob.dam_type),	    ed_int16poslookup,	U(attack_lookup)},
    { "race",	    U(&xMob),			    ed_race,		    0		        },
    { "armor",	    U(&xMob),			    ed_ac,			    0		        },
    { "hitdice",	U(&xMob.hit[0]),        ed_dice,		    0		        },
    { "manadice",	U(&xMob.mana[0]),		ed_dice,		    0		        },
    { "damdice",	U(&xMob.damage[0]),	    ed_dice,		    0		        },
    { "hitroll",	U(&xMob.hitroll),		ed_number_s_pos,	0		        },
    { "wealth",	    U(&xMob.wealth),		ed_number_l_pos,	0		        },
    { "addprog",	U(&xMob.mprogs),		ed_addprog,		    0		        },
    { "delprog",	U(&xMob.mprogs),		ed_delprog,		    0		        },
    { "mshow",	    0,				        ed_olded,		    U(medit_show)	},
    { "oshow",	    0,				        ed_olded,		    U(oedit_show)	},
    { "olist",	    U(&xMob.area),		    ed_olist,		    0		        },
    { "copy",	    0,				        ed_olded,		    U(medit_copy)	},
    { "commands",	0,				        ed_olded,		    U(show_commands)},
    { "?",		    0,				        ed_olded,		    U(show_help)	},
    { "version",	0,				        ed_olded,		    U(show_version)	},
    { NULL,	        0,				        NULL,			    0		        }
};

/* Entry point for editing mob_prototype_data. */
void do_medit(Mobile* ch, char* argument)
{
    MobPrototype* pMob;
    AreaData* area;
    int     value;
    char    arg1[MAX_STRING_LENGTH];

    if (IS_NPC(ch) || ch->desc == NULL)
        return;

    READ_ARG(arg1);

    if (is_number(arg1)) {
        value = atoi(arg1);
        if (!(pMob = get_mob_prototype(value))) {
            send_to_char("MEdit:  That vnum does not exist.\n\r", ch);
            return;
        }

        if (!IS_BUILDER(ch, pMob->area)) {
            send_to_char("You do not have enough security to edit mobs.\n\r", ch);
            return;
        }

        set_editor(ch->desc, ED_MOBILE, U(pMob));
        medit_show(ch, "");
        return;
    }
    else {
        if (!str_cmp(arg1, "create")) {
            value = atoi(argument);
            if (arg1[0] == '\0' || value == 0) {
                send_to_char("Syntax:  edit mobile create [vnum]\n\r", ch);
                return;
            }

            area = get_vnum_area(value);

            if (!area) {
                send_to_char("MEdit:  That vnum is not assigned an area.\n\r", ch);
                return;
            }

            if (!IS_BUILDER(ch, area)) {
                send_to_char("You do not have enough security to edit mobs.\n\r", ch);
                return;
            }

            if (ed_new_mob("create", ch, argument, 0, 0)) {
                SET_BIT(area->area_flags, AREA_CHANGED);
                ch->desc->editor = ED_MOBILE;
                medit_show(ch, "");
            }
            return;
        }
    }

    send_to_char("MEdit:  There is no default mobile to edit.\n\r", ch);
    return;
}

/* Mobile Interpreter, called by do_medit. */
void medit(Mobile* ch, char* argument)
{
    AreaData* area;
    MobPrototype* pMob;

    EDIT_MOB(ch, pMob);
    area = pMob->area;

    if (!IS_BUILDER(ch, area)) {
        send_to_char("MEdit: Insufficient security to modify area.\n\r", ch);
        edit_done(ch);
        return;
    }

    if (!str_cmp(argument, "done")) {
        edit_done(ch);
        return;
    }

    if (emptystring(argument)) {
        medit_show(ch, argument);
        return;
    }

    /* Search Table and Dispatch Command. */
    if (!process_olc_command(ch, argument, mob_olc_comm_table))
        interpret(ch, argument);

    return;
}

// Mobile Editor Functions.
MEDIT(medit_show)
{
    MobPrototype* pMob;
    char arg[MIL];
    MobProg* list;
    int cnt;

    READ_ARG(arg);

    if (IS_NULLSTR(arg)) {
        if (ch->desc->editor == ED_MOBILE)
            EDIT_MOB(ch, pMob);
        else {
            send_to_char("{jERROR: You must specify a vnum to look at.{x\n\r", ch);
            return false;
        }
    }
    else {
        pMob = get_mob_prototype(atoi(arg));

        if (!pMob) {
            send_to_char("{jERROR: That mob does not exist.{x\n\r", ch);
            return false;
        }

        if (!IS_BUILDER(ch, pMob->area)) {
            send_to_char("{jERROR: You do not have access to the area that mob is in.{x\n\r", ch);
            return false;
        }
    }

    INIT_BUF(buffer, MSL);

    addf_buf(buffer, "Name:        {|[{*%s{|]{x\n\r", NAME_STR(pMob));
    addf_buf(buffer, "Vnum:        {|[{*%6d{|]{x\n\r", VNUM_FIELD(pMob));
    addf_buf(buffer, "Area:        {|[{*%6d{|] {_%s{x\n\r",
        !pMob->area ? -1 : VNUM_FIELD(pMob->area),
        !pMob->area ? "No Area" : NAME_STR(pMob->area));

    addf_buf(buffer, "Level:       {|[{*%6d{|]{x Sex:     {|[{*%6s{|]{x Group:   {|[{*%5d{|]{x\n\r"
        "Align:       {|[{*%6d{|]{x Hitroll: {|[{*%6d{|]{x Dam type: {|[{*%s{|]{x\n\r",
        pMob->level,
        ((pMob->sex >= SEX_MIN && pMob->sex <= SEX_MAX) ? sex_table[pMob->sex].name : "ERROR"),
        pMob->group,
        pMob->alignment,
        pMob->hitroll,
        attack_table[pMob->dam_type].name);

    addf_buf(buffer, 
        "Hit dice:    {|[{*%3dd%-3d+%4d{|]{x Damage dice:  {|[{*%3dd%-3d+%4d{|]{x\n\r"
        "Mana dice:   {|[{*%3dd%-3d+%4d{|]{x Material:     {|[{*%12s{|]{x\n\r",
        pMob->hit[DICE_NUMBER],
        pMob->hit[DICE_TYPE],
        pMob->hit[DICE_BONUS],
        pMob->damage[DICE_NUMBER],
        pMob->damage[DICE_TYPE],
        pMob->damage[DICE_BONUS],
        pMob->mana[DICE_NUMBER],
        pMob->mana[DICE_TYPE],
        pMob->mana[DICE_BONUS], 
        pMob->material);
 
    addf_buf(buffer, "Race:        {|[{*%12s{|]{x Size:         {|[{*%12s{|]{x\n\r",
        race_table[pMob->race].name,
        ((pMob->size >= MOB_SIZE_MIN && pMob->size <= MOB_SIZE_MAX) ?
            mob_size_table[pMob->size].name : "ERROR"));

    addf_buf(buffer, "Start pos.:  {|[{*%12s{|]{x Default pos.: {|[{*%12s{|]{x\n\r",
        position_table[pMob->start_pos].name,
        position_table[pMob->default_pos].name);

    addf_buf(buffer, "Wealth:      {|[{*%5d{|]{x\n\r",
        pMob->wealth);

    addf_buf(buffer, "Armor:       {|[{_pierce: {*%d{_  bash: {*%d{_  slash: {*%d{_  magic: {*%d{|]{x\n\r",
        pMob->ac[AC_PIERCE], pMob->ac[AC_BASH],
        pMob->ac[AC_SLASH], pMob->ac[AC_EXOTIC]);

    addf_buf(buffer, "Affected by: {|[{*%s{|]{x\n\r",
        flag_string(affect_flag_table, pMob->affect_flags));

    addf_buf(buffer, "Act:         {|[{*%s{|]{x\n\r",
        flag_string(act_flag_table, pMob->act_flags));

    addf_buf(buffer, "Form:        {|[{*%s{|]{x\n\r", flag_string(form_flag_table, pMob->form));
    addf_buf(buffer, "Parts:       {|[{*%s{|]{x\n\r", flag_string(part_flag_table, pMob->parts));
    addf_buf(buffer, "Imm:         {|[{*%s{|]{x\n\r", flag_string(imm_flag_table, pMob->imm_flags));
    addf_buf(buffer, "Res:         {|[{*%s{|]{x\n\r", flag_string(res_flag_table, pMob->res_flags));
    addf_buf(buffer, "Vuln:        {|[{*%s{|]{x\n\r", flag_string(vuln_flag_table, pMob->vuln_flags));
    addf_buf(buffer, "Off:         {|[{*%s{|]{x\n\r", flag_string(off_flag_table, pMob->atk_flags));

    if (pMob->spec_fun) {
        addf_buf(buffer, "Spec fun:    {|[{*%s{|]{x\n\r", spec_name(pMob->spec_fun));
    }

    addf_buf(buffer, "Short descr: {_%s{x\n\rLong descr:\n\r{_%s{x",
        pMob->short_descr,
        pMob->long_descr);

    addf_buf(buffer, "Description:\n\r{_%s{x", pMob->description);

    if (pMob->pShop) {
        ShopData* pShop;
        int iTrade;

        pShop = pMob->pShop;

        addf_buf(buffer,
            "Shop data for {|[{*%5d{|]{x:\n\r"
            "  Markup for purchaser: {*%d%%{x\n\r"
            "  Markdown for seller:  {*%d%%{x\n\r",
            pShop->keeper, pShop->profit_buy, pShop->profit_sell);
        addf_buf(buffer, "  Hours: {_%d{x to {_%d{x.\n\r",
            pShop->open_hour, pShop->close_hour);

        for (iTrade = 0; iTrade < MAX_TRADE; iTrade++) {
            if (pShop->buy_type[iTrade] != 0) {
                if (iTrade == 0) {
                    add_buf(buffer, "  {TNumber Trades Type\n\r");
                    add_buf(buffer, "  {=------ -----------{x\n\r");
                }
                addf_buf(buffer, "  {|[{*%4d{|]{x {_%s{x\n\r", iTrade,
                    flag_string(type_flag_table, pShop->buy_type[iTrade]));
            }
        }
    }

    if (pMob->mprogs) {
        addf_buf(buffer,
            "\n\rMOBPrograms for {|[{*%5d{|]{x:\n\r", VNUM_FIELD(pMob));

        for (cnt = 0, list = pMob->mprogs; list; NEXT_LINK(list)) {
            if (cnt == 0) {
                add_buf(buffer, " {TNumber Vnum Trigger Phrase\n\r");
                add_buf(buffer, " {=------ ---- ------- ------\n\r");
            }

            addf_buf(buffer, "{|[{*%5d{|] {*%4d %7s {_%s{x\n\r", cnt,
                list->vnum, event_trigger_name(list->trig_type),
                list->trig_phrase);
            cnt++;
        }
    }

    Entity* entity = &pMob->header;
    olc_display_event_info(ch, entity, buffer);
    olc_display_lox_info(ch, entity, buffer);

    //page_to_char(BUF(buffer), ch);
    send_to_char(BUF(buffer), ch);

    free_buf(buffer);

    return false;
}

MEDIT(medit_group)
{
    MobPrototype* pMob;
    MobPrototype* pMTemp;
    char arg[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int temp;
    Buffer* buffer;
    bool found = false;

    EDIT_MOB(ch, pMob);

    if (argument[0] == '\0') {
        send_to_char("Syntax: group [num]\n\r", ch);
        send_to_char("        group show [num]\n\r", ch);
        return false;
    }

    if (is_number(argument)) {
        pMob->group = (SKNUM)atoi(argument);
        send_to_char("Group set.\n\r", ch);
        return true;
    }

    READ_ARG(arg);

    if (!strcmp(arg, "show") && is_number(argument)) {
        if (atoi(argument) == 0) {
            send_to_char("Estas loco? (Are you insane?)\n\r", ch);
            return false;
        }

        buffer = new_buf();

        for (temp = 0; temp < MAX_VNUM; temp++) {
            pMTemp = get_mob_prototype(temp);
            if (pMTemp && (pMTemp->group == atoi(argument))) {
                found = true;
                sprintf(buf, "[%5d] %s\n\r", VNUM_FIELD(pMTemp), NAME_STR(pMTemp));
                add_buf(buffer, buf);
            }
        }

        if (found)
            page_to_char(BUF(buffer), ch);
        else
            send_to_char("There are no mobs in that group.\n\r", ch);

        free_buf(buffer);
    }

    return false;
}

MEDIT(medit_copy)
{
    VNUM vnum;
    MobPrototype* pMob, * mob2;

    EDIT_MOB(ch, pMob);

    if (IS_NULLSTR(argument)) {
        send_to_char("Syntax : copy [vnum]\n\r", ch);
        return false;
    }

    if (!is_number(argument) || (vnum = atoi(argument)) < 0) {
        send_to_char("ERROR : Vnum must be greater than 0.\n\r", ch);
        return false;
    }

    if ((mob2 = get_mob_prototype(vnum)) == NULL) {
        send_to_char("ERROR : That mob does not exist.\n\r", ch);
        return false;
    }

    if (!IS_BUILDER(ch, mob2->area)) {
        send_to_char("ERROR : You do not have access to the area that mob is in.\n\r", ch);
        return false;
    }

    SET_NAME(pMob, NAME_FIELD(mob2));
    free_string(pMob->short_descr);
    pMob->short_descr = str_dup(mob2->short_descr);
    free_string(pMob->long_descr);
    pMob->long_descr = str_dup(mob2->long_descr);
    free_string(pMob->description);
    pMob->description = str_dup(mob2->description);

    pMob->group = mob2->group;
    pMob->act_flags = mob2->act_flags;
    pMob->affect_flags = mob2->affect_flags;
    pMob->alignment = mob2->alignment;
    pMob->level = mob2->level;
    pMob->hitroll = mob2->hitroll;
    ARRAY_COPY(pMob->hit, mob2->hit, 3);
    ARRAY_COPY(pMob->mana, mob2->mana, 3);
    ARRAY_COPY(pMob->damage, mob2->damage, 3);
    ARRAY_COPY(pMob->ac, mob2->ac, 4);
    pMob->dam_type = mob2->dam_type;
    pMob->size = mob2->size;
    pMob->atk_flags = mob2->atk_flags;
    pMob->imm_flags = mob2->imm_flags;
    pMob->res_flags = mob2->res_flags;
    pMob->vuln_flags = mob2->vuln_flags;
    pMob->start_pos = mob2->start_pos;
    pMob->default_pos = mob2->default_pos;
    pMob->sex = mob2->sex;
    pMob->race = mob2->race;
    pMob->wealth = mob2->wealth;
    pMob->form = mob2->form;
    pMob->parts = mob2->parts;
    free_string(pMob->material);
    pMob->material = str_dup(mob2->material);

    send_to_char("Ok.\n\r", ch);
    return true;
}

ED_FUN_DEC(ed_new_mob)
{
    MobPrototype* pMob;
    AreaData* area;
    VNUM  value;

    value = STRTOVNUM(argument);

    if (argument[0] == '\0' || value == 0) {
        send_to_char("Syntax : medit create [vnum]\n\r", ch);
        return false;
    }

    area = get_vnum_area(value);

    if (!area) {
        send_to_char("MEdit : That vnum is not assigned to an area.\n\r", ch);
        return false;
    }

    if (!IS_BUILDER(ch, area)) {
        send_to_char("MEdit : You do not have access to that area.\n\r", ch);
        return false;
    }

    if (get_mob_prototype(value)) {
        send_to_char("MEdit : A mob with that vnum already exists.\n\r", ch);
        return false;
    }

    pMob = new_mob_prototype();
    push(OBJ_VAL(pMob));

    VNUM_FIELD(pMob) = value;
    pMob->area = area;
    pMob->act_flags = ACT_IS_NPC;

    if (value > top_vnum_mob)
        top_vnum_mob = value;

    SET_BIT(area->area_flags, AREA_CHANGED);

    table_set_vnum(&mob_protos, value, OBJ_VAL(pMob));
    pop(); // pMob

    set_editor(ch->desc, ED_MOBILE, U(pMob));
/*    ch->desc->pEdit        = (void *)pMob; */

    send_to_char("Mob created.\n\r", ch);

    return true;
}

ED_FUN_DEC(ed_recval)
{
    MobPrototype* pMob = (MobPrototype*)arg;

    if (pMob->level < 1 || pMob->level > 60) {
        send_to_char("The mob's level must be between 1 and 60.\n\r", ch);
        return false;
    }

    recalc(pMob);

    send_to_char("Ok.\n\r", ch);

    return true;
}

ED_FUN_DEC(ed_shop)
{
    MobPrototype* pMob = (MobPrototype*)arg;
    char command[MAX_INPUT_LENGTH];
    char arg1[MAX_INPUT_LENGTH];

    READ_ARG(command);
    READ_ARG(arg1);

    if (command[0] == '\0') {
        send_to_char("Syntax : {*shop hours [open] [close]\n\r", ch);
        send_to_char("         shop profit [%% buy] [%% sell]\n\r", ch);
        send_to_char("         shop type [0-4] [obj type]\n\r", ch);
        send_to_char("         shop type [0-4] none\n\r", ch);
        send_to_char("         shop assign\n\r", ch);
        send_to_char("         shop remove{x\n\r", ch);

        return false;
    }

    if (!str_cmp(command, "hours")) {
        if (arg1[0] == '\0' || !is_number(arg1)
            || argument[0] == '\0' || !is_number(argument)) {
            send_to_char("Syntax : {*shop hours [open] [close]{x\n\r", ch);
            return false;
        }

        if (!pMob->pShop) {
            send_to_char("{jYou must assign a shop first ('{*shop assign{j').{x\n\r", ch);
            return false;
        }

        pMob->pShop->open_hour = (int16_t)atoi(arg1);
        pMob->pShop->close_hour = (int16_t)atoi(argument);

        send_to_char("Hours set.\n\r", ch);
        return true;
    }

    if (!str_cmp(command, "profit")) {
        if (arg1[0] == '\0' || !is_number(arg1)
            || argument[0] == '\0' || !is_number(argument)) {
            send_to_char("{jSyntax : {*shop profit [%% buy] [%% sell]{x\n\r", ch);
            return false;
        }

        if (!pMob->pShop) {
            send_to_char("{jYou must assign a shop first ('{*shop assign{j').\n\r", ch);
            return false;
        }

        pMob->pShop->profit_buy = (int16_t)atoi(arg1);
        pMob->pShop->profit_sell = (int16_t)atoi(argument);

        send_to_char("Shop profit set.\n\r", ch);
        return true;
    }

    if (!str_cmp(command, "type")) {
        char buf[MAX_INPUT_LENGTH];
        int value = 0;

        if (arg1[0] == '\0' || !is_number(arg1)
            || argument[0] == '\0') {
            send_to_char("Syntax:  {*shop type [#x0-4] [item type]{x\n\r", ch);
            return false;
        }

        if (atoi(arg1) >= MAX_TRADE) {
            sprintf(buf, "{jMEdit:  May sell %d items max.{x\n\r", MAX_TRADE);
            send_to_char(buf, ch);
            return false;
        }

        if (!pMob->pShop) {
            send_to_char("{jMEdit:  You must assign a shop first ('shop assign'){x.\n\r", ch);
            return false;
        }

        if (str_cmp(argument, "none") && (value = flag_value(type_flag_table, argument)) == NO_FLAG) {
            send_to_char("{jMEdit:  That type of item does not exist.{x\n\r", ch);
            return false;
        }

        pMob->pShop->buy_type[atoi(arg1)] = !str_cmp(argument, "none") ? 0 : (int16_t)value;

        send_to_char("Shop type set.\n\r", ch);
        return true;
    }

    /* shop assign && shop delete by Phoenix */
    if (!str_prefix(command, "assign")) {
        if (pMob->pShop) {
            send_to_char("Mob already has a shop assigned to it.\n\r", ch);
            return false;
        }

        pMob->pShop = new_shop_data();
        if (!shop_first)
            shop_first = pMob->pShop;
        if (shop_last)
            shop_last->next = pMob->pShop;
        shop_last = pMob->pShop;

        pMob->pShop->keeper = VNUM_FIELD(pMob);

        send_to_char("New shop assigned to mobile.\n\r", ch);
        return true;
    }

    if (!str_prefix(command, "remove")) {
        ShopData* pShop;

        pShop = pMob->pShop;
        pMob->pShop = NULL;

        if (pShop == shop_first) {
            if (!pShop->next) {
                shop_first = NULL;
                shop_last = NULL;
            }
            else
                shop_first = pShop->next;
        }
        else {
            ShopData* ipShop;

            FOR_EACH(ipShop, shop_first) {
                if (ipShop->next == pShop) {
                    if (!pShop->next) {
                        shop_last = ipShop;
                        shop_last->next = NULL;
                    }
                    else
                        ipShop->next = pShop->next;
                }
            }
        }

        free_shop_data(pShop);

        send_to_char("Mobile is no longer a shopkeeper.\n\r", ch);
        return true;
    }

    ed_shop(n_fun, ch, "", arg, par);

    return false;
}

ED_FUN_DEC(ed_ac)
{
    MobPrototype* pMob = (MobPrototype*)arg;
    char blarg[MAX_INPUT_LENGTH];
    int16_t pierce, bash, slash, exotic;

    do   /* So that I can use break and send the syntax in one place */
    {
        if (emptystring(argument))  break;

        READ_ARG(blarg);

        if (!is_number(blarg))  break;
        pierce = (int16_t)atoi(blarg);
        READ_ARG(blarg);

        if (blarg[0] != '\0') {
            if (!is_number(blarg))  break;
            bash = (int16_t)atoi(blarg);
            READ_ARG(blarg);
        }
        else
            bash = pMob->ac[AC_BASH];

        if (blarg[0] != '\0') {
            if (!is_number(blarg))  break;
            slash = (int16_t)atoi(blarg);
            READ_ARG(blarg);
        }
        else
            slash = pMob->ac[AC_SLASH];

        if (blarg[0] != '\0') {
            if (!is_number(blarg))  break;
            exotic = (int16_t)atoi(blarg);
        }
        else
            exotic = pMob->ac[AC_EXOTIC];

        pMob->ac[AC_PIERCE] = pierce;
        pMob->ac[AC_BASH] = bash;
        pMob->ac[AC_SLASH] = slash;
        pMob->ac[AC_EXOTIC] = exotic;

        send_to_char("Ac set.\n\r", ch);
        return true;
    } while (false);    /* Just do it once.. */

    send_to_char("Syntax:  ac [ac-pierce [ac-bash [ac-slash [ac-exotic]]]]\n\r"
        "help MOB_AC  gives a list of reasonable ac-values.\n\r", ch);

    return false;
}


ED_FUN_DEC(ed_addprog)
{
    int value;
    const struct flag_type* flagtable;
    MobProg* list, ** mprogs = (MobProg**)arg;
    MobProgCode* code;
    MobPrototype* pMob;
    char trigger[MAX_STRING_LENGTH];
    char numb[MAX_STRING_LENGTH];

    READ_ARG(numb);
    READ_ARG(trigger);

    if (!is_number(numb) || trigger[0] == '\0' || argument[0] == '\0') {
        send_to_char("Syntax:   addprog [vnum] [trigger] [phrase]\n\r", ch);
        return false;
    }

    switch (ch->desc->editor) {
    case ED_MOBILE:
        flagtable = mprog_flag_table;
        break;
    default:
        send_to_char("ERROR : Invalid editor.\n\r", ch);
        return false;
    }

    if ((value = flag_value(flagtable, trigger)) == NO_FLAG) {
        send_to_char("Valid flags are:\n\r", ch);
        show_help(ch, "mprog");
        return false;
    }

    if ((code = pedit_prog(atoi(numb))) == NULL) {
        send_to_char("No such MOBProgram.\n\r", ch);
        return false;
    }

    list = new_mob_prog();
    list->vnum = atoi(numb);
    list->trig_type = value;
    list->trig_phrase = str_dup(argument);
    list->code = code->code;
    list->next = *mprogs;
    *mprogs = list;

    switch (ch->desc->editor) {
    case ED_MOBILE:
        EDIT_MOB(ch, pMob);
        SET_BIT(pMob->mprog_flags, value);
        break;
    }

    send_to_char("Mprog Added.\n\r", ch);
    return true;
}

ED_FUN_DEC(ed_delprog)
{
    MobProg* list;
    MobProg* list_next;
    MobProg** mprogs = (MobProg**)arg;
    MobPrototype* pMob;
    char mprog[MAX_STRING_LENGTH];
    int value;
    int cnt = 0, t2rem;

    one_argument(argument, mprog);

    if (!is_number(mprog) || mprog[0] == '\0') {
        send_to_char("Syntax:  delmprog [#mprog]\n\r", ch);
        return false;
    }

    value = atoi(mprog);

    if (value < 0) {
        send_to_char("Only non-negative mprog-numbers allowed.\n\r", ch);
        return false;
    }

    if (!(list = *mprogs)) {
        send_to_char("MEdit:  Non existant mprog.\n\r", ch);
        return false;
    }

    if (value == 0) {
        list = *mprogs;
        t2rem = list->trig_type;
        *mprogs = list->next;
        free_mob_prog(list);
    }
    else {
        while ((list_next = list->next) && (++cnt < value))
            list = list_next;

        if (list_next) {
            t2rem = list_next->trig_type;
            list->next = list_next->next;
            free_mob_prog(list_next);
        }
        else {
            send_to_char("No such mprog.\n\r", ch);
            return false;
        }
    }

    switch (ch->desc->editor) {
    case ED_MOBILE:
        EDIT_MOB(ch, pMob);
        REMOVE_BIT(pMob->mprog_flags, t2rem);
        break;
    }

    send_to_char("Mprog removed.\n\r", ch);
    return true;
}

ED_FUN_DEC(ed_race)
{
    MobPrototype* pMob = (MobPrototype*)arg;
    int16_t race;

    if (argument[0] != '\0'
        && (race = race_lookup(argument)) != 0) {
        pMob->race = race;
        pMob->act_flags |= race_table[race].act_flags;
        pMob->affect_flags |= race_table[race].aff;
        pMob->atk_flags |= race_table[race].off;
        pMob->imm_flags |= race_table[race].imm;
        pMob->res_flags |= race_table[race].res;
        pMob->vuln_flags |= race_table[race].vuln;
        pMob->form |= race_table[race].form;
        pMob->parts |= race_table[race].parts;

        send_to_char("Race set.\n\r", ch);
        return true;
    }

    if (argument[0] == '?') {
        char buf[MIL];

        send_to_char("Available races are:", ch);

        for (race = 0; race_table[race].name != NULL; race++) {
            if ((race % 3) == 0)
                send_to_char("\n\r", ch);
            sprintf(buf, " %-15s", race_table[race].name);
            send_to_char(buf, ch);
        }

        send_to_char("\n\r", ch);
        return false;
    }

    send_to_char("Syntax:  race [race]\n\r"
        "Type 'race ?' for a list of races.\n\r", ch);
    return false;
}
