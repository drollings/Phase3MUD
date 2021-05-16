/* ************************************************************************
*   File: property.cc      	                            Part of TLO	      *
*  Usage: properties					                                  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author: sfn $
*  $Date: 2000/11/18 08:48:54 $
*  $Revision: 1.8 $
************************************************************************ */

#include "types.h"
#include "mud.base.h"
#include "buffer.h"
#include "property.h"
#include "find.h"
#include "scripts.h"
#include "strings.h"
#include "olc.h"
#include "constants.h"
#include "skills.defs.h"

extern void add_var(LList<TrigVarData *> &var_list, const char *name, const char *value, SInt32 id);
extern int script_driver(Scriptable * go, Trigger *trig, int mode);


AffectEditable::AffectEditable(void) :
	location(-1), duration(0), modifier(0), flags(0)
{
}


AffectEditable::AffectEditable(SInt32 mod, SInt32 d, Affect::Location loc, Flags bv) :
	location(loc), duration(d), modifier(mod), flags(bv)
{
}


Ptr	AffectEditable::GetPointer(int arg)
{
	switch (arg) {
	case 0:				return ((Ptr) (&location));			break;
	case 1:				return ((Ptr) (&duration));			break;
	case 2:				return ((Ptr) (&modifier));			break;
	case 3:				return ((Ptr) (&flags));			break;
	}

	return NULL;
}


const struct olc_fields *	AffectEditable::Constraints(int arg)
{
	static const struct olc_fields constraints[] = 
	{
		{ "location", EDT_SINT32, Datatypes::Value,
		  0, 0, 0,  (const char **) apply_types
		},
		{ "duration", EDT_SINT32, Datatypes::Integer,
		  0, 0, 5000,  NULL
		},
		{ "modifier", EDT_SINT32, Datatypes::Integer,
		  0, -50, 50,  NULL
		},
		{ "flags", EDT_UINT32, Datatypes::Bitvector,
		  0, 0, 0,  (const char **) affected_bits
		},
		{ "\n", 0, 0, 0, 0, 0, NULL }
	};

	if (arg >= 0 && arg < 4)
		return &(constraints[arg]);

	return NULL;
}


namespace Property {

	const char * skill_properties[] = {
		"undefined",
		"interior",
		"openable",
		"liquid",
		"ingestable",
		"poison",
		"light",
		"magical",
	    "exit",
		"weapon",
		"fireweapon",
		"missile",
		"armor",
		"trap",
		"container",
		"note",
		"drinkcon",
		"grenade",
		"boat",
	    "board",
		"decay",
		"money",
		"saving",
		"material",
		"damage",
		"affect",
		"areas",
		"summons",
		"points",
		"unaffect",
		"creations",
		"room",
		"scripted",
		"misfire",
		"concentrate",
		"manual",
		"\n"
	};


	PropertyList::PropertyList(void)	{ }
	PropertyList::~PropertyList(void)	{ }

	PropertyList::PropertyList(const PropertyList &from)
	{
		int key;
		PropertyList	*source = const_cast<PropertyList *>(&from);
		
		for (key = Type::Undefined + 1; key < Type::MaxTypes; ++key) {
			if (source->properties.Find(key)) {
				properties[key] = source->properties[key]->Clone();
			}
		}
	}


	Property *&		PropertyList::operator[](const int &key)
	{
		static Property *nullptr;

		if (properties.Find(key)) {
			return properties[key];
		} 
		return ((nullptr = NULL));
	}


	void			PropertyList::Add(Property &adding)
	{
		int key = adding.Type();

		if (!properties.Find(key)) {
			properties[key] = &adding;
		}
	}


	void			PropertyList::Add(const int &key)
	{
		if (!properties.Find(key)) {
			switch (key) {
			case Type::Saving:		properties[key] = new Saving;			break;
			case Type::Materials:	properties[key] = new Materials;		break;
			case Type::Damage:		properties[key] = new Damage;			break;
			case Type::Affects:		properties[key] = new Affects;			break;
			case Type::Areas:		properties[key] = new Areas;			break;
			case Type::Summons:		properties[key] = new Summons;			break;
			case Type::Points:		properties[key] = new Points;			break;
			case Type::Unaffects:	properties[key] = new Unaffects;		break;
			case Type::Creations:	properties[key] = new Creations;		break;
			case Type::Room:		properties[key] = new Room;				break;
			case Type::Scripted:	properties[key] = new Scripted;			break;
			case Type::Concentrate:	properties[key] = new Concentrate;		break;
			case Type::Misfire:		properties[key] = new Misfire;			break;
			case Type::Manual:		properties[key] = new Manual;			break;
			}
		}
	}


