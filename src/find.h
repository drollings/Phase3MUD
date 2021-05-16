#ifndef __FIND_H__
#define __FIND_H__
/**************************************************************************
*  File: find.h                                                           *
*  Usage: contains prototypes of functions for finding mobs and objs      *
*         in lists                                                        *
*                                                                         *
**************************************************************************/

#include "types.h"
#include "stl.llist.h"

class Object;
class Character;
class Room;

Object *	get_obj(const char *name);
Object *	get_obj_num(VNum nr);
Object *	get_obj_vis(const Character *ch, const char *name);
Object *	get_obj_in_list(const char *name, LList<Object *> &list);
Object *	get_obj_in_list_vis(const Character *ch, const char *name, LList<Object *> &list);
Object *	get_obj_in_list_num(VNum num, LList<Object *> &list);
Object *	get_obj_in_list_type(Type type, LList<Object *> &list);
Object *	get_obj_in_list_flagged(Flags flag, LList<Object *> &list);
Object *	get_object_in_equip(const Character * ch, const char *arg, Object * const equipment[], int *j);
Object *	get_object_in_equip_vis(const Character * ch, const char *arg, Object * const equipment[], int *j);
Object *	get_obj_by_obj(const Object *obj, const char *name);
Object *	get_obj_by_room(const Room *room, const char *name);
Object *	find_vehicle_by_vnum(VNum vnum);
Object *	find_obj(IDNum n);

int	get_num_chars_on_obj(const Object * obj);
Character *	get_char_on_obj(const Object *obj);

Character *	get_char(const char *name);
Character *	get_char_room(const char *name, VNum room);
Character *	get_char_num(VNum nr);
Character *	get_char_by_obj(const Object *obj, const char *name);

/* find if character can see */
//Character *	get_char_room_vis(const Character *ch, const char *name);
Character *	get_player_vis(const Character *ch, const char *name, Flags mode);
Character *	get_char_vis(const Character *ch, const char *name, Flags mode);

Character *find_char(IDNum n);
Character *get_char_by_room(const Room *room, const char *name);

Room *get_room(const char *name);
Room *find_room(IDNum n);

VNum find_the_room(const char *roomstr);

int count_mobs_in_room(VNum num, VNum room);
int count_mobs_in_zone(VNum num, int zone);

/* find all dots */

int	find_all_dots(char *arg);

#define FIND_INDIV	0
#define FIND_ALL	1
#define FIND_ALLDOT	2


/* Generic Find */

int	generic_find(const char *arg, int bitvector, const Character *ch, Character **tar_ch, Object **tar_obj);


enum {
	FIND_CHAR_ROOM		= (1 << 0),
	FIND_CHAR_WORLD		= (1 << 1),
	FIND_OBJ_INV		= (1 << 2),
	FIND_OBJ_EQUIP		= (1 << 3),
	FIND_OBJ_ROOM		= (1 << 4),
	FIND_OBJ_WORLD		= (1 << 5)
};


/*
 * Escape character, which indicates that the name is
 * a unique id number rather than a standard name.
 * Should be nonprintable so players can't enter it.
 */
static const char UID_CHAR = '\x1b';

#endif

