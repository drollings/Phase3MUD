/* ************************************************************************
*   File: vehicles.c                                    Part of CircleMUD *
*  Usage: Majority of vehicle-oriented code                               *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Vehicles.c is Copyright (C) 1997 Chris Jacobson                        *
*  CircleMUD is Copyright (C) 1993, 94 by the Trustees of the Johns       *
*  Hopkins University                                                     *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */


#include "characters.h"
#include "objects.h"
#include "rooms.h"
#include "utils.h"
#include "buffer.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "find.h"
#include "event.h"
#include "constants.h"

ACMD(do_drive);
ACMD(do_mount);
ACMD(do_unmount);

ACMD(do_look);
void vehicle_move(Object *vehicle, VNum to);
int sit_otrigger(Object *obj, Character *actor);


ACMD(do_turn);
ACMD(do_accelerate);


class DriveEvent : public Event {
public:
						DriveEvent(time_t when, Character *tch, Object *tvehicle, Object *tcontrols, SInt8 dir, SInt8 spd) :
						Event(when), ch(tch), vehicle(tvehicle), controls(tcontrols), direction(dir), speed(spd) {}
					
	SafePtr<Character>		ch;
	SafePtr<Object>			vehicle;
	SafePtr<Object>			controls;
	SInt8		direction;
	SInt8		speed;

	time_t					Execute(void);
};


time_t	DriveEvent::Execute(void)
{
	if (!ch) 		return 0;
	
	if (!controls || !vehicle) {	//	Event died.
		return 0;
	}
		
	if (IN_ROOM(ch) != IN_ROOM(controls)) {
		return 0;	//	They left, the loser.
	}
		
	if (IS_MOUNTABLE(controls))
		vehicle = controls;
	
	if (!vehicle && !(vehicle = find_vehicle_by_vnum(GET_OBJ_VAL(controls, 0)))) {
		return 0;	//	Vehicle disappeared!
	}
	
	if (EXIT_FLAGGED(EXIT(vehicle, direction), Exit::Closed | Exit::NoVehicles)) {
		//	CRASH
		act("&cRThe $p SLAMS into the wall, throwing you all around!&c0", false, ch(), vehicle(), 0, TO_ROOM);
		act("&cRThe $p SLAMS into the wall!&c0", false, 0, vehicle(), 0, TO_ROOM);
		// GET_DRIVE_SPEED(ch) = 0;
		return 0;
	}
	
	// DO THE MOVE
	do_drive(ch(), (char *) dirs[direction], 0, "drive", 0);
	
	if (EXIT_FLAGGED(EXIT(vehicle, direction), Exit::Closed | Exit::NoVehicles))
		ch->Send("&cWYou are about to crash!&c0\r\n");		//	WARN OF VERY NEAR DOOM
	else if (!EXITN(EXIT(vehicle, direction)->to_room, direction) ||
			EXIT_FLAGGED(EXITN(EXIT(vehicle, direction)->to_room, direction), Exit::Closed | Exit::NoVehicles))
		ch->Send("You are approaching a wall!&c0\r\n");	//	WARN OF IMPENDING DOOM
	
	speed = 5;

//	CREATE(eventObj, DriveEvent, 1);
//	eventObj->ch = GET_ID(ch);
//	eventObj->vehicle = vehicle ? GET_ID(vehicle) : 0;
//	eventObj->controls = GET_ID(controls);
//	eventObj->direction = ((DriveEvent *)event_obj)->direction;
//	eventObj->speed = ((DriveEvent *)event_obj)->speed;
	
//	ch->events = new Event(drive_event, eventObj, drive_speed, ch->events);
	return (speed RL_SEC);
}


#define	VEHICLE_FLAGGED(vehicle, flag)	(OBJVAL_FLAGGED(vehicle, flag))
#define	VEHICLE_GROUND			(1 << 0)
#define VEHICLE_AIR				(1 << 1)
#define VEHICLE_SPACE			(1 << 2)
#define VEHICLE_INTERSTELLAR	(1 << 3)
#define VEHICLE_WATER			(1 << 4)
#define VEHICLE_SUBMERGE		(1 << 5)

#define ROOM_SECT(room)			(world[room].SectorType())