	void			PropertyList::Remove(const int &key)
	{
		if (properties.Find(key)) {
			delete properties[key];
		}
		properties.Remove(key);
	}


	void			PropertyList::SaveDisk(FILE *fp, int indent)
	{
		int				i;
		char			*ptr = NULL,
						*buf = get_buffer(MAX_STRING_LENGTH),
						*indentation = get_buffer(indent + 2);
		Property		**property;
		MapIterator<int, Property *>	iter(properties);

		for (i = 0; i < indent; ++i)
			strcat(indentation, "\t");

		while ((property = iter.Next())) {
			fprintf(fp, "%s%s = {\n", indentation, skill_properties[(*property)->Type()]);
			(*property)->SaveDisk(fp, indent + 1);
			fprintf(fp, "%s};\n", indentation);
		}
		release_buffer(buf);
		release_buffer(indentation);
	}


	Saving::Saving(void) : formula(NULL) { }
	Saving::Saving(Saving::Saving &from) : formula(NULL)
		{	if (from.formula && *(from.formula))	formula = strdup(from.formula);		}
	Saving::~Saving(void) { }


	Ptr	 Saving::GetPointer(int arg)
	{
		switch (arg) {
		case 0:				return ((Ptr) (&formula));			break;
		}

		return NULL;
	}
	

	const struct olc_fields * Saving::Constraints(int arg)
	{
		static const struct olc_fields constraints[] = 
		{
			{ "formula", 0, Datatypes::CStringLine,
			  0, 0, 70,  NULL
			},
			{ "\n", 0, 0, 0, 0, 0, NULL }
		};

		if (arg >= 0 && arg < 1)
			return &(constraints[arg]);

		return NULL;
	}


	Materials::Materials(void) : extract(false), verbose(true) { }
	Materials::Materials(Materials::Materials &from) : components(from.components), extract(from.extract), verbose(from.verbose) { }
	Materials::~Materials(void) { }


	Ptr	 Materials::GetPointer(int arg)
	{
		switch (arg) {
		case 0:				return ((Ptr) (&extract));			break;
		case 1:				return ((Ptr) (&verbose));			break;
 		case 2:				return ((Ptr) (&components));		break;
		}

		return NULL;
	}
	

	const struct olc_fields * Materials::Constraints(int arg)
	{
		static const struct olc_fields constraints[] = 
		{
			{ "extract", 0, Datatypes::Bool,
			  0, 0, 0,  NULL
			},
			{ "verbose", 0, Datatypes::Bool,
			  0, 0, 0,  NULL
			},
			{ "components", EDT_SINT32, Datatypes::Integer + Datatypes::Vector,
			  0, 0, 0,  NULL
			},
			{ "\n", 0, 0, 0, 0, 0, NULL }
		};

		if (arg >= 0 && arg < 3)
			return &(constraints[arg]);

		return NULL;
	}


	Damage::Damage(void) :	numdice(0), sizedice(0), modifier(0), damagetype(0), range(0), 
							hitslocation(false), formula(NULL) { }
	Damage::Damage(Damage::Damage &from) :	
							numdice(from.numdice), sizedice(from.sizedice), modifier(from.modifier), 
							damagetype(from.damagetype), range(from.range), 
							hitslocation(from.hitslocation), formula(NULL) 
		{	if (from.formula && *(from.formula))	formula = strdup(from.formula);		}
	Damage::~Damage(void)  {   if (formula) { delete formula; formula = NULL; }    }


	Ptr	 Damage::GetPointer(int arg)
	{
		switch (arg) {
		case 0:				return ((Ptr) (&numdice));			break;
		case 1:				return ((Ptr) (&sizedice));			break;
		case 2:				return ((Ptr) (&modifier));			break;
		case 3:				return ((Ptr) (&damagetype));		break;
		case 4:				return ((Ptr) (&range));			break;
		case 5:				return ((Ptr) (&hitslocation));		break;
		case 6:				return ((Ptr) (&formula));			break;
		}

		return NULL;
	}
	

