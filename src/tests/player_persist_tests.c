////////////////////////////////////////////////////////////////////////////////
// tests/player_persist_tests.c
////////////////////////////////////////////////////////////////////////////////

#include "tests.h"
#include "test_registry.h"
#include "mock.h"

#include <config.h>
#include <save.h>
#include <fileutils.h>
#include <skills.h>
#include <stringutils.h>
#include <recycle.h>
#include <digest.h>
#include <tables.h>
#include <db.h>
#include <lookup.h>
#include <data/race.h>
#include <comm.h>
#include <handler.h>
#include <color.h>
#include <lox/lox.h>
#include <merc.h>

#include <entities/object.h>
#include <entities/faction.h>

#include <errno.h>
#include <string.h>

#ifndef _MSC_VER
#include <sys/stat.h>
#include <unistd.h>
#else
#include <direct.h>
#endif

static TestGroup player_persist_tests;

static void format_path(char* out, size_t len, const char* part1,
    const char* part2, const char* part3)
{
    size_t total = strlen(part1) + (part2 ? strlen(part2) : 0)
        + (part3 ? strlen(part3) : 0);
    ASSERT(total + 1 < len);
    strcpy(out, part1);
    if (part2)
        strcat(out, part2);
    if (part3)
        strcat(out, part3);
}

static void apply_race_defaults(Mobile* ch)
{
    ch->affect_flags |= race_table[ch->race].aff;
    ch->imm_flags |= race_table[ch->race].imm;
    ch->res_flags |= race_table[ch->race].res;
    ch->vuln_flags |= race_table[ch->race].vuln;
    ch->form = race_table[ch->race].form;
    ch->parts = race_table[ch->race].parts;
}

