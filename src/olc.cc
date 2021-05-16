/***************************************************************************
 *  OasisOLC - olc.c                                                       *
 *                                                                         *
 *  Copyright 1996 Harvey Gilpin.                                          *
 ***************************************************************************/

#define _OASIS_OLC_

#include "structs.h"
#include "characters.h"
#include "interpreter.h"
#include "comm.h"
#include "utils.h"
#include "scripts.h"
#include "db.h"
#include "handler.h"
#include "find.h"
#include "olc.h"
#include "clans.h"
#include "shop.h"
#include "help.h"
#include "buffer.h"
#include "socials.h"
#include "constants.h"
#include "spec.h"

#include "editor.h"
#include "aedit.h"
#include "cedit.h"
#include "hedit.h"
#include "medit.h"
#include "oedit.h"
#include "redit.h"
#include "sedit.h"
#include "scriptedit.h"
#include "zedit.h"

//	External Variables
extern int top_of_zone_table;
extern struct zone_data *zone_table;
extern char *trig_kinds[];
extern const char *ocmd_place_vector[];

//	External Functions.
extern void do_stat_object(Character * ch, Object * j);
extern void do_stat_character(Character * ch, Character * k);
extern void do_stat_room(Character * ch);
extern void fprintstring(FILE *fp, const char *fmt, const char *txt);
extern void fprintbits(FILE *fp, const char *fmt, UInt32 bits, const char *bitnames[]);


//	Internal Functions
ACMD(do_olc);
ACMD(do_zallow);
ACMD(do_zdeny);
ACMD(do_ostat);
ACMD(do_mstat);
ACMD(do_rstat);
ACMD(do_zstat);
ACMD(do_zlist);
ACMD(do_delete);
ACMD(do_move_element);
ACMD(do_massroomsave);

void save_skills(void);
int read_zone_perms(int rnum);
int real_zone(int virt);
void olc_saveinfo(Character *ch);
int get_line2(FILE * fl, char *buf);
int get_zone_rnum(int zone);
int get_zon_num(int vnum);
void fprint_zone(FILE *zfile, SInt32 zone_num);
void gen_olc_save(int savenum, int subcmd);

/*. Internal data .*/

struct olc_save_info *olc_save_list = NULL;


namespace Property {
	extern const char * skill_properties[];
}


const char *olc_modes[] = {
  "",
  "room",
  "object",
  "zone",
  "mobile",
  "shop",
  "action",
  "help",
  "script",
  "clan",
  "\n"
};


const char *save_info_msg[] = { 
  "",
  "Rooms", 
  "Objects", 
  "Zone info", 
  "Mobiles", 
  "Shops", 
  "Actions",
  "Helpfiles",
  "Triggers",
  "Clans",
  "\n"
};


/*------------------------------------------------------------*/


OLCCMD(olc_create)
{
	int i = -1;
	VNum vnum;

	skip_spaces(argument);

	if (!*argument) {
		ch->Send("You must give a vnum to create.\r\n");
		return;
	}

	vnum = atoi(argument);

	if (vnum > 32699) {
		ch->Send("That is not a valid number!\r\n");
		return;
	}
		
	switch (subcmd) {
	case OLC_MEDIT: i = Character::Find(vnum) ? 0 : -1; break;
	case OLC_OEDIT: i = Object::Find(vnum) ? 0 : -1; break;
	case OLC_REDIT: i = Room::Find(vnum);					break;
	case OLC_ZEDIT: i = real_zone(vnum);					break;
	}

	if (i >= 0) {
		ch->Send("That vnum already exists.\r\n");
		return;
	}

	switch (subcmd) {
	case OLC_MEDIT:
	case OLC_OEDIT:
	case OLC_REDIT:
		if (!get_zone_perms(ch, find_owner_zone(vnum))) {
			return;
		}
		break;

	case OLC_ZEDIT:
		break;
	}

	/* all checks done, create the thing */
	switch (subcmd) {
#ifdef GOT_RID_OF_IT
	case OLC_MEDIT:
		create_mob_proto(vnum);
		break;

	case OLC_OEDIT:
		create_obj_proto(vnum);
		break;

#endif
	case OLC_ZEDIT:
		/* only imps can create new zones */
		if (!STF_FLAGGED(ch, Staff::OLCAdmin)) {
			ch->Send("Ask an administrator to create a zone for you.\r\n");
			return;
		}
		ZEdit::NewZone(ch, vnum);
		break;

#ifdef GOT_RID_OF_IT
	case OLC_REDIT:
		create_room(vnum);
		break;
#endif
	}

	ch->Sendf("%s %d created.\r\n", olc_modes[subcmd], vnum);
	olc_add_to_save_list(vnum, subcmd);

	// OLC_NUM(ch) = vnum; /* switch target to new thing */
}


int find_olc_target(Character *ch, char *arg, int mode)
{
	int i = -1;

	if (!*arg) {
		switch (mode) {
		case OLC_MEDIT:
		case OLC_OEDIT:
		case OLC_SEDIT:
		case OLC_CEDIT:
		case OLC_SCRIPTEDIT:
			return -1;
		case OLC_REDIT:
		case OLC_ZEDIT:
			return IN_ROOM(ch);
		}
	} else {
		i = atoi(arg);
		switch (mode) {
		case OLC_SCRIPTEDIT:
		case OLC_MEDIT:
		case OLC_OEDIT:
		case OLC_CEDIT:
		case OLC_SEDIT:
		case OLC_REDIT: break;
		case OLC_ZEDIT:	if (!Room::Find(i)) return -1;
		}

		return i;
	}

	return -1;
}


int verify_olc_target(Character *ch, SInt16 vnum, int mode)
{
  int i = -1;

  switch (mode) {
    case OLC_SCRIPTEDIT: i = (Trigger::Find(vnum) ? vnum : -1); break;
    case OLC_MEDIT: i = (Character::Find(vnum) ? vnum : -1); break;
    case OLC_OEDIT: i = (Object::Find(vnum) ? vnum : -1); break;
    case OLC_CEDIT: i = (Clan::Find(vnum) ? vnum : -1); break;
    case OLC_REDIT:
    case OLC_ZEDIT:  i = (Room::Find(vnum) ? vnum : -1); break;
    case OLC_SEDIT:  i = (Shop::Index.Find(vnum) ? vnum : -1); break;
  }

  return i;
}


#ifdef GOT_RID_OF_IT
//	A DeathGate artifact that I'm not sure if I want to use.

//	Gets a void pointer to the prototype for number "target" for an OLC mode
void *get_olc_ptr(int target, int mode)
{
    IndexData *index;
    VNum real_num;

    switch (mode) {
    case OLC_MEDIT:
		if (Character::Find(number))	return Character::Index[number]->proto;
    case OLC_OEDIT:
		if (Object::Find(number))		return Object::Index[number]->proto;
    case OLC_REDIT:
    case OLC_ZEDIT:
		if (Room::Find(number))			return world[number];
    case OLC_SEDIT:
		if (Shop::Find(number))			return Shop::Index[number]->proto;
    case OLC_SCRIPTEDIT:
		if (Trigger::Find(number))		return Trigger::Index[number]->proto;
    }

    return NULL;
}
#endif


/*
 * Exported ACMD do_olc function.
 *
 * This function is the OLC interface.  It deals with all the 
 * generic OLC stuff, then passes control to the sub-olc sections.
 */
ACMD(do_olc)
{
	VNum targ = -1, copyfrom = 0;
	Character *tch;
	Descriptor *d;
	SInt32	zoneNumber = 0;
	void *ptr;
	char arg[MAX_INPUT_LENGTH], *tempStorage = NULL;
	Editor * tmpeditor = NULL;

	if (IS_NPC(ch) || !ch->desc) {
		ch->Send("Sorry, OLC is limited to carbon based life forms.\r\n");
		return;
	}

	if (!subcmd) {
		olc_saveinfo(ch);
		return;
	}

	if (subcmd == OLC_SKILLEDIT) {
		targ = Skill::Number(argument);
	} else {
		half_chop(argument, arg, argument);
	}

	if (*arg) {
		while (*arg) {
			if (isdigit(*arg)) {
				switch(subcmd) {
				case OLC_ZEDIT:
				case OLC_OEDIT:
				case OLC_MEDIT:
				case OLC_SEDIT:
				case OLC_SCRIPTEDIT:
				case OLC_CEDIT:
					targ = find_olc_target(ch, arg, subcmd);
					if (targ < 0) {
						ch->Send("Edit what virtual number?\r\n");
						return;
					}
					break;
				case OLC_REDIT:
					targ = find_olc_target(ch, arg, subcmd);
					break;
				default:	break;
				}
				break;

			} else if (strncmp("copy", arg, 4) == 0) {
				switch (subcmd) {
					case OLC_REDIT:
					case OLC_MEDIT:
					case OLC_OEDIT:
					case OLC_SCRIPTEDIT:
						break;
					default:
						ch->Send("Sorry, this form of copying is unsupported.\r\n");
						return;
				}

				half_chop(argument, arg, argument);
 
				if (*arg && isdigit(*arg)) {
					copyfrom = atoi(arg);
					if (verify_olc_target(ch, copyfrom, subcmd) == -1) {
						ch->Send("Source vnum does not exist!\r\n");
						return;
					}
				} else {
					ch->Send("Copy from which virtual number?\r\n");
					return;
				}
			} else if (strncmp("new", arg, 3) == 0) {
				if (subcmd != OLC_ZEDIT) {
					ch->Send("You needn't use new, just enter an unused vnum.\r\n");
					return;
				}

				half_chop(argument, arg, argument);
				olc_create(ch, OLC_ZEDIT, arg);
				return;
			} else if (strncmp("save", arg, 4) == 0) {
				switch (subcmd) {
				case OLC_AEDIT: 
					ch->Send("Saving all actions.\r\n");
					mudlogf(NRM, ch, TRUE, "OLC: %s saves all actions", ch->RealName());
					AEdit::SaveDisk(0);
					break;
				case OLC_HEDIT: 
					ch->Send("Saving all help files.\r\n");
					mudlogf(NRM, ch, TRUE, "OLC: %s saves all help files", ch->RealName());
					HEdit::SaveDisk(0); 
					break;
				case OLC_CEDIT: 
					ch->Send("Saving all clans.\r\n");
					mudlogf(NRM, ch, TRUE, "OLC: %s saves all clans", ch->RealName());
					Clan::Save();
					break;
				case OLC_SKILLEDIT: 
					ch->Send("Saving skill info.\r\n");
					mudlogf(NRM, ch, TRUE, "OLC: %s saves all skills", ch->RealName());
					save_skills();
					break;
				case OLC_REDIT: 
				case OLC_ZEDIT: 
				case OLC_OEDIT: 
				case OLC_MEDIT: 
				case OLC_SEDIT: 
				case OLC_SCRIPTEDIT: 
					half_chop(argument, arg, argument);
					if (*arg) {
						targ = atoi(arg);
						if (real_zone(targ) == -1) {
							ch->Send("That zone doesn't exist!\r\n");
							return;
						}
						ch->Sendf("Saving %s for zone %d\r\n", save_info_msg[subcmd], targ);
						gen_olc_save(targ, subcmd);
						return;
					} else {
						ch->Send("What zone do you wish to save?\r\n");
						return;
					}
					half_chop(argument, arg, argument);
				}
				return;
			} else if (subcmd == OLC_AEDIT) {
				tempStorage = str_dup(arg);
				for (targ = 0; targ < Social::internal.Count(); targ++)  {
					if (is_abbrev(tempStorage, Social::internal[targ].command))
						break;
				}
				if (targ >= Social::internal.Count()) {
					if (find_command(tempStorage) > NOTHING)  {
						free(tempStorage);
						ch->Send("That command already exists.\r\n");
						return;
					}
					targ = -1;
				}
			}

			// These never require more than one argument.  Stop parsing.
			if (subcmd == OLC_AEDIT || subcmd == OLC_HEDIT || subcmd == OLC_CEDIT)
				break;

			half_chop(argument, arg, argument);
		}
	} else {
		switch(subcmd) {
		case OLC_ZEDIT:
		case OLC_REDIT:
		case OLC_OEDIT:
		case OLC_MEDIT:
		case OLC_SEDIT:
		case OLC_SCRIPTEDIT:
			targ = find_olc_target(ch, "", subcmd);
			if (targ < 0) {
				ch->Send("Edit what virtual number?\r\n");
				return;
			}
			break;
		case OLC_HEDIT:
			ch->Send("Specify a help topic to edit.\r\n");
			return;
		case OLC_AEDIT:
			ch->Send("Specify an action to edit.\r\n");
			return;
		case OLC_CEDIT:
			ch->Send("Specify an clan number to edit.\r\n");
			return;
		default:	break;
		}
	
	}
	
	/*. Check whatever it is isn't already being edited .*/
	START_ITER(iter, d, Descriptor *, descriptor_list) {
		if ((STATE(d) - CON_REDIT + 1) == subcmd) {	// Some fudging here, 
			if ((targ != -1) && d->Edit()) {
				if (subcmd == OLC_HEDIT) {
					ch->Sendf("Help files are currently being edited by %s\r\n", 
							(d->character->CanBeSeen(ch) ? d->character->RealName() : "someone"));
				} else if (d->Edit()->number == targ) {
					ch->Sendf("That %s is currently being edited by %s.\r\n",
							olc_modes[subcmd],
							(d->character->CanBeSeen(ch) ? d->character->RealName() : "someone"));
				} else
					continue;
				return;
			}
		}
	}

	if (subcmd == OLC_HEDIT) {
		if (*argument) {
			strcat(arg, " ");
			strcat(arg, argument);
		}
		targ = HEdit::Find(arg);
	} else if ((subcmd != OLC_AEDIT) && (subcmd != OLC_CEDIT) && (subcmd != OLC_SKILLEDIT)) {
		zoneNumber = find_owner_zone(targ);
		if (zoneNumber == -1) {
			ch->Send("Sorry, there is no zone for that number!\r\n"); 
			return;
		}

		/*. Everyone but IMPLs can only edit zones they have been assigned .*/
		if (!get_zone_perms(ch, zoneNumber)) {
			return;
		}
	}

	d = ch->desc;

	if (!copyfrom)	copyfrom = targ;

	/*. Steal players descriptor start up subcommands .*/
	switch(subcmd) {
	case OLC_REDIT:
		if (Room::Find(copyfrom))	tmpeditor = new REdit(d, targ, world[copyfrom]);
		else						tmpeditor = new REdit(d, targ);
		STATE(d) = CON_REDIT;
		break;
	case OLC_ZEDIT:
		if (!Room::Find(copyfrom)) {
			ch->Send("That room does not exist.\r\n"); 
			return;
		}
		tmpeditor = new ZEdit(d, copyfrom, zoneNumber);
		STATE(d) = CON_ZEDIT;
		break;
	case OLC_MEDIT:
		if (Character::Find(copyfrom))	tmpeditor = new MEdit(d, targ, Character::Index[copyfrom]);
		else							tmpeditor = new MEdit(d, targ);
		STATE(d) = CON_MEDIT;
		break;
	case OLC_OEDIT:
		if (Object::Find(copyfrom))		tmpeditor = new OEdit(d, targ, Object::Index[copyfrom]);
		else							tmpeditor = new OEdit(d, targ);
		STATE(d) = CON_OEDIT;
		break;
	case OLC_SEDIT:
		if (Shop::Index.Find(copyfrom))	tmpeditor = new SEdit(d, &Shop::Index[copyfrom]);
		else							tmpeditor = new SEdit(d, copyfrom);
		STATE(d) = CON_SEDIT;
		break;
	case OLC_AEDIT:
		if (targ >= 0)			tmpeditor = new AEdit(d, targ, Social::internal[targ]);
		else					tmpeditor = new AEdit(d, tempStorage);
		STATE(d) = CON_AEDIT;
		break;
	case OLC_HEDIT:
		if (targ >= 0)			tmpeditor = new HEdit(d, Help::Table[targ]);
		else					tmpeditor = new HEdit(d, arg);
		STATE(d) = CON_HEDIT;
		break;
	case OLC_SCRIPTEDIT:
		if (Trigger::Find(copyfrom))	tmpeditor = new ScriptEdit(d, targ, Trigger::Index[copyfrom]);
		else							tmpeditor = new ScriptEdit(d, targ);
		STATE(d) = CON_SCRIPTEDIT;
		break;
	case OLC_CEDIT:
		if (Clan::Find(copyfrom))		tmpeditor = new CEdit(d, Clan::Clans[copyfrom]);
		else							tmpeditor = new CEdit(d, copyfrom);
		STATE(d) = CON_CEDIT;
		break;
	case OLC_SKILLEDIT:
		if (targ != -1)					tmpeditor = new SkillEditor(d, targ, Skills[copyfrom]);
		else {
			d->Write("That's not a valid skill.\r\n");
			return;
		}
		STATE(d) = CON_SKILLEDIT;
		break;
	}

	if (tmpeditor) {
		tmpeditor->zoneNum = find_owner_zone(targ);
		d->Edit(tmpeditor);
	} else {
		strcpy(buf, "SYSERR:  Invalid editor pointer in do_olc, warn the imps!");
		ch->Send(buf);
		log(buf);
		return;
	}

	act("$n starts using OLC.", TRUE, d->character, 0, 0, TO_ROOM);
	SET_BIT(PLR_FLAGS(ch), PLR_WRITING);
}


