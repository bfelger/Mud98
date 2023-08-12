////////////////////////////////////////////////////////////////////////////////
// vt.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef ROM__VT_H
#define ROM__VT_H

#define VT_SAVECURSOR			"\0337"     // Save cursor and attrib
#define VT_RESTORECURSOR		"\0338"     // Restore cursor pos and attribs
#define VT_SETWIN_CLEAR			"\033[r"    // Clear scrollable window size
#define VT_CLEAR_SCREEN			"\033[2J"   // Clear screen
#define VT_CLEAR_LINE			"\033[2K"   // Clear this whole line
#define VT_RESET_TERMINAL		"\033c"
#define VT_INITSEQ    			"\033[1;24r"
#define VT_CURSPOS    			"\033[%d;%dH"
#define VT_CURSRIG    			"\033[%dC"
#define VT_CURSLEF    			"\033[%dD"
#define VT_HOMECLR    			"\033[2J\033[0;0H"
#define VT_CTEOTCR    			"\033[K"
#define VT_CLENSEQ    			"\033[r\033[2J"
#define VT_INDUPSC    			"\033M"
#define VT_INDDOSC    			"\033D"
#define VT_SETSCRL    			"\033[%d;24r"
#define VT_INVERTT    			"\033[0;1;7m"
#define VT_BOLDTEX    			"\033[0;1m"
#define VT_NORMALT    			"\033[0m"
#define VT_MARGSET    			"\033[%d;%dr"
#define VT_CURSAVE    			"\0337"
#define VT_CURREST    			"\0338"

#endif // !ROM__VT_H
