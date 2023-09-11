/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

/***************************************************************************
 *  ROM 2.4 is copyright 1993-1998 Russ Taylor                             *
 *  ROM has been brought to you by the ROM consortium                      *
 *      Russ Taylor (rtaylor@hypercube.org)                                *
 *      Gabrielle Taylor (gtaylor@hypercube.org)                           *
 *      Brian Moore (zump@rom.org)                                         *
 *  By using this code, you have agreed to follow the terms of the         *
 *  ROM license, in the file Rom24/doc/rom.license                         *
 ***************************************************************************/

#include "merc.h"

#include "comm.h"
#include "db.h"
#include "handler.h"
#include "note.h"
#include "recycle.h"
#include "strings.h"
#include "tables.h"

#include "entities/player_data.h"

#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#ifndef _MSC_VER 
#include <sys/time.h>
#endif

/* local procedures */
void load_thread(char* name, NoteData** list, int16_t type, time_t free_time);
void parse_note(CharData* ch, char* argument, int16_t type);
bool hide_note(CharData* ch, NoteData* pnote);

NoteData* note_list;
NoteData* idea_list;
NoteData* penalty_list;
NoteData* news_list;
NoteData* changes_list;
NoteData* note_free;

int count_spool(CharData* ch, NoteData* spool)
{
    int count = 0;
    NoteData* pnote;

    for (pnote = spool; pnote != NULL; pnote = pnote->next)
        if (!hide_note(ch, pnote)) count++;

    return count;
}

void do_unread(CharData* ch, char* argument)
{
    char buf[MAX_STRING_LENGTH];
    int count;
    bool found = false;

    if (IS_NPC(ch)) return;

    if ((count = count_spool(ch, news_list)) > 0) {
        found = true;
        sprintf(buf, "There %s %d new news article%s waiting.\n\r",
                count > 1 ? "are" : "is", count, count > 1 ? "s" : "");
        send_to_char(buf, ch);
    }
    if ((count = count_spool(ch, changes_list)) > 0) {
        found = true;
        sprintf(buf, "There %s %d change%s waiting to be read.\n\r",
                count > 1 ? "are" : "is", count, count > 1 ? "s" : "");
        send_to_char(buf, ch);
    }
    if ((count = count_spool(ch, note_list)) > 0) {
        found = true;
        sprintf(buf, "You have %d new note%s waiting.\n\r", count,
                count > 1 ? "s" : "");
        send_to_char(buf, ch);
    }
    if ((count = count_spool(ch, idea_list)) > 0) {
        found = true;
        sprintf(buf, "You have %d unread idea%s to peruse.\n\r", count,
                count > 1 ? "s" : "");
        send_to_char(buf, ch);
    }
    if (IS_TRUSTED(ch, ANGEL) && (count = count_spool(ch, penalty_list)) > 0) {
        found = true;
        sprintf(buf, "%d %s been added.\n\r", count,
                count > 1 ? "penalties have" : "penalty has");
        send_to_char(buf, ch);
    }

    if (!found) send_to_char("You have no unread notes.\n\r", ch);
}

void do_note(CharData* ch, char* argument)
{
    parse_note(ch, argument, NOTE_NOTE);
}

void do_idea(CharData* ch, char* argument)
{
    parse_note(ch, argument, NOTE_IDEA);
}

void do_penalty(CharData* ch, char* argument)
{
    parse_note(ch, argument, NOTE_PENALTY);
}

void do_news(CharData* ch, char* argument)
{
    parse_note(ch, argument, NOTE_NEWS);
}

void do_changes(CharData* ch, char* argument)
{
    parse_note(ch, argument, NOTE_CHANGES);
}

