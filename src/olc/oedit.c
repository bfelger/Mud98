////////////////////////////////////////////////////////////////////////////////
// oedit.c
////////////////////////////////////////////////////////////////////////////////

#include "merc.h"

#include "bit.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "lookup.h"
#include "magic.h"
#include "olc.h"
#include "recycle.h"
#include "save.h"
#include "tables.h"

#include "entities/object_data.h"

#define OEDIT(fun) bool fun( CharData *ch, char *argument )

ObjectPrototype xObj;

#ifdef U
#define OLD_U U
#endif
#define U(x)    (uintptr_t)(x)

const OlcCmdEntry obj_olc_comm_table[] = {
    { "name",	    U(&xObj.name),		    ed_line_string,		0		        },
    { "short",	    U(&xObj.short_descr),	ed_line_string,		0		        },
    { "long",	    U(&xObj.description),	ed_line_string,		0		        },
    { "material",	U(&xObj.material),	    ed_line_string,		0		        },
    { "cost",	    U(&xObj.cost),		    ed_number_pos,		0		        },
    { "level",	    U(&xObj.level),		    ed_number_level,    0		        },
    { "condition",	U(&xObj.condition),	    ed_number_s_pos,	0		        },
    { "weight",	    U(&xObj.weight),		ed_number_s_pos,	0		        },
    { "extra",	    U(&xObj.extra_flags),   ed_flag_toggle,		U(extra_flag_table)  },
    { "wear",	    U(&xObj.wear_flags),	ed_flag_toggle,		U(wear_flag_table)   },
    { "ed",	        U(&xObj.extra_desc),	ed_ed,			    0		        },
    { "type",	    U(&xObj.item_type),	    ed_flag_set_sh,		U(type_flag_table)   },
    { "addaffect",	U(&xObj),			    ed_addaffect,		0		        },
    { "delaffect",	U(&xObj.affected),	    ed_delaffect,		0		        },
    { "addapply",	U(&xObj),			    ed_addapply,		0		        },
    { "v0",	        0,				        ed_value,		    0		        },
    { "v1",	        0,				        ed_value,		    1	            },
    { "v2",	        0,				        ed_value,		    2	            },
    { "v3",	        0,				        ed_value,		    3	            },
    { "v4",	        0,				        ed_value,		    4	            },
    { "create",	    0,				        ed_new_obj,		    0		        },
    { "mshow",	    0,				        ed_olded,		    U(medit_show)   },
    { "oshow",	    0,				        ed_olded,		    U(oedit_show)   },
    { "olist",	    U(&xObj.area),		    ed_olist,		    0		        },
    { "recval",	    U(&xObj),			    ed_objrecval,		0		        },
    { "commands",	0,				        ed_olded,		    U(show_commands)},
    { "?",		    0,				        ed_olded,		    U(show_help)	},
    { "version",	0,				        ed_olded,		    U(show_version)	},
    { NULL,	        0,				        NULL,			    0		        }
};

/* Entry point for editing object_prototype_data. */
void do_oedit(CharData* ch, char* argument)
{
    ObjectPrototype* pObj;
    AreaData* pArea;
    char arg1[MAX_STRING_LENGTH];
    int  value;

    if (IS_NPC(ch) || ch->desc == NULL)
        return;

    READ_ARG(arg1);

    if (is_number(arg1)) {
        value = atoi(arg1);
        if (!(pObj = get_object_prototype(value))) {
            send_to_char("OEdit:  That vnum does not exist.\n\r", ch);
            return;
        }

        if (!IS_BUILDER(ch, pObj->area)) {
            send_to_char("You do not have enough security to edit objects.\n\r", ch);
            return;
        }

        set_editor(ch->desc, ED_OBJECT, U(pObj));
        return;
    }
    else {
        if (!str_cmp(arg1, "create")) {
            value = atoi(argument);
            if (argument[0] == '\0' || value == 0) {
                send_to_char("Syntax:  edit object create [vnum]\n\r", ch);
                return;
            }

            pArea = get_vnum_area(value);

            if (!pArea) {
                send_to_char("OEdit:  That vnum is not assigned an area.\n\r", ch);
                return;
            }

            if (!IS_BUILDER(ch, pArea)) {
                send_to_char("You do not have enough security to edit objects.\n\r", ch);
                return;
            }

            if (oedit_create(ch, argument)) {
                SET_BIT(pArea->area_flags, AREA_CHANGED);
                ch->desc->editor = ED_OBJECT;
            }
            return;
        }
    }

    send_to_char("OEdit:  There is no default object to edit.\n\r", ch);
    return;
}

/* Object Interpreter, called by do_oedit. */
void oedit(CharData* ch, char* argument)
{
    AreaData* pArea;
    ObjectPrototype* pObj;

    EDIT_OBJ(ch, pObj);
    pArea = pObj->area;

    if (!IS_BUILDER(ch, pArea)) {
        send_to_char("OEdit: Insufficient security to modify area.\n\r", ch);
        edit_done(ch);
        return;
    }

    if (!str_cmp(argument, "done")) {
        edit_done(ch);
        return;
    }

    if (emptystring(argument)) {
        oedit_show(ch, argument);
        return;
    }

    /* Search Table and Dispatch Command. */
    if (!process_olc_command(ch, argument, obj_olc_comm_table))
        interpret(ch, argument);

    return;
}

