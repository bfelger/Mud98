////////////////////////////////////////////////////////////////////////////////
// data/social.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__DATA__SOCIAL_H
#define MUD98__DATA__SOCIAL_H

typedef struct social_t {
    char* name;
    char* char_no_arg;
    char* others_no_arg;
    char* char_found;
    char* others_found;
    char* vict_found;
    char* char_not_found;
    char* char_auto;
    char* others_auto;
} Social;

void load_social_table(void);
void save_social_table(void);

extern Social* social_table;
extern int social_count;

#endif // !MUD98__DATA__SOCIAL_H
