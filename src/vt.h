////////////////////////////////////////////////////////////////////////////////
// vt.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__VT_H
#define MUD98__VT_H

#include "stdbool.h"
#include "stdint.h"

#define VT_ESC                  "\033"

#define VT_CSI                  VT_ESC "["
#define VT_OSC                  VT_ESC "]"

#define VT_SAVECURSOR			VT_ESC "7"     // Save cursor and attrib
#define VT_RESTORECURSOR		VT_ESC "8"     // Restore cursor pos and attribs
#define VT_SETWIN_CLEAR			VT_CSI "r"    // Clear scrollable window size
#define VT_CLEAR_SCREEN			VT_CSI "2J"   // Clear screen
#define VT_CLEAR_LINE			VT_CSI "2K"   // Clear this whole line
#define VT_RESET_TERMINAL		VT_ESC "c"
#define VT_INITSEQ    			VT_CSI "1;24r"
#define VT_CURSPOS    			VT_CSI "%d;%dH"
#define VT_CURSRIG    			VT_CSI "%dC"
#define VT_CURSLEF    			VT_CSI "%dD"
#define VT_HOMECLR    			VT_CLEAR_SCREEN VT_CSI "0;0H"
#define VT_CTEOTCR    			VT_CSI "K"
#define VT_CLENSEQ    			VT_SETWIN_CLEAR VT_CLEAR_SCREEN
#define VT_INDUPSC    			VT_ESC "M"
#define VT_INDDOSC    			VT_ESC "D"
#define VT_SETSCRL    			VT_CSI "%d;24r"
#define VT_INVERTT    			VT_CSI "0;1;7m"
#define VT_BOLDTEX    			VT_CSI "0;1m"
#define VT_NORMALT    			VT_CSI "0m"
#define VT_MARGSET    			VT_CSI "%d;%dr"
#define VT_CURSAVE    			VT_ESC "7"
#define VT_CURREST    			VT_ESC "8"

#define VT_ST                   VT_ESC "\\"

#define VT_SAVE_SGR             VT_CSI "8]"
#define VT_OSC_SET              VT_OSC "P"
#define VT_OSC_RESET            VT_OSC "R" VT_ST

////////////////////////////////////////////////////////////////////////////////
// ColoUr stuff v2.0, by Lope.
////////////////////////////////////////////////////////////////////////////////

#define CLEAR                   VT_CSI "0m"        // Resets Colour
//#define C_RED                   "\033[0;31m"     // Normal Colours
//#define C_GREEN                 "\033[0;32m"
//#define C_YELLOW                "\033[0;33m"
//#define C_BLUE                  "\033[0;34m"
//#define C_MAGENTA               "\033[0;35m"
//#define C_CYAN                  "\033[0;36m"
//#define C_WHITE                 "\033[0;37m"
//#define C_D_GRAY                "\033[1;30m"     // Light Colors
//#define C_B_RED                 "\033[1;31m"
//#define C_B_GREEN               "\033[1;32m"
//#define C_B_YELLOW              "\033[1;33m"
//#define C_B_BLUE                "\033[1;34m"
//#define C_B_MAGENTA             "\033[1;35m"
//#define C_B_CYAN                "\033[1;36m"
//#define C_B_WHITE               "\033[1;37m"

#define COLOUR_NONE             7               // White, hmm...
#define RED                     1               // Normal Colours
#define GREEN                   2
#define YELLOW                  3
#define BLUE                    4
#define MAGENTA                 5
#define CYAN                    6
#define WHITE                   7
#define BLACK                   0

#define NORMAL                  0               // Bright/Normal colours
#define BRIGHT                  1

////////////////////////////////////////////////////////////////////////////////
// Extended 16-bit and 24-bit VT102 codes, by Halivar
////////////////////////////////////////////////////////////////////////////////

// These Select Graphic Rendition (SGR) codes are defined by ISO 8613-6 as an
// extension to EMCA-48

#define SGR_MODE_DARK           0
#define SGR_MODE_LIGHT          1
#define SGR_MODE_FG             "38"
#define SGR_MODE_BG             "48"

#define SGR_BITS_8              "5"
#define SGR_BITS_24             "2"

#endif // !MUD98__VT_H
