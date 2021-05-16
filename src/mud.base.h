
#ifndef __MUD_BASE_H__
#define __MUD_BASE_H__

#include "types.h"
#include "stl.llist.h"
#include "stl.map.h"
#include "affects.h"
#include "safeptr.h"
#include <list>

extern IDNum max_id;

const VNum NOWHERE = -1;

class Character;
class Object;
class Script;
class Room;
class GameData;
class MUDObject;
class Affect;
class Concentration;
class Event;
class Descriptor;

#ifdef GOT_RID_OF_IT
#ifndef TO_ROOM
#define TO_ROOM		(1 << 0)
#define TO_VICT		(1 << 1)
#define TO_NOTVICT	(1 << 2)
#define TO_CHAR		(1 << 3)
#define TO_NOTVICTROOM	(1 << 4)
#define TO_ZONE		(1 << 5)
#define TO_GAME		(1 << 6)
#define TO_TRIG		(1 << 7)
#define TO_SLEEP	(1 << 8)	/* to char, even if sleeping */
#endif
#endif

// Material types, imported from NetHack 3.1.3 - thanks NH dev team! :)  -DH
namespace Materials {
	enum Material {
		Undefined,
		Liquid,
		Wax,
		Veggy,
		Flesh,
		Paper,
		Cloth,
		Leather,
		Wood,
		Bone,
		DragonHide,
		Iron,
		Metal,
		Copper,
		Silver,
		Gold,
		Platinum,
		Mithril,
		Titanium,
		Plastic,
		Glass,
		Gemstone,
		Mineral,
		Shadow,
		Ether,
		Steel,
		Adamant,
		Diamond,
		Porcelain,
		Ceramic,
		Jelly,
		Vapor,
		DarkSteel,
		NUM_MATERIALS
	};
}

//	Base class
class Base : public SafePtrList {
public:
							Base(VNum v = -1)  : vnum(v), purged(false), id(0) { };
	virtual					~Base(void) { };

	// This could be replaced by RTTI.  I'd spring for it if 
    // RTTI was really stable as of now. -DH
	virtual Type			DataType(void) const;

	bool					Purged(void) const;
	VNum					Virtual(void) const;
	IDNum					ID(void) const;
	VNum					Virtual(VNum newVNum);
	
protected:
	bool					Purged(bool purge);
	IDNum					ID(IDNum newId);
	VNum					vnum;

private:
	bool					purged;
	IDNum					id;
};


inline Type		Base::DataType(void) const			{ return Datatypes::Base; }

inline VNum		Base::Virtual(void) const			{ return vnum;				}
inline VNum		Base::Virtual(VNum newVNum)			{ return (vnum = newVNum);	}
inline IDNum	Base::ID(void) const				{ return id;				}
inline IDNum	Base::ID(IDNum newId)				{ return (id = newId);		}
inline bool		Base::Purged(void) const			{ return purged;			}
inline bool		Base::Purged(bool purge)			{
	if (purge)	purged = purge;
	return purged;
}


class Editable {
public:
	virtual				~Editable(void) = 0;
	virtual Ptr			GetPointer(int arg)
		{ return NULL; }
	virtual	void 		Print(Descriptor *d, int i, char *buf);

	int 				Edit(Descriptor *d, char *command, int menu, char *argument);
	virtual void		SaveDisk(FILE *fp, int indent = 1);
	void 				SaveByConstraint(FILE *fp, const struct olc_fields *constraint, void *ptr, int indent = 1);

	virtual const struct olc_fields * Constraints(int arg)
		{ return NULL; }
};


//	Scriptable Object Interface
class Scriptable : public Base {
public:
							Scriptable(VNum vnum = -1);
							Scriptable(const Scriptable &scriptObj);
	virtual					~Scriptable(void);
	
	virtual Type			DataType(void) const;

	virtual ::Character *	TargetChar(const char *arg) const;
	virtual ::Character *	TargetCharRoom(const char *arg) const;
	virtual ::Object *		TargetObj(const char *arg) const;
	virtual ::Object *		TargetObjList(const char * arg, LList<Object *> &list) const;
	virtual ::Room *		TargetRoom(const char * arg) const;
	
	virtual const char * 	Name(void) const;
	virtual	const char *	CalledName(const Character *ch) const { return Name(); }
	virtual const SString * SSName(void) const;
	virtual const char * 	UID(void) const;

	inline virtual VNum		InRoom(void) const;
	inline virtual VNum		AbsoluteRoom(void) const;

	virtual void 			Command(char * argument, int cmd = -1);
	virtual void 			AtCommand(VNum target, char * argument) { }
   
	virtual void			Echo(char * arg) const { }
	virtual void			EchoAt(char * arg, GameData * target) const { }
	virtual void			EchoAround(char * arg, GameData * target) const { }
	virtual void			EchoDistance(char * arg, GameData * target) const { }
	virtual void			EchoZone(char * arg, int tozone = -1) const { }

	std::list<Event *>			events;					//	List of events
	virtual void			EventCleanup(void);

public:
	Script *				script;
};


