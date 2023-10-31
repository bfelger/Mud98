////////////////////////////////////////////////////////////////////////////////
// config.c
////////////////////////////////////////////////////////////////////////////////

#include "merc.h"

#include "config.h"

#include "file.h"
#include "stringutils.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#include <winerror.h>
#else
#include <strings.h>
#endif

#pragma GCC diagnostic push
#pragma GCC ignored "-Wunused-variable"

#define DEBUG                   false
#define DEBUGF                  if (DEBUG) printf

#define BACKING_STR_SIZE        256

#define DEFINE_BACKING(val, type, default_val)                                 \
    static type _ ## val = default_val;

#define DEFINE_GETTER(val, type)                                               \
    type cfg_get_ ## val()                                                     \
    {                                                                          \
        return _ ## val;                                                       \
    }

#define DEFINE_SETTER(val, type)                                               \
    void cfg_set_ ## val(const type new_val)                                   \
    {                                                                          \
        _ ## val = new_val;                                                    \
    } 

#define DEFINE_CONFIG(val, type, default_val)                                  \
    DEFINE_BACKING(val, type, default_val)                                     \
    DEFINE_GETTER(val, type)                                                   \
    DEFINE_SETTER(val, type)

#define DEFINE_STR_BACKING(val)                                                \
    static char _ ## val[BACKING_STR_SIZE] = { 0 };

