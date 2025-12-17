////////////////////////////////////////////////////////////////////////////////
// reload.c - Hot-reloading of game data from disk or prototypes
////////////////////////////////////////////////////////////////////////////////

#include "reload.h"

#include "comm.h"
#include "config.h"
#include "db.h"
#include "file.h"
#include "handler.h"
#include "interp.h"

#include <entities/area.h>
#include <entities/descriptor.h>
#include <entities/help_data.h>
#include <entities/mobile.h>
#include <entities/object.h>
#include <entities/room.h>
#include <entities/room_exit.h>

#include <data/direction.h>

#include <lox/memory.h>
#include <lox/value.h>

#include <stdio.h>
#include <string.h>

// Forward declarations for reload subfunctions
static void reload_helps_impl(Mobile* ch);

void do_reload(Mobile* ch, char* argument)
{
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char(COLOR_INFO "Syntax: reload <type>\n\r", ch);
        send_to_char("Available types:\n\r", ch);
        send_to_char("  helps    - Reload all help files from disk\n\r", ch);
        send_to_char("  room     - Reload current room from prototype" COLOR_EOL, ch);
        return;
    }

    if (!str_prefix(arg, "helps")) {
        reload_helps(ch);
        return;
    }

    if (!str_prefix(arg, "room")) {
        if (ch->in_room == NULL) {
            send_to_char(COLOR_INFO "You are not in a room." COLOR_EOL, ch);
            return;
        }
        reload_room(ch, ch->in_room);
        return;
    }

    send_to_char(COLOR_INFO "Unknown reload type.\n\r", ch);
    send_to_char("Type 'reload' with no arguments for syntax." COLOR_EOL, ch);
}

void reload_helps(Mobile* ch)
{
    reload_helps_impl(ch);
}

static void reload_helps_impl(Mobile* ch)
{
    FILE* fpList;
    FILE* fp;
    char area_file[256];
    char fpArea[MAX_INPUT_LENGTH];
    int help_count = 0;
    int file_count = 0;

    send_to_char("Reloading help files...\n\r", ch);

    // Clear existing help data
    HelpData* help;
    HelpData* help_next;
    for (help = help_first; help != NULL; help = help_next) {
        help_next = help->next;
        free_help_data(help);
    }
    help_first = NULL;
    help_last = NULL;

    // Clear help areas
    HelpArea* ha;
    HelpArea* ha_next;
    for (ha = help_area_list; ha != NULL; ha = ha_next) {
        ha_next = ha->next;
        free_string(ha->filename);
        free_help_area(ha);
    }
    help_area_list = NULL;

    // Clear help_greeting
    extern char* help_greeting;
    help_greeting = NULL;

    // Open area.lst to get list of area files
    fpList = open_read_area_list();
    if (fpList == NULL) {
        send_to_char(COLOR_INFO "ERROR: Could not retrieve area list." COLOR_EOL, ch);
        return;
    }

    // Read each area file and reload helps sections
    for (;;) {
        strcpy(fpArea, fread_word(fpList));
        if (fpArea[0] == '$')
            break;

        // Skip JSON files - they don't have HELPS sections in ROM-OLC format
        const char* ext = strrchr(fpArea, '.');
        if (ext && !str_cmp(ext, ".json")) {
            continue;
        }

        sprintf(area_file, "%s%s", cfg_get_area_dir(), fpArea);
        fp = open_read_file(area_file);
        if (fp == NULL) {
            printf_to_char(ch, COLOR_INFO "WARNING: Could not open %s" COLOR_EOL, area_file);
            continue;
        }

        file_count++;

        // Scan through the area file looking for HELPS sections
        bool found_helps = false;
        for (;;) {
            int letter = fread_letter(fp);
            if (letter == EOF)
                break;

            if (letter != '#') {
                fread_to_eol(fp);
                continue;
            }

            char* word = fread_word(fp);
            if (word[0] == '$')
                break;

            if (!str_cmp(word, "HELPS")) {
                load_helps(fp, fpArea);
                found_helps = true;
                help_count++;
            }
            else {
                // Skip to the next section
                fread_to_eol(fp);
            }
        }

        close_file(fp);

        if (found_helps) {
            printf_to_char(ch, COLOR_INFO "  Loaded helps from %s" COLOR_EOL, fpArea);
        }
    }

    close_file(fpList);

    printf_to_char(ch, COLOR_INFO "Help reload complete: scanned %d files, loaded %d help sections." COLOR_EOL, 
                   file_count, help_count);
}

