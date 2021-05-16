/**************************************************************************
*  File: find.c                                                           *
*  Usage: contains functions for finding mobs and objs in lists           *
**************************************************************************/



#include "structs.h"
#include "utils.h"
#include "find.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "buffer.h"
#include "descriptors.h"
#include "rooms.h"
#include "characters.h"
#include "objects.h"

Character *get_char_room_vis(const Character * ch, const char *name);
bool get_valid_keyword(Character *ch, MUDObject *thing, char *arg);

int get_number(char **name) {
	int i;
	char *ppos;
	char *number = get_buffer(MAX_INPUT_LENGTH);

	if ((ppos = strchr(*name, '.'))) {
		*ppos++ = '\0';
		strcpy(number, *name);
		strcpy(*name, ppos);

		for (i = 0; *(number + i); i++)
			if (!isdigit(*(number + i))) {
				release_buffer(number);
				return 0;
			}

		i = atoi(number);
		release_buffer(number);
		return i;
	}
	release_buffer(number);
	return 1;
}


// Calling/introduction support -DH
bool get_valid_keyword(const Character *ch, const MUDObject *thing, const char *arg) {
	if (isname(arg, thing->CalledKeywords(ch)))
		return true;

	return false;
}


Character *get_char(const char *name) {
	Character *i = NULL;

	if (*name == UID_CHAR) {
		i = find_char(atoi(name + 1));
		if (i /* && !GET_INVIS_LEV(i) */)
			return i;
	} else {
		START_ITER(iter, i, Character *, Characters) {
			if (silly_isname(name, i->Keywords("")) /* && !GET_INVIS_LEV(i) */)
				break;
		}
	}
	return i;
}


/* returns the object in the world with name name, or NULL if not found */
Object *get_obj(const char *name) {
	Object *obj = NULL;
	IDNum id;

	if (*name == UID_CHAR) {
		id = atoi(name + 1);

		START_ITER(iter, obj, Object *, Objects) {
			if (id == GET_ID(obj))
				break;
		}
	} else {
		START_ITER(iter, obj, Object *, Objects) {
			if (silly_isname(name, obj->Keywords("")))
				break;
		}
	}
	return obj;
}


/* finds room by with name.  returns NULL if not found */
Room *get_room(const char *name) {
    int nr = -1;

    if (*name == UID_CHAR)								return find_room(atoi(name + 1));

	if (isdigit(*name) && Room::Find(nr = atoi(name)))	return &world[nr];
	else												return NULL;
}


Character *get_player_vis(const Character *ch, const char *name, Flags mode) {
	Character *i = NULL;

	if (*name == UID_CHAR) {
		i = find_char(atoi(name + 1));
		if (i && IS_NPC(i) && (!mode || IN_ROOM(i) == IN_ROOM(ch)) && i->CanBeSeen(ch))
			return i;
	} else {
		START_ITER(iter, i, Character *, Characters) {
			if (!IS_NPC(i) && (mode == FIND_CHAR_WORLD || IN_ROOM(i) == IN_ROOM(ch)) &&
					i->CanBeSeen(ch) && get_valid_keyword(ch, i, name))
				break;
		}
	}
	return i;
}


Character *get_char_vis(const Character * ch, const char *name, Flags mode) {
	Character *i = NULL;
	int j = 0, number;
	static char tmpbuf[MAX_INPUT_LENGTH];
	char *tmp = tmpbuf;		// I believe get_number will twiddle the pointer and cause a memory leak. -DH

	/* check the room first */
	if ((i = get_char_room_vis(ch, name)))
		return i;

	if (*name == UID_CHAR) {
		i = find_char(atoi(name + 1));
		if (i && i->CanBeSeen(ch))
			return i;
	} else if (mode != FIND_CHAR_ROOM) {
		strcpy(tmp, name);
		if (!(number = get_number(&tmp)))
			i = get_player_vis(ch, tmp, mode);
		else {
			START_ITER(iter, i, Character *, Characters) {
				if (j > number) {
					i = NULL;
					break;
				}
				if (i->CanBeSeen(ch) && get_valid_keyword(ch, i, tmp))
					if (++j == number)
						break;
			}
		}
	}
	return i;
}


