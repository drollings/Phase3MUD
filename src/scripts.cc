/***************************************************************************\
[*]    __     __  ___                ___  | LexiMUD is a C++ MUD codebase [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ |        for Sci-Fi MUDs        [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / | Copyright(C) 1997-1999        [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  | All rights reserved           [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   | Chris Jacobson (FearItself)   [*]
[*]      A Creation of the AvP Team!      | fear@technologist.com         [*]
[-]---------------------------------------+-------------------------------[-]
[*] File : scripts.c++                                                    [*]
[*] Usage: Script and Trigger classes                                     [*]
\***************************************************************************/



#include "scripts.h"
#include "interpreter.h"
#include "buffer.h"
#include "utils.h"
#include "files.h"
#include "event.h"



LList<Trigger *>	Triggers;
LList<Trigger *>	PurgedTrigs;		   /* list of mtrigs to be deleted  */
LList<Script *>		PurgedScripts; /* list of mob scripts to be del */
Index<Trigger>		Trigger::Index;


int lookup_script_line(char *p);


int lookup_script_line(char *p) {
	int retval = CommandSwitch::None;
	
	switch (*p) {
		case '*':
			retval = CommandSwitch::Comment;
			break;
		
		case 'a':
			if (!strn_cmp(p, "attach", 6))				retval = CommandSwitch::Attach;
			break;
		
		case 'b':
			if (!strn_cmp(p, "break", 5))				retval = CommandSwitch::Break;
			break;
		
		case 'c':
			if (!strn_cmp(p, "case ", 5))				retval = CommandSwitch::Case;
			else if (!strn_cmp(p, "context ", 8))		retval = CommandSwitch::Context;
			break;
		
		case 'd':
			if (!strn_cmp(p, "done", 4))				retval = CommandSwitch::Done;
//			else if (!strn_cmp(p, "delay ", 6))			retval = CommandSwitch::Delay;
			else if (!strn_cmp(p, "default", 7))		retval = CommandSwitch::Default;
			else if (!strn_cmp(p, "detach", 6))		retval = CommandSwitch::Detach;
			break;
		
		case 'e':
			if (!strn_cmp(p, "end", 3))					retval = CommandSwitch::End;
			else if (!strn_cmp(p, "elseif ", 7))		retval = CommandSwitch::ElseIf;
			else if (!strn_cmp(p, "else", 4))			retval = CommandSwitch::Else;
			else if (!strn_cmp(p, "eval ", 5))			retval = CommandSwitch::Eval;
			else if (!strn_cmp(p, "evalglobal ", 11))	retval = CommandSwitch::EvalGlobal;
			else if (!strn_cmp(p, "evalshared ", 11))	retval = CommandSwitch::EvalShared;
			else if (!strn_cmp(p, "extract ", 8))		retval = CommandSwitch::Extract;
			break;
		
		case 'f':
			if (!strn_cmp(p, "foreach ", 8))			retval = CommandSwitch::Foreach;
			break;
		
		case 'g':
			if (!strn_cmp(p, "global ", 7))				retval = CommandSwitch::Global;
			break;
		
		case 'h':
			if (!strn_cmp(p, "halt", 4))				retval = CommandSwitch::Halt;
			break;
		
		case 'i':
			if (!strn_cmp(p, "if ", 3))					retval = CommandSwitch::If;
			break;
		
		case 'j':
			if (!strn_cmp(p, "jump ", 5))				retval = CommandSwitch::Jump;
//			else if (!strn_cmp(p, "jumpsub ", 8))		retval = CommandSwitch::JumpSub;
			break;
		
		case 'l':
			if (!strn_cmp(p, "label ", 6))				retval = CommandSwitch::Label;
			break;
		
		case 'm':
			if (!strn_cmp(p, "makeuid ", 8))			retval = CommandSwitch::MakeUID;
			break;
			
		case 'p':
			if (!strn_cmp(p, "pop ", 4))				retval = CommandSwitch::Pop;
			else if (!strn_cmp(p, "popback ", 8))		retval = CommandSwitch::Popback;
			else if (!strn_cmp(p, "popfront ", 9))		retval = CommandSwitch::Popfront;
			else if (!strn_cmp(p, "pushback ", 9))		retval = CommandSwitch::Pushback;
			else if (!strn_cmp(p, "pushfront ", 10))	retval = CommandSwitch::Pushfront;
			break;
		
		case 'r':
			if (!strn_cmp(p, "remote ", 7))				retval = CommandSwitch::Remote;
			else if (!strn_cmp(p, "return ", 7))		retval = CommandSwitch::Return;
			break;
		
		case 's':
			if (!strn_cmp(p, "set ", 4))				retval = CommandSwitch::Set;
			else if (!strn_cmp(p, "switch ", 7))		retval = CommandSwitch::Switch;
			else if (!strn_cmp(p, "setglobal ", 10))	retval = CommandSwitch::SetGlobal;
			else if (!strn_cmp(p, "setshared ", 10))	retval = CommandSwitch::SetShared;
			else if (!strn_cmp(p, "shared ", 7))		retval = CommandSwitch::Shared;
			break;
		
		case 'u':
			if (!strn_cmp(p, "unset ", 6))				retval = CommandSwitch::Unset;
			break;
		
		case 'w':
			if (!strn_cmp(p, "wait ", 5))				retval = CommandSwitch::Wait;
			else if (!strn_cmp(p, "while ", 6))			retval = CommandSwitch::While;
			break;
	}
	
	return retval;
}


