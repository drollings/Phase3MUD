/*************************************************************************
*   File: act.informative.c++        Part of Aliens vs Predator: The MUD *
*  Usage: Player-level information commands                              *
*************************************************************************/


#include "characters.h"
#include "objects.h"
#include "rooms.h"
#include "descriptors.h"
#include "utils.h"
#include "buffer.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "find.h"
#include "db.h"
#include "clans.h"
#include "boards.h"
#include "help.h"
#include "extradesc.h"
#include "constants.h"
#include "space.h"
#include "maputils.h"
#include "weather.h"
#include "spells.h"
/* extern variables */
extern struct TimeInfoData time_info;
extern char *help;
extern char *credits;
extern char *news;
extern char *info;
extern char *motd;
extern char *imotd;
extern char *wizlist;
extern char *immlist;
extern char *policies;
extern char *handbook;

extern void reset_command_ops(void);

extern struct zone_data *zone_table;

namespace ShowObj {
	enum {
		Merge = (1 << 0),
		Name = (1 << 1),
		SDesc = (1 << 2),
		Weight = (1 << 3),
		Flags = (1 << 4),
		Examine = (1 << 5),
		Skip = (1 << 6)
	};
}


/* extern functions */
extern struct TimeInfoData real_time_passed(time_t t2, time_t t1);
extern void master_parser (char *buf, Character *ch, Room *room, Object *obj);
extern int wear_message_number (Object *obj, int where);
extern SInt32 exp_from_level(SInt32 chlevel, SInt32 fromlevel, SInt32 modifier);
ACMD(do_action);

/* prototypes */
void diag_char_to_char(Character * i, Character * ch);
void look_at_char(Character * i, Character * ch);
void list_one_char(Character * i, Character * ch);
void list_char_to_char(LList<Character *> &list, Character * ch);
void do_auto_exits(Character * ch, VNum room);
void look_in_direction(Character * ch, SInt32 dir, SInt32 maxDist);
void look_in_obj(Character * ch, char *arg);
void look_at_target(Character * ch, char *arg, int read, int show);
void perform_mortal_where(Character * ch, char *arg);
void perform_immort_where(Character * ch, char *arg);
void sort_commands(void);
void show_obj_to_char(Object * object, Character * ch, int mode, int num = 1);
void list_obj_to_char(LList<Object *> &list, Character * ch, int mode,  int show);
void print_object_location(char *buf, int num, Object * obj, Character * ch, int recur);
void list_scanned_chars(LList<Character *> &list, Character * ch, int distance, int door);
void look_out(Character * ch);

int look_at_equipped(Character * ch, Character * target);

ACMD(do_exits);
ACMD(do_look);
ACMD(do_examine);
ACMD(do_gold);
ACMD(do_score);
ACMD(do_inventory);
ACMD(do_equipment);
ACMD(do_time);
ACMD(do_weather);
ACMD(do_help);
ACMD(do_who);
ACMD(do_users);
ACMD(do_gen_ps);
ACMD(do_where);
ACMD(do_levels);
ACMD(do_consider);
ACMD(do_diagnose);
ACMD(do_toggle);
ACMD(do_commands);
ACMD(do_attributes);
ACMD(do_allow);


extern void proc_color(char *inbuf, int color, int is_html);
extern void str_swing_dice(SInt16 strength, SInt8 &num, SInt8 &dicemod);
extern void str_thrust_dice(SInt16 strength, SInt8 &num, SInt8 &dicemod);


void show_obj_to_char(Object * object, Character * ch, int mode, int num = 1) {
	bool	found = true;
	char *	buf = get_buffer(MAX_STRING_LENGTH);

    *buf = '\0';

	if (IS_SET(mode, ShowObj::SDesc) && object->SSSDesc()) {
		if ((ch->SittingOn() == object) && (GET_POS(ch) <= POS_SITTING))
			sprintf(buf + strlen(buf), "You are %s on %s.",
					(GET_POS(ch) == POS_RESTING) ? "resting" : "sitting",
					object->Name());
		else {
			if (!IS_NPC(ch) && ch->player->CalledNames.Find(GET_ID(object))) {
				sprintf(buf, "%s called %s lies here.", object->Name(), object->CalledName(ch));
			} else {
				strcat(buf, object->SDesc());
			}
			CAP(buf);
		}

	} else if (IS_SET(mode, ShowObj::Name) && object->Name()) {
		if (!IS_NPC(ch) && ch->player->CalledNames.Find(GET_ID(object))) {
			sprintf(buf, "%s called %s", object->Name(), object->CalledName(ch));
		} else {
			strcat(buf, object->Name());
		}
	} else if (IS_SET(mode, ShowObj::Examine)) {
		if (GET_OBJ_TYPE(object) == ITEM_NOTE) {
			if (SSData(object->actionDesc)) {
				strcpy(buf, "There is something written upon it:\r\n\r\n");
				strcat(buf, SSData(object->actionDesc));
				page_string(ch->desc, buf, true);
			} else
				act("It's blank.", FALSE, ch, 0, 0, TO_CHAR);
			release_buffer(buf);
			return;
		} else if (GET_OBJ_TYPE(object) == ITEM_BOARD) {
			Board::Show(object->Virtual(), ch);
			release_buffer(buf);
			return;
		} else if (GET_OBJ_TYPE(object) == ITEM_DRINKCON)
			strcpy(buf, "It looks like a drink container.");
		else
			strcpy(buf, "You see nothing special..");
	} else
		found = false;

	if (IS_SET(mode, ShowObj::Weight)) {
		if ((!IS_CONTAINER(object)) &&
		   (!IS_SITTABLE(object)) && (GET_OBJ_TYPE(object) != ITEM_VEHICLE)) {
			sprintf(buf3, " [%d lbs]", GET_OBJ_WEIGHT(object) * MAX(num, 1));
			strcat (buf, buf3);
		} else {
			sprintf(buf3, " [%d lbs]", GET_OBJ_WEIGHT(object));
			strcat (buf, buf3);
		}
	}

	if (IS_SET(mode, ShowObj::Flags)) {
		if (OBJ_FLAGGED(object, ITEM_BLESS) &&
			 (AFF_FLAGGED(ch, AffectBit::DetectAlign))) {
			strcat(buf, " &c+&cC(blessed)&c-");
			found = TRUE;
		}
		if (OBJ_FLAGGED(object, ITEM_MAGIC) &&
			 (Affect::AffectedBy(ch, SPELL_DETECT_MAGIC))) {
			strcat(buf, " &c+&cY(aura)&c-");
			found = TRUE;
		}
		if (OBJ_FLAGGED(object, ITEM_HIDDEN)) {
			strcat(buf, " (hidden)");
			found = TRUE;
		}
		if (OBJ_FLAGGED(object, ITEM_GLOW)) {
			strcat(buf, " &c+&cW(glowing)&c-");
			found = TRUE;
		}
		if (OBJ_FLAGGED(object, ITEM_HUM)) {
			strcat(buf, " (humming)");
			found = TRUE;
		}
	}
	if (found)	ch->Sendf("%s\r\n", buf);
	release_buffer(buf);
}


void list_obj_to_char(LList<Object *> &list, Character * ch, int mode, int show) {
	Object *i, *j;
	SInt32	num;
	LListIterator<Object *>	iter(list), sub_iter(list);
	bool	found = false;

	while ((i = iter.Next())) {
		num = 0;								/* None found */

		if (!IS_NPC(ch) && !(ch->player->CalledNames.Find(GET_ID(i)))) {
			// Advance j to the point that i has reached, or previous objects with the same name.
			sub_iter.Reset();
			while ((j = sub_iter.Next())) {
				if (j == i)							break;
				if (!j->Name() || !i->Name())		core_dump();
				if (!IS_NPC(ch) && (ch->player->CalledNames.Find(GET_ID(j))))
					continue;
				else if (!strcmp(j->Name(), i->Name()))	break;
			}

			// This prevents previous objects of the same type from being listed again.
			if ((GET_OBJ_TYPE(i) != ITEM_CONTAINER)	&& (j != i) &&
				(!IS_SITTABLE(i)) && (GET_OBJ_TYPE(i) != ITEM_VEHICLE)) {
				continue;
			}

			// Advance j to the point i has reached, skipping over other objects so we don't count them twice.
			sub_iter.Reset();
			while ((j = sub_iter.Next())) {
				if (j == i)							break;
			}

			// Add up the additional objects of the type
			do {
				// This keeps a named object from being included in the list of similar unnamed objects.
				// Es necesario. -DH
				if (!IS_NPC(ch) && (ch->player->CalledNames.Find(GET_ID(j))))
					continue;
				if (!strcmp(j->Name(), i->Name()))	++num;
			} while ((j = sub_iter.Next()));
		}

		if (i->CanBeSeen(ch)) {
			if ((GET_OBJ_TYPE(i) != ITEM_CONTAINER) && (num != 1) &&
					(!IS_SITTABLE(i)) && (GET_OBJ_TYPE(i) != ITEM_VEHICLE) &&
					(!IS_NPC(ch) && !(ch->player->CalledNames.Find(GET_ID(i)))))
				ch->Sendf("(x%i) ", num);
			if (!IS_SET(mode, ShowObj::Skip)) {
				show_obj_to_char(i, ch, mode, num);
				found = true;
			} else if (IS_SITTABLE(i)) {
				if ((get_num_chars_on_obj(i) < GET_OBJ_VAL(i, 0)) ||
					(ch->SittingOn() == i)) {
					show_obj_to_char(i, ch, mode, num);
					found = true;
				}
			} else if (IS_MOUNTABLE(i)) {
				if ((ch->SittingOn() == i) || !get_num_chars_on_obj(i)) {
					show_obj_to_char(i, ch, mode, num);
					found = true;
				}
			} else {
				show_obj_to_char(i, ch, mode, num);
				found = true;
			}
		}
	}

	if (!found && show)
		ch->Send("Nothing.\r\n");
}


void diag_char_to_char(Character * i, Character * ch) {
	int percent;
	char *buf;

	if (GET_MAX_HIT(i) > 0)		percent = (100 * GET_HIT(i)) / GET_MAX_HIT(i);
	else						percent = -1;		//	How could MAX_HIT be < 1??

	if (percent >= 100)			buf = "$N is in excellent condition.\r\n";
	else if (percent >= 90)		buf = "$N has a few scratches.\r\n";
	else if (percent >= 75)		buf = "$N has some small wounds and bruises.\r\n";
	else if (percent >= 50)		buf = "$N has quite a few wounds.\r\n";
	else if (percent >= 30)		buf = "$N has some big nasty wounds and scratches.\r\n";
	else if (percent >= 15)		buf = "$N looks pretty hurt.\r\n";
	else if (percent >= 0)		buf = "$N is in awful condition.\r\n";
	else						buf = "$N is bleeding awfully from big wounds.\r\n";

	act(buf, TRUE, ch, 0, i, TO_CHAR);
}


void look_at_char(Character * i, Character * ch) {
	int j, found;
	LListIterator<Object *>	iter(i->contents);
	Object *tmp_obj;

	if (!ch->desc)
		return;

	if (i->SSLDesc())
		ch->Send(i->LDesc());
	else
		act("You see nothing special about $m.", FALSE, i, 0, ch, TO_VICT);

	diag_char_to_char(i, ch);

	act("$n is using:", FALSE, i, 0, ch, TO_VICT);

	if (!look_at_equipped(ch, i))
		ch->Send(" Nothing.\r\n");

	if (ch != i && (IS_STAFF(ch)) || (SKILLROLL(ch, SKILL_STREETWISE) >= 0)) {
		found = false;
		act("\r\nYou attempt to peek at $s inventory:", FALSE, i, 0, ch, TO_VICT);
		while ((tmp_obj = iter.Next())) {
			if (tmp_obj->CanBeSeen(ch)) {
				show_obj_to_char(tmp_obj, ch, ShowObj::Merge | ShowObj::Name | ShowObj::Flags);
				found = true;
			}
		}

		if (!found)
			ch->Send("You can't see anything.\r\n");
	}
}


void list_one_char(Character * i, Character * ch) {
	static const char *positions[] = {
		" is lying here, dead",
		" is lying here, mortally wounded",
		" is lying here, incapacitated",
		" is lying here, stunned",
		" is sleeping here",
		" is resting here",
		" is sitting here",
		" is here, fighting",
		" is standing here",
		" is here, riding"
	};
	static const char *sit_positions[] = {
		"!DEAD!",
		"!MORTALLY WOUNDED!",
		"!INCAPACITATED!",
		"!STUNNED!",
		"sleeping",
		"resting",
		"sitting",
		"!FIGHTING!",
		"!STANDING!",
		"riding"
	};

	if (IS_NPC(i) && i->SSSDesc() && GET_POS(i) == GET_DEFAULT_POS(i) && !RIDING(i) && !FIGHTING(i)) {
		// This is a mobile in its default position, not fighting or riding.
		// If the viewer is a player who has a name for this mob, show it as such, otherwise,
		// use the short description.
		if (!IS_NPC(ch) && ch->player->CalledNames.Find(GET_ID(i))) {
			// The actual callname determination may seem to be redundant here between this condition
			// and the next, but it is not.  Slightly different outcomes are given for the two cases.
			sprintf(buf, "%s%s", i->CalledName(ch), positions[(int) GET_POS(i)]);
			CAP(buf);
		} else {
			strcpy(buf, i->SDesc());
			buf[strlen(buf) - 2] = '\0';
		}
	} else {
		VNum	clan = GET_CLAN(i);
		bool	isClan = Clan::Find(clan) && (GET_CLAN_LEVEL(i) != Clan::Rank::Applicant);
		int pos = (int) GET_POS(i);

		if (GET_TRUST(ch) > 0 || (IS_NPC(i) && FIGHTING(i))) {
			strcpy(buf, i->CalledName(ch));
        } else if (IS_NPC(i)) {
			strcpy(buf, i->Name("Someone"));
        } else {
			// strcpy(buf, i->SDesc("Someone"));
			strcpy(buf, i->CalledName(ch));
		}
		CAP(buf);

		if (i->Mounted()) {
			switch (i->Mounted()->DataType()) {
			case Datatypes::Character:		pos = POS_RIDING; break;
			default:						break;
				}
			sprintf(buf + strlen(buf), "%s on %s", positions[pos], i->Mounted()->CalledName(ch));
		}

		if (FIGHTING(i)) {
			if (!i->Mounted()) {
				strcat(buf, " is here");
			}
			if (FIGHTING(i) == ch)		strcat(buf, ", fighting YOU!");
			else {
				if (IN_ROOM(i) == IN_ROOM(FIGHTING(i)))
					sprintf(buf + strlen(buf), ", fighting %s!", PERS(FIGHTING(i), ch));
				else
					strcat(buf, ", fighting someone who has already left!");
			}
		} else if (!i->Mounted()) {
			strcat(buf, positions[pos]);
			strcat(buf, ".");
		} else {
			strcat(buf, ".");
		}
	}

	if (i->material == Materials::Ether)
		strcat(buf, " &c+&cw(ethereal)&c-");
	if (AFF_FLAGGED(i, AffectBit::Invisible))
		strcat(buf, " (invisible)");
	if (AFF_FLAGGED(i, AffectBit::Hide))
		strcat(buf, " (hidden)");
	if (AFF_FLAGGED(i, AffectBit::Camoflauge))
		strcat(buf, " (camoflauged)");
	if (!IS_NPC(i) && !i->desc)
		strcat(buf, " (linkless)");
	if (PLR_FLAGGED(i, PLR_WRITING))
		strcat(buf, " (writing)");
	if (PLR_FLAGGED(i, PLR_AFK))
		strcat(buf, " (AFK)");

	if (AFF_FLAGGED(ch, AffectBit::DetectAlign)) {
		if (IS_EVIL(i))
			strcat(buf, " &c+&cR(Red Aura)&c-");
		else if (IS_GOOD(i))
			strcat(buf, " &c+&cB(Blue Aura)&c-");
	}

	strcat(buf, "\r\n");
	if (AFF_FLAGGED(i, AffectBit::MageShield))
		sprintf(buf + strlen(buf), "&c+&cb%s has a softly glowing blue aura.&c-\r\n", HSSH(i));
	if (Affect::AffectedBy(i, SPELL_FREEZE_FOE)) {
		sprintf(buf + strlen(buf), "&c+&cC...%s is encased in a pillar of ice!&c-\r\n", HSSH(i));
	} else if (AFF_FLAGGED(i, AffectBit::Blind)) {
		sprintf(buf + strlen(buf), "&c+&cy...%s is groping around blindly!&c-\r\n", HSSH(i));
	}
	if (RIDING(i) && (AFF_FLAGGED(RIDING(i), AffectBit::Fly))) {
		sprintf(buf + strlen(buf), "&c+&cg...%s mount is hovering in the air.&c-\r\n", HSHR(i));
	} else if (AFF_FLAGGED(i, AffectBit::Fly)) {
		sprintf(buf + strlen(buf), "&c+&cg...%s is hovering in the air.&c-\r\n", HSSH(i));
	}

	if (AFF_FLAGGED(i, AffectBit::BladeBarrier))
		sprintf(buf + strlen(buf), "&c+&cC...%s is surrounded by whirling blades!&c-\r\n", HSSH(i));
	if (AFF_FLAGGED(i, AffectBit::FireShield))
		sprintf(buf + strlen(buf), "&c+&cR...%s is enveloped in a pillar of flame!&c-\r\n", HSSH(i));

	ch->MergeSendf("&cc%s", buf);
}



