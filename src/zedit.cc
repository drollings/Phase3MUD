/************************************************************************
 *  OasisOLC - zedit.c                                            v1.5  *
 *                                                                      *
 *  Copyright 1996 Harvey Gilpin.                                       *
 ************************************************************************/

#include "characters.h"
#include "descriptors.h"
#include "objects.h"
#include "rooms.h"
#include "comm.h"
#include "utils.h"
#include "scripts.h"
#include "db.h"
#include "olc.h"
#include "buffer.h"
#include "constants.h"

#include "zedit.h"

/*-------------------------------------------------------------------*/
/*. external data areas .*/
extern struct zone_data *zone_table;	/*. db.c	.*/
extern SInt32 top_of_zone_table;		/*. db.c	.*/

/*-------------------------------------------------------------------*/
/* function protos */
void add_cmd_to_list(struct reset_com **list, struct reset_com *newcmd, int pos);
void remove_cmd_from_list(struct reset_com **list, int pos);
int count_commands(struct reset_com *list);

//	External functions.
extern int olc_edit_bitvector(Descriptor *d, Flags *result, char *argument,
		Flags old_bv, char *name, const char **values, Flags mask);
extern int olc_edit_value(Descriptor *d, long *result, char *argument,
		char *name, long min, long max, const char **values, bool diff = FALSE);

/*-------------------------------------------------------------------*/
/*. Nasty internal macros to clean up the code .*/

#define ZCMD		(zone_table[zoneNum].cmd[subcmd])
#define MYCMD		(zone->cmd[subcmd])
#define OLC_CMD(d)	(zone->cmd[value])

/*-------------------------------------------------------------------*\
  utility functions 
\*-------------------------------------------------------------------*/

ZEdit::ZEdit(Descriptor *d, VNum room, VNum zoneNumber) : Editor(d) {
	SInt32	subcmd = 0,
			count = 0;
	VNum	cmd_room = NOWHERE;

	zoneNum = zoneNumber;

	CREATE(zone, struct zone_data, 1);

	//	Copy in zone header info
	zone->name			= str_dup(zone_table[zoneNum].name);
	zone->lifespan		= zone_table[zoneNum].lifespan;
	zone->top			= zone_table[zoneNum].top;
	zone->reset_mode	= zone_table[zoneNum].reset_mode;
	zone->climate		= zone_table[zoneNum].climate;
	
	//	The remaining fields are used as a 'has been modified' flag
	zone->number = 0;		//	Header info has changed
	zone->age = 0;			//	Commands have changed

	//	Start the reset command list with a terminator
	CREATE(zone->cmd, struct reset_com, 1);
	zone->cmd[0].command = 'S';

	/*. Add all entried in zone_table that relate to this room .*/
	while(ZCMD.command != 'S') {
		switch(ZCMD.command) {
			case 'M':
			case 'O':
				cmd_room = ZCMD.arg3;
				break;
			case 'D':
			case 'R':
				cmd_room = ZCMD.arg1;
				break;
			case 'T':
				if (ZCMD.arg1 == WLD_TRIGGER)
				cmd_room = ZCMD.arg4;
				break;
			default:
				break;
		}
		if (cmd_room == room) {
			add_cmd_to_list(&(zone->cmd), &ZCMD, count);
			count++;
		}
		subcmd++;
	}
	
	number = world[room].Virtual();
	
	Menu();
}


