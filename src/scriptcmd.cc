#include "structs.h"
#include "scripts.h"
#include "utils.h"
#include "rooms.h"
#include "characters.h"
#include "objects.h"
#include "comm.h"
#include "interpreter.h"
#include "find.h"
#include "handler.h"
#include "buffer.h"
#include "db.h"
#include "olc.h"
#include "event.h"
#include "track.h"
#include "opinion.h"
#include "constants.h"
#include "skills.h"
#include "combat.h"


#define SCMD_SEND		0
#define SCMD_ECHOAROUND	1


long asciiflag_conv(char *flag);
void AlterHit(Character *ch, SInt32 amount);
extern int remove_trigger(Script *sc, char *name);
extern SInt32 exp_from_level(SInt32 chlevel, SInt32 fromlevel, SInt32 modifier);
extern void mag_affects(int level, Character *ch, Character *victim, int spellnum, int concentrate = 0);

extern const struct ScriptCommand scriptCommands[];

#define script_log(msg)	script_error(go, msg, trigger)


void script_error(Scriptable * go, const char *msg, VNum trigger);
bool script_command_interpreter(Scriptable * go, char *argument, int cmd, VNum trigger);

int get_zone_rnum(int zone);

void add_var(LList<TrigVarData *> &var_list, const char *name, const char *value, SInt32 id);

void load_mtrigger(Character *ch);
void load_otrigger(Object *obj);

extern struct zone_data *zone_table;


SCMD(func_asound);
SCMD(func_echo);
SCMD(func_send);
SCMD(func_zoneecho);
SCMD(func_teleport);
SCMD(func_door);
SCMD(func_force);
SCMD(func_gold);
SCMD(func_kill);		//	Mob only
SCMD(func_junk);		//	Mob only
SCMD(func_load);
SCMD(func_purge);
SCMD(func_at);
SCMD(func_damage);
SCMD(func_heal);
SCMD(func_remove);
SCMD(func_goto);
SCMD(func_info);
SCMD(func_copy);
SCMD(func_timer);		//	Obj only
SCMD(func_hunt);		//	Mob only
SCMD(func_transform);	//	Mob/Obj only
SCMD(func_pause);	//	Mob only


void script_error(Scriptable * go, const char *msg, VNum trigger) {
	switch (go->DataType()) {
	case Datatypes::Character:
		mudlogf(NRM, NULL, FALSE, "SCRIPTERR: Mob \"%s\" [%d]; Trigger %d: %s",
				static_cast<Character *>(go)->RealName(), go->Virtual(), trigger, msg);
		break;
	case Datatypes::Object:
		mudlogf(NRM, NULL, FALSE, "SCRIPTERR: Obj \"%s\" [%d]; Trigger %d: %s",
				static_cast<Object *>(go)->Name(), go->Virtual(), trigger, msg);
		break;
	case Datatypes::Room:
		mudlogf(NRM, NULL, FALSE, "SCRIPTERR: Wld Room %d; Trigger %d: %s", go->Virtual(), trigger, msg);
		break;
	default:
		mudlogf(NRM, NULL, FALSE, "SCRIPTERR: Unknown (Ptr %p); Trigger %d: %s", go, trigger, msg);
		break;
	}
}


// Locates a room by number or by thing's lookup routines.
void find_script_target_room(Scriptable * thing, char *arg,
					 Character ** c, Object ** o, Room ** r)
{
	int tmp;
	SInt16 location;

	*c = NULL;
	*o = NULL;
	*r = NULL;

	if (!*arg)
		return;

	if (isdigit(*arg) && !strchr(arg, '.')) {
		tmp = atoi(arg);
		if (!Room::Find(tmp))
			return;
		*r = &world[tmp];
	}
	else if ((*c = thing->TargetChar(arg))) {
		if (PURGED(*c) || ((*c)->InRoom() == NOWHERE))
			c = NULL;
	}
	else if ((*o = thing->TargetObj(arg))) {
		if (PURGED(*o) || ((*o)->InRoom() == NOWHERE))
			o = NULL;
	}
}


SCMD(func_asound) {
	skip_spaces(argument);

	if (!*argument) {
		script_log("asound called with no argument");
		return;
	}

    go->EchoDistance(argument, (GameData *) go);
}


SCMD(func_echo) {
	skip_spaces(argument);

	if (!*argument) {
		script_log("echo called with no args");
		return;
	}

	if (room->people.Count()) {
//		act(argument, TRUE, world[room].people.Top(), 0, 0, TO_ROOM | TO_CHAR);
		go->Echo(argument);
	} 
//	else
//		script_log("echo called from invalid room");
}


SCMD(func_affect) {
	char				*arg1, *s, *t;

	Character			*targch = NULL;
	Object				*targobj = NULL;
	Room				*targroom = NULL;
	
	int					spellnum = -1,
						location = 0,
						modifier = 0,
						duration = 0;
	Flags				flags = 0;

	skip_spaces(argument);

	if (!*argument) {
		script_log("affect called with no args");
		return;
	}

	/* get: blank, spell name, target name */
	s = strtok(argument - 1, "'");
	if (s == NULL)		return;
	s = strtok(NULL, "'");
	if (s == NULL)		return;
	t = strtok(NULL, "\0");

	if (isdigit(*s)) {
		spellnum = atoi(s);
	} else {
		spellnum = Skill::Number(s);
		if (spellnum == -1) {
			script_log("affect:  skill not found.\r\n");
			return;
		}
	}

	if (*t) {
		arg1 = get_buffer(MAX_INPUT_LENGTH);
	
		one_argument(t, arg1);
	
		if (!str_cmp(arg1, "all")) {
			LListIterator<Character *>	iter(room->people);
			while ((targch = iter.Next())) {
				mag_affects(100, targch, targch, spellnum);
			}
		} else if ((targch = go->TargetChar(arg1))) {
			mag_affects(100, targch, targch, spellnum);
		} else {
			script_log("affect: no target found");
		}
		release_buffer(arg1);
	}
}


SCMD(func_unaffect) {
	char				*arg1, *s, *t;

	Character			*targch = NULL;
	Object				*targobj = NULL;
	Room				*targroom = NULL;
	
	int					spellnum = -1,
						location = 0,
						modifier = 0,
						duration = 0;
	Flags				flags = 0;
						
						
	
	skip_spaces(argument);

	if (!*argument) {
		script_log("affect called with no args");
		return;
	}

	/* get: blank, spell name, target name */
	s = strtok(argument - 1, "'");
	if (s == NULL)		return;
	s = strtok(NULL, "'");
	if (s == NULL)		return;
	t = strtok(NULL, "\0");

	if (isdigit(*t)) {
		spellnum = atoi(t);
	} else {
		spellnum = Skill::Number(t);
		if (spellnum == -1) {
			script_log("unaffect:  skill not found.\r\n");
			return;
		}
	}

	skip_spaces(t);
	
	if (*t) {
		arg1 = get_buffer(MAX_INPUT_LENGTH);
	
		one_argument(t, arg1);
	
		if (!str_cmp(arg1, "all")) {
			LListIterator<Character *>	iter(room->people);
			while ((targch = iter.Next())) {
				if (!Affect::AffectedBy(targch, spellnum))	continue;
				Affect::FromThing(targch, spellnum);
			}
		} else if ((targch = go->TargetChar(arg1))) {
			if (Affect::AffectedBy(targch, spellnum)) {
				Affect::FromThing(targch, spellnum);
			}
		} else {
			script_log("unaffect: no target found");
		}
		release_buffer(arg1);
	}
}