void list_char_to_char(LList<Character *> &list, Character * ch) {
	Character *i;

	START_ITER(iter, i, Character *, list) {
		if (ch != i) {
			if (i->CanBeSeen(ch) && !RIDER(i))
				list_one_char(i, ch);
			else if (IS_DARK(IN_ROOM(ch)) && !CAN_SEE_IN_DARK(ch) && HAS_INFRAVISION(i))
				ch->Send("&c+&cRYou see a pair of glowing red eyes looking your way.&c-\r\n");
		}
	}
}


void do_auto_exits(Character * ch, VNum room) {
	SInt8	door;
	SInt32	buflen = 0;
	char *buf = get_buffer(MAX_STRING_LENGTH);

	if (room < 0)
		return;

	for (door = 0; door < NUM_OF_DIRS; ++door)
		if (EXIT2(room, door) && EXIT2(room, door)->to_room != NOWHERE) {
			if (!IS_SET(EXIT2(room, door)->exit_info, Exit::Closed))
				sprintf(buf + strlen(buf), "%c ", LOWER(*dirs[door]));
			else if (!IS_SET(EXIT2(room, door)->exit_info, Exit::Hidden))
				sprintf(buf + strlen(buf), "(%c) ", LOWER(*dirs[door]));
		}

	ch->Sendf("&cB[ Exits: %s]&c0\r\n", *buf ? buf : "None! ");
	release_buffer(buf);
}


void do_long_exits(Character * ch, VNum room)
{
	int door, toroom;

	*buf = '\0';
	*buf2 = '\0';

	for (door = 0; door < NUM_OF_DIRS; ++door) {
		if (EXIT2(room, door) && (EXIT2(room, door)->to_room != NOWHERE)) {
			*buf2 = '\0';
			if (IS_STAFF(ch)) {
				sprintf(buf2, "%-5s - [%5d] &cB%s%s%s%s%s%s%s\r\n", dirs[door],
								world[EXIT2(room, door)->to_room].Virtual(),
								world[EXIT2(room, door)->to_room].Name(),
								IS_SET(EXIT2(room, door)->exit_info, Exit::Closed) ? " [CLOSED]" : "",
								IS_SET(EXIT2(room, door)->exit_info, Exit::Locked) ? " [LOCKED]" : "",
								IS_SET(EXIT2(room, door)->exit_info, Exit::Pickproof) ? " [!PICK]" : "",
								IS_SET(EXIT2(room, door)->exit_info, Exit::Hidden) ? " [SECRET]" : "",
								IS_SET(EXIT2(room, door)->exit_info, Exit::Vista) ? " [VISTA]" : "",
								IS_SET(EXIT2(room, door)->exit_info, Exit::SeeThrough) ? " [SEETHRU]" : "");
			} else {
				if (!(IS_SET(EXIT2(room, door)->exit_info, Exit::Hidden) &&
					 IS_SET(EXIT2(room, door)->exit_info, Exit::Closed)) &&
					 !IS_SET(EXIT2(room, door)->exit_info, Exit::SeeThrough)) {
						sprintf(buf2, "%-5s - ", dirs[door]);
					if (IS_SET(EXIT2(room, door)->exit_info, Exit::Closed)) {
						sprintf(buf2, "%-5s - The %s is closed.\r\n", dirs[door],
										(EXIT2(room, door)->Keyword()) ? fname(EXIT2(room, door)->Keyword()) : "door");
					} else {
						toroom = world[room].Direction(door)->to_room;
						if (!Room::Find(toroom)) {
							strcat(buf2, "<UNDEFINED>\r\n");
						} else if (IS_DARK(EXIT2(room, door)->to_room) && !CAN_SEE_IN_DARK(ch)) {
							strcat(buf2, "Too dark to tell\r\n");
						} else {
							strcat(buf2, world[EXIT2(room, door)->to_room].Name());
							strcat(buf2, "\r\n");
						}
					}
				}
			}
			sprintf(buf, "%s&cB%s&c0", buf, CAP(buf2));
		}
	}

	ch->Send("&cBObvious exits:\r\n");

	if (*buf)
		ch->Send(buf);
	else
		ch->Send(" None.\r\n");
}


#ifdef GOT_RID_OF_IT
ACMD(do_exits) {
	int door;
	char *buf2, *buf;

	if (AFF_FLAGGED(ch, AffectBit::Blind)) {
		ch->Send("You can't see a damned thing, you're blind!\r\n");
		return;
	}
	buf = get_buffer(MAX_STRING_LENGTH);
	buf2 = get_buffer(MAX_STRING_LENGTH);

	for (door = 0; door < NUM_OF_DIRS; door++)
		if (EXIT(ch, door) && EXIT(ch, door)->to_room != NOWHERE &&
/*		!IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED)*/
				!EXIT_IS_HIDDEN(ch, door)) {
			if (IS_STAFF(ch))
				sprintf(buf2, "%-5s - [%5d] %s\r\n", dirs[door],
						world[EXIT(ch, door)->to_room].Virtual(),
						world[EXIT(ch, door)->to_room].Name("<ERROR>"));
			else {
				sprintf(buf2, "%-5s - ", dirs[door]);
				if (IS_DARK(EXIT(ch, door)->to_room) && !CAN_SEE_IN_DARK(ch))
					strcat(buf2, "Too dark to tell\r\n");
				else {
					strcat(buf2, world[EXIT(ch, door)->to_room].Name("<ERROR>"));
					strcat(buf2, "\r\n");
				}
			}
			strcat(buf, CAP(buf2));
//			CAP(buf2);
		}

	release_buffer(buf2);

	ch->Sendf("Obvious exits:\r\n%s", *buf ? buf : " None.\r\n");

	release_buffer(buf);
}
#endif


ACMD(do_exits) {
	if (AFF_FLAGGED(ch, AffectBit::Blind)) {
		ch->Send("You can't see a damned thing, you're blind!\r\n");
		return;
	}
	do_long_exits(ch, IN_ROOM(ch));
}


void look_at_rnum(Character * ch, VNum room, int ignore_brief) {
	// char *buf;

	if (!ch->desc || room == NOWHERE)
		return;
	if (IS_DARK(room) && !CAN_SEE_IN_DARK(ch)) {
		ch->Send("It is pitch black...\r\n");
		return;
	} else if (AFF_FLAGGED(ch, AffectBit::Blind)) {
		ch->Send("You see nothing but infinite darkness...\r\n");
		return;
	}

	/* MAP ROOMS */
	if ((!PRF_FLAGGED(ch, Preference::Brief) || ignore_brief) &&
		(ROOM_FLAGGED(IN_ROOM(ch), ROOM_MAP))) {
		/* show map thing */
		printmap(IN_ROOM(ch), ch);
	}

	if ((ROOM_AFFECTED(IN_ROOM(ch), RAFF_FOG)) &&
			 (!PRF_FLAGGED(ch, Preference::HolyLight))) {
		 ch->Send("Your view is obscured by a thick fog.\r\n");
		 return;
	}

	ch->Send("&cM");
	if (PRF_FLAGGED(ch, Preference::RoomFlags)) {
		sprintbit(ROOM_FLAGS(room), room_bits, buf2);
		sprinttype(SECT(IN_ROOM(ch)), sector_types, buf3);
		strcpy(buf, world[room].Name("<ERROR>"));
		if (ROOM_FLAGGED(room, ROOM_PARSE))
			master_parser(buf, ch, &world[room], NULL);
		ch->Sendf("[%5d] %s [ %s] [%s]&c0\r\n", room, buf, buf2, buf3);
	} else if (ROOM_FLAGGED(room, ROOM_PARSE)) {
		strcpy(buf, world[room].Name("<ERROR>"));
		master_parser(buf, ch, &world[room], NULL);
		ch->Sendf("&cM%s&c0\r\n", buf);
	} else
		ch->Sendf("&cM%s&c0\r\n", world[room].Name("<ERROR>"));
	ch->Send("&c0");

	if (!ROOM_FLAGGED(room, ROOM_MAP)) {
		if (!PRF_FLAGGED(ch, Preference::Brief) || ignore_brief || ROOM_FLAGGED(room, ROOM_DEATH)) {
			if (ROOM_FLAGGED(room, ROOM_PARSE)) {
				strcpy(buf, world[room].Description("<ERROR>"));
				master_parser(buf, ch, &world[room], NULL);
				ch->Sendf("%s\r\n", buf);
			} else
				ch->Sendf("%s", world[room].Description("<ERROR>"));
		}
	}

	/* autoexits */
	if (PRF_FLAGGED(ch, Preference::AutoExit)) {
		if (!PRF_FLAGGED(ch, Preference::Brief) || ignore_brief)
			do_auto_exits(ch, room);
	    else
			do_long_exits(ch, room);
	}

	/* now list characters & objects */
	ch->Send("&cg");
	list_obj_to_char(world[room].contents, ch, (Flags) (ShowObj::Merge | ShowObj::SDesc | ShowObj::Flags), FALSE);
	ch->Send("&cc");
	list_char_to_char(world[room].people, ch);
	ch->Send("&c0");

    // CHANGEPOINT:  This is related to space.C
	// Ship::ShowShips(ch, world[room].ships);
}


void look_at_room(Character * ch, int ignore_brief) {
	if (IN_ROOM(ch) <= NOWHERE) {
		ch->ToRoom(0);
	}
	look_at_rnum(ch, IN_ROOM(ch), ignore_brief);
}


void list_scanned_chars(LList<Character *> &list, Character * ch, int distance, int door) {
	char			relation;
	Character *		i;
	UInt32			count = 0, one_sent = 0;
	LListIterator<Character *>	iter(list);

	while((i = iter.Next())) {
		if (i->CanBeSeen(ch))
			count++;
	}

	if (!count)
		return;

	iter.Reset();
	while((i = iter.Next())) {
		if (!i->CanBeSeen(ch))
			continue;

		relation = relation_colors[ch->GetRelation(i)];

		if (!one_sent)			ch->Sendf("You see &c%c%s", relation, i->Name());
		else 					ch->Sendf("&c%c%s", relation, i->Name());

		if (--count > 1)		ch->Send("&c0, ");
		else if (count == 1)	ch->Send(" &c0and ");
		else					ch->Sendf(" &c0%s %s.\r\n", distance_lower[distance], dir_text_to[door]);

		one_sent++;
	}
}


void scan_direction(Character * ch, SInt32 &dir, SInt32 &maxDist)
{
	VNum	room = IN_ROOM(ch);
	int		distance = 0;

	do {
		room = CAN_GO2(room, dir) ? EXIT2(room, dir)->to_room : NOWHERE;
		++distance;
		list_scanned_chars(world[room].people, ch, distance, dir);
	} while ((room != NOWHERE) && (distance < maxDist));
}


void look_in_direction(Character * ch, SInt32 dir, SInt32 maxDist) {
	VNum	room = IN_ROOM(ch);

	if (EXIT(ch, dir)) {
		ch->Sendf("%s", EXIT(ch, dir)->Description("You see nothing special.\r\n"));

		if (EXIT(ch, dir)->Keyword()) {
			if (EXIT_FLAGGED(EXIT(ch, dir), Exit::Closed))
				act("The $T is closed.\r\n", FALSE, ch, 0, fname(EXIT(ch, dir)->Keyword()), TO_CHAR);
			else if (EXIT_FLAGGED(EXIT(ch, dir), Exit::Door))
				act("The $T is open.\r\n", FALSE, ch, 0, fname(EXIT(ch, dir)->Keyword()), TO_CHAR);
		}
		scan_direction(ch, dir, maxDist);
	} else
		ch->Send("Nothing special there...\r\n");
}


/* do_scan command revised by Daniel Rollings AKA Daniel Houghton */
ACMD(do_scan)
{
  /* >scan
     You quickly scan the area.
     You see John, a large horse and Frank close by north.
     You see a small rabbit a ways off south.
     You see a huge dragon and a griffon far off to the west.

  */
  int door;
  SInt32 maxDist = 3;

  *buf = '\0';

  if (AFF_FLAGGED(ch, AffectBit::Blind)) {
    ch->Send("You can't see a damned thing, you're blind!\r\n");
    return;
  }

  /* may want to add more restrictions here, too */
  ch->Send("You quickly scan the area.\r\n");

  for (door = 0; door < NUM_OF_DIRS; ++door)
	scan_direction(ch, door, maxDist);

}


