/**************************************************************************
*  File: scripts.c                                                        *
*  Usage: contains general functions for using scripts.                   *
**************************************************************************/

 
#include "structs.h"
#include "scripts.h"
#include "utils.h"
#include "descriptors.h"
#include "rooms.h"
#include "characters.h"
#include "objects.h"
#include "comm.h"
#include "interpreter.h"
#include "find.h"
#include "handler.h"
#include "event.h"
#include "olc.h"
#include "clans.h"
#include "buffer.h"
#include "constants.h"
#include "db.h"
#include "skills.h"

#include "scriptedit.h"

//	External Functions
VNum find_target_room(Character * ch, char *rawroomstr);
extern int is_empty(int zone_nr);
extern int find_eq_pos(Character * ch, Object * obj, char *arg, SInt8 bottomslot = 0, SInt8 topslot = NUM_WEARS);

extern struct TimeInfoData time_info;
extern struct zone_data *zone_table;        /*. db.c        .*/

/* function protos from this file */
int script_driver(Scriptable * go, Trigger *trig, int mode);
SInt32 trgvar_in_room(VNum vnum);

void var_subst(Scriptable * go, Script *sc, Trigger *trig, char *line, char *buf);
void eval_op(char *op, char *lhs, char *rhs, char *result, Scriptable * go,
		Script *sc, Trigger *trig);
void eval_expr(char *line, char *result, Scriptable * go, Script *sc, Trigger *trig);
int eval_lhs_op_rhs(char *expr, char *result, Scriptable * go, Script *sc, Trigger *trig);
int process_if(char *cond, Scriptable * go, Script *sc, Trigger *trig);
CmdlistElement *find_end(CmdlistElement *cl);
CmdlistElement *find_else_end(Trigger *trig, CmdlistElement *cl, Scriptable * go, Script *sc);
CmdlistElement *find_case(Trigger *trig, CmdlistElement *cl,
		Scriptable * go, Script *sc, char *cond);
CmdlistElement *find_done(CmdlistElement *cl);
void process_wait(Scriptable * go, Trigger *trig, char *cmd, CmdlistElement *cl, Flags flags);
void process_set(Script *sc, Trigger *trig, char *cmd, int varspace = VarSpace::Local);
void process_eval(Scriptable * go, Script *sc, Trigger *trig, char *cmd, int varspace = VarSpace::Local);
int process_return(Trigger *trig, char *cmd);
void process_unset(Script *sc, Trigger *trig, char *cmd);
void process_global(Script *sc, Trigger *trig, char *cmd, SInt32 id);
int process_foreach(char *cond, Scriptable *go, Script *sc, Trigger *trig, UInt32 &loops);
void process_pushfront(Script *sc, Trigger *trig, char *cmd);
void process_popfront(Script *sc, Trigger *trig, char *cmd);
void process_pushback(Script *sc, Trigger *trig, char *cmd);
void process_popback(Script *sc, Trigger *trig, char *cmd);
void process_pop(Script *sc, Trigger *trig, char *cmd);
void process_shared(Script *sc, Trigger *trig, char *cmd, SInt32 id);
int process_common(const char *var, const char *subfield);

int lookup_script_line(char *p);

void find_uid_name(char *uid, char *name);
void script_stat (Character *ch, Script *sc);
ACMD(do_attach);
int remove_trigger(Script *sc, char *name);
ACMD(do_detach);

void add_var(LList<TrigVarData *> &var_list, const char *name, const char *value, SInt32 id);
int remove_var(LList<TrigVarData *> &var_list, char *name);
void find_replacement(Scriptable * go, Script *sc, Trigger *trig, char *var, char *field, char *subfield, char *str);

TrigVarData *search_for_variable(Script *sc, Trigger *trig, char *var, int varspace = 7);
TrigVarData *extract_variable(Script *sc, Trigger *trig, char *var, int varspace = 7);
bool text_processed(Scriptable *go, Script *sc, Trigger *trig, char *field, char *var,
		char *str, char *subfield);
void makeuid_var(Scriptable * go, Script *sc, Trigger *trig, const char *cmd);
void process_remote(Script *sc, Trigger *trig, const char *cmd);
void process_context(Script *sc, Trigger *trig, const char *cmd);
void extract_value(Script *sc, Trigger *trig, const char *cmd);


const char * foreach_condition_strings[] = {
  "@objs",
  "@mobs",
  "@rooms",
  "zone",
  "room",
  "contents",
  "group",
  "fighting",
//  "riding",
  "\n"
};

/* local structures */
class WaitEvent : public ListedEvent {
public:
						WaitEvent(time_t when, Scriptable *g, Trigger *trg, UInt32 t) :
						ListedEvent(when, &(g->events)), trigger(trg), go(g), type(t) {}
					
	Trigger *		trigger;
	SafePtr<Scriptable>		go;
	UInt32			type;


	time_t					Execute(void);
};


time_t			WaitEvent::Execute(void)
{
	GET_TRIG_WAIT(trigger) = NULL;
	
	script_driver(go(), trigger, TRIG_RESTART);
	return 0;
}


SInt32 trgvar_in_room(VNum vnum) {
	if (!Room::Find(vnum)) {
		mudlogf(NRM, NULL, FALSE, "SCRIPTERR: people.vnum: world[vnum] does not exist");
		return (-1);
	}

	return world[vnum].people.Count();
}


/* checks every PLUSE_SCRIPT for random triggers */
void script_trigger_check(void) {
	Character *	ch;
	Object *	obj;
	Room *		room;
	SInt32		nr;
	Script *	sc;
	ZoneSignal	*sig;
	LListIterator<ZoneSignal *>	sigiter(zoneSignals);

	LListIterator<Character *>	characters(Characters);
	while ((ch = characters.Next())) {
		if ((sc = SCRIPT(ch))) {

			if (IS_SET(SCRIPT_TYPES(sc), MTRIG_SIGNAL) && zoneSignals.Count()) {
				while ((sig = sigiter.Next())) {
					if (sig->zone == ch->Zone())
						signal_trigger(NULL, ch, sig->signal, sig->argument);
				}
				sigiter.Restart(zoneSignals);
			}

			if (!PURGED(ch) && !PURGED(sc) && IS_SET(SCRIPT_TYPES(sc), MTRIG_RANDOM) &&
					(!is_empty(world[IN_ROOM(ch)].zone) || IS_SET(SCRIPT_TYPES(sc), WTRIG_GLOBAL)))
				random_mtrigger(ch);
		}
	}
	
	LListIterator<Object *>	objects(Objects);
	while ((obj = objects.Next())) {
		if ((sc = SCRIPT(obj))) {

			if (IS_SET(SCRIPT_TYPES(sc), MTRIG_SIGNAL) && zoneSignals.Count()) {
				while ((sig = sigiter.Next())) {
					if (sig->zone == obj->Zone())
						signal_trigger(NULL, obj, sig->signal, sig->argument);
				}
				sigiter.Restart(zoneSignals);
			}

			if (!PURGED(obj) && !PURGED(sc) && IS_SET(SCRIPT_TYPES(sc), OTRIG_RANDOM) && (((nr = obj->AbsoluteRoom()) == -1) ||
					!is_empty(world[nr].zone) || IS_SET(SCRIPT_TYPES(sc), WTRIG_GLOBAL)))
				random_otrigger(obj);
		}
	}
	
	
	Map<VNum, Room>::Iterator	iter(world);
	while ((room = iter.Next())) {
		if ((sc = SCRIPT(room))) {
			if (IS_SET(SCRIPT_TYPES(sc), MTRIG_SIGNAL) && zoneSignals.Count()) {
				while ((sig = sigiter.Next())) {
					if (sig->zone == room->Zone())
						signal_trigger(NULL, room, sig->signal, sig->argument);
				}
				sigiter.Restart(zoneSignals);
			}

			if (!PURGED(room) && !PURGED(sc) && IS_SET(SCRIPT_TYPES(sc), WTRIG_RANDOM) &&
					(!is_empty(room->zone) || IS_SET(SCRIPT_TYPES(sc), WTRIG_GLOBAL)))
				random_wtrigger(room);
		}
	}

	while ((sig = sigiter.Next())) {
		delete sig;
	}

	zoneSignals.Clear();
}


void do_stat_trigger(Character *ch, Trigger *trig) {
	CmdlistElement *cmd_list;
	char *string;
	const char **trig_types;
	
	if (!trig) {
		log("SYSERR: NULL trigger passed to do_stat_trigger.");
		return;
	}
	
	string = get_buffer(MAX_STRING_LENGTH * 4);

	sprintf(string, "Name: '&cY%s&c0',  VNum: [&cG%5d&c0]\r\n",
			SSData(GET_TRIG_NAME(trig)), GET_TRIG_VNUM(trig));

	switch (GET_TRIG_DATA_TYPE(trig)) {
		case WLD_TRIGGER:	trig_types = wtrig_types;	break;
		case OBJ_TRIGGER:	trig_types = otrig_types;	break;
		case MOB_TRIGGER:	trig_types = mtrig_types;	break;
		default:			trig_types = wtrig_types;	break;
	}

	sprintbit(GET_TRIG_TYPE_MOB(trig), trig_types, buf1);
	sprintbit(GET_TRIG_TYPE_OBJ(trig), otrig_types, buf2);
	sprintbit(GET_TRIG_TYPE_WLD(trig), wtrig_types, buf3);

	
	sprintf(string + strlen(string), "Mob type: %s, Obj type: %s, Wld type: %s\r\n"
						 "Numeric Arg: %d, Arg list: %s\r\n",
						buf1, buf2, buf3, GET_TRIG_NARG(trig),
						(SSData(GET_TRIG_ARG(trig)) ? SSData(GET_TRIG_ARG(trig)) : "None"));
	
	LListIterator<TrigVarData *>	iter(Trigger::Index[trig->Virtual()].variables);
	TrigVarData *	var;
	char *	name = get_buffer(MAX_INPUT_LENGTH);
	while ((var = iter.Next())) {
		sprintf(buf, "%s:%d", var->name, var->context);
		if (*var->value == UID_CHAR) {
			find_uid_name(var->value, name);
			sprintf(string + strlen(string), "  %-15s:  %s\r\n", var->context ? buf : var->name, name);
		} else
			sprintf(string + strlen(string), "  %-15s:  %s\r\n", var->context ? buf : var->name, var->value);
	}
	release_buffer(name);
	
	strcat(string, "Commands:\r\n");
	
	cmd_list = trig->cmdlist->command;
	while (cmd_list) {
		if (cmd_list->cmd) {
			sprintf(string + strlen(string), "%s\r\n", cmd_list->cmd);
		}
		cmd_list = cmd_list->next;
	}

	page_string(ch->desc, string, true);
	
	release_buffer(string);
}


/* find the name of what the uid points to */
void find_uid_name(char *uid, char *name) {
	Character *ch;
	Object *obj;

	if ((ch = get_char(uid)))		strcpy(name, ch->Name());
	else if ((obj = get_obj(uid)))	strcpy(name, obj->Name());
	else							sprintf(name, "uid = %s, (not found)", uid + 1);
}


/* general function to display stats on script sc */
void script_stat (Character *ch, Script *sc) {
	TrigVarData *tv;
	Trigger *t;
	const char **trig_types;
	char *name = get_buffer(MAX_INPUT_LENGTH),
		*buf = get_buffer(MAX_INPUT_LENGTH);
		
	LListIterator<TrigVarData *>	variables(sc->variables);
	LListIterator<Trigger *>		triggers(TRIGGERS(sc));
	
	ch->Sendf("Global Variables: %s\r\n"
			 "Global Context  : %d\r\n", sc->variables.Count() ? "" : "None", sc->context);
	
	while ((tv = variables.Next())) {
		sprintf(buf, "%s:%d", tv->name, tv->context);
		if (*(tv->value) == UID_CHAR) {
			find_uid_name(tv->value, name);
			ch->Sendf("    %15s:  %s\r\n", tv->context ? buf : tv->name, name);
		} else 
			ch->Sendf("    %15s:  %s\r\n", tv->context ? buf : tv->name, tv->value);
	}
	
	while ((t = triggers.Next())) {
		ch->Sendf("\r\n"
				 "  Trigger: &cY%s&c0, VNum: [&cG%5d&c0]\r\n",
				SSData(GET_TRIG_NAME(t)), GET_TRIG_VNUM(t));

		switch (GET_TRIG_DATA_TYPE(t)) {
		case WLD_TRIGGER:	sprintbit(GET_TRIG_TYPE_WLD(t), wtrig_types, buf);	break;
		case OBJ_TRIGGER:	sprintbit(GET_TRIG_TYPE_OBJ(t), otrig_types, buf);	break;
		case MOB_TRIGGER:	
		default:			sprintbit(GET_TRIG_TYPE_MOB(t), mtrig_types, buf);	break;
		}

		ch->Sendf("  Trigger Type: %s, N Arg: %d, Arg list: %s\r\n", 
				buf, GET_TRIG_NARG(t), 
				(SSData(GET_TRIG_ARG(t)) ? SSData(GET_TRIG_ARG(t)) : "None"));

		if (GET_TRIG_WAIT(t)) {
			ch->Sendf("    Wait: %d, Current line: %s\r\n",
			(int) GET_TRIG_WAIT(t)->Time(), t->curr_state ? t->curr_state->cmd : "(done)");

			ch->Sendf("  Variables: %s\r\n", t->variables.Count() ? "" : "None");
			
			variables.Restart(t->variables);
			while ((tv = variables.Next())) {
				if (*(tv->value) == UID_CHAR) {
					find_uid_name(tv->value, name);
					ch->Sendf("    %15s:  %s\r\n", tv->name, name);
				} else 
					ch->Sendf("    %15s:  %s\r\n", tv->name, tv->value);
			}
			
			variables.Restart(Trigger::Index[t->Virtual()].variables);
			while ((tv = variables.Next())) {
				if (*(tv->value) == UID_CHAR) {
					find_uid_name(tv->value, name);
					ch->Sendf("    %15s:  %s (shared)\r\n", tv->name, name);
				} else 
					ch->Sendf("    %15s:  %s (shared)\r\n", tv->name, tv->value);
			}
		}
	}
	release_buffer(buf);
	release_buffer(name);
}


void do_sstat_room(Character * ch) {
	Room *rm = &world[IN_ROOM(ch)];

	if (!SCRIPT(rm)) {
		act("This room does not have a script.", FALSE, ch, NULL, NULL, TO_CHAR);
		return;
	}

	ch->Sendf("Room name: &cc%s&c0\r\n"
			 "Zone: [%3d], VNum: [&cg%5d&c0]\r\n",
			 rm->Name("<ERROR>"),
			 rm->zone, rm->Virtual());

	script_stat(ch, SCRIPT(rm));
}


#if 0
void GameData::DisplayStat(Character *ch)
{
	char *bitBuf;
	if (!SCRIPT(this)) {
		act("$p does not have a script.", FALSE, ch, j, NULL, TO_CHAR);
		return;
	}
	bitBuf = get_buffer(MAX_STRING_LENGTH);

	ch->Sendf("Name: '&cY%s&c0', VNum: [&cG%5d&c0], Keywords: %s\r\n",
		Name("<None>"), Name(), Virtual());

	script_stat(ch, SCRIPT(j));
	release_buffer(bitBuf);
}
#endif


