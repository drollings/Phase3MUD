/* ************************************************************************
*   File: spells.h                                      Part of CircleMUD *
*  Usage: header file: constants and fn prototypes for spell system       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author: sfn $
*  $Date: 2001/02/04 15:57:59 $
*  $Revision: 1.9 $
************************************************************************ */

#ifndef _SPELLS_H_
#define _SPELLS_H_

#include "skills.h"
#include <string>

#define DEFAULT_STAFF_LVL	12
#define DEFAULT_WAND_LVL	12

#define CAST_UNDEFINED	-1
#define CAST_SPELL	0
#define CAST_POTION	1
#define CAST_WAND	2
#define CAST_STAFF	3
#define CAST_SCROLL	4

#define MAG_DAMAGE	(1 << 0)
#define MAG_AFFECTS	(1 << 1)
#define MAG_UNAFFECTS	(1 << 2)
#define MAG_POINTS	(1 << 3)
#define MAG_ALTER_OBJS	(1 << 4)
#define MAG_GROUPS	(1 << 5)
#define MAG_MASSES	(1 << 6)
#define MAG_AREAS	(1 << 7)
#define MAG_SUMMONS	(1 << 8)
#define MAG_CREATIONS	(1 << 9)
#define MAG_MANUAL	(1 << 10)
#define MAG_MATERIALS	(1 << 11)
#define MAG_ROOM	(1 << 12)
#define MAG_HITGROUPS	(1 << 13)
#define MAG_SCRIPT	(1 << 14)
#define MAG_MISFIRE	(1 << 15)


#define TYPE_PERFORM_VIOLENCE     -2 /* Attack is called from perform_violence */
#define TYPE_UNDEFINED               -1
#define SPELL_RESERVED_DBC            0  /* SKILL NUMBER ZERO -- RESERVED */

#define SAVING_PARA   0
#define SAVING_ROD    1
#define SAVING_PETRI  2
#define SAVING_BREATH 3
#define SAVING_SPELL  4

#define SPELL_TYPE_SPELL   0
#define SPELL_TYPE_POTION  1
#define SPELL_TYPE_WAND    2
#define SPELL_TYPE_STAFF   3
#define SPELL_TYPE_SCROLL  4


#define ASPELL(spellname) \
void	spellname(int level, Character *ch, \
		  Character *victim, Object *obj, char *arg)

#define MANUAL_SPELL(spellname)	spellname(level, caster, cvict, ovict, arg);

ASPELL(spell_create_water);
ASPELL(spell_recall);
ASPELL(spell_teleport);
ASPELL(spell_summon);
ASPELL(spell_locate_object);
ASPELL(spell_charm);
ASPELL(spell_information);
ASPELL(spell_identify);
ASPELL(spell_enchant_weapon);
ASPELL(spell_remove_curse);
ASPELL(spell_detect_poison);
ASPELL(spell_recharge);
ASPELL(spell_relocate);
ASPELL(spell_peace);
ASPELL(spell_planeshift);
ASPELL(spell_horrific_illusion);
ASPELL(spell_portal);
ASPELL(spell_bind);
ASPELL(spell_raise_dead);
ASPELL(spell_reincarnate);


class SpellData {
public:
	SpellData(void);
	~SpellData(void);

	SInt16 mana;
	SInt16 spellnum;
	SInt16 hp;
	bool isprayer; // Spell or prayer?
	std::string arg;
};


class MethodMessage {
public:
	MethodMessage(void) : tochar(NULL), toroom(NULL) { }
	~MethodMessage(void);

	char *	tochar;
	char *	toroom;
};

// A method is a means of summoning energy to cast a spell.  Once the energy
// is gathered, it may dissipate slower, faster, or not at all, depending on
// the method used.

#define METHOD_FUNC(name)	void (name)(Character *ch, SInt16 root, SInt16 amount, SInt16 method, MUDObject *target)

class Method {
public:
	Method(void) : number(-1), build(5), decay(1),
    				startchar(NULL), startroom(NULL), 
                    startvis(true), endvis(true), 
    				hit(0), mana(100), move(0), ceiling(100), skill(0),
                    startfunc(NULL), endfunc(NULL), disruptfunc(NULL) { }
	~Method(void);