void ZEdit::NewZone(Character *ch, VNum vzone_num) {
	FILE *				fp;
	struct zone_data *	new_table;
	char *				buf;
	int					i, room;

	if (vzone_num < 0) {
		ch->Send("You can't make negative zones.\r\n");
		return;
	} else if (vzone_num > 326) {
		ch->Send("326 is the highest zone allowed.\r\n");
		return;
	}

	//	Check zone does not exist
	room = vzone_num * 100;
	for (i = 0; i <= top_of_zone_table; i++) {
		if ((zone_table[i].number * 100 <= room) && (zone_table[i].top >= room)) {
			ch->Send("A zone already covers that area.\r\n");
			return;
		}
	}

	//	Create Zone file .*/
	buf = get_buffer(MAX_INPUT_LENGTH);

	sprintf(buf, ZON_PREFIX "%i.zon", vzone_num);
  
	if(!(fp = fopen(buf, "w"))) {
		mudlog("SYSERR: Can't write new zone file", BRF, NULL, true);
		ch->Send("Could not write zone file.\r\n");
		release_buffer(buf);
		return;
	}
	fprintf(fp, "#%d\n"
				"New Zone~\n"
				"%d 30 2\n"
				"S\n"
				"$\n", 
			vzone_num,
			(vzone_num * 100) + 99
	);
	fclose(fp);

	//	Create Rooms file
	sprintf(buf, WLD_PREFIX "%d.wld", vzone_num);
  
	if(!(fp = fopen(buf, "w"))) {
		mudlog("SYSERR: Can't write new world file", BRF, NULL, true);
		ch->Send("Could not write world file.\r\n");
		release_buffer(buf);
		return;
	}
	fprintf(fp, "#%d\n"
				"The Beginning~\n"
				"Not much here.\n"
				"~\n"
				"%d 0 0\n"
				"S\n"
				"$\n",
			vzone_num * 100,
			vzone_num
	);
	fclose(fp);

	//	Create Mobiles file 
	sprintf(buf, MOB_PREFIX "%i.mob", vzone_num);
  
	if(!(fp = fopen(buf, "w"))) {
		mudlog("SYSERR: Can't write new mob file", BRF, NULL, true);
		release_buffer(buf);
		return;
	}
	fprintf(fp, "$\n");
	fclose(fp);

	//	Create Objects file
	sprintf(buf, OBJ_PREFIX "%i.obj", vzone_num);
  
	if(!(fp = fopen(buf, "w"))) {
		mudlog("SYSERR: Can't write new obj file", BRF, NULL, true);
		ch->Send("Could not write object file.\r\n");
		release_buffer(buf);
		return;
	}
	fprintf(fp, "$\n");
	fclose(fp);

	//	Create Shops file
	sprintf(buf, SHP_PREFIX "%i.shp", vzone_num);
  
	if(!(fp = fopen(buf, "w"))) {
		mudlog("SYSERR: Can't write new shop file", BRF, NULL, true);
		ch->Send("Could not write shop file.\r\n");
		release_buffer(buf);
		return;
	}
	fprintf(fp, "$~\n");
	fclose(fp);


	//	Create Triggers file
	sprintf(buf, TRG_PREFIX "%i.trg", vzone_num);
  
	if(!(fp = fopen(buf, "w"))) {
		mudlog("SYSERR: Can't write new script file", BRF, NULL, true);
		ch->Send("Could not write script file.\r\n");
		release_buffer(buf);
		return;
	}
	fprintf(fp, "$~\n");
	fclose(fp);

	release_buffer(buf);
    
	//	Update index files
	add_to_index(vzone_num, "zon");
	add_to_index(vzone_num, "wld");
	add_to_index(vzone_num, "mob");
	add_to_index(vzone_num, "obj");
	add_to_index(vzone_num, "shp");
	add_to_index(vzone_num, "trg");

	/*
	 * Make a new zone in memory. This was the source of all the zedit new
	 * crashes reported to the CircleMUD list. It was happily overwriting
	 * the stack.  This new loop by Andrew Helm fixes that problem and is
	 * more understandable at the same time.
	 *
	 * The variable is 'top_of_zone_table_table + 2' because we need record 0
	 * through top_of_zone (top_of_zone_table + 1 items) and a new one which
	 * makes it top_of_zone_table + 2 elements large.
	 */
	CREATE(new_table, struct zone_data, top_of_zone_table + 2);
	new_table[top_of_zone_table + 1].number = 32000;

	if (vzone_num > zone_table[top_of_zone_table].number) {
		/*
	     * We're adding to the end of the zone table, copy all of the current
	     * top_of_zone_table + 1 items over and set write point to before the
	     * the last record for the for() loop below.
	     */
		memcpy(new_table, zone_table, (sizeof (struct zone_data) * (top_of_zone_table + 1)));
		i = top_of_zone_table + 1;
	} else
	
	//	Copy over all the zones that are before this zone
	for (i = 0; vzone_num > zone_table[i].number; i++)
		new_table[i] = zone_table[i];

	//	Ok, insert the new zone here.  
	new_table[i].name = str_dup("New Zone");
	new_table[i].number = vzone_num;
	new_table[i].top = (vzone_num * 100) + 99;
	new_table[i].lifespan = 30;
	new_table[i].age = 0;
	new_table[i].reset_mode = 2;

	//	No zone commands, just terminate it with an 'S'

	CREATE(new_table[i].cmd, struct reset_com, 1);
	new_table[i].cmd[0].command = 'S';

	/*
	 * Copy remaining zones into the table one higher, unless of course we
	 * are appending to the end in which case this loop will not be used.
	 */
	for (; i <= top_of_zone_table; i++) {
		new_table[i + 1] = zone_table[i];
		for (room = zone_table[i].number * 100; room <= zone_table[i].top; room++)
			world[room].zone++;
	}

	FREE(zone_table);
	zone_table = new_table;
	top_of_zone_table++;

	/*
	 * Previously, creating a new zone while invisible gave you away.
	 * That quirk has been fixed with the MAX() statement.
	 */
	mudlogf(BRF, ch, true,  "OLC: %s creates new zone #%d", ch->RealName(), vzone_num);
	ch->Send("Zone created successfully.\r\n");
	return;
}

/*-------------------------------------------------------------------*/

#if 0
void ZEdit::CreateIndex(SInt32 znum, char *type) {
	FILE *newfile, *oldfile;
	char *new_name, *old_name, *buf;
	int num;
	bool	found = false;
	const char *	prefix;

	switch(*type) {
		case 'z':		prefix = ZON_PREFIX;		break;
		case 'w':		prefix = WLD_PREFIX;		break;
		case 'o':		prefix = OBJ_PREFIX;		break;
		case 'm':		prefix = MOB_PREFIX;		break;
		case 's':		prefix = SHP_PREFIX;		break;
		case 't':		prefix = TRG_PREFIX;		break;
		default:		/*. Caller messed up .*/	return;
	}
  
	new_name = get_buffer(64);
	old_name = get_buffer(64);
	sprintf(old_name, "%sindex", prefix);
	sprintf(new_name, "%snewindex", prefix);

	if(!(oldfile = fopen(old_name, "r"))) {
		mudlogf( BRF, NULL, true,  "SYSERR: Failed to open %s", old_name);
		release_buffer(new_name);
		release_buffer(old_name);
		return;
	} else if(!(newfile = fopen(new_name, "w"))) {
		mudlogf( BRF, NULL, true,  "SYSERR: Failed to open %s", new_name);
		fclose(oldfile);
		release_buffer(new_name);
		release_buffer(old_name);
		return;
	}
	//	Index contents must be in order: search through the old file for
	//	the right place, insert the new file, then copy the rest over.
      
	buf = get_buffer(MAX_INPUT_LENGTH);
  
	while(get_line(oldfile, buf)) {
		if (*buf == '$') {
			if (!found)
				fprintf(newfile,"%d.%s\n", znum, type);
				fprintf(newfile,"$\n");
				break;
		} else if (!found) {
			sscanf(buf, "%d", &num);
			if (num > znum) {
				found = true;
				fprintf(newfile, "%d.%s\n", znum, type);
			}
		}
		fprintf(newfile, "%s\n", buf);
	}
	release_buffer(buf);
  
	fclose(newfile);
	fclose(oldfile);
	//	Out with the old, in with the new
	remove(old_name);
	rename(new_name, old_name);
	release_buffer(new_name);
	release_buffer(old_name);
}
#endif

/*-------------------------------------------------------------------*/

/*
 * Save all the information in the player's temporary buffer back into
 * the current zone table.
 */
