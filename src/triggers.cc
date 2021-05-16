/**************************************************************************
*  File: triggers.c                                                       *
*                                                                         *
*  Usage: contains all the trigger functions for scripts.                 *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Death's Gate MUD is based on CircleMUD, Copyright (C) 1993, 94.        *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author: sfn $
*  $Date: 2000/02/21 23:29:26 $
*  $Revision: 1.10 $
**************************************************************************/

#include "structs.h"
#include "scripts.h"
#include "utils.h"
#include "rooms.h"
#include "characters.h"
#include "objects.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "find.h"
#include "db.h"
#include "buffer.h"
#include "constants.h"


/* external functions from scripts.c */
void add_var(LList<TrigVarData *> &var_list, const char *name, const char *value, SInt32 id);
int script_driver(Scriptable * go, Trigger *trig, int mode);

void signal_trigger(Scriptable *go, Scriptable *targ, char *signal, char *argument);

int greet_mtrigger(Character *actor, int dir);
int entry_mtrigger(Character *ch, int dir);
int selfcommand_mtrigger(Character *ch, char *cmd, char *argument);
int command_mtrigger(Character *actor, char *cmd, char *argument);
void speech_mtrigger(Character *actor, char *str);
void act_mtrigger(Character *ch, const char *str, const Character *actor, 
		const Character *victim, const MUDObject *object, const Object *target, const char *arg);
void fight_mtrigger(Character *ch);
void hitprcnt_mtrigger(Character *ch);
int receive_mtrigger(Character *ch, Character *actor, Object *obj);
int death_mtrigger(Character *ch, Character *actor);
int get_otrigger(Object *obj, Character *actor);
int cmd_otrig(Object *obj, Character *actor, char *cmd, char *argument, int type);
int greet_otrigger(Character *actor, int dir);
int command_otrigger(Character *actor, char *cmd, char *argument);
int wear_otrigger(Object *obj, Character *actor, int where);
int drop_otrigger(Object *obj, Character *actor);
int give_otrigger(Object *obj, Character *actor, Character *victim);
int sit_otrigger(Object *obj, Character *actor);
int enter_wtrigger(Room *room, Character *actor, int dir);
int command_wtrigger(Character *actor, char *cmd, char *argument);
void speech_wtrigger(Character *actor, char *str);
int drop_wtrigger(Object *obj, Character *actor);
int consume_otrigger(Object *obj, Character *actor);

int remove_otrigger(Object *obj, Character *actor);

int leave_mtrigger(Character *actor, int dir);
int leave_otrigger(Character *actor, int dir);
int leave_wtrigger(Room *room, Character *actor, int dir);

int door_mtrigger(Character *actor, int subcmd, int dir);
int door_otrigger(Character *actor, int subcmd, int dir);
int door_wtrigger(Character *actor, int subcmd, int dir);

void load_mtrigger(Character *ch);
void load_otrigger(Object *obj);
void timer_otrigger(Object *obj);
void reset_wtrigger(Room *room);

int speech_otrig(Object *obj, Character *actor, char *str, int type);
void speech_otrigger(Character *actor, char *str);

void start_otrigger(Object *obj);
void start_otrigger(LList<Object *> &objList);
void quit_otrigger(Object *obj);
void quit_otrigger(LList<Object *> &objList);

int is_command_substring(char *sub, char *string);
int command_check(char *str, const char *wordlist, char **phraseptr);

int is_empty(int zone_nr);


LList<ZoneSignal *> zoneSignals;


/* mob trigger types */
const char *mtrig_types[] = {
	"Global",
	"Random",
	"Command",
	"Speech",
	"Act",
	"Death",
	"Greet",
	"Greet-All",
	"Entry",
	"Receive",
	"Fight",
	"HitPrcnt",
	"Bribe",
	"Leave",
	"Leave-All",
	"Door",
	"Load",
	"Memory",
	"Signal",
	"SelfCommand",
	"\n"
};


/* obj trigger types */
const char *otrig_types[] = {
	"Global",
	"Random",
	"Command",
	"Sit",
	"Leave",
	"Greet",
	"Get",
	"Drop",
	"Give",
	"Wear",
	"Door",
	"Speech",
	"Consume",
	"Load",
	"Timer",
	"Remove",
	"Start",
	"Quit",
	"Signal",
	"\n"
};


/* wld trigger types */
const char *wtrig_types[] = {
	"Global",
	"Random",
	"Command",
	"Speech",
	"Door",
	"Leave",
	"Enter",
	"Drop",
	"Reset",
	"UNUSED",
	"UNUSED",
	"UNUSED",
	"UNUSED",
	"UNUSED",
	"UNUSED",
	"UNUSED",
	"UNUSED",
	"UNUSED",
	"Signal",
	"\n"
};



const char *door_act[] = {
	"open",
	"close",
	"unlock",
	"lock",
	"pick",
	"\n"
};

	

/*
 *  mob triggers
 */

void random_mtrigger(Character *ch) {
	Trigger *t;
	bool executed = false;

	if (!SCRIPT_CHECK(ch, MTRIG_RANDOM) || AFF_FLAGGED(ch, AffectBit::Charm))
		return;

	START_ITER(iter, t, Trigger *, TRIGGERS(SCRIPT(ch))) {
		if (TRIGGER_CHECK_MOB(t, MTRIG_RANDOM) && ((GET_TRIG_NARG(t) > 100) || 
			(!executed && (Number(1, 100) <= GET_TRIG_NARG(t))))) {
			if (TRIGGER_CHECK_MOB(t, MTRIG_GLOBAL) || !is_empty(world[IN_ROOM(ch)].zone)) {
				script_driver(ch, t, TRIG_NEW);
				executed = true;
			}
		}
	}
}


