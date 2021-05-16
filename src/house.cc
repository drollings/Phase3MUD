/* ************************************************************************
*   File: house.c                                       Part of CircleMUD *
*  Usage: Handling of player houses                                       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */


#include "house.h"
#include "structs.h"
#include "utils.h"
#include "scripts.h"
#include "buffer.h"
#include "rooms.h"
#include "characters.h"
#include "objects.h"
#include "comm.h"
#include "handler.h"
#include "find.h"
#include "db.h"
#include "interpreter.h"
#include "constants.h"
#include "worldparser.h"
#include "files.h"

void hcontrol_list_houses(Character * ch);
void hcontrol_build_house(Character * ch, char *arg);
void hcontrol_destroy_house(Character * ch, char *arg);
void hcontrol_pay_house(Character * ch, char *arg);
ACMD(do_hcontrol);
ACMD(do_house);

LList<House *>	Houses;


House::House(void) : vnum(0), created(0), lastPayment(0), owner(0), maker(0),
	mode(HOUSE_PRIVATE) {
}

House::~House(void) {
}


void House::Parser(char *input) {
	char *	keyword = get_buffer(PARSER_BUFFER_LENGTH);
	SInt32	temp;

	while (*input /* && !Parser::error */) {
		input = Parser::Parse(input, keyword);
		
		if (!*keyword)	break;
		
		if (!str_cmp(keyword, "owner")) {
			input = Parser::GetKeyword(input, keyword);
			owner = atoi(keyword);
		} else if (!str_cmp(keyword, "maker")) {
			input = Parser::GetKeyword(input, keyword);
			maker = atoi(keyword);
		} else if (!str_cmp(keyword, "vnum")) {
			input = Parser::GetKeyword(input, keyword);
			vnum = atoi(keyword);
		} else if (!str_cmp(keyword, "created")) {
			input = Parser::GetKeyword(input, keyword);
			created = atoi(keyword);
		} else if (!str_cmp(keyword, "lastpayment")) {
			input = Parser::GetKeyword(input, keyword);
			lastPayment = atoi(keyword);
		} else if (!str_cmp(keyword, "guests")) {
			input = Parser::ReadBlock(input, keyword);
			char *	item = get_buffer(PARSER_BUFFER_LENGTH);
			for (char *itemPtr = Parser::GetKeyword(keyword, item); *item;
					itemPtr = Parser::GetKeyword(itemPtr, item)) {
				temp = atoi(item);
				if (*get_name_by_id(temp) != '<')
					guests.Append(temp);
			}
			release_buffer(item);
		} else {	//	ERROR HANDLER - Unknown keyword
			Parser::Error("Unknown keyword \"%s\"", keyword);
		}
		while (*input && (*input != ';'))			input++;
		while (isspace(*input) || *input == ';')	input++;
	}
	release_buffer(keyword);
}


bool House::GetFilename(VNum vnum, char *filename) {
	if (vnum < 0)	return false;

	sprintf(filename, HOUSE_DIR "%d.house", vnum);
	
	return true;
}


#define MAX_BAG_ROW 5

