////////////////////////////////////////////////////////////////////////////////
// data/class.c
////////////////////////////////////////////////////////////////////////////////

#include "class.h"

#include "item.h"

#include <comm.h>
#include <config.h>
#include <db.h>
#include <tablesave.h>
#include <persist/class/class_persist.h>

extern bool test_output_enabled;

Class* class_table = NULL;
int class_count = 0;

Class tmp_class;

#ifdef U
#define OLD_U U
#endif
#define U(x)    (uintptr_t)(x)

const SaveTableEntry class_save_table[] = {
    { "name",	        FIELD_STRING,           U(&tmp_class.name),	        0,		            0   },
    { "wname",	        FIELD_STRING,           U(&tmp_class.who_name),	    0,		            0   },
    { "basegrp",        FIELD_STRING,           U(&tmp_class.base_group),   0,                  0   },
    { "dfltgrp",        FIELD_STRING,           U(&tmp_class.default_group),0,                  0   },
    { "weap",           FIELD_VNUM,             U(&tmp_class.weapon),       0,                  0   },
    { "guild",          FIELD_VNUM_ARRAY,       U(&tmp_class.guild),        U(MAX_GUILD),       0   },
    { "pstat",          FIELD_INT16,            U(&tmp_class.prime_stat),   0,                  0   },
    { "scap",           FIELD_INT16,            U(&tmp_class.skill_cap),    0,                  0   },
    { "t0_0",           FIELD_INT16,            U(&tmp_class.thac0_00),     0,                  0   },
    { "t0_32",          FIELD_INT16,            U(&tmp_class.thac0_32),     0,                  0   },
    { "hpmin",          FIELD_INT16,            U(&tmp_class.hp_min),       0,                  0   },
    { "hpmax",          FIELD_INT16,            U(&tmp_class.hp_max),       0,                  0   },
    { "fmana",          FIELD_BOOL,             U(&tmp_class.fMana),        0,                  0   },
    { "titles",         FIELD_N_STRING_ARRAY,   U(&tmp_class.titles),       U((MAX_LEVEL+1)*2), 0   },
    { "start_loc",      FIELD_VNUM,             U(&tmp_class.start_loc),    0,                  0   },
    { NULL,		        0,				        0,			                0,		            0   }
};

void load_class_table()
{
    PersistResult res = class_persist_load(cfg_get_classes_file());
    if (!persist_succeeded(res)) {
        bugf("load_class_table: failed to load classes (%s)", res.message ? res.message : "unknown error");
        exit(1);
    }

    if (!test_output_enabled)
        printf_log("Class table loaded (%d classes).", class_count);
}

void save_class_table()
{
    PersistResult res = class_persist_save(cfg_get_classes_file());
    if (!persist_succeeded(res))
        bugf("save_class_table: failed to save classes (%s)", res.message ? res.message : "unknown error");
}

#undef U
#ifdef OLD_U
#define U OLD_U
#undef OLD_U
#endif
