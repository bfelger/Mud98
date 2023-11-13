////////////////////////////////////////////////////////////////////////////////
// mobile.c
// Mobiles
////////////////////////////////////////////////////////////////////////////////

#include "mobile.h"

#include "comm.h"
#include "db.h"
#include "handler.h"
#include "recycle.h"

#include "object.h"

#include "data/mobile_data.h"

Mobile* mob_list;
Mobile* mob_free;

int mob_count = 0;
int mob_perm_count = 0;
int last_mob_id = 0;

Mobile* new_mobile()
{
    LIST_ALLOC_PERM(mob, Mobile);

    VALIDATE(mob);
    mob->name = &str_empty[0];
    mob->short_descr = &str_empty[0];
    mob->long_descr = &str_empty[0];
    mob->description = &str_empty[0];
    mob->prompt = &str_empty[0];
    mob->prefix = &str_empty[0];
    mob->logon = current_time;
    mob->lines = PAGELEN;
    for (int i = 0; i < 4; i++)
        mob->armor[i] = 100;
    mob->position = POS_STANDING;
    mob->hit = 20;
    mob->max_hit = 20;
    mob->mana = 100;
    mob->max_mana = 100;
    mob->move = 100;
    mob->max_move = 100;
    for (int i = 0; i < STAT_COUNT; i++) {
        mob->perm_stat[i] = 13;
        mob->mod_stat[i] = 0;
    }

    return mob;
}

void free_mobile(Mobile* mob)
{
    Object* obj;
    Affect* affect;

    if (!IS_VALID(mob))
        return;

    while((obj = mob->carrying) != NULL) {
        extract_obj(obj);
    }

    while((affect = mob->affected) != NULL) {
        affect_remove(mob, affect);
    }

    free_string(mob->name);
    free_string(mob->short_descr);
    free_string(mob->long_descr);
    free_string(mob->description);
    free_string(mob->prompt);
    free_string(mob->prefix);
    free_note(mob->pnote);
    free_player_data(mob->pcdata);

    INVALIDATE(mob);

    LIST_FREE(mob);

    return;
}

void clone_mobile(Mobile* parent, Mobile* clone)
{
    int i;
    Affect* affect;

    if (parent == NULL || clone == NULL || !IS_NPC(parent)) 
        return;

    /* start fixing values */
    clone->name = str_dup(parent->name);
    clone->version = parent->version;
    clone->short_descr = str_dup(parent->short_descr);
    clone->long_descr = str_dup(parent->long_descr);
    clone->description = str_dup(parent->description);
    clone->group = parent->group;
    clone->sex = parent->sex;
    clone->ch_class = parent->ch_class;
    clone->race = parent->race;
    clone->level = parent->level;
    clone->trust = 0;
    clone->timer = parent->timer;
    clone->wait = parent->wait;
    clone->hit = parent->hit;
    clone->max_hit = parent->max_hit;
    clone->mana = parent->mana;
    clone->max_mana = parent->max_mana;
    clone->move = parent->move;
    clone->max_move = parent->max_move;
    clone->gold = parent->gold;
    clone->silver = parent->silver;
    clone->exp = parent->exp;
    clone->act_flags = parent->act_flags;
    clone->comm_flags = parent->comm_flags;
    clone->imm_flags = parent->imm_flags;
    clone->res_flags = parent->res_flags;
    clone->vuln_flags = parent->vuln_flags;
    clone->invis_level = parent->invis_level;
    clone->affect_flags = parent->affect_flags;
    clone->position = parent->position;
    clone->practice = parent->practice;
    clone->train = parent->train;
    clone->saving_throw = parent->saving_throw;
    clone->alignment = parent->alignment;
    clone->hitroll = parent->hitroll;
    clone->damroll = parent->damroll;
    clone->wimpy = parent->wimpy;
    clone->form = parent->form;
    clone->parts = parent->parts;
    clone->size = parent->size;
    clone->material = str_dup(parent->material);
    clone->atk_flags = parent->atk_flags;
    clone->dam_type = parent->dam_type;
    clone->start_pos = parent->start_pos;
    clone->default_pos = parent->default_pos;
    clone->spec_fun = parent->spec_fun;

    for (i = 0; i < AC_COUNT; i++)
        clone->armor[i] = parent->armor[i];

    for (i = 0; i < STAT_COUNT; i++) {
        clone->perm_stat[i] = parent->perm_stat[i];
        clone->mod_stat[i] = parent->mod_stat[i];
    }

    for (i = 0; i < 3; i++)
        clone->damage[i] = parent->damage[i];

    /* now add the affects */
    FOR_EACH(affect, parent->affected)
        affect_to_mob(clone, affect);
}