void do_sstat_object(Character *ch, Object *j) {
	char *bitBuf;
	if (!SCRIPT(j)) {
		act("$p does not have a script.", FALSE, ch, j, NULL, TO_CHAR);
		return;
	}
	bitBuf = get_buffer(MAX_STRING_LENGTH);

	sprinttype(GET_OBJ_TYPE(j), item_types, bitBuf);
	ch->Sendf("Name: '&cY%s&c0', Aliases: %s\r\n"
			 "VNum: [&cG%5d&c0], Type: %s\r\n",
		(j->Name() ? j->Name() : "<None>"), j->Name(),
		j->Virtual(), bitBuf);

	script_stat(ch, SCRIPT(j));
	release_buffer(bitBuf);
}


void do_sstat_character(Character *ch, Character *k) {
	if (!SCRIPT(k)) {
		act("$N does not have a script.", FALSE, ch, NULL, k, TO_CHAR);
		return;
	}
	
	ch->Sendf("Name: '&cY%s&c0', Alias: %s\r\n"
			 "VNum: [&cG%5d&c0], In room [%5d]\r\n",
			k->Name(), k->Keywords(),
			k->Virtual(), IN_ROOM(k));
	
	script_stat(ch, SCRIPT(k));
}


/*
 * adds the trigger t to script sc in in location loc.  loc = -1 means
 * add to the end, loc = 0 means add before all other triggers.
 */
void add_trigger(Script *sc, Trigger *t) /*, int loc) */ {
//	Trigger *i;
//	int n;

//	for (n = loc, i = TRIGGERS(sc); i && i->next && (n != 0); n--, i = i->next);

/*	if (!loc) {
		t->next = TRIGGERS(sc);
		TRIGGERS(sc) = t;
	} else if (!i)
		TRIGGERS(sc) = t;
	else {
		t->next = i->next;
		i->next = t;
	}
*/
	TRIGGERS(sc).Add(t);

	switch (sc->attached) {
	case Datatypes::Character:
		SCRIPT_TYPES(sc) |= GET_TRIG_TYPE_MOB(t);
		break;
	case Datatypes::Object:
		SCRIPT_TYPES(sc) |= GET_TRIG_TYPE_OBJ(t);
		break;
	case Datatypes::Room:
		SCRIPT_TYPES(sc) |= GET_TRIG_TYPE_WLD(t);
		break;
	default:
		sprintf(buf, "SCRIPTERR:	Invalid attach type %d in add_trigger", sc->attached);
		log(buf);
		break;
	}
	// SCRIPT_TYPES(sc) |= GET_TRIG_TYPE(t);
}


ACMD(do_attach)  {
	Character *victim;
	Object *object;
	Trigger *trig;
	char *targ_name, *trig_name, *loc_name, *arg;
	int loc, room, tn;

	trig_name = get_buffer(MAX_INPUT_LENGTH);
	targ_name = get_buffer(MAX_INPUT_LENGTH);
	loc_name = get_buffer(MAX_INPUT_LENGTH);
	arg = get_buffer(MAX_STRING_LENGTH);
	
	argument = two_arguments(argument, arg, targ_name);
	two_arguments(argument, trig_name, loc_name);
	
	if (!*arg || !*targ_name || !*trig_name)
		ch->Send("Usage: attach { mob | obj | room } { name } { trigger } [ location ]\r\n");
	else {  
		tn = atoi(trig_name);
		loc = (*loc_name) ? atoi(loc_name) : -1;
  
		if (is_abbrev(arg, "mob")) {
			if ((victim = get_char_vis(ch, targ_name, FIND_CHAR_WORLD))) {
				if (IS_STAFF(victim) && GET_TRUST(ch) != MAX_TRUST) {
					ch->Send("You don't have the power to do that.\r\n");
				} else {
					/* have a valid mob, now get trigger */
					if ((trig = Trigger::Read(tn))) {

						if (!SCRIPT(victim))
							SCRIPT(victim) = new Script(Datatypes::Character);
						add_trigger(SCRIPT(victim), trig); //, loc);

						ch->Sendf("Trigger %d (%s) attached to %s.\r\n",
								tn, SSData(GET_TRIG_NAME(trig)), victim->RealName());
					} else
						ch->Send("That trigger does not exist.\r\n");
				}
			} else {
				ch->Send("That mob does not exist.\r\n");
			}
		} else if (is_abbrev(arg, "obj")) {
			if ((object = get_obj_vis(ch, targ_name))) {
				
				/* have a valid obj, now get trigger */
				if ((trig = Trigger::Read(tn))) {
	
					if (!SCRIPT(object))
						SCRIPT(object) = new Script(Datatypes::Object);
					add_trigger(SCRIPT(object), trig); //, loc);
	  
					ch->Sendf("Trigger %d (%s) attached to %s.\r\n", tn, SSData(GET_TRIG_NAME(trig)), 
							object->Name());
				} else
					ch->Send("That trigger does not exist.\r\n");
			} else
				ch->Send("That object does not exist.\r\n");
		} else if (is_abbrev(arg, "room")) {
			if (isdigit(*targ_name) && !strchr(targ_name, '.')) {
				if ((room = find_target_room(ch, targ_name)) != NOWHERE) {
	
					/* have a valid room, now get trigger */
					if ((trig = Trigger::Read(tn))) {

						if (!world[room].script)
							world[room].script = new Script(Datatypes::Room);
						add_trigger(world[room].script, trig); //, loc);
  
						ch->Sendf("Trigger %d (%s) attached to room %d.\r\n",
								tn, SSData(GET_TRIG_NAME(trig)), room);
					} else
						ch->Send("That trigger does not exist.\r\n");
				}
	    	} else
				ch->Send("You need to supply a room number.\r\n");
		} else
			ch->Send("Please specify 'mob', 'obj', or 'room'.\r\n");
	}
	release_buffer(targ_name);
	release_buffer(trig_name);
	release_buffer(loc_name);
	release_buffer(arg);
}


/*
 *  removes the trigger specified by name, and the script of o if
 *  it removes the last trigger.  name can either be a number, or
 *  a 'silly' name for the trigger, including things like 2.beggar-death.
 *  returns 0 if did not find the trigger, otherwise 1.  If it matters,
 *  you might need to check to see if all the triggers were removed after
 *  this function returns, in order to remove the script.
 */
int remove_trigger(Script *sc, char *name) {
	Trigger *i;
	int num = 0, string = FALSE, n;
	char *cname;
  
	if (!sc)
		return 0;

	if ((cname = strstr(name,".")) || (!isdigit(*name)) ) {
		string = TRUE;
		if (cname) {
			*cname = '\0';
			num = atoi(name);
			name = ++cname;
		}
	} else
		num = atoi(name);
  
	n = 0;
	START_ITER(iter, i, Trigger *, TRIGGERS(sc)) {
		if (string && silly_isname(name, SSData(GET_TRIG_NAME(i))) &&
			++n >= num)					break;
		else if (++n >= num)			break;
		else if (i->Virtual() == num)	break;
	}

	if (!i)	return 0;
	
	TRIGGERS(sc).Remove(i);
	i->Extract();
	
	//	update the script type bitvector
	SCRIPT_TYPES(sc) = 0;
	START_ITER(type_iter, i, Trigger *, TRIGGERS(sc)) {
		switch (sc->attached) {
		case Datatypes::Character:
			SET_BIT(SCRIPT_TYPES(sc), GET_TRIG_TYPE_MOB(i));
			break;
		case Datatypes::Object:
			SET_BIT(SCRIPT_TYPES(sc), GET_TRIG_TYPE_OBJ(i));
			break;
		case Datatypes::Room:
			SET_BIT(SCRIPT_TYPES(sc), GET_TRIG_TYPE_WLD(i));
			break;
		default:
			sprintf(buf, "SCRIPTERR:	Invalid attach type %d in remove_trigger", sc->attached);
			log(buf);
			break;
		}
	}

	return 1;
}


// CHANGEPOINT:  This code is just SCREAMING for a rewrite - for the love of
// GOD!!! -DH
ACMD(do_detach)  {
	Character *victim = NULL;
	Object *object = NULL;
	Room *room;
	char *arg1, *arg2, *arg3;
	char *trigger = 0;
	int tmp;

	arg1 = get_buffer(MAX_INPUT_LENGTH);
	arg2 = get_buffer(MAX_INPUT_LENGTH);
	arg3 = get_buffer(MAX_INPUT_LENGTH);
	
	argument = two_arguments(argument, arg1, arg2);
	one_argument(argument, arg3);

	if (!arg1 || !arg2 || !*arg1 || !*arg2) {
		ch->Send("Usage: detach [ mob | object | room ] { target } { trigger | 'all' }\r\n");
	} else {
		if (!str_cmp(arg1, "room")) {
			room = &world[IN_ROOM(ch)];
			if (!SCRIPT(room)) 
				ch->Send("This room does not have any triggers.\r\n");
			else if (!str_cmp(arg2, "all")) {
				SCRIPT(room)->Extract();
				SCRIPT(room) = NULL;
				ch->Send("All triggers removed from room.\r\n");
			} else if (remove_trigger(SCRIPT(room), arg2)) {
				ch->Send("Trigger removed.\r\n");
				if (!TRIGGERS(SCRIPT(room)).Top()) {
					SCRIPT(room)->Extract();
					SCRIPT(room) = NULL;
				}
			} else
				ch->Send("That trigger was not found.\r\n");
		} else {
			if (is_abbrev(arg1, "mob")) {
				if (!(victim = get_char_vis(ch, arg2, FIND_CHAR_WORLD)))
					ch->Send("No such mobile around.\r\n");
				else if (!*arg3)
					ch->Send("You must specify a trigger to remove.\r\n");
				else
					trigger = arg3;
			} else if (is_abbrev(arg1, "object")) {
				if (!(object = get_obj_vis(ch, arg2)))
					ch->Send("No such object around.\r\n");
				else if (!*arg3)
					ch->Send("You must specify a trigger to remove.\r\n");
				else
					trigger = arg3;
			} else  {
				if ((object = get_object_in_equip_vis(ch, arg1, ch->equipment, &tmp)));
				else if ((object = get_obj_in_list_vis(ch, arg1, ch->contents)));
				else if ((victim = get_char_vis(ch, arg1, FIND_CHAR_ROOM)));
				else if ((object = get_obj_in_list_vis(ch, arg1, world[IN_ROOM(ch)].contents)));
				else if ((victim = get_char_vis(ch, arg1, FIND_CHAR_WORLD)));
				else if ((object = get_obj_vis(ch, arg1)));
				else ch->Send("Nothing around by that name.\r\n");
				trigger = arg2;
			}

			if (victim) {
				if (!SCRIPT(victim))
					ch->Send("That mob doesn't have any triggers.\r\n");
				else if (!trigger || !str_cmp(trigger, "all")) {
					SCRIPT(victim)->Extract();
					SCRIPT(victim) = NULL;
					act("All triggers removed from $N.", TRUE, ch, 0, victim, TO_CHAR);
				} else if (remove_trigger(SCRIPT(victim), trigger)) {
					ch->Send("Trigger removed.\r\n");
					if (!TRIGGERS(SCRIPT(victim)).Top()) {
						SCRIPT(victim)->Extract();
						SCRIPT(victim) = NULL;
					}
				} else
					ch->Send("That trigger was not found.\r\n");
			} else if (object) {
				if (!SCRIPT(object))
					ch->Send("That object doesn't have any triggers.\r\n");
				else if (!str_cmp(arg3, "all")) {
					SCRIPT(object)->Extract();
					SCRIPT(object) = NULL;
					act("All triggers removed from $p.", TRUE, ch, 0, object, TO_CHAR);
				} else if (remove_trigger(SCRIPT(object), trigger)) {
					ch->Send("Trigger removed.\r\n");
					if (!TRIGGERS(SCRIPT(object)).Top()) {
						SCRIPT(object)->Extract();
						SCRIPT(object) = NULL;
					}
				} else
					ch->Send("That trigger was not found.\r\n");
			}
		}
	}
	release_buffer(arg1);
	release_buffer(arg2);
	release_buffer(arg3);
}


/* adds a variable with given name and value to trigger */
void add_var(LList<TrigVarData *> &var_list, const char *name, const char *value, SInt32 context) {
	TrigVarData *vd;
	
	LListIterator<TrigVarData *>	iter(var_list);
	while ((vd = iter.Next()))
		if (!str_cmp(vd->name, name) && (!context || !vd->context || vd->context == context))
			break;
	
	if (vd) {
		if (vd->value)	free(vd->value);
	} else {
		vd = new TrigVarData;
		vd->name = str_dup(name);

		var_list.Add(vd);
	}
	vd->value = str_dup(value);
	vd->context = context;
}


/*
 * remove var name from var_list
 * returns 1 if found, else 0
 */
int remove_var(LList<TrigVarData *> &var_list, char *name) {
	TrigVarData *i;

	LListIterator<TrigVarData *>	iter(var_list);
	while ((i = iter.Next())) {
		if (!str_cmp(name, i->name))
			break;
	}
	
	if (i) {
		var_list.Remove(i);
		delete i;
		return 1;
	}
	return 0;
}


bool text_processed(Scriptable *go, Script *sc, Trigger *trig, char *field, char *var,
		char *str, char *subfield) {
	char *buf = get_buffer(MAX_INPUT_LENGTH);
	if (subfield && *subfield) {
		var_subst(go, sc, trig, subfield, buf);
		subfield = buf;
	}
	
	if (!str_cmp(field, "strlen")) {				//	str length
		sprintf(str, "%d", strlen(var));
	} else if (!str_cmp(field, "trim")) {			//	trim whitespace
		char *p = var;
		char *p2 = var + strlen(var) - 1;
		skip_spaces(p);
		while ((p >= p2) && isspace(*p2)) p2--;
		if (p > p2)		//	Nothing left
			*str = '\0';
		else {
			while (p <= p2)
				*str++ = *p++;
			*str = '\0';
		}
	} else if (!str_cmp(field, "head")) {			//	First word
		char *car = var;
		while (*car && !isspace(*car))	*str++ = *car++;
		*str = '\0';
	} else if (!str_cmp(field, "tail")) {			//	Remainder
		char *cdr = var;
		while (*cdr && !isspace(*cdr))	cdr++;
		while (*cdr && isspace(*cdr))	cdr++;
		while (*cdr)	*str++ = *cdr++;
		*str = '\0';
	} else if (!str_cmp(field, "word")) {
		int num = atoi(subfield);
		if (num > 0) {
			char *cdx = var;
			
			for (int i = 0; cdx && *cdx && i < num; i++) {
				while (*cdx && !isspace(*cdx))	cdx++;
				while (*cdx && isspace(*cdx))	cdx++;
			}
			while (cdx && *cdx && !isspace(*cdx))	*str++ = *cdx++;
			*str = '\0';
		}
	} else if (!str_cmp(field, "contains")) {
		strcpy(str, str_str(var, subfield) ? "1" : "0");
	} else if (!str_cmp(field, "common")) {
		strcpy(str, process_common(var, subfield) ? "1" : "0");
#ifdef UNDER_CONSTRUCTION
} else if (!str_cmp(field, "pushback")) {
	cstring_pushback(&value, subfield);
	return TRUE;
  } else if (!str_cmp(field, "popback")) {
	cstring_popback(&value, str);
	return TRUE;
  } else if (!str_cmp(field, "pushfront")) {
	cstring_pushfront(&value, subfield);
	return TRUE;
  } else if (!str_cmp(field, "popfront")) {
	cstring_popfront(&value, str);
	return TRUE;
#endif
	} else if (is_number(subfield)) {
		SInt32	num = atoi(subfield);
		
		while (num > 0)		var = one_argument(var, str);
/*		if (num > 0) {
			char *cdx = var;
			
			for (int i = 0; *cdx && i < num; i++) {
				while (*cdx && !isspace(*cdx))	cdx++;
				while (*cdx && isspace(*cdx))	cdx++;
			}
			while (*cdx && !isspace(*cdx))	*str++ = *cdx++;
			*str = '\0';
		}
*/
	} else {
		release_buffer(buf);
		return false;
	}
	release_buffer(buf);
	return true;
}


