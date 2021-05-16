/*************************************************************************
*   File: act.wizard.c++             Part of Aliens vs Predator: The MUD *
*  Usage: Staff commands                                                 *
*************************************************************************/


#include "structs.h"
#include "opinion.h"
#include "scripts.h"
#include "utils.h"
#include "buffer.h"
#include "descriptors.h"
#include "rooms.h"
#include "characters.h"
#include "objects.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "find.h"
#include "db.h"
#include "house.h"
#include "affects.h"
#include "event.h"
#include "olc.h"
#include "extradesc.h"
#include "track.h"
#include "ident.h"
#include "constants.h"
#include "skills.h"
#include "combat.h"

/* extern functions */
void show_shops(Character * ch, char *value);
void hcontrol_list_houses(Character *ch);
int parse_race(char * arg);
void reset_zone(int zone);
void roll_real_abils(Character *ch);
void forget(Character * ch, Character * victim);
void remember(Character * ch, Character * victim);
void CheckRegenRates(Character *ch);
int _parse_name(char *arg, char *name);
bool reserved_word(const char *argument);

// For copyover
extern SInt32 mother_desc;
extern SInt32 port;

extern int vnum_room(char *searchname, Character * ch);
extern int vnum_shop(char *searchname, Character * ch);

void load_otrigger(Object *obj);
void load_mtrigger(Character *mob);
/*   external vars  */
extern struct zone_data *zone_table;
extern time_t boot_time;
extern int top_of_zone_table;
extern SInt32 circle_restrict;
extern int circle_shutdown, circle_reboot, circle_copyover;
extern int buf_switches, buf_largecount, buf_overflows;
extern bool no_external_procs;


/* prototypes */
VNum	find_target_room(Character * ch, char *rawroomstr);
void do_stat_room(Character * ch);
void do_stat_object(Character * ch, Object * j);
void do_stat_character(Character * ch, Character * k);
void stop_snooping(Character * ch);
void perform_immort_vis(Character *ch);
void perform_immort_invis(Character *ch, bool goAdmin);
void print_zone_to_buf(char *bufptr, int zone);
void show_deleted(Character * ch);
void send_to_imms(char *msg);
bool perform_set(Character *ch, Character *vict, SInt32 mode, char *val_arg);
extern int file_to_string(const char *name, char *buf);
ACMD(do_echo);
ACMD(do_send);
ACMD(do_at);
ACMD(do_goto);
ACMD(do_trans);
ACMD(do_teleport);
ACMD(do_vnum);
ACMD(do_stat);
ACMD(do_shutdown);
ACMD(do_snoop);
ACMD(do_switch);
ACMD(do_return);
ACMD(do_load);
ACMD(do_vstat);
ACMD(do_purge);
ACMD(do_advance);
ACMD(do_restore);
ACMD(do_invis);
ACMD(do_gecho);
ACMD(do_poofset);
ACMD(do_dc);
ACMD(do_wizlock);
ACMD(do_date);
ACMD(do_last);
ACMD(do_force);
ACMD(do_wiznet);
ACMD(do_zreset);
ACMD(do_wizutil);
ACMD(do_show);
ACMD(do_set);
ACMD(do_syslog);
ACMD(do_depiss);
ACMD(do_repiss);
ACMD(do_hunt);
ACMD(do_vwear);
ACMD(do_copyover);
ACMD(do_tedit);
ACMD(do_reward);
ACMD(do_string);
ACMD(do_peace);


#define ENOUGH_TRUST(ch, vict) (STF_FLAGGED(ch, Staff::Security | Staff::Admin) && \
					GET_TRUST(ch) > GET_TRUST(vict))

ACMD(do_echo) {
	skip_spaces(argument);

	if (!*argument)
		ch->Send("Yes.. but what?\r\n");
	else {
		char *buf = get_buffer(MAX_STRING_LENGTH);
		if (subcmd == SCMD_EMOTE)
			sprintf(buf, "`t%c%d %s&c0", UID_CHAR, GET_ID(ch), argument);
		else
			strcpy(buf, argument);

		sub_write(buf, ch, FALSE, TO_ROOM);

		if (PRF_FLAGGED(ch, Preference::NoRepeat))
			ch->Send(OK);
		else {
			if (subcmd == SCMD_EMOTE)
				sprintf(buf, "%s %s&c0", ch->Name(), argument);
			sub_write(buf, ch, FALSE, TO_CHAR);
		}
		release_buffer(buf);
	}
}


ACMD(do_send) {
	Character *vict;
	char	*buf = get_buffer(MAX_INPUT_LENGTH),
			*arg = get_buffer(MAX_INPUT_LENGTH);

	half_chop(argument, arg, buf);

	if (!*arg) {
		ch->Send("Send what to who?\r\n");
	} else if (!(vict = get_char_vis(ch, arg, FIND_CHAR_WORLD))) {
		ch->Send(NOPERSON);
	} else {
		vict->Sendf("%s\r\n", buf);
		if (PRF_FLAGGED(ch, Preference::NoRepeat))
			ch->Send("Sent.\r\n");
		else
			act("You send '$t' to $N.\r\n", TRUE, ch, (Object *)buf, vict, TO_CHAR);
	}
	release_buffer(buf);
	release_buffer(arg);
}



/* take a string, and return an rnum.. used for goto, at, etc.  -je 4/6/93 */
VNum find_target_room(Character * ch, char *rawroomstr) {
	VNum location;
	Character *target_mob;
	Object *target_obj;
	char *roomstr = get_buffer(MAX_INPUT_LENGTH);

	one_argument(rawroomstr, roomstr);

	if (!*roomstr) {
		ch->Send("You must supply a room number or name.\r\n");
		release_buffer(roomstr);
		return NOWHERE;
	}
	if (isdigit(*roomstr) && !strchr(roomstr, '.')) {
		if (!Room::Find(location = atoi(roomstr))) {
			ch->Send("No room exists with that number.\r\n");
			release_buffer(roomstr);
			return NOWHERE;
		}
	} else if ((target_mob = get_char_vis(ch, roomstr, FIND_CHAR_WORLD)))
		location = IN_ROOM(target_mob);
	else if ((target_obj = get_obj_vis(ch, roomstr))) {
		if (IN_ROOM(target_obj) != NOWHERE)
			location = IN_ROOM(target_obj);
		else {
			ch->Send("That object is not available.\r\n");
			release_buffer(roomstr);
			return NOWHERE;
		}
	} else {
		ch->Send("No such creature or object around.\r\n");
		release_buffer(roomstr);
		return NOWHERE;
	}
	release_buffer(roomstr);

	if (ROOM_FLAGGED(location, ROOM_GODROOM) && !IS_STAFF(ch)) {
		ch->Send("Only Staff are allowed in that room!\r\n");
		return NOWHERE;
	}

	if ((ROOM_FLAGGED(IN_ROOM(ch), ROOM_JAIL)) &&
			(PLR_FLAGGED(ch, PLR_JAILED))) {
		ch->Send("You are in jail!  You cannot leave!\r\n");
		return NOWHERE;
	}

	/* a location has been found -- if you're < GRGOD, check restrictions. */
	if (GET_TRUST(ch) < TRUST_GRGOD) {
		if (ROOM_FLAGGED(location, ROOM_PRIVATE) && (world[location].people.Count() > 1)) {
			ch->Send("There's a private conversation going on in that room.\r\n");
			return NOWHERE;
		}
		if (ROOM_FLAGGED(location, ROOM_HOUSE) && !House::CanEnter(ch, world[location].Virtual())) {
			ch->Send("That's private property -- no trespassing!\r\n");
			return NOWHERE;
		}
		if (ROOM_FLAGGED(location, ROOM_GRGODROOM)) {
			ch->Send("Only senior staff members are allowed in that room!\r\n");
			return NOWHERE;
		}
	}

#ifdef GOT_RID_OF_IT
	if (ROOM_FLAGGED(location, ROOM_ULTRAPRIVATE) && (!IS_STAFF(ch) || !STF_FLAGGED(ch, Staff::Coder))) {
		ch->Send("That room is off limits to you.\r\n");
		return NOWHERE;
	}
#endif
	return location;
}



ACMD(do_at) {
	char	*the_command = get_buffer(MAX_INPUT_LENGTH),
			*buf = get_buffer(MAX_INPUT_LENGTH);
	int location, original_loc;

	half_chop(argument, buf, the_command);
	if (!*buf) {
		ch->Send("You must supply a room number or a name.\r\n");
	} else if (!*the_command) {
		ch->Send("What do you want to do there?\r\n");
	} else if ((location = find_target_room(ch, buf)) >= 0) {

		/* a location has been found. */
		original_loc = IN_ROOM(ch);
		ch->FromRoom();
		ch->ToRoom(location);
		command_interpreter(ch, the_command);

		/* check if the char is still there */
		if (IN_ROOM(ch) == location) {
			ch->FromRoom();
			ch->ToRoom(original_loc);
		}
	}
	release_buffer(the_command);
	release_buffer(buf);
}


ACMD(do_goto) {
	VNum location;
	char * buf;

	if ((location = find_target_room(ch, argument)) < 0)
		return;

	buf = get_buffer(MAX_STRING_LENGTH);

	if (POOFOUT(ch))
		strcpy(buf, POOFOUT(ch));
	else
		strcpy(buf, "$n disappears in a puff of smoke.");

	act(buf, TRUE, ch, 0, 0, TO_ROOM);
	ch->FromRoom();
	ch->ToRoom(location);

	if (POOFIN(ch))
		strcpy(buf, POOFIN(ch));
	else
		strcpy(buf, "$n appears with an ear-splitting bang.");

	act(buf, TRUE, ch, 0, 0, TO_ROOM);
	look_at_room(ch, 0);
	release_buffer(buf);
}



ACMD(do_trans) {
	Descriptor *i;
	Character *victim;
	char *buf = get_buffer(MAX_INPUT_LENGTH);

	one_argument(argument, buf);
	if (!*buf)
		ch->Send("Whom do you wish to transfer?\r\n");
	else if (str_cmp("all", buf)) {
		if (!(victim = get_char_vis(ch, buf, FIND_CHAR_WORLD)))
			ch->Send(NOPERSON);
		else if (victim == ch)
			ch->Send("That doesn't make much sense, does it?\r\n");
		else if (GET_TRUST(victim) > GET_TRUST(ch))
			ch->Send("Go transfer someone your own size.\r\n");
		else if ((ROOM_FLAGGED(IN_ROOM(victim), ROOM_JAIL)) && (PLR_FLAGGED(victim, PLR_JAILED)))
			ch->Send("You cannot transfer somebody out of jail!\r\n");
		else {
			act("$n disappears in a mushroom cloud.", FALSE, victim, 0, 0, TO_ROOM);
			victim->FromRoom();
			victim->ToRoom(IN_ROOM(ch));
			act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
			act("$n has transferred you!", FALSE, ch, 0, victim, TO_VICT);
			look_at_room(victim, 0);
		}
	} else {			/* Trans All */
		if (!STF_FLAGGED(ch, Staff::Game | Staff::Admin | Staff::Security)) {
			ch->Send("I think not.\r\n");
			release_buffer(buf);
			return;
		}

		START_ITER(iter, i, Descriptor *, descriptor_list) {
			if ((STATE(i) == CON_PLAYING) && i->character && (i->character != ch)) {
				victim = i->character;
				if ((IS_STAFF(victim) && !STF_FLAGGED(ch, Staff::Admin)) || STF_FLAGGED(victim, Staff::Admin))
					continue;
				else if ((ROOM_FLAGGED(IN_ROOM(victim), ROOM_JAIL)) && (PLR_FLAGGED(victim, PLR_JAILED)))
					continue;
				act("$n disappears in a mushroom cloud.", FALSE, victim, 0, 0, TO_ROOM);
				victim->FromRoom();
				victim->ToRoom(IN_ROOM(ch));
				act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
				act("$n has transferred you!", FALSE, ch, 0, victim, TO_VICT);
				look_at_room(victim, 0);
			}
		}
		ch->Send(OK);
	}
	release_buffer(buf);
}



ACMD(do_teleport) {
	Character *victim;
	SInt16	target;
	char *buf = get_buffer(MAX_INPUT_LENGTH),
		*buf2 = get_buffer(MAX_INPUT_LENGTH);

	two_arguments(argument, buf, buf2);

	if (!*buf)
		ch->Send("Whom do you wish to teleport?\r\n");
	else if (!(victim = get_char_vis(ch, buf, FIND_CHAR_WORLD)))
		ch->Send(NOPERSON);
	else if (victim == ch)
		ch->Send("Use 'goto' to teleport yourself.\r\n");
	else if (GET_TRUST(victim) > GET_TRUST(ch))
		ch->Send("Maybe you shouldn't do that.\r\n");
	else if ((ROOM_FLAGGED(IN_ROOM(victim), ROOM_JAIL)) && (PLR_FLAGGED(victim, PLR_JAILED)))
		ch->Send("You cannot teleport somebody out of jail!\r\n");
	else if ((IS_STAFF(victim) && !STF_FLAGGED(ch, Staff::Admin)) || STF_FLAGGED(victim, Staff::Admin))
		ch->Send("Maybe you shouldn't do that.\r\n");
	else if (!*buf2)
		ch->Send("Where do you wish to send this person?\r\n");
	else if ((target = find_target_room(ch, buf2)) >= 0) {
		ch->Send(OK);
		act("$n disappears in a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
		victim->FromRoom();
		victim->ToRoom(target);
		act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
		act("$n has teleported you!", FALSE, ch, 0, victim, TO_VICT);
		look_at_room(victim, 0);
	}
	release_buffer(buf);
	release_buffer(buf2);
}



ACMD(do_vnum) {
	char *buf = get_buffer(MAX_INPUT_LENGTH),
		*buf2 = get_buffer(MAX_INPUT_LENGTH);

	half_chop(argument, buf, buf2);

	if (!*buf || !*buf2)
		ch->Send("Usage: vnum { obj | mob | trg | room | shop } <name>\r\n");
	else if (is_abbrev(buf, "mob")) {
		if (!vnum_mobile(buf2, ch))
			ch->Send("No mobiles by that name.\r\n");
	} else if (is_abbrev(buf, "obj")) {
		if (!vnum_object(buf2, ch))
			ch->Send("No objects by that name.\r\n");
	} else if (is_abbrev(buf, "trg")) {
		if (!vnum_trigger(buf2, ch))
			ch->Send("No triggers by that name.\r\n");
	} else if (is_abbrev(buf, "room")) {
		if (!vnum_room(buf2, ch))
			ch->Send("No rooms found.\r\n");
	} else if (is_abbrev(buf, "shop")) {
		if (!vnum_shop(buf2, ch))
			ch->Send("No shops found.\r\n");
	} else
		ch->Send("Usage: vnum { obj | mob | trg | room | shop } <name>\r\n");

	release_buffer(buf);
	release_buffer(buf2);
}



void do_stat_room(Character * ch) {
	ExtraDesc *desc;
	Room *rm = &world[IN_ROOM(ch)];
	int i, found = 0;
	Object *j = 0;
	Character *k = 0;
	char	*buf = get_buffer(MAX_STRING_LENGTH);

	sprinttype(rm->SectorType(), sector_types, buf);
	ch->Sendf(	"Room name     : &cc%s&c0\r\n"
				"     Zone     : [%3d]      VNum     : [&cg%5d&c0]\r\n"
				"     Type     : %-10s "   "RNum     : [%5d]\r\n",
			rm->Name("<ERROR>"),
			zone_table[rm->zone].number, rm->Virtual(),
			buf, IN_ROOM(ch));

	sprintbit(rm->RoomFlags(), room_bits, buf);
	ch->Sendf(	"     SpecProc : %-10s "   "Script   : %s\r\n"
				"Room Flags    : %s\r\n"
				"[Description]\r\n"
				"%s",
			!rm->func ? "None" : "Exists", SCRIPT(rm) ? "Exists" : "None",
			buf,
			rm->Description("  None.\r\n"));

	if (rm->ex_description.Count()) {
		ch->Send("Extra descs   :&cc");
		LListIterator<ExtraDesc *>	descIter(rm->ex_description);
		while ((desc = descIter.Next()))
			ch->Sendf(" %s", SSData(desc->keyword));
		ch->Send("&c0\r\n");
	}

	if (rm->people.Count()) {
		*buf = found = 0;
		ch->Send(	"[Chars present]\r\n     &cy");
		START_ITER(iter, k, Character *, rm->people) {
			if (!k->CanBeSeen(ch))	continue;
			sprintf(buf + strlen(buf),	"%s %s(%s)",
					found++ ? "," : "", k->Name(),
					(!IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB")));
			if (strlen(buf) >= 62) {
				if (iter.Peek())		ch->Sendf("%s,\r\n", buf);
				else					ch->Sendf("%s\r\n", buf);
				*buf = found = 0;
			}
		}
		ch->Sendf("%s%s&c0", buf, found ? "\r\n" : "");
	}

	if (rm->contents.Count()) {
		*buf = found = 0;
		ch->Send("[Contents]\r\n     &cg");
		START_ITER(iter, j, Object *, rm->contents) {
			if (!j->CanBeSeen(ch))	continue;
			sprintf(buf + strlen(buf), "%s %s", found++ ? "," : "", j->Name());
			if (strlen(buf) >= 62) {
				if (iter.Peek())		ch->Sendf("%s,\r\n", buf);
				else					ch->Sendf("%s\r\n", buf);
				*buf = found = 0;
			}
		}
		ch->Sendf("%s%s&c0", buf, found ? "\r\n" : "");
	}

	for (i = 0; i < NUM_OF_DIRS; i++) {
		if (rm->Direction(i)) {
			if (rm->Direction(i)->to_room == NOWHERE)
				strcpy(buf, " &ccNONE&c0");
			else
				sprintf(buf, "&cc%5d&c0", world[rm->Direction(i)->to_room].Virtual());
			ch->Sendf("Exit &cc%-5s&c0:  To: [%s], Key: [%5d], Keywrd: %s, ",
					dirs[i], buf, rm->Direction(i)->key, rm->Direction(i)->Keyword("None"));

			sprintbit(rm->Direction(i)->exit_info, exit_bits, buf);
			ch->Sendf("Type: %s\r\n"
					 "%s",
					buf,
					rm->Direction(i)->Description("  No exit description.\r\n"));
		}
	}
	release_buffer(buf);
}


extern const char *weapon_types[];


void objval_info(Character *ch, Object *j)
{
	*buf = '\0';

	switch (GET_OBJ_TYPE(j)) {
	case ITEM_LIGHT:
		if (GET_OBJ_VAL(j, 2) == -1)
			strcpy(buf, "Hours left: Infinite");
		else
			sprintf(buf, "Hours left: [%d]", GET_OBJ_VAL(j, 2));
		break;
	case ITEM_SCROLL:
	case ITEM_POTION:
		sprintf(buf, "Spells: (Level %d) %s, %s, %s", GET_OBJ_VAL(j, 0),
			Skill::Name(GET_OBJ_VAL(j, 1)), Skill::Name(GET_OBJ_VAL(j, 2)),
			Skill::Name(GET_OBJ_VAL(j, 3)));
		break;
	case ITEM_WAND:
	case ITEM_STAFF:
		sprintf(buf, "Spell: %s at level %d, %d (of %d) charges remaining",
			Skill::Name(GET_OBJ_VAL(j, 3)), GET_OBJ_VAL(j, 0),
			GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 1));
		break;
#ifdef RANGED_WEAPONS
	case ITEM_MISSILE:
		sprintf(buf, "Todam: %dd%d, Weapon type: %d",
			GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 3));
		break;
	case ITEM_GRENADE:
		sprintf(buf, "Timer: %d	Todam: %dd%d, %s",
						GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2),
						GET_OBJ_VAL(j, 3) ? "Activated!" : "");
		break;
	case ITEM_FIREWEAPON:
		sprintf(buf, "Proficiency: %s, Accuracy bonus: %d, Damage Type: %s, Damage bonus: %d\r\n"
								 "Range: %d, Speed: %d",
			Skill::Name(GET_OBJ_VAL(j, 0)), GET_OBJ_VAL(j, 1),
			damage_types[GET_OBJ_VAL(j, 3)], GET_OBJ_VAL(j, 2),
			GET_OBJ_VAL(j, 6), GET_OBJ_VAL(j, 7));
		if (GET_OBJ_VAL(j, 4) > 0)
			sprintf(buf + strlen(buf), ", Can hold rounds: %d, Currently contains rounds: %d",
							GET_OBJ_VAL(j, 4), GET_OBJ_VAL(j, 5));
		break;
#endif
	case ITEM_WEAPON:
		sprintf(buf, "Proficiency: %s, Speed: %d\r\n",
			Skill::Name(GET_OBJ_VAL(j, 0)), GET_OBJ_VAL(j, 7));
		if (GET_OBJ_VAL(j, 2) > -1)
			sprintf(buf, "%sSwing bonus: %d, Damage Type: %s, Message: %s\r\n", buf,
				GET_OBJ_VAL(j, 1), damage_types[GET_OBJ_VAL(j, 2)],
				Skill::Name(GET_OBJ_VAL(j, 3) + TYPE_HIT));
		if (GET_OBJ_VAL(j, 5) > -1)
			sprintf(buf, "%sThrust bonus: %d, Damage Type: %s, Message: %s", buf,
				GET_OBJ_VAL(j, 4), damage_types[GET_OBJ_VAL(j, 5)],
				Skill::Name(GET_OBJ_VAL(j, 6) + TYPE_HIT));
		break;
	case ITEM_ARMOR:
		sprintf(buf, "Passive Defense: [%d]	Damage Resistance: [%d]",
					GET_PD(j), GET_DR(j));
		break;
	case ITEM_TRAP:
		sprintf(buf, "Spell: %d, - Hitpoints: %d",
			GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1));
		break;
	case ITEM_CONTAINER:
		sprintbit(GET_OBJ_VAL(j, 1), container_bits, buf2);
		sprintf(buf, "Weight capacity: %d, Lock Type: %s, Key Num: %d, Corpse: %s",
			GET_OBJ_VAL(j, 0), buf2, GET_OBJ_VAL(j, 2),
			YESNO(GET_OBJ_VAL(j, 3)));
		break;
	case ITEM_DRINKCON:
	case ITEM_FOUNTAIN:
		sprinttype(GET_OBJ_VAL(j, 2), drinks, buf2);
		sprintf(buf, "Capacity: %d, Contains: %d, Poisoned: %s, Liquid: %s",
			GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), YESNO(GET_OBJ_VAL(j, 3)),
			buf2);
		break;
	case ITEM_NOTE:
		sprintf(buf, "Tongue: %d", GET_OBJ_VAL(j, 0));
		break;
	case ITEM_KEY:
		strcpy(buf, "");
		break;
	case ITEM_FOOD:
		sprintf(buf, "Makes full: %d, Poisoned: %s", GET_OBJ_VAL(j, 0),
			YESNO(GET_OBJ_VAL(j, 3)));
		break;
	case ITEM_MONEY:
		sprintf(buf, "Coins: %d", GET_OBJ_VAL(j, 0));
		break;
	default:
		sprintf(buf, "Values 0-3: [%d] [%d] [%d] [%d]",
			GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1),
			GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 3));
		break;
	}
	ch->Send(strcat(buf, "\r\n"));
	ch->Send("\r\n");
}