SCMD(func_send) {
	char *buf = get_buffer(MAX_INPUT_LENGTH);
	Character *ch;

	argument = any_one_arg(argument, buf);

	if (!*buf)
		script_log("send called with no args");
	else {
		skip_spaces(argument);

		if (!*argument)
			script_log("send called without a message");
		else {
			ch = go->TargetCharRoom(buf);

			if (ch)	act(argument, TRUE, ch, 0, go, (subcmd == SCMD_SEND) ? TO_CHAR : TO_ROOM);
			else	script_log((subcmd == SCMD_SEND) ? "no target found for send" : "no target found for echoaround");
		}
	}
	release_buffer(buf);
}


SCMD(func_zoneecho) {
	int zone;
	char	*zone_name = get_buffer(MAX_INPUT_LENGTH),
			*buf = get_buffer(MAX_INPUT_LENGTH);

	argument = any_one_arg(argument, zone_name);
	skip_spaces(argument);

	if (!*zone_name || !*argument)
		script_log("zoneecho called with too few args");
	else if ((zone = get_zone_rnum(atoi(zone_name))) < 0)
		script_log("zoneecho called for nonexistant zone");
	else {
		sprintf(buf, "%s\r\n", argument);
		send_to_zone(buf, zone);
//		act(buf, TRUE, 0, 0, 0, TO_ZONE);
	}
	release_buffer(zone_name);
	release_buffer(buf);
}


SCMD(func_teleport) {
	Character *ch;
	Room	*location;
	VNum	target;
	char	*arg1 = get_buffer(MAX_INPUT_LENGTH),
			*arg2 = get_buffer(MAX_INPUT_LENGTH);

	two_arguments(argument, arg1, arg2);

	if (!*arg1 || !*arg2)
		script_log("teleport called with too few args");
	else if ((location = go->TargetRoom(arg2))) {
		target = location->Virtual();
	} else {
		script_log("teleport target is an invalid room");
		return;
	}

	if (!str_cmp(arg1, "all")) {
		VNum	room = go->AbsoluteRoom();

		if (room == NOWHERE)		script_log("teleport called from NOWHERE");
		else if (target == room)	script_log("teleport target is itself");
		else {
			LListIterator<Character *>	iter(world[room].people);
			while ((ch = iter.Next())) {
				ch->FromRoom();
				ch->ToRoom(target);
			}
		}
	} else if ((ch = go->TargetChar(arg1))) {
		ch->FromRoom();
		ch->ToRoom(target);
	} else
		script_log("teleport: no target found");
	release_buffer(arg1);
	release_buffer(arg2);
}


SCMD(func_door) {
	char *value;
	Room *rm;
	RoomDirection *exit;
	int dir, fd, to_room;
	char 	*target = get_buffer(MAX_INPUT_LENGTH),
			*direction = get_buffer(MAX_INPUT_LENGTH),
			*field = get_buffer(MAX_INPUT_LENGTH);

	const char *door_field[] = {
		"purge",
		"description",
		"flags",
		"key",
		"name",
		"room",
		"\n"
	};



	argument = two_arguments(argument, target, direction);
	value = one_argument(argument, field);
	skip_spaces(value);

	if (!*target || !*direction || !*field)
		script_log("door called with too few args");
	else if ((rm = get_room(target)) == NULL)
		script_log("door: invalid target");
	else if ((dir = search_block(direction, (const char **)dirs, FALSE)) == -1)
		script_log("door: invalid direction");
	else if ((fd = search_block(field, door_field, FALSE)) == -1)
		script_log("door: invalid field");
	else {
		exit = rm->dir_option[dir];

		/* purge exit */
		if (fd == 0) {
			if (exit) {
				delete exit;
				rm->dir_option[dir] = NULL;
			}
		} else {
			if (!exit) {
				exit = new RoomDirection;
				rm->dir_option[dir] = exit;
			}

			switch (fd) {
				case 1:  /* description */
					if (exit->general_description)  free(exit->general_description);
					free(exit->general_description);
					CREATE(exit->general_description, char, strlen(value) + 3);
					strcpy(exit->general_description, value);
					strcat(exit->general_description, "\r\n");
					break;
				case 2:  /* flags       */
					exit->exit_info = asciiflag_conv(value);
					break;
				case 3:  /* key         */
					exit->key = atoi(value);
					break;
				case 4:  /* name        */
					if (exit->keyword)				free(exit->keyword);
					free(exit->keyword);
					CREATE(exit->keyword, char, strlen(value) + 3);
					strcpy(exit->keyword, value);
					strcat(exit->keyword, "\r\n");
					break;
				case 5:  /* room        */
					if (Room::Find(to_room = atoi(value)))	exit->to_room = to_room;
					else									script_log("door: invalid door target");
					break;
			}
		}
	}
	release_buffer(target);
	release_buffer(direction);
	release_buffer(field);
}


