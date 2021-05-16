/* ************************************************************************
*   File: spec_assign.c                                 Part of CircleMUD *
*  Usage: Functions to assign function pointers to objs/mobs/rooms        *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */


#include "structs.h"
#include "spec.h"
#include "rooms.h"
#include "characters.h"
#include "objects.h"
#include "buffer.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"

extern bool mini_mud;
extern int dts_are_dumps;

void assign_elevators(void);

/* local functions */
void ASSIGNMOB(int mob, SPECIAL(fname));
void ASSIGNOBJ(int obj, SPECIAL(fname));
void ASSIGNROOM(int room, SPECIAL(fname));
void assign_mobiles(void);
void assign_objects(void);
void assign_rooms(void);


/* table of valid mob procs */
const struct spec_type spec_mob_table[] =
{
    { "postmaster"    , postmaster },
    { "cityguard"     , cityguard },
    { "puff"	      , puff },
    { "fido"	      , fido },
    { "janitor"       , janitor },
    { "snake"	      , snake },
    { "thief"	      , thief },
    { "magic_user"    , magic_user },
    { "bank"	      , bank },

    { NULL, NULL }
};


const struct spec_type spec_shopkeeper_table[] =
{
    { "shopkeeper"    , shop_keeper },

    { NULL, NULL }
};


/* table of valid obj procs */
const struct spec_type spec_obj_table[] =
{
    { "bank"	      , bank },
    { "portal"	      , portal },

    { NULL, NULL }
};


/* table of valid room procs */
const struct spec_type spec_room_table[] =
{
    { "bank"	      , bank },
    { "dump"	      , dump },
    { "pet_shop"      , pet_shops },
    
    { NULL, NULL }
};


SpecProc spec_lookup(const char *name, const struct spec_type spec_table[], bool exact = FALSE)
{
    int i;
  
    for (i = 0; spec_table[i].name != NULL; i++) {
      if (exact) {
      if (!str_cmp(name, spec_table[i].name))
	  return spec_table[i].func;
      } else {
        if (!strn_cmp(name, spec_table[i].name, strlen(name)))
	  return spec_table[i].func;
      }
    }
  
    return NULL;
}

SpecProc spec_mob_lookup(const char *name)
{
    return spec_lookup(name, spec_mob_table);
}

SpecProc spec_obj_lookup(const char *name)
{
    return spec_lookup(name, spec_obj_table);
}

SpecProc spec_room_lookup(const char *name)
{
    return spec_lookup(name, spec_room_table);
}


const char *spec_name(SPECIAL(func), const struct spec_type spec_table[])
{
    int i;

    for (i = 0; spec_table[i].name != NULL && spec_table[i].func != func; i++);

    return spec_table[i].name;
}


const char *spec_mob_name(SPECIAL(func))
{
    if (func == shop_keeper)
      return (spec_shopkeeper_table[0].name);
    return spec_name(func, spec_mob_table);
}


const char *spec_obj_name(SPECIAL(func))
{
    return spec_name(func, spec_obj_table);
}


const char *spec_room_name(SPECIAL(func))
{
    return spec_name(func, spec_room_table);
}


/* functions to perform assignments */

void ASSIGNMOB(int mob, SPECIAL(fname)) {
	if (Character::Find(mob))
		Character::Index[mob].func = fname;
	else if (!mini_mud)
		log("SYSERR: Attempt to assign spec to non-existant mob #%d", mob);
}

void ASSIGNOBJ(int obj, SPECIAL(fname)) {
	if (Object::Find(obj))
		Object::Index[obj].func = fname;
	else if (!mini_mud)
		log("SYSERR: Attempt to assign spec to non-existant obj #%d", obj);
}

void ASSIGNROOM(int room, SPECIAL(fname)) {
	if (Room::Find(room))
		world[room].func = fname;
	else if (!mini_mud)
		log("SYSERR: Attempt to assign spec to non-existant rm. #%d", room);
}


/* ********************************************************************
*  Assignments                                                        *
******************************************************************** */

/* assign special procedures to mobiles */
void assign_mobiles(void)
{
  SPECIAL(puff);
  SPECIAL(postmaster);
  SPECIAL(guild_proc);
/*  
  SPECIAL(cityguard);
  SPECIAL(receptionist);
  SPECIAL(cryogenicist);
  SPECIAL(guild_guard);
  SPECIAL(guild);
  SPECIAL(fido);
  SPECIAL(janitor);
  SPECIAL(mayor);
  SPECIAL(snake);
  SPECIAL(thief);
  SPECIAL(magic_user); */
/*  void assign_kings_castle(void); */

/*  assign_kings_castle(); */

  ASSIGNMOB(1, puff);
  ASSIGNMOB(1201, postmaster);
//  ASSIGNMOB(8005, temp_guild);
//  ASSIGNMOB(7005, temp_guild);
  
//  ASSIGNMOB(8020, guild_proc);		//	Marine trainer
//  ASSIGNMOB(688, guild_proc);		//	Yautja Trainer
//  ASSIGNMOB(25000, guild_proc);		//	Alien trainer
//  ASSIGNMOB(8898, guild_proc);
}



/* assign special procedures to objects */
void assign_objects(void)
{
}

/* assign special procedures to rooms */
void assign_rooms(void) {
	SPECIAL(dump);

	// assign_elevators();
  
	if (!dts_are_dumps)	return;

	Map<VNum, Room>::Iterator	iter(world);
	Room *r;
	while ((r = iter.Next()))
		if (IS_SET(r->RoomFlags(), ROOM_DEATH))
			r->func = dump;
}


