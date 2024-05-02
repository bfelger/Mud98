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

#include "entities/object.h"

#define OEDIT(fun) bool fun( Mobile *ch, char *argument )

ObjPrototype xObj;

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
    { "copy",	    0,				        ed_olded,		    U(oedit_copy)	},
    { "commands",	0,				        ed_olded,		    U(show_commands)},
    { "?",		    0,				        ed_olded,		    U(show_help)	},
    { "version",	0,				        ed_olded,		    U(show_version)	},
    { NULL,	        0,				        NULL,			    0		        }
};

/* Entry point for editing object_prototype_data. */
void do_oedit(Mobile* ch, char* argument)
{
    ObjPrototype* pObj;
    AreaData* area;
    char arg1[MAX_STRING_LENGTH];
    int  value;

    if (IS_NPC(ch) || ch->desc == NULL)
        return;

    READ_ARG(arg1);

    if (is_number(arg1)) {
        value = atoi(arg1);
        if (!(pObj = get_object_prototype(value))) {
            send_to_char("{jOEdit:  That vnum does not exist.{x\n\r", ch);
            return;
        }

        if (!IS_BUILDER(ch, pObj->area)) {
            send_to_char("{jYou do not have enough security to edit objects.{x\n\r", ch);
            return;
        }

        set_editor(ch->desc, ED_OBJECT, U(pObj));
        oedit_show(ch, "");
        return;
    }
    else {
        if (!str_cmp(arg1, "create")) {
            value = atoi(argument);
            if (argument[0] == '\0' || value == 0) {
                send_to_char("{jSyntax:  {*EDIT OBJECT CREATE [VNUM]{x\n\r", ch);
                return;
            }

            area = get_vnum_area(value);

            if (!area) {
                send_to_char("{jOEdit:  That vnum is not assigned an area.{x\n\r", ch);
                return;
            }

            if (!IS_BUILDER(ch, area)) {
                send_to_char("{jYou do not have enough security to edit objects.{x\n\r", ch);
                return;
            }

            if (oedit_create(ch, argument)) {
                SET_BIT(area->area_flags, AREA_CHANGED);
                ch->desc->editor = ED_OBJECT;
                oedit_show(ch, "");
            }
            return;
        }
    }

    send_to_char("{jOEdit:  There is no default object to edit.{x\n\r", ch);
    return;
}

