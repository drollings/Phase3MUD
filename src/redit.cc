/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*  _TwyliteMud_ by Rv.                          Based on CircleMud3.0bpl9 *
*    				                                          *
*  OasisOLC - redit.c 		                                          *
*    				                                          *
*  Copyright 1996 Harvey Gilpin.                                          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*. Original author: Levork .*/




#include "structs.h"
#include "objects.h"
#include "characters.h"
#include "comm.h"
#include "utils.h"
#include "scripts.h"
#include "db.h"
#include "boards.h"
#include "olc.h"
#include "buffer.h"
#include "extradesc.h"
#include "constants.h"


#include "redit.h"


/*------------------------------------------------------------------------*/

//	External functions.
extern int olc_edit_bitvector(Descriptor *d, Flags *result, char *argument,
		Flags old_bv, char *name, const char **values, Flags mask);
extern int olc_edit_value(Descriptor *d, long *result, char *argument,
		char *name, long min, long max, const char **values, bool diff = FALSE);

/*
 * External data structures.
 */
extern struct zone_data	*zone_table;
extern int		top_of_zone_table;

/*------------------------------------------------------------------------*/

/*
 * Function Prototypes
 */


/*------------------------------------------------------------------------*/

#define  W_EXIT(room, num) (world[(room)].dir_option[(num)])

/*------------------------------------------------------------------------*\
  Utils and exported functions.
\*------------------------------------------------------------------------*/

REdit::REdit(Descriptor *d, VNum newVNum, bool showmenu = true) : Editor(d), desc(NULL) {
	room = new Room;
//	room->name = SString::Create("An unfinished room");
//	room->description = SString::Create("You are in an unfinished room.\r\n");
	room->SetName("An unfinished room");
	room->SetDescription("You are in an unfinished room.\r\n");

	save = 0;
	room->Virtual(number = newVNum);
	
	if (showmenu)	Menu();
}

/*------------------------------------------------------------------------*/

REdit::REdit(Descriptor *d, VNum newVNum, Room &roomEntry, bool showmenu = true) : 
	Editor(d), desc(NULL) {
	//	Build a copy of the room for editing.
	room = new Room;
	
	//	Allocate space for all strings.
//	room->name = SString::Create(world[real_num].GetName("undefined"));
//	room->description = SString::Create(world[real_num].GetDesc("undefined"));
	room->SetName(roomEntry.Name("undefined"));
	room->SetDescription(roomEntry.Description("undefined"));	// FIXME!

	room->sector_type = roomEntry.sector_type;
	room->flags = roomEntry.flags;
	
	//	Exits - We allocate only if necessary.
	for (SInt32 counter = 0; counter < NUM_OF_DIRS; counter++)
		if (roomEntry.dir_option[counter])
			room->dir_option[counter] = new RoomDirection(roomEntry.dir_option[counter]);

	//	Extra descriptions, if necessary.
	// roomEntry.ex_description.Erase();
	if (roomEntry.ex_description.Count()) {
		ExtraDesc *extradesc;
		LListIterator<ExtraDesc *>	descIter(roomEntry.ex_description);
		while ((extradesc = descIter.Next()))
			room->ex_description.Append(new ExtraDesc(*extradesc));
	}

	if (roomEntry.Virtual() != newVNum) {
		if (Room::Find(newVNum))		save = OLC_SAVE_OVERWRITE;
		else							save = OLC_SAVE_COPY;
	}

	room->Virtual(number = newVNum);
	
	if (showmenu)	Menu();
}


/*------------------------------------------------------------------------*/