void look_in_obj(Character * ch, char *arg) {
	Object *obj = NULL;
	Character *dummy = NULL;
	int amt, bits;

	if (!*arg)
		ch->Send("Look in what?\r\n");
	else if (!(bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP, ch, &dummy, &obj)))
		ch->Sendf("There doesn't seem to be %s %s here.\r\n", AN(arg), arg);
	else {
    	switch (GET_OBJ_TYPE(obj)) {
        case ITEM_DRINKCON:
        case ITEM_FOUNTAIN:
			if (GET_OBJ_VAL(obj, 1) <= 0)		/* item must be a fountain or drink container */
				ch->Send("It is empty.\r\n");
			else if (GET_OBJ_VAL(obj,0) <= 0 || GET_OBJ_VAL(obj,1)>GET_OBJ_VAL(obj,0))
				ch->Send("Its contents seem somewhat murky.\r\n"); /* BUG */
			else {
				char *buf2 = get_buffer(128);
				amt = (GET_OBJ_VAL(obj, 1) * 3) / GET_OBJ_VAL(obj, 0);
				sprinttype(GET_OBJ_VAL(obj, 2), color_liquid, buf2);
				ch->Sendf("It's %sfull of a %s liquid.\r\n", fullness[amt], buf2);
				release_buffer(buf2);
			}
        break;
        case ITEM_CONTAINER:
        case ITEM_WEAPONCON:
			if (OBJVAL_FLAGGED(obj, CONT_CLOSED))
				ch->Send("It is closed.\r\n");
			else {
				ch->Send(fname(obj->Keywords()));
				switch (bits) {
					case FIND_OBJ_INV:
						ch->Send(" (carried): \r\n");
						break;
					case FIND_OBJ_ROOM:
						ch->Send(" (here): \r\n");
						break;
					case FIND_OBJ_EQUIP:
						ch->Send(" (used): \r\n");
						break;
				}

				list_obj_to_char(obj->contents, ch, (Flags) (ShowObj::Merge | ShowObj::Name), TRUE);
			}
        	break;
        default:
			ch->Send("There's nothing inside that!\r\n");
            break;
        }
	}
}



const char *ExtraDesc::Find(const char *word, LList<ExtraDesc *> &list) {
	ExtraDesc *i;

	LListIterator<ExtraDesc *>	iter(list);
	while ((i = iter.Next()))
		if (isname(word, SSData(i->keyword)))
			return SSData(i->description);

	return NULL;
}


/*
 * Given the argument "look at <target>", figure out what object or char
 * matches the target.  First, see if there is another char in the room
 * with the name.  Then check local objs for exdescs.
 */
void look_at_target(Character * ch, char *arg, int read, int show) {
	int bits, j;
	IDNum msg = 1;
	Character *found_char = NULL;
	Object *obj = NULL, *found_obj = NULL;
	const char *	desc;
	char *	number;
	bool	found = false;

	if (!ch->desc)			return;
	else if (!*arg)			ch->Send("Look at what?\r\n");
	else if (read) {
		if (!(obj = get_obj_in_list_type(ITEM_BOARD, ch->contents)))
			obj = get_obj_in_list_type(ITEM_BOARD, world[IN_ROOM(ch)].contents);

		if (obj) {
			number = get_buffer(MAX_INPUT_LENGTH);
			one_argument(arg, number);
			if (!*number)
				ch->Send("Read what?\r\n");
			else if (isname(number, obj->Name()))
				Board::Show(obj->Virtual(), ch);
			else if (!isdigit(*number) || !(msg = atoi(number)) || strchr(number,'.'))
				look_at_target(ch, arg, 0, show);
			else
				Board::DisplayMessage(obj->Virtual(), ch, msg);
			release_buffer(number);
		} else	look_at_target(ch, arg, 0, show);
	} else {
		bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP |
				FIND_CHAR_ROOM, ch, &found_char, &found_obj);

		/* Is the target a character? */
		if (found_char) {
			look_at_char(found_char, ch);
			if (ch != found_char) {
				if (ch->CanBeSeen(found_char))
					act("$n looks at you.", TRUE, ch, 0, found_char, TO_VICT);
				act("$n looks at $N.", TRUE, ch, 0, found_char, TO_NOTVICT);
			}
			return;
		}
		/* Does the argument match an extra desc in the room? */
		if ((desc = ExtraDesc::Find(arg, world[IN_ROOM(ch)].ex_description)) != NULL) {
			page_string(ch->desc, desc, false);
			return;
		}
		/* Does the argument match an extra desc in the char's equipment? */
		for (j = 0; j < NUM_WEARS && !found; j++)
			if (GET_EQ(ch, j) && (GET_EQ(ch, j))->CanBeSeen(ch))
				if ((desc = ExtraDesc::Find(arg, GET_EQ(ch, j)->exDesc)) != NULL) {
					ch->Send(desc);
					found = true;
				}
		/* Does the argument match an extra desc in the char's inventory? */
		START_ITER(ch_iter, obj, Object *, ch->contents) {
			if (found)
				break;
			if (obj->CanBeSeen(ch) && (desc = ExtraDesc::Find(arg, obj->exDesc))) {
				if(GET_OBJ_TYPE(obj) == ITEM_BOARD)
					Board::Show(obj->Virtual(), ch);
				else
					ch->Send(desc);
				found = true;
			}
		}

		/* Does the argument match an extra desc of an object in the room? */
		START_ITER(rm_iter, obj, Object *, world[IN_ROOM(ch)].contents) {
			if (!obj->CanBeSeen(ch))							continue;
			if (!(desc = ExtraDesc::Find(arg, obj->exDesc)))	continue;

			if(GET_OBJ_TYPE(obj) == ITEM_BOARD)	Board::Show(obj->Virtual(), ch);
			else								ch->Send(desc);

			found = true;
			break;
		}

		if (bits) {			//	If an object was found back in generic_find
			// if (!found)	show_obj_to_char(found_obj, ch, ShowObj::Merge | ShowObj::Name);		//	Show no-description
			// else		show_obj_to_char(found_obj, ch, ShowObj::Merge | ShowObj::Name | ShowObj::Flags);		//	Find hum, glow etc
			if (!found)	show_obj_to_char(found_obj, ch, ShowObj::Examine | ShowObj::Flags);	//	Show no-description
			else		show_obj_to_char(found_obj, ch, ShowObj::Flags);				//	Find hum, glow etc
		} else if (!found)
			ch->Send("You do not see that here.\r\n");
	}
}


void look_out(Character * ch) {
	Object * viewport, * vehicle;
	viewport = get_obj_in_list_type(ITEM_V_WINDOW, world[IN_ROOM(ch)].contents);
	if (!viewport) {
		ch->Send("There is no window to see out of.\r\n");
		return;
	}
	vehicle = find_vehicle_by_vnum(GET_OBJ_VAL(viewport, 0));
	if (!vehicle || IN_ROOM(vehicle) == NOWHERE) {
		ch->Send("All you see is an infinite blackness.\r\n");
		return;
	}
	look_at_rnum(ch, IN_ROOM(vehicle), FALSE);
	return;
}


ACMD(do_look) {
	int look_type;

	if (!ch->desc)
		return;

	if (GET_POS(ch) < POS_SLEEPING)
		ch->Send("You can't see anything but stars!\r\n");
	else if (AFF_FLAGGED(ch, AffectBit::Blind))
		ch->Send("You can't see a damned thing, you're blind!\r\n");
	else if (IS_DARK(IN_ROOM(ch)) && !CAN_SEE_IN_DARK(ch)) {
		ch->Send("It is pitch black...\r\n");
		list_char_to_char(world[IN_ROOM(ch)].people, ch);	/* glowing red eyes */
	} else {
		char * arg2 = get_buffer(MAX_INPUT_LENGTH);
		char * arg = get_buffer(MAX_INPUT_LENGTH);

		half_chop(argument, arg, arg2);

		if (subcmd == SCMD_READ) {										//	read
			if (!*arg)	ch->Send("Read what?\r\n");
			else		look_at_target(ch, arg, 1, ShowObj::Examine);
		} else if (!*arg)												//	look
			look_at_room(ch, 1);
		else if (is_abbrev(arg, "in"))									//	look in
			look_in_obj(ch, arg2);
		else if (is_abbrev(arg, "out"))									//	look out
			look_out(ch);
		else if ((look_type = search_block(arg, (const char **)dirs, FALSE)) >= 0)		//	look <dir>
			look_in_direction(ch, look_type, MAX_DISTANCE);
		else if (is_abbrev(arg, "at"))									//	look at
			look_at_target(ch, arg2, 0, ShowObj::Examine | ShowObj::Flags);
		else
			look_at_target(ch, arg, 0, ShowObj::Examine | ShowObj::Flags);									//	look <something>

		release_buffer(arg);
		release_buffer(arg2);
	}
}



ACMD(do_examine) {
	int bits;
	Character *tmp_char;
	Object *tmp_object;
	char * arg = get_buffer(MAX_STRING_LENGTH);

	one_argument(argument, arg);

	if (!*arg) {
		ch->Send("Examine what?\r\n");
	} else {
		look_at_target(ch, arg, 0, ShowObj::Examine | ShowObj::Flags);

		bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_CHAR_ROOM |
					FIND_OBJ_EQUIP, ch, &tmp_char, &tmp_object);

		if (tmp_object) {
			if ((GET_OBJ_TYPE(tmp_object) == ITEM_DRINKCON) ||
					(GET_OBJ_TYPE(tmp_object) == ITEM_FOUNTAIN) ||
					(GET_OBJ_TYPE(tmp_object) == ITEM_WEAPONCON) ||
					(GET_OBJ_TYPE(tmp_object) == ITEM_CONTAINER)) {
				ch->Send("When you look inside, you see:\r\n");
				look_in_obj(ch, arg);
			}
		}
	}
	release_buffer(arg);
}


ACMD(do_gold) {
	if (GET_GOLD(ch) == 0)
		ch->Send("You're broke!\r\n");
	else if (GET_GOLD(ch) == 1)
		ch->Send("You have one miserable little gold coin.\r\n");
	else {
		ch->Sendf("You have %d gold coins.\r\n", GET_GOLD(ch));
	}
}


char *make_bar(long val, long max, long len)
{
	static char outbuf[MAX_INPUT_LENGTH];
	long i, n;
	char *pos = outbuf;

	strcpy(pos++, "(&cb");
	if (val > max)
	{
		for (i = 0; i < len; ++i)
			*(pos++) = '*';
		strcpy(pos, "&cc)&cw+");
	} else {
		for (i = (val * len) / max, n = 0; n < i; ++n)
			*(pos++) = '*';
		while ((n++)<len)
			*(pos++) = ' ';
		strcpy(pos, "&cc)&cw");
	}
	return outbuf;
}


const char *rate_align(int value)
{
if (value <= -900)
   return("a sadistic person!");
else if (value <= -501)
   return("an evil person.");
else if (value <= 500)
   return("an okay person.");
else if (value <= 900)
   return("a good person.");
else
   return("a saint!");
}


