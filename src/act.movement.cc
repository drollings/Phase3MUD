/* ************************************************************************
*   File: act.movement.c                                Part of CircleMUD *
*  Usage: movement commands, door handling, & sleep/rest/etc state        *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */



#include "structs.h"
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
#include "event.h"
#include "constants.h"
#include "skills.h"
#include "scripts.h"
#include "eventdefines.h"

/* external functs */
int special(Character *ch, const char *cmd, char *arg);
int find_eq_pos(Character * ch, Object * obj, char *arg,
				SInt8 bottomslot = 0, SInt8 topslot = NUM_WEARS);
void add_follower(Character *ch, Character *leader);
int greet_mtrigger(Character *actor, int dir);
int entry_mtrigger(Character *ch, int dir);
int enter_wtrigger(Room *room, Character *actor, int dir);
int greet_otrigger(Character *actor, int dir);
int sit_otrigger(Object *obj, Character *actor);
int leave_mtrigger(Character *actor, int dir);
int leave_otrigger(Character *actor, int dir);
int leave_wtrigger(Room *room, Character *actor, int dir);
int door_mtrigger(Character *actor, int subcmd, int dir);
int door_otrigger(Character *actor, int subcmd, int dir);
int door_wtrigger(Character *actor, int subcmd, int dir);

ACMD(do_mount);

void AlterMove(Character *ch, SInt32 amount);

/* local functions */
int has_boat(Character *ch);
int find_door(Character *ch, char *type, char *dir, char *cmdname);
bool has_key(Character *ch, int key);
void do_doorcmd(Character *ch, Object *obj, int door, int scmd);
int ok_pick(Character *ch, int keynum, int pickproof, int scmd, SInt8 pick_modifier = 0);
int do_enter_vehicle(Character * ch, char * buf);
ACMD(do_move);
ACMD(do_gen_door);
ACMD(do_enter);
ACMD(do_leave);
ACMD(do_stand);
ACMD(do_sit);
ACMD(do_rest);
ACMD(do_sleep);
ACMD(do_wake);
ACMD(do_follow);
ACMD(do_push_drag);
ACMD(do_recall);
ACMD(do_push_drag_out);		// Expands do_push_drag

extern SInt32 base_response[NUM_RACES+1];

void handle_motion_leaving(Character *ch, int direction);
void handle_motion_entering(Character *ch, int direction);

/* simple function to determine if char can walk on water */
int has_boat(Character *ch)
{
	Object *obj;
	int i;

	if (AFF_FLAGGED(ch, AffectBit::Fly) || (Affect::AffectedBy(ch, SPELL_LEVITATE)) || IS_STAFF(ch))
		return 1;

	/* non-wearable boats in inventory will do it */
	START_ITER(iter, obj, Object *, ch->contents) {
		if (GET_OBJ_TYPE(obj) == ITEM_BOAT && (find_eq_pos(ch, obj, NULL) < 0)) {
			return 1;
		}
	}

	/* and any boat you're wearing will do it too */
	for (i = 0; i < NUM_WEARS; i++)
		if (GET_EQ(ch, i) && GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_BOAT)
			return 1;

	return 0;
}


class AutodoorEvent : public Event {
public:
					AutodoorEvent(time_t when, VNum roomnum, SInt8 todir) :
						Event(when), room(roomnum), dir(todir) {}
					
	VNum		room;
	SInt8			dir;

	time_t			Execute(void);
};


time_t AutodoorEvent::Execute(void) {
	RoomDirection *		exit = NULL;

	if (dir < 0 || dir >= NUM_OF_DIRS || !world.Find(room) || !(exit = world[room].Direction(dir)))
		return 0;		//	Exit not found...

	if (!EXIT_FLAGGED(exit, Exit::Door) || !EXIT_FLAGGED(exit, Exit::Automatic))
		return 0;		//	Exit not good...

	world[room].Send("The %s %s you closes.\n", exit->Keyword("door"), dir_text_to_of[dir]);
	SET_BIT(exit->exit_info, Exit::Closed);
	RoomDirection *	back = world[exit->to_room].Direction(rev_dir[dir]);
	if (back) {
		world[exit->to_room].Send("The %s %s you closes.\n", back->Keyword("door"), dir_text_to_of[rev_dir[dir]]);
		SET_BIT(back->exit_info, Exit::Closed);
	}
	return 0;
};


int vacuum_explode(Character *ch)
{
  char buf[256];

  if ((SECT(IN_ROOM(ch)) == SECT_VACUUM) &&
      (!(AFF_FLAGGED(ch, AffectBit::VacuumSafe) || IS_IMMORTAL(ch)))) {
    ch->Send("It's a vacuum!  Blood boiling, flesh bursting apart... you die in agony!\r\n");
    act("$n screams silently in agony before $s flesh explodes!", TRUE, ch, 0, 0, TO_ROOM);

    sprintf(buf, "%s hit vacuum room #%d (%s)", GET_NAME(ch),
	  world[ch->InRoom()].Virtual(), world[ch->InRoom()].Name());
    mudlog(buf, BRF, 0, TRUE);

    death_mtrigger(ch, NULL);
    ch->Die(NULL);
    return 1;
  }

  return 0;
}


time_t FallingEvent::Execute(void) {
	if (!faller)			return 0;	// Problemo...
	
	/* Character is falling... hehe */
	if ((IN_ROOM(faller) == NOWHERE) || !CAN_GO2(IN_ROOM(faller), DOWN) || 
		!ROOM_FLAGGED(IN_ROOM(faller), ROOM_GRAVITY)) {
		return 0;
	}
	
	if (faller->Fall(EXITN(IN_ROOM(faller), DOWN)->to_room, previous)) {
		++(previous);
		// faller->events.push_back(this);
		return (3 RL_SEC);
	}
	
	return 0;
};


/* do_simple_move assumes
 *    1. That there is no master and no followers.
 *    2. That the direction exists.
 *
 *   Returns :
 *   1 : If succes.
 *   0 : If fail
 */