static void populate_player(Mobile* player)
{
    player->level = 47;
    player->trust = 50;
    player->sex = SEX_FEMALE;
    player->ch_class = 2;
    player->race = race_lookup("elf");
    player->alignment = 250;
    player->act_flags = PLR_AUTOEXIT | PLR_COLOUR | PLR_AUTOSPLIT;
    player->affect_flags = AFF_SANCTUARY | AFF_HASTE;
    player->comm_flags = COMM_PROMPT | COMM_NOSHOUT;
    player->wiznet = WIZ_LINKS | WIZ_LOGINS;
    player->invis_level = 3;
    player->incog_level = 2;
    player->position = POS_STANDING;
    player->practice = 7;
    player->train = 2;
    player->saving_throw = -3;
    player->hitroll = 11;
    player->damroll = 9;
    player->wimpy = 15;
    player->gold = 123;
    player->silver = 456;
    player->copper = 789;
    player->exp = 5555;
    player->hit = 320;
    player->max_hit = 450;
    player->mana = 210;
    player->max_mana = 300;
    player->move = 160;
    player->max_move = 320;
    for (int i = 0; i < STAT_COUNT; ++i) {
        player->perm_stat[i] = (int16_t)(15 + i);
        player->mod_stat[i] = (int16_t)(2 - i);
    }
    for (int i = 0; i < AC_COUNT; ++i)
        player->armor[i] = (int16_t)(-20 - i);

    free_string(player->pcdata->title);
    player->pcdata->title = str_dup(" the Persistent");
    free_string(player->pcdata->bamfin);
    player->pcdata->bamfin = str_dup("^parrives in a shimmer of light^x");
    free_string(player->pcdata->bamfout);
    player->pcdata->bamfout = str_dup("^pleaves in a trail of sparks^x");

    player->pcdata->points = 32;
    player->pcdata->armor_prof = ARMOR_MEDIUM;
    player->pcdata->true_sex = SEX_FEMALE;
    player->pcdata->last_level = 30;
    player->pcdata->perm_hit = 150;
    player->pcdata->perm_mana = 120;
    player->pcdata->perm_move = 180;
    player->pcdata->condition[COND_DRUNK] = 5;
    player->pcdata->condition[COND_FULL] = 25;
    player->pcdata->condition[COND_THIRST] = 35;
    player->pcdata->condition[COND_HUNGER] = 10;
    player->pcdata->recall = 3700;
    player->pcdata->security = 3;
    player->pcdata->theme_config.hide_24bit = true;
    player->pcdata->theme_config.hide_256 = false;
    player->pcdata->theme_config.xterm = true;
    player->pcdata->theme_config.hide_rgb_help = false;
    free_string(player->pcdata->theme_config.current_theme_name);
    player->pcdata->theme_config.current_theme_name = str_dup("ansi");

    player->pcdata->last_note = 111;
    player->pcdata->last_idea = 222;
    player->pcdata->last_penalty = 333;
    player->pcdata->last_news = 444;
    player->pcdata->last_changes = 555;

    player->pcdata->alias[0] = str_dup("g");
    player->pcdata->alias_sub[0] = str_dup("say hello everyone");

    set_password("persist123", player);

    SKNUM skill = skill_lookup("sneak");
    if (skill >= 0)
        player->pcdata->learned[skill] = 87;
    SKNUM group = group_lookup("rom basics");
    if (group >= 0)
        gn_add(player, group);

    faction_set(player->pcdata, 4001, 250);

    Affect* affect = new_affect();
    affect->type = (skill >= 0) ? skill : 0;
    affect->where = TO_AFFECTS;
    affect->level = 20;
    affect->duration = 12;
    affect->modifier = 3;
    affect->location = APPLY_STR;
    affect->bitvector = AFF_HASTE;
    affect_to_mob(player, affect);

    ObjPrototype* sword_proto = get_object_prototype(OBJ_VNUM_SCHOOL_SWORD);
    if (sword_proto) {
        Object* sword = create_object(sword_proto, 0);
        sword->value[0] = 3;
        sword->value[1] = 4;
        sword->value[2] = 12;
        sword->wear_loc = WEAR_WIELD;
        sword->level = 10;
        sword->timer = 0;
        sword->condition = 85;

        ExtraDesc* ed = new_extra_desc();
        ed->keyword = str_dup("etching");
        ed->description = str_dup("Intricate patterns cover the blade.");
        ed->next = sword->extra_desc;
        sword->extra_desc = ed;

        Affect* obj_aff = new_affect();
        obj_aff->type = skill;
        obj_aff->where = TO_OBJECT;
        obj_aff->level = 12;
        obj_aff->duration = -1;
        obj_aff->modifier = 2;
        obj_aff->location = APPLY_HITROLL;
        obj_aff->bitvector = 0;
        obj_aff->next = sword->affected;
        sword->affected = obj_aff;

        obj_to_char(sword, player);
    }

    ObjPrototype* pouch_proto = get_object_prototype(OBJ_VNUM_SCHOOL_SHIELD);
    if (pouch_proto) {
        Object* pouch = create_object(pouch_proto, 0);
        pouch->wear_loc = WEAR_UNHELD;
        pouch->value[0] = 30;
        ObjPrototype* gem_proto = get_object_prototype(OBJ_VNUM_SCHOOL_MACE);
        if (gem_proto) {
            Object* gem = create_object(gem_proto, 0);
            gem->item_type = ITEM_GEM;
            SET_NAME(gem, lox_string("sparkling gem"));
            obj_to_obj(gem, pouch);
        }
        obj_to_char(pouch, player);
    }

    MobPrototype* pet_proto = get_mob_prototype(MOB_VNUM_FIDO);
    if (pet_proto) {
        Mobile* pet = create_mobile(pet_proto);
        pet->level = 15;
        pet->hit = pet->max_hit = 200;
        pet->mana = pet->max_mana = 50;
        pet->move = pet->max_move = 120;
        pet->alignment = 100;
        SET_NAME(pet, lox_string("persistent companion"));
        pet->clan = player->clan;
        pet->master = player;
        pet->leader = player;
        player->pet = pet;
    }

    ColorTheme* theme = dup_color_theme(get_default_system_color_theme());
    if (theme) {
        free_string(theme->name);
        theme->name = str_dup("tester_theme");
        theme->channels[SLOT_TITLE].mode = COLOR_MODE_RGB;
        theme->channels[SLOT_TITLE].code[0] = 200;
        theme->channels[SLOT_TITLE].code[1] = 100;
        theme->channels[SLOT_TITLE].code[2] = 50;
        player->pcdata->color_themes[0] = theme;
        free_string(player->pcdata->theme_config.current_theme_name);
        player->pcdata->theme_config.current_theme_name = str_dup(theme->name);
    }

    apply_race_defaults(player);
}

static void assert_affects_equal(Affect* a, Affect* b)
{
    while (a != NULL && b != NULL) {
        ASSERT(a->type == b->type);
        ASSERT(a->where == b->where);
        ASSERT(a->level == b->level);
        ASSERT(a->duration == b->duration);
        ASSERT(a->modifier == b->modifier);
        ASSERT(a->location == b->location);
        ASSERT(a->bitvector == b->bitvector);
        a = a->next;
        b = b->next;
    }
    ASSERT(a == NULL && b == NULL);
}