/*
 * Object Editor Functions.
 */
void show_obj_values(CharData* ch, ObjectPrototype* obj)
{
    char buf[MAX_STRING_LENGTH];

    switch (obj->item_type) {
    default:    /* No values. */
        break;

    case ITEM_LIGHT:
        if (obj->value[2] == -1 || obj->value[2] == 999) /* ROM OLC */
            sprintf(buf, "[v2] Light:  Infinite[-1]\n\r");
        else
            sprintf(buf, "[v2] Light:  [%d]\n\r", obj->value[2]);
        send_to_char(buf, ch);
        break;

    case ITEM_WAND:
    case ITEM_STAFF:
        sprintf(buf,
            "[v0] Level:          [%d]\n\r"
            "[v1] Charges Total:  [%d]\n\r"
            "[v2] Charges Left:   [%d]\n\r"
            "[v3] Spell:          %s\n\r",
            obj->value[0],
            obj->value[1],
            obj->value[2],
            obj->value[3] != -1 ? skill_table[obj->value[3]].name
            : "none");
        send_to_char(buf, ch);
        break;

    case ITEM_PORTAL:
    {
        char buf2[MIL];

        sprintf(buf2, "%s", flag_string(exit_flag_table, obj->value[1]));

        sprintf(buf,
            "[v0] Charges:        [%d]\n\r"
            "[v1] Exit Flags:     %s\n\r"
            "[v2] Portal Flags:   %s\n\r"
            "[v3] Goes to (vnum): [%d]\n\r",
            obj->value[0],
            buf2,
            flag_string(portal_flag_table, obj->value[2]),
            obj->value[3]);
        send_to_char(buf, ch);
    }
    break;

    case ITEM_FURNITURE:
        sprintf(buf,
            "[v0] Max people:      [%d]\n\r"
            "[v1] Max weight:      [%d]\n\r"
            "[v2] Furniture Flags: %s\n\r"
            "[v3] Heal bonus:      [%d]\n\r"
            "[v4] Mana bonus:      [%d]\n\r",
            obj->value[0],
            obj->value[1],
            flag_string(furniture_flag_table, obj->value[2]),
            obj->value[3],
            obj->value[4]);
        send_to_char(buf, ch);
        break;

    case ITEM_SCROLL:
    case ITEM_POTION:
    case ITEM_PILL:
        sprintf(buf,
            "[v0] Level:  [%d]\n\r"
            "[v1] Spell:  %s\n\r"
            "[v2] Spell:  %s\n\r"
            "[v3] Spell:  %s\n\r"
            "[v4] Spell:  %s\n\r",
            obj->value[0],
            obj->value[1] != -1 ? skill_table[obj->value[1]].name
            : "none",
            obj->value[2] != -1 ? skill_table[obj->value[2]].name
            : "none",
            obj->value[3] != -1 ? skill_table[obj->value[3]].name
            : "none",
            obj->value[4] != -1 ? skill_table[obj->value[4]].name
            : "none");
        send_to_char(buf, ch);
        break;

/* ARMOR for ROM */

    case ITEM_ARMOR:
        sprintf(buf,
            "[v0] Ac pierce       [%d]\n\r"
            "[v1] Ac bash         [%d]\n\r"
            "[v2] Ac slash        [%d]\n\r"
            "[v3] Ac exotic       [%d]\n\r",
            obj->value[0],
            obj->value[1],
            obj->value[2],
            obj->value[3]);
        send_to_char(buf, ch);
        break;

/* WEAPON changed in ROM: */
/* I had to split the output here, I have no idea why, but it helped -- Hugin */
/* It somehow fixed a bug in showing scroll/pill/potions too ?! */
    case ITEM_WEAPON:
        sprintf(buf, "[v0] Weapon class:   %s\n\r",
            flag_string(weapon_class, obj->value[0]));
        send_to_char(buf, ch);
        sprintf(buf, "[v1] Number of dice: [%d]\n\r", obj->value[1]);
        send_to_char(buf, ch);
        sprintf(buf, "[v2] Type of dice:   [%d]\n\r", obj->value[2]);
        send_to_char(buf, ch);
        sprintf(buf, "[v3] Type:           %s\n\r",
            attack_table[obj->value[3]].name);
        send_to_char(buf, ch);
        sprintf(buf, "[v4] Special type:   %s\n\r",
            flag_string(weapon_type2, obj->value[4]));
        send_to_char(buf, ch);
        break;

    case ITEM_CONTAINER:
        sprintf(buf,
            "[v0] Weight:     [%d kg]\n\r"
            "[v1] Flags:      [%s]\n\r"
            "[v2] Key:     %s [%d]\n\r"
            "[v3] Capacity    [%d]\n\r"
            "[v4] Weight Mult [%d]\n\r",
            obj->value[0],
            flag_string(container_flag_table, obj->value[1]),
            get_object_prototype(obj->value[2])
            ? get_object_prototype(obj->value[2])->short_descr
            : "none",
            obj->value[2],
            obj->value[3],
            obj->value[4]);
        send_to_char(buf, ch);
        break;

    case ITEM_DRINK_CON:
        sprintf(buf,
            "[v0] Liquid Total: [%d]\n\r"
            "[v1] Liquid Left:  [%d]\n\r"
            "[v2] Liquid:       %s\n\r"
            "[v3] Poisoned:     %s\n\r",
            obj->value[0],
            obj->value[1],
            liquid_table[obj->value[2]].name,
            obj->value[3] != 0 ? "Yes" : "No");
        send_to_char(buf, ch);
        break;

    case ITEM_FOUNTAIN:
        sprintf(buf,
            "[v0] Liquid Total: [%d]\n\r"
            "[v1] Liquid Left:  [%d]\n\r"
            "[v2] Liquid:       %s\n\r",
            obj->value[0],
            obj->value[1],
            liquid_table[obj->value[2]].name);
        send_to_char(buf, ch);
        break;

    case ITEM_FOOD:
        sprintf(buf,
            "[v0] Food hours: [%d]\n\r"
            "[v1] Full hours: [%d]\n\r"
            "[v3] Poisoned:   %s\n\r",
            obj->value[0],
            obj->value[1],
            obj->value[3] != 0 ? "Yes" : "No");
        send_to_char(buf, ch);
        break;

    case ITEM_MONEY:
        sprintf(buf, "[v0] Gold:   [%d]\n\r"
            "[v1] Silver: [%d]\n\r",
            obj->value[0],
            obj->value[1]);
        send_to_char(buf, ch);
        break;
    }

    return;
}