	const struct olc_fields * Damage::Constraints(int arg)
	{
		static const struct olc_fields constraints[] = 
		{
			{ "numdice", EDT_SINT16, Datatypes::Integer,
				0, 0, 20, NULL
			},
			{ "sizedice", EDT_SINT16, Datatypes::Integer,
				0, 0, 1000, NULL
			},
			{ "modifier", EDT_SINT16, Datatypes::Integer,
				0, -1000, 1000, NULL
			},
			{ "damagetype", EDT_SINT8, Datatypes::Value,
			  0, 0, 0,  (const char **) damage_types
			},
			{ "range", EDT_SINT8, Datatypes::Integer,
			  0, 0, 0,  NULL
			},
			{ "hitslocation", 0, Datatypes::Bool,
			  0, 0, 0,  NULL
			},
			{ "formula", 0, Datatypes::CStringLine,
			  0, 0, 70,  NULL
			},
			{ "\n", 0, 0, 0, 0, 0, NULL }
		};

		if (arg >= 0 && arg < 7)
			return &(constraints[arg]);

		return NULL;
	}


	Affects::Affects(void) : accum_affect(false), accum_duration(false), 
							 tovict(NULL), toroom(NULL), totrig(NULL), fromtrig(NULL) 
		{	affects.SetSize(0);		}
	Affects::Affects(Affects::Affects &from) : accum_affect(from.accum_affect), 
							accum_duration(from.accum_duration), 
							tovict(NULL), toroom(NULL), totrig(NULL), fromtrig(NULL) 
		{	if (from.tovict && *(from.tovict))	tovict = strdup(from.tovict);
			if (from.toroom && *(from.toroom))	toroom = strdup(from.toroom);
			if (from.totrig)	totrig = new Trigger(*(from.totrig));
			if (from.fromtrig)	fromtrig = new Trigger(*(from.fromtrig));
			affects = from.affects;		}
	Affects::~Affects(void)
	    {	if (tovict) {   delete tovict; tovict = NULL; }
	    	if (toroom) {	delete toroom; toroom = NULL; }     }

	void Affects::ToScript(Scriptable *go, Scriptable *target)
	{
		static char	field[20];

		sprintf(field, "%c%d", UID_CHAR, target->ID());
		add_var(totrig->variables, "victim", field, 0);

		script_driver(go, totrig, 0);
		totrig->variables.Clear();
	}


	void Affects::FromScript(Scriptable *go, Scriptable *target)
	{
		static char	field[20];

		sprintf(field, "%c%d", UID_CHAR, target->ID());
		add_var(fromtrig->variables, "victim", field, 0);

		script_driver(go, fromtrig, 0);
		fromtrig->variables.Clear();
	}


	Ptr	 Affects::GetPointer(int arg)
	{
		switch (arg) {
		case 0:				return ((Ptr) (&affects));			break;
		case 1:				return ((Ptr) (&accum_affect));		break;
		case 2:				return ((Ptr) (&accum_duration));	break;
		case 3:				return ((Ptr) (&tovict));			break;
		case 4:				return ((Ptr) (&toroom));			break;
		case 5:				return ((Ptr) (&totrig));			break;
		case 6:				return ((Ptr) (&fromtrig));			break;
		}

		return NULL;
	}
	

	const struct olc_fields * Affects::Constraints(int arg)
	{
		static const struct olc_fields constraints[] = 
		{
			{ "affect", 0, Datatypes::AffectEditable + Datatypes::Vector,
				0, 0, 0, NULL
			},
			{ "addaffect", EDT_SINT16, Datatypes::Bool,
			  0, 0, 0,  NULL
			},
			{ "addduration", 0, Datatypes::Bool,
			  0, 0, 0,  NULL
			},
			{ "tovict", 0, Datatypes::CStringLine,
			  0, 0, 80,  NULL
			},
			{ "toroom", 0, Datatypes::CStringLine,
			  0, 0, 80,  NULL
			},
			{ "totrig", 0, Datatypes::Pointer + Datatypes::Trigger,
			  0, 0, 0,  NULL
			},
			{ "fromtrig", 0, Datatypes::Pointer + Datatypes::Trigger,
			  0, 0, 0,  NULL
			},
			{ "\n", 0, 0, 0, 0, 0, NULL }
		};

		if (arg >= 0 && arg < 7)
			return &(constraints[arg]);

		return NULL;
	}


