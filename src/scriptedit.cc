/***************************************************************************\
[*]    __     __  ___                ___  | LexiMUD is a C++ MUD codebase [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ |        for Sci-Fi MUDs        [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / | Copyright(C) 1997-1999        [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  | All rights reserved           [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   | Chris Jacobson (FearItself)   [*]
[*]      A Creation of the AvP Team!      | fear@technologist.com         [*]
[-]---------------------------------------+-------------------------------[-]
[*] File : scriptedit.c++                                                 [*]
[*] Usage: ScriptEdit - OLC Editor for Scripts                            [*]
\***************************************************************************/

#include "types.h"

#include "scripts.h"
#include "characters.h"
#include "descriptors.h"
#include "buffer.h"
#include "comm.h"
#include "utils.h"
#include "db.h"
#include "olc.h"
#include "event.h"
#include "constants.h"

#include "scriptedit.h"

extern struct zone_data *zone_table;
extern int top_of_zone_table;

/*
 * Utils and exported functions.
 */

ScriptEdit::ScriptEdit(Descriptor *d, VNum newVNum) : Editor(d) {
	trig = new Trigger;
	
	GET_TRIG_NAME(trig) = SString::Create("new script");
	GET_TRIG_ARG(trig) = NULL;
	GET_TRIG_TYPE_MOB(trig) = MTRIG_GREET;
	GET_TRIG_TYPE_OBJ(trig) = 0;
	GET_TRIG_TYPE_WLD(trig) = 0;
	
	storage = str_dup("* Empty script\r\n");
	
	number = newVNum;
	value = 0;
	
	Menu();
}


/*------------------------------------------------------------------------*/

ScriptEdit::ScriptEdit(Descriptor *d, VNum newVNum, IndexData<Trigger> &trigEntry) : Editor(d) {
	Trigger *	proto = trigEntry.proto;
	char *		buf = get_buffer(MAX_STRING_LENGTH);
	CmdlistElement *cmd;
	
	trig = new Trigger;
	
	if (proto->Virtual() != newVNum) {
		if (Trigger::Find(newVNum))		save = OLC_SAVE_OVERWRITE;
		else							save = OLC_SAVE_COPY;
	}

	trig->Virtual(number = newVNum);

	GET_TRIG_TYPE_MOB(trig) = GET_TRIG_TYPE_MOB(proto);
	GET_TRIG_TYPE_OBJ(trig) = GET_TRIG_TYPE_OBJ(proto);
	GET_TRIG_TYPE_WLD(trig) = GET_TRIG_TYPE_WLD(proto);
	trig->narg = proto->narg;
	GET_TRIG_NAME(trig) = SString::Create(SSData(GET_TRIG_NAME(proto)));
	GET_TRIG_ARG(trig) = SString::Create(SSData(GET_TRIG_ARG(proto)));
	
	for(cmd = proto->cmdlist->command; cmd; cmd = cmd->next) {
		if (cmd->cmd)	strcat(buf, cmd->cmd);
		strcat(buf, "\r\n");
	}
	
	trig->Virtual(number = newVNum);

	storage = str_dup(buf);
	release_buffer(buf);
	value = 0;
	
	Menu();
}

#define ZCMD zone_table[zone].cmd[cmd_no]

