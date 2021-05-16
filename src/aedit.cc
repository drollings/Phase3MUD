/*
 * File: aedit.c
 * Comment: OLC for MUDs -- this one edits socials
 * by Michael Scott <scottm@workcomm.net> -- 06/10/96
 * for use with OasisOLC
 * ftpable from ftp.circlemud.org:/pub/CircleMUD/contrib/code
 */


#include "characters.h"
#include "descriptors.h"
#include "interpreter.h"
#include "utils.h"
#include "db.h"
#include "aedit.h"
#include "olc.h"
#include "constants.h"
#include "buffer.h"


/* external functs */
void sort_commands(void); /* aedit patch -- M. Scott */
void create_command_list(void);
extern int olc_edit_bitvector(Descriptor *d, Flags *result, char *argument,
		       Flags old_bv, char *name, const char **values, Flags mask);


AEdit::AEdit(Descriptor *d, char *actionName) : Editor(d) {
	action = new Social();
	action->command = str_dup(actionName);
	action->sort_as = str_dup(actionName);
	action->hide    = 0;
	action->victim_position = P_ST;
	action->char_position   = P_ST;
	action->min_level_char      = 0;
	action->char_no_arg = str_dup("This action is unfinished.");
	action->others_no_arg = str_dup("This action is unfinished.");
	action->char_found = NULL;
	action->others_found = NULL;
	action->vict_found = NULL;
	action->not_found = NULL;
	action->char_auto = NULL;
	action->others_auto = NULL;
	action->char_body_found = NULL;
	action->others_body_found = NULL;
	action->vict_body_found = NULL;
	action->char_obj_found = NULL;
	action->others_obj_found = NULL;
	
	number = -1;
	
	Menu();
}


AEdit::AEdit(Descriptor *d, VNum num, Social &social) : Editor(d) {
	action = new Social(social);
	action->command = str_dup(social.command);
	action->sort_as = str_dup(social.sort_as);
	action->hide    = social.hide;
	action->victim_position = social.victim_position;
	action->char_position   = social.char_position;
	action->min_level_char      = social.min_level_char;
	action->char_no_arg			= social.char_no_arg ? str_dup(social.char_no_arg) : NULL;
	action->others_no_arg		= social.others_no_arg ? str_dup(social.others_no_arg) : NULL;
	action->char_found			= social.char_found ? str_dup(social.char_found) : NULL;
	action->others_found		= social.others_found ? str_dup(social.others_found) : NULL;
	action->vict_found			= social.vict_found ? str_dup(social.vict_found) : NULL;
	action->not_found			= social.not_found ? str_dup(social.not_found) : NULL;
	action->char_auto			= social.char_auto ? str_dup(social.char_auto) : NULL;
	action->others_auto			= social.others_auto ? str_dup(social.others_auto) : NULL;
	action->char_body_found		= social.char_body_found ? str_dup(social.char_body_found) : NULL;
	action->others_body_found	= social.others_body_found ? str_dup(social.others_body_found) : NULL;
	action->vict_body_found		= social.vict_body_found ? str_dup(social.vict_body_found) : NULL;
	action->char_obj_found		= social.char_obj_found ? str_dup(social.char_obj_found) : NULL;
	action->others_obj_found	= social.others_obj_found ? str_dup(social.others_obj_found) : NULL;
	
//	zoneNum = &social - soc_mess_list;
	number = num;
	
	Menu();
}


void AEdit::SaveInternally(void) {
//	struct social_messg *new_soc_mess_list = NULL;
//	int i;

	/* add a new social into the list */
	if (number < 0)  {
		Social::internal.SetSize(Social::internal.Count() + 1);
		Social::internal.Top() = *action;
		create_command_list();
		sort_commands();
	} else {	//	pass the editted action back to the list - no need to add
	
		// This is bloody inefficient.  However, this appears to be the
		// key to allowing simultaneous aedits.
		for (number = 0; number < Social::internal.Count(); ++number)  {
			if (is_abbrev(action->command, Social::internal[number].command))
				break;
		}
		
		if (number == Social::internal.Count()) {
			log("SYSERR:  Attempting to save action but original not found.");
			return;
		}

		action->act_nr = Social::internal[number].act_nr;
		/* why did i do this..? hrm */
		Social::internal[number].Clear();
		Social::internal[number] = *action;
		
		int i = find_command(action->command);
		if (i > -1) {
			complete_cmd_info[i].command			= Social::internal[number].command;
			complete_cmd_info[i].sort_as			= Social::internal[number].sort_as;
			complete_cmd_info[i].position	= Social::internal[number].char_position;
			complete_cmd_info[i].minimum_level		= Social::internal[number].min_level_char;
			Social::internal[number].act_nr = i;
		} else {
			create_command_list();
			sort_commands();
		}
	}
	olc_add_to_save_list(AEDIT_PERMISSION, OLC_AEDIT);
}