#define DRIVE_NOT		0
#define DRIVE_CONTROL	1
#define DRIVE_MOUNT		2

ACMD(do_drive) {
	int dir, drive_mode = DRIVE_NOT, i;
	Object *vehicle, *controls = NULL, *vehicle_in_out;
	char *arg1;
	
	if (ch->SittingOn() && IS_MOUNTABLE(ch->SittingOn()))
		drive_mode = DRIVE_MOUNT;
	else {
		drive_mode = DRIVE_CONTROL;
		if (!(controls = get_obj_in_list_type(ITEM_V_CONTROLS, world[IN_ROOM(ch)].contents)))
			if (!(controls = get_obj_in_list_type(ITEM_V_CONTROLS, ch->contents)))
				for (i = 0; i < NUM_WEARS; i++)
					if (GET_EQ(ch, i) && GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_V_CONTROLS) {
						controls = GET_EQ(ch, i);
						break;
					}
				
		if (!controls) {
			ch->Send("You can't do that here.\r\n");
			return;
		}
		if (!ch->CanUse(controls)) {
			ch->Send("You can't figure out the controls.\r\n");
			return;
		}
		if (GET_POS(ch) > POS_SITTING) {
			ch->Send("You must be sitting to drive!\r\n");
			return;
		}
	}
	
	if (drive_mode == DRIVE_CONTROL) {
		vehicle = find_vehicle_by_vnum(GET_OBJ_VAL(controls, 0));
		if (!vehicle || (GET_OBJ_TYPE(vehicle) != ITEM_VEHICLE)) {
			ch->Send("The controls seem to be broken.\r\n");
			return;
		}
	} else if (!(vehicle = (ch->SittingOn())) || (IN_ROOM(vehicle) == NOWHERE)) {
		ch->Send("The vehicle exists yet it doesn't.  Sorry, you're screwed...\r\n");
		return;
	}
	if (AFF_FLAGGED(ch, AffectBit::Blind)) {
		/* Blind characters can't drive! */
		ch->Send("You can't see the controls!\r\n");
		return;
	}
	
	arg1 = get_buffer(MAX_INPUT_LENGTH);
	argument = one_argument(argument, arg1);
	
	/* Gotta give us a direction... */
	if (!*arg1)
		ch->Send("Drive which direction?\r\n");
	else if (is_abbrev(arg1, "into")) {	// Driving Into another Vehicle
		int is_going_to;
		
		one_argument(argument, arg1);
		
		if (!*arg1)
			ch->Send("Drive into what?\r\n");
		else if (!(vehicle_in_out = get_obj_in_list_vis(ch, arg1, world[IN_ROOM(vehicle)].contents)))
			ch->Send("Nothing here by that name!\r\n");
		else if (GET_OBJ_TYPE(vehicle_in_out) != ITEM_VEHICLE)
			ch->Send("Thats not a vehicle.\r\n");
		else if(vehicle == vehicle_in_out)
			ch->Send("Very funny...\r\n");
		else if (IS_MOUNTABLE(vehicle_in_out))
			ch->Send("Thats a mountable vehicle, not one you can drive into!\r\n");
		else if (GET_OBJ_VAL(vehicle_in_out, 0) == NOWHERE)
			ch->Send("Thats not a vehicle.\r\n");
		else if ((GET_OBJ_VAL(vehicle_in_out, 4) / 2) < GET_OBJ_VAL(vehicle, 4))
			act("$p is too large to drive into $P.", TRUE, ch, vehicle, vehicle_in_out, TO_CHAR);
		else if (!Room::Find(is_going_to = GET_OBJ_VAL(vehicle_in_out, 0)))
//				|| !ROOM_FLAGGED(is_going_to, ROOM_VEHICLE))
			ch->Send("That vehicle can't carry other vehicles.\r\n");
		else if(drive_mode == DRIVE_MOUNT) {
			act("$n drives $p into $P.", TRUE, ch, vehicle, vehicle_in_out, TO_ROOM);
			act("You drive $p into $P.", TRUE, ch, vehicle, vehicle_in_out, TO_CHAR);
			
			vehicle_move(vehicle, is_going_to);
			
			act("$n drives $p in.", TRUE, ch, vehicle, 0, TO_ROOM);
			
//			look_at_room(ch, 0);
		} else {
			act("You drive into $P", TRUE, ch, 0, vehicle_in_out, TO_CHAR);
			act("$n drives into $P.", TRUE, ch, 0, vehicle_in_out, TO_ROOM);
			act("$p enters $P.\r\n", TRUE, 0, vehicle, vehicle_in_out, TO_ROOM);

			vehicle_move(vehicle, is_going_to);
			
			act("$p enters.", FALSE, 0, vehicle, 0, TO_ROOM);
			
			look_at_rnum(ch, IN_ROOM(vehicle), FALSE);
		}
	} else if (is_abbrev(arg1, "out")) {
		Object *hatch;
		
		if (!(hatch = get_obj_in_list_type(ITEM_V_HATCH, world[IN_ROOM(vehicle)].contents)))
			ch->Send("Nowhere to drive out of.\r\n");
		else if (!(vehicle_in_out = find_vehicle_by_vnum(GET_OBJ_VAL(hatch, 0))) ||
				(IN_ROOM(vehicle_in_out) == NOWHERE))
			ch->Send("ERROR!  Vehicle to drive out of doesn't exist!\r\n");
		else if (drive_mode == DRIVE_MOUNT) {
			act("$n drives $p out.", TRUE, ch, vehicle, 0, TO_ROOM);
			act("You drive $p out of $P.", TRUE, ch, vehicle, vehicle_in_out, TO_CHAR);

			vehicle_move(vehicle, IN_ROOM(vehicle_in_out));
			
			act("$n drives $p out of $P.", TRUE, ch, vehicle, vehicle_in_out, TO_ROOM);
			look_at_room(ch, 0);
		} else {
			act("$p exits $P.", TRUE, 0, vehicle, vehicle_in_out, TO_ROOM);

			vehicle_move(vehicle, IN_ROOM(vehicle_in_out));
			
			act("$p drives out of $P.", TRUE, 0, vehicle, vehicle_in_out, TO_ROOM);
			act("$n drives out of $P.", TRUE, ch, 0, vehicle_in_out, TO_ROOM);
			act("You drive $p out of $P.", TRUE, ch, vehicle, vehicle_in_out, TO_CHAR);
			
			look_at_rnum(ch, IN_ROOM(vehicle), FALSE);
		}
	} else if ((dir = search_block(arg1, (const char **)dirs, FALSE)) >= 0) {	// Drive in a direction...
		/* Ok we found the direction! */
		if (!ch || (dir < 0) || (dir >= NUM_OF_DIRS) || (IN_ROOM(vehicle) == NOWHERE))
			/* But something is invalid */
			ch->Send("Sorry, an internal error has occurred.\r\n");
		else if (!CAN_GO(vehicle, dir))	// But there is no exit that way */
			ch->Send("Alas, you cannot go that way...\r\n");
		else if (EXIT_FLAGGED(EXIT(vehicle, dir), Exit::Closed)) { // But the door is closed
			if (EXIT(vehicle, dir)->Keyword())
				act("The $F seems to be closed.", FALSE, ch, 0, const_cast<char *>(EXIT(vehicle, dir)->Keyword()), TO_CHAR);
			else
				act("It seems to be closed.", FALSE, ch, 0, 0, TO_CHAR);
//		} else if (!ROOM_FLAGGED(EXIT(vehicle, dir)->to_room, ROOM_VEHICLE) ||
//				EXIT_FLAGGED(EXIT(vehicle, dir), Exit::NoVehicles))	// But the vehicle can't go that way */
//			ch->Send("The vehicle can't manage that terrain.\r\n");
		} else if (ROOM_FLAGGED(EXIT(vehicle, dir)->to_room, ROOM_NOVEHICLE) ||
				EXIT_FLAGGED(EXIT(vehicle, dir), Exit::NoVehicles))	// But the vehicle can't go that way */
			ch->Send("The vehicle can't manage that terrain.\r\n");
		else if (ROOM_SECT(EXIT(vehicle, dir)->to_room) == SECT_FLYING &&
				!VEHICLE_FLAGGED(vehicle, VEHICLE_AIR)) {
			/* This vehicle isn't a flying vehicle */
			if (ROOM_SECT(IN_ROOM(vehicle)) == SECT_SPACE)
				act("The $o can't enter the atmosphere.", FALSE, ch, vehicle, 0, TO_CHAR);
			else
				act("The $o can't fly.", FALSE, ch, vehicle, 0, TO_CHAR);
		} else if ((ROOM_SECT(EXIT(vehicle, dir)->to_room) == SECT_WATER_NOSWIM) &&
				!VEHICLE_FLAGGED(vehicle, VEHICLE_WATER | VEHICLE_SUBMERGE))
			/* This vehicle isn't a water vehicle */
			act("The $o can't go in such deep water!", FALSE, ch, vehicle, 0, TO_CHAR);
		else if ((ROOM_SECT(EXIT(vehicle, dir)->to_room) == SECT_UNDERWATER) &&
				!VEHICLE_FLAGGED(vehicle, VEHICLE_SUBMERGE))
			act("The $o can't go underwater!", FALSE, ch, vehicle, 0, TO_CHAR);
		else if (ROOM_SECT(EXIT(vehicle, dir)->to_room) == SECT_DEEPSPACE &&
					!VEHICLE_FLAGGED(vehicle, VEHICLE_INTERSTELLAR))
			/* This vehicle isn't a space vehicle */
			act("The $o can't go into space!", FALSE, ch, vehicle, 0, TO_CHAR);
		else if (ROOM_SECT(EXIT(vehicle, dir)->to_room) == SECT_SPACE &&
				!VEHICLE_FLAGGED(vehicle, VEHICLE_SPACE))
			act("The $o cannot handle space travel.", FALSE, ch, vehicle, 0, TO_CHAR);
		else if ((ROOM_SECT(EXIT(vehicle, dir)->to_room) > SECT_FIELD) &&
				(ROOM_SECT(EXIT(vehicle, dir)->to_room) <= SECT_MOUNTAIN) &&
				!VEHICLE_FLAGGED(vehicle, VEHICLE_GROUND))
			act("The $o cannot land on that terrain.", FALSE, ch, vehicle, 0, TO_CHAR);
		else {	//	But nothing!  Let's go that way!			
			if(drive_mode == DRIVE_MOUNT)
				act("$n drives $p $T.", TRUE, ch, vehicle, dirs[dir], TO_ROOM);
			else
				act("$p leaves $T.", FALSE, 0, vehicle, dirs[dir], TO_ROOM);

			vehicle_move(vehicle, world[IN_ROOM(vehicle)].Direction(dir)->to_room);
			
			if (drive_mode == DRIVE_MOUNT) {
				act("You drive $p $T.", TRUE, ch, vehicle, dirs[dir], TO_CHAR);
				act("$n enters from $T on $p.", TRUE, ch, vehicle,
						dir_text[rev_dir[dir]], TO_ROOM);
				if (ch->desc != NULL)
					look_at_room(ch, 0);
			} else {
				act("$p enters from $T.", FALSE, 0, vehicle, dir_text[rev_dir[dir]], TO_ROOM);
				act("$n drives $T.", TRUE, ch, 0, dirs[dir], TO_ROOM);
				act("You drive $T.", TRUE, ch, 0, dirs[dir], TO_CHAR);
				if (ch->desc != NULL)
					look_at_rnum(ch, IN_ROOM(vehicle), FALSE);
			}
		}
	} else
		ch->Send("Thats not a valid direction.\r\n");
	release_buffer(arg1);
}