int bribe_mtrigger(Character *ch, Character *actor, int amount) {
	Trigger *t;
  
	if (!SCRIPT_CHECK(ch, MTRIG_BRIBE) || AFF_FLAGGED(ch, AffectBit::Charm))
		return 1;

 	LListIterator<Trigger *>	triggers(TRIGGERS(SCRIPT(ch)));
	while ((t = triggers.Next())) {
		if (TRIGGER_CHECK_MOB(t, MTRIG_BRIBE)) {
			ADD_UID_VAR(t, actor, "actor", 0);
			sprintf(buf, "%d", amount);
			add_var(t->variables, "amount", buf, 0);
			return script_driver(ch, t, TRIG_NEW);
		}
	}

	return 1;
}


int greet_mtrigger(Character *actor, int dir) {
	Trigger *t;
	Character *ch;
 	SInt32	intermediate;
 	bool		final = true;
 	
 	LListIterator<Character *>	people(world[IN_ROOM(actor)].people);
 	LListIterator<Trigger *>	triggers;

	while ((ch = people.Next())) {
		if (!SCRIPT_CHECK(ch, MTRIG_GREET | MTRIG_GREET_ALL) || !AWAKE(ch) ||
				FIGHTING(ch) || (ch == actor) || AFF_FLAGGED(ch, AffectBit::Charm))
			continue;

		triggers.Restart(TRIGGERS(SCRIPT(ch)));
		while ((t = triggers.Next())) {
			if (((IS_SET(GET_TRIG_TYPE_MOB(t), MTRIG_GREET) && actor->CanBeSeen(ch)) ||
					IS_SET(GET_TRIG_TYPE_MOB(t), MTRIG_GREET_ALL)) && 
					!GET_TRIG_DEPTH(t) && (Number(1, 100) <= GET_TRIG_NARG(t))) {
				add_var(t->variables, "direction", dirs[rev_dir[dir]], 0);
				ADD_UID_VAR(t, actor, "actor", 0);
				
				intermediate = script_driver(ch, t, TRIG_NEW);
				if (!intermediate)	final = false;
			}
		}
	}
	return final;
}


int entry_mtrigger(Character *ch, int dir) {
	Trigger *t;

	if (!SCRIPT_CHECK(ch, MTRIG_ENTRY) || AFF_FLAGGED(ch, AffectBit::Charm))
		return 1;

 	LListIterator<Trigger *>	triggers(TRIGGERS(SCRIPT(ch)));
 	while ((t = triggers.Next())) {
		if (TRIGGER_CHECK_MOB(t, MTRIG_ENTRY) && (Number(1, 100) <= GET_TRIG_NARG(t))){
			add_var(t->variables, "direction", dirs[dir], 0);
			return script_driver(ch, t, TRIG_NEW);
		}
	}
	return 1;
}


int is_command_substring(char *sub, char *string)
{
	char *s;
	int cmd = -1;
	extern int find_command_abbrev(char *command);
	
	half_chop(string, buf3, buf2);
	
	cmd = find_command_abbrev(buf3);

	if (cmd > -1) {
		strcpy(buf3, complete_cmd_info[cmd].command);

		if ((s = (char *) str_str(buf3, sub))) {
		int len = strlen(buf3);
		int sublen = strlen(sub);

		/* check front */
		if ((s == buf3 || isspace(*(s - 1)) || ispunct(*(s - 1))) &&

			/* check end */
			((s + sublen == buf3 + len) || isspace(s[sublen]) ||
			 ispunct(s[sublen])))
			return 1;
		}
	} else {
		if (is_abbrev(buf3, sub))
			return 1;
	}

	return 0;
}


/*
 * return 1 if str contains a word or phrase from wordlist.
 * phrases are in double quotes (").
 * if wrdlist is NULL, then return 1, if str is NULL, return 0.
 */
int command_check(char *str, const char *wordlist, char **phraseptr)
{
    static char words[MAX_INPUT_LENGTH], phrase[MAX_INPUT_LENGTH];
	char *s;

    *phraseptr = NULL;

    if (*wordlist=='*') return 1;

    strcpy(words, wordlist);
    
    for (s = one_phrase(words, phrase); *phrase; s = one_phrase(s, phrase))
		if (is_command_substring(phrase, str)) {
            strcpy(buf3, phrase);
            *phraseptr = buf3;
		    return 1;
        }

    return 0;
}


int command_mtrigger(Character *actor, char *cmd, char *argument) {
	Character *		ch;
	Trigger *		t;
	const char *	arg;
	char *phrase = NULL;

 	LListIterator<Character *>	people(world[IN_ROOM(actor)].people);
 	LListIterator<Trigger *>	triggers;
 	
 	while ((ch = people.Next())) {
		if (SCRIPT_CHECK(ch, MTRIG_COMMAND) && !AFF_FLAGGED(ch, AffectBit::Charm) && (ch != actor)) {
			triggers.Restart(TRIGGERS(SCRIPT(ch)));
			while ((t = triggers.Next())) {
				if (!TRIGGER_CHECK_MOB(t, MTRIG_COMMAND))
					continue;
				if (!(arg = SSData(GET_TRIG_ARG(t))))
					continue;
		        if (command_check(cmd, SSData(GET_TRIG_ARG(t)), &phrase)) {
				// if (*arg == '*' || !strn_cmp(arg, cmd, strlen(arg))) {
					ADD_UID_VAR(t, actor, "actor", 0);
					skip_spaces(argument);
					add_var(t->variables, "arg", argument, 0);
					add_var(t->variables, "cmd", phrase ? phrase : cmd, 0);
					if (script_driver(ch, t, TRIG_NEW))
						return 1;
				}
			}
		}
	}
	return 0;
}