/*------------------------------------------------------------*\
 Internal utilities 
\*------------------------------------------------------------*/


void Editor::Finish(void) {
	//	Restore desciptor playing status
	if (storage)	
		free(storage);
	if (d->character)
		REMOVE_BIT(PLR_FLAGS(d->character), PLR_WRITING);
	d->editor = prev_editor; 
	if (prev_editor) {
		// We need to preserve save flags.
		if (save && (prev_editor->save == 0)) {
			prev_editor->save = 1;
		}
		prev_editor->Menu();
	} else {
		switch (d->connected) {
		case CON_EXDESC:	d->connected = CON_MENU;
							d->Write(MENU);
							break;
		default:			d->connected = CON_PLAYING;		
							break;
		}
	}

	delete this;
}


void olc_saveinfo(Character *ch) {
	struct olc_save_info *entry;
	static char *save_info_msg[] = {
		"Undefined", 
		"Rooms", 
		"Objects", 
		"Zone info", 
		"Mobiles", 
		"Shops", 
		"Actions", 
		"Help", 
		"Scripts", 
		"Clans",
		"Skills"
	};

	if (olc_save_list)	ch->Send("The following OLC components need saving:-\r\n");
	else				ch->Send("The database is up to date.\r\n");

	for (entry = olc_save_list; entry; entry = entry->next) {
		switch (entry->type) {
		case OLC_AEDIT:
		case OLC_HEDIT:
		case OLC_CEDIT:
		case OLC_SKILLEDIT:
			ch->Sendf(" - %s.\r\n", save_info_msg[(int)entry->type]);
			break;
		default:
			ch->Sendf(" - %s for zone %d.\r\n", save_info_msg[(int)entry->type], entry->zone);
			break;
		}
	}
}


/* returns the real number of the zone with given virtual number */
int real_zone(int virt)
{
  int i;

  for (i = 0; i <= top_of_zone_table; ++i)
    if (virt == zone_table[i].number)
      return i;
  
  return -1;
}

/*------------------------------------------------------------*\
 Exported utilities 
\*------------------------------------------------------------*/

/*
 * Add an entry to the 'to be saved' list.
 */
void olc_add_to_save_list(int zone, UInt8 type) {
	struct olc_save_info *new_info;

	/*
	 * Return if it's already in the list
	 */
	for(new_info = olc_save_list; new_info; new_info = new_info->next)
		if ((new_info->zone == zone) && (new_info->type == type))
			return;

	CREATE(new_info, struct olc_save_info, 1);
	new_info->zone = zone;
	new_info->type = type;
	new_info->next = olc_save_list;
	olc_save_list = new_info;
}

/*
 * Remove an entry from the 'to be saved' list
 */
void olc_remove_from_save_list(int zone, UInt8 type) {
	struct olc_save_info **entry;
	struct olc_save_info *temp;

	for (entry = &olc_save_list; *entry; entry = &(*entry)->next)
		if (((*entry)->zone == zone) && ((*entry)->type == type)) {
			temp = *entry;
			*entry = temp->next;
			FREE(temp);
			return;
		}
}


/*
 * This procedure removes the '\r\n' from a string so that it may be
 * saved to a file.  Use it only on buffers, not on the original
 * strings.
 */
void strip_string(char *buffer) {
	char *ptr, *str;

	str = ptr = buffer;

	while((*str++ = *ptr++)) {
//		str++;
//		ptr++;
		if (*ptr == '\r')
			ptr++;
	}
}



/* read builders from zone file into memory */
int read_zone_perms(int rnum) {
	FILE *old_file;
	char *arg1, *arg2;
	char *buf = get_buffer(MAX_INPUT_LENGTH);

	sprintf(buf, ZON_PREFIX "%i.zon", zone_table[rnum].number);

	if(!(old_file = fopen(buf, "r"))) {
		log("SYSERR: Could not open %s for read.", buf);
		release_buffer(buf);
		return(FALSE);
	}
	arg1 = get_buffer(MAX_INPUT_LENGTH);
	arg2 = get_buffer(MAX_INPUT_LENGTH);
	do {
		get_line2(old_file, buf);
		if(*buf == '*') {
			half_chop(buf+1, arg1, arg2);
			if (*arg1 && *arg2 && is_abbrev(arg1, "Builder"))
				zone_table[rnum].builders.Append(str_dup(arg2));
		}
	} while(*buf != '$' && !feof(old_file));
	release_buffer(buf);
	release_buffer(arg1);
	release_buffer(arg2);
	
	fclose(old_file);
	
	return(TRUE);
}


ACMD(do_zallow) {
	int zone, rnum = -1;
	FILE *old_file, *new_file;
	char *old_fname, *new_fname, *line;
	char	*arg1 = get_buffer(MAX_INPUT_LENGTH),
			*arg2 = get_buffer(MAX_INPUT_LENGTH),
			*bldr;

	half_chop(argument, arg1, arg2);

	if(!*arg1 || !*arg2 || !isdigit(*arg2)) {
		ch->Send("Usage  : zallow <player> <zone>\r\n");
		release_buffer(arg1);
		release_buffer(arg2);
		return;
	}
	*arg1 = UPPER(*arg1);
	zone = atoi(arg2);
	release_buffer(arg2);
	if((rnum = get_zone_rnum(zone)) == -1) {
		ch->Send("That zone doesn't seem to exist.\r\n");
		release_buffer(arg1);
		return;
	}

//	if(zone_table[rnum].builders == NULL)
//		read_zone_perms(rnum);

	LListIterator<char *>	iter(zone_table[rnum].builders);
	while ((bldr = iter.Next())) {
		if(bldr && !str_cmp(bldr, arg1)) {
			ch->Send("That person already has access to that zone.\r\n");	
			release_buffer(arg1);
			return;
		}
	}

	zone_table[rnum].builders.Add(str_dup(arg1));
	
	old_fname = get_buffer(256);
	new_fname = get_buffer(256);

	sprintf(old_fname, ZON_PREFIX SEPERATOR "%i.zon", zone);
	sprintf(new_fname, ZON_PREFIX SEPERATOR "%i.zon.temp", zone);
	
	if(!(old_file = fopen(old_fname, "r"))) {
		ch->Sendf("Error: Could not open %s for read.\r\n", old_fname);
		log("SYSERR: Could not open %s for read.", old_fname);
		release_buffer(old_fname);
		release_buffer(new_fname);
		release_buffer(arg1);
		return;
	}
	if(!(new_file = fopen(new_fname, "w"))) {
		ch->Sendf("Error: Could not open %s for write.\r\n", new_fname);
		log("SYSERR: Could not open %s for write.", new_fname);
		fclose(old_file);
		release_buffer(old_fname);
		release_buffer(new_fname);
		release_buffer(arg1);
		return;
	}
	line = get_buffer(256);
	get_line2(old_file, line);
	fprintf(new_file, "%s\n", line);
	get_line2(old_file, line);
	fprintf(new_file, "%s\n", line);
	get_line2(old_file, line);
	fprintf(new_file, "%s\n", line);
	fprintf(new_file, "* Builder %s\n", arg1);
	do {
		get_line2(old_file, line);
		fprintf(new_file, "%s\n", line);
	} while(*line != '$');
	release_buffer(line);
	fclose(old_file);
	fclose(new_file);
	remove(old_fname);
	rename(new_fname, old_fname);
	ch->Send("Zone file edited.\r\n");
	mudlogf(BRF, ch, TRUE, "OLC: %s gives %s zone #%i builder access.", ch->RealName(), arg1, zone);

	release_buffer(arg1);
	release_buffer(old_fname);
	release_buffer(new_fname);
}


ACMD(do_zdeny) {
	int zone, rnum, found = FALSE;
	FILE *old_file, *new_file;
	char *old_fname, *new_fname, *line;
	char *arg1, *arg2, *arg3;
	char *bldr;

	arg1 = get_buffer(MAX_INPUT_LENGTH);
	arg2 = get_buffer(MAX_INPUT_LENGTH);
	half_chop(argument, arg1, arg2);

	if(!*arg1 || !*arg2 || !isdigit(*arg2)) {
		ch->Send("Usage  : zdeny <player> <zone>\r\n");
		release_buffer(arg1);
		release_buffer(arg2);
		return;
	}
	*arg1 = UPPER(*arg1);
	zone = atoi(arg2);
	release_buffer(arg2);
	if((rnum = get_zone_rnum(zone)) == -1) {
		ch->Send("That zone doesn't seem to exist.\r\n");
		release_buffer(arg1);
		return;
	}

//	if(zone_table[rnum].builders == NULL)
//		read_zone_perms(rnum);

	LListIterator<char *>	iter(zone_table[rnum].builders);
	
	while ((bldr = iter.Next())) {
		if(!str_cmp(bldr, arg1)) {
			zone_table[rnum].builders.Remove(bldr);
			FREE(bldr);
			found = TRUE;
		}
	}

	if(!found) {
		ch->Send("That person doesn't have access to that zone.\r\n");
		release_buffer(arg1);
		return;
	}

	old_fname = get_buffer(256);
	new_fname = get_buffer(256);
	
	sprintf(old_fname, ZON_PREFIX "%i.zon", zone);
	sprintf(new_fname, ZON_PREFIX "%i.zon.temp", zone);

	if(!(old_file = fopen(old_fname, "r"))) {
		ch->Sendf("Error: Could not open %s for read.\r\n", old_fname);
		log("SYSERR: Could not open %s for read.", old_fname);
		release_buffer(old_fname);
		release_buffer(new_fname);
		return;
	}
	if(!(new_file = fopen(new_fname, "w"))) {
		ch->Sendf("Error: Could not open %s for write.\r\n", new_fname);
		log("SYSERR: Could not open %s for write.", new_fname);
		fclose(old_file);
		release_buffer(old_fname);
		release_buffer(new_fname);
		return;
	}
	
	line = get_buffer(MAX_STRING_LENGTH);
	arg2 = get_buffer(MAX_INPUT_LENGTH);
	arg3 = get_buffer(MAX_INPUT_LENGTH);
	do {
		get_line2(old_file, line);
		if(*line == '*') {
			two_arguments(line + 1, arg2, arg3);
			if(!*arg2 || str_cmp(arg2, "Builder") || str_cmp(arg1, arg3))
				fprintf(new_file, "%s\n", line);
		} else	fprintf(new_file, "%s\n", line);
	} while(*line != '$');
	release_buffer(line);
	release_buffer(arg2);
	release_buffer(arg3);
	
	fclose(old_file);
	fclose(new_file);
	remove(old_fname);
	rename(new_fname, old_fname);
	ch->Send("Zone file edited.\r\n");
	mudlogf(BRF, ch, TRUE, "OLC: %s removes %s's zone #%i builder access.", ch->RealName(), arg1, zone);

	release_buffer(arg1);
	release_buffer(old_fname);
	release_buffer(new_fname);
}


bool get_zone_perms(Character * ch, int rnum) {
	int allowed = FALSE /*, rnum*/;
	char *bldr;
	
	if(STF_FLAGGED(ch, Staff::OLCAdmin))
		return true;

//	if(zone_table[rnum].builders == NULL)
//		read_zone_perms(rnum);

    /* check memory */
//	for(bldr = zone_table[rnum].builders; bldr; bldr = bldr->next)
	LListIterator<char *>	iter(zone_table[rnum].builders);
	while ((bldr = iter.Next()))
		if(!str_cmp(ch->Name(), bldr))
			return true;

	ch->Sendf("You don't have builder rights in zone #%i\r\n", zone_table[rnum].number);
	return false;
}


ACMD(do_zstat) {
	int i, zn = -1, pos = 0, none = TRUE;
	char *bldr;
	char *rmode[] = {	"Never resets",
						"Resets only when deserted",
						"Always resets"  };

	skip_spaces(argument);

	if(*argument) {
		if(isdigit(*argument))
			zn = atoi(argument);
		else {
			ch->Send("Usage: zstat [zone number]\r\n");
			return;
		}
	} else zn = get_zon_num(IN_ROOM(ch));

	if(zn < 0) {
		log("SYSERR: Zone #%d doesn't seem to exist.", IN_ROOM(ch) / 100);
		ch->Send("Warning!  The room you're in doesn't belong to any zone in memory.\r\n");
		return;
	}

	for(i = 0; i <= top_of_zone_table && zone_table[i].number != zn; i++);
	if(i > top_of_zone_table) {
		ch->Send("That zone doesn't exist.\r\n");
		return;
	}

	if((zone_table[i].reset_mode < 0) || (zone_table[i].reset_mode > 2))
		zone_table[i].reset_mode = 2;

	sprintbit(zone_table[i].climate.flags, Weather::climateflags, buf);

	ch->Sendf("&cmZone #      : &cw%3d       &cmName      : &cw%s\r\n"
			 "&cmAge/Lifespan : &cw%3d&cm/&cw%3d   &cmTop vnum  : &cw%5d\r\n"
			 "&cmReset mode   : &cw%s\r\n"
			 "&cmClimate  : &cw%s  &cmPrecipitation  : &cw%s  &cmEnergy  : &cw%d\r\n"
			 "&cmClimate Flags : &cw%s\r\n"
			 "&cmBuilders     : &cw",
			zone_table[i].number, zone_table[i].name,
			zone_table[i].age, zone_table[i].lifespan, zone_table[i].top,
			rmode[zone_table[i].reset_mode],
			Weather::temperatures[zone_table[i].climate.temperature[0]],
			Weather::precipitations[zone_table[i].climate.precipitation[0]],
			zone_table[i].climate.energy,
			buf);

	LListIterator<char *>	iter(zone_table[i].builders);
	while ((bldr = iter.Next())) {
		if(*(bldr) != '*') {
			none = FALSE;
			switch (pos) {
				case 0:
					ch->Sendf("%s", bldr);
					pos = 2;
					break;
				case 1:
					ch->Sendf("\r\n              %s", bldr);
					pos = 2;
					break;
				case 2:
					ch->Sendf(", %s", bldr);
					pos = 3;
					break;
				case 3:
					ch->Sendf(", %s", bldr);
					pos = 1;
					break;
			}
		}
	}

	if(none)	ch->Send("NONE");
	ch->Send("&c0\r\n");
}


/* return zone rnum */
int get_zone_rnum(int zone) {
	int i;

	for(i = 0; i <= top_of_zone_table && zone_table[i].number != zone; i++);
	if(i > top_of_zone_table)	return -1;
	else						return i;
}


int get_line2(FILE * fl, char *buf) {
	char *temp = get_buffer(256);
	int lines = 0;

	do {
		lines++;
		fgets(temp, 256, fl);
		if (*temp)	temp[strlen(temp) - 1] = '\0';
	} while (!feof(fl) && (!*temp));

	if (feof(fl)) {
		release_buffer(temp);
		return 0;
	} else {
		strcpy(buf, temp);
		release_buffer(temp);
		return lines;
	}
}


/* check for true zone number */
int get_zon_num(int vnum) {
	int i, zone;

	zone = vnum / 100;

	for(i = 0; i <= top_of_zone_table && zone_table[i].number <= zone; i++);
	if(i <= top_of_zone_table || zone_table[i - 1].top >= vnum)
		return zone_table[i - 1].number;

	return -1;
}