/* Object Interpreter, called by do_oedit. */
void oedit(Mobile* ch, char* argument)
{
    AreaData* area;
    ObjPrototype* pObj;

    EDIT_OBJ(ch, pObj);
    area = pObj->area;

    if (!IS_BUILDER(ch, area)) {
        send_to_char("{jOEdit: Insufficient security to modify area.{x\n\r", ch);
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

// Object Editor Functions.
void show_obj_values(Mobile* ch, ObjPrototype* obj)
{
    switch (obj->item_type) {
    default:    /* No values. */
        break;

    case ITEM_LIGHT:
        if (obj->value[2] == -1 || obj->value[2] == 999) /* ROM OLC */
            printf_to_char(ch, "{|[{*v2{|]{x Light:  {|[{*-1{|] {_(infinite){x\n\r");
        else
            printf_to_char(ch, "{|[{*v2{|]{x Light:  {|[{*%d{|]{x\n\r", obj->value[2]);
        break;

    case ITEM_WAND:
    case ITEM_STAFF:
        printf_to_char(ch,
            "{|[{*v0{|]{x Level:          {|[{*%d{|]{x\n\r"
            "{|[{*v1{|]{x Charges Total:  {|[{*%d{|]{x\n\r"
            "{|[{*v2{|]{x Charges Left:   {|[{*%d{|]{x\n\r"
            "{|[{*v3{|]{x Spell:          {_%s{x\n\r",
            obj->value[0],
            obj->value[1],
            obj->value[2],
            obj->value[3] != -1 ? skill_table[obj->value[3]].name
            : "none");
        break;

    case ITEM_PORTAL:
        printf_to_char(ch,
            "{|[{*v0{|]{x Charges:        {|[{*%d{|]{x\n\r"
            "{|[{*v1{|]{x Exit Flags:     {|[{*%s{|]{x\n\r"
            "{|[{*v2{|]{x Portal Flags:   {|[{*%s{|]{x\n\r"
            "{|[{*v3{|]{x Goes to VNUM:   {|[{*%d{|]{x\n\r",
            obj->value[0],
            flag_string(exit_flag_table, obj->value[1]),
            flag_string(portal_flag_table, obj->value[2]),
            obj->value[3]);
        break;

    case ITEM_FURNITURE:
        printf_to_char(ch,
            "{|[{*v0{|]{x Max people:      {|[{*%d{|]{x\n\r"
            "{|[{*v1{|]{x Max weight:      {|[{*%d{|]{x\n\r"
            "{|[{*v2{|]{x Furniture Flags: {|[{*%s{|]{x\n\r"
            "{|[{*v3{|]{x Heal bonus:      {|[{*%d{|]{x\n\r"
            "{|[{*v4{|]{x Mana bonus:      {|[{*%d{|]{x\n\r",
            obj->value[0],
            obj->value[1],
            flag_string(furniture_flag_table, obj->value[2]),
            obj->value[3],
            obj->value[4]);
        break;

    case ITEM_SCROLL:
    case ITEM_POTION:
    case ITEM_PILL:
        printf_to_char(ch,
            "{|[{*v0{|]{x Level:  {|[{*%d{|]{x\n\r"
            "{|[{*v1{|]{x Spell:  {_%s{x\n\r"
            "{|[{*v2{|]{x Spell:  {_%s{x\n\r"
            "{|[{*v3{|]{x Spell:  {_%s{x\n\r"
            "{|[{*v4{|]{x Spell:  {_%s{x\n\r",
            obj->value[0],
            obj->value[1] != -1 ? skill_table[obj->value[1]].name : "none",
            obj->value[2] != -1 ? skill_table[obj->value[2]].name : "none",
            obj->value[3] != -1 ? skill_table[obj->value[3]].name : "none",
            obj->value[4] != -1 ? skill_table[obj->value[4]].name : "none");
        break;

/* ARMOR for ROM */

    case ITEM_ARMOR:
        printf_to_char(ch,
            "{|[{*v0{|]{x AC pierce       {|[{*%d{|]{x\n\r"
            "{|[{*v1{|]{x AC bash         {|[{*%d{|]{x\n\r"
            "{|[{*v2{|]{x AC slash        {|[{*%d{|]{x\n\r"
            "{|[{*v3{|]{x AC exotic       {|[{*%d{|]{x\n\r",
            obj->value[0],
            obj->value[1],
            obj->value[2],
            obj->value[3]);
        break;

/* WEAPON changed in ROM: */
/* I had to split the output here, I have no idea why, but it helped -- Hugin */
/* It somehow fixed a bug in showing scroll/pill/potions too ?! */
    case ITEM_WEAPON:
        printf_to_char(ch, "{|[{*v0{|]{x Weapon class:   {|[{*%s{|]{x\n\r", flag_string(weapon_class, obj->value[0]));
        printf_to_char(ch, "{|[{*v1{|]{x Number of dice: {|[{*%d{|]{x\n\r", obj->value[1]);
        printf_to_char(ch, "{|[{*v2{|]{x Type of dice:   {|[{*%d{|]{x\n\r", obj->value[2]);
        printf_to_char(ch, "{|[{*v3{|]{x Type:           {|[{*%s{|]{x\n\r", attack_table[obj->value[3]].name);
        printf_to_char(ch, "{|[{*v4{|]{x Special type:   {|[{*%s{|]{x\n\r", flag_string(weapon_type2, obj->value[4]));
        break;

    case ITEM_CONTAINER:
        printf_to_char(ch,
            "{|[{*v0{|]{x Weight:     {|[{*%d kg{|]{x\n\r"
            "{|[{*v1{|]{x Flags:      {|[{*%s{|]{x\n\r"
            "{|[{*v2{|]{x Key:        {|[{*%d{|]{_ %s{x\n\r"
            "{|[{*v3{|]{x Capacity    {|[{*%d{|]{x\n\r"
            "{|[{*v4{|]{x Weight Mult {|[{*%d{|]{x\n\r",
            obj->value[0],
            flag_string(container_flag_table, obj->value[1]),
            obj->value[2],
            get_object_prototype(obj->value[2])
            ? get_object_prototype(obj->value[2])->short_descr
            : "none",
            obj->value[3],
            obj->value[4]);
        break;

    case ITEM_DRINK_CON:
        printf_to_char(ch,
            "{|[{*v0{|]{x Liquid Total: {|[{*%d{|]{x\n\r"
            "{|[{*v1{|]{x Liquid Left:  {|[{*%d{|]{x\n\r"
            "{|[{*v2{|]{x Liquid:       {|[{*%s{|]{x\n\r"
            "{|[{*v3{|]{x Poisoned:     {|[{*%s{|]{x\n\r",
            obj->value[0],
            obj->value[1],
            liquid_table[obj->value[2]].name,
            obj->value[3] != 0 ? "Yes" : "No");
        break;

    case ITEM_FOUNTAIN:
        printf_to_char(ch,
            "{|[{*v0{|]{x Liquid Total: {|[{*%d{|]{x\n\r"
            "{|[{*v1{|]{x Liquid Left:  {|[{*%d{|]{x\n\r"
            "{|[{*v2{|]{x Liquid:       {|[{*%s{|]{x\n\r",
            obj->value[0],
            obj->value[1],
            liquid_table[obj->value[2]].name);
        break;

    case ITEM_FOOD:
        printf_to_char(ch,
            "{|[{*v0{|]{x Food hours:   {|[{*%d{|]{x\n\r"
            "{|[{*v1{|]{x Full hours:   {|[{*%d{|]{x\n\r"
            "{|[{*v3{|]{x Poisoned:     {|[{*%s{|]{x\n\r",
            obj->value[0],
            obj->value[1],
            obj->value[3] != 0 ? "Yes" : "No");
        break;

    case ITEM_MONEY:
        printf_to_char(ch,
            "{|[{*v0{|]{x Gold:         {|[{*%d{|]{x\n\r"
            "{|[{*v1{|]{x Silver:       {|[{*%d{|]{x\n\r",
            obj->value[0],
            obj->value[1]);
        break;
    }

    return;
}

bool set_obj_values(Mobile* ch, ObjPrototype* pObj, int value_num, char* argument)
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
    ObjPrototype* pObj;
    char buf[MAX_STRING_LENGTH];
    Affect* affect;
    int cnt;

    READ_ARG(buf);

    if (IS_NULLSTR(buf)) {
        if (ch->desc->editor == ED_OBJECT)
            EDIT_OBJ(ch, pObj);
        else {
            send_to_char("{jSyntax : {*OSHOW [VNUM]{x\n\r", ch);
            return false;
        }
    }
    else {
        pObj = get_object_prototype(atoi(buf));

        if (!pObj) {
            send_to_char("{jERROR: That object does not exist.{x\n\r", ch);
            return false;
        }

        if (!IS_BUILDER(ch, pObj->area)) {
            send_to_char("{jERROR: You do not have access to the area that obj is in.{x\n\r", ch);
            return false;
        }
    }

    printf_to_char(ch, "Name:        {|[{*%s{|]{x\n\r", pObj->name);
    printf_to_char(ch, "Area:        {|[{*%5d{|]{_ %s{x\n\r",
        !pObj->area ? -1 : pObj->area->vnum,
        !pObj->area ? "No Area" : C_STR(pObj->area->name));
    printf_to_char(ch, "Vnum:        {|[{*%5d{|]{x\n\r", pObj->vnum);
    printf_to_char(ch, "Type:        {|[{*%s{|]{x\n\r", flag_string(type_flag_table, pObj->item_type));
    printf_to_char(ch, "Level:       {|[{*%5d{|]{x\n\r", pObj->level);
    printf_to_char(ch, "Wear flags:  {|[{*%s{|]{x\n\r", flag_string(wear_flag_table, pObj->wear_flags));
    printf_to_char(ch, "Extra flags: {|[{*%s{|]{x\n\r", flag_string(extra_flag_table, pObj->extra_flags));
    printf_to_char(ch, "Material:    {|[{*%s{|]{x\n\r", pObj->material);
    printf_to_char(ch, "Condition:   {|[{*%5d{|]{x\n\r", pObj->condition);
    printf_to_char(ch, "Weight:      {|[{*%5d{|]{x\n\r", pObj->weight);
    printf_to_char(ch, "Cost:        {|[{*%5d{|]{x\n\r", pObj->cost);

    if (pObj->extra_desc) {
        ExtraDesc* ed;

        send_to_char("Ex desc kwd: ", ch);

        FOR_EACH(ed, pObj->extra_desc) {
            send_to_char("{|[{*", ch);
            send_to_char(ed->keyword, ch);
            send_to_char("{|]{x", ch);
        }

        send_to_char("\n\r", ch);
    }

    printf_to_char(ch, "Short desc:  {_%s{x\n\r", pObj->short_descr);
    printf_to_char(ch, "Long desc:\n\r     {_%s{x\n\r", pObj->description);

    for (cnt = 0, affect = pObj->affected; affect; NEXT_LINK(affect)) {
        if (cnt == 0) {
            send_to_char("{TNumber Modifier Affects      Adds\n\r", ch);
            send_to_char("{=------ -------- ------------ ----\n\r", ch);
        }
        printf_to_char(ch, "{|[{*%4d{|]{* %-8d %-12s ", cnt, affect->modifier, flag_string(apply_flag_table, affect->location));
        printf_to_char(ch, "%s ", flag_string(bitvector_type[affect->where].table, affect->bitvector));
        printf_to_char(ch, "%s{x\n\r", flag_string(apply_types, affect->where));
        cnt++;
    }

    show_obj_values(ch, pObj);

    return false;
}

// Need to issue warning if flag isn't valid. -- does so now -- Hugin.
OEDIT(oedit_addaffect)
{
    int value;
    ObjPrototype* pObj;
    Affect* pAf;
    char loc[MAX_STRING_LENGTH];
    char mod[MAX_STRING_LENGTH];

    EDIT_OBJ(ch, pObj);

    READ_ARG(loc);
    one_argument(argument, mod);

    if (loc[0] == '\0' || mod[0] == '\0' || !is_number(mod)) {
        send_to_char("{jSyntax:  ADDAFFECT [LOCATION] [# MOD]{x\n\r", ch);
        return false;
    }

    if ((value = flag_value(apply_flag_table, loc)) == NO_FLAG) {
        send_to_char("{jValid affects are:{x\n\r", ch);
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

    send_to_char("{jAffect added.{x\n\r", ch);
    return true;
}

OEDIT(oedit_addapply)
{
    bool rc = true;
    int value, bv, typ;
    ObjPrototype* pObj;
    Affect* pAf;
    INIT_BUF(loc, MAX_STRING_LENGTH);
    INIT_BUF(mod, MAX_STRING_LENGTH);
    INIT_BUF(type, MAX_STRING_LENGTH);
    INIT_BUF(bvector, MAX_STRING_LENGTH);

    EDIT_OBJ(ch, pObj);

    if (IS_NULLSTR(argument)) {
        send_to_char("{jSyntax:  {*ADDAPPLY [TYPE] [LOCATION] [# MOD] [BITVECTOR]{x\n\r", ch);
        rc = false;
        goto oedit_addapply_cleanup;
    }

    READ_ARG(BUF(type));
    READ_ARG(BUF(loc));
    READ_ARG(BUF(mod));
    one_argument(argument, BUF(bvector));

    if (BUF(type)[0] == '\0' || (typ = flag_value(apply_types, BUF(type))) == NO_FLAG) {
        send_to_char("{jInvalid apply type. Valid apply types are:{x\n\r", ch);
        show_help(ch, "apptype");
        rc = false;
        goto oedit_addapply_cleanup;
    }

    if (BUF(loc)[0] == '\0' || (value = flag_value(apply_flag_table, BUF(loc))) == NO_FLAG) {
        send_to_char("{jValid applys are:{x\n\r", ch);
        show_help(ch, "apply");
        rc = false;
        goto oedit_addapply_cleanup;
    }

    if (BUF(bvector)[0] == '\0' || (bv = flag_value(bitvector_type[typ].table, BUF(bvector))) == NO_FLAG) {
        send_to_char("{jInvalid bitvector type.\n\r", ch);
        send_to_char("Valid bitvector types are:{x\n\r", ch);
        show_help(ch, bitvector_type[typ].help);
        rc = false;
        goto oedit_addapply_cleanup;
    }

    if (BUF(mod)[0] == '\0' || !is_number(BUF(mod))) {
        send_to_char("{jSyntax:  {*ADDAPPLY [TYPE] [LOCATION] [# MOD] [BITVECTOR]{x\n\r", ch);
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

    send_to_char("{jApply added.{x\n\r", ch);

oedit_addapply_cleanup:
    free_buf(loc);
    free_buf(mod);
    free_buf(type);
    free_buf(bvector);

    return rc;
}

// My thanks to Hans Hvidsten Birkeland and Noam Krendel(Walker)
// for really teaching me how to manipulate pointers.
OEDIT(oedit_delaffect)
{
    ObjPrototype* pObj;
    Affect* pAf;
    Affect* pAf_next;
    char affect[MAX_STRING_LENGTH];
    int  value;
    int  cnt = 0;

    EDIT_OBJ(ch, pObj);

    one_argument(argument, affect);

    if (!is_number(affect) || affect[0] == '\0') {
        send_to_char("{jSyntax:  DELAFFECT [# AFFECT]{*\n\r", ch);
        return false;
    }

    value = atoi(affect);

    if (value < 0) {
        send_to_char("{jAffect list references are only positive numbers.{x\n\r", ch);
        return false;
    }

    if (!(pAf = pObj->affected)) {
        send_to_char("{jOEdit: Non-existant affect.{x\n\r", ch);
        return false;
    }

    if (value == 0) {
        // First case: Remove first affect
        pAf = pObj->affected;
        pObj->affected = pAf->next;
        free_affect(pAf);
    }
    else {
        // Affect to remove is not the first
        while ((pAf_next = pAf->next) && (++cnt < value))
            pAf = pAf_next;

        if (pAf_next) {
            // See if it's the next affect
            pAf->next = pAf_next->next;
            free_affect(pAf_next);
        }
        else {
            // Doesn't exist
            send_to_char("{jNo such affect.{x\n\r", ch);
            return false;
        }
    }

    send_to_char("{jAffect removed.{x\n\r", ch);
    return true;
}

bool set_value(Mobile* ch, ObjPrototype* pObj, char* argument, int value)
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
bool oedit_values(Mobile* ch, char* argument, int value)
{
    ObjPrototype* pObj;

    EDIT_OBJ(ch, pObj);

    if (set_value(ch, pObj, argument, value))
        return true;

    return false;
}

OEDIT(oedit_create)
{
    ObjPrototype* pObj;
    AreaData* area;
    VNUM  value;
    int  hash;

    value = STRTOVNUM(argument);
    if (argument[0] == '\0' || value == 0) {
        send_to_char("{jSyntax:  {*OEDIT CREATE [VNUM]{x\n\r", ch);
        return false;
    }

    area = get_vnum_area(value);
    if (!area) {
        send_to_char("{jOEdit:  That vnum is not assigned an area.{x\n\r", ch);
        return false;
    }

    if (!IS_BUILDER(ch, area)) {
        send_to_char("{jOEdit:  Vnum in an area you cannot build in.{x\n\r", ch);
        return false;
    }

    if (get_object_prototype(value)) {
        send_to_char("{jOEdit:  Object vnum already exists.{x\n\r", ch);
        return false;
    }

    pObj = new_object_prototype();
    pObj->vnum = value;
    pObj->area = area;
    pObj->extra_flags = 0;

    if (value > top_vnum_obj)
        top_vnum_obj = value;

    hash = value % MAX_KEY_HASH;
    pObj->next = obj_proto_hash[hash];
    obj_proto_hash[hash] = pObj;

    set_editor(ch->desc, ED_OBJECT, U(pObj));

    send_to_char("{jObject Created.{x\n\r", ch);
    return true;
}

ED_FUN_DEC(ed_objrecval)
{
    ObjPrototype* pObj = (ObjPrototype*)arg;

    switch (pObj->item_type) {
    default:
        send_to_char("{jYou cannot do that to a non-weapon.{x\n\r", ch);
        return false;

    case ITEM_WEAPON:
        pObj->value[1] = UMIN(pObj->level / 4 + 1, 5);
        pObj->value[2] = (pObj->level + 7) / pObj->value[1];
        break;
    }

    send_to_char("{jOk.{x\n\r", ch);
    return true;
}

ED_FUN_DEC(ed_addapply)
{
    bool rc = true;
    int value, bv, typ;
    ObjPrototype* pObj = (ObjPrototype*)arg;
    Affect* pAf;
    INIT_BUF(loc, MAX_STRING_LENGTH);
    INIT_BUF(mod, MAX_STRING_LENGTH);
    INIT_BUF(type, MAX_STRING_LENGTH);
    INIT_BUF(bvector, MAX_STRING_LENGTH);

    if (IS_NULLSTR(argument)) {
        send_to_char("{jSyntax:  {*ADDAPPLY [TYPE] [LOCATION] [# MOD] [BITVECTOR]{x\n\r", ch);
        rc = false;
        goto ed_addapply_cleanup;
    }

    READ_ARG(BUF(type));
    READ_ARG(BUF(loc));
    READ_ARG(BUF(mod));
    one_argument(argument, BUF(bvector));

    if (BUF(type)[0] == '\0' || (typ = flag_value(apply_types, BUF(type))) == NO_FLAG) {
        send_to_char("{jInvalid apply type. Valid apply types are:{x\n\r", ch);
        show_help(ch, "apptype");
        rc = false;
        goto ed_addapply_cleanup;
    }

    if (BUF(loc)[0] == '\0' || (value = flag_value(apply_flag_table, BUF(loc))) == NO_FLAG) {
        send_to_char("{jValid applys are:{x\n\r", ch);
        show_help(ch, "apply");
        rc = false;
        goto ed_addapply_cleanup;
    }

    if (BUF(bvector)[0] == '\0' || (bv = flag_value(bitvector_type[typ].table, BUF(bvector))) == NO_FLAG) {
        send_to_char("{jInvalid bitvector type.\n\r", ch);
        send_to_char("Valid bitvector types are:{x\n\r", ch);
        show_help(ch, bitvector_type[typ].help);
        rc = false;
        goto ed_addapply_cleanup;
    }

    if (BUF(mod)[0] == '\0' || !is_number(BUF(mod))) {
        send_to_char("{jSyntax:  {*ADDAPPLY [TYPE] [LOCATION] [# MOD] [BITVECTOR]{x\n\r", ch);
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

    send_to_char("{jApply added.{x\n\r", ch);

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
    ObjPrototype* pObj;
    AreaData* area;
    VNUM  value;
    int  hash;

    value = STRTOVNUM(argument);

    if (argument[0] == '\0' || value == 0) {
        send_to_char("{jSyntax: OEDIT CREATE [VNUM]{x\n\r", ch);
        return false;
    }

    area = get_vnum_area(value);

    if (!area) {
        send_to_char("{jOEdit: That vnum is not assigned an area.{x\n\r", ch);
        return false;
    }

    if (!IS_BUILDER(ch, area)) {
        send_to_char("{jOEdit: Vnum in an area you cannot build in.{x\n\r", ch);
        return false;
    }

    if (get_object_prototype(value)) {
        send_to_char("{jOEdit: Object vnum already exists.{x\n\r", ch);
        return false;
    }

    pObj = new_object_prototype();
    pObj->vnum = value;
    pObj->area = area;
    pObj->extra_flags = 0;

    if (value > top_vnum_obj)
        top_vnum_obj = value;

    hash = value % MAX_KEY_HASH;
    pObj->next = obj_proto_hash[hash];
    obj_proto_hash[hash] = pObj;

    set_editor(ch->desc, ED_OBJECT, U(pObj));

    send_to_char("{jObject Created.{x\n\r", ch);

    return true;
}

ED_FUN_DEC(ed_olist)
{
    ObjPrototype* obj_proto;
    AreaData* area;
    Buffer* buf1;
    char blarg[MAX_INPUT_LENGTH];
    bool fAll, found;
    VNUM vnum;
    int  col = 0;

    one_argument(argument, blarg);

    if (blarg[0] == '\0') {
        send_to_char("{jSyntax: OLIST [ALL|<NAME>|<TYPE>]{x\n\r", ch);
        return false;
    }

    area = *(AreaData**)arg;
    buf1 = new_buf();
    fAll = !str_cmp(blarg, "all");
    found = false;

    for (vnum = area->min_vnum; vnum <= area->max_vnum; vnum++) {
        if ((obj_proto = get_object_prototype(vnum))) {
            if (fAll || is_name(blarg, obj_proto->name)
                || (ItemType)flag_value(type_flag_table, blarg) == obj_proto->item_type) {
                found = true;
                addf_buf(buf1, "{|[{*%5d{|]{x %-17.16s",
                    obj_proto->vnum, capitalize(obj_proto->short_descr));
                if (++col % 3 == 0)
                    add_buf(buf1, "\n\r");
            }
        }
    }

    if (!found) {
        send_to_char("{jObject(s) not found in this area.{x\n\r", ch);
        return false;
    }

    if (col % 3 != 0)
        add_buf(buf1, "\n\r");

    page_to_char(BUF(buf1), ch);
    free_buf(buf1);

    return false;
}

OEDIT(oedit_copy)
{
    VNUM vnum;
    ObjPrototype* obj;
    ObjPrototype* obj2;

    EDIT_OBJ(ch, obj);

    if (IS_NULLSTR(argument)) {
        send_to_char("Syntax: copy [vnum]\n\r", ch);
        return false;
    }

    if (!is_number(argument) || (vnum = STRTOVNUM(argument)) < 0) {
        send_to_char("ERROR: VNUM must be greater than 0.\n\r", ch);
        return false;
    }

    if ((obj2 = get_object_prototype(vnum)) == NULL) {
        send_to_char("ERROR: That object does not exist.\n\r", ch);
        return false;
    }

    if (!IS_BUILDER(ch, obj2->area)) {
        send_to_char("ERROR: You do not have access to the area that object is in.\n\r", ch);
        return false;
    }

    RESTRING(obj->name, obj2->name);
    RESTRING(obj->short_descr, obj2->short_descr);
    RESTRING(obj->description, obj2->description);
    RESTRING(obj->material, obj2->material);

    ExtraDesc* ed_next = NULL;
    for (ExtraDesc* ed = obj->extra_desc; ed != NULL; ed = ed_next) {
        ed_next = ed->next;
        free_extra_desc(ed);
    }
    obj->extra_desc = NULL;

    obj->item_type = obj2->item_type;
    obj->extra_flags = obj2->extra_flags;
    obj->wear_flags = obj2->wear_flags;
    obj->weight = obj2->weight;
    obj->cost = obj2->cost;
    obj->level = obj2->level;
    obj->condition = obj2->condition;

    for (int i = 0; i < 5; ++i)
        obj->value[i] = obj2->value[i];

    Affect* af;
    FOR_EACH(af, obj2->affected) {
        Affect* af_new = new_affect();
        *af_new = *af;
        af_new->next = obj->affected;
        obj->affected = af_new;
    }

    ExtraDesc* ed;
    FOR_EACH(ed, obj2->extra_desc) {
        ExtraDesc* ed_new = new_extra_desc();
        ed_new->keyword = str_dup(ed->keyword);
        ed_new->description = str_dup(ed->description);
        ed_new->next = obj->extra_desc;
        obj->extra_desc = ed_new;
    }

    send_to_char("Ok.\n\r", ch);
    return true;
}