int selfcommand_mtrigger(Character *ch, char *cmd, char *argument) {
	Trigger *		t;
	const char *	arg;
	char *phrase = NULL;

 	LListIterator<Trigger *>	triggers;
 	
	if (SCRIPT_CHECK(ch, MTRIG_SELFCOMMAND)) {
		triggers.Restart(TRIGGERS(SCRIPT(ch)));
		while ((t = triggers.Next())) {
			if (!TRIGGER_CHECK_MOB(t, MTRIG_SELFCOMMAND))
				continue;
			if (!(arg = SSData(GET_TRIG_ARG(t))))
				continue;
	        if (command_check(cmd, SSData(GET_TRIG_ARG(t)), &phrase)) {
			// if (*arg == '*' || !strn_cmp(arg, cmd, strlen(arg))) {
				skip_spaces(argument);
				add_var(t->variables, "arg", argument, 0);
				add_var(t->variables, "cmd", phrase ? phrase : cmd, 0);
				if (script_driver(ch, t, TRIG_NEW))
					return 1;
			}
		}
	}
	return 0;
}


void signal_trigger(Scriptable *go, Scriptable *targ, char *signal, char *argument) 
{
	Trigger *		t;
	const char *	arg;
	char *phrase = NULL;

 	LListIterator<Trigger *>	triggers;

	if (SCRIPT_CHECK(targ, TRIG_SIGNAL)) {
		triggers.Restart(TRIGGERS(SCRIPT(targ)));
		while ((t = triggers.Next())) {

			if (!(arg = SSData(GET_TRIG_ARG(t))))
				continue;

			switch (targ->DataType()) {
			case Datatypes::Character:	if (!TRIGGER_CHECK_MOB(t, TRIG_SIGNAL))		continue;	break;
			case Datatypes::Room:		if (!TRIGGER_CHECK_WLD(t, TRIG_SIGNAL))		continue;	break;
			case Datatypes::Object:		if (!TRIGGER_CHECK_OBJ(t, TRIG_SIGNAL))		continue;	break;
			default:																return;
			}

			if (*arg == '*' || (GET_TRIG_NARG(t) ?
						word_check(signal, arg) : is_substring(arg, signal))) {
				if (go)
					ADD_UID_VAR(t, go, "actor", 0);
				skip_spaces(argument);
				add_var(t->variables, "arg", argument, 0);
				add_var(t->variables, "signal", phrase ? phrase : signal, 0);
				if (script_driver(targ, t, TRIG_NEW))
					return;
			}
		}
	}
}


void speech_mtrigger(Character *actor, char *str) {
	Character *ch;
	Trigger *t;
	
	LListIterator<Character *>	people(world[IN_ROOM(actor)].people);
	while ((ch = people.Next())) {
		if (SCRIPT_CHECK(ch, MTRIG_SPEECH) && !AFF_FLAGGED(ch, AffectBit::Charm) &&
				AWAKE(ch) && (ch != actor)) {
			
			LListIterator<Trigger *>	triggers(TRIGGERS(SCRIPT(ch)));
			while ((t = triggers.Next())) {
				if (TRIGGER_CHECK_MOB(t, MTRIG_SPEECH) && (!SSData(GET_TRIG_ARG(t)) ||
						(GET_TRIG_NARG(t) ?
						word_check(str, SSData(GET_TRIG_ARG(t))) :
						is_substring(SSData(GET_TRIG_ARG(t)), str)))) {
					ADD_UID_VAR(t, actor, "actor", 0);
					add_var(t->variables, "speech", str, 0);
					script_driver(ch, t, TRIG_NEW);
					break;
				}
			}
		}
	}
}


void act_mtrigger(Character *ch, const char *str, const Character *actor, 
		const Character *victim, const MUDObject *object, const Object *target, const char *arg) {
	Trigger *t;

	if (SCRIPT_CHECK(ch, MTRIG_ACT) && !AFF_FLAGGED(ch, AffectBit::Charm)) {
//		for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next) 
		START_ITER(iter, t, Trigger *, TRIGGERS(SCRIPT(ch))) {
			if (!SSData(GET_TRIG_ARG(t))) {
				continue;
			}
//			if (TRIGGER_CHECK(t, MTRIG_ACT) && word_check(str, SSData(GET_TRIG_ARG(t)))) {
			if ((ch != actor) && TRIGGER_CHECK_MOB(t, MTRIG_ACT) && (GET_TRIG_NARG(t) ?
					word_check(str, SSData(GET_TRIG_ARG(t))) :
					is_substring(SSData(GET_TRIG_ARG(t)), str))) {

				if (actor)		ADD_UID_VAR(t, actor, "actor", 0);
				if (victim)		ADD_UID_VAR(t, victim, "victim", 0);
				if (object)		ADD_UID_VAR(t, object, "object", 0);
				if (target)		ADD_UID_VAR(t, target, "target", 0);
				if (arg) {
					skip_spaces(arg);
					add_var(t->variables, "arg", arg, 0);
				}
				script_driver(ch, t, TRIG_NEW);
				break;
			}
		}
	}
}


