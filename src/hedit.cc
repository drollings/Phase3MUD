/*
 * File: hedit.c
 * Comment: OLC for MUDs -- this one edits help files
 * By Steve Wolfe
 * for use with OasisOLC
 */


#include "characters.h"
#include "descriptors.h"
#include "utils.h"
#include "db.h"
#include "olc.h"
#include "buffer.h"
#include "handler.h"
#include "hedit.h"


HEdit::HEdit(Descriptor *d, char *arg) : Editor(d) {
	help = new Help;
	
	help->keywords = str_dup(arg);
	help->entry = str_dup("This help entry is unfinished.\r\n");
	help->level = 0;

	value = 0;
	
//	zoneNum = -1;
	
	Menu();
}

/*------------------------------------------------------------------------*/

HEdit::HEdit(Descriptor *d, Help &helpItem) : Editor(d) {
	help = new Help;
	
	help->keywords = str_dup(helpItem.keywords);
	help->entry = str_dup(helpItem.entry);
	help->level = helpItem.level;
	
//	zoneNum = &helpItem - help_table;
	
	value = 0;
	
	Menu();
}


void HEdit::SaveInternally(void) {
	/* add a new help element into the list */
	if (zoneNum >= 0) {
		Help::Table[zoneNum].Free();
		Help::Table[zoneNum] = *help;
	} else {
		RECREATE(Help::Table, Help, Help::Top + 1);
		Help::Table[Help::Top++] = *help;
	}
	olc_add_to_save_list(HEDIT_PERMISSION, OLC_HEDIT);
}


/*------------------------------------------------------------------------*/

void HEdit::SaveDisk(SInt32 unused) {
	FILE *fp;
	char *buf;
	
	if (!(fp = fopen(HELP_FILE ".new", "w+"))) {
		log("SYSERR: Can't open help file '%s'", HELP_FILE);
		exit(1);
	}
	
	buf = get_buffer(32384);
	
	for (SInt32 i = 0; i < Help::Top; i++) {
		Help &help = Help::Table[i];
		
		if (!help.keywords || !*help.keywords ||
			!help.entry || !*help.entry)	continue;
		
		/*. Remove the '\r\n' sequences from entry . */
		strcpy(buf, help.entry);
		strip_string(buf);
		
		fprintf(fp, "%s\n"
					"%s#%d\n",
				help.keywords,
				buf,
				help.level);
	}
	release_buffer(buf);

	fprintf(fp, "$\n");
	fclose(fp);
	
	remove(HELP_FILE);
	rename(HELP_FILE ".new", HELP_FILE);
	
	olc_remove_from_save_list(HEDIT_PERMISSION, OLC_HEDIT);
}


void HEdit::Finish(FinishMode mode) {
	if (mode == Structs) {
		help->keywords = NULL;
		help->entry = NULL;
	}
	delete help;
	
	if (d->character) {
		STATE(d) = CON_PLAYING;
		d->character->Echo("$n stops using OLC.\r\n");
	}
	Editor::Finish();
}


/*------------------------------------------------------------------------*/

/* Menu functions */

/* the main menu */
void HEdit::Menu(void) {
	d->Writef(
#if defined(CLEAR_SCREEN)
			"\x1B[H\x1B[J"
#endif
			"&cG1&c0) Keywords    : &cY%s\r\n"
			"&cG2&c0) Entry       :\r\n%s"
			"&cG3&c0) Min Level   : &cY%d\r\n"
			"&cGQ&c0) Quit\r\n"
			"Enter choice : ",
			
			help->keywords,
			help->entry,
			help->level);

	mode = MainMenu;
}

/*
 * The main loop
 */

void HEdit::Parse(char *arg) {
	int i;
	Editor * tmpeditor = NULL;

	switch (mode) {
		case ConfirmSave:
			switch (*arg) {
				case 'y':
				case 'Y':
					mudlogf(CMP, d->character, true, "OLC: %s edits help.", d->character->RealName());
					d->Write("Help saved to memory.\r\n");
					SaveInternally();
					Finish(Structs);
					break;
				case 'n':
				case 'N':
					Finish(All);
					break;
				default:
					d->Write("Invalid choice!\r\nDo you wish to save this help internally? ");
					break;
			}
			return;			/* end of HEDIT_CONFIRM_SAVESTRING */

		case MainMenu:
			switch (*arg) {
				case 'q':
				case 'Q':
					if (value) {		/* Something was modified */
						d->Write("Do you wish to save this help internally? ");
						mode = ConfirmSave;
					} else
						Finish(All);
					break;
				case '1':
					d->Write("Enter keywords:-\r\n| ");
					mode = Keywords;
					break;
				case '2':
					mode = Entry;
					d->Write("\x1B[H\x1B[J"
							 "Enter the help info: (/s saves /h for help)\r\n\r\n");
					tmpeditor = new TextEditor(d, &(help->entry), MAX_STRING_LENGTH);
					d->Edit(tmpeditor);
					value = 1;
					break;
				case '3':
					d->Write("Enter the minimum level: ");
					mode = MinTrust;
					break;
				default:
					Menu();
					break;
			}
			return;
		
		case Keywords:
			if (strlen(arg) > MAX_HELP_KEYWORDS)	arg[MAX_HELP_KEYWORDS - 1] = '\0';
			if (help->keywords);					free(help->keywords);
			help->keywords = str_dup(*arg ? arg : "UNDEFINED");
			break;
			
		case Entry:
			/* should never get here */
			mudlog("SYSERR: Reached HEDIT_ENTRY in hedit_parse", BRF, NULL, true);
			break;

		case MinTrust:
			if (*arg) {
				i = atoi(arg);
				if ((i >= 0) && (i <= MAX_TRUST)) {
					help->level = i;
					value = 1;
				}
			}
			break;
		default:
			/* we should never get here */
			break;
	}
	Menu();
}


VNum HEdit::Find(const char *keyword) {
	int i;

	for (i = 0; i < Help::Top; ++i)
		if (isword(keyword, Help::Table[i].keywords))
			return i;

	return -1;
}