SCMD(func_force) {
	Character *ch = NULL;
	Object *obj = NULL;
	char *line, *arg1;
	Flags afk_flag;

	arg1 = get_buffer(MAX_INPUT_LENGTH);
	line = one_argument(argument, arg1);

	if (!*arg1 || !*line) {
		script_log("force called with too few args");
		release_buffer(arg1);
		return;
	}

	switch (*arg1) {
	case 'a':
		if (!str_cmp(arg1, "all")) {
			LListIterator<Character *>	citer(room->people);
			while ((ch = citer.Next())) {
				afk_flag = PLR_FLAGS(ch);
				REMOVE_BIT(PLR_FLAGS(ch), PLR_AFK);
				if (!IS_STAFF(ch))	ch->Command(line);
				PLR_FLAGS(ch) = afk_flag;
			}
			LListIterator<Object *>	oiter(room->contents);
			while ((obj = oiter.Next())) {
				obj->Command(line);
			}
			release_buffer(arg1);
			return;
		}
	case 'm':
		if (!str_cmp(arg1, "mobs")) {
			LListIterator<Character *>	citer(room->people);
			while ((ch = citer.Next())) {
				afk_flag = PLR_FLAGS(ch);
				REMOVE_BIT(PLR_FLAGS(ch), PLR_AFK);
				if (!IS_STAFF(ch))	ch->Command(line);
				PLR_FLAGS(ch) = afk_flag;
			}
			release_buffer(arg1);
			return;
		}
		break;
	case 'o':
		if (!str_cmp(arg1, "objs")) {
			LListIterator<Object *>	oiter(room->contents);
			while ((obj = oiter.Next())) {
				obj->Command(line);
			}
			release_buffer(arg1);
			return;
		}
		break;
	case 'r':
		if (!str_cmp(arg1, "room")) {
			room->Command(line);
			release_buffer(arg1);
			return;
		}
		break;
	}

	ch = go->TargetCharRoom(arg1);
	if (ch) {
		if (!IS_STAFF(ch)) {
			afk_flag = PLR_FLAGS(ch);
			REMOVE_BIT(PLR_FLAGS(ch), PLR_AFK);
			ch->Command(line);
			PLR_FLAGS(ch) = afk_flag;
		}
	}
	else {
		obj = go->TargetObjList(arg1, room->contents);
		if (obj)	obj->Command(line);
	}

	if (!ch && !obj)	script_log("force: no target found");

	release_buffer(arg1);
}


//	Reward mission points
//	NEEDS MODIFICATION: subtract
SCMD(func_gold) {
	// Replace w/ equivalent from Phase 2
	Character *ch;
	char *name = get_buffer(MAX_INPUT_LENGTH),
		*amount = get_buffer(MAX_INPUT_LENGTH);

	two_arguments(argument, name, amount);

	if (!*name || !*amount)
		script_log("gold: too few arguments");
	else if ((ch = go->TargetChar(name)))
		GET_GOLD(ch) += atoi(amount);
	else
		script_log("gold: target not found");

	release_buffer(name);
	release_buffer(amount);
}


SCMD(func_kill) {
	char *		arg;
	Character *	victim;
	Character *	ch;

	if (go->DataType() != Datatypes::Character)
		return;

	ch = static_cast<Character *>(go);

	arg = get_buffer(MAX_INPUT_LENGTH);

	one_argument(argument, arg);

	if (!*arg)
//		script_log("kill called with no argument");
		;
	else if (!(victim = get_char_vis(ch, arg, FIND_CHAR_ROOM)))
//	else if (!(victim = get_char_vis(ch, arg, FIND_CHAR_ROOM)))
//		script_log("kill: victim not in room");
		;
	else if (victim == ch)
		script_log("kill: victim is self");
	else if (AFF_FLAGGED(ch, AffectBit::Charm) && ch->master == victim )
		script_log("kill: charmed mob attacking master");
//	else if (FIGHTING(ch))
//		script_log("kill: already fighting");
	else
		ch->Hit(victim);

	release_buffer(arg);
}


SCMD(func_junk) {
	SInt32		pos, dotmode;
	Object *	obj;

	if (go->DataType() != Datatypes::Character)
		return;

	Character *ch = static_cast<Character *>(go);

	char *arg = get_buffer(MAX_INPUT_LENGTH);
	one_argument(argument, arg);

	if (!*arg)	script_log("junk called with no argument");
	dotmode = find_all_dots(arg);

	if (dotmode == FIND_ALL) {
		START_ITER(iter, obj, Object *, ch->contents) {
			if (obj->CanBeSeen(ch))	obj->Extract();
		}
	} else if (dotmode == FIND_ALLDOT) {
		if (!*arg)	script_log("junk called with invalid all argument");
		else {
			START_ITER(iter, obj, Object *, ch->contents) {
				if (obj->CanBeSeen(ch) && isname(arg, obj->Keywords()))
					obj->Extract();
			}
		}
	} else if ((obj = get_obj_in_list_vis(ch, arg, ch->contents))) {
		obj->Extract();
	} else if ((obj = get_object_in_equip_vis(ch, arg, ch->equipment, &pos))) {
		unequip_char(ch, obj->worn_on);
		obj->Extract();
	}

	release_buffer(arg);
}


SCMD(func_load) {
	VNum 		number = -1, roomnum = room->Virtual();
	Character 	*mob, *target = NULL;
	Object		*object;
	char		*arg1 = get_buffer(MAX_INPUT_LENGTH),
				*arg2 = get_buffer(MAX_INPUT_LENGTH),
				*p = NULL;

	p = two_arguments(argument, arg1, arg2);

	if (!*arg1 || !*arg2 || !is_number(arg2) || ((number = atoi(arg2)) < 0))
		script_log("load: bad syntax");
	else if (is_abbrev(arg1, "mob")) {
		if ((mob = Character::Read(number))) {
			mob->ToRoom(roomnum);
			load_mtrigger(mob);
		} else
			script_log("load: bad mob vnum");
	} else if (is_abbrev(arg1, "obj")) {
		if ((object = Object::Read(number))) {
			skip_spaces(p);
			if (*p) {
				if (isdigit(*p)) {
					VNum tmp = atoi(p);
					if (Room::Find(tmp))
						roomnum = tmp;
				} else {
					target = go->TargetChar(p);
				}
			} else {
				if (CAN_WEAR(object, ITEM_WEAR_TAKE) && (go->DataType() == Datatypes::Character)) {
					target = static_cast<Character *>(go);
				} else {
					roomnum = room->Virtual();
				}
			}

			if (target) 	object->ToChar(target);
			else 			object->ToRoom(roomnum);

			load_otrigger(object);
		} else
			script_log("load: bad object vnum");
	} else
		script_log("load: bad type");

	release_buffer(arg1);
	release_buffer(arg2);
}


/* purge all objects an npcs in room, or specified object or mob */
SCMD(func_purge) {
	char *		arg = get_buffer(MAX_INPUT_LENGTH);
	Character *	ch;
	Object *	obj;

	one_argument(argument, arg);

	if (!*arg) {
		VNum	room = go->AbsoluteRoom();

		LListIterator<Character *>	people(world[room].people);
		LListIterator<Object *>		contents(world[room].contents);

		while ((ch = people.Next())) {
			if (IS_NPC(ch) && (ch != go))
				ch->Extract();
		}

		while ((obj = contents.Next())) {
			if (obj != go)
				obj->Extract();
		}
	} else if ((ch = go->TargetChar(arg))) {
		if (!IS_NPC(ch))	script_log("purge: purging a PC");
		else				ch->Extract();
	} else if ((obj = go->TargetObj(arg)))
		obj->Extract();
	else
		script_log("purge: bad argument");
	release_buffer(arg);
}