TrigVarData *search_for_variable(Script *sc, Trigger *trig, char *var, int varspace) {
	TrigVarData *vd = NULL;
	Trigger *	tmp = NULL;
	LListIterator<TrigVarData *>	iter;
	while ((vd = iter.Next()) && str_cmp(vd->name, var));

	if (IS_SET(varspace, (1 << VarSpace::Local))) {
		iter.Restart(trig->variables);
		while ((vd = iter.Next()))
			if (!str_cmp(vd->name, var) && (vd->context == 0 || vd->context == sc->context))
				break;
	}
	
	if (IS_SET(varspace, (1 << VarSpace::Global)) && !vd) {
		iter.Restart(sc->variables);
		while ((vd = iter.Next()))
			if (!str_cmp(vd->name, var) && (vd->context == 0 || vd->context == sc->context))
				break;
	}
	
	if (IS_SET(varspace, (1 << VarSpace::Shared)) && !vd) {
		LListIterator<Trigger *>	trg_iter(sc->triggers);
		while (!vd && (tmp = trg_iter.Next())) {
			iter.Restart(Trigger::Index[tmp->Virtual()].variables);
			while ((vd = iter.Next()))
				if (!str_cmp(vd->name, var) && (vd->context == 0 || vd->context == sc->context))
					break;
		}
	}
	
	return vd;

}


TrigVarData *extract_variable(Script *sc, Trigger *trig, char *var, int varspace) {
	TrigVarData *vd = NULL;
	Trigger *	tmp = NULL;
	LListIterator<TrigVarData *>	iter;
	while ((vd = iter.Next()) && str_cmp(vd->name, var));

	if (IS_SET(varspace, (1 << VarSpace::Local))) {
		iter.Restart(trig->variables);
		while ((vd = iter.Next()))
			if (!str_cmp(vd->name, var) && (vd->context == 0 || vd->context == sc->context))
				break;
		
		if (vd)	trig->variables.Remove(vd);
	}
	
	if (IS_SET(varspace, (1 << VarSpace::Global)) && !vd) {
		iter.Restart(sc->variables);
		while ((vd = iter.Next()))
			if (!str_cmp(vd->name, var) && (vd->context == 0 || vd->context == sc->context))
				break;
		
		if (vd)	sc->variables.Remove(vd);
	}
	
	if (IS_SET(varspace, (1 << VarSpace::Shared)) && !vd) {
		LListIterator<Trigger *>	trg_iter(sc->triggers);
		while (!vd && (tmp = trg_iter.Next())) {
			iter.Restart(Trigger::Index[tmp->Virtual()].variables);
			while ((vd = iter.Next()))
				if (!str_cmp(vd->name, var) && (vd->context == 0 || vd->context == sc->context))
					break;
					
			if (vd)	Trigger::Index[tmp->Virtual()].variables.Remove(vd);
		}
	}
	
	return vd;
}



