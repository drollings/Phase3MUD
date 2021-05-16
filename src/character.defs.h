/*************************************************************************
*   File: character.defs.h           Part of Aliens vs Predator: The MUD *
*  Usage: Header for character-related defines                           *
*************************************************************************/

#ifndef __CHARACTER_DEFS_H__
#define __CHARACTER_DEFS_H__


#include "types.h"


class Character;


#define NOBODY				-1    /* nil reference for mobiles		*/

enum Race {
	RACE_UNDEFINED	= -1,
	RACE_ANIMAL	= 0,
	RACE_HYUMANN,
	RACE_ELF,
	RACE_DWOMAX,
	RACE_SHASTICK,
	RACE_MURRASH,
	RACE_LAORI,
	RACE_THADYRI,
	RACE_TERRAN,
	RACE_TRULOR,
	RACE_DRAGON,
	RACE_DEVA,
	RACE_SOLAR,
	RACE_INSECT,
	RACE_FISH,
	RACE_AMPHIBIAN,
	RACE_FELINE,
	RACE_CANINE,
	RACE_EQUINE,
	RACE_BIRD,
	RACE_GOBLIN,
	RACE_ORC,
	RACE_GITHYANKI,
	NUM_RACES
};


enum Class {
	CLASS_WRONGRACE = -1,
	CLASS_UNDEFINED = 0,
	CLASS_MAGICIAN,
	CLASS_CLERIC,
	CLASS_THIEF,
	CLASS_WARRIOR,
	CLASS_RANGER,
	CLASS_ASSASSIN,
	CLASS_ALCHEMIST,
	CLASS_PSIONICIST,
	CLASS_INSECT,
	CLASS_MAMMAL,
	CLASS_AVIAN,
	CLASS_FISH,
	CLASS_REPTILE,
	CLASS_ZOMBIE,
	CLASS_GHOUL,
	CLASS_GHOST,
	CLASS_POLTERGEIST,
	CLASS_WRAITH,
	CLASS_SPECTRE,
	CLASS_VAMPIRE,
	NUM_CLASSES
};

//	Sex
enum Sex {
	Neutral,
	Male,
	Female
};


//	(Im)mortality
enum Mortality {
	MORTALITY_UNDEFINED = -1,
	MORTALITY_MORTAL = 0,
	MORTALITY_IMMORTAL,
	MORTALITY_DEITY,
	MORTALITY_STAFF,
	NUM_MORTALITIES
};


#ifdef GOT_RID_OF_IT
//	Positions
enum Position {
	POS_PRONE = 0,			//	sleeping
	POS_SITTING,			//	sitting
	POS_STANDING,			//	standing
	POS_RIDING,
	POS_FLYING
};
#else
//	Positions
enum Position {
	POS_DEAD = 0,			//	dead
	POS_MORTALLYW,			//	mortally wounded
	POS_INCAP,				//	incapacitated
	POS_STUNNED,			//	stunned
	POS_SLEEPING,			//	sleeping
	POS_RESTING,			//	resting
	POS_SITTING,			//	sitting
	POS_FIGHTING,			//	fighting
	POS_STANDING,			//	standing
	POS_RIDING,				//	DO NOT USE
	POS_AUDIBLE,			//	DO NOT USE
	POS_MOBILE				//	DO NOT USE
};
#endif


#define HIT_INCAP      -3     /* The hit level for incapacitation   */
#define HIT_MORTALLYW  -6     /* The hit level for mortally wound   */
#define HIT_DEAD       -11	/* The point you never want to get to */


//	Positions
enum CharState {
	STATE_DEAD = 0,			//	dead
	STATE_MORTALLYW,		//	mortally wounded
	STATE_INCAP,			//	incapacitated
	STATE_STUNNED,			//	stunned
	STATE_FROZEN,			//	stunned
	STATE_PARALYZED,		//	stunned
	STATE_SLEEPING,			//	sleeping
	STATE_RESTING,			//	resting
	STATE_AWAKE,			//	resting
	STATE_FIGHTING,			//	fighting
    STATE_AUDIBLE			//  audible
};


/* mount check types */
enum MountChecks {
	MNT_MOUNT,		/* Mount attempt      */
	MNT_DISMOUNT,	/* Dismount attempt   */
	MNT_COMBAT,		/* combat check to stay on */
	MNT_TOROOM,		/* char in different room  */
	MNT_MOVE,		/* trying to move on mount */
	MNT_EXTRACT,	/* char or mount extracted */
	MNT_FLEE,		/* mount fled          */
};


