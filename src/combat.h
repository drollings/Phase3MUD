/***************************************************************************\
[*]    __     __  ___                ___  | LexiMUD is a C++ MUD codebase [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ |        for Sci-Fi MUDs        [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / | Copyright(C) 1997-1999        [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  | All rights reserved           [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   | Chris Jacobson (FearItself)   [*]
[*]      A Creation of the AvP Team!      | fear@technologist.com         [*]
[-]---------------------------------------+-------------------------------[-]
[*] File : combat.h                                                       [*]
[*] Usage: Combat Engine                                                  [*]
\***************************************************************************/


#ifndef __COMBAT_H__
#define __COMBAT_H__


#include "types.h"

#include "skills.h"
#include "stl.llist.h"
#include "event.h"


class SkillStore;
class Character;
class Object;
class Attack;
class MUDObject;

extern SInt8 find_hit_location(Character *victim, SInt8 modifier);
extern int is_ammo_attack(int proficiency);
extern int fire_missile(Character * ch, Character * victim, Attack * attack, int dir);
extern void combat_round(Character *ch, Character *victim);
// extern int skill_message(int dam, Character * ch, Character * vict,
// 			int attacktype, SInt8 hitlocation = -1);
extern int in_melee_range(Character * ch, Character * victim, int attacktype);
extern Character * check_for_intercept(Character * ch, Character * victim, Attack *attack);
extern void armor_message(int dam, Character * ch, Character * victim, int w_type, SInt8 hitlocation);



namespace CombatMessages {
	class Message {
	public:
							Message(void) : attacker(NULL), victim(NULL), room(NULL) { }
							~Message(void);
					
		void				Set(FILE * fl, int i);
		inline const char *	Attacker(void) const	{ return attacker;	}
		inline const char *	Victim(void) const		{ return victim;	}
		inline const char *	Room(void) const		{ return room;		}
		
		void				Send(Character *ch, Character *vict, MUDObject *weap) const;
		void				SendVar(Character *ch, Character *vict, MUDObject *weap, 
									char *damsingular, char *damplural,
									char *attackadj, char *attackadverb,
									char *defenseadj, char *defenseadverb, char *hitlocation,
									char *attacktype) const;
	protected:
		char *				attacker;
		char *				victim;
		char *				room;

	};

	class MessageType {
	public:
		Message				die;
		Message				miss;
		Message				hit;
		Message				staff;
	};
	class MessageList {
	public:
							MessageList(void) : attack(0) { };
		SInt32				attack;
		LList<MessageType *>Messages;
	};
	
	void				Load(const char *filename);

	MessageList			*Find(SInt32 attack);

	extern LList<MessageList *>Messages;
}


namespace Combat {
	void			Load(void);
	
	bool			Valid(Character *ch, Character *vict);
	void			Check(Character *ch, Character *vict);
	void			CheckFighting(Character *ch, Character *vict);
	int 			DamageMessage(int dam, Character * ch, Character * victim, int w_type);
	int 			AttackMessage(Character *ch, Character *victim, Attack *attack = NULL, SInt16 defense = -1);
	int 			SkillMessage(int dam, Character *ch, Character *vict, int attacktype, Attack *attack = NULL);
	void			Explosion(SInt32 ownerId, UInt32 diceNumber, UInt32 diceSize, VNum room, bool shrapnel);
	void 			SendAttack(Character *ch, Character *victim, Attack *attack, SInt16 defensemove = -1);

	void			MakeCorpse(Character * ch);
	void			DeathCry(Character * ch);
	void			Announce(const char *buf);
	UInt32			CalcReward(Character *ch, Character *victim);
//	void			GroupGain(Character *ch, Character *victim);
	void			MobReaction(Character *attacker, Character *mob, SInt32 dir);
	
//	SInt32			Delay(Character *ch);
//					EVENTFUNC(AttackEvent);

}


namespace Ranged {
#ifdef GOT_RID_OF_IT
	Character *		FindVictimDirection(Character *attacker, const char *victimArg,
							SInt32 direction, SInt32 &range);
	Character *		FindVictim(Character *attacker, const char *victimArg,
							SInt32 &direction, SInt32 &range);
	SInt32			FireMissile(Character *attacker, Object *weapon, Character *victim,
							SInt32 direction, SInt32 range, SInt32 attackType);
#endif
	Character *		GrabRangedTarget(Character *attacker, char *arg1, SInt32 dir, SInt32 range, SInt32 &exception);
	SInt32 			FireMissile(Character *attacker, Character * victim, Attack * attack, SInt32 dir);
	Character * 	AcquireTarget(Character * ch, char *argument, SInt32 &dir, SInt32 &exception);
	SInt32 			DistanceToTarget(Character * ch, Character * victim, SInt32 range, SInt32 dir);
	Object * 		GrabMissile(Character * ch, Object * weapon);
}