	Areas::Areas(void) : flags(0), how_many(1), tochar(NULL), toroom(NULL) { }
	Areas::Areas(Areas::Areas &from) : flags(from.flags), how_many(from.how_many), 
								tochar(NULL), toroom(NULL)
		{	if (from.tochar && *(from.tochar))	tochar = strdup(from.tochar);
			if (from.toroom && *(from.toroom))	toroom = strdup(from.toroom);	}
	Areas::~Areas(void)
	    {	if (tochar) {   delete tochar; tochar = NULL; }
	    	if (toroom) {	delete toroom; toroom = NULL; }     }
			
	const char * Areas::targeting_types[] = {
		"Caster",
		"CasterGroup",
		"Fighting",
		"TargetGroup",
		"FightingCaster",
		"FightingGroup",
		"Bystander",
		"MissFlying",
		"\n"
	};


	Ptr	 Areas::GetPointer(int arg)
	{
		switch (arg) {
		case 0:				return ((Ptr) (&flags));			break;
		case 1:				return ((Ptr) (&how_many));			break;
		case 2:				return ((Ptr) (&tochar));			break;
		case 3:				return ((Ptr) (&toroom));			break;
		}

		return NULL;
	}
	

	const struct olc_fields * Areas::Constraints(int arg)
	{
		static const struct olc_fields constraints[] = 
		{
			{ "flags", EDT_UINT32, Datatypes::Bitvector,
			  0, 0, 0,  (const char **) targeting_types
			},
			{ "how_many", EDT_SINT16, Datatypes::Integer,
			  0, 0, 0,  NULL
			},
			{ "tochar", EDT_UINT32, Datatypes::CStringLine,
			  0, 0, 70,  NULL
			},
			{ "toroom", EDT_UINT32, Datatypes::CStringLine,
			  0, 0, 70,  NULL
			},
			{ "\n", 0, 0, 0, 0, 0, NULL }
		};

		if (arg >= 0 && arg < 4)
			return &(constraints[arg]);

		return NULL;
	}


	Summons::Summons(void) : tochar(NULL), toroom(NULL), fail(NULL), tosummon(-1), 
							 how_many(1), corpse(false) { }
	Summons::Summons(Summons::Summons &from) : 
							tochar(NULL), toroom(NULL), fail(NULL), tosummon(from.tosummon), 
							how_many(from.how_many), corpse(from.corpse) 
		{	if (from.tochar && *(from.tochar))	tochar = strdup(from.tochar);
			if (from.toroom && *(from.toroom))	toroom = strdup(from.toroom);
			if (from.fail && *(from.fail))	fail = strdup(from.fail);			}
	Summons::~Summons(void)
	    {	if (tochar) {   delete tochar; tochar = NULL; }
	    	if (toroom) {	delete toroom; toroom = NULL;	}
	    	if (fail) 	{	delete fail; fail = NULL;		}     }


	Ptr	 Summons::GetPointer(int arg)
	{
		switch (arg) {
		case 0:				return ((Ptr) (&tochar));			break;
		case 1:				return ((Ptr) (&toroom));			break;
		case 2:				return ((Ptr) (&fail));				break;
		case 3:				return ((Ptr) (&tosummon));			break;
		case 4:				return ((Ptr) (&how_many));			break;
		case 5:				return ((Ptr) (&corpse));			break;
		}

		return NULL;
	}
	

	const struct olc_fields * Summons::Constraints(int arg)
	{
		static const struct olc_fields constraints[] = 
		{
			{ "tochar", 0, Datatypes::CStringLine,
			  0, 0, 70,  NULL
			},
			{ "toroom", 0, Datatypes::CStringLine,
			  0, 0, 70,  NULL
			},
			{ "fail", 0, Datatypes::CStringLine,
			  0, 0, 70,  NULL
			},
			{ "tosummon", EDT_VNUM, Datatypes::Integer,
			  0, 0, 0,  NULL
			},
			{ "how_many", EDT_UINT8, Datatypes::Integer,
			  0, 0, 10,  NULL
			},
			{ "corpse", 0, Datatypes::Bool,
			  0, 0, 0,  NULL
			},
			{ "\n", 0, 0, 0, 0, 0, NULL }
		};

		if (arg >= 0 && arg < 6)
			return &(constraints[arg]);

		return NULL;
	}


