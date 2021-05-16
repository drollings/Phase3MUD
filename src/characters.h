/*************************************************************************
*   File: characters.h               Part of Aliens vs Predator: The MUD *
*  Usage: Header file for characters                                     *
*************************************************************************/


#ifndef	__CHARACTERS_H__
#define __CHARACTERS_H__


#include "types.h"
#include "structs.h"
#include "skills.h"
#include "screen.h"
#include "character.defs.h"
#include "room.defs.h"
#include "find.h"
#include "mud.base.h"
#include "combat.h"
#include "spells.h"
#include "spec.h"
#include "index.h"

//	STL
#include "stl.llist.h"
#include "stl.map.h"
#include "stl.vector.h"

class Descriptor;
class Opinion;
class Affect;
class Concentration;
class Event;
class Alias;
// class Path;
class CombatProfile;

extern void command_interpreter(Character *ch, char *argument, int cmdnum = 0);

/* These data contain information about a players time data */
struct time_data {
	time_t	birth;			/* This represents the characters age                */
	time_t	logon;			/* Time of the last logon (used to calculate played) */
	UInt32	played;			/* This is the total accumulated time played in secs */
};


struct title_type {
	char *	title_m;
	char *	title_f;
};


struct MiscData {
				MiscData(void) : watching(0), carry_weight(0), clannum(-1), carry_items(0), clanrank(-1) { }
				MiscData(const MiscData &data) : watching(0), carry_weight(0), clannum(data.clannum),
						carry_items(0), clanrank(data.clanrank) { }
	SInt32		watching;

	SInt32		carry_weight;
	VNum		clannum;

	UInt8		carry_items;
	SInt8		clanrank;

};


struct GeneralData {
				GeneralData(void) : longDesc(NULL), title(NULL), sex(Neutral),  race(RACE_UNDEFINED),
						clss(CLASS_UNDEFINED), position(POS_STANDING),  state(STATE_AWAKE),
						bodytype(BODYTYPE_HUMANOID), mortality(MORTALITY_MORTAL), totalpts(0), level(0), height(0),
						alignment(0), lawfulness(0), exp_modifier(0), damage_taken(0), act(0),
						advantages(0), disadvantages(0), hometown(0), group_pos(0),
						fighting(NULL), stalking(0), aiming(0) { }
				GeneralData(const GeneralData &data) : longDesc(SSShare(data.longDesc)),
						title(SSShare(data.title)), sex(data.sex), race(data.race),  clss(data.clss),
						position(data.position), state(data.state),  bodytype(data.bodytype),
						mortality(data.mortality), totalpts(data.totalpts), level(data.level), height(data.height),
						alignment(data.alignment), lawfulness(data.lawfulness),
						exp_modifier(data.exp_modifier), damage_taken(data.damage_taken), act(data.act), 
						advantages(data.advantages), disadvantages(data.disadvantages), hometown(data.hometown),
						group_pos(data.group_pos), fighting(NULL), stalking(0), aiming(0), misc(data.misc) {
					this->conditions[0] = data.conditions[0];
					this->conditions[1] = data.conditions[1];
					this->conditions[2] = data.conditions[2];
				}
	SString *	longDesc;
	SString *	title;

	Sex			sex;
	Race		race;
    	Class		clss;
	Position	position;
	CharState	state;
	Bodytype	bodytype;
	Mortality	mortality;

    SInt32		totalpts;

	SInt16		level;
	UInt16		height;
	SInt16		alignment;
	SInt16		lawfulness;
	SInt16		exp_modifier;
	SInt16		damage_taken;

	Flags		act;				// CHANGEPOINT:  Shouldn't this be in MobData?
    Flags		advantages;
    Flags		disadvantages;

	SInt8		conditions[3];
    UInt8		hometown;
    UInt8		group_pos;

	Character *	fighting;
	IDNum		stalking;
	IDNum		aiming;

	SkillStore	skills;

	MiscData	misc;
};


struct PlayerSpecialData {
	// SInt8			skills[MAX_SKILLS+1];
	SInt32			wimp_level;
	VNum			load_room;
	Flags			preferences;
	Flags			channels;
	Flags			staff_flags;

	time_data		last_died;

    SInt16			freeze_level;
	SInt16			max_level;
	SInt16			level_died;

