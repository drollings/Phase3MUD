/***************************************************************************\
[*]    __     __  ___                ___  | LexiMUD is a C++ MUD codebase [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ |        for Sci-Fi MUDs        [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / | Copyright(C) 1997-1999        [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  | All rights reserved           [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   | Chris Jacobson (FearItself)   [*]
[*]      A Creation of the AvP Team!      | fear@technologist.com         [*]
[-]---------------------------------------+-------------------------------[-]
[*] File : affects.c++                                                    [*]
[*] Usage: Primary code for affections                                    [*]
\***************************************************************************/


#include "types.h"

#include "mud.base.h"
#include "characters.h"
#include "objects.h"
#include "utils.h"
#include "comm.h"
#include "affects.h"
#include "event.h"

void CheckRegenRates(Character *ch);


// CHANGEPOINT:  This is a temporary hack.
#define NoAffect 0

// char *affect_wear_off_msg[] = {
// 	"",					//	0
// 	"The whiteness in your vision slowly fades and you can see clearly.",
// 	"You begin to think for yourself.",
// 	"You slowly come to.",
// 	"You don't feel as sick.",
// 	"",					//	5
// 	"You don't feel as tired",
// 	"\n"
// };
// 
// 
// char *affects[] = {
// 	"NONE",				//	0
// 	"BLIND",
// 	"CHARM",
// 	"SLEEP",
// 	"POISON",
// 	"SNEAK",			//	5
// 	"TIRED",
// 	"\n"
// };


class AffectEvent : public ListedEvent {
public:
						AffectEvent(time_t when, GameData *what, Affect *aff) :
						ListedEvent(when, &(what->events)), thing(what), affect(aff) {}
					
	GameData			*thing;
	Affect				*affect;

	time_t					Execute(void);
};


time_t	AffectEvent::Execute(void)
{
	Affect *other;
	SInt32	type =	affect->AffType();
	
	affect->event = NULL;
	affect->Remove(thing);
	
	if (VALID_SKILL(type)) {
		LListIterator<Affect *>		iter(thing->affected);
		while ((other = iter.Next()))
			if ((other->type == type) && other->event)
				return 0;
// 		if (Skills[type].wearoff && (IN_ROOM(thing) != NOWHERE)) {
// 			thing->Send(Skills[type].wearoff);
// 			thing->Send("\r\n");	// CHANGEPOINT:  Either it's a sprintf and a Send, or
// 									// a Send and a Send.  Either way, we can't get by
// 									// with a single function unless we write Sendf
// 									// for GameData.
// 		}
	}
//		if (!affect->next || (affect->next->type != affect->type) || !affect->next->event)	// Compound Affects

	return 0;
}


Affect::Affect(SInt16 t, SInt32 mod, Location loc, Flags bv, Character * thecaster = NULL) :
		type(t), location(loc), modifier(mod), flags(bv), event(NULL) {
	if (VALID_SKILL(t))		type = t;
	caster = thecaster;
}


//	Copy affect and attach to char - a "type" of copy-constructor
Affect::Affect(const Affect &aff, GameData *thing, Character * thecaster = NULL) : type(aff.type), location(aff.location),
		modifier(aff.modifier), flags(aff.flags), event(NULL) {
	if (aff.event) {
		event = new AffectEvent(aff.event->Time(), thing, this);
	}
	caster = thecaster;
}


Affect::~Affect(void) {
	if (event)	event->Cancel();
	event = NULL;
	if (caster) {
		BreakConcentration();
		caster = NULL;
	}
}


//	Insert an Affect in a Character structure
//	Automatically sets apropriate bits and apply's
void Affect::ToThing(GameData * thing, UInt32 duration) {
	thing->affected.Add(this);

	if (duration > 0) {
		event = new AffectEvent(duration, thing, this);
	}

	thing->AffectModify(location, modifier, flags, true);
	thing->AffectTotal();
}


//	Remove an Affect from a char (called when duration reaches zero).
//	Frees mem and calls affect_modify
void Affect::Remove(GameData * thing) {
	thing->AffectModify(location, modifier, flags, false);
	thing->affected.Remove(this);
	thing->AffectTotal();
	delete this;
}


void Affect::Join(GameData * thing, UInt32 duration, bool add_dur, bool avg_dur, bool add_mod, bool avg_mod) {
	Affect *hjp;
	
	LListIterator<Affect *>		iter(thing->affected);
	while ((hjp = iter.Next())) {
		if ((hjp->type == type) && (hjp->location == location)) {
			if (add_dur && hjp->event)		duration += hjp->event->Time();
			if (avg_dur)					duration /= 2;
			if (add_mod)					modifier += hjp->modifier;
			if (avg_mod)					modifier /= 2;

			hjp->Remove(thing);
			ToThing(thing, duration);
			return;
		}
	}
	ToThing(thing, duration);
}


