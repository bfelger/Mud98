////////////////////////////////////////////////////////////////////////////////
// entities/faction.c
////////////////////////////////////////////////////////////////////////////////

#include "faction.h"

#include "act_comm.h"

#include <comm.h>
#include <db.h>
#include <handler.h>
#include <lookup.h>
#include <stringutils.h>

#include <entities/area.h>
#include <entities/mobile.h>
#include <entities/player_data.h>

#include <lox/memory.h>

#include <olc/string_edit.h>

#include <stdio.h>
#include <string.h>

Table faction_table;

static Faction* faction_free = NULL;
static int faction_count = 0;
static int faction_perm_count = 0;

#define FACTION_REP_MAX         42000
#define FACTION_REP_MIN         -42000
#define FACTION_REP_FRIENDLY    3000
#define FACTION_REP_HOSTILE    -3000

typedef struct faction_level_info_t {
    FactionStanding standing;
    int min_value;
    int max_value;
    const char* name;
} FactionLevelInfo;

static FactionLevelInfo faction_levels[FACTION_STANDING_COUNT] = {
    { FACTION_STANDING_HATED,       FACTION_REP_MIN,-6000,          "Hated"     },
    { FACTION_STANDING_HOSTILE,     -5999,          -3000,          "Hostile"   },
    { FACTION_STANDING_UNFRIENDLY,  -2999,          -1,             "Unfriendly"},
    { FACTION_STANDING_NEUTRAL,     0,              2999,           "Neutral"   },
    { FACTION_STANDING_FRIENDLY,    3000,           8999,           "Friendly"  },
    { FACTION_STANDING_HONORED,     9000,           20999,          "Honored"   },
    { FACTION_STANDING_REVERED,     21000,          41999,          "Revered"   },
    { FACTION_STANDING_EXALTED,     42000,          FACTION_REP_MAX,"Exalted"   },
};

static Faction* new_faction()
{
    LIST_ALLOC_PERM(faction, Faction);

    gc_protect(OBJ_VAL(faction));
    init_header(&faction->header, OBJ_FACTION);

    faction->area = NULL;
    faction->header.script = 0;
    init_value_array(&faction->allies);
    init_value_array(&faction->enemies);
    faction->default_standing = 0;

    return faction;
}

static FactionReputation* find_reputation(PlayerData* pcdata, VNUM vnum)
{
    if (pcdata == NULL)
        return NULL;

    FactionReputationList* list = &pcdata->reputations;
    if (list->entries == NULL)
        return NULL;

    for (size_t i = 0; i < list->count; ++i) {
        if (list->entries[i].vnum == vnum)
            return &list->entries[i];
    }
    return NULL;
}

static FactionReputation* ensure_reputation(PlayerData* pcdata, VNUM vnum, int default_value)
{
    if (pcdata == NULL || vnum == 0)
        return NULL;

    FactionReputation* rep = find_reputation(pcdata, vnum);
    if (rep != NULL)
        return rep;

    FactionReputationList* list = &pcdata->reputations;
    if (list->count == list->capacity) {
        size_t old_capacity = list->capacity;
        size_t new_capacity = old_capacity == 0 ? 4 : old_capacity * 2;
        FactionReputation* temp = alloc_mem(new_capacity * sizeof(FactionReputation));
        memset(temp, 0, new_capacity * sizeof(FactionReputation));
        if (list->entries != NULL) {
            memcpy(temp, list->entries, list->count * sizeof(FactionReputation));
            free_mem(list->entries, old_capacity * sizeof(FactionReputation));
        }
        list->entries = temp;
        list->capacity = new_capacity;
    }

    rep = &list->entries[list->count++];
    rep->vnum = vnum;
    rep->value = default_value;
    return rep;
}

Faction* faction_create(VNUM vnum)
{
    if (vnum <= 0) {
        bug("faction_create: invalid vnum %" PRVNUM, vnum);
        return NULL;
    }

    Value existing;
    if (table_get_vnum(&faction_table, vnum, &existing)) {
        // I have no problem with defining factions multiple times in the area
        // files; that way it doesn't matter what order the files are loaded in.
        //bug("faction_create: duplicate vnum %" PRVNUM, vnum);
        return AS_FACTION(existing);
    }

    Faction* faction = new_faction();
    VNUM_FIELD(faction) = vnum;
    faction->area = current_area_data;

    table_set_vnum(&faction_table, vnum, OBJ_VAL(faction));
    return faction;
}

Faction* get_faction(VNUM vnum)
{
    if (vnum == 0)
        return NULL;

    Value value;
    if (table_get_vnum(&faction_table, vnum, &value) && IS_FACTION(value))
        return AS_FACTION(value);

    return NULL;
}

Faction* get_mob_faction(Mobile* mob)
{
    if (mob == NULL)
        return NULL;

    return get_faction(get_mob_faction_vnum(mob));
}

VNUM get_mob_faction_vnum(Mobile* mob)
{
    if (mob == NULL)
        return 0;

    if (mob->faction_vnum != 0)
        return mob->faction_vnum;

    if (mob->prototype != NULL)
        return mob->prototype->faction_vnum;

    return 0;
}