void ZEdit::SaveInternally(void) {
	SInt32	subcmd = 0;
	VNum	cmd_room = -2;
	
	/*
	 * Delete all entries in zone_table that relate to this room so we
	 * can add all the ones we have in their place.
	 */
	while(ZCMD.command != 'S') {
		switch(ZCMD.command) {
			case 'M':
			case 'O':
				cmd_room = ZCMD.arg3;
				break;
			case 'D':
			case 'R':
				cmd_room = ZCMD.arg1;
				break;
			case 'T':
				if (ZCMD.arg1 == WLD_TRIGGER)
					cmd_room = ZCMD.arg4;
			default:
				break;
	    }
		if(cmd_room == number)		remove_cmd_from_list(&(zone_table[zoneNum].cmd), subcmd);
		else						subcmd++;
	}

	/*. Now add all the entries in the players descriptor list .*/
	subcmd = 0;
	while(MYCMD.command != 'S') {
		add_cmd_to_list(&(zone_table[zoneNum].cmd), &MYCMD, subcmd);
		subcmd++;
	}

	/*. Finally, if zone headers have been changed, copy over .*/
	if (zone->number) {
		FREE(zone_table[zoneNum].name);
		zone_table[zoneNum].name		= str_dup(zone->name);
		zone_table[zoneNum].top			= zone->top;
		zone_table[zoneNum].reset_mode	= zone->reset_mode;
		zone_table[zoneNum].lifespan	= zone->lifespan;
		zone_table[zoneNum].climate		= zone->climate;
	}
	olc_add_to_save_list(zone_table[zoneNum].number, OLC_ZEDIT);
}


void ZEdit::Finish(FinishMode mode) {
	if (zone->name)		free(zone->name);
	if (zone->cmd)		free(zone->cmd);
	free(zone);
	if (d->character) {
		STATE(d) = CON_PLAYING;
		act("$n stops using OLC.", TRUE, d->character, 0, 0, TO_ROOM);
	}
	Editor::Finish();
}

/*-------------------------------------------------------------------*/

/*
 * Some common code to count the number of comands in the list.
 */
int count_commands(struct reset_com *list) {
	int count = 0;
	while (list[count].command != 'S')	count++;
	return count;
}

/*-------------------------------------------------------------------*/

/*. Adds a new reset command into a list.  Takes a pointer to the list
    so that it may play with the memory locations.*/

void add_cmd_to_list(struct reset_com **list, struct reset_com *newcmd, int pos) {
	int count = 0, i, l;
	struct reset_com *newlist;

	//	Count number of commands (not including terminator).
	count = count_commands(*list);

	CREATE(newlist, struct reset_com, count + 2);

	//	Tight loop to copy old list and insert new command
	l = 0;
	for(i = 0; i <= count;i++)
		newlist[i] = ((i == pos) ? *newcmd : (*list)[l++]);

	//	Add terminator then insert new list
	newlist[count+1].command = 'S';
	FREE(*list);
	*list = newlist;
}

/*-------------------------------------------------------------------*/
//	Remove a reset command from a list.  Takes a pointer to the list
//	so that it may play with the memory locations

void remove_cmd_from_list(struct reset_com **list, int pos) {
	int count = 0, i, l;
	struct reset_com *newlist;

	//	Count number of commands (not including terminator)  
	count = count_commands(*list);

	CREATE(newlist, struct reset_com, count);

	//	Tight loop to copy old list and skip unwanted command
	l = 0;
	for(i = 0; i < count; i++)
		if(i != pos)
			newlist[l++] = (*list)[i];

	if(((*list)[i].command == 'C') && (*list)[i].command_string)
		FREE((*list)[i].command_string);			// Can't use the FREE macro here.

	//	Add terminator then insert new list
	newlist[count-1].command = 'S';
	FREE(*list);
	*list = newlist;
}

/*-------------------------------------------------------------------*/
//	Error check user input and then add new (blank) command

bool ZEdit::NewCommand(SInt32 pos) {
	int subcmd = 0;
	struct reset_com *new_com;

	//	Error check to ensure users hasn't given too large an index
	while(MYCMD.command != 'S')	subcmd++;

	if ((pos > subcmd) || (pos < 0))
		return false;

	//	Ok, let's add a new (blank) command.
	CREATE(new_com, struct reset_com, 1);
	new_com->command = 'N'; 
	add_cmd_to_list(&zone->cmd, new_com, pos);
	return true;
}


/*-------------------------------------------------------------------*/
//	Error check user input and then remove command

void ZEdit::DeleteCommand(SInt32 pos) {
	int subcmd = 0;

	//	Error check to ensure users hasn't given too large an index
	while(MYCMD.command != 'S')	subcmd++;

	if ((pos >= subcmd) || (pos < 0))
		return;

	//	Ok, let's zap it
	remove_cmd_from_list(&zone->cmd, pos);
}

/*-------------------------------------------------------------------*/
//	Error check user input and then setup change

bool ZEdit::StartChangeCommand(SInt32 pos) {
	int subcmd = 0;

	//	Error check to ensure users hasn't given too large an index
	while(MYCMD.command != 'S')	subcmd++;

	if ((pos >= subcmd) || (pos < 0))
		return false;

	//	Ok, let's get editing
	value = pos;
	return true;
}

/**************************************************************************
 Menu functions 
 **************************************************************************/


void ZEdit::DispClimateMenu(void) {
	int i;
	
	sprintbit(zone->climate.flags, Weather::climateflags, buf);

	d->Writef(
		"&cGE&c0) Energy        : &cC%d&c0\r\n"
		"&cGP&c0) Precipitation : &cC%s&c0\r\n"
		"&cGT&c0) Temperature   : &cC%s&c0\r\n"
		"&cGF&c0) Flags         : &cC%s&c0\r\n"
//		"&cGP&c0) Pattern       : %s\r\n"
		"\r\nEnter a selection : ",
		zone->climate.energy,
		Weather::precipitations[zone->climate.precipitation[0]],
		Weather::temperatures[zone->climate.temperature[0]],
		buf);

#if 0
	d->Write("Season            Temperature   Wind        Wind       Precipitation\r\n"
			 "                                Direction   Variance\r\n");
	
	for (i = 0; i < Weather::MaxSeasons; ++i) {
		d->Writef(
			"&cG%d&c0) %-10s : %-10d %-10s %10s\r\n",
			i + 1,
			Weather::temperatures[zone->climate.temperature[i]],
			zone->climate.windDirection[i],
			zone->climate.windVariance[i],
			Weather::precipitations[zone->climate.precipitation[i]]);
	}
#endif		

	mode = ZoneClimate;
}