	Points::Points(void) :	hitnumdice(0), mananumdice(0), movenumdice(0),
	    					hitsizedice(0), manasizedice(0), movesizedice(0),
							hit(0), mana(0), move(0),
							formula(NULL), tochar(NULL), tovict(NULL), toroom(NULL) { }
	Points::Points(Points::Points &from) :
							hitnumdice(from.hitnumdice), mananumdice(from.mananumdice), movenumdice(from.movenumdice),
	    					hitsizedice(from.hitsizedice), manasizedice(from.manasizedice), movesizedice(from.movesizedice),
							hit(from.hit), mana(from.mana), move(from.move),
							formula(NULL), tochar(NULL), tovict(NULL), toroom(NULL)
		{	if (from.formula && *(from.formula))	formula = strdup(from.formula);
			if (from.tochar && *(from.tochar))	tochar = strdup(from.tochar);
			if (from.tovict && *(from.tovict))	tovict = strdup(from.tovict);
			if (from.toroom && *(from.toroom))	toroom = strdup(from.toroom);	}
	Points::~Points(void) { }


	Ptr	 Points::GetPointer(int arg)
	{
		switch (arg) {
		case 0:				return ((Ptr) (&hitnumdice));		break;
		case 1:				return ((Ptr) (&mananumdice));		break;
		case 2:				return ((Ptr) (&movenumdice));		break;
		case 3:				return ((Ptr) (&hitsizedice));		break;
		case 4:				return ((Ptr) (&manasizedice));		break;
		case 5:				return ((Ptr) (&movesizedice));		break;
		case 6:				return ((Ptr) (&hit));				break;
		case 7:				return ((Ptr) (&mana));				break;
		case 8:				return ((Ptr) (&move));				break;
		case 9:				return ((Ptr) (&formula));			break;
		case 10:			return ((Ptr) (&tochar));			break;
		case 11:			return ((Ptr) (&tovict));			break;
		case 12:			return ((Ptr) (&toroom));			break;
		}

		return NULL;
	}
	

	const struct olc_fields * Points::Constraints(int arg)
	{
		static const struct olc_fields constraints[] = 
		{
			{ "hitnumdice", EDT_SINT8, Datatypes::Integer,
				0, 0, 20, NULL
			},
			{ "mananumdice", EDT_SINT8, Datatypes::Integer,
				0, 0, 20, NULL
			},
			{ "movenumdice", EDT_SINT8, Datatypes::Integer,
				0, 0, 20, NULL
			},
			{ "hitsizedice", EDT_SINT8, Datatypes::Integer,
				0, 0, 200, NULL
			},
			{ "manasizedice", EDT_SINT8, Datatypes::Integer,
				0, 0, 200, NULL
			},
			{ "movesizedice", EDT_SINT8, Datatypes::Integer,
				0, 0, 200, NULL
			},
			{ "hit", EDT_SINT16, Datatypes::Integer,
				0, 0, 200, NULL
			},
			{ "mana", EDT_SINT16, Datatypes::Integer,
				0, 0, 200, NULL
			},
			{ "move", EDT_SINT16, Datatypes::Integer,
				0, 0, 200, NULL
			},
			{ "formula", 0, Datatypes::CStringLine,
			  0, 0, 70,  NULL
			},
			{ "tochar", EDT_UINT32, Datatypes::CStringLine,
			  0, 0, 70,  NULL
			},
			{ "tovict", EDT_UINT32, Datatypes::CStringLine,
			  0, 0, 70,  NULL
			},
			{ "toroom", EDT_UINT32, Datatypes::CStringLine,
			  0, 0, 70,  NULL
			},
			{ "\n", 0, 0, 0, 0, 0, NULL }
		};

		if (arg >= 0 && arg < 13)
			return &(constraints[arg]);

		return NULL;
	}


	Unaffects::Unaffects(void) : spellnum(0), tovict(NULL), toroom(NULL) { }
	Unaffects::Unaffects(Unaffects::Unaffects &from) : spellnum(from.spellnum), tovict(NULL), toroom(NULL)
		{	if (from.tovict && *(from.tovict))	tovict = strdup(from.tovict);
			if (from.toroom && *(from.toroom))	toroom = strdup(from.toroom);	}
	Unaffects::~Unaffects(void)
	    {	if (tovict) {   delete tovict; tovict = NULL; }
	    	if (toroom) {	delete toroom; toroom = NULL; }     }