inline			Scriptable::Scriptable(VNum vnum) : Base(vnum), script(NULL) { }
inline			Scriptable::Scriptable(const Scriptable &so) : Base(so), script(so.script) { };
inline			Scriptable::~Scriptable(void) { }

inline Type		Scriptable::DataType(void) const			{ return Datatypes::Scriptable; };

inline Character *Scriptable::TargetChar(const char *arg) const	{ return NULL;		}
inline Character *Scriptable::TargetCharRoom(const char *arg) const	{ return NULL;		}
inline Object *	Scriptable::TargetObj(const char *arg) const	{ return NULL;		}
inline Object *	Scriptable::TargetObjList(const char *arg, LList<Object *> &list) const	{ return NULL;	}
inline Room *	Scriptable::TargetRoom(const char *arg) const	{ return NULL;		}

inline const char * 	Scriptable::Name(void) const { return NULL; }
inline const SString *	Scriptable::SSName(void) const { return NULL; }

inline VNum Scriptable::InRoom(void) const  		{ return NOWHERE;		}
inline VNum Scriptable::AbsoluteRoom(void) const	{ return NOWHERE;		}


// This is our guaranteed ancestor of rooms, objects, mobiles, etc.
// CHANGEPOINT:  We'll probably want to fill in Inside relationships later.
class GameData : public Scriptable {
public:
							GameData(VNum vnum = -1);
							GameData(const GameData &scriptObj);
	virtual					~GameData(void);

	virtual Type			DataType(void) const;

	virtual const char * 	Keywords(const char *def = NULL) const;
	virtual const char *	Name(const char *def = NULL) const;
	virtual void			SetName(const char * val = NULL);
	virtual void			SetName(SString * val = NULL);
	virtual const SString * SSName(const SString * def = NULL) const;

//	virtual void			DisplayStat(Character *ch);
//	virtual void			ScriptStat(Character *ch);

	virtual bool			CanBeSeen(const Character *viewer) const;
    virtual const GameData *		Inside(void) const { return NULL; }
	virtual int				Zone(void) const = 0;

	virtual void			Echo(char * arg) const { };
	virtual void			EchoAt(char * arg, GameData * target) const { };
	virtual void			EchoAround(char * arg, GameData * target) const { };
	virtual void			EchoDistance(char * arg, GameData * target) const { };
	virtual void			EchoZone(char * arg, int tozone = -1) const;

	virtual SInt32 			Send(const char *messg);
	// virtual SInt32			Send(const char *messg, ...);

	LList<Affect *>			affected;			//	affects
	LList<Object *>			contents;				//	List of items in room
	Flags					affectbits;				//	Affect flags

	virtual void			AffectTotal(void) { }
	virtual	void 			AffectModify(Affect::Location loc, SInt8 mod, Flags bitv, bool add);

protected:
	SString *				name;					//	Title of object: get, etc

friend void nanny(Descriptor *d, char *arg);
    
};

inline			GameData::GameData(VNum vnum) : Scriptable(vnum), affectbits(0), name(NULL) { }
inline			GameData::GameData(const GameData &so) : Scriptable(so), 
							affectbits(so.affectbits), name(so.name->Share()) { };
// inline			GameData::~GameData(void) { SSFree(name); }
inline Type		GameData::DataType(void) const			{ return Datatypes::GameData; }
inline const char * 	GameData::Keywords(const char *def) const 
	{ return (name ? name->Data() : def); }
inline const char * 	GameData::Name(const char *def) const 
	{ return (name ? name->Data() : def); }
inline void				GameData::SetName(const char *val) 
	{	if (name)	SSFree(name);
		name = NULL;
		if (val)	name = SString::Create(val); }
inline void			 	GameData::SetName(SString *val) 
	{	if (name)	SSFree(name);
		name = NULL;
		if (val)	name = val->Share();	}
inline const SString * 	GameData::SSName(const SString *def) const 
	{ return (name ? name : def);	}
// inline SInt32	GameData::Send(const char *messg, ...)
// 	{	return 0;	}
inline SInt32	GameData::Send(const char *messg)
	{	return 0;	}


//	MUDObject Interface
class MUDObject : public GameData {
public:
							MUDObject(VNum vnum = -1);
							MUDObject(const MUDObject &mo);
	virtual					~MUDObject(void);
							
	virtual Type			DataType(void) const;

	virtual VNum    		InRoom(void) const;
	void					SetRoom(VNum theRoom);
	virtual void			FromRoom(void);
	virtual void			ToRoom(VNum theRoom);
	virtual VNum			AbsoluteRoom(void) const;
	
	virtual int				Zone(void) const;
	virtual bool			SameRoom(MUDObject * other) const;

	virtual const GameData *		Inside(void) const;
	virtual const GameData *		AbsoluteInside(void) const;

	virtual void			Extract(void);
	
	virtual const char * 	Keywords(const char *def = NULL) const;
	virtual void		 	SetKeywords(const char *def = NULL) ;
	virtual void		 	SetKeywords(SString *def = NULL) ;
	virtual const SString * SSKeywords(const SString *def = NULL) const;
	virtual const char * 	SDesc(const char *def = NULL) const;
	virtual void			SetSDesc(const char *def = NULL);
	virtual void			SetSDesc(SString *def = NULL);
	virtual const SString * SSSDesc(const SString *def = NULL) const;
	virtual	const char *	CalledName(const Character *ch) const;
	virtual	const char *	CalledKeywords(const Character *ch) const;