enum Bodytype {
	BODYTYPE_HUMANOID	= 0,
	BODYTYPE_INSECTOID,
	BODYTYPE_LIZARD,
	BODYTYPE_DRAGON,
	BODYTYPE_AVIAN,
	BODYTYPE_AMORPHOUS,
	BODYTYPE_QUADHOOVED,
	BODYTYPE_QUADPAW,
	BODYTYPE_SHASTICK,
	BODYTYPE_TERRAN_ARMOR,
	BODYTYPE_TRULOR_ARMOR,
	MAX_BODYTYPE
};


// Array used to match a race to a bodytype.
const Bodytype race_bodytype[NUM_RACES] =
{
	BODYTYPE_QUADPAW,      // Animal
    BODYTYPE_HUMANOID,     // Hyu-Mann
    BODYTYPE_HUMANOID,     // Elf
    BODYTYPE_HUMANOID,     // Dwomax
    BODYTYPE_SHASTICK,     // Shastick
    BODYTYPE_LIZARD,       // Murrash
    BODYTYPE_AVIAN,        // Laori
    BODYTYPE_INSECTOID,    // Thadyri
    BODYTYPE_HUMANOID,     // Terran
    BODYTYPE_AMORPHOUS,    // Trulor
    BODYTYPE_DRAGON,       // Dragon
    BODYTYPE_AVIAN,        // Deva
    BODYTYPE_AVIAN         // Solar
};


/* Player flags: used by char_data.char_specials.act */
/* Player flags: used by char_data.char_specials.act */
#define PLR_KILLER		(1 << 0)   /* Player is a player-killer		*/
#define PLR_THIEF		(1 << 1)   /* Player is a player-thief		*/
#define PLR_FROZEN		(1 << 2)   /* Player is frozen			*/
#define PLR_DONTSET     (1 << 3)   /* Don't EVER set (ISNPC bit)	*/
#define PLR_WRITING		(1 << 4)   /* Player writing (board/mail/olc)	*/
#define PLR_MAILING		(1 << 5)   /* Player is writing mail		*/
#define PLR_CRASH		(1 << 6)   /* Player needs to be crash-saved	*/
#define PLR_SITEOK		(1 << 7)   /* Player has been site-cleared	*/
#define PLR_NOSHOUT		(1 << 8)   /* Player not allowed to shout/goss	*/
#define PLR_NOTITLE		(1 << 9)   /* Player not allowed to set title	*/
#define PLR_UNJUST		(1 << 10)   /* Player has been unjust            */
#define PLR_DELETED		(1 << 11)  /* Player deleted - space reusable	*/
#define PLR_LOADROOM	(1 << 12)  /* Player uses nonstandard loadroom	*/
#define PLR_NODELETE	(1 << 13)  /* Player shouldn't be deleted	*/
#define PLR_INVSTART	(1 << 14)  /* Player should enter game wizinvis	*/
#define PLR_CRYO		(1 << 15)  /* Player is cryo-saved (purge prog)	*/
#define PLR_AFK         (1 << 16)  /* Player is away from keyboard      */
#define PLR_TELEPATHY   (1 << 17)  /* Player is telepathic  		*/
#define PLR_WATCHED		(1 << 18)  /* Player needs to be watched	*/
#define PLR_NOWIZLIST	(1 << 19)  /* Do not show player as immortal	*/
#define PLR_DOZY		(1 << 20)  /* Player is dozy!  Warn the imms...	*/
#define PLR_JAILED		(1 << 21)  /* Player in Jail!					*/