void AEdit::SaveDisk(SInt32 unused) {
	FILE *fp;
	
	if (!(fp = fopen(SOCMESS_FILE, "w+")))  {
		log("SYSERR: Can't open socials file '%s'", SOCMESS_FILE);
		exit(1);
	}

	for (int i = 0; i < Social::internal.Count(); i++)  {
		fprintf(fp, "~%s %s %d %d %d %d\n",
				Social::internal[i].command,
				Social::internal[i].sort_as,
				Social::internal[i].hide,
				Social::internal[i].char_position,
				Social::internal[i].victim_position,
				Social::internal[i].min_level_char);
		fprintf(fp, "%s\n%s\n%s\n%s\n",
				((Social::internal[i].char_no_arg)?Social::internal[i].char_no_arg:"#"),
				((Social::internal[i].others_no_arg)?Social::internal[i].others_no_arg:"#"),
				((Social::internal[i].char_found)?Social::internal[i].char_found:"#"),
				((Social::internal[i].others_found)?Social::internal[i].others_found:"#"));
		fprintf(fp, "%s\n%s\n%s\n%s\n",
				((Social::internal[i].vict_found)?Social::internal[i].vict_found:"#"),
				((Social::internal[i].not_found)?Social::internal[i].not_found:"#"),
				((Social::internal[i].char_auto)?Social::internal[i].char_auto:"#"),
				((Social::internal[i].others_auto)?Social::internal[i].others_auto:"#"));
		fprintf(fp, "%s\n%s\n%s\n",
				((Social::internal[i].char_body_found)?Social::internal[i].char_body_found:"#"),
				((Social::internal[i].others_body_found)?Social::internal[i].others_body_found:"#"),
				((Social::internal[i].vict_body_found)?Social::internal[i].vict_body_found:"#"));
		fprintf(fp, "%s\n%s\n\n",
				((Social::internal[i].char_obj_found)?Social::internal[i].char_obj_found:"#"),
				((Social::internal[i].others_obj_found)?Social::internal[i].others_obj_found:"#"));
	}
   
	fprintf(fp, "$\n");
	fclose(fp);
	olc_remove_from_save_list(AEDIT_PERMISSION, OLC_AEDIT);
}


void AEdit::Finish(FinishMode mode) {
	if (mode == Editor::All) {
		if (action->command)		free(action->command);
		if (action->sort_as)		free(action->sort_as);
		if (action->char_no_arg)	free(action->char_no_arg);
		if (action->others_no_arg)	free(action->others_no_arg);
		if (action->char_found)		free(action->char_found);
		if (action->others_found)	free(action->others_found);
		if (action->vict_found)		free(action->vict_found);
		if (action->not_found)		free(action->not_found);
		if (action->char_auto)		free(action->char_auto);
		if (action->others_auto)	free(action->others_auto);
		if (action->char_body_found)free(action->char_body_found);
		if (action->others_body_found)free(action->others_body_found);
		if (action->vict_body_found)free(action->vict_body_found);
		if (action->char_obj_found)	free(action->char_obj_found);
		if (action->others_obj_found)free(action->others_obj_found);
	}
	delete action;
	
	if (d->character) {
		STATE(d) = CON_PLAYING;
		d->character->Echo("$n stops using OLC.\r\n");
	}
	Editor::Finish();		
}


/*------------------------------------------------------------------------*/
//	Menu functions */

