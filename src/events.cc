/***************************************************************************\
[*]    __     __  ___                ___  | LexiMUD is a C++ MUD codebase [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ |        for Sci-Fi MUDs        [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / | Copyright(C) 1997-1999        [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  | All rights reserved           [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   | Chris Jacobson (FearItself)   [*]
[*]      A Creation of the AvP Team!      | fear@technologist.com         [*]
[-]---------------------------------------+-------------------------------[-]
[*] File : events.cp                                                      [*]
[*] Usage: Events used by the Events Services                             [*]
\***************************************************************************/


#include "structs.h"
#include "mud.base.h"
#include "utils.h"
#include "find.h"
#include "comm.h"
#include "handler.h"
#include "event.h"
#include "constants.h"
#include "combat.h"
#include "eventdefines.h"
#include "property.h"

int greet_mtrigger(Character *actor, int dir);
int entry_mtrigger(Character *ch);
int enter_wtrigger(Room *room, Character *actor, int dir);
int greet_otrigger(Character *actor, int dir);
int leave_mtrigger(Character *actor, int dir);
int leave_otrigger(Character *actor, int dir);
int leave_wtrigger(Room *room, Character *actor, int dir);


/*
EVENT(room_fire) {
	Character *tch, next_tch;
	int room = info;
	
	if (!world[room].people)
		return;
	
	act("The room is on fire!", FALSE, world[room].people, 0, 0, TO_ROOM);
	
	for (tch = world[room].people; tch; tch = tch->next_tch) {
		next_tch = tch->next_in_room;
	}
}

*/

// CHANGEPOINT:  This really ought to go elsewhere.
void Scriptable::EventCleanup(void) {
	EventList::iterator	event;

	event = events.begin();
	while (event != events.end()) {
		(*event)->Cancel();
		event = events.begin();
	}
}