int do_simple_move(Character *ch, int dir, int need_specials_check) {
	int was_in, need_movement;
	was_in = IN_ROOM(ch);

	// BUG FIX!	If the character has been killed by a script, proceeding
	// with do_simple_move will crash the MUD!	-DH
	if ((PURGED(ch)) || (IN_ROOM(ch) == NOWHERE)) {
		return 0;
	}

	/*
	 * Check for special routines (North is 1 in command list, but 0 here) Note
	 * -- only check if following; this avoids 'double spec-proc' bug
	 */
	if (need_specials_check && special(ch, complete_cmd_info[dir + 1].command, ""))
		return 0;

	if ((PURGED(ch)) || (IN_ROOM(ch) == NOWHERE)) {
		return 0;
	}

	/* charmed? */
	if (AFF_FLAGGED(ch, AffectBit::Charm) && ch->master &&
		 (IN_ROOM(ch) == IN_ROOM(ch->master)) && !RIDING(ch) && !RIDER(ch)) {
		ch->Send("The thought of leaving your master makes you weep.\r\n");
		act("$n bursts into tears.", FALSE, ch, 0, 0, TO_ROOM);
		return 0;
	}

	if (Affect::AffectedBy(ch, SPELL_BIND)) {
		ch->Send("Cords of magic hold you fast!  You cannot move!\r\n");
		return 0;
	}

	/* if this room or the one we're going to is air, check for fly */
	if (((SECT(IN_ROOM(ch)) == SECT_FLYING) ||
		(SECT(EXIT(ch, dir)->to_room) == SECT_FLYING) ||
		(SECT(EXIT(ch, dir)->to_room) == SECT_HIGH_MOUNTAIN)) && !RIDING(ch)) {
		if (!AFF_FLAGGED(ch, AffectBit::Fly) && !NO_STAFF_HASSLE(ch)) {
			ch->Send("You would have to fly to go there.\r\n");
			return 0;
		}
	}

	if (!leave_mtrigger(ch, dir) || !leave_otrigger(ch, dir) ||
			!leave_wtrigger(&world[IN_ROOM(ch)], ch, dir))
		return 0;

	if ((PURGED(ch)) || (IN_ROOM(ch) == NOWHERE)) {
		return 0;
	}

	if (IS_NPC(ch) && EXIT_FLAGGED(EXIT(ch, dir), Exit::NoMob) && !MOB_FLAGGED(ch, MOB_SEEKER))
		return 0;

	/* if this room or the one we're going to needs a boat, check for one */
	if (((SECT(IN_ROOM(ch)) == SECT_WATER_NOSWIM) ||
		(SECT(EXIT(ch, dir)->to_room) == SECT_WATER_NOSWIM)) && !RIDING(ch)) {
		if (!has_boat(ch)) {
			ch->Send("You need a boat to go there.\r\n");
			return 0;
		}
	}

	/* move points needed is avg. move loss for src and destination sect type */
	need_movement = ((movement_loss[SECT(ch->InRoom())] + movement_loss[SECT(EXIT(ch, dir)->to_room)]) +
					GET_ENCUMBRANCE(ch, IS_CARRYING_W(ch))) / 2;
	if (need_movement < 0) need_movement = 0;
	if (ch->material == Materials::Ether)	need_movement = 0;

	if (AFF_FLAGGED(ch, AffectBit::Fly)) {
		switch (dir) {
		case 4: need_movement *= 2; break;	// Up
		case 5: need_movement = 0; break;	// Down
		// What to do?	The speed is greater, but so is the strain...
		default: need_movement /= 2;
		}
	} else if (Affect::AffectedBy(ch, SPELL_LEVITATE))
		need_movement -= need_movement / 2;
	if (AFF_FLAGGED(ch, AffectBit::Haste))
		need_movement /= 2;

	if (ROOM_FLAGGED(EXIT(ch, dir)->to_room, ROOM_MAP))
		need_movement *= 3; // This is done to give greater scale on the map.

	if (MOB_FLAGGED(ch, MOB_STAY_ZONE) &&
		 ((world[EXIT(ch, dir)->to_room].zone != world[ch->InRoom()].zone))) {
		if (!AFF_FLAGGED(ch, AffectBit::Charm) || !(RIDER(ch) && !IS_NPC(RIDER(ch)))) {
		return 0;
		}
	}

	if (MOB_FLAGGED(ch, MOB_STAY_SECTOR) &&
		((SECT(EXIT(ch, dir)->to_room) != SECT(ch->InRoom())))) {
		if (!AFF_FLAGGED(ch, AffectBit::Charm) || !(RIDER(ch) && !IS_NPC(RIDER(ch)))) {
			return 0;
		}
	}

	if ((GET_MOVE(ch) < need_movement) && !IS_NPC(ch) && !NO_STAFF_HASSLE(ch) && !RIDING(ch)) {
		ch->Send((need_specials_check && ch->master) ?
				"You are too exhausted to follow.\r\n" :
				"You are too exhausted.\r\n");
		return 0;
	}

	if (!IS_IMMORTAL(ch) && (GET_ENCUMBRANCE(ch, IS_CARRYING_W(ch)) > 5) && !RIDING(ch)) {
		ch->Send((need_specials_check && ch->master) ?
				"You are too weighed down to follow.\r\n" :
				"You are too weighed down!\r\n");
		return 0;
	}

	if (ROOM_FLAGGED(EXIT(ch, dir)->to_room, ROOM_GODROOM) &&
			!IS_STAFF(ch)) {
		ch->Send("Only staff memebers are allowed in that room!\r\n");
		return 0;
	}

	if (ROOM_FLAGGED(EXIT(ch, dir)->to_room, ROOM_GRGODROOM) &&
			(GET_TRUST(ch) < TRUST_GRGOD)) {
		ch->Send("Only senior staff members are allowed in that room!\r\n");
		return 0;
	}

	if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_JAIL) &&
			(PLR_FLAGGED(ch, PLR_JAILED))) {
		ch->Send("You are in jail!  You cannot leave!\r\n");
		return 0;
	}

	if (ROOM_FLAGGED(EXIT(ch, dir)->to_room, ROOM_HOUSE)) {
		if (!House::CanEnter(ch, EXIT(ch, dir)->to_room)) {
			ch->Send("That's private property -- no trespassing!\r\n");
			return 0;
		}
	}

	if (ROOM_FLAGGED(EXIT(ch, dir)->to_room, ROOM_TUNNEL) &&
			world[EXIT(ch, dir)->to_room].people.Count() > 2) {
		ch->Send("There isn't enough room there for more than two people!\r\n");
		return 0;
	}

	if (!NO_STAFF_HASSLE(ch) && !IS_NPC(ch) && !RIDING(ch))
		AlterMove(ch, need_movement);

	if ((AFF_FLAGGED(ch, AffectBit::Sneak)) && (SKILLROLL(ch, SKILL_SNEAK) < 0)) {
		REMOVE_BIT(AFF_FLAGS(ch), AffectBit::Sneak);
	}

	if ((AFF_FLAGGED(ch, AffectBit::Camoflauge)) && (SKILLROLL(ch, SKILL_CAMOFLAUGE) < 0)) {
		REMOVE_BIT(AFF_FLAGS(ch), AffectBit::Camoflauge);
	}

	if (EXIT_FLAGGED(EXIT(ch, dir), Exit::Automatic) &&		//	Automatic door!
			EXIT_FLAGGED(EXIT(ch, dir), Exit::Closed)) {	//	...that is not open
		world[ch->InRoom()].Send("The %s %s you opens.\n", EXIT(ch, dir)->Keyword("door"), dir_text_to_of[dir]);
		REMOVE_BIT(EXIT(ch, dir)->exit_info, Exit::Closed);
		RoomDirection *	back = world[EXIT(ch, dir)->to_room].Direction(rev_dir[dir]);
		if (back) {
			world[EXIT(ch, dir)->to_room].Send("The %s %s you opens.\n", back->Keyword("door"), dir_text_to_of[rev_dir[dir]]);
			REMOVE_BIT(back->exit_info, Exit::Closed);
		}

		new AutodoorEvent(5 RL_SEC, ch->InRoom(), dir);
	}

	if (RIDING(ch)) {
		sprintf(buf2, "$n %s %s riding $N.",
						(AFF_FLAGGED(RIDING(ch), AffectBit::Fly) ? "flies" : "leaves"), dirs[dir]);
		act(buf2, TRUE, ch, NULL, RIDING(ch), TO_ROOM);
	}
	else if (!AFF_FLAGGED(ch, AffectBit::Sneak) && !RIDER(ch)) {
		sprintf(buf2, "$n %s %s.",
						(AFF_FLAGGED(ch, AffectBit::Fly) ? "flies" : "leaves"), dirs[dir]);
		act(buf2, TRUE, ch, 0, 0, TO_ROOM);
	}

	// BUG FIX!	If the character has been killed by a script, proceeding
	// with do_simple_move will crash the MUD!	-DH
	if ((PURGED(ch)) || (IN_ROOM(ch) == NOWHERE)) {
		return 0;
	}

	handle_motion_leaving(ch, dir);

	if (!entry_mtrigger(ch, dir) || !enter_wtrigger(&world[EXIT(ch, dir)->to_room], ch, dir))
		return 0;

	if (PURGED(ch))
		return 0;

	ch->FromRoom();
	ch->ToRoom(EXIT2(was_in, dir)->to_room);

	if (!NO_STAFF_HASSLE(ch))
		WAIT_STATE(ch, (2 / MAX(1, GET_MOVEMENT(ch))) RL_SEC);
	//	CHANGEPOINT:  Ugh.  Let's be sure about this.
	//	WAIT_STATE(ch, MAX(5, ((base_response[GET_RACE(ch)] RL_SEC) / 10) - (GET_STAT(ch, Ag) / 10) + 5));
	//	WAIT_STATE(ch, MAX(5, ((base_response[GET_RACE(ch)] RL_SEC) / 10) - (GET_STAT(ch, Ag) / 10) + 5));
	//	(8 to 12) - (0 to 10) + 5
	//	.3 - 1.7 sec

	handle_motion_entering(ch, dir);

	// BUG FIX!	If the character has been killed by a script, proceeding
	// with do_simple_move will crash the MUD!	-DH
	if ((PURGED(ch)) || (IN_ROOM(ch) == NOWHERE)) {
		return 0;
	}

	if (RIDING(ch)) {
		if ((!AFF_FLAGGED(RIDING(ch), AffectBit::Sneak)) || (!AFF_FLAGGED(RIDING(ch), AffectBit::Camoflauge))) {
			sprintf(buf2, "$n rides in from %s on $N.", from_dir[dir]);
			act(buf2, TRUE, ch, NULL, RIDING(ch), TO_ROOM);
		}
	} else if (((!AFF_FLAGGED(ch, AffectBit::Sneak)) && (!AFF_FLAGGED(ch, AffectBit::Camoflauge))) && !RIDER(ch)) {
		sprintf(buf2, "$n has arrived from %s.", from_dir[dir]);
		act(buf2, TRUE, ch, 0, 0, TO_ROOM);
	}

	if (0 == entry_mtrigger(ch, dir))
		return 0;
//	if (0 == entry_memory_mtrigger(ch))
//		return 0;
	if (0 == greet_mtrigger(ch, dir))
		return 0;
//	if (0 == greet_memory_mtrigger(ch))
//		return 0;

	// BUG FIX!	If the character has been killed by a script, proceeding
	// with do_simple_move will crash the MUD!	-DH
	if ((PURGED(ch)) || (IN_ROOM(ch) == NOWHERE)) {
		return 0;
	}

	if (ch->desc)				look_at_room(ch, 0);

	if ((ch->material != Materials::Ether) && (ch->material != Materials::Shadow)) {
		if (vacuum_explode(ch))		return 0;

		if (IS_SET(ROOM_FLAGS(IN_ROOM(ch)), ROOM_DEATH) && !IS_STAFF(ch)) {
		    death_mtrigger(ch, NULL);
			ch->Die(NULL);
		    return 0;
		}
	}

	if (AIMING(ch)) {
		ch->Send("You seem to have lost your aim!\r\n");
		AIMING(ch) = 0;
	}

	return 1;
}


bool movement_ok(Character *ch, int dir, RoomDirection *exit)
{
	if (ch->material == Materials::Ether) {
		return true;
	}

	if (EXIT_FLAGGED(exit, Exit::Closed) &&			//	Closed
			(!EXIT_FLAGGED(exit, Exit::Automatic) ||	//	...and not automatic
			(EXIT_FLAGGED(exit, Exit::Locked) &&		//	...or it is automatic
			!has_key(ch, exit->key) &&				//	......and they don't have key
			!NO_STAFF_HASSLE(ch)))) {				//	......and aren't staff
		if (exit->Keyword())
			act("The $T seems to be closed.", FALSE, ch, 0, fname(exit->Keyword()), TO_CHAR);
		else
			ch->Send("It seems to be closed.\r\n");
		return false;
	} else if (EXIT_FLAGGED(EXIT(ch, dir), Exit::Vista)) {
		if (EXIT(ch, dir)->Keyword())
			act("The $T is a window.", FALSE, ch, 0, fname(exit->Keyword()), TO_CHAR);
		else
			ch->Send("That is a window.\r\n");
		return false;
	} else if (IS_NPC(ch) && EXIT_FLAGGED(exit, Exit::NoMob)) {
		ch->Send("Alas, you cannot go that way...\r\n");
		return false;
	}

	return true;
}


