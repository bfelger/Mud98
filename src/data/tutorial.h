////////////////////////////////////////////////////////////////////////////////
// data/tutorial.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__DATA__TUTORIAL_H
#define MUD98__DATA__TUTORIAL_H

#include <stdbool.h>

#include <entities/mobile.h>

typedef struct Tutorial Tutorial;

typedef struct TutorialStep {
    char* prompt;
    char* match;
} TutorialStep;

typedef struct Tutorial {
    char* name;
    char* blurb;
    char* finish;
    int min_level;
    TutorialStep* steps;
    int step_count;
} Tutorial;

void show_tutorial_step(Mobile* ch, Tutorial* t, int step);
void advance_tutorial_step(Mobile* ch);
Tutorial* get_tutorial(const char* name);

void load_tutorials();
void save_tutorials();

extern Tutorial** tutorials;
extern int tutorial_count;

#endif // !MUD98__DATA__TUTORIAL_H
