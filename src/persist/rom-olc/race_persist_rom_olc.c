////////////////////////////////////////////////////////////////////////////////
// persist/rom-olc/race_persist_rom_olc.c
////////////////////////////////////////////////////////////////////////////////

#include "race_persist_rom_olc.h"

#include <persist/persist_io_adapters.h>

#include <data/race.h>
#include <tablesave.h>
#include <db.h>
#include <comm.h>

#ifdef U
#undef U
#endif
#define U(x) (uintptr_t)(x)

extern bool test_output_enabled;

extern SaveTableEntry race_save_table[]; // from race.c
extern Race tmp_race;

PersistResult race_persist_rom_load(const PersistReader* reader, const char* filename)
{
    if (!reader || reader->ops != &PERSIST_FILE_STREAM_OPS)
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "race ROM load requires FILE reader", -1 };
    FILE* fp = (FILE*)reader->ctx;
    char* word;
    int i;

    int maxrace = fread_number(fp);

    if (race_table)
        free(race_table);

    size_t new_size = sizeof(Race) * ((size_t)maxrace + 1);
    if (!test_output_enabled)
    printf_log("Creating race_table of length %d, size %zu", maxrace + 1, new_size);

    if ((race_table = calloc((size_t)maxrace + 1, sizeof(Race))) == NULL) {
        perror("race_load_rom: Could not allocate race_table!");
        return (PersistResult){ PERSIST_ERR_INTERNAL, "alloc failed", -1 };
    }

    i = 0;

    while (true) {
        word = fread_word(fp);

        if (str_cmp(word, "#race")) {
            bugf("race_load_rom : word %s", word);
            return (PersistResult){ PERSIST_ERR_FORMAT, "bad race header", -1 };
        }

        load_struct(fp, U(&tmp_race), race_save_table, U(&race_table[i++]));

        if (i == maxrace) {
            if (!test_output_enabled)
                printf_log("Race table loaded.");
            race_count = maxrace;
            race_table[i].name = NULL;
            break;
        }
    }

    init_race_table_lox();
    return (PersistResult){ PERSIST_OK, NULL, -1 };
}

PersistResult race_persist_rom_save(const PersistWriter* writer, const char* filename)
{
    if (!writer || writer->ops != &PERSIST_FILE_WRITER_OPS)
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "race ROM save requires FILE writer", -1 };
    FILE* fp = (FILE*)writer->ctx;

    const Race* temp;
    int cnt = 0;

    for (temp = race_table; temp && !IS_NULLSTR(temp->name); temp++)
        cnt++;

    fprintf(fp, "%d\n\n", cnt);

    for (temp = race_table, cnt = 0; temp && !IS_NULLSTR(temp->name); temp++) {
        fprintf(fp, "#RACE\n");
        save_struct(fp, U(&tmp_race), race_save_table, U(temp));
        fprintf(fp, "#END\n\n");
    }

    return (PersistResult){ PERSIST_OK, NULL, -1 };
}

#ifdef U
#undef U
#endif