int perform_move(Character *ch, int dir, int need_specials_check) {
	VNum		was_in;
	Character *	follower, *tch;
	RoomDirection *	exit;

	if (!ch || dir < 0 || dir >= NUM_OF_DIRS || PURGED(ch) || FIGHTING(ch) || 
		AFF_FLAGGED(ch, AffectBit::Immobilized))
		return 0;
	else if (!(exit = EXIT(ch, dir)) || exit->to_room == NOWHERE)
		ch->Send("Alas, you cannot go that way...\r\n");
	else if (movement_ok(ch, dir, exit)) {
		was_in = IN_ROOM(ch);

		if (RIDING(ch) && (RIDER(RIDING(ch)) == ch) && (IN_ROOM(ch) == IN_ROOM(RIDING(ch)))) {
			if (!perform_move(RIDING(ch), dir, need_specials_check)) {
				act("$N refuses to go that way.", FALSE, ch, NULL, RIDING(ch), TO_CHAR);
				return 0;
			}
		}

		else if (!do_simple_move(ch, dir, need_specials_check))
			return 0;

		START_ITER(mount_iter, tch, Character *, ch->riders) {
			if (was_in == IN_ROOM(tch)) {
				if (FIGHTING(tch) || !perform_move(tch, dir, 1))
					ch->RemoveRider(tch, MNT_MOVE);
			}
		}

		START_ITER(iter, follower, Character *, ch->followers) {
			if ((was_in == IN_ROOM(follower)) &&
	  			(GET_POS(follower) >= POS_STANDING) && !FIGHTING(follower)) {
				act("You follow $N.\r\n", FALSE, follower, NULL, ch, TO_CHAR);
				perform_move(follower, dir, 1);
			}
		}

		START_ITER(ch_iter, tch, Character *, world[was_in].people) {
			if (STALKING(tch) == GET_ID(ch)) {
				act("You follow $N.\r\n", FALSE, tch, NULL, ch, TO_CHAR);
				perform_move(tch, dir, 1);
			}
		}

		return 1;
	}
	return 0;
}


ACMD(do_move) {
	/*
	* This is basically a mapping of cmd numbers to perform_move indices.
	* It cannot be done in perform_move because perform_move is called
	* by other functions which do not require the remapping.
	*/
	if (Event::Find(ch->events, typeid(FallingEvent))) {
		if (subcmd == 5)	// Going down
			ch->Send("You really haven't much choice!\r\n");
		else
			ch->Send("You can't go that way!  How about down instead?\r\n");
	}
	else if (RIDING(ch) && (RIDER(RIDING(ch)) != ch))
		act("You aren't controlling $N.", FALSE, ch, NULL, RIDING(ch), TO_CHAR);
	else
		perform_move(ch, subcmd - 1, 0);
}


int find_door(Character *ch, char *type, char *dir, char *cmdname) {
  int door;

	if (*dir) {		//	a direction was specified
		if ((door = search_block(dir, (const char **)dirs, FALSE)) == -1)	//	Partial Match
			ch->Send("That's not a direction.\r\n");
		else if (EXIT(ch, door)) {
			if (EXIT(ch, door)->Keyword()) {
				if (isname(type, EXIT(ch, door)->Keyword()))
					return door;
				else
					ch->Sendf("I see no %s there.", type);
			} else
				return door;
		} else	ch->Send("I really don't see how you can close anything there.\r\n");
	} else {		//	try to locate the keyword
		if (!*type)	ch->Sendf("What is it you want to %s?\r\n", cmdname);
		else {
			for (door = 0; door < NUM_OF_DIRS; ++door)
				if (EXIT(ch, door) && EXIT(ch, door)->Keyword() && isname(type, EXIT(ch, door)->Keyword()))
					return door;

			if (((door = search_block(type, (const char **)dirs, FALSE)) != -1) && EXIT(ch, door))
				return door;
			else
				ch->Sendf("There doesn't seem to be %s %s here.\r\n", AN(type), type);
		}
	}
	return -1;
}


bool has_key(Character *ch, int key) {
	Object *o;
	int i;

	if (key < 0)
		return false;

	START_ITER(iter, o, Object *, ch->contents)
		if (o->Virtual() == key)
			return true;

	for (i = WEAR_HAND_R; i <= WEAR_HAND_4; ++i) {
		if (GET_EQ(ch, i) && (GET_EQ(ch, i)->Virtual() == key))
			return true;
	}

	return false;
}



#define NEED_OPEN	1
#define NEED_CLOSED	2
#define NEED_UNLOCKED	4
#define NEED_LOCKED	8

char *cmd_door[] =
{
  "open",
  "close",
  "unlock",
  "lock",
  "pick"
};


const int flags_door[] =
{
  NEED_CLOSED | NEED_UNLOCKED,
  NEED_OPEN,
  NEED_CLOSED | NEED_LOCKED,
  NEED_CLOSED | NEED_UNLOCKED,
  NEED_CLOSED | NEED_LOCKED
};


#define OPEN_DOOR(room, obj, door)	((obj) ?\
		(REMOVE_BIT(GET_OBJ_VAL(obj, 1), CONT_CLOSED)) :\
		(REMOVE_BIT(EXIT2(room, door)->exit_info, Exit::Closed)))
#define CLOSE_DOOR(room, obj, door)	((obj) ?\
		(SET_BIT(GET_OBJ_VAL(obj, 1), CONT_CLOSED)) :\
		(SET_BIT(EXIT2(room, door)->exit_info, Exit::Closed)))
#define LOCK_DOOR(room, obj, door)	((obj) ?\
		(SET_BIT(GET_OBJ_VAL(obj, 1), CONT_LOCKED)) :\
		(SET_BIT(EXIT2(room, door)->exit_info, Exit::Locked)))
#define UNLOCK_DOOR(room, obj, door)	((obj) ?\
		(REMOVE_BIT(GET_OBJ_VAL(obj, 1), CONT_LOCKED)) :\
		(REMOVE_BIT(EXIT2(room, door)->exit_info, Exit::Locked)))

void do_doorcmd(Character *ch, Object *obj, int door, int scmd) {
	int other_room = 0;
	RoomDirection *back = NULL;
	char *buf;

	if (!obj && (scmd == SCMD_OPEN || scmd == SCMD_CLOSE) &&
			EXIT_FLAGGED(EXIT(ch, door), Exit::Automatic)) {
		ch->Send("But this is an automatic door!\r\n");
		return;
	}

	if (!door_mtrigger(ch, scmd, door))	return;
	if (!door_otrigger(ch, scmd, door))	return;
	if (!door_wtrigger(ch, scmd, door))	return;

	buf = get_buffer(MAX_STRING_LENGTH);
	sprintf(buf, "$n %ss ", cmd_door[scmd]);
	if (!obj && ((other_room = EXIT(ch, door)->to_room) != NOWHERE))
		if ((back = world[other_room].Direction(rev_dir[door])))
			if (back->to_room != IN_ROOM(ch))
				back = NULL;

	switch (scmd) {
		case SCMD_OPEN:
			OPEN_DOOR(IN_ROOM(ch), obj, door);
			if (back)	OPEN_DOOR(other_room, obj, rev_dir[door]);
//			ch->Send(OK);
			break;
		case SCMD_CLOSE:
			CLOSE_DOOR(IN_ROOM(ch), obj, door);
			if (back)	CLOSE_DOOR(other_room, obj, rev_dir[door]);
//			ch->Send(OK);
			break;
		case SCMD_UNLOCK:
			UNLOCK_DOOR(IN_ROOM(ch), obj, door);
			if (back)	UNLOCK_DOOR(other_room, obj, rev_dir[door]);
//			ch->Send("*Click*\r\n");
			break;
		case SCMD_LOCK:
			LOCK_DOOR(IN_ROOM(ch), obj, door);
			if (back)	LOCK_DOOR(other_room, obj, rev_dir[door]);
//			ch->Send("*Click*\r\n");
			break;
		case SCMD_PICK:
			UNLOCK_DOOR(IN_ROOM(ch), obj, door);
			if (back)	UNLOCK_DOOR(other_room, obj, rev_dir[door]);
			ch->Send("The lock quickly yields to your skills.\r\n");
			strcpy(buf, "$n skillfully picks the lock on ");
			break;
	}
	if (scmd != SCMD_PICK)
		act(obj ? "You $t $P." : "You $t the $F.", FALSE, ch, (Object *)cmd_door[scmd],
				obj ? obj : (void *)EXIT(ch, door)->Keyword("door"), TO_CHAR);

	/* Notify the room */
	sprintf(buf + strlen(buf), "%s%s.", ((obj) ? "" : "the "), (obj) ? "$p" :
			(EXIT(ch, door)->Keyword() ? "$F" : "door"));
	if (!obj || (IN_ROOM(obj) != NOWHERE))
		act(buf, FALSE, ch, obj, obj ? 0 : EXIT(ch, door)->Keyword(), TO_ROOM);

	/* Notify the other room */
	if ((scmd == SCMD_OPEN || scmd == SCMD_CLOSE) && back && world[EXIT(ch, door)->to_room].people.Count()) {
		world[EXIT(ch, door)->to_room].Send("The %s is %s%s from the other side.\r\n",
				fname(back->Keyword("door")), cmd_door[scmd],
				(scmd == SCMD_CLOSE) ? "d" : "ed");
//		act(buf, FALSE, world[EXIT(ch, door)->to_room].people.Top(), 0, 0, TO_ROOM | TO_CHAR);
	}
	release_buffer(buf);
}