/* sets str to be the value of var.field */
void find_replacement(Scriptable * go, Script *sc, Trigger *trig,
		char *var, char *field, char *subfield, char *str) {
	TrigVarData *	vd;
	Character		*ch = NULL,
					*c = NULL,
					*rndm;
	Object			*obj,
					*o = NULL;
	Room		*room,
					*r = NULL;
	char *			name;
	SInt32			num, count, i;
	SInt16			skillnum = -1;
	bool			unknown = false;
	
	Weather::Weather *conditions = &zone_table[world[go->AbsoluteRoom()].zone].conditions;

	*str = '\0';
	
	vd = search_for_variable(sc, trig, var);
	
	if (!*field) {
		if (vd)								strcpy(str, vd->value);
		else if (!str_cmp(var, "self"))		sprintf(str, "%c%d", UID_CHAR, GET_ID(go));
		else								*str = '\0';
	} else {
		if (vd) {
			name = vd->value;
			
			switch (go->DataType()) {
				case Datatypes::Character:
					ch = static_cast<Character *>(go);
					i = 0;
					if ((o = get_object_in_equip(ch, name, ch->equipment, &i)));
					else if ((o = get_obj_in_list(name, ch->contents)));
					else if ((c = get_char_room(name, IN_ROOM(ch))));
					else if ((o = get_obj_in_list(name, world[IN_ROOM(ch)].contents)));
					else if ((c = get_char(name)));
					else if ((o = get_obj(name)));
					else if ((r = get_room(name)));
					break;
					
				case Datatypes::Object:
					obj = static_cast<Object *>(go);

					if ((c = get_char_by_obj(obj, name)));
					else if ((o = get_obj_by_obj(obj, name)));
					else if ((r = get_room(name)));
					break;
					
				case Datatypes::Room:
					room = static_cast<Room *>(go);

					if ((c = get_char_by_room(room, name)));
					else if ((o = get_obj_by_room(room, name)));
					else if ((r = get_room(name)));
					break;
				
				default:	break;
			}
		} else {
			if (!str_cmp(var, "self")) {
				switch (go->DataType()) {
					case Datatypes::Character:		c = static_cast<Character *>(go);	break;
					case Datatypes::Object:	o = static_cast<Object *>(go);		break;
					case Datatypes::Room:	r = static_cast<Room *>(go);		break;
					default:	break;
				}
			} else if (!str_cmp(var, "people")) {
				sprintf(str, "%d", ((num = atoi(field)) > 0) ? trgvar_in_room(num) : 0);
				return;
			} else if (!str_cmp(var, "random")) {
				if (!str_cmp(field, "char")) {
					LListIterator<Character *>	iter;
					rndm = NULL;
					count = 0;
					
					switch (go->DataType()) {
						case Datatypes::Character:
							ch = (Character *) go;
							
							iter.Start(world[IN_ROOM(ch)].people);
							while ((c = iter.Next())) {
								if (!PRF_FLAGGED(c, Preference::NoHassle) && (c != ch) && c->CanBeSeen(ch))
									if (!Number(0, count++))	rndm = c;
							}
							break;
							
						case Datatypes::Object:
						case Datatypes::Room:
							iter.Start(world[go->AbsoluteRoom()].people);
							while ((c = iter.Next())) {
//								if (!PRF_FLAGGED(c, Preference::NoHassle) /* && !GET_INVIS_LEV(c) */)
									if (!Number(0, count++))	rndm = c;
							}
							break;

						default:	break;
					}
					if (rndm)	sprintf(str, "%c%d", UID_CHAR, GET_ID(rndm));
				} else
					sprintf(str, "%d", ((num = atoi(field)) > 0) ? Number(1, num) : 0);
				return;
			} else if (!str_cmp(var, "time")) {
				if (!str_cmp(field, "hour"))		sprintf(str,"%d", time_info.hours);
				else if (!str_cmp(field, "day"))	sprintf(str,"%d", time_info.day + 1);
				else if (!str_cmp(field, "month"))	sprintf(str,"%d", time_info.month + 1);
				else if (!str_cmp(field, "year"))	sprintf(str,"%d", time_info.year);
			} else if (!str_cmp(var, "weather")) {
				if (!str_cmp(field, "rain"))		sprintf(str,"%d", conditions->precipRate);
				else if (!str_cmp(field, "wind"))	sprintf(str,"%d", conditions->windspeed);
				else if (!str_cmp(field, "temp"))	sprintf(str,"%d", conditions->temperature);
			} else if (*var == UID_CHAR) {
				if ((c = find_char(atoi(var + 1))));
				else if ((o = find_obj(atoi(var + 1))));
				else if ((r = find_room(atoi(var + 1))));
			} else if (!c && !o && !r && isdigit(*var)) {	// Hopefully this will catch self.room.name. -DH
				num = atoi(var);
				if (num > 0)					// This prevents 0.name from yielding "The Void"
					r = &(world[num]);
			} else {
				switch (go->DataType()) {
					case Datatypes::Character:
						ch = static_cast<Character *>(go);
						i = 0;
						if ((o = get_object_in_equip(ch, var, ch->equipment, &i)));
						else if ((o = get_obj_in_list(var, ch->contents)));
						else if ((c = get_char_room(var, IN_ROOM(ch))));
						else if ((o = get_obj_in_list(var, world[IN_ROOM(ch)].contents)));
						else if ((c = get_char(var)));
						else if ((o = get_obj(var)));
						else if ((r = get_room(var)));
						break;
						
					case Datatypes::Object:
						obj = static_cast<Object *>(go);

						if ((c = get_char_by_obj(obj, var)));
						else if ((o = get_obj_by_obj(obj, var)));
						else if ((r = get_room(var)));
						break;
						
					case Datatypes::Room:
						room = static_cast<Room *>(go);

						if ((c = get_char_by_room(room, var)));
						else if ((o = get_obj_by_room(room, var)));
						else if ((r = get_room(var)));
						break;
					
					default:	break;
				}
			}
//			 if (*var == UID_CHAR) {
//				if ((c = find_char(atoi(var + 1))));
//				else if ((o = find_obj(atoi(var + 1))));
//				else if ((r = find_room(atoi(var + 1))));
//			}
/*
			 else if (!str_cmp(var, "mob")) {
				if (!str_cmp(field, "exists")) {
					i = atoi(subfield);
					sprintf(str, "%d", (Character::Real(i) != -1));
				} else
					mudlogf(NRM, NULL, FALSE, "SCRIPTERR: Trigger: \"%s\" [%d]: unknown \"mob\" field: '%s'",
							SSData(GET_TRIG_NAME(trig)), GET_TRIG_VNUM(trig), field);
				return;
			} else if (!str_cmp(var, "obj")) {
				if (!str_cmp(field, "exists")) {
					i = atoi(subfield);
					sprintf(str, "%d", (Object::Real(i) != -1));
				} else
					mudlogf(NRM, NULL, FALSE, "SCRIPTERR: Trigger: \"%s\" [%d]: unknown \"obj\" field: '%s'",
							SSData(GET_TRIG_NAME(trig)), GET_TRIG_VNUM(trig), field);
				return;
			} else if (!str_cmp(var, "room")) {
				if (!str_cmp(field, "exists")) {
					i = atoi(subfield);
					sprintf(str, "%d", (Room::Real(i) != -1));
				} else
					mudlogf(NRM, NULL, FALSE, "SCRIPTERR: Trigger: \"%s\" [%d]: unknown \"room\" field: '%s'",
							SSData(GET_TRIG_NAME(trig)), GET_TRIG_VNUM(trig), field);
				return;
			}
*/
		}

#if 0
		if (vd && (num = atoi(field))) {
			// GET WORD - recycle field buffer!
			char *buf = get_buffer(MAX_INPUT_LENGTH);
			name = vd->value;
			for (count = 0; (count < num) && (*name); count++)
				name = one_argument(name, buf);
			if (count == num)	strcpy(str, buf);
			release_buffer(buf);
		// This broke a loop which did "while (%offenders.head%)" prior to offenders being created
		// as a variable.  It output the raw string "offenders", causing an infinite loop. -DH
		} else if (text_processed(go, sc, trig, field, vd ? vd->value : var, str, subfield)) ;
#else
		if (vd) {
			if ((num = atoi(field))) {
				// GET WORD - recycle field buffer!
				char *buf = get_buffer(MAX_INPUT_LENGTH);
				name = vd->value;
				for (count = 0; (count < num) && (*name); count++)
					name = one_argument(name, buf);
				if (count == num)	strcpy(str, buf);
				release_buffer(buf);
//			} else {
				// This'll work for variables, but presently %self.name.head% doesn't work.
//				text_processed(go, sc, trig, field, vd->value, str, subfield);
			} else if (text_processed(go, sc, trig, field, vd->value, str, subfield)) {
				return;
			}
		}
#endif

		if (c) {
//			if (text_processed(field, vd ? vd->value : var, str, subfield))	return;
			//	else
			switch (*field) {
			case 'a':
				if (!str_cmp(field, "alias")) {
					if (IS_NPC(c))		strcpy(str, c->Keywords("undefined"));
					else				strcpy(str, c->Name("undefined"));
				}
				else if (!str_cmp(field, "align"))	sprintf(str, "%d", GET_ALIGNMENT(c));
				else if (!str_cmp(field, "aiming")) {
					if (!AIMING(c))					strcpy(str, "0");
					else							sprintf(str, "%c%d", UID_CHAR, AIMING(c));
				}
				else								unknown = true;
				break;

			case 'c':
				if (!str_cmp(field, "cha"))			sprintf(str, "%d", GET_CHA(c));
				else if (!str_cmp(field, "con"))	sprintf(str, "%d", GET_CON(c));
				else if (!str_cmp(field, "class"))	sprinttype(GET_CLASS(c), class_types, str);
				else if (!str_cmp(field, "canbeseen"))
					strcpy(str, ((go->DataType() == Datatypes::Character) && !c->CanBeSeen(static_cast<Character *>(go))) ? "0" : "1");
				else if (!str_cmp(field, "carrying")) {
					Object *held;
					LListIterator<Object *>	iter(c->contents);
					while ((held = iter.Next()))
						sprintf(str + strlen(str), "%c%d ", UID_CHAR, GET_ID(held));
				}
				else								unknown = true;
				break;

			case 'd':
				if (!str_cmp(field, "dex"))			sprintf(str, "%d", GET_DEX(c));
				else								unknown = true;
				break;

			case 'e':
				if (!str_cmp(field, "encumbrance"))	sprintf(str, "%d", GET_ENCUMBRANCE(c, IS_CARRYING_W(c)));

				else if (!str_cmp(field, "eq")) {
					Object *	worn;
					SInt32		i;
					for (i = 0; i < NUM_WEARS; i++) {
						if ((worn = GET_EQ(c, i)))
							sprintf(str + strlen(str), "%c%d ", UID_CHAR, GET_ID(worn));
					}
				}

				else								unknown = true;
				break;

			case 'f':
				if(!str_cmp(field, "fighting")) {
					if (FIGHTING(c) == NULL)		strcpy(str, "0");
					else							sprintf(str, "%c%d", UID_CHAR, GET_ID(FIGHTING(c)));
				}
				
				else if (!str_cmp(field, "following")) {
					if (c->master == NULL)			strcpy(str, "0");
					else							sprintf(str, "%c%d", UID_CHAR, GET_ID(c->master));
				} 
				
				else								unknown = true;
				break;
			
			case 'g':
				if (!str_cmp(field, "gold"))		sprintf(str, "%d", GET_GOLD(c));
				else if (!str_cmp(field, "groupfront"))		sprintf(str, "%d", GROUP_POS(c) ? 0 : 1);
				else if (!str_cmp(field, "grouprear"))		sprintf(str, "%d", GROUP_POS(c) ? 1 : 0);
				else								unknown = true;
				break;
			
			case 'h':
				if (!str_cmp(field, "hit"))				sprintf(str, "%d", GET_HIT(c));
				else if (!str_cmp(field, "height"))		sprintf(str, "%d", GET_HEIGHT(c));
				else if (!str_cmp(field, "heshe"))		strcpy(str, HSSH(c));
				else if (!str_cmp(field, "hisher"))		strcpy(str, HSHR(c));
				else if (!str_cmp(field, "himher"))		strcpy(str, HMHR(c));
				else if (!str_cmp(field, "hishers"))	strcpy(str, HSHRS(c));
				else if (!str_cmp(field, "hunting")) {
					if (!HUNTING(c))			strcpy(str, "0");
					else						sprintf(str, "%c%d", UID_CHAR, HUNTING(c));
				} 
			
				else if (!str_cmp(field, "hitpercent"))
					sprintf(str, "%d", (int) (100 * GET_HIT(c)) / MAX(GET_MAX_HIT(c), (SInt16) 1));
			
				else									unknown = true;
				break;
			
			case 'i':
				if (!str_cmp(field, "id"))				sprintf(str, "%d", GET_ID(c));
				else if (!str_cmp(field, "int"))		sprintf(str, "%d", GET_INT(c));
				else if (!str_cmp(field, "ispc"))		sprintf(str, "%d", IS_NPC(c) ? 0 : 1);
				else if (!str_cmp(field, "isnpc"))		sprintf(str, "%d", IS_NPC(c) ? 1 : 0);
				else if (!str_cmp(field, "isgood"))		sprintf(str, "%d", IS_GOOD(c) ? 1 : 0);
				else if (!str_cmp(field, "isevil"))		sprintf(str, "%d", IS_EVIL(c) ? 1 : 0);
				else if (!str_cmp(field, "isneutral"))	sprintf(str, "%d", IS_NEUTRAL(c) ? 1 : 0);
				else									unknown = true;
				break;
			
			case 'k':
				if (!str_cmp(field, "keywords"))		strcpy(str, c->Keywords());
				else									unknown = true;
				break;
			
			case 'l':
				if (!str_cmp(field, "lawful"))			sprintf(str, "%d", GET_LAWFULNESS(c));
				else if (!str_cmp(field, "level"))		sprintf(str, "%d", GET_LEVEL(c));
			
				else if (!str_cmp(field, "leader")) {
					if ((c->master == NULL) || (!AFF_FLAGGED(c, AffectBit::Group)))
						sprintf(str, "%c%d", UID_CHAR, GET_ID(c));
					else
						sprintf(str, "%c%d", UID_CHAR, GET_ID(c->master));
				} 
			
				else									unknown = true;
				break;
			
			case 'm':
				if (!str_cmp(field, "mana"))			sprintf(str, "%d", GET_MANA(c));
				else if (!str_cmp(field, "move"))		sprintf(str, "%d", GET_MOVE(c));

				else if (!str_cmp(field, "master")) {
					if ((c->master == NULL) || (!AFF_FLAGGED(ch, AffectBit::Charm)))
						strcpy(str, "0");
					else
						sprintf(str, "%c%d", UID_CHAR, GET_ID(c->master));
				}

				else if (!str_cmp(field, "material"))	sprintf(str, "%s", material_types[c->material]);
				else if (!str_cmp(field, "maxhit"))		sprintf(str, "%d", GET_MAX_HIT(c));
				else if (!str_cmp(field, "maxmana"))	sprintf(str, "%d", GET_MAX_MANA(c));
				else if (!str_cmp(field, "maxmove"))	sprintf(str, "%d", GET_MAX_MOVE(c));

				else									unknown = true;
				break;
			
			case 'n':
				if (!str_cmp(field, "name"))			strcpy(str, c->Name("undefined"));
				else if (!str_cmp(field, "npc"))		strcpy(str, IS_NPC(c) ? "1" : "0");
			
// 				else if (!str_cmp(field, "nextinroom")) {
// 					if (c->next_in_room == NULL)
// 						strcpy(str, "0");
// 					else
// 						sprintf(str, "%c%d", UID_CHAR, GET_ID(c->next_in_room));
// 				} 
			
				else									unknown = true;
				break;
			
			case 'p':
				if (!str_cmp(field, "position"))		strcpy(str, position_types[(int) GET_POS(c)]);
				else									unknown = true;
				break;
			
			case 'r':
				if (!str_cmp(field, "race"))			sprinttype(GET_RACE(c), race_types, str);
				else if (!str_cmp(field, "room"))		sprintf(str, "%d", IN_ROOM(c));

				else if (!str_cmp(field, "realname"))	strcpy(str, c->Name("undefined"));

				else if (!str_cmp(field, "ridden_by")) {
					if (RIDER(c) == NULL)	strcpy(str, "0");
					else					sprintf(str, "%c%d", UID_CHAR, GET_ID(RIDER(c)));
				} 
				
				else if (!str_cmp(field, "riding")) {
					if (RIDING(c) == NULL)		strcpy(str, "0");
					else						sprintf(str, "%c%d", UID_CHAR, GET_ID(RIDING(c)));
				}
				
				else if (!str_cmp(field, "roomseeking")) {
					if (!ROOMSEEKING(c))	strcpy(str, "0");
					else					sprintf(str, "%d", ROOMSEEKING(c));
				}
				
				else
					unknown = true;
				break;
			
			case 's':
				if (!str_cmp(field, "sex"))			strcpy(str, genders[(int) GET_SEX(c)]);
				else if (!str_cmp(field, "sana"))		sprintf(str, "%s", SANA(c));
				else if (!str_cmp(field, "shortdesc"))	strcpy(str, c->SDesc());
				else if (!str_cmp(field, "str"))	sprintf(str, "%d", GET_STR(c));
			
				else if (!str_cmp(field, "seeking")) {
					if (!SEEKING(c))	strcpy(str, "0");
					else				sprintf(str, "%c%d", UID_CHAR, SEEKING(c));
				}
				
				else if (!str_cmp(field, "stalking")) {
					if (!STALKING(c))	strcpy(str, "0");
					else				sprintf(str, "%c%d", UID_CHAR, STALKING(c));
				}
			
				else if (!str_cmp(field, "skillpts")) {
					skillnum = Skill::Number(subfield, FIRST_STAT, LAST_STAT);
					if (skillnum < 0)
						skillnum = Skill::Number(subfield);
						sprintf(str, "%d", CHAR_SKILL(c).GetSkillPts(skillnum, NULL));
					}
			
				else if (!str_cmp(field, "skillroll")) {
					skillnum = Skill::Number(subfield, FIRST_STAT, LAST_STAT);
					if (skillnum < 0)
						skillnum = Skill::Number(subfield);
					sprintf(str, "%d", SKILLROLL(c, skillnum));
				}
			
				else if (!str_cmp(field, "skillchance")) {
					skillnum = Skill::Number(subfield, FIRST_STAT, LAST_STAT);
					if (skillnum < 0)
						skillnum = Skill::Number(subfield);
					sprintf(str, "%d", SKILLCHANCE(c, skillnum, NULL));
				}
			
				else if (!str_cmp(field, "skilltheory")) {
					skillnum = Skill::Number(subfield, FIRST_STAT, LAST_STAT);
					if (skillnum < 0)
						skillnum = Skill::Number(subfield);
					sprintf(str, "%d", CHAR_SKILL(c).GetSkillTheory(skillnum, NULL));
				}
			
				else if (!str_cmp(field, "sitting")) {
					if (c->SittingOn())				sprintf(str, "%c%d", UID_CHAR, GET_ID(c->SittingOn()));
				}

				else							unknown = true;
				break;
			
			case 't':
				if (!str_cmp(field, "trust"))	sprintf(str, "%d", GET_TRUST(c));
				else							unknown = true;
				break;
			
			case 'u':
				if (!str_cmp(field, "uid"))		sprintf(str, "%c%d", UID_CHAR, GET_ID(c));
				else							unknown = true;
				break;
			
			case 'v':
				if (!str_cmp(field, "vnum"))			sprintf(str, "%d", c->Virtual());
				else if (!str_cmp(field, "varclass"))	strcpy(str, "character");
				else									unknown = true;
				break;
			
			case 'w':
				if (!str_cmp(field, "wis"))					sprintf(str, "%d", GET_WIS(c));
				else if (!str_cmp(field, "weight"))			sprintf(str, "%d", GET_WEIGHT(c));
				else if (!str_cmp(field, "weightcarried"))	sprintf(str, "%d", IS_CARRYING_W(c));

				else if (!str_cmp(field, "weapon")) {
					SInt32 pos;
					for (pos = WEAR_HAND_R; pos < NUM_WEARS; ++pos) {
						if (!GET_EQ(c, pos))
							continue;
						if (GET_OBJ_TYPE(GET_EQ(c, pos)) == ITEM_WEAPON)
							break;
					}
					if (pos < NUM_WEARS) {
						sprintf(str, "%c%d", UID_CHAR, GET_ID(GET_EQ(c, pos)));
					}
				}

				else if (!str_cmp(field, "wearing")) {
					if (*subfield) {
						SInt32 pos;
						for (pos = 0; pos < NUM_WEARS; ++pos) {
							if (!GET_EQ(c, pos))
								continue;
							if (!strcmp(subfield, GET_WEARSLOT(pos, GET_BODYTYPE(c)).keywords))
								break;
						}
						if (pos < NUM_WEARS) {
							sprintf(str, "%c%d", UID_CHAR, GET_ID(GET_EQ(c, pos)));
						}
					}
				}

				else										unknown = true;
				break;
			
			case 'z':
				if (!str_cmp(field, "zone")) {
					if (!PURGED(c) && (IN_ROOM(c) != NOWHERE))
						sprintf(str, "%d", zone_table[world[IN_ROOM(c)].Zone()].number);
					else
						*str = '\0';
				} 
				else	unknown = true;
				break;
			
			default:
				unknown = true;
			}
			
			if (unknown) {
				if (SCRIPT(c)) {
					vd = search_for_variable(SCRIPT(c), trig, field, 6);
					if (vd)	{
						sprintf(str, "%s", vd->value);
						unknown = false;
					}
					else		unknown = true;
				}
			}

			if (unknown) {	// I hate the redundancy, but see above for why we need the second check.
					mudlogf(NRM, NULL, FALSE, "SCRIPTERR: Trigger: \"%s\" [%d]: unknown char field: '%s'",
							SSData(GET_TRIG_NAME(trig)), GET_TRIG_VNUM(trig), field);
			}

		} else if (o) {
//			if (text_processed(field, vd ? vd->value : var, str, subfield))	return;
//			else

			switch (*field) {
			case 'a':
				if (!str_cmp(field, "ana"))		sprintf(str, "%s", ANA(o));
				else							unknown = true;
				break;

			case 'c':
				if (!str_cmp(field, "contents")) {
					Object *content;
					LListIterator<Object *>	iter(o->contents);
					while ((content = iter.Next()))
						sprintf(str + strlen(str), "%c%d ", UID_CHAR, GET_ID(content));
				}
				else if (!str_cmp(field, "cost"))		sprintf(str, "%d", o->cost);
				else							unknown = true;
				break;

			case 'i':
				if (!str_cmp(field, "id"))		sprintf(str, "%d", GET_ID(o));

				else if (!str_cmp(field, "inside")) {
					if (o->InObj())				sprintf(str, "%c%d", UID_CHAR, o->Inside()->ID());
					else						strcpy(str, "0");
				}
				
				else							unknown = true;
				break;
			
			case 'k':
				if (!str_cmp(field, "keywords"))	strcpy(str, o->Keywords());
				else								unknown = true;
				break;
			
			case 'm':
				if (!str_cmp(field, "material"))	sprintf(str, "%s", material_types[o->material]);
				else								unknown = true;
				break;

			case 'n':
				if (!str_cmp(field, "name"))		strcpy(str, o->Name());
			
// 				else if (!str_cmp(field, "nextinroom")) {
// 					if (o->next_content == NULL)
// 					strcpy(str, "0");
// 					else
// 									sprintf(str, "%c%d", UID_CHAR, GET_ID(o->next_content));
// 				}
				
				else								unknown = true;
				break;
			
			case 'o':
				if (!str_cmp(field, "owner")) {
					if (o->Inside() && o->Inside()->DataType() == Datatypes::Character)
						sprintf(str, "%c%d", UID_CHAR, o->Inside()->ID());
					else					strcpy(str, "0");
				} 
			
				else								unknown = true;
				break;
			
			case 'r':
				if (!str_cmp(field, "room"))		sprintf(str, "%d", IN_ROOM(o));
				else								unknown = true;
				break;
			
			case 's':
				if (!str_cmp(field, "sana"))			sprintf(str, "%s", SANA(o));
				else if (!str_cmp(field, "shortdesc"))	strcpy(str, o->SDesc());
				else									unknown = true;
				break;
			
			case 't':
				if (!str_cmp(field, "type"))		sprinttype(GET_OBJ_TYPE(o), item_types, str);
				else if (!str_cmp(field, "timer"))	sprintf(str, "%d", GET_OBJ_TIMER(o));
				else								unknown = true;
				break;
			
			case 'u':
				if (!str_cmp(field, "uid"))			sprintf(str, "%c%d", UID_CHAR, GET_ID(o));
				else								unknown = true;
				break;
			
			case 'v':
				if (!str_cmp(field, "vnum"))			sprintf(str, "%d", o->Virtual());
				else if (!str_cmp(field, "val0"))		sprintf(str, "%d", GET_OBJ_VAL(o, 0));
				else if (!str_cmp(field, "val1"))		sprintf(str, "%d", GET_OBJ_VAL(o, 1));
				else if (!str_cmp(field, "val2"))		sprintf(str, "%d", GET_OBJ_VAL(o, 2));
				else if (!str_cmp(field, "val3"))		sprintf(str, "%d", GET_OBJ_VAL(o, 3));
				else if (!str_cmp(field, "val4"))		sprintf(str, "%d", GET_OBJ_VAL(o, 4));
				else if (!str_cmp(field, "val5"))		sprintf(str, "%d", GET_OBJ_VAL(o, 5));
				else if (!str_cmp(field, "val6"))		sprintf(str, "%d", GET_OBJ_VAL(o, 6));
				else if (!str_cmp(field, "val7"))		sprintf(str, "%d", GET_OBJ_VAL(o, 7));
				else if (!str_cmp(field, "varclass"))	strcpy(str, "object");
				else									unknown = true;
				break;

			case 'w':
				if (!str_cmp(field, "wearer")) {
					if (o->WornBy())	sprintf(str, "%c%d", UID_CHAR, o->WornBy()->ID());
					else				strcpy(str, "0");
				} 
				
				else if (!str_cmp(field, "weight"))	sprintf(str, "%d", GET_OBJ_WEIGHT(o));
				else								unknown = true;
				break;
			
			case 'z':
				if (!str_cmp(field, "zone")) {
					if (!PURGED(c) && (IN_ROOM(o) != NOWHERE))
						sprintf(str, "%d", zone_table[world[IN_ROOM(c)].Zone()].number);
					else
						*str = '\0';
				} 
				else	unknown = true;
				break;
			
			default:
				unknown = true;
			}
			
			if (unknown) {
				if (SCRIPT(o)) {
					vd = search_for_variable(SCRIPT(o), trig, field, 6);
					if (vd)	{
						sprintf(str, "%s", vd->value);
						unknown = false;
					}
					else		unknown = true;
				}
			}

			if (unknown) {	// I hate the redundancy, but see above for why we need the second check.
					mudlogf(NRM, NULL, FALSE, "SCRIPTERR: Trigger: \"%s\" [%d]: unknown object field: '%s'",
							SSData(GET_TRIG_NAME(trig)), GET_TRIG_VNUM(trig), field);
			}

		} else if (r) {
//			if (text_processed(field, vd ? vd->value : var, str, subfield))	return;
//			else

			switch (*field) {
			case 'c':
				if (!str_cmp(field, "contents")) {
					Object *content;
					LListIterator<Object *>	iter(r->contents);
					while ((content = iter.Next()))
						sprintf(str + strlen(str), "%c%d ", UID_CHAR, GET_ID(content));
				}
				else								unknown = true;
				break;

			case 'd':
				if (!str_cmp(field, "down")) {
					if (r->Direction(DOWN)) {
						if (r->Direction(DOWN)->to_room != NOWHERE && world.Find(r->Direction(DOWN)->to_room))
							sprintf(str, "%c%d ", UID_CHAR, world[r->Direction(DOWN)->to_room].ID());
						sprintbit(r->Direction(DOWN)->exit_info, exit_bits, str + strlen(str));
					}
				} 
				
				else								unknown = true;
				break;
			case 'e':
				if (!str_cmp(field, "east")) {
					if (r->Direction(EAST)) {
						if (r->Direction(EAST)->to_room != NOWHERE && world.Find(r->Direction(EAST)->to_room))
							sprintf(str, "%c%d ", UID_CHAR, world[r->Direction(EAST)->to_room].ID());
						sprintbit(r->Direction(EAST)->exit_info, exit_bits, str + strlen(str));
					}
				} 
				
				else								unknown = true;
				break;

			case 'i':
				if (!str_cmp(field, "id"))			sprintf(str, "%d", GET_ID(r));
				else								unknown = true;
				break;

			case 'n':
				if (!str_cmp(field, "name"))		strcpy(str, r->Name(""));

				else if (!str_cmp(field, "north")) {
					if (r->Direction(NORTH)) {
						if (r->Direction(NORTH)->to_room != NOWHERE && world.Find(r->Direction(NORTH)->to_room))
							sprintf(str, "%c%d ", UID_CHAR, world[r->Direction(NORTH)->to_room].ID());
						sprintbit(r->Direction(NORTH)->exit_info, exit_bits, str + strlen(str));
					}
				}
				
				else								unknown = true;
				break;

			case 'p':
				if (!str_cmp(field, "people")) {
					Character *ch;
					LListIterator<Character *>	iter(r->people);
					while ((ch = iter.Next()))
						sprintf(str + strlen(str), "%c%d ", UID_CHAR, GET_ID(ch));
				}
				else								unknown = true;
				break;

			case 'r':
				if (!str_cmp(field, "room"))
					sprintf(str, "%d", r->Virtual());
				
				else								unknown = true;
				break;
			
			case 's':
				if (!str_cmp(field, "south")) {
					if (r->Direction(SOUTH)) {
						if (r->Direction(SOUTH)->to_room != NOWHERE && world.Find(r->Direction(SOUTH)->to_room))
							sprintf(str, "%c%d ", UID_CHAR, world[r->Direction(SOUTH)->to_room].ID());
						sprintbit(r->Direction(SOUTH)->exit_info, exit_bits, str + strlen(str));
					}
				}
				
				else								unknown = true;
				break;

			case 'u':
				if (!str_cmp(field, "up")) {
					if (r->Direction(UP)) {
						if (r->Direction(UP)->to_room != NOWHERE && world.Find(r->Direction(UP)->to_room))
							sprintf(str, "%c%d ", UID_CHAR, world[r->Direction(UP)->to_room].ID());
						sprintbit(r->Direction(UP)->exit_info, exit_bits, str + strlen(str));
					}
				}
				
				else if (!str_cmp(field, "uid"))	sprintf(str, "%c%d", UID_CHAR, GET_ID(r));
				else								unknown = true;
				break;

			case 'v':
				if (!str_cmp(field, "vnum"))			sprintf(str, "%d", r->Virtual());
				else if (!str_cmp(field, "varclass"))	strcpy(str, "room");
				else									unknown = true;
				break;
			
			case 'w':
				if (!str_cmp(field, "west")) {
					if (r->Direction(WEST)) {
						if (r->Direction(WEST)->to_room != NOWHERE && world.Find(r->Direction(WEST)->to_room))
							sprintf(str, "%c%d ", UID_CHAR, world[r->Direction(WEST)->to_room].ID());
						sprintbit(r->Direction(WEST)->exit_info, exit_bits, str + strlen(str));
					}
				} 
				
				else								unknown = true;
				break;
			
			case 'z':
				if (!str_cmp(field, "zone"))		sprintf(str, "%d", zone_table[world[r->Virtual()].Zone()].number);
				
				else								unknown = true;
				break;

			default:
				unknown = true;
			}

			if (unknown) {
				if (SCRIPT(r)) {
					vd = search_for_variable(SCRIPT(r), trig, field, 6);
					if (vd)	{
						sprintf(str, "%s", vd->value);
						unknown = false;
					}
					else		unknown = true;
				}
			}

			if (unknown) {	// I hate the redundancy, but see above for why we need the second check.
				mudlogf(NRM, NULL, FALSE, "SCRIPTERR: Trigger: \"%s\" [%d]: unknown room field: '%s'",
					SSData(GET_TRIG_NAME(trig)), GET_TRIG_VNUM(trig), field);
			}
		} else
#if 0
	// This is more for Phase 2 compatibility than anything; it allows undeclared variables to be
	// passed over without comment.
			mudlogf(NRM, NULL, FALSE, "SCRIPTERR: Trigger: \"%s\" [%d]: unknown arg field: '%s'",
					SSData(GET_TRIG_NAME(trig)), GET_TRIG_VNUM(trig), field);
#else
			*str = '\0';
#endif
	}
}