void do_stat_object(Character * ch, Object * j) {
	int i, found = 0;
	Affect *aff;
	Object *j2 = NULL;
	ExtraDesc *desc;

	sprinttype(GET_OBJ_TYPE(j), item_types, buf);
	ch->Sendf("%s '&cG%s&c-'  VNum: [&cC%d&c-], In room [&cY%d&c-]\r\n",
			buf, j->Name("<NONE>"), j->Virtual(),
			IN_ROOM(j) != NOWHERE ? world[IN_ROOM(j)].Virtual() : -1);

	ch->Sendf("Keywords: %s, ID: [&cC%d&c-], SpecProc: %s\r\n",
			j->Keywords(), j->ID(),
			(Object::Index[j->Virtual()].func ?
			spec_obj_name(Object::Index[j->Virtual()].func) : "None"));

	if (j->exDesc.Count()) {
		ch->Send("Extra descs   :&cC");
		LListIterator<ExtraDesc *>	descIter(j->exDesc);
		while ((desc = descIter.Next()))
			ch->Sendf(" %s", SSData(desc->keyword));
		ch->Send("&c-\r\n");
	}

	sprintbit(GET_OBJ_WEAR(j), wear_bits, buf);
	ch->Sendf(	"Can be worn on: %s\r\n", buf);
	sprintbit(j->affectbits, affected_bits, buf);
	ch->Sendf(	"Set char bits : %s\r\n", buf);
	sprintbit(GET_OBJ_EXTRA(j), extra_bits, buf);
	ch->Sendf(	"Extra flags   : %s\r\n", buf);

	if(IN_ROOM(j) > 0)		ch->Sendf("In room       : %s&c- [&cc%d&c-]\r\n", world[IN_ROOM(j)].Name("<ERROR>"), world[IN_ROOM(j)].Virtual());
	else if(j->InObj())		ch->Sendf("In object     : %s&c-\r\n", j->InObj()->Name());
	else if(j->CarriedBy())	ch->Sendf("Carried by    : %s&c-\r\n", j->CarriedBy()->Name());
	else if(j->WornBy())	ch->Sendf("Worn by       : %s&c-\r\n", j->WornBy()->Name());

	ch->Sendf(	"Weight: [&cC%d&c-], Value: [&cC%d&c-], Timer: [&cC%d&c-]\r\n",
			GET_OBJ_WEIGHT(j), GET_OBJ_COST(j),
			GET_OBJ_TIMER(j));

	ch->Send("[Values]\r\n");

	objval_info(ch, j);

	if (j->contents.Count()) {
		LListIterator<Object *>	iter(j->contents);
		strcpy(buf, "\r\nContents:&cg");
		*buf = found = 0;
		while ((j2 = iter.Next())) {
			sprintf(buf + strlen(buf), "%s %s", found++ ? "," : "", j2->Name());
			if (strlen(buf) >= 62) {
				ch->Sendf("%s%s\r\n", buf, iter.Peek() ? "," : "");
				*buf = found = 0;
			}
		}
		ch->Sendf(	"%s%s&c-"
					"Total content value: [&cC%d&c-]\r\n", buf, found ? "\r\n" : "",
					j->TotalValue() - GET_OBJ_COST(j));
	}

	found = 0;

	ch->Send("Affections:");
	for (i = 0; i < MAX_OBJ_AFFECT; i++)
		if (j->affectmod[i].modifier) {
			sprinttype(j->affectmod[i].location, apply_types, buf);
			ch->Sendf("%s %+d to %s", found++ ? "," : "", j->affectmod[i].modifier, buf);
		}
	ch->Sendf("%s\r\n", found ? "" : " None");
}


