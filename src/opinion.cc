/***************************************************************************\
[*]    __     __  ___                ___  | LexiMUD is a C++ MUD codebase [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ |        for Sci-Fi MUDs        [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / | Copyright(C) 1997-1999        [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  | All rights reserved           [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   | Chris Jacobson (FearItself)   [*]
[*]      A Creation of the AvP Team!      | fear@technologist.com         [*]
[-]---------------------------------------+-------------------------------[-]
[*] File : opinion.c++                                                    [*]
[*] Usage: Opinion module code                                            [*]
\***************************************************************************/


#include "types.h"

#include "opinion.h"
#include "characters.h"
#include "descriptors.h"
#include "rooms.h"
#include "index.h"
#include "utils.h"

void free_intlist(struct int_list *intlist);


Opinion::Opinion(void) : sex(0), race(0), vnum(-1), active(0) {
}


Opinion::Opinion(const Opinion &old) : sex(old.sex), race(old.race), vnum(old.vnum),
		active(old.active) {
}


Opinion::~Opinion(void) {
}


bool Opinion::RemChar(IDNum id) {
	bool temp = charlist.Remove(id);
	if (!charlist.Count())
		REMOVE_BIT(active, OP_CHAR);
	return temp;
}


void Opinion::Clear(void) {
	charlist.Clear();
	REMOVE_BIT(active, OP_CHAR);
}


bool Opinion::AddChar(IDNum id) {
	if (Contains(id))	return false;
	charlist.Add(id);
	SET_BIT(active, OP_CHAR);
	return true;
}


void Opinion::RemOther(Flags type) {
	switch (type) {
		case OP_CHAR:
			break;
		case OP_SEX:
			REMOVE_BIT(active, OP_SEX);
			sex = 0;
			break;
		case OP_RACE:
			REMOVE_BIT(active, OP_RACE);
			race = 0;
			break;
		case OP_VNUM:
			REMOVE_BIT(active, OP_VNUM);
			vnum = -1;
			break;
		default:
			log("SYSERR: Bad type (%d) passed to Opinion::RemOther.", type);
	}
}


void Opinion::AddOther(Flags type, UInt32 param) {
	switch (type) {
		case OP_CHAR:
			AddChar(param);
			break;
		case OP_SEX:
			SET_BIT(active, OP_SEX);
			sex = param;
			break;
		case OP_RACE:
			SET_BIT(active, OP_RACE);
			race = param;
			break;
		case OP_VNUM:
			SET_BIT(active, OP_VNUM);
			vnum = param;
			break;
		default:
			log("SYSERR: Bad type (%d) passed to Opinion::AddOther.", type);
	}
}


bool Opinion::IsTrue(Character *ch) {
	if (race && (ch->general.race != RACE_UNDEFINED)) {
		if ((1 << GET_RACE(ch)) & race)	return true;
	}
	if (sex) {
		if ((GET_SEX(ch) == Male) && IS_SET(sex, OP_MALE))		return true;
		if ((GET_SEX(ch) == Female) && IS_SET(sex, OP_FEMALE))	return true;
		if ((GET_SEX(ch) == Neutral) && IS_SET(sex, OP_NEUTRAL))return true;
	}
	if (vnum != -1) {
		if (ch->Virtual() == vnum)	return true;
	}
	if (Contains(GET_ID(ch)))	return true;
	return false;
}


Character *Opinion::FindTarget(Character *ch) {
	Character *victim;
	
	if (IN_ROOM(ch) == NOWHERE)
		return NULL;
	
	LListIterator<Character *>	people(world[IN_ROOM(ch)].people);
	while ((victim = people.Next())) {
		if ((ch != victim) && victim->CanBeSeen(ch) && IsTrue(victim))
			return victim;
	}
	return NULL;
}