/* substitutes any variables into line and returns it as buf */
void var_subst(Scriptable * go, Script *sc, Trigger *trig, char *line, char *buf) {
	char *tmp, *repl_str, *var, *field, *p, *varfield;
	char *subfield, *subfield_p;
	int left, len;
	int	paren_count = 0;

	if (!strchr(line, '%')) {
		strcpy(buf, line);
		return;
	}
	
	// SET UP BUFS

	tmp = get_buffer(MAX_INPUT_LENGTH);
	repl_str = get_buffer(MAX_INPUT_LENGTH);
	subfield = get_buffer(MAX_INPUT_LENGTH);
	varfield = get_buffer(MAX_INPUT_LENGTH);
	
	p = strcpy(tmp, line);
	subfield_p = subfield;
	
	left = MAX_INPUT_LENGTH - 1;

	while (*p && (left > 0)) {
		while (*p && (*p != '%') && (left > 0)) {
			*(buf++) = *(p++);
			left--;
		}

		*buf = '\0';

		/* double % */
		if (*p && (*(++p) == '%') && (left > 0)) {
			*(buf++) = *(p++);
			*buf = '\0';
			left--;
			continue;
		} else if (*p && (left > 0)) {	//	Variable found
			for (var = p; *p && (*p != '%') && (*p != '.'); p++);

			field = p;
			if (*p == '.') {
				*(p++) = '\0';
				for (field = p; *p && ((*p != '%') || (paren_count)); p++) {
					if (*p == '(') {
						*p = '\0';
						paren_count++;
					} else if (*p == ')') {
						*p = '\0';
						paren_count--;
					} else if (*p == '.' && !paren_count) {	//	Another variable...
						*(p++) = '\0';
						strcpy(varfield, var);
						find_replacement(go, sc, trig, varfield, field, NULL, repl_str);
						var = repl_str;
						field = p;
					} else if (paren_count)
						*subfield_p++ = *p;
				}
			}
			
			*(p++) = '\0';
			*subfield_p = '\0';
			
	//		if (*subfield) {
	//			eval_expr(subfield, subfield_repl, go, sc, trig);
	//			strcpy(subfield, subfield_repl);
	//		}
			
			strcpy(varfield, var);
			
			find_replacement(go, sc, trig, varfield, field, subfield, repl_str);
			
			strncat(buf, repl_str, left);
			len = strlen(repl_str);
			buf += len;
			left -= len;
		}
	}
	release_buffer(tmp);		// FIXME:  Attempted to release this when not allocated
	release_buffer(repl_str);
	release_buffer(subfield);
	release_buffer(varfield);
}


/* evaluates 'lhs op rhs', and copies to result */
void eval_op(char *op, char *lhs, char *rhs, char *result, Scriptable * go, Script *sc, Trigger *trig) {
	char *p;
	int n;

	/* strip off extra spaces at begin and end */
	while (*lhs && isspace(*lhs))	lhs++;
	while (*rhs && isspace(*rhs))	rhs++;

	for (p = lhs; *p; p++);
	for (--p; isspace(*p) && (p > lhs); *p-- = '\0');
	for (p = rhs; *p; p++);
	for (--p; isspace(*p) && (p > rhs); *p-- = '\0');

	/* find the op, and figure out the value */
	if (!strcmp("||", op))
		strcpy(result, ((!*lhs || (*lhs == '0')) && (!*rhs || (*rhs == '0'))) ? "0" : "1");
	else if (!strcmp("&&", op))
		strcpy (result, (!*lhs || (*lhs == '0') || !*rhs || (*rhs == '0')) ? "0" : "1");
	else if (!strcmp("==", op)) {
		if (is_number(lhs) && is_number(rhs))	sprintf(result, "%d", atoi(lhs) == atoi(rhs));
		else									sprintf(result, "%d", !str_cmp(lhs, rhs));
	} else if (!strcmp("!=", op)) {
		if (is_number(lhs) && is_number(rhs))	sprintf(result, "%d", atoi(lhs) != atoi(rhs));
		else									sprintf(result, "%d", str_cmp(lhs, rhs));
	} else if (!strcmp("<=", op)) {
		if (is_number(lhs) && is_number(rhs))	sprintf(result, "%d", atoi(lhs) <= atoi(rhs));
		else									sprintf(result, "%d", str_cmp(lhs, rhs) <= 0);
	} else if (!strcmp(">=", op)) {
		if (is_number(lhs) && is_number(rhs))	sprintf(result, "%d", atoi(lhs) >= atoi(rhs));
		else									sprintf(result, "%d", str_cmp(lhs, rhs) <= 0);
	} else if (!strcmp("<", op)) {
		if (is_number(lhs) && is_number(rhs))	sprintf(result, "%d", atoi(lhs) < atoi(rhs));
		else									sprintf(result, "%d", str_cmp(lhs, rhs) < 0);
	} else if (!strcmp(">", op)) {
		if (is_number(lhs) && is_number(rhs))	sprintf(result, "%d", atoi(lhs) > atoi(rhs));
		else									sprintf(result, "%d", str_cmp(lhs, rhs) > 0);
	} else if (!strcmp("/=", op))				sprintf(result, "%c", str_str(lhs, rhs) ? '1' : '0');
	else if (!strcmp("/!", op))					sprintf(result, "%c", str_str(lhs, rhs) ? '0' : '1');
	else if (!strcmp("*", op))					sprintf(result, "%d", atoi(lhs) * atoi(rhs));
	else if (!strcmp("/", op))					sprintf(result, "%d", (n = atoi(rhs)) ? (atoi(lhs) / n) : 0);
	else if (!strcmp("+", op))					sprintf(result, "%d", atoi(lhs) + atoi(rhs));
	else if (!strcmp("-", op))					sprintf(result, "%d", atoi(lhs) - atoi(rhs));
	else if (!strcmp("!", op))					sprintf(result, "%d", (is_number(rhs)) ? !atoi(rhs) : !*rhs);
}


/* evaluates line, and returns answer in result */
void eval_expr(char *line, char *result, Scriptable * go, Script *sc, Trigger *trig) {
	char *p, *expr = get_buffer(MAX_INPUT_LENGTH);

	skip_spaces(line);
	
	if (eval_lhs_op_rhs(line, result, go, sc, trig));
	else if (*line == '(') {
		p = strcpy(expr, line);
		p = matching_paren(expr);
		*p = '\0';
		eval_expr(expr + 1, result, go, sc, trig);
	} else
		var_subst(go, sc, trig, line, result);
	release_buffer(expr);
}


/*
 * evaluates expr if it is in the form lhs op rhs, and copies
 * answer in result.  returns 1 if expr is evaluated, else 0
 */
int eval_lhs_op_rhs(char *expr, char *result, Scriptable * go, Script *sc, Trigger *trig) {
	char *p, *tokens[256];
	char *line, *lhr, *rhr;
	int i, j, rslt = 0;

	/*
	 * valid operands, in order of priority
	 * each must also be defined in eval_op()
	 */
	static char *ops[] = {
		"||",
		"&&",
		"==",
		"!=",
		"<=",
		">=",
		"<",
		">",
		"/=",
		"-",
		"+",
		"/",
		"*",
		"!",
		"\n"
	};
	
	line = get_buffer(MAX_STRING_LENGTH);
	lhr = get_buffer(MAX_STRING_LENGTH);
	rhr = get_buffer(MAX_STRING_LENGTH);
	
	p = strcpy(line, expr);

	/*
	 * initialize tokens, an array of pointers to locations
	 * in line where the ops could possibly occur.
	 */
	for (j = 0; *p; j++) {
		tokens[j] = p;
		if (*p == '(')			p = matching_paren(p) + 1;
		else if (*p == '"')		p = matching_quote(p) + 1;
		else if (isalnum(*p))	for (p++; *p && (isalnum(*p) || isspace(*p)); p++);
		else					p++;
	}
	tokens[j] = NULL;

	for (i = 0; (*ops[i] != '\n') && !rslt; i++)
		for (j = 0; tokens[j] && !rslt; j++)
			if (!strn_cmp(ops[i], tokens[j], strlen(ops[i]))) {
				*tokens[j] = '\0';
				p = tokens[j] + strlen(ops[i]);

				eval_expr(line, lhr, go, sc, trig);
				eval_expr(p, rhr, go, sc, trig);
				eval_op(ops[i], lhr, rhr, result, go, sc, trig);

				rslt = 1;
			}
			
	release_buffer(lhr);
	release_buffer(rhr);
	release_buffer(line);

	return rslt;
}


/* returns 1 if cond is true, else 0 */
int process_if(char *cond, Scriptable * go, Script *sc, Trigger *trig) {
	char *p, *resultString = get_buffer(MAX_STRING_LENGTH);
	int result;
	
	eval_expr(cond, resultString, go, sc, trig);
	  
	p = resultString;
	skip_spaces(p);

	if (!*p || *p == '0')	result = 0;
	else					result = 1;
	release_buffer(resultString);
	return result;
}