/* Load all objects for a house */
bool House::Load(VNum vnum) {
	FILE *	fl;
	char *	fname, *line;
	bool	read_file = true;
//	bool	result = true;
	Object *obj, *obj1;
	LList<Object *>	cont_row[MAX_BAG_ROW];
	SInt32	locate, j, obj_vnum;

	if (!Room::Find(vnum))
		return false;
	if (!House::GetFilename(vnum, (fname = get_buffer(MAX_STRING_LENGTH)))) {
		release_buffer(fname);
		return false;
	}
	
	if (!(fl = fopen(fname, "r+b"))) {
		release_buffer(fname);
		return false;
	}
	
	line = get_buffer(MAX_INPUT_LENGTH);
	
//	for (j = 0;j < MAX_BAG_ROW;j++)
//		cont_row[j] = NULL; /* empty all cont lists (you never know ...) */

	while (get_line(fl, line) && read_file) {
		switch (*line) {
			case '#':
				if (sscanf(line, "#%d %d", &obj_vnum, &locate) < 1) {
					log("House file %s is damaged: no vnum", fname);
					read_file = false;
					if ((obj_vnum == -1) || !Object::Find(obj_vnum)) {
						obj = new Object();
						obj->SetKeywords("Empty.");
						obj->SetName("Empty.");
						obj->SetSDesc("Empty.\r\n");
					} else
						obj = Object::Read(obj_vnum);
						
					read_file = obj->Load(fl, fname);
					
					for (j = MAX_BAG_ROW-1;j > -locate;j--)
						if (cont_row[j].Count()) { /* no container -> back to ch's inventory */
//							for (;cont_row[j];cont_row[j] = obj1) {
//								obj1 = cont_row[j]->next_content;
//								cont_row[j]->ToRoom(vnum);
//							}
//							cont_row[j] = NULL;
							while((obj1 = cont_row[j].Top())) {
								cont_row[j].Remove(obj1);
								obj1->ToRoom(vnum);
							}
						}

					if (j == -locate && cont_row[j].Count()) { /* content list existing */
						if (IS_CONTAINER(obj)) {
							/* take item ; fill ; give to char again */
							obj->FromChar();
//							for (;cont_row[j];cont_row[j] = obj1) {
//								obj1 = cont_row[j]->next_content;
//								cont_row[j]->ToObj(obj);
//							}
							while((obj1 = cont_row[j].Top())) {
								cont_row[j].Remove(obj1);
								obj1->ToObj(obj);
							}
							obj->ToRoom(vnum); /* add to inv first ... */
						} else { /* object isn't container -> empty content list */
//							for (;cont_row[j];cont_row[j] = obj1) {
//								obj1 = cont_row[j]->next_content;
//								cont_row[j]->ToRoom(vnum);
//							}
//							cont_row[j] = NULL;
							while((obj1 = cont_row[j].Top())) {
								cont_row[j].Remove(obj1);
								obj1->ToRoom(vnum);
							}
						}
					}

					if (locate < 0 && locate >= -MAX_BAG_ROW) {
						/*	let obj be part of content list but put it at the list's end
						 *	thus having the items
						 *	in the same order as before renting
						 */
						obj->FromChar();
//						if ((obj1 = cont_row[-locate-1])) {
//							while (obj1->next_content)
//								obj1 = obj1->next_content;
//							obj1->next_content = obj;
//						} else
//							cont_row[-locate-1] = obj;
						cont_row[-locate-1].Prepend(obj);
					}
				}
				break;
			case '$':
				read_file = false;
				break;
			default:
				log("House file %s is damaged: bad line %s", fname, line);
				read_file = false;
		}
	}

	release_buffer(fname);
	release_buffer(line);
	fclose(fl);
	return true;
}


/* Save all objects for a house (recursive; initial call must be followed
   by a call to House_restore_weight)  Assumes file is open already. */
void House::Save(Object * obj, FILE * fp, int locate) {
	Object *tmp;

	if (obj) {
		START_ITER(iter, tmp, Object *, obj->contents)
			Save(tmp, fp, MIN(0, locate)-1);
		
		obj->Save(fp, locate);

		for (tmp = obj->InObj(); tmp; tmp = tmp->InObj())
			GET_OBJ_WEIGHT(tmp) -= GET_OBJ_WEIGHT(obj);
	}
}


/* restore weight of containers after House_save has changed them for saving */
void House::RestoreWeight(Object * obj) {
	Object *tmp;
	if (obj) {
		START_ITER(iter, tmp, Object *, obj->contents) {
			RestoreWeight(tmp);
		}
		if (obj->InObj())
			GET_OBJ_WEIGHT(obj->InObj()) += GET_OBJ_WEIGHT(obj);
	}
}