void do_stat_character(Character * ch, Character * k)
{
	int i, found = 0;
	Affect *aff;
	Character *follower;

	switch (GET_SEX(k)) {
		case Neutral:		strcpy(buf, "NEUTRAL-SEX");		break;
		case Male:			strcpy(buf, "MALE");			break;
		case Female:		strcpy(buf, "FEMALE");			break;
		default:			strcpy(buf, "ILLEGAL-SEX!!");	break;
	}

	if IS_ADMIN(ch) {
		ch->Sendf("%s %s '&cG%s&c-'  ID: [&cC%d&c-], Trust: [&cC%d&c-], In room [&cY%5d&c-]\r\n",
		buf, (!IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB")),
		k->Name(), (IS_NPC(k) ? GET_ID(k) : GET_IDNUM(k)), GET_TRUST(k),
		IN_ROOM(k) != NOWHERE ? world[IN_ROOM(k)].Virtual() : -1);
	} else {
		ch->Sendf("%s %s '&cG%s&c-'  ID: [&cC%d&c-], In room [&cY%5d&c-]\r\n",
		buf, (!IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB")),
		k->Name(), (IS_NPC(k) ? GET_ID(k) : GET_IDNUM(k)),
		IN_ROOM(k) != NOWHERE ? world[IN_ROOM(k)].Virtual() : -1);
	}

	*buf = '\0';
	if (k->desc) {
		sprintf(buf, "  Connected:  %s", connected_types[STATE(k->desc)]);
	}

	if (IS_MOB(k)) {
		ch->Sendf("Keywords: %s, VNum: [%5d]%s\r\n",
				k->Keywords("NONE"), k->Virtual(), buf ? buf : "");
	} else {
		ch->Sendf("EMail Address: %s%s\r\n"
				"L-Des: &cY%s&c-"
				"Race: [&cG%s&c-],  Mort: [&cG%s&c-],  Class: [&cG%s&c-]\r\n"
				"Title: &cY%s&c-\r\n",
				k->player->email ? k->player->email : "NONE",
				buf ? buf : "",
				k->SSLDesc() ? k->LDesc() : "<None>\r\n",
				race_types[(int)GET_RACE(k)],
				mortality_types[(int)GET_MORTALITY(k)],
				class_types[(int)GET_CLASS(k)],
				GET_TITLE(k) ? GET_TITLE(k) : "<None>");

		strcpy(buf, asctime(localtime(&(k->player->time.birth))));
		strcpy(buf2, asctime(localtime(&(k->player->time.logon))));
		buf[10] = buf2[10] = '\0';

		ch->Sendf(	"Created: [%s], Last Logon: [%s], Played [%dh %dm], Age [%d]\r\n",
				buf, buf2, k->player->time.played / 3600, ((k->player->time.played % 3600) / 60), age(k).year);
	}

	sprintf(buf, "Str : &cC%2d&c-   Hit       : &cC%4d&c-/&cC%4d&c-   Level    : &cC%4d&c-   Loadroom   : &cC%d&c-\r\n"
		"Int : &cC%2d&c-   Mana      : &cC%4d&c-/&cC%4d&c-   Max Level: &cC%4d&c-   Pos        : &cY%s&c-\r\n"
		"Wis : &cC%2d&c-   Move      : &cC%4d&c-/&cC%4d&c-   PD       : &cC%4d&c-   Fighting   : &cY%s&c-\r\n"
		"Dex : &cC%2d&c-   Alignment :    &cC%6d&c-   DR       : &cC%4d&c-   Hunting    : &cC%d&c-\r\n"
		"Con : &cC%2d&c-   Total Pts :    &cC%6d&c-   Hitroll  : &cC%4d&c-   Seeking    : &cC%d&c-\r\n"
		"Cha : &cC%2d&c-   Practices :    &cC%6d&c-   Damroll  : &cC%4d&c-   Roomseeking: &cC%d&c-\r\n",
		GET_STR(k), GET_HIT(k), GET_MAX_HIT(k), GET_LEVEL(k), IS_MOB(k) ? 0 : GET_LOADROOM(k),
		GET_INT(k), GET_MANA(k), GET_MAX_MANA(k), IS_NPC(k) ? GET_LEVEL(k) : GET_MAX_LEVEL(k),
		position_types[(int)GET_POS(k)],
		GET_WIS(k), GET_MOVE(k), GET_MAX_MOVE(k), GET_PD(k),
		FIGHTING(k) ? FIGHTING(k)->Name() : "Nobody",
		GET_DEX(k), GET_ALIGNMENT(k), GET_DR(k), HUNTING(k),
		GET_CON(k), GET_TOTALPTS(k), GET_HITROLL(k), SEEKING(k),
		GET_CHA(k), GET_PRACTICES(k), GET_DAMROLL(k), ROOMSEEKING(k));

	ch->Send(buf);

	*buf = '\0';
	*buf2 = '\0';
	*buf3 = '\0';


	ch->Sendf("%s  Idle Timer (in tics) [%d]\r\n", buf ? buf : "", k->player->timer);

	*buf = '\0';

	for (i = 0; i < 32; ++i) {
	    if (ADVANTAGED(k, 1 << i)) {
	    	if (*buf2)	sprintf(buf2 + strlen(buf2), ", %s", named_advantages[i]);
	    	else		strcpy(buf2, named_advantages[i]);
	    }
	}

	for (i = 0; i < 32; ++i) {
	    if (DISADVANTAGED(k, 1 << i)) {
	    	if (*buf3)	sprintf(buf3 + strlen(buf3), ", %s", named_disadvantages[i]);
	    	else		strcpy(buf3, named_disadvantages[i]);
	    }
	}

	if (*buf2)
	    sprintf(buf + strlen(buf), "Advantages:    &cG%s&c-\r\n", buf2);

	if (*buf3)
	    sprintf(buf + strlen(buf), "Disadvantages: &cG%s&c-\r\n", buf3);

	ch->Send(buf);

	if (IS_NPC(k)) {
		sprintbit(MOB_FLAGS(k), action_bits, buf);			ch->Sendf("NPC flags: &cG%s&c-\r\n", buf);
	} else {
		sprintbit(PLR_FLAGS(k), player_bits, buf);			ch->Sendf("PLR: &cG%s&c-\r\n", buf);
		sprintbit(PRF_FLAGS(k), preference_bits, buf);		ch->Sendf("PRF: &cG%s&c-\r\n", buf);
		sprintbit(STF_FLAGS(k), staff_bits, buf);			ch->Sendf("STF: &cG%s&c-\r\n", buf);
	}

	/* Showing the bitvector */
	sprintbit(AFF_FLAGS(k), affected_bits, buf);
	ch->Sendf("AFF: &cG%s&c0\r\n", buf);

	ch->Sendf("Gold/Bank: &cY%d/%d&c- (Total: &cY%d&c-)\r\n",
		GET_GOLD(k), GET_BANK_GOLD(k), GET_GOLD(k) + GET_BANK_GOLD(k));

	if (IS_MOB(k))	{
		ch->Sendf("Attack: %s, DefaultPos: %s, Specproc: %s\r\n",
			Skill::Name(k->mob->attack_type + TYPE_HIT), position_types[(int)GET_DEFAULT_POS(k)],
			Character::Index[k->Virtual()].func ? spec_mob_name(Character::Index[k->Virtual()].func) : "None");
	} else {
		ch->Sendf("Hunger: %d, Thirst: %d, Drunk: %d\r\n",
			GET_COND(k, FULL), GET_COND(k, THIRST), GET_COND(k, DRUNK));
	}

	ch->Sendf("Weight: %d, items: %d; Master is: %s\r\n",
			IS_CARRYING_W(k), k->contents.Count(),
			((k->master) ? k->master->Name() : "<none>"));


	if (k->followers.Count()) {
		ch->Send("Followers are:  ");
		LListIterator<Character *>	iter(k->followers);
		*buf = found = 0;
		while ((follower = iter.Next())) {
			sprintf(buf + strlen(buf), "%s %s", found++ ? "," : "", PERS(follower, ch));
			if (strlen(buf) >= 62) {
				ch->Sendf("%s%s\r\n", buf, iter.Peek() ? "," : "");
				*buf = found = 0;
			}
		}

		ch->Sendf("%s%s&c0", buf, found ? "\r\n" : "");
	}

	if (IS_NPC(k) && k->mob->hates) {
		Character *vict;
		SInt32		id;
		LListIterator<IDNum>	hated(k->mob->hates->charlist);

		ch->Send("Pissed List:\r\n");
		while ((id = hated.Next())) {
			vict = find_char(id);
			if (vict)	ch->Sendf("  %s\r\n", vict->Name());
		}
		ch->Send("\r\n");
	}

	/* Routine to show what spells a char is affected by */
	if (k->affected.Count()) {
		LListIterator<Affect *>		iter(k->affected);
		while ((aff = iter.Next())) {
			ch->Sendf("SKL: (%3dhr) &cc%-21s&c0",
					aff->Time() / PULSES_PER_MUD_HOUR,
//					((aff->AffType() > 0) && (aff->AffType() <= NUM_AFFECTS)) ?
						Skill::Name(aff->AffType()));
			if (aff->AffModifier())
				ch->Sendf("%+d to %s", aff->AffModifier(), apply_types[aff->AffLocation()]);
			if (aff->AffFlags()) {
				sprintbit(aff->AffFlags(), affected_bits, buf);
				ch->Sendf("%ssets %s", aff->AffModifier() ? ", " : "", buf);
			}
			ch->Send("\r\n");
		}
	}

	ch->Send("\r\n");
}


ACMD(do_stat) {
	Character *victim = 0;
	Object *object = 0;
	char	*buf1 = get_buffer(MAX_INPUT_LENGTH),
			*buf2 = get_buffer(MAX_INPUT_LENGTH);

	half_chop(argument, buf1, buf2);

	if (!*buf1)
		ch->Send("Stats on who or what?\r\n");
	else if (is_abbrev(buf1, "room"))
		if (subcmd == SCMD_STAT)	do_stat_room(ch);
		else						do_sstat_room(ch);
	else if (is_abbrev(buf1, "mob")) {
		if (!*buf2)
			ch->Send("Stats on which mobile?\r\n");
		else if (!(victim = get_char_vis(ch, buf2, FIND_CHAR_WORLD)))
			ch->Send("No such mobile around.\r\n");
	} else if (is_abbrev(buf1, "player")) {
		if (!*buf2)
			ch->Send("Stats on which player?\r\n");
		else if (!(victim = get_player_vis(ch, buf2, false)))
			ch->Send("No such player around.\r\n");
	} else if (is_abbrev(buf1, "file")) {
		if (!*buf2)
			ch->Send("Stats on which player?\r\n");
		else {
			victim = new Character();
			if (victim->Load(buf2) > -1) {
				if (IS_STAFF(victim) && !STF_FLAGGED(ch, Staff::Security | Staff::Admin))
					ch->Send("Sorry, you can't do that.\r\n");
				else
					do_stat_character(ch, victim);
			} else
				ch->Send("There is no such player.\r\n");
			delete victim;
			victim = NULL;
		}
	} else if (is_abbrev(buf1, "object")) {
		if (!*buf2)
			ch->Send("Stats on which object?\r\n");
		else if (!(object = get_obj_vis(ch, buf2)))
			ch->Send("No such object around.\r\n");
	} else {
		if ((victim = get_char_vis(ch, buf1, FIND_CHAR_WORLD)));
		else if ((object = get_obj_vis(ch, buf1)));
		else ch->Send("Nothing around by that name.\r\n");
	}
	if (object) {
		if (subcmd == SCMD_STAT)	do_stat_object(ch, object);
		else						do_sstat_object(ch, object);
	} else if (victim) {
		if (subcmd == SCMD_STAT)	do_stat_character(ch, victim);
		else						do_sstat_character(ch, victim);
	}
	release_buffer(buf1);
	release_buffer(buf2);
}


ACMD(do_shutdown) {
	char *arg = get_buffer(MAX_INPUT_LENGTH);
	one_argument(argument, arg);

	if (str_cmp(command, "shutdown")) {
		ch->Send("If you want to shut something down, say so!\r\n");
		return;
	}

	else if (!str_cmp(arg, "reboot")) {
		log("(GC) Reboot by %s.", ch->RealName());
		send_to_all("Rebooting.. come back in a minute or two.\r\n");
		touch(FASTBOOT_FILE);
		circle_shutdown = circle_reboot = 1;
	} else if (!str_cmp(arg, "now")) {
		if (!STF_FLAGGED(ch, Staff::Coder)) {
			ch->Send("Only coders may do that.\r\n");
			return;
		} else {
			log("(GC) Shutdown NOW by %s.", ch->RealName());
			send_to_all("Rebooting.. come back in a minute or two.\r\n");
			touch(FASTBOOT_FILE);
			circle_shutdown = 1;
			circle_reboot = 2;
		}
	} else if (!str_cmp(arg, "prog")) {
		if (!STF_FLAGGED(ch, Staff::Coder)) {
			ch->Send("Only coders may do that.\r\n");
			return;
		} else {
			sprintf(buf, "(GC) Programming Reboot by %s.", GET_NAME(ch));
			log(buf);
			send_to_all("Rebooting for new programming.  Come back in a minute or two!\r\n");
			touch("../.compile");
			touch("../.fastboot");
			circle_reboot = circle_shutdown = 1;
		}
	} else if (!str_cmp(arg, "ptgz")) {
		if (!STF_FLAGGED(ch, Staff::Coder)) {
			ch->Send("Only coders may do that.\r\n");
			return;
		} else {
			sprintf(buf, "(GC) Programming *tgz* Reboot by %s.", GET_NAME(ch));
			log(buf);
			send_to_all("Rebooting for new programming.. Come back in a minute or two!\r\n");
			touch("../.compile");
			touch("../.tgzboot");
			circle_reboot = circle_shutdown = 1;
		}
	} else if (!str_cmp(arg, "die")) {
		if (!STF_FLAGGED(ch, Staff::Coder)) {
			ch->Send("Only coders may do that.\r\n");
			return;
		} else {
			log("(GC) Shutdown DIE by %s.", ch->RealName());
			send_to_all("Shutting down for maintenance.\r\n");
			touch(KILLSCRIPT_FILE);
			circle_shutdown = 1;
		}
	} else if (!str_cmp(arg, "pause")) {
		if (!STF_FLAGGED(ch, Staff::Coder)) {
			ch->Send("Only coders may do that.\r\n");
			return;
		} else {
			log("(GC) Shutdown PAUSE by %s.", ch->RealName());
			send_to_all("Shutting down for maintenance.\r\n");
			touch(PAUSE_FILE);
			circle_shutdown = 1;
		}
	} else
		ch->Send("Unknown shutdown option.\r\n");
	release_buffer(arg);
}


void stop_snooping(Character * ch) {
	if (!ch->desc->snooping)
		ch->Send("You aren't snooping anyone.\r\n");
	else {
		ch->Send("You stop snooping.\r\n");
		ch->desc->snooping->snoop_by = NULL;
		ch->desc->snooping = NULL;
	}
}


ACMD(do_snoop) {
	Character *victim, *tch;
	char *arg;

	if (!ch->desc)
		return;

	arg = get_buffer(MAX_INPUT_LENGTH);
	one_argument(argument, arg);

	if (!*arg)
		stop_snooping(ch);
	else if (!(victim = get_char_vis(ch, arg, FIND_CHAR_WORLD)))
		ch->Send("No such person around.\r\n");
	else if (!victim->desc)
		ch->Send("There's no link.. nothing to snoop.\r\n");
	else if (victim == ch)
		stop_snooping(ch);
	else if (victim->desc->snoop_by)
		ch->Send("Busy already. \r\n");
	else if (victim->desc->snooping == ch->desc)
		ch->Send("Don't be stupid.\r\n");
	else {
		tch = (victim->desc->original ? victim->desc->original : victim);

		if (!ENOUGH_TRUST(ch, victim))
			ch->Send("You can't.\r\n");
		else {
			ch->Send(OK);

			if (ch->desc->snooping)
				ch->desc->snooping->snoop_by = NULL;

			ch->desc->snooping = victim->desc;
			victim->desc->snoop_by = ch->desc;
		}
	}
	release_buffer(arg);
}



ACMD(do_switch) {
	Character *victim;
	char *arg = get_buffer(MAX_INPUT_LENGTH);

	one_argument(argument, arg);

	if (ch->desc->original)
		ch->Send("You're already switched.\r\n");
	else if (!*arg)
		ch->Send("Switch with who?\r\n");
	else if (!(victim = get_char_vis(ch, arg, FIND_CHAR_WORLD)))
		ch->Send("No such character.\r\n");
	else if (ch == victim)
		ch->Send("Hee hee... we are jolly funny today, eh?\r\n");
	else if (victim->desc)
		ch->Send("You can't do that, the body is already in use!\r\n");
	else if (!ENOUGH_TRUST(ch, victim) && !IS_NPC(victim))
		ch->Send("You don't have the clearance to use a mortal's body.\r\n");
	else {
		ch->Send(OK);

		ch->desc->character = victim;
		ch->desc->original = ch;

		victim->desc = ch->desc;
		ch->desc = NULL;
	}
	release_buffer(arg);
}


ACMD(do_return) {
	if (ch->desc && ch->desc->original) {
		ch->Send("You return to your original body.\r\n");

		/* JE 2/22/95 */
		/* if someone switched into your original body, disconnect them */
		if (ch->desc->original->desc)
			STATE(ch->desc->original->desc) = CON_DISCONNECT;

		ch->desc->character = ch->desc->original;
		ch->desc->original = NULL;

		ch->desc->character->desc = ch->desc;
		ch->desc = NULL;
	}
}



ACMD(do_load) {
	Character *mob;
	Object *obj;
	IndexData<Character> *c;
	IndexData<Object> *o;
	int number = -1;
	bool	stringmode = false;

	two_arguments(argument, buf, buf2);

	if (!*buf || !*buf2) {
		ch->Send("Usage: load { obj | mob } { number | name }\r\n");
		return;
	}
	if (strstr(buf2,".") || (!isdigit(*buf2)))
		 stringmode = true;
	else {
		if ((number = atoi(buf2)) < 0) {
			ch->Send("A NEGATIVE number??\r\n");
			return;
		}
	}

	if (is_abbrev(buf, "mob")) {
		if (stringmode) {
			Index<Character>::Iterator	iter(Character::Index);
			while ((c = iter.Next()))
				if (silly_isname(buf2, c->proto->Keywords()))
					number = c->proto->Virtual();
		}

		if (!Character::Find(number))
			ch->Send("There is no monster with that number.\r\n");
		else {
			mob = Character::Read(number);
			mob->ToRoom(IN_ROOM(ch));

			act("$n draws a glowing rune in the air.", TRUE, ch, 0, 0, TO_ROOM);
			act("You draw a glowing rune in the air.", FALSE, ch, 0, 0, TO_CHAR);
			act("$N steps out of the rune!", FALSE, ch, 0, mob, TO_ROOM);
			act("$N steps out of the rune!", FALSE, ch, 0, mob, TO_CHAR);
			load_mtrigger(mob);
		}
	} else if (is_abbrev(buf, "obj")) {
		if (stringmode) {
			Index<Object>::Iterator	iter(Object::Index);
			while ((o = iter.Next()))
				if (silly_isname(buf2, o->proto->Keywords()))
					number = o->proto->Virtual();
		}

		if (!Object::Find(number))
			ch->Send("There is no object with that number.\r\n");
		else {
			obj = Object::Read(number);
			if (obj->wear)
				obj->ToChar(ch);
			else
				obj->ToRoom(IN_ROOM(ch));
			act("$n draws a glowing rune in the air.", TRUE, ch, 0, 0, TO_ROOM);
			act("You draw a glowing rune in the air.", FALSE, ch, 0, 0, TO_CHAR);
			act("$p falls out of the rune into $n's hand!", FALSE, ch, obj, 0, TO_ROOM);
			act("$p falls out of the rune into your hand.", FALSE, ch, obj, 0, TO_CHAR);
			load_otrigger(obj);
		}
	} else if ((is_abbrev(buf, "player")) && (GET_TRUST(ch) == MAX_TRUST)) {
			if (get_player_vis(ch, buf2, 0)) {
				ch->Send("That body is already in the game.\r\n");
				return;
			}

			Character *vict = new Character;

			if (vict->Load(buf2) == -1) {
				ch->Send("That character doesn't exist.\r\n");
				delete vict;
				return;
			}

			act("$n sings a rune, summoning $N from the netherworld.", FALSE, ch, NULL, vict, TO_ROOM);
			act("You sing a rune, summoning $N from the netherworld.", FALSE, ch, NULL, vict, TO_CHAR);
			vict->ToWorld();
			vict->ToRoom(IN_ROOM(ch));
			Crash_load(vict);
	} else
		ch->Send("That'll have to be either 'obj' or 'mob'.\r\n");
}



ACMD(do_vstat) {
	Character *mob;
	Object *obj;
	Trigger *trig;
	int number;
	char	*buf = get_buffer(MAX_INPUT_LENGTH),
			*buf2 = get_buffer(MAX_INPUT_LENGTH);

	two_arguments(argument, buf, buf2);

	if (!*buf || !*buf2 || !isdigit(*buf2))
		ch->Send("Usage: vstat { obj | mob | trg } <number>\r\n");
	else if ((number = atoi(buf2)) < 0)
		ch->Send("A NEGATIVE number??\r\n");
	else if (is_abbrev(buf, "mob")) {
		if (!Character::Find(number))
			ch->Send("There is no monster with that number.\r\n");
		else {
			mob = Character::Read(number);
			mob->ToRoom(0);
			do_stat_character(ch, mob);
			mob->Extract();
		}
	} else if (is_abbrev(buf, "obj")) {
		if (!Object::Find(number))
			ch->Send("There is no object with that number.\r\n");
		else {
			obj = Object::Read(number);
			do_stat_object(ch, obj);
			obj->Extract();
		}
	} else if (is_abbrev(buf, "trg")) {
		if (!Trigger::Find(number))
			ch->Send("There is no trigger with that number.\r\n");
		else {
			trig = Trigger::Read(number);
			do_stat_trigger(ch, trig);
			trig->Extract();
		}
	} else
		ch->Send("That'll have to be 'mob', 'obj', or 'trg'.\r\n");
	release_buffer(buf);
	release_buffer(buf2);
}




/* clean a room of all mobiles and objects */
ACMD(do_purge) {
	Character *vict;
	Object *obj;
	char *buf = get_buffer(MAX_INPUT_LENGTH);

	one_argument(argument, buf);

	if (*buf) {			/* argument supplied. destroy single object
						 * or char */
		if ((vict = get_char_vis(ch, buf, FIND_CHAR_ROOM))) {
			if (!IS_NPC(vict) && !ENOUGH_TRUST(ch, vict)) {
				ch->Send("Fuuuuuuuuu!\r\n");
				release_buffer(buf);
				return;
			}
			act("$n disintegrates $N.", FALSE, ch, 0, vict, TO_NOTVICT);

			if (!IS_NPC(vict)) {
				mudlogf(BRF, ch, TRUE, "(GC) %s has purged %s.", ch->RealName(), vict->RealName());
				if (vict->desc) {
					STATE(vict->desc) = CON_CLOSE;
					vict->desc->character = NULL;
					vict->desc = NULL;
				}
			}
			vict->Extract();
		} else if ((obj = get_obj_in_list_vis(ch, buf, world[IN_ROOM(ch)].contents))) {
			act("$n destroys $p.", FALSE, ch, obj, 0, TO_ROOM);
			obj->Extract();
		} else {
			ch->Send("Nothing here by that name.\r\n");
			release_buffer(buf);
			return;
		}

		ch->Send(OK);
	} else {			/* no argument. clean out the room */
		act("$n gestures... You are surrounded by scorching flames!", FALSE, ch, 0, 0, TO_ROOM);
		world[IN_ROOM(ch)].Send("The world seems a little cleaner.\r\n");

		START_ITER(ch_iter, vict, Character *, world[IN_ROOM(ch)].people)
			if (IS_NPC(vict))
				vict->Extract();

		START_ITER(obj_iter, obj, Object *, world[IN_ROOM(ch)].contents)
			obj->Extract();
	}
	release_buffer(buf);
}



ACMD(do_advance) {
	Character *victim;
	char	*name = get_buffer(MAX_STRING_LENGTH),
			*level = get_buffer(MAX_STRING_LENGTH);
	int newlevel, oldlevel;

	two_arguments(argument, name, level);

	if (!*name)
		ch->Send("Advance who?\r\n");
	else if (!(victim = get_char_vis(ch, name, FIND_CHAR_WORLD)))
		ch->Send("That player is not here.\r\n");
	else if (IS_STAFF(victim) && !STF_FLAGGED(ch, Staff::Admin))
		ch->Send("Maybe that's not such a great idea.\r\n");
	else if (IS_NPC(victim))
		ch->Send("NO!  Not on NPC's.\r\n");
	else if (!*level || (newlevel = atoi(level)) <= 0)
		ch->Send("That's not a level!\r\n");
	else if (newlevel > MAX_LEVEL)
		ch->Sendf("%d is the highest possible level.\r\n", MAX_LEVEL);
//	else if (newlevel == GET_LEVEL(ch))
//		ch->Send("Yeah, right.\r\n");
	else if (newlevel == GET_LEVEL(victim))
		ch->Send("They are already at that level.\r\n");
	else {
		oldlevel = GET_LEVEL(victim);
		if (newlevel < GET_LEVEL(victim)) {
	    	victim->Send("You are momentarily enveloped by darkness!\r\n"
						 "You feel somewhat diminished.\r\n");
		} else {
		    act("$n makes some strange gestures.\r\n"
			"A strange feeling comes upon you,\r\n"
			"Like a giant hand, light comes down\r\n"
			"from above, grabbing your body, that\r\n"
			"begins to pulse with colored lights\r\n"
			"from inside.\r\n\r\n"
			"Your head seems to be filled with voices\r\n"
			"from another plane as your body dissolves\r\n"
			"to the elements of time and space itself.\r\n"
			"Suddenly a silent explosion of light\r\n"
			"snaps you back to reality.\r\n\r\n"
			"You feel slightly different.", FALSE, ch, 0, victim, TO_VICT);
		}

		ch->Send(OK);

		mudlogf(BRF, ch, TRUE, "(GC) %s has advanced %s to level %d (from %d)", ch->RealName(), victim->RealName(), newlevel, oldlevel);
		victim->SetLevel(newlevel);
		victim->Save(victim->AbsoluteRoom());
	}
	release_buffer(name);
	release_buffer(level);
}



ACMD(do_restore) {
	Character *vict;
	Descriptor *d;
	int i;
	char *buf = get_buffer(MAX_INPUT_LENGTH);

	one_argument(argument, buf);

	if (!*buf)
		ch->Send("Whom do you wish to restore?\r\n");
	else if (!str_cmp(buf, "all")) {
		mudlogf(NRM, ch, TRUE, "(GC) %s restores all", ch->RealName());
		START_ITER(iter, d, Descriptor *, descriptor_list) {
			if ((vict = d->character) && (vict != ch)) {
				GET_HIT(vict) = GET_MAX_HIT(vict);
				GET_MOVE(vict) = GET_MAX_MOVE(vict);

				vict->UpdatePos();
				act("You have been fully healed by $N!", FALSE, vict, 0, ch, TO_CHAR);
			}
		}
	} else if (!(vict = get_char_vis(ch, buf, FIND_CHAR_WORLD)))
		ch->Send(NOPERSON);
	else {
		GET_HIT(vict) = GET_MAX_HIT(vict);
		GET_MANA(vict) = GET_MAX_MANA(vict);
		GET_MOVE(vict) = GET_MAX_MOVE(vict);

#ifdef GOT_RID_OF_IT
		if (IS_STAFF(vict)) {
			for (i = 1; i <= MAX_SKILLS; i++)
				SET_SKILL(vict, i, 100);

			vict->real_abils.Co = 200;
			vict->real_abils.Ag = 200;
			vict->real_abils.SD = 200;
			vict->real_abils.Me = 200;
			vict->real_abils.Re = 200;
			vict->real_abils.St = 200;
			vict->real_abils.Qu = 200;
			vict->real_abils.Pr = 200;
			vict->real_abils.In = 200;
			vict->real_abils.Em = 200;

			vict->aff_abils = vict->real_abils;
		}
#endif
		vict->UpdatePos();
		ch->Send(OK);
		act("You have been fully healed by $N!", FALSE, vict, 0, ch, TO_CHAR);
	}
	release_buffer(buf);
}


void perform_immort_vis(Character *ch) {
	if (!PRF_FLAGGED(ch, Preference::StaffInvis | Preference::AdminInvis) &&
		!AFF_FLAGGED(ch, AffectBit::Hide | AffectBit::Invisible) &&
		!GET_STAFF_INVIS(ch)) {
		ch->Send("You are already fully visible.\r\n");
		return;
	}

	ch->Appear();
	ch->Send("You are now fully visible.\r\n");
}


void perform_immort_invis(Character *ch, int level)
{
	Character *tch;

	if (IS_NPC(ch))
		return;

	START_ITER(iter, tch, Character *, world[IN_ROOM(ch)].people) {
		if (tch == ch)
			continue;
		if (GET_TRUST(tch) >= GET_STAFF_INVIS(ch) && GET_TRUST(tch) < level)
			act("You blink and suddenly realize that $n is gone.", FALSE, ch, 0, tch, TO_VICT);
		if (GET_TRUST(tch) < GET_STAFF_INVIS(ch) && GET_TRUST(tch) >= level)
			act("You suddenly realize that $n is standing beside you.", FALSE, ch, 0, tch, TO_VICT);
	}

	if (level == 1)		sprintf(buf, "You are now invisible to mortals.\r\n");
	else				sprintf(buf, "Your invisibility level is %d.\r\n", level);
	ch->Send(buf);
	GET_STAFF_INVIS(ch) = level;
}


ACMD(do_invis) {
	char *arg;
	int level;

	if (IS_NPC(ch)) {
		ch->Send("You can't do that!\r\n");
		return;
	}

	arg = get_buffer(MAX_INPUT_LENGTH);
	one_argument(argument, arg);
	if (!*arg) {
		if (GET_STAFF_INVIS(ch) > 0)	perform_immort_vis(ch);
		else							perform_immort_invis(ch, 1);
	} else {
		level = atoi(arg);
		if (level > GET_TRUST(ch))		ch->Send("You can't go invisible at that level.\r\n");
		else if (level <= 0)			perform_immort_vis(ch);
		else							perform_immort_invis(ch, level);
	}
	release_buffer(arg);
}


ACMD(do_gecho) {
//	Descriptor *pt;

	skip_spaces(argument);
	delete_doubledollar(argument);

	if (!*argument)
		ch->Send("That must be a mistake...\r\n");
	else {
		send_to_playersf(ch, "%s\r\n", argument);
		if (PRF_FLAGGED(ch, Preference::NoRepeat))	ch->Send(OK);
		else										ch->Sendf("%s\r\n", argument);
	}
}


ACMD(do_poof)
{
	act("Your poofin is:", FALSE, ch, NULL, NULL, TO_CHAR);
	if (POOFIN(ch))
		act(POOFIN(ch), FALSE, ch, NULL, NULL, TO_CHAR);
	else
		act("$n appears with an ear-splitting bang.",
	FALSE, ch, NULL, NULL, TO_CHAR);

	act("\r\nYour poofout is:", FALSE, ch, NULL, NULL, TO_CHAR);
	if (POOFOUT(ch))
		act(POOFOUT(ch), FALSE, ch, NULL, NULL, TO_CHAR);
	else
		act("$n disappears in a puff of smoke.", TRUE, ch, NULL, NULL, TO_CHAR);
}


ACMD(do_poofset)
{
	char **msg;

	switch (subcmd) {
		case SCMD_POOFIN:		msg = &(POOFIN(ch));		break;
		case SCMD_POOFOUT:	 msg = &(POOFOUT(ch));	 break;
		default:		return;		break;
	}

	skip_spaces(argument);
	delete_doubledollar(argument);

	if (*msg)
		free(*msg);

	if (!*argument)
		*msg = NULL;

	else if (!strstr(argument, "$n") && (GET_TRUST(ch) < TRUST_GRGOD)) {
		CREATE(*msg, char, strlen(argument) + 4);
		sprintf(*msg, "$n %s", argument);
	}

	else
		*msg = str_dup(argument);

	if ((subcmd == SCMD_POOFIN) && *msg)
		act("Poofin set to:", FALSE, ch, 0, 0, TO_CHAR);
	else if (*msg)
		act("Poofout set to:", FALSE, ch, 0, 0, TO_CHAR);

	if (*msg)
		act(*msg, FALSE, ch, 0, 0, TO_CHAR);
	else
	ch->Send(OK);
}


ACMD(do_dc) {
	Descriptor *d;
	int num_to_dc;
	char *arg = get_buffer(MAX_INPUT_LENGTH);

	one_argument(argument, arg);
	if (!(num_to_dc = atoi(arg))) {
		ch->Send("Usage: DC <user number> (type USERS for a list)\r\n");
		release_buffer(arg);
		return;
	}
	release_buffer(arg);

	START_ITER(iter, d, Descriptor *, descriptor_list) {
		if (d->desc_num == num_to_dc)
			break;
	}

	if (!d) {
		ch->Send("No such connection.\r\n");
		return;
	}
	if (d->character && !ENOUGH_TRUST(ch, d->character)) {
//		ch->Send("Umm.. maybe that's not such a good idea...\r\n");
		if (!d->character->CanBeSeen(ch))
			ch->Send("No such connection.\r\n");
		else
			ch->Send("Umm.. maybe that's not such a good idea...\r\n");
		return;
	}
	/* We used to just close the socket here using close_socket(), but
	 * various people pointed out this could cause a crash if you're
	 * closing the person below you on the descriptor list.  Just setting
	 * to CON_CLOSE leaves things in a massively inconsistent state so I
	 * had to add this new flag to the descriptor.
	 */
	STATE(d) = CON_DISCONNECT;
	ch->Sendf("Connection #%d closed.\r\n", num_to_dc);
	log("(GC) Connection closed by %s.", ch->RealName());
}



ACMD(do_wizlock) {
	int value;
	const char *when;
	char *buf = get_buffer(MAX_INPUT_LENGTH);

	one_argument(argument, buf);
	if (*buf) {
		value = atoi(buf);
		if (value < 0 || value > MAX_LEVEL) {
			ch->Send("Invalid wizlock value.\r\n");
			release_buffer(buf);
			return;
		}
		circle_restrict = value;
		when = "now";
	} else
		when = "currently";

	release_buffer(buf);

	switch (circle_restrict) {
		case 0:
			ch->Sendf("The game is %s completely open.\r\n", when);
			break;
		case 1:
			ch->Sendf("The game is %s closed to new players.\r\n", when);
			break;
		default:
			ch->Sendf("Only level %d and above may enter the game %s.\r\n", circle_restrict, when);
			break;
	}
}


ACMD(do_date) {
	char *tmstr;
	time_t mytime;
	int d, h, m;

	if (subcmd == SCMD_DATE)
		mytime = time(0);
	else
		mytime = boot_time;

	tmstr = asctime(localtime(&mytime));
	*(tmstr + strlen(tmstr) - 1) = '\0';

	if (subcmd == SCMD_DATE)
		ch->Sendf("Current machine time: %s\r\n", tmstr);
	else {
		mytime = time(0) - boot_time;
		d = mytime / 86400;
		h = (mytime / 3600) % 24;
		m = (mytime / 60) % 60;

		ch->Sendf("Up since %s: %d day%s, %d:%02d\r\n", tmstr, d, ((d == 1) ? "" : "s"), h, m);
	}
}



ACMD(do_last) {
	char *buf = get_buffer(MAX_INPUT_LENGTH);
 	Character *tempchar = new Character();

	one_argument(argument, buf);
	if (!*buf)
		ch->Send("For whom do you wish to search?\r\n");
	else if (tempchar->Load(buf) < 0)
		ch->Send("There is no such player.\r\n");
	else {
		ch->Sendf("[%5d] [%2d %s] %-12s : %-18s : %-20s\r\n",
				GET_IDNUM(tempchar), GET_LEVEL(tempchar),
				race_abbrevs[(int) GET_RACE(tempchar)], tempchar->Name(), tempchar->player->host,
				ctime(&tempchar->player->time.logon));
	}
	delete tempchar;
	release_buffer(buf);
}


ACMD(do_force) {
	Descriptor *i;
	Character *vict;
	char	*to_force = get_buffer(MAX_INPUT_LENGTH + 2),
			*arg = get_buffer(MAX_INPUT_LENGTH),
			*buf = get_buffer(SMALL_BUFSIZE),
			*buf1 = get_buffer(SMALL_BUFSIZE);

	half_chop(argument, arg, to_force);

	sprintf(buf1, "$n has forced you to '%s'.", to_force);

	if (!*arg || !*to_force)
		ch->Send("Whom do you wish to force do what?\r\n");
	else if ((str_cmp("all", arg) && str_cmp("room", arg))) {
		if (!(vict = get_char_vis(ch, arg, FIND_CHAR_WORLD)))
			ch->Send(NOPERSON);
		else if (!IS_NPC(vict) && !ENOUGH_TRUST(ch, vict))
			ch->Send("No, no, no!\r\n");
		else {
			ch->Send(OK);
			act(buf1, TRUE, ch, NULL, vict, TO_VICT);
			mudlogf(NRM, ch, TRUE, "(GC) %s forced %s to %s", ch->RealName(), vict->RealName(), to_force);
			command_interpreter(vict, to_force);
		}
	} else if (!str_cmp("room", arg)) {
		ch->Send(OK);
		mudlogf(NRM, ch, TRUE, "(GC) %s forced room %d to %s", ch->RealName(), world[IN_ROOM(ch)].Virtual(), to_force);

		START_ITER(iter, vict, Character *, world[IN_ROOM(ch)].people) {
			if ((IS_STAFF(vict) && !STF_FLAGGED(ch, Staff::Admin)) || STF_FLAGGED(vict, Staff::Admin))
				continue;
			act(buf1, TRUE, ch, NULL, vict, TO_VICT);
			command_interpreter(vict, to_force);
		}
	} else { /* force all */
		ch->Send(OK);
		mudlogf(NRM, ch, TRUE, "(GC) %s forced all to %s", ch->RealName(), to_force);

		START_ITER(iter, i, Descriptor *, descriptor_list) {
			if ((STATE(i) != CON_PLAYING) || !(vict = i->character) || (vict == ch) || STF_FLAGGED(vict, Staff::Admin))
				continue;
			if (IS_STAFF(vict) && !STF_FLAGGED(ch, Staff::Admin))
				continue;
			act(buf1, TRUE, ch, NULL, vict, TO_VICT);
			command_interpreter(vict, to_force);
		}
	}
	release_buffer(to_force);
	release_buffer(buf);
	release_buffer(buf1);
	release_buffer(arg);
}



ACMD(do_wiznet) {
	Descriptor *d;
	bool			any = false,
					emote = false,
					admin = false;
	LListIterator<Descriptor *>	iter(descriptor_list);

	skip_spaces(argument);
	delete_doubledollar(argument);

	if (!*argument) {
		ch->Send("Usage: think <text> | # <text> | *<emotetext> |\r\n "
				 "       think @<level> *<emotetext> | think @\r\n");
		return;
	}

	switch (*argument) {
		case '*':
			emote = true;
			++argument;
			break;
		case '#':
			admin = true;
			++argument;
			break;
		case '@':
			while ((d = iter.Next())) {
				if ((STATE(d) == CON_PLAYING) && (STF_FLAGGED(d->character, Staff::Wiznet) &&
						!CHN_FLAGGED(d->character, Channel::NoWiz)) && (d->character->CanBeSeen(ch)
						|| STF_FLAGGED(ch, Staff::Coder))) {
					if (!any) {
						ch->Send("Gods online:\r\n");
						any = true;
					}
					ch->Sendf("  %s", d->character->Name());
					if (PLR_FLAGGED(d->character, PLR_WRITING))			ch->Send(" (Writing)");
					else if (PLR_FLAGGED(d->character, PLR_MAILING))	ch->Send(" (Writing mail)");
					ch->Send("\r\n");
				}
			}
			any = false;
			iter.Reset();
			while ((d = iter.Next())) {
				if ((STATE(d) == CON_PLAYING) && (STF_FLAGGED(d->character, Staff::Wiznet) &&
						CHN_FLAGGED(d->character, Channel::NoWiz)) && d->character->CanBeSeen(ch)) {
					if (!any) {
						ch->Send("Gods offline:\r\n");
						any = true;
					}
					ch->Sendf("  %s\r\n", d->character->Name());
				}
			}
			return;
		case '\\':
			++argument;
			break;
		default:
			break;
	}
	if (CHN_FLAGGED(ch, Channel::NoWiz)) {
		ch->Send("You are offline!\r\n");
		return;
	}
	skip_spaces(argument);

	if (!*argument) {
		ch->Send("Don't bother the staff like that!\r\n");
		return;
	}

	while ((d = iter.Next())) {
		if ((STATE(d) == CON_PLAYING) && STF_FLAGGED(d->character, Staff::Wiznet) &&
				(!admin || STF_FLAGGED(d->character, Staff::Admin)) &&
				!CHN_FLAGGED(d->character, Channel::NoWiz) &&
				!PLR_FLAGGED(d->character, PLR_WRITING | PLR_MAILING)
				&& ((d != ch->desc) || !PRF_FLAGGED(d->character, Preference::NoRepeat))) {
			if (ch->CanBeSeen(d->character))	d->Writef("&cC%s: ", ch->Name());
			else							d->Write("&cCSomeone: ");
			d->Writef("%s%s%s\r\n&c0", admin ? "<Admin> " : "", emote ? "<--- " : "", argument);
		}
	}

	if (PRF_FLAGGED(ch, Preference::NoRepeat))
		ch->Send(OK);
}



ACMD(do_zreset) {
	int i, j;
	char *arg = get_buffer(MAX_INPUT_LENGTH);

	one_argument(argument, arg);
	if (!*arg) {
		ch->Send("You must specify a zone.\r\n");
		release_buffer(arg);
		return;
	}
	if (*arg == '*') {
		for (i = 0; i <= top_of_zone_table; i++)
			reset_zone(i);
			ch->Send("Reset world.\r\n");
			mudlogf(NRM, ch, TRUE, "(GC) %s reset entire world.", ch->RealName());
			release_buffer(arg);
			return;
	} else if (*arg == '.')
		i = world[IN_ROOM(ch)].zone;
	else {
		j = atoi(arg);
		for (i = 0; i <= top_of_zone_table; i++)
			if (zone_table[i].number == j)
				break;
	}
	if (i >= 0 && i <= top_of_zone_table) {
		reset_zone(i);
		ch->Sendf("Reset zone %d (#%d): %s.\r\n", i, zone_table[i].number, zone_table[i].name);
		mudlogf(NRM, ch, TRUE, "(GC) %s reset zone %d (%s)", ch->RealName(), zone_table[i].number, zone_table[i].name);
	} else
		ch->Send("Invalid zone number.\r\n");

	release_buffer(arg);
}


/*
 *  General fn for wizcommands of the sort: cmd <player>
 */

ACMD(do_wizutil) {
	Character *vict;
	SInt32 result;
	char *arg = get_buffer(MAX_INPUT_LENGTH);

	one_argument(argument, arg);

	if (!*arg)
		ch->Send("Yes, but for whom?!?\r\n");
	else if (!(vict = get_char_vis(ch, arg, FIND_CHAR_WORLD)))
		ch->Send("There is no such player.\r\n");
	else if (IS_NPC(vict))
		ch->Send("You can't do that to a mob!\r\n");
	else if ((IS_STAFF(vict) && !STF_FLAGGED(ch, Staff::Admin) || GET_TRUST(vict) > GET_TRUST(ch)))
		ch->Send("Hmmm...you'd better not.\r\n");
	else {
		switch (subcmd) {
			case SCMD_REROLL:
//				ch->Send("Rerolled...\r\n");
//				roll_real_abils(vict);
//				log("(GC) %s has rerolled %s.", ch->RealName(), vict->RealName());
//				ch->Sendf("New stats: Str %d, Int %d, Per %d, Agl %d, Con %d\r\n",
//						GET_STR(vict), GET_INT(vict), GET_PER(vict),
//						GET_AGI(vict), GET_CON(vict));
				break;
			case SCMD_PARDON:
				if (!PLR_FLAGGED(vict, PLR_KILLER | PLR_THIEF | PLR_JAILED)) {
					ch->Send("Your victim is not flagged.\r\n");
					release_buffer(arg);
					return;
				}
				else if (ch == vict) {
					ch->Send("You cannot pardon yourself!\r\n");
					release_buffer(arg);
					return;
				}
				if (PLR_FLAGGED(vict, PLR_JAILED)) {
					//if (GET_JAILED_LEV(vict) > GET_TRUST(ch)) {
					//	ch->Sendf("Sorry, you are not authorized to pardon %s.\r\n",
					//		vict->RealName());
					//	release_buffer(arg);
					//	return;
					//}
					REMOVE_BIT(PLR_FLAGS(vict), PLR_KILLER | PLR_THIEF | PLR_JAILED);
					vict->Send("You have been pardoned by the Gods!\r\n");

					extern SInt16 r_mortal_start_room;
					act("$n disappears.", TRUE, vict, 0, 0, TO_ROOM);
					vict->FromRoom();
					vict->ToRoom(r_mortal_start_room);
					act("$n appears in the middle of the room.", TRUE, vict, 0, 0, TO_ROOM);
						if (!IS_NPC(vict))
					look_at_room(vict, 0);
					ch->Sendf("%s pardoned.\r\n", vict->RealName());
					mudlogf(BRF, ch, TRUE,  "(GC) %s pardoned by %s", vict->RealName(), ch->RealName());
					break;
				}
				else {
					REMOVE_BIT(PLR_FLAGS(vict), PLR_KILLER | PLR_THIEF | PLR_JAILED);
				ch->Sendf("%s pardoned.\r\n", vict->RealName());
				vict->Send("You have been pardoned by the Gods!\r\n");
				mudlogf(BRF, ch, TRUE,  "(GC) %s pardoned by %s", vict->RealName(), ch->RealName());
				break;
				}
			case SCMD_NOTITLE:
				result = PLR_TOG_CHK(vict, PLR_NOTITLE);
				mudlogf(NRM, ch, TRUE, "(GC) Notitle %s for %s by %s.", ONOFF(result), vict->RealName(), ch->RealName());
				ch->Sendf("(GC) Notitle %s for %s.\r\n", ONOFF(result), vict->RealName());
				break;
			case SCMD_SQUELCH:
				result = PLR_TOG_CHK(vict, PLR_NOSHOUT);
				mudlogf(BRF, ch, TRUE, "(GC) Squelch %s for %s by %s.", ONOFF(result), vict->RealName(), ch->RealName());
				ch->Sendf("(GC) Squelch %s for %s by %s\r\n.", ONOFF(result), vict->RealName(), ch->RealName());
				break;
			case SCMD_FREEZE:
				if (ch == vict) {
					ch->Send("Oh, yeah, THAT'S real smart...\r\n");
					release_buffer(arg);
					return;
				}
				if (PLR_FLAGGED(vict, PLR_FROZEN)) {
					ch->Send("Your victim is already pretty cold.\r\n");
					release_buffer(arg);
					return;
				}
				SET_BIT(PLR_FLAGS(vict), PLR_FROZEN);
//				GET_FREEZE_LEV(vict) = GET_TRUST(ch);
				vict->Send("A bitter wind suddenly rises and drains every bit of heat from your body!\r\nYou feel frozen!\r\n");
				ch->Send("Frozen.\r\n");
				act("A sudden cold wind conjured from nowhere freezes $n!", FALSE, vict, 0, 0, TO_ROOM);
				mudlogf(BRF, ch, TRUE,  "(GC) %s frozen by %s.", vict->RealName(), ch->RealName());
				break;
			case SCMD_THAW:
				if (!PLR_FLAGGED(vict, PLR_FROZEN)) {
					ch->Send("Sorry, your victim is not morbidly encased in ice at the moment.\r\n");
					release_buffer(arg);
					return;
				}
//				if (GET_FREEZE_LEV(vict) > GET_TRUST(ch)) {
//					ch->Sendf("Sorry, you are not authorized to unfreeze %s.\r\n",
//							vict->RealName());
//					release_buffer(arg);
//					return;
//				}
				mudlogf(BRF, ch, TRUE,  "(GC) %s un-frozen by %s.", vict->RealName(), ch->RealName());
				REMOVE_BIT(PLR_FLAGS(vict), PLR_FROZEN);
				ch->Send("Thawed.\r\n");
				act("$N turns up the heat, thawing you out.", FALSE, vict, 0, ch, TO_CHAR);
				act("A sudden fireball conjured from nowhere thaws $n!", FALSE, vict, 0, 0, TO_ROOM);
				break;
			case SCMD_UNAFFECT:
//				if (vict->affected) {
				{
				Affect *aff;
				START_ITER(iter, aff, Affect *, vict->affected)
					aff->Remove(vict);
				}
				vict->Send("There is a brief flash of light!\r\n"
						   "You feel slightly different.\r\n");
				ch->Send("All spells removed.\r\n");
				CheckRegenRates(vict);
//				} else {
//					ch->Send("Your victim does not have any affections!\r\n");
//					release_buffer(arg);
//					return;
//				}
				break;
			case SCMD_IMMORT:
				if (IS_IMMORTAL(vict) || IS_STAFF(vict) || IS_DEITY(vict)) {
					ch->Send("They are already an immortal.\r\n");
					release_buffer(arg);
					return;
				}
				GET_MORTALITY(vict) = MORTALITY_IMMORTAL;
				mudlogf(BRF, ch, TRUE,  "(GC) %s immorted by %s.", vict->RealName(), ch->RealName());
				act("You touch $N and confer immortality upon $M.", FALSE, ch, 0, vict, TO_CHAR);
				act("$n touches you and you feel unimaginable power run though your veins.", FALSE, ch, 0, vict, TO_VICT);
				act("$n touches $N and there is a look of sheer power in $N's eyes.", FALSE, ch, 0, vict, TO_NOTVICT);
				break;
			case SCMD_JAIL:
				if (PLR_FLAGGED(vict, PLR_JAILED)) {
					ch->Send("That person is already in jail.\r\n");
					release_buffer(arg);
					return;
				}
				else if (ch == vict) {
					ch->Send("It might not be a good idea to jail yourself.\r\n");
					release_buffer(arg);
					return;
				}
				SET_BIT(PLR_FLAGS(vict), PLR_JAILED);
//				GET_JAILED_LEV(vict) = GET_TRUST(ch);
				vict->Send("You fall asleep and awake in a cold, lonely room.\r\n");
				ch->Send("Jailed.\r\n");
				act("$n falls suddenly asleep and disappears before your eyes!", FALSE, vict, 0, 0, TO_ROOM);
					extern SInt16 jailed_start_room;
					vict->FromRoom();
					vict->ToRoom(jailed_start_room);
					act("$n appears in the middle of the room.", TRUE, vict, 0, 0, TO_ROOM);
						if (!IS_NPC(vict))
					look_at_room(vict, 0);
				vict->Send("You are in jail!\r\n");
				mudlogf(BRF, ch, TRUE,  "(GC) %s jailed by %s.", vict->RealName(), ch->RealName());
				break;
			case SCMD_STAFF:
				if (IS_STAFF(vict)) {
					ch->Send("They are already part of the staff.\r\n");
					release_buffer(arg);
					return;
				}
				GET_TRUST(vict) = 1;
				GET_MORTALITY(vict) = MORTALITY_STAFF;
				SET_BIT(STF_FLAGS(vict), Staff::General);
				SET_BIT(STF_FLAGS(vict), Staff::Wiznet);
				SET_BIT(PRF_FLAGS(vict), Preference::Invuln);
				SET_BIT(PRF_FLAGS(vict), Preference::HolyLight);
				vict->SetKeywords(vict->Name());	// This is done to make immortals visible to everyone.
				vict->SetSDesc(vict->Name());
				mudlogf(BRF, ch, TRUE,  "(GC) %s staffed by %s.", vict->RealName(), ch->RealName());
				act("You touch $N and add $M to the staff.", FALSE, ch, 0, vict, TO_CHAR);
				act("$n touches you, recruiting you to the staff.", FALSE, ch, 0, vict, TO_VICT);
				act("$n touches $N, recruiting $N to the staff.", FALSE, ch, 0, vict, TO_NOTVICT);
				break;

			default:
				log("SYSERR: Unknown subcmd %d passed to do_wizutil (" __FILE__ ")", subcmd);
				break;
		}
		vict->Save(vict->AbsoluteRoom());
	}
	release_buffer(arg);
}


/* single zone printing fn used by "show zone" so it's not repeated in the
   code 3 times ... -je, 4/6/93 */

void print_zone_to_buf(char *bufptr, int zone) {
	sprintf(bufptr + strlen(bufptr), "%3d %-30.30s Age: %3d; Reset: %3d (%1d); Top: %5d\r\n",
			zone_table[zone].number, zone_table[zone].name,
			zone_table[zone].age, zone_table[zone].lifespan,
			zone_table[zone].reset_mode, zone_table[zone].top);
}


void show_deleted(Character * ch) {
	UInt32	found;
	char	*buf = get_buffer(MAX_STRING_LENGTH);

	found = 0;
	Index<Character>::Iterator	charIter(Character::Index);
	IndexData<Character> *c;
	while ((c = charIter.Next())) {			//	List mobs
		if (MOB_FLAGGED(c->proto, MOB_DELETED)) {
			if (!found)	strcat(buf, "Deleted Mobiles:\r\n");
			sprintf(buf + strlen(buf), "&cc%3d.  &c0[&cy%5d&c0] %s\r\n",
					++found, c->vnum, c->proto->SDesc());
		}
	}

	found = 0;
	Index<Object>::Iterator	objIter(Object::Index);
	IndexData<Object> *o;
	while ((o = objIter.Next())) {				//	List objs
		if (OBJ_FLAGGED(o->proto, ITEM_DELETED)) {
			if (!found)	strcat(buf, "\r\nDeleted Objects:\r\n");
			sprintf(buf + strlen(buf), "&cc%3d.  &c0[&cy%5d&c0] %s\r\n", ++found, o->vnum, o->proto->Name());
		}
	}

	found = 0;
	Map<VNum, Room>::Iterator	roomIter(world);
	Room *r;
	while ((r = roomIter.Next())) {				//	List rooms
		if (IS_SET(r->RoomFlags(), ROOM_DELETED)) {
			if (!found)	strcat(buf, "\r\nDeleted Rooms:\r\n");
			sprintf(buf + strlen(buf), "&cc%3d.  &c0[&cy%5d&c0] %s\r\n", ++found, r->Virtual(), r->Name("<ERROR>"));
		}
	}
//	strcat(buf, "\r\n");
	page_string(ch->desc, buf, true);

	release_buffer(buf);
}


ACMD(do_show) {
	int j, k, l, con;
	SInt32	i;
	char self = 0;
	Character *vict;
	char *file, *field, *value, *birth, *arg, *buf;
//	extern LList<SString *>	SStrings;
	LListIterator<Character *>	iter(Characters);
	Map<VNum, Room>::Iterator	roomIter(world);
	Room *r;

	struct show_struct {
		const char *	cmd;
		const SInt32	level;
	} fields[] = {
		{ "nothing"	, 0				},	//	0
		{ "zones"	, TRUST_IMMORTAL		},	//	1
		{ "player"	, TRUST_GOD				},
		{ "rent"	, TRUST_IMMORTAL		},
		{ "stats"	, TRUST_IMMORTAL		},
		{ "errors"	, TRUST_IMMORTAL		},	//	5
		{ "death"	, TRUST_IMMORTAL		},
		{ "godrooms", TRUST_IMMORTAL		},
		{ "shops"	, TRUST_IMMORTAL		},
		{ "houses"	, TRUST_IMMORTAL		},
		{ "deleted"	, TRUST_GOD				},	//	10
		{ "buffers"	, MAX_TRUST				},
		{ "file"	, TRUST_GOD				},
		{ "\n", 0 }
	};

	skip_spaces(argument);

	if (!*argument) {
		ch->Send("Show options:\r\n");
		for (j = 0, i = 1; fields[i].level; i++)
			if (fields[i].level <= GET_LEVEL(ch))
				ch->Sendf("%-15s%s", fields[i].cmd, (!(++j % 5) ? "\r\n" : ""));
		ch->Send("\r\n");
		return;
	}

	arg = get_buffer(MAX_INPUT_LENGTH);
	field = get_buffer(MAX_INPUT_LENGTH);
	value = get_buffer(MAX_INPUT_LENGTH);

	strcpy(arg, two_arguments(argument, field, value));

	for (l = 0; *(fields[l].cmd) != '\n'; l++)
		if (!strncmp(field, fields[l].cmd, strlen(field)))
			break;

	if (GET_LEVEL(ch) < fields[l].level) {
		ch->Send("You are not godly enough for that!\r\n");
		release_buffer(field);
		release_buffer(value);
		release_buffer(arg);
		return;
	}
	if (!strcmp(value, "."))
		self = 1;

	switch (l) {
		case 1:			/* zone */
			buf = get_buffer(MAX_STRING_LENGTH);
			/* tightened up by JE 4/6/93 */
			if (self)
				print_zone_to_buf(buf, world[IN_ROOM(ch)].zone);
			else if (*value && is_number(value)) {
				for (j = atoi(value), i = 0; zone_table[i].number != j && i <= top_of_zone_table; i++);
					if (i <= top_of_zone_table)
						print_zone_to_buf(buf, i);
					else {
						ch->Send("That is not a valid zone.\r\n");
						release_buffer(buf);
						break;
					}
			} else
				for (i = 0; i <= top_of_zone_table; i++)
					print_zone_to_buf(buf, i);
			page_string(ch->desc, buf, true);
			release_buffer(buf);
			break;
		case 2:			/* player */
			vict = new Character();
			if (!*value)
				ch->Send("A name would help.\r\n");
			else if (vict->Load(value) < 0)
				ch->Send("There is no such player.\r\n");
			else {
				ch->Sendf(	"Player: %-12s (%s) [%2d %s]\r\n"
							"Pracs: %-3d\r\n",
						vict->RealName(), genders[GET_SEX(vict)], GET_LEVEL(vict), race_abbrevs[GET_RACE(vict)],
						GET_PRACTICES(vict));
				birth = get_buffer(80);
				strcpy(birth, ctime(&vict->player->time.birth));
				ch->Sendf("Started: %-20.16s  Last: %-20.16s  Played: %3dh %2dm\r\n",
						birth, ctime(&vict->player->time.logon), (int) (vict->player->time.played / 3600),
						(int) (vict->player->time.played / 60 % 60));
				release_buffer(birth);
			}
			delete vict;
			break;
		case 3:
			Crash_listrent(ch, value);
			break;
		case 4:
			i = j = con = 0;
			while ((vict = iter.Next())) {
				if (IS_NPC(vict))
					j++;
				else if (vict->CanBeSeen(ch)) {
					i++;
					if (vict->desc)
						con++;
				}
			}
			ch->Sendf(	"Current stats:\r\n"
						"  %5d players in game  %5d connected\r\n"
						"  %5d registered\r\n"
						"  %5d mobiles          %5d prototypes\r\n"
						"  %5d objects          %5d prototypes\r\n"
						"  %5d triggers         %5d prototypes\r\n"
						"  %5d rooms            %5d zones\r\n"
//						"  %5d shared strings\r\n"
						"  %5d large bufs\r\n"
						"  %5d buf switches     %5d overflows\r\n",

					i, con,
					player_table.Count(),
					j, Character::Index.Count(),
					Objects.Count(), Object::Index.Count(),
					Triggers.Count(), Trigger::Index.Count(),
					world.Count(), top_of_zone_table + 1,
//					SStrings.Count(),
					buf_largecount,
					buf_switches, buf_overflows);
			break;
		case 5:
			buf = get_buffer(MAX_STRING_LENGTH);
			strcpy(buf, "Errant Rooms\r\n------------\r\n");
			k = 0;
			while ((r = roomIter.Next()))
				for (j = 0; j < NUM_OF_DIRS; j++)
					if (r->Direction(j) && r->Direction(j)->to_room == 0)
						sprintf(buf + strlen(buf), "%2d: [%5d] %s\r\n", ++k, r->Virtual(), r->Name("<ERROR>"));
			page_string(ch->desc, buf, true);
			release_buffer(buf);
			break;
		case 6:
			buf = get_buffer(MAX_STRING_LENGTH);
			strcpy(buf, "Death Traps\r\n-----------\r\n");
			j = 0;
			while ((r = roomIter.Next()))
				if (IS_SET(r->RoomFlags(), ROOM_DEATH))
					sprintf(buf + strlen(buf), "%2d: [%5d] %s\r\n", ++j, r->Virtual(), r->Name("<ERROR>"));
			page_string(ch->desc, buf, true);
			release_buffer(buf);
			break;
		case 7:
			buf = get_buffer(MAX_STRING_LENGTH);
			strcpy(buf, "Godrooms\r\n--------------------------\r\n");
			j = 0;
			while ((r = roomIter.Next()))
				if (IS_SET(r->RoomFlags(), ROOM_GODROOM | ROOM_GRGODROOM))
					sprintf(buf + strlen(buf), "%2d: [%5d] %s\r\n", j++, r->Virtual(), r->Name("<ERROR>"));
			page_string(ch->desc, buf, true);
			release_buffer(buf);
			break;
		case 8:
			show_shops(ch, value);
			break;
		case 9:
			hcontrol_list_houses(ch);
			break;
		case 10:
			show_deleted(ch);
			break;
		case 11:
			show_buffers(ch, static_cast<Buffer::Type>(-1), 1);
			show_buffers(ch, static_cast<Buffer::Type>(-1), 2);
			show_buffers(ch, static_cast<Buffer::Type>(-1), 0);
			break;
		case 12:
			if (!value || !*value) {
				if (GET_TRUST(ch) < TRUST_GOD)
					ch->Send("Usage: show file [ ideas | typos ]\r\n");
				else
					ch->Send("Usage: show file [ bugs | ideas | typos ]\r\n");
				break;
			}

			i = strlen(value);

			if (!strn_cmp(value, "bugs", i) && (GET_TRUST(ch) >= TRUST_GOD))
				file = BUG_FILE;
			else if (!strn_cmp(value, "ideas", i))
				file = IDEA_FILE;
			else if (!strn_cmp(value, "typos", i))
				file = TYPO_FILE;
			else {
				ch->Send("That is not a valid file.\r\n");
				break;
			}

			buf = get_buffer(MAX_STRING_LENGTH * 2);
			if (file_to_string(file, buf) == -1)
				ch->Send("Sorry, an error occurred while reading that file.\r\n");
			else
				page_string(ch->desc, buf, 1);
			release_buffer(buf);
			break;

		default:
			ch->Send("Sorry, I don't understand that.\r\n");
			break;
	}
	release_buffer(field);
	release_buffer(value);
	release_buffer(arg);
}


#define SET_OR_REMOVE(flagset, flags) { \
	if (on) SET_BIT(flagset, flags); \
	else if (off) REMOVE_BIT(flagset, flags); }


enum SetAllowedOn	{ PC = (1 << 0), NPC = (1 << 1), BOTH = (PC | NPC) };
enum SetType		{ MISC, BINARY, NUMBER };
struct set_struct {
	const char *	cmd;
	const UInt32	staffFlag;
	SetAllowedOn	pcnpc;
	SetType			type;
} set_fields[] = {
	{ "brief"		, Staff::General			, PC	, BINARY	},	/* 0 */
	{ "invstart"	, Staff::Chars				, PC	, BINARY	},	/* 1 */
	{ "title"		, Staff::General			, PC	, MISC		},
	{ "nosummon"	, Staff::Game				, PC	, BINARY	},
	{ "maxhit"		, Staff::Chars				, BOTH	, NUMBER	},
	{ "nodelete"	, Staff::Admin				, PC	, BINARY	},	/* 5 */
	{ "invuln"		, Staff::Chars				, BOTH	, NUMBER	},
	{ "hit"			, Staff::Chars				, BOTH	, NUMBER	},
	{ "quest"		, Staff::Chars				, PC	, BINARY	},
	{ "staffinvis"	, Staff::Chars				, PC	, NUMBER	},
	{ "race"		, Staff::Admin				, BOTH	, MISC		},	/* 10 */
	{ "str"			, Staff::Chars				, BOTH	, NUMBER	},
	{ "passwd"		, Staff::Chars				, PC	, MISC		},
	{ "int"			, Staff::Chars				, BOTH	, NUMBER	},
	{ "wis"			, Staff::Chars				, BOTH	, NUMBER	},
	{ "dex"			, Staff::Chars				, BOTH	, NUMBER	},	/* 15 */
	{ "con"			, Staff::Chars				, BOTH	, NUMBER	},
	{ "cha"			, Staff::Chars				, BOTH	, NUMBER	},
	{ "sex"			, Staff::Admin				, BOTH	, MISC		},
	{ "pd"			, Staff::Admin				, BOTH	, NUMBER	},
	{ "dr"			, Staff::Admin				, BOTH	, NUMBER	},  /* 20 */
	{ "loadroom"	, Staff::Chars				, PC	, MISC		},
	{ "color"		, Staff::General			, PC	, BINARY	},
	{ "hitroll"		, Staff::Chars				, BOTH	, NUMBER	},
	{ "damroll"		, Staff::Chars				, BOTH	, NUMBER	},
	{ "invis"		, Staff::Security			, PC	, NUMBER	},  /* 25 */
	{ "nohassle"	, Staff::Admin				, PC	, BINARY	},
	{ "frozen"		, Staff::Admin				, PC	, BINARY	},
	{ "practices"	, Staff::Chars				, PC	, NUMBER	},
	{ "thirst"		, Staff::General			, BOTH	, MISC		},
	{ "hunger"		, Staff::General			, BOTH	, MISC		},  /* 30 */
	{ "drunk"		, Staff::General			, BOTH	, MISC		},
	{ "killer"		, Staff::Chars				, PC	, BINARY	},
	{ "idnum"		, Staff::Coder				, PC	, NUMBER	},
	{ "level"		, Staff::Admin				, BOTH	, NUMBER	},
	{ "room"		, Staff::Admin				, BOTH	, NUMBER	},  /* 35 */
	{ "roomflag"	, Staff::Admin				, PC	, BINARY	},
	{ "siteok"		, Staff::Security			, PC	, BINARY	},
	{ "deleted"		, Staff::Admin				, PC	, BINARY	},
	{ "name"		, Staff::Admin				, PC	, MISC		},  //  40
	{ "email"		, Staff::Chars				, PC	, MISC		},
	{ "maxmana"		, Staff::Admin				, BOTH	, NUMBER	},
	{ "mana"		, Staff::Admin				, BOTH	, NUMBER	},
	{ "maxmove"		, Staff::Admin				, BOTH	, NUMBER	},
	{ "move"		, Staff::Admin				, BOTH	, NUMBER	},
	{ "trust"		, Staff::Admin				, PC	, NUMBER	},
	{ "dozy"		, Staff::Chars				, PC	, BINARY	},
	{ "gold"		, Staff::Chars				, BOTH	, NUMBER	},
	{ "align"		, Staff::Chars				, BOTH	, NUMBER	},
	{ "bodytype"	, Staff::Admin				, BOTH	, MISC		},	/* 10 */
	{ "mortality"	, Staff::Admin				, BOTH	, MISC		},	/* 10 */
	{ "\n", 0, BOTH, MISC }
};


bool perform_set(Character *ch, Character *vict, SInt32 mode, char *val_arg) {
	SInt32	i, value = 0;
	bool	on = false, off = false;
	char *output, *temp;

	if (IS_STAFF(vict) && !STF_FLAGGED(ch, Staff::Admin)) {
		ch->Send("Maybe that's not such a great idea...\r\n");
		return 0;
	}

	if (!STF_FLAGGED(ch, set_fields[mode].staffFlag)) {
		ch->Send("You do not have permission to do that.\r\n");
		return 0;
	}

	//	Make sure the PC/NPC is correct
	if (IS_NPC(vict) && !(set_fields[mode].pcnpc & NPC)) {
		ch->Send("You can't do that to a mob!");
		return 0;
	} else if (!IS_NPC(vict) && !(set_fields[mode].pcnpc & PC)) {
		ch->Send("You can only do that to a mob!");
		return 0;
	}

	output = get_buffer(MAX_STRING_LENGTH);

	if (set_fields[mode].type == BINARY) {
		if (!strcmp(val_arg, "on") || !strcmp(val_arg, "yes"))
			on = true;
		else if (!strcmp(val_arg, "off") || !strcmp(val_arg, "no"))
			off = true;
		if (!(on || off)) {
			ch->Send("Value must be 'on' or 'off'.\r\n");
			release_buffer(output);
			return 0;
		}
		sprintf(output, "%s %s for %s.", set_fields[mode].cmd, ONOFF(on), vict->RealName());
	} else if (set_fields[mode].type == NUMBER) {
		value = atoi(val_arg);
		sprintf(output, "%s's %s set to %d.", vict->RealName(), set_fields[mode].cmd, value);
	} else {
		strcpy(output, "Okay.");
	}

	switch (mode) {
		case 0:
			SET_OR_REMOVE(PRF_FLAGS(vict), Preference::Brief);
			break;
		case 1:
			SET_OR_REMOVE(PLR_FLAGS(vict), PLR_INVSTART);
			break;
		case 2:
			vict->SetTitle(val_arg);
			sprintf(output, "%s's title is now: %s", vict->RealName(), GET_TITLE(vict));
			vict->Sendf("%s set your title to \"%s\".\r\n", ch->RealName(), GET_TITLE(vict));
			break;
		case 3:
			SET_OR_REMOVE(PRF_FLAGS(vict), Preference::Summonable);
			sprintf(output, "Nosummon %s for %s.\r\n", ONOFF(!on), vict->RealName());
			break;
		case 4:
			vict->points.max_hit = value = RANGE(1, value, 5000);
			mudlogf(NRM, ch, true, "(SET) %s sets %s's MAXHIT to %d.", ch->Name(), vict->Name(), GET_MAX_HIT(vict));
			vict->AffectTotal();
			break;
		case 5:
			SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NODELETE);
			break;
		case 6:
			SET_OR_REMOVE(STF_FLAGS(vict), Staff::Invuln);
			break;
		case 7:
			vict->points.hit = value = RANGE(-99, value, (SInt32)vict->points.max_hit);
			mudlogf(NRM, ch, true, "(SET) %s sets %s's HIT to %d.", ch->Name(), vict->Name(), GET_HIT(vict));
			vict->AffectTotal();
			break;
		case 8:
			SET_OR_REMOVE(CHN_FLAGS(vict), Channel::Quest);
			break;
		case 9:
			if (GET_TRUST(ch) < TRUST_ADMIN && ch != vict) {
				ch->Send("You aren't godly enough for that!\r\n");
				release_buffer(output);
				return false;
			}
			ch->player->special.staff_invis = value = RANGE(0, value, GET_TRUST(vict));
		case 10:
			if ((i = parse_race(val_arg)) == RACE_UNDEFINED) {
				ch->Send("That is not a race.\r\n");
				release_buffer(output);
				return false;
			}
			GET_RACE(vict) = (Race)i;
			break;
		case 11:
			value = RANGE(1, value, 200);
			mudlogf(NRM, ch, true, "(SET) %s sets %s's Str to %d.", ch->Name(), vict->Name(), value);
			vict->real_abils.Str = value;
			vict->AffectTotal();
			break;
		case 12:
			if (STF_FLAGGED(vict, Staff::Admin)) {
				ch->Send("You cannot change that.\r\n");
				release_buffer(output);
				return false;
			} else {
				strncpy(vict->player->passwd, CRYPT(val_arg, tmp_store.name), MAX_PWD_LENGTH);
				vict->player->passwd[MAX_PWD_LENGTH] = '\0';
				sprintf(output, "Password changed to '%s'.", val_arg);
			}
			break;
		case 13:
			value = RANGE(1, value, 200);
			mudlogf(NRM, ch, true, "(SET) %s sets %s's Int to %d.", ch->Name(), vict->Name(), value);
			vict->real_abils.Int = value;
			vict->AffectTotal();
			break;
		case 14:
			value = RANGE(1, value, 200);
			mudlogf(NRM, ch, true, "(SET) %s sets %s's Wis to %d.", ch->Name(), vict->Name(), value);
			vict->real_abils.Wis = value;
			vict->AffectTotal();
			break;
		case 15:
			value = RANGE(1, value, 200);
			mudlogf(NRM, ch, true, "(SET) %s sets %s's Dex to %d.", ch->Name(), vict->Name(), value);
			vict->real_abils.Dex = value;
			vict->AffectTotal();
			break;
		case 16:
			value = RANGE(1, value, 200);
			mudlogf(NRM, ch, true, "(SET) %s sets %s's Con to %d.", ch->Name(), vict->Name(), value);
			vict->real_abils.Con = value;
			vict->AffectTotal();
			break;
		case 17:
			value = RANGE(1, value, 200);
			mudlogf(NRM, ch, true, "(SET) %s sets %s's Cha to %d.", ch->Name(), vict->Name(), value);
			vict->real_abils.Cha = value;
			vict->AffectTotal();
			break;
		case 18:
			if (!str_cmp(val_arg, "male"))			vict->general.sex = Male;
			else if (!str_cmp(val_arg, "female"))	vict->general.sex = Female;
			else if (!str_cmp(val_arg, "neutral"))	vict->general.sex = Neutral;
			else {
				ch->Send("Must be 'male', 'female', or 'neutral'.\r\n");
				release_buffer(output);
				return false;
			}
			mudlogf(NRM, ch, true, "(SET) %s sets %s's SEX to %s.", ch->Name(), vict->Name(), genders[GET_SEX(vict)]);
			break;
		case 19:
			GET_PD(vict) = value = RANGE(0, value, 100);
			mudlogf(NRM, ch, true, "(SET) %s sets %s's PD to %d.", ch->Name(), vict->Name(), GET_PD(vict));
			vict->AffectTotal();
			break;
		case 20:
			GET_DR(vict) = value = RANGE(0, value, 100);
			mudlogf(NRM, ch, true, "(SET) %s sets %s's DR to %d.", ch->Name(), vict->Name(), GET_DR(vict));
			vict->AffectTotal();
			break;
		case 21:
			if (!str_cmp(val_arg, "off")) {
				REMOVE_BIT(PLR_FLAGS(vict), PLR_LOADROOM);
				mudlogf(NRM, ch, true, "(SET) %s turns %s's LOADROOM off.", ch->Name(), vict->Name());
			} else if (is_number(val_arg)) {
				value = atoi(val_arg);
				if (!Room::Find(value)) {
					ch->Send("That room does not exist!");
					release_buffer(output);
					return false;
				}
				SET_BIT(PLR_FLAGS(vict), PLR_LOADROOM);
				GET_LOADROOM(vict) = value;
				sprintf(output, "%s will enter at room #%d.", vict->RealName(), GET_LOADROOM(vict));
				mudlogf(NRM, ch, true, "(SET) %s sets %s's LOADROOM to %d.", ch->Name(), vict->Name(), GET_LOADROOM(vict));
			} else {
				ch->Send("Must be 'off' or a room's virtual number.\r\n");
				release_buffer(output);
				return false;
			}
			break;
		case 22:
			SET_OR_REMOVE(PRF_FLAGS(vict), Preference::Color);
			break;
		case 23:
			vict->points.hitroll = value = RANGE(-200, value, 200);
			mudlogf(NRM, ch, true, "(SET) %s sets %s's HITROLL to %d.", ch->Name(), vict->Name(), GET_HITROLL(vict));
			vict->AffectTotal();
			break;
		case 24:
			vict->points.damroll = value = RANGE(-200, value, 200);
			mudlogf(NRM, ch, true, "(SET) %s sets %s's DAMROLL to %d.", ch->Name(), vict->Name(), GET_DAMROLL(vict));
			vict->AffectTotal();
			break;
		case 25:
//			if (IS_STAFF(ch) < LVL_ADMIN && ch != vict) {
//				ch->Send("You aren't godly enough for that!\r\n");
//				release_buffer(output);
//				return false;
//			}
//			GET_INVIS_LEV(vict) = value = RANGE(0, value, 101);
			break;
		case 26:
			if (!STF_FLAGGED(ch, Staff::Admin) && ch != vict) {
				ch->Send("You don't have the privileges to do that!\r\n");
				release_buffer(output);
				return false;
			}
			SET_OR_REMOVE(PRF_FLAGS(vict), Preference::NoHassle);
			break;
		case 27:
			if (ch == vict) {
				ch->Send("Better not -- could be a long winter!\r\n");
				release_buffer(output);
				return false;
			}
			SET_OR_REMOVE(PLR_FLAGS(vict), PLR_FROZEN);
			break;
		case 28:
			GET_PRACTICES(vict) = value = RANGE(0, value, 1000);
			break;
		case 29:
		case 30:
		case 31:
			if (!str_cmp(val_arg, "off")) {
				GET_COND(vict, (mode - 29)) = (char) -1;
				sprintf(output, "%s's %s now off.", vict->RealName(), set_fields[mode].cmd);
			} else if (is_number(val_arg)) {
				value = RANGE(0, atoi(val_arg), 24);
				GET_COND(vict, (mode - 29)) = (char) value;
				sprintf(output, "%s's %s set to %d.", vict->RealName(), set_fields[mode].cmd, value);
			} else {
				ch->Send("Must be 'off' or a value from 0 to 24.\r\n");
				release_buffer(output);
				return false;
			}
//			CheckRegenRates(vict);
			break;
		case 32:
			SET_OR_REMOVE(PLR_FLAGS(vict), PLR_KILLER);
			mudlogf(NRM, ch, true, "(SET) %s turns %s's KILLER %s.", ch->Name(), vict->Name(),
					PLR_FLAGGED(vict, PLR_KILLER) ? "on" : "off");
			break;
		case 33:
			if (GET_IDNUM(ch) != 1 || !IS_NPC(vict)) {
				ch->Send("Nuh-uh.");
				release_buffer(output);
				return false;
			}
			GET_IDNUM(vict) = value;
			break;
		case 34:
			if (value > MAX_LEVEL) {
				ch->Send("You can't do that.\r\n");
				release_buffer(output);
				return false;
			}
			value = RANGE(1, value, MAX_LEVEL);
			GET_LEVEL(vict) = value;
			break;
		case 35:
			if (!Room::Find(i = value)) {
				ch->Send("No room exists with that number.\r\n");
				release_buffer(output);
				return false;
			}
			if (IN_ROOM(vict) != NOWHERE)
				vict->FromRoom();
			vict->ToRoom(i);
			break;
		case 36:
			SET_OR_REMOVE(PRF_FLAGS(vict), Preference::RoomFlags);
			break;
		case 37:
			SET_OR_REMOVE(PLR_FLAGS(vict), PLR_SITEOK);
			mudlogf(NRM, ch, true, "(SET) %s turns %s's SITEOK %s.", ch->Name(), vict->Name(),
					PLR_FLAGGED(vict, PLR_SITEOK) ? "on" : "off");
			break;
		case 38:
			SET_OR_REMOVE(PLR_FLAGS(vict), PLR_DELETED);
			mudlogf(NRM, ch, true, "(SET) %s turns %s's DELETED %s.", ch->Name(), vict->Name(),
					PLR_FLAGGED(vict, PLR_DELETED) ? "on" : "off");
			break;
		case 39:
			if ((_parse_name(val_arg, val_arg)) || strlen(val_arg) < 2 ||
						strlen(val_arg) > MAX_NAME_LENGTH || // !Valid_Name(tmp_name) ||
						fill_word(val_arg) || reserved_word(val_arg)) {
				ch->Send("That name is not allowed.\r\n");
				release_buffer(output);
				return false;
			}
			if (str_cmp(vict->Name(), val_arg) && get_id_by_name(val_arg)) {
				ch->Send("There is already a player with that name!\r\n");
				release_buffer(output);
				return false;
			}
			for (i = 0; i < player_table.Count(); i++)
				if (player_table[i].id == GET_ID(vict))
					break;
			if (i >= player_table.Count()) {
				ch->Send("Player not in table?!?\r\n");
				return false;
			}
			act("$N sets your name to $t.", false, vict, (Object *)val_arg, ch, TO_CHAR);
			mudlogf(NRM, ch, true, "(SET) %s changes %s's NAME to %s.", ch->Name(), vict->Name(),
					val_arg);

			temp = get_buffer(MAX_INPUT_LENGTH);

			sprintf(temp, PLR_PREFIX "%c" SEPERATOR "%s", *vict->Name(), vict->Name());
			for (int x = 0; (temp[x] = LOWER(temp[x])); x++);
			unlink(temp);

			vict->SetName(val_arg);

			free(player_table[i].name);
			strcpy(temp, val_arg);
			for (int x = 0; (temp[x] = LOWER(temp[x])); x++);
			player_table[i].name = str_dup(temp);

			get_filename(vict->Name(), temp);
			unlink(temp);

			release_buffer(temp);

			save_player_index();
			break;
		case 40:
			if (!str_cmp(val_arg, "NONE")) {
				if (vict->player->email)	free(vict->player->email);
				vict->player->email = NULL;
			} else if (strchr(val_arg, '@') && !strchr(val_arg, ' ')) {
				if (vict->player->email)	free(vict->player->email);
				vict->player->email = str_dup(val_arg);
			} else {
				ch->Send("That is not a valid email address.\r\n");
				release_buffer(output);
				return false;
			}
			mudlogf(NRM, ch, true, "(SET) %s changes %s's email to %s.", ch->Name(), vict->Name(),
					val_arg);
			break;
		case 41:
			vict->points.max_mana = value = RANGE(-99, value, 30000);
			mudlogf(NRM, ch, true, "(SET) %s sets %s's MAXMANA to %d.", ch->Name(), vict->Name(), GET_MAX_MANA(vict));
			vict->AffectTotal();
			break;
		case 42:
			vict->points.mana = value = RANGE(-99, value, MIN(30000, GET_MAX_MANA(vict)));
			mudlogf(NRM, ch, true, "(SET) %s sets %s's MANA to %d.", ch->Name(), vict->Name(), GET_MANA(vict));
			vict->AffectTotal();
			break;
		case 43:
			vict->points.max_move = value = RANGE(1, value, 5000);
			mudlogf(NRM, ch, true, "(SET) %s sets %s's MAXMOVE to %d.", ch->Name(), vict->Name(), GET_MAX_MOVE(vict));
			vict->AffectTotal();
			break;
		case 44:
			vict->points.move = value = RANGE(-99, value, MIN(5000, GET_MAX_MOVE(vict)));
			mudlogf(NRM, ch, true, "(SET) %s sets %s's MOVE to %d.", ch->Name(), vict->Name(), GET_MOVE(vict));
			vict->AffectTotal();
			break;
		case 45:
			GET_TRUST(vict) = value = RANGE(0, value, GET_TRUST(ch));
			break;
		case 46:
			SET_OR_REMOVE(PLR_FLAGS(vict), PLR_DOZY);
			break;
		case 47:
			GET_GOLD(vict) = value = RANGE(0, value, 2000000000);
			break;
		case 48:
			GET_ALIGNMENT(vict) = value = RANGE(-1000, value, 1000);
			break;
		case 49:
			if ((i = search_block(val_arg, bodytypes, FALSE)) == -1) {
				ch->Send("That is not a bodytype.\r\n");
				release_buffer(output);
				return false;
			}
			GET_BODYTYPE(vict) = (Bodytype)i;
			break;
		case 50:
			if ((i = search_block(val_arg, mortality_types, FALSE)) == -1) {
				ch->Send("That is not a bodytype.\r\n");
				release_buffer(output);
				return false;
			}
			GET_MORTALITY(vict) = (Mortality)i;
			break;
		default:
			ch->Send("Can't set that!");
			release_buffer(output);
			return false;
	}
	ch->Sendf("%s\r\n", CAP(output));
	release_buffer(output);
	return true;
}



