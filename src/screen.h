////////////////////////////////////////////////////////////////////////////////
// screen.c
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef ROM__SCREEN_H
#define ROM__SCREEN_H

void InitScreen(DESCRIPTOR_DATA *);
void InitScreenMap(DESCRIPTOR_DATA *);
void UpdateOLCScreen(DESCRIPTOR_DATA *);

#define OLCS_STRING		1
#define OLCS_INT		2
#define OLCS_INT16		3
#define OLCS_STRFUNC		4
#define OLCS_FLAGSTR_INT	5
#define OLCS_FLAGSTR_INT16	6
#define OLCS_BOOL		7
#define OLCS_TAG		8

struct olc_show_table_type {
	char*		name;
	uintptr_t	point;
	char*		desc;
	int16_t		type;
	int16_t		x;
	int16_t		y;
	int16_t		sizex;
	int16_t		sizey;
	int16_t		page;
	const uintptr_t	func;
};

extern const struct olc_show_table_type redit_olc_show_table[];
extern const struct olc_show_table_type medit_olc_show_table[];
extern const struct olc_show_table_type oedit_olc_show_table[];

typedef char * STRFUNC ( void * );

extern STRFUNC areaname;

#endif // !ROM__SCREEN_H
