/* ************************************************************************
*   File: property.h      	                            Part of TLO		  *
*  Usage: header file for properties					                  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author: sfn $
*  $Date: 2001/02/04 15:57:58 $
*  $Revision: 1.8 $
************************************************************************ */

//	Properties are designed to take the place of object types and possibly
//	extend into other types.

#ifndef __PROPERTY_H__
#define __PROPERTY_H__

#include "types.h"
#include "mud.base.h"
#include "affects.h"
#include "scripts.h"

#define PROPERTY(spellnum, type) static_cast<Property::type *> (Skills[spellnum]->properties[Property::Type::type])


class RoomInterior;
class Trigger;
class Script;
class SkillRef;

namespace ManualSpells {
	extern const char * Names[];
}


class AffectEditable : public Editable
{
public:
					AffectEditable(void);
					AffectEditable(SInt32 mod, SInt32 d, Affect::Location loc, Flags bv);

	virtual Ptr							GetPointer(int arg);
	virtual const struct olc_fields *	Constraints(int arg);

	int			location;		//	Tells which ability to change(APPLY_XXX)
	int			duration;		//	Tells which ability to change(APPLY_XXX)
	int			modifier;		//	This is added to apropriate ability
	Flags		flags;			//	Tells which bits to set (AFF_XXX)
};


namespace Property
{
	namespace Type {
	enum Types {
		Undefined,					// De facto

		Interior,					// For rooms

		Openable,					// For objects
		Liquid,
		Ingestable,
		Poison,
		Light,
		Magical,
	    Exit,
		Weapon,
		Fireweapon,
		Missile,
		Armor,
		Trap,
		Container,
		Note,
		Drinkcon,
		Grenade,
		Boat,
	    Board,
		Decay,
		Money,

		Saving,						// For spells
		Materials,
		Damage,
		Affects,
		Areas,
		Summons,
		Points,
		Unaffects,
		Creations,
		Room,
		Scripted,
		Misfire,
		Concentrate,
		Manual,

		MaxTypes					// Just to be sure.
	};
	
		extern const char * Names[];
	}

	class Property : public ::Editable
	{
	public:
		Property(void) { }
		virtual ~Property(void) { }

		virtual void			Parse(char *input) = 0;
		virtual Type::Types 	Type(void)						{	return Type::Undefined;	}
		const char *			Name(void)						{	return Type::Names[Type()];	}

		virtual Property *		Clone(void) = 0;

		// virtual void			Display(void) = 0;
		// virtual void			Set(void) = 0;
		// virtual void			ChangeQuality(SInt8 mod) = 0;
	};
	

	class PropertyList
	{
	public:
		PropertyList(void);
		PropertyList(const PropertyList &from);
		~PropertyList(void);
	
		Property *&			operator[](const int &key);

		void				Add(Property &adding);
		void				Add(const int &key);
		void				Remove(const int &key);

		void				SaveDisk(FILE *fp, int indent = 1);
		 
	private:
	
		Map<int, Property::Property *>	properties;
	};


	// Properties for skills / spells
	class Saving : public Property
	{
	public:
		Saving(void);
		Saving(Saving &from);
		virtual ~Saving(void);

		void	Parse(char *input);

		Type::Types 	Type(void)						{	return Type::Saving;	}
		virtual Property *Clone(void)					{	return new Saving(*this);	}

		char	*formula;
		Ptr		GetPointer(int arg);
		const struct olc_fields * Constraints(int arg);
	};


	class Materials : public Property
	{
	public:
		Materials(void);
		Materials(Materials &from);
		virtual ~Materials(void);

		void	Parse(char *input);

		Type::Types 	Type(void)						{	return Type::Materials;	}
		virtual Property *Clone(void)					{	return new Materials(*this);	}
		
		Vector<int>	components;
		bool 	extract;
		bool	verbose;

		Ptr		GetPointer(int arg);
		const struct olc_fields * Constraints(int arg);
	};