/* search a room for a char, and return a pointer if found..  */
Character *get_char_room(const char *name, VNum room) {
	Character *i = NULL;
	int j = 0, number;
	char *tmpname, *tmp;

	if (*name == UID_CHAR) {
		i = find_char(atoi(name + 1));
		if (i && (IN_ROOM(i) == room) /* && !GET_INVIS_LEV(i) */)
			return i;
	} else {
		tmpname = get_buffer(MAX_INPUT_LENGTH);
		tmp = tmpname;

		strcpy(tmp, name);
		if ((number = get_number(&tmp))) {
			START_ITER(iter, i, Character *, world[room].people) {
				if (j > number) {
					i = NULL;
					break;
				}
				if (isname(tmp, i->Keywords("")))
					if (++j == number)
						break;
			}
		}

		release_buffer(tmpname);
	}
	return i;
}


Character *get_char_room_vis(const Character * ch, const char *name) {
	Character *i = NULL;
	int j = 0, number;
	char *tmp;

	if (*name == UID_CHAR) {
		i = find_char(atoi(name + 1));

		if (i && (IN_ROOM(ch) == IN_ROOM(i)) && i->CanBeSeen(ch))
			return i;
	} else {
		if (!str_cmp(name, "self") || !str_cmp(name, "me"))
			return const_cast<Character *>(ch);

		tmp = get_buffer(MAX_INPUT_LENGTH);
		/* 0.<name> means PC with name */
		strcpy(tmp, name);
		if (!(number = get_number(&tmp))) {
			i = get_player_vis(ch, tmp, FIND_CHAR_ROOM);
		} else {
			START_ITER(iter, i, Character *, world[IN_ROOM(ch)].people) {
				if (j > number) {
					i = NULL;
					break;
				}
				if (get_valid_keyword(ch, i, tmp) && i->CanBeSeen(ch) && (++j == number))
					break;
			}
		}

		release_buffer(tmp);
	}
	return i;
}



Character *get_char_by_obj(const Object *obj, const char *name) {
	Character *ch = NULL;

	if (*name == UID_CHAR) {
		ch = find_char(atoi(name + 1));
		if (ch /* && !GET_INVIS_LEV(ch) */)
			return ch;
	} else {
		if (obj->CarriedBy() && silly_isname(name, obj->CarriedBy()->Keywords("")) /* &&
				!GET_INVIS_LEV(obj->CarriedBy()) */)
			ch = obj->CarriedBy();
		else if (obj->WornBy() && silly_isname(name, obj->WornBy()->Keywords("")) /* &&
				!GET_INVIS_LEV(obj->WornBy()) */)
			ch = obj->WornBy();
		else {
			START_ITER(iter, ch, Character *, Characters)
				if (silly_isname(name, ch->Keywords("")) /* && !GET_INVIS_LEV(ch) */)
					break;
		}
	}
	return ch;
}


Character *get_char_by_room(const Room *room, const char *name) {
	Character *ch;
	LListIterator<Character *>	iter;

	if (*name == UID_CHAR) {
		ch = find_char(atoi(name + 1));
		if (ch /* && !GET_INVIS_LEV(ch) */)
			return ch;
	} else {
		iter.Start(room->people);
		while ((ch = iter.Next())) {
			if (silly_isname(name, ch->Keywords("")) /* && !GET_INVIS_LEV(ch) */)
				return ch;
		}
		iter.Restart(Characters);
		while ((ch = iter.Next())) {
			if (silly_isname(name, ch->Keywords("")) /* && !GET_INVIS_LEV(ch) */)
				return ch;
		}
	}
	return NULL;
}


/************************************************************
 * object searching routines
 ************************************************************/

/* search the entire world for an object, and return a pointer  */
Object *get_obj_vis(const Character * ch, const char *name) {
	Object *i = NULL;
	int j = 0, number, eq = 0;
	IDNum id;
	char *tmp;

	/* scan items carried */
	if ((i = get_obj_in_list_vis(ch, name, ((Character *) ch)->contents)))
		return i;

	if ((i = get_object_in_equip_vis(ch, name, ch->equipment, &eq)))
		return i;

	/* scan room */
	if ((i = get_obj_in_list_vis(ch, name, world[IN_ROOM(ch)].contents)))
		return i;

	if (*name == UID_CHAR) {
		id = atoi(name + 1);
		START_ITER(iter, i, Object *, Objects)
			if ((id == GET_ID(i)) && i->CanBeSeen(ch))
				break;
	} else {
		tmp = get_buffer(MAX_INPUT_LENGTH);
		strcpy(tmp, name);
		if ((number = get_number(&tmp))) {
			START_ITER(iter, i, Object *, Objects) {
				if (j > number) {
					i = NULL;
					break;
				}
				if (i->CanBeSeen(ch) && (!i->CarriedBy() || i->CarriedBy()->CanBeSeen(ch)) &&
						isname(tmp, i->CalledKeywords(ch)) && (++j == number))
					break;
			}
		}
		release_buffer(tmp);
	}
	return i;
}