void save_notes(int type)
{
    FILE* fp;
    char* name;
    NoteData* pnote;

    switch (type) {
    default:
        return;
    case NOTE_NOTE:
        name = NOTE_FILE;
        pnote = note_list;
        break;
    case NOTE_IDEA:
        name = IDEA_FILE;
        pnote = idea_list;
        break;
    case NOTE_PENALTY:
        name = PENALTY_FILE;
        pnote = penalty_list;
        break;
    case NOTE_NEWS:
        name = NEWS_FILE;
        pnote = news_list;
        break;
    case NOTE_CHANGES:
        name = CHANGES_FILE;
        pnote = changes_list;
        break;
    }

    fclose(fpReserve);
    char filename[256];
    sprintf(filename, "%s%s", area_dir, name);
    if ((fp = fopen(filename, "w")) == NULL) { perror(name); }
    else {
        for (; pnote != NULL; pnote = pnote->next) {
            fprintf(fp, "Sender  %s~\n", pnote->sender);
            fprintf(fp, "Date    %s~\n", pnote->date);
            fprintf(fp, "Stamp   "TIME_FMT"\n", pnote->date_stamp);
            fprintf(fp, "To      %s~\n", pnote->to_list);
            fprintf(fp, "Subject %s~\n", pnote->subject);
            fprintf(fp, "Text\n%s~\n", pnote->text);
        }
        fclose(fp);
        fpReserve = fopen(NULL_FILE, "r");
        return;
    }
}
void load_notes(void)
{
    load_thread(NOTE_FILE, &note_list, NOTE_NOTE, (time_t)14 * 24 * 60 * 60);
    load_thread(IDEA_FILE, &idea_list, NOTE_IDEA, (time_t)28 * 24 * 60 * 60);
    load_thread(PENALTY_FILE, &penalty_list, NOTE_PENALTY, 0);
    load_thread(NEWS_FILE, &news_list, NOTE_NEWS, 0);
    load_thread(CHANGES_FILE, &changes_list, NOTE_CHANGES, 0);
}

void load_thread(char* name, NoteData** list, int16_t type, time_t free_time)
{
    FILE* fp;
    NoteData* pnotelast;

    char filename[256];
    sprintf(filename, "%s%s", area_dir, name);
    if ((fp = fopen(filename, "r")) == NULL) return;

    pnotelast = NULL;
    for (;;) {
        NoteData* pnote;
        char letter;

        do {
            letter = (char)getc(fp);
            if (feof(fp)) {
                fclose(fp);
                return;
            }
        }
        while (ISSPACE(letter));
        ungetc(letter, fp);

        pnote = alloc_perm(sizeof(*pnote));

        if (str_cmp(fread_word(fp), "sender")) break;
        pnote->sender = fread_string(fp);

        if (str_cmp(fread_word(fp), "date")) break;
        pnote->date = fread_string(fp);

        if (str_cmp(fread_word(fp), "stamp")) break;
        pnote->date_stamp = fread_number(fp);

        if (str_cmp(fread_word(fp), "to")) break;
        pnote->to_list = fread_string(fp);

        if (str_cmp(fread_word(fp), "subject")) break;
        pnote->subject = fread_string(fp);

        if (str_cmp(fread_word(fp), "text")) break;
        pnote->text = fread_string(fp);

        if (free_time && pnote->date_stamp < current_time - free_time) {
            free_note(pnote);
            continue;
        }

        pnote->type = (int16_t)type;

        if (*list == NULL)
            *list = pnote;
        else if (pnotelast)
            pnotelast->next = pnote;

        pnotelast = pnote;
    }

    strcpy(strArea, NOTE_FILE);
    fpArea = fp;
    bug("Load_notes: bad key word.", 0);
    exit(1);
    return;
}