namespace Damage {
	enum {
		Undefined = 0,
		Basic,
		Slashing,
		Piercing,
		Fire,
		Electricity,
		Acid,
		Freezing,
		Poison,
		Psionic,
		Vim,
		Explosion,
		Fall,
		Suffering,
		NumTypes
	};
	
	class Result {
	public:
							Result(void) : damage(0), critical(0) { }
		UInt8				damage;
		UInt8				critical;
	};
	typedef Result		Chart[20][50];
	extern Chart		charts[NumTypes];
	extern const char *	types[];
	extern const char *	messages[];
}


// If 0 on the skill and offensive, it is a weapon attack.
// If both offensive and defensive, adding 1 to the message value 
// should give an offensive message.

// Format: move, skillnum, message, basecost, location modifier, type, 
//         command, subcmd

#define IS_WEAPON(type) (((type) >= FIRST_COMBAT_MESSAGE) && ((type) < LAST_COMBAT_MESSAGE))

extern const struct combat_profile_moves combat_moves[];

namespace Combat {
	enum {
		Undefined,
		Hit,
		SwingRight,
		ThrustRight,
		ParryRight,
		SwingLeft,
		ThrustLeft,
		ParryLeft,
		SwingThird,
		ThrustThird,
		ParryThird,
		SwingFourth,
		ThrustFourth,
		ParryFourth,
		Punch,
		Kick,
		Bash,
		Circle,
		Headbutt,
		Flip,
		Bite,
		IronFist,
		Uppercut,
		Disarm,
		KarateChop,
		KarateKick,
		Bladesong,
		Feint,
		RiposteRight,
		RiposteLeft,
		RiposteThird,
		RiposteFourth,
		Dodge,
		Block,
		Backstab,
		ShieldBash,
		AutoAttack,
		AutoDefend,
	};
}

// A "package" to be handed down to hit() containing all necessary information
// for an attack.
class Attack {
public:
	Object	*weapon;		// Weapon used

	SInt16	move,			// What combat move is being performed
			skillnum,		// Skill number to be rolled against for success
			message,		// Message number to use
			dam;			// Damage plus

	SInt8	chance,			// Chance on 3d6 of the attack succeeding
			mod,			// Modifier to skillroll
			speed,			// Cost in Combat Points
			damtype,		// Damage type (basic, slashing, piercing)
			location,		// Body location to be struck
			hand,			// Which hand is being used.	
							// Will be useful for dual wielding.
			response,		// Indicates success of defense move.
			succeed_by;		// Modified by hit(), indicating success.
	 
	Attack() : weapon(NULL), move(0), skillnum(0), message(0), dam(0), 
			   chance(SKILL_NOCHANCE), mod(0), speed(5),
			   damtype(Damage::Basic), location(-1), hand(0),
			   response(SKILL_NOCHANCE), succeed_by(SKILL_NOCHANCE) { }
	~Attack() { }

	int WeaponMove(Character *ch, SInt16 do_move);

};


// #define NUM_COMBAT_MOVES 	7 // Number of elements in combat_moves struct
#define COMBAT_PROFILE_SIZE 	7 // Number of elements in combat profile array
#define COMBAT_DEFENSIVE	0
#define COMBAT_OFFENSIVE	1
#define COMBAT_BOTH		2


// Daniel Rollings AKA Daniel Houghton's combat profile
class CombatProfile {
private:
	SInt8 attack_offset, defense_offset;
	SInt8 posture;			// Normal, defense, or offense?
    UInt8 moves[COMBAT_PROFILE_SIZE + 1];

public:
	// This is a minor violation of object-oriented methodology, but rather
	// than get religious about it, let's do what works.	A lot of functions
	// will be wanting to access these variables, and we don't want to muck
	// with friend classes just for this, do we? -DH
	SInt8 attack_pts, defense_pts;

	CombatProfile();
	~CombatProfile();
		
	int GetMove(UInt8 offset);
	int SetMove(SInt16 &move, SInt8 &offset);
	int SetPosture(char *string);
	void NextMove(SInt8 &move_offset);
	void NextOffense();
	void NextDefense();
	int CombatMove(SInt8 type);
	void CombatCost(Character *ch, SInt8 type, SInt16 cost);
	void PointUpdate(Character *ch);
	void PointUpdate(int increase);
	void ZeroPoints(SInt8 type = COMBAT_BOTH);
	void ProfileDisplay(Character *ch);
	void Read(char *string);
	void Save(FILE *outfile);

	friend void do_circle(Character *ch, char *argument, SInt32 cmd, const char *command, int subcmd);
	friend void combat_round(Character *ch, Character *victim);

};
#endif

