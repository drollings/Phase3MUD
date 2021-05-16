/* ************************************************************************
*  File: skills.h										  Part of TLO Phase ][	  *
*																								 *
*  Usage: Contains routines related to the SkillStore class,		  *
*	  which contains a linked list of the player's skills.		  *
*  This code is Copyright (C) 1997 by Daniel Rollings AKA Daniel Houghton.			  *
*																								 *
*  $Author: sfn $
*  $Date: 2001/02/04 15:57:58 $
*  $Revision: 1.9 $
************************************************************************ */


#ifndef _SKILLS_H_
#define _SKILLS_H_

#include "types.h"
#include "skills.defs.h"
#include "stl.vector.h"
#include "stl.map.h"
#include "property.h"

class Character;
class Object;
class Skill;

extern Vector<Skill *>	Skills;

void list_skills(Character *ch, bool show_known_only, Flags type);

struct SkillRef
{
	SInt16	skillnum;
	SInt8	chance;
};

class SkillKnown 
{
public:
	SInt16 skillnum;			/* Number of skill to refer to			*/
	UInt8 proficient;			/* The modifier to the skill roll		 */
	UInt8 theory;			/* The number of points invested		  */
	
	SkillKnown *next;
	
	bool operator== (SkillKnown &other) const;
	bool operator<= (SkillKnown &other) const;
	bool operator>= (SkillKnown &other) const;
	bool operator< (SkillKnown &other) const;
	bool operator> (SkillKnown &other) const;

};

inline bool SkillKnown::operator== (SkillKnown &other) const
{
	return (skillnum == other.skillnum);
}

inline bool SkillKnown::operator<= (SkillKnown &other) const
{
	return (skillnum <= other.skillnum);
}

inline bool SkillKnown::operator>= (SkillKnown &other) const
{
	return (skillnum >= other.skillnum);
}

inline bool SkillKnown::operator< (SkillKnown &other) const
{
	return (skillnum < other.skillnum);
}

inline bool SkillKnown::operator> (SkillKnown &other) const
{
	return (skillnum > other.skillnum);
}


class SkillStore {
  private:
	SkillKnown *skills;

  public:
	SkillStore();
	~SkillStore();


	// Functions to acquire
	int GetTable(SkillKnown **i) const;
	int FindSkill(SInt16 num, SkillKnown **i = NULL) const;
	int GetSkillMod(SInt16 num = 0, SkillKnown *i = NULL) const;
	int GetSkillPts(SInt16 num = 0, SkillKnown *i = NULL) const;
	int GetSkillTheory(SInt16 skillnum, SkillKnown *i) const;
	int CheckSkillPrereq(Character *ch, SInt16 &skillnum);
	int GetSkillChance(Character *ch, SInt16 skillnum, 
							 SkillKnown *i, SInt8 chkdefault = 0);
	void FindBestRoll(Character *ch, SInt16 &skillnum, SInt16 &defaultnum,
							SInt8 &chance, SInt8 &mod, int type = 0);
	SInt8 DiceRoll(Character *ch, SInt16 &skillnum, SInt8 &chance, 
					  SkillKnown *i = NULL);
	int RollSkill(Character *ch, SInt16 skillnum);
	int RollSkill(Character *ch, SInt16 skillnum, SInt8 chance);
	int RollSkill(Character *ch, SInt16 skillnum, SInt16 defaultnum, SInt8 chance);
	int RollSkill(Character *ch, SkillKnown *i);
	int SkillContest(Character *ch, SInt16 cskillnum, SInt8 cmod,
						  Character *victim, SInt16 vskillnum, SInt8 vmod,
						  SInt8 &offense, SInt8 &defense);
	int AddSkill(SInt16 skillnum, 
		SkillKnown **i = NULL, 
		UInt8 pts = 1, UInt8 theory = 0);
	int RemoveSkill(SInt16 num = 0);
	int LearnSkill(SInt16 num, SkillKnown *i = NULL, 
		  SInt16 pts = 1);
	int LearnTheory(SInt16 num, SkillKnown *i = NULL, 
			SInt16 pts = 1);
	int SetSkill(SInt16 num = 0, SkillKnown *i = NULL, 
		UInt8 trainpts = 0, UInt8 theorypts = 0);
	int LearnSkill(Character *ch, SInt16 num, SkillKnown *i = NULL, 
		  SInt16 pts = 1);
	int SetSkill(Character *ch, SInt16 num = 0, SkillKnown *i = NULL, 
		UInt8 trainpts = 0, UInt8 theorypts = 0);
	int LoadSkills(FILE *infile, int nr);
	int SaveSkills(FILE *outfile);
	int TotalPoints(Character *ch);
	void ClearSkills();
	SInt16 FindBestDefault(Character *ch, SInt16 &skillnum, int chance, 
								  SInt8 &mod, SInt8 chkdefault);
	void RollToLearn(Character *ch, SInt16 skillnum, SInt16 roll, SkillKnown *i = NULL);

};


