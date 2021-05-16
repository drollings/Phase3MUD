/***************************************************************************\
[*]    __     __  ___                ___  | LexiMUD is a C++ MUD codebase [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ |        for Sci-Fi MUDs        [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / | Copyright(C) 1997-1999        [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  | All rights reserved           [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   | Chris Jacobson (FearItself)   [*]
[*]      A Creation of the AvP Team!      | fear@technologist.com         [*]
[-]---------------------------------------+-------------------------------[-]
[*] File : rooms.c++                                                      [*]
[*] Usage: Room code                                                      [*]
\***************************************************************************/


#include "rooms.h"
#include "characters.h"
#include "objects.h"
#include "descriptors.h"
#include "utils.h"
#include "buffer.h"
#include "db.h"
#include "extradesc.h"
#include "files.h"
#include "find.h"
#include "interpreter.h"
#include "scripts.h"
#include "constants.h"

Map<VNum, Room>	world;
extern struct zone_data *zone_table;	//	zone table
extern int top_of_zone_table;			//	top element of zone table
extern bool mini_mud;


Room::Room(void) : GameData(-1), /*number(-1), */func(NULL), farg(NULL), zone(0), 
					coordx(-1), coordy(-1) {
	for (SInt32 x = 0; x < NUM_OF_DIRS; x++)
		dir_option[x] = NULL;
}


Room::~Room(void) {
	ExtraDesc *extradesc;

	if (name) {
		SSFree(name);
		name = NULL;
	}
	
	if (description) {
		SSFree(description);
		description = NULL;
	}
	
	//	Free exits.
	for (SInt32 i = 0; i < NUM_OF_DIRS; i++) {
		if (dir_option[i])	delete dir_option[i];
	}

	//	Free extra descriptions.
	while ((extradesc = ex_description.Top())) {
		ex_description.Remove(extradesc);
		delete extradesc;
	}
	
	if (contents.Count()) {
		log("SYSERR:  Destroying room with contents.");
		core_dump();
		exit(1);
	}

	if (script)	delete script;
}


SInt32 Room::Send(const char *messg, ...) const {
	Character *i;
	va_list args;
	char *send_buf;
	SInt32	length = 0;
	Descriptor *	d;
	
	if (!messg || !*messg || !this || !people.Count())
		return 0;
 
 	send_buf = get_buffer(MAX_STRING_LENGTH);
 	
	va_start(args, messg);
	length += vsprintf(send_buf, messg, args);
	va_end(args);
	
	LListIterator<Character *>	iter(people);
	
	while ((i = iter.Next())) {
		d = i->desc;
		if (d && (STATE(d) == CON_PLAYING) && !PLR_FLAGGED(i, PLR_WRITING) && AWAKE(i))
			d->Write(send_buf);
	}
	
	release_buffer(send_buf);

	return length;
}


RoomDirection::RoomDirection(void) : exit_info(0), key(0), to_room(-1), material(Materials::Undefined),
	max_hit_points(0), hit_points(0), dam_resist(0), pick_modifier(0), general_description(NULL),
	keyword(NULL) 
{

//	general_description = NULL;
//	keyword = NULL;
//	exit_info = 0;
//	key = 0;
//	to_room = -1;
}


RoomDirection::RoomDirection(const RoomDirection *dir)
{
	*this = *dir;
	general_description = str_dup(dir->general_description);
	keyword = str_dup(dir->keyword);
//	general_description = SString::Create(SSData(dir->general_description));
//	keyword = SString::Create(SSData(dir->keyword));
};

RoomDirection::~RoomDirection(void) {
	if (general_description)	free(general_description);
	if (keyword)				free(keyword);
//	SSFree(general_description);
//	SSFree(keyword);
}



RoomInterior::RoomInterior(void) : sector_type(SECT_FIELD), flags(0), light(1), description(NULL)
{
}


RoomInterior::RoomInterior(const RoomInterior &ri) :
	sector_type(ri.sector_type), flags(ri.flags), light(1)
{
	description = ri.description->Share();
}


RoomInterior::~RoomInterior(void) 
{
	if (people.Count()) {
		log("SYSERR:  Destroying room interior with occupants.");
		core_dump();
		exit(1);
	}
	SSFree(description);
}


Character *	Room::TargetChar(const char *arg) const	{ return get_char_by_room(this, arg);	}
Object *	Room::TargetObj(const char *arg) const	{ return get_obj_by_room(this, arg);	}

Room *		Room::TargetRoom(const char *arg) const {
	VNum		location = NOWHERE;
	Character *	targetMob;
	Object *	targetObj;
	
	if (is_number(arg) && !strchr(arg, '.'))	location = atoi(arg);
	else if ((targetMob = TargetChar(arg)))		location = IN_ROOM(targetMob);
	else if ((targetObj = TargetObj(arg)))		location = IN_ROOM(targetObj);

	return Room::Find(location) ? &(world[location]) : NULL;
}


// rdig support
RoomDirection *	Room::MakeDirection(SInt8 dir, VNum toroom, Flags doorflags = 0) {
	if (dir_option[dir])	delete dir_option[dir];

	dir_option[dir] = new RoomDirection;
	dir_option[dir]->to_room = toroom;
	dir_option[dir]->exit_info = doorflags;
	
	return dir_option[dir];
}


void Room::RemoveDirection(SInt8 dir) {
	if (dir_option[dir]) {
		delete dir_option[dir];
		dir_option[dir] = NULL;
	}
}