ACMD(do_score) {
	char *buf = get_buffer(2400);
	char *buf2 = get_buffer(2400);
	char *buf3 = get_buffer(2400);

	extern Character *mob_proto;
	struct TimeInfoData playing_time;
	struct TimeInfoData real_time_passed(time_t t2, time_t t1);
	static char bufs[3][128];
	static char bufs2[3][128];
	static char bufs3[3][128];
	int l, i, next_level=0;

	SInt8 dicenum, dicemod;

	extern const char *encumbrance_levels[];

	sprintf(buf, "  &bb%39.39s %s%15s&c0\r\n\n",
	        "&cYScore information for", ch->Name(), "");

	sprintf(buf + strlen(buf),
	  "&ccRace     &cL:&cG %-12s  "
	  "&ccStrength      &cL:&cG %3d  "
	  "&ccHealth        &cL:&cG %d/%d\r\n"
	  "&ccSex      &cL:&cG %-12s  "
	  "&ccDexterity     &cL:&cG %3d  "
	  "&ccGold          &cL:&cG %d\r\n"
	  "&ccAge      &cL:&cG %-12d  "
	  "&ccConstitution  &cL:&cG %3d  "
	  "&ccIn Bank       &cL:&cG %d\r\n",
	  race_types[GET_RACE(ch)], GET_STR(ch), GET_HIT(ch), GET_MAX_HIT(ch),
	  genders[(int) GET_SEX(ch)], GET_DEX(ch), GET_GOLD(ch),
	  GET_AGE(ch), GET_CON(ch), GET_BANK_GOLD(ch));

	sprintf(buf3, "%d' %d\"",
	              ((int) (GET_HEIGHT(ch) / 2.54)) / 12,
	              ((int) (GET_HEIGHT(ch) / 2.54)) % 12);
	str_thrust_dice(GET_STR(ch), dicenum, dicemod);

	sprintf(buf + strlen(buf),
	  "&ccLevel    &cL:&cG %-12d  "
	  "&ccWisdom        &cL:&cG %3d  "
	  "&ccMove Rate     &cL:&cG %d\r\n"
	  "&ccHeight   &cL:&cG %-12s  "
	  "&ccIntelligence  &cL:&cG %3d  "
	  "&ccThrust Damage &cL:&cG %dd6%s%d\r\n",
	  GET_LEVEL(ch),
	  GET_WIS(ch),
	  GET_MOVEMENT(ch),
	  buf3,
	  GET_INT(ch),
	  dicenum,
	  (dicemod + GET_DAMROLL(ch) >= 0 ? "+" : ""),
	  dicemod + GET_DAMROLL(ch));

	str_swing_dice(GET_STR(ch), dicenum, dicemod);

	sprintf(buf3, "%d lbs.", GET_WEIGHT(ch));

	sprintf(buf + strlen(buf),
	  "&ccWeight   &cL:&cG %-12s  "
	  "&ccCharisma      &cL:&cG %3d  "
	  "&ccSwing Damage  &cL:&cG %dd6%s%d\r\n\n",
	  buf3,
	  GET_CHA(ch),
	  dicenum,
	  (dicemod + GET_DAMROLL(ch) >= 0 ? "+" : ""),
	  dicemod + GET_DAMROLL(ch));

	*buf2 = '\0';
	*buf3 = '\0';

	 for (i = 0; i < 32; ++i) {
	   if (ADVANTAGED(ch, 1 << i)) {
	     if (*buf2) {
	       sprintf(buf2 + strlen(buf2), ", %s", named_advantages[i]);
	     } else {
	       strcpy(buf2, named_advantages[i]);
	     }
	   }
	 }

	 for (i = 0; i < 32; ++i) {
	   if (DISADVANTAGED(ch, 1 << i)) {
	     if (*buf3) {
	       sprintf(buf3 + strlen(buf3), ", %s", named_disadvantages[i]);
	     } else {
	       strcpy(buf3, named_disadvantages[i]);
	     }
	   }
	 }

	 if (*buf2)
	   sprintf(buf + strlen(buf), "&ccAdvantages:  &cm%s\r\n", buf2);

	 if (*buf3)
	   sprintf(buf + strlen(buf), "&ccDisadvantages:  &cm%s\r\n", buf3);

	 sprintf(buf + strlen(buf), "&ccEncumbrance:  &cm%s&cc (Carried: %d lbs.)&c0\r\n\r\n",
	        encumbrance_levels[GET_ENCUMBRANCE(ch, IS_CARRYING_W(ch))],
	  IS_CARRYING_W(ch));

	 sprintf(buf + strlen(buf), "You are %s\r\n", rate_align(GET_ALIGNMENT(ch)));
	 sprintf(buf + strlen(buf), "You have a total worth of %d points.\r\n",
	         GET_TOTALPTS(ch));

	if ((age(ch).month == 0) && (age(ch).day == 0))
	  strcat(buf, "->  It's your birthday today.\r\n");

	if (!IS_NPC(ch)) {
	    next_level = GET_LEVEL(ch) + 1;
	    sprintf(buf, "%sYou need %d exp to reach your next level.\r\n", buf,
	      (EXP_TO_LEVEL - GET_EXP(ch)));

	  sprintf(buf, "%sYou have been playing for %d days and %d hours.\r\n",
	  		buf, playing_time.day, playing_time.hours);

	}

#ifdef GOT_RID_OF_IT
	strcat(buf, "You are ");
	switch (GET_POS(ch)) {
// CHANGEPOINT
	case POS_PRONE:
	  strcat(buf, "prone");
	  break;
	case POS_RESTING:
	  strcat(buf, "resting");
	  break;
	case POS_SITTING:
	  strcat(buf, "sitting");
	  break;
	case POS_STANDING:
	  strcat(buf, "standing");
	  break;
	case POS_RIDING:
	  strcat(buf, "standing");
	  break;
	default:
	  strcat(buf, "floating");
	  break;
	}

	strcat(buf, ", and ");
	switch (GET_STATE(ch)) {
	case STATE_DEAD:
	  strcat(buf, "DEAD!\r\n");
	  break;
	case STATE_MORTALLYW:
	  strcat(buf, "mortally wounded!  You should seek help!\r\n");
	  break;
	case STATE_INCAP:
	  strcat(buf, "incapacitated, slowly fading away...\r\n");
	  break;
	case STATE_STUNNED:
	  strcat(buf, "stunned!  You can't move!\r\n");
	  break;
	case STATE_PARALYZED:
	  strcat(buf, "paralyzed!  You can't move!\r\n");
	  break;
	case STATE_FROZEN:
	  strcat(buf, "frozen!  You can't move!\r\n");
	  break;
	case STATE_SLEEPING:
	  strcat(buf, "sleeping.\r\n");
	  break;
	case STATE_FIGHTING:
	  sprintf(buf + strlen(buf), "are fighting %s.\r\n", PERS(FIGHTING(ch), ch));
	  break;
	case STATE_AWAKE:
	  strcat(buf, "awake.\r\n");
	  break;
	default:
		strcat(buf, "&cRneed to report a status bug to the admins!&c0\r\n");
	  break;
	}
#endif

	switch (GET_POS(ch)) {
	case POS_DEAD:
		strcat(buf, "You are DEAD!\r\n");
		break;
	case POS_MORTALLYW:
		strcat(buf, "You are mortally wounded!  You should seek help!\r\n");
		break;
	case POS_INCAP:
		strcat(buf, "You are incapacitated, slowly fading away...\r\n");
		break;
	case POS_STUNNED:
		strcat(buf, "You are stunned!  You can't move!\r\n");
		break;
	case POS_SLEEPING:
		strcat(buf, "You are sleeping.\r\n");
		break;
	case POS_RESTING:
		strcat(buf, "You are resting.\r\n");
		break;
	case POS_SITTING:
		strcat(buf, "You are sitting.\r\n");
		break;
	case POS_STANDING:
		strcat(buf, "You are standing.\r\n");
		break;
	default:
		strcat(buf, "You are floating.\r\n");
		break;
	}

	if (GET_COND(ch, DRUNK) > 10)
	  strcat(buf, "You are intoxicated.\r\n");

	if (GET_COND(ch, FULL) == 0)
	  strcat(buf, "You are hungry.\r\n");

	if (GET_COND(ch, THIRST) == 0)
	  strcat(buf, "You are thirsty.\r\n");

	if (AFF_FLAGGED(ch, AffectBit::Blind))
	  strcat(buf, "You have been blinded!\r\n");

	if (AFF_FLAGGED(ch, AffectBit::Invisible))
	  strcat(buf, "You are invisible.\r\n");

	if (AFF_FLAGGED(ch, AffectBit::DetectInvis))
	  strcat(buf, "You are sensitive to the presence of invisible things.\r\n");

	if (AFF_FLAGGED(ch, AffectBit::Regeneration))
	  strcat(buf, "Your wounds are healing before your eyes!\r\n");

	if (AFF_FLAGGED(ch, AffectBit::VampRegen))
	  strcat(buf, "You feel consumed with an unholy hunger.\r\n");

    if (Affect::AffectedBy(ch, SPELL_HOLY_ARMOR))
	  strcat(buf, "You are protected by the Powers!\r\n");

	if (AFF_FLAGGED(ch, AffectBit::Poison))
	  strcat(buf, "You are poisoned!\r\n");

	if (AFF_FLAGGED(ch, AffectBit::Charm))
	  strcat(buf, "You have been charmed!\r\n");

    if (Affect::AffectedBy(ch, SPELL_ARMOR))
	  strcat(buf, "You feel protected.\r\n");

	if (AFF_FLAGGED(ch, AffectBit::Infravision))
	  strcat(buf, "Your eyes have a faint red glow.\r\n");

	if (PRF_FLAGGED(ch, Preference::Summonable))
	  strcat(buf, "You are summonable by other players.\r\n");

	if (AFF_FLAGGED(ch, AffectBit::BladeBarrier))
	  strcat(buf, "You are protected by a barrier of whirling blades.\r\n");

	if (AFF_FLAGGED(ch, AffectBit::FireShield))
	  strcat(buf, "You are shielded by a pillar of fire.\r\n");

    if (Affect::AffectedBy(ch, SPELL_FAERIE_FIRE))
	  strcat(buf, "You are enveloped by faerie fire.\r\n");

	if (AFF_FLAGGED(ch, AffectBit::DetectAlign))
	  strcat(buf, "You have a peculiar awareness of good and evil intent.\r\n");

    if (Affect::AffectedBy(ch, SPELL_LEVITATE))
	  strcat(buf, "You are levitating.\r\n");

	if (AFF_FLAGGED(ch, AffectBit::Haste))
	  strcat(buf, "You are hasted.\r\n");

    if (Affect::AffectedBy(ch, SPELL_STONESKIN))
	  strcat(buf, "Your skin is as hard as stone.\r\n");

    if (Affect::AffectedBy(ch, SPELL_PROT_FROM_EVIL))
	  strcat(buf, "You feel safe from evil.\r\n");

	ch->Send(buf);

	release_buffer(buf);
	release_buffer(buf2);
	release_buffer(buf3);

#ifdef GOT_RID_OF_IT
	Character *	vict;
	char *		arg;

//	if (IS_NPC(ch))		return;

	arg = get_buffer(MAX_INPUT_LENGTH);
	one_argument(argument, arg);
	if (!*arg)	vict = ch;
	else {
		vict = get_player_vis(ch, arg, false);

		if (!vict)	ch->Send("No such player.\r\n");
	}
	release_buffer(arg);

	if (!vict)	return;

	ch->Send("%s %s\r\n"
			 "%*s\r\n",
			 IS_STAFF(vict) ? vict->Name() : (IS_NPC(vict) ? vict->Name() : GET_TITLE(vict)),
			 IS_STAFF(vict) ? GET_TITLE(vict) : (IS_NPC(vict) ? "(MOB)" : vict->Name()),
			 strlen(vict->Name()) + strlen(GET_TITLE(vict)) + 1,
			 "-----------------------------------------------------");
	ch->Send("Level   : %d\r\n"
			 "Race    : %s\r\n"
//			 "Age     : %d\r\n"
			 "Hits    : %-3d/%3d   Stamina: %-3d/%3d\r\n"
			 "Gold      : %d\r\n",
			GET_LEVEL(vict),
			RACE_ABBR(ch),
//			GET_AGE(vict), // (!age(vict).month && !age(vict).day) ? "  It's your birthday today." : "",
			GET_HIT(vict), GET_MAX_HIT(vict), GET_MOVE(vict), GET_MAX_MOVE(vict),
			GET_AC(vict),
			GET_MISSION_POINTS(vict));

	ch->Send("Co: %3d  Ag: %3d  SD: %3d  Me: %3d  Re: %3d\r\n"
			 "St: %3d  Qu: %3d  Pr: %3d  In: %3d  Em: %3d\r\n",
			GET_STAT(ch, Co), GET_STAT(ch, Ag), GET_STAT(ch, SD), GET_STAT(ch, Me), GET_STAT(ch, Re),
			GET_STAT(ch, St), GET_STAT(ch, Qu), GET_STAT(ch, Pr), GET_STAT(ch, In), GET_STAT(ch, Em));

	if (!IS_NPC(vict)) {
		SInt32	pKills = vict->player->special.PKills,
				mKills = vict->player->special.MKills,
				pDeaths = vict->player->special.PDeaths,
				mDeaths = vict->player->special.MDeaths;
		SInt32	killTotal = pKills + mKills,
				deathTotal = pDeaths + mDeaths;


		if (killTotal == 0)		killTotal = 1;
		if (deathTotal == 0)	deathTotal = 1;

		ch->Send(
				" +--------------------------------------+\r\n"
				" |          Kills          Deaths       |\r\n"
				" | Players: %-4d [%4.1f%%] | %-4d [%4.1f%%] |\r\n"
				" | Mobs   : %-4d [%4.1f%%] | %-4d [%4.1f%%] |\r\n"
				" | Total  : %-4d         | %-4d         |\r\n"
//				" | Kill Score: %8.2f    Ratio: N/A   |\r\n"
				" +--------------------------------------+\r\n",
				pKills, pKills * 100.0 / killTotal,		pDeaths, pDeaths * 100.0 / deathTotal,
				mKills, mKills * 100.0 / killTotal,		mDeaths, mDeaths * 100.0 / deathTotal,
				killTotal,								deathTotal
//				vict->player->special.killScore
		);

		TimeInfoData playing_time = real_time_passed((time(0) - vict->player->time.logon) + vict->player->time.played, 0);

		ch->Sendf("PlayTime: %dd %dh\r\n",
				playing_time.day, playing_time.hours);
	}

	ch->Sendf("Status  : %s", positions[GET_POS(vict)]);
	if (GET_POS(vict) == POS_FIGHTING)
		ch->Sendf(" %s", FIGHTING(vict) ? PERS(FIGHTING(vict), ch) : "a shadow");
	ch->Send("\r\n");
/*	switch (GET_POS(vict)) {
		case POS_DEAD:		ch->Send("Status  : DEAD\r\n");				break;
		case POS_MORTALLYW:	ch->Send("Status  : Mortally Wounded\r\n");	break;
		case POS_INCAP:		ch->Send("Status  : Incapacitated\r\n");	break;
		case POS_STUNNED:	ch->Send("Status  : Stunned\r\n");			break;
		case POS_SLEEPING:	ch->Send("Status  : Sleeping\r\n");			break;
		case POS_RESTING:	ch->Send("Status  : Resting\r\n");			break;
		case POS_SITTING:	ch->Send("Status  : Sitting\r\n");			break;
		case POS_FIGHTING:	ch->Sendf("Status  : Fighting %s\r\n",
				FIGHTING(vict) ? PERS(FIGHTING(vict), ch) : "a shadow");
				break;
		case POS_STANDING:	ch->Send("Status  : Standing\r\n");			break;
		default:			ch->Send("Status  : Floating\r\n");			break;
	}
*/

	if (AFF_FLAGGED(vict, AffectBit::Blind | AffectBit::Invisible | AffectBit::Sanctuary |
			AffectBit::Poison | AffectBit::Charm) || (GET_COND(vict, DRUNK) > 10) ||
			!GET_COND(vict, FULL) || !GET_COND(vict, THIRST)) {
		ch->Send("Ailments:");
		if (GET_COND(vict, DRUNK) > 10)					ch->Send(" Intoxicated");
		if (!GET_COND(vict, FULL))						ch->Send(" Hungry");
		if (!GET_COND(vict, THIRST))					ch->Send(" Thirsty");
		if (AFF_FLAGGED(vict, AffectBit::Blind))		ch->Send(" Blind");
		if (AFF_FLAGGED(vict, AffectBit::Invisible))	ch->Send(" Invisible");
		if (AFF_FLAGGED(vict, AffectBit::Sanctuary))	ch->Send(" Shielded");
		if (AFF_FLAGGED(vict, AffectBit::Poison))		ch->Send(" Poisoned");
		if (AFF_FLAGGED(vict, AffectBit::Charm))		ch->Send(" Being Manipulated");
		ch->Send("\r\n");
	}

	if (AFF_FLAGGED(vict, AffectBit::DetectInvis | AffectBit::Infravision)) {
		ch->Send("Can See : ");
		if (AFF_FLAGGED(vict, AffectBit::DetectInvis))	ch->Send("Invisible");
		if (AFF_FLAGGED(vict, AffectBit::Infravision))	ch->Send("Infravision");
	}
#endif
}


ACMD(do_inventory) {
	ch->Send("You are carrying:\r\n");
	list_obj_to_char(ch->contents, ch, ShowObj::Merge | ShowObj::Name | ShowObj::Flags | ShowObj::Weight, TRUE);
}


// Routine separated from do_equipment and look_at_char to eliminate
// redundancy.	- DH
int look_at_equipped(Character *ch, Character *target) {
	int i, found = 0;
	UInt8 msgnum;
	Bodytype bodytype = GET_BODYTYPE(target);

	int wear_message_number(Object *obj, int where);

	for (i = 0; i < NUM_WEARS; ++i) {
		if (GET_EQ(target, i)) {
			msgnum = GET_WEARSLOT(i, bodytype).msg_wearon;

			// Special handling for objects held in hands/claws/etc.
			if (i >= WEAR_HAND_R) {
				if (GET_OBJ_TYPE(GET_EQ(target, i)) == ITEM_WEAPON) {
					msgnum = WEAR_MSG_WIELD;
				} else if (CAN_WEAR(GET_EQ(target, i), ITEM_WEAR_SHIELD)) {
					msgnum = WEAR_MSG_STRAPTO;
				}
				if (OBJ_FLAGGED(GET_EQ(target, i), ITEM_TWO_HAND)) {
					sprintf(buf2, "%s both %ss", wear_slot_messages[msgnum].worn_msg,
									GET_WEARSLOT(i, bodytype).partname);
				} else {
					sprintf(buf2, "%s %s %s", wear_slot_messages[msgnum].worn_msg,
									GET_WEARSLOT(i, bodytype).keywords,
									GET_WEARSLOT(i, bodytype).partname);
				}

			} else {
				sprintf(buf2, "%s %s", wear_slot_messages[msgnum].worn_msg,
								GET_WEARSLOT(i, bodytype).partname);
			}

			sprintf(buf, "&cL<&cc%-26.26s&cL> &c0", buf2);
			ch->Send(buf);
			if ((GET_EQ(target, i))->CanBeSeen(ch)) {
				if (ch == target)
					show_obj_to_char(GET_EQ(target, i), ch, ShowObj::Name | ShowObj::Flags | ShowObj::Weight); // Show weights
				else
					show_obj_to_char(GET_EQ(target, i), ch, ShowObj::Name | ShowObj::Flags);
			} else {
				ch->Send("Something.\r\n");
			}
				found = TRUE;
			}
		}

	return (found);
}


ACMD(do_equipment) {
	ch->Send("You are using:\r\n");

	if (!look_at_equipped(ch, ch)) {
		ch->Send(" Nothing.\r\n");
	}
}