	virtual void			Echo(char * arg) const;
	virtual void			EchoAt(char * arg, GameData * target) const;
	virtual void			EchoAround(char * arg, GameData * target) const;
	virtual void			EchoDistance(char * arg, GameData * target) const;

	UInt16					Invis(void);
	void					SetInvis(UInt16 num);
	virtual bool			CanBeSeen(const Character *viewer) const;
	virtual bool			Fall(VNum toroom, int prev);

	//	virtual SInt32			Damage(Character *attacker, SInt32 damage, SInt32 type, UInt32 times);
	//	virtual void			ToRoom(const MUDObject &with);

    Materials::Material		material;

	UInt16					invis_level;
	UInt16					weight;

    SInt8					passive_defense;
    SInt8					damage_resistance;
	UInt8					max_riders;

	SString *				keywords;
	SString *				shortDesc;

	LList<Character *>		riders;			//	List of chars riding
	
	Character *				Rider(void);
	virtual void			AddRider(Character * ch);
	virtual void			RemoveRider(Character * ch, int type);

protected:
	VNum					room;			//	Location (real room number)
	GameData *				inside;
	
// 	friend class Object;					// This is done to overcome issues
// 	friend class Character;					// with contents list manipulation.
 	friend void equip_char(Character * ch, Object * obj, UInt8 pos);
 	friend Object *unequip_char(Character * ch, UInt8 pos);

};


//inline			MUDObject::MUDObject(void) : Base(-1), Scriptable(Scriptable::None), room(-1) { }
//inline			MUDObject::MUDObject(VNum vnum) : Base(vnum), Scriptable(Scriptable::Base), room(-1) { }
//inline			MUDObject::MUDObject(ScriptObject::Kind type) : Base(-1), Scriptable(type), room(-1) { }

inline			MUDObject::MUDObject(VNum vnum) : GameData(vnum), material(Materials::Undefined),
						invis_level(0), weight(0), passive_defense(0), damage_resistance(0), max_riders(0),
						keywords(NULL), shortDesc(NULL), room(-1), inside(NULL) { }
inline			MUDObject::MUDObject(const MUDObject &mo) : GameData(mo),  material(mo.material),
						invis_level(mo.invis_level), weight(mo.weight),
						passive_defense(mo.passive_defense), damage_resistance(mo.damage_resistance),
						max_riders(mo.max_riders), 
						keywords(mo.keywords->Share()), shortDesc(mo.shortDesc->Share()), room(-1),
						inside(NULL) { }
inline 			MUDObject::~MUDObject(void) {
	SSFree(keywords);
	SSFree(shortDesc);
}

inline Type				MUDObject::DataType(void) const		{ return Datatypes::MUDObject; }

inline const GameData *	MUDObject::Inside(void)	const	{ return inside; }

inline int	MUDObject::Zone(void) const	{	return AbsoluteInside()->Zone();	}

inline const char * 	MUDObject::Keywords(const char *def) const 
	{ 	return (keywords ? keywords->Data() : def);	}

inline void			 	MUDObject::SetKeywords(const char *val) 
	{	if (keywords)	SSFree(keywords);
		keywords = NULL;
		if (val)		keywords = SString::Create(val);	}

inline void			 	MUDObject::SetKeywords(SString *val) 
	{	if (keywords)	SSFree(keywords);
		keywords = NULL;
		if (val)		keywords = val->Share();	}

inline const SString * 	MUDObject::SSKeywords(const SString *def) const 
	{ 	return (keywords ? keywords : def);	}

inline const char * 	MUDObject::SDesc(const char *def) const 
	{	return (shortDesc ? shortDesc->Data() : def);	}

inline void		MUDObject::SetSDesc(const char *val) 
	{	if (shortDesc)	SSFree(shortDesc);
		shortDesc = NULL;
		if (val)	shortDesc = SString::Create(val);	}

inline void				MUDObject::SetSDesc(SString *val) 
	{	if (shortDesc)	SSFree(shortDesc);
		shortDesc = NULL;
		if (val)	shortDesc = val->Share();	}

inline const SString * 	MUDObject::SSSDesc(const SString *def) const 
	{ 	return (shortDesc ? shortDesc : def);	}




inline VNum		MUDObject::InRoom(void) const		{ return room;			}
inline void		MUDObject::FromRoom(void)			{ return;				}
inline void		MUDObject::ToRoom(VNum theRoom)		{ return;				}
inline VNum		MUDObject::AbsoluteRoom(void) const { return InRoom();		}
inline void		MUDObject::Extract(void)			{ return;				}

inline Character * MUDObject::Rider(void)
{	return riders.Bottom();	}

inline UInt16 MUDObject::Invis(void)
{	return invis_level;	}

inline void MUDObject::SetInvis(UInt16 num)
{	invis_level = num;	}

#endif

