////////////////////////////////////////////////////////////////////////////////
// class.c
////////////////////////////////////////////////////////////////////////////////

#include "class.h"

#include "item.h"

#include "comm.h"
#include "db.h"
#include "tablesave.h"

Class* class_table = NULL;
int class_count = 0;

Class tmp_class;

const ArchetypeInfo arch_table[ARCH_COUNT] = {
    { "Arcane",     "Arc",      ARCH_ARCANE     },
    { "Divine",     "Div",      ARCH_DIVINE     },
    { "Rogue",      "Rog",      ARCH_ROGUE      },
    { "Martial",    "Mar",      ARCH_MARTIAL    },
};

#ifdef U
#define OLD_U U
#endif
#define U(x)    (uintptr_t)(x)

const SaveTableEntry class_save_table[] = {
    { "name",	    FIELD_STRING,		        U(&tmp_class.name),	        0,		        0           },
    { "wname",	    FIELD_STRING,		        U(&tmp_class.who_name),	    0,		        0           },
    { "basegrp",    FIELD_STRING,               U(&tmp_class.base_group),   0,              0           },
    { "dfltgrp",    FIELD_STRING,               U(&tmp_class.default_group),0,              0           },
    { "weap",       FIELD_VNUM,                 U(&tmp_class.weapon),       0,              0           },
    { "guild",      FIELD_VNUM_ARRAY,           U(&tmp_class.guild),        U(MAX_GUILD),   0           },
    { "pstat",      FIELD_INT16,                U(&tmp_class.prime_stat),   0,              0           },
    { "arch",       FIELD_INT16,                U(&tmp_class.arch),         0,              0           },
    { "scap",       FIELD_INT16,                U(&tmp_class.skill_cap),    0,              0           },
    { "t0_0",       FIELD_INT16,                U(&tmp_class.thac0_00),     0,              0           },
    { "t0_32",      FIELD_INT16,                U(&tmp_class.thac0_32),     0,              0           },
    { "hpmin",      FIELD_INT16,                U(&tmp_class.hp_min),       0,              0           },
    { "hpmax",      FIELD_INT16,                U(&tmp_class.hp_max),       0,              0           },
    { "fmana",      FIELD_BOOL,                 U(&tmp_class.fMana),        0,              0           },
    { NULL,		    0,				            0,			                0,		        0           }
};

void load_class_table()
{
    FILE* fp;
    char* word;
    int i;

    char classes_file[256];
    sprintf(classes_file, "%s%s", area_dir, CLASS_FILE);
    fp = fopen(classes_file, "r");

    if (!fp) {
        perror("load_class_table");
        return;
    }

    int maxclass = fread_number(fp);

    size_t new_size = sizeof(Class) * ((size_t)maxclass + 1);
    flog("Creating class of length %d, size %zu", maxclass + 1, new_size);

    if ((class_table = calloc(sizeof(Class), (size_t)maxclass + 1)) == NULL) {
        perror("load_class_table(): Could not allocate class_table!");
        exit(-1);
    }

    i = 0;

    while (true) {
        word = fread_word(fp);

        if (str_cmp(word, "#CLASS")) {
            bugf("load_class_table : word %s", word);
            fclose(fp);
            return;
        }

        load_struct(fp, U(&tmp_class), class_save_table, U(&class_table[i++]));

        if (i == maxclass) {
            flog("Class table loaded.");
            fclose(fp);
            class_count = maxclass;
            class_table[i].name = NULL;
            return;
        }
    }
}

void save_class_table()
{
    FILE* fp;
    const Class* temp;
    int cnt = 0;

    char tempclasses_file[256];
    sprintf(tempclasses_file, "%s%s", area_dir, DATA_DIR "tempclasses");
    fp = fopen(tempclasses_file, "w");
    if (!fp) {
        bugf("save_classes : Can't open %s", tempclasses_file);
        return;
    }

    for (temp = class_table; !IS_NULLSTR(temp->name); temp++)
        cnt++;

    fprintf(fp, "%d\n\n", cnt);

    for (temp = class_table, cnt = 0; !IS_NULLSTR(temp->name); temp++) {
        fprintf(fp, "#CLASS\n");
        save_struct(fp, U(&tmp_class), class_save_table, U(temp));
        fprintf(fp, "#END\n\n");
    }

    fclose(fp);

    char classes_file[256];
    sprintf(classes_file, "%s%s", area_dir, CLASS_FILE);

#ifdef _MSC_VER
    if (!MoveFileExA(tempclasses_file, classes_file, MOVEFILE_REPLACE_EXISTING)) {
        bugf("save_class : Could not rename %s to %s!", tempclasses_file, classes_file);
        perror(classes_file);
    }
#else
    if (rename(tempclasses_file, classes_file) != 0) {
        bugf("save_class : Could not rename %s to %s!", tempclasses_file, classes_file);
        perror(classes_file);
    }
#endif
}