void ScriptEdit::SaveInternally(void) {	
	char *		s;
	CmdlistElement *cmd;
	Trigger *	live_trig;
	
	REMOVE_BIT(GET_TRIG_TYPE_WLD(trig), TRIG_DELETED);
	
	//	Recompile the command list from the new script
	if (!storage)
		storage = str_dup("* Empty script\r\n");
	
	s = storage;
	
	trig->cmdlist = new Cmdlist(strtok(s, "\n\r"));
	cmd = trig->cmdlist->command;
	while ((s = strtok(NULL, "\n\r"))) {
		cmd->next = new CmdlistElement(s);
		cmd = cmd->next;
	}

	if (Trigger::Find(number)) {
		Trigger *	proto = Trigger::Index[number].proto;
		proto->cmdlist->Free();
		proto->cmdlist = NULL;
		delete proto;
		
		trig->Virtual(number);
		Trigger::Index[number].proto = trig;
		
		/* HERE IT HAS TO GO THROUGH AND FIX ALL SCRIPTS/TRIGS OF THIS KIND */
		START_ITER(trig_iter, live_trig, Trigger *, Triggers) {
			if (GET_TRIG_VNUM(live_trig) == number && !GET_TRIG_WAIT(live_trig)) {
				SSFree(live_trig->arglist);
				SSFree(live_trig->name);
				
				live_trig->arglist = SSShare(trig->arglist);
				live_trig->name = SSShare(trig->name);

				live_trig->cmdlist->Free();
				live_trig->cmdlist = Cmdlist::Share(trig->cmdlist);
				live_trig->trigger_type_mob = trig->trigger_type_mob;
				live_trig->trigger_type_obj = trig->trigger_type_obj;
				live_trig->trigger_type_wld = trig->trigger_type_wld;
				live_trig->narg = trig->narg;
				live_trig->data_type = trig->data_type;
			}
		}
	} else {
		Trigger::Index[number].vnum = number;
		Trigger::Index[number].number = 0;
		Trigger::Index[number].func = NULL;
		Trigger::Index[number].proto = trig;
		
		trig->Virtual(number);
	}
	
	olc_add_to_save_list(zone_table[zoneNum].number, OLC_SCRIPTEDIT);
}


void ScriptEdit::Finish(FinishMode mode) {
	if (mode == All)		delete trig;
	
	if (d->character) {
		STATE(d) = CON_PLAYING;
		act("$n stops using OLC.", TRUE, d->character, 0, 0, TO_ROOM);
	}
	Editor::Finish();
}


/*------------------------------------------------------------------------*/

/* Menu functions */

#define PAGE_LENGTH	78
/* the main menu */
void ScriptEdit::Menu(void) {
	char *buf = get_buffer(MAX_STRING_LENGTH);
	char trgtypes_mob[256], trgtypes_obj[256], trgtypes_wld[256], trgtypes_player[256];
	char *str, *bufptr;
	UInt8	x;

	bufptr = buf;
	x = 0;
	if (storage) {
		for (str = storage; *str; str++) {
			if (*str == '\n')	x++;	// Newline puts us on the next line.
			*bufptr++ = *str;
			if (x == 10)
				break;
		}
	}
	*bufptr = '\0';
	
	sprintbit (GET_TRIG_TYPE_MOB(trig), (const char **) mtrig_types, trgtypes_mob);
	sprintbit (GET_TRIG_TYPE_OBJ(trig), (const char **) otrig_types, trgtypes_obj);
	sprintbit (GET_TRIG_TYPE_WLD(trig), (const char **) wtrig_types, trgtypes_wld);

	d->Writef(
#if defined(CLEAR_SCREEN)
			"\x1B[H\x1B[J"
#endif
			"Trigger Editor [&cC%d&c0]		 %s %s\r\n"
			"&cG1&c0) Name                 : &cY%s\r\n"
			"&cG2&c0) Mobile trigger types : &cY%s\r\n"
			"&cG3&c0) Object trigger types : &cY%s\r\n"
			"&cG4&c0) Room trigger types   : &cY%s\r\n"
			"&cG5&c0) Numeric Arg          : &cY%d\r\n"
			"&cG6&c0) Arguments            : &cY%s\r\n"
			"&cG7&c0) Commands:\r\n&cC",
			
			number,
			(save == OLC_SAVE_COPY) ? "&cW&bb[ Copy ]&c0" : "",
			(save == OLC_SAVE_OVERWRITE) ? "&cW&br[ OVERWRITING ]&c0" : "",
			SSData(GET_TRIG_NAME(trig)),
			trgtypes_mob,			// greet/drop/etc												 
			trgtypes_obj,			// greet/drop/etc												 
			trgtypes_wld,			// greet/drop/etc												 
			GET_TRIG_NARG(trig),
			SSData(GET_TRIG_ARG(trig)) ? SSData(GET_TRIG_ARG(trig)) : "<NONE>");

	d->Write(buf);
	d->Writef("%s\r\n"
			"&cGQ&c0) Quit\r\n"
			"Enter Choice :", (x == 10 ? "&c0... continued ...\r\n" : ""));

//	page_string(d, buf, true);

	mode = MainMenu;
	release_buffer(buf);
}