/* Mobile flags: used by char_data.char_specials.act */
#define MOB_SPEC         (1 << 0)  /* Mob has a callable spec-proc	*/
#define MOB_SENTINEL     (1 << 1)  /* Mob should not move		*/
#define MOB_SCAVENGER    (1 << 2)  /* Mob picks up stuff on the ground	*/
#define MOB_ISNPC        (1 << 3)  /* (R) Automatically set on all Mobs	*/
#define MOB_AWARE	 (1 << 4)  /* Mob can't be backstabbed		*/
#define MOB_AGGRESSIVE   (1 << 5)  /* Mob hits players in the room	*/
#define MOB_STAY_ZONE    (1 << 6)  /* Mob shouldn't wander out of zone	*/
#define MOB_WIMPY        (1 << 7)  /* Mob flees if severely injured	*/
#define MOB_AGGR_GOOD    (1 << 8)  /* auto attack good PC's		*/
#define MOB_AGGR_NEUTRAL (1 << 9)  /* auto attack neutral PC's		*/
#define MOB_AGGR_EVIL    (1 << 10) /* auto attack good PC's		*/
#define MOB_AGGR_ALL   (1 << 11)  /* Mob hits players in the room	*/
#define MOB_MEMORY	 (1 << 12) /* remember attackers if attacked	*/
#define MOB_HELPER	 (1 << 13) /* attack PCs fighting other NPCs	*/
#define MOB_NOCHARM	 (1 << 14) /* Mob can't be charmed		*/
#define MOB_NOSUMMON	 (1 << 15) /* Mob can't be summoned		*/
#define MOB_NOSLEEP	 (1 << 16) /* Mob can't be slept		*/
#define MOB_NOBASH	 (1 << 17) /* Mob can't be bashed (e.g. trees)	*/
#define MOB_NOBLIND	 (1 << 18) /* Mob can't be blinded		*/
#define MOB_NOSHOOT	 (1 << 19) /* Mob can't be shot at		*/
#define MOB_HELPER_RACE	 (1 << 20) /* assists NPCs of same race         */
#define MOB_HELPER_ALIGN (1 << 21) /* assists NPCs of same race         */
#define MOB_HUNTER	 (1 << 22) /* Mob hunts hated characters	*/
#define MOB_SEEKER	 (1 << 23) /* Mob hunts/seeks across zones	*/
#define MOB_MERCIFUL	 (1 << 24) /* Don't attack fallen or surrendered foes */
#define MOB_STAY_SECTOR    (1 << 25)  /* Mob shouldn't wander out of zone	*/
#define MOB_APPROVED		(1 << 29) /* MOB has been APPROVED for Auto-Loading */
#define MOB_KILLER			(1 << 30) /* MOB is a traitor */
#define MOB_DELETED			(1 << 31) /* Mob is deleted */


//	Staff Flags
namespace Staff {
	enum {
		General		= (1 << 0),		//	General commands
		Admin		= (1 << 1),		//	Administration
		Security	= (1 << 2),		//	Player Relations - Security
		Game		= (1 << 3),		//	Game Control
		Houses		= (1 << 4),		//	Houses
		Chars		= (1 << 5),		//	Player Relations - Character Maint.
		Clans		= (1 << 6),		//	Clans
		Mobiles		= (1 << 7),		//	Medit
		Objects		= (1 << 8),		//	Oedit
		Rooms		= (1 << 9),		//	Redit
		OLCAdmin	= (1 << 10),	//	OLC Administration
		Socials		= (1 << 11),	//	AEdit
		Help		= (1 << 12),	//	HEdit
		Shops		= (1 << 13),	//	SEdit
		Scripts		= (1 << 14),	//	ScriptEdit
		Force		= (1 << 15),	//	IMC Commands
		Coder		= (1 << 16),
		Wiznet		= (1 << 17),
		Invuln		= (1 << 18),
		Allow		= (1 << 19),	//	Administration
		Extra		= (1 << 20),
		SkillEd		= (1 << 21)
	};
}


//	Preference flags
namespace Preference {
	enum {
		Brief		= (1 << 0),		//	Room descs won't normally be shown
		Compact		= (1 << 1),		//	No extra CRLF pair before prompts
		StaffInvis	= (1 << 2),
		AdminInvis	= (1 << 3),
		AutoExit	= (1 << 4),		//	Display exits in a room
		NoHassle	= (1 << 5),		//	Staff are immortal
		Summonable	= (1 << 6),		//	Can be summoned
		NoRepeat	= (1 << 7),		//	No repetition of comm commands
		HolyLight	= (1 << 8),		//	Can see in dark
		Color		= (1 << 9),		//	Color
		Log1		= (1 << 10),	//	On-line System Log (low bit)
		Log2		= (1 << 11),	//	On-line System Log (high bit)
		RoomFlags	= (1 << 12),	//	Can see room flags (ROOM_x)	*/
        Invuln		= (1 << 13),
        AutoLoot	= (1 << 14),
        AutoSplit	= (1 << 15),
        Merciful	= (1 << 16),
        AutoAssist	= (1 << 17),
		AutoSwitch	= (1 << 18),
		Sound		= (1 << 19),
		Quest		= (1 << 20)
	};
}


namespace Channel {
	enum {
		NoShout		= (1 << 0),
		NoTell		= (1 << 1),
		NoGossip	= (1 << 2),
		Quest		= (1 << 3),
		NoSound		= (1 << 4),
		NoGratz		= (1 << 5),
		NoInfo		= (1 << 6),
		NoWiz		= (1 << 7),
		NoClan		= (1 << 8)
	};
}