	void Start(Character * ch, SInt16 root, SInt16 amount, MUDObject *target = NULL);
	void Disrupt(Character * ch, SInt16 root, SInt16 amount, MUDObject *target = NULL);
	void Complete(Character * ch, SInt16 root, SInt16 amount, MUDObject *target = NULL);

	void Parser(char *input);

    const char *Name(void) { return spells[number]; }
    const char *Verb(void) { return verb; }

    void StageMessage(Character *ch, SInt16 &root, SInt16 &amount, SInt8 &stage, MUDObject *target = NULL);

    static Vector<Method> Methods;
    static const char *func_names[];
    static SInt32 * funcs[];

    VNum number;

    SInt8 build;
    SInt8 decay;

	char *startchar;
	char *startroom;

    bool startvis;
    bool endvis;

	char *verb;

    SInt16 hit;
    SInt16 mana;
    SInt16 move;
    SInt16 ceiling;
    SInt16 skill;
    
	METHOD_FUNC(*startfunc);
	METHOD_FUNC(*endfunc);
	METHOD_FUNC(*disruptfunc);

    Vector<MethodMessage> Messages;
};


// An energy to draw from to cast a spell.
class Energy {
public:
	Energy(void) : mana(0), lifetime(0), root(SPHERE_VIM), method(0), nextdrain(1), bodypart(-1) { }
	~Energy(void) { }

private:
	SInt16	mana;
	SInt16	lifetime;
	SInt8	root;
	SInt8	method;
	SInt8	nextdrain;
	SInt8	bodypart;
	
public:
	void	Draw(Character *ch, SInt16 amount);
	void	Charge(Character *ch, SInt16 amount, bool drain = true);
	bool	Update(Character *ch);
	
	friend class EnergyPool;
};


// A pool of energies to draw from.  Energies of a given type may be tapped,
// starting with the most volatile.
class EnergyPool {
public:
	EnergyPool(void) { };
	~EnergyPool(void) { };

public:
	LList <Energy *>	Pools;
	
	SInt16	Draw(Character *ch, SInt16 root, SInt16 amount);
	void	Charge(Character *ch, SInt16 root, SInt16 amount, SInt16 method, bool drain = true);
	void	Update(Character *ch);
	void	Clear(void);
	bool	List(Character *ch);
	SInt16	Check(Character *ch, SInt16 root);
	void	Save(Character *ch, FILE *outfile);
};


/* basic magic calling functions */

void mag_damage(int level, Character *ch, Character *victim,
  int spellnum);

void mag_affects(int level, Character *ch, Character *victim,
  int spellnum, int concentrate = 0);

void mag_group_switch(int level, Character *ch, Character *tch, int spellnum);

void mag_groups(int level, Character *ch, int spellnum);

void mag_masses(int level, Character *ch, int spellnum);

void mag_areas(int level, Character *ch, Character *victim, int spellnum);

void mag_hitgroup(int level, Character * ch, Character * victim, int spellnum);

void mag_summons(int level, Character *ch, Object *obj, int spellnum);

void mag_points(int level, Character *ch, Character *victim, int spellnum);

void mag_unaffects(int level, Character *ch, Character *victim, int spellnum);

void mag_alter_objs(int level, Character *ch, Object *obj, int spellnum);

void mag_creations(int level, Character *ch, int spellnum);

int	call_magic(Character *caster, Character *cvict,
  Object *ovict, int spellnum, int level, int casttype, char *arg);

void	mag_objectmagic(Character *ch, Object *obj,
			char *argument);

int cast_event(Character *ch, Character *tch, Object *tobj, SpellData *spell);

int mag_materials(	int level, Character * ch, Character * victim, 
					SInt16 spellnum, bool extract, bool verbose);
// int mag_materials(Character *ch, int spellnum, int item0, int item1,
//  int item2, int extract, int verbose, int level, Character *victim, int savetype);

void mag_room(int level, Character * ch, int spellnum);

/* other prototypes */
void spell_level(int spell, int clss, int level);
void init_spell_levels(void);
char *skill_name(int num);

#endif