static Object* find_object_with_vnum(const Mobile* ch, VNUM vnum)
{
    for (Node* node = ch->objects.front; node != NULL; node = node->next) {
        Object* obj = AS_OBJECT(node->value);
        if (VNUM_FIELD(obj->prototype) == vnum)
            return obj;
    }
    return NULL;
}

static Object* find_object_in_object(const Object* container, VNUM vnum)
{
    if (!container)
        return NULL;
    for (Node* node = container->objects.front; node != NULL; node = node->next) {
        Object* obj = AS_OBJECT(node->value);
        if (VNUM_FIELD(obj->prototype) == vnum)
            return obj;
    }
    return NULL;
}

static void assert_players_equal(Mobile* original, Mobile* loaded)
{
    ASSERT(!str_cmp(NAME_STR(original), NAME_STR(loaded)));
    ASSERT(original->level == loaded->level);
    ASSERT(original->trust == loaded->trust);
    ASSERT(original->sex == loaded->sex);
    ASSERT(original->ch_class == loaded->ch_class);
    ASSERT(original->race == loaded->race);
    ASSERT(original->alignment == loaded->alignment);
    ASSERT(original->act_flags == loaded->act_flags);
    ASSERT(original->affect_flags == loaded->affect_flags);
    ASSERT(original->comm_flags == loaded->comm_flags);
    ASSERT(original->wiznet == loaded->wiznet);
    ASSERT(original->invis_level == loaded->invis_level);
    ASSERT(original->incog_level == loaded->incog_level);
    ASSERT(original->position == loaded->position);
    ASSERT(original->practice == loaded->practice);
    ASSERT(original->train == loaded->train);
    ASSERT(original->saving_throw == loaded->saving_throw);
    ASSERT(original->hitroll == loaded->hitroll);
    ASSERT(original->damroll == loaded->damroll);
    ASSERT(original->wimpy == loaded->wimpy);
    ASSERT(original->gold == loaded->gold);
    ASSERT(original->silver == loaded->silver);
    ASSERT(original->copper == loaded->copper);
    ASSERT(original->exp == loaded->exp);
    ASSERT(original->hit == loaded->hit);
    ASSERT(original->max_hit == loaded->max_hit);
    ASSERT(original->mana == loaded->mana);
    ASSERT(original->max_mana == loaded->max_mana);
    ASSERT(original->move == loaded->move);
    ASSERT(original->max_move == loaded->max_move);
    for (int i = 0; i < STAT_COUNT; ++i) {
        ASSERT(original->perm_stat[i] == loaded->perm_stat[i]);
        ASSERT(original->mod_stat[i] == loaded->mod_stat[i]);
    }
    for (int i = 0; i < AC_COUNT; ++i)
        ASSERT(original->armor[i] == loaded->armor[i]);

    ASSERT(!str_cmp(original->pcdata->title, loaded->pcdata->title));
    ASSERT(!str_cmp(original->pcdata->bamfin, loaded->pcdata->bamfin));
    ASSERT(!str_cmp(original->pcdata->bamfout, loaded->pcdata->bamfout));
    ASSERT(original->pcdata->points == loaded->pcdata->points);
    ASSERT(original->pcdata->armor_prof == loaded->pcdata->armor_prof);
    ASSERT(original->pcdata->true_sex == loaded->pcdata->true_sex);
    ASSERT(original->pcdata->last_level == loaded->pcdata->last_level);
    ASSERT(original->pcdata->perm_hit == loaded->pcdata->perm_hit);
    ASSERT(original->pcdata->perm_mana == loaded->pcdata->perm_mana);
    ASSERT(original->pcdata->perm_move == loaded->pcdata->perm_move);
    for (int i = 0; i < COND_MAX; ++i)
        ASSERT(original->pcdata->condition[i] == loaded->pcdata->condition[i]);
    ASSERT(original->pcdata->recall == loaded->pcdata->recall);
    ASSERT(original->pcdata->security == loaded->pcdata->security);
    ASSERT(original->pcdata->theme_config.hide_24bit == loaded->pcdata->theme_config.hide_24bit);
    ASSERT(original->pcdata->theme_config.hide_256 == loaded->pcdata->theme_config.hide_256);
    ASSERT(original->pcdata->theme_config.xterm == loaded->pcdata->theme_config.xterm);
    ASSERT(original->pcdata->theme_config.hide_rgb_help == loaded->pcdata->theme_config.hide_rgb_help);
    ASSERT(!str_cmp(original->pcdata->theme_config.current_theme_name,
        loaded->pcdata->theme_config.current_theme_name));
    ASSERT(original->pcdata->last_note == loaded->pcdata->last_note);
    ASSERT(original->pcdata->last_idea == loaded->pcdata->last_idea);
    ASSERT(original->pcdata->last_penalty == loaded->pcdata->last_penalty);
    ASSERT(original->pcdata->last_news == loaded->pcdata->last_news);
    ASSERT(original->pcdata->last_changes == loaded->pcdata->last_changes);

    ASSERT(!str_cmp(original->pcdata->alias[0], loaded->pcdata->alias[0]));
    ASSERT(!str_cmp(original->pcdata->alias_sub[0], loaded->pcdata->alias_sub[0]));

    SKNUM skill = skill_lookup("sneak");
    if (skill >= 0)
        ASSERT(original->pcdata->learned[skill] == loaded->pcdata->learned[skill]);

    SKNUM group = group_lookup("rom basics");
    if (group >= 0)
        ASSERT(original->pcdata->group_known[group] == loaded->pcdata->group_known[group]);

    ASSERT(original->pcdata->reputations.count == loaded->pcdata->reputations.count);
    if (original->pcdata->reputations.count > 0) {
        ASSERT(original->pcdata->reputations.entries[0].vnum == loaded->pcdata->reputations.entries[0].vnum);
        ASSERT(original->pcdata->reputations.entries[0].value == loaded->pcdata->reputations.entries[0].value);
    }

    ASSERT(!str_cmp(original->pcdata->pwd_digest_hex, loaded->pcdata->pwd_digest_hex));
    ASSERT(validate_password("persist123", loaded));

    if (original->pcdata->color_themes[0] && loaded->pcdata->color_themes[0]) {
        ASSERT(!str_cmp(original->pcdata->color_themes[0]->name,
            loaded->pcdata->color_themes[0]->name));
        ASSERT(original->pcdata->color_themes[0]->channels[SLOT_TITLE].mode ==
            loaded->pcdata->color_themes[0]->channels[SLOT_TITLE].mode);
        for (int i = 0; i < 3; ++i)
            ASSERT(original->pcdata->color_themes[0]->channels[SLOT_TITLE].code[i] ==
                loaded->pcdata->color_themes[0]->channels[SLOT_TITLE].code[i]);
    }

    assert_affects_equal(original->affected, loaded->affected);

    Object* orig_sword = find_object_with_vnum(original, OBJ_VNUM_SCHOOL_SWORD);
    Object* loaded_sword = find_object_with_vnum(loaded, OBJ_VNUM_SCHOOL_SWORD);
    ASSERT(orig_sword && loaded_sword);
    ASSERT(orig_sword->value[0] == loaded_sword->value[0]);
    ASSERT(orig_sword->value[1] == loaded_sword->value[1]);
    ASSERT(orig_sword->value[2] == loaded_sword->value[2]);
    ASSERT(orig_sword->wear_loc == loaded_sword->wear_loc);
    ASSERT(orig_sword->condition == loaded_sword->condition);
    assert_affects_equal(orig_sword->affected, loaded_sword->affected);

    Object* orig_pouch = find_object_with_vnum(original, OBJ_VNUM_SCHOOL_SHIELD);
    Object* loaded_pouch = find_object_with_vnum(loaded, OBJ_VNUM_SCHOOL_SHIELD);
    ASSERT(orig_pouch && loaded_pouch);
    ASSERT(orig_pouch->value[0] == loaded_pouch->value[0]);

    Object* orig_gem = find_object_in_object(orig_pouch, OBJ_VNUM_SCHOOL_MACE);
    Object* loaded_gem = find_object_in_object(loaded_pouch, OBJ_VNUM_SCHOOL_MACE);
    ASSERT(orig_gem && loaded_gem);
    ASSERT(!str_cmp(NAME_STR(orig_gem), NAME_STR(loaded_gem)));

    if (original->pet && loaded->pet) {
        ASSERT(!str_cmp(NAME_STR(original->pet), NAME_STR(loaded->pet)));
        ASSERT(original->pet->level == loaded->pet->level);
        ASSERT(original->pet->hit == loaded->pet->hit);
        ASSERT(original->pet->mana == loaded->pet->mana);
        ASSERT(original->pet->move == loaded->pet->move);
        ASSERT(original->pet->alignment == loaded->pet->alignment);
    }
}