ACMD(do_time) {
  char *suf;
  int weekday, day;

  sprintf(buf, "It is %d o'clock %s, on ",
	  ((time_info.hours % 12 == 0) ? 12 : ((time_info.hours) % 12)),
	  ((time_info.hours >= 12) ? "pm" : "am"));

  /* 35 days in a month */
  weekday = ((35 * time_info.month) + time_info.day + 1) % 7;

  strcat(buf, weekdays[weekday]);
  strcat(buf, "\r\n");
  ch->Send(buf);

  day = time_info.day + 1;	/* day in [1..35] */

  if (day == 1)
    suf = "st";
  else if (day == 2)
    suf = "nd";
  else if (day == 3)
    suf = "rd";
  else if (day < 20)
    suf = "th";
  else if ((day % 10) == 1)
    suf = "st";
  else if ((day % 10) == 2)
    suf = "nd";
  else if ((day % 10) == 3)
    suf = "rd";
  else
    suf = "th";

  sprintf(buf, "The %d%s Day of the %s, Year %d.\r\n",
	  day, suf, month_name[(int) time_info.month], time_info.year);

  ch->Send(buf);
}


ACMD(do_weather)
{
	Weather::Weather *conditions = &zone_table[world[ch->AbsoluteRoom()].zone].conditions;

#if 0
	ch->Sendf("Windspeed: %d  Temperature: %d  Precipitation: %d\r\n",
			conditions->windspeed, conditions->temperature, conditions->precipRate);
#endif

	if (conditions->windspeed > 60) {
		if (conditions->temperature > 50)
			ch->Send("A violent scorching wind blows hard in your face.\r\n");
		else if (conditions->temperature > 21)
			ch->Send("A hot wind gusts wildly through the area.\r\n");
		else if (conditions->temperature > 0)
			ch->Send("A fierce wind cuts the air like a razor-sharp knife.\r\n");
		else if (conditions->temperature > -10)
			ch->Send("A freezing gale blasts through the area.\r\n");
		else
			ch->Send("An icy wind drains the warmth from all in sight.\r\n");
	} else if (conditions->windspeed > 25) {
		if (conditions->temperature > 50)
			ch->Send("A hot, dry breeze blows languidly around.\r\n");
		else if (conditions->temperature > 22)
			ch->Send("A warm pocket of air is rolling through here.\r\n");
		else if (conditions->temperature > 2)
			ch->Send("A cool breeze wafts by.\r\n");
		else if (conditions->temperature > -5)
			ch->Send("A freezing wind blows gently but firmly against you.\r\n");
		else
			ch->Send("The wind isn't very strong here, but the cold makes it quite noticeable.\r\n");
	} else if (conditions->temperature > 52)
		ch->Send("It's hotter than anyone could imagine.\r\n");
	else if (conditions->temperature > 37)
		ch->Send("It's really, really hot here.  A slight breeze would really improve things.\r\n");
	else if (conditions->temperature > 25)
		ch->Send("It's hot out here.\r\n");
	else if (conditions->temperature > 19)
		ch->Send("It's nice and warm out.\r\n");
	else if (conditions->temperature > 9)
		ch->Send("It's quite mild out right now.\r\n");
	else if (conditions->temperature > 1)
		ch->Send("It's cool out here.\r\n");
	else if (conditions->temperature > -5)
		ch->Send("It's a bit nippy here.\r\n");
	else if (conditions->temperature > -20)
		ch->Send("It's cold!\r\n");
	else if (conditions->temperature > -25)
		ch->Send("It's really cold!\r\n");
	else
		ch->Send("Better get inside - this is too cold for you!\r\n");

	if (conditions->temperature > 0) {
		if (conditions->precipRate > 80) {
			if (conditions->windspeed > 80)
				ch->Send("There's a hurricane out here!\r\n");
			else if (conditions->windspeed > 40)
				ch->Send("The wind and rain are nearly too much to handle.\r\n");
			else
				ch->Send("It's raining really hard right now.\r\n");
		} else if (conditions->precipRate > 50) {
			if (conditions->windspeed > 60)
				ch->Send("What a rainstorm!\r\n");
			else if (conditions->windspeed > 30)
				ch->Send("The wind is lashing this wild rain straight into your face.\r\n");
			else
				ch->Send("It's raining pretty hard.\r\n");
		} else if (conditions->precipRate > 30) {
			if (conditions->windspeed > 50)
				ch->Send("A respectable rain is being thrashed about by a vicious wind.\r\n");
			else if (conditions->windspeed > 25)
				ch->Send("It's rainy and windy but, altogether not too uncomfortable.\r\n");
			else
				ch->Send("It's raining.\r\n");
		} else if (conditions->precipRate > 10) {
			if (conditions->windspeed > 50)
				ch->Send("The light rain here is nearly unnoticeable compared to the horrendous wind.\r\n");
			else if (conditions->windspeed > 24)
				ch->Send("A light rain is being driven fiercely by the wind.\r\n");
			else
				ch->Send("It's raining lightly.\r\n");
		} else if (conditions->precipRate > 0) {
			if (conditions->windspeed > 50)
				ch->Send("A few drops of rain are falling amidst a fierce windstorm.\r\n");
			else if (conditions->windspeed > 30)
				ch->Send("The wind and a bit of rain hint at the possibility of a storm.\r\n");
			else
				ch->Send("A light drizzle is falling.\r\n");
		}
	} else {
		if (conditions->precipRate > 40) {	//	snow
			if (conditions->windspeed > 60)
				ch->Send("The heavily falling snow is being whipped up to a frenzy by a ferocious wind.\r\n");
			else if (conditions->windspeed > 35)
				ch->Send("A heavy snow is being blown randomly about by a brisk wind.\r\n");
			else if (conditions->windspeed > 18)
				ch->Send("Drifts in the snow are being formed by the wind.\r\n");
			else
				ch->Send("The snow's coming down pretty fast now.\r\n");
		} else if (conditions->precipRate > 19) {
			if (conditions->windspeed > 70)
				ch->Send("The snow wouldn't be too bad, except for the awful wind blowing it everywhere.\r\n");
			else if (conditions->windspeed > 45)
				ch->Send("There's a minor blizzard, more wind than snow.\r\n");
			else if (conditions->windspeed > 12)
				ch->Send("Snow is being blown about by a stiff breeze.\r\n");
			else
				ch->Send("It is snowing.\r\n");
		} else if (conditions->precipRate > 0) {
			if (conditions->windspeed > 60)
				ch->Send("A light snow is being tossed about by a fierce wind.\r\n");
			else if (conditions->windspeed > 42)
				ch->Send("A lightly falling snow is being driven by a strong wind.\r\n");
			else if (conditions->windspeed > 12)
				ch->Send("A light snow is falling admist an unsettled wind.\r\n");
			else
				ch->Send("It is lightly snowing.\r\n");
		}
	}
}


ACMD(do_help) {
	Help *this_help = NULL;
	char *entry;

	if (!ch->desc)	return;

	skip_spaces(argument);

	if (subcmd == SCMD_WIZHELP) {
		if (!*argument)
			do_commands(ch, "", 0, "wizcomm", SCMD_WIZHELP);
		else if (!Help::WizTable)
			ch->Send("No wizhelp available.\r\n");
		else if (!(this_help = Help::Find(argument, TRUE))) {
			ch->Send("There is no wizhelp on that word.\r\n");
			log("WIZHELP: %s tried to get wizhelp on %s", ch->RealName(), argument);
			return;
		}
	} else {
		if (!*argument)
			page_string(ch->desc, help, false);
		else if (!Help::Table)
			ch->Send("No help available.\r\n");
		else if (!(this_help = Help::Find(argument))) {
			ch->Send("There is no help on that word.\r\n");
			log("HELP: %s tried to get help on %s", ch->RealName(), argument);
			return;
		}
	}

//	if (this_help->level > GET_LEVEL(ch))
//		ch->Send("There is no help on that word.\r\n");
//	else {
	if (this_help) {
		entry = get_buffer(MAX_STRING_LENGTH);
		sprintf(entry, "%s\r\n\r\n%s", this_help->keywords, this_help->entry);
		page_string(ch->desc, entry, true);
		release_buffer(entry);
	}
//	}

}


#define WHOFLAGS_NAME		(1 << 0)
#define WHOFLAGS_GOD		(1 << 1)
#define WHOFLAGS_MORTAL		(1 << 2)
#define WHOFLAGS_QUEST		(1 << 3)
#define WHOFLAGS_ZONE		(1 << 4)
#define WHOFLAGS_LEVEL		(1 << 5)
#define WHOFLAGS_CLASS		(1 << 6)
#define WHOFLAGS_RACE		(1 << 7)
#define WHOFLAGS_TRUST		(1 << 8)
#define WHOFLAGS_UNJUST		(1 << 9)
#define WHOFLAGS_FLAG		(1 << 10)
#define WHOFLAGS_CLAN		(1 << 11)
#define WHOFLAGS_SINFO		(1 << 12)
#define WHOFLAGS_HLP		(1 << 13)

ACMD(do_who)
{
	Descriptor 		*d;
	Character 		*tch;
	Flags 			showflags = (WHOFLAGS_NAME | WHOFLAGS_LEVEL | WHOFLAGS_FLAG | WHOFLAGS_SINFO | WHOFLAGS_RACE);
	SInt32 			len,
					clan_num,
					num_can_see,
					loop_control = 0,
					column;
	bool 			showname = false, spaceit = false;
	char 			*line, *ptr, prename[40], flags[80];

	static const char *who_args[] = {
		"&ccr&cb - &cgrace",
		"&ccg&cb - &cggod",
		"&ccl&cb - &cglevel",
		"&ccm&cb - &cgmortal",
		"&ccq&cb - &cgquest",
		"&ccu&cb - &cgunjust",
		"&ccz&cb - &cgzone",
		"&ccf&cb - &cgflags",
		"&ccc&cb - &cgclan",
		"&cci&cb - &cgno-titles",
		"\n"
	};

	static const char *mortality_colors[] = {
		"&cg",
		"&cG",
		"&cW",
		"&cy",
		"\n"
	};

	line = any_one_arg(argument, arg);
	*flags = '\0';
	*buf = '\0';
	*buf2 = '\0';
	*buf3 = '\0';

	if (*arg) {
		showflags = WHOFLAGS_SINFO;
		ptr = arg;
		while (*ptr && (!isspace(*ptr))) {
			switch (*ptr) {
			case 'g':
				SET_BIT(showflags, WHOFLAGS_GOD);
				break;
			case 'm':
				SET_BIT(showflags, WHOFLAGS_MORTAL);
				break;
			case 'q':
				SET_BIT(showflags, WHOFLAGS_QUEST);
				break;
			case 'r':
				SET_BIT(showflags, WHOFLAGS_RACE);
				break;
			case 'l':
				SET_BIT(showflags, WHOFLAGS_LEVEL);
				break;
			case 'u':
				SET_BIT(showflags, WHOFLAGS_UNJUST);
				break;
			case 'z':
				SET_BIT(showflags, WHOFLAGS_ZONE);
				break;
			case 'f':
				SET_BIT(showflags, WHOFLAGS_FLAG);
				break;
			case 'c':
				SET_BIT(showflags, WHOFLAGS_CLAN);
				break;
			case 'i':
				REMOVE_BIT(showflags, WHOFLAGS_SINFO);
				break;
			case '?':
			case 'h':
				SET_BIT(showflags, WHOFLAGS_HLP);
				break;
			case 't':
				if (GET_TRUST(ch) >= TRUST_SUBIMPL)	SET_BIT(showflags, WHOFLAGS_TRUST);
				break;
			default:
				break;
			}
			++ptr;
		}

		if (IS_SET(showflags, WHOFLAGS_HLP)) {

			strcpy(buf, "&cMwho options:\r\n");
			for (column = 0; *who_args[column] != '\n'; ++column) {
				sprintf(buf + strlen(buf), "%s%-22.22s",
							 ((column % 4) ? "	" : "\r\n	"), who_args[column]);
			}

			strcat(buf, "\r\n\r\n"
									"	&cR? &cB- &cGthis help (cancels all other options)&c0\r\n"
									"	&cce.g. '&cGwho -lq&cc' lists the levels of everyone questing.&c0\r\n");

			ch->Send(buf);

			return;
		}
		line = any_one_arg(line, arg);
	}

	if (IS_SET(showflags, WHOFLAGS_ZONE) ||
			IS_SET(showflags, WHOFLAGS_GOD) ||
			IS_SET(showflags, WHOFLAGS_QUEST))
		line = any_one_arg(line, arg);

	if (IS_SET(showflags, WHOFLAGS_LEVEL) ||
			IS_SET(showflags, WHOFLAGS_RACE) ||
			IS_SET(showflags, WHOFLAGS_CLASS) ||
			IS_SET(showflags, WHOFLAGS_TRUST))
		showname = TRUE;

	if (GET_TRUST(ch) < 1) {
		REMOVE_BIT(showflags, WHOFLAGS_LEVEL);
	}

	sprintf(buf, "&cGPlayers in %s&c0\r\n&c0\r\n",
		(IS_SET(showflags, WHOFLAGS_ZONE) ? zone_table[world[IN_ROOM(ch)].zone].name :
		 "the Struggle for Nyrilis"));

	sprintf(buf + strlen(buf), "%s%s%s%s%s%s&cgName&c0\r\n"
		"&cL----------------------------------------\r\n",
	/* [	Lvl Rce Cla ] [Trust] Name								*/
		(showname ? "&cB[ " : ""),
		(IS_SET(showflags, WHOFLAGS_LEVEL) ? "&ccLevel " : ""),
		(IS_SET(showflags, WHOFLAGS_CLASS) ? "&ccClan " : ""),
		(IS_SET(showflags, WHOFLAGS_RACE) ? "&ccRace " : ""),
		(IS_SET(showflags, WHOFLAGS_TRUST) ? "&ccTrust " : ""),
		(showname ? "&cB] " : "")
					);

	len = strlen(arg);
	num_can_see = 0;

	START_ITER(iter, d, Descriptor *, descriptor_list) {
		*buf2 = '\0';
		*prename = '\0';
		*flags = '\0';

		if (d->connected)
			continue;

		if (d->original)
			tch = d->original;
		else if (!(tch = d->character))
			continue;

		if (!tch->CanBeSeen(ch))
			continue;

		if (IS_SET(showflags, WHOFLAGS_NAME) && strn_cmp(tch->Name(), arg, len))
			continue;
		if (IS_SET(showflags, WHOFLAGS_GOD) && (!IS_STAFF(tch) || PLR_FLAGGED(tch, PLR_NOWIZLIST)))
			continue;
		if (IS_SET(showflags, WHOFLAGS_ZONE) && world[IN_ROOM(ch)].zone != world[IN_ROOM(tch)].zone)
			continue;
		if (IS_SET(showflags, WHOFLAGS_MORTAL) && IS_STAFF(tch) && !PLR_FLAGGED(tch, PLR_NOWIZLIST))
			continue;
		if (IS_SET(showflags, WHOFLAGS_QUEST) && !CHN_FLAGGED(tch, Channel::Quest))
			continue;
		if (IS_SET(showflags, WHOFLAGS_UNJUST) && !PLR_FLAGGED(tch, PLR_THIEF | PLR_KILLER | PLR_UNJUST))
			continue;

		loop_control = 0;

		while (loop_control < 32) {
			if (IS_SET(showflags, (1 << loop_control))) {
				switch (1 << loop_control) {

				case WHOFLAGS_LEVEL:
					sprintf(prename + strlen(prename), "%s%s%5d&cc", *prename ? " " : "",
					((GET_LEVEL(tch) > 80) ? "&cg" :
								((GET_LEVEL(tch) > 30) ? "&cy" : "&cr")), GET_LEVEL(tch));
					break;

				case WHOFLAGS_CLASS:
					sprintf(prename + strlen(prename), "%s%5s", *prename ? " " : "", CLASS_ABBR(tch));
					break;

				case WHOFLAGS_RACE:
					sprintf(prename + strlen(prename), "%s%4s", *prename ? " " : "", RACE_ABBR(tch));
					break;

				case WHOFLAGS_TRUST:
					sprintf(prename + strlen(prename), "%s%5d", *prename ? " " : "", GET_TRUST(tch));
					break;

				case WHOFLAGS_FLAG:
					if (GET_STAFF_INVIS(tch) > 0)
						sprintf(flags + strlen(flags), "%s&cyI%d&cc", *flags ? " " : "", GET_STAFF_INVIS(tch));
					else if (GET_INVIS_LEV(tch) > 0)
						sprintf(flags + strlen(flags), "%s&cyi%d&cc", *flags ? " " : "", GET_INVIS_LEV(tch));

					if (PLR_FLAGGED(tch, PLR_MAILING))
						sprintf(flags + strlen(flags), "%sM", *flags ? " " : "");

					else if (PLR_FLAGGED(tch, PLR_WRITING))
						sprintf(flags + strlen(flags), "%sW", *flags ? " " : "");

					if (CHN_FLAGGED(tch, Channel::NoShout))
						sprintf(flags + strlen(flags), "%s&cmD&cc", *flags ? " " : "");

					if (CHN_FLAGGED(tch, Channel::NoTell))
						sprintf(flags + strlen(flags), "%s&cmN&cc", *flags ? " " : "");

					if (CHN_FLAGGED(tch, Channel::Quest))
						sprintf(flags + strlen(flags), "%s&cmQ&cc", *flags ? " " : "");

					if (PLR_FLAGGED(tch, PLR_THIEF))
						sprintf(flags + strlen(flags), "%s&crT&cc", *flags ? " " : "");

					if (PLR_FLAGGED(tch, PLR_KILLER))
						sprintf(flags + strlen(flags), "%s&crK&cc", *flags ? " " : "");

					if (PLR_FLAGGED(tch, PLR_UNJUST))
						sprintf(flags + strlen(flags), "%s&crU&cc", *flags ? " " : "");
				}
			}

			++loop_control;
		}

		if (*prename)
			sprintf(buf2, "&cB[&cc %s &cB] ", prename);

		sprintf(buf2 + strlen(buf2), "%s%s%s %s",
			(PLR_FLAGGED(tch, PLR_AFK)) ? "&cB-&crAFK&cB- " : "",
			mortality_colors[GET_MORTALITY(tch)],
			tch->Name(),
			(IS_SET(showflags, WHOFLAGS_SINFO) ? GET_TITLE(tch) :
			mortality_colors[GET_MORTALITY(tch)]));

		if (*flags) {
			sprintf(buf2 + strlen(buf2), " &cB( %s &cB)", flags);
		}

		strcat(buf, buf2);
		strcat(buf, "\r\n");
		++num_can_see;
	}

	if (num_can_see == 0)
		sprintf(buf + strlen(buf), "\r\n&cmNo-one at all!");
	else if (num_can_see == 1)
		sprintf(buf + strlen(buf), "\r\n&cmOne lonely character displayed.");
	else
		sprintf(buf + strlen(buf), "\r\n&cm%d characters displayed.", num_can_see);

	strcat(buf, "&c0\r\n");

	page_string(ch->desc, buf, true);
}


