////////////////////////////////////////////////////////////////////////////////
// medit.c
////////////////////////////////////////////////////////////////////////////////

#include <merc.h>

#include <limits.h>

#include "bit.h"
#include "event_edit.h"
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

#include <data/mobile_data.h>

#include <entities/event.h>
#include <entities/faction.h>
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
    { "faction",    0,                      ed_olded,          U(medit_faction) },
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
    { "ac",	    U(&xMob),			    ed_ac,			    0		        },
    { "hitdice",	U(&xMob.hit[0]),        ed_dice,		    0		        },
    { "manadice",	U(&xMob.mana[0]),		ed_dice,		    0		        },
    { "damdice",	U(&xMob.damage[0]),	    ed_dice,		    0		        },
    { "hitroll",	U(&xMob.hitroll),		ed_number_s_pos,	0		        },
    { "wealth",	    0,                      ed_olded,           U(medit_wealth)  },
    { "addprog",	U(&xMob.mprogs),		ed_addprog,		    0		        },
    { "delprog",	U(&xMob.mprogs),		ed_delprog,		    0		        },
    { "show",       0,				        ed_olded,		    U(medit_show)	},
    { "mshow",	    0,				        ed_olded,		    U(medit_show)	},
    { "oshow",	    0,				        ed_olded,		    U(oedit_show)	},
    { "olist",	    U(&xMob.area),		    ed_olist,		    0		        },
    { "event",      0,                      ed_olded,           U(olc_edit_event)   },
    { "lox",        0,                      ed_olded,           U(olc_edit_lox)     },
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
            send_to_char(COLOR_INFO "That vnum does not exist." COLOR_EOL, ch);
            return;
        }

        if (!IS_BUILDER(ch, pMob->area)) {
            send_to_char(COLOR_INFO "You do not have enough security to edit mobs." COLOR_EOL, ch);
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
                send_to_char(COLOR_INFO "Syntax:  edit mobile create <vnum>" COLOR_EOL, ch);
                return;
            }

            area = get_vnum_area(value);

            if (!area) {
                send_to_char(COLOR_INFO "That vnum is not assigned an area." COLOR_EOL, ch);
                return;
            }

            if (!IS_BUILDER(ch, area)) {
                send_to_char(COLOR_INFO "You do not have enough security to edit mobs." COLOR_EOL, ch);
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

    send_to_char(COLOR_INFO "There is no default mobile to edit." COLOR_EOL, ch);
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
        send_to_char(COLOR_INFO "Insufficient security to modify area." COLOR_EOL, ch);
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
    char buf[MIL];
    bool hide_unused = true;

    READ_ARG(arg);

    if (IS_NULLSTR(arg) || !str_cmp(arg, "all")) {
        if (ch->desc->editor == ED_MOBILE)
            EDIT_MOB(ch, pMob);
        else {
            send_to_char(COLOR_INFO "You must specify a vnum to look at." COLOR_EOL, ch);
            return false;
        }

        if (!IS_NULLSTR(arg) && !str_cmp(arg, "all")) 
            hide_unused = false;
    }
    else if (is_number(arg)) {
        pMob = get_mob_prototype(atoi(arg));

        if (!pMob) {
            send_to_char(COLOR_INFO "That mob does not exist." COLOR_EOL, ch);
            return false;
        }

        if (!IS_BUILDER(ch, pMob->area)) {
            send_to_char(COLOR_INFO "You do not have access to the area that mob is in." COLOR_EOL, ch);
            return false;
        }

        READ_ARG(arg);
        if (!IS_NULLSTR(arg) && !str_cmp(arg, "all")) 
            hide_unused = false;
    }
    else {
        send_to_char(COLOR_INFO "You must specify a vnum to look at." COLOR_EOL, ch);
        return false;
    }

    olc_print_str(ch, "Name", NAME_STR(pMob));
    olc_print_num(ch, "Vnum", VNUM_FIELD(pMob));
    olc_print_num_str(ch, "Area", !pMob->area ? -1 : VNUM_FIELD(pMob->area),
        !pMob->area ? "No Area" : NAME_STR(pMob->area));

    if (!hide_unused) {
        olc_print_text(ch, "Short Desc", pMob->short_descr);
        olc_print_text(ch, "Long Desc", pMob->long_descr);
        olc_print_text(ch, "Description", IS_NULLSTR(pMob->description) ? "(none)" : pMob->description);
    }
    else {
        
        olc_print_text_ex(ch, "Short Desc", pMob->short_descr, 63);
        olc_print_text_ex(ch, "Long Desc", pMob->long_descr, 63);
        olc_print_text_ex(ch, "Description", IS_NULLSTR(pMob->description) ? "(none)" : pMob->description, 63);
    }

    olc_print_num_str(ch, "Faction", pMob->faction_vnum,
        pMob->faction_vnum == 0 ? "(none)" :
        (get_faction(pMob->faction_vnum) ? NAME_STR(get_faction(pMob->faction_vnum)) : "unknown"));
    olc_print_num(ch, "Level", pMob->level);
    olc_print_num(ch, "Hitroll", pMob->hitroll);
    if (!hide_unused || pMob->dam_type != 0)
        olc_print_str_box(ch, "Dam Type", attack_table[pMob->dam_type].name, "");

    static const char* DICE_FORMAT = "%dd%d+%d";
    sprintf(buf, DICE_FORMAT, pMob->hit[DICE_NUMBER], pMob->hit[DICE_TYPE], pMob->hit[DICE_BONUS]);
    olc_print_str_box(ch, "Hit Dice", buf, "");
    sprintf(buf, DICE_FORMAT, pMob->damage[DICE_NUMBER], pMob->damage[DICE_TYPE], pMob->damage[DICE_BONUS]);
    olc_print_str_box(ch, "Damage Dice", buf, "");
    if (!hide_unused || (pMob->mana[DICE_NUMBER] != 0 || pMob->mana[DICE_TYPE] != 0)) {
        sprintf(buf, DICE_FORMAT, pMob->mana[DICE_NUMBER], pMob->mana[DICE_TYPE], pMob->mana[DICE_BONUS]);
        olc_print_str_box(ch, "Mana Dice", buf, "");
    }
    sprintf(buf, COLOR_ALT_TEXT_2"pierce: " COLOR_ALT_TEXT_1 "%d" COLOR_ALT_TEXT_2 "  bash: " COLOR_ALT_TEXT_1 "%d" COLOR_ALT_TEXT_2 "  slash: " COLOR_ALT_TEXT_1 "%d" COLOR_ALT_TEXT_2 "  exotic: " COLOR_ALT_TEXT_1 "%d",
        pMob->ac[AC_PIERCE], pMob->ac[AC_BASH],
        pMob->ac[AC_SLASH], pMob->ac[AC_EXOTIC]);
    olc_print_str_box(ch, "AC", buf, "");
    if (!hide_unused || pMob->alignment != 0)
        olc_print_num(ch, "Alignment", pMob->alignment);
    if (!hide_unused || pMob->group != 0)
        olc_print_num_str(ch, "Group", pMob->group, get_mob_group_name(pMob->group));
    if (!hide_unused || pMob->sex != SEX_MIN)
        olc_print_str_box(ch, "Sex", (pMob->sex >= SEX_MIN && pMob->sex <= SEX_MAX) ? sex_table[pMob->sex].name : "ERROR", "");
    if (!hide_unused || pMob->default_pos != POS_STANDING)
        olc_print_str_box(ch, "Default Pos", position_table[pMob->default_pos].name, "");
    if (!hide_unused || pMob->start_pos != pMob->default_pos)
        olc_print_str_box(ch, "Start Pos", position_table[pMob->start_pos].name, "");
    if (!hide_unused || (!IS_NULLSTR(pMob->material) && str_cmp(pMob->material, "unknown")))
        olc_print_str_box(ch, "Material", pMob->material ? pMob->material : "(none)", "");

    if (!hide_unused || pMob->wealth != 0) {
        int16_t wealth_gold = 0, wealth_silver = 0, wealth_copper = 0;
        convert_copper_to_money(pMob->wealth, &wealth_gold, &wealth_silver, &wealth_copper);
        char wealth_desc[64];
        format_money_string(wealth_desc, sizeof(wealth_desc), wealth_gold, wealth_silver, wealth_copper, false);
        olc_print_num_str(ch, "Wealth (cp)", pMob->wealth, wealth_desc);
    }

    // Race-derived stats
    Race* race = &race_table[pMob->race];
    olc_print_str_box(ch, "Race", race->name, "");

    if (!hide_unused || pMob->size != race->size)
        olc_print_str_box(ch, "Size", (pMob->size >= MOB_SIZE_MIN && pMob->size <= MOB_SIZE_MAX) ?
            mob_size_table[pMob->size].name : "ERROR", "");
    if (!hide_unused || pMob->affect_flags != race->aff)
        olc_print_flags(ch, "Aff flags", affect_flag_table, pMob->affect_flags);
    if (!hide_unused || pMob->act_flags != race->act_flags)
        olc_print_flags(ch, "Act flags", act_flag_table, pMob->act_flags);
    if (!hide_unused || pMob->form != race->form)
        olc_print_flags_ex(ch, "Form Flags", form_flag_table, form_defaults_flag_table, pMob->form);
    if (!hide_unused || pMob->parts != race->parts)
        olc_print_flags_ex(ch, "Part Flags", part_flag_table, part_defaults_flag_table, pMob->parts);
    if (!hide_unused || pMob->imm_flags != race->imm)
        olc_print_flags(ch, "Imm flags", imm_flag_table, pMob->imm_flags);
    if (!hide_unused || pMob->res_flags != race->res)
        olc_print_flags(ch, "Res flags", res_flag_table, pMob->res_flags);
    if (!hide_unused || pMob->vuln_flags != race->vuln)
        olc_print_flags(ch, "Vuln flags", vuln_flag_table, pMob->vuln_flags);
    if (!hide_unused || pMob->atk_flags != race->off)
        olc_print_flags(ch, "Off flags", off_flag_table, pMob->atk_flags);
    
    if (!hide_unused || pMob->spec_fun != NULL)
        olc_print_str_box(ch, "Spec Fun", pMob->spec_fun ? spec_name(pMob->spec_fun) : "(none)", "");

    Entity* entity = &pMob->header;
    olc_display_lox_info(ch, entity);
    olc_display_events(ch, entity);
    
    if (pMob->pShop) {
        ShopData* pShop;
        int iTrade;

        pShop = pMob->pShop;

        printf_to_char(ch,
            "Shop data for " COLOR_DECOR_1 "[" COLOR_ALT_TEXT_1 "%5d" COLOR_DECOR_1 "]" COLOR_CLEAR ":\n\r"
            "  Markup for purchaser: " COLOR_ALT_TEXT_1 "%d%%" COLOR_EOL
            "  Markdown for seller:  " COLOR_ALT_TEXT_1 "%d%%" COLOR_EOL,
            pShop->keeper, pShop->profit_buy, pShop->profit_sell);
        printf_to_char(ch, "  Hours: " COLOR_ALT_TEXT_2 "%d" COLOR_CLEAR " to " COLOR_ALT_TEXT_2 "%d" COLOR_CLEAR ".\n\r",
            pShop->open_hour, pShop->close_hour);

        for (iTrade = 0; iTrade < MAX_TRADE; iTrade++) {
            if (pShop->buy_type[iTrade] != 0) {
                if (iTrade == 0) {
                    printf_to_char(ch, "  " COLOR_TITLE "Number Trades Type\n\r");
                    printf_to_char(ch, "  " COLOR_DECOR_2 "------ -----------" COLOR_EOL);
                }
                printf_to_char(ch, "  " COLOR_DECOR_1 "[" COLOR_ALT_TEXT_1 "%4d" COLOR_DECOR_1 "]" COLOR_CLEAR " " COLOR_ALT_TEXT_2 "%s" COLOR_EOL, iTrade,
                    flag_string(type_flag_table, pShop->buy_type[iTrade]));
            }
        }
    }

    if (pMob->mprogs) {
        MobProg* list;
        int cnt;
        printf_to_char(ch,
            "\n\rMOBPrograms for " COLOR_DECOR_1 "[" COLOR_ALT_TEXT_1 "%5d" COLOR_DECOR_1 "]" COLOR_CLEAR ":\n\r", VNUM_FIELD(pMob));

        for (cnt = 0, list = pMob->mprogs; list; NEXT_LINK(list)) {
            if (cnt == 0) {
                printf_to_char(ch, " " COLOR_TITLE "Number Vnum Trigger Phrase\n\r");
                printf_to_char(ch, " " COLOR_DECOR_2 "------ ---- ------- ------\n\r");
            }

            printf_to_char(ch, COLOR_DECOR_1 "[" COLOR_ALT_TEXT_1 "%5d" COLOR_DECOR_1 "] " COLOR_ALT_TEXT_1 "%4d %7s " COLOR_ALT_TEXT_2 "%s" COLOR_EOL, cnt,
                list->vnum, event_trigger_name(list->trig_type),
                list->trig_phrase);
            cnt++;
        }
    }

    if (hide_unused)
        send_to_char(COLOR_INFO "\n\r(Note: Some fields are hidden. Use 'show all' to see all fields.)" COLOR_EOL, ch);

    return false;
}