bool set_obj_values(CharData* ch, ObjectPrototype* pObj, int value_num, char* argument)
{
    int tmp;

    switch (pObj->item_type) {
    default:
        break;

    case ITEM_LIGHT:
        switch (value_num) {
        default:
            do_help(ch, "ITEM_LIGHT");
            return false;
        case 2:
            send_to_char("HOURS OF LIGHT SET.\n\r\n\r", ch);
            pObj->value[2] = atoi(argument);
            break;
        }
        break;

    case ITEM_WAND:
    case ITEM_STAFF:
        switch (value_num) {
        default:
            do_help(ch, "ITEM_STAFF_WAND");
            return false;
        case 0:
            send_to_char("SPELL LEVEL SET.\n\r\n\r", ch);
            pObj->value[0] = atoi(argument);
            break;
        case 1:
            send_to_char("TOTAL NUMBER OF CHARGES SET.\n\r\n\r", ch);
            pObj->value[1] = atoi(argument);
            break;
        case 2:
            send_to_char("CURRENT NUMBER OF CHARGES SET.\n\r\n\r", ch);
            pObj->value[2] = atoi(argument);
            break;
        case 3:
            send_to_char("SPELL TYPE SET.\n\r", ch);
            pObj->value[3] = skill_lookup(argument);
            break;
        }
        break;

    case ITEM_SCROLL:
    case ITEM_POTION:
    case ITEM_PILL:
        switch (value_num) {
        default:
            do_help(ch, "ITEM_SCROLL_POTION_PILL");
            return false;
        case 0:
            send_to_char("SPELL LEVEL SET.\n\r\n\r", ch);
            pObj->value[0] = atoi(argument);
            break;
        case 1:
            send_to_char("SPELL TYPE 1 SET.\n\r\n\r", ch);
            pObj->value[1] = skill_lookup(argument);
            break;
        case 2:
            send_to_char("SPELL TYPE 2 SET.\n\r\n\r", ch);
            pObj->value[2] = skill_lookup(argument);
            break;
        case 3:
            send_to_char("SPELL TYPE 3 SET.\n\r\n\r", ch);
            pObj->value[3] = skill_lookup(argument);
            break;
        case 4:
            send_to_char("SPELL TYPE 4 SET.\n\r\n\r", ch);
            pObj->value[4] = skill_lookup(argument);
            break;
        }
        break;

/* ARMOR for ROM: */

    case ITEM_ARMOR:
        switch (value_num) {
        default:
            do_help(ch, "ITEM_ARMOR");
            return false;
        case 0:
            send_to_char("AC PIERCE SET.\n\r\n\r", ch);
            pObj->value[0] = atoi(argument);
            break;
        case 1:
            send_to_char("AC BASH SET.\n\r\n\r", ch);
            pObj->value[1] = atoi(argument);
            break;
        case 2:
            send_to_char("AC SLASH SET.\n\r\n\r", ch);
            pObj->value[2] = atoi(argument);
            break;
        case 3:
            send_to_char("AC EXOTIC SET.\n\r\n\r", ch);
            pObj->value[3] = atoi(argument);
            break;
        }
        break;

/* WEAPONS changed in ROM */

    case ITEM_WEAPON:
        switch (value_num) {
        default:
            do_help(ch, "ITEM_WEAPON");
            return false;
        case 0:
            send_to_char("WEAPON CLASS SET.\n\r\n\r", ch);
            pObj->value[0] = flag_value(weapon_class, argument);
            break;
        case 1:
            send_to_char("NUMBER OF DICE SET.\n\r\n\r", ch);
            pObj->value[1] = atoi(argument);
            break;
        case 2:
            send_to_char("TYPE OF DICE SET.\n\r\n\r", ch);
            pObj->value[2] = atoi(argument);
            break;
        case 3:
            send_to_char("WEAPON TYPE SET.\n\r\n\r", ch);
            pObj->value[3] = attack_lookup(argument);
            break;
        case 4:
            send_to_char("SPECIAL WEAPON TYPE TOGGLED.\n\r\n\r", ch);
            pObj->value[4] ^= (flag_value(weapon_type2, argument) != NO_FLAG
                ? flag_value(weapon_type2, argument) : 0);
            break;
        }
        break;

    case ITEM_PORTAL:
        switch (value_num) {
        default:
            do_help(ch, "ITEM_PORTAL");
            return false;

        case 0:
            send_to_char("CHARGES SET.\n\r\n\r", ch);
            pObj->value[0] = atoi(argument);
            break;
        case 1:
            send_to_char("EXIT FLAGS SET.\n\r\n\r", ch);
            pObj->value[1] = ((tmp = flag_value(exit_flag_table, argument)) == NO_FLAG ? 0 : tmp);
            break;
        case 2:
            send_to_char("PORTAL FLAGS SET.\n\r\n\r", ch);
            pObj->value[2] = ((tmp = flag_value(portal_flag_table, argument)) == NO_FLAG ? 0 : tmp);
            break;
        case 3:
            send_to_char("EXIT VNUM SET.\n\r\n\r", ch);
            pObj->value[3] = atoi(argument);
            break;
        }
        break;

    case ITEM_FURNITURE:
        switch (value_num) {
        default:
            do_help(ch, "ITEM_FURNITURE");
            return false;

        case 0:
            send_to_char("NUMBER OF PEOPLE SET.\n\r\n\r", ch);
            pObj->value[0] = atoi(argument);
            break;
        case 1:
            send_to_char("MAX WEIGHT SET.\n\r\n\r", ch);
            pObj->value[1] = atoi(argument);
            break;
        case 2:
            send_to_char("FURNITURE FLAGS TOGGLED.\n\r\n\r", ch);
            pObj->value[2] ^= (flag_value(furniture_flag_table, argument) != NO_FLAG
                ? flag_value(furniture_flag_table, argument) : 0);
            break;
        case 3:
            send_to_char("HEAL BONUS SET.\n\r\n\r", ch);
            pObj->value[3] = atoi(argument);
            break;
        case 4:
            send_to_char("MANA BONUS SET.\n\r\n\r", ch);
            pObj->value[4] = atoi(argument);
            break;
        }
        break;

    case ITEM_CONTAINER:
        switch (value_num) {
            int value;

        default:
            do_help(ch, "ITEM_CONTAINER");
            return false;
        case 0:
            send_to_char("WEIGHT CAPACITY SET.\n\r\n\r", ch);
            pObj->value[0] = atoi(argument);
            break;
        case 1:
            if ((value = flag_value(container_flag_table, argument))
                != NO_FLAG)
                TOGGLE_BIT(pObj->value[1], value);
            else {
                do_help(ch, "ITEM_CONTAINER");
                return false;
            }
            send_to_char("CONTAINER TYPE SET.\n\r\n\r", ch);
            break;
        case 2:
            if (atoi(argument) != 0) {
                if (!get_object_prototype(atoi(argument))) {
                    send_to_char("THERE IS NO SUCH ITEM.\n\r\n\r", ch);
                    return false;
                }

                if (get_object_prototype(atoi(argument))->item_type != ITEM_KEY) {
                    send_to_char("THAT ITEM IS NOT A KEY.\n\r\n\r", ch);
                    return false;
                }
            }
            send_to_char("CONTAINER KEY SET.\n\r\n\r", ch);
            pObj->value[2] = atoi(argument);
            break;
        case 3:
            send_to_char("CONTAINER MAX WEIGHT SET.\n\r", ch);
            pObj->value[3] = atoi(argument);
            break;
        case 4:
            send_to_char("WEIGHT MULTIPLIER SET.\n\r\n\r", ch);
            pObj->value[4] = atoi(argument);
            break;
        }
        break;

    case ITEM_DRINK_CON:
        switch (value_num) {
        default:
            do_help(ch, "ITEM_DRINK");
/* OLC            do_help( ch, "liquids" );    */
            return false;
        case 0:
            send_to_char("MAXIMUM AMOUT OF LIQUID HOURS SET.\n\r\n\r", ch);
            pObj->value[0] = atoi(argument);
            break;
        case 1:
            send_to_char("CURRENT AMOUNT OF LIQUID HOURS SET.\n\r\n\r", ch);
            pObj->value[1] = atoi(argument);
            break;
        case 2:
            send_to_char("LIQUID TYPE SET.\n\r\n\r", ch);
            pObj->value[2] = (liquid_lookup(argument) != -1 ?
                liquid_lookup(argument) : 0);
            break;
        case 3:
            send_to_char("POISON VALUE TOGGLED.\n\r\n\r", ch);
            pObj->value[3] = (pObj->value[3] == 0) ? 1 : 0;
            break;
        }
        break;

    case ITEM_FOUNTAIN:
        switch (value_num) {
        default:
            do_help(ch, "ITEM_FOUNTAIN");
/* OLC            do_help( ch, "liquids" );    */
            return false;
        case 0:
            send_to_char("LIQUID MAXIMUM SET.\n\r\n\r", ch);
            pObj->value[0] = atoi(argument);
            break;
        case 1:
            send_to_char("LIQUID LEFT SET.\n\r\n\r", ch);
            pObj->value[1] = atoi(argument);
            break;
        case 2:
            send_to_char("LIQUID TYPE SET.\n\r\n\r", ch);
            pObj->value[2] = (liquid_lookup(argument) != -1 ?
                liquid_lookup(argument) : 0);
            break;
        }
        break;

    case ITEM_FOOD:
        switch (value_num) {
        default:
            do_help(ch, "ITEM_FOOD");
            return false;
        case 0:
            send_to_char("HOURS OF FOOD SET.\n\r\n\r", ch);
            pObj->value[0] = atoi(argument);
            break;
        case 1:
            send_to_char("HOURS OF FULL SET.\n\r\n\r", ch);
            pObj->value[1] = atoi(argument);
            break;
        case 3:
            send_to_char("POISON VALUE TOGGLED.\n\r\n\r", ch);
            pObj->value[3] = (pObj->value[3] == 0) ? 1 : 0;
            break;
        }
        break;

    case ITEM_MONEY:
        switch (value_num) {
        default:
            do_help(ch, "ITEM_MONEY");
            return false;
        case 0:
            send_to_char("GOLD AMOUNT SET.\n\r\n\r", ch);
            pObj->value[0] = atoi(argument);
            break;
        case 1:
            send_to_char("SILVER AMOUNT SET.\n\r\n\r", ch);
            pObj->value[1] = atoi(argument);
            break;
        }
        break;
    }

    show_obj_values(ch, pObj);

    return true;
}