void ZEdit::DispCommandMenu(void) {
	char *cmdtype;
	
	switch(OLC_CMD(d).command) {
		case 'M':
			cmdtype = "Load Mobile";
			break;
		case 'G':
			cmdtype = "Give Object to Mobile";
			break;
		case 'C':
			cmdtype = "Command Mobile";
			break;
		case 'O':
			cmdtype = "Load Object";
			break;
		case 'E':
			cmdtype = "Equip Mobile with Object";
			break;
		case 'P':
			cmdtype = "Put Object in Object";
			break;
		case 'R':
			cmdtype = "Remove Object";
			break;
		case 'D':
			cmdtype = "Set Door";
			break;
		case 'T':
			cmdtype = "Trigger";
			break;
		case 'Z':
			cmdtype = "Randomized Zone";
			break;
		case '*':
			cmdtype = "<Comment>";
			break;
		default:
			cmdtype = "<Unknown>";
	}
	d->Writef(
#if defined(CLEAR_SCREEN)
			"\x1B[H\x1B[J"
#endif
			"&cGC&c0) Command   : %s\r\n"
			"&cGD&c0) Dependent : %s\r\n",
			cmdtype,
			OLC_CMD(d).if_flag ? "YES" : "NO");
	
	/* arg1 */
	switch(OLC_CMD(d).command) {
		case 'M':
			d->Writef("&cG1&c0) Mobile    : %s [&cC%d&c0]\r\n",
					Character::Find(OLC_CMD(d).arg1) ?
						Character::Index[OLC_CMD(d).arg1].proto->Name() : "<NONE>",
					OLC_CMD(d).arg1);
			break;
		case 'G':
		case 'O':
		case 'E':
		case 'P':
			
			d->Writef("&cG1&c0) Object    : %s [&cC%d&c0]\r\n",
					Object::Find(OLC_CMD(d).arg1) ?
						Object::Index[OLC_CMD(d).arg1].proto->Name() : "<NONE>",
					OLC_CMD(d).arg1);
			break;
		case 'C':
			d->Writef("&cG1&c0) Command   : %s\r\n", OLC_CMD(d).command_string);
			break;
	}
	
	/* arg2 */
	switch(OLC_CMD(d).command) {
		case 'M':
		case 'G':
		case 'O':
		case 'E':
		case 'P':
			d->Writef("&cG2&c0) Maximum   : &cC%d\r\n",
					OLC_CMD(d).arg2);
			break;
		case 'R':
			d->Writef("&cG2&c0) Object    : %s [&cC%d&c0]\r\n",
					Object::Find(OLC_CMD(d).arg2) ?
						Object::Index[OLC_CMD(d).arg2].proto->Name() : "<NONE>",
					OLC_CMD(d).arg2);
			break;
		case 'D':
			d->Writef("&cG2&c0) Door      : &cC%s\r\n",
					dirs[OLC_CMD(d).arg2]);
			break;
		case 'T':
			d->Writef("&cG2&c0) Script    : %s [&cC%d&c0]\r\n",
					Trigger::Find(OLC_CMD(d).arg2) ?
						SSData(Trigger::Index[OLC_CMD(d).arg2].proto->name) : "<NONE>",
					OLC_CMD(d).arg2);
			break;
	}
	
	/* arg3 */
	switch(OLC_CMD(d).command) {
		case 'E':
			d->Writef("&cG3&c0) Location  : %s\r\n",
				equipment_types[OLC_CMD(d).arg3]);
			break;
		case 'P':
			d->Writef("&cG3&c0) Container : %s [&cC%d&c0]\r\n",
					Object::Find(OLC_CMD(d).arg3) ?
						Object::Index[OLC_CMD(d).arg3].proto->Name() : "<NONE>",
					OLC_CMD(d).arg3);
			break;
		case 'D':
			d->Writef("&cG3&c0) Door state: %s\r\n",
					OLC_CMD(d).arg3 ? (OLC_CMD(d).arg3 == 1 ? "Closed" : "Locked") : "Open");
			break;
		case 'T':
			d->Writef("&cG3&c0) Maximum   : &cC%d\r\n",
					OLC_CMD(d).arg3);
			break;
	}
	
 	/* arg4 */

	switch(OLC_CMD(d).command) {
		case 'M':
			d->Writef("&cG4&c0) Max/zone  : &cC%d\r\n",
					OLC_CMD(d).arg4);
			break;
	}

	if (strchr("GORP", OLC_CMD(d).command) != NULL)
		d->Writef("&cGR&c0) Repeat    : &cC%d\r\n", OLC_CMD(d).repeat);
	d->Writef(
			"&cGQ&c0) Quit\r\n\n"
			"Enter your choice : ");

	mode = CommandMenu;
}