ACMD(do_set) {
	Character *vict = NULL, *cbuf = NULL;
	char	*field = get_buffer(MAX_INPUT_LENGTH),
			*name = get_buffer(MAX_INPUT_LENGTH),
			*buf = get_buffer(MAX_INPUT_LENGTH);
	SInt32	mode = -1, len = 0, player_i = 0;
	bool	is_file = false, is_mob = false, is_player = false, retval;

  if (IS_IMMPUN(ch) && !IS_ADMIN(ch)) {
    ch->Send("You do not have that power.\r\n");
    return;
  }

	half_chop(argument, name, buf);
	if (!strcmp(name, "file")) {
		is_file = true;
		half_chop(buf, name, buf);
	} else if (!str_cmp(name, "player")) {
		is_player = true;
		half_chop(buf, name, buf);
	} else if (!str_cmp(name, "mob")) {
		is_mob = true;
		half_chop(buf, name, buf);
	}
	half_chop(buf, field, buf);

	if (!*name || !*field) {
		ch->Send("Usage: set <victim> <field> <value>\r\n");
	}

	if (!is_file) {
		if (is_player) {
			if (!(vict = get_player_vis(ch, name, false))) {
				ch->Send("There is no such player.\r\n");
				release_buffer(field);
				release_buffer(name);
				release_buffer(buf);
				return;
			}
		} else {
			if (!(vict = get_char_vis(ch, name, FIND_CHAR_WORLD))) {
				ch->Send("There is no such creature.\r\n");
				release_buffer(field);
				release_buffer(name);
				release_buffer(buf);
				return;
			}
		}
	} else {
		cbuf = new Character();
		if ((player_i = cbuf->Load(name)) > -1) {
			GET_PFILEPOS(cbuf) = player_i;

			if (!ENOUGH_TRUST(ch, cbuf)) {
				delete cbuf;
				ch->Send("Sorry, you can't do that.\r\n");
				release_buffer(field);
				release_buffer(name);
				release_buffer(buf);
				return;
			}
			vict = cbuf;
		} else {
			ch->Send("There is no such player.\r\n");
			delete cbuf;
			release_buffer(field);
			release_buffer(name);
			release_buffer(buf);
			return;
		}
	}

	len = strlen(field);
	for (mode = 0; *(set_fields[mode].cmd) != '\n'; mode++)
		if (!strncmp(field, set_fields[mode].cmd, len))
			break;

	retval = perform_set(ch, vict, mode, buf);

	if (retval) {
		if (!IS_NPC(vict)) {
			// CHANGEPOINT:  They're losing loadrooms here.
			vict->Save(NOWHERE);
			if (is_file)
				ch->Send("Saved in file.\r\n");
		}
	}

	if (is_file)
		delete cbuf;

	release_buffer(field);
	release_buffer(name);
	release_buffer(buf);
}


