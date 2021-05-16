/* ************************************************************************
*   File: screen.h                                      Part of CircleMUD *
*  Usage: header file with ANSI color codes for online color              *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*  $Author: sfn $
*  $Date: 2001/02/04 15:57:58 $
*  $Revision: 1.3 $
************************************************************************ */

#ifndef __SCREEN_H__
#define __SCREEN_H__

enum Color 
{
	COL_PREUNSET = -2,
	COL_UNSET = -1,
	COL_OFF = 0, 
	COL_NONE, 

	COL_BLACK, COL_RED, COL_GREEN, COL_YELLOW, 
	COL_BLUE, COL_MAGENTA, COL_CYAN, COL_WHITE, 

	COL_D_BLACK, COL_D_RED, COL_D_GREEN, COL_D_YELLOW, 
	COL_D_BLUE, COL_D_MAGENTA, COL_D_CYAN, COL_D_WHITE, 

	COL_E_BLACK, COL_E_RED, COL_E_GREEN, COL_E_YELLOW, 
	COL_E_BLUE, COL_E_MAGENTA, COL_E_CYAN, COL_E_WHITE, 

	COL_FG_OFF,

	COL_BK_BLACK, COL_BK_RED, COL_BK_GREEN,COL_BK_YELLOW, 
	COL_BK_BLUE, COL_BK_MAGENTA,COL_BK_CYAN, COL_BK_WHITE, 

	COL_C_UP, COL_C_DOWN, COL_C_RIGHT, COL_C_LEFT, 
	COL_C_HOME, COL_C_CLR,
	
	COL_STYLE_UNDERLINE, COL_STYLE_FLASH, COL_STYLE_REVERSE,
	COL_STORE = 100,
	COL_RESTORE = 101
};


/* The standard ANSI colors */
#define ANSI_NONE	""
#define ANSI_OFF 	"\E[0;0m"
#define ANSI_BLACK	"\E[30m"
#define ANSI_RED 	"\E[31m"
#define ANSI_GREEN 	"\E[32m"
#define ANSI_YELLOW 	"\E[33m"
#define ANSI_BLUE 	"\E[34m"
#define ANSI_MAGENTA 	"\E[35m"
#define ANSI_CYAN 	"\E[36m"
#define ANSI_WHITE 	"\E[37m"

/* The standard ANSI colors */
#define ANSI_D_BLACK	"\E[0;30m"
#define ANSI_D_RED 	"\E[0;31m"
#define ANSI_D_GREEN 	"\E[0;32m"
#define ANSI_D_YELLOW 	"\E[0;33m"
#define ANSI_D_BLUE 	"\E[0;34m"
#define ANSI_D_MAGENTA 	"\E[0;35m"
#define ANSI_D_CYAN 	"\E[0;36m"
#define ANSI_D_WHITE 	"\E[0;37m"


#define ANSI_FG_OFF	"\E[38m"	/* works on normal terminals but not
						 color_xterms */

/* The bold or extended ANSI colors */
#define ANSI_E_BLACK	"\E[1;30m"		 /* &cl */
#define ANSI_E_RED 	"\E[1;31m"
#define ANSI_E_GREEN 	"\E[1;32m"
#define ANSI_E_YELLOW 	"\E[1;33m"
#define ANSI_E_BLUE 	"\E[1;34m"
#define ANSI_E_MAGENTA 	"\E[1;35m"
#define ANSI_E_CYAN 	"\E[1;36m"
#define ANSI_E_WHITE 	"\E[1;37m"

/* The background colors */

#define ANSI_BK_BLACK	"\E[40m"	/* &cL */
#define ANSI_BK_RED 	"\E[41m"
#define ANSI_BK_GREEN 	"\E[42m"
#define ANSI_BK_YELLOW 	"\E[43m"
#define ANSI_BK_BLUE 	"\E[44m"
#define ANSI_BK_MAGENTA "\E[45m"
#define ANSI_BK_CYAN 	"\E[46m"
#define ANSI_BK_WHITE 	"\E[47m"

#define STYLE_UNDERLINE	"\E[4m"	/* &cu */
#define STYLE_FLASH		"\E[5m"	/* &cf */
#define STYLE_REVERSE	"\E[7m"	/* &cv */


/* The standard ANSI colors */
#define HTML_NONE	"<FONT COLOR='#CCCCCC'>"
#define HTML_OFF	"<FONT COLOR='#CCCCCC'>"
#define HTML_BLACK	"<FONT COLOR='#000000'>"
#define HTML_RED	"<FONT COLOR='#FF0000'>"
#define HTML_GREEN	"<FONT COLOR='#009900'>"
#define HTML_YELLOW	"<FONT COLOR='#FFFF99'>"
#define HTML_BLUE	"<FONT COLOR='#000099'>"
#define HTML_MAGENTA	"<FONT COLOR='#993399'>"
#define HTML_CYAN	"<FONT COLOR='#33CCFF'>"
#define HTML_WHITE	"<FONT COLOR='#CCCCCC'>"
#define HTML_FG_OFF	"<FONT COLOR='#CCCCCC'>"	/* works on normal terminals but not
								 color_xterms */

