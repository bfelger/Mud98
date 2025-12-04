////////////////////////////////////////////////////////////////////////////////
// persist/theme/rom-olc/theme_rom_olc_io.h
// Helpers for reading/writing ROM-OLC color theme blocks.
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__PERSIST__THEME__ROM_OLC__THEME_ROM_OLC_IO_H
#define MUD98__PERSIST__THEME__ROM_OLC__THEME_ROM_OLC_IO_H

#include <stdio.h>

#include <color.h>

ColorTheme* theme_rom_olc_read_theme(FILE* fp, const char* end_token, const char* owner);
void theme_rom_olc_write_theme(FILE* fp, const ColorTheme* theme, const char* end_token);

#endif // !MUD98__PERSIST__THEME__ROM_OLC__THEME_ROM_OLC_IO_H