const char *logtypes[] = {"off", "brief", "normal", "complete", "\n"};

ACMD(do_syslog) {
	int tp;
	char *arg = get_buffer(MAX_INPUT_LENGTH);

	one_argument(argument, arg);

	if (!*arg) {
		tp = ((PRF_FLAGGED(ch, Preference::Log1) ? 1 : 0) + (PRF_FLAGGED(ch, Preference::Log2) ? 2 : 0));
		ch->Sendf("Your syslog is currently %s.\r\n", logtypes[tp]);
	} else if (((tp = search_block(arg, logtypes, FALSE)) == -1))
		ch->Send("Usage: syslog { Off | Brief | Normal | Complete }\r\n");
	else {
		REMOVE_BIT(PRF_FLAGS(ch), Preference::Log1 | Preference::Log2);
		SET_BIT(PRF_FLAGS(ch), (Preference::Log1 * (tp & 1)) | (Preference::Log2 * (tp & 2) >> 1));

		ch->Sendf("Your syslog is now %s.\r\n", logtypes[tp]);
	}
	release_buffer(arg);
}



ACMD(do_depiss) {
	char	*arg1 = get_buffer(MAX_INPUT_LENGTH),
			*arg2 = get_buffer(MAX_INPUT_LENGTH);
	Character *vic = 0;
	Character *mob  = 0;

	two_arguments(argument, arg1, arg2);

	if (arg2)
		ch->Send("Usage: depiss <victim> <mobile>\r\n");
	else if (((vic = get_char_vis(ch, arg1, FIND_CHAR_WORLD))) /* && !IS_NPC(vic) */) {
		if (((mob = get_char_vis(ch, arg2, FIND_CHAR_WORLD))) && (IS_NPC(mob))) {
			if (mob->mob->hates && mob->mob->hates->Contains(GET_ID(vic))) {
				ch->Sendf("%s no longer hates %s.\r\n", mob->RealName(), vic->RealName());
				forget(mob, vic);
			} else
				ch->Send("Mobile does not hate the victim!\r\n");
		} else
			ch->Send("Sorry, Mobile Not Found!\r\n");
	} else
		ch->Send("Sorry, Victim Not Found!\r\n");
	release_buffer(arg1);
	release_buffer(arg2);
}