OEDIT(oedit_show)
{
    ObjectPrototype* pObj;
    char buf[MAX_STRING_LENGTH];
    AffectData* paf;
    int cnt;

    READ_ARG(buf);

    if (IS_NULLSTR(buf)) {
        if (ch->desc->editor == ED_OBJECT)
            EDIT_OBJ(ch, pObj);
        else {
            send_to_char("Syntax : oshow [vnum]\n\r", ch);
            return false;
        }
    }
    else {
        pObj = get_object_prototype(atoi(buf));

        if (!pObj) {
            send_to_char("ERROR : That object does not exist.\n\r", ch);
            return false;
        }

        if (!IS_BUILDER(ch, pObj->area)) {
            send_to_char("ERROR : You do not have access to the area that obj is in.\n\r", ch);
            return false;
        }
    }

    sprintf(buf, "Name:        [%s]\n\rArea:        [%5d] %s\n\r",
        pObj->name,
        !pObj->area ? -1 : pObj->area->vnum,
        !pObj->area ? "No Area" : pObj->area->name);
    send_to_char(buf, ch);


    sprintf(buf, "Vnum:        [%5d]\n\rType:        [%s]\n\r",
        pObj->vnum,
        flag_string(type_flag_table, pObj->item_type));
    send_to_char(buf, ch);

    sprintf(buf, "Level:       [%5d]\n\r", pObj->level);
    send_to_char(buf, ch);

    sprintf(buf, "Wear flags:  [%s]\n\r",
        flag_string(wear_flag_table, pObj->wear_flags));
    send_to_char(buf, ch);

    sprintf(buf, "Extra flags: [%s]\n\r",
        flag_string(extra_flag_table, pObj->extra_flags));
    send_to_char(buf, ch);

    sprintf(buf, "Material:    [%s]\n\r",                /* ROM */
        pObj->material);
    send_to_char(buf, ch);

    sprintf(buf, "Condition:   [%5d]\n\r",               /* ROM */
        pObj->condition);
    send_to_char(buf, ch);

    sprintf(buf, "Weight:      [%5d]\n\rCost:        [%5d]\n\r",
        pObj->weight, pObj->cost);
    send_to_char(buf, ch);

    if (pObj->extra_desc) {
        ExtraDesc* ed;

        send_to_char("Ex desc kwd: ", ch);

        FOR_EACH(ed, pObj->extra_desc) {
            send_to_char("[", ch);
            send_to_char(ed->keyword, ch);
            send_to_char("]", ch);
        }

        send_to_char("\n\r", ch);
    }

    sprintf(buf, "Short desc:  %s\n\rLong desc:\n\r     %s\n\r",
        pObj->short_descr, pObj->description);
    send_to_char(buf, ch);

    for (cnt = 0, paf = pObj->affected; paf; NEXT_LINK(paf)) {
        if (cnt == 0) {
            send_to_char("Number Modifier Affects      Adds\n\r", ch);
            send_to_char("------ -------- ------------ ----\n\r", ch);
        }
        sprintf(buf, "[%4d] %-8d %-12s ", cnt,
            paf->modifier,
            flag_string(apply_flag_table, paf->location));
        send_to_char(buf, ch);
        sprintf(buf, "%s ", flag_string(bitvector_type[paf->where].table, paf->bitvector));
        send_to_char(buf, ch);
        sprintf(buf, "%s\n\r", flag_string(apply_types, paf->where));
        send_to_char(buf, ch);
        cnt++;
    }

    show_obj_values(ch, pObj);

    return false;
}