void fight_mtrigger(Character *ch) {
	Trigger *t;

	if (!SCRIPT_CHECK(ch, MTRIG_FIGHT) || !FIGHTING(ch) || AFF_FLAGGED(ch, AffectBit::Charm))
		return;

	START_ITER(iter, t, Trigger *, TRIGGERS(SCRIPT(ch))) {
		if (TRIGGER_CHECK_MOB(t, MTRIG_FIGHT) && (Number(1, 100) <= GET_TRIG_NARG(t))){
			ADD_UID_VAR(t, FIGHTING(ch), "actor", 0);
			script_driver(ch, t, TRIG_NEW);
			break;
		}
	}
}


void hitprcnt_mtrigger(Character *ch) {
	Trigger *t;
  
	if (!SCRIPT_CHECK(ch, MTRIG_HITPRCNT) || !FIGHTING(ch) || AFF_FLAGGED(ch, AffectBit::Charm))
		return;

	START_ITER(iter, t, Trigger *, TRIGGERS(SCRIPT(ch))) {
		if (TRIGGER_CHECK_MOB(t, MTRIG_HITPRCNT) && GET_MAX_HIT(ch) &&
				(((GET_HIT(ch) * 100) / GET_MAX_HIT(ch)) <= GET_TRIG_NARG(t))) {
			ADD_UID_VAR(t, FIGHTING(ch), "actor", 0);
			script_driver(ch, t, TRIG_NEW);
			break;
		}
	}
}


int receive_mtrigger(Character *ch, Character *actor, Object *obj) {
	Trigger *t;

	if (!SCRIPT_CHECK(ch, MTRIG_RECEIVE) || AFF_FLAGGED(ch, AffectBit::Charm))
		return 1;
	
	LListIterator<Trigger *>	triggers(TRIGGERS(SCRIPT(ch)));
	while ((t = triggers.Next())) {
		if (TRIGGER_CHECK_MOB(t, MTRIG_RECEIVE) && (Number(1, 100) <= GET_TRIG_NARG(t))){
			ADD_UID_VAR(t, actor, "actor", 0);
			ADD_UID_VAR(t, obj, "object", 0);
			return script_driver(ch, t, TRIG_NEW);
		}
	}
	
	return 1;
}


int death_mtrigger(Character *ch, Character *actor) {
	Trigger *t;
  
	if (!SCRIPT_CHECK(ch, MTRIG_DEATH) || AFF_FLAGGED(ch, AffectBit::Charm))
		return 1;
	
	LListIterator<Trigger *>	triggers(TRIGGERS(SCRIPT(ch)));
	while ((t = triggers.Next())) {
		if (TRIGGER_CHECK_MOB(t, MTRIG_DEATH)) {
			if (actor)	ADD_UID_VAR(t, actor, "actor", 0);
			return script_driver(ch, t, TRIG_NEW);
		}
	}
	
	return 1;
}


int leave_mtrigger(Character *actor, int dir) {
	Trigger *t;
	Character *ch;
	
	START_ITER(ch_iter, ch, Character *, world[IN_ROOM(actor)].people) {
		if (!SCRIPT_CHECK(ch, MTRIG_LEAVE | MTRIG_LEAVE_ALL) || !AWAKE(ch) ||
				FIGHTING(ch) || (ch == actor) || AFF_FLAGGED(ch, AffectBit::Charm))
			continue;

		START_ITER(trig_iter, t, Trigger *, TRIGGERS(SCRIPT(ch))) {
			if (((IS_SET(GET_TRIG_TYPE_MOB(t), MTRIG_LEAVE) && actor->CanBeSeen(ch)) ||
					IS_SET(GET_TRIG_TYPE_MOB(t), MTRIG_LEAVE_ALL)) &&
					!GET_TRIG_DEPTH(t) && (Number(1, 100) <= GET_TRIG_NARG(t))) {
				add_var(t->variables, "direction", dirs[dir], 0);
				ADD_UID_VAR(t, actor, "actor", 0);
				return script_driver(ch, t, TRIG_NEW);
			}
		}
	}
	return 1;
}


int door_mtrigger(Character *actor, int subcmd, int dir) {
	Trigger *t;
	Character *ch;
	
	START_ITER(ch_iter, ch, Character *, world[IN_ROOM(actor)].people) {
		if (!SCRIPT_CHECK(ch, MTRIG_DOOR) || !AWAKE(ch) || FIGHTING(ch) ||
				(ch == actor) || AFF_FLAGGED(ch, AffectBit::Charm))
			continue;

		START_ITER(trig_iter, t, Trigger *, TRIGGERS(SCRIPT(ch))) {
			if (TRIGGER_CHECK_MOB(t, MTRIG_DOOR) && actor->CanBeSeen(ch) &&
					(Number(1, 100) <= GET_TRIG_NARG(t))) {
				add_var(t->variables, "cmd", door_act[subcmd], 0);
				add_var(t->variables, "direction", dirs[dir], 0);
				ADD_UID_VAR(t, actor, "actor", 0);
				return script_driver(ch, t, TRIG_NEW);
			}
		}
	}
	
	return 1;
}