void REdit::SaveInternally(void) {
	Trigger *trig = NULL;
	REMOVE_BIT(room->flags, ROOM_DELETED);
	
	if (Room::Find(number)) {
		//	Room exists: move contents over then free and replace it.
		FreeWorldRoom(world[number]);
		
		world[number].sector_type = room->sector_type;
		world[number].flags = room->flags;
		world[number].func = room->func;
		world[number].ex_description = room->ex_description;
		
		for (SInt32 dirCounter = 0; dirCounter < NUM_OF_DIRS; dirCounter++)
			world[number].dir_option[dirCounter] = room->dir_option[dirCounter];

		if (world[number].name)			SSFree(world[number].name);
		world[number].name = room->name->Share();
		if (world[number].description)	SSFree(world[number].description);
		world[number].description = room->description->Share();
	} else {
		world[number] = *(room);
		world[number].Virtual(number);
		room = NULL;
	}

	if (world[number].script) {
		world[number].script->Extract();
		world[number].script = NULL;
	}

	if (triggers.Count()) {
		world[number].script = new Script(Datatypes::Room);
		for (int i = 0; i < triggers.Count(); ++i) {
			if ((trig = Trigger::Read(triggers[i]))) {
				add_trigger(world[number].script, trig);
			}
		}
	}

	olc_add_to_save_list(zone_table[zoneNum].number, OLC_REDIT);
}


void REdit::Finish(FinishMode mode) {
	if (room) {
		if (mode == Structs)	FreeRoom(room);
		delete room;
	}
	
	if (d->character) {
		STATE(d) = CON_PLAYING;
		act("$n stops using OLC.", TRUE, d->character, 0, 0, TO_ROOM);
	}
	Editor::Finish();
}


/*------------------------------------------------------------------------*/

void REdit::FreeRoom(Room *room) {
	SSFree(room->name);
	room->name = NULL;
	SSFree(room->description);
	room->description = NULL;
	for (SInt32 i = 0; i < NUM_OF_DIRS; ++i)
		room->dir_option[i] = NULL;
	room->people.Erase();
	room->contents.Erase();
	room->ex_description.Erase();
}


void REdit::FreeWorldRoom(Room &room) {
	SInt32 i;
	ExtraDesc *extradesc;
	
//	SSFree(room->name);
//	SSFree(room->description);
	if (room.name) {
		SSFree(room.name);
		room.name = NULL;
	}
	if (room.description) {
		SSFree(room.description);
		room.description = NULL;
	}

	for (i = 0; i < NUM_OF_DIRS; ++i)
		if (room.dir_option[i]) {
			delete room.dir_option[i];
			room.dir_option[i] = NULL;
		}

	while ((extradesc = room.ex_description.Top())) {
		room.ex_description.Remove(extradesc);
		delete extradesc;
	}
}


/**************************************************************************
 Menu functions 
 **************************************************************************/

/*
 * For extra descriptions.
 */
void REdit::DispExtradescMenu(void) {
	bool	next = false;
	
	LListIterator<ExtraDesc *>	descIter(room->ex_description);
	ExtraDesc *	temp;
	while ((temp = descIter.Next())) {
		if (temp == desc) {
			next = descIter.Peek();
			break;
		}
	}
	
	d->Writef(
#if defined(CLEAR_SCREEN)
			"\x1B[H\x1B[J"
#endif
			"&cG1&c0) Keyword: &cY%s\r\n"
			"&cG2&c0) Description:\r\n"
			"&cY%s\r\n"
			"&cG3&c0) Goto next description: %s\r\n"
			"&cG4&c0) Erase this description\r\n"
			"Enter choice (0 to quit): ",
			SSData(desc->keyword) ? SSData(desc->keyword) : "<NONE>",
			SSData(desc->description) ? SSData(desc->description) : "<NONE>",
			next ? "Set." : "<NOT SET>");
	
	mode = ExtradescMenu;
}


/*
 * For exits.
 */