static void ensure_temp_dir(const char* dir)
{
#ifndef _MSC_VER
    mkdir("temp", 0775);
    mkdir(dir, 0775);
#else
    _mkdir("temp");
    _mkdir(dir);
#endif
}

static void remove_temp_dir(const char* dir)
{
#ifndef _MSC_VER
    rmdir(dir);
#else
    _rmdir(dir);
#endif
}

static void cleanup_player_file(const char* path)
{
    remove(path);
}

static int player_round_trip_with_format(const char* format, const char* temp_dir,
    const char* temp_dir_no_slash)
{
    char old_dir[MIL];
    strncpy(old_dir, cfg_get_player_dir(), sizeof old_dir - 1);
    old_dir[sizeof old_dir - 1] = '\0';
    char old_format[32];
    strncpy(old_format, cfg_get_default_format(), sizeof old_format - 1);
    old_format[sizeof old_format - 1] = '\0';

    ensure_temp_dir(temp_dir_no_slash);
    cfg_set_player_dir(temp_dir);
    cfg_set_default_format(format);

    Mobile* player = mock_player("PersistUser");
    populate_player(player);

    bool prev = test_output_enabled;
    test_output_enabled = false;
    save_char_obj(player);
    test_output_enabled = prev;

    Descriptor* d = mock_descriptor();
    ASSERT(load_char_obj(d, "PersistUser"));

    assert_players_equal(player, d->character);

    const char* ext = (!str_cmp(format, "json")) ? ".json" : "";
    char cap_name[MAX_INPUT_LENGTH];
    sprintf(cap_name, "%s", capitalize("PersistUser"));
    char path[MIL];
    format_path(path, sizeof path, temp_dir, cap_name, ext);
    cleanup_player_file(path);
    cfg_set_player_dir(old_dir);
    cfg_set_default_format(old_format);
    remove_temp_dir(temp_dir_no_slash);
    return 0;
}