//	WARNING: In the world files, NEVER set the bits marked "R" ("Reserved")
namespace AffectBit {
	enum {
		Blind			= (1 << 0),		// (R) Char is blind
		Invisible		= (1 << 1),		//  Char is invisible
		Immobilized		= (1 << 2),		//  Char is immobilized
		DetectInvis		= (1 << 3),		//  Char can see invis chars
		DetectMagic		= (1 << 4),		//  Char is sensitive to magic
		SenseLife		= (1 << 5),		//  Char can sense hidden lif
		Fly				= (1 << 6),		//  Char is flying
		Sanctuary		= (1 << 7),		//  Char protected by sanct.
		Group			= (1 << 8),		// (R) Char is grouped
		MageShield		= (1 << 9),		//  Char prot by en armor sp.
		Infravision		= (1 << 10),	//  Char can see in dark
		ProtectEvil		= (1 << 11),	//  Char protected from evil
		DetectAlign		= (1 << 12),	//  Char sees alignments
		Sleep			= (1 << 13),	// (R) Char magically asleep
		NoTrack			= (1 << 14),	//  Char can't be tracked
		Light			= (1 << 15),	//  Character gives off light
		Regeneration	= (1 << 16),	//  Character regens 2x hp
		Sneak			= (1 << 17),	//  Char can move quietly
		Hide			= (1 << 18),	//  Char is hidden
		VacuumSafe		= (1 << 19),	//  Char is okay in space
		Charm			= (1 << 20),	//  Char is charmed
		Silence			= (1 << 21),	//  Char can not speak
		Haste			= (1 << 22),	//  Char is hasted
		BladeBarrier	= (1 << 23),	//  Char is protected
		FireShield		= (1 << 24),	//  Char is protected
        Tracking		= (1 << 25),	//  Character detects motion
		Poison			= (1 << 26),	//  Poison of some kind
		Camoflauge		= (1 << 27),	//  Character is camoflauged
		VampRegen		= (1 << 28),	//  Vampiric Regeneration
		Berserk			= (1 << 29)		//  Character cannot flee
	};
}


// Bits used for racial and intrinsic advantanges.
namespace Advantages {
	enum {
		Lucky					= (1 << 0),
		FastHpRegen				= (1 << 1),
		FastManaRegen			= (1 << 2),
		FastMoveRegen			= (1 << 3),
		SlowMetabolism			= (1 << 4),
		Infravision				= (1 << 5),
		Flight					= (1 << 6),
		NatureRecharge			= (1 << 7),
		Ambidextrous			= (1 << 8),
		GroupMagicBonus			= (1 << 9),
		RuneAware				= (1 << 10),
		CombatReflexes			= (1 << 11),
		Sapient					= (1 << 12)
    };
}


namespace Disadvantages {
	enum {
		Unlucky					= (1 << 0),
		SlowHpRegen				= (1 << 1),
		SlowManaRegen			= (1 << 2),
		SlowMoveRegen			= (1 << 3),
		FastMetabolism			= (1 << 4),
		NoMagic					= (1 << 5),
		Atheist					= (1 << 6),
		ColdBlooded				= (1 << 7),
		NecroDestroysFlesh		= (1 << 8)
	};
}


/* Character equipment positions: used as index for char_data.equipment[] */
/* NOTE: Don't confuse these constants with the ITEM_ bitvectors
   which control the valid places you can wear a piece of equipment */
enum {
	WEAR_FINGER_R,
	WEAR_FINGER_L,
	WEAR_FINGER_3,
	WEAR_FINGER_4,
	WEAR_NECK_1,
	WEAR_NECK_2,
	WEAR_BODY,
	WEAR_HEAD,
	WEAR_FACE,
	WEAR_EYES,
	WEAR_EAR_R,
	WEAR_EAR_L,
	WEAR_LEGS,
	WEAR_FEET,
	WEAR_HANDS,
	WEAR_HANDS_2,
	WEAR_ARMS,
	WEAR_ARMS_2,
	WEAR_ABOUT,
	WEAR_BACK,
	WEAR_WAIST,
	WEAR_ONBELT,
	WEAR_WRIST_R,
	WEAR_WRIST_L,
	WEAR_WRIST_3,
	WEAR_WRIST_4,
	WEAR_HAND_R,
	WEAR_HAND_L,
	WEAR_HAND_3,
	WEAR_HAND_4,
	POS_WIELD_TWO,
	POS_HOLD_TWO,
	POS_WIELD,
	POS_WIELD_OFF,
	POS_LIGHT,
	POS_SHIELD,
	POS_HOLD
};


const SInt32 NUM_WEARS =	WEAR_HAND_4 + 1;	/* This must be the # of eq positions!! */