static int clamp_value(int value)
{
    if (value < FACTION_REP_MIN)
        return FACTION_REP_MIN;
    if (value > FACTION_REP_MAX)
        return FACTION_REP_MAX;
    return value;
}

FactionStanding faction_standing_from_value(int value)
{
    for (size_t i = 0; i < FACTION_STANDING_COUNT; ++i) {
        if (value >= faction_levels[i].min_value && value <= faction_levels[i].max_value)
            return faction_levels[i].standing;
    }
    return FACTION_STANDING_NEUTRAL;
}

const char* faction_standing_name(FactionStanding standing)
{
    for (size_t i = 0; i < FACTION_STANDING_COUNT; ++i) {
        if (faction_levels[i].standing == standing)
            return faction_levels[i].name;
    }
    return "Unknown";
}

const char* faction_standing_name_from_value(int value)
{
    return faction_standing_name(faction_standing_from_value(value));
}

bool faction_is_friendly_value(int value)
{
    return value >= FACTION_REP_FRIENDLY;
}

bool faction_is_hostile_value(int value)
{
    return value <= FACTION_REP_HOSTILE;
}

static int get_default_standing(Faction* faction)
{
    if (faction == NULL)
        return 0;
    return clamp_value(faction->default_standing);
}

int faction_get_value(Mobile* ch, Faction* faction, bool touch)
{
    if (ch == NULL || IS_NPC(ch) || faction == NULL)
        return 0;

    FactionReputation* rep = touch
        ? ensure_reputation(ch->pcdata, VNUM_FIELD(faction), get_default_standing(faction))
        : find_reputation(ch->pcdata, VNUM_FIELD(faction));
    if (rep != NULL)
        return rep->value;

    return get_default_standing(faction);
}

void faction_touch(Mobile* ch, Faction* faction)
{
    if (ch == NULL || IS_NPC(ch) || faction == NULL)
        return;

    ensure_reputation(ch->pcdata, VNUM_FIELD(faction), get_default_standing(faction));
}

void faction_set(PlayerData* pcdata, VNUM vnum, int value)
{
    if (pcdata == NULL || vnum == 0)
        return;

    Faction* faction = get_faction(vnum);
    FactionReputation* rep = ensure_reputation(pcdata, vnum,
        faction != NULL ? get_default_standing(faction) : 0);
    if (rep != NULL)
        rep->value = clamp_value(value);
}

static void notify_reputation_change(Mobile* ch, Faction* faction, int old_value, int new_value)
{
    if (ch == NULL || faction == NULL)
        return;

    const char* direction = new_value > old_value ? "increases" : "decreases";
    const char* old_name = faction_standing_name_from_value(old_value);
    const char* new_name = faction_standing_name_from_value(new_value);
    char buf[MAX_STRING_LENGTH];

    if (!str_cmp(old_name, new_name)) {
        sprintf(buf, COLOR_INFO "Your reputation with %s %s (%s)." COLOR_EOL,
            NAME_STR(faction), direction, new_name);
    }
    else {
        sprintf(buf, COLOR_INFO "Your reputation with %s %s (%s -> %s)." COLOR_EOL,
            NAME_STR(faction), direction, old_name, new_name);
    }

    send_to_char(buf, ch);
}

void faction_adjust(Mobile* ch, Faction* faction, int delta)
{
    if (ch == NULL || IS_NPC(ch) || faction == NULL || delta == 0)
        return;

    FactionReputation* rep = ensure_reputation(ch->pcdata, VNUM_FIELD(faction),
        get_default_standing(faction));
    if (rep == NULL)
        return;

    int old_value = rep->value;
    int clamped = clamp_value(rep->value + delta);
    if (clamped == old_value)
        return;

    rep->value = clamped;
    notify_reputation_change(ch, faction, old_value, clamped);
}

static void add_relation(ValueArray* array, VNUM vnum)
{
    if (vnum <= 0 || array == NULL)
        return;
    write_value_array(array, INT_VAL(vnum));
}

void load_factions(FILE* fp)
{
    if (global_areas.count == 0) {
        bug("Load_factions: no #AREA seen yet.", 0);
        exit(1);
    }

    for (;;) {
        char letter = fread_letter(fp);
        if (letter != '#') {
            bug("Load_factions: # not found.", 0);
            exit(1);
        }

        VNUM vnum = fread_number(fp);
        if (vnum == 0)
            break;

        fBootDb = false;
        Faction* faction = faction_create(vnum);
        faction->area = LAST_AREA_DATA;
        fBootDb = true;

        bool done = false;
        for (;;) {
            char* word = fread_word(fp);

            if (!str_cmp(word, "Name")) {
                SET_NAME(faction, fread_lox_string(fp));
            }
            else if (!str_cmp(word, "DefaultStanding") || !str_cmp(word, "Default")) {
                faction->default_standing = clamp_value(fread_number(fp));
            }
            else if (!str_cmp(word, "Allies")) {
                for (;;) {
                    VNUM ally = fread_number(fp);
                    if (ally == 0)
                        break;
                    add_relation(&faction->allies, ally);
                }
            }
            else if (!str_cmp(word, "Opposing") || !str_cmp(word, "Opposition")) {
                for (;;) {
                    VNUM enemy = fread_number(fp);
                    if (enemy == 0)
                        break;
                    add_relation(&faction->enemies, enemy);
                }
            }
            else if (!str_cmp(word, "End")) {
                done = true;
                break;
            }
            else {
                bug("Load_factions: unknown keyword '%s'.", word);
                exit(1);
            }
        }

        if (!done) {
            bug("Load_factions: faction %" PRVNUM " not terminated correctly.", vnum);
            exit(1);
        }
    }
}