void vehicle_move(Object *vehicle, VNum to) {
	VNum	prev_in;
	Character *tch;
	LListIterator<Character *>	iter;
	
	prev_in = IN_ROOM(vehicle);
	
	// First things first... move all the riders to the room
	// Otherwise, vehicle->FromRoom will set everyone who was
	// sitting on the object, as sitting on NULL
	iter.Start(world[prev_in].people);
	while ((tch = iter.Next())) {
		if (tch->SittingOn() == vehicle) {
			tch->FromRoom();	// This forgets the sitting object
			tch->ToRoom(to);
			tch->SetMount(vehicle);
		}
	}
	
	vehicle->FromRoom();
	vehicle->ToRoom(to);
	
	iter.Restart(world[to].people);
	while ((tch = iter.Next())) {
		if (tch->SittingOn() == vehicle)
			look_at_rnum(tch, to, FALSE);
	}
}

#if 0
UInt8 turn_dir[4][2] = {
//  L R 
{WEST,EAST},	 //	NORTH
{NORTH,SOUTH},	//	EAST
{EAST,WEST},	//	SOUTH
{SOUTH,NORTH}	//	WEST
};

int temp;
#define GET_DRIVE_DIR(ch)	(temp)
#define GET_DRIVE_SPEED(ch)	(temp)