	Ptr	 Unaffects::GetPointer(int arg)
	{
		switch (arg) {
		case 0:				return ((Ptr) (&spellnum));			break;
		case 1:				return ((Ptr) (&tovict));			break;
		case 2:				return ((Ptr) (&toroom));			break;
		}

		return NULL;
	}
	

	const struct olc_fields * Unaffects::Constraints(int arg)
	{
		static const struct olc_fields constraints[] = 
		{
			{ "spellnum", EDT_SINT16, Datatypes::SpellValue,
				0, FIRST_SPELL, FIRST_STAT - 1, (const char **) spells
			},
			{ "tovict", EDT_UINT32, Datatypes::CStringLine,
			  0, 0, 70,  NULL
			},
			{ "toroom", EDT_UINT32, Datatypes::CStringLine,
			  0, 0, 70,  NULL
			},
			{ "\n", 0, 0, 0, 0, 0, NULL }
		};

		if (arg >= 0 && arg < 3)
			return &(constraints[arg]);

		return NULL;
	}


	Creations::Creations(void) { }
	Creations::Creations(Creations &from)
	{
		for (int i = 0; i < from.objects.Count(); ++i) {
			objects.Append(from.objects[i]);
		}
	}
	Creations::~Creations(void) { }


	Ptr	 Creations::GetPointer(int arg)
	{
		switch (arg) {
		case 0:				return ((Ptr) (&objects));			break;
		}

		return NULL;
	}
	

	const struct olc_fields * Creations::Constraints(int arg)
	{
		static const struct olc_fields constraints[] = 
		{
			{ "objects", EDT_SINT16, Datatypes::Integer + Datatypes::Vector,
				0, FIRST_SPELL, FIRST_STAT - 1, (const char **) spells
			},
			{ "\n", 0, 0, 0, 0, 0, NULL }
		};

		if (arg >= 0 && arg < 1)
			return &(constraints[arg]);

		return NULL;
	}


	Room::Room(void) : tochar(NULL), toroom(NULL) { }
	Room::Room(Room &from) : tochar(NULL), toroom(NULL)
		{	if (from.tochar && *(from.tochar))	tochar = strdup(from.tochar);
			if (from.toroom && *(from.toroom))	toroom = strdup(from.toroom);	}
	Room::~Room(void)
	    {	if (tochar) {   delete tochar; tochar = NULL; }
	    	if (toroom) {	delete toroom; toroom = NULL; }     }


	Ptr	 Room::GetPointer(int arg)
	{
		switch (arg) {
		case 0:				return ((Ptr) (&affects));			break;
		case 1:				return ((Ptr) (&tochar));			break;
		case 2:				return ((Ptr) (&toroom));			break;
		}

		return NULL;
	}
	

	const struct olc_fields * Room::Constraints(int arg)
	{
		static const struct olc_fields constraints[] = 
		{
			{ "affect", 0, Datatypes::AffectEditable + Datatypes::Vector,
				0, 0, 0, NULL
			},
			{ "tochar", 0, Datatypes::CStringLine,
			  0, 0, 80,  NULL
			},
			{ "toroom", 0, Datatypes::CStringLine,
			  0, 0, 80,  NULL
			},
			{ "\n", 0, 0, 0, 0, 0, NULL }
		};

		if (arg >= 0 && arg < 3)
			return &(constraints[arg]);

		return NULL;
	}


	Scripted::Scripted(void) : trig(new Trigger) { }


	Scripted::Scripted(Scripted &from) : trig(new Trigger) 
	{
		if (from.trig && from.trig->cmdlist) {
			trig->cmdlist = new Cmdlist;
			trig->cmdlist->command = new CmdlistElement(*(from.trig->cmdlist->command));
		} else {
			trig->cmdlist = new Cmdlist("* Empty trigger.\r\n");
		}
	}


	Scripted::~Scripted(void)
	{
		if (trig && trig->cmdlist) {
			trig->cmdlist->Free();
			trig->cmdlist = NULL;	// We don't want to delete it outright.  Let Free() do the work.
		}
	}