Object *get_object_in_equip(const Character * ch, const char *arg, Object * const equipment[], int *j) {
	IDNum id;
	char *tmp, *tmpname;
	int n = 0, number;
	Object *obj = NULL;

	if (*arg == UID_CHAR) {
		id = atoi(arg + 1);
		for ((*j) = 0; (*j) < NUM_WEARS; (*j)++) {
			if ((obj = equipment[(*j)]) && (id == GET_ID(obj)))
				break;
		}
	} else {
		tmpname = get_buffer(MAX_INPUT_LENGTH);
		tmp = tmpname;

		strcpy(tmp, arg);

		if ((number = get_number(&tmp))) {
			for ((*j) = 0; (*j) < NUM_WEARS; (*j)++) {
				if ((obj = equipment[(*j)]) && isname(arg, obj->CalledKeywords(ch)) &&
						(++n == number))
					break;
			}
		}
		release_buffer(tmpname);
	}
	return (*j < NUM_WEARS) ? obj : NULL;
}


Object *get_object_in_equip_vis(const Character * ch, const char *arg, Object * const equipment[], int *j) {
	IDNum id;
	Object *obj = NULL;
	char *tmp, *tmpname;
	int n = 0, number;

	if (*arg == UID_CHAR) {
		id = atoi(arg + 1);
		for ((*j) = 0; (*j) < NUM_WEARS; (*j)++) {
			if ((obj = equipment[(*j)]) && obj->CanBeSeen(ch) &&
					(id == GET_ID(obj)))
				break;
		}
	} else {
		tmpname = get_buffer(MAX_INPUT_LENGTH);
		tmp = tmpname;

		strcpy(tmp, arg);

		if ((number = get_number(&tmp))) {
			for ((*j) = 0; (*j) < NUM_WEARS; (*j)++) {

				obj = equipment[(*j)];
				if (obj == NULL)		continue;

				if (obj->CanBeSeen(ch) && get_valid_keyword(ch, obj, tmp) && (++n == number))
					break;
			}
		}
		release_buffer(tmpname);
	}
	return (*j < NUM_WEARS) ? obj : NULL;
}


Object *get_obj_in_list(const char *name, LList<Object *> &list) {
	Object *i = NULL;
	IDNum id;
	char *tmp;
	int j = 0, number;

	if (*name == UID_CHAR) {
		id = atoi(name +1);
		START_ITER(iter, i, Object *, list)
			if (id == GET_ID(i))
				break;
	} else {
		tmp = get_buffer(MAX_INPUT_LENGTH);
		strcpy(tmp, name);

		if ((number = get_number(&tmp))) {
			START_ITER(iter, i, Object *, list) {
				if (j > number) {
					i = NULL;
					break;
				}
				if (isname(tmp, i->Keywords("")))
					if (++j == number) {
						release_buffer(tmp);
						return i;
					}
			}
		}
		release_buffer(tmp);
	}
	return i;
}


Object *get_obj_in_list_vis(const Character * ch, const char *name, LList<Object *> &list) {
	Object *i = NULL;
	int j = 0, number;
	char *tmp;
	IDNum id;
	LListIterator<Object *>	iter(list);

	if (*name == UID_CHAR) {
		id = atoi(name + 1);
		while ((i = iter.Next())) {
			if ((id == GET_ID(i)) && i->CanBeSeen(ch))
				break;
		}
	} else {
		tmp = get_buffer(MAX_INPUT_LENGTH);
		strcpy(tmp, name);
		if ((number = get_number(&tmp))) {
			while ((i = iter.Next())) {
				if (j > number) {
					i = NULL;
					break;
				}
				if (i->CanBeSeen(ch) && get_valid_keyword(ch, i, tmp))
					if (++j == number)
						break;
			}
		}
		release_buffer(tmp);
	}
	return i;
}


/* Search the given list for an object type, and return a ptr to that obj */
Object *get_obj_in_list_type(Type type, LList<Object *> &list) {
	Object * i;

	START_ITER(iter, i, Object *, list)
		if (GET_OBJ_TYPE(i) == type)
			break;
	return i;
}


/* Search the given list for an object that has 'flag' set, and return a ptr to that obj */
Object *get_obj_in_list_flagged(Flags flag, LList<Object *> &list) {
	Object * i = NULL;

	START_ITER(iter, i, Object *, list)
		if (OBJ_FLAGGED(i, flag))
			break;
	return i;
}