	UInt8			bad_pws;
	UInt8			saycolor;
	SInt8			page_length;
    UInt8			trust;
    UInt8			staff_invis;

	// UInt32			PKills, MKills;
	// UInt32			PDeaths, MDeaths;
	// float			killScore;

	struct {
		char *		listen;
		char *		rreply;
		char *		rreply_name;
		Flags		deaf;		//	Deaf-to-channel
		Flags		allow;		//	Allow overrides
		Flags		deny;		//	Deny overrides
	} imc;
};


class PlayerData {
public:
					PlayerData(void);
					~PlayerData(void);

	struct PlayerSpecialData special;
	char			passwd[MAX_PWD_LENGTH+1];
	char *			afk;
	char *			prompt;
	char *			host;
	char *			email;
	char *			poofin;
	char *			poofout;
	LList<Alias *>	aliases;
	time_data		time;

	SInt32			last_tell;

	IDNum			idnum;
	SInt32			timer;

	Map<IDNum, CallName>	CalledNames;
	EnergyPool		MagicPool;
};


class MobData {
public:
					MobData(void);
					MobData(const MobData &mob);
					~MobData(void);

	SInt32			attack_type;
	SInt32			wait_state;
	struct Dice		damage;

	IDNum			hunting;		// We'll use these instead of Path for now. -DH
	IDNum			seeking;
	VNum			roomseeking;

    UInt16			disposition;

	Position		default_pos;
	UInt8			last_direction;
	UInt8			tick;

	Opinion *		hates;
	Opinion *		fears;
//	Opinion			likes;

	SInt8			type;	// Simple/extended?  Simple uses rendering routine.
	SInt16			numhitdice;
	SInt16			sizehitdice;
	SInt16			modhitdice;

};


/* Char's abilities. */
struct Abilities {
				Abilities(void) : Str(10), Int(10), Wis(10), Dex(10), Con(10), Cha(10),
						Vision(10), Hearing(10), Movement(5), Magery(8) { }
				Abilities(const Abilities &a) { *this = a; };
	UInt8		Str, Int, Wis, Dex, Con, Cha;
    UInt8		Vision, Hearing, Movement, Magery;
};


struct Points {
				Points(void) : hit(0), max_hit(0), move(0), max_move(0), mana(0),
						max_mana(0), gold(0), bank_gold(0), pracs(0), total_pts(0), exp(0),
                        hitroll(0), damroll(0) { }
	SInt16		hit, max_hit;
	SInt16		move, max_move;
	SInt16		mana, max_mana;

	SInt32		gold, bank_gold;

	SInt32		pracs;
	SInt32		total_pts;

    SInt16		exp;

	SInt16		hitroll;
	SInt16		damroll;

};


class Character : public MUDObject, public Editable {
public:
	friend void close_socket(Descriptor *d);
						Character(void);
						Character(const Character &ch);
						~Character(void);

	virtual Type		DataType(void) const;

//	Accessor Functions
	const char *		RealName(void) const;
	virtual const char *		CalledName(const Character *ch) const;
	virtual const char *		CalledKeywords(const Character *ch) const;

//	Game Functions
	void				ToWorld(void);
	void				FromWorld(void);

	void				Save(VNum load_room = NOWHERE);
	SInt32				Load(char *name);

	void				Initialize(void);
	void				Reset(void);

	const char * 		LDesc(const char *def = "<ERROR>") const;
	void		 		SetLDesc(const char *def = NULL);
	void		 		SetLDesc(SString *def = NULL);
	const SString * 	SSLDesc(const SString *def = NULL) const;
//	void				set_name(char *name);
//	void				set_short(char *short_descr = NULL);
	void				SetTitle(char *title = NULL);
	void				SetLevel(UInt16 level);

	//	Former seperate functions
	virtual void		Extract(void);

	virtual void		FromRoom(void);
	virtual void		ToRoom(VNum room);
	virtual bool		Fall(VNum toroom, int prev);

	void				UpdatePos(void);
	void				UpdateObjects(void);
	void				AffectTotal(void);
	void 				AffectModify(Affect::Location loc, SInt8 mod, Flags bitv, bool add);

	void				Appear(void);

	void				DieFollower(void);
	void				StopFollower(void);