ACMD(do_repiss) {
	char	*arg1 = get_buffer(MAX_INPUT_LENGTH),
			*arg2 = get_buffer(MAX_INPUT_LENGTH);
	Character *vic = 0;
	Character *mob  = 0;

	two_arguments(argument, arg1, arg2);

	if (!*arg1 || !*arg2)
		ch->Send("Usage: repiss <victim> <mobile>\r\n");
	else if (((vic = get_char_vis(ch, arg1, FIND_CHAR_WORLD))) /* && !IS_NPC(vic) */) {
		if (((mob = get_char_vis(ch, arg2, FIND_CHAR_WORLD))) && (IS_NPC(mob))) {
			ch->Sendf("%s now hates %s.\r\n", mob->RealName(), vic->RealName());
			remember(mob, vic);
		} else
			ch->Send("Sorry, Hunter Not Found!\r\n");
	} else
		ch->Send("Sorry, Victim Not Found!\r\n");
	release_buffer(arg1);
	release_buffer(arg2);
}


ACMD(do_hunt) {
	char	*arg1 = get_buffer(MAX_INPUT_LENGTH),
			*arg2 = get_buffer(MAX_INPUT_LENGTH);
	Character *vic = NULL;
	Character *mob  = NULL;

	two_arguments(argument, arg1, arg2);

	if (!*arg1 || !*arg2)
		ch->Send("Usage: hunt <victim> <hunter>\r\n");
	else if ((vic = get_char_vis(ch, arg1, FIND_CHAR_WORLD))) {
		if ((mob = get_char_vis(ch, arg2, FIND_CHAR_WORLD)) && IS_NPC(mob)) {
			// mob->path = Path::ToChar(IN_ROOM(mob), vic, 200, Path::Global | Path::ThruDoors);
			// if (mob->path)	ch->Sendf("%s is now hunted by %s.\r\n", vic->RealName(), mob->RealName());
			// else			ch->Sendf("%s can't reach %s.\r\n", mob->RealName(), vic->RealName());
			HUNTING(mob) = GET_ID(vic);
		} else				ch->Send("Hunter not found.\r\n");
	} else					ch->Send("Victim not found.\r\n");
	release_buffer(arg1);
	release_buffer(arg2);
}