SCMD(func_at) {
	char *arg = get_buffer(MAX_INPUT_LENGTH), *p;
	Character *c;
	Object *o;
	Room *r;
	VNum target = NOWHERE;

	p = one_argument(argument, arg);

	if (!*arg) {
		script_log("at: bad syntax");
	} else {
		find_script_target_room(go, arg, &c, &o, &r);
		if (c)			target = c->InRoom();
		else if (o)		target = o->InRoom();
		else if (r)		target = r->Virtual();

		if (target == NOWHERE) {
			script_log("at: target not found");
		} else if (ROOM_FLAGGED(target, ROOM_GODROOM) ||	// a room has been found.	Check for permission
				ROOM_FLAGGED(target, ROOM_HOUSE) ||
				ROOM_FLAGGED(target, ROOM_PRIVATE)) {
			script_log("at target is a private room");
		} else {
			go->AtCommand(target, p);
		}
	}

	release_buffer(arg);
}


SCMD(func_damage) {
	Character *victim;
	Attack	attack;
	Property::Damage	*property;

	char *	name = get_buffer(MAX_INPUT_LENGTH);
	char *	amount = get_buffer(MAX_INPUT_LENGTH);
	char *	message;
	
	message = two_arguments(argument, name, amount);

	// sprintf(buf, "damage:  Trying to damage using %s", message);
	// script_log(buf);

	skip_spaces(message);

	if (*message) {
		attack.message = Skill::Number(message);
		if (attack.message != -1) {
			property = PROPERTY(attack.message, Damage);
			if (property) {
				attack.damtype = property->damagetype;
				if (property->hitslocation) {
					attack.location = find_hit_location(victim, 0);
				}
			}
		}
	}

	if (!*name || !*amount)
		script_log("damage: too few arguments");
	else if (!is_number(amount))
		script_log("damage: bad syntax");
	else if (!str_cmp(name, "all")) {
		VNum	room = go->AbsoluteRoom();

		attack.dam = atoi(amount);

		if ((room != NOWHERE) && !ROOM_FLAGGED(room, ROOM_PEACEFUL)) {
			LListIterator<Character *>	iter(world[room].people);
			while ((victim = iter.Next())) {
				if (victim == static_cast<Character *>(go))
					continue;
				if (PURGED(victim))
					continue;
				if (NO_STAFF_HASSLE(victim))
					victim->Send("Being the cool staff member you are, you escape unharmed.");
				else {
					victim->Damage(((go->DataType() == Datatypes::Character) ? static_cast<Character *>(go) : NULL), &attack);
				}
			}
		}
	} else if ((victim = go->TargetChar(name)) && (!PURGED(victim))) {
		attack.dam = atoi(amount);

		if (NO_STAFF_HASSLE(victim))
			victim->Send("Being the cool staff member you are, you escape unharmed.");
		else {
			attack.message = Skill::Number(message);
			victim->Damage(((go->DataType() == Datatypes::Character) ? static_cast<Character *>(go) : NULL), &attack);
		}
	} else
		script_log("damage: target not found");
	release_buffer(name);
	release_buffer(amount);
}


SCMD(func_skillset)
{
	Character *vict;
	int skill, value, theory = 0, i, qend, add;
	char help[MAX_INPUT_LENGTH];

	argument = one_argument(argument, buf);

	if (!*buf) {			/* no arguments. print an informative text */
		script_log("skillset: no arguments");
		return;
	}

	if (!(vict = go->TargetCharRoom(buf))) {
		script_log("skillset: target not found");
		return;
	}

	skip_spaces(argument);

	/* If there is no chars in argument */
	if (!*argument) {
		script_log("skillset: no skill specified");
		return;
	}
	if (*argument != '\'') {
		script_log("skillset: skill must be enclosed in ''");
		return;
	}
	/* Locate the last quote && lowercase the magic words (if any) */

	for (qend = 1; *(argument + qend) && (*(argument + qend) != '\''); qend++)
		*(argument + qend) = LOWER(*(argument + qend));

	if (*(argument + qend) != '\'') {
		script_log("skillset: skill must be enclosed in ''");
		return;
	}
	strcpy(help, (argument + 1));
	help[qend - 1] = '\0';
	if ((skill = Skill::Number(help, 1, LAST_SKILL)) <= 0) {
		script_log("skillset: unrecognized skill");
		return;
	}
	argument += qend + 1;		/* skip to next parameter */
	argument = one_argument(argument, buf);

	if (!*buf) {
		script_log("skillset: no points value");
		return;
	}

	value = atoi(buf);
	value = MAX(MIN(value, 250), 0);
	if (*argument) {
		theory = atoi(argument);
		theory = MAX(MIN(value, 250), 0);
	}

	add = CHAR_SKILL(vict).SetSkill(skill, NULL, value, theory);
	if (!SKL_IS_CSPHERE(skill))
		GET_TOTALPTS(vict) += add;

}


SCMD(func_setchar)
{
	int field, setto, setdiff;
	char fieldarg[MAX_INPUT_LENGTH];
	Character *ch;
	Attack	attack;

	const char *wset_table[] =
	{
		"hit",
		"maxhit",
		"mana",
		"maxmana",
		"move",
		"maxmove",
		"alignment",
		"lawfulness",
		"gold",
		"var",
		"\n"
	};

	half_chop(argument, buf1, argument);

	ch = go->TargetCharRoom(buf1);

	if (!ch || IS_IMMORTAL(ch) || PURGED(ch) || (IN_ROOM(ch) == NOWHERE))
		return;

	half_chop(argument, fieldarg, argument);

	field = search_block(fieldarg, wset_table, TRUE);

	// Didn't find the argument?
	if (field == -1) {
		script_log("set:  Invalid parameter field");
		return;
	}
	half_chop(argument, buf1, argument);
	setto = atoi(buf1);

	switch (field) {
	case 0: // hit
		setto = MAX(setto, -10);
		setdiff = GET_HIT(ch) - setto;
		attack.dam = setdiff;
		ch->Damage(((go->DataType() == Datatypes::Character) ? static_cast<Character *>(go) : NULL), &attack);
		break;
	case 1: // maxhit
		setto = MAX(setto, -10);
		setdiff = GET_HIT(ch) - setto;
		attack.dam = setdiff;
		ch->Damage(((go->DataType() == Datatypes::Character) ? static_cast<Character *>(go) : NULL), &attack);
		GET_MAX_HIT(ch) = setto;
		break;
	case 2: // mana
		setto = MAX(setto, 0);
		setdiff = GET_MANA(ch) - setto;
		AlterMana(ch, setdiff);
		break;
	case 3: // maxmana
		setto = MAX(setto, 0);
		setdiff = GET_MANA(ch) - setto;
		AlterMana(ch, setdiff);
		GET_MAX_MANA(ch) = setto;
		break;
	case 4: // move
		setto = MAX(setto, 0);
		setdiff = GET_MOVE(ch) - setto;
		AlterMove(ch, setdiff);
		break;
	case 5: // maxmove
		setto = MAX(setto, 0);
		setdiff = GET_MOVE(ch) - setto;
		AlterMove(ch, setdiff);
		GET_MAX_MOVE(ch) = setto;
		break;
	case 6: // alignment
		setto = MAX(MIN(setto, 1000), -1000);
		GET_ALIGNMENT(ch) = setto;
		break;
	case 7: // lawfulness
		setto = MAX(MIN(setto, 1000), -1000);
		GET_LAWFULNESS(ch) = setto;
		break;
	case 8: // gold
		GET_GOLD(ch) = setto;
		break;
	case 9: // var
		if (!SCRIPT(go))
			SCRIPT(go) = new Script(go->DataType());
		half_chop(argument, buf2, argument);
		add_var(SCRIPT(go)->variables, buf1, buf2, 0);
		break;
	default:
		sprintf(buf, "set:	Invalid field (%s) in switch", fieldarg);
		script_log(buf);
		return;
	}
}