int ok_pick(Character *ch, int keynum, int pickproof, int scmd, SInt8 pick_modifier = 0)
{
	int percent;

	if (scmd == SCMD_PICK) {
		if (keynum < 0)		ch->Send("Odd - you can't seem to find a keyhole.\r\n");
		else if (pickproof)	ch->Send("It resists your attempts to pick it.\r\n");
		else if ((SKILLROLL(ch, SKILL_PICK_LOCK) + pick_modifier) < 0)
			ch->Send("You failed to pick the lock.\r\n");
		else				return 1;
		return 0;
	}
	return 1;
}


#define DOOR_IS_OPENABLE(ch, obj, door)	((obj) ? \
			((GET_OBJ_TYPE(obj) == ITEM_CONTAINER) && \
			(OBJVAL_FLAGGED(obj, CONT_CLOSEABLE))) :\
			(EXIT_FLAGGED(EXIT(ch, door), Exit::Door)))
#define DOOR_IS_OPEN(ch, obj, door)	((obj) ? \
			(!OBJVAL_FLAGGED(obj, CONT_CLOSED)) :\
			(!EXIT_FLAGGED(EXIT(ch, door), Exit::Closed)))
#define DOOR_IS_UNLOCKED(ch, obj, door)	((obj) ? \
			(!OBJVAL_FLAGGED(obj, CONT_LOCKED)) :\
			(!EXIT_FLAGGED(EXIT(ch, door), Exit::Locked)))
#define DOOR_IS_PICKPROOF(ch, obj, door) ((obj) ? \
			(OBJVAL_FLAGGED(obj, CONT_PICKPROOF)) : \
			(EXIT_FLAGGED(EXIT(ch, door), Exit::Pickproof)))

#define DOOR_IS_CLOSED(ch, obj, door)	(!(DOOR_IS_OPEN(ch, obj, door)))
#define DOOR_IS_LOCKED(ch, obj, door)	(!(DOOR_IS_UNLOCKED(ch, obj, door)))
#define DOOR_KEY(ch, obj, door)		((obj) ? (GET_OBJ_VAL(obj, 2)) : \
					(EXIT(ch, door)->key))
#define DOOR_LOCK(ch, obj, door)	((obj) ? (GET_OBJ_VAL(obj, 1)) : \
					(EXIT(ch, door)->exit_info))



int perform_gen_door(Character *ch, Object *obj, int door, int subcmd)
{
	int keynum;
	SInt8 modifier = 0;

	if ((obj) || (door >= 0)) {
		keynum = DOOR_KEY(ch, obj, door);

		if (obj)	modifier = GET_OBJ_VAL(obj, 4);
		else		modifier = EXIT(ch, door)->pick_modifier;

		if (!(DOOR_IS_OPENABLE(ch, obj, door)))
			act("You can't $F that!", FALSE, ch, 0, cmd_door[subcmd], TO_CHAR);
		else if (!DOOR_IS_OPEN(ch, obj, door) && IS_SET(flags_door[subcmd], NEED_OPEN))
			ch->Send("But it's already closed!\r\n");
		else if (!DOOR_IS_CLOSED(ch, obj, door) && IS_SET(flags_door[subcmd], NEED_CLOSED))
			ch->Send("But it's currently open!\r\n");
		else if (!(DOOR_IS_LOCKED(ch, obj, door)) && IS_SET(flags_door[subcmd], NEED_LOCKED))
			ch->Send("Oh.. it wasn't locked, after all..\r\n");
		else if (!(DOOR_IS_UNLOCKED(ch, obj, door)) && IS_SET(flags_door[subcmd], NEED_UNLOCKED))
			ch->Send("It seems to be locked.\r\n");
		else if (!has_key(ch, keynum) && !IS_STAFF(ch) &&
				((subcmd == SCMD_LOCK) || (subcmd == SCMD_UNLOCK)))
			ch->Send("You don't seem to have the proper key.\r\n");
		else if (ok_pick(ch, keynum, DOOR_IS_PICKPROOF(ch, obj, door), subcmd, modifier))
			do_doorcmd(ch, obj, door, subcmd);

		return TRUE;
	}

	return FALSE;
}

ACMD(do_gen_door) {
	int door = -1, keynum, modifier;
	char *type, *dir;
	Object *obj = NULL;
	Character *victim = NULL;

	skip_spaces(argument);
	if (!*argument) {
		ch->Sendf("%s what?\r\n", cmd_door[subcmd]);
		return;
	}

	type = get_buffer(MAX_INPUT_LENGTH);
	dir = get_buffer(MAX_INPUT_LENGTH);
	two_arguments(argument, type, dir);

	if (!generic_find(type, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &victim, &obj))
		door = find_door(ch, type, dir, cmd_door[subcmd]);

	if ((obj) || (door >= 0)) {
		perform_gen_door(ch, obj, door, subcmd);
	}
	release_buffer(type);
	release_buffer(dir);
	return;
}


int do_enter_vehicle(Character * ch, char *buf) {
	Object * vehicle = get_obj_in_list_vis(ch, buf, world[IN_ROOM(ch)].contents);
	VNum	vehicle_room;

	if (vehicle && (GET_OBJ_TYPE(vehicle) == ITEM_VEHICLE)) {
		if (IS_MOUNTABLE(vehicle))
			act("You can ride $p, but not get into it.", TRUE, ch, vehicle, 0, TO_CHAR);
		else if (!Room::Find(vehicle_room = GET_OBJ_VAL(vehicle, 0)))
			ch->Send("Serious problem: Vehicle has no insides!");
		else {
			act("You climb into $p.", TRUE, ch, vehicle, 0, TO_CHAR);
			act("$n climbs into $p", TRUE, ch, vehicle, 0, TO_ROOM);
			ch->FromRoom();
			ch->ToRoom(vehicle_room);
			act("$n climbs in.", TRUE, ch, 0, 0, TO_ROOM);
			if (ch->desc != NULL)
				look_at_room(ch, 0);
		}
		return 1;
	}
	return 0;
}


ACMD(do_enter) {
	int door;
	char *buf = get_buffer(MAX_INPUT_LENGTH);

	one_argument(argument, buf);

	if (*buf) {			/* an argument was supplied, search for door keyword */
		for (door = 0; door < NUM_OF_DIRS; door++)
			if (EXIT(ch, door) && EXIT(ch, door)->Keyword())
				if (!str_cmp(EXIT(ch, door)->Keyword(), buf)) {
					perform_move(ch, door, 1);
					release_buffer(buf);
					return;
				}
		//	No door was found.  Search for a vehicle
		if (!do_enter_vehicle(ch, buf))
			ch->Sendf("There is no %s here.\r\n", buf);
	} else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_INDOORS)) {
		ch->Send("You are already indoors.\r\n");
	} else {	//	try to locate an entrance
		for (door = 0; door < NUM_OF_DIRS; door++)
			if (EXIT(ch, door))
				if (EXIT(ch, door)->to_room != NOWHERE)
					if (!EXIT_FLAGGED(EXIT(ch, door), Exit::Closed) &&
							ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_INDOORS)) {
						perform_move(ch, door, 1);
						release_buffer(buf);
						return;
					}
		//	No door was found.  Search for a vehicle
		if (!do_enter_vehicle(ch, buf))
			ch->Send("You can't seem to find anything to enter.\r\n");
	}
	release_buffer(buf);
}


ACMD(do_leave) {
	int door;
	Object * hatch, * vehicle;

	/* FIRST see if there is a hatch in the room */
	hatch = get_obj_in_list_type(ITEM_V_HATCH, world[IN_ROOM(ch)].contents);
	if (hatch) {
		vehicle = find_vehicle_by_vnum(GET_OBJ_VAL(hatch, 0));
		if (!vehicle || IN_ROOM(vehicle) == NOWHERE) {
			ch->Send("The hatch seems to lead to nowhere...\r\n");
			return;
		}
		act("$n leaves $p.", TRUE, ch, vehicle, 0, TO_ROOM);
		act("You climb out of $p.", TRUE, ch, vehicle, 0, TO_CHAR);
		ch->FromRoom();
		ch->ToRoom(IN_ROOM(vehicle));
		act("$n climbs out of $p.", TRUE, ch, vehicle, 0, TO_ROOM);

		/* Now show them the room */
		if (ch->desc != NULL)
			look_at_room(ch, 0);

		return;
	}

	if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_INDOORS))
		ch->Send("You are outside.. where do you want to go?\r\n");
	else {
		for (door = 0; door < NUM_OF_DIRS; door++)
			if (EXIT(ch, door))
				if (EXIT(ch, door)->to_room != NOWHERE)
					if (!EXIT_FLAGGED(EXIT(ch, door), Exit::Closed) &&
							!ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_INDOORS)) {
						perform_move(ch, door, 1);
						return;
					}
		ch->Send("I see no obvious exits to the outside.\r\n");
	}
}