void send_to_imms(char *msg) {
	Descriptor *pt;

	START_ITER(iter, pt, Descriptor *, descriptor_list) {
		if ((STATE(pt) == CON_PLAYING) && pt->character && IS_STAFF(pt->character))
			pt->Write(msg);
	}
}


ACMD(do_vwear) {
	char *arg = get_buffer(MAX_INPUT_LENGTH);

	one_argument(argument, arg);

	if (!*arg) ch->Send("Usage: vwear <wear position>\r\n"
						"Possible positions are:\r\n"
						"finger    neck    body    head    legs    feet    hands\r\n"
						"shield    arms    about   waist   wrist   wield   hold   eyes\r\n");
	else if (is_abbrev(arg, "finger"))	vwear_object(ITEM_WEAR_FINGER, ch);
	else if (is_abbrev(arg, "neck"))		vwear_object(ITEM_WEAR_NECK, ch);
	else if (is_abbrev(arg, "body"))		vwear_object(ITEM_WEAR_BODY, ch);
	else if (is_abbrev(arg, "head"))		vwear_object(ITEM_WEAR_HEAD, ch);
	else if (is_abbrev(arg, "legs"))		vwear_object(ITEM_WEAR_LEGS, ch);
	else if (is_abbrev(arg, "feet"))		vwear_object(ITEM_WEAR_FEET, ch);
	else if (is_abbrev(arg, "hands"))		vwear_object(ITEM_WEAR_HANDS, ch);
	else if (is_abbrev(arg, "arms"))		vwear_object(ITEM_WEAR_ARMS, ch);
	else if (is_abbrev(arg, "shield"))		vwear_object(ITEM_WEAR_SHIELD, ch);
	else if (is_abbrev(arg, "about body"))	vwear_object(ITEM_WEAR_ABOUT, ch);
	else if (is_abbrev(arg, "waist"))		vwear_object(ITEM_WEAR_WAIST, ch);
	else if (is_abbrev(arg, "wrist"))		vwear_object(ITEM_WEAR_WRIST, ch);
	else if (is_abbrev(arg, "wield"))		vwear_object(ITEM_WEAR_WIELD, ch);
	else if (is_abbrev(arg, "hold"))		vwear_object(ITEM_WEAR_HOLD, ch);
	else if (is_abbrev(arg, "eyes"))		vwear_object(ITEM_WEAR_EYES, ch);
	else ch->Send("Possible positions are:\r\n"
				  "finger    neck    body    head    legs    feet    hands\r\n"
				  "shield    arms    about   waist   wrist   wield   hold   eyes\r\n");
	release_buffer(arg);
}


//extern int mother_desc, port;
//extern FILE *player_fl;

void Crash_rentsave(Character * ch);
/* (c) 1996-97 Erwin S. Andreasen <erwin@pip.dknet.dk> */


#define EXE_FILE "bin/circle.new"

ACMD(do_copyover) {
	FILE *fp;
	Descriptor *d; //, *d_next;
	char buf[256];	//, buf2[64];

	fp = fopen (COPYOVER_FILE, "w");

	if (!fp) {
		ch->Sendf("Copyover file not writeable, aborted.\r\n");
		return;
	}

	/* Consider changing all saved areas here, if you use OLC */
	sprintf (buf, "\r\n *** COPYOVER by %s - please remain seated!\r\n", ch->RealName());
	log("COPYOVER by %s", ch->RealName());
	circle_shutdown = 1;
	circle_copyover = 1;
	/* For each playing descriptor, save its state */
	LListIterator<Descriptor *>	iter(descriptor_list);
	while ((d = iter.Next())) {
		Character * och = d->Original();

		if (!och || (STATE(d) != CON_PLAYING)) { /* drop those logging on */
			write_to_descriptor (d->descriptor, "\r\nSorry, we are rebooting. Come back in a few minutes.\r\n");
			close_socket (d); /* throw'em out */
		} else {
			fprintf (fp, "%d %s %s\n", d->descriptor, och->RealName(), d->host);

			/* save och */
			Crash_rentsave(och);
			och->Save(IN_ROOM(och));
			write_to_descriptor (d->descriptor, buf);
		}
	}

	fprintf (fp, "-1\n");
	fclose (fp);

	/* Failed - sucessful exec will not return */

	perror ("do_copyover: execl");
	ch->Send("Copyover FAILED!\r\n");

	exit (1); /* too much trouble to try to recover! */
}



