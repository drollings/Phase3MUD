#include "characters.h"
#include "descriptors.h"
#include "event.h"
#include "index.h"
#include "objects.h"
#include "opinion.h"
#include "rooms.h"
#include "comm.h"
#include "handler.h"
#include "interpreter.h"
#include "utils.h"
#include "track.h"
#include "combat.h"
#include "buffer.h"

ACMD(do_flee);
ACMD(do_hit);
ACMD(do_stand);
ACMD(do_wake);
extern int no_specials;

/* local functions */
void mobile_activity(Character *ch);
void clearMemory(Character * ch);
void character_update(UInt32 pulse);
bool clearpath(Character *ch, VNum room, int dir);
void MobHunt(Character *ch);
void MobSeek(Character *ch);
void FoundHated(Character *ch, Character *vict);
Character *AggroScan(Character *mob, SInt32 range);

extern int find_first_step(VNum src, char *target, Flags edgetype = 0);
extern int find_first_step(VNum src, VNum target, Flags edgetype = 0);
extern int perform_gen_door(Character *ch, Object *obj, int door, int subcmd);


bool clearpath(Character *ch, VNum room, int dir) {
	RoomDirection *exit;

	if (room == -1)	return false;
	
	exit = world[room].Direction(dir);

	if (!exit || (exit->to_room == NOWHERE))					return false;
	if (EXIT_FLAGGED(exit, Exit::Vista | Exit::Closed))		return false;
	if (IS_NPC(ch) && EXIT_FLAGGED(exit, Exit::NoMob))			return false;
	if (IS_NPC(ch) && ROOM_FLAGGED(exit->to_room, ROOM_NOMOB))	return false;
	if (MOB_FLAGGED(ch, MOB_STAY_ZONE) &&
			world[room].zone != world[exit->to_room].zone)		return false;
	
	return true;
}


Character *AggroScan(Character *mob, SInt32 range) {
	SInt32	i, r;
	VNum	room;
	Character *ch = NULL;
	LListIterator<Character *>	iter;
	
	for (i = 0; i < NUM_OF_DIRS; i++) {
		r = 0;
		room = IN_ROOM(mob);
		while (r < range) {
			if (clearpath(mob, room, i)) {
				room = world[room].Direction(i)->to_room;
				r++;
				iter.Start(world[room].people);
				while ((ch = iter.Next())) {
					if (ch->CanBeSeen(mob) && mob->mob->hates &&
							mob->mob->hates->Contains(GET_ID(ch)))
						return ch;
				}
				iter.Finish();
			} else
				r = range + 1;
		}
	}
	return NULL;
}


void FoundHated(Character *ch, Character *vict) {
	if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
		ch->Hit(vict);
}


void MobHunt(Character *ch) {
#if 0
	Character *	vict;
	VNum		dest;
	SInt32		dir;
	
	if (!ch->path)	return;
	
//	if (--ch->persist <= 0) {
//		delete ch->path;
//		ch->path = NULL;
//		return;
//	}
	
	vict = find_char(HUNTING(ch));
	if (!vict)	return;
	
	dest = ch->path->dest;
	
	if ((dir = Track(ch)) != -1) {
		perform_move(ch, dir, 1);
		if (vict) {
			if (IN_ROOM(ch) == IN_ROOM(vict)) {
				//	Determined mobs: they will wait till you leave the room, then
				//	give chase!!!
				if (ch->mob->hates && ch->mob->hates->Contains(GET_ID(vict)) &&
						!ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
					FoundHated(ch, vict);
					delete ch->path;
					ch->path = NULL;
				}
			
			}
		} else if (IN_ROOM(ch) == dest) {
			delete ch->path;
			ch->path = NULL;
		}
	}
#else
	Character *hunting = NULL;

	ACMD(do_say);

	int dir;
	SInt8 found;
	Character *tmp;

	if (!ch || !IS_NPC(ch) || (!HUNTING(ch)))
		return;

	if (HUNTING(ch) == GET_ID(ch))
		HUNTING(ch) = 0;

	if (HUNTING(ch))
		hunting = find_char(HUNTING(ch));

/* Daniel Rollings AKA Daniel Houghton's new code: Only seeker mobs can hunt across zones! */
	if (hunting) {
		if ((world[IN_ROOM(ch)].zone != world[IN_ROOM(hunting)].zone) &&
				MOB_FLAGGED(ch, MOB_STAY_ZONE)) {
			HUNTING(ch) = 0;
			return;
		}
	} else {
/*		do_say(ch, "Damn!	My prey is gone!!", 0, 0); */
		HUNTING(ch) = 0;
		return;
	}

/* Daniel Rollings AKA Daniel Houghton's new code: Wimpy mobs won't hunt with > 33% HP. */
	if (MOB_FLAGGED(ch, MOB_WIMPY) && (GET_HIT(ch) < (GET_MAX_HIT(ch) / 3)))
		return;

	if (GET_HIT(ch) < (GET_MAX_HIT(ch) / 6))
		return;

	if (GET_POS(ch) <= POS_SLEEPING) {
		do_wake(ch, "", 0, "wake", 0);
		return;
	}

	if (GET_POS(ch) < POS_STANDING && !RIDING(ch)) {
		do_stand(ch, "", 0, "stand", 0);
		return;
	}

	dir = find_first_step(IN_ROOM(ch), IN_ROOM(hunting), 0);

	if (IN_ROOM(ch) != IN_ROOM(hunting)) {
		if ((dir < 0) || 
			(AFF_FLAGGED(hunting, AffectBit::Sneak) && (!Number(0,2))) ) {
/*			do_say(ch, "Damn!	Lost them!", 0, 0); */
			HUNTING(ch) = 0;
			return;
		} else {
			if ((EXIT_FLAGGED(EXIT(ch, dir), Exit::Closed)) && 
					(!EXIT_FLAGGED(EXIT(ch, dir), Exit::Locked)) && 
					(GET_INT(ch) > 4)) {
					perform_gen_door(ch, NULL, dir, SCMD_OPEN);
				}
				perform_move(ch, dir, 1);
			}
		}

	if (IN_ROOM(ch) == hunting->InRoom()) {
		if (hunting->CanBeSeen(ch)) {
			ch->Hit(hunting);
			return;
		} else { 
/*			do_say(ch, "Damn!	Where'd they go?", 0, 0); */
			hunting = NULL;
			return;
		}
		return;
	}
#endif
}


