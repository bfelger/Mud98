////////////////////////////////////////////////////////////////////////////////
// data/spell.c
////////////////////////////////////////////////////////////////////////////////

#include "merc.h"

#include "spell.h"

#include <db.h>
#include <spell_list.h>

#if defined(SPELL)
#undef SPELL
#endif
#define SPELL(spell)    { #spell,  spell },

const Spell spell_table[] = {
// I hate everything about this.
#include "spell_list.h"
    { NULL, NULL }
};

SpellFunc* spell_function(char* argument)
{
    int i;
    char buf[MSL];

    if (IS_NULLSTR(argument))
        return NULL;

    for (i = 0; spell_table[i].name; ++i)
        if (!str_cmp(spell_table[i].name, argument))
            return spell_table[i].spell;

    if (fBootDb) {
        sprintf(buf, "spell_function : spell %s does not exist", argument);
        bug(buf, 0);
    }

    return spell_null;
}

void spell_list(Buffer* pBuf)
{
    char buf[MSL];
    int i;

    sprintf(buf, "Num %-35.35s Num %-35.35s\n\r",
        "Name", "Name");
    add_buf(pBuf, buf);

    for (i = 0; spell_table[i].name; ++i) {
        sprintf(buf, COLOR_ALT_TEXT_1 "%3d" COLOR_CLEAR " %-35.35s", i,
            spell_table[i].name);
        if (i % 2 == 1)
            strcat(buf, "\n\r");
        else
            strcat(buf, " ");
        add_buf(pBuf, buf);
    }

    if (i % 2 != 0)
        add_buf(pBuf, "\n\r");
}


bool spell_fun_read(void* temp, char* arg)
{
    SpellFunc** spfun = (SpellFunc**)temp;
    SpellFunc* blah = spell_function(arg);

    *spfun = blah;

    return !str_cmp(arg, "") || blah != NULL;
}

char* spell_fun_str(void* temp)
{
    SpellFunc** spfun = (SpellFunc**)temp;
    return spell_name(*spfun);
}

char* spell_name(SpellFunc* spell)
{
    int i = 0;

    if (spell == NULL)
        return "";

    while (spell_table[i].name)
        if (spell_table[i].spell == spell)
            return spell_table[i].name;
        else
            i++;

    if (fBootDb)
        bug("spell_name : spell_fun does not exist", 0);

    return "";
}