Mobile* create_mobile(MobPrototype* p_mob_proto)
{
    Mobile* mob;
    int i;
    Affect af = { 0 };

    if (p_mob_proto == NULL) {
        bug("Create_mobile: NULL p_mob_proto.", 0);
        exit(1);
    }

    mob = new_mobile();

    mob->prototype = p_mob_proto;

    mob->name = str_dup(p_mob_proto->name);
    mob->short_descr = str_dup(p_mob_proto->short_descr);
    mob->long_descr = str_dup(p_mob_proto->long_descr);
    mob->description = str_dup(p_mob_proto->description);
    mob->id = get_mob_id();
    mob->spec_fun = p_mob_proto->spec_fun;
    mob->prompt = NULL;
    mob->mprog_target = NULL;

    if (p_mob_proto->wealth == 0) {
        mob->silver = 0;
        mob->gold = 0;
    }
    else {
        int16_t wealth = (int16_t)number_range(p_mob_proto->wealth / 2,
            3 * p_mob_proto->wealth / 2);
        mob->gold = (int16_t)number_range(wealth / 200, wealth / 100);
        mob->silver = wealth - (mob->gold * 100);
    }

    /* read from prototype */
    mob->group = p_mob_proto->group;
    mob->act_flags = p_mob_proto->act_flags;
    mob->comm_flags = COMM_NOCHANNELS | COMM_NOSHOUT | COMM_NOTELL;
    mob->affect_flags = p_mob_proto->affect_flags;
    mob->alignment = p_mob_proto->alignment;
    mob->level = p_mob_proto->level;
    mob->hitroll = p_mob_proto->hitroll;
    mob->damroll = p_mob_proto->damage[DICE_BONUS];
    mob->max_hit = (int16_t)dice(p_mob_proto->hit[DICE_NUMBER],
        p_mob_proto->hit[DICE_TYPE]) + p_mob_proto->hit[DICE_BONUS];
    mob->hit = mob->max_hit;
    mob->max_mana = (int16_t)dice(p_mob_proto->mana[DICE_NUMBER],
        p_mob_proto->mana[DICE_TYPE]) + p_mob_proto->mana[DICE_BONUS];
    mob->mana = mob->max_mana;
    mob->damage[DICE_NUMBER] = p_mob_proto->damage[DICE_NUMBER];
    mob->damage[DICE_TYPE] = p_mob_proto->damage[DICE_TYPE];
    mob->dam_type = p_mob_proto->dam_type;
    if (mob->dam_type == 0) {
        switch (number_range(1, 3)) {
        case (1):
            mob->dam_type = 3;
            break; /* slash */
        case (2):
            mob->dam_type = 7;
            break; /* pound */
        case (3):
            mob->dam_type = 11;
            break; /* pierce */
        }
    }
    for (i = 0; i < 4; i++)
        mob->armor[i] = p_mob_proto->ac[i];
    mob->atk_flags = p_mob_proto->atk_flags;
    mob->imm_flags = p_mob_proto->imm_flags;
    mob->res_flags = p_mob_proto->res_flags;
    mob->vuln_flags = p_mob_proto->vuln_flags;
    mob->start_pos = p_mob_proto->start_pos;
    mob->default_pos = p_mob_proto->default_pos;
    mob->sex = p_mob_proto->sex;
    if (mob->sex == SEX_EITHER) /* random sex */
        mob->sex = (Sex)number_range(1, 2);
    mob->race = p_mob_proto->race;
    mob->form = p_mob_proto->form;
    mob->parts = p_mob_proto->parts;
    mob->size = p_mob_proto->size;
    mob->material = str_dup(p_mob_proto->material);

    /* computed on the spot */

    for (i = 0; i < STAT_COUNT; i++)
        mob->perm_stat[i] = UMIN(25, 11 + mob->level / 4);

    if (IS_SET(mob->act_flags, ACT_WARRIOR)) {
        mob->perm_stat[STAT_STR] += 3;
        mob->perm_stat[STAT_INT] -= 1;
        mob->perm_stat[STAT_CON] += 2;
    }

    if (IS_SET(mob->act_flags, ACT_THIEF)) {
        mob->perm_stat[STAT_DEX] += 3;
        mob->perm_stat[STAT_INT] += 1;
        mob->perm_stat[STAT_WIS] -= 1;
    }

    if (IS_SET(mob->act_flags, ACT_CLERIC)) {
        mob->perm_stat[STAT_WIS] += 3;
        mob->perm_stat[STAT_DEX] -= 1;
        mob->perm_stat[STAT_STR] += 1;
    }

    if (IS_SET(mob->act_flags, ACT_MAGE)) {
        mob->perm_stat[STAT_INT] += 3;
        mob->perm_stat[STAT_STR] -= 1;
        mob->perm_stat[STAT_DEX] += 1;
    }

    if (IS_SET(mob->atk_flags, ATK_FAST))
        mob->perm_stat[STAT_DEX] += 2;

    mob->perm_stat[STAT_STR] += (int16_t)(mob->size - SIZE_MEDIUM);
    mob->perm_stat[STAT_CON] += (int16_t)(mob->size - SIZE_MEDIUM) / 2;

    /* let's get some spell action */
    if (IS_AFFECTED(mob, AFF_SANCTUARY)) {
        af.where = TO_AFFECTS;
        af.type = skill_lookup("sanctuary");
        af.level = mob->level;
        af.duration = -1;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.bitvector = AFF_SANCTUARY;
        affect_to_mob(mob, &af);
    }

    if (IS_AFFECTED(mob, AFF_HASTE)) {
        af.where = TO_AFFECTS;
        af.type = skill_lookup("haste");
        af.level = mob->level;
        af.duration = -1;
        af.location = APPLY_DEX;
        af.modifier = 1 + (mob->level >= 18) + (mob->level >= 25)
            + (mob->level >= 32);
        af.bitvector = AFF_HASTE;
        affect_to_mob(mob, &af);
    }

    if (IS_AFFECTED(mob, AFF_PROTECT_EVIL)) {
        af.where = TO_AFFECTS;
        af.type = skill_lookup("protection evil");
        af.level = mob->level;
        af.duration = -1;
        af.location = APPLY_SAVES;
        af.modifier = -1;
        af.bitvector = AFF_PROTECT_EVIL;
        affect_to_mob(mob, &af);
    }

    if (IS_AFFECTED(mob, AFF_PROTECT_GOOD)) {
        af.where = TO_AFFECTS;
        af.type = skill_lookup("protection good");
        af.level = mob->level;
        af.duration = -1;
        af.location = APPLY_SAVES;
        af.modifier = -1;
        af.bitvector = AFF_PROTECT_GOOD;
        affect_to_mob(mob, &af);
    }

    mob->position = mob->start_pos;

    /* link the mob to the world list */
    mob->next = mob_list;
    mob_list = mob;
    p_mob_proto->count++;
    return mob;
}