/* the main menu */
void ZEdit::Menu(void) {
	int subcmd = 0, counter = 0;
	char *buf = get_buffer(MAX_STRING_LENGTH);

  /*. Menu header .*/
	d->Writef(
#if defined(CLEAR_SCREEN)
			"\x1B[H\x1B[J"
#endif
			"Room number: &cC%d&c0		Room zone: &cC%d\r\n"
			"&cGZ&c0) Zone name   : &cY%s\r\n"
			"&cGL&c0) Lifespan    : &cY%d minutes\r\n"
			"&cGT&c0) Top of zone : &cY%d\r\n"
			"&cGR&c0) Reset Mode  : &cY%s\r\n"
			"&cGC&c0) Climate\r\n"
			"&c0[Command list]\r\n",

			number, zone_table[zoneNum].number,
			zone->name ? zone->name : "<NONE!>",
			zone->lifespan,
			zone->top,
			zone->reset_mode ? ((zone->reset_mode == 1) ?
				"Reset when no players are in zone." : 
				"Normal reset.") :
				"Never reset");

	/*. Print the commands for this room into display buffer .*/
	while(MYCMD.command != 'S') {	// Translate what the command means
		if (strchr("GORP", MYCMD.command) != NULL && MYCMD.repeat > 1)
			sprintf(buf, "(x%2d) ", MYCMD.repeat);
		else
			strcpy(buf, "");
			
		switch(MYCMD.command) {
			case'M':
				sprintf(buf, "Load %s [&cC%d&cY]",
						Character::Find(MYCMD.arg1) ?
							Character::Index[MYCMD.arg1].proto->Name() : "<NONE>",
						MYCMD.arg1);
				break;
			case'G':
				sprintf(buf + strlen(buf), "Give it %s [&cC%d&cY]",
						Object::Find(MYCMD.arg1) ? Object::Index[MYCMD.arg1].proto->Name() : "<NONE>",
						MYCMD.arg1);
				break;
			case'C':
				sprintf(buf, "Do command: &cC%s&cY", MYCMD.command_string);
				break;
			case'O':
				sprintf(buf + strlen(buf), "Load %s [&cC%d&cY]",
						Object::Find(MYCMD.arg1) ? Object::Index[MYCMD.arg1].proto->Name() : "<NONE>",
						MYCMD.arg1);
				break;
			case'E':
				sprintf(buf, "Equip with %s [&cC%d&cY], %s",
						Object::Find(MYCMD.arg1) ? Object::Index[MYCMD.arg1].proto->Name() : "<NONE>",
						MYCMD.arg1,
						equipment_types[MYCMD.arg3]);
				break;
			case'P':
				sprintf(buf + strlen(buf), "Put %s [&cC%d&cY] in %s [&cC%d&cY]",
						Object::Find(MYCMD.arg1) ? Object::Index[MYCMD.arg1].proto->Name() : "<NONE>",
						MYCMD.arg1,
						Object::Find(MYCMD.arg3) ? Object::Index[MYCMD.arg3].proto->Name() : "<NONE>",
						MYCMD.arg3);
				break;
			case'R':
				sprintf(buf + strlen(buf), "Remove %s [&cC%d&cY] from room.",
						Object::Find(MYCMD.arg2) ? Object::Index[MYCMD.arg2].proto->Name() : "<NONE>",
						MYCMD.arg2);
				break;
			case'D':
				sprintf(buf, "Set door %s as %s.",
						dirs[MYCMD.arg2],
						MYCMD.arg3 ? ((MYCMD.arg3 == 1) ? "closed" : "locked") : "open");
				break;
			case 'T':
				sprintf(buf, "Attach script %s [&cC%d&cY]",
						Trigger::Find(MYCMD.arg2) ? SSData(Trigger::Index[MYCMD.arg2].proto->name) : "<NONE>",
						MYCMD.arg2);
				break;
			case 'Z':
				strcpy(buf, "Zone is Randomized");
				break;
			case '*':
				strcpy(buf, "<Comment>");
				break;
			default:
				strcpy(buf, "<Unknown Command>");
				break;
		}
		/* Build the display buffer for this command */
		d->Writef("&c0%-2d - &cY%s%s",
				counter++,
				MYCMD.if_flag ? " then " : "",
				buf);
		if (strchr("EGMOP", MYCMD.command) != NULL && MYCMD.arg2 > 0)
			d->Writef(", Max : %d", MYCMD.arg2);
		else if (MYCMD.command == 'T' && MYCMD.arg3 > 0)
			d->Writef(", Max : %d", MYCMD.arg3);
		d->Writef("\r\n");
		subcmd++;
	}
	/* Finish off menu */
	d->Writef(
			"&c0%-2d - <END OF LIST>\r\n"
			"&cGN&c0) New command.\r\n"
			"&cGE&c0) Edit a command.\r\n"
			"&cGD&c0) Delete a command.\r\n"
			"&cGQ&c0) Quit\r\n"
			"Enter your choice : ",
			counter);

	mode = MainMenu;
	release_buffer(buf);
}


/*-------------------------------------------------------------------*/
//	Print the command type menu and setup response catch.

void ZEdit::DispCommandType(void) {
#if defined(CLEAR_SCREEN)
	d->Writef("\x1B[H\x1B[J");
#endif
	d->Write("&cgM&c0) Load Mobile to room             &cgO&c0) Load Object to room\r\n"
			 "&cgE&c0) Equip mobile with object        &cgG&c0) Give an object to a mobile\r\n"
			 "&cgP&c0) Put object in another object    &cgD&c0) Open/Close/Lock a Door\r\n"
			 "&cgR&c0) Remove an object from the room  &cgC&c0) Force mobile to do command\r\n"
			 "&cgT&c0) Attach Script\r\n"
			 "What sort of command will this be? ");
	mode = CommandType;
}


/*-------------------------------------------------------------------*/
//	Print the appropriate message for the command type for arg1 and set
//	up the input catch clause

void ZEdit::DispArg1(void) {
	switch(OLC_CMD(d).command) {
		case 'M':
			d->Writef("Input mob's vnum: ");
			break;
		case 'O':
		case 'E':
		case 'P':
		case 'G':
			d->Writef("Input object vnum: ");
			break;
		case 'D':
		case 'R':
			/*. Arg1 for these is the room number, skip to arg2 .*/
			OLC_CMD(d).arg1 = Room::Find(number) ? number : -1;
			DispArg2();
			return;
		case 'C':
			d->Writef("Command: ");
			break;
		default:
			//	We should never get here
			mudlog("SYSERR: zedit_disp_arg1(): Help!", BRF, NULL, true);
			d->Writef("Oops...\r\n");
			Finish(All);
			return;
	}
	mode = Arg1;
}

    

/*-------------------------------------------------------------------*/
/*. Print the appropriate message for the command type for arg2 and set
    up the input catch clause .*/