	void Scripted::RunScript(Scriptable *go, Scriptable *victim)
	{
		static char	field[20];

		sprintf(field, "%c%d", UID_CHAR, victim->ID());
		add_var(trig->variables, "victim", field, 0);
		
		script_driver(go, trig, 0);

		trig->variables.Clear();
	}


	Ptr	 Scripted::GetPointer(int arg)
	{
		switch (arg) {
		case 0:				return ((Ptr) (&trig));			break;
		}

		return NULL;
	}
	

	const struct olc_fields * Scripted::Constraints(int arg)
	{
		static const struct olc_fields constraints[] = 
		{
			{ "script", 0, Datatypes::Pointer + Datatypes::Trigger,
			  0, 0, 0,  NULL
			},
			{ "\n", 0, 0, 0, 0, 0, NULL }
		};

		if (arg >= 0 && arg < 1)
			return &(constraints[arg]);

		return NULL;
	}


	Manual::Manual(void) : routine(-1) { }
	Manual::~Manual(void) { }
	Manual::Manual(Manual &from) : routine(from.routine) { }


	Ptr	 Manual::GetPointer(int arg)
	{
		switch (arg) {
		case 0:				return ((Ptr) (&routine));			break;
		}

		return NULL;
	}
	

	const struct olc_fields * Manual::Constraints(int arg)
	{
		static const struct olc_fields constraints[] = 
		{
			{ "routine", EDT_SINT32, Datatypes::Value,
			  0, 0, 0,  (const char **) ::ManualSpells::Names
			},
			{ "\n", 0, 0, 0, 0, 0, NULL }
		};

		if (arg >= 0 && arg < 1)
			return &(constraints[arg]);

		return NULL;
	}


	Concentrate::Concentrate(void) : wearoff(NULL), interval(1), cost(0), trig(NULL) { }
	Concentrate::Concentrate(Concentrate &from) : wearoff(NULL), interval(from.interval), 
			cost(from.cost), trig(NULL) 
		{	if (from.wearoff && *(from.wearoff))	wearoff = strdup(from.wearoff);		
			if (from.trig)		trig = new Trigger(*(from.trig));	}
	Concentrate::~Concentrate(void) { }

	void Concentrate::ConcScript(Scriptable *ch, Scriptable *target, bool &broken)
	{
		static char	field[20];
		LListIterator<TrigVarData *>	iter;
		TrigVarData *vd = NULL;

		sprintf(field, "%c%d", UID_CHAR, target->ID());
		add_var(trig->variables, "victim", field, 0);
		add_var(trig->variables, "broken", "0", 0);
		
		script_driver(ch, trig, 0);

		iter.Restart(trig->variables);

		while ((vd = iter.Next())) {
			if (!str_cmp(vd->name, "broken"))
				break;
		}

		if (vd) {
			broken = atoi(vd->value) ? true : false;
		}

		trig->variables.Clear();
	}


	Ptr	 Concentrate::GetPointer(int arg)
	{
		switch (arg) {
		case 0:				return ((Ptr) (&wearoff));			break;
		case 1:				return ((Ptr) (&interval));			break;
		case 2:				return ((Ptr) (&cost));				break;
		case 3:				return ((Ptr) (&trig));				break;
		}

		return NULL;
	}
	

	const struct olc_fields * Concentrate::Constraints(int arg)
	{
		static const struct olc_fields constraints[] = 
		{
			{ "wearoff", 0, Datatypes::CStringLine,
			  0, 0, 80,  NULL
			},
			{ "interval", EDT_SINT16, Datatypes::Integer,
				0, 0, 100, NULL
			},
			{ "cost", EDT_SINT16, Datatypes::Integer,
				0, 0, 1000, NULL
			},
			{ "trig", 0, Datatypes::Pointer + Datatypes::Trigger,
			  0, 0, 0,  NULL
			},
			{ "\n", 0, 0, 0, 0, 0, NULL }
		};

		if (arg >= 0 && arg < 4)
			return &(constraints[arg]);

		return NULL;
	}


	Misfire::Misfire(void) : vnum(-1) { }
	Misfire::~Misfire(void) { }
	Misfire::Misfire(Misfire &from) : vnum(from.vnum) { }