/* the main menu */
void AEdit::Menu(void) {
	sprintbit(action->char_position, position_types, buf);
	sprintbit(action->victim_position, position_types, buf1);

	d->Writef("\x1B[H\x1B[J"
			"&c0-- Action editor\r\n\r\n"
			"&cGN&c0) Command		  : &cY%-15.15s &cG1&c0) Sort as Command  : &cY%-15.15s\r\n"
			"&cG2&c0) Min Position[CH]: &cC%s\r\n"
			"&cG3&c0) Min Position[VT]: &cC%s\r\n"
			"&cG4&c0) Min Level   [CH]: &cC%-3d 			&cG5&c0) Show if Invisible: &cC%s\r\n"
			"&cGA&c0) Char    [NO ARG]: &cC%s\r\n"
			"&cGB&c0) Others  [NO ARG]: &cC%s\r\n"
			"&cGC&c0) Char [NOT FOUND]: &cC%s\r\n"
			"&cGD&c0) Char  [ARG SELF]: &cC%s\r\n"
			"&cGE&c0) Others[ARG SELF]: &cC%s\r\n"
			"&cGF&c0) Char  	[VICT]: &cC%s\r\n"
			"&cGG&c0) Others	[VICT]: &cC%s\r\n"
			"&cGH&c0) Victim	[VICT]: &cC%s\r\n"
			"&cGI&c0) Char  [BODY PRT]: &cC%s\r\n"
			"&cGJ&c0) Others[BODY PRT]: &cC%s\r\n"
			"&cGK&c0) Victim[BODY PRT]: &cC%s\r\n"
			"&cGL&c0) Char  	 [OBJ]: &cC%s\r\n"
			"&cGM&c0) Others	 [OBJ]: &cC%s\r\n"
			"&cGQ&c0) Quit\r\n",
			action->command, action->sort_as,
			buf, buf1,
			action->min_level_char, (action->hide?"HIDDEN" : "NOT HIDDEN"),
			action->char_no_arg ? action->char_no_arg : "<Null>",
			action->others_no_arg ? action->others_no_arg : "<Null>",
			action->not_found ? action->not_found : "<Null>",
			action->char_auto ? action->char_auto : "<Null>",
			action->others_auto ? action->others_auto : "<Null>",
			action->char_found ? action->char_found : "<Null>",
			action->others_found ? action->others_found : "<Null>",
			action->vict_found ? action->vict_found : "<Null>",
			action->char_body_found ? action->char_body_found : "<Null>",
			action->others_body_found ? action->others_body_found : "<Null>",
			action->vict_body_found ? action->vict_body_found : "<Null>",
			action->char_obj_found ? action->char_obj_found : "<Null>",
			action->others_obj_found ? action->others_obj_found : "<Null>"
			);

	d->Write("Enter choice: ");

	mode = MainMenu;
}


/*
 * The main loop
 */

