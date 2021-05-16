/* ************************************************************************
*  File: spec.h                                  Part of Death's Gate MUD *
*                                                                         *
*  Usage: Header stuff for special procedures                             *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Death's Gate MUD is based on CircleMUD, Copyright (C) 1993, 94.        *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author: sfn $
*  $Date: 2001/02/04 15:57:58 $
*  $Revision: 1.3 $
************************************************************************ */

#ifndef __SPEC_H__
#define __SPEC_H__
/* predef of all spec procs available */
SPECIAL(postmaster);
SPECIAL(cityguard);
SPECIAL(receptionist);
SPECIAL(cryogenicist);
SPECIAL(guild);
SPECIAL(puff);
SPECIAL(fido);
SPECIAL(janitor);
SPECIAL(snake);
SPECIAL(thief);
SPECIAL(magic_user);
SPECIAL(lag_monster);
SPECIAL(bank);
SPECIAL(gen_board);
SPECIAL(dump);
SPECIAL(pet_shops);
SPECIAL(portal);

SPECIAL(shop_keeper);

struct spec_type
{
  char *name;               /* special function name */
  SPECIAL(*func);           /* the function */
};


/* special tables */
extern const struct spec_type spec_mob_table[];
extern const struct spec_type spec_obj_table[];
extern const struct spec_type spec_room_table[];

typedef SPECIAL(*SpecProc);

/* functions for specials */
SpecProc spec_lookup(const char *name, const struct spec_type spec_table[], 
                     bool exact = false);
SpecProc spec_mob_lookup(const char *name);
SpecProc spec_obj_lookup(const char *name);
SpecProc spec_room_lookup(const char *name);

const char *spec_name(SPECIAL(func), const struct spec_type spec_table[]);
const char *spec_mob_name(SPECIAL(func));
const char *spec_obj_name(SPECIAL(func));
const char *spec_room_name(SPECIAL(func));

void assign_mobiles(void);
void assign_objects(void);
void assign_rooms(void);

#endif