/* Save all objects in a house */
void House::CrashSave(VNum vnum) {
	char *buf;
	FILE *fp;
	Object *obj;

	if (!Room::Find(vnum))
		return;
	
	buf = get_buffer(MAX_STRING_LENGTH);
	if (GetFilename(vnum, buf)) {
		if (!(fp = fopen(buf, "wb")))
			perror("SYSERR: Error saving house file");
		else {
			START_ITER(save_iter, obj, Object *, world[vnum].contents)
				Save(obj, fp, 0);
			fclose(fp);
			START_ITER(weight_iter, obj, Object *, world[vnum].contents)
				RestoreWeight(obj);
		}
		REMOVE_BIT(ROOM_FLAGS(vnum), ROOM_HOUSE_CRASH);
	}
	release_buffer(buf);
}


/* Delete a house save file */
void House::DeleteFile(VNum vnum) {
	char	*buf = get_buffer(MAX_INPUT_LENGTH),
			*fname = get_buffer(MAX_INPUT_LENGTH);
	FILE *fl;

	if (GetFilename(vnum, fname)) {
		if (!(fl = fopen(fname, "rb"))) {
			if (errno != ENOENT) {
				sprintf(buf, "SYSERR: Error deleting house file #%d. (1)", vnum);
				perror(buf);
			}
		} else {
			fclose(fl);
			if (remove(fname) < 0) {
				sprintf(buf, "SYSERR: Error deleting house file #%d. (2)", vnum);
				perror(buf);
			}
		}
	}
	release_buffer(buf);
	release_buffer(fname);
}


/* List all objects in a house file */
void House::Listrent(Character * ch, VNum vnum) {
	FILE *fl;
	char *fname = get_buffer(MAX_STRING_LENGTH), *buf;
	Object *obj = NULL;
	char *line;
	bool read_file = true;
	SInt32	obj_vnum;

	if (!GetFilename(vnum, fname)) {
		release_buffer(fname);
		return;
	}
	fl = fopen(fname, "rb");
	if (!fl) {
		ch->Sendf("No objects on file for house #%d.\r\n", vnum);
		release_buffer(fname);
		return;
	}
	
	buf = get_buffer(MAX_STRING_LENGTH);
	line = get_buffer(MAX_INPUT_LENGTH);
	while (get_line(fl, line) && read_file) {
		switch (*line) {
			case '#':
				if (sscanf(line, "#%d", &obj_vnum) < 1) {
					log("House file %s is damaged: no vnum", fname);
					read_file = false;
				} else {
					if (!(obj = Object::Read(obj_vnum))) {
						obj = new Object();
						obj->SetKeywords("Empty.");
						obj->SetName("Empty.");
						obj->SetSDesc("Empty.");
					}
					if (obj->Load(fl, fname)) {
						sprintf(buf + strlen(buf), " [%5d] %-20s\r\n", obj->Virtual(), obj->Name());
					} else {
						log("House file %s is damaged: failed to load obj #%d", fname, obj_vnum);
						read_file = false;
					}
					obj->Extract();
				}
				break;
			case '$':
				read_file = false;
				break;
			default:
				log("Player object file %s is damaged: bad line %s", fname, line);
				read_file = false;
		}
	}

	ch->Send(buf);
	release_buffer(buf);
	release_buffer(line);
	release_buffer(fname);
	fclose(fl);
}




/******************************************************************
 *  Functions for house administration (creation, deletion, etc.  *
 *****************************************************************/

House *House::Find(VNum vnum) {
	House *	house;
	LListIterator<House *>	iter(Houses);
	
	while ((house = iter.Next())) {
		if (house->vnum == vnum)
			return house;
	}

	return NULL;
}



/* Save the house control information */
void House::SaveHouses(void) {
	FILE *	fl;
	House *	house;

	if (!(fl = fopen(HCONTROL_FILE, "wb"))) {
		perror("SYSERR: Unable to open house control file");
		return;
	}

	LListIterator<House *>	iter(Houses);
	while ((house = iter.Next())) {
		fprintf(fl,
//				"House = {\n"
				"{\n"
				"\tvnum = %d;\n"
				"\towner = %d;\n"
				"\tmaker = %d;\n"
				"\tcreated = %ld;\n"
				"\tlastpayment = %ld;\n"
				"\tmode = %d;\n",
				house->vnum,
				house->owner,
				house->maker,
				house->created,
				house->lastPayment,
				house->mode);
		if (house->guests.Count()) {
			fprintf(fl, "\tguests = { ");
			for (int i = 0; i < house->guests.Count(); i++)
				fprintf(fl, "%d ", house->guests[i]);
			fprintf(fl, "};\n");
		}
		fprintf(fl, "};\n\n");
	}

	fclose(fl);
}