CmdlistElement::CmdlistElement() : cmd(NULL), original(NULL), next(NULL) {
}


CmdlistElement::CmdlistElement(const char *string) : cmd(string ? str_dup(string) : NULL),
		original(NULL), next(NULL), loops(0), type(0), number(-1) {
	int cmdnum = 0;
	char *p = cmd;
	char *buf1, *buf2;
	
	if (!cmd)	return;
	
	skip_spaces(p);
	
	if (*p == '%')	return;
	
	type = lookup_script_line(p);
	if (type)
		return;
	
	buf1 = get_buffer(MAX_INPUT_LENGTH);
	buf2 = get_buffer(MAX_INPUT_LENGTH);
	
	half_chop(p, buf2, buf1);
	
	for (cmdnum = 0; *scriptCommands[cmdnum].command != '\n'; ++cmdnum)
		if (!str_cmp(scriptCommands[cmdnum].command, buf2))
			break;
	
	if (*scriptCommands[cmdnum].command != '\n') {
		number = cmdnum;
		type = CommandSwitch::Command;
	} else {
		if (!isalpha(*buf2))	
			buf2[1] = '\0';

		for (cmdnum = 0; *complete_cmd_info[cmdnum].command != '\n'; ++cmdnum)
			if (is_abbrev(buf2, complete_cmd_info[cmdnum].command))
				break;
		
		if (*complete_cmd_info[cmdnum].command != '\n') {
			number = cmdnum;
			type = CommandSwitch::PCCommand;
		}
	}
	
	release_buffer(buf1);
	release_buffer(buf2);
}


CmdlistElement::~CmdlistElement(void) {
	if (cmd)	free(cmd);
}


Cmdlist::Cmdlist(char *str) : command(str ? new CmdlistElement(str) : NULL), count(1) {
}


Cmdlist::~Cmdlist(void) {
	CmdlistElement *e = command, *next;
	
	while (e) {
		next = e->next;
		delete e;
		e = next;
	}
}


Cmdlist *Cmdlist::Share(Cmdlist *s) {
	if (!s)	return NULL;
//	if (s->count < 100) {
		++(s->count);
		return s;
//	} else
//		return new Cmdlist(s->command->cmd);
}


void Cmdlist::Free(void) {
	if (--count == 0)	delete this;
}


void Cmdlist::ResetCommandOps(void) {
	CmdlistElement *cmd;
	char *p;
	int cmdnum;

	cmd = command;
	while (cmd) {
		if (cmd->type == CommandSwitch::PCCommand) {
			p = cmd->cmd;
			skip_spaces(p);
			half_chop(p, buf1, buf2);
			
			for (cmdnum = 0; *complete_cmd_info[cmdnum].command != '\n'; ++cmdnum)
				if (is_abbrev(buf1, complete_cmd_info[cmdnum].command)) {
					cmd->number = cmdnum;
					break;
				}
		}
		cmd = cmd->next;
	}
}


TrigVarData::TrigVarData(void) {
	name = NULL;
	value = NULL;
}