/*
 * Need to issue warning if flag isn't valid. -- does so now -- Hugin.
 */
OEDIT(oedit_addaffect)
{
    int value;
    ObjectPrototype* pObj;
    AffectData* pAf;
    char loc[MAX_STRING_LENGTH];
    char mod[MAX_STRING_LENGTH];

    EDIT_OBJ(ch, pObj);

    READ_ARG(loc);
    one_argument(argument, mod);

    if (loc[0] == '\0' || mod[0] == '\0' || !is_number(mod)) {
        send_to_char("Syntax:  addaffect [location] [#xmod]\n\r", ch);
        return false;
    }

    if ((value = flag_value(apply_flag_table, loc)) == NO_FLAG) /* Hugin */
    {
        send_to_char("Valid affects are:\n\r", ch);
        show_help(ch, "apply");
        return false;
    }

    pAf = new_affect();
    pAf->location = (LEVEL)value;
    pAf->modifier = (int16_t)atoi(mod);
    pAf->where = TO_OBJECT;
    pAf->type = -1;
    pAf->duration = -1;
    pAf->bitvector = 0;
    pAf->level = pObj->level;
    pAf->next = pObj->affected;
    pObj->affected = pAf;

    send_to_char("Affect added.\n\r", ch);
    return true;
}

OEDIT(oedit_addapply)
{
    bool rc = true;
    int value, bv, typ;
    ObjectPrototype* pObj;
    AffectData* pAf;
    INIT_BUF(loc, MAX_STRING_LENGTH);
    INIT_BUF(mod, MAX_STRING_LENGTH);
    INIT_BUF(type, MAX_STRING_LENGTH);
    INIT_BUF(bvector, MAX_STRING_LENGTH);

    EDIT_OBJ(ch, pObj);

    if (IS_NULLSTR(argument)) {
        send_to_char("Syntax:  addapply [type] [location] [#xmod] [bitvector]\n\r", ch);
        rc = false;
        goto oedit_addapply_cleanup;
    }

    READ_ARG(BUF(type));
    READ_ARG(BUF(loc));
    READ_ARG(BUF(mod));
    one_argument(argument, BUF(bvector));

    if (BUF(type)[0] == '\0' || (typ = flag_value(apply_types, BUF(type))) == NO_FLAG) {
        send_to_char("Invalid apply type. Valid apply types are:\n\r", ch);
        show_help(ch, "apptype");
        rc = false;
        goto oedit_addapply_cleanup;
    }

    if (BUF(loc)[0] == '\0' || (value = flag_value(apply_flag_table, BUF(loc))) == NO_FLAG) {
        send_to_char("Valid applys are:\n\r", ch);
        show_help(ch, "apply");
        rc = false;
        goto oedit_addapply_cleanup;
    }

    if (BUF(bvector)[0] == '\0' || (bv = flag_value(bitvector_type[typ].table, BUF(bvector))) == NO_FLAG) {
        send_to_char("Invalid bitvector type.\n\r", ch);
        send_to_char("Valid bitvector types are:\n\r", ch);
        show_help(ch, bitvector_type[typ].help);
        rc = false;
        goto oedit_addapply_cleanup;
    }

    if (BUF(mod)[0] == '\0' || !is_number(BUF(mod))) {
        send_to_char("Syntax:  addapply [type] [location] [#xmod] [bitvector]\n\r", ch);
        rc = false;
        goto oedit_addapply_cleanup;
    }

    pAf = new_affect();
    pAf->location = (int16_t)value;
    pAf->modifier = (int16_t)atoi(BUF(mod));
    pAf->where = (int16_t)apply_flag_table[typ].bit;
    pAf->type = -1;
    pAf->duration = -1;
    pAf->bitvector = bv;
    pAf->level = pObj->level;
    pAf->next = pObj->affected;
    pObj->affected = pAf;

    send_to_char("Apply added.\n\r", ch);

oedit_addapply_cleanup:
    free_buf(loc);
    free_buf(mod);
    free_buf(type);
    free_buf(bvector);

    return rc;
}