char *turn_dir_names[] = {
"north",
"east",
"south",
"west",
"up",
"down",
"left",
"right"
};

#define LEFT	6
#define	RIGHT	7

ACMD(do_turn) {
	UInt32	dir;
	char *arg;
	Object *controls = NULL, *vehicle = NULL;
//	UInt32	i;
	struct event_info *event;
	
/*
	if (!ch->sitting_on || !IS_MOUNTABLE(ch->sitting_on)) {
		if (!(controls = get_obj_in_list_type(ITEM_V_CONTROLS, world[IN_ROOM(ch)].contents)))
			if (!(controls = get_obj_in_list_type(ITEM_V_CONTROLS, ch->contents)))
				for (i = 0; i < NUM_WEARS; i++)
					if (GET_EQ(ch, i) && GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_V_CONTROLS) {
						controls = GET_EQ(ch, i);
						break;
					}
		if (controls)
			vehicle = find_vehicle_by_vnum(GET_OBJ_VAL(controls, 0));
		else {
			ch->Send("Turn what?\r\n");
			return;
		}
	} else
		vehicle = ch->sitting_on;

	if (!vehicle) {
		ch->Send("The vehicle mysteriously has disappeared...\r\n");
		return;
	}
*/

	if (!(event = get_event(ch, typeid(DriveEvent), EVENT_CAUSER))) {
		ch->Send("You aren't driving anything.\r\n");
		return;
	}

	arg = get_buffer(MAX_INPUT_LENGTH);
	
	one_argument(argument, arg);

	if (!*arg)
		ch->Send("Turn which way?\r\n");
	else if ((dir = search_block(arg, (const char **)turn_dir_names, FALSE)) < 0)
		ch->Send("Turn WHICH way?");
	else if (GET_DRIVE_DIR(ch) == UP) {
		if (dir > WEST)
			ch->Send("You're going UP, you can't turn like that.\r\n");
		else {
			// TURN AND LEVEL OUT IN THE DIRECTION
			act("You turn the $p to the $T.", TRUE, ch, vehicle, turn_dir_names[dir], TO_CHAR);
			act("$n turns the $p to the $T.", TRUE, ch, vehicle, turn_dir_names[dir], TO_ROOM);
			GET_DRIVE_DIR(ch) = dir;
		}	
	} else if (GET_DRIVE_DIR(ch) == DOWN) {
		if (dir > WEST)
			ch->Send("You're going DOWN, you can't turn like that.\r\n");
		else {
			// TURN AND LEVEL OUT IN THE DIRECTION
			act("You turn the $p to the $T.", TRUE, ch, vehicle, turn_dir_names[dir], TO_CHAR);
			act("$n turns the $p to the $T.", TRUE, ch, vehicle, turn_dir_names[dir], TO_ROOM);
			GET_DRIVE_DIR(ch) = dir;
		}
	} else {
		if ((dir < LEFT) && (dir != turn_dir[GET_DRIVE_DIR(ch)][0]) &&
				(dir != turn_dir[GET_DRIVE_DIR(ch)][1]))
			ch->Send("You can't turn more than 90 degrees at one time!\r\n");
		else {
			act("You turn the $p to the $T.", TRUE, ch, vehicle, turn_dir_names[dir], TO_CHAR);
			act("$n turns the $p to the $T.", TRUE, ch, vehicle, turn_dir_names[dir], TO_ROOM);
			GET_DRIVE_DIR(ch) = dir;
		}
	}
	release_buffer(arg);
}


ACMD(do_accelerate) {
	
}


#endif