void MobSeek(Character *ch) {
#if 0
	Character *	vict;
	VNum		dest;
	SInt32		dir;
	
	if (!ch->path)	return;
	
//	if (--ch->persist <= 0) {
//		delete ch->path;
//		ch->path = NULL;
//		return;
//	}
	
	vict = find_char(SEEKING(ch));
	if (!vict)	return;
	
	dest = ch->path->dest;
	
	if ((dir = Track(ch)) != -1) {
		perform_move(ch, dir, 1);
		if (vict) {
			if (IN_ROOM(ch) == IN_ROOM(vict)) {
				delete ch->path;
				ch->path = NULL;
			}
		} else if (IN_ROOM(ch) == dest) {
			delete ch->path;
			ch->path = NULL;
		}
	}
#else
	Character *seeking = NULL;

	ACMD(do_say);
	extern Character *character_list;

	int dir;
	SInt8 found;
	Character *tmp;

	if (!ch || !IS_NPC(ch) || (SEEKING(ch) == 0))
		return;

	if (SEEKING(ch) == GET_ID(ch))
		SEEKING(ch) = 0;

	if (SEEKING(ch) != 0)
		seeking = find_char(SEEKING(ch));

	if (seeking) {
		if ((world[IN_ROOM(ch)].zone != world[seeking->InRoom()].zone) &&
				MOB_FLAGGED(ch, MOB_STAY_ZONE)) {
			SEEKING(ch) = 0;
			return;
		}
	} else {
/*		do_say(ch, "Damn!	They're gone!", 0, 0); */
		SEEKING(ch) = 0;
		return;
	}

	if (GET_POS(ch) <= POS_SLEEPING) {
		do_wake(ch, "", 0, "wake", 0);
		return;
	}

	if (GET_POS(ch) < POS_STANDING && !RIDING(ch)) {
		do_stand(ch, "", 0, "stand", 0);
		return;
	}

	dir = find_first_step(IN_ROOM(ch), seeking->InRoom(), 0);

	if (IN_ROOM(ch) != seeking->InRoom()) {
		if (dir < 0) {
/*			do_say(ch, "Damn.	Lost them.", 0, 0); */
			SEEKING(ch) = 0;
			return;
		} else {
			if ((EXIT_FLAGGED(EXIT(ch, dir), Exit::Closed)) && 
					(!EXIT_FLAGGED(EXIT(ch, dir), Exit::Locked)) && 
					(GET_INT(ch) > 4)) {
					perform_gen_door(ch, NULL, dir, SCMD_OPEN);
				}
				perform_move(ch, dir, 1);
			}
		}

	if (IN_ROOM(ch) == seeking->InRoom())
		SEEKING(ch) = 0;

	return;
#endif
}