MEDIT(medit_wealth)
{
    MobPrototype* pMob;
    char amount_arg[MIL];
    char unit_arg[MIL];

    EDIT_MOB(ch, pMob);

    argument = one_argument(argument, amount_arg);
    argument = one_argument(argument, unit_arg);

    if (IS_NULLSTR(amount_arg)) {
        send_to_char(COLOR_INFO "Syntax: wealth <amount> [cp|sp|gp]" COLOR_EOL, ch);
        return false;
    }

    if (!is_number(amount_arg)) {
        send_to_char(COLOR_INFO "Amount must be numeric." COLOR_EOL, ch);
        return false;
    }

    long amount = atol(amount_arg);
    if (amount < 0) {
        send_to_char(COLOR_INFO "Amount must be non-negative." COLOR_EOL, ch);
        return false;
    }

    long multiplier = 1;
    if (!IS_NULLSTR(unit_arg)) {
        MoneyType money_type;
        if (!parse_money_type(unit_arg, &money_type)) {
            send_to_char(COLOR_INFO "Unit must be cp, sp, or gp." COLOR_EOL, ch);
            return false;
        }
        multiplier = money_type_value(money_type);
    }

    long total = amount * multiplier;
    if (total > INT_MAX) {
        send_to_char(COLOR_INFO "Amount is too large." COLOR_EOL, ch);
        return false;
    }

    pMob->wealth = (int)total;

    int16_t gold = 0, silver = 0, copper = 0;
    convert_copper_to_money(pMob->wealth, &gold, &silver, &copper);
    char wealth_desc[64];
    format_money_string(wealth_desc, sizeof(wealth_desc), gold, silver, copper, false);
    printf_to_char(ch, COLOR_INFO "Wealth set to " COLOR_ALT_TEXT_1 "%d cp" COLOR_INFO " (%s)." COLOR_EOL,
        pMob->wealth, wealth_desc);

    return true;
}