	Ptr	 Misfire::GetPointer(int arg)
	{
		switch (arg) {
		case 0:				return ((Ptr) (&vnum));			break;
		}

		return NULL;
	}
	

	const struct olc_fields * Misfire::Constraints(int arg)
	{
		static const struct olc_fields constraints[] = 
		{
			{ "vnum", EDT_VNUM, Datatypes::Integer,
			  0, 0, 0,  NULL
			},
			{ "\n", 0, 0, 0, 0, 0, NULL }
		};

		if (arg >= 0 && arg < 1)
			return &(constraints[arg]);

		return NULL;
	}


	namespace Type {
		const char * Names[] = {
			"Undefined",

			"Interior",

			"Openable",
			"Liquid",
			"Ingestable",
			"Poison",
			"Light",
			"Magical",
			"Exit",
			"Weapon",
			"Fireweapon",
			"Missile",
			"Armor",
			"Trap",
			"Container",
			"Note",
			"Drinkcon",
			"Grenade",
			"Boat",
			"Board",
			"Decay",
			"Money",

			"Saving",
			"Materials",
			"Damage",
			"Affects",
			"Areas",
			"Summons",
			"Points",
			"Unaffects",
			"Creations",
			"Room",
			"Scripted",
			"Misfire",
			"Concentrate",
			"Manual",

			"\n"
		};

	}

#if 0
	Interior::Interior(void) : description(NULL), ex_description(NULL), room_flags(0),
	    				 sector_type(0), light(0), room_affections(0) {}
	Interior::~Interior(void) {}

	Openable::Openable(void) : maxWeight(0), state(0), key(0), corpseOf(0) {}
	Openable::~Openable(void) {}

	Liquid::Liquid(void) : amount(0), color(0) {}
	Liquid::~Liquid(void) {}

	Ingestable::Ingestable(void) : modHunger(0), modThirst(0), modDrunk(0) {}
	Ingestable::~Ingestable(void) {}

	Poison::Poison(void) : severity(0), duration(0), affects(0) {}
	Poison::~Poison(void) {}

	Light::Light(void) : hours(0) {}
	Light::~Light(void) {}

	Magical::Magical(void) : useHow(0), level(0), charges(0)
	    {  spells[0] = 0;  spells[1] = 0;  spells[2] = 0;  spells[3] = 0;  }
	Magical::~Magical(void) {}

	Exit::Exit(void) : direction(0), to_room(0) {}
	Exit::~Exit(void) {}


	Weapon::Weapon(void) : proficiency(0), swingbonus(0), swingdamtype(0), swingmessage(0),
	    					thrustbonus(0), thrustdamtype(0), thrustmessage(0), speed(0) { }
	Weapon::~Weapon(void) { }


	FireWeapon::FireWeapon(void) : proficiency(0), toHit(0), damBonus(0), damType(0),
	    							ammo(0), maxAmmo(0), range(0), speed(0) { }
	FireWeapon::~FireWeapon(void) { }


	Missile::Missile(void) : proficiency(0), toHit(0), damBonus(0), damType(0), ammo(0), maxAmmo(0) { }
	Missile::~Missile(void) { }


	Armor::Armor(void) :   SInt16 plusAttack(0), SInt8 damBonus(0), SInt8 damType(0),
	    					UInt8 layer(0), Flags covers(0) { }
	Armor::~Armor(void) { }


	Container::Container(void) : flags(0), maxWeight(0), pick_modifier(0), scabbardOf(0) { }
	Container::~Container(void) { }


	Note::Note(void) : writing(NULL), language(0) { }
	Note::~Note(void) { }


	DrinkCon::DrinkCon(void) : flags(0), maxWeight(0) { }
	DrinkCon::~DrinkCon(void) { }


	Grenade::Grenade(void) : countdown(0), explodes(NULL) { }
	Grenade::~Grenade(void) { }


	Boat::Boat(void) {}
	Boat::~Boat(void) {}


	Money::Money(void) : amount(0), issue(0) {}
	Money::~Money(void) {}


	Decay::Decay(void) : time(0) {}
	Decay::~Decay(void) {}


	Board::Board(void) : time(0) {}
	Board::~Board(void) {}


	Saving::Saving(void) : str(NULL) { }
	Saving::~Saving(void) { }


	Materials::Materials(void) { }
	Materials::~Materials(void) { }
#endif


}