ACMD(do_delete) {
	Character *victim = NULL;
	Object *object = NULL;
	Room *room = NULL;
	Trigger *trigger = NULL;
	int which, zone;
	char	*arg1, *arg2;
	
	if (subcmd != SCMD_DELETE && subcmd != SCMD_UNDELETE) {
		ch->Send("ERROR: Unknown subcmd to do_delete");
		return;
	}
	arg1 = get_buffer(MAX_INPUT_LENGTH);
	arg2 = get_buffer(MAX_INPUT_LENGTH);
	
	two_arguments(argument, arg1, arg2);
	
	if (!*arg1 || !*arg2 || (which = atoi(arg2)) <= 0) {
		if (subcmd == SCMD_DELETE)
			ch->Send("Usage: delete mob|obj|room <vnum>\r\n");
		else if (subcmd == SCMD_UNDELETE)
			ch->Send("Usage: undelete mob|obj|room vnum\r\n");
	} 
	
	zone = find_owner_zone(which);
	
	if (zone == -1)
		ch->Send("That number is not valid.\r\n");
	else if (!STF_FLAGGED(ch, Staff::OLCAdmin) && !get_zone_perms(ch, zone))
		ch->Send("You don't have permission to (un)delete that.\r\n");
	else if (is_abbrev(arg1, "room")) {
		if (!Room::Find(which))
			ch->Send("That is not a valid room number.\r\n");
		else {
			if (subcmd == SCMD_DELETE)	SET_BIT(ROOM_FLAGS(which), ROOM_DELETED);
			else						REMOVE_BIT(ROOM_FLAGS(which), ROOM_DELETED);
			mudlogf(NRM, ch, TRUE, "OLC: %s marks room %d as %sdeleted.",
					ch->RealName(), which, subcmd == SCMD_DELETE ? "" : "un");
			olc_add_to_save_list(zone_table[zone].number, OLC_REDIT);
		}
	} else if (is_abbrev(arg1, "mob")) {
		if (!Character::Find(which))
			ch->Send("That is not a valid mobile number.\r\n");
		else {
			victim = Character::Index[which].proto;
			if (subcmd == SCMD_DELETE)	SET_BIT(MOB_FLAGS(victim), MOB_DELETED);
			else						REMOVE_BIT(MOB_FLAGS(victim), MOB_DELETED);
			mudlogf(NRM, ch, TRUE, "OLC: %s marks mob %d as %sdeleted.",
					ch->RealName(), which, subcmd == SCMD_DELETE ? "" : "un");
			olc_add_to_save_list(zone_table[zone].number, OLC_MEDIT);
		}
	} else if (is_abbrev(arg1, "object")) {
		if (!Object::Find(which))
			ch->Send("That is not a valid object number.\r\n");
		else {
			object = Object::Index[which].proto;
			if (subcmd == SCMD_DELETE)	SET_BIT(GET_OBJ_EXTRA(object), ITEM_DELETED);
			else						REMOVE_BIT(GET_OBJ_EXTRA(object), ITEM_DELETED);
			mudlogf(NRM, ch, TRUE, "OLC: %s marks object %d as %sdeleted.",
					ch->RealName(), which, subcmd == SCMD_DELETE ? "" : "un");
			olc_add_to_save_list(zone_table[zone].number, OLC_OEDIT);
		}
	} else if (is_abbrev(arg1, "trigger") || is_abbrev(arg1, "script")) {
		if (!Trigger::Find(which))
			ch->Send("That is not a valid trigger number.\r\n");
		else {
			trigger = Trigger::Index[which].proto;
			if (subcmd == SCMD_DELETE)	SET_BIT(GET_TRIG_TYPE_WLD(trigger), TRIG_DELETED);
			else						REMOVE_BIT(GET_TRIG_TYPE_WLD(trigger), TRIG_DELETED);
			mudlogf(NRM, ch, TRUE, "OLC: %s marks trigger %d as %sdeleted.",
					ch->RealName(), which, subcmd == SCMD_DELETE ? "" : "un");
			olc_add_to_save_list(zone_table[zone].number, OLC_SCRIPTEDIT);
		}
// 	} else if (is_abbrev(arg1, "shop")) {
// 		if (!Shop::Find(which))
// 			ch->Send("That is not a valid shop number.\r\n");
// 		else {
// 			trigger = Shop::Index[which].proto;
// 			if (subcmd == SCMD_DELETE)	SET_BIT(GET_TRIG_TYPE_WLD(trigger), TRIG_DELETED);
// 			else						REMOVE_BIT(GET_TRIG_TYPE_WLD(trigger), TRIG_DELETED);
// 			mudlogf(NRM, ch, TRUE, "OLC: %s marks trigger %d as %sdeleted.",
// 					ch->RealName(), which, subcmd == SCMD_DELETE ? "" : "un");
// 			olc_add_to_save_list(zone_table[zone].number, OLC_SCRIPTEDIT);
// 		}
	} else if (subcmd == SCMD_DELETE)
		ch->Send("Usage: delete mob|obj|room|trigger <vnum>\r\n");
	else if (subcmd == SCMD_UNDELETE)
		ch->Send("Usage: undelete mob|obj|room|trigger <vnum>\r\n");
	else
		mudlogf(NRM, ch, TRUE, "SYSERR: Reached default ELSE in do_delete!  Builder: %s  Argument: %s", ch->RealName(), argument);

	release_buffer(arg1);
	release_buffer(arg2);
}


#define ZCMD (zone_table[zone].cmd[cmd_no])
#define S_ROOM(i, num)		(Shop::Index[i].in_room[(num)])
#define S_PRODUCT(i, num)	(Shop::Index[i].producing[(num)])
#define S_KEEPER(i)			(Shop::Index[i].keeper)

ACMD(do_move_element) {
	char	*arg = get_buffer(MAX_INPUT_LENGTH),
			*arg2 = get_buffer(MAX_INPUT_LENGTH);
	VNum	orig, targ;
	SInt32	znum, znum2;
	SInt32	zone, cmd_no, dir;
	SInt32	n;
	
	two_arguments(argument, arg, arg2);
	
	if (!*arg || !*arg2) {
		switch (subcmd) {
			case SCMD_RMOVE:	ch->Send("Usage: rmove <orig> <new>\r\n");	break;
			case SCMD_OMOVE:	ch->Send("Usage: omove <orig> <new>\r\n");	break;
			case SCMD_MMOVE:	ch->Send("Usage: mmove <orig> <new>\r\n");	break;
			case SCMD_TMOVE:	ch->Send("Usage: tmove <orig> <new>\r\n");	break;
			default:
				mudlogf(BRF, NULL, TRUE, "SYSERR:: invalid SCMD passed to do_move_element!  (SCMD = %d)", subcmd);
				break;
		}
	} else {
		orig = atoi(arg);
		targ = atoi(arg2);
		znum = real_zone(orig);
		znum2 = real_zone(targ);
		if ((znum == -1) || (znum2== -1)) {
			ch->Sendf("Zone %d does not exist.\r\n", (znum == -1) ? (orig / 100) : (targ / 100));
		} else if (get_zone_perms(ch, znum) && get_zone_perms(ch, znum2)) {
			switch (subcmd) {
				case SCMD_RMOVE:
					if (!Room::Find(orig))		ch->Send("No such room exists.");
					else if (Room::Find(targ))	ch->Send("You can't replace an existing room.");
					else {
						world.Move(orig, targ);
						world[targ].Virtual(targ);
						ch->Sendf("Room %d moved to %d.", orig, targ);
						olc_add_to_save_list(zone_table[znum].number, OLC_REDIT);
						olc_add_to_save_list(zone_table[znum2].number, OLC_REDIT);
						for (zone = 0; zone <= top_of_zone_table; zone++) {
							for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++) {
								switch(ZCMD.command) {
									case 'M':
									case 'O':
										if (ZCMD.arg3 == orig) {
											ZCMD.arg3 = targ;
											olc_add_to_save_list(zone_table[zone].number, OLC_ZEDIT);
										}
										break;
									case 'D':
									case 'R':
										if (ZCMD.arg1 == orig) {
											ZCMD.arg1 = targ;
											olc_add_to_save_list(zone_table[zone].number, OLC_ZEDIT);
										}
										break;
									case 'T':
										if ((ZCMD.arg1 == WLD_TRIGGER) && (ZCMD.arg4 == orig)) {
											ZCMD.arg4 = targ;
											olc_add_to_save_list(zone_table[zone].number, OLC_ZEDIT);
										}
								}
							}
						}
						
						Map<VNum, Room>::Iterator	roomIter(world);
						Room *rm;
						while ((rm = roomIter.Next())) {
							for (dir = 0; dir < NUM_OF_DIRS; dir++) {
								if (rm->Direction(dir) && rm->Direction(dir)->to_room == orig) {
									rm->Direction(dir)->to_room = targ;
									olc_add_to_save_list(get_zon_num(rm->Virtual()), OLC_REDIT);
								}
							}
						}
						Map<VNum, Shop>::Iterator	shopIter(Shop::Index);
						Shop *shop;
						while ((shop = shopIter.Next())) {
							for (n = 0; n < shop->in_room.Count(); n++) {
								if (shop->in_room[n] == orig) {
									shop->in_room[n] = targ;
									olc_add_to_save_list(get_zon_num(shop->vnum), OLC_SEDIT);
								}
							}
						}
					}
					break;
				case SCMD_OMOVE:
					if (!Object::Find(orig))		ch->Send("No such object exists.");
					else if (Object::Find(targ))	ch->Send("You can't replace an existing object.");
					else {
						Object::Index.Move(orig, targ);
						Object::Index[targ].vnum = targ;
						Object::Index[targ].proto->Virtual(targ);
						ch->Sendf("Obj %d moved to %d.", orig, targ);
						olc_add_to_save_list(zone_table[znum].number, OLC_OEDIT);
						olc_add_to_save_list(zone_table[znum2].number, OLC_OEDIT);
						Object *o;
						START_ITER(iter, o, Object *, Objects) {
							if (o->Virtual() == orig)
								o->Virtual(targ);
						}
						START_ITER(iter2, o, Object *, PurgedObjs) {
							if (o->Virtual() == orig)
								o->Virtual(targ);
						}
						for (zone = 0; zone <= top_of_zone_table; zone++) {
							for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++) {
								switch(ZCMD.command) {
									case 'O':
									case 'G':
									case 'E':
										if (ZCMD.arg1 == orig) {
											ZCMD.arg1 = targ;
											olc_add_to_save_list(zone_table[zone].number, OLC_ZEDIT);
										}
										break;
									case 'R':
										if (ZCMD.arg2 == orig) {
											ZCMD.arg2 = targ;
											olc_add_to_save_list(zone_table[zone].number, OLC_ZEDIT);
										}
										break;
									case 'P':
										if (ZCMD.arg3 == orig) {
											ZCMD.arg3 = targ;
											olc_add_to_save_list(zone_table[zone].number, OLC_ZEDIT);
										}
										break;
									default:
										continue;
								}
							}
						}
						Map<VNum, Shop>::Iterator	shopIter(Shop::Index);
						Shop *shop;
						while ((shop = shopIter.Next())) {
							for (n = 0; n < shop->producing.Count(); n++) {
								if (shop->producing[n] == orig) {
									shop->producing[n] = targ;
									olc_add_to_save_list(get_zon_num(shop->vnum), OLC_SEDIT);
								}
							}
						}
					}
					break;
				case SCMD_MMOVE:
					if (!Character::Find(orig))		ch->Send("No such mob exists.");
					else if (Character::Find(targ))	ch->Send("You can't replace an existing mob.");
					else {
						Character::Index.Move(orig, targ);
						Character::Index[targ].vnum = targ;
						Character::Index[targ].proto->Virtual(targ);
						ch->Sendf("Mob %d moved to %d.", orig, targ);
						olc_add_to_save_list(zone_table[znum].number, OLC_MEDIT);
						olc_add_to_save_list(zone_table[znum2].number, OLC_MEDIT);
						Character *c;
						START_ITER(iter, c, Character *, Characters) {
							if (c->Virtual() == orig)
								c->Virtual(targ);
						}
						START_ITER(iter2, c, Character *, PurgedChars) {
							if (c->Virtual() == orig)
								c->Virtual(targ);
						}
						for (zone = 0; zone <= top_of_zone_table; zone++) {
							for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++) {
								if ((ZCMD.command == 'M') && (ZCMD.arg1 == orig)) {
									ZCMD.arg1 = targ;
									olc_add_to_save_list(zone_table[zone].number, OLC_ZEDIT);
								}
//										ch->Sendf("Zone %d ZEDITS affected.\r\n", zone);
							}
						}
						Map<VNum, Shop>::Iterator	shopIter(Shop::Index);
						Shop *shop;
						while ((shop = shopIter.Next())) {
							if (shop->keeper == orig) {
								shop->keeper = targ;
								olc_add_to_save_list(get_zon_num(shop->vnum), OLC_SEDIT);
							}
						}
					}
					break;
				case SCMD_TMOVE:
					if (!Trigger::Find(orig))		ch->Send("No such mob exists.");
					else if (Trigger::Find(targ))	ch->Send("You can't replace an existing mob.");
					else {
						Trigger::Index.Move(orig, targ);
						Trigger::Index[targ].vnum = targ;
						Trigger::Index[targ].proto->Virtual(targ);
						ch->Sendf("Trig %d moved to %d.", orig, targ);
						olc_add_to_save_list(zone_table[znum].number, OLC_SCRIPTEDIT);
						olc_add_to_save_list(zone_table[znum2].number, OLC_SCRIPTEDIT);
						Trigger *t;
						START_ITER(iter, t, Trigger *, Triggers) {
							if (t->Virtual() == orig)
								t->Virtual(targ);
						}
						START_ITER(iter2, t, Trigger *, PurgedTrigs) {
							if (t->Virtual() == orig)
								t->Virtual(targ);
						}
						for (zone = 0; zone <= top_of_zone_table; zone++) {
							for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++) {
								if ((ZCMD.command == 'T') && (ZCMD.arg2 == orig)) {
									ZCMD.arg2 == targ;
									olc_add_to_save_list(zone_table[zone].number, OLC_ZEDIT);
								}
							}
						}
						Index<Character>::Iterator	charIter(Character::Index);
						IndexData<Character> *c;
						while ((c = charIter.Next())) {
							for (n = 0; n < c->triggers.Count(); n++) {
								if (c->triggers[n] == orig) {
									c->triggers[n] = targ;
									olc_add_to_save_list(get_zon_num(c->vnum), OLC_MEDIT);
								}
							}
						}
						Index<Object>::Iterator	objIter(Object::Index);
						IndexData<Object> *o;
						while ((o = objIter.Next())) {
							for (n = 0; n < o->triggers.Count(); n++) {
								if (o->triggers[n] == orig) {
									o->triggers[n] = targ;
									olc_add_to_save_list(get_zon_num(o->vnum), OLC_OEDIT);
								}
							}
						}
					}
					break;
				default:
					ch->Send("Hm, reached default case on do_move_element.\r\n");
					mudlogf(BRF, NULL, TRUE, "SYSERR:: invalid SCMD passed to do_move_element!  (SCMD = %d)", subcmd);
					break;
			}
		}
	}
	release_buffer(arg);
	release_buffer(arg2);
}


ACMD(do_massroomsave) {
	int	zone;
//	char *buf = get_buffer(MAX_INPUT_LENGTH);
	for (zone = 0; zone <= top_of_zone_table; zone++) {
//		sprintf(buf, "save %d", zone_table[zone].number);
//		do_olc(ch, buf, 0, "medit", OLC_REDIT);
			; // CHANGEPOINT: ZEdit::SaveDisk(zone);
	}
//	release_buffer(buf);
}


int olc_edit_integer(Descriptor *d, long *result, char *argument,
		     char *name, long min, long max)
{
  long n;

  if (*argument) {
    n = atoi(argument);
    if (min == max || (n <= max && n >= min)) {
      *result = n;
      sprintf(buf, "%s set to %ld.\r\n", name, n);
      d->Write(CAP(buf));
      return 1;
    }
  }

//   if (min != max)
//     sprintf(buf, "%s must be between %ld and %ld.\r\n", name, min, max);
//   else
//     sprintf(buf, "Enter a number for %s.\r\n", name);

//  d->Write(CAP(buf));

  return 0;
}


int olc_edit_value(Descriptor *d, long *result, char *argument,
		   char *name, long min, long max, const char **values, bool diff = FALSE)
{
  long n, i, start;
  
  start = MAX(min, 0);
  if (!max)
    max = 32000;

  if (!*argument || *argument == '?') {
//    sprintf(buf, "Valid values for %s are:\r\n", name);

    *buf = '\0';
    
    /* list values in columns */
    for (i = 0; (*values[i + start] != '\n') && ((i + start) <= max); ++i) {
      sprintf(buf + strlen(buf), "  &cG%2ld&c0) %-17.17s", i + 1, values[i + start]);
      if ((i % 3) == 2)
	strcat(buf, "\r\n");
    }

    if (i % 3)
      strcat(buf, "\r\n");
    page_string(d, buf, true);
    return 0;
  }

  if ((*argument == '-') || isdigit(*argument)) {
    n = atoi(argument);
    --n;
    for (i = 0; ((*values[i] != '\n') && ((i + start) <= max)); ++i);
    n += start;
    if ((n < min) || (n > (i + start))) {
      sprintf(buf, "Unknown value for %s.\r\n", name);
      d->Write(buf);
      return 0;
    }
  } else {
    if ((n = search_block(argument, values, FALSE)) < 0) {
      sprintf(buf, "Unknown value for %s.\r\n", name);
      d->Write(buf);
      return 0;
    }
  }

//  if (n >= 0)
    d->Writef("%s set to %s.\r\n", name, values[n]);

  if (diff) {
    n -= min;
  }

  *result = n;

  return 1;
}


// Display function used by olc_edit_bitvector
void olc_display_bitvector(Descriptor *d, const char **values, Flags &mask, Flags &i)
{
  /* list values */
  sprintf(buf, "Valid flags are:\r\n");
  
  for (i = 0; *values[i] != '\n'; ++i) {
    if (!(mask & (1 << i)))
      sprintf(buf1, "&cG%2d&c0", i + 1);
    else
      sprintf(buf1, "%s", " *");
    sprintf(buf + strlen(buf), "  %s) %-20s%s", buf1,
            values[i], ((i + 1) % 3) ? "" : "\r\n");
  }
  
  if (i % 3)
    strcat(buf, "\r\n");
  
  if (mask)
    strcat(buf, "Selections marked with * cannot be changed.\r\n");
    
  d->Write(buf);
}