/* make sure the start rooms exist & resolve their vnums to rnums */
void check_start_rooms(void) {
	if (!Room::Find(mortal_start_room)) {
		log("SYSERR:  Mortal start room does not exist.  Change in config.c.");
		exit(1);
	}
	if (!Room::Find(immort_start_room)) {
		if (!mini_mud)
			log("SYSERR:  Warning: Immort start room does not exist.  Change in config.c.");
		immort_start_room = mortal_start_room;
	}
	if (!Room::Find(frozen_start_room)) {
		if (!mini_mud)
			log("SYSERR:  Warning: Frozen start room does not exist.  Change in config.c.");
		frozen_start_room = mortal_start_room;
	}
	if (!Room::Find(jailed_start_room)) {
		if (!mini_mud)
			log("SYSERR:  Warning: Jailed start room does not exist.  Change in config.c.");
		jailed_start_room = mortal_start_room;
	}

}




/* returns the real number of the object with given virtual number */
/* reditmod - changed binary search to incremental                 */
/* -JEE- 0.4 - changed to binary & incremental                     */
/* thanks to Johan Eenfeldt for the idea and the code :)  *.06*    */
/*
RNum Room::Real(VNum vnum) {
	int bot, top, mid, nr, found = NOWHERE;

	// First binary search.
	bot = 0;
	top = top_of_world;

	for (;;) {
		mid = (bot + top) / 2;
		
		if (mid == -1) {
			log("SYSERR: Room::Real mid == -1!  (vnum == %d)", vnum);
			break;
		}
		if ((world + mid)->number == vnum)
			return mid;
		if (bot >= top)
			break;
		if ((world + mid)->number > vnum)
			top = mid - 1;
		else
			bot = mid + 1;
	}

	// If not found - use linear on the "new" parts.
	for(nr = 0; nr <= top_of_world; nr++) {
		if(world[nr].number == vnum) {
			found = nr;
			break;
		}
	}
	return(found);
}
*/


void fprintstring(FILE *fp, const char *fmt, const char *txt);
void fprintbits(FILE *fp, const char *fmt, UInt32 bits, const char *bitnames[]);

void Room::SaveDisk(Room *room, FILE *fp)
{
	int counter2;
	char *bitList = get_buffer(32);
	ExtraDesc *ex_desc;
	Trigger *t;

	if (room->Virtual() == NOWHERE) {
		core_dump();
		exit(1);
	}

	fprintf(fp, "#%d = {\n", room->Virtual());
	fprintstring(fp, "\tname = %s;\n", room->Name("Undefined"));
	fprintstring(fp, "\tdesc = %s;\n", room->Description("Empty"));
	fprintf(fp, "\tsector = \"%s\";\n", sector_types[room->SectorType()]);
	
	if (room->RoomFlags())	fprintbits(fp, "\tflags = { %s };\n", room->RoomFlags(), room_bits);

	fprintf(fp, "\texits = {\n");
	for (counter2 = 0; counter2 < NUM_OF_DIRS; ++counter2) {
		if (!room->dir_option[counter2])
			continue;
		
		fprintf(fp, "\t\t%s = {\n", dirs[counter2]);

		if (room->dir_option[counter2]->Keyword())
			fprintstring(fp, "\t\t\tkeyword = %s;\n", room->dir_option[counter2]->Keyword());
		if (room->dir_option[counter2]->Description())
			fprintstring(fp, "\t\t\tdesc = %s;\n", room->dir_option[counter2]->Description());

		UInt32 temp_door_flag = room->dir_option[counter2]->exit_info & ~(Exit::Closed | Exit::Locked);
		
		if (temp_door_flag)
			fprintbits(fp, "\t\t\tflags = { %s };\n", temp_door_flag, exit_bits);
		if (room->dir_option[counter2]->key)
			fprintf(fp, "\t\t\tkey = %d;\n", room->dir_option[counter2]->key);
		if (room->dir_option[counter2]->to_room != -1)
			fprintf(fp, "\t\t\troom = %d;\n", room->dir_option[counter2]->to_room);

		if (room->dir_option[counter2]->material != Materials::Undefined)
			fprintf(fp, "\t\t\tmaterial = \"%s\";\n", material_types[(int) room->dir_option[counter2]->material]);
		if (room->dir_option[counter2]->dam_resist)
			fprintf(fp, "\t\t\tdr = %d;\n", room->dir_option[counter2]->dam_resist);
		if (room->dir_option[counter2]->pick_modifier)
			fprintf(fp, "\t\t\tpickmod = %d;\n", room->dir_option[counter2]->pick_modifier);
		if (room->dir_option[counter2]->max_hit_points)
			fprintf(fp, "\t\t\thp = %d;\n", room->dir_option[counter2]->max_hit_points);

		fprintf(fp, "\t\t};\n");
	}
	fprintf(fp, "\t};\n");
	
	if (room->coordx > -1) {
		fprintf(fp, "\tcoord = %d %d;\n",
				room->coordx, room->coordy);
	}
	
	if (SCRIPT(room)) {
		LListIterator<Trigger *>		triggers(TRIGGERS(SCRIPT(room)));
		fprintf(fp, "\ttriggers = { ");
		while ((t = triggers.Next())) {
			fprintf(fp, "%d ", t->Virtual());
		}
		fprintf(fp, "};\n");
	}

	if (room->ex_description.Count()) {
		fprintf(fp, "\texdesc = {\n");
		LListIterator<ExtraDesc *>	descIter(room->ex_description);
		while ((ex_desc = descIter.Next())) {
			fprintstring(fp, "\t\t%s = ", SSData(ex_desc->keyword));
			fprintstring(fp, "%s;\n", SSData(ex_desc->description));
		}
		fprintf(fp, "\t};\n");
	}

	fprintf(fp, "};\n");

	release_buffer(bitList);
}


inline int Room::Realm(void) 
{
	return (zone_table[zone].realm);
}