//	The main loop
void ScriptEdit::Parse(char *arg) {
	int i, num_trig_types;
	Editor * tmpeditor = NULL;

	switch (mode) {
		case ConfirmSave:
			switch (*arg) {
				case 'y':
				case 'Y':
					d->Write("Saving script to memory.\r\n");
					mudlogf(NRM, d->character, true, "OLC: %s edits script %d", d->character->RealName(), number);
					SaveInternally();
					Finish(Structs);
					break;
				case 'n':
				case 'N':
					Finish(All);
					break;
				default:
					d->Write("Invalid choice!\r\n"
							 "Do you wish to save the script? ");
					break;
			}
			return;			/* end of SCRIPTEDIT_CONFIRM_SAVESTRING */

			case MainMenu:
				switch (*arg) {
					case 'q':
					case 'Q':
						if (save) {		/* Something was modified */
							d->Write("Do you wish to save the changes to this script? (y/n) : ");
							mode = ConfirmSave;
						} else
							Finish(All);
						break;
					case '1':
						d->Write("Enter the name of this script: ");
						mode = Trigger::TrigName;
						return;
					case '2':
						mode = Trigger::TrigMobTypes;
						trig->Edit(d, "", mode, "");
						return;
					case '3':
						mode = Trigger::TrigObjTypes;
						trig->Edit(d, "", mode, "");
						return;
					case '4':
						mode = Trigger::TrigWldTypes;
						trig->Edit(d, "", mode, "");
						return;
					case '5':
						mode = Trigger::TrigNarg;
						d->Write("Enter the numeric argument for this script: ");
						return;
					case '6':
						d->Write("Enter the argument for this script: ");
						mode = Trigger::TrigArgument;
						return;
					case '7':
						mode = Trigger::TrigScript;
						d->Write(
#if defined(CLEAR_SCREEN)
								"\x1B[H\x1B[J"
#endif
								 "Edit the script: (/s saves /h for help)\r\n\r\n");
						tmpeditor = new TextEditor(d, &storage, MAX_STRING_LENGTH);
						d->Edit(tmpeditor);
						if (!save)	save = OLC_SAVE_YES;
						break;
					default:
						Menu();
						break;
				}
				return;
			case Trigger::TrigName:
				if (trig->name)		SSFree(trig->name);
				trig->name = NULL;
				if (arg)			trig->name = SString::Create(arg);
				else				trig->name = SString::Create("undefined");
				Menu();
				break;

			case Trigger::TrigNarg:
				if (*arg && trig->Edit(d, "", mode, arg)) {
					if (!save)	save = OLC_SAVE_YES;
				}
				break;

			case Trigger::TrigMobTypes:
			case Trigger::TrigObjTypes:
			case Trigger::TrigWldTypes:
				switch (*arg && trig->Edit(d, "", mode, arg)) {
				case 0:
					Menu();
					return;
				default:
					if (!save)	save = OLC_SAVE_YES;
					trig->Edit(d, "", mode, "");
					return;
				}
				return;
	
			case Trigger::TrigArgument:
				SSFree(GET_TRIG_ARG(trig));
				GET_TRIG_ARG(trig) = SString::Create(*arg ? arg : NULL);
				break;
			case Trigger::TrigScript:
				break;
			default:
				/* we should never get here */
				mudlog("SYSERR: Reached default case in scriptedit_parse", BRF, NULL, true);
				break;
	}
	if (!save)	save = OLC_SAVE_YES;
	Menu();
}