void load_mtrigger(Character *ch) {
	Trigger *t;
	bool executed = false;
	
	if (!SCRIPT_CHECK(ch, MTRIG_LOAD))
		return;
	
	LListIterator<Trigger *>	iter(TRIGGERS(SCRIPT(ch)));
	while ((t = iter.Next())) {
		if (TRIGGER_CHECK_MOB(t, MTRIG_LOAD) && ((GET_TRIG_NARG(t) > 100) || 
			(!executed && (Number(1, 100) <= GET_TRIG_NARG(t))))) {
			script_driver(ch, t, TRIG_NEW);
			executed = true;
		}
	}
}


/*
 *  object triggers
 */

void random_otrigger(Object *obj) {
	Trigger *t;
	VNum	room;
	bool executed = false;

	if (!SCRIPT_CHECK(obj, OTRIG_RANDOM))
		return;

	START_ITER(iter, t, Trigger *, TRIGGERS(SCRIPT(obj))) {
		if (TRIGGER_CHECK_OBJ(t, OTRIG_RANDOM) && ((GET_TRIG_NARG(t) > 100) || 
		   (!executed && (Number(1, 100) <= GET_TRIG_NARG(t))))) {
			if (((room = obj->AbsoluteRoom()) != NOWHERE) && (TRIGGER_CHECK_OBJ(t, OTRIG_GLOBAL) ||
					!is_empty(world[room].zone))) {
				script_driver(obj, t, TRIG_NEW);
				executed = true;
			}
		}
	}
}


int get_otrigger(Object *obj, Character *actor) {
	Trigger *t;
	
	if (!SCRIPT_CHECK(obj, OTRIG_GET))
		return 1;

	START_ITER(iter, t, Trigger *, TRIGGERS(SCRIPT(obj))) {
		if (TRIGGER_CHECK_OBJ(t, OTRIG_GET) && (Number(1, 100) <= GET_TRIG_NARG(t))) {
			ADD_UID_VAR(t, actor, "actor", 0);
			return script_driver(obj, t, TRIG_NEW);
		}
	}
	return 1;
}


/* checks for command trigger on specific object */
int cmd_otrig(Object *obj, Character *actor, char *cmd, char *argument, int type) {
	Trigger *t;
	char *phrase = NULL;
  
	if (obj && SCRIPT_CHECK(obj, OTRIG_COMMAND)) {
		LListIterator<Trigger *>	iter(TRIGGERS(SCRIPT(obj)));
		while ((t = iter.Next())) {
			const char *arg = SSData(GET_TRIG_ARG(t));
			if (!arg)
				continue;
			
			if (!TRIGGER_CHECK_OBJ(t, OTRIG_COMMAND) || !IS_SET(GET_TRIG_NARG(t), type))
				continue;
			 
			if (command_check(cmd, SSData(GET_TRIG_ARG(t)), &phrase)) {
			// if (*arg == '*' || !strn_cmp(arg, cmd, strlen(arg))) {
				ADD_UID_VAR(t, actor, "actor", 0);
				skip_spaces(argument);
				add_var(t->variables, "arg", argument, 0);
				add_var(t->variables, "cmd", phrase ? phrase : cmd, 0);
	
				if (script_driver(obj, t, TRIG_NEW))
					return 1;
			}
		}
	}

	return 0;
}


int command_otrigger(Character *actor, char *cmd, char *argument) {
	Object *obj;
	int i;

	for (i = 0; i < NUM_WEARS; i++)
		if (cmd_otrig(GET_EQ(actor, i), actor, cmd, argument, OCMD_EQUIP))
			return 1;
  
  	LListIterator<Object *>	ch_iter(actor->contents);
  	while ((obj = ch_iter.Next()))
		if (cmd_otrig(obj, actor, cmd, argument, OCMD_INVEN))
			return 1;
		
  	LListIterator<Object *>	rm_iter(world[IN_ROOM(actor)].contents);
  	while ((obj = rm_iter.Next()))
		if (cmd_otrig(obj, actor, cmd, argument, OCMD_ROOM))
			return 1;

	return 0;
}


/* checks for signal trigger on specific object */
int sig_otrig(Object *obj, Character *actor, char *cmd, char *argument) {
	Trigger *t;
	char *phrase = NULL;
	LListIterator<Trigger *>	iter;
  
	if (obj && SCRIPT_CHECK(obj, OTRIG_SIGNAL)) {
		iter.Restart(TRIGGERS(SCRIPT(obj)));
		while ((t = iter.Next())) {
			const char *arg = SSData(GET_TRIG_ARG(t));
			if (!arg)
				continue;
			
			if (!TRIGGER_CHECK_OBJ(t, OTRIG_SIGNAL))
				continue;
			 
			if (command_check(cmd, SSData(GET_TRIG_ARG(t)), &phrase)) {
			// if (*arg == '*' || !strn_cmp(arg, cmd, strlen(arg))) {
				ADD_UID_VAR(t, actor, "actor", 0);
				skip_spaces(argument);
				add_var(t->variables, "arg", argument, 0);
				add_var(t->variables, "signal", phrase ? phrase : cmd, 0);
	
				if (script_driver(obj, t, TRIG_NEW) && GET_TRIG_NARG(t))
					return 1;
			}
		}
	}

	return 0;
}


