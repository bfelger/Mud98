////////////////////////////////////////////////////////////////////////////////
// note.h
////////////////////////////////////////////////////////////////////////////////

typedef struct note_data_t NoteData;

#pragma once
#ifndef MUD98__NOTE_H
#define MUD98__NOTE_H

#include <stdint.h>
#include <time.h>

#define NOTE_NOTE    0
#define NOTE_IDEA    1
#define NOTE_PENALTY 2
#define NOTE_NEWS    3
#define NOTE_CHANGES 4

typedef struct note_data_t {
    NoteData* next;
    char* sender;
    char* date;
    char* to_list;
    char* subject;
    char* text;
    time_t date_stamp;
    int16_t type;
    bool valid;
} NoteData;

NoteData* new_note();
void free_note(NoteData* note);

extern NoteData* note_list;

#endif // !MUD98__NOTE_H
