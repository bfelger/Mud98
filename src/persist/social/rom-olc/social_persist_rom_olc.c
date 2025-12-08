////////////////////////////////////////////////////////////////////////////////
// persist/social/rom-olc/social_persist_rom_olc.c
////////////////////////////////////////////////////////////////////////////////

#include "social_persist_rom_olc.h"

#include <persist/persist_io_adapters.h>

#include <tablesave.h>

#include <comm.h>
#include <db.h>

#include <data/social.h>

#include <stdlib.h>

#ifdef U
#undef U
#endif
#define U(x) (uintptr_t)(x)

extern bool test_output_enabled;

static Social tmp_soc;

static const SaveTableEntry socialsavetable[] = {
    { "name",          FIELD_STRING, U(&tmp_soc.name),         0, 0 },
    { "char_no_arg",   FIELD_STRING, U(&tmp_soc.char_no_arg),  0, 0 },
    { "others_no_arg", FIELD_STRING, U(&tmp_soc.others_no_arg),0, 0 },
    { "char_found",    FIELD_STRING, U(&tmp_soc.char_found),   0, 0 },
    { "others_found",  FIELD_STRING, U(&tmp_soc.others_found), 0, 0 },
    { "vict_found",    FIELD_STRING, U(&tmp_soc.vict_found),   0, 0 },
    { "char_auto",     FIELD_STRING, U(&tmp_soc.char_auto),    0, 0 },
    { "others_auto",   FIELD_STRING, U(&tmp_soc.others_auto),  0, 0 },
    { NULL,            0,            0,                        0, 0 }
};

const SocialPersistFormat SOCIAL_PERSIST_ROM_OLC = {
    .name = "rom-olc",
    .load = social_persist_rom_olc_load,
    .save = social_persist_rom_olc_save,
};

PersistResult social_persist_rom_olc_load(const PersistReader* reader, const char* filename)
{
    if (!reader || reader->ops != &PERSIST_FILE_STREAM_OPS)
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "social ROM load requires FILE reader", -1 };
    FILE* fp = (FILE*)reader->ctx;
    extern Social* social_table;
    extern int social_count;
    char* word;

    if (fscanf(fp, "%d\n", &social_count) < 1)
        return (PersistResult){ PERSIST_ERR_FORMAT, "social ROM load: missing count", -1 };

    if ((social_table = calloc((size_t)social_count + 1, sizeof(Social))) == NULL)
        return (PersistResult){ PERSIST_ERR_INTERNAL, "social ROM load: alloc failed", -1 };

    if (!test_output_enabled)
        printf_log("Creating social_table of length %d, size %zu", social_count + 1,
            sizeof(Social) * ((size_t)social_count + 1));

    for (int i = 0; i < social_count; ++i) {
        word = fread_word(fp);
        if (str_cmp(word, "#SOCIAL"))
            return (PersistResult){ PERSIST_ERR_FORMAT, "load_social_table : Expected '#SOCIAL'", -1 };
        load_struct(fp, U(&tmp_soc), socialsavetable, U(&social_table[i]));
    }

    social_table[social_count].name = str_dup("");
    return (PersistResult){ PERSIST_OK, NULL, -1 };
}

PersistResult social_persist_rom_olc_save(const PersistWriter* writer, const char* filename)
{
    if (!writer || writer->ops != &PERSIST_FILE_WRITER_OPS)
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "social ROM save requires FILE writer", -1 };
    FILE* fp = (FILE*)writer->ctx;
    extern Social* social_table;
    extern int social_count;

    fprintf(fp, "%d\n\n", social_count);

    for (int i = 0; i < social_count; ++i) {
        fprintf(fp, "#SOCIAL\n");
        save_struct(fp, U(&tmp_soc), socialsavetable, U(&social_table[i]));
        fprintf(fp, "#END\n\n");
    }

    return (PersistResult){ PERSIST_OK, NULL, -1 };
}

#ifdef U
#undef U
#endif