SCMD(func_heal) {
	Character *victim;

	char *	name = get_buffer(MAX_INPUT_LENGTH);
	char *	amount = get_buffer(MAX_INPUT_LENGTH);

	two_arguments(argument, name, amount);

	if (!*name || !*amount)
		script_log("heal: too few arguments");
	else if (!is_number(amount))
		script_log("heal: bad syntax");
	else if (!str_cmp(name, "all")) {
		VNum	room = go->AbsoluteRoom();
		START_ITER(iter, victim, Character *, world[room].people) {
			if (NO_STAFF_HASSLE(victim))
				victim->Send("Being the cool staff member you are, you don't need to be healed!");
			else
				AlterHit(victim, -atoi(amount));
		}
	} else if ((victim = go->TargetChar(name))) {
		if (NO_STAFF_HASSLE(victim))
			victim->Send("Being the cool staff member you are, you don't need to be healed!");
		else
			AlterHit(victim, -atoi(amount));
	} else
		script_log("heal: target not found");

	release_buffer(name);
	release_buffer(amount);
}


SCMD(func_remove) {
	Character *victim;
	Object *the_obj;
	int		x;
	bool	fAll = false;
	VNum	vnum = 0;
	char *vict = get_buffer(MAX_INPUT_LENGTH),
		 *what = get_buffer(MAX_INPUT_LENGTH);

	two_arguments(argument, vict, what);

	if (!*vict || !*what || (!is_number(what) && str_cmp(what, "all")))
		script_log("remove: bad syntax");
	else if ((victim = go->TargetChar(vict))) {
		if (!str_cmp(what, "all"))	fAll = true;
		else 						vnum = atoi(what);
		START_ITER(iter, the_obj, Object *, victim->contents)
			if (fAll ? !NO_LOSE(the_obj) : (the_obj->Virtual() == vnum))
				the_obj->Extract();
		for (x = 0; x < NUM_WEARS; x++) {
			the_obj = GET_EQ(victim, x);
			if (the_obj && (fAll ? !NO_LOSE(the_obj) : (the_obj->Virtual() == vnum)))
				the_obj->Extract();
		}
	}
	release_buffer(vict);
	release_buffer(what);
}


/* lets the mobile goto any location it wishes that is not private */
SCMD(func_goto) {
	char *arg;
	Room *location = NULL;

	if (go->DataType() != Datatypes::Character)
		return;

	arg = get_buffer(MAX_INPUT_LENGTH);

	one_argument(argument, arg);

	if (!*arg)
		script_log("goto called with no argument");
	else if (!(location = go->TargetRoom(arg)))
		script_log("goto: invalid location");
	else {
		Character *	ch = static_cast<Character *>(go);
		if (FIGHTING(ch))
			ch->StopFighting();

		if (ch->InRoom() != NOWHERE)
			ch->FromRoom();
		ch->ToRoom(location->Virtual());
	}
	release_buffer(arg);
}


SCMD(func_info) {
	ACMD(do_gen_comm);
	char *buf;

	skip_spaces(argument);

	if (!*argument)
		script_log("info called with no argument");
//	else if (type == MOB_TRIGGER)	do_gen_comm((Character *)go, argument, 0, "broadcast", SCMD_BROADCAST);
	else {
		buf = get_buffer(MAX_STRING_LENGTH);
		sprintf(buf, "&cY[&cBINFO&cY]&c0 %s", argument);
		Combat::Announce(buf);
		release_buffer(buf);
	}
}


SCMD(func_copy) {
	char *arg1, *arg2;
	int 		number = 0;
	Character *	mob;
	Object *	obj;

	arg1 = get_buffer(MAX_INPUT_LENGTH);
	arg2 = get_buffer(MAX_INPUT_LENGTH);

	two_arguments(argument, arg1, arg2);

	if (!*arg1 || !*arg2)
		script_log("copy: bad syntax");
	else if (is_abbrev(arg1, "mob")) {
		if (!(mob = go->TargetChar(arg2)))
			script_log("copy: mob not found");
		else if (!(mob = new Character(*mob)))
			script_log("copy: error in \"new Character(mob)\"");
		else
			mob->ToRoom(room->Virtual());
	} else if (is_abbrev(arg1, "obj")) {
		if (!(obj = go->TargetObj(arg2)))
			script_log("copy: mob not found");
		else if (!(obj = new Object(*obj)))
			script_log("copy: error in \"new Object(obj)\"");
		else if (CAN_WEAR(obj, ITEM_WEAR_TAKE) && (go->DataType() == Datatypes::Character))
			obj->ToChar(static_cast<Character *>(go));
		else
			obj->ToRoom(room->Virtual());
	} else
		script_log("copy: bad type");
	release_buffer(arg1);
	release_buffer(arg2);
}


/* increases the target's exp */
SCMD(func_exp)
{
	Character *ch;
	char name[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH];

	two_arguments(argument, name, amount);

	if (!*name || !*amount) {
		script_log("exp: too few arguments");
		return;
	}
	if ((ch = go->TargetCharRoom(name)))
		ch->GainExp(exp_from_level(GET_LEVEL(ch), atoi(amount), 0));
	else {
		script_log("exp: target not found");
		return;
	}
}


SCMD(func_timer) {
	char *		arg;
	SInt32		time;

	if (go->DataType() != Datatypes::Object)
		return;

	arg = get_buffer(MAX_INPUT_LENGTH);
	one_argument(argument, arg);

	if (!*arg)							script_log("timer: missing argument");
	else if (!is_number(arg))			script_log("timer: argument is not a number");
	else if ((time = atoi(arg)) <= 0)	script_log("timer: argument is <= 0");
	else								GET_OBJ_TIMER(static_cast<Object *>(go)) = time;

	release_buffer(arg);
}