long get_mob_id()
{
    last_mob_id++;
    return last_mob_id;
}

void clear_mob(Mobile* ch)
{
    static Mobile ch_zero = { 0 };
    int i;

    *ch = ch_zero;
    ch->name = &str_empty[0];
    ch->short_descr = &str_empty[0];
    ch->long_descr = &str_empty[0];
    ch->description = &str_empty[0];
    ch->prompt = &str_empty[0];
    ch->logon = current_time;
    ch->lines = PAGELEN;
    for (i = 0; i < 4; i++) ch->armor[i] = 100;
    ch->position = POS_STANDING;
    ch->hit = 20;
    ch->max_hit = 20;
    ch->mana = 100;
    ch->max_mana = 100;
    ch->move = 100;
    ch->max_move = 100;
    ch->on = NULL;
    for (i = 0; i < STAT_COUNT; i++) {
        ch->perm_stat[i] = 13;
        ch->mod_stat[i] = 0;
    }
}

////////////////////////////////////////////////////////////////////////////////
// Lox representation
////////////////////////////////////////////////////////////////////////////////

static ObjClass* mobile_class = NULL;

void init_mobile_class()
{
    char* source =
        "class Mobile { "
        "   name() { return marshal(this._name); }"
        "   vnum() { return marshal(this._vnum); }"
        "   short_desc() { return marshal(this._short_desc); } "
        "   in_room() { return get_room(this._in_room); } "
        "   carrying() { return get_carrying(this._base); } "
        "   hp() { return this._hp; } "
        "   max_hp() { return this._max_hp; } "
        "}";

    InterpretResult result = interpret_code(source);

    if (result == INTERPRET_COMPILE_ERROR) exit(65);
    if (result == INTERPRET_RUNTIME_ERROR) exit(70);

    mobile_class = find_class("Mobile");
}

Value create_mobile_value(Mobile* mobile)
{
    if (!mobile || !mobile_class)
        return NIL_VAL;

    ObjInstance* inst = new_instance(mobile_class);
    push(OBJ_VAL(inst));

    SET_NATIVE_FIELD(inst, mobile, base, OBJ);
    SET_NATIVE_FIELD(inst, mobile->name, name, STR);
    SET_NATIVE_FIELD(inst, mobile->prototype->vnum, vnum, I32);
    SET_NATIVE_FIELD(inst, mobile->short_descr, short_desc, STR);
    SET_NATIVE_FIELD(inst, mobile->in_room, in_room, OBJ);
    SET_NATIVE_FIELD(inst, mobile->hit, hp, I16);
    SET_NATIVE_FIELD(inst, mobile->max_hit, max_hp, I16);

    pop(); // instance

    return OBJ_VAL(inst);
}

Value get_mobile_carrying_native(int arg_count, Value* args)
{
    if (arg_count != 1) {
        printf("get_people() takes 1 argument; %d given.", arg_count);
        return NIL_VAL;
    }

    Mobile* mobile = NULL;

    if (IS_RAW_PTR(args[0]) && AS_RAW_PTR(args[0])->type == RAW_OBJ) {
        mobile = (Mobile*)AS_RAW_PTR(args[0])->addr;
    }

    ObjArray* array_ = new_obj_array();
    push(OBJ_VAL(array_));

    if (mobile) {
        // Just return an empty array if there is no room. Don't force scripters
        // to use guards. NULL is a disease.
        Object* obj;
        FOR_EACH_CONTENT(obj, mobile->carrying)
        {
            Value obj_val = create_object_value(obj);
            push(obj_val);
            write_value_array(&array_->val_array, obj_val);
            pop(); // obj_val
        }
    }

    pop(); // array_
    return OBJ_VAL(array_);
}