ACMD(do_tedit) {
	int l, i;
	extern char *credits;
	extern char *news;
	extern char *motd;
	extern char *imotd;
	extern char *help;
	extern char *info;
	extern char *background;
	extern char *handbook;
	extern char *policies;
	char	*field, *buf, **strptr = NULL;
	Editor * tmpeditor = NULL;

	struct editor_struct {
		char *cmd;
		char level;
		char **buffer;
		int  size;
		char *filename;
	} fields[] = {
		/* edit the lvls to your own needs */
		{ "credits",	TRUST_IMMORTAL,      &credits,   2400,   CREDITS_FILE},
		{ "news",		TRUST_IMMORTAL,      &news,      8192,   NEWS_FILE},
		{ "motd",		TRUST_IMMORTAL,      &motd,      2400,   MOTD_FILE},
		{ "imotd",		TRUST_IMMORTAL,      &imotd,     2400,   IMOTD_FILE},
		{ "help",       TRUST_IMMORTAL,      &help,      2400,   HELP_PAGE_FILE},
		{ "info",		TRUST_IMMORTAL,      &info,      8192,   INFO_FILE},
		{ "background",	TRUST_IMMORTAL,      &background,8192,   BACKGROUND_FILE},
		{ "handbook",   TRUST_IMMORTAL,      &handbook,  8192,   HANDBOOK_FILE},
		{ "policies",	TRUST_IMMORTAL,      &policies,  8192,   POLICIES_FILE},
		{ "file",		TRUST_IMMORTAL,      NULL,       8192,   NULL},
		{ "\n",			0,				NULL,		0,		NULL }
	};

	if (!ch->desc) {
		ch->Send("Get outta here you linkdead head!\r\n");
		return;
	}

   	field = get_buffer(MAX_INPUT_LENGTH);
	buf = get_buffer(MAX_INPUT_LENGTH);

	half_chop(argument, field, buf);

	if (!*field) {
		strcpy(buf, "Files available to be edited:\r\n");
		i = 1;
		for (l = 0; *fields[l].cmd != '\n'; l++) {
			if (GET_LEVEL(ch) >= fields[l].level) {
				sprintf(buf + strlen(buf), "%-11.11s", fields[l].cmd);
				if (!(i % 7)) strcat(buf, "\r\n");
				i++;
			}
		}
		if (--i % 7)	strcat(buf, "\r\n");
		if (i == 0)		strcat(buf, "None.\r\n");
		ch->Send(buf);
		release_buffer(field);
		release_buffer(buf);
		return;
	}
	for (l = 0; *(fields[l].cmd) != '\n'; l++)
		if (!strncmp(field, fields[l].cmd, strlen(field)))
			break;

	if (*fields[l].cmd == '\n') {
		ch->Send("Invalid text editor option.\r\n");
		release_buffer(field);
		release_buffer(buf);
		return;
	}

	if (GET_LEVEL(ch) < fields[l].level) {
		ch->Send("You are not godly enough for that!\r\n");
		release_buffer(field);
		release_buffer(buf);
		return;
	}

	switch (l) {
		case 0: strptr = &credits; break;
		case 1: strptr = &news; break;
		case 2: strptr = &motd; break;
		case 3: strptr = &imotd; break;
		case 4: strptr = &help; break;
		case 5: strptr = &info; break;
		case 6: strptr = &background; break;
		case 7: strptr = &handbook; break;
		case 8: strptr = &policies; break;
		case 9:	//	break;
		default:
			ch->Send("Invalid text editor option.\r\n");
			release_buffer(field);
			release_buffer(buf);
			return;
	}

	/* set up editor stats */
	ch->Send("\x1B[H\x1B[J");
	ch->Send("Edit file below: (/s saves /h for help)\r\n");
	tmpeditor = new FileEditor(ch->desc, strptr, fields[l].size, fields[l].filename);
	ch->desc->Edit(tmpeditor);
	act("$n begins writing a scroll.", TRUE, ch, 0, 0, TO_ROOM);
	SET_BIT(PLR_FLAGS(ch), PLR_WRITING);
	STATE(ch->desc) = CON_TEXTED;
	release_buffer(field);
	release_buffer(buf);
}


#ifdef GOT_RID_OF_IT
ACMD(do_reward) {
	Character * victim;
	int amount;
	char	*arg1 = get_buffer(MAX_INPUT_LENGTH),
			*arg2 = get_buffer(MAX_INPUT_LENGTH);

	half_chop(argument, arg1, arg2);

	if (!*arg1 || !*arg2 || (amount = atoi(arg2)) == 0)
		ch->Send("Usage: reward <player> <amount>\r\n");
	else if (!(victim = get_char_vis(ch, arg1, FIND_CHAR_WORLD)))
		ch->Send(NOPERSON, ch);
	else if (IS_NPC(victim))
		ch->Send("Not on NPCs, you dolt!\r\n");
	else {
		if ((amount < 0) && (amount < (0-GET_MISSION_POINTS(victim)))) {
			amount = 0 - (GET_MISSION_POINTS(victim));
			ch->Sendf("%s only has %d MP... taking all of them away.\r\n", victim->RealName(), GET_MISSION_POINTS(victim));
		}

		if (amount == 0)
			ch->Send("Zero MP, eh?");
		else {
			GET_MISSION_POINTS(victim) += amount;

			ch->Sendf("You %s %d MP %s %s.\r\n",
					(amount > 0) ? "give" : "take away",
					abs(amount),
					(amount > 0) ? "to" : "from",
					victim->RealName());

			victim->Sendf("%s %s %d MP %s you.\r\n",
					PERS(ch, victim),
					(amount > 0) ? "gives" : "takes away",
					abs(amount),
					(amount > 0) ? "to" : "from");

			victim->Save(NOWHERE);
		}
	}
	release_buffer(arg1);
	release_buffer(arg2);
}
#endif


/*
Usage: string <type> <name> <field> [<string> | <keyword>]

For changing the text-strings associated with objects and characters.  The
format is:

Type is either 'obj' or 'char'.
Name                  (the call-name of an obj/char - kill giant)
Short                 (for inventory lists (obj's) and actions (char's))
Long                  (for when obj/character is seen in room)
Title                 (for players)
Description           (For look at.  For obj's, must be followed by a keyword)
Delete-description    (only for obj's. Must be followed by keyword)
*/
ACMD(do_string) {
	char *type, *name, *field, *string;
	Character *vict = NULL;
	Object *obj = NULL;

	type = get_buffer(MAX_INPUT_LENGTH);
	name = get_buffer(MAX_INPUT_LENGTH);
	field = get_buffer(MAX_INPUT_LENGTH);
	string = get_buffer(MAX_INPUT_LENGTH);

	argument = two_arguments(argument, type, name);
	half_chop(argument, field, string);

	if (!*type || !*name || !*field || !*string) {
		ch->Send("Usage: string [character|object] <name> <field> [<string> | <keyword>]\r\n");
	} else {
		if (is_abbrev(type, "character"))	vict = get_char_vis(ch, name, FIND_CHAR_WORLD);
		else if (is_abbrev(type, "object"))	obj = get_obj_vis(ch, name);

		if (!vict && !obj)
			ch->Send("Usage: string [character|object] <name> <field> [<string> | <keyword>]\r\n");
		else {
			if (is_abbrev(field, "name")) {
				if (vict) {
					if (IS_NPC(vict)) {
						ch->Sendf("%s's name restrung to '%s'.\r\n", vict->RealName(), string);
						vict->SetName(string);
					} else
						ch->Send("You can't re-string player's names.\r\n");
				} else if (obj) {
					ch->Sendf("%s's name restrung to '%s'.\r\n", obj->Name(), string);
					obj->SetName(string);
				}
			} else if (is_abbrev(field, "short")) {
				if (vict) {
					if (IS_NPC(vict)) {
						ch->Sendf("%s's short desc restrung to '%s'.\r\n", vict->RealName(), string);
						vict->SetSDesc(string);
					} else ch->Send("Players don't have short descs.\r\n");
				} else if (obj) {
					ch->Sendf("%s's short desc restrung to '%s'.\r\n", obj->Name(), string);
					SSFree(obj->shortDesc);
					obj->shortDesc = SString::Create(string);
				}
			} else if (is_abbrev(field, "long")) {
				strcat(string, "\r\n");
				if (vict) {
					if (IS_NPC(vict)) {
						ch->Sendf("%s's long desc restrung to '%s'.\r\n", vict->RealName(), string);
						SSFree(vict->general.longDesc);
						vict->general.longDesc = SString::Create(string);
					} else ch->Send("Players don't have long descs.\r\n");
				} else if (obj) {
					ch->Sendf("%s's long desc restrung to '%s'.\r\n", obj->Name(), string);
					obj->SetSDesc(string);
				}
			} else if (is_abbrev(field, "title")) {
				if (vict) {
					if (!IS_NPC(vict)) {
						if (!IS_STAFF(vict) || STF_FLAGGED(ch, Staff::Admin)) {
							ch->Sendf("%s's title restrung to '%s'.\r\n", vict->RealName(), string);
							SSFree(vict->general.title);
							vict->general.title = SString::Create(string);
						} else ch->Send("You can't restring the title of staff members!");
					} else ch->Send("Mobs don't have titles.\r\n");
				} else if (obj) ch->Send("Objs don't have titles.\r\n");
			} else if (is_abbrev(field, "description")) {
				ch->Send("Feature not implemented yet.\r\n");
			} else if (is_abbrev(field, "delete-description")) {
				if (vict) ch->Send("Only for objects.\r\n");
				else if (obj) {
					ExtraDesc *extra;

					LListIterator<ExtraDesc *>	descIter(obj->exDesc);
					while ((extra = descIter.Next()))
						if (!str_cmp(SSData(extra->keyword), string))
							break;
					if (extra) {
						ch->Sendf("%s's extra description '%s' deleted.\r\n", obj->Name(), SSData(extra->keyword));
						obj->exDesc.Remove(extra);
						delete extra;
					} else
						ch->Send("Extra description not found.\r\n");
				}
			} else
				ch->Send("No such field.\r\n");
		}
	}
	release_buffer(type);
	release_buffer(name);
	release_buffer(field);
	release_buffer(string);
}


ACMD(do_wizcall);
ACMD(do_wizassist);


LList<SInt32> wizcallers;

ACMD(do_wizcall) {
	if (wizcallers.Contains(GET_ID(ch))) {
		wizcallers.Remove(GET_ID(ch));
		ch->Send("You remove yourself from the help queue.\r\n");
		mudlogf(BRF, ch, FALSE, "ASSIST: %s no longer needs assistance.", ch->RealName());
	} else {
		wizcallers.Add(GET_ID(ch));
		ch->Send("A staff member will be with you as soon as possible.\r\n");
		mudlogf(BRF, ch, FALSE, "ASSIST: %s calls for assistance.", ch->RealName());
	}
}


ACMD(do_wizassist) {
	LListIterator<SInt32>	iter(wizcallers);
	SInt32		which, i;
	Character *	who = NULL;
	char *		arg = get_buffer(MAX_INPUT_LENGTH);

	one_argument(argument, arg);
	which = atoi(arg);

	if (!wizcallers.Count()) {
		ch->Send("Nobody needs assistance right now.\r\n");
	} else if (!is_number(arg)) {
		ch->Send("Players who need assistance\r\n"
				 "---------------------------\r\n");
		while ((i = iter.Next(0))) {
			who = find_char(i);
			if (who && (IN_ROOM(who) != NOWHERE))
				ch->Sendf("%2d. %s\r\n", ++which, who->RealName());
			else
				wizcallers.Remove(i);
		}
		if (!which)
			ch->Send("Nobody needs assistance right now.\r\n");

	} else if ((which < 1) || (which > wizcallers.Count()))
		ch->Send("That number doesn't exist in the help queue.\r\n");
	else {
		while ((i = iter.Next(0))) {
			who = find_char(i);
			if (who && (IN_ROOM(who) != NOWHERE))		which--;
			else										wizcallers.Remove(i);
			if (!which)		break;
		}

		if (i && who && (ch != who)) {
			mudlogf(BRF, ch, FALSE, "ASSIST: %s goes to assist %s.", ch->RealName(),
					who->RealName());
			ch->Sendf("You go to assist %s.\r\n", who->RealName());
			ch->FromRoom();
			ch->ToRoom(IN_ROOM(who));
			look_at_room(ch, false);
			act("$n appears in the room, to assist $N.", TRUE, ch, 0, who, TO_NOTVICT);
			act("$n appears in the room, to assist you.", TRUE, ch, 0, who, TO_VICT);

			wizcallers.Remove(i);
		} else if (ch == who)
			ch->Send("You can't help yourself.  You're helpless!\r\n");
		else
			ch->Send("That number doesn't exist in the help queue. [ERROR - Report]\r\n");
	}
	release_buffer(arg);
}


ACMD(do_invuln)
{
	any_one_arg(argument, arg);

	if (!*arg) {
		if (IS_SET(PRF_FLAGS(ch), Preference::Invuln))
			act("You are currently protected by an impenetrable shield of force.",
		FALSE, ch, 0, 0, TO_CHAR);
		else
			act("You are currently vulnerable to mortal attacks.",
		FALSE, ch, 0, 0, TO_CHAR);
		return;
	}

	if (!strncmp(arg, "off", strlen(arg))) {
		REMOVE_BIT(PRF_FLAGS(ch), Preference::Invuln);
		act("You allow your shield to dissipate, once again posing as a mortal.", FALSE, ch, 0, 0, TO_CHAR);
		act("$n's divine shield flickers and goes out.", TRUE, ch, 0, 0, TO_ROOM);
	}
	else if (!strncmp(arg, "on", strlen(arg))) {
		SET_BIT(PRF_FLAGS(ch), Preference::Invuln);
		act("You use your power to raise an impenetrable shield around yourself.", FALSE, ch, 0, 0, TO_CHAR);
		act("$n summons a gleaming translucent shield around $mself.", TRUE, ch, 0, 0, TO_ROOM);
	}
	else
		act("Usage: invuln { On | Off }", FALSE, ch, 0, 0, TO_CHAR);
}


ACMD(do_calm) {
	Character *	victim;
	bool		combatFound = false;

	LListIterator<Character *>	people(world[IN_ROOM(ch)].people);
	while ((victim = people.Next())) {
		if (FIGHTING(victim)) {
			if (!combatFound) {
				combatFound = true;

				ch->Send("You get fed up with the fighting and shout, 'STOP IT!  JUST STOP!'\n");
				act("$n shouts, 'STOP IT!  JUST STOP!'", false, ch, 0, 0, TO_ROOM);
			}
			FIGHTING(victim)->StopFighting();
			FIGHTING(victim)->Send("You are shocked, and forget about fighting.\r\n");
			victim->StopFighting();
			victim->Send("You are shocked, and forget about fighting.\r\n");
		}
	}

	if (!combatFound) {
		ch->Send("But nobody is fighting!");
	}
}


ACMD(do_skillroll)
{
	Character *vict;
	char name[MAX_INPUT_LENGTH], buf2[100], buf[MAX_INPUT_LENGTH];
	char help[MAX_STRING_LENGTH];
	int result, i, qend;
	SInt16 skill, tmp = -1;
	SInt8 chance;

	argument = one_argument(argument, name);

	if (!*name) {			/* no arguments. print an informative text */
		ch->Send("Syntax: skillroll <name> '<skill>' <value>\r\n");
	}
	if (!(vict = get_char_vis(ch, name, FIND_CHAR_WORLD))) {
		ch->Send(NOPERSON);
		return;
	}
	skip_spaces(argument);

	/* If there is no chars in argument */
	if (!*argument) {
		ch->Send("Skill name expected.\n\r");
		return;
	}
	if (*argument != '\'') {
		ch->Send("Skill must be enclosed in: ''\n\r");
		return;
	}
	/* Locate the last quote && lowercase the magic words (if any) */

	for (qend = 1; *(argument + qend) && (*(argument + qend) != '\''); qend++)
		*(argument + qend) = LOWER(*(argument + qend));

	if (*(argument + qend) != '\'') {
		ch->Send("Skill must be enclosed in: ''\n\r");
		return;
	}
	strcpy(help, (argument + 1));
	help[qend - 1] = '\0';
	if ((skill = Skill::Number(help, 0, TOP_SKILLO_DEFINE)) <= 0) {
		ch->Send("Unrecognized skill.\n\r");
		return;
	}

	chance = SKILLCHANCE(vict, skill, NULL);

	// -1 to prevent LearnSkill from kicking in
	result = CHAR_SKILL(vict).DiceRoll(vict, tmp, chance);

	if (result >= 10)
		strcpy (buf1, "&cGCritical Success");
	else if (result >= 0)
		strcpy (buf1, "&cgSuccess");
	else if (result > -10)
		strcpy (buf1, "&crFailure");
	else
		strcpy (buf1, "&cRCritical Failure");

	sprintf(buf2, "Rolling %s for %s: chance %d, result %d (%s&c0).\r\n",
					Skill::Name(skill), vict->Name(), chance, result, buf1);
	ch->Send(buf2);
}


ACMD(do_exittrace)
{
	char where[MAX_INPUT_LENGTH];
	int door;
	VNum location;
	Room *room;

	argument = one_argument(argument, where);

	if (!*where) {
		ch->Send("You must supply a room number or a name.\r\n");
		return;
	}

	if (!Room::Find(location = atoi(where))) {
		ch->Send("No such room exists.\r\n");
		return;
	}

	*buf = '\0';

	MapIterator<VNum, Room>		iter(world);
	while ((room = iter.Next())) {
		for (door = 0; door < NUM_OF_DIRS; ++door) {
			if (room->Direction(door) && (room->Direction(door)->to_room == location)) {
				sprintf(buf2, "%-6d - %-30s : %s\r\n", room->Virtual(),
					room->Name(), dirs[door]);
				strcat(buf, buf2);
			}
			if (strlen(buf) > (MAX_STRING_LENGTH - 200)) {
				door = NUM_OF_DIRS;
				strcat(buf, "** Overflow! **\r\n");
				ch->Sendf("Exits leading to room %d, %s:\r\n", world[location].Virtual(), world[location].Name());
				return;
			}
		}
	}

	ch->Sendf("Exits leading to room %d, %s:\r\n", world[location].Virtual(), world[location].Name());

	if (!*buf)		ch->Send("None!\r\n");
	else			ch->Send(buf);
}


ACMD(do_coredump)
{
	core_dump();
}