#define DEFINE_STR_GETTER(val, default_val)                                    \
    const char* cfg_get_ ## val()                                              \
    {                                                                          \
        if (!_ ## val[0])                                                      \
            return default_val;                                                \
        return _ ## val;                                                       \
    }

#define DEFINE_STR_SETTER(val)                                                 \
    void cfg_set_ ## val(const char* new_val)                                  \
    {                                                                          \
        strcpy(_ ## val, new_val);                                             \
    }

#define DEFINE_STR_CONFIG(val, default_val)                                    \
    DEFINE_STR_BACKING(val)                                                    \
    DEFINE_STR_GETTER(val, default_val)                                        \
    DEFINE_STR_SETTER(val)                                                     \

#define DEFINE_DIR_CONFIG(val, default_val)                                    \
    DEFINE_STR_BACKING(val)                                                    \
    DEFINE_STR_GETTER(val, default_val)                                        \
    void cfg_set_ ## val(const char* new_val)                                  \
    {                                                                          \
        strcpy(_ ## val, new_val);                                             \
        size_t len = strlen(new_val);                                          \
        if (new_val[len - 1] != '\\' && new_val[len - 1] != '/')               \
            strcat(_ ## val, "/");                                             \
    }

#define DEFINE_OPEN_CFG_FILE(val, subdir, rw)                                  \
    FILE* open_ ## rw ## _ ## val()                                            \
    {                                                                          \
        char temp[256] = { 0 };                                                \
        sprintf(temp, "%s%s",                                                  \
        cfg_get_ ## subdir(),                                                  \
        cfg_get_ ## val());                                                    \
        return open_ ## rw ##_file(temp);                                      \
    }

#define DEFINE_FILE_EXISTS(val, subdir)                                        \
    bool val ## _exists()                                                      \
    {                                                                          \
        char temp[256] = { 0 };                                                \
        sprintf(temp, "%s%s",                                                  \
        cfg_get_ ## subdir(),                                                  \
        cfg_get_ ## val());                                                    \
        return file_exists(temp);                                              \
    }

#define DEFINE_FILE_CONFIG(val, subdir, default_val)                           \
    DEFINE_STR_CONFIG(val, default_val)                                        \
    DEFINE_OPEN_CFG_FILE(val, subdir, read)                                    \
    DEFINE_OPEN_CFG_FILE(val, subdir, write)                                   \
    DEFINE_FILE_EXISTS(val, subdir)

#define DEFINE_LOG_CONFIG(val, subdir, default_val)                            \
    DEFINE_STR_CONFIG(val, default_val)                                        \
    DEFINE_OPEN_CFG_FILE(val, subdir, append)                                  \
    DEFINE_FILE_EXISTS(val, subdir)

#define DEFAULT_MUD_NAME            "Mud98"

// Connection Info
#define DEFAULT_HOSTNAME            "localhost"
#define DEFAULT_TELNET_ENABLED      false
#define DEFAULT_TELNET_PORT         4000
#define DEFAULT_TLS_ENABLED         true
#define DEFAULT_TLS_PORT            4043
#define DEFAULT_KEYS_DIR            "keys/"
#define DEFAULT_CERT_FILE           "mud98.pem"
#define DEFAULT_PKEY_FILE           "mud98.key"
#define DEFAULT_DBG_TELOPT          false

// GMCP Values
#define DEFAULT_GMCP_ENABLED        true

// MCCP Values
#define DEFAULT_MCCP2_ENABLED       true
#define DEFAULT_MCCP3_ENABLED       true

// MSDP Values
#define DEFAULT_MSDP_ENABLED        true

// MSSP Values
#define DEFAULT_MSSP_ENABLED        true
#define DEFAULT_CODEBASE            "Mud98"
#define DEFAULT_CONTACT             "your@email.com"
#define DEFAULT_CREATED             2023
#define DEFAULT_ICON                "http://example.com/icon.gif"
#define DEFAULT_LANGUAGE            "En-US"
#define DEFAULT_LOCATION            "US"
#define DEFAULT_MIN_AGE             13
#define DEFAULT_WEBSITE             "https://github.com/bfelger/Mud98"
#define DEFAULT_FAMILY              "ROM"
#define DEFAULT_GENRE               "Fantasy"
#define DEFAULT_GAMEPLAY            "Hack and Slash"
#define DEFAULT_STATUS              "Live"
#define DEFAULT_SUBGENRE            "High Fantasy"

// Assume it is run from the area folder (as in the olden days of yore)
#define DEFAULT_BASE_DIR            "../"
#define DEFAULT_CONFIG_FILE         "mud98.cfg"

// Area Files
#define DEFAULT_AREA_DIR            "area/"
#define DEFAULT_AREA_LIST           "area.lst"
#define DEFAULT_MUSIC_FILE          "music.txt"
#define DEFAULT_BUG_FILE            "bugs.txt"
#define DEFAULT_TYPO_FILE           "typos.txt"
#define DEFAULT_NOTE_FILE           "notes.not"
#define DEFAULT_IDEA_FILE           "ideas.not"
#define DEFAULT_PENALTY_FILE        "penalty.not"
#define DEFAULT_NEWS_FILE           "news.not"
#define DEFAULT_CHANGES_FILE        "change.not"
#define DEFAULT_SHUTDOWN_FILE       "shutdown.txt"
#define DEFAULT_BAN_FILE            "ban.txt"

// Data Files
#define DEFAULT_DATA_DIR            "data/"
#define DEFAULT_PROGS_DIR           "progs/"
#define DEFAULT_SOCIALS_FILE        "socials"
#define DEFAULT_GROUPS_FILE         "groups"
#define DEFAULT_SKILLS_FILE         "skills"
#define DEFAULT_COMMANDS_FILE       "commands"
#define DEFAULT_RACES_FILE          "races"
#define DEFAULT_CLASSES_FILE        "classes"

// Temp Files
#define DEFAULT_TEMP_DIR            "temp/"
#define DEFAULT_MEM_DUMP_FILE       "mem.dmp"
#define DEFAULT_MOB_DUMP_FILE       "mob.dmp"
#define DEFAULT_OBJ_DUMP_FILE       "obj.dmp"

// Other Top-Level Dirs
#define DEFAULT_PLAYER_DIR          "player/"
#define DEFAULT_GODS_DIR            "gods/"

// Gameplay Defaults
#define DEFAULT_CHARGEN_CUSTOM      true
#define DEFAULT_RECALL              3001
#define DEFAULT_START_LOC           3700

DEFINE_STR_CONFIG(mud_name,         DEFAULT_MUD_NAME)

// Connnection Info
DEFINE_STR_CONFIG(hostname,         DEFAULT_HOSTNAME)
DEFINE_CONFIG(telnet_enabled,       bool,       DEFAULT_TELNET_ENABLED)
DEFINE_CONFIG(telnet_port,          int,        DEFAULT_TELNET_PORT)
DEFINE_CONFIG(tls_enabled,          bool,       DEFAULT_TLS_ENABLED)
DEFINE_CONFIG(tls_port,             int,        DEFAULT_TLS_PORT)
DEFINE_DIR_CONFIG(keys_dir,         DEFAULT_KEYS_DIR)
DEFINE_FILE_CONFIG(cert_file,       keys_dir,   DEFAULT_CERT_FILE)
DEFINE_FILE_CONFIG(pkey_file,       keys_dir,   DEFAULT_PKEY_FILE)
DEFINE_CONFIG(debug_telopt,         bool,       DEFAULT_DBG_TELOPT)

// GMCP Values
DEFINE_CONFIG(gmcp_enabled,         bool,       DEFAULT_GMCP_ENABLED)

// MCCP Values
DEFINE_CONFIG(mccp2_enabled,        bool,       DEFAULT_MCCP2_ENABLED)
DEFINE_CONFIG(mccp3_enabled,        bool,       DEFAULT_MCCP3_ENABLED)

// MSDP Values
DEFINE_CONFIG(msdp_enabled,         bool,       DEFAULT_MSDP_ENABLED)

// MSSP Values
DEFINE_CONFIG(mssp_enabled,         bool,       DEFAULT_MSSP_ENABLED)
DEFINE_STR_CONFIG(codebase,         DEFAULT_CODEBASE)
DEFINE_STR_CONFIG(contact,          DEFAULT_CONTACT)
DEFINE_CONFIG(created,              int,        DEFAULT_CREATED)
DEFINE_STR_CONFIG(discord,          "")
DEFINE_STR_CONFIG(icon,             DEFAULT_ICON)
DEFINE_STR_CONFIG(language,         DEFAULT_LANGUAGE)
DEFINE_STR_CONFIG(location,         DEFAULT_LOCATION)
DEFINE_CONFIG(min_age,              int,        DEFAULT_MIN_AGE)
DEFINE_STR_CONFIG(website,          DEFAULT_WEBSITE)
DEFINE_STR_CONFIG(family,           DEFAULT_FAMILY)
DEFINE_STR_CONFIG(genre,            DEFAULT_GENRE)
DEFINE_STR_CONFIG(gameplay,         DEFAULT_GAMEPLAY)
DEFINE_STR_CONFIG(status,           DEFAULT_STATUS)
DEFINE_STR_CONFIG(subgenre,         DEFAULT_SUBGENRE)

// Paths
DEFINE_DIR_CONFIG(base_dir,         DEFAULT_BASE_DIR)
DEFINE_STR_CONFIG(config_file,      DEFAULT_CONFIG_FILE)
DEFINE_DIR_CONFIG(player_dir,       DEFAULT_PLAYER_DIR)
DEFINE_DIR_CONFIG(gods_dir,         DEFAULT_GODS_DIR)
DEFINE_DIR_CONFIG(area_dir,         DEFAULT_AREA_DIR)
DEFINE_FILE_CONFIG(area_list,       area_dir,   DEFAULT_AREA_LIST)
DEFINE_FILE_CONFIG(music_file,      area_dir,   DEFAULT_MUSIC_FILE)
DEFINE_LOG_CONFIG(bug_file,         area_dir,   DEFAULT_BUG_FILE)
DEFINE_LOG_CONFIG(typo_file,        area_dir,   DEFAULT_TYPO_FILE)
DEFINE_LOG_CONFIG(note_file,        area_dir,   DEFAULT_NOTE_FILE)
DEFINE_LOG_CONFIG(idea_file,        area_dir,   DEFAULT_IDEA_FILE)
DEFINE_LOG_CONFIG(penalty_file,     area_dir,   DEFAULT_PENALTY_FILE)
DEFINE_LOG_CONFIG(news_file,        area_dir,   DEFAULT_NEWS_FILE)
DEFINE_LOG_CONFIG(changes_file,     area_dir,   DEFAULT_CHANGES_FILE)
DEFINE_LOG_CONFIG(shutdown_file,    area_dir,   DEFAULT_SHUTDOWN_FILE)
DEFINE_FILE_CONFIG(ban_file,        area_dir,   DEFAULT_BAN_FILE)
DEFINE_DIR_CONFIG(data_dir,         DEFAULT_DATA_DIR)
DEFINE_DIR_CONFIG(progs_dir,        DEFAULT_PROGS_DIR)
DEFINE_FILE_CONFIG(socials_file,    data_dir,   DEFAULT_SOCIALS_FILE)
DEFINE_FILE_CONFIG(groups_file,     data_dir,   DEFAULT_GROUPS_FILE)
DEFINE_FILE_CONFIG(skills_file,     data_dir,   DEFAULT_SKILLS_FILE)
DEFINE_FILE_CONFIG(commands_file,   data_dir,   DEFAULT_COMMANDS_FILE)
DEFINE_FILE_CONFIG(races_file,      data_dir,   DEFAULT_RACES_FILE)
DEFINE_FILE_CONFIG(classes_file,    data_dir,   DEFAULT_CLASSES_FILE)
DEFINE_DIR_CONFIG(temp_dir,         DEFAULT_TEMP_DIR)
DEFINE_FILE_CONFIG(mem_dump_file,   temp_dir,   DEFAULT_MEM_DUMP_FILE)
DEFINE_FILE_CONFIG(mob_dump_file,   temp_dir,   DEFAULT_MOB_DUMP_FILE)
DEFINE_FILE_CONFIG(obj_dump_file,   temp_dir,   DEFAULT_OBJ_DUMP_FILE)

// Gameplay Configs
DEFINE_CONFIG(chargen_custom,       bool,       DEFAULT_CHARGEN_CUSTOM)
DEFINE_CONFIG(default_recall,       int,        DEFAULT_RECALL)
DEFINE_CONFIG(default_start_loc,    int,        DEFAULT_START_LOC)
DEFINE_CONFIG(start_loc_by_race,    bool,       false)
DEFINE_CONFIG(start_loc_by_class,   bool,       false)
DEFINE_CONFIG(train_anywhere,       bool,       false)
DEFINE_CONFIG(practice_anywhere,    bool,       false)

#pragma GCC diagnostic pop

typedef enum config_type_t {
    CFG_STR,
    CFG_DIR,
    CFG_INT,
    CFG_BOOL,
} ConfigType;

typedef struct config_entry_t {
    const char* field;
    const ConfigType type;
    const uintptr_t target;
} ConfigEntry;

typedef void CfgStrSet(const char*);
typedef void CfgIntSet(const int);
typedef void CfgBoolSet(const bool);

#ifdef U
#define OLD_U U
#endif
#define U(x)    (uintptr_t)(x)

const ConfigEntry config_entries[] = {
    { "mud_name",           CFG_STR,    U(cfg_set_mud_name)             },

    // Connection Info
    { "hostname",           CFG_STR,    U(cfg_set_hostname)             },
    { "telnet_enabled",     CFG_BOOL,   U(cfg_set_telnet_enabled)       },
    { "telnet_port",        CFG_INT,    U(cfg_set_telnet_port)          },
    { "tls_enabled",        CFG_BOOL,   U(cfg_set_tls_enabled)          },
    { "tls_port",           CFG_INT,    U(cfg_set_tls_port)             },
    { "keys_dir",           CFG_DIR,    U(cfg_set_keys_dir)             },
    { "cert_file",          CFG_STR,    U(cfg_set_cert_file)            },
    { "pkey_file",          CFG_STR,    U(cfg_set_pkey_file)            },
    { "debug_telopt",       CFG_BOOL,   U(cfg_set_debug_telopt)         },

    // GMCP
    { "gmcp_enabled",       CFG_BOOL,   U(cfg_set_gmcp_enabled)         },

    // MCCP
    { "mccp2_enabled",      CFG_BOOL,   U(cfg_set_mccp2_enabled)        },
    { "mccp3_enabled",      CFG_BOOL,   U(cfg_set_mccp3_enabled)        },

    // MSDP
    { "msdp_enabled",       CFG_BOOL,   U(cfg_set_msdp_enabled)         },

    // MSSP
    { "mssp_enabled",       CFG_BOOL,   U(cfg_set_mssp_enabled)         },
    { "codebase",           CFG_STR,    U(cfg_set_codebase)             },
    { "contact",            CFG_STR,    U(cfg_set_contact)              },
    { "created",            CFG_INT,    U(cfg_set_created)              },
    { "icon",               CFG_STR,    U(cfg_set_icon)                 },
    { "language",           CFG_STR,    U(cfg_set_language)             },
    { "location",           CFG_STR,    U(cfg_set_location)             },
    { "min_age",            CFG_INT,    U(cfg_set_min_age)              },
    { "website",            CFG_STR,    U(cfg_set_website)              },
    { "family",             CFG_STR,    U(cfg_set_family)               },
    { "genre",              CFG_STR,    U(cfg_set_genre)                },
    { "gameplay",           CFG_STR,    U(cfg_set_gameplay)             },
    { "status",             CFG_STR,    U(cfg_set_status)               },
    { "subgenre",           CFG_STR,    U(cfg_set_subgenre)             },

    // Filepaths
    { "area_dir",           CFG_DIR,    U(cfg_set_area_dir)             },
    { "area_list",          CFG_STR,    U(cfg_set_area_list)            },
    { "music_file",         CFG_STR,    U(cfg_set_music_file)           },
    { "bug_file",           CFG_STR,    U(cfg_set_bug_file)             },
    { "typo_file",          CFG_STR,    U(cfg_set_typo_file)            },
    { "note_file",          CFG_STR,    U(cfg_set_note_file)            },
    { "idea_file",          CFG_STR,    U(cfg_set_idea_file)            },
    { "penalty_file",       CFG_STR,    U(cfg_set_penalty_file)         },
    { "news_file",          CFG_STR,    U(cfg_set_news_file)            },
    { "changes_file",       CFG_STR,    U(cfg_set_changes_file)         },
    { "shutdown_file",      CFG_STR,    U(cfg_set_shutdown_file)        },
    { "ban_file",           CFG_STR,    U(cfg_set_ban_file)             },
    { "data_dir",           CFG_DIR,    U(cfg_set_data_dir)             },
    { "progs_dir",          CFG_DIR,    U(cfg_set_progs_dir)            },
    { "socials_file",       CFG_STR,    U(cfg_set_socials_file)         },
    { "groups_file",        CFG_STR,    U(cfg_set_groups_file)          },
    { "skills_file",        CFG_STR,    U(cfg_set_skills_file)          },
    { "commands_file",      CFG_STR,    U(cfg_set_commands_file)        },
    { "races_file",         CFG_STR,    U(cfg_set_races_file)           },
    { "classes_file",       CFG_STR,    U(cfg_set_classes_file)         },
    { "temp_dir",           CFG_DIR,    U(cfg_set_temp_dir)             },
    { "mem_dump_file",      CFG_STR,    U(cfg_set_mem_dump_file)        },
    { "mob_dump_file",      CFG_STR,    U(cfg_set_mob_dump_file)        },
    { "obj_dump_file",      CFG_STR,    U(cfg_set_obj_dump_file)        },
    { "player_dir",         CFG_DIR,    U(cfg_set_player_dir)           },
    { "gods_dir",           CFG_DIR,    U(cfg_set_gods_dir)             },

    // Gameplay
    { "chargen_custom",     CFG_BOOL,   U(cfg_set_chargen_custom)       },
    { "default_recall",     CFG_INT,    U(cfg_set_default_recall)       },
    { "default_start_loc",  CFG_INT,    U(cfg_set_default_start_loc)    },
    { "start_loc_by_race",  CFG_BOOL,   U(cfg_set_start_loc_by_race)    },
    { "start_loc_by_class", CFG_BOOL,   U(cfg_set_start_loc_by_class)   },
    { "train_anywhere",     CFG_BOOL,   U(cfg_set_train_anywhere)       },
    { "practice_anywhere",  CFG_BOOL,   U(cfg_set_practice_anywhere)    },
    { "",                   0,          0                               },
};

#undef U
#ifdef OLD_U
#define U OLD_U
#undef OLD_U
#endif

void parse_file(FILE* fp);

void load_config()
{
    FILE* cfg_fp = NULL;
    char cfg_path[BACKING_STR_SIZE] = { 0 };

    if (_base_dir[0]) {
        change_dir(_base_dir);
    }
    else if(_area_dir[0]) {
        change_dir(_area_dir);
        cfg_set_area_dir("area/");
    }

    if (!file_exists(cfg_get_config_file()) && !_config_file[0] && !_base_dir[0]) {
        // No config file or based dir was passed in. Maybe we're in the area
        // folder? Look in the parent.
        sprintf(cfg_path, "%s%s", "../", cfg_get_config_file());
        if (file_exists(cfg_path))
            change_dir("../");
        else {
            fprintf(stderr, "WARNING: No configuration file found; taking defaults.\n");
            return;
        }
    }

    cfg_fp = open_read_file(cfg_get_config_file());

    // No config file? Fine. Take the defaults (other than what was set on the 
    // command-line).
    if (!cfg_fp) {
        fprintf(stderr, "WARNING: No configuration file found; taking defaults.\n");
        return;
    }

    parse_file(cfg_fp);

    close_file(cfg_fp);
    cfg_fp = NULL;
}

char* read_file(FILE* f)
{
    char* buffer = NULL;

    if (!f || fseek(f, 0, SEEK_END)) {
        return NULL;
    }

    int32_t len = ftell(f);
    rewind(f);
    if (len < 0) {
        return NULL;
    }

    size_t ulen = (size_t)len;

    if ((buffer = malloc(ulen + 1)) == NULL) {
        free(buffer);
        return NULL;
    }

    size_t read;
    read = fread(buffer, 1, ulen, f);
    buffer[read] = '\0';

    // Windows hates us.
    if (read < ulen) {
        char* temp = strdup(buffer);
        free(buffer);
        buffer = temp;
    }
    else if (read > ulen) {
        free(buffer);
        return NULL;
    }

    return buffer;
}

struct parse_ctx {
    char* pos;
    bool error;
} parser = { NULL, false };

static inline char peek()
{
    return *parser.pos;
}

static inline char advance()
{
    return *parser.pos++;
}

static inline bool match(char c)
{
    if (*parser.pos == c) {
        ++parser.pos;
        return true;
    }

    return false;
}

static inline void skip_ws()
{
    while (match(' ') || match('\t') || match('\n') || match('\r'))
        ;
}

static char* parse_ident()
{
    char* start = parser.pos - 1;
    char c;

    while (ISALNUM(c = peek()) || c == '_')
        advance();

    size_t len = parser.pos - start;

    char* ident = (char*)malloc(sizeof(char) * (len + 1));
    if (!ident) {
        perror("parse_ident(): malloc() error while parsing config.\n");
        exit(1);
    }

    strncpy(ident, start, len);
    ident[len] = '\0';

    return ident;
}

static char* parse_val()
{
    char* start = parser.pos;
    char quote = 0;
    char c;
    size_t len = 0;
    bool escape = false;

    if ((c = peek()) == '\'' || c == '"') {
        advance();
        ++start;
        quote = c;
    }

    while ((c = advance()) != '\0') {
        if (quote) {
            if (c == '\\') {
                escape = true;
            }
            else if (escape) {
                escape = false;
            }
            else if (quote == c) {
                len = (parser.pos - 1) - start;
                advance();
                break;
            }
        }
        else if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            len = (parser.pos - 1) - start;
            break;
        }
    }

    char* value = (char*)malloc(sizeof(char) * (len + 1));
    if (!value) {
        perror("parse_value(): malloc() error while parsing config.\n");
        exit(1);
    }

    strncpy(value, start, len);
    value[len] = '\0';

    return value;
}

static void map_val(const ConfigEntry* entry, const char* val)
{
    switch (entry->type) {
    case CFG_DIR:
    case CFG_STR: {
        CfgStrSet* setter = (CfgStrSet*)entry->target;
        (*setter)(val);
        return;
    }
    case CFG_INT: {
        int int_val = atoi(val);
        CfgIntSet* setter = (CfgIntSet*)entry->target;
        (*setter)(int_val);
        return;
    }
    case CFG_BOOL: {
        bool b_val = false;
        if (!strcmp(val, "yes") || !strcasecmp(val, "true")
            || !strcasecmp(val, "enable") || !strcasecmp(val, "enabled"))
            b_val = true;
        else if (!strcasecmp(val, "no") || !strcasecmp(val, "false")
            || !strcasecmp(val, "disable") || !strcasecmp(val, "disabled"))
            b_val = false;
        CfgBoolSet* setter = (CfgBoolSet*)entry->target;
        (*setter)(b_val);
        return;
    }
    }
}

static void set_key_val(const char* key, const char* val)
{
    const ConfigEntry* entry = NULL;
    for (int i = 0; config_entries[i].field[0]; ++i) {
        entry = &config_entries[i];
        if (!strcasecmp(key, entry->field)) {
            map_val(entry, val);
            return;
        }
    }

    fprintf(stderr, "Unknown config key '%s'.\n", key);
}

void parse_file(FILE* fp)
{
    char* str = read_file(fp);

    parser.pos = str;

    skip_ws();

    char c;
    while ((c = advance()) != '\0') {
        if (c == '#') {
            // Scarf comments
            while ((c = advance()) != '\0' && c != '\n' && c != '\r')
                ;
        }
        else if (ISALPHA(c)) {
            char* key = parse_ident();
            skip_ws();
            if (!match('=')) {
                fprintf(stderr, "Expected '=' after '%s'\n", key);
                free(key);
                break;
            }
            skip_ws();
            char* val = parse_val();
            set_key_val(key, val);
            DEBUGF("PARSED: '%s' = '%s'\n", key, val);
            free(key);
            free(val);
        }

        skip_ws();

        if (c == '\0')
            break;
    }

    free(str);
}