SCMD(func_hunt) {
	Character *	victim;
	Character *	ch;
	char *		arg;

	if (go->DataType() != Datatypes::Character)
		return;

	ch = static_cast<Character *>(go);

	arg = get_buffer(MAX_INPUT_LENGTH);
	one_argument(argument, arg);

	if (!*arg)
		script_log("hunt: Missing argument");
	else if (!(victim = get_char_vis(ch, arg, FIND_CHAR_WORLD)))
		script_log("hunt: Victim not found");
	// else if (!(ch->path = Path::ToChar(IN_ROOM(ch), victim, 200, Path::Global | Path::ThruDoors)))
	//	script_log("hunt: Unable to build path");
	else {
	    SET_BIT(MOB_FLAGS(ch), MOB_SEEKER); // Allows the mob to pass !MOB rooms
		HUNTING(ch) = GET_ID(victim);
	}

	release_buffer(arg);
}


SCMD(func_seek) {
	Character *	victim;
	Character *	ch;
	char *		arg;

	if (go->DataType() != Datatypes::Character)
		return;

	ch = static_cast<Character *>(go);

	arg = get_buffer(MAX_INPUT_LENGTH);
	one_argument(argument, arg);

	if (!*arg)
		script_log("seek: Missing argument");
	else if (!(victim = get_char_vis(ch, arg, FIND_CHAR_WORLD)))
		script_log("seek: Victim not found");
	// else if (!(ch->path = Path::ToChar(IN_ROOM(ch), victim, 200, Path::Global | Path::ThruDoors)))
	// 	script_log("seek: Unable to build path");
	else {
	    SET_BIT(MOB_FLAGS(ch), MOB_SEEKER); // Allows the mob to pass !MOB rooms
		SEEKING(ch) = GET_ID(victim);
	}

	release_buffer(arg);
}


SCMD(func_roomseek) {
	Character *	ch;
	VNum		target;
	char *		arg;

	if (go->DataType() != Datatypes::Character)
		return;

	ch = static_cast<Character *>(go);

	arg = get_buffer(MAX_INPUT_LENGTH);
	one_argument(argument, arg);

	if (!*arg)
		script_log("roomseek: Missing argument");

	target = atoi(arg);

	if (!(Room::Find(target)))
		script_log("roomseek: Target not found");
	// else if (!(ch->path = Path::ToRoom(IN_ROOM(ch), target, 200, Path::Global | Path::ThruDoors)))
	// 	script_log("roomseek: Unable to build path");
	else {
	    SET_BIT(MOB_FLAGS(ch), MOB_SEEKER); // Allows the mob to pass !MOB rooms
		ROOMSEEKING(ch) = target;
	}

	release_buffer(arg);
}


SCMD(func_transform) {
	char *		arg;
	int			i;
	ExtraDesc *	ed;

	arg = get_buffer(MAX_INPUT_LENGTH);

	one_argument(argument, arg);

	if (!*arg)						script_log("transform: missing argument");
	else if (!is_number(arg))		script_log("transform: bad argument");
	else if (go->DataType() == Datatypes::Character) {
		Character *	newMob, *ch = static_cast<Character *>(go);
		Object *	equipList[NUM_WEARS];
		Object *	tmp;
		LList<Object *>	tmpList;

		if (!(newMob = Character::Read(atoi(arg))))
			script_log("transform: invalid mob vnum");
		else {
			//	Unequip, etc.
			for (SInt32 pos = 0; pos < NUM_WEARS; pos++)
				equipList[pos] = GET_EQ(ch, pos) ? unequip_char(ch, pos) : NULL;
			while ((tmp = ch->contents.Top())) {
				ch->contents.Remove(tmp);
				tmpList.Add(tmp);
			}

			ch->Virtual(newMob->Virtual());

			//	Copy data...
			//	General data first
			//	Copy data...
			//	General data first
			ch->SetKeywords((SString *) newMob->SSKeywords());
			ch->SetName((SString *) newMob->SSName());
			ch->SetSDesc((SString *) newMob->SSSDesc());
			ch->SetLDesc((SString *) newMob->SSLDesc());

			GET_SEX(ch) = GET_SEX(newMob);
			GET_RACE(ch) = GET_RACE(newMob);
			GET_LEVEL(ch) = GET_LEVEL(newMob);
			GET_WEIGHT(ch) = GET_WEIGHT(newMob);
			GET_HEIGHT(ch) = GET_HEIGHT(newMob);
			ch->general.act = newMob->general.act;
			SET_BIT(AFF_FLAGS(ch), AFF_FLAGS(newMob));
			ch->general.misc.clannum = newMob->general.misc.clannum;

			//	Mob Data
			ch->mob->attack_type = newMob->mob->attack_type;
			GET_DEFAULT_POS(ch) = GET_DEFAULT_POS(newMob);
			ch->mob->damage = newMob->mob->damage;
			if (ch->mob->hates) {
				delete ch->mob->hates;
				if (newMob->mob->hates)
					ch->mob->hates = new Opinion(*newMob->mob->hates);
			}
			if (ch->mob->fears) {
				delete ch->mob->fears;
				if (newMob->mob->fears)
					ch->mob->fears = new Opinion(*newMob->mob->fears);
			}

			//	Points
			ch->real_abils = newMob->real_abils;
			GET_MAX_HIT(ch) = GET_MAX_HIT(newMob);
			GET_MAX_MOVE(ch) = GET_MAX_MOVE(newMob);
			GET_HITROLL(ch) = GET_HITROLL(newMob);
			GET_DAMROLL(ch) = GET_DAMROLL(newMob);
			GET_DR(ch) = GET_DR(newMob);
			ch->AffectTotal();


			//	Reequip
			for (SInt32 pos = 0; pos < NUM_WEARS; pos++) {
				if (equipList[pos])
					equip_char(ch, equipList[pos], pos);
			}
			while ((tmp = tmpList.Top())) {
				tmpList.Remove(tmp);
				ch->contents.Add(tmp);
			}

			newMob->ToRoom(2);
			newMob->Extract();
		}
	} else if (go->DataType() == Datatypes::Object) {
		Object *	newObj, *obj = static_cast<Object *>(go);

		if (!(newObj = Object::Read(atoi(arg))))
			script_log("transform: invalid obj vnum");
		else {
			obj->SetKeywords((SString *) newObj->SSKeywords());
			obj->SetName((SString *) newObj->SSName());
			obj->SetSDesc((SString *) newObj->SSSDesc());

			obj->Virtual(newObj->Virtual());

			for (i = 0; i < NUM_OBJ_VALUES; ++i) {
				GET_OBJ_VAL(obj, i) = GET_OBJ_VAL(newObj, i);
			}

			for (i = 0; i < MAX_OBJ_AFFECT; i++)
				obj->affectmod[i] = newObj->affectmod[i];
	
			LListIterator<ExtraDesc *>	descIter(obj->exDesc);
			while ((ed = descIter.Next())) {
				ed->Free();
			}
			
			obj->exDesc.Clear();

			// Duplicate extra descriptions
			descIter.Restart(newObj->exDesc);
			while ((ed = descIter.Next())) {
				obj->exDesc.Append(ed->Share());
			}
			
			newObj->Extract();

		}
	}
	release_buffer(arg);
}