// Edit routine designed for Oasis bitvector menus.  Returns 0 for quitting
// a menu, 1 for invalid input, 2 for successful input.
int olc_edit_bitvector(Descriptor *d, Flags *result, char *argument,
		       Flags old_bv, char *name, const char **values, Flags mask)
{
  Flags i = 0;
  bool numbers = TRUE, remove = FALSE;
  Flags flag, bv = 0;
  char *s, buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];

  if (!*argument || *argument == '?') {

    olc_display_bitvector(d, values, mask, i);
    *buf = '\0';

    for (i = 0; i < 32; ++i) {
      if (IS_SET(old_bv, (1 << i))) {
        if (*buf) {
          sprintf(buf + strlen(buf), ", %s", values[i]);
        } else {
          strcpy(buf, values[i]);
        }
      }
    }
    d->Writef("\r\nCurrent %s: &cC%s&c0\r\n"
                  "Enter selections separated by spaces, or select 0 to quit: ", 
                  name, (*buf ? buf : "NONE") );

    return 1;
  }

  if (*argument == '0') {
    *result = old_bv;
    return 0;
  }
  
  // Figure out the top value in this bitvector.
  for (i = 0; *values[i] != '\n'; ++i);
  
  half_chop(argument, buf, argument);

  while (*buf) {
    flag = atoi(buf);
    --flag;
    if (flag < 0)
      break;
    if ((flag < i) && (!(mask & (1 << flag))))
      SET_BIT(bv, 1 << flag);

    half_chop(argument, buf, argument);
  }

  /* mask out changes to reserved bits */
  *result = old_bv ^ bv;

  return 2;
}


#ifdef GOT_RID_OF_IT
int olc_edit_string(Descriptor *d, char **string, char *argument,
				char *name, EDITFUNC(*func), void *ed_data, int size,
				int append)
{
	int len = 0, alen;
	char buf[MAX_STRING_LENGTH], *str;

	if (*argument == '+') {	/* append mode */
		if (*string) {
			len = strlen(*string);
			if (len >= size) {
				ch->Send("The string is already at its maximum length.\r\n");
				return 0;
			}

			SET_BIT(append, EDIT_APPEND);
		}

		++argument;
	}

	if (!*argument) {		/* switch to editor */
		if (IS_SET(append, EDIT_APPEND))
			sprintf(buf, "Appending %s (maximum of %d characters).\r\n"
				"Current string is:\r\n%s\r\n", name, size - len, *string);
		else
			sprintf(buf, "Enter new %s (maximum of %d characters).\r\n",
				name, size);

		ch->Send(buf);
		string_edit(ch->desc, *string, size, func, ed_data, append);

		act("$n starts editing.", TRUE, ch, NULL, NULL, TO_ROOM);
		
		return 1;
	}

	else {			/* string given with command */
		if (ed_data)
			FREE(ed_data);

		alen = strlen(argument);
		if ((alen + len) >= size)
			argument[size - len - 1] = '\0';

		CREATE(str, char, MIN(size, len + alen + 1));
		if (IS_SET(append, EDIT_APPEND))
			strcpy(str, *string);
		strcpy(str + len, argument);

		if (*string)
			FREE(*string);
		
		*string = str;

		sprintf(buf, "%s %s.\r\n", name, (IS_SET(append, EDIT_APPEND)) ? 
						"appended" : "changed");
		ch->Send(CAP(buf));

		return 1;
	}			
}


int olc_edit_sstring(Descriptor *d, sstring **string, char *argument,
				 char *name, EDITFUNC(*func), void *ed_data, int size, 
				 int append)
{
	int len = 0, alen;
	char buf[MAX_STRING_LENGTH], *str;

	if (*argument == '+') {	/* append string */
		if (*string) {
			len = strlen(ss_data(*string));
			if (len >= size) {
	ch->Send("The string is already at its maximum length.\r\n");
	return 0;
			}

			SET_BIT(append, EDIT_APPEND);
		}

		argument++;
	}

	if (!*argument) {		/* switch to editor */
		if (IS_SET(append, EDIT_APPEND))
			sprintf(buf, "Appending %s (maximum of %d characters).\r\n"
				"Current string is:\r\n%s\r\n\r\n", name, size - len,
				ss_data(*string));
		else
			sprintf(buf, "Enter new %s (maximum of %d characters).\r\n",
				name, size);

		ch->Send(buf);
		string_edit(ch->desc, ss_data(*string), size, func, ed_data, append);

		act("$n starts editing.", TRUE, ch, NULL, NULL, TO_ROOM);

		return 1;
	}

	else {			/* string given with command */
		if (ed_data)
			FREE(ed_data);

		alen = strlen(argument);
		if ((alen + len) >= size)
			argument[size - len - 1] = '\0';

		CREATE(str, char, MIN(size, len + alen + 1));
		if (IS_SET(append, EDIT_APPEND))
			strcpy(str, ss_data(*string));
		strcpy(str + len, argument);

		ss_free(*string);
		*string = ss_create(str);
		FREE(str);

		sprintf(buf, "%s %s.\r\n", name, (IS_SET(append, EDIT_APPEND)) ? 
						"appended" : "changed");
		ch->Send(CAP(buf));

		return 1;
	}			
}
#endif


void Editable::Print(Descriptor *d, int i, char *buf)
{
	char		*selection = get_buffer(80), 
				*lineptr;
	void		*ptr;
	char		**stringptr;
	SString		**ssptr;
	long		numvalue = 0;
	const struct olc_fields *constraints = NULL;

	ptr = GetPointer(i);

	if (!ptr) {
		log("Bad pointer in EditMenu.	 Tell the imps!");
		release_buffer(selection);
		core_dump();
		return;
	}

	constraints = Constraints(i);
	if (!constraints) {
		log("Bad constraints pointer in EditMenu.  Tell the imps!");
		release_buffer(selection);
		core_dump();
		return;
	}

	strcpy(selection, constraints->name);
	if ((lineptr = strchr(selection, '(')) != NULL)		*lineptr = '\0';
	constraints->values;
	

	/* assign value */
	switch (constraints->type) {
	case EDT_SINT8:		numvalue = *(SInt8 *) ptr;		break;
	case EDT_UINT8:		numvalue = *(UInt8 *) ptr;		break;
	case EDT_SINT16:	numvalue = *(SInt16 *) ptr;		break;
	case EDT_UINT16:	numvalue = *(UInt16 *) ptr;		break;
	case EDT_SINT32:	numvalue = *(SInt32 *) ptr;		break;
	case EDT_UINT32:	numvalue = *(UInt32 *) ptr;		break;
	case EDT_VNUM:		numvalue = *(VNum *) ptr;		break;
	case EDT_IDNUM:		numvalue = *(IDNum *) ptr;		break;
	default:											break;
	}

	switch (constraints->what) {
	case Datatypes::Integer:
		sprintf(buf + strlen(buf), "&cC%ld\r\n", numvalue);
		break;
	case Datatypes::Bitvector:
		strcat(buf, "&cC");
		sprintbit(numvalue, constraints->values, buf + strlen(buf));
		strcat(buf, "&c0\r\n");
		break;
	case Datatypes::Bool:
		sprintf(buf + strlen(buf), "&cC%s\r\n", (*(bool *) ptr ? "yes" : "no"));
		break;
	case Datatypes::Value:
	case Datatypes::SpellValue:
		if (numvalue < 0)	strcpy(buf, "&cC-\r\n");
		else				sprintf(buf, "&cC%s\r\n", (constraints->values)[numvalue]);
		break;
	case Datatypes::DiffValue:
		if (numvalue < 0)	strcpy(buf, "&cC-\r\n");
		else				sprintf(buf, "&cC%s\r\n", (constraints->values)[numvalue - constraints->min]);
		break;
	case Datatypes::CString:
	case Datatypes::CStringLine:
		stringptr = (char **) ptr;
		if (stringptr && *stringptr) {
			sprintf(buf, "&cC%s\r\n", *stringptr);
		} else {
			strcat(buf, "\r\n");
		}
		break;
	case Datatypes::SString:
		ssptr = (SString **) ptr;
		if (*ssptr)
			sprintf(buf, "&cC%s\r\n", (*ssptr)->str);
		break;
	default:
		strcat(buf, "\r\n");
		break;
	}

	release_buffer(selection);
}


void Editable::SaveByConstraint(FILE *fp, const struct olc_fields *constraint, void *ptr, int indent = 1)
{
	int			i;
	char		*selection = NULL,
				*lineptr,
				*indentation;
	char		**stringptr;
	SString		**ssptr;
	Trigger		**trig;
	Vector<AffectEditable> * aevector;
	Vector<int> * intvector;
	AffectEditable *ae;
	CmdlistElement *cmd;
	long		numvalue = 0;

	selection = get_buffer(80);
	indentation = get_buffer(indent + 2);
	while (indent--) {
		strcat(indentation, "\t");
	}

	one_argument(constraint->name, selection);
		if ((lineptr = strchr(selection, '(')) != NULL)		*lineptr = '\0';

	/* assign value */
	switch (constraint->type) {
	case EDT_SINT8:		numvalue = *(SInt8 *) ptr;		break;
	case EDT_UINT8:		numvalue = *(UInt8 *) ptr;		break;
	case EDT_SINT16:	numvalue = *(SInt16 *) ptr;		break;
	case EDT_UINT16:	numvalue = *(UInt16 *) ptr;		break;
	case EDT_SINT32:	numvalue = *(SInt32 *) ptr;		break;
	case EDT_UINT32:	numvalue = *(UInt32 *) ptr;		break;
	case EDT_VNUM:		numvalue = *(VNum *) ptr;		break;
	case EDT_IDNUM:		numvalue = *(IDNum *) ptr;		break;
	default:											break;
	}

	switch (constraint->what) {
	case Datatypes::Integer:
		fprintf(fp, "%s%s = %ld;\n", indentation, selection, numvalue);
		break;
	case Datatypes::Bitvector:
		if (numvalue) {	
			sprintbit(numvalue, constraint->values, buf);
			fprintf(fp, "%s%s = { %s };\n", indentation, selection, buf);
		}
		break;
	case Datatypes::Bool:
		fprintf(fp, "%s%s = %d;\n", indentation, selection, (*(bool *) ptr ? 1 : 0));
		break;
	case Datatypes::Value:
	case Datatypes::SpellValue:
	case Datatypes::DiffValue:
		if (numvalue >= 0)
			fprintf(fp, "%s%s = \"%s\";\n", indentation, selection, (constraint->values)[numvalue]);
		break;
	case Datatypes::CStringLine:
		stringptr = (char **) ptr;
		if (stringptr && *stringptr) {
			sprintf(buf, "%s%s = %%s;\n", indentation, selection);
			fprintstring(fp, buf, *stringptr);
		}
		break;
	case Datatypes::CString:
		stringptr = (char **) ptr;
		if (stringptr && *stringptr) {
			sprintf(buf, "%s%s = %%s", indentation, selection);
			fprintstring(fp, buf, *stringptr);
		}
		break;
	case Datatypes::SString:
		ssptr = (SString **) ptr;
		if (ssptr && *ssptr) {
			sprintf(buf, "%s%s = %%s", indentation, selection);
			fprintstring(fp, buf, (*ssptr)->str);
		}
		break;
	case (Datatypes::Pointer + Datatypes::Trigger):
		*buf2 = '\0';
		trig = (Trigger **) ptr;
		if (*trig) {
			cmd = (*trig)->cmdlist->command; 
			while (cmd) {
				if (cmd->cmd)
					strcat(buf2, cmd->cmd);
				strcat(buf2, "\\n");
				cmd = cmd->next;
				if (cmd)	strcat(buf2, "\n");
			}
			fprintf(fp, "%s%s = \"%s\";\n", indentation, selection, *buf2 ? buf2 : "* Empty script\n");
		}
		break;
	case (Datatypes::AffectEditable + Datatypes::Vector):
		*buf2 = '\0';
		aevector = (Vector<AffectEditable> *) ptr;
		if (aevector) {
			for (i = 0; i < aevector->Count(); ++i) {
				ae = &((*aevector)[i]);
				if (ae) {
					sprintf(buf2, "%s%s = %s %d %d { %%s };\n", indentation, selection, 
							apply_types[ae->location], ae->modifier, ae->duration);

					fprintbits(fp, buf2, ae->flags, affected_bits);
				}
			}
		}
		break;
	case (Datatypes::Integer + Datatypes::Vector):
		*buf2 = '\0';
		intvector = (Vector<int> *) ptr;
		if (intvector) {
			for (i = 0; i < intvector->Count(); ++i) {
				sprintf(buf2 + strlen(buf2), "%d ", (*intvector)[i]);
			}
			fprintf(fp, "%s%s = { %s};\n", indentation, selection, buf2);
		}
		break;
	case (Datatypes::SkillRef + Datatypes::Vector):
		{
			char			*lineptr = NULL,
							*name = get_buffer(MAX_STRING_LENGTH);

			*buf2 = '\0';
			Vector<SkillRef> * sklvector;
			sklvector = (Vector<SkillRef> *) ptr;
			if (sklvector) {
				for (i = 0; i < sklvector->Count(); ++i) {
					strcpy(name, Skill::Name((*sklvector)[i].skillnum));
					while ((lineptr = strchr(name, ' ')) != NULL)
						*lineptr = '_';
					fprintf(fp, "%s%s = %s %d;\n", indentation, selection, name, (*sklvector)[i].chance);
				}
			}
			release_buffer(name);
			break;
		}
	}

	release_buffer(indentation);
	release_buffer(selection);
}


void Editable::SaveDisk(FILE *fp, int indent = 1)
{
	void		*ptr;
	int			i;
	const struct olc_fields *constraint = NULL;

	i = 0;
	constraint = Constraints(0);

	while (constraint) {
		ptr = GetPointer(i);
		if (ptr) {
			SaveByConstraint(fp, constraint, ptr, indent);
		}

		++i;
		constraint = Constraints(i);
	}
}


Ptr	Character::GetPointer(int arg)
{
	Ptr	retval = NULL;

switch (arg) {
	case MobAffFlags:		retval = ((Ptr) &affectbits);				break;
	case MobAttackType:		retval = ((Ptr) &(mob->attack_type));		break;
	case MobBodytype:		retval = ((Ptr) &(general.bodytype));		break;
	case MobAlignment:		retval = ((Ptr) &(general.alignment));	break;
	case MobHitroll:		retval = ((Ptr) &(points.hitroll));		break;
	case MobDamroll:		retval = ((Ptr) &(points.damroll));		break;
	case MobDisposition:	retval = ((Ptr) &(mob->disposition));		break;
	case MobLevel:			retval = ((Ptr) &(general.level));		break;
	case MobGold:			retval = ((Ptr) &(points.gold));			break;
	case MobMaxRiders:		retval = ((Ptr) &max_riders);				break;
	case MobNumHPDice:		retval = ((Ptr) &(mob->numhitdice));			break;
	case MobSizeHPDice:		retval = ((Ptr) &(mob->sizehitdice));			break;
	case MobAddHP:			retval = ((Ptr) &(mob->modhitdice));			break;
	case MobDamResist:		retval = ((Ptr) &(damage_resistance));		break;
	case MobExpBonus:		retval = ((Ptr) &(general.exp_modifier));	break;
	case MobTotalPoints:	retval = ((Ptr) &(general.totalpts));		break;
	case MobType:			retval = ((Ptr) &(mob->type));			break;
	case MobNPCFlags:		retval = ((Ptr) &(general.act));			break;
	case MobPosition:		retval = ((Ptr) &(general.position));		break;
	case MobDefPosition:	retval = ((Ptr) &(mob->default_pos));		break;
	case MobRace:			retval = ((Ptr) &(general.race));			break;
	case MobClass:			retval = ((Ptr) &(general.clss));			break;
	case MobSex:			retval = ((Ptr) &(general.sex));			break;
	case MobStr:			retval = ((Ptr) &(aff_abils.Str));	break;
	case MobInt:			retval = ((Ptr) &(aff_abils.Int));	break;
	case MobWis:			retval = ((Ptr) &(aff_abils.Wis));	break;
	case MobDex:			retval = ((Ptr) &(aff_abils.Dex));	break;
	case MobCon:			retval = ((Ptr) &(aff_abils.Con));	break;
	case MobCha:			retval = ((Ptr) &(aff_abils.Cha));	break;
	case MobKeywords:		retval = ((Ptr) &keywords);				break;
	case MobName:			retval = ((Ptr) &name);					break;
	case MobSDesc:			retval = ((Ptr) &shortDesc);				break;
	case MobLDesc:			retval = ((Ptr) &(general.longDesc));		break;
	}

	return retval;
}