/*
 * My thanks to Hans Hvidsten Birkeland and Noam Krendel(Walker)
 * for really teaching me how to manipulate pointers.
 */
OEDIT(oedit_delaffect)
{
    ObjectPrototype* pObj;
    AffectData* pAf;
    AffectData* pAf_next;
    char affect[MAX_STRING_LENGTH];
    int  value;
    int  cnt = 0;

    EDIT_OBJ(ch, pObj);

    one_argument(argument, affect);

    if (!is_number(affect) || affect[0] == '\0') {
        send_to_char("Syntax:  delaffect [#xaffect]\n\r", ch);
        return false;
    }

    value = atoi(affect);

    if (value < 0) {
        send_to_char("Only non-negative affect-numbers allowed.\n\r", ch);
        return false;
    }

    if (!(pAf = pObj->affected)) {
        send_to_char("OEdit:  Non-existant affect.\n\r", ch);
        return false;
    }

    if (value == 0)    /* First case: Remove first affect */
    {
        pAf = pObj->affected;
        pObj->affected = pAf->next;
        free_affect(pAf);
    }
    else        /* Affect to remove is not the first */
    {
        while ((pAf_next = pAf->next) && (++cnt < value))
            pAf = pAf_next;

        if (pAf_next)        /* See if it's the next affect */
        {
            pAf->next = pAf_next->next;
            free_affect(pAf_next);
        }
        else                                 /* Doesn't exist */
        {
            send_to_char("No such affect.\n\r", ch);
            return false;
        }
    }

    send_to_char("Affect removed.\n\r", ch);
    return true;
}