	class Damage : public Property
	{
	public:
		Damage(void);
		Damage(Damage &from);
		virtual ~Damage(void);

		void	Parse(char *input);

		Type::Types 	Type(void)						{	return Type::Damage;	}
		virtual Property *Clone(void)					{	return new Damage(*this);	}

		SInt16	numdice;
		SInt16	sizedice;
		SInt16	modifier;

		SInt8	damagetype;
		UInt8	range;
		bool	hitslocation;

		char	*formula;

		Ptr		GetPointer(int arg);
		const struct olc_fields * Constraints(int arg);
	};

	class Affects : public Property
	{
	public:
		Affects(void);
		Affects(Affects &from);
		virtual ~Affects(void);

		void	Parse(char *input);

		Type::Types 	Type(void)						{	return Type::Affects;	}
		virtual Property *Clone(void)					{	return new Affects(*this);	}

		Vector<AffectEditable>	affects;
		
		bool	accum_affect;
		bool	accum_duration;
		
		char	*tovict;
		char	*toroom;

		Trigger			* totrig;
		Trigger			* fromtrig;

		void			ToScript(Scriptable *go, Scriptable *target);
		void			FromScript(Scriptable *ch, Scriptable *target);

		Ptr		GetPointer(int arg);
		const struct olc_fields * Constraints(int arg);
	};


	class Areas : public Property
	{
	public:
		Areas(void);
		Areas(Areas &from);
		virtual ~Areas(void);

		void	Parse(char *input);

		static const char * targeting_types[];

		Type::Types 	Type(void)						{	return Type::Areas;	}
		virtual Property *Clone(void)					{	return new Areas(*this);	}
		
		enum {
			Caster,
			CasterGroup,
			Fighting,
			TargetGroup,
			FightingCaster,
			FightingGroup,
			Bystander,
			MissFlying,
			MAX
		};

		Flags	flags;
		SInt16	how_many;

		char	*tochar;
		char	*toroom;

		Ptr		GetPointer(int arg);
		const struct olc_fields * Constraints(int arg);
	};


	class Summons : public Property
	{
	public:
		Summons(void);
		Summons(Summons &from);
		virtual ~Summons(void);
		
		void	Parse(char *input);

		Type::Types 	Type(void)						{	return Type::Summons;	}
		virtual Property *Clone(void)					{	return new Summons(*this);	}

		char	*tochar;
		char	*toroom;
		char	*fail;
		
		VNum	tosummon;
		UInt8	how_many;
		bool	corpse;

		Ptr		GetPointer(int arg);
		const struct olc_fields * Constraints(int arg);
	};


	class Points : public Property
	{
	public:
		Points(void);
		Points(Points &from);
		virtual ~Points(void);

		void	Parse(char *input);

		Type::Types 	Type(void)						{	return Type::Points;	}
		virtual Property *Clone(void)					{	return new Points(*this);	}
		
		SInt8	hitnumdice;
		SInt8	mananumdice;
		SInt8	movenumdice;

		SInt8	hitsizedice;
		SInt8	manasizedice;
		SInt8	movesizedice;

		SInt16	hit;
		SInt16	mana;
		SInt16	move;

		char	*formula;
		char	*tochar;
		char	*tovict;
		char	*toroom;

		Ptr		GetPointer(int arg);
		const struct olc_fields * Constraints(int arg);
	};


	class Unaffects : public Property
	{
	public:
		Unaffects(void);
		Unaffects(Unaffects &from);
		virtual ~Unaffects(void);

		void	Parse(char *input);

		Type::Types 	Type(void)						{	return Type::Unaffects;	}
		virtual Property *Clone(void)					{	return new Unaffects(*this);	}
		
		VNum	spellnum;
		char	*tovict;
		char	*toroom;

		Ptr		GetPointer(int arg);
		const struct olc_fields * Constraints(int arg);
	};


	class Creations : public Property
	{
		public:
		Creations();
		Creations(Creations &from);
		virtual ~Creations();
		
