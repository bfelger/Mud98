////////////////////////////////////////////////////////////////////////////////
// snippets/lore.c
////////////////////////////////////////////////////////////////////////////////

/* ROM 2.4 Lore Skill v 1.0. */

/*  The lore skill is to replace the one that DOES not seem to come
    with the basic STOCK rom.  This is a smaller version of identify
    as lore should not reflect a lot of specific spellcaster
    information since it is mainly used by warriors and the like.

    This file can be used.. and have fun with it.  Give me credit or
    not.. This is free.  I, however, do not take resposibility if you
    do not insert this code correctly.  Heck, it is free..  use it and
    enjoy.

    The Mage
*/

/* Lore skill by The Mage */

#include <data/item.h>

#include <entities/affect.h>
#include <entities/mobile.h>
#include <entities/object.h>

#include <comm.h>
#include <db.h>

void do_lore(Mobile* ch, char* argument)
{
	char object_name[MAX_INPUT_LENGTH + 100];

	Object* obj;

	//char buf[MAX_STRING_LENGTH];  // Unreferenced
	//Affect* paf;					// Unreferenced

	argument = one_argument(argument, object_name);
	if (object_name[0] == '\0') {
		send_to_char("You need to specify an item to study.\n\r", ch);
		return;
	}

	if ((obj = get_obj_carry(ch, object_name, ch)) == NULL)
	{
		send_to_char("You are not carrying that.\n\r", ch);
		return;
	}

	send_to_char("You ponder the item.\n\r", ch);
	if (number_percent() < get_skill(ch, gsn_lore) &&
		ch->level >= obj->level) {

		printf_to_char(ch,
			"Object '%s' is type %s, extra flags %s.\n\rWeight is %d, value is %d, level is %d.\n\r",

			NAME_STR(obj),		// obj->name
			item_name(obj->item_type),
			extra_bit_name(obj->extra_flags),
			obj->weight / 10,
			obj->cost,
			obj->level
		);

		switch (obj->item_type)
		{
		case ITEM_SCROLL:
		case ITEM_POTION:
		case ITEM_PILL:
			printf_to_char(ch, "Some level spells of:");

			if (obj->value[1] >= 0 && obj->value[1] < skill_count) // MAX_SKILL
			{
				send_to_char(" '", ch);
				send_to_char(skill_table[obj->value[1]].name, ch);
				send_to_char("'", ch);
			}

			if (obj->value[2] >= 0 && obj->value[2] < skill_count) // MAX_SKILL
			{
				send_to_char(" '", ch);
				send_to_char(skill_table[obj->value[2]].name, ch);
				send_to_char("'", ch);
			}

			if (obj->value[3] >= 0 && obj->value[3] < skill_count) // MAX_SKILL
			{
				send_to_char(" '", ch);
				send_to_char(skill_table[obj->value[3]].name, ch);
				send_to_char("'", ch);
			}

			if (obj->value[4] >= 0 && obj->value[4] < skill_count) // MAX_SKILL
			{
				send_to_char(" '", ch);
				send_to_char(skill_table[obj->value[4]].name, ch);
				send_to_char("'", ch);
			}

			send_to_char(".\n\r", ch);
			break;

		case ITEM_WAND:
		case ITEM_STAFF:
			printf_to_char(ch, "Has some charges of some level");

			if (obj->value[3] >= 0 && obj->value[3] < skill_count) // MAX_SKILL
			{
				send_to_char(" '", ch);
				send_to_char(skill_table[obj->value[3]].name, ch);
				send_to_char("'", ch);
			}

			send_to_char(".\n\r", ch);
			break;

		case ITEM_DRINK_CON:
			printf_to_char(ch, "It holds %s-colored %s.\n\r",
				liquid_table[obj->value[2]].color,			// liq_table[obj->value[2]].liq_color,
				liquid_table[obj->value[2]].name);			// liq_table[obj->value[2]].liq_name);
			break;

		case ITEM_CONTAINER:
			printf_to_char(ch, "Capacity: %d#  Maximum weight: %d#  flags: %s\n\r",
				obj->value[0], obj->value[3], cont_bit_name(obj->value[1]));
			if (obj->value[4] != 100)
			{
				printf_to_char(ch, "Weight multiplier: %d%%\n\r",
					obj->value[4]);
			}
			break;

		case ITEM_WEAPON:
			send_to_char("Weapon type is ", ch);
			switch (obj->value[0])
			{
			case(WEAPON_EXOTIC): send_to_char("exotic.\n\r", ch);	break;
			case(WEAPON_SWORD): send_to_char("sword.\n\r", ch);	break;
			case(WEAPON_DAGGER): send_to_char("dagger.\n\r", ch);	break;
			case(WEAPON_SPEAR): send_to_char("spear/staff.\n\r", ch);	break;
			case(WEAPON_MACE): send_to_char("mace/club.\n\r", ch);	break;
			case(WEAPON_AXE): send_to_char("axe.\n\r", ch);		break;
			case(WEAPON_FLAIL): send_to_char("flail.\n\r", ch);	break;
			case(WEAPON_WHIP): send_to_char("whip.\n\r", ch);		break;
			case(WEAPON_POLEARM): send_to_char("polearm.\n\r", ch);	break;
			default: send_to_char("unknown.\n\r", ch);	break;
			}

			printf_to_char(ch, "Damage is %dd%d (average %d).\n\r",	
				obj->value[1], obj->value[2], 
				(1 + obj->value[2]) * obj->value[1] / 2);
			
			// weapon flags
			if (obj->value[4]) {
				printf_to_char(ch, "Weapons flags: %s\n\r", weapon_bit_name(obj->value[4]));
			}
			break;

		case ITEM_ARMOR:
			printf_to_char(ch,
				"Armor class is %d pierce, %d bash, %d slash, and %d vs. magic.\n\r",
				obj->value[0], obj->value[1], obj->value[2], obj->value[3]);
			break;


		case ITEM_BOAT:
		case ITEM_CLOTHING:
		case ITEM_CORPSE_NPC:
		case ITEM_CORPSE_PC:
		case ITEM_FOOD:
		case ITEM_FOUNTAIN:
		case ITEM_FURNITURE:
		case ITEM_GEM:
		case ITEM_JEWELRY:
		case ITEM_JUKEBOX:
		case ITEM_KEY:
		case ITEM_LIGHT:
		case ITEM_MAP:
		case ITEM_MONEY:
		case ITEM_NONE:
		case ITEM_PORTAL:
		case ITEM_PROTECT:
		case ITEM_ROOM_KEY:
		case ITEM_TRASH:
		case ITEM_TREASURE:
		case ITEM_WARP_STONE:
		default:
			send_to_char("You learn nothing of value.\n\r", ch);
			break;
		}
	}
	return;
}