Object *get_obj_by_obj(const Object *obj, const char *name) {
	Object *i = NULL;
	int rm, j = 0;
	IDNum id;

	if (!str_cmp(name, "self") || !str_cmp(name, "me"))
		return const_cast<Object *>(obj);

	if ((i = get_obj_in_list(name, const_cast<Object *>(obj)->contents)))
		return i;

	if (obj->InObj()) {
		if (*name == UID_CHAR) {
			id = atoi(name + 1);
			if (id == GET_ID(obj->InObj()))
				return obj->InObj();
		} else if (silly_isname(name, obj->InObj()->Keywords("")))
			return obj->InObj();
	} else if (obj->WornBy() && (i = get_object_in_equip(obj->WornBy(), name, obj->WornBy()->equipment, &j)))
		return i;
	else if (obj->CarriedBy() && (i = get_obj_in_list(name, obj->CarriedBy()->contents)))
		return i;
	else if (((rm = obj->AbsoluteRoom()) != NOWHERE) && (i = get_obj_in_list(name, world[rm].contents)))
		return i;

	if (*name == UID_CHAR) {
		id = atoi(name + 1);
		START_ITER(iter, i, Object *, Objects)
			if (id == GET_ID(i))
				break;
	} else {
		START_ITER(iter, i, Object *, Objects)
			if (silly_isname(name, i->Keywords("")))
				break;
	}
	return i;
}


Object *get_obj_by_room(const Room *room, const char *name) {
	Object *obj = NULL;
	IDNum id;

	if (*name == UID_CHAR) {
		id = atoi(name + 1);
		START_ITER(room_iter, obj, Object *, room->contents)
			if (id == GET_ID(obj))
				break;

		if (!obj) {
			START_ITER(obj_iter, obj, Object *, Objects)
				if (id == GET_ID(obj))
					break;
		}
	} else {
		START_ITER(room_iter, obj, Object *, room->contents)
			if (silly_isname(name, obj->Keywords("")))
				break;

		if (!obj) {
			START_ITER(obj_iter, obj, Object *, Objects)
				if (silly_isname(name, obj->Keywords("")))
					break;
		}
	}
	return obj;
}


/************************************************************
 * search by number routines
 ************************************************************/

/* search all over the world for a char num, and return a pointer if found */
Character *get_char_num(VNum nr) {
	Character *i = NULL;

	START_ITER(iter, i, Character *, Characters)
		if (i->Virtual() == nr)
			break;

	return i;
}


/* search the entire world for an object number, and return a pointer  */
Object *get_obj_num(VNum nr) {
	Object *i = NULL;

	START_ITER(iter, i, Object *, Objects)
		if (i->Virtual() == nr)
			break;

	return i;
}


/* Search a given list for an object number, and return a ptr to that obj */
Object *get_obj_in_list_num(VNum num, LList<Object *> &list) {
	Object *i = NULL;

	START_ITER(iter, i, Object *, list)
		if (i->Virtual() == num)
			break;

	return i;
}


Character *find_char(IDNum n) {
	Character *ch = NULL;

	START_ITER(iter, ch, Character *, Characters)
		if (n == GET_ID(ch))
			break;

	return ch;
}


Object *find_obj(IDNum n) {
	Object *i;

	START_ITER(iter, i, Object *, Objects)
		if (n == GET_ID(i))
			break;

	return i;
}


/* return room with UID n */
Room *find_room(IDNum n) {
	if (n > ROOM_ID_BASE)	return NULL;
    n -= ROOM_ID_BASE;

    if (world.Find(n))	return &world[n];

    return NULL;
}


/* Generic Find, designed to find any object/character                    */
/* Calling :                                                              */
/*  *arg     is the sting containing the string to be searched for.       */
/*           This string doesn't have to be a single word, the routine    */
/*           extracts the next word itself.                               */
/*  bitv..   All those bits that you want to "search through".            */
/*           Bit found will be result of the function                     */
/*  *ch      This is the person that is trying to "find"                  */
/*  **tar_ch Will be NULL if no character was found, otherwise points     */
/* **tar_obj Will be NULL if no object was found, otherwise points        */
/*                                                                        */
/* The routine returns a pointer to the next word in *arg (just like the  */
/* one_argument routine).                                                 */