const struct olc_fields * Character::Constraints(int arg)
{
	static const struct olc_fields constraints[] = 
	{
		{ "affflags",	EDT_UINT32,	Datatypes::Bitvector,
			0, 0, AffectBit::Group | AffectBit::Charm,	(const char **)affected_bits
	},
		{ "attack", EDT_SINT32, Datatypes::DiffValue,
			0, FIRST_COMBAT_MESSAGE, TYPE_GARROTE,	(const char **) spells
	},
		{ "bodytype", EDT_UINT8, Datatypes::Value,
			0, 0, 0, (const char **) bodytypes
	},
		{ "align", EDT_SINT32, Datatypes::Integer,
			0, -1000, 1000, NULL
	},
		{ "hitroll", EDT_SINT8, Datatypes::Integer,
			0, 0, 50, NULL
	},
		{ "damroll", EDT_SINT8, Datatypes::Integer,
			0, 0, 50, NULL
	},
		{ "disposition", EDT_SINT16, Datatypes::Integer,
			0, -1000, 1000, NULL
	},
		{ "level", EDT_SINT32, Datatypes::Integer,
			0, 0, MAX_LEVEL, 0
	},
		{ "gold", EDT_SINT32, Datatypes::Integer,
			0, 0, 2000000000, NULL
	},
		{ "maxriders", EDT_SINT8, Datatypes::Integer,
			0, 0, 9, NULL
	},
		{ "numdice", EDT_SINT16, Datatypes::Integer,
			0, 0, 30, NULL
	},
		{ "sizedice", EDT_SINT16, Datatypes::Integer,
			0, 0, 1000, NULL
	},
		{ "addhp", EDT_SINT16, Datatypes::Integer,
			0, 1, 30000, NULL
	},
		{ "dr", EDT_SINT8, Datatypes::Integer,
			0, 0, 50, NULL
	},
		{ "expbonus", EDT_SINT16, Datatypes::Integer,
			0, -32000, 32000, NULL
	},
		{ "totalpoints", EDT_SINT32, Datatypes::Integer,
			0, -500, 500000, NULL
	},
		{ "type", EDT_SINT8, Datatypes::Value,
			0, 0, 0, (const char **)mobile_types
	},
		{ "npcflags", EDT_UINT32,		Datatypes::Bitvector,
			0, 0, MOB_SPEC | MOB_ISNPC | MOB_DELETED | MOB_APPROVED, (const char **)action_bits
	},
		{ "position", EDT_SINT8, Datatypes::Value,
			0, 0, 0, (const char **) position_types
	},
		{ "defpos", EDT_SINT8, Datatypes::Value,
			0, 0, 0, (const char **) position_types
	},
		{ "race", EDT_SINT8, Datatypes::Value,
			0, 0, NUM_RACES - 1,	(const char **) race_types
	},
		{ "class", EDT_SINT8, Datatypes::Value,
			0, 0, CLASS_PSIONICIST,	(const char **) class_types
	},
		{ "sex", EDT_SINT8, Datatypes::Value,
			0, 0, 2, (const char **) genders
	},
		{ "str", EDT_SINT8, Datatypes::Integer,
			0, 1, 99, NULL
	},
		{ "int", EDT_SINT8, Datatypes::Integer,
			0, 1, 99, NULL
	},
		{ "wis", EDT_SINT8, Datatypes::Integer,
			0, 1, 99, NULL
	},
		{ "dex", EDT_SINT8, Datatypes::Integer,
			0, 1, 99, NULL
	},
		{ "con", EDT_SINT8, Datatypes::Integer,
			0, 1, 99, NULL
	},
		{ "cha", EDT_SINT8, Datatypes::Integer,
			0, 1, 99, NULL
	},
		{ "keywords", 0,	Datatypes::SString,
			EDIT_ONELINE, 0,	MAX_MOB_DESC,		NULL
	},
		{ "name", 0,	Datatypes::SString,
			EDIT_ONELINE, 0,	MAX_MOB_DESC,		NULL
	},
		{ "shortdesc", 0,	Datatypes::SString,
			EDIT_ONELINE, 0,	MAX_MOB_DESC,		NULL
	},
		{ "longdesc", 0,	Datatypes::SString,
			EDIT_APPEND, 0,	MAX_MOB_DESC,		NULL
	},
		{ "\n", 0, 0, 0, 0, 0, NULL }
	};

	if (arg >= 0 && arg < MAXVAR)
		return &(constraints[arg]);

	return NULL;
}


Ptr	Object::GetPointer(int arg)
{
	Ptr	retval = NULL;

	switch (arg) {
	case ObjKeywords:		return ((Ptr) &keywords);				break;
	case ObjName:			return ((Ptr) &name);					break;
	case ObjSDesc:			return ((Ptr) &shortDesc);				break;
	case ObjADesc:			return ((Ptr) &actionDesc);				break;
	case ObjExtraFlags:		return ((Ptr) &(extra));				break;
	case ObjWearFlags:		return ((Ptr) &(wear));			 break;
	case ObjAffFlags:		return ((Ptr) &(affectbits));  			 break;
	case ObjType:			return ((Ptr) &(type));				 break;
	case ObjWeight:			return ((Ptr) &(weight)); 			 break;
	case ObjCost:			return ((Ptr) &(cost));				 break;
	case ObjTimer:			return ((Ptr) &(timer));  			 break;
	case ObjMaterial:		return ((Ptr) &(material));				 break;
	case ObjPD:				return ((Ptr) &(passive_defense));			 break;
	case ObjDR:				return ((Ptr) &(damage_resistance));  			 break;
	case ObjHit:			return ((Ptr) &(max_hit));			 break;
	case ObjMaxHit:			return ((Ptr) &(hit));			 break;
	case ObjVal1:			return ((Ptr) &(value[0])); 			   break;
	case ObjVal2:			return ((Ptr) &(value[1])); 			   break;
	case ObjVal3:			return ((Ptr) &(value[2])); 			   break;
	case ObjVal4:			return ((Ptr) &(value[3])); 			   break;
	case ObjVal5:			return ((Ptr) &(value[4])); 			   break;
	case ObjVal6:			return ((Ptr) &(value[5])); 			   break;
	case ObjVal7:			return ((Ptr) &(value[6])); 			   break;
	case ObjVal8:			return ((Ptr) &(value[7])); 			   break;
	}

	return retval;
}


const struct olc_fields * Object::Constraints(int arg)
{
	static const struct olc_fields constraints[] = 
	{
		{ "keywords", 0,	Datatypes::SString,
			EDIT_APPEND, 0,	MAX_OBJ_DESC,		NULL
	},
		{ "name", 0,	Datatypes::SString,
			EDIT_APPEND, 0,	MAX_OBJ_DESC,		NULL
	},
		{ "shortdesc", 0,	Datatypes::SString,
			EDIT_APPEND, 0,	MAX_OBJ_DESC,		NULL
	},
	    { "actdesc", 0,	Datatypes::SString,
	      EDIT_APPEND, 0,	MAX_OBJ_DESC,		NULL
    },
	    { "extraflags", EDT_UINT32,	Datatypes::Bitvector,
	      0, 0, ITEM_APPROVED | ITEM_DELETED,	(const char **)extra_bits
    },
	    { "wearflags", EDT_UINT32,	Datatypes::Bitvector,
	      0, 0,	0,	(const char **)wear_bits
    },
	    { "affflags", EDT_UINT32,	Datatypes::Bitvector,
	      0, 0,	0,	(const char **)affected_bits
    },
	    { "type", EDT_SINT8, Datatypes::Value,
	      0, 0, 0, (const char **)item_types
    },
	    { "weight", EDT_SINT32, Datatypes::Integer,
	      0, 0, 32000, NULL
    },
	    { "cost", EDT_SINT32, Datatypes::Integer,
	      0, 0, 2000000000, NULL
    },
	    { "timer", EDT_UINT32, Datatypes::Integer,
	      0, 0, 2000000000, NULL
    },
	    { "material", EDT_SINT8, Datatypes::Value,
	      0, 0, 0, (const char **)material_types
    },
	    { "pd", EDT_SINT8, Datatypes::Integer,
	      0, 0, 120, NULL
    },
	    { "dr", EDT_SINT8, Datatypes::Integer,
	      0, 0, 120, NULL
    },
	    { "maxhit", EDT_SINT16, Datatypes::Integer,
	      0, 0, 32000, NULL
    },
	    { "hit", EDT_SINT16, Datatypes::Integer,
	      0, 0, 32000, NULL
    },
	    { "value1", EDT_SINT32, Datatypes::ObjValue, 0, 
	      0, 0,	NULL
    },
	    { "value2", EDT_SINT32, Datatypes::ObjValue, 0, 
	      0, 0,	NULL
    },
	    { "value3", EDT_SINT32, Datatypes::ObjValue, 0, 
	      0, 0,	NULL
    },
	    { "value4", EDT_SINT32, Datatypes::ObjValue, 0, 
	      0, 0,	NULL
    },
	    { "value5", EDT_SINT32, Datatypes::ObjValue, 0, 
	      0, 0,	NULL
	    },
	    { "value6", EDT_SINT32, Datatypes::ObjValue, 0, 
	      0, 0,	NULL
	    },
	    { "value7", EDT_SINT32, Datatypes::ObjValue, 0, 
	      0, 0,	NULL
	    },
	    { "value8", EDT_SINT32, Datatypes::ObjValue, 0, 
	      0, 0,	NULL
	    },
		{ "\n", 0, 0, 0, 0, 0, NULL }
	};

	if (arg >= 0 && arg < MAXVAR)
		return &(constraints[arg]);

	return NULL;
}


Ptr	Trigger::GetPointer(int arg)
{
	Ptr	retval = NULL;

	switch (arg) {
	case TrigName:			return ((Ptr) (&name));				break;
	case TrigArgument:		return ((Ptr) (&arglist));			break;
	case TrigMobTypes:		return ((Ptr) (&trigger_type_mob));	break;
	case TrigObjTypes:		return ((Ptr) (&trigger_type_obj));	break;
	case TrigWldTypes:		return ((Ptr) (&trigger_type_wld));	break;
	case TrigNarg:			return ((Ptr) (&narg));				break;
	case TrigNargBitv:		return ((Ptr) (&narg));				break;
	case TrigScript:		return ((Ptr) (&cmdlist));			break;
	}

	return retval;
}


const struct olc_fields * Trigger::Constraints(int arg)
{
	static const struct olc_fields constraints[] = 
	{
	    { "name", 0, Datatypes::SString,
	      0, 0, 40,  NULL
	    },
	    { "argument", 0, Datatypes::SString,
	      0, 0, 120,  NULL
	    },
	    { "mtypes", EDT_SINT32, Datatypes::Bitvector,
	      0, 0, 0, (const char **) mtrig_types
	    },
	    { "otypes", EDT_SINT32, Datatypes::Bitvector,
	      0, 0, 0, (const char **) otrig_types
	    },
	    { "wtypes", EDT_SINT32, Datatypes::Bitvector,
	      0, 0, 0, (const char **) wtrig_types
    },
	    { "numarg", EDT_SINT32, Datatypes::Integer,
      0,	0, 0, NULL
    },
	    { "numargbit", EDT_SINT32, Datatypes::Bitvector,
	      0, 0, 0, (const char **) ocmd_place_vector
	    },
		{ "\n", 0, 0, 0, 0, 0, NULL }
	};
	
	if (arg >= 0 && arg < MAXVAR)
		return &(constraints[arg]);

	return NULL;
}




Ptr	Room::GetPointer(int arg)
{
	Ptr	retval = NULL;

	switch (arg) {
	case RoomName:			return ((Ptr) (&name));			break;
	case RoomDesc:			return ((Ptr) (&description));	break;
//	case RoomFlagList:		return ((Ptr) (&flags));		break;
//	case RoomSector:		return ((Ptr) (&sector_type));	break;
	case RoomCoordX:		return ((Ptr) (&coordx));		break;
	case RoomCoordY:		return ((Ptr) (&coordy));		break;
	}

	return retval;
}


const struct olc_fields * Room::Constraints(int arg)
{
	static const struct olc_fields constraints[] = 
	{
	    { "name", 0, Datatypes::SString,
	      0, 0, 40,  NULL
	    },
	    { "description", 0, Datatypes::SString,
	      0, 0, MAX_STRING_LENGTH,  NULL
	    },
	    { "coordx", EDT_SINT32, Datatypes::Integer,
      0,	0, 0, NULL
    },
	    { "coordy", EDT_SINT32, Datatypes::Integer,
      0,	0, 0, NULL
    },
		{ "\n", 0, 0, 0, 0, 0, NULL }
	};

	if (arg >= 0 && arg < MAXVAR)
		return &(constraints[arg]);

	return NULL;
}

Ptr	RoomDirection::GetPointer(int arg)
{
	Ptr	retval = NULL;

	switch (arg) {
	case ExitNumber:		return ((Ptr) (&to_room)); 	break;
	case ExitDesc:			return ((Ptr) (&general_description)); 	break;
	case ExitKeyword:		return ((Ptr) (&keyword)); 	break;
	case ExitKey:			return ((Ptr) (&key)); 	break;
	case ExitFlags:			return ((Ptr) (&exit_info)); 	break;
	case ExitMaterial:		return ((Ptr) (&material)); 	break;
	case ExitDR:			return ((Ptr) (&dam_resist)); 	break;
	case ExitHP:			return ((Ptr) (&max_hit_points)); 	break;
	case ExitPick:			return ((Ptr) (&pick_modifier));		break;
	}

	return retval;
}


const struct olc_fields * RoomDirection::Constraints(int arg)
{
	static const struct olc_fields constraints[] = 
	{
	    { "toroom", EDT_SINT32, Datatypes::Integer,
	      0, 0, 0, NULL
	    },
	    { "description", 0, Datatypes::SString,
	      0, 0, MAX_STRING_LENGTH,  NULL
    },
	    { "keyword", 0, Datatypes::SString,
	      0, 0, 40,  NULL
    },
	    { "key", EDT_SINT32, Datatypes::Integer,
	      0, 0, 0, NULL
	    },
	    { "flags", EDT_SINT32, Datatypes::Bitvector,
	      0, 0, 0, NULL
    },
	    { "material", EDT_SINT8, Datatypes::Value,
	      0, 0, 0, (const char **)material_types
    },
	    { "damresist", EDT_UINT8, Datatypes::Integer,
	      0, 0, 0, NULL
    },
	    { "hitpoints", EDT_SINT32, Datatypes::Integer,
	      0, 0, 0, NULL
    },
	    { "pickmodifier", EDT_SINT8, Datatypes::Integer,
	      0, 0, 0, NULL
    },
		{ "\n", 0, 0, 0, 0, 0, NULL }
	};

	if (arg >= 0 && arg < MAXVAR)
		return &(constraints[arg]);

	return NULL;
}


void Editor::DispSpecProcMenu(Descriptor *d, GameData *thing)
{
	int i;
    struct spec_type *spec_table = NULL;

    switch (thing->DataType()) {
	case Datatypes::Character:
		spec_table = (struct spec_type *) spec_mob_table;
		break;
	case Datatypes::Object:
		spec_table = (struct spec_type *) spec_obj_table;
		break;
	case Datatypes::Room:
		spec_table = (struct spec_type *) spec_room_table;
		break;
	default:
		log("Illegal object type passed to DispSpecProcMenu!");
		return;
    }
	
	strcpy(buf, "Available special procedures are:\r\n");
	for (i = 0; spec_table[i].name; ++i)
		sprintf(buf + strlen(buf), "  %-20s%s", spec_table[i].name, ((i % 3) == 2) ? "\r\n" : "");

	if ((i % 3) != 0)	strcat(buf, "\r\n");

	d->Write(buf);
}


void Editor::ParseSpecProcMenu(Descriptor *d, char *argument, GameData *thing)
{
    struct spec_type *spec_table = NULL;
    int i;
    SpecProc proc;
    char **farg = NULL;
    void *ptr = NULL;

    argument = any_one_arg(argument, buf);

    if (!*buf) {
		d->Write("Usage:  special [<name>| none ] [<argument>]\r\n");
		DispSpecProcMenu(d, thing);
		return;
    }

    switch (thing->DataType()) {
	case Datatypes::Character:
		spec_table = (struct spec_type *) spec_mob_table;
		break;
	case Datatypes::Object:
		spec_table = (struct spec_type *) spec_obj_table;
		break;
	case Datatypes::Room:
		spec_table = (struct spec_type *) spec_room_table;
		break;
    }

	if (!str_cmp(buf, "none"))
		proc = NULL;

    else if ((proc = spec_lookup(buf, spec_table)) == NULL) {
		d->Write("Special procedure not found.\r\n");
		return;
    }
	
	func = proc;

    if (proc == NULL) {
		if (funcarg)	free(funcarg);
		d->Write("Special procedure set to none.\r\n");
    } else {
		d->Writef("Special procedure set to %s.\r\n", spec_name(proc, spec_table));
    }
}