void reload_room(Mobile* ch, Room* room)
{
    if (room == NULL) {
        send_to_char(COLOR_INFO "Invalid room." COLOR_EOL, ch);
        return;
    }

    RoomData* room_data = room->data;
    if (room_data == NULL) {
        send_to_char(COLOR_INFO "Room has no prototype data (cannot reload)." COLOR_EOL, ch);
        return;
    }

    Area* area = room->area;
    if (area == NULL) {
        send_to_char(COLOR_INFO "Room has no area (cannot reload)." COLOR_EOL, ch);
        return;
    }

    VNUM vnum = VNUM_FIELD(room);
    printf_to_char(ch, "Reloading room [%d] %s...\n\r", vnum, NAME_FIELD(room));

    // Step 1: Nullify all inbound exits' to_room pointers
    // This prevents them from being freed when we free the room
    // Also clear the inbound_exits list so free_room doesn't iterate it
    Node* node = room->inbound_exits.front;
    while (node != NULL) {
        RoomExit* inbound = AS_ROOM_EXIT(node->value);
        inbound->to_room = NULL;
        node = node->next;
    }
    // Clear the list (free_room will free the list itself, but won't process it)
    room->inbound_exits.front = NULL;
    room->inbound_exits.back = NULL;
    room->inbound_exits.count = 0;

    // Step 2: Save all mobiles in the room (with gc_protect)
    ValueArray saved_mobs;
    init_value_array(&saved_mobs);
    gc_protect(OBJ_VAL(&saved_mobs));
    
    Mobile* mob;
    FOR_EACH_ROOM_MOB(mob, room) {
        write_value_array(&saved_mobs, OBJ_VAL(mob));
        mob_from_room(mob);
    }

    // Step 3: Save all objects in the room
    ValueArray saved_objs;
    init_value_array(&saved_objs);
    gc_protect(OBJ_VAL(&saved_objs));
    
    Object* obj;
    FOR_EACH_ROOM_OBJ(obj, room) {
        write_value_array(&saved_objs, OBJ_VAL(obj));
        obj_from_room(obj);
    }

    // Step 4: Free the room (will free outbound exits, but not inbound due to nullification)
    // free_room() will handle removing from area->rooms table and data->instances list
    free_room(room);
    room = NULL; // Prevent accidental use

    // Step 5: Recreate room from prototype
    Room* new_room_inst = new_room(room_data, area);

    // Step 6: Recreate outbound exits from RoomExitData prototypes
    for (int dir = 0; dir < DIR_MAX; dir++) {
        RoomExitData* exit_data = room_data->exit_data[dir];
        if (exit_data != NULL) {
            new_room_inst->exit[dir] = new_room_exit(exit_data, new_room_inst);
        }
    }

    // Step 7: Restore mobiles
    for (int i = 0; i < (int)saved_mobs.count; i++) {
        Mobile* mob_restore = AS_MOBILE(saved_mobs.values[i]);
        mob_to_room(mob_restore, new_room_inst);
    }
    free_value_array(&saved_mobs);

    // Step 8: Restore objects
    for (int i = 0; i < (int)saved_objs.count; i++) {
        Object* obj_restore = AS_OBJECT(saved_objs.values[i]);
        obj_to_room(obj_restore, new_room_inst);
    }
    free_value_array(&saved_objs);

    // Step 9: Notify all characters in the room
    FOR_EACH_ROOM_MOB(mob, new_room_inst) {
        if (!IS_NPC(mob)) {
            send_to_char(COLOR_INFO "\n\rThe room shimmers and reloads around you!\n\r" COLOR_EOL, mob);
            do_function(mob, &do_look, "auto");
        }
    }

    gc_protect_clear();

    send_to_char(COLOR_INFO "Room reloaded successfully." COLOR_EOL, ch);
}