#ifdef GOT_RID_OF_IT
ACMD(do_who) {
	Descriptor *d;
	Character *		wch;
	VNum			clan;
	bool			isClan = false;
	SInt32			low = 0,
					high = MAX_LEVEL;
    UInt32 			i;
	bool			who_room = false,
					who_zone = false,
					who_mission = false,
					traitors = false,
					no_staff = false,
					no_players = false;
	UInt32			staff = 0,
					players = 0;
	Flags			showrace = 0;

	int  players_clan = 0, players_clannum = 0, ccmd = 0;
	extern char *level_string[];

	char	*buf = get_buffer(MAX_STRING_LENGTH),
			*arg = get_buffer(MAX_STRING_LENGTH),
			*name_search = get_buffer(MAX_INPUT_LENGTH),
			*staff_buf, *player_buf,
			mode;

	skip_spaces(argument);
	strcpy(buf, argument);

	while (*buf) {
		half_chop(buf, arg, buf);
		if (isdigit(*arg)) {
			sscanf(arg, "%d-%d", &low, &high);
		} else if (*arg == '-') {
			mode = *(arg + 1);	/* just in case; we destroy arg in the switch */
			switch (mode) {
				case 't':				// Only traitors
					traitors = true;
					break;
				case 'z':				// In Zone
					who_zone = true;
					break;
				case 'm':				// Only Missoin
					who_mission = true;
					break;
				case 'l':				// Range
					half_chop(buf, arg, buf);
					sscanf(arg, "%d-%d", &low, &high);
					break;
				case 'n':				// Name
					half_chop(buf, name_search, buf);
					break;
				case 'r':				// In room
					who_room = true;
					break;
				case 's':				// Only staff
					no_players = true;
					break;
				case 'p':				// Only Players
					no_staff = true;
					break;
				case 'c':				//	Race specific...
					half_chop(buf, arg, buf);
					for (i = 0; i < strlen(arg); i++)
						showrace |= find_race_bitvector(arg[i]);
					break;
				default:
					ch->Send(WHO_USAGE);
					release_buffer(buf);
					release_buffer(arg);
					release_buffer(name_search);
					return;
			}				/* end of switch */
		} else {			/* endif */
			ch->Send(WHO_USAGE);
			release_buffer(buf);
			release_buffer(arg);
			release_buffer(name_search);
			return;
		}
	}

	staff_buf = get_buffer(MAX_STRING_LENGTH);
	player_buf = get_buffer(MAX_STRING_LENGTH);

	strcpy(staff_buf,	"Staff currently online\r\n"
						"----------------------\r\n");
	strcpy(player_buf,	"Players currently online\r\n"
						"------------------------\r\n");

	START_ITER(iter, d, Descriptor *, descriptor_list) {
		if (STATE(d) != CON_PLAYING)				continue;
		if (!(wch = d->Original()))					continue;
		if (!ch->CanSeeStaff(wch))					continue;
		if ((GET_LEVEL(wch) < low) || (GET_LEVEL(wch) > high))
			continue;
		if ((no_staff && IS_STAFF(wch)) || (no_players && !IS_STAFF(wch)))
			continue;
		if (*name_search && str_cmp(wch->Name(), name_search) && !strstr(GET_TITLE(wch), name_search))
			continue;
#ifdef GOT_RID_OF_IT
// CHANGEPOINT:  This who will need to be redesigned anyway.
		if (traitors && !PLR_FLAGGED(wch, PLR_TRAITOR))
			continue;
		if (who_mission && !CHN_FLAGGED(wch, Channel::Mission))
			continue;
#endif
		if (who_zone && world[IN_ROOM(ch)].zone != world[IN_ROOM(wch)].zone)
			continue;
		if (who_room && (IN_ROOM(wch) != IN_ROOM(ch)))
			continue;
		if (showrace && !(showrace & (1 << (GET_RACE(wch)))))
			continue;

		*buf = '\0';

		clan = GET_CLAN(wch);
		isClan = (Clan::Find(clan) && GET_CLAN_LEVEL(wch) >= Clan::Rank::Member);

		if (IS_STAFF(wch)) {
			sprintf(buf, "&cy %s %s %s&c0", level_string[0],
					wch->Name(), GET_TITLE(wch));
			staff++;
		} else {
			sprintf(buf, "&c0[%3d %s] &c%c%s %s&c0", GET_LEVEL(wch), RACE_ABBR(wch),
					relation_colors[ch->GetRelation(wch)],
					isClan ? Clan::Clans[clan].ranks[GET_CLAN_LEVEL(wch) - 1] : GET_TITLE(wch),
					wch->Name());
			players++;
		}

//		if (GET_INVIS_LEV(wch))						sprintf(buf + strlen(buf), " (&cwi%d&c0)", GET_INVIS_LEV(wch));
//		else
		if (AFF_FLAGGED(wch, AffectBit::Invisible))	strcat(buf, " (&cwinvis&c0)");

		if (PLR_FLAGGED(wch, PLR_MAILING))			strcat(buf, " (&cbmailing&c0)");
		else if (PLR_FLAGGED(wch, PLR_WRITING))		strcat(buf, " (&crwriting&c0)");

		if (CHN_FLAGGED(wch, Channel::NoShout))		strcat(buf, " (&cgdeaf&c0)");
		if (CHN_FLAGGED(wch, Channel::NoTell))		strcat(buf, " (&cgnotell&c0)");
#ifdef GOT_RID_OF_IT
		if (CHN_FLAGGED(wch, Channel::Mission))		strcat(buf, " (&cmmission&c0)");
		if (PLR_FLAGGED(wch, PLR_TRAITOR))			strcat(buf, " (&cRTRAITOR&c0)");
#endif
		if (isClan)									sprintf(buf + strlen(buf), " <&cC%s&c0>", Clan::Clans[clan].name);
		if (GET_AFK(wch))							strcat(buf, " &cc[AFK]");
		strcat(buf, "&c0\r\n");

		if (IS_STAFF(wch))		strcat(staff_buf, buf);
		else					strcat(player_buf, buf);
	}

	*buf = '\0';

	if (staff) {
		strcat(buf, staff_buf);
		strcat(buf, "\r\n");
	}
	if (players) {
		strcat(buf, player_buf);
		strcat(buf, "\r\n");
	}

	if ((staff + players) == 0) {
		strcat(buf, "No staff or players are currently visible to you.\r\n");
	}
	if (staff)
		sprintf(buf + strlen(buf), "There %s %d visible staff%s", (staff == 1 ? "is" : "are"), staff, players ? " and there" : ".");
	if (players)
		sprintf(buf + strlen(buf), "%s %s %d visible player%s.", staff ? "" : "There", (players == 1 ? "is" : "are"), players, (players == 1 ? "" : "s"));
	strcat(buf, "\r\n");

	if ((staff + players) > boot_high)
		boot_high = staff+players;

	sprintf(buf + strlen(buf), "There is a boot time high of %d player%s.\r\n", boot_high, (boot_high == 1 ? "" : "s"));

	page_string(ch->desc, buf, true);


	release_buffer(buf);
	release_buffer(arg);
	release_buffer(name_search);
	release_buffer(staff_buf);
	release_buffer(player_buf);
}
#endif


const char *USERS_FORMAT = "format: users [-l minlevel[-maxlevel]] [-n name] [-h host] [-o] [-p] [-d] [-c] [-m]\r\n";

ACMD(do_users) {
	Character *	tch;
	Descriptor *d, *match;
	SInt32		low = 0,
				high = MAX_LEVEL,
				num_can_see = 0;
	bool 		traitors = false,
				playing = false,
				deadweight = false,
				complete = false,
				matching = false;
	/*
	 * I think this function needs more (char *)'s, it doesn't have enough.
	 * XXX: Help! I'm drowning in (char *)'s!
	 */
	char		*buf = get_buffer(MAX_STRING_LENGTH),
				*arg = get_buffer(MAX_INPUT_LENGTH),
				*name_search = get_buffer(MAX_INPUT_LENGTH),
				*host_search = get_buffer(MAX_INPUT_LENGTH),
				*idletime,
				*timeptr,
				mode;
	const char *state;

	strcpy(buf, argument);
	while (*buf) {
		half_chop(buf, arg, buf);
		if (*arg == '-') {
			mode = *(arg + 1);  /* just in case; we destroy arg in the switch */
			switch (mode) {
				case 'm':	matching = true;							break;
				case 'c':	complete = true;							break;
				case 'o':	traitors = true;	playing = true;			break;
				case 'p':	playing = true;								break;
				case 'd':	deadweight = true;							break;
				case 'l':
					playing = true;
					half_chop(buf, arg, buf);
					sscanf(arg, "%d-%d", &low, &high);
					break;
				case 'n':
					playing = true;
					half_chop(buf, name_search, buf);
					break;
				case 'h':
					playing = true;
					half_chop(buf, host_search, buf);
					break;
				default:
					ch->Send(USERS_FORMAT);
					release_buffer(buf);
					release_buffer(arg);
					release_buffer(name_search);
					release_buffer(host_search);
					return;
			}
		} else {
			ch->Send(USERS_FORMAT);
			release_buffer(buf);
			release_buffer(arg);
			release_buffer(name_search);
			release_buffer(host_search);
			return;
		}
	}

	idletime = get_buffer(10);

	strcpy(buf, "Num Race      Name         State          Idl Login@   Site\r\n"
				"--- --------- ------------ -------------- --- -------- ------------------------\r\n");

	one_argument(argument, arg);

	LListIterator<Descriptor *>	iter(descriptor_list);
	LListIterator<Descriptor *>	match_iter(descriptor_list);

	while ((d = iter.Next())) {
		tch = d->Original();
		if ((STATE(d) == CON_PLAYING)) {
			if (deadweight)												continue;
			if (!tch)													continue;
			if (*host_search && !strstr(d->host, host_search))			continue;
			if (*name_search && str_cmp(tch->Name(), name_search))	continue;
			if (!tch->CanBeSeen(ch))										continue;
			if ((GET_LEVEL(tch) < low) || (GET_LEVEL(tch) > high))		continue;
#ifdef GOT_RID_OF_IT
			if (traitors && !PLR_FLAGGED(tch, PLR_TRAITOR))				continue;
#endif
		} else {
			if (playing)												continue;
			if (tch && !tch->CanBeSeen(ch))								continue;
		}
		if (matching) {
			if (!d->host)												continue;
			match_iter.Reset();
			while ((match = match_iter.Next())) {
				if (!match->host || (match == d))						continue;
				if (!str_cmp(match->host, d->host))
					break;
			}
			if (!match)													continue;
		}


		timeptr = asctime(localtime(&d->login_time));
		timeptr += 11;
		*(timeptr + 8) = '\0';

		state = ((STATE(d) == CON_PLAYING) && d->original) ? "Switched" : connected_types[STATE(d)];

		if ((STATE(d) == CON_PLAYING) && d->character && !IS_STAFF(d->character))
			sprintf(idletime, "%3d", d->character->player->timer * SECS_PER_MUD_HOUR / SECS_PER_REAL_MIN);
		else
			strcpy(idletime, "");

		strcat(buf, STATE(d) == CON_PLAYING ? "&c0" : "&cg");

		if (tch && tch->Name())
			sprintf(buf + strlen(buf), "%3d [%3d %3s] %-12s %-14.14s %-3s %-8s ", d->desc_num,
					GET_LEVEL(tch), GET_RACE(tch) != RACE_UNDEFINED ? RACE_ABBR(tch) : "???",
					tch->Name(), state, idletime, timeptr);
		else
			sprintf(buf + strlen(buf), "%3d     -     UNDEFINED    %-14.14s %-3s %-8s ", d->desc_num,
					state, idletime, timeptr);

		sprintf(buf + strlen(buf), complete ? "[%-22s]\r\n" : "[%-22.22s]\r\n",
				(d->host && *d->host) ? d->host : "Hostname unknown");

		num_can_see++;
	}

	sprintf(buf + strlen(buf), "\r\n&c0%d visible sockets connected.\r\n", num_can_see);

	page_string(ch->desc, buf, true);

	release_buffer(buf);
	release_buffer(arg);
	release_buffer(name_search);
	release_buffer(host_search);
	release_buffer(idletime);
}