void ZEdit::DispArg2(void) {
	int i = 0;

	switch(OLC_CMD(d).command) {
		case 'M':
		case 'O':
		case 'E':
		case 'P':
		case 'G':
			d->Writef("Input the maximum number that can exist: ");
			break;
		case 'D':
			while(*dirs[i] != '\n') {
				d->Writef("%d) Exit %s.\r\n", i, dirs[i]);
				i++;
			}
			d->Writef("Enter exit number for door: ");
			break;
		case 'R':
			d->Writef("Input object's vnum: ");
			break;
		case 'T':
			d->Writef("Input script's vnum: ");
			break;
		default:
			mudlog("SYSERR: zedit_disp_arg2(): Help!", BRF, NULL, true);
			Finish(All);
			return;
	}
	mode = Arg2;
}


/*-------------------------------------------------------------------*/
/*. Print the appropriate message for the command type for arg3 and set
    up the input catch clause .*/

void ZEdit::DispArg3(void) {
	int i = 0;
	
	switch(OLC_CMD(d).command) { 
		case 'E':
			while(*equipment_types[i] != '\n') {
				d->Writef("%2d) %26.26s %2d) %26.26s\r\n", i, 
						equipment_types[i], i+1, (*equipment_types[i+1] != '\n') ?
						equipment_types[i+1] : "");
				if(*equipment_types[i+1] != '\n')	i += 2;
				else								break;
			}
			d->Writef("Input location to equip: ");
			break;
		case 'P':
			d->Writef("Input the vnum of the container: ");
			break;
		case 'D':
			d->Writef(
					"0)  Door open\r\n"
					"1)  Door closed\r\n"
					"2)  Door locked\r\n" 
					"Enter state of the door: ");
			break;
		case 'T':
			d->Writef("Input the maximum number that can exist on the mud: ");
			break;
		default:
			Finish(All);
			mudlog("SYSERR: zedit_disp_arg3(): Help!", BRF, NULL, true);
			return;
	}
	mode = Arg3;
}

    
void ZEdit::DispArg4(void) {
	int i = 0;

	switch(OLC_CMD(d).command) { 
		case 'M':
			d->Writef("Input the maximum number that can exist in the zone: ");
			break;
		default:
			Finish(All);
			mudlog("SYSERR: zedit_disp_arg3(): Help!", BRF, NULL, true);
			return;
	}
	mode = Arg4;
}


void ZEdit::DispRepeat(void) {
	switch(OLC_CMD(d).command) { 
		case 'O':
		case 'G':
		case 'P':
		case 'R':
			d->Writef("Enter number of times to repeat (0 or 1 to do once): ");
			break;
		default:
			Menu();
			return;
	}
	mode = Repeat;
}


/**************************************************************************
  The GARGANTAUN event handler
 **************************************************************************/

