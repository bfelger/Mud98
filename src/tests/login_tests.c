////////////////////////////////////////////////////////////////////////////////
// tests/login_tests.c
////////////////////////////////////////////////////////////////////////////////

#include "tests.h"
#include "test_registry.h"
#include "mock.h"

#include <config.h>
#include <db.h>
#include <digest.h>
#include <save.h>
#include <stringutils.h>

#include <sys/stat.h>
#include <string.h>

#ifdef _MSC_VER
#include <direct.h>
#define mkdir _mkdir
#else
#include <unistd.h>
#endif

static TestGroup login_tests;

static int test_set_and_validate_password()
{
    Mobile* player = mock_player("LoginUser");

    ASSERT(set_password("hunter2", player));
    ASSERT(validate_password("hunter2", player));
    ASSERT(!validate_password("wrong", player));

    return 0;
}

static int test_password_hex_round_trip()
{
    Mobile* player = mock_player("HexUser");
    ASSERT(set_password("letmein", player));

    size_t len = player->pcdata->pwd_digest_len;
    char hex[128] = { 0 };

    bin_to_hex(hex, player->pcdata->pwd_digest, len);
    ASSERT(strlen(hex) == len * 2);

    /* Decode into a fresh pcdata and ensure the password still validates. */
    PlayerData* pd = new_player_data();
    pd->pwd_digest = NULL;
    pd->pwd_digest_len = 0;
    decode_digest(pd, hex);

    /* Reuse a mock player shell so validate_password can run. */
    Mobile* loaded = mock_player("LoadedUser");
    free_player_data(loaded->pcdata);
    loaded->pcdata = pd;
    pd->ch = loaded;

    ASSERT(validate_password("letmein", loaded));
    ASSERT(!validate_password("wrong", loaded));

    return 0;
}

static int test_password_survives_save_and_load()
{
    const char* temp_dir = "temp/login_tests/";
    const char* temp_dir_no_slash = "temp/login_tests";
    char old_player_dir[MIL];
    strncpy(old_player_dir, cfg_get_player_dir(), sizeof(old_player_dir) - 1);
    old_player_dir[sizeof(old_player_dir) - 1] = '\0';
#ifndef _MSC_VER
    mkdir("temp", 0775);
    mkdir(temp_dir, 0775);
#else
    _mkdir("temp");
    _mkdir(temp_dir);
#endif

    cfg_set_player_dir(temp_dir);

    Mobile* player = mock_player("FileUser");
    ASSERT(set_password("opensesame", player));
    bool prev_output = test_output_enabled;
    test_output_enabled = false;
    save_char_obj(player);
    test_output_enabled = prev_output;

    Descriptor* d = mock_descriptor();
    ASSERT(load_char_obj(d, "FileUser"));

    Mobile* loaded = d->character;
    ASSERT(validate_password("opensesame", loaded));
    ASSERT(!validate_password("wrong", loaded));

    /* Cleanup: remove the saved file and restore config. */
    char saved_path[MIL];
    const char* fmt = cfg_get_default_format();
    bool use_json = (fmt && !str_cmp(fmt, "json"));
    sprintf(saved_path, "%s%s%s", temp_dir, capitalize(NAME_STR(player)), use_json ? ".json" : "");
    remove(saved_path);
#ifndef _MSC_VER
    rmdir(temp_dir_no_slash);
#else
    _rmdir(temp_dir_no_slash);
#endif
    cfg_set_player_dir(old_player_dir);

    return 0;
}

void register_login_tests()
{
#define REGISTER(name, fn) register_test(&login_tests, (name), (fn))

    init_test_group(&login_tests, "LOGIN TESTS");
    register_test_group(&login_tests);

    REGISTER("Set And Validate Password", test_set_and_validate_password);
    REGISTER("Password Hex Round Trip", test_password_hex_round_trip);
    REGISTER("Password Survives Save And Load", test_password_survives_save_and_load);

#undef REGISTER
}