ACMD(do_stand)
{
	ACMD(do_dismount);

	if (ch->SittingOn()) {
		if (IN_ROOM(ch->SittingOn()) == IN_ROOM(ch)) {
			if (IS_MOUNTABLE(ch->SittingOn())) {
				do_dismount(ch, "", 0, "dismount", 0);
				return;
			}
			switch (GET_POS(ch)) {
			case POS_SITTING:
				act("You stop sitting on $p and stand up.",
						FALSE, ch, ch->SittingOn(), 0, TO_CHAR);
				act("$n stops sitting on $p and clambers to $s feet.",
						FALSE, ch, ch->SittingOn(), 0, TO_ROOM);
				GET_POS(ch) = POS_STANDING;
				ch->Dismount();
				return;
			case POS_RESTING:
				act("You stop resting on $p and stand up.",
						FALSE, ch, ch->SittingOn(), 0, TO_CHAR);
				act("$n stops resting on $p and clambers to $s feet.",
						FALSE, ch, ch->SittingOn(), 0, TO_ROOM);
				GET_POS(ch) = POS_STANDING;
				ch->Dismount();
				return;
			case POS_RIDING:
				act("You dismount $p.",
						FALSE, ch, ch->SittingOn(), 0, TO_CHAR);
				act("$n dismounts $p.",
						FALSE, ch, ch->SittingOn(), 0, TO_ROOM);
				GET_POS(ch) = POS_STANDING;
				ch->Dismount();
				return;
			default:
				break;
			}
		}
	}
	switch (GET_POS(ch)) {
	case POS_STANDING:
		act("You are already standing.", FALSE, ch, 0, 0, TO_CHAR);
		break;
	case POS_SITTING:
		act("You stand up.", FALSE, ch, 0, 0, TO_CHAR);
		act("$n clambers to $s feet.", TRUE, ch, 0, 0, TO_ROOM);
		GET_POS(ch) = POS_STANDING;
		break;
	case POS_RESTING:
		act("You stop resting, and stand up.", FALSE, ch, 0, 0, TO_CHAR);
		act("$n stops resting, and clambers on $s feet.", TRUE, ch, 0, 0, TO_ROOM);
		GET_POS(ch) = POS_STANDING;
		break;
	case POS_SLEEPING:
		act("You have to wake up first!", FALSE, ch, 0, 0, TO_CHAR);
		break;
	default:
		act("You stop floating around, and put your feet on the ground.",
	FALSE, ch, 0, 0, TO_CHAR);
		act("$n stops floating around, and puts $s feet on the ground.",
	TRUE, ch, 0, 0, TO_ROOM);
		GET_POS(ch) = POS_STANDING;
		break;
	}
}


ACMD(do_sit)
{
  Object *obj = NULL;
  char *arg	= get_buffer(MAX_INPUT_LENGTH);

  one_argument(argument, arg);

  if (*arg) {
	obj = get_obj_in_list_vis(ch, arg, world[IN_ROOM(ch)].contents);
	release_buffer(arg);
	if ((obj) && (GET_POS(ch) >= POS_STANDING))
		if (IS_MOUNTABLE(obj)) {
			do_mount(ch, argument, 0, "mount", 0);
			return;
		} else if (IS_SITTABLE(obj)) {
			if (get_num_chars_on_obj(obj) >= GET_OBJ_VAL(obj, 0)) {
				act("There is no more room on $p.", FALSE, ch, obj, 0, TO_CHAR);
				return;
			} else if (!sit_otrigger(obj, ch))
				return;
		} else {
			act("You can't sit on $p.", FALSE, ch, obj, 0, TO_CHAR);
				return;
		}
	} else {
		release_buffer(arg);
	}

	switch (GET_POS(ch)) {
		case POS_STANDING:
			if (obj) {
				act("You sit on $p.", FALSE, ch, obj, 0, TO_CHAR);
				act("$n sits on $p.", FALSE, ch, obj, 0, TO_ROOM);
				ch->SetMount(obj);
			} else {
				act("You sit down.", FALSE, ch, 0, 0, TO_CHAR);
				act("$n sits down.", FALSE, ch, 0, 0, TO_ROOM);
			}
			GET_POS(ch) = POS_SITTING;
			break;
		case POS_SITTING:
			ch->Send("You're sitting already.\r\n");
			break;
		case POS_RESTING:
			act("You stop resting, and sit up.", FALSE, ch, 0, 0, TO_CHAR);
			act("$n stops resting.", TRUE, ch, 0, 0, TO_ROOM);
			GET_POS(ch) = POS_SITTING;
			break;
		case POS_SLEEPING:
			act("You have to wake up first.", FALSE, ch, 0, 0, TO_CHAR);
			break;
		default:
			if (obj) {
				act("You stop floating around, and sit on $p.", FALSE, ch, obj, 0, TO_CHAR);
				act("$n stops floating around, and sits on $p.", TRUE, ch, obj, 0, TO_ROOM);
				ch->SetMount(obj);
			} else {
				act("You stop floating around, and sit down.", FALSE, ch, 0, 0, TO_CHAR);
				act("$n stops floating around, and sits down.", TRUE, ch, 0, 0, TO_ROOM);
			}
			GET_POS(ch) = POS_SITTING;
			break;
	}
}


ACMD(do_rest) {
  Object *obj = NULL;
  char * arg = get_buffer(MAX_INPUT_LENGTH);

  one_argument(argument, arg);

  if ((*arg) && (GET_POS(ch) != POS_SITTING)) {
	obj = get_obj_in_list_vis(ch, arg, world[IN_ROOM(ch)].contents);
	release_buffer(arg);
	if ((obj) && (GET_POS(ch) >= POS_STANDING))
		if (IS_SITTABLE(obj)) {
			if (get_num_chars_on_obj(obj) >= GET_OBJ_VAL(obj, 0)) {
					act("There is no more room on $p.", FALSE, ch, obj, 0, TO_CHAR);
					return;
			} else if (!sit_otrigger(obj, ch))
				return;
		} else {
			act("You can't rest on $p.", FALSE, ch, obj, 0, TO_CHAR);
			return;
		}
  } else
  	release_buffer(arg);

  switch (GET_POS(ch)) {
  case POS_STANDING:
  	if (obj) {
	    act("You sit down on $p and rest your tired bones.", FALSE, ch, obj, 0, TO_CHAR);
	    act("$n sits down on $p and rests.", TRUE, ch, obj, 0, TO_ROOM);
		ch->SetMount(obj);
  	} else {
	    act("You sit down and rest your tired bones.", FALSE, ch, 0, 0, TO_CHAR);
	    act("$n sits down and rests.", TRUE, ch, 0, 0, TO_ROOM);
    }
    GET_POS(ch) = POS_RESTING;
    break;
  case POS_SITTING:
    if (ch->SittingOn())
    	if(IS_MOUNTABLE(ch->SittingOn())) {
    		act("You can't rest on $p.", TRUE, ch, ch->SittingOn(), 0, TO_CHAR);
    		return;
    	}
    act("You rest your tired bones.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n rests.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_RESTING;
    break;
  case POS_RESTING:
    act("You are already resting.", FALSE, ch, 0, 0, TO_CHAR);
    break;
  case POS_SLEEPING:
    act("You have to wake up first.", FALSE, ch, 0, 0, TO_CHAR);
    break;
  default:
  	if (obj) {
	    act("You stop floating around, and stop to rest your tired bones on $p.",
				FALSE, ch, obj, 0, TO_CHAR);
	    act("$n stops floating around, and rests on $p.", FALSE, ch, obj, 0, TO_ROOM);
		ch->SetMount(obj);
  	} else {
	    act("You stop floating around, and stop to rest your tired bones.",
				FALSE, ch, 0, 0, TO_CHAR);
	    act("$n stops floating around, and rests.", FALSE, ch, 0, 0, TO_ROOM);
    }
    GET_POS(ch) = POS_SITTING;
    break;
  }
}


ACMD(do_sleep) {
	Object *obj = NULL;
	char * arg = get_buffer(MAX_INPUT_LENGTH);

	one_argument(argument, arg);

	if ((ch->SittingOn()) && (IN_ROOM(ch) == IN_ROOM(ch->SittingOn())) && !IS_SLEEPABLE(ch->SittingOn())) {
		act("You can't sleep on $p.", FALSE, ch, ch->SittingOn(), 0, TO_CHAR);
		release_buffer(arg);
		return;
	}

	if (*arg) {
		obj = get_obj_in_list_vis(ch, arg, world[IN_ROOM(ch)].contents);
		release_buffer(arg);
		if (obj && !IS_SLEEPABLE(obj)) {
			act("You won't be able to sleep on $p.", FALSE, ch, obj, 0, TO_CHAR);
			return;
		}
		if (obj && (GET_POS(ch) >= POS_STANDING)) {
			if (!IS_SLEEPABLE(obj)) {
				act("You can't sleep on $p.", FALSE, ch, obj, 0, TO_CHAR);
				return;
			} else if (get_num_chars_on_obj(obj) >= GET_OBJ_VAL(obj, 0)) {
				act("There is no more room on $p.", FALSE, ch, obj, 0, TO_CHAR);
				return;
			} else if (!sit_otrigger(obj, ch) || PURGED(ch))
				return;
			else if (PURGED(obj))
				obj = NULL;		//	In case the script purged the object, but we still let the
								//	player sleep
		}
	} else
		release_buffer(arg);


	switch (GET_POS(ch)) {
		case POS_SITTING:
		case POS_RESTING:
			if (obj && ch->SittingOn() && (IN_ROOM(ch->SittingOn()) == IN_ROOM(ch)) && (ch->SittingOn() != obj)) {
				act("You are already $T on $p.", FALSE, ch, ch->SittingOn(),
						(GET_POS(ch) == POS_SITTING) ? "sitting" : "resting", TO_CHAR);
				return;
			}
		case POS_STANDING:
			if (obj) {
				act("You lie down on $p and go to sleep.",
				FALSE, ch, obj, 0, TO_CHAR);
				act("$n lies down on $p and falls asleep.", FALSE, ch, obj, 0, TO_ROOM);
				ch->SetMount(obj);
			} else {
				ch->Send("You go to sleep.\r\n");
				act("$n lies down and falls asleep.", TRUE, ch, 0, 0, TO_ROOM);
			}
			GET_POS(ch) = POS_SLEEPING;
			break;
		case POS_SLEEPING:
			ch->Send("You are already sound asleep.\r\n");
			break;
		default:
			if (obj) {
				act("You stop floating around, and fall asleep on $p.",
						FALSE, ch, obj, 0, TO_CHAR);
				act("$n stops floating around, and falls asleep on $p.", FALSE, ch, obj, 0, TO_ROOM);
				ch->SetMount(obj);
			} else {
				act("You stop floating around, and lie down to sleep.",
						FALSE, ch, 0, 0, TO_CHAR);
				act("$n stops floating around, and lie down to sleep.",
						TRUE, ch, 0, 0, TO_ROOM);
			}
			GET_POS(ch) = POS_SLEEPING;
			break;
	}
}


ACMD(do_wake) {
	Character *vict;
	Object *obj = NULL;
	int self = 0;
	char *arg = get_buffer(MAX_INPUT_LENGTH);

	one_argument(argument, arg);

	if (*arg) {
		if (GET_POS(ch) == POS_SLEEPING)
			ch->Send("Maybe you should wake yourself up first.\r\n");
		else if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM)))
			ch->Send(NOPERSON);
		else if (vict == ch)
			self = 1;
		else if (GET_POS(vict) > POS_SLEEPING)
			act("$E is already awake.", FALSE, ch, 0, vict, TO_CHAR);
		else if (AFF_FLAGGED(vict, AffectBit::Sleep) || (GET_MOVE(vict) <= 0))
			act("You can't wake $M up!", FALSE, ch, 0, vict, TO_CHAR);
		else if (GET_POS(vict) < POS_SLEEPING)
			act("$E's in pretty bad shape!", FALSE, ch, 0, vict, TO_CHAR);
		else {
			act("You wake $M up.", FALSE, ch, 0, vict, TO_CHAR);
			act("You are awakened by $n.", FALSE, ch, 0, vict, TO_VICT | TO_SLEEP);
			act("$n wakes $N up.", FALSE, ch, 0, vict, TO_NOTVICT);
			GET_POS(vict) = POS_STANDING;
		}
		if (!self) {
			release_buffer(arg);
			return;
		}
	}

	release_buffer(arg);

	if (AFF_FLAGGED(ch, AffectBit::Sleep) || (GET_MOVE(ch) <= 0)) {
    	ch->Send("You can't wake up!\r\n");
	} else if (GET_POS(ch) > POS_SLEEPING) {
    	ch->Send("You are already awake...\r\n");
	} else {
		ch->Send("You awaken, and clamber to your feet.\r\n");
	    act("$n awakens and clambers upright.", TRUE, ch, 0, 0, TO_ROOM);
		GET_POS(ch) = POS_STANDING;
	}

	if (ch->SittingOn() && !IS_SLEEPABLE(ch->SittingOn())) {
		ch->Dismount();
	}

}