TrigVarData::~TrigVarData(void) {
	if (name)		free(name);
	if (value)		free(value);
}


#if 0
TrigVarData::TrigVarData(void) : name(NULL), value(NULL), context(0) { }


TrigVarData::TrigVarData(char *setname, char *setvalue, SInt32 setcontext = 0) : 
	 name(NULL), value(NULL), context(setcontext)
{
	if (setname)	SetName(setname);
	if (setvalue)	SetValue(setvalue);
}


TrigVarData::~TrigVarData(void) {
	if (name)		delete name;
	if (value)		delete value;
}
#endif


// We now have a datatype requirement to make sure the scripts know what 
// they're attached to.
Script::Script(Type datatype) : types(0), context(0) {
	attached = datatype;
}



/* release memory allocated for a script */
Script::~Script(void) {
	TRIGGERS(this).Clear();

	TrigVarData *	var;
	while ((var = variables.Top())) {
		variables.Remove(var);
		delete var;
	}
}


void Script::Extract(void) {
	Trigger *trig;

	if (Purged())	return;
	Purged(true);

	PurgedScripts.Add(this);
	
	/* extract all the triggers too */
	while ((trig = triggers.Top())) {
		triggers.Remove(trig);
		trig->Extract();
	}
}


Trigger::Trigger(void) : data_type(0), name(NULL),
		trigger_type_mob(0), trigger_type_obj(0), trigger_type_wld(0), cmdlist(NULL), 
		curr_state(NULL), narg(0), arglist(NULL), depth(0), loops(0), event(NULL) {
}


Trigger::Trigger(const Trigger &trig) : Base(trig), data_type(trig.data_type),
		name(SSShare(trig.name)), 
		trigger_type_mob(trig.trigger_type_mob), 
		trigger_type_obj(trig.trigger_type_obj), 
		trigger_type_wld(trig.trigger_type_wld), 
		cmdlist(Cmdlist::Share(trig.cmdlist)),
		curr_state(NULL), narg(trig.narg), arglist(SSShare(trig.arglist)), depth(0), loops(0),
		event(NULL) {
}


Trigger::~Trigger(void) {
//	UInt32 i;

	SSFree(name);
	SSFree(arglist);
	
	TrigVarData *	var;
	while ((var = variables.Top())) {
		variables.Remove(var);
		delete var;
	}

	if (event)
		event->Cancel();
	
	if (cmdlist)	cmdlist->Free();
}



void Trigger::Extract(void) {
	if (Purged()) 	return;

	Purged(true);

	if (event) {
		event->Cancel();
	event = NULL;
	}
	
	Triggers.Remove(this);
	PurgedTrigs.Add(this);
	
	if (Virtual() >= 0)	Index[Virtual()].number--;
}

/*
RNum Trigger::Real(VNum vnum) {
	int bot, top, mid, nr, found = -1;

	// First binary search.
	bot = 0;
	top = top_of_trigt;

	for (;;) {
		mid = (bot + top) / 2;

		if ((trig_index + mid)->vnum == vnum)
			return mid;
		if (bot >= top)
			break;
		if ((trig_index + mid)->vnum > vnum)
			top = mid - 1;
		else
			bot = mid + 1;
	}

	// If not found - use linear on the "new" parts. 
	for(nr = 0; nr <= top_of_trigt; nr++) {
		if(trig_index[nr].vnum == vnum) {
			found = nr;
			break;
		}
	}
	return(found);
}
*/

/*
 * create a new trigger from a prototype.
 * nr is the real number of the trigger.
 */
Trigger *Trigger::Read(VNum nr) {
	Trigger *trig;

	if (!Trigger::Find(nr))	{
		log("Trigger (V) %d does not exist in database.", nr);
		return NULL;
	}

	trig = new Trigger(*Index[nr].proto);
	
	Triggers.Add(trig);
	
	Index[nr].number++;

	return trig;
}

void reset_command_ops(void)
{
	Trigger *live_trig;

	// Whenever the command list is resorted, we must update the opcodes of all the scripts.
	START_ITER(trig_iter, live_trig, Trigger *, Triggers) {
		if (live_trig->cmdlist)
			live_trig->cmdlist->ResetCommandOps();
	}
}


bool Trigger::Find(VNum vnum) {	return (Index.GetElement(vnum));	}