int process_foreach(char *cond, Scriptable *go, Script *sc, Trigger *trig, UInt32 &loops) {
	char *variable, *args, *expr, *line, *p;
	int tmp;
	
	TrigVarData *vd = NULL;
	Character *c = NULL;
	Object *o = NULL;
	Room *r = NULL;
	
	Flags flags = 0;
	
	line = cond;
	
	if (!strn_cmp(line, "mob ", 4))			SET_BIT(flags, (1 << Foreach::Mobs));
	else if (!strn_cmp(line, "obj ", 4))	SET_BIT(flags, (1 << Foreach::Objs));
	else if (!strn_cmp(line, "room ", 5))	SET_BIT(flags, (1 << Foreach::Rooms));
	else {
		mudlogf(NRM, NULL, FALSE, "SCRIPTERR: Trigger: \"%s\" [%d]: foreach without type obj|mob|room",
				SSData(GET_TRIG_NAME(trig)), GET_TRIG_VNUM(trig));
		return 0;
	}
	
	variable = get_buffer(MAX_INPUT_LENGTH);

	while (*line && (*line != ' '))	++line;
	while (*line && (*line == ' ')) ++line;
	
	p = variable;
	while (*line && (*line != ' '))	*p++ = *line++;
	*p = '\0';
	
	if (!*variable) {
		mudlogf(NRM, NULL, FALSE, "SCRIPTERR: Trigger: \"%s\" [%d]: foreach without variable specified",
				SSData(GET_TRIG_NAME(trig)), GET_TRIG_VNUM(trig));
		release_buffer(variable);
		return 0;
	}
	
	args = get_buffer(MAX_INPUT_LENGTH);
	expr = get_buffer(MAX_INPUT_LENGTH);
	
	while (*line && (*line == ' '))	++line;
	while (*line && (*line != '(') && (*line != '%')) {
		p = args;
		while (*line && (*line != ' '))	*p++ = *line++;
		*p = '\0';
		
		if ((tmp = search_block(args, foreach_condition_strings, false)) >= 0)	SET_BIT(flags, (1 << tmp));
	}
	
	while (*line && (*line == ' '))	++line;
	
	if (!loops) {
		eval_expr(line, expr, go, sc, trig);
		
		if (!*expr || !strcmp(expr, "0")) {
			release_buffer(args);
			release_buffer(variable);
			release_buffer(expr);
			return 0;
		}
		
		if (!str_cmp(expr, "self")) {
			switch (go->DataType()) {
				case Datatypes::Character:		c = static_cast<Character *>(go);	break;
				case Datatypes::Object:	o = static_cast<Object *>(go);		break;
				case Datatypes::Room:	r = static_cast<Room *>(go);		break;
			}
		} else {
			if (IS_SET(flags, (1 << Foreach::Mobs)))		c = get_char(expr);
			else if (IS_SET(flags, (1 << Foreach::Objs)))	o = get_obj(expr);
			else if (IS_SET(flags, (1 << Foreach::Rooms)))	r = get_room(expr);
		}
		
		if (c)			sprintf(expr, "%c%d", UID_CHAR, GET_ID(c));
		else if (o)		sprintf(expr, "%c%d", UID_CHAR, GET_ID(o));
		else if (r)		sprintf(expr, "%c%d", UID_CHAR, GET_ID(r));
		else {
			release_buffer(args);
			release_buffer(variable);
			release_buffer(expr);
			return 0;
		}
	} else {
		vd = search_for_variable(sc, trig, variable, (1 << VarSpace::Local));
		
		if (!vd) {
			release_buffer(args);
			release_buffer(variable);
			release_buffer(expr);
			return 0;
		}
		
		if (IS_SET(flags, (1 << Foreach::Mobs))) {
			c = get_char(vd->value);
			LListIterator<Character *>	iter(world[c->InRoom()].people);
			Character *t;
			while ((t = iter.Next()))
				if (t == c)
					break;
			
			if (t)	t = iter.Next();
			
			if (t)	sprintf(expr, "%c%d", UID_CHAR, GET_ID(t));
			else {
				release_buffer(args);
				release_buffer(variable);
				release_buffer(expr);
				return 0;
			}
			
		} else if (IS_SET(flags, (1 << Foreach::Objs))) {
			o = get_obj(vd->value);
			LListIterator<Object *>	iter;
			
			//	Next in content...hooboy...
			
			if (o->Inside())		iter.Start(o->Inside()->contents);
			
			Object *t;
			while ((t = iter.Next()))
				if (t == o)
					break;
			
			if (t)	t = iter.Next();
			
			if (t)	sprintf(expr, "%c%d", UID_CHAR, GET_ID(t));
			else {
				release_buffer(args);
				release_buffer(variable);
				release_buffer(expr);
				return 0;
			}
			
		} else if (IS_SET(flags, (1 << Foreach::Rooms))) {
			r = get_room(vd->value);
			int roomnum = r->Virtual();
			r = NULL;
			while (!r) {
				if (++roomnum > world.Top())	break;
				if (!world.Find(roomnum))		continue;
				r = &world[roomnum];
			}
			if (r)	sprintf(expr, "%c%d", UID_CHAR, GET_ID(r));
			else {
				release_buffer(args);
				release_buffer(variable);
				release_buffer(expr);
				return 0;
			}
		} else {
			mudlogf(NRM, NULL, false, "SCRIPTERR: Trigger: \"%s\" [%d]: foreach target '%s' not found",
					SSData(GET_TRIG_NAME(trig)), GET_TRIG_VNUM(trig), args);
			release_buffer(args);
			release_buffer(variable);
			release_buffer(expr);
			return 0;
		}
	}

	release_buffer(args);
	
	if (!*expr) {
		vd = extract_variable(sc, trig, variable, (1 << VarSpace::Local));
		if (vd)	delete vd;
				
		release_buffer(variable);
		release_buffer(expr);
		
		return 0;
	} else
		add_var(trig->variables, variable, expr, sc->context);
	
	release_buffer(variable);
	release_buffer(expr);
		
	return 1;

}


// Returns 1 if subfield has any word in common with var, 0 otherwise.
int process_common(const char *var, const char *subfield) {
	skip_spaces(var);
	skip_spaces(subfield);

	half_chop(var, buf, buf2);

	while (*buf) {
		if (str_str(subfield, buf))
	    	return 1;
		half_chop(buf2, buf, buf2);
	}

	return 0;
}


/*
 * scans for a case/default instance
 * returns the line containg the correct case instance, or the last
 * line of the trigger if not found.
 */
CmdlistElement *find_case(Trigger *trig, CmdlistElement *cl, Scriptable * go, Script *sc, char *cond) {
	CmdlistElement *c;
	char *p, *buf, *buf2;

	if (!(cl->next))
		return cl;
	
	buf = get_buffer(MAX_STRING_LENGTH);
	eval_expr(cond, buf, go, sc, trig);
	
	for (c = cl->next; c && c->next; c = c->next) {
		if ((c->type == CommandSwitch::While) || (c->type == CommandSwitch::Switch) ||
			(c->type == CommandSwitch::Foreach))		c = find_done(c);
		else if (c->type == CommandSwitch::Case) {
			p = c->cmd;
			skip_spaces(p);
			buf2 = get_buffer(MAX_STRING_LENGTH);
			eval_op("==", buf, p + 5, buf2, go, sc, trig);
			if (*buf2 && *buf2 != '0') {
				release_buffer(buf2);
				break;
			}
			release_buffer(buf2);
		} else if (c->type == CommandSwitch::Default)	break;
		else if (c->type == CommandSwitch::Done)		break;
		if (!c->next)									break;
	}
	release_buffer(buf);
	return c;
}



/*
 * scans for end of while/switch-blocks.
 * returns the line containg 'end', or the last
 * line of the trigger if not found.
 */
CmdlistElement *find_done(CmdlistElement *cl) {
	CmdlistElement *c;

	if (!(cl->next))
		return cl;

	for (c = cl->next; c && c->next; c = c->next) {
		if (c->type == CommandSwitch::While || c->type == CommandSwitch::Switch ||
				c->type == CommandSwitch::Foreach)
			c = find_done(c);
		else if (c->type == CommandSwitch::Done)	return c;
		if (!c->next)								return c;
	}
	return c;
}


/*
 * scans for end of if-block.
 * returns the line containg 'end', or the last
 * line of the trigger if not found.
 */
CmdlistElement *find_end(CmdlistElement *cl) {
	CmdlistElement *c;
	char *p;

	if (!(cl->next))
		return cl;

	for (c = cl->next; c->next; c = c->next) {
		p = c->cmd;
		skip_spaces(p);

//		if (!strn_cmp("if ", p, 3))			c = find_end(c);
//		else if (!strn_cmp("end", p, 3))	return c;
		if (c->type == CommandSwitch::If)	c = find_end(c);
		else if (c->type == CommandSwitch::End)	return c;
		if (!c->next)						return c;
	}
	return c;
}


/*
 * searches for valid elseif, else, or end to continue execution at.
 * returns line of elseif, else, or end if found, or last line of trigger.
 */
CmdlistElement *find_else_end(Trigger *trig, CmdlistElement *cl, Scriptable * go, Script *sc) {
	CmdlistElement *c;
	char *p;

	if (!(cl->next))	return cl;

	for (c = cl->next; c->next; c = c->next) {
		p = c->cmd;
		skip_spaces(p);

		if (c->type == CommandSwitch::If)		c = find_end(c);
		else if (c->type == CommandSwitch::ElseIf) {
			if (process_if(p + 7, go, sc, trig)) {
				GET_TRIG_DEPTH(trig)++;
				return c;
			}
		} else if (c->type == CommandSwitch::Else) {
			GET_TRIG_DEPTH(trig)++;
			return c;
		} else if (c->type == CommandSwitch::End)	return c;
		if (!c->next)								return c;
	}
	return c;
}


/* processes any 'wait' commands in a trigger */
void process_wait(Scriptable * go, Trigger *trig, char *cmd, CmdlistElement *cl, Flags flags) {
//	char *buf, *arg;
	char *arg;
	SInt32	time, hr, min, ntime, type = 0;
	char c = '\0';

	extern struct TimeInfoData time_info;
	extern long pulse;

//	buf = get_buffer(MAX_INPUT_LENGTH);
	
//	arg = any_one_arg(cmd, buf);
	arg = cmd;
	skip_spaces(arg);

	if (!*arg)
		mudlogf(NRM, NULL, FALSE, "SCRIPTERR: Trigger: \"%s\" [%d]: wait w/o an arg: 'wait %s'",
				SSData(GET_TRIG_NAME(trig)), GET_TRIG_VNUM(trig), cmd);
	else if (!strn_cmp(arg, "until ", 6)) {
		/* valid forms of time are 14:30 and 1430 */
		if (sscanf(arg, " until %d:%d", &hr, &min) == 2)
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
	} else if (sscanf(arg, " %d %c", &time, &c) == 2) {
		if (c == 'd')		time *= 24 * SECS_PER_MUD_HOUR * PASSES_PER_SEC;
		else if (c == 'h')	time *= SECS_PER_REAL_HOUR * PASSES_PER_SEC;
		else if (c == 't')	time *= SECS_PER_MUD_HOUR * PASSES_PER_SEC;	//	't': ticks
		else if (c == 's')	time *= PASSES_PER_SEC;
		else if (c == 'p')	;											//	'p': pulses
		else 				time *= PASSES_PER_SEC;						//	Default: seconds
	} else					time *= PASSES_PER_SEC / 5;
	
//	release_buffer(buf);

	type = (go->DataType() == Datatypes::Room) ? WLD_TRIGGER : 0;

	GET_TRIG_WAIT(trig) = new WaitEvent(time, go, trig, type);
	trig->curr_state = cl->next;
}


//	processes a script set command
void process_set(Script *sc, Trigger *trig, char *cmd, int varspace) {
	char 	*name = get_buffer(MAX_INPUT_LENGTH),
			*value;
	LList<TrigVarData *>*varlist = NULL;
	TrigVarData *vd = NULL;
	
	switch (varspace) {
		case VarSpace::Global:	varlist = &sc->variables;		break;
		case VarSpace::Shared:	varlist = &Trigger::Index[trig->Virtual()].variables;	break;
		default:				varlist = &trig->variables;		break;
	}
	
	value = any_one_arg(cmd, name);
	
	if (!*name)
		mudlogf(NRM, NULL, FALSE, "SCRIPTERR: Trigger: \"%s\" [%d]: set w/o an arg: 'set %s'",
			SSData(GET_TRIG_NAME(trig)), GET_TRIG_VNUM(trig), cmd);
	else {
		vd = search_for_variable(sc, trig, name);
		
		if (vd) {
			if (vd->value)	free(vd->value);
			vd->value = str_dup(value);
		} else
			add_var(*varlist, name, value, sc->context);
	}
	release_buffer(name);
}


//	processes a script eval command
void process_eval(Scriptable * go, Script *sc, Trigger *trig, char *cmd, int varspace) {
	char	*name = get_buffer(MAX_INPUT_LENGTH),
			*result = get_buffer(MAX_INPUT_LENGTH),
			*expr;
	LList<TrigVarData *>*varlist = NULL;
	TrigVarData *vd = NULL;
	
	switch (varspace) {
		case VarSpace::Global:	varlist = &sc->variables;		break;
		case VarSpace::Shared:	varlist = &Trigger::Index[trig->Virtual()].variables;	break;
		default:				varlist = &trig->variables;		break;
	}
	
	expr = one_argument(cmd, name);

	if (!*name)
		mudlogf(NRM, NULL, FALSE, "SCRIPTERR: Trigger: \"%s\" [%d]: eval w/o an arg: '%s'",
				SSData(GET_TRIG_NAME(trig)), GET_TRIG_VNUM(trig), cmd);
	else {
		eval_expr(expr, result, go, sc, trig);
		
		vd = search_for_variable(sc, trig, name);
		
		if (vd) {
			if (vd->value)	free(vd->value);
			vd->value = str_dup(result);
		} else
			add_var(*varlist, name, result, sc->context);
	}
	release_buffer(name);
	release_buffer(result);
}


void process_pushfront(Script *sc, Trigger *trig, char *cmd) {
	char *name = get_buffer(MAX_INPUT_LENGTH),
		*newval = NULL;
	char *value;
	TrigVarData *vd = NULL;
	
	value = one_argument(cmd, name);
	
	if (!*name) {
		mudlogf(NRM, NULL, FALSE, "SCRIPTERR: Trigger: \"%s\" [%d]: pushfront w/o an arg: '%s'",
				SSData(GET_TRIG_NAME(trig)), GET_TRIG_VNUM(trig), cmd);
	} else {
		vd = search_for_variable(sc, trig, name);
		if (vd) {
			newval = get_buffer((vd->value ? (strlen(vd->value) + 1) : 0) + strlen(value) + 3);
			strcpy(newval, value);
			if (vd->value) {
				if (*vd->value)	strcat(newval, " ");
				strcat(newval, vd->value);
				free(vd->value);
			}
			vd->value = str_dup(newval);
		} else
			add_var(trig->variables, name, value, sc->context);
	}
	release_buffer(name);
	if (newval)		release_buffer(newval);
}