ACMD(do_follow) {
	Character *leader;
	char *buf = get_buffer(MAX_INPUT_LENGTH);

	one_argument(argument, buf);

	if (!*buf)
		ch->Send("Whom do you wish to follow?\r\n");
	else if (!(leader = get_char_vis(ch, buf, FIND_CHAR_ROOM)))
		ch->Send(NOPERSON);
	else if (ch->master == leader)
		act("You are already following $M.", FALSE, ch, 0, leader, TO_CHAR);
	else if (AFF_FLAGGED(ch, AffectBit::Charm) && (ch->master))
		act("But you only feel like following $N!", FALSE, ch, 0, ch->master, TO_CHAR);
	else if (leader == ch) {			/* Not Charmed follow person */
		if (!ch->master)	ch->Send("You are already following yourself.\r\n");
		else				ch->StopFollower();
	} else if (circle_follow(ch, leader))
		act("Sorry, but following in loops is not allowed.", FALSE, ch, 0, 0, TO_CHAR);
	else {
		if (ch->master)
			ch->StopFollower();
		REMOVE_BIT(AFF_FLAGS(ch), AffectBit::Group);
		add_follower(ch, leader);
	}
	release_buffer(buf);
}


#define CAN_DRAG_CHAR(ch, vict)	((GET_STR(ch) * 8) >= (GET_WEIGHT(vict) + IS_CARRYING_W(ch) + IS_CARRYING_W(vict)))
#define CAN_PUSH_CHAR(ch, vict)	((GET_STR(ch) * 8) >= (GET_WEIGHT(vict) + IS_CARRYING_W(ch) + IS_CARRYING_W(vict)))
#define CAN_DRAG_OBJ(ch, obj)	((GET_STR(ch) * 8) >= GET_WEIGHT(obj))
#define CAN_PUSH_OBJ(ch, obj)	((GET_STR(ch) * 8) >= GET_WEIGHT(obj))

ACMD(do_push_drag) {
	Object *	obj = NULL;
	Character *	vict;
	int					mode, dir;
	VNum				dest;
	char	*arg, *arg2;

	if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL) && !NO_STAFF_HASSLE(ch)) {
		act("But its so peaceful here...", FALSE, ch, 0, 0, TO_CHAR);
		return;
	}

	arg = get_buffer(MAX_INPUT_LENGTH);
	arg2 = get_buffer(MAX_INPUT_LENGTH);

	two_arguments(argument, arg, arg2);

	if (!*arg)
		act("Move what?", FALSE, ch, 0, 0, TO_CHAR);
	else if (!*arg2)
		act("Move it in what direction?", FALSE, ch, 0, 0, TO_CHAR);
	else if (is_abbrev(arg2, "out"))
		do_push_drag_out(ch, arg, cmd, command, subcmd);
	else if ((dir = search_block(arg2, (const char **)dirs, FALSE)) < 0 || dir > NUM_OF_DIRS)
		act("That's not a valid direction.", FALSE, ch, 0, 0, TO_CHAR);
	else if (!CAN_GO(ch, dir) || EXIT_FLAGGED(EXIT(ch, dir), Exit::Vista))
		act("You can't go that way.", FALSE, ch, 0, 0, TO_CHAR);
	else if (dir == UP)
		act("No pushing people up, lama.", FALSE, ch, 0, 0, TO_CHAR);
	else {
		dest = EXIT(ch, dir)->to_room;

		release_buffer(arg2);

		if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
			if (!(obj = get_obj_in_list_vis(ch, arg, world[IN_ROOM(ch)].contents))) {
				act("There is nothing like that here.", FALSE, ch, 0, 0, TO_CHAR);
				release_buffer(arg);
				return;
			} else
				mode = 0;
		} else if (vict == ch) {
			act("Very funny...", FALSE, ch, 0, 0, TO_CHAR);
			release_buffer(arg);
			return;
		} else
			mode = 1;

		release_buffer(arg);

		if (!mode) {
			if (!OBJ_FLAGGED(obj, ITEM_MOVEABLE) ||
					(subcmd == SCMD_PUSH ? !CAN_PUSH_OBJ(ch, obj) : !CAN_DRAG_OBJ(ch, obj))) {
				act("You try to $T $p, but it won't move.", FALSE, ch, obj,
						(subcmd == SCMD_PUSH) ? "push" : "drag", TO_CHAR);
				act("$n tries in vain to move $p.", TRUE, ch,obj, 0, TO_ROOM);
				return;
			}
		} else {
			if (GET_POS(vict) <= POS_SITTING && subcmd == SCMD_PUSH) {
				act("Trying to push people who are on the ground won't work.", FALSE, ch, 0, 0, TO_CHAR);
				return;
			}
			if (!NO_STAFF_HASSLE(ch)) {
				if (IS_NPC(vict)) {
					if (MOB_FLAGGED(vict, MOB_NOBASH)) {
						act("You try to move $N, but fail.", FALSE, ch, 0, vict, TO_CHAR);
						act("$n tries to move $N, but fails.", FALSE, ch, 0, vict, TO_ROOM);
						return;
					}
					if (MOB_FLAGGED(vict, MOB_SENTINEL)) {
						act("$N won't budge.", FALSE, ch, 0, vict, TO_CHAR);
						act("$n tries to move $N, who won't budge.", FALSE, ch, 0, vict, TO_ROOM);
						return;
					}
					if (EXIT_FLAGGED(EXIT(ch, dir), Exit::NoMob)) {
						act("$N can't go that way.", FALSE, ch, 0, vict, TO_CHAR);
						act("$n tries to move $N, who won't budge.", FALSE, ch, 0, vict, TO_ROOM);
						return;
					}
					// I'll take my chances. -DH
					// act("Due to recent abuse of the PUSH and DRAG commands, you can no longer\r\n"
					// 	"do that to MOBs.", FALSE, ch, 0, 0, TO_CHAR);
					// return;
				}
				if (NO_STAFF_HASSLE(vict)) {
					act("You can't push staff around!", FALSE, ch, 0, vict, TO_CHAR);
					act("$n is trying to push you around.", FALSE, ch, 0, vict, TO_VICT);
					return;
				}
				if ((subcmd == SCMD_PUSH) ? !CAN_PUSH_CHAR(ch, vict) : !CAN_DRAG_CHAR(ch, vict)) {
					if (subcmd == SCMD_PUSH) {
						act("You try to push $N $t, but fail.", FALSE, ch, (Object *)dirs[dir], vict, TO_CHAR);
						act("$n tries to push $N $t, but fails.", FALSE, ch, (Object *)dirs[dir], vict, TO_NOTVICT);
						act("$n tried to push you $t!", FALSE, ch, (Object *)dirs[dir], vict, TO_VICT);
					} else {
						act("You try to push $N $t, but fail.", FALSE, ch, (Object *)dirs[dir], vict, TO_CHAR);
						act("$n tries to push $N $t, but fails.", FALSE, ch, (Object *)dirs[dir], vict, TO_NOTVICT);
						act("$n grabs on to you and tries to drag you $T, but can't seem to move you!", FALSE, ch, (Object *)dirs[dir], vict, TO_VICT);
					}
					return;
				}
			}
		}


		if (subcmd == SCMD_DRAG) {
			switch (mode) {
				case 0:
					act("You drag $p $T.", FALSE, ch, obj, dirs[dir], TO_CHAR );
					act("$n drags $p $T.", FALSE, ch, obj, dirs[dir], TO_ROOM );
					ch->FromRoom();
					ch->ToRoom(dest);
					obj->FromRoom();
					obj->ToRoom(dest);
					act("$n drags $p into the room from $T.", FALSE, ch, obj, dir_text[rev_dir[dir]], TO_ROOM );
					break;
				case 1:
					act("You drag $N $t.", FALSE, ch, (Object *)dirs[dir], vict, TO_CHAR );
					act("$n drags $N $t.", FALSE, ch, (Object *)dirs[dir], vict, TO_NOTVICT);
					act("$n drags you $t!", FALSE, ch, (Object *)dirs[dir], vict, TO_VICT);
					ch->FromRoom();
					ch->ToRoom(dest);
					vict->FromRoom();
					vict->ToRoom(dest);
					act("$n drags $N into the room from $t.", FALSE, ch, (Object *)dir_text[rev_dir[dir]], vict, TO_NOTVICT );
					look_at_room(vict, 0);
					break;
			}
			look_at_room(ch, 0);
		} else {
			switch (mode) {
				case 0:
					act("You push $p $T.", FALSE, ch, obj, dirs[dir], TO_CHAR );
					act("$n pushes $p $T.", FALSE, ch, obj, dirs[dir], TO_ROOM );
					obj->FromRoom();
					obj->ToRoom(dest);
					act("$p is pushed into the room from $T.", FALSE, 0, obj,
							dir_text[rev_dir[dir]], TO_ROOM);
					break;
				case 1:
					act("You push $N $t.", FALSE, ch, (Object *)dirs[dir], vict, TO_CHAR );
					act("$n pushes $N $t.", FALSE, ch, (Object *)dirs[dir], vict, TO_NOTVICT );
					act("$n pushes you $t!", FALSE, ch, (Object *)dirs[dir], vict, TO_VICT);
					vict->FromRoom();
					vict->ToRoom(dest);
					if (GET_POS(vict) > POS_SITTING) GET_POS(vict) = POS_SITTING;
					look_at_room(vict, 0);
					act("$n is pushed into the room from $T.", FALSE, vict, 0, dir_text[rev_dir[dir]], TO_ROOM );
					break;
			}
		}
		WAIT_STATE(ch, 10 RL_SEC);
		return;
	}
	release_buffer(arg);
	release_buffer(arg2);
}