bool faction_block_player_attack(Mobile* ch, Mobile* victim)
{
    if (ch == NULL || victim == NULL || IS_NPC(ch) || !IS_NPC(victim))
        return false;

    Faction* faction = get_mob_faction(victim);
    if (faction == NULL)
        return false;

    int value = faction_get_value(ch, faction, true);
    if (!faction_is_friendly_value(value))
        return false;

    char buf[MAX_STRING_LENGTH];
    sprintf(buf, "You are %s with %s and refuse to attack.\n\r",
        faction_standing_name_from_value(value), NAME_STR(faction));
    send_to_char(buf, ch);
    return true;
}

#define ANGRY_SHOPKEEPER_MESSAGE_COUNT 6
static const char* angry_shopkeeper_messages[ANGRY_SHOPKEEPER_MESSAGE_COUNT] = {
    "We do not trade with our enemies.",
    "Begone, enemy of %s!",
    "I will not serve someone hostile to %s.",
    "Your presence offends me.",
    "Leave at once! We do not deal with your kind here.",
    "Disgusting! Stay away from my shop!",
};

bool faction_block_shopkeeper(Mobile* keeper, Mobile* ch)
{
    if (keeper == NULL || ch == NULL || IS_NPC(ch))
        return false;

    Faction* faction = get_mob_faction(keeper);
    if (faction == NULL)
        return false;

    int value = faction_get_value(ch, faction, true);
    if (value >= 0)
        return false;

    int msg_idx = number_range(0, ANGRY_SHOPKEEPER_MESSAGE_COUNT - 1);
    char msg_buf[MAX_STRING_LENGTH];
    strcpy(msg_buf, angry_shopkeeper_messages[msg_idx]);

    if (strstr(msg_buf, "%s") != NULL) {
        char* msg_str = str_dup(msg_buf);
        msg_str = string_replace(msg_str, "%s", NAME_STR(faction));
        strcpy(msg_buf, msg_str);
        free_string(msg_str);
    }

    do_function(keeper, &do_say, msg_buf);

    char buf[MAX_STRING_LENGTH];
    sprintf(buf, "%s refuses to do business with you.\n\r", PERS(keeper, ch));
    send_to_char(buf, ch);
    return true;
}

void faction_handle_kill(Mobile* killer, Mobile* victim)
{
    if (killer == NULL || victim == NULL || IS_NPC(killer))
        return;

    Faction* faction = get_mob_faction(victim);
    if (faction != NULL)
        faction_adjust(killer, faction, -5);

    if (faction == NULL || faction->enemies.count == 0)
        return;

    for (int i = 0; i < faction->enemies.count; ++i) {
        Value entry = faction->enemies.values[i];
        if (!IS_INT(entry))
            continue;
        Faction* opposing = get_faction((VNUM)AS_INT(entry));
        if (opposing != NULL)
            faction_adjust(killer, opposing, 5);
    }
}

void faction_show_reputations(Mobile* ch)
{
    if (ch == NULL || IS_NPC(ch)) {
        send_to_char(COLOR_INFO "You have no recorded reputations." COLOR_EOL, ch);
        return;
    }

    PlayerData* pcdata = ch->pcdata;
    FactionReputationList* list = &pcdata->reputations;
    if (list->entries == NULL || list->count == 0) {
        send_to_char(COLOR_INFO "You have not interacted with any factions yet." COLOR_EOL, ch);
        return;
    }

    send_to_char(COLOR_TITLE "Faction                      Standing       Value" COLOR_EOL, ch);
    send_to_char(COLOR_DECOR_1 "-------------------------------------------------" COLOR_EOL, ch);

    for (size_t i = 0; i < list->count; ++i) {
        FactionReputation* rep = &list->entries[i];
        if (rep->vnum == 0)
            continue;

        Faction* faction = get_faction(rep->vnum);
        char name_buf[MAX_INPUT_LENGTH];
        if (faction != NULL)
            strncpy(name_buf, NAME_STR(faction), sizeof(name_buf));
        else
            sprintf(name_buf, "Unknown (%" PRVNUM ")", rep->vnum);

        char buf[MAX_STRING_LENGTH];
        sprintf(buf, COLOR_ALT_TEXT_1 "%-28s %-12s %7d" COLOR_EOL,
            name_buf, faction_standing_name_from_value(rep->value), rep->value);
        send_to_char(buf, ch);
    }
}