/* call from boot_db - will load control recs, load objs, set atrium bits */
/* should do sanity checks on vnums & remove invalid records */
void House::Boot(void) {
	House *	house;
	FILE *	fl;

	if (!(fl = fopen(HCONTROL_FILE, "rb"))) {
		if (errno == ENOENT)	log("House control file " HCONTROL_FILE " does not exist.");
		else					perror(HCONTROL_FILE);
		return;
	}

	char *	block = get_buffer(MAX_STRING_LENGTH * 4);
	Parser::file = HCONTROL_FILE;

	while (!feof(fl)) {
//		char *word = fread_word(fl);
//		if (str_cmp(word, "House"))														break;
		
		if (fread_keyblock(block, fl, "Error reading file...", HCONTROL_FILE) == -1)	break;
		
		house = new House();
		house->Parser(block);
		
		Parser::error = false;
		
		if (!get_name_by_id(house->owner)) {
			delete house;
			continue;	//	owner no longer exists -- skip
		} else if (!Room::Find(house->vnum)) {
			delete house;
			continue;	//	this vnum doesn't exist -- skip
		} else if (House::Find(house->vnum)) {
			delete house;
			continue;	//	this vnum is already a hosue -- skip
		}

		Houses.Append(house);
		
		SET_BIT(ROOM_FLAGS(house->vnum), ROOM_HOUSE | ROOM_PRIVATE);
		House::Load(house->vnum);
	}
	release_buffer(block);

	fclose(fl);
	House::SaveHouses();
}



/* "House Control" functions */

const char *HCONTROL_FORMAT =
"Usage: hcontrol build <house vnum> <exit direction> <player name>\r\n"
"       hcontrol destroy <house vnum>\r\n"
"       hcontrol pay <house vnum>\r\n"
"       hcontrol show\r\n";

#define NAME(x) ((temp = get_name_by_id(x)) ? temp : "<UNDEF>")

void hcontrol_list_houses(Character * ch) {
	char *timestr, *built_on, *last_pay, *own_name, *buf;
	const char *temp;
	
	if (!Houses.Count()) {
		ch->Send("No houses have been defined.\r\n");
		return;
	}
	built_on = get_buffer(128);
	last_pay = get_buffer(128);
	own_name = get_buffer(128);
	buf = get_buffer(MAX_STRING_LENGTH);

	strcpy(buf, "Address  Build Date  Guests  Owner         Last Paymt\r\n"
				"-------  ----------  ------  ------------  ----------\r\n");

	House *house;
	LListIterator<House *>	iter(Houses);
	while ((house = iter.Next())) {
		//	Avoid seeing <UNDEF> entries from self-deleted people. -gg 6/21/98
		if (!(temp = get_name_by_id(house->owner)))
			continue;

		if (house->created) {
			timestr = asctime(localtime(&(house->created)));
			*(timestr + 10) = '\0';
			strcpy(built_on, timestr);
		} else
			strcpy(built_on, "Unknown");

		if (house->lastPayment) {
			timestr = asctime(localtime(&(house->lastPayment)));
			*(timestr + 10) = '\0';
			strcpy(last_pay, timestr);
		} else
			strcpy(last_pay, "None");

		strcpy(own_name, temp);

		sprintf(buf + strlen(buf), "%7d  %-10s  %6d  %-12s  %s\r\n",
				house->vnum, built_on,
				house->guests.Count(), CAP(own_name), last_pay);

		house->ListGuests(buf,true);
	}
	page_string(ch->desc, buf, true);
	
	release_buffer(buf);
	release_buffer(built_on);
	release_buffer(last_pay);
	release_buffer(own_name);
}