void REdit::DispExitMenu(void) {
	char	*buf = get_buffer(MAX_STRING_LENGTH);

	/*
	 * if exit doesn't exist, alloc/create it 
	 */
	if(!OLC_EXIT)	OLC_EXIT = new RoomDirection;

	sprintbit(OLC_EXIT->exit_info, exit_bits, buf);
	
	*buf2 = '\0';	
	if (OLC_EXIT->key > 0) {
		if (Object::Find(OLC_EXIT->key)) {
			sprintf(buf2, " (%s)", Object::Index[OLC_EXIT->key].proto->Name());
		} else {
			strcpy(buf2, " &cR(Not found!)&c0");
		}
	}
  
	d->Writef(
#if defined(CLEAR_SCREEN)
				"\x1B[H\x1B[J"
#endif
				"&cG1&c0) Exit to       : &cC%d\r\n"
				"&cG2&c0) Description   :-\r\n&cY%s\r\n"
				"&cG3&c0) Door name     : &cY%s\r\n"
				"&cG4&c0) Key           : &cC%d%s\r\n"
				"&cG5&c0) Door flags    : &cC%s\r\n"
				"&cG6&c0) Material      : &cC%s\r\n"
				"&cG7&c0) Hit points    : &cC%d\r\n"
				"&cG8&c0) Dam resist    : &cC%d\r\n"
				"&cG9&c0) Pick modifier : &cC%d\r\n"
				"&cGX&c0) Purge exit.\r\n"
				"Enter choice, 0 to quit : ",
			OLC_EXIT->to_room != -1 ? OLC_EXIT->to_room : -1,
			OLC_EXIT->Description("<NONE>"),
			OLC_EXIT->Keyword("<NONE>"),
			OLC_EXIT->key,
			*buf2 ? buf2 : "",
			*buf ? buf : "None",
			material_types[OLC_EXIT->material],
			OLC_EXIT->max_hit_points,
			OLC_EXIT->dam_resist,
			OLC_EXIT->pick_modifier);
	release_buffer(buf);
	
	mode = ExitMenu;
}


/*
 * For exit flags.
 */
void REdit::DispExitFlagsMenu(void) {
#if defined(CLEAR_SCREEN)
	d->Write("\x1B[H\x1B[J");
#endif
	olc_edit_bitvector(d, &(OLC_EXIT->exit_info), "", OLC_EXIT->exit_info, 
					"exitflags", exit_bits, 0);
	mode = Room::RoomExitFlags;
}


/*
 * For room flags.
 */
void REdit::DispFlagsMenu(void) {
#if defined(CLEAR_SCREEN)
	d->Write("\x1B[H\x1B[J");
#endif
	olc_edit_bitvector(d, &(room->flags), "", room->flags, "flags", room_bits, 0);
	mode = Room::RoomFlagList;
}


//	For sector type.
void REdit::DispSectorsMenu(void) {
	long sector = room->sector_type;
#if defined(CLEAR_SCREEN)
	d->Write("\x1B[H\x1B[J");
#endif
	mode = Room::RoomSector;
	olc_edit_value(d, &(sector), "", "sector", 0, 0, sector_types);
	d->Write("Select a sector type : ");
}