void AEdit::Parse(char *arg) {
	int i;

	switch (mode) {
		case ConfirmSave:
			switch (*arg) {
				case 'y': case 'Y':
					mudlogf(NRM, d->character, true, "OLC: %s edits action %s", d->character->RealName(),
							action->command);
					d->Write("Action saved to memory.\r\n");
					SaveInternally();
					Finish(Editor::Structs);
					break;
				case 'n': case 'N':
					Finish(Editor::All);
					break;
				default:
					d->Write("Invalid choice!\r\nDo you wish to save this action internally? ");
					break;
			}
			return;

		case MainMenu:
			switch (*arg) {
				case 'q': case 'Q':
					if (save)  { /* Something was modified */
						d->Write("Do you wish to save this action internally? ");
						mode = ConfirmSave;
					} else
						Finish(Editor::All);
					break;
				case 'n':
					d->Write("Enter action name: ");
					mode = ActionName;
					break;
				case '1':
					d->Write("Enter sort info for this action (for the command listing): ");
					mode = SortAs;
					break;
				case '2':
					olc_edit_bitvector(d, &(action->char_position), "", 
							action->char_position, "positions", position_types, 0);
					mode = MinCharPos;
					break;
				case '3':
					olc_edit_bitvector(d, &(action->victim_position), "", 
							action->victim_position, "positions", position_types, 0);
					mode = MinVictPos;
					break;
				case '4':
					d->Write("Enter new minimum level for social: ");
					mode = MinCharLevel;
					break;
				case '5':
					action->hide = !action->hide;
					Menu();
					save = 1;
					break;
				case 'a': case 'A':
					d->Writef("Enter social shown to the Character when there is no argument supplied.\r\n[OLD]: %s\r\n[NEW]: ",
							(action->char_no_arg ? action->char_no_arg:"NULL"));
					mode = NoVictChar;
					break;
				case 'b': case 'B':
					d->Writef("Enter social shown to Others when there is no argument supplied.\r\n[OLD]: %s\r\n[NEW]: ",
							(action->others_no_arg ? action->others_no_arg : "NULL"));
					mode = NoVictOthers;
					break;
				case 'c': case 'C':
					d->Writef("Enter text shown to the Character when his victim isnt found.\r\n[OLD]: %s\r\n[NEW]: ",
							(action->not_found ? action->not_found : "NULL"));
					mode = VictNotFound;
					break;
				case 'd': case 'D':
					d->Writef("Enter social shown to the Character when it is its own victim.\r\n[OLD]: %s\r\n[NEW]: ",
							(action->char_auto ? action->char_auto : "NULL"));
					mode = SelfChar;
					break;
				case 'e': case 'E':
					d->Writef("Enter social shown to Others when the Char is its own victim.\r\n[OLD]: %s\r\n[NEW]: ",
							(action->others_auto ? action->others_auto : "NULL"));
					mode = SelfOthers;
					break;
				case 'f': case 'F':
					d->Writef("Enter normal social shown to the Character when the victim is found.\r\n[OLD]: %s\r\n[NEW]: ",
							(action->char_found ? action->char_found : "NULL"));
					mode = VictFoundChar;
					break;
				case 'g': case 'G':
					d->Writef("Enter normal social shown to Others when the victim is found.\r\n[OLD]: %s\r\n[NEW]: ",
							(action->others_found ? action->others_found : "NULL"));
					mode = VictFoundOthers;
					break;
				case 'h': case 'H':
					d->Writef("Enter normal social shown to the Victim when the victim is found.\r\n[OLD]: %s\r\n[NEW]: ",
							(action->vict_found ? action->vict_found : "NULL"));
					mode = VictFoundVict;
					break;
				case 'i': case 'I':
					d->Writef("Enter 'body part' social shown to the Character when the victim is found.\r\n[OLD]: %s\r\n[NEW]: ",
							(action->char_body_found ? action->char_body_found : "NULL"));
					mode = VictFoundBodyChar;
					break;
				case 'j': case 'J':
					d->Writef("Enter 'body part' social shown to Others when the victim is found.\r\n[OLD]: %s\r\n[NEW]: ",
							(action->others_body_found ? action->others_body_found : "NULL"));
					mode = VictFoundBodyOthers;
					break;
				case 'k': case 'K':
					d->Writef("Enter 'body part' social shown to the Victim when the victim is found.\r\n[OLD]: %s\r\n[NEW]: ",
							(action->vict_body_found ? action->vict_body_found : "NULL"));
					mode = VictFoundBodyVict;
					break;
				case 'l': case 'L':
					d->Writef("Enter 'object' social shown to the Character when the object is found.\r\n[OLD]: %s\r\n[NEW]: ",
							(action->char_obj_found ? action->char_obj_found : "NULL"));
					mode = ObjFoundChar;
					break;
				case 'm': case 'M':
					d->Writef("Enter 'object' social shown to the Room when the object is found.\r\n[OLD]: %s\r\n[NEW]: ",
							(action->others_obj_found ? action->others_obj_found : "NULL"));
					mode = ObjFoundOthers;
					break;
				default:
					Menu();
			}
			return;

		case ActionName:
			if (*arg) {
				if (strchr(arg,' '))
					Menu();
				else  {
					if (action->command)
						free(action->command);
					action->command = str_dup(arg);
					break;
				}
			} else
				Menu();
			return;

		case SortAs:
			if (*arg) {
				if (strchr(arg,' '))
					Menu();
				else  {
					if (action->sort_as)
						free(action->sort_as);
					action->sort_as = str_dup(arg);
					break;
				}
			} else
				Menu();
			return;

		case MinCharPos:
			switch (*arg && olc_edit_bitvector(d, &(action->char_position), arg, 
				action->char_position, "positions", position_types, 0)) {
			case 0: 
				Menu(); 
				return;
			default: 
				if (!save)	save = OLC_SAVE_YES;
				olc_edit_bitvector(d, &(action->char_position), "", 
				action->char_position, "positions", position_types, 0);
				return;
			}
			
		case MinVictPos:
			switch (*arg && olc_edit_bitvector(d, &(action->victim_position), arg, 
				action->victim_position, "positions", position_types, 0)) {
			case 0: 
				Menu(); 
				return;
			default: 
				if (!save)	save = OLC_SAVE_YES;
				olc_edit_bitvector(d, &(action->victim_position), "", 
				action->victim_position, "positions", position_types, 0);
				return;
			}

		case MinCharLevel:
			if (*arg)  {
				i = atoi(arg);
				if ((i < 0) || (i > MAX_LEVEL))
					Menu();
				else {
					action->min_level_char = i;
					break;
				}
			} else
				Menu();
			return;

		case NoVictChar:
			if (action->char_no_arg)
				free(action->char_no_arg);
			if (*arg)	{
				delete_doubledollar(arg);
				action->char_no_arg = str_dup(arg);
			} else
				action->char_no_arg = NULL;
			break;

		case NoVictOthers:
			if (action->others_no_arg)
				free(action->others_no_arg);
			if (*arg)	{
				delete_doubledollar(arg);
				action->others_no_arg = str_dup(arg);
			} else
				action->others_no_arg = NULL;
			break;

		case VictFoundChar:
			if (action->char_found)
				free(action->char_found);
			if (*arg)	{
				delete_doubledollar(arg);
				action->char_found = str_dup(arg);
			} else
				action->char_found = NULL;
			break;

		case VictFoundOthers:
			if (action->others_found)
				free(action->others_found);
			if (*arg)	{
				delete_doubledollar(arg);
				action->others_found = str_dup(arg);
			} else
				action->others_found = NULL;
			break;

		case VictFoundVict:
			if (action->vict_found)
				free(action->vict_found);
			if (*arg)	{
				delete_doubledollar(arg);
				action->vict_found = str_dup(arg);
			} else
				action->vict_found = NULL;
			break;

		case VictNotFound:
			if (action->not_found)
				free(action->not_found);
			if (*arg) {
				delete_doubledollar(arg);
				action->not_found = str_dup(arg);
			} else
				action->not_found = NULL;
			break;

		case SelfChar:
			if (action->char_auto)
				free(action->char_auto);
			if (*arg)	{
				delete_doubledollar(arg);
				action->char_auto = str_dup(arg);
			} else
				action->char_auto = NULL;
			break;

		case SelfOthers:
			if (action->others_auto)
				free(action->others_auto);
			if (*arg)	{
				delete_doubledollar(arg);
				action->others_auto = str_dup(arg);
			} else
				action->others_auto = NULL;
			break;

		case VictFoundBodyChar:
			if (action->char_body_found)
				free(action->char_body_found);
			if (*arg)	{
				delete_doubledollar(arg);
				action->char_body_found = str_dup(arg);
			} else
				action->char_body_found = NULL;
			break;

		case VictFoundBodyOthers:
			if (action->others_body_found)
				free(action->others_body_found);
			if (*arg)	{
				delete_doubledollar(arg);
				action->others_body_found = str_dup(arg);
			} else
				action->others_body_found = NULL;
			break;

		case VictFoundBodyVict:
			if (action->vict_body_found)
				free(action->vict_body_found);
			if (*arg)	{
				delete_doubledollar(arg);
				action->vict_body_found = str_dup(arg);
			} else
				action->vict_body_found = NULL;
			break;

		case ObjFoundChar:
			if (action->char_obj_found)
				free(action->char_obj_found);
			if (*arg)	{
				delete_doubledollar(arg);
				action->char_obj_found = str_dup(arg);
			} else
				action->char_obj_found = NULL;
			break;

		case ObjFoundOthers:
			if (action->others_obj_found)
				free(action->others_obj_found);
			if (*arg)	{
				delete_doubledollar(arg);
				action->others_obj_found = str_dup(arg);
			} else
				action->others_obj_found = NULL;
			break;

		default:
			/* we should never get here */
			break;
	}
	if (!save)	save = OLC_SAVE_YES;
	Menu();
}