class CommandEvent : public ListedEvent {
public:
						CommandEvent(time_t when, Scriptable *g, const char *str, VNum trg = -2) :
						ListedEvent(when, &(g->events)), go(g), command(str), trigger(trg) {}
					
	Scriptable				*go;
	string					command;
	VNum					trigger;

	time_t					Execute(void);
};


time_t	CommandEvent::Execute(void)
{
	int cmd;
	char *line;

	line = (char *) command.c_str();
	
	// We're not going to use -1 for the triggers, as scripted spells might use that number. -DH
	if (trigger != -2) {
		for (cmd = 0; *scriptCommands[cmd].command != '\n'; ++cmd)
			if (!strcmp(scriptCommands[cmd].command, line))
				break;
		if (*scriptCommands[cmd].command != '\n') {
			script_command_interpreter(go, line, cmd, trigger);
			return 0;
		}
	}

	go->Command(line);
	return 0;
}

SCMD(func_delay)
{
// CHANGEPOINT:  This breaks if cancelled prematurely.

	char 			arg1[20];
	unsigned long 	delay;

	// if (go->DataType() != Datatypes::Character)
	//	return;

	*arg1 = '\0';

	half_chop(argument, arg1, buf);
	delay = atoi(arg1);

	if (delay <= 0) {
		sprintf(buf3, "delay - the delay is invalid. - vnum #%d", go->Virtual());
		script_log(buf3);
		return;
	}

	go->events.push_back(new CommandEvent(delay RL_SEC / 5, go, buf, trigger));
}


void send_signal_room(Scriptable *go, char *signal, char *p)
{
	Character	*c = NULL;
	Object		*o = NULL;
	Room		*r = &(world[go->InRoom()]);

	LListIterator<Character *>	c_iter(r->people);
	while ((c = c_iter.Next())) {
		if (!SCRIPT_CHECK(c, MTRIG_SIGNAL))	continue;
		signal_trigger(go, c, signal, p);
	}

	LListIterator<Object *>	o_iter(r->contents);
	while ((o = o_iter.Next())) {
		if (!SCRIPT_CHECK(o, OTRIG_SIGNAL))	continue;
		signal_trigger(go, o, signal, p);
	}

	if ((go->DataType() != Datatypes::Room || go != r) && (!SCRIPT_CHECK(r, WTRIG_SIGNAL))) {
		signal_trigger(go, r, signal, p);
	}
}


SCMD(func_signal)
{
	char		*arg = get_buffer(MAX_INPUT_LENGTH), 
				*signal = NULL,
				*where = NULL,
				*p = NULL;
	Character	*c = NULL;
	Object		*o = NULL;
	Room		*r = NULL;
	Scriptable	*targ = NULL;

	if (!*argument) {
		script_log("signal: no arguments given");
	} else {
		where = get_buffer(MAX_INPUT_LENGTH);
			p = one_argument(argument, where);

		if (!where || !p) {
			script_log("signal: invalid arguments");
			release_buffer(where);
			release_buffer(arg);
			return;
		}

				signal = get_buffer(MAX_INPUT_LENGTH);
				p = one_argument(p, signal);
				
		switch (*where) {
		case 'r':
			send_signal_room(go, signal, p);
			break;
		case 'z':
//			zoneSignals.Append(new ZoneSignal(world[go->AbsoluteRoom()].zone, signal, p));
			break;
		case 's':
			signal_trigger(go, go, signal, p);
			break;
		case 'g':
			if (go->DataType() == Datatypes::Character) {
				Character *k, *follower;
				c = static_cast<Character *>(go);

				k = c->master ? c->master : c;
				
				signal_trigger(go, k, signal, p);
				START_ITER(iter, follower, Character *, k->followers) {
					signal_trigger(go, follower, signal, p);
				}
			}
			break;
		default:
			find_script_target_room(go, where, &c, &o, &r);
			if (c)			{	targ = c;	}
			else if (o)		{	targ = o;	}
			else if (r)		{	targ = r;	}

			if (!targ)		script_log("signal: target not found");
			else			signal_trigger(go, targ, signal, p);
			break;
			}
		}

		release_buffer(arg);
		release_buffer(where);
		release_buffer(signal);
}
		

SCMD(func_pause)
{
	extern long pulse;
	extern struct TimeInfoData time_info;

	Character *victim;

	char *	name = get_buffer(MAX_INPUT_LENGTH);
	char *	amount;
	SInt32	time, hr, min, ntime;
	char c = '\0';

	amount = one_argument(argument, name);
	skip_spaces(amount);

	if (!*name || !*amount)
		script_log("pause: too few arguments");

	if (!strn_cmp(amount, "until ", 6)) {
		/* valid forms of time are 14:30 and 1430 */
		if (sscanf(amount, " until %d:%d", &hr, &min) == 2)
			min += (hr * 60);
		else
			min = (hr % 100) + ((hr / 100) * 60);

		/* calculate the pulse of the day of "until" time */
		ntime = (min * SECS_PER_MUD_HOUR * PASSES_PER_SEC) / 60;

		/* calculate pulse of day of current time */
		time = (pulse % (SECS_PER_MUD_HOUR * PASSES_PER_SEC)) +
				(time_info.hours * SECS_PER_MUD_HOUR * PASSES_PER_SEC);

		/* adjust for next day */
		if (time >= ntime)	time = (SECS_PER_MUD_DAY * PASSES_PER_SEC) - time + ntime;
		else				time = ntime - time;
	} else if (sscanf(amount, " %d %c", &time, &c) == 2) {
		if (c == 'd')		time *= 24 * SECS_PER_MUD_HOUR * PASSES_PER_SEC;
		else if (c == 'h')	time *= SECS_PER_REAL_HOUR * PASSES_PER_SEC;
		else if (c == 't')	time *= SECS_PER_MUD_HOUR * PASSES_PER_SEC;	//	't': ticks
		else if (c == 's')	time *= PASSES_PER_SEC;
		else if (c == 'p')	;											//	'p': pulses
		else 				time *= PASSES_PER_SEC;						//	Default: seconds
	} else					time *= PASSES_PER_SEC / 5;

	if (!str_cmp(name, "all")) {
		VNum	room = go->AbsoluteRoom();
		START_ITER(iter, victim, Character *, world[room].people) {
			if (NO_STAFF_HASSLE(victim))
				victim->Send("Being the cool staff member you are, you don't need to be paused!");
			else
				WAIT_STATE(victim, time);
	}
	} else if ((victim = go->TargetChar(name))) {
		if (NO_STAFF_HASSLE(victim))
			victim->Send("Being the cool staff member you are, you don't need to be paused!");
		else
			WAIT_STATE(victim, time);
	} else
		script_log("pause: target not found");
	
	release_buffer(name);
	release_buffer(amount);
}