class Skill : public Editable
{
public:
	Skill(void);
	Skill(const Skill &from);
	virtual ~Skill(void);

	// enum {	General, Movement, Combat, CloseCombat, RangedCombat, Healing, Technical, Stealth };
	enum {
		GENERAL 		= (1 << 0),
		MOVEMENT 		= (1 << 1),
		WEAPON 			= (1 << 2),
		COMBAT 			= (1 << 3),
		RANGEDCOMBAT 	= (1 << 4),
		STEALTH 		= (1 << 5),
		ENGINEERING 	= (1 << 6),
		LANGUAGE 		= (1 << 7),
		ROOT 			= (1 << 8),
		METHOD 			= (1 << 9),
		TECHNIQUE  		= (1 << 10),
		ORDINATION	 	= (1 << 11),
		SPHERE 			= (1 << 12),
		VOW				= (1 << 13),
		SPELL  			= (1 << 14),
		PRAYER 			= (1 << 15),
		PSI				= (1 << 16),
		STAT			= (1 << 17),
		MESSAGE 		= (1 << 18),
		DEFAULT 		= (1 << 19)
	};

	static const char *namedtypes[];
	static const char *targeting[];

	enum Target {
		Ignore		= (1 << 0),		//	IGNORE TARGET
		CharRoom	= (1 << 1),		//	PC/NPC in room
		CharWorld	= (1 << 2),		//	PC/NPC in world
		FightSelf	= (1 << 3),		//	If fighting, and no argument, target is self
		FightVict	= (1 << 4),		//	If fighting, and no argument, target is opponent
		SelfOnly	= (1 << 5),		//	Flag: Self only
		NotSelf		= (1 << 6),		//	Flag: Anybody BUT self
		ObjInv		= (1 << 7),		//	Object in inventory
		ObjRoom		= (1 << 8),		//	Object in room
		ObjWorld	= (1 << 9),		//	Object in world
		ObjEquip	= (1 << 10),	//	Object held
		ObjDest		= (1 << 11),
		CharObj		= (1 << 12),	//	Object character is holding
		IgnChar		= (1 << 13),
		IgnObj		= (1 << 14),
		Melee		= (1 << 15),	//	Spell only hits at melee range
		Ranged		= (1 << 16)		//	Spell can hit targets in other rooms
	};

	SInt32				number;
	SInt32				delay;
	SInt32				lag;
	char *				name;
	// char *				fade;
	// char *				faderoom;
	char *				wearoff;
	char *				wearoffroom;
	Flags				types;
	Flags				stats;
	Flags				min_position;
	Flags				target;
	Flags				routines;
	
	SInt16 mana;

	SInt8 startmod;	// Beginning modifier used when one point is invested
	SInt8 maxincr;	// The maximum number of point increase between skill levels
	bool violent;
	bool learnable;
	
	Vector<SkillRef>	prerequisites;
	Vector<SkillRef>	defaults;
	Vector<SkillRef>	anrequisites;

	enum Vars {
		SkillName,
		SkillType,
		SkillStats,
		SkillMinPos,
		SkillTarget,
		SkillRoutines,
		SkillDelay,
		SkillLag,
		SkillWearOff,
		SkillWearOffRoom,
		SkillMana,
		SkillStartMod,
		SkillMaxIncr,
		SkillViolent,
		SkillPrereq,
		SkillDefault,
		SkillAnreq,
		MAXVAR
	};

	Ptr					GetPointer(int arg);
	virtual const struct olc_fields * Constraints(int arg);

	Property::PropertyList		properties;
	
public:

// 	static SInt32		RankScore(SInt32 rank);
// 	static SInt32		StatBonus(SInt32 stat);
// 
// 	static SInt32		Value(Character *ch, char *argument, SInt32 skill);
// 	static Result		Roll(Character *ch, SInt32 skill, SInt32 modifier, char *argument,
// 								Character **vict, Object **obj);

	static const char *	Name(SInt32 number);
	static SInt32 		Number(const char *name, int type);
	static SInt32 		Number(const char *name, SInt16 index = 1, SInt16 top = TOP_SKILLO_DEFINE);
	static Result		CalculateResult(SInt32 roll);
	static const Skill *ByNumber(SInt32 number);
	static const Skill *ByName(const char *name);
	void				SaveDisk(FILE *fp, int indent = 1);

	void				Parser(char *input);

// CHANGEPOINT:  This will need to be private later
public:
	friend class SkillStore;

};


// extern const SInt32	NumSkills;

/* Attacktypes with grammar */
struct attack_hit_type {
	char	*singular;
	char	*plural;
};

#endif