bool set_value(CharData* ch, ObjectPrototype* pObj, char* argument, int value)
{
    if (argument[0] == '\0') {
        set_obj_values(ch, pObj, -1, "");     /* '\0' changed to "" -- Hugin */
        return false;
    }

    if (set_obj_values(ch, pObj, value, argument))
        return true;

    return false;
}



/*****************************************************************************
 Name:        oedit_values
 Purpose:    Finds the object and sets its value.
 Called by:    The four valueX functions below. (now five -- Hugin )
 ****************************************************************************/
bool oedit_values(CharData* ch, char* argument, int value)
{
    ObjectPrototype* pObj;

    EDIT_OBJ(ch, pObj);

    if (set_value(ch, pObj, argument, value))
        return true;

    return false;
}

OEDIT(oedit_create)
{
    ObjectPrototype* pObj;
    AreaData* pArea;
    VNUM  value;
    int  iHash;

    value = STRTOVNUM(argument);
    if (argument[0] == '\0' || value == 0) {
        send_to_char("Syntax:  oedit create [vnum]\n\r", ch);
        return false;
    }

    pArea = get_vnum_area(value);
    if (!pArea) {
        send_to_char("OEdit:  That vnum is not assigned an area.\n\r", ch);
        return false;
    }

    if (!IS_BUILDER(ch, pArea)) {
        send_to_char("OEdit:  Vnum in an area you cannot build in.\n\r", ch);
        return false;
    }

    if (get_object_prototype(value)) {
        send_to_char("OEdit:  Object vnum already exists.\n\r", ch);
        return false;
    }

    pObj = new_object_prototype();
    pObj->vnum = value;
    pObj->area = pArea;
    pObj->extra_flags = 0;

    if (value > top_vnum_obj)
        top_vnum_obj = value;

    iHash = value % MAX_KEY_HASH;
    pObj->next = object_prototype_hash[iHash];
    object_prototype_hash[iHash] = pObj;

    set_editor(ch->desc, ED_OBJECT, U(pObj));

    send_to_char("Object Created.\n\r", ch);
    return true;
}

ED_FUN_DEC(ed_objrecval)
{
    ObjectPrototype* pObj = (ObjectPrototype*)arg;

    switch (pObj->item_type) {
    default:
        send_to_char("You cannot do that to a non-weapon.\n\r", ch);
        return false;

    case ITEM_WEAPON:
        pObj->value[1] = UMIN(pObj->level / 4 + 1, 5);
        pObj->value[2] = (pObj->level + 7) / pObj->value[1];
        break;
    }

    send_to_char("Ok.\n\r", ch);
    return true;
}