//	Player conditions
enum { FULL, THIRST, DRUNK };

const UInt32	MAX_NAME_LENGTH		= 12;
const UInt32	MAX_PWD_LENGTH		= 10;
const UInt32	HOST_LENGTH			= 30;
const UInt32	MAX_TITLE_LENGTH	= 80;
const UInt32	EXDSCR_LENGTH		= 8192;
const UInt32	MAX_ICE_LENGTH		= 160;

// const SInt32	MAX_SKILLS			= 25;
const SInt32	MAX_AFFECT			= 32;


enum Relation {
	RELATION_NONE = -1,
	RELATION_FRIEND,
	RELATION_NEUTRAL,
	RELATION_ENEMY
};


#define WEAR_MSG_SLIDEONTO	0
#define WEAR_MSG_WEARAROUND	1
#define WEAR_MSG_WEARABOUT	2
#define WEAR_MSG_WEARON		3
#define WEAR_MSG_WEAROVER	4
#define WEAR_MSG_WEARFROM	5
#define WEAR_MSG_PUTON		6
#define WEAR_MSG_ENGULFED	7
#define WEAR_MSG_HELDIN		8
#define WEAR_MSG_LIGHT		9
#define WEAR_MSG_SLUNGOVER	10
#define WEAR_MSG_ATTACH		11
#define MAX_WEAR_MSG		12
#define WEAR_MSG_WIELD		12
#define WEAR_MSG_STRAPTO	13
#define MAX_HANDWEAR_MSG	14

#define WEAR_ANYTHING_ELSE		0
#define WEARING_SOMETHING_AROUND	1
#define WEARING_SOMETHING_OVER		2
#define WEARING_SOMETHING_ON		3
#define WEARING_SOMETHING_ONBOTH	4
#define WEARING_SOMETHING_ONALL		5
#define WEARING_SOMETHING_ABOUT		6
#define HAVE_SOMETHING_ON		7
#define HAVE_SOMETHING_AROUND		8
#define HAVE_SOMETHING_ATTACHED		9
#define HOLDING_SOMETHING_IN		10
#define HOLDING_SOMETHING_INBOTH	11
#define MAX_ALREADY_WEARING		12

/* special affects that should not be set by objects/mob files */
#define SPEC_AFFS	      (AFF_CHARM | AFF_GROUP)

#define MAX_LEVEL	1000

#define MAX_TRUST	8
#define TRUST_IMMORTAL	1

#define TRUST_ADMIN	MAX_TRUST
#define TRUST_IMPL	7
#define TRUST_SUBIMPL	6
#define TRUST_GRGOD	5
#define TRUST_GOD	4
#define TRUST_CREATOR	3
#define TRUST_LORD	2
#define TRUST_AMBAS	1

#define LVL_FREEZE    TRUST_GRGOD

#define VIEW_OTHERS_CMDS  TRUST_SUBIMPL	/* miminum trust to view other's commands   */


// Body wear slots added in by Daniel Rollings AKA Daniel Houghton
struct body_wear_slots {
  SInt8 hitlocation;	// The location on the 2d6 hit table, -1 means that it doesn't exist.
  UInt8 damage_mult;	// Damage multiplier for hitting sensitive locations (default is 10)
  UInt8 armor_value;	// Added to passive defense for a given location
  UInt8 damage_max;	// Maximum damage for this slot
  SInt8 msg_wearon;	// The number element to use in the wear_on_message array.
  SInt8 msg_already_wear;	// The number element to use in the already_wearing array.
  int wear_bitvector;	// The bitvector of what items can be held in a given spot.
  char *partname;	// The SInt32 name of the body part
  char *keywords;	// The short name used to designate a wear location.
};


struct wear_on_message {
  char *worn_msg;
  char *to_room;
  char *to_char;
};


struct combat_profile_moves {
  char *move;
  SInt16 skillnum;
  SInt16 message;
  SInt8 speed;
  SInt8 locationmod;
  SInt8 type;  // If true, offense.
  void (*command_pointer)
  (Character *ch, char *argument, SInt32 cmd, const char *command, SInt32 subcmd);
  int subcmd;
};


// CHANGEPOINT:  This should go elsewhere.
extern int parse_race(char *arg);
extern Flags find_race_bitvector(char arg);
extern void remove_racial_affects(Character *ch);
extern void apply_racial_affects(Character *ch);
extern void roll_height_weight(Character *ch);
extern const SInt8 race_select_costs[NUM_RACES];
extern const Flags race_advantages[NUM_RACES];
extern const Flags race_disadvantages[NUM_RACES];

#endif

