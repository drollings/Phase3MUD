/***************************************************************************\
[*]    __     __  ___                ___  | LexiMUD is a C++ MUD codebase [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ |        for Sci-Fi MUDs        [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / | Copyright(C) 1997-1999        [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  | All rights reserved           [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   | Chris Jacobson (FearItself)   [*]
[*]      A Creation of the AvP Team!      | fear@technologist.com         [*]
[-]---------------------------------------+-------------------------------[-]
[*] File : affects.h                                                      [*]
[*] Usage: Header for affections                                          [*]
\***************************************************************************/


#ifndef __AFFECTS_H__
#define __AFFECTS_H__


#include "mud.base.h"
#include "event.h"


class Character;
class GameData;

#define ACMD(name)	void (name)(Character *ch, char *argument, SInt32 cmd, const char *command, SInt32 subcmd)

class Affect {
public:
// 	enum Type { 
// 		NoAffect, 
// 		Blind, 
// 		Charm, 
// 		Sleep, 
// 		Poison, 
// 		Sneak, 
// 		Tired
// 	};	//	0-6
	
	//	Modifier constants used with obj affects ('A' fields)
	enum Location {
		None,
		Str, Int, Wis, Dex, Con, Cha,
		
		Weight, Height, Age,
		Hit, Mana, Move,

		PD, DR, Gold, 
		Hitroll, Damroll,
		
		SavingFire,
		SavingElectricity,
		SavingAcid,
		SavingFreezing,
		SavingPoison,
		SavingPsionic,
		SavingVim,
		SavingSpell,
		SavingPetri,
		MAX
	};
	
					Affect(void) : type(-1), location(None), modifier(0), flags(0),
								caster(NULL), event(NULL) { }
					Affect(SInt16 t, SInt32 mod, Location loc, Flags bv, Character * thecaster = NULL);
					Affect(const Affect &aff, GameData *thing, Character * thecaster = NULL);
					~Affect(void);
	
	void			Remove(GameData *thing);
	void			ToThing(GameData *thing, UInt32 duration);
	void			Join(GameData *thing, UInt32 duration, bool add_dur, bool avg_dur, bool add_mod, bool avg_mod);
	
	static void		FromThing(GameData *thing, SInt16 type);
	static bool		AffectedBy(GameData *thing, SInt16 type);
	// static void		Modify(Character * ch, Location loc, SInt8 mod, Flags bitv, bool add);
	void			Modify(GameData * thing, bool add);

	// static EVENTFUNC(AffEvent);
	
	inline SInt16	AffType(void)			  { return type;						  }
	inline Location	AffLocation(void)		  { return location;					  }
	inline SInt32 	AffModifier(void)		  { return modifier;					  }
	inline Flags	AffFlags(void)  		  { return flags;						  }
	
	inline SInt32	Time(void) const			{ return event ? event->Time() : 0;		}

    inline Character * Caster(void) const		{ return caster;						}
	void BreakConcentration(void);
	
protected:
	SInt16			type;			//	The type of spell that caused this
	Location		location;		//	Tells which ability to change(APPLY_XXX)
	SInt32			modifier;		//	This is added to apropriate ability
	Flags			flags;			//	Tells which bits to set (AFF_XXX)

    Character		*caster;		// The thing that caused this affect
	
	Event *			event;

	friend void mag_affects(int level, Character * ch, Character * victim,
					int spellnum, int savetype, int concentrate = 0);

	friend class Concentration;
	friend class AffectEvent;
	friend int do_break_concentration(Character *ch, int type);
	friend ACMD(do_concentrate);
};


// Used for concentration on spells or affects
class Concentration {
public:
	Concentration();
	~Concentration();
	void BreakConcentration(void);
	void Update(Character *ch, bool &broken);
    
    inline Affect * Affecting(void) const		{ return affect;						}
    inline GameData * Target(void) const		{ return target;						}

	void ToChar(Character *ch, Concentration *con, long interval);
	static void FromChar(Character *ch, Concentration *con);
	void Join(Character *ch, long interval, GameData *conc_target, Affect *aff);

protected:
	Affect *affect;  // This must match MAX_SPELL_AFFECTS in spells.h!
	GameData *target;

	Event *			event;

	friend class Affect;
	friend int do_break_concentration(Character *ch, int type);
	friend ACMD(do_concentrate);
	// friend EVENTFUNC(concentrate_event);
};


#if 0
class ConcentrateEvent : public EventData {
public:
	Concentration		* concentration;
	Character			* ch;
};
#endif

#endif