/*
enum {
	Mob		= (1 << 0),
	Obj		= (1 << 1),
	Room	= (1 << 2),
	All		= Mob | Obj | Room
};

struct ScriptCommand {
	char *	command;
	SCMD(*command_pointer);
//	void	(*command_pointer)	(Scriptable *go, char *argument, int subcmd);
	SInt32	type;
	SInt32	subcmd;
};
*/

const struct ScriptCommand scriptCommands[] = {
//	{ "RESERVED", 0, 0, 0 },/* this must be first -- for specprocs */

	{ "asound"		, func_asound	, ScriptCommand::All, 0 },
	{ "at"			, func_at		, ScriptCommand::All, 0 },
	{ "affect"		, func_affect	, ScriptCommand::All, 0 },
//	{ "attach"		, func_attach	, ScriptCommand::All, 0 },
	{ "copy"		, func_copy		, ScriptCommand::All, 0 },
	{ "damage"		, func_damage	, ScriptCommand::All, 0 },
//	{ "detach"		, func_detach	, ScriptCommand::All, 0 },
	{ "delay"		, func_delay	, ScriptCommand::Mob, 0 },  // Oh, the cruft.  I need a generic delayed.
	{ "door"		, func_door		, ScriptCommand::All, 0 },
	{ "echo"		, func_echo		, ScriptCommand::All, 0 },
	{ "echoaround"	, func_send		, ScriptCommand::All, SCMD_ECHOAROUND },
	{ "exp"			, func_exp		, ScriptCommand::All, 0 },
	{ "force"		, func_force	, ScriptCommand::All, 0 },
	{ "goto"		, func_goto		, ScriptCommand::All, 0 },
	{ "heal"		, func_heal		, ScriptCommand::All, 0 },
	{ "hunt"		, func_hunt     , ScriptCommand::Mob, 0 },
	{ "info"		, func_info		, ScriptCommand::All, 0 },
	{ "junk"		, func_junk		, ScriptCommand::Mob, 0 },
	{ "kill"		, func_kill		, ScriptCommand::Mob, 0 },
	{ "load"		, func_load		, ScriptCommand::All, 0 },
	{ "purge"		, func_purge	, ScriptCommand::All, 0 },
	{ "pause"		, func_pause	, ScriptCommand::All, 0 },
//	{ "remove"		, func_remove	, ScriptCommand::All, 0 },
	{ "roomseek"	, func_roomseek , ScriptCommand::Mob, 0 },
	{ "send"		, func_send		, ScriptCommand::All, SCMD_SEND },
	{ "seek"		, func_seek     , ScriptCommand::Mob, 0 },
	{ "setchar"		, func_setchar	, ScriptCommand::All, 0 },
	{ "skillset"	, func_skillset	, ScriptCommand::All, 0 },
	{ "signal"		, func_signal	, ScriptCommand::All, 0 },
	{ "teleport"	, func_teleport	, ScriptCommand::All, 0 },
	{ "timer"		, func_timer	, ScriptCommand::Obj, 0 },
	{ "transform"	, func_transform, ScriptCommand::Mob | ScriptCommand::Obj, 0	},
	{ "zoneecho"	, func_zoneecho	, ScriptCommand::All, 0 },

	{ "\n", 0, 0, 0 }	/* this must be last */
};


// This is the command interpreter called by script_driver.
// It returns true if a valid command is processed.
bool script_command_interpreter(Scriptable * go, char *argument, int cmd, VNum trigger) {
	int mode = cmd;
	VNum room;
	char *line, *arg;

	room = go->AbsoluteRoom();

	if (room <= NOWHERE) {
		sprintf(buf, "Command '%s' issued in NOWHERE!", argument);
		script_log(buf);
		return true;
	}

	skip_spaces(argument);

	/* just drop to next line for hitting CR */
	if (!*argument)
		return true;

	arg = get_buffer(MAX_INPUT_LENGTH);

	line = any_one_arg(argument, arg);

	if (cmd < 0) {
		/* find the command */
		for (cmd = 0; *scriptCommands[cmd].command != '\n'; cmd++)
			if (!strcmp(scriptCommands[cmd].command, arg))
				break;
	}

	if (*scriptCommands[cmd].command == '\n') {
		if (mode != -1) {
			sprintf(arg, "Unknown script cmd: '%s'", argument);
			script_log(arg);
		}
		release_buffer(arg);
		return false;
	} else
		((*scriptCommands[cmd].command_pointer) (go, line, scriptCommands[cmd].subcmd, trigger, &world[room]));

	release_buffer(arg);
	return true;
}


void Scriptable::Command(char * argument, int cmd = -1)
{
	script_command_interpreter(this, argument, cmd, -1);
}


void Character::AtCommand(VNum target, char * p)
{
	VNum original = IN_ROOM(this);
	Character *fighting = FIGHTING(this);

	this->FromRoom();
	this->ToRoom(target);

	Command(p);

	if (!PURGED(this)) {
		this->FromRoom();
		this->ToRoom(original);
		FIGHTING(this) = fighting;
	}
}


void Room::AtCommand(VNum target, char * p)
{
	if (Room::Find(target)) {
		world[target].Command(p);
	}
}

// Look out!	This is highly experimental!
void Object::AtCommand(VNum target, char * p)
{
	int original = InRoom();
	Object *insideobj = NULL;
	Character *wearing = NULL, *carrying = NULL;
	SInt16 wornwear = -1;

	if (target == original) {
		Command(p);
		return;
	}

	if (WornBy() != NULL) {
		wearing = (Character *) Inside();
		wornwear = worn_on;
		if (unequip_char(wearing, worn_on) != this)
			log("SYSERR: Inconsistent worn_by and worn_on pointers!!");
	}

	if (original != NOWHERE) {
		this->FromRoom();
	} else if (CarriedBy()) {
		carrying = (Character *) Inside();
		this->FromChar();
	} else if (InObj()) {
		insideobj = (Object *) Inside();
		this->FromObj();
	}

	this->ToRoom(target);

	Command(p);

	if (PURGED(this) || Inside())
		return;

	this->FromRoom();

	if (wearing) {
		equip_char(wearing, this, wornwear);
	} else if (original != NOWHERE) {
		this->ToRoom(original);
	} else if (carrying) {
		this->ToChar(carrying);
	} else if (insideobj) {
		this->ToObj(insideobj);
	}
}