ED_FUN_DEC(ed_addapply)
{
    bool rc = true;
    int value, bv, typ;
    ObjectPrototype* pObj = (ObjectPrototype*)arg;
    AffectData* pAf;
    INIT_BUF(loc, MAX_STRING_LENGTH);
    INIT_BUF(mod, MAX_STRING_LENGTH);
    INIT_BUF(type, MAX_STRING_LENGTH);
    INIT_BUF(bvector, MAX_STRING_LENGTH);

    if (IS_NULLSTR(argument)) {
        send_to_char("Syntax:  addapply [type] [location] [#xmod] [bitvector]\n\r", ch);
        rc = false;
        goto ed_addapply_cleanup;
    }

    READ_ARG(BUF(type));
    READ_ARG(BUF(loc));
    READ_ARG(BUF(mod));
    one_argument(argument, BUF(bvector));

    if (BUF(type)[0] == '\0' || (typ = flag_value(apply_types, BUF(type))) == NO_FLAG) {
        send_to_char("Invalid apply type. Valid apply types are:\n\r", ch);
        show_help(ch, "apptype");
        rc = false;
        goto ed_addapply_cleanup;
    }

    if (BUF(loc)[0] == '\0' || (value = flag_value(apply_flag_table, BUF(loc))) == NO_FLAG) {
        send_to_char("Valid applys are:\n\r", ch);
        show_help(ch, "apply");
        rc = false;
        goto ed_addapply_cleanup;
    }

    if (BUF(bvector)[0] == '\0' || (bv = flag_value(bitvector_type[typ].table, BUF(bvector))) == NO_FLAG) {
        send_to_char("Invalid bitvector type.\n\r", ch);
        send_to_char("Valid bitvector types are:\n\r", ch);
        show_help(ch, bitvector_type[typ].help);
        rc = false;
        goto ed_addapply_cleanup;
    }

    if (BUF(mod)[0] == '\0' || !is_number(BUF(mod))) {
        send_to_char("Syntax:  addapply [type] [location] [#xmod] [bitvector]\n\r", ch);
        rc = false;
        goto ed_addapply_cleanup;
    }

    pAf = new_affect();
    pAf->location = (int16_t)value;
    pAf->modifier = (int16_t)atoi(BUF(mod));
    pAf->where = (int16_t)apply_flag_table[typ].bit;
    pAf->type = -1;
    pAf->duration = -1;
    pAf->bitvector = bv;
    pAf->level = pObj->level;
    pAf->next = pObj->affected;
    pObj->affected = pAf;

    send_to_char("Apply added.\n\r", ch);

ed_addapply_cleanup:
    free_buf(loc);
    free_buf(mod);
    free_buf(type);
    free_buf(bvector);

    return rc;
}

ED_FUN_DEC(ed_value)
{
    return oedit_values(ch, argument, (int)par);
}

ED_FUN_DEC(ed_new_obj)
{
    ObjectPrototype* pObj;
    AreaData* pArea;
    VNUM  value;
    int  iHash;

    value = STRTOVNUM(argument);

    if (argument[0] == '\0' || value == 0) {
        send_to_char("Syntax:  oedit create [vnum]\n\r", ch);
        return false;
    }

    pArea = get_vnum_area(value);

    if (!pArea) {
        send_to_char("OEdit:  That vnum is not assigned an area.\n\r", ch);
        return false;
    }

    if (!IS_BUILDER(ch, pArea)) {
        send_to_char("OEdit:  Vnum in an area you cannot build in.\n\r", ch);
        return false;
    }

    if (get_object_prototype(value)) {
        send_to_char("OEdit:  Object vnum already exists.\n\r", ch);
        return false;
    }

    pObj = new_object_prototype();
    pObj->vnum = value;
    pObj->area = pArea;
    pObj->extra_flags = 0;

    if (value > top_vnum_obj)
        top_vnum_obj = value;

    iHash = value % MAX_KEY_HASH;
    pObj->next = object_prototype_hash[iHash];
    object_prototype_hash[iHash] = pObj;

    set_editor(ch->desc, ED_OBJECT, U(pObj));

    send_to_char("Object Created.\n\r", ch);

    return true;
}

ED_FUN_DEC(ed_olist)
{
    ObjectPrototype* obj_proto;
    AreaData* pArea;
    char buf[MAX_STRING_LENGTH];
    Buffer* buf1;
    char blarg[MAX_INPUT_LENGTH];
    bool fAll, found;
    VNUM vnum;
    int  col = 0;

    one_argument(argument, blarg);

    if (blarg[0] == '\0') {
        send_to_char("Syntax : olist <all/name/item_type>\n\r", ch);
        return false;
    }

    pArea = *(AreaData**)arg;
    buf1 = new_buf();
    fAll = !str_cmp(blarg, "all");
    found = false;

    for (vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++) {
        if ((obj_proto = get_object_prototype(vnum))) {
            if (fAll || is_name(blarg, obj_proto->name)
                || (ItemType)flag_value(type_flag_table, blarg) == obj_proto->item_type) {
                found = true;
                sprintf(buf, "[%5d] %-17.16s",
                    obj_proto->vnum, capitalize(obj_proto->short_descr));
                add_buf(buf1, buf);
                if (++col % 3 == 0)
                    add_buf(buf1, "\n\r");
            }
        }
    }

    if (!found) {
        send_to_char("Object(s) not found in this area.\n\r", ch);
        return false;
    }

    if (col % 3 != 0)
        add_buf(buf1, "\n\r");

    page_to_char(BUF(buf1), ch);
    free_buf(buf1);

    return false;
}
