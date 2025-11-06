////////////////////////////////////////////////////////////////////////////////
// data/tutorial.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__DATA__TUTORIAL_H
#define MUD98__DATA__TUTORIAL_H

#include <stdbool.h>

typedef struct Tutorial Tutorial;

typedef struct TutorialStep {
    const char* prompt;
    const char* match;
} TutorialStep;

typedef struct Tutorial {
    const char* name;
    const char* blurb;
    const char* finish;
    int min_level;
    TutorialStep* steps;
    int step_count;
} Tutorial;

void load_tutorials();
void save_tutorials();
Tutorial* get_tutorial(const char* name);

#endif // !MUD98__DATA__TUTORIAL_H