MEDIT(medit_faction)
{
    MobPrototype* pMob;
    char arg[MIL];

    EDIT_MOB(ch, pMob);

    argument = one_argument(argument, arg);

    if (IS_NULLSTR(arg)) {
        send_to_char(COLOR_INFO "Syntax: faction <vnum|name|none>" COLOR_EOL, ch);
        send_to_char(COLOR_INFO "        faction none - clears the faction." COLOR_EOL, ch);
        return false;
    }

    if (!str_cmp(arg, "none")) {
        pMob->faction_vnum = 0;
        if (pMob->area != NULL)
            SET_BIT(pMob->area->area_flags, AREA_CHANGED);
        send_to_char(COLOR_INFO "Faction cleared." COLOR_EOL, ch);
        return true;
    }

    Faction* faction = NULL;
    if (is_number(arg))
        faction = get_faction((VNUM)atoi(arg));
    else
        faction = get_faction_by_name(arg);

    if (faction == NULL) {
        send_to_char(COLOR_INFO "That faction does not exist." COLOR_EOL, ch);
        return false;
    }

    pMob->faction_vnum = VNUM_FIELD(faction);
    if (pMob->area != NULL)
        SET_BIT(pMob->area->area_flags, AREA_CHANGED);

    send_to_char(COLOR_INFO "Faction updated." COLOR_EOL, ch);
    return true;
}

