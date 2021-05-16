/***************************************************************************\
[*]    __     __  ___                ___  | LexiMUD is a C++ MUD codebase [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ |        for Sci-Fi MUDs        [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / | Copyright(C) 1997-1999        [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  | All rights reserved           [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   | Chris Jacobson (FearItself)   [*]
[*]      A Creation of the AvP Team!      | fear@technologist.com         [*]
[-]---------------------------------------+-------------------------------[-]
[*] File : help.h                                                         [*]
[*] Usage: Help system header                                             [*]
\***************************************************************************/


#ifndef __HELP_H__
#define __HELP_H__


#include "types.h"


class Help {
public:
						Help(void);
						~Help(void);
	void				Free();
	
	char *				keywords;
	char *				entry;

	SInt32				level;
	SInt16				prerequisite;	// Daniel Rollings AKA Daniel Houghton's new addition:  Prerequisite
	SInt8				prereqchance; // skills for accessing certain help sections.
	SInt8				trust;

public:
	static Help *		Table;
	static SInt32		Top;
	
	static Help *		WizTable;
	static SInt32		WizTop;

	static Help *		Find(const char *keyword, int type = 0);
	static void			Load(FILE *fl);
	static void			WizLoad(FILE *fl);
};

#endif