/* The bold or extended ANSI colors */
#define HTML_E_BLACK	"<FONT COLOR='#000000'>"	/* &cl */
#define HTML_E_RED	"<FONT COLOR='#FF0000'>"
#define HTML_E_GREEN	"<FONT COLOR='#009900'>"
#define HTML_E_YELLOW	"<FONT COLOR='#FFFF99'>"
#define HTML_E_BLUE	"<FONT COLOR='#000099'>"
#define HTML_E_MAGENTA	"<FONT COLOR='#993399'>"
#define HTML_E_CYAN	"<FONT COLOR='#33CCFF'>"
#define HTML_E_WHITE	"<FONT COLOR='#CCCCCC'>"

/* The background colors */

#define HTML_BK_BLACK	"<FONT BGCOLOR='#000000'>"	/* &cL */
#define HTML_BK_RED	"<FONT BGCOLOR='#FF0000'>"
#define HTML_BK_GREEN	"<FONT BGCOLOR='#009900'>"
#define HTML_BK_YELLOW	"<FONT BGCOLOR='#FFFF99'>"
#define HTML_BK_BLUE	"<FONT BGCOLOR='#000099'>"
#define HTML_BK_MAGENTA	"<FONT BGCOLOR='#993399'>"
#define HTML_BK_CYAN	"<FONT BGCOLOR='#33CCFF'>"
#define HTML_BK_WHITE	"<FONT BGCOLOR='#CCCCCC'>"

#define IS_DIM(num)	(((num) >= COL_D_BLACK) && ((num) <= COL_D_WHITE))
#define IS_BOLD(num)	(((num) >= COL_E_BLACK) && ((num) <= COL_E_WHITE))

#define DEFAULT_PAGE_LENGTH	22   /* default number of lines in a page */


// VT100 defines

#define VTMOVE(x,y)	    	ESC[x;yH
#define VTUP(y)			ESC[yA
#define VTDOWN(y)		ESC[yB
#define VTRIGHT(x)		ESC[xC
#define VTLEFT(x)		ESC[xD
#define VTFIND(x,y) 		ESC[x;yR
#define VTSAVE			ESC[s
#define VTRETURN		ESC[u

/* Erase Functions */

#define VTCLRSCR		ESC[2J
#define VTCLRTOEOL		ESC[K

/* Set Graphics Rendition */

#define VTSET(args)		ESC[args

#define VTNORM			0      /* Normal Text */
#define VTBOLD                  1      /* Bold Text */
#define VTUNDERLINE             4      /* Underline (mono only) */
#define VTBLINK                 5      /* Blink on */
#define VTREVERSE               7      /* Reverse on */
#define VTINVIS                 8      /* Invisible */
#define VTBLKFGRD               30     /* Black Foreground */
#define VTREDFGRD               31     /* Red Foreground */
#define VTGRNFGRD               32     /* Green fgrd */
#define VTYELFGRD               33     /* Yellow fgrd */
#define VTBLUFGRD		34     /* Blue fgrd */
#define VTMAGFGRD               35     /* Magenta fgrd */
#define VTCYNFGRD               36     /* Cyan fgrd */
#define VTWHTFGRD               37     /* White fgrd */
#define VTBLKBGRD               40     /* Black Background */
#define VTREDBGRD               41     /* Red bgrd */
#define VTGRNBGRD               42     /* Green fgrd */
#define VTYELBGRD               43     /* Yellow fgrd */
#define VTBLUBGRD               44     /* Blue fgrd */
#define VTMAGBGRD               45     /* Magenta fgrd */
#define VTCYNBGRD               46     /* Cyan fgrd */
#define VTWHTBGRD               47     /* White fgrd */

/*
Keyboard Reassignments:
         ESC[#;#;...p                   Keyboard reassignment. The first ASCII
         or ESC["string"p               code defines which code is to be 
         or ESC[#;"string";#;           changed. The remaining codes define
            #;"string";#p               what it is to be changed to.

         E.g. Reassign the Q and q keys to the A and a keys (and vice versa).
         ESC [65;81p                    A becomes Q
         ESC [97;113p                   a becomes q
         ESC [81;65p                    Q becomes A
         ESC [113;97p                   q becomes a

         E.g. Reassign the F10 key to a DIR command.
         ESC [0;68;"dir";13p            The 0;68 is the extended ASCII code 
                                        for the F10 key and 13 is the ASCII
                                        code for a carriage return.
         
         Other function key codes       F1=59,F2=60,F3=61,F4=62,F5=63
                                        F6=64,F7=65,F8=66,F9=67,F10=68

*/



#endif