/* Generic page_string function for displaying text */
ACMD(do_gen_ps) {
	switch (subcmd) {
		case SCMD_CREDITS:	page_string(ch->desc, credits, false);		break;
		case SCMD_NEWS:		page_string(ch->desc, news, false);			break;
		case SCMD_INFO:		page_string(ch->desc, info, false);			break;
		case SCMD_WIZLIST:	page_string(ch->desc, wizlist, false);		break;
		case SCMD_IMMLIST:	page_string(ch->desc, immlist, false);		break;
		case SCMD_HANDBOOK:	page_string(ch->desc, handbook, false);		break;
		case SCMD_POLICIES:	page_string(ch->desc, policies, false);		break;
		case SCMD_MOTD:		page_string(ch->desc, motd, false);			break;
		case SCMD_IMOTD:	page_string(ch->desc, imotd, false);		break;
		case SCMD_CLEAR:	ch->Send("\033[H\033[J");					break;
		case SCMD_VERSION:	ch->Sendf("%s\r\n", circlemud_version);		break;
		case SCMD_WHOAMI:	ch->Sendf("%s\r\n", ch->Name());				break;
	}
}


void perform_mortal_where(Character * ch, char *arg) {
	Descriptor *d;
	Character *	i;
	char *buf = get_buffer(MAX_STRING_LENGTH);

	if (!*arg) {
		strcpy(buf, "Players in your Zone\r\n--------------------\r\n");

		LListIterator<Descriptor *>		iter(descriptor_list);
		while ((d = iter.Next())) {
			if (strlen(buf) > (MAX_STRING_LENGTH - MAX_SOCK_BUF)) {
				strcat(buf, "*** OVERFLOW ***");
				break;
			}

			if ((STATE(d) != CON_PLAYING) || (d->character == ch))	continue;
			if (!(i = d->Original()))								continue;
			if ((IN_ROOM(i) == NOWHERE) || !i->CanBeSeen(ch))			continue;
			if (world[IN_ROOM(ch)].zone != world[IN_ROOM(i)].zone)	continue;
			sprintf(buf + strlen(buf), "%-20s - %s\r\n", i->Name(), world[IN_ROOM(i)].Name("<ERROR>"));
		}
	} else {			/* print only FIRST char, not all. */
		LListIterator<Character *>			iter(Characters);
		while ((i = iter.Next())) {
			if ((IN_ROOM(i) == NOWHERE) || (i == ch))				continue;
			if (!i->CanBeSeen(ch))										continue;
			if (world[IN_ROOM(ch)].zone != world[IN_ROOM(i)].zone)	continue;
			if (!isname(arg, i->Name()))							continue;
			sprintf(buf + strlen(buf), "%-25s - %s\r\n", i->Name(), world[IN_ROOM(i)].Name("<ERROR>"));
			break;
		}
		if (!*buf)
			strcpy(buf, "No-one around by that name.\r\n");
	}
	page_string(ch->desc, buf, true);
	release_buffer(buf);
}


void print_object_location(char *buf, int num, Object * obj, Character * ch, int recur) {

	if (strlen(buf) > (MAX_STRING_LENGTH - MAX_SOCK_BUF)) {
		return;
	}

	if (num > 0)	sprintf(buf + strlen(buf), "O%3d. %-25s - ", num, obj->Name());
	else			sprintf(buf + strlen(buf), "%33s", " - ");

	if (IN_ROOM(obj) > NOWHERE) {
		sprintf(buf + strlen(buf), "[%5d] %s\r\n", world[IN_ROOM(obj)].Virtual(), world[IN_ROOM(obj)].Name("<ERROR>"));
	} else if (obj->CarriedBy()) {
		sprintf(buf + strlen(buf), "carried by %s\r\n", obj->Inside()->Name());
	} else if (obj->WornBy()) {
		sprintf(buf + strlen(buf), "worn by %s\r\n", obj->Inside()->Name());
	} else if (obj->InObj()) {
		sprintf(buf + strlen(buf), "inside %s%s\r\n", obj->Inside()->Name(), (recur ? ", which is" : " "));
		if (recur)
			print_object_location(buf, 0, obj->InObj(), ch, recur);
	} else
		strcat(buf, "in an unknown location\r\n");
}


void perform_immort_where(Character * ch, char *arg) {
	char *		buf = get_buffer(MAX_STRING_LENGTH);

	if (!*arg) {
		Descriptor *d;
		strcpy(buf, "Players\r\n-------\r\n");

		LListIterator<Descriptor *>		descriptors(descriptor_list);
		while ((d = descriptors.Next())) {
			if (strlen(buf) > (MAX_STRING_LENGTH - MAX_SOCK_BUF)) {
				strcat(buf, "*** OVERFLOW ***");
				break;
			}
			if (STATE(d) == CON_PLAYING) {
				Character *i = d->Original();
				if (i && i->CanBeSeen(ch) && (IN_ROOM(i) != NOWHERE)) {
					sprintf(buf + strlen(buf), "%-20s - [%5d] %s",
							i->Name(), IN_ROOM(d->character),
							world[IN_ROOM(d->character)].Name("<ERROR>"));
					if (d->original)
						sprintf(buf + strlen(buf), " (in %s)", d->character->Name());
					strcat(buf, "\r\n");
				}
			}
		}
	} else {
		SInt32		num;
		Character *	i;
		Object *	k;

		num = 0;
		LListIterator<Character *>	characters(Characters);
		while ((i = characters.Next())) {
			if (strlen(buf) > (MAX_STRING_LENGTH - MAX_SOCK_BUF)) {
				strcat(buf, "*** OVERFLOW ***");
				break;
			}
			if (i->CanBeSeen(ch) && (IN_ROOM(i) != NOWHERE) && isname(arg, i->Keywords())) {
				sprintf(buf + strlen(buf), "M%3d. %-25s - [%5d] %s\r\n", ++num, i->Name(),
						world[IN_ROOM(i)].Virtual(), world[IN_ROOM(i)].Name("<ERROR>"));
			}
		}

		num = 0;
		LListIterator<Object *>		objects(Objects);
		while ((k = objects.Next())) {
			if (strlen(buf) > (MAX_STRING_LENGTH - MAX_SOCK_BUF)) {
				strcat(buf, "*** OVERFLOW ***");
				break;
			}
			if (k->CanBeSeen(ch) && isname(arg, k->Keywords()) &&
					(!k->CarriedBy() || k->CarriedBy()->CanBeSeen(ch))) {
				print_object_location(buf, ++num, k, ch, TRUE);
			}
		}
		if (!*buf)
			strcpy(buf, "Couldn't find any such thing.\r\n");
	}
	page_string(ch->desc, buf, true);
	release_buffer(buf);
}



ACMD(do_where) {
	char * arg = get_buffer(MAX_STRING_LENGTH);
	one_argument(argument, arg);

	if (IS_STAFF(ch))			perform_immort_where(ch, arg);
	else						perform_mortal_where(ch, arg);
	release_buffer(arg);
}


#ifdef GOT_RID_OF_IT
ACMD(do_levels) {
	char * buf;
	int	i;
	int final_level = MAX_LEVEL;

	if (IS_NPC(ch) || !ch->desc) {
		ch->Send("You ain't nothin' but a hound-dog.\r\n");
		return;
	}

	buf = get_buffer(MAX_STRING_LENGTH);

	for (i = 1 ; i <= final_level; i++) {
		sprintf(buf + strlen(buf), "[%3d] : ", ((i - 1) * 10) + 1);
		switch (GET_SEX(ch)) {
			case Male:
			case Neutral:
				strcat(buf, titles[(int) GET_RACE(ch)][i].title_m);
				break;
			case Female:
				strcat(buf, titles[(int) GET_RACE(ch)][i].title_f);
				break;
			default:
				ch->Send("Oh dear.  You seem to be sexless.\r\n");
				release_buffer(buf);
				return;
		}
		strcat(buf, "\r\n");
	}
	page_string(ch->desc, buf, true);
	release_buffer(buf);
}
#endif


ACMD(do_consider) {
	Character *victim;
	int diff;
	char *buf = get_buffer(MAX_STRING_LENGTH);
	const char *	result;

	one_argument(argument, buf);

	if (!(victim = get_char_vis(ch, buf, FIND_CHAR_ROOM)))
		result = "Consider killing who?\r\n";
	else if (victim == ch)
		result = "Easy!  Very easy indeed!\r\n";
	else if (!IS_NPC(victim))
		result = "Would you like to borrow a cross and a shovel?\r\n";
	else {
		diff = exp_from_level(GET_TOTALPTS(ch), GET_TOTALPTS(victim), GET_EXP_MOD(victim));

		if (diff <= 5)		result = "Now where did that chicken go?\r\n";
		else if (diff <= 20)	result = "You could do it with a needle!\r\n";
		else if (diff <= 50)	result = "Easy.\r\n";
		else if (diff <= 90)	result = "Fairly easy.\r\n";
		else if (diff <= 110)	result = "The perfect match!\r\n";
		else if (diff <= 140)	result = "You would need some luck!\r\n";
		else if (diff <= 180)	result = "You would need a lot of luck!\r\n";
		else if (diff <= 210)	result = "You would need a lot of luck and great equipment!\r\n";
		else if (diff <= 250)	result = "Do you feel lucky, punk?\r\n";
		else if (diff <= 300)	result = "Are you mad!?\r\n";
		else 					result = "You ARE mad!\r\n";
	}
	release_buffer(buf);
	ch->Send(result);
}



ACMD(do_diagnose) {
	Character *vict;
	char *buf = get_buffer(MAX_STRING_LENGTH);

	one_argument(argument, buf);

	if (*buf) {
		if (!(vict = get_char_vis(ch, buf, FIND_CHAR_ROOM))) {
			ch->Send(NOPERSON);
			release_buffer(buf);
			return;
		} else
			diag_char_to_char(vict, ch);
	} else {
		if (FIGHTING(ch))
			diag_char_to_char(FIGHTING(ch), ch);
		else
			ch->Send("Diagnose who?\r\n");
	}
	release_buffer(buf);
}


#ifdef GOT_RID_OF_IT
ACMD(do_toggle) {
	char *buf;

	if (IS_NPC(ch))	return;

	buf = get_buffer(4);

	if (!GET_WIMP_LEV(ch))		strcpy(buf, "OFF");
	else						sprintf(buf, "%-3d", GET_WIMP_LEV(ch));

	ch->Sendf(
			"&cy[&cGGeneral config&cy]&c0\r\n"
			"     Brief Mode: %-3s    "" Summon Protect: %-3s    ""   Compact Mode: %-3s\r\n"
			"          Color: %-3s    ""   Repeat Comm.: %-3s    ""     Show Exits: %-3s\r\n"
			"&cy[&cGChannels      &cy]&c0\r\n"
			"           Chat: %-3s    ""          Grats: %-3s    ""           Info: %-3s\r\n"
            "          Shout: %-3s    ""           Tell: %-3s    ""          Quest: %-3s\r\n"
			"&cy[&cGGame Specifics&cy]&c0\r\n"
			"     Wimp Level: %-3s\r\n",

			ONOFF(PRF_FLAGGED(ch, Preference::Brief)),	ONOFF(!PRF_FLAGGED(ch, Preference::Summonable)),
					ONOFF(PRF_FLAGGED(ch, Preference::Compact)),
			ONOFF(PRF_FLAGGED(ch, Preference::Color)),	YESNO(!PRF_FLAGGED(ch, Preference::NoRepeat)),
					ONOFF(PRF_FLAGGED(ch, Preference::AutoExit)),

			ONOFF(!CHN_FLAGGED(ch, Channel::NoGossip)),
#ifdef GOT_RID_OF_IT
            ONOFF(!CHN_FLAGGED(ch, Channel::NoMusic)),
#endif
            ONOFF(!CHN_FLAGGED(ch, Channel::NoGratz)),
			ONOFF(!CHN_FLAGGED(ch, Channel::NoInfo)),ONOFF(!CHN_FLAGGED(ch, Channel::NoShout)),	ONOFF(!CHN_FLAGGED(ch, Channel::NoTell)),
			YESNO(CHN_FLAGGED(ch, Channel::Quest)),

			buf
	);

	release_buffer(buf);
}
#endif


ACMD(do_toggle)
{
	char buf1[10];

	if (IS_NPC(ch))
		return;
	if (GET_PAGE_LENGTH(ch) == 0)
		strcpy(buf1, "Unlimited");
	else
		sprintf(buf1, "%-3d", GET_PAGE_LENGTH(ch));
//	if (!GET_PROMPT(ch))
//		strcpy(buf2, "n/a");
//	else
//		sprintf(buf2, "%-3d", GET_PROMPT(ch));
	if (GET_WIMP_LEV(ch) == 0)
		strcpy(buf3, "OFF");
	else
		sprintf(buf3, "%-3d", GET_WIMP_LEV(ch));

	sprintf(buf,
		"&cy[&cGGeneral&cy]&c0\r\n"
		"&cMAutoassist&cy: &cg%-3s     "
		"&cMAutoexit&cy: &cg%-3s       "
		"&cMBrief&cy: &cg%-3s          "
		"&cMColor&cy: &cg%-3s&c0\r\n"
		"&cMCompact&cy: &cg%-3s        "
		"&cMQuesting&cy: &cg%-3s       "
		"&cMSound&cy: &cg%-3s          "
		"&cMSummonable&cy: &cg%-3s&c0\r\n"
		"&cMPagelength&cy: &cg%s&c0\r\n"
		"&cMPrompt&cy: &cg%s&c0\r\n"
		"&cMWimpy&cy: &cg%-3s&c0       "
		"&cMMercy&cy: &cg%-3s&c0\r\n"

		"\r\n&cy[&cGCommunication&cy]&c0\r\n"
		"&cMChat&cy: &cg%-3s           "
		"&cMGrats&cy: &cg%-3s          "
		"&cMRepeat&cy: &cg%-3s         "
		"&cMShouts&cy: &cg%-3s&c0\r\n"
		"&cMTells&cy: &cg%-3s&c0\r\n",

		ONOFF(PRF_FLAGGED(ch, Preference::AutoAssist)),
		ONOFF(PRF_FLAGGED(ch, Preference::AutoExit)),
		ONOFF(PRF_FLAGGED(ch, Preference::Brief)),
		ONOFF(PRF_FLAGGED(ch, Preference::Color)),
		ONOFF(PRF_FLAGGED(ch, Preference::Compact)),
		YESNO(PRF_FLAGGED(ch, Preference::Quest)),
		ONOFF(PRF_FLAGGED(ch, Preference::Sound)),
		YESNO(PRF_FLAGGED(ch, Preference::Summonable)),
		buf1,
		(GET_PROMPT(ch) ? GET_PROMPT(ch) : "n/a"),
		buf3,
		ONOFF(PRF_FLAGGED(ch, Preference::Merciful)),

		ONOFF(!CHN_FLAGGED(ch, Channel::NoGossip)),
		ONOFF(!CHN_FLAGGED(ch, Channel::NoGratz)),
		YESNO(!PRF_FLAGGED(ch, Preference::NoRepeat)),
		ONOFF(!CHN_FLAGGED(ch, Channel::NoShout)),
		ONOFF(!CHN_FLAGGED(ch, Channel::NoTell)));

	if (IS_STAFF(ch))
	sprintf(buf + strlen(buf),
		"\r\n&cy[&cGStaff&cy]&c0\r\n"
		"&cMHolylight&cy: &cg%-3s      "
		"&cMInvuln&cy: &cg%-3s         "
		"&cMNohassle&cy: &cg%-3s       "
		"&cMRoomflags&cy: &cg%-3s&c0\r\n"
		"&cMThink Channel&cy: &cg%-3s&c0\r\n",

		ONOFF(PRF_FLAGGED(ch, Preference::HolyLight)),
		YESNO(PRF_FLAGGED(ch, Preference::Invuln)),
		ONOFF(PRF_FLAGGED(ch, Preference::NoHassle)),
		ONOFF(PRF_FLAGGED(ch, Preference::RoomFlags)),
		ONOFF(!CHN_FLAGGED(ch, Channel::NoWiz)));

	ch->Send(buf);
}