		void	Parse(char *input);
		Type::Types 	Type(void)						{	return Type::Creations;	}
		virtual Property *Clone(void)					{	return new Creations(*this);	}

		Vector<int>		objects;

		Ptr		GetPointer(int arg);
		const struct olc_fields * Constraints(int arg);
	};


	class Room : public Property
	{
	public:
		Room(void);
		Room(Room &from);
		virtual ~Room(void);

		void	Parse(char *input);

		Type::Types 	Type(void)						{	return Type::Room;	}
		virtual Property *Clone(void)					{	return new Room(*this);	}

		Vector<AffectEditable>	affects;
		
		char	*tochar;
		char	*toroom;

		Ptr		GetPointer(int arg);
		const struct olc_fields * Constraints(int arg);
	};


	class Scripted : public Property
	{
		public:
		Scripted();
		Scripted(Scripted &from);
		virtual ~Scripted();
		
		void	Parse(char *input);

		Type::Types 	Type(void)						{	return Type::Scripted;	}
		virtual Property *Clone(void)					{	return new Scripted(*this);	}

		Trigger			* trig;
		
		void			RunScript(Scriptable *go, Scriptable *victim);

		Ptr		GetPointer(int arg);
		const struct olc_fields * Constraints(int arg);
	};


	class Concentrate : public Property
	{
	public:
		Concentrate(void);
		Concentrate(Concentrate &from);
		virtual ~Concentrate(void);

		void	Parse(char *input);

		Type::Types 	Type(void)						{	return Type::Concentrate;	}
		virtual Property *Clone(void)					{	return new Concentrate(*this);	}

		char	* wearoff;
		SInt16	interval;
		SInt16	cost;	
		Trigger			* trig;
		
		void			ConcScript(Scriptable *ch, Scriptable *target, bool &broken);

		Ptr		GetPointer(int arg);
		const struct olc_fields * Constraints(int arg);
	};


	class Misfire : public Property
	{
		public:
		Misfire();
		Misfire(Misfire &from);
		virtual ~Misfire();
		
		void	Parse(char *input);

		Type::Types 	Type(void)						{	return Type::Misfire;	}
		virtual Property *Clone(void)					{	return new Misfire(*this);	}

		VNum			vnum;

		Ptr		GetPointer(int arg);
		const struct olc_fields * Constraints(int arg);
	};


	class Manual : public Property
	{
		public:
		Manual();
		Manual(Manual &from);
		virtual ~Manual();
		
		void	Parse(char *input);

		Type::Types 	Type(void)						{	return Type::Manual;	}
		virtual Property *Clone(void)					{	return new Manual(*this);	}

		SInt32	routine;

		Ptr		GetPointer(int arg);
		const struct olc_fields * Constraints(int arg);
	};


#if 0
	class Interior : public Property, public RoomInterior
	{
	private:
		SString *description;						// When in room									 
		struct extra_descr_data *ex_description; 	// for examine/look			 
	
		LList <Object *>		contents;
		LList <Character *>		people;
	
		Flags 					room_flags;		// DEATH,DARK ... etc								 
		UInt8					sector_type;	// sector type (move/hide)						
		SInt8					light;			// Number of lightsources in room		 

		Flags room_affections;					// Bitvector for room affections			

		LList<std::Affect *>	affected;			//	affects

	public:

		Interior(void);
		~Interior(void);
		
		Type::Types 	Type(void)						{	return Type::Interior; }
	};
		
		
	class Openable : public Property
	{
	public:
		Openable(void);
		~Openable(void);
		
		Type::Types 	Type(void)						{	return Type::Openable; }

	private:
	UInt16 maxWeight;
    Flags state;
    VNum key;
    VNum corpseOf;
    
	};


	class Liquid : public Property
	{
	public:
		Liquid(void);
		~Liquid(void);

		Type::Types 	Type(void)						{	return Type::Liquid; }

	private:
	SInt16 amount;
	    SInt8			color;
	};