void hcontrol_build_house(Character * ch, char *arg) {
	char *	arg1;
	House *	house;
	VNum	virt_house;
	SInt32	owner;

	arg1 = get_buffer(MAX_INPUT_LENGTH);
	/* first arg: house's vnum */
	arg = one_argument(arg, arg1);
	if (!*arg1) {
		ch->Send(HCONTROL_FORMAT);
		release_buffer(arg1);
		return;
	}
	virt_house = atoi(arg1);
	release_buffer(arg1);
	if (!Room::Find(virt_house)) {
		ch->Send("No such room exists.\r\n");
		return;
	}
	if (House::Find(virt_house)) {
		ch->Send("House already exists.\r\n");
		return;
	}

/*	arg1 = get_buffer(MAX_INPUT_LENGTH);
	// second arg: direction of house's exit
	arg = one_argument(arg, arg1);
	if (!*arg1) {
		ch->Send(HCONTROL_FORMAT);
		release_buffer(arg1);
		return;
	}
	if ((exit_num = search_block(arg1, (const char **)dirs, FALSE)) < 0) {
		char *buf = get_buffer(SMALL_BUFSIZE);
		sprintf(buf, "'%s' is not a valid direction.\r\n", arg1);
		ch->Send(buf);
		release_buffer(buf);
		release_buffer(arg1);
		return;
	}
	release_buffer(arg1);
	if (TOROOM(virt_house, exit_num) == NOWHERE) {
		char *buf = get_buffer(SMALL_BUFSIZE);
		sprintf(buf, "There is no exit %s from room %d.\r\n", dirs[exit_num], virt_house);
		ch->Send(buf);
		release_buffer(buf);
		return;
	}
*/

//	real_atrium = TOROOM(real_house, exit_num);
//	virt_atrium = world[real_atrium].number;

//	if (TOROOM(real_atrium, rev_dir[exit_num]) != real_house) {
//		ch->Send("A house's exit must be a two-way door.\r\n");
//		return;
//	}

	arg1 = get_buffer(MAX_INPUT_LENGTH);
	/* third arg: player's name */
	arg = one_argument(arg, arg1);
	if (!*arg1) {
		ch->Send(HCONTROL_FORMAT);
		release_buffer(arg1);
		return;
	}
	if ((owner = get_id_by_name(arg1)) < 0) {
		char *buf = get_buffer(SMALL_BUFSIZE);
		sprintf(buf, "Unknown player '%s'.\r\n", arg1);
		ch->Send(buf);
		release_buffer(buf);
		release_buffer(arg1);
		return;
	}
	release_buffer(arg1);

	house = new House;
	house->mode = HOUSE_PRIVATE;
	house->vnum = virt_house;
	house->maker = GET_ID(ch);
	house->created = time(0);
	house->lastPayment = 0;
	house->owner = owner;

	Houses.Add(house);

	SET_BIT(ROOM_FLAGS(virt_house), ROOM_HOUSE | ROOM_PRIVATE);
//	SET_BIT(ROOM_FLAGS(real_atrium), ROOM_ATRIUM);
	House::CrashSave(virt_house);

	ch->Send("House built.  Mazel tov!\r\n");
	House::SaveHouses();
}



void hcontrol_destroy_house(Character * ch, char *arg) {
	House *	house;

	if (!*arg) {
		ch->Send(HCONTROL_FORMAT);
		return;
	}
	if (!(house = House::Find(atoi(arg)))) {
		ch->Send("Unknown house.\r\n");
		return;
	}

	if (!Room::Find(house->vnum))
		log("SYSERR: House %d had invalid vnum %d!", atoi(arg), house->vnum);
	else
		REMOVE_BIT(ROOM_FLAGS(house->vnum), ROOM_HOUSE | ROOM_PRIVATE | ROOM_HOUSE_CRASH);

	House::DeleteFile(house->vnum);

	Houses.Remove(house);

	ch->Send("House deleted.\r\n");
	House::SaveHouses();
}


