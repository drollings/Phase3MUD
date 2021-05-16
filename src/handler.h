/* ************************************************************************
*   File: handler.h                                     Part of CircleMUD *
*  Usage: header file: prototypes of handling and utility functions       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#ifndef __HANDLER_H__
#define __HANDLER_H__

class Object;

/* utility */
int			isname(const char *str, const char *namelist);
int			isword(const char *str, const char *namelist);
char *		fname(const char *namelist);
int			get_number(char **name);
int			silly_isname(const char *str, const char *namelist);
int			split_string(char *str, char *sep, char **argv);

/* ******* characters ********* */

int is_same_group(const Character *ach, const Character *bch );


/* prototypes from crash save system */

void	Crash_listrent(Character *ch, char *name);
int		Crash_load(Character *ch);
void	Crash_crashsave(Character *ch);
void	Crash_save_all(void);

/* prototypes from fight.c */
void	forget(Character *ch, Character *victim);
void	remember(Character *ch, Character *victim);

extern char *money_desc(int amount);
extern Object *create_money(int amount);

#endif