ACMD(do_push_drag_out) {
	Object * hatch, *vehicle, *obj;
	Character *vict = NULL;
	int dest, mode;
	char *arg;

	if (!(hatch = get_obj_in_list_type(ITEM_V_HATCH, world[IN_ROOM(ch)].contents))) {
		act("What?  There is no vehicle exit here.", FALSE, ch, 0, 0, TO_CHAR);
		return;
	}

	if (!(vehicle = find_vehicle_by_vnum(GET_OBJ_VAL(hatch, 0)))) {
		act("The hatch seems to lead to nowhere...", FALSE, ch, 0, 0, TO_CHAR);
		return;
	}

	dest = IN_ROOM(vehicle);
	arg = get_buffer(MAX_INPUT_LENGTH);
	one_argument(argument, arg);

	if (!(obj = get_obj_in_list_vis(ch, arg, world[IN_ROOM(ch)].contents))) {
		if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
			act("There is nothing like that here.", FALSE, ch, 0, 0, TO_CHAR);
			release_buffer(arg);
			return;
		} else if (vict == ch) {
			act("Very funny...", FALSE, ch, 0, 0, TO_CHAR);
			release_buffer(arg);
			return;
		} else
			mode = 1;
		if (!NO_STAFF_HASSLE(ch) && !IS_NPC(ch) && !IS_NPC(vict)) {	// Mort pushing mort out
			act("Shya, as if.  Now everyone knows you are a loser.", FALSE, ch, 0, vict, TO_CHAR);
			act("$n tried to push $N OUT...  What a loser!", FALSE, ch, 0, vict, TO_NOTVICT);
			act("$n tried to push you OUT, because $e is such a loser!", FALSE, ch, 0, vict, TO_VICT);
			release_buffer(arg);
			return;
		}
	} else
		mode = 0;

	release_buffer(arg);

	if (!mode) {
		if (!OBJ_FLAGGED(obj, ITEM_MOVEABLE) ||
					(subcmd == SCMD_PUSH ? !CAN_PUSH_OBJ(ch, obj) : !CAN_DRAG_OBJ(ch, obj))) {
			act("You try to $T $p, but it won't move.", FALSE, ch, obj,
					(subcmd == SCMD_PUSH) ? "push" : "drag", TO_CHAR);
			act("$n tries in vain to move $p.", TRUE, ch,obj, 0, TO_ROOM);
			return;
		}
	} else {
		if ((GET_POS(vict) <= POS_SITTING) && (subcmd == SCMD_PUSH)) {
			act("Trying to push people who are on the ground won't work.", FALSE, ch, 0, 0, TO_CHAR);
			return;
		}
		if (!NO_STAFF_HASSLE(ch)) {
			if (IS_NPC(vict)) {
				if (MOB_FLAGGED(vict, MOB_NOBASH)) {
					act("You try to move $N, but fail.", FALSE, ch, 0, vict, TO_CHAR);
					act("$n tries to move $N, but fails.", FALSE, ch, 0, vict, TO_ROOM);
					return;
				}
				if (MOB_FLAGGED(vict, MOB_SENTINEL)) {
					act("$N won't budge.", FALSE, ch, 0, vict, TO_CHAR);
					act("$n tries to move $N, who won't budge.", FALSE, ch, 0, vict, TO_ROOM);
					return;
				}
	/*			if (IS_SET(EXIT(ch, dir)->exit_info, Exit::NoMob)) {
					act("$N can't go that way.", FALSE, ch, 0, vict, TO_CHAR);
					act("$n tries to move $N, who won't budge.", FALSE, ch, 0, vict, TO_ROOM);
					return;
				}*/
			}
			if (NO_STAFF_HASSLE(vict)) {
				act("You can't push staff around!", FALSE, ch, 0, vict, TO_CHAR);
				act("$n is trying to push you around.", FALSE, ch, 0, vict, TO_VICT);
				return;
			}
			if ((subcmd == SCMD_PUSH) ? !CAN_PUSH_CHAR(ch, vict) : !CAN_DRAG_CHAR(ch, vict)) {
				if (subcmd == SCMD_PUSH) {
					act("You try to push $N out, but fail.", FALSE, ch, 0, vict, TO_CHAR);
					act("$n tries to push $N out, but fails.", FALSE, ch, 0, vict, TO_NOTVICT);
					act("$n tried to push you out!", FALSE, ch, 0, vict, TO_VICT);
				} else {
					act("You try to push $N out, but fail.", FALSE, ch, 0, vict, TO_CHAR);
					act("$n tries to push $N out, but fails.", FALSE, ch, 0, vict, TO_NOTVICT);
					act("$n grabs on to you and tries to drag you out, but can't seem to move you!", FALSE, ch, 0, vict, TO_VICT);
				}
				return;
			}
		}
	}


	if (subcmd == SCMD_DRAG) {
		if (mode) {
			act("You drag $N out.", FALSE, ch, 0, vict, TO_CHAR );
			act("$n drags $N out.", FALSE, ch, 0, vict, TO_NOTVICT);
			act("$n drags you out!", FALSE, ch, 0, vict, TO_VICT);
			ch->FromRoom();
			ch->ToRoom(dest);
			vict->FromRoom();
			vict->ToRoom(dest);
			act("$n drags $N out of $p.", FALSE, ch, vehicle, vict, TO_NOTVICT );
			look_at_room(vict, 0);
			look_at_room(ch, 0);
		} else {
			act("You drag $p out.", FALSE, ch, obj, 0, TO_CHAR );
			act("$n drags $p out.", FALSE, ch, obj, 0, TO_ROOM );
			ch->FromRoom();
			ch->ToRoom(dest);
			obj->FromRoom();
			obj->ToRoom(dest);
			act("$n drags $p out of $P.", FALSE, ch, obj, vehicle, TO_ROOM );
			look_at_room(ch, 0);
		}
		look_at_room(ch, 0);
	} else {
		if (mode) {
			act("You push $N out.", FALSE, ch, 0, vict, TO_CHAR );
			act("$n pushes $N out.", FALSE, ch, 0, vict, TO_NOTVICT );
			act("$n pushes you out!", FALSE, ch, 0, vict, TO_VICT);
			vict->FromRoom();
			vict->ToRoom(dest);
			if (GET_POS(vict) > POS_SITTING) GET_POS(vict) = POS_SITTING;
			look_at_room(vict, 0);
			act("$n is pushed out of $p.", FALSE, vict, vehicle, 0, TO_ROOM );
		} else {
			act("You push $p out.", FALSE, ch, obj, 0, TO_CHAR );
			act("$n pushes $p out.", FALSE, ch, obj, 0, TO_ROOM );
			obj->FromRoom();
			obj->ToRoom(dest);
			act("$p is pushed out of $P.", FALSE, 0, obj, vehicle, TO_ROOM);
		}
	}
	WAIT_STATE(ch, 10 RL_SEC);
	return;
}