/* checks for speech trigger on specific object  */
int speech_otrig(Object *obj, Character *actor, char *str, int type) {
	Trigger *t;

	if (obj && SCRIPT_CHECK(obj, OTRIG_SPEECH)) {
		START_ITER(iter, t, Trigger *, TRIGGERS(SCRIPT(obj))) {
			if (TRIGGER_CHECK_OBJ(t, OTRIG_SPEECH) && IS_SET(GET_TRIG_NARG(t), type) &&
					(!SSData(GET_TRIG_ARG(t)) ||
					(IS_SET(GET_TRIG_NARG(t), 8) ? word_check(str, SSData(GET_TRIG_ARG(t))) :
					is_substring(SSData(GET_TRIG_ARG(t)), str)))) {
				ADD_UID_VAR(t, actor, "actor", 0);
				add_var(t->variables, "speech", str, 0);
	
				if (script_driver(obj, t, TRIG_NEW))
					return 1;
			}
		}
	}
	return 0;
}


void speech_otrigger(Character *actor, char *str) {
	Object *obj;
	int i;

	for (i = 0; i < NUM_WEARS; i++)
		if (speech_otrig(GET_EQ(actor, i), actor, str, OCMD_EQUIP))
			return;
  
	START_ITER(ch_iter, obj, Object *, actor->contents)
		if (speech_otrig(obj, actor, str, OCMD_INVEN))
			return;

	START_ITER(obj_iter, obj, Object *, world[IN_ROOM(actor)].contents)
		if (speech_otrig(obj, actor, str, OCMD_ROOM))
			return;
}


int greet_otrigger(Character *actor, int dir) {
	Trigger *t;
	Object *obj;
	
	LListIterator<Object *>	contents(world[IN_ROOM(actor)].contents);
	LListIterator<Trigger *>	triggers;
	while ((obj = contents.Next())) {
		if (!SCRIPT_CHECK(obj, OTRIG_GREET))
			continue;
		
		triggers.Restart(TRIGGERS(SCRIPT(obj)));
		while ((t = triggers.Next())) {
			if (TRIGGER_CHECK_OBJ(t, OTRIG_GREET) && (Number(1, 100) <= GET_TRIG_NARG(t))) {
				add_var(t->variables, "direction", dirs[rev_dir[dir]], 0);
				ADD_UID_VAR(t, actor, "actor", 0);
				return script_driver(obj, t, TRIG_NEW);
			}
		}
	}
	
	return 1;
}


int wear_otrigger(Object *obj, Character *actor, int where) {
	Trigger *t;

	if (!SCRIPT_CHECK(obj, OTRIG_WEAR))
		return 1;

	START_ITER(iter, t, Trigger *, TRIGGERS(SCRIPT(obj))) {
		if (TRIGGER_CHECK_OBJ(t, OTRIG_WEAR)) {
			ADD_UID_VAR(t, actor, "actor", 0);
			return script_driver(obj, t, TRIG_NEW);
		}
	}
	return 1;
}


int remove_otrigger(Object *obj, Character *actor) {
	Trigger *t;

	if (!SCRIPT_CHECK(obj, OTRIG_WEAR))
		return 1;
	
	LListIterator<Trigger *>	iter(TRIGGERS(SCRIPT(obj)));
	while ((t = iter.Next())) {
		if (TRIGGER_CHECK_OBJ(t, OTRIG_WEAR)) {
			ADD_UID_VAR(t, actor, "actor", 0);
			return script_driver(obj, t, TRIG_NEW);
		}
	}
	return 1;
}


int consume_otrigger(Object *obj, Character *actor) {
	Trigger *t;

	if (!SCRIPT_CHECK(obj, OTRIG_CONSUME))
		return 1;

	LListIterator<Trigger *>	iter(TRIGGERS(SCRIPT(obj)));
	while ((t = iter.Next())) {
		if (TRIGGER_CHECK_OBJ(t, OTRIG_CONSUME) && (Number(1, 100) <= GET_TRIG_NARG(t))) {
			ADD_UID_VAR(t, actor, "actor", 0);
			return script_driver(obj, t, TRIG_NEW);
		}
	}
	return 1;
}


int drop_otrigger(Object *obj, Character *actor) {
	Trigger *t;

	if (!SCRIPT_CHECK(obj, OTRIG_DROP))
		return 1;

	LListIterator<Trigger *>	iter(TRIGGERS(SCRIPT(obj)));
	while ((t = iter.Next())) {
		if (TRIGGER_CHECK_OBJ(t, OTRIG_DROP) && (Number(1, 100) <= GET_TRIG_NARG(t))) {
			ADD_UID_VAR(t, actor, "actor", 0);
			return script_driver(obj, t, TRIG_NEW);
		}
	}

	return 1;
}


int give_otrigger(Object *obj, Character *actor, Character *victim) {
	Trigger *t;

	if (!SCRIPT_CHECK(obj, OTRIG_GIVE))
		return 1;

	LListIterator<Trigger *>	iter(TRIGGERS(SCRIPT(obj)));
	while ((t = iter.Next())) {
		if (TRIGGER_CHECK_OBJ(t, OTRIG_GIVE) && (Number(1, 100) <= GET_TRIG_NARG(t))) {
			ADD_UID_VAR(t, actor, "actor", 0);
			ADD_UID_VAR(t, victim, "victim", 0);
			return script_driver(obj, t, TRIG_NEW);
		}
	}
	
	return 1;
}


int sit_otrigger(Object *obj, Character *actor) {
	Trigger *t;

	if (!SCRIPT_CHECK(obj, OTRIG_SIT))
		return 1;

	LListIterator<Trigger *>	iter(TRIGGERS(SCRIPT(obj)));
	while ((t = iter.Next())) {
		if (TRIGGER_CHECK_OBJ(t, OTRIG_SIT) && (Number(1, 100) <= GET_TRIG_NARG(t))) {
			ADD_UID_VAR(t, actor, "actor", 0);
			return script_driver(obj, t, TRIG_NEW);
		}
	}
	
	return 1;
}