	void				Fight(Character *vict);
	void				StopFighting(void);
	void				StopFoesFighting(void);
	void				Die(Character *killer);
	SInt32				Hit(Character *victim, Attack * attackmove = NULL, SInt32 range = 0);
	SInt32 				Damage(Character *attacker, Attack * attack);
	SInt32				Critical(Character *attacker, char critical, SInt32 type);

//	void				DisplayStat(Character *ch);

	void				GainExp(int amount);
	void				AdvanceLevel(int amount);
	// void				LoseBodypart(SInt32 part);

	Relation			GetRelation(const Character *target) const;
	VNum				StartRoom(void);

	//	Utility Classes, mostly former Macros
	bool				LightOk(void) const;
	bool				InvisOk(const Character *target) const;
	bool				CanSeeStaff(const Character *target) const;
#if 0
	bool				CanSee(const Character *target) const;
	bool				CanSee(const Object *target) const;
	bool				CanSee(const GameData *target) const;
#endif

	bool				CanUse(const Object *obj) const;

	UInt32				PointsSpent(void);
	UInt32				CombatPointsSpent(void);

	SInt32				Send(const char *messg);
	SInt32				Sendf(const char *messg, ...) __attribute__ ((format (printf, 2, 3)));
	SInt32				MergeSend(const char *messg);
	SInt32				MergeSendf(const char *messg, ...) __attribute__ ((format (printf, 2, 3)));
	Color				DefColor(void) const;
	void				SetDefColor(Color num);

	CombatProfile		combat_profile;

	SInt32				pfilepos;			//	playerfile pos
	VNum				was_in_room;		//	location for linkdead people

	GeneralData			general;			//	Normal data
	Abilities			real_abils;			//	Unmodified Abilities
	Abilities			aff_abils;			//	Current Abililities
	Points				points;				//	Points
	PlayerData *		player;				//	PC specials
	MobData *			mob;				//	NPC specials

    LList<Concentration *> concentrations;	//	concentration on spells

	// mutable	LList<Object *>	equipment;				//	Head of list
	Object *			equipment[NUM_WEARS];	//	Equipment array

	Descriptor *		desc;				//	NULL for mobiles

	LList<Character *>	followers;			//	List of chars followers
	Character *			master;				//	Who is char following?

	Event *				points_event[3];
	Event *				action;

	// Path *				path;

	virtual bool		CanBeSeen(const Character *viewer) const;

	MUDObject *			Mounted(void) const;
	Character *			Riding(void) const;		// Returns only Character mounts
	Object *			SittingOn(void) const;	// Returns only Object mounts
	void				SetMount(MUDObject * thing);
	void 				AddRider(Character * ch);
	virtual void 		RemoveRider(Character * ch, int type);
	virtual void 		Dismount(void);

private:
	MUDObject *			sitting_on;

public:
	static Character *	Read(VNum nr);
	static bool			Find(VNum vnum);

//	static VNum			Real(VNum vnum);
	static Index<Character>		_Index;

	void				Parser(char *input);

//	Scriptable Overrides
	virtual void			Echo(char * arg) const;
	virtual void			EchoAt(char * arg, GameData * target) const;
	virtual void			EchoAround(char * arg, GameData * target) const;

	virtual Character *		TargetChar(const char *arg) const;
	virtual Character *		TargetCharRoom(const char *arg) const;
	virtual Object *		TargetObj(const char *arg) const;
	virtual Object *		TargetObjList(const char * arg, LList<Object *> &list) const;
	virtual Room *			TargetRoom(const char *arg) const;

	virtual void 			Command(char * argument, int cmd = 0);
	virtual void 			AtCommand(VNum target, char * argument);

	enum Vars {
		MobAffFlags,
		MobAttackType,
		MobBodytype,
		MobAlignment,
		MobHitroll,
		MobDamroll,
		MobDisposition,
		MobLevel,
		MobGold,
		MobMaxRiders,
		MobNumHPDice,
		MobSizeHPDice,
		MobAddHP,
		MobDamResist,
		MobExpBonus,
		MobTotalPoints,
		MobType,
		MobNPCFlags,
		MobPosition,
		MobDefPosition,
		MobRace,
		MobClass,
		MobSex,
		MobStr,
		MobInt,
		MobWis,
		MobDex,
		MobCon,
		MobCha,
		MobKeywords,
		MobName,
		MobSDesc,
		MobLDesc,
		MAXVAR
	};