static int test_player_round_trip_json()
{
    return player_round_trip_with_format("json", "temp/player_persist_tests/",
        "temp/player_persist_tests");
}

static int test_player_round_trip_rom()
{
    return player_round_trip_with_format("olc", "temp/player_persist_tests_rom/",
        "temp/player_persist_tests_rom");
}

static int test_player_format_migration()
{
    const char* temp_dir = "temp/player_migrate/";
    const char* temp_dir_no_slash = "temp/player_migrate";
    char old_dir[MIL];
    strncpy(old_dir, cfg_get_player_dir(), sizeof old_dir - 1);
    old_dir[sizeof old_dir - 1] = '\0';
    char old_format[32];
    strncpy(old_format, cfg_get_default_format(), sizeof old_format - 1);
    old_format[sizeof old_format - 1] = '\0';

    ensure_temp_dir(temp_dir_no_slash);
    cfg_set_player_dir(temp_dir);
    cfg_set_default_format("olc");

    Mobile* player = mock_player("Migrator");
    populate_player(player);

    bool prev = test_output_enabled;
    test_output_enabled = false;
    save_char_obj(player);
    test_output_enabled = prev;

    char cap_name[MAX_INPUT_LENGTH];
    sprintf(cap_name, "%s", capitalize("Migrator"));
    char rom_path[MIL];
    format_path(rom_path, sizeof rom_path, temp_dir, cap_name, NULL);
    ASSERT(file_exists(rom_path));

    cfg_set_default_format("json");

    Descriptor* d = mock_descriptor();
    ASSERT(load_char_obj(d, "Migrator"));

    char json_path[MIL];
    format_path(json_path, sizeof json_path, temp_dir, cap_name, ".json");
    ASSERT(file_exists(json_path));
    ASSERT(!file_exists(rom_path));

    cleanup_player_file(json_path);
    cfg_set_player_dir(old_dir);
    cfg_set_default_format(old_format);
    remove_temp_dir(temp_dir_no_slash);
    return 0;
}

void register_player_persist_tests()
{
#define REGISTER(name, fn) register_test(&player_persist_tests, (name), (fn))

    init_test_group(&player_persist_tests, "PLAYER PERSIST TESTS");
    register_test_group(&player_persist_tests);

    REGISTER("Player Round Trip JSON", test_player_round_trip_json);
    REGISTER("Player Round Trip ROM", test_player_round_trip_rom);
    REGISTER("Player Format Migration", test_player_format_migration);

#undef REGISTER
}