// Edit object values generically; returns 0 for invalid parameter -DH
int olc_edit_objval(Descriptor *d, Object *obj, int &mode, char *argument, char *name)
{
	long n, i, min, max, *result = NULL;
	SInt8 objtype;
	SInt8 objval;
	const char **values;
	
	objtype = GET_OBJ_TYPE(obj);
	objval = mode - Object::ObjVal1;
	min = obj_values_tab[objtype][objval].min;
	max = obj_values_tab[objtype][objval].max;
	values = obj_values_tab[objtype][objval].values;
	result = (long *) &obj->value[objval];

	switch (obj_values_tab[objtype][objval].type) {
	case Datatypes::Integer:
		if (!olc_edit_integer(d, result, argument, name,
				 obj_values_tab[objtype][objval].min,
				 obj_values_tab[objtype][objval].max))
			return 0;
		break;
	case Datatypes::Bitvector:
		if (olc_edit_bitvector(d, (Flags *) result, argument, *((long *) result), name, values, 
		obj_values_tab[objtype][objval].max) != 2)
			return 0;
		break;
	case Datatypes::Value:
		if (!olc_edit_value(d, result, argument, name, min, max, values))
			return 0;
		break;
	case Datatypes::SpellValue:
		if (*argument == '0') {
			*result = -1;
			return 2;
		} else if (!olc_edit_value(d, result, argument, name, min, max, values))
			return 0;
		break;
	case Datatypes::DiffValue:
		if (*argument == '0') {
			*result = -1;
			return 2;
		} else if (!olc_edit_value(d, result, argument, name, min, max, values, TRUE))
			return 0;
		break;
	default:
		d->Write("Undefined case in olc_edit_objval!	Tell the imps!\r\n");
	}
		
	return 1;
}


int Editable::Edit(Descriptor *d, char *command, int menu, char *argument)
{
	long result;
	Ptr	ptr;
	int i, editmode = 0;
	StrLenInt len;
	Editor * tmpeditor = NULL;

	SString **ssptr;
	char **stringptr;
	Vector<std::AffectEditable>	*aevector;
	Trigger	**trig;

	const struct olc_fields *constraints = Constraints(menu);

	if (!constraints) {
		log("Invalid constraints pointer in Editable::Edit.  Tell the imps!");
		return 0;
	}

	if (*command) {
		for (i = 0; i < menu; ++i) {
			if (*(Constraints(i)->name) == '\n') {
				log("Invalid menu number in oasis_gen_edit.	Tell the imps!");
			return 0;
			}
			if (is_abbrev(command, Constraints(i)->name))
				break;
		}
		ptr = GetPointer(i);
	} else {
		ptr = GetPointer(menu);
		if (!ptr) {
			log("Invalid menu number in oasis_gen_edit.	Tell the imps!");
			return 0;
		}
		i = menu;
	}

	// In all cases, we have three parts of the address of the edited variable to resolve:
	// bptr: the beginning address of the edited object
	// displacement:  offset of the variable from the beginning of the structure it is a member of
	// substructure:  for substructures that are included as aggregate, we must afterwards add the offset
	// 				  substructure to get the complete address.

	// If a substructure is contained within a pointer, we have some different tricks to pull.

	/* parse arg and get value */

	skip_spaces(argument);

	switch (Constraints(i)->what) {
	case Datatypes::Integer:
		if (!olc_edit_integer(d, &result, argument, Constraints(i)->name,
				Constraints(i)->min, Constraints(i)->max))
			return 0;
		break;
	case Datatypes::Bool:
		(*(bool *) ptr) = (*(bool *) ptr ? false : true);
		return 1;
	case Datatypes::Bitvector:
		if (olc_edit_bitvector(d, (Flags *) &result, argument, *((Flags *) ptr),
					Constraints(i)->name, Constraints(i)->values, Constraints(i)->max) != 2)
			return 0;
		break;
	case Datatypes::Value:
		if (!olc_edit_value(d, &result, argument, Constraints(i)->name, Constraints(i)->min, Constraints(i)->max, Constraints(i)->values))
			return 0;
		break;
	case Datatypes::SpellValue:
		if (!olc_edit_value(d, &result, argument, Constraints(i)->name, Constraints(i)->min, Constraints(i)->max, Constraints(i)->values))
			return 0;
		break;
	case Datatypes::DiffValue:
		if (!olc_edit_value(d, &result, argument, Constraints(i)->name, Constraints(i)->min, Constraints(i)->max, Constraints(i)->values, TRUE))
			return 0;
		break;
	case Datatypes::ObjValue:
		return (olc_edit_objval(d, (Object *) this, menu, argument, Constraints(i)->name));
	case Datatypes::CStringLine:
		stringptr = (char **) ptr;
		if (*stringptr)
			delete (*stringptr);
		if (*argument) {
			*stringptr = strdup(argument);
		} else {
			*stringptr = NULL;
			d->Writef("Enter %s: ", Constraints(i)->name);
		}
		return 0;
		break;
	case Datatypes::CString:
		tmpeditor = new TextEditor(d, (char **) &ptr, (StrLenInt) Constraints(i)->max);
		d->Edit(tmpeditor);

		return 1;
		break;
	case Datatypes::SString:
		// Abstract shared string editors should edit ->str, and not share or
		// create new strings.

		ssptr = (SString **) ptr;
		
		// CHANGEPOINT:  This is plugging the hole of a design flaw here.  Somehow shared strings 
		// aren't being handled altogether correctly.  I think a StringEditor class with descendants
		// for editing things like this properly is the key. -DH
// 		if (*ssptr == NULL)	{
// 			*ssptr = SString::Create("");
// 			CREATE((*ssptr)->str, char, Constraints(i)->max);
// 		}	// End kludge

		d->Writef("Enter %s: (/s saves /h for help)\r\n\r\n", Constraints(i)->name);
		if (ssptr && (*ssptr)->str) {
			d->Write((*ssptr)->str);
		}
		
		tmpeditor = new SStringEditor(d, ssptr, (StrLenInt) Constraints(i)->max);
		d->Edit(tmpeditor);

 		return 1;
	case (Datatypes::Pointer + Datatypes::Trigger):
		trig = (Trigger **) ptr;
		
		d->Writef("Enter %s: (/s saves /h for help)\r\n\r\n", Constraints(i)->name);
		
		tmpeditor = new CmdlistEditor(d, &((*trig)->cmdlist), (StrLenInt) Constraints(i)->max);
		d->Edit(tmpeditor);

 		return 1;
	case (Datatypes::AffectEditable + Datatypes::Vector):
		aevector = (Vector<AffectEditable> *) ptr;
		if (!aevector) {
			return 0;
		}
		tmpeditor = new AffectVectorEditor(d, aevector);
		d->Edit(tmpeditor);
		tmpeditor->Menu();

 		return 1;
	default:
		strcpy(buf, "SYSERR:  Invalid editor pointer in Editable::Edit, warn the imps!");
		d->Write(buf);
		log(buf);
		return 0;
	}

	/* assign value */
	switch (Constraints(i)->type) {
	case EDT_SINT8:	*(SInt8 *) ptr = (SInt8) result;		break;
	case EDT_UINT8: *(UInt8 *) ptr = (UInt8) result;		break;
	case EDT_SINT16: *(SInt16 *) ptr = (SInt16) result;		break;
	case EDT_UINT16: *(UInt16 *) ptr = (UInt16) result;		break;
	case EDT_SINT32: *(SInt32 *) ptr = (SInt32) result;		break;
	case EDT_UINT32: *(UInt32 *) ptr = (UInt32) result;		break;
	case EDT_VNUM: *(VNum *) ptr = (VNum) result;			break;
	case EDT_IDNUM: *(IDNum *) ptr = (IDNum) result;		break;
	default:											break;
	}

	return 1;
}


/* Ray's dig code for Oasis OLC, added by Daniel Rollings AKA Daniel Houghton */
// Boy has this gone through a lot of rewrites. -DH
ACMD(do_rdig)
{
	/* Only works if you have Oasis OLC */
	extern const int rev_dir[];

	extern const char *dirs[];
	char *extra, *p;
	int toroom = 0, rroom = 0, inzone = -1, tozone = -1, dir = 0, n;
	Flags doorflags = 0;
	/*	Room *room; */

	if (ch->desc == NULL) {
		ch->Send("This command is for carbon-based life forms only.\r\n");
		return;
	}

	extra = two_arguments(argument, buf2, buf3);
	/* buf2 is the direction, buf3 is the room */

	if ((!*buf2) || (!*buf3) || (!isdigit(*buf3))) {
		ch->Send("Format: rdig <dir> <room number>\r\n");
		return;
	}

	/* Main stuff */
	switch (LOWER(*buf2)) {
	case 'n':	dir = NORTH;	break;
	case 'e':	dir = EAST;		break;
	case 's':	dir = SOUTH;	break;
	case 'w':	dir = WEST;		break;
	case 'u':	dir = UP;		break;
	case 'd':	dir = DOWN;		break;
	default:	ch->Send("What direction is that?!\r\n");	return;
	}

	toroom = atoi(buf3);
	inzone = find_owner_zone(IN_ROOM(ch));
	tozone = find_owner_zone(toroom);

	if (tozone == -1) {
		ch->Send("There is no zone for that number!\r\n");
		return;
	}

	if (GET_TRUST(ch) < TRUST_SUBIMPL) {
		if (!get_zone_perms(ch, inzone)) {
			return;
		}
	}
	
	half_chop(extra, buf2, buf3);
	while (*buf2) {
		p = buf2;
		while (p && *p) {
			*p = UPPER(*p);
			++p;
		}
		if ((n = search_block(buf2, exit_bits, FALSE)) >= 0)
			SET_BIT(doorflags, 1 << n);
		half_chop(buf3, buf2, buf3);
	}

	// Make the door from the present room
	world[IN_ROOM(ch)].MakeDirection(dir, toroom, doorflags);
	olc_add_to_save_list(zone_table[inzone].number, OLC_REDIT);

	// Make the other room, if necessary, and possibly a corresponding door.
	if (Room::Find(toroom)) {
		if ((GET_TRUST(ch) < TRUST_IMPL) && 
			 (ROOM_FLAGGED(rroom, ROOM_PRIVATE | ROOM_GODROOM))) {
			ch->Send("You aren't godly enough to make an exit to that room.\r\n");
			return;
		}

		if ((!world[toroom].Direction(rev_dir[dir])) && 
			 (get_zone_perms(ch, tozone))) {

			// Make the door from the other room leading back.
			world[toroom].MakeDirection(rev_dir[dir], IN_ROOM(ch), doorflags);
			olc_add_to_save_list(zone_table[tozone].number, OLC_REDIT);
		} else {
			ch->Send("No reverse exit created.\r\n");
		}

	} else {
		if (!get_zone_perms(ch, tozone)) { 
			return;
		}
		REdit * tmpeditor = new REdit(ch->desc, toroom, false);
		ch->desc->Edit(tmpeditor);
		tmpeditor->SaveInternally();
		tmpeditor->Finish(Editor::All);

		world[toroom].MakeDirection(rev_dir[dir], IN_ROOM(ch), doorflags);
	}

	ch->Sendf("You make an exit %s from room %d to room %d.\r\n", dirs[dir], IN_ROOM(ch), toroom);
}


// Takes a zone rnum and an OLC state as arguments, and saves the file.
void gen_olc_save(int savenum, int subcmd)
{
	// void *ptr;
	int i, zone, rec = 0, bot, top;;
	FILE *fl;
	char filename[256], buf[256];
	Character *mob;
	Object *obj;
	Trigger *trig;

	const char *suffix[] = {
		"ERROR",
		"wld",
		"obj",
		"zon",
		"mob",
		"shp",
		"act",
		"hlp",
		"trg",
		"cln",
		"ERROR"
	};
	const char *prefix[] = {
		"ERROR",
		WLD_PREFIX,
		OBJ_PREFIX,
		ZON_PREFIX,
		MOB_PREFIX,
		SHP_PREFIX,
		MSC_PREFIX,
		HLP_PREFIX,
		TRG_PREFIX,
		CLAN_PREFIX,
		"ERROR"
	};

	zone = real_zone(savenum);

	sprintf(filename, "%s/tmp%d.%s", prefix[subcmd],
		zone_table[zone].number, suffix[subcmd]);

	log("OLC:	Writing data for zone v%d, r%d.", zone_table[zone].number, zone);

	if ((fl = fopen(filename, "w")) == NULL) {
		sprintf(buf, "OLC:	Unable to open %s for writing.", filename);
		log(buf);
		return;
	}

	/* write the file */
	switch (subcmd) {
	case OLC_MEDIT:
		bot = zone ? zone_table[zone - 1].top + 1 : 0;
		top = zone_table[zone].top;
		for (i = bot; i <= top; ++i) {
			if (Character::Find(i)) {
				mob = Character::Index[i].proto;
				if (MOB_FLAGGED(mob, MOB_DELETED))
					continue;
				Character::SaveDisk(mob, fl);
				++rec;
			}
		}
		fprintf(fl, "$\n");
		break;

	case OLC_OEDIT:
		bot = zone ? zone_table[zone - 1].top + 1 : 0;
		top = zone_table[zone].top;
		for (i = bot; i <= top; ++i) {
			if (Object::Find(i)) {
				obj = Object::Index[i].proto;
				if (OBJ_FLAGGED(obj, ITEM_DELETED))
					continue;
				Object::SaveDisk(obj, fl);
				++rec;
			}
		}
		fprintf(fl, "$\n");
		break;

	case OLC_REDIT:
		bot = zone ? zone_table[zone - 1].top + 1 : 0;
		top = zone_table[zone].top;
		for (i = bot; i <= top; ++i) {
			if (Room::Find(i)) {
				if (ROOM_FLAGGED(i, ROOM_DELETED))	
					continue;
				Room &room = world[i];
				Room::SaveDisk(&room, fl);
				++rec;
			}
		}
		fprintf(fl, "$\n");
		break;

	case OLC_ZEDIT:
		fprint_zone(fl, zone);
		rec = 1;
		break;

	case OLC_SEDIT:
		bot = zone ? zone_table[zone - 1].top + 1 : 0;
		top = zone_table[zone].top;
		for (i = bot; i <= top; ++i) {
			if (Shop::Index.Find(i)) {
				Shop &shop = Shop::Index[i];
				Shop::SaveDisk(shop, fl);
				++rec;
			}
		}
		fprintf(fl, "$\n");
		break;

	case OLC_SCRIPTEDIT:
		bot = zone ? zone_table[zone - 1].top + 1 : 0;
		top = zone_table[zone].top;

		for (i = bot; i <= top; ++i) {
			if (!Trigger::Find(i))	continue;
			trig = Trigger::Index[i].proto;
			if (IS_SET(GET_TRIG_TYPE_WLD(trig), TRIG_DELETED))
				continue;

			fprintf(fl, "#%d = {\n", i);
			fprintstring(fl, "\tname = %s;\n", SSData(trig->name) ? SSData(trig->name) : "unknown trigger");

			if (SSData(trig->arglist))	fprintstring(fl, "\targ = %s;\n", SSData(trig->arglist));
			if (trig->narg)			fprintf(fl, "\tnarg = %d;\n", trig->narg);

			if (trig->trigger_type_mob) fprintbits(fl, "\tmobtriggers = { %s };\n", trig->trigger_type_mob, mtrig_types);
			if (trig->trigger_type_obj) fprintbits(fl, "\tobjtriggers = { %s };\n", trig->trigger_type_obj, otrig_types);
			if (trig->trigger_type_wld) fprintbits(fl, "\twldtriggers = { %s };\n", trig->trigger_type_wld, wtrig_types);
	
			// Build the text for the script
			*buf2 = '\0';
			CmdlistElement *cmd = trig->cmdlist->command; 
			while (cmd) {
				if (cmd->cmd)
					strcat(buf2, cmd->cmd);
				strcat(buf2, "\\n");
				cmd = cmd->next;
				if (cmd)	strcat(buf2, "\n");
			}
			
			fprintf(fl, "\tcommands = \"%s\";\n", *buf2 ? buf2 : "* Empty script\n");

			// fprintstring(fl, "\tcommands = %s;\n", *buf ? buf : "* Empty script\n");
			fprintf(fl, "};\n");
			++rec;

		}
		fprintf(fl, "$\n");
		break;
	}

	if (fclose(fl) == EOF)
		perror("fclose");

	if (!rec)	sprintf(buf, "OLC:	No %ss found to write!", suffix[subcmd]);
	else		sprintf(buf, "OLC:	%d %s%s written to file %s.", rec, suffix[subcmd], (rec > 1) ? "s" : "", filename);

	log(buf);

	sprintf(buf2, "mv %s ", filename);
	sprintf(filename, "%s/%d.%s", prefix[subcmd], zone_table[zone].number, suffix[subcmd]);
	strcat(buf2, filename);
		
	if (system(buf2) == -1)		mudlogf(NRM, NULL, TRUE, "Error on port open for OLC write!");
		
	add_to_index(zone_table[zone].number, suffix[subcmd]);

	olc_remove_from_save_list(zone_table[zone].number, subcmd);
}


/*-------------------------------------------------------------------*/
/*. Save all the zone_table for this zone to disk.  Yes, this could
    automatically comment what it saves out, but why bother when you
    have an OLC as cool as this ? :> .*/
#undef ZCMD
#define ZCMD		(zone_table[zone_num].cmd[subcmd])