	Ptr	GetPointer(int arg);
	virtual const struct olc_fields * Constraints(int arg);

	static void SaveDisk(Character *mob, FILE *fp);
};

inline Type		Character::DataType(void) const			{ return Datatypes::Character; }

inline const char * 	Character::LDesc(const char *def) const
{	return (general.longDesc ? general.longDesc->Data() : def);	}

inline 	void			Character::SetLDesc(const char *val)
{	if (general.longDesc)	SSFree(general.longDesc);
	general.longDesc = NULL;
	if (val)	general.longDesc = SString::Create(val);	}

inline 	void			Character::SetLDesc(SString *val)
{	if (general.longDesc)	SSFree(general.longDesc);
	general.longDesc = NULL;
	if (val)	general.longDesc = val->Share();	}

inline const SString * 	Character::SSLDesc(const SString *def) const
{ 	return (general.longDesc ? general.longDesc : def);	}

inline void 			Character::Command(char * argument, int cmd)
{	command_interpreter(this, argument, cmd);	}

inline Character *	Character::TargetCharRoom(const char *arg) const
{
	return get_char_vis(this, arg, (*arg == UID_CHAR) ? FIND_CHAR_WORLD : FIND_CHAR_ROOM);
}

inline Object *	Character::TargetObjList(const char * arg, LList<Object *> &list) const
{	if (*arg == UID_CHAR)
		return get_obj(arg);
	else
		return get_obj_in_list_vis(this, arg, list);	}

inline MUDObject * Character::Mounted(void) const
{	return sitting_on;	}
inline Character * Character::Riding(void) const
{	if (sitting_on && sitting_on->DataType() == Datatypes::Character)
		return (Character *) sitting_on;
	return NULL;	}
inline Object * Character::SittingOn(void) const
{	if (sitting_on && sitting_on->DataType() == Datatypes::Object)
		return (Object *) sitting_on;
	return NULL;	}
inline void Character::Dismount(void)
{	if (sitting_on)		sitting_on->RemoveRider(this, 0);		}



extern LList<Character *>	Characters;
extern LList<Character *>	PurgedChars;
extern LList<Character *>	CombatList;

//extern LList<Character *>	character_list;
//extern LList<Character *>	purged_chars;
//extern LList<Character *>	combat_list;
//extern Index<Character>		mob_index;

//extern UInt32	top_of_mobt;
//extern SInt32	top_of_p_table;
//extern struct player_index_element *player_table;	/* index to plr file	 */


struct PlayerIndex {
				PlayerIndex(void) : name(NULL), id(0) { }
	char *		name;
	IDNum		id;
};

inline bool Character::Find(VNum vnum) {	return (_Index.find(vnum) != _Index.end());	}

extern Vector<PlayerIndex>	player_table;

extern const struct title_type titles[NUM_RACES][12];
extern const struct body_wear_slots wear_slots[MAX_BODYTYPE][NUM_WEARS];

extern void str_swing_dice(SInt16 strength, SInt8 &num, SInt8 &dicemod);
extern void str_thrust_dice(SInt16 strength, SInt8 &num, SInt8 &dicemod);
extern int str_dam(SInt16 strength, SInt16 move);
extern SInt8 rate_encumbrance(Character *ch, int weight) ;
extern int stat_cost(int level);
extern int stat_ptlevel(int cost);
extern int generic_cost(int level, SInt16 start, SInt16 mod);
extern int generic_ptlevel(int cost, SInt16 start, SInt16 mod);
extern int sum_char_pts(Character *ch);
extern int character_pts_spent(Character *ch);
extern SInt16 get_weapon_skill(Character *ch, SInt8 hand = 0);

extern const char *class_types[];

#define GET_POINTS_EVENT(ch, i) ((ch)->points_event[i])


extern SInt8 find_eqtype_pos(Character *ch, SInt8 seektype, SInt8 bottomslot = 0,
						SInt8 topslot = NUM_WEARS - 1, bool random = false);
extern int perform_move(Character *ch, int dir, int specials_check);
extern int riding_check(Character *ch, Character *mount, int type);
extern SInt16 pick_weapon(Character * ch);
extern SInt16 swing_or_thrust(Character * ch, SInt8 & hand);

extern void render_simple_mob(Character *ch);

#endif	// __CHARACTERS_H__

