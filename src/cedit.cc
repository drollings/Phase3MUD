/*************************************************************************
*   File: cedit.c++                  Part of Aliens vs Predator: The MUD *
*  Usage: Primary code for characters                                    *
*************************************************************************/


#include "clans.h"
#include "cedit.h"
#include "descriptors.h"
#include "characters.h"
#include "olc.h"
#include "utils.h"
#include "comm.h"
#include "buffer.h"
#include "rooms.h"
#include "db.h"


CEdit::CEdit(Descriptor *d, VNum newNum) : Editor(d) {
	clan = new Clan;
	
	clan->name = str_dup("New Clan");
	clan->description = str_dup("A new clan!\r\n");
	clan->owner = 0;
	clan->room = -1;
	clan->vnum = newNum;
	clan->savings = 0;
	
#ifdef GOT_RID_OF_IT
	for (SInt32 x = 0; x < Clan::NumRanks - 1; x++)
		clan->ranks[x] = str_dup(titles[RACE_HYUMANN][x + 1].title_m);
#endif
	
	number = newNum;
	value = 0;
	
	Menu();
}


CEdit::CEdit(Descriptor *d, const Clan &clanEntry) : Editor(d) {
	clan = new Clan(clanEntry);
	
//	SInt32	playerId;
//	LListIterator<UInt32>	members(clan->members);
//	while ((playerId = members.Next()))
//		Clan::Clans[rnum].members.Append(playerId);
	
	number = clanEntry.vnum;
	value = 0;
	
	Menu();
}


void CEdit::SaveInternally(void) {
//	bool	found = false;
	
	if (get_name_by_id(clan->owner))
		clan->AddMember(clan->owner);
	
//	if (Clan::Find(number)) {
	if (Clan::Clans[number].name)			free(Clan::Clans[number].name);
	if (Clan::Clans[number].description)	free(Clan::Clans[number].description);
	
	Clan::Clans[number].name = clan->name;
	Clan::Clans[number].description = clan->description;
	Clan::Clans[number].vnum = number;
	
	for (SInt32 x = 0; x < Clan::NumRanks - 1; x++) {
		if (Clan::Clans[number].ranks[x])
			free(Clan::Clans[number].ranks[x]);
		Clan::Clans[number].ranks[x] = clan->ranks[x];
	}
	
	Clan::Clans[number].owner = clan->owner;
	Clan::Clans[number].room = clan->room;
	Clan::Clans[number].savings = clan->savings;
	Clan::Clans[number].members = clan->members;
/*	} else {
		Clan::Clans[number];
		
		new_index[rnum] = *clan;

		free(Clan::Clans);
		Clan::Clans = new_index;
		Clan::Top++;
	}
*/

	olc_add_to_save_list(CEDIT_PERMISSION, OLC_CEDIT);
}


void CEdit::Finish(FinishMode mode) {
	if (mode == Structs) {
		clan->name = NULL;
		clan->description = NULL;
		for (SInt32 x = 0; x < Clan::NumRanks - 1; x++)
			clan->ranks[x] = NULL;
		clan->members.Erase();
	}
	delete clan;
	
	if (d->character) {
		STATE(d) = CON_PLAYING;
		act("$n stops using OLC.", TRUE, d->character, 0, 0, TO_ROOM);
	}
	Editor::Finish();
}


void CEdit::Menu(void) {
//	SInt32			room;
	const char *	name;
	char *			buf = get_buffer(MAX_INPUT_LENGTH);
	
//	room = Room::Real(clan->room);
	
	name = get_name_by_id(clan->owner);
	strcpy(buf, name ? name : "<NONE>");
	CAP(buf);
	
	d->Writef(
#if defined(CLEAR_SCREEN)
				"\x1B[H\x1B[J"
#endif
				"-- Clan Number:  [&cC%d&c0]\r\n"
				"&cGA&c0) Name       : &cY%s\r\n"
				"&cGB&c0) Description:-\r\n&cY%s"
				"&cGC&c0) Owner      : &cY%s\r\n"
				"&cGD&c0) Room       : &cY%s &c0[&cC%5d&c0]\r\n"
				"&cGE&c0) Savings    : &cY%d\r\n"
				"&cGF&c0) Ranks\r\n"
				"G) Member Management\r\n"
				"&cGQ&c0) Quit\r\n"
				"Enter choice: ",
				clan->vnum,
				clan->name,
				clan->description,
				buf,
				Room::Find(clan->room) ? world[clan->room].Name("<ERROR>") : "<Not Created>" , clan->room,
				clan->savings);

	mode = MainMenu;
	release_buffer(buf);
}