void Character::AffectModify(Affect::Location loc, SInt8 mod, Flags bitv, bool add) {
	if (add) {
		SET_BIT(affectbits, bitv);
	} else {
		REMOVE_BIT(affectbits, bitv);
		mod = -mod;
	}

	switch (loc) {
		case Affect::None:																		break;
		case Affect::Str:		GET_STAT(this, Str) = RANGE(0, GET_STAT(this, Str) + mod, 99);		break;
		case Affect::Int:		GET_STAT(this, Int) = RANGE(0, GET_STAT(this, Int) + mod, 99);		break;
		case Affect::Wis:		GET_STAT(this, Wis) = RANGE(0, GET_STAT(this, Wis) + mod, 99);		break;
		case Affect::Dex:		GET_STAT(this, Dex) = RANGE(0, GET_STAT(this, Dex) + mod, 99);		break;
		case Affect::Con:		GET_STAT(this, Con) = RANGE(0, GET_STAT(this, Con) + mod, 99);		break;
		case Affect::Cha:		GET_STAT(this, Cha) = RANGE(0, GET_STAT(this, Cha) + mod, 99);		break;
		
		case Affect::Weight:	GET_WEIGHT(this) = MAX(1, GET_WEIGHT(this) + mod);		break;
		case Affect::Height:	GET_HEIGHT(this) = MAX(1, GET_HEIGHT(this) + mod);		break;
		case Affect::Hit:
			GET_MAX_HIT(this) += mod;
			GET_HIT(this) = MIN(GET_HIT(this), GET_MAX_HIT(this));
			break;
		case Affect::Mana:
			GET_MAX_MANA(this) += mod;
			GET_MANA(this) = MIN(GET_MANA(this), GET_MAX_MANA(this));
			break;
		case Affect::Move:
			GET_MAX_MOVE(this) += mod;
			GET_MOVE(this) = MIN(GET_MOVE(this), GET_MAX_MOVE(this));
			break;
		case Affect::Gold:		GET_GOLD(this) = RANGE(-2000000000, GET_GOLD(this) + mod, 2000000000);		break;
		case Affect::Hitroll:	GET_HITROLL(this) += mod;		break;
		case Affect::Damroll:	GET_DAMROLL(this) += mod;		break;

		case Affect::PD:		GET_PD(this) += mod;	break;
		case Affect::DR:		GET_DR(this) += mod;	break;


		default:
//			log("SYSERR: affect_modify: Unknown apply adjust %d attempt", loc);
			break;
	}
}


void GameData::AffectModify(Affect::Location loc, SInt8 mod, Flags bitv, bool add) {
	if (add) {
		SET_BIT(affectbits, bitv);
	} else {
		REMOVE_BIT(affectbits, bitv);
		mod = -mod;
	}
}


void Object::AffectModify(Affect::Location loc, SInt8 mod, Flags bitv, bool add) {
	if (add) {
		SET_BIT(affectbits, bitv);
	} else {
		REMOVE_BIT(affectbits, bitv);
		mod = -mod;
	}
}


/* Call affect_remove with every spell of spelltype "skill" */
void Affect::FromThing(GameData * thing, SInt16 type) {
	Affect *hjp;
	bool removed = false;
	
	LListIterator<Affect *>		iter(thing->affected);
	while ((hjp = iter.Next())) {
		if (hjp->type == type) {
			hjp->Remove(thing);
			removed = true;
		}
	}
	
	if (removed) {
		if (Skills[type]->wearoff) {
			thing->Send(Skills[type]->wearoff);
			thing->Send("\r\n");
		}
		if (Skills[type]->wearoffroom)	thing->Echo(Skills[type]->wearoffroom);
	}
}


//	Return if a char is affected by something (Affect::XXX), NULL indicates
//	not affected
bool Affect::AffectedBy(GameData * thing, SInt16 type) {
	Affect *hjp;

	LListIterator<Affect *>		iter(thing->affected);
	while ((hjp = iter.Next()))
		if (hjp->type == type)
			return true;

	return false;
}


void	Affect::Modify(GameData * thing, bool add)
{
	thing->AffectModify(location, modifier, flags, add); 
}


void Affect::BreakConcentration()
{
	assert(caster);

	Concentration *temp_con;
	// bool itsbroken = true;		// Necessary for passing by reference

	START_ITER(iter, temp_con, Concentration *, caster->concentrations) {
		if (temp_con->affect == this) {
			// temp_con->Update(caster, itsbroken);
			caster->concentrations.Remove(temp_con);
			temp_con->target = NULL;	// Prevent recursive deletion
			delete temp_con;
		}
	}
}