	class Ingestable : public Property
	{
	public:
		Ingestable(void);
		~Ingestable(void);

		Type::Types 	Type(void)						{	return Type::Ingestable; }

	private:
 	SInt8 modHunger;
    SInt8 modThirst;
    SInt8 modDrunk;
	};


	class Poison : public Property
	{
	private:
	SInt8 severity;
    UInt8 duration;
    Flags affects;
	public:
		Poison(void);
		~Poison(void);

		Type::Types 	Type(void)						{	return Type::Poison; }

	};


	class Light : public Property
	{
	private:
		SInt8 hours;

	public:
		Light(void);
		~Light(void);

		Type::Types 	Type(void)						{	return Type::Light; }

	};


	class Magical : public Property
	{
	private:
	Flags useHow;
	UInt16 level;
    SInt16 spells[4];
    SInt16 charges;
	public:
		Magical(void);
		~Magical(void);

		Type::Types 	Type(void)						{	return Type::Magical; }

	};


	class Exit : public Property
	{
	private:
	SInt8 direction;
	    VNum	to_room;
	public:
		Type::Types 	Type(void)						{	return Type::Exit; }

	};


	class Weapon : public Property
	{
	private:
	SInt8 proficiency;
    SInt8 swingbonus;
    SInt8 swingdamtype;
    SInt8 swingmessage;
    SInt8 thrustbonus;
    SInt8 thrustdamtype;
    SInt8 thrustmessage;
    SInt8 speed;
	public:
		Type::Types 	Type(void)						{	return Type::Weapon; }

	};


	class FireWeapon : public Property
	{
	private:
	SInt8 proficiency;
    SInt8 toHit;
    SInt8 damBonus;
    SInt8 damType;
    SInt8 ammo;
    SInt8 maxAmmo;
    SInt8 range;
    SInt8 speed;
	public:
		Type::Types 	Type(void)						{	return Type::Fireweapon; }

	};


	class Missile : public Property
	{
	private:
	SInt8 proficiency;
    SInt8 toHit;
    SInt8 damBonus;
    SInt8 damType;
    SInt8 ammo;
    SInt8 maxAmmo;
	public:
		Type::Types 	Type(void)						{	return Type::Missile; }

	};


	class Armor : public Property
	{
	private:
    SInt16 plusAttack;
    SInt8 damBonus;
    SInt8 damType;
    UInt8 layer;
    Flags covers;
	public:
		Type::Types 	Type(void)						{	return Type::Armor; }

	};


	class Container : public Property
	{
	private:
    Flags flags;
    SInt16 maxWeight;
    SInt8 pick_modifier;
    SInt8 scabbardOf;
	public:
		Type::Types 	Type(void)						{	return Type::Container; }

	};


	class Note : public Property
	{
	public:
		Type::Types 	Type(void)						{	return Type::Note; }

		Note(void);
		~Note(void);

	private:
	SString *writing;
	SInt8 language;
	};


	class DrinkCon : public Property
	{
	private:
    Flags flags;
    SInt16 maxWeight;
	public:
		Type::Types 	Type(void)						{	return Type::Drinkcon; }

	};


	class Grenade : public Property
	{
	public:
		Type::Types 	Type(void)						{	return Type::Grenade; }

		Grenade(void);
		~Grenade(void);
	private:
	UInt32 countdown;
    Dice damage;
	    Event	*explodes;
	};


	class Boat : public Property
	{
	private:
	public:
		Type::Types 	Type(void)						{	return Type::Boat; }

	};


	class Money : public Property
	{
	private:
	UInt32 amount;
		VNum	issue;	
	public:
		Type::Types 	Type(void)						{	return Type::Money; }

	};


	class Decay : public Property
	{
	private:
	UInt32 time;
	public:
		Type::Types 	Type(void)						{	return Type::Decay; }

	};


	class Board : public Property
	{
	private:
	UInt32 time;
	public:
		Type::Types 	Type(void)						{	return Type::Board; }

	};
#endif


}




#endif