void hcontrol_pay_house(Character * ch, char *arg) {
	House *	house;

	if (!*arg)										ch->Send(HCONTROL_FORMAT);
	else if (!(house = House::Find(atoi(arg))))		ch->Send("Unknown house.\r\n");
	else {
		mudlogf( NRM, ch, TRUE,  "Payment for house %s collected by %s.", arg, ch->RealName());

		house->lastPayment = time(0);
		House::SaveHouses();
		ch->Send("Payment recorded.\r\n");
	}
}


/* The hcontrol command itself, used by imms to create/destroy houses */
ACMD(do_hcontrol) {
	char	*arg1 = get_buffer(MAX_INPUT_LENGTH),
			*arg2 = get_buffer(MAX_INPUT_LENGTH);

	half_chop(argument, arg1, arg2);

	if (is_abbrev(arg1, "build"))			hcontrol_build_house(ch, arg2);
	else if (is_abbrev(arg1, "destroy"))	hcontrol_destroy_house(ch, arg2);
	else if (is_abbrev(arg1, "pay"))		hcontrol_pay_house(ch, arg2);
	else if (is_abbrev(arg1, "show"))		hcontrol_list_houses(ch);
	else									ch->Send(HCONTROL_FORMAT);
	
	release_buffer(arg1);
	release_buffer(arg2);
}


/* The house command, used by mortal house owners to assign guests */
ACMD(do_house) {
	House *	house = NULL;
	IDNum	id;
	char	*arg = get_buffer(MAX_INPUT_LENGTH),
			*buf = get_buffer(MAX_STRING_LENGTH);


	one_argument(argument, arg);

	if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_HOUSE))
		ch->Send("You must be in your house to set guests.\r\n");
	else if (!(house = House::Find(IN_ROOM(ch))))
		ch->Send("Um.. this house seems to be screwed up.\r\n");
	else if (ch->ID() != house->owner)
		ch->Send("Only the primary owner can set guests.\r\n");
	else if (!*arg) {
		house->ListGuests(buf, false);
		page_string(ch->desc, buf, true);
	} else if ((id = get_id_by_name(arg)) < 0)
		ch->Send("No such player.\r\n");
	else if (id == GET_IDNUM(ch))
		ch->Send("It's your house!");
	else {
		if (house->guests.Contains(id)) {
			house->guests.Remove(id);
			House::SaveHouses();
			ch->Send("Guest deleted.\r\n");
		} else if (house->guests.Count() == MAX_GUESTS)
			ch->Send("You have too many guests.\r\n");
		else {
			house->guests.Add(id);
			House::SaveHouses();
			ch->Send("Guest added.\r\n");
		}
	}
	release_buffer(buf);
	release_buffer(arg);
}



/* Misc. administrative functions */


/* crash-save all the houses */
void House::SaveAll(void) {
	House *	house;
	LListIterator<House *>	iter(Houses);
	while ((house = iter.Next()))
		if (Room::Find(house->vnum))
			if (ROOM_FLAGGED(house->vnum, ROOM_HOUSE_CRASH))
				House::CrashSave(house->vnum);
}


/* note: arg passed must be house vnum, so there. */
bool House::CanEnter(Character * ch, VNum vnum) {
	House *	house = NULL;

	if (STF_FLAGGED(ch, Staff::Admin | Staff::Security | Staff::Houses) ||
			!(house = House::Find(vnum)))
		return true;

	switch (house->mode) {
		case HOUSE_PRIVATE:
			if (ch->ID() == house->owner || house->guests.Contains(ch->ID()))
				return true;
			break;
	}

	return false;
}


void House::ListGuests(char *buf, bool quiet) {
	SInt32			j;
	const char *	temp;
	char *			name;
	
	if (!guests.Count()) {
		if (!quiet)
			strcat(buf, "  Guests: None\r\n");
		return;
	}
	
	name = get_buffer(MAX_INPUT_LENGTH);
	
	strcat(buf, "  Guests: ");
	for (j = 0; j < guests.Count(); j++) {
		if ((temp = get_name_by_id(guests[j]))) {
			sprintf(name, "%s ", temp);
			strcat(buf, CAP(name));
		}
	}
	strcat(buf, "\r\n");
	
	release_buffer(name);
}