void fprint_zone(FILE *zfile, SInt32 zone_num)
{
	int subcmd, arg1 = -1, arg2 = -1, arg3 = -1, arg4 = -1;
	char	*buf, *fname = get_buffer(64), *bldr;
	const char *comment;

	//	Print zone header to file
	fprintf(zfile, 
			"#%d\n"
			"%s~\n"
			"%d %d %d\n",
			zone_table[zone_num].number,
			zone_table[zone_num].name ? zone_table[zone_num].name : "undefined",
			zone_table[zone_num].top, zone_table[zone_num].lifespan, zone_table[zone_num].reset_mode
	);
	
	fprintf(zfile, "%d %d %d\n",
			zone_table[zone_num].climate.pattern,
			zone_table[zone_num].climate.flags,
			zone_table[zone_num].climate.energy);
	
	for (int i = 0; i < Weather::MaxSeasons; i++)
		fprintf(zfile, "%d ", zone_table[zone_num].climate.wind[i]);
	fprintf(zfile, "\n");
	for (int i = 0; i < Weather::MaxSeasons; i++)
		fprintf(zfile, "%d ", zone_table[zone_num].climate.windDirection[i]);
	fprintf(zfile, "\n");
	for (int i = 0; i < Weather::MaxSeasons; i++)
		fprintf(zfile, "%d ", zone_table[zone_num].climate.windVariance[i]);
	fprintf(zfile, "\n");
	for (int i = 0; i < Weather::MaxSeasons; i++)
		fprintf(zfile, "%d ", zone_table[zone_num].climate.precipitation[i]);
	fprintf(zfile, "\n");
	for (int i = 0; i < Weather::MaxSeasons; i++)
		fprintf(zfile, "%d ", zone_table[zone_num].climate.temperature[i]);
	fprintf(zfile, "\n");

#ifdef GOT_RID_OF_IT
	fprintf(zfile,
			"5 0 0\n"
			"0 0 0 0\n"
			"0 0 0 0\n"
			"0 0 0 0\n"
			"4 4 4 4\n"
			"5 5 5 5\n");
#endif
	
#if defined(ZEDIT_HELP_IN_FILE)
	fprintf(zfile,
		"* Field #1		Field #3	 Field #4	Field #5	 FIELD #6\n"
		"* M (Mobile)	Mob-Vnum	 Wld-Max	 Room-Vnum\n"
		"* O (Object)	Obj-Vnum	 Wld-Max	 Room-Vnum\n"
		"* G (Give)		Obj-Vnum	 Wld-Max	 Unused\n"
		"* E (Equip)	 Obj-Vnum	 Wld-Max	 EQ-Position\n"
		"* P (Put)		 Obj-Vnum	 Wld-Max	 Target-Obj-Vnum\n"
		"* D (Door)		Room-Vnum	Door-Dir	Door-State\n"
		"* R (Remove)	Room-Vnum	Obj-Vnum	Unused\n"
		"* C (Command) String...\n"
		"* Z (Maze)		Unused		 Unused		Unused\n"
		"* T (Trigger) Trig Type	Trg-VNum	Wld-Max		[Room-VNum]\n"
	);
#endif
	
	LListIterator<char *>	iter(zone_table[zone_num].builders);
	while ((bldr = iter.Next()))
		if (bldr && (*bldr != '*'))
			fprintf(zfile, "* Builder %s\n", bldr);

	for(subcmd = 0; ZCMD.command != 'S'; subcmd++) {
//		arg1 = arg2 = arg3 = arg4 = -1;
		arg1 = ZCMD.arg1;
		arg2 = ZCMD.arg2;
		arg3 = ZCMD.arg3;
		arg4 = -1;
		switch (ZCMD.command) {
			case 'M':
//				arg1 = ZCMD.arg1;
//				arg2 = ZCMD.arg2;
//				arg3 = ZCMD.arg3;
				comment = Character::Index[ZCMD.arg1].proto->Name();
				break;
			case 'O':
//				arg1 = ZCMD.arg1;
//				arg2 = ZCMD.arg2;
//				arg3 = ZCMD.arg3;
				comment = Object::Index[ZCMD.arg1].proto->Name();
				break;
			case 'G':
//			 	arg1 = ZCMD.arg1;
//				arg2 = ZCMD.arg2;
				arg3 = -1;
				comment = Object::Index[ZCMD.arg1].proto->Name();
				break;
			case 'E':
//				arg1 = ZCMD.arg1;
//				arg2 = ZCMD.arg2;
//				arg3 = ZCMD.arg3;
				comment = Object::Index[ZCMD.arg1].proto->Name();
				break;
			case 'P':
//				arg1 = ZCMD.arg1;
//				arg2 = ZCMD.arg2;
//				arg3 = ZCMD.arg3;
				comment = Object::Index[ZCMD.arg1].proto->Name();
				break;
			case 'D':
//				arg1 = ZCMD.arg1;
//				arg2 = ZCMD.arg2;
//				arg3 = ZCMD.arg3;
				comment = world[ZCMD.arg1].Name("<ERROR>");
				break;
			case 'R':
//				arg1 = ZCMD.arg1;
//				arg2 = ZCMD.arg2;
				arg3 = -1;
				comment = Object::Index[ZCMD.arg2].proto->Name();
				break;
			case 'Z':
//				arg1 = ZCMD.arg1;
//				arg2 = ZCMD.arg2;
//				arg3 = ZCMD.arg3;
				comment = "Automagically Generated Maze.";
				break;
			case 'T':
//				arg1 = ZCMD.arg1;
//				arg2 = ZCMD.arg2;
//				arg3 = ZCMD.arg3;
				arg4 = (ZCMD.arg1 == WLD_TRIGGER) ? ZCMD.arg4 : 0;
				comment = SSData(Trigger::Index[ZCMD.arg2].proto->name);
				break;
			case 'C':
				break;
			case '*':
				//	Invalid commands are replaced with '*' - Ignore them
				continue;
			default:
				mudlogf(BRF, NULL, true, "SYSERR: z_save_to_disk(): Unknown cmd '%c' - NOT saving", ZCMD.command);
				continue;
		}
#if defined(ZEDIT_HELP_IN_FILE)
		switch(ZCMD.command) {
//			case 'G':
//			case 'R':
//				fprintf(zfile, "%c %d %d %d %d \t(%s)\n", ZCMD.command, ZCMD.if_flag, arg1, arg2, ZCMD.repeat, comment);
//				break;
//			case 'O':
//			case 'P':
//				fprintf(zfile, "%c %d %d %d %d %d \t(%s)\n", ZCMD.command, ZCMD.if_flag, arg1, arg2, arg3, ZCMD.repeat, comment);
//				break;
			case 'C':
				fprintf(zfile, "%c %d %s\n", ZCMD.command, ZCMD.if_flag, ZCMD.command_string);
				break;
//			case 'D':
//			case 'E':
//			case 'M':
//			case 'T':
//			case 'Z':
			default:
				fprintf(zfile, "%c %d %d %d %d %d %d \t(%s)\n", ZCMD.command, ZCMD.if_flag, arg1, arg2, arg3, arg4, ZCMD.repeat, comment);
				break;				
		}
#else
		switch(ZCMD.command) {
//			case 'G':
//			case 'R':
//				fprintf(zfile, "%c %d %d %d %d\n", ZCMD.command, ZCMD.if_flag, arg1, arg2, ZCMD.repeat);
//				break;
//			case 'O':
//			case 'P':
//				fprintf(zfile, "%c %d %d %d %d %d\n", ZCMD.command, ZCMD.if_flag, arg1, arg2, arg3, ZCMD.repeat);
//				break;
			case 'C':
				fprintf(zfile, "%c %d %s\n", ZCMD.command, ZCMD.if_flag, ZCMD.command_string);
				break;
//			case 'D':
//			case 'E':
//			case 'M':
//			case 'T':
//			case 'Z':
			default:
				fprintf(zfile, "%c %d %d %d %d %d %d\n", ZCMD.command, ZCMD.if_flag, arg1, arg2, arg3, arg4, ZCMD.repeat);
				break;				
		}
#endif
	}
	
	release_buffer(fname);
	fprintf(zfile, "S\n$\n");
}


//	Display scripts attached
void Editor::DispScriptsMenu(Descriptor *d, Vector<VNum> &triggers) 
{
	d->Write("-- Scripts Attachments:\r\n");

	for (int i = 0; i < triggers.Count(); ++i) {
		d->Writef("%2d) [&cC%d&c0] &cC%s&c0\r\n", i + 1, triggers[i], Trigger::Find(triggers[i]) ?
				(SSData(Trigger::Index[triggers[i]].proto->name) ?
				SSData(Trigger::Index[triggers[i]].proto->name) : "(Unnamed)") :
				"(Uncreated)");
	}

	if (!triggers.Count()) d->Write("No scripts attached.\r\n");

	d->Write("\r\n"
			"&cGA&c0) Attach script\r\n"
			"&cGD&c0) Detach script\r\n"
			"&cGQ&c0) Exit menu\r\n"
			"Enter choice:  ");

	mode = TriggersMenu;
}


void Editor::ParseScriptsMenu(Descriptor *d, char *arg, Vector<VNum> &triggers) 
{
	int i;

	switch (mode) {
	case TriggersMenu:
		switch (*arg) {
			case 'a':
			case 'A':
				d->Write("Enter the VNUM of script to attach: ");
				mode = TriggerAdd;
				return;
			case 'd':
			case 'D':
				d->Write("Enter script in list to detach: ");
				mode = TriggerPurge;
				return;
			case 'q':
			case 'Q':
				Menu();
				return;
			default:
				DispScriptsMenu(d, triggers);
				return;
		}
		break;

	case TriggerAdd:
		i = atoi(arg);
		if (Trigger::Find(i)) {
			triggers.Append(i);
			d->Write("Script attached.\r\n");
			if (!save)	save = OLC_SAVE_YES;
		} else
			d->Write("No such script.\r\n");
		DispScriptsMenu(d, triggers);
		return;

	case TriggerPurge:
		i = atoi(arg);
		if (i > 0 && i <= triggers.Count()) {
			triggers.Remove(i - 1);
			if (!save)	save = OLC_SAVE_YES;
			d->Write("Script detached.\r\n");
		} else
			d->Write("Invalid number.\r\n");
		DispScriptsMenu(d, triggers);
		return;
	}
}


void add_to_index(SInt32 znum, const char *type) 
{
	char *old_fname, *new_fname, line[256];
	FILE *old_file, *new_file;
	char *prefix, entry[16];
	int fdsign = FALSE, wrote = FALSE;
	int last = -1, iline;

	switch(*type) {
		case 'z':		prefix = ZON_PREFIX;		break;
		case 'w':		prefix = WLD_PREFIX;		break;
		case 'o':		prefix = OBJ_PREFIX;		break;
		case 'm':		prefix = MOB_PREFIX;		break;
		case 's':		prefix = SHP_PREFIX;		break;
		case 't':		prefix = TRG_PREFIX;		break;
		default:		/*. Caller messed up .*/	return;
	}

	sprintf(entry, "%i.%s", znum, type);
	sprintf(buf1, "%s/index", prefix);
	old_fname = str_dup(buf1);
	sprintf(buf2, "%s/index.temp", prefix);
	new_fname = str_dup(buf2);

	if (!(old_file = fopen(old_fname, "r"))) {
		sprintf(buf1, "SYSERR: Error opening old index file '%s' for index update", buf2);
		perror(buf1);
		return;
	}
	if (!(new_file = fopen(new_fname, "w"))) {
		fclose(old_file);
		sprintf(buf1, "SYSERR: Error opening new index file '%s' for index update", buf2);
		perror(buf1);
		return;
	}

/* Daniel Rollings AKA Daniel Houghton's code change:	Add check for existing zones numbers here */
	while ((get_line(old_file, line)) && (*buf1 != '$')) {
		if (znum == atoi(line)) {
			fclose(old_file);
			fclose(new_file);
			return;
		}
	}
	rewind(old_file);

/* File open routine was originally here */

	do {
		if(!get_line(old_file, line) && !wrote) {
			fclose(old_file);
			fclose(new_file);
			return;
		}
		if(*line == '$') {
			fdsign = TRUE;
			if (!wrote) {

				if(fprintf(new_file, "%s\n", entry) < 0) {
					break;
				}

				wrote = TRUE;
			}
			fprintf(new_file, "$\n");
			break;
		}

		iline = atoi(line);

		if ((last < znum) && (znum < iline)) {
			fprintf(new_file, "%s\n%s\n", entry, line);
			wrote = TRUE;
		} else fprintf(new_file, "%s\n", line);

		last = iline;
	} while(!fdsign && !feof(old_file));

	fclose(old_file);
	fclose(new_file);
	remove(old_fname);
	rename(new_fname, old_fname);
	return;
}


MenuEditor::MenuEditor(Descriptor *d, SInt16 targ) :
	Editor(d)
{
	number = targ;
}


MenuEditor::~MenuEditor(void)
{
}


Ptr MenuEditor::Pointer(void)
{
	return static_cast<Ptr>(Source());
}


void MenuEditor::Finish(FinishMode mode)
{
	if (d->character) {
		STATE(d) = CON_PLAYING;
		act("$n stops using OLC.", TRUE, d->character, 0, 0, TO_ROOM);
	}
	Editor::Finish();
}


void SkillEditor::Menu(void)
{
	d->Writef("-- Number:  [&cC%d&c0]\r\n", number);

	GenericMenu(this);
	d->Write("&cG A&c0) Anrequisites\r\n"
				"&cG D&c0) Defaults\r\n"
				"&cG P&c0) Prerequisites\r\n"
				"&cG R&c0) Properties\r\n"
				"\r\n&cG Q&c0) Quit\r\n"
				"&c0Input: ");
}


void SkillEditor::Parse(char *arg)
{
	Editor *ed = NULL;
	switch (mode) {
	case MainMenu:
		switch (*arg) {
		case 'a':	
		case 'A':	ed = new SkillRefEditor(d, &(editing->anrequisites));
					d->Edit(ed);
					ed->Menu();
					return;
		case 'd':	
		case 'D':	ed = new SkillRefEditor(d, &(editing->defaults));
					d->Edit(ed);
					ed->Menu();
					return;
		case 'p':	
		case 'P':	ed = new SkillRefEditor(d, &(editing->prerequisites));
					d->Edit(ed);
					ed->Menu();
					return;
		case 'r':	
		case 'R':	ed = new PropertyListEditor(d, &(editing->properties), 
						Property::Type::Saving, Property::Type::Manual);
					d->Edit(ed);
					ed->Menu();
					return;
		default:	break;
		}
	default:
		break;
	}

	GenericParse(this, arg);
}


SkillEditor::SkillEditor(Descriptor *d, SInt16 targ, Skill *edited) :
	MenuEditor(d, targ), ptr(edited)
{
	editing = new Skill(*edited);
	Menu();
}


SkillEditor::~SkillEditor(void)
{
	if (editing)
		delete (editing);
}


void SkillEditor::SaveInternally(void)
{
	if (Skills[number])
		delete Skills[number];
	Skills[number] = new Skill(*editing);
	delete editing;
	editing = NULL;

	olc_add_to_save_list(SKILLEDIT_PERMISSION, OLC_SKILLEDIT);
}


Editable	* SkillEditor::Editing(void)
{
	return (Editable *) editing;
}


Editable	* SkillEditor::Source(void)
{
	return (Editable *) ptr;
}


SkillRefEditor::SkillRefEditor(Descriptor *d, Vector<SkillRef> *orig) : Editor(d), editing(orig)
{
}


SkillRefEditor::~SkillRefEditor(void)
{
}


void SkillRefEditor::SaveInternally(void)
{
}


void SkillRefEditor::Menu(void)
{
	int i, skillnum = -1, chance = 0;
	char *buf = get_buffer(MAX_STRING_LENGTH);

	switch (mode) {
	case MainMenu:
		for (i = 0; i < editing->Count(); ++i) {
			skillnum = (* editing)[i].skillnum;
			chance = (* editing)[i].chance;
			sprintf(buf + strlen(buf),
				"&cG%2d&c0) %-25s &cC%d &c0points\r\n", i + 1, ::Skill::Name(skillnum), chance);
		}
	
		strcat(buf, "\r\n&cG A&c0) Add a skill reference\r\n"
					"&cG D&c0) Delete a skill reference\r\n"
					"&cG Q&c0) Quit\r\n");
		d->Write(buf);
		break;
	case InputSkill:
		d->Write("Enter skill name:  ");
		break;
    case InputPoints:
		d->Write("Enter skill points:  ");
		break;
	case DeleteSkill:
		d->Write("Input number of skill reference to delete:  ");
		break;
	}

	release_buffer(buf);
}


