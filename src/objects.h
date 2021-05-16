/*************************************************************************
*   File: objects.h                  Part of Aliens vs Predator: The MUD *
*  Usage: Header file for objects                                        *
*************************************************************************/


#ifndef	__OBJECTS_H__
#define __OBJECTS_H__

#include "types.h"
#include "structs.h"
#include "index.h"
#include "object.defs.h"
#include "mud.base.h"
#include "affects.h"
#include "find.h"
// #include "oedit.h"
#include "spec.h"

#include "stl.llist.h"
#include "stl.map.h"


class ExtraDesc;
class Event;


class Object : public MUDObject, public Editable {
public:
	class AffectMod {
	public:
		AffectMod(void) : location(::Affect::None), modifier(0) { }	
		::Affect::Location	location;		//	Which ability to change
		SInt16				modifier;		//	How much it changes by
	};

public:
						Object(void);
						Object(const Object &proto);
						~Object(void);
	
	virtual Type		DataType(void) const;

	//	MUDObject
	virtual void		Extract(void);
	virtual void		ToRoom(VNum room);
	virtual void		FromRoom(void);
	VNum				AbsoluteRoom(void) const;
	

	void				ToChar(Character *ch);
	void				FromChar(void);
	void				ToObj(Object *obj_to);
	void				FromObj(void);
	void				Unequip(void);
	void				Equip(Character *ch);
	
	void				Update(UInt32 use);
	
	bool				Load(FILE *fl, char *filename, Character *ch = NULL);
	void				Save(FILE *fl, SInt32 location, Character *ch = NULL);
	
	SInt32				TotalValue(void);
	void				ExtractNoKeep(Object *corpse, Character *ch);
	
//	void				DisplayStat(Character *ch);

	Character *			CarriedBy(void) const;
	Character *			WornBy(void) const;
	Object *			InObj(void) const;

	void 				AddRider(Character * ch);
	
//	UNIMPLEMENTED:	Replace bool same_obj(Object *, Object *) with this
//	bool				operator==(const Object &) const;
	
//	VNum				number;					//	Where in data-base

	SString *			actionDesc;				//	What to write when used
	LList<ExtraDesc *>	exDesc;					//	extra descriptions

	// Character *			carried_by;				//	Carried by - NULL in room/container
	// Character *			worn_by;				//	Worn by?
	SInt8				worn_on;				//	Worn where?
	// Object *			in_obj;					//	In what object NULL when none

	//	Object information
	SInt32				value[NUM_OBJ_VALUES];	//	Values of the item
	SInt32				cost;					//	Cost of the item
	SInt32				weight;					//	Weight of the item
	UInt32				timer;					//	Timer
	SInt16				hit;
	SInt16				max_hit;
	Type				type;					//	Type of item
	Flags				wear;					//	Wear flags
	Flags				extra;					//	Extra flags (glow, hum, etc)
	
	AffectMod			affectmod[MAX_OBJ_AFFECT];  /* affects */

	void 				AffectModify(Affect::Location loc, SInt8 mod, Flags bitv, bool add);
	
	static void 		SaveDisk(Object *obj, FILE *fp);

friend void name_from_drinkcon(Object * obj);
friend void name_to_drinkcon(Object * obj, int type);

public:
//	static RNum			Real(VNum vnum);
	static bool			Find(VNum vnum);
	static Object *		Read(VNum nr);

	static Index<Object>	_Index;
	
	static Object *		Create(void);
	void				Parser(char *input);

//	ScriptObject Overrides
public:
	virtual Character *		TargetChar(const char *arg) const;
	virtual Character *		TargetCharRoom(const char *arg) const;
	virtual Object *		TargetObj(const char *arg) const;
	virtual Object *		TargetObjList(const char * arg, LList<Object *> &list) const;
	virtual Room *			TargetRoom(const char *arg) const;

	virtual void 			AtCommand(VNum target, char * argument);

	enum Vars {
		ObjKeywords,
		ObjName,
		ObjSDesc,
		ObjADesc,
		ObjExtraFlags, 
		ObjWearFlags, 
		ObjAffFlags, 
		ObjType,
		ObjWeight,
		ObjCost,
		ObjTimer,
		ObjMaterial,
		ObjPD,
		ObjDR,
		ObjHit,
		ObjMaxHit,
		ObjVal1,
		ObjVal2,
		ObjVal3,
		ObjVal4,
		ObjVal5,
		ObjVal6,
		ObjVal7,
		ObjVal8,
		MAXVAR
	};

	Ptr	GetPointer(int arg);
	virtual const struct olc_fields * Constraints(int arg);
};

inline Type		Object::DataType(void) const			{ return Datatypes::Object; }

inline Character * Object::TargetChar(const char * arg) const
{
	if (*arg == UID_CHAR)
	return get_char(arg);
	else
	return get_char_by_obj(this, arg); 
};
inline Character * Object::TargetCharRoom(const char * arg) const
{
	if (*arg == UID_CHAR)
	return get_char(arg);
	else
	return get_char_by_obj(this, arg); 
};

inline Object * Object::TargetObj(const char * arg) const
{
	if (*arg == UID_CHAR)
	return get_obj(arg); 
	else
	return get_obj_by_obj(this, arg); 
};
inline Object * Object::TargetObjList(const char * arg, LList<Object *> &list) const
{ 
	if (*arg == UID_CHAR)
	return get_obj(arg); 
	else
	return get_obj_by_obj(this, arg); 
};

extern LList<Object *>	Objects;
extern LList<Object *>	PurgedObjs;

void equip_char(Character * ch, Object * obj, UInt8 pos);
Object *unequip_char(Character * ch, UInt8 pos);


#endif	// __OBJECTS_H__