void append_note(NoteData* pnote)
{
    FILE* fp;
    char* name;
    NoteData** list;
    NoteData* last;

    switch (pnote->type) {
    default:
        return;
    case NOTE_NOTE:
        name = NOTE_FILE;
        list = &note_list;
        break;
    case NOTE_IDEA:
        name = IDEA_FILE;
        list = &idea_list;
        break;
    case NOTE_PENALTY:
        name = PENALTY_FILE;
        list = &penalty_list;
        break;
    case NOTE_NEWS:
        name = NEWS_FILE;
        list = &news_list;
        break;
    case NOTE_CHANGES:
        name = CHANGES_FILE;
        list = &changes_list;
        break;
    }

    if (*list == NULL)
        *list = pnote;
    else {
        for (last = *list; last->next != NULL; last = last->next)
            ;
        last->next = pnote;
    }

    fclose(fpReserve);
    char filename[256];
    sprintf(filename, "%s%s", area_dir, name);
    if ((fp = fopen(filename, "a")) == NULL) { perror(name); }
    else {
        fprintf(fp, "Sender  %s~\n", pnote->sender);
        fprintf(fp, "Date    %s~\n", pnote->date);
        fprintf(fp, "Stamp   "TIME_FMT"\n", pnote->date_stamp);
        fprintf(fp, "To      %s~\n", pnote->to_list);
        fprintf(fp, "Subject %s~\n", pnote->subject);
        fprintf(fp, "Text\n%s~\n", pnote->text);
        fclose(fp);
    }
    fpReserve = fopen(NULL_FILE, "r");
}

bool is_note_to(CharData* ch, NoteData* pnote)
{
    if (!str_cmp(ch->name, pnote->sender)) return true;

    if (is_exact_name("all", pnote->to_list)) return true;

    if (IS_IMMORTAL(ch) && is_exact_name("immortal", pnote->to_list))
        return true;

    if (ch->clan && is_exact_name(clan_table[ch->clan].name, pnote->to_list))
        return true;

    if (is_exact_name(ch->name, pnote->to_list)) return true;

    return false;
}

void note_attach(CharData* ch, int16_t type)
{
    NoteData* pnote;

    if (ch->pnote != NULL) return;

    pnote = new_note();

    pnote->next = NULL;
    pnote->sender = str_dup(ch->name);
    pnote->date = str_dup("");
    pnote->to_list = str_dup("");
    pnote->subject = str_dup("");
    pnote->text = str_dup("");
    pnote->type = type;
    ch->pnote = pnote;
    return;
}

void note_remove(CharData* ch, NoteData* pnote, bool delete)
{
    char to_new[MAX_INPUT_LENGTH] = "";
    char to_one[MAX_INPUT_LENGTH] = "";
    NoteData* prev;
    NoteData** list;
    char* to_list;

    if (!delete) {
        /* make a new list */
        to_new[0] = '\0';
        to_list = pnote->to_list;
        while (*to_list != '\0') {
            to_list = one_argument(to_list, to_one);
            if (to_one[0] != '\0' && str_cmp(ch->name, to_one)) {
                strcat(to_new, " ");
                strcat(to_new, to_one);
            }
        }
        /* Just a simple recipient removal? */
        if (str_cmp(ch->name, pnote->sender) && to_new[0] != '\0') {
            free_string(pnote->to_list);
            pnote->to_list = str_dup(to_new + 1);
            return;
        }
    }
    /* nuke the whole note */

    switch (pnote->type) {
    default:
        return;
    case NOTE_NOTE:
        list = &note_list;
        break;
    case NOTE_IDEA:
        list = &idea_list;
        break;
    case NOTE_PENALTY:
        list = &penalty_list;
        break;
    case NOTE_NEWS:
        list = &news_list;
        break;
    case NOTE_CHANGES:
        list = &changes_list;
        break;
    }

    /*
     * Remove note from linked list.
     */
    if (pnote == *list) { *list = pnote->next; }
    else {
        for (prev = *list; prev != NULL; prev = prev->next) {
            if (prev->next == pnote) break;
        }

        if (prev == NULL) {
            bug("Note_remove: pnote not found.", 0);
            return;
        }

        prev->next = pnote->next;
    }

    save_notes(pnote->type);
    free_note(pnote);
    return;
}