void MobRoomSeek(Character *ch) {
#if 0
	VNum		dest;
	SInt32		dir;
	
	if (!ch->path)	return;
	
//	if (--ch->persist <= 0) {
//		delete ch->path;
//		ch->path = NULL;
//		return;
//	}
	
	dest = ch->path->dest;
	
	if ((dir = Track(ch)) != -1) {
		perform_move(ch, dir, 1);
		if (IN_ROOM(ch) == dest) {
			delete ch->path;
			ch->path = NULL;
		}
	}
#else
	int dir = 0;

	if (!ch || !IS_NPC(ch) || (ROOMSEEKING(ch) <= 0))
		return;

/* Daniel Rollings AKA Daniel Houghton's new code: Only seeker mobs can seek across zones! */
	if ((world[IN_ROOM(ch)].zone != world[ROOMSEEKING(ch)].zone) &&
			MOB_FLAGGED(ch, MOB_STAY_ZONE) && (!MOB_FLAGGED(ch, MOB_SEEKER))) {
		ROOMSEEKING(ch) = 0;
		return;
	}

/* Okay, time for the mobile to find some direction in life */

	if (GET_POS(ch) <= POS_SLEEPING) {
		do_wake(ch, "", 0, "wake", 0);
		return;
	}

	if (GET_POS(ch) < POS_STANDING && !RIDING(ch)) {
		do_stand(ch, "", 0, "stand", 0);
		return;
	}

	dir = find_first_step(IN_ROOM(ch), ROOMSEEKING(ch), 0);

	if (IN_ROOM(ch) != ROOMSEEKING(ch)) {
		if (dir < 0) {
			ROOMSEEKING(ch) = 0;
			return;
		} else {
			if ((EXIT_FLAGGED(EXIT(ch, dir), Exit::Closed)) && 
					(!EXIT_FLAGGED(EXIT(ch, dir), Exit::Locked)) && 
					(GET_INT(ch) > 4)) {
					perform_gen_door(ch, NULL, dir, SCMD_OPEN);
				}
				perform_move(ch, dir, 1);
		}
	} else {
		ROOMSEEKING(ch) = 0;
	}

	if (IN_ROOM(ch) == ROOMSEEKING(ch)) {
		ROOMSEEKING(ch) = 0;
	}

	return;
#endif
}


void character_update(UInt32 pulse) {
	Character *	ch;
	UInt32		tick;
	UInt8		stagger = Number(1, 4);
	
	tick = (pulse / PULSE_MOBILE) % TICK_WRAP_COUNT;
	
	START_ITER(iter, ch, Character *, Characters) {
		if (IS_NPC(ch) && (!AFF_FLAGGED(ch, AffectBit::Immobilized))) {
			// Take care of this now.  We want fast-acting MOBs.

			if (ch->mob->tick == tick)						mobile_activity(ch);
			if (GET_MOVE(ch) >= 5) {
				if (HUNTING(ch) && (tick % stagger))		MobHunt(ch);
				if (SEEKING(ch) && (tick % stagger))		MobSeek(ch);
				if (ROOMSEEKING(ch) && (tick % stagger))	MobRoomSeek(ch);
            }
		} else {
			ENERGY(ch).Update(ch);
        }
	}
}


