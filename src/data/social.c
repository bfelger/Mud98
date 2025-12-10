////////////////////////////////////////////////////////////////////////////////
// data/social.c
////////////////////////////////////////////////////////////////////////////////

#include "social.h"

#include <comm.h>
#include <config.h>
#include <db.h>
#include <tables.h>
#include <persist/social/social_persist.h>

extern bool test_output_enabled;

Social* social_table;
int social_count;
void load_social_table()
{
    PersistResult res = social_persist_load(cfg_get_socials_file());
    if (!persist_succeeded(res)) {
        bugf("load_social_table: failed to load socials (%s)",
            res.message ? res.message : "unknown error");
        exit(1);
    }

    if (!test_output_enabled)
        printf_log("Social table loaded (%d socials).", social_count);
}

void save_social_table(void)
{
    PersistResult res = social_persist_save(cfg_get_socials_file());
    if (!persist_succeeded(res))
        bugf("save_social_table: failed to save socials (%s)",
            res.message ? res.message : "unknown error");
}