void CEdit::RankMenu(void) {
	d->Write(
#if defined(CLEAR_SCREEN)
			"\x1B[H\x1B[J"
#endif
			"-- Clan Ranks:\r\n");
	for (SInt32 x = 0; x < Clan::NumRanks - 1; x++) {
		d->Writef("&cG%d&c0) %-30s", x + 1, clan->ranks[x]);
		switch (x + 1) {
			case Clan::Rank::Commander:		d->Write(" Commander");		break;
			case Clan::Rank::Leader:		d->Write(" Leader");		break;
		}
		d->Write("\r\n");
	}
	d->Write("&cG0&c0) Exit\r\n"
			"Enter Choice: ");
	
	mode = ClanRanks;
}


void CEdit::Parse(char *arg) {
	SInt32	i;
	Editor 	*tmpeditor = NULL;
	
	switch (mode) {
		case ConfirmSave:
			switch (*arg) {
				case 'y':
				case 'Y':
					mudlogf(NRM, d->character, true, "OLC: %s edits clan %d", d->character->RealName(), number);
					d->Write("Saving clan to memory.\r\n");
					SaveInternally();
					Finish(Structs);
					break;
				case 'n':
				case 'N':
					Finish(All);
					break;
				default:
					d->Write("Invalid choice!\r\n"
							 "Do you wish to save the clan? ");
			}
			return;
		case MainMenu:
			switch (*arg) {
				case 'Q':
				case 'q':
					if (value) {
						d->Write("Do you wish to save the changes to the clan? (y/n) : ");
						mode = ConfirmSave;
					} else
						Finish(All);
					break;
				case 'A':
				case 'a':
					mode = ClanName;
					d->Write("Enter new name:\r\n] ");
					break;
				case 'B':
				case 'b':
					mode = ClanDescription;
					value = 1;
					d->Write("Enter clan description: (/s saves /h for help)\r\n\r\n");
					tmpeditor = new TextEditor(d, &(clan->description), MAX_INPUT_LENGTH);
					d->Edit(tmpeditor);
					break;
				case 'C':
				case 'c':
					mode = ClanOwner;
					d->Write("Enter owners name: ");
					break;
				case 'D':
				case 'd':
					mode = ClanRoom;
					d->Write("Enter clan room vnum: ");
					break;
				case 'E':
				case 'e':
					mode = ClanSavings;
					d->Write("Enter clan's total savings: ");
					break;
				case 'F':
				case 'f':
					RankMenu();
					break;
				case 'G':
				case 'g':
					break;
				default:
					Menu();
					break;
			}
			return;
		
		case ClanName:
			if (*arg) {
				if (clan->name)	free(clan->name);
				clan->name = str_dup(arg);
			}
			break;
		case ClanDescription:
			mudlog("SYSERR: CEdit::Parse(): Reached DESCRIPTION case!", BRF, NULL, true);
			d->Write("Oops...\r\n");
			break;
		case ClanOwner:
			clan->owner = get_id_by_name(arg);
			break;
		case ClanRoom:
			clan->room = atoi(arg);
			break;
		case ClanSavings:
			clan->savings = atoi(arg);
			break;
		case ClanRanks:
			i = atoi(arg);
			if (i == 0)										Menu();
			else if ((i < 1) || (i > Clan::NumRanks - 1))	RankMenu();
			else {
				value = i;
				d->Writef("Enter new name for rank #%d", i);
				if (i == Clan::Rank::Member)			d->Write(" (Member)");
				else if (i == Clan::Rank::Commander)	d->Write(" (Commander)");
				else if (i == Clan::Rank::Leader)		d->Write(" (Leader)");
				d->Write(": ");
				mode = ClanRankEntry;
			}
			return;
		case ClanRankEntry:
			if (clan->ranks[value - 1])	free(clan->ranks[value - 1]);
			clan->ranks[value - 1] = str_dup(arg);
			RankMenu();
			return;
		default:
			mudlogf(BRF, NULL, true, "SYSERR: CEdit::Parse(): Reached default case!  Case: %d", mode);
			d->Write("Oops...\r\n");
			break;
	}
	value = 1;
	Menu();
}