void process_popfront(Script *sc, Trigger *trig, char *cmd) {
	char *popped = get_buffer(MAX_INPUT_LENGTH),
		*name = get_buffer(MAX_INPUT_LENGTH),
		*newval = NULL;
	char *popvar;
	TrigVarData *vd = NULL;
	
	popvar = one_argument(cmd, name);
	
	if (!*name) {
		mudlogf(NRM, NULL, FALSE, "SCRIPTERR: Trigger: \"%s\" [%d]: popfront w/o an arg: '%s'",
				SSData(GET_TRIG_NAME(trig)), GET_TRIG_VNUM(trig), cmd);
	} else {
		vd = search_for_variable(sc, trig, name);
		if (vd) {
			newval = get_buffer(strlen(vd->value) + 3);
			half_chop(vd->value, popped, newval);
			free(vd->value);
			
			vd->value = str_dup(newval);
			
			if (popvar && *popvar)
				add_var(trig->variables, popvar, popped, sc->context);
		}
	}
	release_buffer(popped);
	release_buffer(name);
	if (newval)		release_buffer(newval);
}


void process_pushback(Script *sc, Trigger *trig, char *cmd) {
	char		*name = get_buffer(MAX_INPUT_LENGTH),
				*newval = NULL;
	char		*value;
	TrigVarData *vd = NULL;
	
	value = one_argument(cmd, name);
	
	if (!*name) {
		mudlogf(NRM, NULL, FALSE, "SCRIPTERR: Trigger: \"%s\" [%d]: pushfront w/o an arg: '%s'",
				SSData(GET_TRIG_NAME(trig)), GET_TRIG_VNUM(trig), cmd);
	} else {
		vd = search_for_variable(sc, trig, name);
		if (vd) {
			newval = get_buffer((vd->value ? strlen(vd->value) : 0) + strlen(value) + 3);
			if (vd->value) {
				strcpy(newval, vd->value);
				free(vd->value);
			}
			if (*newval)	strcat(newval, " ");
			strcat(newval, value);
			vd->value = str_dup(newval);
		} else
			add_var(trig->variables, name, value, sc->context);
	}
	release_buffer(name);
	if (newval)		release_buffer(newval);
}


void process_popback(Script *sc, Trigger *trig, char *cmd) {
	char *name = get_buffer(MAX_INPUT_LENGTH),
		*newval = NULL;
	char *popvar, *p, *popped = NULL;
	int place;
	TrigVarData *vd = NULL;
	
	popvar = one_argument(cmd, name);
	
	if (!*name) {
		mudlogf(NRM, NULL, FALSE, "SCRIPTERR: Trigger: \"%s\" [%d]: popfront w/o an arg: '%s'",
				SSData(GET_TRIG_NAME(trig)), GET_TRIG_VNUM(trig), cmd);
	} else {
		vd = search_for_variable(sc, trig, name);
		if (vd) {
			if (vd->value) {
				newval = get_buffer(strlen(vd->value));
				strcpy(newval, vd->value);
				free(vd->value);
			}
			
			p = newval;
			place = strlen(p) - 1;
			p += place;
			
			while (place) {
				if (*p && (*p != ' '))
					break;
				--p;
				--place;
			}
			
			while (place) {
				if (*p && (*p == ' '))
					break;
				--p;
				--place;
			}
			
			if (place) {
				popped = p + 1;
				*p = '\0';
				skip_spaces(newval);
				vd->value = str_dup(newval);
			} else
				vd->value = str_dup("");
			
			if (popvar && *popvar)
				add_var(trig->variables, popvar, popped ? popped : "", sc->context);
		}
	}
	release_buffer(name);
	if (newval)		release_buffer(newval);
}


void process_pop(Script *sc, Trigger *trig, char *cmd)
{
#if 0
	skip_spaces(var);
	skip_spaces(subfield);

	half_chop(var, buf, buf2);

	while (*buf) {
		if (str_str(subfield, buf))
	    	return;
		half_chop(buf2, buf, buf2);
	}

	return 0;


	char *name = get_buffer(MAX_INPUT_LENGTH),
		*newval = NULL;
	char *popvar, *p, *popped = NULL;
	int place;
	TrigVarData *vd = NULL;
	
	popvar = one_argument(cmd, name);
	
	if (!*name) {
		mudlogf(NRM, NULL, FALSE, "SCRIPTERR: Trigger: \"%s\" [%d]: pop w/o an arg: '%s'",
				SSData(GET_TRIG_NAME(trig)), GET_TRIG_VNUM(trig), cmd);
	} else {
		vd = search_for_variable(sc, trig, name);
		if (vd) {
			if (vd->value) {
				newval = get_buffer(strlen(vd->value));
				strcpy(newval, vd->value);
				free(vd->value);
			}
			
			p = newval;
			place = strstr(p) - 1;
			p += place;
			
			while (place) {
				if (*p && (*p != ' '))
					break;
				--p;
				--place;
			}
			
			while (place) {
				if (*p && (*p == ' '))
					break;
				--p;
				--place;
			}
			
			if (place) {
				popped = p + 1;
				*p = '\0';
				skip_spaces(newval);
				vd->value = str_dup(newval);
			} else
				vd->value = str_dup("");
			
			if (popvar && *popvar)
				add_var(trig->variables, popvar, popped ? popped : "", sc->context);
		}
	}
	release_buffer(name);
	if (newval)		release_buffer(newval);
#endif
}


/* create a UID variable from the id number */
void makeuid_var(Scriptable * go, Script *sc, Trigger *trig, const char *cmd) {
	char *result, *uid_p;
	char *buf = get_buffer(MAX_INPUT_LENGTH);
	char *varname = get_buffer(MAX_INPUT_LENGTH);
	
	uid_p = one_argument(cmd, varname);
	skip_spaces(uid_p);

	if (!*varname) {
		mudlogf(NRM, NULL, FALSE, "SCRIPTERR: Trigger: \"%s\" [%d]: makeuid w/o an arg: '%s'",
				SSData(GET_TRIG_NAME(trig)), GET_TRIG_VNUM(trig), cmd);
	} else if (!*uid_p || !atoi(uid_p)) {
		mudlogf(NRM, NULL, FALSE, "SCRIPTERR: Trigger: \"%s\" [%d]: makeuid invalid id arg: '%s'",
				SSData(GET_TRIG_NAME(trig)), GET_TRIG_VNUM(trig), cmd);
	} else {
		result = get_buffer(MAX_INPUT_LENGTH);
		eval_expr(uid_p, result, go, sc, trig);
		sprintf(buf, "%c%s", UID_CHAR, result);
		add_var(trig->variables, varname, buf, sc->context);
	}
	
	release_buffer(varname);
	release_buffer(buf);
}


 /*
 * copy a locally owned variable to the globals of another script
 *     'remote <variable_name> <uid>'
 */
void process_remote(Script *sc, Trigger *trig, const char *cmd) {
	TrigVarData *	vd;
	Script *	sc_remote=NULL;
	SInt32			uid;
	Room *		room;
	Character *		mob;
	Object *		obj;
	
	char *var = get_buffer(MAX_INPUT_LENGTH);
	char *uid_p;
	
	uid_p = one_argument(cmd, var);

	if (!*var || !*uid_p) {
		mudlogf(NRM, NULL, FALSE, "SCRIPTERR: Trigger: \"%s\" [%d]: remote: invalid arguments '%s'",
			SSData(GET_TRIG_NAME(trig)), GET_TRIG_VNUM(trig), cmd);
	} else {
		/* find the locally owned variable */
		vd = search_for_variable(sc, trig, var);
		
		if (!vd)
			mudlogf(NRM, NULL, FALSE, "SCRIPTERR: Trigger: \"%s\" [%d]: local var '%s' not found in remote call",
					SSData(GET_TRIG_NAME(trig)), GET_TRIG_VNUM(trig), var);
		else if (!*uid_p || (uid = atoi(uid_p)) <= 0)		//	find the target script from the uid number
			mudlogf(NRM, NULL, FALSE, "SCRIPTERR: Trigger: \"%s\" [%d]: remote: illegal uid '%s'",
					SSData(GET_TRIG_NAME(trig)), GET_TRIG_VNUM(trig), uid_p);
		else if (uid < ROOM_ID_BASE)
			mudlogf(NRM, NULL, FALSE, "SCRIPTERR: Trigger: \"%s\" [%d]: remote: uid '%d' is a PC?",
					SSData(GET_TRIG_NAME(trig)), GET_TRIG_VNUM(trig), uid);
		else if ((room = find_room(uid)))	sc_remote = SCRIPT(room);
		else if ((mob = find_char(uid)))	sc_remote = SCRIPT(mob);
		else if ((obj = find_obj(uid)))		sc_remote = SCRIPT(obj);
		else
			mudlogf(NRM, NULL, FALSE, "SCRIPTERR: Trigger: \"%s\" [%d]: remote: uid '%d' invalid",
					SSData(GET_TRIG_NAME(trig)), GET_TRIG_VNUM(trig), uid);
		if (sc_remote)
			add_var(sc_remote->variables, vd->name, vd->value, vd->context);
	}

	release_buffer(var);
	release_buffer(uid_p);
}



//	processes a script return command.
//	returns the new value for the script to return.
int process_return(Trigger *trig, char *cmd) {
	int result;
	
	skip_spaces(cmd);
	
	if (!*cmd) {
		mudlogf(NRM, NULL, FALSE, "SCRIPTERR: Trigger: \"%s\" [%d]: return w/o an arg: 'return %s'",
				SSData(GET_TRIG_NAME(trig)), GET_TRIG_VNUM(trig), cmd);
		result = 1;
	} else
		result = atoi(cmd);
	
	return result;
}


//	removes a variable from the global vars of sc,
//	or the local vars of trig if not found in global list.
void process_unset(Script *sc, Trigger *trig, char *cmd) {
	char *var = get_buffer(MAX_INPUT_LENGTH);
	
	cmd = any_one_arg(cmd, var);
	
	if (!*var)
		mudlogf(NRM, NULL, FALSE, "SCRIPTERR: Trigger: \"%s\" [%d]: unset w/o an arg: 'unset %s'",
				SSData(GET_TRIG_NAME(trig)), GET_TRIG_VNUM(trig), cmd);
	else {
		while (*var) {
			TrigVarData *vd = extract_variable(sc, trig, var);
			delete vd;
			cmd = any_one_arg(cmd, var);
		}
	}
	
	release_buffer(var);
}


//	makes a local variable into a global variable
void process_global(Script *sc, Trigger *trig, char *cmd, SInt32 id) {
	TrigVarData *vd;

	skip_spaces(cmd);
	
	if (!*cmd)
		mudlogf(NRM, NULL, FALSE, "SCRIPTERR: Trigger: \"%s\" [%d]: global w/o an arg: 'global %s'",
				SSData(GET_TRIG_NAME(trig)), GET_TRIG_VNUM(trig), cmd);
	else {
		vd = extract_variable(sc, trig, cmd, 5);

		if (vd)
			add_var(sc->variables, vd->name, vd->value, id);
		else if (!search_for_variable(sc, trig, cmd, 2))
			mudlogf(NRM, NULL, FALSE, "SCRIPTERR: Trigger: \"%s\" [%d]: local var 'global %s' not found in global call",
					SSData(GET_TRIG_NAME(trig)), GET_TRIG_VNUM(trig), cmd);
	}
}


//	makes a local variable into a shared variable
void process_shared(Script *sc, Trigger *trig, char *cmd, SInt32 id) {
	TrigVarData *vd;

	skip_spaces(cmd);
	
	if (!*cmd)
		mudlogf(NRM, NULL, FALSE, "SCRIPTERR: Trigger: \"%s\" [%d]: share w/o an arg: 'global %s'",
				SSData(GET_TRIG_NAME(trig)), GET_TRIG_VNUM(trig), cmd);
	else {
		vd = extract_variable(sc, trig, cmd, 3);

		if (vd)
			add_var(Trigger::Index[trig->Virtual()].variables, vd->name, vd->value, id);
		else if (!search_for_variable(sc, trig, cmd, 4))
			mudlogf(NRM, NULL, FALSE, "SCRIPTERR: Trigger: \"%s\" [%d]: local var 'global %s' not found in shared call",
					SSData(GET_TRIG_NAME(trig)), GET_TRIG_VNUM(trig), cmd);
	}
}


/* set the current context for a script */
void process_context(Script *sc, Trigger *trig, const char *cmd) {
	skip_spaces(cmd);
	
	if (!*cmd) {
		mudlogf(NRM, NULL, FALSE, "SCRIPTERR: Trigger: \"%s\" [%d]: context w/o an arg: '%s'",
				SSData(GET_TRIG_NAME(trig)), GET_TRIG_VNUM(trig), cmd);
	} else
		sc->context = atoi(cmd);
}


void process_attach(Scriptable * go, char *argument, VNum trigger)
{
	Character *targ_ch;
	Object *targ_obj;
	Room *targ_room;
	Script **targ_script = NULL;
	Trigger *trig, *t;
	Type	attachtype = Datatypes::Undefined;

	char *p = argument;
	char type[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH], trigname[MAX_INPUT_LENGTH];

	int tn;

	p = two_arguments(argument, type, name);
	one_argument(p, trigname);

	if (!*type || !*trigname || !*name) {
		if (!*type)	{
			mudlogf(NRM, NULL, FALSE, "SCRIPTERR: Trigger: \"%s\" [%d]: attach missing mob|obj|room: '%s'",
					SSData(GET_TRIG_NAME(trig)), GET_TRIG_VNUM(trig), argument);
		}
		if (!*name)	{
			mudlogf(NRM, NULL, FALSE, "SCRIPTERR: Trigger: \"%s\" [%d]: attach missing target name/ID: '%s'",
					SSData(GET_TRIG_NAME(trig)), GET_TRIG_VNUM(trig), argument);
		}
		if (!*trigname)	{
			mudlogf(NRM, NULL, FALSE, "SCRIPTERR: Trigger: \"%s\" [%d]: attach missing trigname vnum: '%s'",
					SSData(GET_TRIG_NAME(trig)), GET_TRIG_VNUM(trig), argument);
		}
		return;	 
	}

	tn = atoi(trigname);

	if (!Trigger::Find(tn)) {
		sprintf(buf, "SCRIPTERR: Trigger: \"%s\" [%d]: attach: trigname %d does not exist.", 
	    	SSData(GET_TRIG_NAME(trig)), GET_TRIG_VNUM(trig), tn);
		mudlogf(NRM, NULL, FALSE, buf);
		return;
	}

	if (!str_cmp(type, "mob")) {
		if ((targ_ch = go->TargetCharRoom(name))) {
			targ_script = &SCRIPT(targ_ch);
			attachtype = Datatypes::Character;
		} else {
			sprintf(buf, "SCRIPTERR: Trigger: \"%s\" [%d]: attach: target mob '%s' does not exist.", 
	        	SSData(GET_TRIG_NAME(trig)), GET_TRIG_VNUM(trig), name);
			mudlogf(NRM, NULL, FALSE, buf);
			return;
		}
	}
	else if (!str_cmp(type, "obj")) {
		if ((targ_obj = go->TargetObjList(name, world[go->InRoom()].contents))) {
			targ_script = &SCRIPT(targ_obj);
			attachtype = Datatypes::Object;
		} else {
			sprintf(buf, "SCRIPTERR: Trigger: \"%s\" [%d]: attach: target obj '%s' does not exist.", 
	        	SSData(GET_TRIG_NAME(trig)), GET_TRIG_VNUM(trig), name);
			mudlogf(NRM, NULL, FALSE, buf);
			return;
		}
	}
	else if (!str_cmp(type, "room")) {
		if ((targ_room = go->TargetRoom(name))) {
			targ_script = &SCRIPT(targ_room);
			attachtype = Datatypes::Room;
		} else {
			sprintf(buf, "SCRIPTERR: Trigger: \"%s\" [%d]: attach: target room '%s' does not exist.", 
	        	SSData(GET_TRIG_NAME(trig)), GET_TRIG_VNUM(trig), name);
			mudlogf(NRM, NULL, FALSE, buf);
			return;
		}
	}
	
	if (!targ_script)
		return;

	if (*targ_script) {
		START_ITER(iter, t, Trigger *, TRIGGERS(*targ_script)) {
			if (GET_TRIG_VNUM(t) == tn) {
				// sprintf(buf, "attach: trigname '%d' already attached to target.", tn);
				// script_log(thing, buf);
				return;
			}
		}
	} else {
		*targ_script = new Script(attachtype);
	}
	
	trig = Trigger::Read(tn);
	add_trigger(*targ_script, trig);
}