//	The main menu.
void REdit::Menu(void) {
	char	*buf1 = get_buffer(256),
			*buf2 = get_buffer(256);

	sprintbit((long)room->flags, room_bits, buf1);
	sprinttype(room->sector_type, sector_types, buf2);
	d->Writef(
#if defined(CLEAR_SCREEN)
				"\x1B[H\x1B[J"
#endif
				"-- Room number : [&cC%d&c0]  	Room zone: [&cC%d&c0]\r\n"
				"&cG1&c0) Name        : &cY%s\r\n"
				"&cG2&c0) Description :\r\n&cY%s"
				"&cG3&c0) Room flags  : &cC%s\r\n"
				"&cG4&c0) Sector type : &cC%s\r\n"
				"&cG5&c0) Exit north  : &cC%d\r\n"
				"&cG6&c0) Exit east   : &cC%d\r\n"
				"&cG7&c0) Exit south  : &cC%d\r\n"
				"&cG8&c0) Exit west   : &cC%d\r\n"
				"&cG9&c0) Exit up     : &cC%d\r\n"
				"&cGA&c0) Exit down   : &cC%d\r\n"
				"&cGB&c0) Extra descriptions menu\r\n"
				"&cGP&c0) Special procedure : &cC%s\r\n",
			number, zone_table[zoneNum].number,
			room->Name("<ERROR>"),
			room->Description("<ERROR>"),
			buf1,
			buf2,
			room->dir_option[NORTH]	&& room->dir_option[NORTH]->to_room != -1	?	world[room->dir_option[NORTH]->to_room].Virtual()	: NOWHERE,
			room->dir_option[EAST]	&& room->dir_option[EAST]->to_room != -1	?	world[room->dir_option[EAST]->to_room].Virtual()	: NOWHERE,
			room->dir_option[SOUTH]	&& room->dir_option[SOUTH]->to_room != -1	?	world[room->dir_option[SOUTH]->to_room].Virtual()	: NOWHERE,
			room->dir_option[WEST]	&& room->dir_option[WEST]->to_room != -1	?	world[room->dir_option[WEST]->to_room].Virtual()	: NOWHERE,
			room->dir_option[UP]	&& room->dir_option[UP]->to_room != -1		?	world[room->dir_option[UP]->to_room].Virtual()		: NOWHERE,
			room->dir_option[DOWN]	&& room->dir_option[DOWN]->to_room != -1	?	world[room->dir_option[DOWN]->to_room].Virtual()	: NOWHERE,
			(func == NULL) ? "None" : spec_room_name(func));

	*buf1 = '\0';
	for (int i = 0; i < triggers.Count(); ++i) {
		sprintf(buf1 + strlen(buf1), "%d ", triggers[i]);
	}
	d->Writef("&cGS&c0) Scripts     : &cC%s\r\n"
				 "&cGQ&c0) Quit\r\n"
				 "Enter choice : ",
			*buf1 ? buf1 : "None");

	release_buffer(buf1);
	release_buffer(buf2);
	mode = MainMenu;
}



/**************************************************************************
 * The main loop
 **************************************************************************/