int generic_find(const char *arg, int bitvector, const Character * ch,
		     Character ** tar_ch, Object ** tar_obj) {
	int i, found;
	char *name = get_buffer(256);

	*tar_ch = NULL;
	*tar_obj = NULL;

	one_argument(arg, name);

	if (!*name) {
		release_buffer(name);
		return (0);
	}

	if (IS_SET(bitvector, FIND_CHAR_ROOM)) {	/* Find person in room */
		if ((*tar_ch = get_char_room_vis(ch, name))) {
			release_buffer(name);
			return (FIND_CHAR_ROOM);
		}
	}
	if (IS_SET(bitvector, FIND_CHAR_WORLD)) {
		if ((*tar_ch = get_char_vis(ch, name, FIND_CHAR_WORLD))) {
			release_buffer(name);
			return (FIND_CHAR_WORLD);
		}
	}
	if (IS_SET(bitvector, FIND_OBJ_EQUIP)) {
		for (found = FALSE, i = 0; i < NUM_WEARS && !found; i++)
			if (GET_EQ(ch, i) && isname(name, GET_EQ(ch, i)->CalledKeywords(ch))) {
				*tar_obj = GET_EQ(ch, i);
				found = TRUE;
			}
		if (found) {
			release_buffer(name);
			return (FIND_OBJ_EQUIP);
		}
	}
	if (IS_SET(bitvector, FIND_OBJ_INV)) {
		if ((*tar_obj = get_obj_in_list_vis(ch, name, ((Character *) ch)->contents))) {
			release_buffer(name);
			return (FIND_OBJ_INV);
		}
	}
	if (IS_SET(bitvector, FIND_OBJ_ROOM)) {
		if ((*tar_obj = get_obj_in_list_vis(ch, name, world[IN_ROOM(ch)].contents))) {
			release_buffer(name);
			return (FIND_OBJ_ROOM);
		}
	}
	if (IS_SET(bitvector, FIND_OBJ_WORLD)) {
		if ((*tar_obj = get_obj_vis(ch, name))) {
			release_buffer(name);
			return (FIND_OBJ_WORLD);
		}
	}
	release_buffer(name);
	return (0);
}


/* a function to scan for "all" or "all.x" */
int find_all_dots(char *arg) {
	if (!strcmp(arg, "all"))
		return FIND_ALL;
	else if (!strncmp(arg, "all.", 4)) {
		strcpy(arg, arg + 4);
		return FIND_ALLDOT;
	} else
		return FIND_INDIV;
}



int	get_num_chars_on_obj(const Object * obj) {
	Character * ch;
	int temp = 0;

	START_ITER(iter, ch, Character *, world[IN_ROOM(obj)].people) {
		if (obj == ch->SittingOn()) {
			if (GET_POS(ch) <= POS_SITTING)		temp++;
			else								ch->SetMount(NULL);
		}
	}

	return temp;
}


Character *get_char_on_obj(const Object *obj) {
	Character * ch;
	START_ITER(iter, ch, Character *, world[IN_ROOM(obj)].people)
		if(obj == ch->SittingOn())
			break;

	return ch;
}


int count_mobs_in_room(VNum num, VNum room) {
	Character * current;
	int counter;

	counter = 0;

	if (!world.Find(room))	return 0;
	START_ITER(iter, current, Character *, world[room].people)
		if (IS_NPC(current) && current->Virtual() == num)
			counter++;

	return counter;
}


int count_mobs_in_zone(VNum num, int zone) {
	Character * current;
	int counter;

	counter = 0;

	if (zone < 0)	return 0;

	START_ITER(iter, current, Character *, Characters)
		if (IS_NPC(current) && current->Virtual() == num && (IN_ROOM(current) != -1))
			if (world[IN_ROOM(current)].zone == zone)
				counter++;

	return counter;
}


/* Find a vehicle by VNUM */
Object *find_vehicle_by_vnum(VNum vnum) {
	Object * i = NULL;

	START_ITER(iter, i, Object *, Objects)
		if (i->Virtual() == vnum)
			break;

	return i;
}


VNum find_the_room(const char *roomstr) {
	int tmp;
	VNum location;
	Character *target_mob;
	Object *target_obj;

	if (isdigit(*roomstr) && !strchr(roomstr, '.')) {
		tmp = atoi(roomstr);
		location = Room::Find(tmp) ? tmp : NOWHERE;
	} else if ((target_mob = get_char(roomstr)))
		location = IN_ROOM(target_mob);
	else if ((target_obj = get_obj(roomstr)))
		location = IN_ROOM(target_obj);
	else
		return NOWHERE;
	return location;
}