void ZEdit::Parse(char *arg) {
	SInt32	pos, i = 0;
	long tmp;
	
	switch (mode) {
/*-------------------------------------------------------------------*/
		case ConfirmSave:
			switch (*arg) {
				case 'y':
				case 'Y':
					d->Writef("Saving zone info in memory.\r\n");
					mudlogf(NRM, d->character, true, "OLC: %s edits zone info for room %d",
							d->character->RealName(), number);
					SaveInternally();
					Finish(Structs);
					break;
				case 'n':
				case 'N':
					Finish(All);
					break;
				default:
					d->Writef("Invalid choice!\r\n");
					d->Writef("Do you wish to save the zone info? ");
					break;
			}
			return;
			
/*-------------------------------------------------------------------*/
		case MainMenu:
			switch (*arg) {
				case 'q':
				case 'Q':
					if(zone->age || zone->number || save) {
						d->Writef("Do you wish to save the changes to the zone info? (y/n) ");
						mode = ConfirmSave;
					} else {
						d->Writef("No changes made.\r\n");
						Finish(All);
					}
					return;
				case 'n':	//	New entry
				case 'N':
					d->Writef("What number in the list should the new command be? ");
					mode = NewEntry;
					break;
				case 'e':	//	Change an entry 
				case 'E':
					d->Writef("Which command do you wish to change? ");
					mode = ChangeEntry;
					break;
				case 'd':	//	Delete an entry
				case 'D':
					d->Writef("Which command do you wish to delete? ");
					mode = DeleteEntry;
					break;
				case 'z':	//	Edit zone name
				case 'Z':
					d->Writef("Enter new zone name: ");
					mode = ZoneName;
					break;
				case 't':	//	Edit zone top
				case 'T':
					if (!STF_FLAGGED(d->character, Staff::OLCAdmin))
						Menu();
					else {
						d->Writef("Enter new top of zone: ");
						mode = ZoneTop;
					}
					break;
				case 'l':	//	Edit zone lifespan
				case 'L':
					d->Writef("Enter new zone lifespan: ");
					mode = ZoneLife;
					break;
				case 'r':	//	Edit zone reset mode
				case 'R':
					d->Writef("\r\n"
							"0) Never reset\r\n"
							"1) Reset only when no players in zone\r\n"
							"2) Normal reset\r\n"
							"Enter new zone reset type: ");
					mode = ZoneReset;
					break;
				case 'c':
				case 'C':
					DispClimateMenu();
					return;
				default:
					Menu();
					break;
			}
			break;

/*-------------------------------------------------------------------*/
		case NewEntry:
			/*. Get the line number and insert the new line .*/
			pos = atoi(arg);
			if (isdigit(*arg) && NewCommand(pos)) {
				if (StartChangeCommand(pos)) {
					DispCommandType();
					zone->age = 1;
				}
			} else
				Menu();
			break;

/*-------------------------------------------------------------------*/
		case DeleteEntry:
			/*. Get the line number and delete the line .*/
			pos = atoi(arg);
			if(isdigit(*arg)) {
				DeleteCommand(pos);
				zone->age = 1;
			}
			Menu();
			break;

/*-------------------------------------------------------------------*/
		case ChangeEntry:
			//	Parse the input for which line to edit, and goto next quiz
			pos = atoi(arg);
			if(isdigit(*arg) && StartChangeCommand(pos))	DispCommandMenu();
			else											Menu();
			break;

/*-------------------------------------------------------------------*/
		case CommandType:
			//	Parse the input for which type of command this is, and goto next quiz
			OLC_CMD(d).command = toupper(*arg);
			if (!OLC_CMD(d).command || !strchr("MOPEDGTRC", OLC_CMD(d).command))
				d->Writef("Invalid choice, try again: ");
			else {
				OLC_CMD(d).arg1 = 0;
				OLC_CMD(d).arg2 = 0;
				OLC_CMD(d).arg3 = 0;
				OLC_CMD(d).arg4 = 0;
				switch (OLC_CMD(d).command) {
					case 'C':
						OLC_CMD(d).command_string = str_dup("say I need a command!");
						break;
					case 'O':
					case 'M':	OLC_CMD(d).arg3 = Room::Find(number) ? number : -1;	break;
					case 'R':
					case 'D':	OLC_CMD(d).arg1 = Room::Find(number) ? number : -1;	break;
					case 'T':	OLC_CMD(d).arg4 = Room::Find(number) ? number : -1;	break;
				}
				DispCommandMenu();
			}
			break;

/*-------------------------------------------------------------------*/
		case IfFlag:
			//	Parse the input for the if flag, and goto next quiz
			switch(*arg) {
				case 'y':
				case 'Y':	OLC_CMD(d).if_flag = 1;			break;
				case 'n':
				case 'N':	OLC_CMD(d).if_flag = 0;			break;
				default:	d->Writef("Try again: ");		return;
			}
			DispCommandMenu();
			break;


/*-------------------------------------------------------------------*/
		case Arg1:
			//	Parse the input for arg1, and goto next quiz
			if (OLC_CMD(d).command == 'C') {
				OLC_CMD(d).command_string = str_dup(arg);
				DispCommandMenu();
				break;
			}
			if (!isdigit(*arg)) {
				d->Writef("Must be a numeric value, try again: ");
				return;
			}
			switch(OLC_CMD(d).command) {
				case 'M':
					if (Character::Find(pos = atoi(arg))) {
						OLC_CMD(d).arg1 = pos;
						DispCommandMenu();
					} else
						d->Writef("That mobile does not exist, try again: ");
					break;
				case 'O':
				case 'P':
				case 'E':
				case 'G':
					if (Object::Find(pos = atoi(arg))) {
						OLC_CMD(d).arg1 = pos;
						DispCommandMenu();
					} else
						d->Writef("That object does not exist, try again: ");
					break;
				case 'D':
				case 'R':
				case 'T':
				case 'Z':
				default:
					//	We should never get here
					Finish(All);
					mudlog("SYSERR: zedit_parse(): case ARG1: Ack!", BRF, NULL, true);
					d->Writef("Oops...\r\n");
					break;
			}
			break;


/*-------------------------------------------------------------------*/
		case Arg2:
			//	Parse the input for arg2, and goto next quiz
			if  (!isdigit(*arg)) {
				d->Writef("Must be a numeric value, try again: ");
				return;
			}
			switch(OLC_CMD(d).command) {
				case 'M':
					OLC_CMD(d).arg2 = atoi(arg);
					DispCommandMenu();
					break;
				case 'O':
				case 'G':
				case 'P':
				case 'E':
					OLC_CMD(d).arg2 = atoi(arg);
					DispCommandMenu();
					break;
				case 'D':
					pos = atoi(arg);
					if ((pos < 0) || (pos > NUM_OF_DIRS))
						d->Writef("Try again: ");
					else {
						OLC_CMD(d).arg2 = pos;
						DispCommandMenu();
					}
					break;
				case 'R':
					if (Object::Find(pos = atoi(arg))) {
						OLC_CMD(d).arg2 = pos;
						DispCommandMenu();
					} else
						d->Writef("That object does not exist, try again: ");
					break;
				case 'T':
					if (Trigger::Find(pos = atoi(arg))) {
						OLC_CMD(d).arg2 = pos;
						// OLC_CMD(d).arg1 = GET_TRIG_DATA_TYPE(Trigger::Index[pos].proto);
						DispCommandMenu();
					} else 
						d->Writef("That trigger does not exist, try again: ");
					break;
				default:
					//	We should never get here
					Finish(All);
					mudlog("SYSERR: zedit_parse(): case ARG2: Ack!", BRF, NULL, true);
					d->Writef("Oops...\r\n");
					break;
			}
			break;

/*-------------------------------------------------------------------*/
		case Arg3:
			//	Parse the input for arg3, and go back to main menu.
			if  (!isdigit(*arg)) {
				d->Writef("Must be a numeric value, try again: ");
				return;
			}
			switch(OLC_CMD(d).command) {
				case 'E':
					pos = atoi(arg);
					//	Count number of wear positions (could use NUM_WEARS,
					//	this is more reliable)
					while(*equipment_types[i] != '\n')	i++;
					if ((pos < 0) || (pos > i))
						d->Writef("Try again: ");
					else {
						OLC_CMD(d).arg3 = pos;
						DispCommandMenu();
					}
					break;
				case 'P':
					if (Object::Find(pos = atoi(arg))) {
						OLC_CMD(d).arg3 = pos;
						DispCommandMenu();
					} else
						d->Writef("That object does not exist, try again: ");
					break;
				case 'D':
					pos = atoi(arg);
					if ((pos < 0) || (pos > 2))
						d->Writef("Try again: ");
					else {
						OLC_CMD(d).arg3 = pos;
						DispCommandMenu();
					}
					break;
				case 'T':
					OLC_CMD(d).arg3 = atoi(arg);
					DispCommandMenu();
					break;
				default:
					//	We should never get here
					Finish(All);
					mudlog("SYSERR: zedit_parse(): case ARG3: Ack!", BRF, NULL, true);
					d->Writef("Oops...\r\n");
					break;
			}
			break;

/*-------------------------------------------------------------------*/
		case Arg4:
			//	Parse the input for arg4, and go back to main menu
			if  (!isdigit(*arg)) {
				d->Writef("Must be a numeric value, try again: ");
				return;
			}
			switch(OLC_CMD(d).command) { 
				case 'M':
					OLC_CMD(d).arg4 = atoi(arg);
					DispCommandMenu();
					break;
				default:
					//	We should never get here
					Finish(All);
					mudlog("SYSERR: zedit_parse(): case ARG3: Ack!", BRF, NULL, true);
					d->Writef("Oops...\r\n");
					break;
			}
			break;

/*-------------------------------------------------------------------*/
		case Repeat:
			//	Repeat for several times
			pos = atoi(arg);
			if (pos > 0)	OLC_CMD(d).repeat = pos;
			else			OLC_CMD(d).repeat = 0;
			DispCommandMenu();
			break;
  
/*-------------------------------------------------------------------*/
		case ZoneName:
			//	Add new name and return to main menu
			FREE(zone->name);
			zone->name = str_dup(arg);
			zone->number = 1;
			Menu();
			break;

/*-------------------------------------------------------------------*/
		case ZoneReset:
			//	Parse and add new reset_mode and return to main menu
			pos = atoi(arg);
			if (!isdigit(*arg) || (pos < 0) || (pos > 2))
				d->Writef("Try again (0-2): ");
			else {
				zone->reset_mode = pos;
				zone->number = 1;
				Menu();
			}
			break; 

/*-------------------------------------------------------------------*/
		case ZoneLife:
			//	Parse and add new lifespan and return to main menu
			pos = atoi(arg);
			if (!isdigit(*arg) || (pos < 0) || (pos > 240))
				d->Writef("Try again (0-240): ");
			else {
				zone->lifespan = pos;
				zone->number = 1;
				Menu();
			}
			break; 

/*-------------------------------------------------------------------*/
		case ZoneTop:
			//	Parse and add new top room in zone and return to main menu
			if (zoneNum == top_of_zone_table)
				zone->top = RANGE(zoneNum * 100, atoi(arg), 32000);
			else
				zone->top = RANGE(zoneNum * 100, atoi(arg), zone_table[zoneNum + 1].number * 100);
			zone->number = 1;
			Menu();
			break; 

/*-------------------------------------------------------------------*/
		case ZoneClimate:
			switch (*arg) {
				case 'e':	//	Change an entry 
				case 'E':
					d->Writef("What amount of energy? ");
					mode = ZoneClimateEnergy;
					break;
				case 'f':	//	Change an entry 
				case 'F':
					tmp = zone->climate.flags;
					olc_edit_bitvector(d, (Flags *) &(tmp), "", tmp, 
						"climate flags", Weather::climateflags, 0);
					mode = ZoneClimateFlags;
					break;
				case 'p':	//	Change an entry 
				case 'P':
					tmp = zone->climate.precipitation[0];
					olc_edit_value(d, &(tmp), "", "precipitation", 0, 0, Weather::precipitations);
					d->Writef("What level of precipitation? ");
					mode = ZoneClimatePrecipitation;
					break;
				case 't':	//	Change an entry 
				case 'T':
					tmp = zone->climate.temperature[0];
					olc_edit_value(d, &(tmp), "", "temperature", 0, 0, Weather::temperatures);
					d->Writef("What temperature range? ");
					mode = ZoneClimateTemperature;
					break;
				default:
					Menu();
					break;
			}
			break;

/*-------------------------------------------------------------------*/
		case CommandMenu:
			switch(*arg) {
				case 'C':
				case 'c':
					DispCommandType();
					zone->age = 1;
					break;
				case 'D':
				case 'd':
					if(!strchr("Z", OLC_CMD(d).command) && value) { /*. If there was a previous command .*/
						d->Writef("Is this command dependent on the success of the previous one? (y/n)\r\n");
						mode = IfFlag;
						zone->age = 1;
					} else
						DispCommandMenu();
					break;
				case '1':
					if(!strchr("RDZT", OLC_CMD(d).command)) {
						DispArg1();
						zone->age = 1;
					} else
						DispCommandMenu();
					break;
				case '2':
					if (!strchr("ZC", OLC_CMD(d).command)) {
						DispArg2();
						zone->age = 1;
					} else
						DispCommandMenu();
					break;
				case '3':
					if (strchr("EPDT", OLC_CMD(d).command)) {
						DispArg3();
						zone->age = 1;
					} else
						DispCommandMenu();
					break;
				case '4':
					if (strchr("M", OLC_CMD(d).command)) {
						DispArg4();
						zone->age = 1;
					} else
						DispCommandMenu();
					break;
				case 'R':
				case 'r':
					if (strchr("GORP", OLC_CMD(d).command)) {
						DispRepeat();
						zone->age = 1;
					} else
						DispCommandMenu();
					break;
				case 'Q':
				case 'q':
					Menu();
					break;
				default:
					DispCommandMenu();
			}
			break;
  	
/*-------------------------------------------------------------------*/
		case ZoneClimateEnergy:
			zone->number = 1;
			zone->climate.energy = RANGE(0, atoi(arg), 100);
			DispClimateMenu();
			break;

		case ZoneClimateFlags:
			switch (*arg && olc_edit_bitvector(d, &(zone->climate.flags), arg, zone->climate.flags, "climate flags", Weather::climateflags, 0)) {
			case 0:
				DispClimateMenu();
				return;
			default: 
				zone->number = 1;
				olc_edit_bitvector(d, &(zone->climate.flags), "", zone->climate.flags, "climate flags", Weather::climateflags, 0);
				return;
			}

		case ZoneClimatePrecipitation:
			tmp = zone->climate.precipitation[0];
			switch (*arg && (*arg != '0') && olc_edit_value(d, &(tmp), arg, "precipitation", 0, 0, Weather::precipitations)) {
				default: 
				zone->number = 1;
				zone->climate.precipitation[0] = tmp;
				case 0: DispClimateMenu(); return;
			}
			break;

		case ZoneClimateTemperature:
			tmp = zone->climate.temperature[0];
			switch (*arg && (*arg != '0') && olc_edit_value(d, &(tmp), arg, "temperature", 0, 0, Weather::temperatures)) {
				default: 
				zone->number = 1;
				zone->climate.temperature[0] = tmp;
				case 0: DispClimateMenu(); return;
			}
			break;
/*-------------------------------------------------------------------*/

		default:
			//	We should never get here
			mudlog("SYSERR: zedit_parse(): Reached default case!",BRF,NULL,true);
			d->Writef("Oops...\r\n");
			Finish(All);
			return;
	}
}