// Return true if the script detaches itself
bool process_detach(Scriptable * go, char *argument, VNum trigger)
{
	Character *targ_ch = NULL;
	Object *targ_obj = NULL;
	Room *targ_room = NULL;
	Script **targ_script = NULL;
	Trigger *trig;
	int tn;
	bool purgeself = false;

	char type[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH], trigname[MAX_INPUT_LENGTH];

	argument = two_arguments(argument, type, name);
	one_argument(argument, trigname);

	if (!*type || !*trigname || !*name) {
		if (!*type)	{
			mudlogf(NRM, NULL, FALSE, "SCRIPTERR: Trigger: \"%s\" [%d]: detach missing mob|obj|room: '%s'",
					SSData(GET_TRIG_NAME(trig)), GET_TRIG_VNUM(trig), argument);
		}
		if (!*name)	{
			mudlogf(NRM, NULL, FALSE, "SCRIPTERR: Trigger: \"%s\" [%d]: detach missing target name/ID: '%s'",
					SSData(GET_TRIG_NAME(trig)), GET_TRIG_VNUM(trig), argument);
		}
		if (!*trigname)	{
			mudlogf(NRM, NULL, FALSE, "SCRIPTERR: Trigger: \"%s\" [%d]: detach missing trigname vnum: '%s'",
					SSData(GET_TRIG_NAME(trig)), GET_TRIG_VNUM(trig), argument);
		}
		return false;	 
	}

	if (!str_cmp(type, "mob")) {
		if ((targ_ch = go->TargetCharRoom(name))) {
			targ_script = &SCRIPT(targ_ch);

		} else {
			sprintf(buf, "SCRIPTERR: Trigger: \"%s\" [%d]: detach: target mob '%s' does not exist.", 
	        	SSData(GET_TRIG_NAME(trig)), GET_TRIG_VNUM(trig), name);
			mudlogf(NRM, NULL, FALSE, buf);
			return false;
		}
	}
	else if (!str_cmp(type, "obj")) {
		if ((targ_obj = go->TargetObjList(type, world[go->InRoom()].contents))) {
			targ_script = &SCRIPT(targ_obj);
		} else {
			sprintf(buf, "SCRIPTERR: Trigger: \"%s\" [%d]: detach: target obj '%s' does not exist.", 
	        	SSData(GET_TRIG_NAME(trig)), GET_TRIG_VNUM(trig), name);
			mudlogf(NRM, NULL, FALSE, buf);
			return false;
		}
	}
	else if (!str_cmp(arg, "room")) {
		if ((targ_room = go->TargetRoom(name))) {
			targ_script = &SCRIPT(targ_room);
		} else {
			sprintf(buf, "SCRIPTERR: Trigger: \"%s\" [%d]: detach: target room '%s' does not exist.", 
	        	SSData(GET_TRIG_NAME(trig)), GET_TRIG_VNUM(trig), name);
			mudlogf(NRM, NULL, FALSE, buf);
			return false;
		}
	}

	if (targ_script && !*targ_script) {
		// sprintf(buf, "detach: Target '%s' does not have any trignames to remove.", name);
		// script_log(thing, buf);
		return false;
	}
	
	if (tn == trigger)		purgeself = true;

	if (!str_cmp(trigname, "all")) {
		(*targ_script)->Extract();
		*targ_script = NULL;
	} else {
		remove_trigger(*targ_script, trigname);
	}
	
	return purgeself;
}


void extract_value(Script *sc, Trigger *trig, const char *cmd) {
	char *		buf = get_buffer(MAX_INPUT_LENGTH);
	char *		name = get_buffer(MAX_INPUT_LENGTH);
	SInt32		num;
	
	cmd = any_one_arg(any_one_arg(cmd, name), buf);

	num = atoi(buf);
	if (num < 1) {
		mudlogf(NRM, NULL, FALSE, "SCRIPTERR: Trigger: \"%s\" [%d]: extract number < 1!",
				SSData(GET_TRIG_NAME(trig)), GET_TRIG_VNUM(trig));
	} else {
		while (num > 0) {
			cmd = any_one_arg(cmd, buf);
			num--;
		}
		add_var(trig->variables, name, buf, sc->context);
	}
	release_buffer(name);
	release_buffer(buf);
}


/*  This is the core driver for scripts. */
int script_driver(Scriptable *go, Trigger *trig, int mode) {
	static int depth = 0;
	int ret_val = 1;
	CmdlistElement *cl, *temp;
	char *cmd, *loopp, *p;
	Script *sc = NULL;
	bool interpreted = true,
		halted = false,
		breakhit = false,
		createdsc = false;
	int linetype = CommandSwitch::None;

//	void obj_command_interpreter(Object *obj, char *argument);
//	void wld_command_interpreter(Room *room, char *argument);
	void script_command_interpreter(Scriptable * go, char *argument, int cmd, VNum trigger);

	if (depth > MAX_SCRIPT_DEPTH) {
		mudlogf(NRM, NULL, FALSE, "SCRIPTERR: Triggers recursed beyond maximum allowed depth.");
		return ret_val;
	}

	if (PURGED(go))
		return 0;
	
	sc = SCRIPT(go);
	
	if (!sc) {
		sc = new Script(go->DataType());
		SCRIPT(go) = sc;
		createdsc = true;
	}
		

	depth++;
	cmd = get_buffer(MAX_INPUT_LENGTH);

	if (mode == TRIG_NEW) {
		GET_TRIG_LOOPS(trig) = 0;
		GET_TRIG_DEPTH(trig) = 1;
		sc->context = 0;
		for (cl = trig->cmdlist->command; cl; cl = cl->next)
			cl->loops = 0;
	}
	
	for (cl = (mode == TRIG_NEW) ? trig->cmdlist->command : trig->curr_state;
			!halted && cl && GET_TRIG_DEPTH(trig); cl = cl->next) {
/*		It purges the trig if the owner is purged, right?  If so, no need for this check
		switch (type) {
			case MOB_TRIGGER:
				if (PURGED((Character *)go))	return 0;
				sc = SCRIPT((Character *) go);
				break;
			case OBJ_TRIGGER:
				if (PURGED((Object *)go))	return 0;
				sc = SCRIPT((Object *) go);
				break;
			case WLD_TRIGGER:
				sc = SCRIPT((Room *) go);
				break;
			default:
				return 0;
		}
*/
		if (PURGED(sc))		break;
		if (PURGED(trig))	break;
		
		linetype = cl->type;
		interpreted = true;
		breakhit = false;
		
		if (linetype == CommandSwitch::Comment)
			continue;
		
		p = cl->cmd;
		skip_spaces(p);
		
		if (!*p)										continue;
		
		switch (linetype) {
			case CommandSwitch::If:
				if (process_if(p + 3, go, sc, trig))		GET_TRIG_DEPTH(trig)++;
				else										cl = find_else_end(trig, cl, go, sc);
				break;
			case CommandSwitch::ElseIf:
			case CommandSwitch::Else:
				cl = find_end(cl);
				GET_TRIG_DEPTH(trig)--;
				break;
			case CommandSwitch::While:
				temp = find_done(cl);
				if (process_if(p + 6, go, sc, trig))		temp->original = cl;
				else {
					cl->loops = 0;
					cl = temp;
				}
				break;
			case CommandSwitch::Foreach:
				temp = find_done(cl);
				if (process_foreach(p + 8, go, sc, trig, cl->loops)) {
					temp->original = cl;
					cl->loops += 1;
				} else {
					cl->loops = 0;
					cl = temp;
				}
				break;
			case CommandSwitch::Switch:
				cl = find_case(trig, cl, go, sc, p + 7);
				break;
			case CommandSwitch::Done:
				if (cl->original) {
					loopp = cl->original->cmd;
					skip_spaces(loopp);
					
					if (((cl->original->type == CommandSwitch::While) &&
							process_if(loopp + 6, go, sc, trig)) ||
							((cl->original->type == CommandSwitch::Foreach) &&
							process_foreach(loopp + 8, go, sc, trig, cl->original->loops))) {
						cl = cl->original;
						cl->loops += 1;
						++GET_TRIG_LOOPS(trig);
						
						if ((cl->loops % 15) == 0) {
							process_wait(go, trig, "3 p", cl, 0);
							depth--;
							release_buffer(cmd);
							return ret_val;
						}
						
						if (GET_TRIG_LOOPS(trig) == 100)
							mudlogf(NRM, NULL, TRUE, "SCRIPTERR: Trigger \"%s\" [%d] has looped %d times!!!",
									SSData(GET_TRIG_NAME(trig)), GET_TRIG_VNUM(trig), GET_TRIG_LOOPS(trig));
					}
				}
				break;
			case CommandSwitch::Break:
				cl = find_done(cl);
				if (cl->original)	cl->original->loops = 0;
				breakhit = true;
				break;
			
			case CommandSwitch::Jump:
				if (!cl->original) {
					for (temp = trig->cmdlist->command; temp; temp = temp->next) {
						if (temp->type != CommandSwitch::Label)	continue;
						char *t = temp->cmd;
						skip_spaces(t);
						if (!str_cmp(p + 5, t + 6))
							break;
					}
					cl->original = temp;
				}
				if (cl->original) {
					cl = cl->original;
					if (cl->loops++ > 0) {
						process_wait(go, trig, "3 p", cl, 0);
						depth--;
						release_buffer(cmd);
						return ret_val;
					}
				} else
					mudlogf(NRM, NULL, TRUE, "SCRIPTERR: Trigger \"%s\" [%d]: label not found",
							SSData(GET_TRIG_NAME(trig)), GET_TRIG_VNUM(trig));
				break;
				
			case CommandSwitch::Label:
				break;
				
			case CommandSwitch::Case:
			case CommandSwitch::End:
			case CommandSwitch::Default:
				break;
				
			default:
				interpreted = false;
				break;
		}

		if (breakhit)
			continue;
		
		if (!interpreted) {
			var_subst(go, sc, trig, p, cmd);

			if (!linetype)	linetype = lookup_script_line(cmd);
			
			switch (linetype) {
				case CommandSwitch::Context:		process_context(sc, trig, cmd + 8);			break;
//				case CommandSwitch::Delay:			process_delay(go, trig, cmd, cl, 0);		break;
				case CommandSwitch::Eval:			process_eval(go, sc, trig, cmd + 5, VarSpace::Local);	break;
				case CommandSwitch::EvalGlobal:		process_eval(go, sc, trig, cmd + 11, VarSpace::Global);	break;
				case CommandSwitch::EvalShared:		process_eval(go, sc, trig, cmd + 11, VarSpace::Shared);	break;
				case CommandSwitch::Extract:		extract_value(sc, trig, cmd + 8);			break;
				case CommandSwitch::Global:			process_global(sc, trig, cmd + 7, sc->context);	break;
				case CommandSwitch::Halt:			halted = true;								break;
				case CommandSwitch::MakeUID:		makeuid_var(go, sc, trig, cmd + 8);			break;
				case CommandSwitch::Pushfront:		process_pushfront(sc, trig, cmd + 10);		break;
				case CommandSwitch::Popfront:		process_popfront(sc, trig, cmd + 9);		break;
				case CommandSwitch::Pushback:		process_pushback(sc, trig, cmd + 9);		break;
				case CommandSwitch::Popback:		process_popback(sc, trig, cmd + 8);			break;
				case CommandSwitch::Pop:			process_pop(sc, trig, cmd + 4);				break;
				case CommandSwitch::Remote:			process_remote(sc, trig, cmd + 7);			break;
				case CommandSwitch::Return:
					ret_val = process_return(trig, cmd + 7);
					//halted = true;
					break;

				case CommandSwitch::Attach:			
	            	process_attach(go, cmd + 7, trig->Virtual());
	                break;
				case CommandSwitch::Detach:			
	            	if (process_detach(go, cmd + 7, trig->Virtual())) {
						return ret_val;
					}
		        	break;

				case CommandSwitch::Set:			process_set(sc, trig, cmd + 4);					break;
				case CommandSwitch::SetGlobal:		process_set(sc, trig, cmd + 10, VarSpace::Global);	break;
				case CommandSwitch::SetShared:		process_set(sc, trig, cmd + 10, VarSpace::Shared);	break;
				case CommandSwitch::Shared:			process_shared(sc, trig, cmd + 7, sc->context);	break;
				case CommandSwitch::Unset:			process_unset(sc, trig, cmd + 6);			break;
				case CommandSwitch::Wait:
					process_wait(go, trig, cmd + 5, cl, 0);
					depth--;
					release_buffer(cmd);
					return ret_val;
				case CommandSwitch::Command:		script_command_interpreter(go, cmd, cl->number, trig->Virtual());	break;
				case CommandSwitch::PCCommand:
				default:
					if (go->DataType() == Datatypes::Character)
						command_interpreter((Character *)go, cmd);
					break;
			}
		}
	}
	release_buffer(cmd);
	
	TrigVarData *	var;
	LListIterator<TrigVarData *>	variables(trig->variables);
	while ((var = variables.Next())) {
		trig->variables.Remove(var);
		delete var;
	}
	GET_TRIG_DEPTH(trig) = 0;

	depth--;

	//	Trigger needs to be brought up to date
	if (trig->Virtual() != -1 && trig->cmdlist != Trigger::Index[trig->Virtual()].proto->cmdlist) {
		Trigger *proto = Trigger::Index[trig->Virtual()].proto;
		SSFree(trig->arglist);
		SSFree(trig->name);
	
		trig->arglist = SSShare(proto->arglist);
		trig->name = SSShare(proto->name);
		
		trig->cmdlist->Free();
		trig->cmdlist = Cmdlist::Share(proto->cmdlist);
		trig->trigger_type_mob = proto->trigger_type_mob;
		trig->trigger_type_obj = proto->trigger_type_obj;
		trig->trigger_type_wld = proto->trigger_type_wld;
		trig->narg = proto->narg;
		trig->data_type = proto->data_type;
	}

	if (createdsc) {
		SCRIPT(go) = NULL;
		delete sc;
	}

	return ret_val;
}
