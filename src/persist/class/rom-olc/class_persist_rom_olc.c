////////////////////////////////////////////////////////////////////////////////
// persist/rom-olc/class_persist_rom_olc.c
////////////////////////////////////////////////////////////////////////////////

#include "class_persist_rom_olc.h"

#include <persist/class/class_persist.h>
#include <persist/persist_io_adapters.h>

#include <data/class.h>

#include <db.h>
#include <comm.h>
#include <tablesave.h>

const ClassPersistFormat CLASS_PERSIST_ROM_OLC = { 
    .name = "rom-olc", 
    .load = class_persist_rom_olc_load, 
    .save = class_persist_rom_olc_save 
};

#ifdef U
#undef U
#endif
#define U(x) (uintptr_t)(x)

extern bool test_output_enabled;

extern SaveTableEntry class_save_table[]; // from class.c
extern Class tmp_class;

PersistResult class_persist_rom_olc_load(const PersistReader* reader, const char* filename)
{
    if (!reader || reader->ops != &PERSIST_FILE_STREAM_OPS)
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "class ROM load requires FILE reader", -1 };
    FILE* fp = (FILE*)reader->ctx;
    char* word;
    int i;

    int maxclass = fread_number(fp);

    if (class_table)
        free(class_table);

    size_t new_size = sizeof(Class) * ((size_t)maxclass + 1);
    if (!test_output_enabled)
        printf_log("Creating class of length %d, size %zu", maxclass + 1, new_size);

    if ((class_table = calloc((size_t)maxclass + 1, sizeof(Class))) == NULL) {
        perror("class_load_rom: Could not allocate class_table!");
        return (PersistResult){ PERSIST_ERR_INTERNAL, "alloc failed", -1 };
    }

    i = 0;

    while (true) {
        word = fread_word(fp);

        if (str_cmp(word, "#CLASS")) {
            bugf("load_class_table : word %s", word);
            return (PersistResult){ PERSIST_ERR_FORMAT, "bad class header", -1 };
        }

        load_struct(fp, U(&tmp_class), class_save_table, U(&class_table[i++]));

        if (i == maxclass) {
            if (!test_output_enabled)
                printf_log("Class table loaded.");
            class_count = maxclass;
            class_table[i].name = NULL;
            return (PersistResult){ PERSIST_OK, NULL, -1 };
        }
    }
}

PersistResult class_persist_rom_olc_save(const PersistWriter* writer, const char* filename)
{
    if (!writer || writer->ops != &PERSIST_FILE_WRITER_OPS)
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "class ROM save requires FILE writer", -1 };
    FILE* fp = (FILE*)writer->ctx;

    fprintf(fp, "%d\n\n", class_count);

    for (const Class* temp = class_table; temp && !IS_NULLSTR(temp->name); ++temp) {
        fprintf(fp, "#CLASS\n");
        save_struct(fp, U(&tmp_class), class_save_table, U(temp));
        fprintf(fp, "#END\n\n");
    }

    return (PersistResult){ PERSIST_OK, NULL, -1 };
}

#ifdef U
#undef U
#endif