int leave_otrigger(Character *actor, int dir) {
	Trigger *t;
	Object *obj;
	
	LListIterator<Object *>	obj_iter(world[IN_ROOM(actor)].contents);
	while ((obj = obj_iter.Next())) {
		if (!SCRIPT_CHECK(obj, OTRIG_LEAVE))
			continue;

		LListIterator<Trigger *>	iter(TRIGGERS(SCRIPT(obj)));
		while ((t = iter.Next())) {
			if (TRIGGER_CHECK_OBJ(t, OTRIG_LEAVE) && (Number(1, 100) <= GET_TRIG_NARG(t))) {
				add_var(t->variables, "direction", dirs[rev_dir[dir]], 0);
				ADD_UID_VAR(t, actor, "actor", 0);
				return script_driver(obj, t, TRIG_NEW);
			}
		}
	}
	
	return 1;
}


int door_otrigger(Character *actor, int subcmd, int dir) {
	Trigger *t;
	Object *obj;

	START_ITER(obj_iter, obj, Object *, world[IN_ROOM(actor)].contents) {
		if (!SCRIPT_CHECK(obj, MTRIG_DOOR))
			continue;

		START_ITER(trig_iter, t, Trigger *, TRIGGERS(SCRIPT(obj))) {
			if (TRIGGER_CHECK_OBJ(t, OTRIG_DOOR) && (Number(1, 100) <= GET_TRIG_NARG(t))) {
				add_var(t->variables, "cmd", door_act[subcmd], 0);
				add_var(t->variables, "direction", dirs[dir], 0);
				ADD_UID_VAR(t, actor, "actor", 0);
				return script_driver(obj, t, TRIG_NEW);
			}
		}
	}
	return 1;
}


void load_otrigger(Object *obj) {
	Trigger *t;
	bool executed = false;
	
	if (!SCRIPT_CHECK(obj, OTRIG_LOAD))
		return;
	
	LListIterator<Trigger *>	iter(TRIGGERS(SCRIPT(obj)));
	while ((t = iter.Next())) {
		if (TRIGGER_CHECK_OBJ(t, OTRIG_LOAD) && ((GET_TRIG_NARG(t) > 100) || 
			(!executed && (Number(1, 100) <= GET_TRIG_NARG(t))))) {
			script_driver(obj, t, TRIG_NEW);
			executed = true;
		}
	}
}


void timer_otrigger(Object *obj) {
	Trigger *t;
	
	if (!SCRIPT_CHECK(obj, OTRIG_TIMER))
		return;
	
	LListIterator<Trigger *>	iter(TRIGGERS(SCRIPT(obj)));
	while ((t = iter.Next())) {
		if (TRIGGER_CHECK_OBJ(t, OTRIG_TIMER))
			script_driver(obj, t, TRIG_NEW);
	}
}


void start_otrigger(Object *obj) {
	Trigger *t;
	
	start_otrigger(obj->contents);
	
	if (!SCRIPT_CHECK(obj, OTRIG_LOAD))
		return;
	
	LListIterator<Trigger *>	iter(TRIGGERS(SCRIPT(obj)));
	while ((t = iter.Next())) {
		if (TRIGGER_CHECK_OBJ(t, OTRIG_START) && (Number(1, 100) <= GET_TRIG_NARG(t))) {
			script_driver(obj, t, TRIG_NEW);
			break;
		}
	}
}


void start_otrigger(LList<Object *> &objList) {
	Object *	obj;
	LListIterator<Object *>	iter(objList);
	while ((obj = iter.Next()))
		start_otrigger(obj);
}


void quit_otrigger(Object *obj) {
	Trigger *t;
	
	quit_otrigger(obj->contents);
	
	if (!SCRIPT_CHECK(obj, OTRIG_LOAD))
		return;
	
	LListIterator<Trigger *>	iter(TRIGGERS(SCRIPT(obj)));
	while ((t = iter.Next())) {
		if (TRIGGER_CHECK_OBJ(t, OTRIG_QUIT) && (Number(1, 100) <= GET_TRIG_NARG(t))) {
			script_driver(obj, t, TRIG_NEW);
			break;
		}
	}
}


void quit_otrigger(LList<Object *> &objList) {
	Object *	obj;
	LListIterator<Object *>	iter(objList);
	while ((obj = iter.Next()))
		quit_otrigger(obj);
}


/*
 *  world triggers
 */

void random_wtrigger(Room *room) {
	Trigger *t;
	bool executed = false;

	if (!SCRIPT_CHECK(room, WTRIG_RANDOM))
		return;

	LListIterator<Trigger *>	iter(TRIGGERS(SCRIPT(room)));
	while ((t = iter.Next())) {
		if (TRIGGER_CHECK_WLD(t, WTRIG_RANDOM) && ((GET_TRIG_NARG(t) > 100) || 
			(!executed && (Number(1, 100) <= GET_TRIG_NARG(t))))) {
			if (TRIGGER_CHECK_MOB(t, MTRIG_GLOBAL) || !is_empty(room->zone)) {
				script_driver(room, t, TRIG_NEW);
				executed = true;
			}
		}
	}
}