void mobile_activity(Character *ch) {
	Character	*temp;
	Object		*obj, *best_obj;
	SInt32		max, door;
	Relation	relation;
	
	if (IN_ROOM(ch) == -1) {
		log("Char %s in NOWHERE", ch->RealName());
		ch->ToRoom(0);
	}
	
	if (GET_MOVE(ch) <= 0)		return;

	//	SPECPROC
	if (MOB_FLAGGED(ch, MOB_SPEC) && !no_specials) {
		if (!Character::Index[ch->Virtual()].func) {
			log("SYSERR: %s (#%d) - Attempting to call non-existing mob func", ch->RealName(), ch->Virtual());
			REMOVE_BIT(MOB_FLAGS(ch), MOB_SPEC);
		} else if ((Character::Index[ch->Virtual()].func) (ch, ch, 0, ""))
			return;		/* go to next char */
	}

	if (AWAKE(ch) && !FIGHTING(ch)) {
		//	ASSIST
		if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL) && MOB_FLAGGED(ch, MOB_HELPER)) {
			START_ITER(iter, temp, Character *, world[IN_ROOM(ch)].people) {
				if ((ch != temp) && !PURGED(temp) && FIGHTING(temp) && !PURGED(FIGHTING(temp)) &&
					temp->CanBeSeen(ch) && (FIGHTING(temp) != ch) && !NO_STAFF_HASSLE(temp)) {

					if (!is_same_group(ch, temp)) {
						if (!MOB_FLAGGED(ch, MOB_HELPER))
							continue;

						relation = ch->GetRelation(temp);
						if (relation != RELATION_FRIEND)
							continue;
						if (ch->GetRelation(FIGHTING(temp)) == RELATION_FRIEND)
							continue;
					}
					act("$n jumps to the aid of $N!", FALSE, ch, 0, temp, TO_ROOM);
					ch->Hit(FIGHTING(temp));
					return;

				}
			}
		}
		
		//	SCAVENGE
		if (MOB_FLAGGED(ch, MOB_SCAVENGER) && world[IN_ROOM(ch)].contents.Count() && !Number(0, 10)) {
			max = 1;
			best_obj = NULL;
			START_ITER(iter, obj, Object *, world[IN_ROOM(ch)].contents) {
				if (CAN_GET_OBJ(ch, obj) && GET_OBJ_COST(obj) > max) {
					best_obj = obj;
					max = GET_OBJ_COST(obj);
				}
			}
			if (best_obj) {
				best_obj->FromRoom();
				best_obj->ToChar(ch);
				act("$n gets $p.", FALSE, ch, best_obj, 0, TO_ROOM);
				return;
			}
		}
		
		//	WANDER
		door = Number(0, 18);
		if (!MOB_FLAGGED(ch, MOB_SENTINEL) && (GET_POS(ch) == POS_STANDING) &&
				(door < NUM_OF_DIRS) && CAN_GO(ch, door) &&
				!EXIT_FLAGGED(EXIT(ch, door), Exit::Vista | Exit::NoMob) &&
				!ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_NOMOB | ROOM_DEATH) &&
				(!MOB_FLAGGED(ch, MOB_STAY_ZONE) ||
				(world[EXIT(ch, door)->to_room].zone == world[IN_ROOM(ch)].zone))) {
			perform_move(ch, door, 1);
			return;		// You've done enough for now...
		}
		
		
		//	It's debatable why I have this check against peacefulness...
		//	The primary reason is because mobs should be smart enough to know
		//	when they are safe from other mobs
		if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
			//	FEAR (normal)
			if ((GET_HIT(ch) > (GET_MAX_HIT(ch) / 2)) && ch->mob->fears && 
            	(temp = ch->mob->fears->FindTarget(ch)) && !Number(0, 2)) {
				do_flee(ch, "", 0, "flee", 0);
				return;
			}
			
			//	HATE
			if (ch->mob->hates && (temp = ch->mob->hates->FindTarget(ch))) {
				FoundHated(ch, temp);
				return;
			}
			
			//	FEAR (desperate non-haters)
			if (ch->mob->fears && (temp = ch->mob->fears->FindTarget(ch))) {
				do_flee(ch, "", 0, "flee", 0);
				return;
			}
		}


		//	AGGR
		if (MOB_FLAGGED(ch, MOB_AGGRESSIVE | MOB_AGGR_ALL)) {
			START_ITER(iter, temp, Character *, world[IN_ROOM(ch)].people) {
				if (ch == temp)
					continue;
				if (!temp->CanBeSeen(ch) || NO_STAFF_HASSLE(temp))
					continue;
				if (MOB_FLAGGED(ch, MOB_WIMPY) && AWAKE(temp))
					continue;
				relation = ch->GetRelation(temp);
				if ((MOB_FLAGGED(ch, MOB_AGGR_ALL)) || 
					(MOB_FLAGGED(ch, MOB_AGGRESSIVE) && relation != RELATION_FRIEND)) {
					ch->Hit(temp);
					return;
				}
			}
		}
		
		if (ch->mob->hates) {
			if ((temp = AggroScan(ch, 3))) {
				// ch->path = Path::ToChar(IN_ROOM(ch), temp, 5, Path::Global | Path::ThruDoors);
				ROOMSEEKING(ch) = IN_ROOM(temp);
			}
		}
		
	}
	//	SOUNDS ?
	
}




void remember(Character * ch, Character * victim) {
	if (!IS_NPC(ch) || NO_STAFF_HASSLE(victim))
		return;
	
	if (!ch->mob->hates)
		ch->mob->hates = new Opinion;
	
	ch->mob->hates->AddChar(GET_ID(victim));
}


void forget(Character * ch, Character * victim) {
	if (!IS_NPC(ch) || !ch->mob->hates)
		return;
	
	ch->mob->hates->RemChar(GET_ID(victim));
}


void clearMemory(Character * ch) {
	if (!IS_NPC(ch) || !ch->mob->hates)
		return;
	ch->mob->hates->Clear();
}


/* do combat action for mob */
void mob_combat_action(Character *ch)
{
	Object *obj = NULL, *next_obj = NULL;

#ifdef GOT_RID_OF_IT
	if (MOB_FLAGGED(ch, MOB_SPEC) && GET_MOB_SPEC(ch) &&
			 (GET_MOB_SPEC(ch) (ch, ch, 0, "", GET_MOB_FARG(ch))))
		return;

	switch (GET_CLASS(ch)) {
//	case CLASS_MAGICIAN:
//		combat_action_magician(ch);
//		break;
//	case CLASS_ENCHANTER:
//		combat_action_enchanter(ch);
//		break;
	case CLASS_THIEF:
		combat_action_thief(ch);
		break;
	case CLASS_WARRIOR:
		combat_action_warrior(ch);
		break;
	default:
		break;
	}			
#endif
}