class SortStruct {
public:
			SortStruct(void) : sort_pos(0), type(0) { }

	SInt32	sort_pos;
	Flags	type;
} *cmd_sort_info = NULL;

int num_of_cmds;

enum {
	TYPE_CMD = (1 << 0),
	TYPE_SOCIAL = (1 << 1),
	TYPE_STAFFCMD = (1 << 2)
};


void sort_commands(void) {
	int a, b, tmp;
	Trigger *live_trig;

	num_of_cmds = 0;

	// first, count commands (num_of_commands is actually one greater than the
	//	number of commands; it inclues the '\n').
	while (*complete_cmd_info[num_of_cmds].command != '\n')
		num_of_cmds++;

	//	check if there was an old sort info.. then free it -- aedit -- M. Scott
	if (cmd_sort_info) delete [] cmd_sort_info;
	//	create data array
	cmd_sort_info = new SortStruct[num_of_cmds];

	//	initialize it
	for (a = 1; a < num_of_cmds; a++) {
		cmd_sort_info[a].sort_pos = a;
		if (complete_cmd_info[a].command_pointer == do_action)
			cmd_sort_info[a].type = TYPE_SOCIAL;
		// if (IS_STAFFCMD(a) || (complete_cmd_info[a].minimum_level == MAX_LEVEL))
		if (IS_STAFFCMD(a))
			cmd_sort_info[a].type |= TYPE_STAFFCMD;
		if (!cmd_sort_info[a].type)
			cmd_sort_info[a].type = TYPE_CMD;
	}

	/* the infernal special case */
	cmd_sort_info[find_command("insult")].type = TYPE_SOCIAL;

	/* Sort.  'a' starts at 1, not 0, to remove 'RESERVED' */
	for (a = 1; a < num_of_cmds - 1; a++) {
		for (b = a + 1; b < num_of_cmds; b++) {
			if (strcmp(complete_cmd_info[cmd_sort_info[a].sort_pos].command,
					complete_cmd_info[cmd_sort_info[b].sort_pos].command) > 0) {
				tmp = cmd_sort_info[a].sort_pos;
				cmd_sort_info[a].sort_pos = cmd_sort_info[b].sort_pos;
				cmd_sort_info[b].sort_pos = tmp;
			}
		}
	}

	reset_command_ops();
}


ACMD(do_commands) {
	int no, i, cmd_num;
	int wizhelp = 0, socials = 0;
	Character *vict;
	char *arg = get_buffer(MAX_STRING_LENGTH);
	char *buf;
	Flags	type;

	one_argument(argument, arg);

	if (*arg) {
		if (!(vict = get_char_vis(ch, arg, FIND_CHAR_WORLD)) || IS_NPC(vict)) {
			ch->Send("Who is that?\r\n");
			release_buffer(arg);
			return;
		}
		if (!IS_STAFF(ch) /* || (GET_LEVEL(ch) < GET_LEVEL(vict)) */) {
			ch->Send("You can't see the commands of people above your level.\r\n");
			release_buffer(arg);
			return;
		}
	} else
		vict = ch;

	release_buffer(arg);

	if (subcmd == SCMD_SOCIALS)			type = TYPE_SOCIAL;
	else if (subcmd == SCMD_WIZHELP)	type = TYPE_STAFFCMD;
	else								type = TYPE_CMD;

	buf = get_buffer(MAX_STRING_LENGTH);

	sprintf(buf, "The following %s%s are available to %s:\r\n",
			(subcmd == SCMD_WIZHELP) ? "privileged " : "",
			(subcmd == SCMD_SOCIALS) ? "socials" : "commands",
			vict == ch ? "you" : vict->RealName());

	/* cmd_num starts at 1, not 0, to remove 'RESERVED' */
	for (no = 1, cmd_num = 1; cmd_num < num_of_cmds; cmd_num++) {
		i = cmd_sort_info[cmd_num].sort_pos;

		if (!IS_SET(cmd_sort_info[i].type, type))
			continue;

		if ((GET_LEVEL(vict) < complete_cmd_info[i].minimum_level) /* && !IS_STAFF(vict) */)
			continue;

		if (IS_NPCCMD(i) && !IS_NPC(vict))
			continue;

		if ((subcmd == SCMD_WIZHELP) && IS_STAFFCMD(i) && !STF_FLAGGED(vict, complete_cmd_info[i].staffcmd))
			continue;
		if ((subcmd == SCMD_WIZHELP) && !IS_STAFFCMD(i) && (complete_cmd_info[i].minimum_level < TRUST_IMMORTAL))
			continue;

		sprintf(buf + strlen(buf), "%-11s", complete_cmd_info[i].command);
		if (!(no % 7))
			strcat(buf, "\r\n");
		no++;
	}

	strcat(buf, "\r\n");

	page_string(ch->desc, buf, true);

	release_buffer(buf);
}


ACMD(do_watch) {
	SInt32		dir;
	char *		arg;
	Result		result;

	arg = get_buffer(MAX_INPUT_LENGTH);
	argument = one_argument(argument, arg);

	if (!*arg) {
		CHAR_WATCHING(ch) = 0;
		ch->Send("You stop watching.\r\n");
	} else if ((dir = search_block(arg, (const char **)dirs, FALSE)) >= 0) {
		ch->Sendf("You begin watching %s you.\r\n", dir_text_to_of[dir]);

//		if (SKILLROLL(ch, SKILL_STREETWISE) >= 0)
			CHAR_WATCHING(ch) = dir + 1;

// CHANGEPOINT:  The working code should be cleaner.  I like the below.
// 		Skill::Roll(ch, SKILL_WATCH, 0, argument, NULL, NULL);
// 		if (result == Ignored)	return;
//
// 		switch (result) {
// 			case Blunder:
// 			case AFailure:
// 			case Failure:									break;
// 			case PSuccess:
// 			case NSuccess:
// 			case Success:
// 			case ASuccess:	CHAR_WATCHING(ch) = dir + 1;	break;
// 			default:										break;
// 		}
	} else
		ch->Send("That's not a valid direction...\r\n");
	release_buffer(arg);
}


#define ENOUGH_TRUST(ch, vict) (STF_FLAGGED(ch, Staff::Security | Staff::Admin) && \
					GET_TRUST(ch) > GET_TRUST(vict))

struct {
	char *	field;
	Flags	permission;
} StaffPermissions[] = {
	{ "allow",		Staff::Allow	},
	{ "admin",		Staff::Admin	},
	{ "characters",	Staff::Chars	},
	{ "clans",		Staff::Clans	},
	{ "force",		Staff::Force	},
	{ "extra",		Staff::Extra	},
	{ "game",		Staff::Game		},
	{ "general",	Staff::General	},
	{ "help",		Staff::Help		},
	{ "houses",		Staff::Houses	},
	{ "invuln",		Staff::Invuln	},
	{ "mobiles",	Staff::Mobiles	},
	{ "objects",	Staff::Objects	},
	{ "rooms",		Staff::Rooms	},
	{ "olcadmin",	Staff::OLCAdmin	},
	{ "scripts",	Staff::Scripts	},
	{ "security",	Staff::Security	},
	{ "shops",		Staff::Shops	},
	{ "skilledit",	Staff::SkillEd	},
	{ "socials",	Staff::Socials	},
	{ "coder",		Staff::Coder	},
	{ "invuln",		Staff::Invuln	},
	{ "wiznet",		Staff::Wiznet	},
	{ "\n", 0 }
};


ACMD(do_allow) {
	Character *	vict;
	SInt32		i;
	char 		*tmp;

	if (IS_NPC(ch)) {
		ch->Send("Nope.\r\n");
		return;
	}

	char *arg1 = get_buffer(MAX_INPUT_LENGTH);
	char *arg2 = get_buffer(MAX_INPUT_LENGTH);

	half_chop(argument, arg1, argument);
	half_chop(argument, arg2, argument);

	if (!*arg1 || !*arg2 || !STF_FLAGGED(ch, Staff::Admin)) {
		ch->Sendf("Usage: %s <name> <permission>\r\n"
				 "Permissions available:\r\n",
				subcmd == SCMD_ALLOW ? "allow" : "deny");
		for (SInt32 l = 0; *StaffPermissions[l].field != '\n'; l++) {
			if (!STF_FLAGGED(ch, StaffPermissions[l].permission))	continue;
			i = 1;
			ch->Sendf("%-9.9s: ", StaffPermissions[l].field);
			for (SInt32 cmd = 0; cmd < num_of_cmds; cmd++) {
				SInt32 tempCmd = cmd_sort_info[cmd].sort_pos;
				if (IS_SET(complete_cmd_info[tempCmd].staffcmd, StaffPermissions[l].permission)) {
					if (((i % 6) == 1) && (i != 1))	ch->Send("           ");	//	Indent
					ch->Sendf("%-11.11s%s", complete_cmd_info[tempCmd].command,
							!(i % 6) ? "\r\n" : "");
					i++;
				}
			}
			if (--i % 6)	ch->Send("\r\n");
			if (i == 0)		ch->Send("None.\r\n");
		}
	} else if (!(vict = get_player_vis(ch, arg1, false)))
		ch->Send("Player not found.");
	else {
		while (*arg2) {
			for (i = 0; *StaffPermissions[i].field != '\n'; i++)
				if (is_abbrev(arg2, StaffPermissions[i].field))
					break;
			if (*StaffPermissions[i].field == '\n')
				ch->Send("Invalid choice.\r\n");
			else {
				if (subcmd == SCMD_DENY && !ENOUGH_TRUST(ch, vict)) {
					ch->Send("You cannot deny them that privilege.\r\n");
					release_buffer(arg1);
					release_buffer(arg2);
					return;
				}

				if (!STF_FLAGGED(ch, StaffPermissions[i].permission)) {
					ch->Send("You cannot grant or deny that privilege.\r\n");
					release_buffer(arg1);
					release_buffer(arg2);
					return;
				}
				if (subcmd == SCMD_ALLOW)	SET_BIT(STF_FLAGS(vict), StaffPermissions[i].permission);
				else						REMOVE_BIT(STF_FLAGS(vict), StaffPermissions[i].permission);

				mudlogf(BRF, ch, TRUE, "(GC) %s %s %s access to %s commands.", ch->RealName(),
						subcmd == SCMD_ALLOW ? "allowed" : "denied", vict->RealName(), StaffPermissions[i].field);

				ch->Sendf("You %s %s access to %s commands.\r\n", subcmd == SCMD_ALLOW ? "allowed" : "denied",
						vict->RealName(), StaffPermissions[i].field);
				vict->Sendf("%s %s you access to %s commands.\r\n", ch->RealName(),
						subcmd == SCMD_ALLOW ? "allowed" : "denied", StaffPermissions[i].field);
			}
			half_chop(argument, arg2, argument);
		}
	}
	release_buffer(arg1);
	release_buffer(arg2);
}


ACMD(do_call)
{
	Character *found_char = NULL;
	Object *found_obj = NULL;
	IDNum	id = 0;

	if (!ch->desc)			return;
	else if (IS_NPC(ch)) {
		ch->Send("This function is only available to carbon-based lifeforms.\r\n");
		return;
	}

	if (!*argument) {
    	ch->Send("You recall the names of the following:\r\n");

		CallName 						*name = NULL;
		MapIterator<IDNum, CallName>	iter(ch->player->CalledNames);
        const char 						*target = NULL;
        Character 						*i = NULL;

		while ((name = iter.Next())) {
			i = find_char(name->ID());

            if (i == NULL) {
            	if (name->ID() >= 200000)		continue;

				i = new Character();

                target = get_name_by_id(name->ID());
                if (target && i->Load((char *) target) <= -1) {
                	continue;
				}
				strcpy(buf3, i->SDesc("<ERROR>"));
				CAP(buf3);

	            delete i;
			} else {
            	if (IS_NPC(i)) {
                	if (i->SSName())	strcpy(buf3, i->Name("<ERROR>"));
                    else				continue;
				} else {
                	if (i->SSSDesc())	strcpy(buf3, i->SDesc("<ERROR>"));
                    else				continue;
				}
				CAP(buf3);
            }

			ch->Sendf("  %s called %s.\r\n", buf3, name->Value());
		}

    	return;
    }

	two_arguments(argument, buf1, buf2);

	if (!*buf1)			ch->Send("Who or what are you trying to name?\r\n");
	else if (!*buf2)			ch->Send("You need to enter a name?\r\n");
	else {
		generic_find(buf1, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP |
				FIND_CHAR_ROOM, ch, &found_char, &found_obj);

		if (found_char)			id = GET_ID(found_char);
		else if (found_obj)		id = GET_ID(found_obj);

		if (!id) {
			ch->Send("You don't see that here.\r\n");
			return;
		}

		if (found_char && found_char == ch) {
			ch->Sendf("Eh... you ARE that %s.\r\n", GET_SEX(ch) == Male ? "guy" : "girl");
			return;
		}

		if (found_char)	{
			strcpy(buf, found_char->CalledName(ch));
			CAP(buf2);
			ch->Sendf("You now refer to %s as '%s' instead.\r\n", buf, buf2);
		} else if (found_obj) {
			strcpy(buf, found_obj->CalledName(ch));
			CAP(buf2);
			ch->Sendf("You now refer to %s as '%s' instead.\r\n", buf, buf2);
		}

		ch->player->CalledNames[id].Set(id, buf2);
		ch->Save();
	}
}


ACMD(do_forget)
{
	CallName *name = NULL;

	if (!ch->desc)			return;
	else if (IS_NPC(ch)) {
		ch->Send("This function is only available to carbon-based lifeforms.\r\n");
		return;
	}

	one_argument(argument, buf1);

	if (!*buf1)	{
		ch->Send("Who or what are you trying to forget?\r\n");
		return;
	}
	else {
		CAP(buf1);
		MapIterator<IDNum, CallName>	iter(ch->player->CalledNames);
		while ((name = iter.Next())) {
			if (!str_cmp(buf1, name->Value())) {
				ch->player->CalledNames.Remove(name->ID());
				ch->Sendf("Okay, you'll not use that name anymore.\r\n");
				ch->Save();
				return;
			}
		}
	}

	ch->Send("You don't know anyone or anything by that name.\r\n");
}