MEDIT(medit_group)
{
    MobPrototype* pMob;
    MobPrototype* pMTemp;
    char arg[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    Buffer* buffer;
    bool found = false;

    EDIT_MOB(ch, pMob);

    if (argument[0] == '\0') {
        send_to_char(COLOR_INFO "Syntax: group <num>\n\r", ch);
        send_to_char("        group show <num>\n\r", ch);
        send_to_char("        group list" COLOR_EOL, ch);
        return false;
    }

    if (is_number(argument)) {
        pMob->group = (VNUM)atoi(argument);
        send_to_char(COLOR_INFO "Group set." COLOR_EOL, ch);
        return true;
    }

    READ_ARG(arg);

    if (!strcmp(arg, "list")) {
        send_to_char(COLOR_TITLE   " Group VNUM  Group Name" COLOR_EOL, ch);
        send_to_char(COLOR_DECOR_2 " ========== ============" COLOR_EOL, ch);
        for (int i = 0; i < mob_group_count; i++) {
            sprintf(buf, COLOR_DECOR_1 "  [ " COLOR_ALT_TEXT_1 "%6d" COLOR_DECOR_1 " ] " COLOR_ALT_TEXT_2 "%s" COLOR_EOL, mob_groups[i].vnum, mob_groups[i].name);
            send_to_char(buf, ch);
        }

        return false;
    }

    if (!strcmp(arg, "show")) {
        if (!is_number(argument)) {
            send_to_char(COLOR_INFO "You must specify a mob group." COLOR_EOL, ch);
            return false;
        }

        buffer = new_buf();

        VNUM vnum = atoi(argument);

        int i;
        for (i = 0; i < mob_group_count; i++) {
            if (mob_groups[i].vnum == vnum) {
                sprintf(buf, COLOR_TITLE "Mob Group %d (%s)" COLOR_EOL, mob_groups[i].vnum, mob_groups[i].name);
                add_buf(buffer, buf);
                break;
            }
        }

        if (i == mob_group_count) {
            send_to_char(COLOR_INFO "That mob group does not exist. Use GROUP LIST to see all groups." COLOR_EOL, ch);
            free_buf(buffer);
            return false;
        }

        FOR_EACH_MOB_PROTO(pMTemp) {
            if (pMTemp->group == vnum) {
                found = true;
                sprintf(buf, COLOR_DECOR_1 "  [ " COLOR_ALT_TEXT_1 "%6d" COLOR_DECOR_1 " ] " COLOR_ALT_TEXT_2 "%s" COLOR_EOL, VNUM_FIELD(pMTemp), NAME_STR(pMTemp));
                add_buf(buffer, buf);
            }
        }

        if (found)
            page_to_char(BUF(buffer), ch);
        else
            send_to_char(COLOR_INFO "There are no mobs in that group." COLOR_EOL, ch);

        free_buf(buffer);
        return false;
    }

    send_to_char(COLOR_INFO "Syntax: group <num>\n\r", ch);
    send_to_char("        group show <num>\n\r", ch);
    send_to_char("        group list" COLOR_EOL, ch);

    return false;
}

MEDIT(medit_copy)
{
    VNUM vnum;
    MobPrototype* pMob, * mob2;

    EDIT_MOB(ch, pMob);

    if (IS_NULLSTR(argument)) {
        send_to_char(COLOR_INFO "Syntax: copy <vnum>" COLOR_EOL, ch);
        return false;
    }

    if (!is_number(argument) || (vnum = atoi(argument)) < 0) {
        send_to_char(COLOR_INFO "Vnum must be greater than 0." COLOR_EOL, ch);
        return false;
    }

    if ((mob2 = get_mob_prototype(vnum)) == NULL) {
        send_to_char(COLOR_INFO "That mob does not exist." COLOR_EOL, ch);
        return false;
    }

    if (!IS_BUILDER(ch, mob2->area)) {
        send_to_char(COLOR_INFO "You do not have access to the area that mob is in." COLOR_EOL, ch);
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

    send_to_char(COLOR_INFO "Ok." COLOR_EOL, ch);
    return true;
}

ED_FUN_DEC(ed_new_mob)
{
    MobPrototype* pMob;
    AreaData* area;
    VNUM  value;

    value = STRTOVNUM(argument);

    if (argument[0] == '\0' || value == 0) {
        send_to_char(COLOR_INFO "Syntax: medit create <vnum>" COLOR_EOL, ch);
        return false;
    }

    area = get_vnum_area(value);

    if (!area) {
        send_to_char(COLOR_INFO "That vnum is not assigned to an area." COLOR_EOL, ch);
        return false;
    }

    if (!IS_BUILDER(ch, area)) {
        send_to_char(COLOR_INFO "You do not have access to that area." COLOR_EOL, ch);
        return false;
    }

    if (get_mob_prototype(value)) {
        send_to_char(COLOR_INFO "A mob with that vnum already exists." COLOR_EOL, ch);
        return false;
    }

    pMob = new_mob_prototype();

    VNUM_FIELD(pMob) = value;
    pMob->area = area;
    pMob->act_flags = ACT_IS_NPC;

    if (value > top_vnum_mob)
        top_vnum_mob = value;

    SET_BIT(area->area_flags, AREA_CHANGED);

    global_mob_proto_set(pMob);

    set_editor(ch->desc, ED_MOBILE, U(pMob));
/*    ch->desc->pEdit        = (void *)pMob; */

    send_to_char(COLOR_INFO "Mob created." COLOR_EOL, ch);

    return true;
}

ED_FUN_DEC(ed_recval)
{
    MobPrototype* pMob = (MobPrototype*)arg;

    if (pMob->level < 1 || pMob->level > 60) {
        send_to_char(COLOR_INFO "The mob's level must be between 1 and 60." COLOR_EOL, ch);
        return false;
    }

    recalc(pMob);

    send_to_char(COLOR_INFO "Ok." COLOR_EOL, ch);

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
        send_to_char(COLOR_INFO "Syntax : " COLOR_ALT_TEXT_1 "shop hours [open] [close]\n\r", ch);
        send_to_char("         shop profit [%% buy] [%% sell]\n\r", ch);
        send_to_char("         shop type [0-4] [obj type]\n\r", ch);
        send_to_char("         shop type [0-4] none\n\r", ch);
        send_to_char("         shop assign\n\r", ch);
        send_to_char("         shop remove" COLOR_EOL, ch);

        return false;
    }

    if (!str_cmp(command, "hours")) {
        if (arg1[0] == '\0' || !is_number(arg1)
            || argument[0] == '\0' || !is_number(argument)) {
            send_to_char(COLOR_INFO "Syntax : " COLOR_ALT_TEXT_1 "shop hours [open] [close]" COLOR_EOL, ch);
            return false;
        }

        if (!pMob->pShop) {
            send_to_char(COLOR_INFO "You must assign a shop first ('" COLOR_ALT_TEXT_1 "shop assign" COLOR_INFO "')." COLOR_EOL, ch);
            return false;
        }

        pMob->pShop->open_hour = (int16_t)atoi(arg1);
        pMob->pShop->close_hour = (int16_t)atoi(argument);

        send_to_char(COLOR_INFO "Hours set." COLOR_EOL, ch);
        return true;
    }

    if (!str_cmp(command, "profit")) {
        if (arg1[0] == '\0' || !is_number(arg1)
            || argument[0] == '\0' || !is_number(argument)) {
            send_to_char(COLOR_INFO "Syntax : " COLOR_ALT_TEXT_1 "shop profit [%% buy] [%% sell]" COLOR_EOL, ch);
            return false;
        }

        if (!pMob->pShop) {
            send_to_char(COLOR_INFO "You must assign a shop first ('" COLOR_ALT_TEXT_1 "shop assign" COLOR_INFO "').\n\r", ch);
            return false;
        }

        pMob->pShop->profit_buy = (int16_t)atoi(arg1);
        pMob->pShop->profit_sell = (int16_t)atoi(argument);

        send_to_char(COLOR_INFO "Shop profit set." COLOR_EOL, ch);
        return true;
    }

    if (!str_cmp(command, "type")) {
        char buf[MAX_INPUT_LENGTH];
        int value = 0;

        if (arg1[0] == '\0' || !is_number(arg1)
            || argument[0] == '\0') {
            send_to_char(COLOR_INFO "Syntax:  " COLOR_ALT_TEXT_1 "shop type [#x0-4] [item type]" COLOR_EOL, ch);
            return false;
        }

        if (atoi(arg1) >= MAX_TRADE) {
            sprintf(buf, COLOR_INFO "Cannot exceed %d items." COLOR_EOL, MAX_TRADE);
            send_to_char(buf, ch);
            return false;
        }

        if (!pMob->pShop) {
            send_to_char(COLOR_INFO "You must assign a shop first ('shop assign')" COLOR_CLEAR "." COLOR_EOL, ch);
            return false;
        }

        if (str_cmp(argument, "none") && (value = flag_value(type_flag_table, argument)) == NO_FLAG) {
            send_to_char(COLOR_INFO "That type of item does not exist." COLOR_EOL, ch);
            return false;
        }

        pMob->pShop->buy_type[atoi(arg1)] = !str_cmp(argument, "none") ? 0 : (int16_t)value;

        send_to_char(COLOR_INFO "Shop type set." COLOR_EOL, ch);
        return true;
    }

    /* shop assign && shop delete by Phoenix */
    if (!str_prefix(command, "assign")) {
        if (pMob->pShop) {
            send_to_char(COLOR_INFO "Mob already has a shop assigned to it." COLOR_EOL, ch);
            return false;
        }

        pMob->pShop = new_shop_data();
        if (!shop_first)
            shop_first = pMob->pShop;
        if (shop_last)
            shop_last->next = pMob->pShop;
        shop_last = pMob->pShop;

        pMob->pShop->keeper = VNUM_FIELD(pMob);

        send_to_char(COLOR_INFO "New shop assigned to mobile." COLOR_EOL, ch);
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

        send_to_char(COLOR_INFO "Mobile is no longer a shopkeeper." COLOR_EOL, ch);
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

        send_to_char(COLOR_INFO "AC set." COLOR_EOL, ch);
        return true;
    } while (false);    /* Just do it once.. */

    send_to_char(COLOR_INFO "Syntax:  ac [ac-pierce [ac-bash [ac-slash [ac-exotic]]]]\n\r"
        "help MOB_AC  gives a list of reasonable ac-values.\n\r" COLOR_EOL, ch);

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
        send_to_char(COLOR_INFO "Syntax:   addprog <vnum> <trigger> [phrase]\n\r" COLOR_EOL, ch);
        return false;
    }

    switch (ch->desc->editor) {
    case ED_MOBILE:
        flagtable = mprog_flag_table;
        break;
    default:
        send_to_char(COLOR_INFO "Invalid editor.\n\r" COLOR_EOL, ch);
        return false;
    }

    if ((value = flag_value(flagtable, trigger)) == NO_FLAG) {
        send_to_char(COLOR_INFO "Invalid trigger. Valid triggers are:\n\r" COLOR_EOL, ch);
        show_help(ch, "mprog");
        return false;
    }

    if ((code = pedit_prog(atoi(numb))) == NULL) {
        send_to_char(COLOR_INFO "No such MOBProgram.\n\r" COLOR_EOL, ch);
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

    send_to_char(COLOR_INFO "Mprog Added." COLOR_EOL, ch);
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
        send_to_char(COLOR_INFO "Syntax:  delmprog <#mprog>\n\r" COLOR_EOL, ch);
        return false;
    }

    value = atoi(mprog);

    if (value < 0) {
        send_to_char(COLOR_INFO "Only non-negative mprog-numbers allowed.\n\r" COLOR_EOL, ch);
        return false;
    }

    if (!(list = *mprogs)) {
        send_to_char(COLOR_INFO "Non-existant mprog.\n\r" COLOR_EOL, ch);
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
            send_to_char(COLOR_INFO "No such mprog.\n\r" COLOR_EOL, ch);
            return false;
        }
    }

    switch (ch->desc->editor) {
    case ED_MOBILE:
        EDIT_MOB(ch, pMob);
        REMOVE_BIT(pMob->mprog_flags, t2rem);
        break;
    }

    send_to_char(COLOR_INFO "Mprog removed." COLOR_EOL, ch);
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

        send_to_char(COLOR_INFO "Race set." COLOR_EOL, ch);
        return true;
    }

    if (argument[0] == '?') {
        char buf[MIL];

        send_to_char(COLOR_INFO "Available races are:\n\r" COLOR_ALT_TEXT_1, ch);

        for (race = 0; race_table[race].name != NULL; race++) {
            if ((race % 3) == 0)
                send_to_char("\n\r", ch);
            sprintf(buf, " %-15s", race_table[race].name);
            send_to_char(buf, ch);
        }

        send_to_char(COLOR_EOL, ch);
        return false;
    }

    send_to_char(COLOR_INFO "Syntax:  race <race>\n\r"
        "Type 'race ?' for a list of races.\n\r" COLOR_EOL, ch);
    return false;
}