int enter_wtrigger(Room *room, Character *actor, int dir) {
	Trigger *t;

	if (!SCRIPT_CHECK(room, WTRIG_ENTER))
		return 1;
	
	LListIterator<Trigger *>	iter(TRIGGERS(SCRIPT(room)));
	while ((t = iter.Next())) {
		if (TRIGGER_CHECK_WLD(t, WTRIG_ENTER) && (Number(1, 100) <= GET_TRIG_NARG(t))) {
			add_var(t->variables, "direction", dirs[rev_dir[dir]], 0);
			ADD_UID_VAR(t, actor, "actor", 0);
			return script_driver(room, t, TRIG_NEW);
		}
	}
	return 1;
}


int command_wtrigger(Character *actor, char *cmd, char *argument) {
	Room *room;
	Trigger *t;
	char *phrase = NULL;

	if (!actor || !SCRIPT_CHECK((&world[IN_ROOM(actor)]), WTRIG_COMMAND))
		return 0;

	room = &world[IN_ROOM(actor)];
	LListIterator<Trigger *>	iter(TRIGGERS(SCRIPT(room)));
	while ((t = iter.Next())) {
		const char *arg = SSData(GET_TRIG_ARG(t));
		if (!arg || !TRIGGER_CHECK_WLD(t, WTRIG_COMMAND))
			continue;
		
		if (command_check(cmd, SSData(GET_TRIG_ARG(t)), &phrase)) {
		// if (*arg == '*' || !strn_cmp(arg, cmd, strlen(arg))) {
			ADD_UID_VAR(t, actor, "actor", 0);
			skip_spaces(argument);
			add_var(t->variables, "arg", argument, 0);
			add_var(t->variables, "cmd", phrase ? phrase : cmd, 0);
			return script_driver(room, t, TRIG_NEW);
		}
	}

	return 0;
}


void speech_wtrigger(Character *actor, char *str) {
	Room *room;
	Trigger *t;

	if (!actor || (IN_ROOM(actor) == NOWHERE) || !SCRIPT_CHECK((&world[IN_ROOM(actor)]), WTRIG_SPEECH))
		return;

	room = &world[IN_ROOM(actor)];
	LListIterator<Trigger *>	iter(TRIGGERS(SCRIPT(room)));
	while ((t = iter.Next())) {
		if (TRIGGER_CHECK_WLD(t, WTRIG_SPEECH) && (!SSData(GET_TRIG_ARG(t)) ||
				(GET_TRIG_NARG(t) ?
				word_check(str, SSData(GET_TRIG_ARG(t))) :
				is_substring(SSData(GET_TRIG_ARG(t)), str)))) {
			ADD_UID_VAR(t, actor, "actor", 0);
			script_driver(room, t, TRIG_NEW);
			break;
		}
	}
}


int drop_wtrigger(Object *obj, Character *actor) {
	Room *room;
	Trigger *t;

	if (!actor || !SCRIPT_CHECK((&world[IN_ROOM(actor)]), WTRIG_DROP))
		return 1;

	room = &world[IN_ROOM(actor)];
	LListIterator<Trigger *>	iter(TRIGGERS(SCRIPT(room)));
	while ((t = iter.Next())) {
		if (TRIGGER_CHECK_WLD(t, WTRIG_DROP) && (Number(1, 100) <= GET_TRIG_NARG(t))) {	
			ADD_UID_VAR(t, actor, "actor", 0);
			ADD_UID_VAR(t, obj, "object", 0);
			return script_driver(room, t, TRIG_NEW);
		}	
	}
	
	return 1;
}


int leave_wtrigger(Room *room, Character *actor, int dir) {
	Trigger *t;

	if (!SCRIPT_CHECK(room, WTRIG_LEAVE))
		return 1;

	LListIterator<Trigger *>	iter(TRIGGERS(SCRIPT(room)));
	while ((t = iter.Next())) {
		if (TRIGGER_CHECK_WLD(t, WTRIG_LEAVE) && (Number(1, 100) <= GET_TRIG_NARG(t))) {
			add_var(t->variables, "direction", dirs[dir], 0);
			ADD_UID_VAR(t, actor, "actor", 0);
			return script_driver(room, t, TRIG_NEW);
		}
	}

	return 1;
}


int door_wtrigger(Character *actor, int subcmd, int dir) {
	Room *room;
	Trigger *t;

	if (!actor || !SCRIPT_CHECK(&world[IN_ROOM(actor)], WTRIG_DOOR))
		return 1;

	room = &world[IN_ROOM(actor)];
	LListIterator<Trigger *>	iter(TRIGGERS(SCRIPT(room)));
	while ((t = iter.Next())) {
		if (TRIGGER_CHECK_WLD(t, WTRIG_DOOR) && (Number(1, 100) <= GET_TRIG_NARG(t))) {
			add_var(t->variables, "cmd", door_act[subcmd], 0);
			add_var(t->variables, "direction", dirs[dir], 0);
			ADD_UID_VAR(t, actor, "actor", 0);
			return script_driver(room, t, TRIG_NEW);
		}
	}

	return 1;
}


void reset_wtrigger(Room *room) {
	Trigger *t;
	
	if (!SCRIPT_CHECK(room, WTRIG_RESET))
		return;
	
	LListIterator<Trigger *>	iter(TRIGGERS(SCRIPT(room)));
	while ((t = iter.Next())) {
		if (TRIGGER_CHECK_WLD(t, WTRIG_RESET) && (Number(1, 100) <= GET_TRIG_NARG(t))) {
			script_driver(room, t, TRIG_NEW);
			break;
		}
	}
}