void SkillRefEditor::Parse(char *arg)
{
	int i, skillnum = -1, chance = 0, select;
	char *buf = get_buffer(MAX_STRING_LENGTH);


	switch (mode) {
	case MainMenu:
		switch (*arg) {
		case 'q':
		case 'Q':
		case 0:
			release_buffer(buf);
			Editor::Finish();
			return;
		case 'a':
		case 'A':
			value = editing->Count();
			editing->SetSize(editing->Count() + 1);
			d->Write("Enter skill name:  ");
			mode = InputSkill;
			break;
		case 'd':
		case 'D':
			d->Write("Select number to delete:  ");
			mode = DeleteSkill;
			break;
		default:
			if (!isdigit(*arg)) {
				release_buffer(buf);
				Menu();
				return;
			}
			select = atoi(arg);
			if (select)  {
				value = MIN(select - 1, editing->Count());
				if (value == editing->Count()) {
					editing->SetSize(editing->Count() + 1);
				}
				mode = InputSkill;
				Menu();
			}
		}
		break;
	case InputSkill:
		select = Skill::Number(arg, FIRST_STAT, LAST_STAT);
		if (select <= 0) {
			select = Skill::Number(arg);
		}
		if (select > 0) {
			(* editing)[value].skillnum = select;
			d->Write("\r\nEnter skill points:  ");
			mode = InputPoints;
			if (!(save))	save = OLC_SAVE_YES;
		} else {
			d->Write("\r\nInvalid skill, enter skill name:  ");
		}
		break;
    case InputPoints:
		select = atoi(arg);
		(* editing)[value].chance = select;
		mode = MainMenu;
		if (!(save))	save = OLC_SAVE_YES;
		Menu();
		break;
	case DeleteSkill:
		if (isdigit(*arg)) {
			select = MIN(atoi(arg) - 1, editing->Count() - 1);
			editing->Remove(select);
		}
		mode = MainMenu;
		if (!(save))	save = OLC_SAVE_YES;
		Menu();
		break;
	}

	release_buffer(buf);
}


Ptr	SkillRefEditor::Pointer(void)
{
	return NULL;
}


void SkillRefEditor::Finish(FinishMode mode)
{
	Editor::Finish();
}


Ptr	SkillRefEditor::GetPointer(int arg)
{
	return NULL;
}


const struct olc_fields *	SkillRefEditor::Constraints(int arg)
{
	return NULL;
}


AffectVectorEditor::AffectVectorEditor(Descriptor *d, Vector<AffectEditable> * edit) :
	Editor(d), editing(edit)
{
}


AffectVectorEditor::~AffectVectorEditor(void)
{
}


void AffectVectorEditor::SaveInternally(void)
{
}


void AffectVectorEditor::Menu(void)
{
	int i, j;
	AffectEditable *ae;
	char *buf = get_buffer(MAX_STRING_LENGTH);
	char *buf2;

	switch (mode) {
	case MainMenu:
		if (editing) {
			buf2 = get_buffer(MAX_STRING_LENGTH);
			for (i = 0; i < editing->Count(); ++i) {
				ae = &((*editing)[i]);
				sprintbit(ae->flags, affected_bits, buf2);
				sprintf(buf + strlen(buf), "&cG%2d&c0) %s %d %d   { %s }\r\n", i + 1, 
						apply_types[ae->location], ae->modifier, ae->duration, buf2);
			}
			release_buffer(buf2);
		}
		
		strcat(buf, "\r\n&cG A&c0) Add an affect\r\n"
					"&cG D&c0) Delete an affect\r\n"
					"&cG Q&c0) Quit\r\n");
		d->Write(buf);
		break;
	case Delete:
		d->Write("Input number of affect to delete:  ");
		break;
	}

	release_buffer(buf);
}


void AffectVectorEditor::Parse(char *arg)
{
	int i, j, select;
	char *buf = get_buffer(MAX_STRING_LENGTH);
	AffectEditable tmpae;

	switch (mode) {
	case MainMenu:
		if (isdigit(*arg)) {
			select = MAX(0, atoi(arg));
			if (!select || select > editing->Count()) {
				release_buffer(buf);
				Editor::Finish();
				return;
			}
			--select;
			MenuEditor *ed = new GenericEditor(d, &((*editing)[i]));
			d->Edit(ed);
			ed->Menu();

			release_buffer(buf);
			return;
		} else {
			switch (*arg) {
			case 'q':
			case 'Q':
				release_buffer(buf);
				Editor::Finish();
				return;
			case 'a':
			case 'A':
				if (!save)	save = OLC_SAVE_YES;
				editing->Append(tmpae);
				Menu();
				break;
			case 'd':
			case 'D':
				mode = Delete;
				Menu();
				break;
			default:		
				release_buffer(buf);
				return;
			}
		}
	case Delete:
		if (isdigit(*arg)) {
			select = MAX(0, atoi(arg));
			
			if (!select || select >= editing->Count()) {
				release_buffer(buf);
				Editor::Finish();
				return;
			}
			--select;
			if (!save)	save = OLC_SAVE_YES;
			editing->Remove(select);

			release_buffer(buf);
			mode = MainMenu;
			Menu();
			return;
		};
		break;
	}

	release_buffer(buf);
}


Ptr	AffectVectorEditor::Pointer(void)
{
	return (Ptr) editing;
}


void AffectVectorEditor::Finish(FinishMode mode)
{
}


IntVectorEditor::IntVectorEditor(Descriptor *d, Vector<int> *edit) :
	Editor(d), editing(edit)
{
}


IntVectorEditor::~IntVectorEditor(void)
{
}


void IntVectorEditor::SaveInternally(void)
{
}


void IntVectorEditor::Menu(void)
{
	int i, j;
	char *buf = get_buffer(MAX_STRING_LENGTH);

	switch (mode) {
	case MainMenu:
		for (i = 0; i < editing->Count(); ++i) {
			sprintf(buf + strlen(buf), "&cG%2d&c0) &cC%5d&c0%8s", i + 1, 
				(*editing)[i], ((i + 1) % 4) ? "" : "\r\n");
		}
		
		strcat(buf, "\r\n&cG A&c0) Add a number\r\n"
					"&cG D&c0) Delete a number\r\n"
					"&cG Q&c0) Quit\r\n");
		d->Writef("Numbers:\r\n%s", buf);
		break;
	case Add:
		break;
	case Delete:
		d->Write("Select number to delete:  ");
		break;
	default:
		break;
	}

	release_buffer(buf);
}


void IntVectorEditor::Parse(char *arg)
{
	int i, j, select;
	char *buf = get_buffer(MAX_STRING_LENGTH);

	switch (mode) {
	case MainMenu:
		if (isdigit(*arg)) {
			select = MAX(0, atoi(arg));
			if (!select || select > editing->Count()) {
				release_buffer(buf);
				Editor::Finish();
				return;
			}
			mode = select;
			d->Writef("Input value for number %d:  ", mode + 1);
			release_buffer(buf);
			return;
		} else {
			switch (*arg) {
			case 'q':
			case 'Q':
				release_buffer(buf);
				Editor::Finish();
				return;
			case 'a':
			case 'A':
				if (!save)	save = OLC_SAVE_YES;
				editing->Append(atoi(arg));
				Menu();
				break;
			case 'd':
			case 'D':
				mode = Delete;
				Menu();
				break;
			default:		
				release_buffer(buf);
				return;
			}
		}
	case Delete:
		if (isdigit(*arg)) {
			select = MAX(0, atoi(arg));
			
			if (!select || select >= editing->Count()) {
				release_buffer(buf);
				Editor::Finish();
				return;
			}
			--select;
			if (!save)	save = OLC_SAVE_YES;
			// editing->Remove(select);

			release_buffer(buf);
			mode = MainMenu;
			Menu();
			return;
		};
		break;
	default:
		if (isdigit(*arg) && (mode <= editing->Count())) {
			(*editing)[mode - 1] = atoi(arg);
			mode = MainMenu;
			Menu();
		}
	}

	release_buffer(buf);
}

Ptr	IntVectorEditor::Pointer(void)
{
	return (Ptr) editing;
}


void IntVectorEditor::Finish(FinishMode mode)
{
}


// Editable * SkillEditor::Editing(void)
// {
// 	return editing;
// }
// 
// 
// Editable * SkillEditor::Source(void)
// {
// 	return ptr;
// }


void GenericMenu(MenuEditor *ed)
{
	int			i = 0;
	char		*buf = get_buffer(MAX_STRING_LENGTH),
				*selection = get_buffer(80),
				*lineptr;
	const struct olc_fields * constraint = NULL;
	Descriptor	*&d = ed->d;
	SInt32		&mode = ed->mode;
	SInt32		&save = ed->save;
	SInt16		&number = ed->number;

	while (1) {
		constraint = ed->Editing()->Constraints(i);
		if (!constraint)
			break;
		strcpy(selection, constraint->name);
		if ((lineptr = strchr(selection, '(')) != NULL)		*lineptr = '\0';
		sprintf(buf + strlen(buf), "&cG%2d&c0) %-15s : ", i + 1, selection);
		ed->Editing()->Print(d, i, buf + strlen(buf));
		++i;
	}
	mode = Editor::MainMenu;

	d->Write(buf);
	release_buffer(buf);
	release_buffer(selection);
}


void GenericParse(MenuEditor *ed, char *arg)
{
	int i, num;
	const struct olc_fields * constraint = NULL;
	Descriptor	*&d = ed->d;
	SInt32		&mode = ed->mode;
	SInt32		&save = ed->save;
	SInt16		&number = ed->number;

	skip_spaces(arg);
	switch (mode) {
	case Editor::MainMenu:
		switch (*arg) {
		case 'q':
		case 'Q':
			if (save) {	//	Anything been changed?
				d->Write("Do you wish to save this internally? (y/n) : ");
				mode = Editor::ConfirmSave;
			} else
				ed->Finish(Editor::All);
			if (!(*arg)) {
				ed->Menu();
			}
			return;
		default:
			num = atoi(arg) - 1;
			i = num;
				constraint = ed->Editing()->Constraints(i);
				if (!constraint || *(constraint->name) == '\n') {
					ed->Menu();
					return;
				}
			switch (constraint->what) {
			case Datatypes::Bool:
				ed->Editing()->Edit(d, "", i, "");
				ed->Menu();
				return;
			case Datatypes::CStringLine:
				mode = i;
				d->Writef("Enter %s: ", constraint->name);
				break;
			case (Datatypes::Pointer + Datatypes::Trigger):
				ed->Editing()->Edit(d, "", i, "");
				break;
			default:
				mode = i;
				ed->Editing()->Edit(d, "", i, "");
				d->Write("Input: ");
				if (constraint->min || constraint->max) {
					d->Writef("(Minimum %ld, Maximum %ld) ", constraint->min, constraint->max);
				}
				return;
			}
			if (!(*arg)) {
				ed->Menu();
			}
			return;
		}
	case Editor::ConfirmSave:
		switch (*arg) {
		case 'y':
		case 'Y':
			d->Write("Saving to memory.\r\n");
			// mudlogf(NRM, d->character, true, "OLC: %s edits skill %d", d->character->RealName(), Skill::Name(targ));
			ed->SaveInternally();
			ed->Finish(Editor::Structs);
			break;
		case 'n':
		case 'N':
			ed->Finish(Editor::All);
			break;
		default:
			d->Write("Invalid choice!\r\n"
					"Do you wish to save your work? ");
		}
		return;
	default:
		constraint = ed->Editing()->Constraints(mode);
		if (constraint) {
			switch (constraint->what) {
			case Datatypes::Bitvector:
				switch (*arg && ed->Editing()->Edit(d, "", mode, arg)) {
				case 0: 
					ed->Menu(); 
					return;
				default: 
					if (!(save))	save = OLC_SAVE_YES;
					ed->Editing()->Edit(d, "", mode, "");
				}
				return;
			case Datatypes::Integer:
			case Datatypes::Value:
			case Datatypes::SpellValue:
			case Datatypes::DiffValue:
				if (!(*arg)) {
					ed->Menu();
				}
				ed->Editing()->Edit(d, "", mode, arg);
				break;
			case Datatypes::CStringLine:
				ed->Editing()->Edit(d, "", mode, arg);
				break;
			}
		}

		if (!(save))	save = OLC_SAVE_YES;
	}

	ed->Menu();
}




const char *	TextEditor::Prompt(void)
{
	static const char * str = "] ";
	return str;
}


PropertyListEditor::PropertyListEditor(Descriptor *d, Property::PropertyList *editmap, int min, int max) :
	Editor(d), editing(editmap), mintype(min), maxtype(max)
{
}


PropertyListEditor::~PropertyListEditor(void)
{
}


void PropertyListEditor::SaveInternally(void)
{
}


void PropertyListEditor::Menu(void)
{
	int i, j;
	char *buf = get_buffer(MAX_STRING_LENGTH);

	switch (mode) {
	case MainMenu:
		for (i = mintype, j = 0; i <= maxtype; ++i) {
			if ((*editing)[i]) {
				sprintf(buf + strlen(buf),
					"&cG%2d&c0) %s\r\n", ++j, (*editing)[i]->Name());
			}
		}
		
		strcat(buf, "\r\n&cG A&c0) Add a property\r\n"
					"&cG D&c0) Delete a property\r\n"
					"&cG Q&c0) Quit\r\n");
		d->Write(buf);
		break;
	case AddProperty:
		for (i = mintype, j = 0; i <= maxtype; ++i) {
			sprintf(buf + strlen(buf), "&cG%2d&c0) %-20s\r\n", ++j, 
				Property::skill_properties[i]);
		}
		d->Write(buf);
		d->Write("Select property:  ");
		break;
	case DeleteProperty:
		d->Write("Input number of property to delete:  ");
		break;
	}

	release_buffer(buf);
}


void PropertyListEditor::Parse(char *arg)
{
	int i, j, select;
	char *buf = get_buffer(MAX_STRING_LENGTH);

	switch (mode) {
	case MainMenu:
		if (isdigit(*arg)) {
			select = MAX(0, atoi(arg));
			if (!select) {
				release_buffer(buf);
				Editor::Finish();
				return;
			}
			for (i = mintype, j = select; i <= maxtype; ++i) {
				if ((*editing)[i]) {
					--j;
					if (!j) {
						MenuEditor *ed = new GenericEditor(d, (*editing)[i]);
						d->Edit(ed);
						ed->Menu();
						break;
					}
				}
			}

			if (j)	Menu();
			release_buffer(buf);
			return;
		} else {
			switch (*arg) {
			case 'q':
			case 'Q':
				release_buffer(buf);
				Editor::Finish();
				return;
			case 'a':
			case 'A':
				mode = AddProperty;
				Menu();
				break;
			case 'd':
			case 'D':
				mode = DeleteProperty;
				Menu();
				break;
			default:		
				release_buffer(buf);
				return;
			}
		}
	case AddProperty:
		if (isdigit(*arg)) {
			select = MAX(0, atoi(arg));
			i = select + mintype - 1;

			if (select && (i <= maxtype)) {
				editing->Add(i);
			}

			if (!save)	save = OLC_SAVE_YES;
			release_buffer(buf);
			mode = MainMenu;
			Menu();
			return;
		}
		break;
	case DeleteProperty:
		if (isdigit(*arg)) {
			select = MAX(0, atoi(arg));
			
			if (select) {
				for (i = mintype, j = select; i <= maxtype; ++i) {
					if ((*editing)[i]) {
						if (--j <= 0) {
							break;
						}
					}
				}
				if (!j) {
					if (!save)	save = OLC_SAVE_YES;
					editing->Remove(i);
				}
			}

			release_buffer(buf);
			mode = MainMenu;
			Menu();
			return;
		};
		break;
	}

	release_buffer(buf);
}


Ptr	PropertyListEditor::Pointer(void)
{
	return (Ptr) editing;
}


void PropertyListEditor::Finish(FinishMode mode)
{
}


GenericEditor::GenericEditor(Descriptor *d, Editable *thing) :
	MenuEditor(d, -1), editing(thing)
{
}


GenericEditor::~GenericEditor(void)
{
}


void GenericEditor::SaveInternally(void)
{
}


void GenericEditor::Menu(void)
{
	GenericMenu(this);
	d->Write("\r\n&cG Q&c0) Exit menu\r\n");
}


void GenericEditor::Parse(char *arg)
{
	switch (*arg) {
	case 'q':	
	case 'Q':	Finish(Editor::All);
				return;
	default:	break;
	}
	GenericParse(this, arg);
}


Editable	* GenericEditor::Editing(void)
{
	return (Editable *) editing;
}


Editable	* GenericEditor::Source(void)
{
	return (Editable *) editing;
}


Ptr	GenericEditor::Pointer(void)
{
	return (Ptr) editing;
}


void GenericEditor::Finish(FinishMode mode)
{
	Editor::Finish();
}


void save_skills(void)
{
	FILE *fp;
	char *buf;
	
	if (!(fp = fopen(SKILL_FILE ".new", "w+"))) {
		log("SYSERR: Can't open skill file '%s'", SKILL_FILE);
		exit(1);
	}
	
	for (SInt32 i = 1; i < Skills.Count(); ++i) {
		if (!Skills[i] || (Skills[i] == Skills[0]))
			continue;
		fprintf(fp, "#%d = {\n", i);
		Skills[i]->SaveDisk(fp);
		fprintf(fp, "};\n");
	}

	fclose(fp);
	
	remove(SKILL_FILE);
	rename(SKILL_FILE ".new", SKILL_FILE);
	
	olc_remove_from_save_list(SKILLEDIT_PERMISSION, OLC_SKILLEDIT);
}