#undef U
#ifdef OLD_U
#define U OLD_U
#undef OLD_U
#endif

char* const title_table[ARCH_COUNT][MAX_LEVEL + 1][2] = {
    {
        {"Man", "Woman"},
        {"Apprentice of Magic", "Apprentice of Magic"},
        {"Spell Student", "Spell Student"},
        {"Scholar of Magic", "Scholar of Magic"},
        {"Delver in Spells", "Delveress in Spells"},
        {"Medium of Magic", "Medium of Magic"},
        {"Scribe of Magic", "Scribess of Magic"},
        {"Seer", "Seeress"},
        {"Sage", "Sage"},
        {"Illusionist", "Illusionist"},
        {"Abjurer", "Abjuress"},
        {"Invoker", "Invoker"},
        {"Enchanter", "Enchantress"},
        {"Conjurer", "Conjuress"},
        {"Magician", "Witch"},
        {"Creator", "Creator"},
        {"Savant", "Savant"},
        {"Magus", "Craftess"},
        {"Wizard", "Wizard"},
        {"Warlock", "War Witch"},
        {"Sorcerer", "Sorceress"},
        {"Elder Sorcerer", "Elder Sorceress"},
        {"Grand Sorcerer", "Grand Sorceress"},
        {"Great Sorcerer", "Great Sorceress"},
        {"Golem Maker", "Golem Maker"},
        {"Greater Golem Maker", "Greater Golem Maker"},
        {"Maker of Stones", "Maker of Stones"},
        {"Maker of Potions", "Maker of Potions"},
        {"Maker of Scrolls", "Maker of Scrolls"},
        {"Maker of Wands", "Maker of Wands"},
        {"Maker of Staves", "Maker of Staves"},
        {"Demon Summoner", "Demon Summoner"},
        {"Greater Demon Summoner", "Greater Demon Summoner"},
        {"Dragon Charmer", "Dragon Charmer"},
        {"Greater Dragon Charmer", "Greater Dragon Charmer"},
        {"Master of all Magic", "Master of all Magic"},
        {"Master Mage", "Master Mage"},
        {"Master Mage", "Master Mage"},
        {"Master Mage", "Master Mage"},
        {"Master Mage", "Master Mage"},
        {"Master Mage", "Master Mage"},
        {"Master Mage", "Master Mage"},
        {"Master Mage", "Master Mage"},
        {"Master Mage", "Master Mage"},
        {"Master Mage", "Master Mage"},
        {"Master Mage", "Master Mage"},
        {"Master Mage", "Master Mage"},
        {"Master Mage", "Master Mage"},
        {"Master Mage", "Master Mage"},
        {"Master Mage", "Master Mage"},
        {"Master Mage", "Master Mage"},
        {"Mage Hero", "Mage Heroine"},
        {"Avatar of Magic", "Avatar of Magic"},
        {"Angel of Magic", "Angel of Magic"},
        {"Demigod of Magic", "Demigoddess of Magic"},
        {"Immortal of Magic", "Immortal of Magic"},
        {"God of Magic", "Goddess of Magic"},
        {"Deity of Magic", "Deity of Magic"},
        {"Supremity of Magic", "Supremity of Magic"},
        {"Creator", "Creator"},
        {"Implementor", "Implementress"}
    },
    {
        {"Man", "Woman"},
        {"Believer", "Believer"},
        {"Attendant", "Attendant"},
        {"Acolyte", "Acolyte"},
        {"Novice", "Novice"},
        {"Missionary", "Missionary"},
        {"Adept", "Adept"},
        {"Deacon", "Deaconess"},
        {"Vicar", "Vicaress"},
        {"Priest", "Priestess"},
        {"Minister", "Lady Minister"},
        {"Canon", "Canon"},
        {"Levite", "Levitess"},
        {"Curate", "Curess"},
        {"Monk", "Nun"},
        {"Healer", "Healess"},
        {"Chaplain", "Chaplain"},
        {"Expositor", "Expositress"},
        {"Bishop", "Bishop"},
        {"Arch Bishop", "Arch Lady of the Church"},
        {"Patriarch", "Matriarch"},
        {"Elder Patriarch", "Elder Matriarch"},
        {"Grand Patriarch", "Grand Matriarch"},
        {"Great Patriarch", "Great Matriarch"},
        {"Demon Killer", "Demon Killer"},
        {"Greater Demon Killer", "Greater Demon Killer"},
        {"Cardinal of the Sea", "Cardinal of the Sea"},
        {"Cardinal of the Earth", "Cardinal of the Earth"},
        {"Cardinal of the Air", "Cardinal of the Air"},
        {"Cardinal of the Ether", "Cardinal of the Ether"},
        {"Cardinal of the Heavens", "Cardinal of the Heavens"},
        {"Avatar of an Immortal", "Avatar of an Immortal"},
        {"Avatar of a Deity", "Avatar of a Deity"},
        {"Avatar of a Supremity", "Avatar of a Supremity"},
        {"Avatar of an Implementor", "Avatar of an Implementor"},
        {"Master of all Divinity", "Mistress of all Divinity"},
        {"Master Cleric", "Master Cleric"},
        {"Master Cleric", "Master Cleric"},
        {"Master Cleric", "Master Cleric"},
        {"Master Cleric", "Master Cleric"},
        {"Master Cleric", "Master Cleric"},
        {"Master Cleric", "Master Cleric"},
        {"Master Cleric", "Master Cleric"},
        {"Master Cleric", "Master Cleric"},
        {"Master Cleric", "Master Cleric"},
        {"Master Cleric", "Master Cleric"},
        {"Master Cleric", "Master Cleric"},
        {"Master Cleric", "Master Cleric"},
        {"Master Cleric", "Master Cleric"},
        {"Master Cleric", "Master Cleric"},
        {"Master Cleric", "Master Cleric"},
        {"Holy Hero", "Holy Heroine"},
        {"Holy Avatar", "Holy Avatar"},
        {"Angel", "Angel"},
        {"Demigod",  "Demigoddess"},
        {"Immortal", "Immortal"},
        {"God", "Goddess"},
        {"Deity", "Deity"},
        {"Supreme Master", "Supreme Mistress"},
        {"Creator", "Creator"},
        {"Implementor", "Implementress"}
    },
    {
        {"Man", "Woman"},
        {"Pilferer", "Pilferess"},
        {"Footpad", "Footpad"},
        {"Filcher", "Filcheress"},
        {"Pick-Pocket", "Pick-Pocket"},
        {"Sneak", "Sneak"},
        {"Pincher", "Pincheress"},
        {"Cut-Purse", "Cut-Purse"},
        {"Snatcher", "Snatcheress"},
        {"Sharper", "Sharpress"},
        {"Rogue", "Rogue"},
        {"Robber", "Robber"},
        {"Magsman", "Magswoman"},
        {"Highwayman", "Highwaywoman"},
        {"Burglar", "Burglaress"},
        {"Thief", "Thief"},
        {"Knifer", "Knifer"},
        {"Quick-Blade", "Quick-Blade"},
        {"Killer", "Murderess"},
        {"Brigand", "Brigand"},
        {"Cut-Throat", "Cut-Throat"},
        {"Spy", "Spy"},
        {"Grand Spy", "Grand Spy"},
        {"Master Spy", "Master Spy"},
        {"Assassin", "Assassin"},
        {"Greater Assassin", "Greater Assassin"},
        {"Master of Vision", "Mistress of Vision"},
        {"Master of Hearing", "Mistress of Hearing"},
        {"Master of Smell", "Mistress of Smell"},
        {"Master of Taste", "Mistress of Taste"},
        {"Master of Touch", "Mistress of Touch"},
        {"Crime Lord", "Crime Mistress"},
        {"Infamous Crime Lord", "Infamous Crime Mistress"},
        {"Greater Crime Lord", "Greater Crime Mistress"},
        {"Master Crime Lord", "Master Crime Mistress"},
        {"Godfather", "Godmother"},
        {"Master Thief", "Master Thief"},
        {"Master Thief", "Master Thief"},
        {"Master Thief", "Master Thief"},
        {"Master Thief", "Master Thief"},
        {"Master Thief", "Master Thief"},
        {"Master Thief", "Master Thief"},
        {"Master Thief", "Master Thief"},
        {"Master Thief", "Master Thief"},
        {"Master Thief", "Master Thief"},
        {"Master Thief", "Master Thief"},
        {"Master Thief", "Master Thief"},
        {"Master Thief", "Master Thief"},
        {"Master Thief", "Master Thief"},
        {"Master Thief", "Master Thief"},
        {"Master Thief", "Master Thief"},
        {"Assassin Hero", "Assassin Heroine"},
        {"Avatar of Death", "Avatar of Death"},
        {"Angel of Death", "Angel of Death"},
        {"Demigod of Assassins", "Demigoddess of Assassins"},
        {"Immortal Assasin", "Immortal Assassin"},
        {"God of Assassins", "God of Assassins"},
        {"Deity of Assassins", "Deity of Assassins"},
        {"Supreme Master", "Supreme Mistress"},
        {"Creator", "Creator"},
        {"Implementor", "Implementress"}
    },
    {
        {"Man", "Woman"},
        {"Swordpupil", "Swordpupil"},
        {"Recruit", "Recruit"},
        {"Sentry", "Sentress"},
        {"Fighter", "Fighter"},
        {"Soldier", "Soldier"},
        {"Warrior", "Warrior"},
        {"Veteran", "Veteran"},
        {"Swordsman", "Swordswoman"},
        {"Fencer", "Fenceress"},
        {"Combatant", "Combatess"},
        {"Hero", "Heroine"},
        {"Myrmidon", "Myrmidon"},
        {"Swashbuckler", "Swashbuckleress"},
        {"Mercenary", "Mercenaress"},
        {"Swordmaster", "Swordmistress"},
        {"Lieutenant", "Lieutenant"},
        {"Champion", "Lady Champion"},
        {"Dragoon", "Lady Dragoon"},
        {"Cavalier", "Lady Cavalier"},
        {"Knight", "Lady Knight"},
        {"Grand Knight", "Grand Knight"},
        {"Master Knight", "Master Knight"},
        {"Paladin", "Paladin"},
        {"Grand Paladin", "Grand Paladin"},
        {"Demon Slayer", "Demon Slayer"},
        {"Greater Demon Slayer", "Greater Demon Slayer"},
        {"Dragon Slayer", "Dragon Slayer"},
        {"Greater Dragon Slayer", "Greater Dragon Slayer"},
        {"Underlord", "Underlord"},
        {"Overlord", "Overlord"},
        {"Baron of Thunder", "Baroness of Thunder"},
        {"Baron of Storms", "Baroness of Storms"},
        {"Baron of Tornadoes", "Baroness of Tornadoes"},
        {"Baron of Hurricanes", "Baroness of Hurricanes"},
        {"Baron of Meteors", "Baroness of Meteors"},
        {"Master Warrior", "Master Warrior"},
        {"Master Warrior", "Master Warrior"},
        {"Master Warrior", "Master Warrior"},
        {"Master Warrior", "Master Warrior"},
        {"Master Warrior", "Master Warrior"},
        {"Master Warrior", "Master Warrior"},
        {"Master Warrior", "Master Warrior"},
        {"Master Warrior", "Master Warrior"},
        {"Master Warrior", "Master Warrior"},
        {"Master Warrior", "Master Warrior"},
        {"Master Warrior", "Master Warrior"},
        {"Master Warrior", "Master Warrior"},
        {"Master Warrior", "Master Warrior"},
        {"Master Warrior", "Master Warrior"},
        {"Master Warrior", "Master Warrior"},
        {"Knight Hero", "Knight Heroine"},
        {"Avatar of War", "Avatar of War"},
        {"Angel of War", "Angel of War"},
        {"Demigod of War", "Demigoddess of War"},
        {"Immortal Warlord", "Immortal Warlord"},
        {"God of War", "God of War"},
        {"Deity of War", "Deity of War"},
        {"Supreme Master of War", "Supreme Mistress of War"},
        {"Creator", "Creator"},
        {"Implementor", "Implementress"}
    }
};