void handle_motion_leaving(Character *ch, int direction) {
	int door, nextroom = NOWHERE, distance;
	Character *vict;
	char *buf = get_buffer(MAX_INPUT_LENGTH);

	LListIterator<Character *>	iter(world[nextroom].people);
	while ((vict = iter.Next())) {
		if (AIMING(vict) == GET_ID(ch))		CHAR_WATCHING(vict) = direction + 1;
	}

	for (door = 0; door < NUM_OF_DIRS; door++) {
		if (!CAN_GO(ch, door))
			continue;
		nextroom = IN_ROOM(ch);		/* Get the next room */


		for (distance = 1; (distance < MAX_DISTANCE + 1); distance++) {
			if (!CAN_GO2(nextroom, door))
				break;
			nextroom = EXIT2(nextroom, door)->to_room;

			LListIterator<Character *>	iter(world[nextroom].people);

			while ((vict = iter.Next())) {
				if (!AWAKE(vict))	continue;

				if (AFF_FLAGGED(vict, AffectBit::Tracking)) {
					vict->Sendf("You detect motion %s %s you.\r\n", distance_lower[distance],
							dir_text_to_of[rev_dir[door]] );
				}

				if (!CHAR_WATCHING(vict))	continue;

				if (rev_dir[CHAR_WATCHING(vict) - 1] == door && ch->CanBeSeen(vict)) {
					if (((distance == MAX_DISTANCE) &&						/* Moving out	*/
								(CHAR_WATCHING(vict) - 1 == direction)) ||
								((door != direction) && (door != rev_dir[direction]))) {	/* Moving out of view */
						sprintf(buf, "%s %s you see $N move out of view.", distance_upper[distance],
								dir_text_to_of[CHAR_WATCHING(vict) - 1]);
						act(buf, TRUE, vict, NULL, ch, TO_CHAR);
						if (AIMING(vict) == GET_ID(ch)) {
							act("You seem to have lost your aim!", TRUE, vict, NULL, ch, TO_CHAR);
							AIMING(vict) = 0;
						}
					} else if ((door != direction) || (distance > 1)) {
						sprintf(buf, "%s %s you see $N move %s you.", distance_upper[distance],
								dirs[CHAR_WATCHING(vict) - 1],
								(direction == ( CHAR_WATCHING(vict) - 1 )) ? "away from" : "towards");
						act(buf, TRUE, vict, NULL, ch, TO_CHAR);
					}
				}	/* IF ...watching... */
			}
		}	/* FOR distance */
	}	/* FOR door */
	release_buffer(buf);
}


void handle_motion_entering(Character *ch, int direction) {
	int door, nextroom, distance;
	Character *vict;
	char *buf = get_buffer(MAX_INPUT_LENGTH);

	direction = rev_dir[direction];

	for (door = 0; door < NUM_OF_DIRS; door++) {
		if (!CAN_GO(ch, door))	continue;
		nextroom = IN_ROOM(ch);		/* Get the next room */

		for (distance = 1; (distance < MAX_DISTANCE + 1); ++distance) {
			if (!CAN_GO2(nextroom, door))
				break;
			nextroom = EXIT2(nextroom, door)->to_room;
			LListIterator<Character *>	iter(world[nextroom].people);

			while ((vict = iter.Next())) {
				if (!AWAKE(vict))	continue;

				if (AFF_FLAGGED(vict, AffectBit::Tracking) &&
						(((distance == MAX_DISTANCE) && (direction == rev_dir[door])) ||
						((door != direction) && (door != rev_dir[direction])))) {
					vict->Sendf("You detect motion %s %s you.\r\n", distance_lower[distance],
							dir_text_to_of[rev_dir[door]] );
				}

				if (!CHAR_WATCHING(vict) && (AIMING(vict) != GET_ID(ch)))	continue;

				if (rev_dir[CHAR_WATCHING(vict) - 1] == door && ch->CanBeSeen(vict)) {
					if (((distance == MAX_DISTANCE) && (CHAR_WATCHING(vict) - 1 == direction)) ||
								((door != direction) && (door != rev_dir[direction]))) {	/* Moving into view */
						sprintf(buf, "%s %s you see $N move into view.", distance_upper[distance],
								dir_text_to_of[CHAR_WATCHING(vict) - 1]);
						act(buf, TRUE, vict, 0, ch, TO_CHAR);
					}
				}	/* IF ...watching... */
			}
		}	/* FOR distance */


	}	/* FOR door */
	release_buffer(buf);
}


ACMD(do_stalk)
{
	Character *leader = NULL;

	void stop_follower(Character *ch);
	void add_follower(Character *ch, Character *leader);

	one_argument(argument, buf);

	if (*buf) {
		leader = get_char_vis(ch, buf, FIND_CHAR_ROOM);

		if (leader == NULL) {
			ch->Send(NOPERSON);
			return;
		}

	} else {
		if (!STALKING(ch))		leader = find_char(STALKING(ch));

		if (leader == NULL)		ch->Send("You aren't stalking anyone.\r\n");
		else					ch->Send("You cease your stalking activities.\r\n");

		STALKING(ch) = 0;
		return;
	}

	if (GET_ID(leader) == GET_ID(ch)) {
		ch->Send("Stalking yourself makes no sense.\r\n");
	} else if ((STALKING(ch)) && (STALKING(ch) == GET_ID(leader))) {
		act("You are already stalking $M.", FALSE, ch, 0, leader, TO_CHAR);
	} else if (AFF_FLAGGED(ch, AffectBit::Charm) && (ch->master)) {
		act("But you only feel like following $N!", FALSE, ch, 0, ch->master, TO_CHAR);
	} else {			/* Not Charmed follow person */
		act("You start stalking $N.", FALSE, ch, 0, leader, TO_CHAR);
		STALKING(ch) = GET_ID(leader);
	}
}


// Returns true if we are to continue falling.
bool MUDObject::Fall(VNum toroom, int prev)
{
	VNum			was_in = IN_ROOM(this);
	
	if ((IN_ROOM(this) == NOWHERE) || !CAN_GO2(IN_ROOM(this), DOWN)) {
		return false;
	}
	
	act("$p falls downward!", TRUE, 0, this, 0, TO_ROOM);
	
	this->FromRoom();
	this->ToRoom(EXITN(was_in, DOWN)->to_room);

	act("$p falls from above!", FALSE, NULL, this, NULL, TO_ROOM);

	if ((IN_ROOM(this) == was_in) || !ROOM_FLAGGED(IN_ROOM(this), ROOM_GRAVITY) ||
			!CAN_GO2(IN_ROOM(this), DOWN) /*|| IS_SET(EXIT(this, DOWN)->exit_info, EX_NOMOVE)*/) {
		act("$p hits the ground with a thud.", TRUE, NULL, this, NULL, TO_ROOM);
		return false;
	}
	
	return true;
		
}


// Returns true if we are to continue falling.
bool Character::Fall(VNum toroom, int prev)
{
	VNum			was_in = IN_ROOM(this);
	Attack			attack;
	
	/* Character is falling... hehe */
	if (NO_STAFF_HASSLE(this) || (IN_ROOM(this) == NOWHERE) || !CAN_GO2(IN_ROOM(this), DOWN) ||
			AFF_FLAGGED(this, AffectBit::Fly) || !ROOM_FLAGGED(IN_ROOM(this), ROOM_GRAVITY)) {
		return false;
	}
	
	if (!leave_mtrigger(this, DOWN) || !leave_otrigger(this, DOWN) || !leave_wtrigger(&world[IN_ROOM(this)], this, DOWN) ||
			!enter_wtrigger(&world[EXIT(this, DOWN)->to_room], this, DOWN)) {
		return 0;
	}
	
	attack.damtype = Damage::Fall;
	
	if (prev)				act("You are falling!\r\n", TRUE, this, 0, 0, TO_CHAR);
	else					act("You fall!\r\n", TRUE, this, 0, 0, TO_CHAR);
	
	act("$n falls downward!", TRUE, this, 0, 0, TO_ROOM);
	
	this->FromRoom();
	this->ToRoom(EXITN(was_in, DOWN)->to_room);
	
	look_at_room(this, 0);

	act("$n falls from above!", TRUE, this, 0, 0, TO_ROOM);
	
	if ((IN_ROOM(this) == was_in) || !ROOM_FLAGGED(IN_ROOM(this), ROOM_GRAVITY) ||
			!CAN_GO(this, DOWN) /*|| IS_SET(EXIT(this, DOWN)->exit_info, EX_NOMOVE)*/) {
		was_in = IN_ROOM(this);
		act("You hit the ground with a $T.", TRUE, this, 0, (prev <= 2) ? "thud" : "splat", TO_CHAR);
		act("$n hits the ground with a $T.", TRUE, this, 0, (prev <= 2) ? "thud" : "splat", TO_ROOM);
		attack.dam = MIN(dice(prev * 5, 20), GET_HIT(this) + 99);
		

		if (this->Damage(NULL, &attack) >= 0) {
			switch (GET_POS(this)) {
			case POS_MORTALLYW:
				act("Damn... you broke your neck... you can't move... all you can do is...\r\n"
					"...lie here until you die.",
						TRUE, this, 0, 0, TO_CHAR);
				// Don't break, hit the default case.
			default:
				if (!PURGED(this) || (Room::Find(IN_ROOM(this))))	greet_mtrigger(this, DOWN);
				if (!PURGED(this) || (Room::Find(IN_ROOM(this))))	greet_otrigger(this, DOWN);
				break;
			case POS_DEAD:
				break;
			}
		}
		
		return false;
	}
	
	if (!PURGED(this) || (Room::Find(IN_ROOM(this))))	greet_mtrigger(this, DOWN);
	if (!PURGED(this) || (Room::Find(IN_ROOM(this))))	greet_otrigger(this, DOWN);

	return true;
}