bool hide_note(CharData* ch, NoteData* pnote)
{
    time_t last_read;

    if (IS_NPC(ch)) return true;

    switch (pnote->type) {
    default:
        return true;
    case NOTE_NOTE:
        last_read = ch->pcdata->last_note;
        break;
    case NOTE_IDEA:
        last_read = ch->pcdata->last_idea;
        break;
    case NOTE_PENALTY:
        last_read = ch->pcdata->last_penalty;
        break;
    case NOTE_NEWS:
        last_read = ch->pcdata->last_news;
        break;
    case NOTE_CHANGES:
        last_read = ch->pcdata->last_changes;
        break;
    }

    if (pnote->date_stamp <= last_read) return true;

    if (!str_cmp(ch->name, pnote->sender)) return true;

    if (!is_note_to(ch, pnote)) return true;

    return false;
}

void update_read(CharData* ch, NoteData* pnote)
{
    time_t stamp;

    if (IS_NPC(ch)) return;

    stamp = pnote->date_stamp;

    switch (pnote->type) {
    default:
        return;
    case NOTE_NOTE:
        ch->pcdata->last_note = UMAX(ch->pcdata->last_note, stamp);
        break;
    case NOTE_IDEA:
        ch->pcdata->last_idea = UMAX(ch->pcdata->last_idea, stamp);
        break;
    case NOTE_PENALTY:
        ch->pcdata->last_penalty = UMAX(ch->pcdata->last_penalty, stamp);
        break;
    case NOTE_NEWS:
        ch->pcdata->last_news = UMAX(ch->pcdata->last_news, stamp);
        break;
    case NOTE_CHANGES:
        ch->pcdata->last_changes = UMAX(ch->pcdata->last_changes, stamp);
        break;
    }
}