void REdit::Parse(char *arg) {
	SInt32	i;
	long tmp;
	Editor * tmpeditor = NULL;
	
	switch (mode) {
		case ConfirmSave:
			switch (*arg) {
				case 'y':
				case 'Y':
					mudlogf(NRM, d->character, true,  "OLC: %s edits room %d.", d->character->RealName(), number);
					d->Write("Room saved to memory.\r\n");
					SaveInternally();
					Finish(Structs);
					break;
				case 'n':
				case 'N':
					Finish(All);
					break;
				default:
					d->Write("Invalid choice!\r\n"
								 "Do you wish to save this room internally? : ");
					break;
			}
			return;
		case MainMenu:
			switch (*arg) {
				case 'q':
				case 'Q':
					if (save) { /* Something has been modified. */
						d->Write("Do you wish to save this room internally? : ");
						mode = ConfirmSave;
					} else
						Finish(All);
					break;
				case '1':
					d->Write("Enter room name:-\r\n| ");
					mode = Room::RoomName;
					break;
				case '2':
					mode = Room::RoomDesc;
					d->Write(
#if defined(CLEAR_SCREEN)
							"\x1B[H\x1B[J"
#endif
							"Enter room description: (/s saves /h for help)\r\n\r\n");
					tmpeditor = new SStringEditor(d, &room->description, MAX_ROOM_DESC);
					d->Edit(tmpeditor);
					if (!save)	save = OLC_SAVE_YES;
					break;
				case '3':
					DispFlagsMenu();
					break;
				case '4':
					DispSectorsMenu();
					break;
				case '5':
					value = NORTH;
					DispExitMenu();
					break;
				case '6':
					value = EAST;
					DispExitMenu();
					break;
				case '7':
					value = SOUTH;
					DispExitMenu();
					break;
				case '8':
					value = WEST;
					DispExitMenu();
					break;
				case '9':
					value = UP;
					DispExitMenu();
					break;
				case 'a':
				case 'A':
					value = DOWN;
					DispExitMenu();
					break;
				case 'b':
				case 'B':
					/*
					 * If the extra description doesn't exist.
					 */
					if (!(desc = room->ex_description.Top())) {
						room->ex_description.Append(desc = new ExtraDesc);
					}
					DispExtradescMenu();
					break;
				case 'p':
				case 'P':
					DispSpecProcMenu(d, room);
					mode = SpecProcMenu;
					d->Write("Enter the name of the desired specproc, or \"none\" to clear: ");
					return;
				case 's':
				case 'S':
					DispScriptsMenu(d, triggers);
					return;
				default:
					d->Write("Invalid choice!");
					Menu();
					break;
			}
			return;

		case Room::RoomName:
			if (strlen(arg) > MAX_ROOM_NAME)	arg[MAX_ROOM_NAME - 1] = 0;
//			SSFree(room->name);
//			room->name = SString::Create(*arg ? arg : "undefined");
			room->SetName(*arg ? arg : "undefined");
			break;
			
		case Room::RoomDesc:
			/*
			 * We will NEVER get here, we hope.
			 */
			mudlogf(BRF, NULL, true, "SYSERR: %s reached REDIT_DESC case in parse_redit", d->character->RealName());
			break;

		case Room::RoomFlagList:
			switch (*arg && olc_edit_bitvector(d, &(room->flags), arg, room->flags, "flags", room_bits, 0)) {
			case 0:
				Menu();
				return;
			default: 
				if (!save)	save = OLC_SAVE_YES;
				DispFlagsMenu();
				return;
			}

		case Room::RoomSector:
			tmp = (long) room->sector_type;
			switch (*arg && (*arg != '0') && olc_edit_value(d, &(tmp), arg, "sector", 0, 0, sector_types)) {
				default: 
				if (!save)	save = OLC_SAVE_YES;
				room->sector_type = (Sector) tmp;
				case 0: Menu(); return;
			}
			break;

		case ExitMenu:
			switch (*arg) {
				case 'q':
				case 'Q':
				case '0':
					break;
				case '1':
					mode = Room::RoomExitNumber;
					d->Write("Exit to room number : ");
					return;
				case '2':
					mode = Room::RoomExitDesc;
					d->Write("Enter exit description: (/s saves /h for help)\r\n\r\n");
					tmpeditor = new TextEditor(d, &(OLC_EXIT->general_description), MAX_EXIT_DESC);
					d->Edit(tmpeditor);
					return;
				case '3':
					mode = Room::RoomExitKeyword;
					d->Write("Enter keywords: ");
					return;
				case '4':
					mode = Room::RoomExitKey;
					d->Write("Enter key number: ");
					return;
				case '5':
					DispExitFlagsMenu();
					mode = Room::RoomExitFlags;
					return;
				case '6':
					olc_edit_value(d, &(tmp), "", "material", 0, 0, material_types);
					mode = Room::RoomExitMaterial;
					return;
				case '7':
					d->Write("Enter the exit's hit points : ");
					mode = Room::RoomExitHP;
					return;
				case '8':
					d->Write("Enter the exit's damage resistance : ");
					mode = Room::RoomExitDR;
					return;
				case '9':
					d->Write("Enter the exit's pick modifier on 3d6 (+ easier, - harder): ");
					mode = Room::RoomExitPick;
					return;
				case 'x':
				case 'X':
					delete OLC_EXIT;
					OLC_EXIT = NULL;
					break;
				default:
					DispExitMenu();
					return;
			}
			break;

		case Room::RoomExitNumber:
			if ((i = atoi(arg)) != NOWHERE) {
				if (!Room::Find(i)) {
					d->Write("That room does not exist, try again: ");
					return;
				}
			}
			OLC_EXIT->to_room = i;
			DispExitMenu();
			return;

		case Room::RoomExitDesc:
			/*
			 * We should NEVER get here, hopefully.
			 */
			mudlogf(BRF, NULL, true, "SYSERR: %s reached REDIT_EXIT_DESC case in parse_redit", d->character->RealName());
			break;

		case Room::RoomExitKeyword:
//			SSFree(exit->keyword);
//			exit->keyword = (*arg ? SString::Create(arg) : NULL);
			if (OLC_EXIT->keyword)	free(OLC_EXIT->keyword);
			OLC_EXIT->keyword = (*arg ? str_dup(arg) : NULL);
			DispExitMenu();
			return;

		case Room::RoomExitKey:
			i = atoi(arg);
			OLC_EXIT->key = i;
			DispExitMenu();
			return;

		case Room::RoomExitFlags:
			switch (*arg && olc_edit_bitvector(d, &(OLC_EXIT->exit_info), arg, 
					OLC_EXIT->exit_info, "exitflags", exit_bits, 0)) {
			case 0:
				DispExitMenu();
				return;
			default: 
				if (!save)	save = OLC_SAVE_YES;
				DispExitFlagsMenu();
				return;
			}

		case Room::RoomExitMaterial:
			tmp = (long) OLC_EXIT->material;
			switch (*arg && (*arg != '0') && olc_edit_value(d, &(tmp), arg, "material", 0, 0, material_types)) {
				default: 
				if (!save)	save = OLC_SAVE_YES;
				OLC_EXIT->material = (Materials::Material) tmp;
				case 0: DispExitMenu(); return;
			}
			break;
		
		case Room::RoomExitDR:
			i = MIN(MAX(atoi(arg), 0), 30);
			OLC_EXIT->dam_resist = i;
			DispExitMenu();
			return;

		case Room::RoomExitHP:
			i = MIN(MAX(atoi(arg), 0), 255);
			OLC_EXIT->max_hit_points = i;
			DispExitMenu();
			return;

		case Room::RoomExitPick:
			i = MIN(MAX(atoi(arg), -10), 10);
			OLC_EXIT->pick_modifier = i;
			DispExitMenu();
			return;


		case ExtradescKey:
			SSFree(desc->keyword);
			if (*arg)	desc->keyword = SString::Create(arg);
			else		desc->keyword = NULL;
			DispExtradescMenu();
			return;

		case ExtradescMenu:
			switch ((i = atoi(arg))) {
				case 0:
					/*
					 * If something got left out, delete the extra description
					 * when backing out to the menu.
					 */
					if (!SSData(desc->keyword) || !SSData(desc->description)) {
						room->ex_description.Remove(desc);
						delete desc;
						desc = NULL;
					}

/*					if (!SSData(desc->keyword) || !SSData(desc->description)) {
						ExtraDesc **tmp_desc;

						for(tmp_desc = &(room->ex_description); *tmp_desc; tmp_desc = &((*tmp_desc)->next))
							if(*tmp_desc == desc) {
								*tmp_desc = NULL;
								break;
							}
						delete desc;
						desc = NULL;
					}
*/
					break;
				case 1:
					mode = ExtradescKey;
					d->Write("Enter keywords, separated by spaces: ");
					return;
				case 2:
					mode = ExtradescDesc;
					d->Write("Enter extra description: (/s saves /h for help)\r\n\r\n");
					tmpeditor = new SStringEditor(d, &(desc->description), MAX_MESSAGE_LENGTH);
					d->Edit(tmpeditor);
					return;

				case 3:
					if (!SSData(desc->keyword) || !SSData(desc->description))
						d->Write("You can't edit the next extra desc without completing this one.\r\n");
					else {
						ExtraDesc *	newDesc = new ExtraDesc;
						room->ex_description.InsertAfter(newDesc, desc);
						desc = newDesc;
					}
					DispExtradescMenu();
					return;
				case 4:
					room->ex_description.Remove(desc);
					if (!(desc = room->ex_description.Top())) {
						room->ex_description.Append(desc = new ExtraDesc);
					}
					DispExtradescMenu();
					return;
			}
			break;

		case TriggersMenu:
		case TriggerAdd:
		case TriggerPurge:
			ParseScriptsMenu(d, arg, triggers);
			return;
		
		case SpecProcMenu:
			if (*arg) {
				ParseSpecProcMenu(d, arg, room);
				Menu();
				save = OLC_SAVE_YES;
			} else {
				Menu();
			}
			return;

		default:
			/* we should never get here */
			mudlogf(BRF, d->character, true, "SYSERR: %s reached default case in parse_redit", d->character->RealName());
			break;
	}
	/*
	 * If we get this far, something has be changed
	 */
	if (!save)	save = OLC_SAVE_YES;
	Menu();
}