void parse_note(CharData* ch, char* argument, int16_t type)
{
    BUFFER* buffer;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    NoteData* pnote;
    NoteData** list;
    char* list_name;
    VNUM vnum;
    int anum;

    if (IS_NPC(ch)) return;

    switch (type) {
    default:
        return;
    case NOTE_NOTE:
        list = &note_list;
        list_name = "notes";
        break;
    case NOTE_IDEA:
        list = &idea_list;
        list_name = "ideas";
        break;
    case NOTE_PENALTY:
        list = &penalty_list;
        list_name = "penalties";
        break;
    case NOTE_NEWS:
        list = &news_list;
        list_name = "news";
        break;
    case NOTE_CHANGES:
        list = &changes_list;
        list_name = "changes";
        break;
    }

    argument = one_argument(argument, arg);
    smash_tilde(argument);

    if (arg[0] == '\0' || !str_prefix(arg, "read")) {
        bool fAll;

        if (!str_cmp(argument, "all")) {
            fAll = true;
            anum = 0;
        }

        else if (argument[0] == '\0' || !str_prefix(argument, "next"))
        /* read next unread note */
        {
            vnum = 0;
            for (pnote = *list; pnote != NULL; pnote = pnote->next) {
                if (!hide_note(ch, pnote)) {
                    sprintf(buf, "[%3d] %s: %s\n\r%s\n\rTo: %s\n\r", vnum,
                            pnote->sender, pnote->subject, pnote->date,
                            pnote->to_list);
                    send_to_char(buf, ch);
                    page_to_char(pnote->text, ch);
                    update_read(ch, pnote);
                    return;
                }
                else if (is_note_to(ch, pnote))
                    vnum++;
            }
            sprintf(buf, "You have no unread %s.\n\r", list_name);
            send_to_char(buf, ch);
            return;
        }

        else if (is_number(argument)) {
            fAll = false;
            anum = atoi(argument);
        }
        else {
            send_to_char("Read which number?\n\r", ch);
            return;
        }

        vnum = 0;
        for (pnote = *list; pnote != NULL; pnote = pnote->next) {
            if (is_note_to(ch, pnote) && (vnum++ == anum || fAll)) {
                sprintf(buf, "[%3d] %s: %s\n\r%s\n\rTo: %s\n\r", vnum - 1,
                        pnote->sender, pnote->subject, pnote->date,
                        pnote->to_list);
                send_to_char(buf, ch);
                page_to_char(pnote->text, ch);
                update_read(ch, pnote);
                return;
            }
        }

        sprintf(buf, "There aren't that many %s.\n\r", list_name);
        send_to_char(buf, ch);
        return;
    }

    if (!str_prefix(arg, "list")) {
        vnum = 0;
        for (pnote = *list; pnote != NULL; pnote = pnote->next) {
            if (is_note_to(ch, pnote)) {
                sprintf(buf, "[%3d%s] %s: %s\n\r", vnum,
                        hide_note(ch, pnote) ? " " : "N", pnote->sender,
                        pnote->subject);
                send_to_char(buf, ch);
                vnum++;
            }
        }
        if (!vnum) {
            switch (type) {
            case NOTE_NOTE:
                send_to_char("There are no notes for you.\n\r", ch);
                break;
            case NOTE_IDEA:
                send_to_char("There are no ideas for you.\n\r", ch);
                break;
            case NOTE_PENALTY:
                send_to_char("There are no penalties for you.\n\r", ch);
                break;
            case NOTE_NEWS:
                send_to_char("There is no news for you.\n\r", ch);
                break;
            case NOTE_CHANGES:
                send_to_char("There are no changes for you.\n\r", ch);
                break;
            }
        }
        return;
    }

    if (!str_prefix(arg, "remove")) {
        if (!is_number(argument)) {
            send_to_char("Note remove which number?\n\r", ch);
            return;
        }

        anum = atoi(argument);
        vnum = 0;
        for (pnote = *list; pnote != NULL; pnote = pnote->next) {
            if (is_note_to(ch, pnote) && vnum++ == anum) {
                note_remove(ch, pnote, false);
                send_to_char("Ok.\n\r", ch);
                return;
            }
        }

        sprintf(buf, "There aren't that many %s.", list_name);
        send_to_char(buf, ch);
        return;
    }

    if (!str_prefix(arg, "delete") && get_trust(ch) >= MAX_LEVEL - 1) {
        if (!is_number(argument)) {
            send_to_char("Note delete which number?\n\r", ch);
            return;
        }

        anum = atoi(argument);
        vnum = 0;
        for (pnote = *list; pnote != NULL; pnote = pnote->next) {
            if (is_note_to(ch, pnote) && vnum++ == anum) {
                note_remove(ch, pnote, true);
                send_to_char("Ok.\n\r", ch);
                return;
            }
        }

        sprintf(buf, "There aren't that many %s.", list_name);
        send_to_char(buf, ch);
        return;
    }

    if (!str_prefix(arg, "catchup")) {
        switch (type) {
        case NOTE_NOTE:
            ch->pcdata->last_note = current_time;
            break;
        case NOTE_IDEA:
            ch->pcdata->last_idea = current_time;
            break;
        case NOTE_PENALTY:
            ch->pcdata->last_penalty = current_time;
            break;
        case NOTE_NEWS:
            ch->pcdata->last_news = current_time;
            break;
        case NOTE_CHANGES:
            ch->pcdata->last_changes = current_time;
            break;
        }
        return;
    }

    /* below this point only certain people can edit notes */
    if ((type == NOTE_NEWS && !IS_TRUSTED(ch, ANGEL))
        || (type == NOTE_CHANGES && !IS_TRUSTED(ch, CREATOR))) {
        sprintf(buf, "You aren't high enough level to write %s.", list_name);
        send_to_char(buf, ch);
        return;
    }

    if (!str_cmp(arg, "+")) {
        note_attach(ch, type);
        if (ch->pnote->type != type) {
            send_to_char("You already have a different note in progress.\n\r",
                         ch);
            return;
        }

        if (strlen(ch->pnote->text) + strlen(argument) >= 4096) {
            send_to_char("Note too long.\n\r", ch);
            return;
        }

        buffer = new_buf();

        add_buf(buffer, ch->pnote->text);
        add_buf(buffer, argument);
        add_buf(buffer, "\n\r");
        free_string(ch->pnote->text);
        ch->pnote->text = str_dup(BUF(buffer));
        free_buf(buffer);
        send_to_char("Ok.\n\r", ch);
        return;
    }

    if (!str_cmp(arg, "-")) {
        bool found = false;

        note_attach(ch, type);
        if (ch->pnote->type != type) {
            send_to_char("You already have a different note in progress.\n\r",
                         ch);
            return;
        }

        if (ch->pnote->text == NULL || ch->pnote->text[0] == '\0') {
            send_to_char("No lines left to remove.\n\r", ch);
            return;
        }

        strcpy(buf, ch->pnote->text);

        for (size_t len = strlen(buf); len > 0; len--) {
            if (buf[len] == '\r') {
                if (!found) /* back it up */
                {
                    if (len > 0) len--;
                    found = true;
                }
                else /* found the second one */
                {
                    buf[len + 1] = '\0';
                    free_string(ch->pnote->text);
                    ch->pnote->text = str_dup(buf);
                    return;
                }
            }
        }
        buf[0] = '\0';
        free_string(ch->pnote->text);
        ch->pnote->text = str_dup(buf);
        return;
    }

    if (!str_prefix(arg, "subject")) {
        note_attach(ch, type);
        if (ch->pnote->type != type) {
            send_to_char("You already have a different note in progress.\n\r",
                         ch);
            return;
        }

        free_string(ch->pnote->subject);
        ch->pnote->subject = str_dup(argument);
        send_to_char("Ok.\n\r", ch);
        return;
    }

    if (!str_prefix(arg, "to")) {
        note_attach(ch, type);
        if (ch->pnote->type != type) {
            send_to_char("You already have a different note in progress.\n\r",
                         ch);
            return;
        }
        free_string(ch->pnote->to_list);
        ch->pnote->to_list = str_dup(argument);
        send_to_char("Ok.\n\r", ch);
        return;
    }

    if (!str_prefix(arg, "clear")) {
        if (ch->pnote != NULL) {
            free_note(ch->pnote);
            ch->pnote = NULL;
        }

        send_to_char("Ok.\n\r", ch);
        return;
    }

    if (!str_prefix(arg, "show")) {
        if (ch->pnote == NULL) {
            send_to_char("You have no note in progress.\n\r", ch);
            return;
        }

        if (ch->pnote->type != type) {
            send_to_char("You aren't working on that kind of note.\n\r", ch);
            return;
        }

        sprintf(buf, "%s: %s\n\rTo: %s\n\r", ch->pnote->sender,
                ch->pnote->subject, ch->pnote->to_list);
        send_to_char(buf, ch);
        send_to_char(ch->pnote->text, ch);
        return;
    }

    if (!str_prefix(arg, "post") || !str_prefix(arg, "send")) {
        char* strtime;

        if (ch->pnote == NULL) {
            send_to_char("You have no note in progress.\n\r", ch);
            return;
        }

        if (ch->pnote->type != type) {
            send_to_char("You aren't working on that kind of note.\n\r", ch);
            return;
        }

        if (!str_cmp(ch->pnote->to_list, "")) {
            send_to_char(
                "You need to provide a recipient (name, all, or immortal).\n\r",
                ch);
            return;
        }

        if (!str_cmp(ch->pnote->subject, "")) {
            send_to_char("You need to provide a subject.\n\r", ch);
            return;
        }

        ch->pnote->next = NULL;
        strtime = ctime(&current_time);
        strtime[strlen(strtime) - 1] = '\0';
        ch->pnote->date = str_dup(strtime);
        ch->pnote->date_stamp = current_time;

        append_note(ch->pnote);
        ch->pnote = NULL;
        return;
    }

    send_to_char("You can't do that.\n\r", ch);
    return;
}

NoteData* new_note()
{
    NoteData* note;

    if (note_free == NULL)
        note = alloc_perm(sizeof(*note));
    else {
        note = note_free;
        note_free = note_free->next;
    }
    VALIDATE(note);
    return note;
}

void free_note(NoteData* note)
{
    if (!IS_VALID(note)) return;

    free_string(note->text);
    free_string(note->subject);
    free_string(note->to_list);
    free_string(note->date);
    free_string(note->sender);
    INVALIDATE(note);

    note->next = note_free;
    note_free = note;
}
