/*************************************************************************
*   File: rooms.h                    Part of Aliens vs Predator: The MUD *
*  Usage: Header file for room data                                      *
*************************************************************************/

#ifndef __ROOMS_H__
#define __ROOMS_H__

#include "types.h"
#include "structs.h"
#include "room.defs.h"
#include "internal.defs.h"
#include "find.h"
#include "mud.base.h"
#include "redit.h"
#include "spec.h"
#include "olc.h"

#include "stl.llist.h"
#include "stl.map.h"

class Editor;
class ExtraDesc;
class Room;


//	Room Exits
class RoomDirection : public Editable
{
public:
	friend class Room;
	friend class REdit;
	friend class Editable;
	
						RoomDirection(void);
						RoomDirection(const RoomDirection *dir);
						virtual ~RoomDirection(void);
	
	const char *		Keyword(const char *def = NULL) const;
	const char *		Description(const char *def = NULL) const;

	Flags				exit_info;				//	Exit info
	VNum				key;					//	Key's number (-1 for no key)
	VNum				to_room;				//	Where direction leads (NOWHERE)
    Materials::Material	material;
    SInt16				max_hit_points, hit_points;
    UInt8				dam_resist;
    SInt8				pick_modifier;

// CHANGEPOINT:  When we have real vehicles, we'll want this to be different.
// protected:
	char *				general_description;
	char *				keyword;

	enum Vars {
		ExitNumber,
		ExitDesc,
		ExitKeyword,
		ExitKey,
		ExitFlags,
        ExitMaterial,
        ExitDR,
        ExitHP,
        ExitPick,
		MAXVAR
	};

	Ptr	GetPointer(int arg);
	virtual const struct olc_fields * Constraints(int arg);

public:
	void				Parser(char *input);

};


inline const char *	RoomDirection::Keyword(const char *def) const {
	return (keyword ? keyword : def);
}


inline const char *	RoomDirection::Description(const char *def) const {
	return (general_description ? general_description : def);
}


//	Mixin class for anything requiring an interior.
class RoomInterior
{
public:
							RoomInterior(void);
							RoomInterior(const RoomInterior &ri);
	virtual					~RoomInterior(void);

	virtual int				Zone(void) const = 0;
    virtual Sector &		SectorType(void);
	virtual Sector &		SetSectorType(Sector val);
    virtual Flags &			RoomFlags(void);
    virtual Flags &			SetRoomFlags(Flags val);
    virtual	UInt8 &			Light(void);
    virtual	UInt8 &			Light(SInt8 val);
    virtual	UInt8 &			ChangeLight(SInt8 val);

	virtual RoomDirection *	Direction(SInt8 dir) = 0;
	virtual RoomDirection *	MakeDirection(SInt8 dir, VNum toroom, Flags doorflags = 0) = 0;
	virtual void			RemoveDirection(SInt8 dir) = 0;

    virtual	int				Realm(void) = 0;
    
	virtual const char *	Description(const char *def = NULL) const;
	virtual void			SetDescription(const char * val = NULL);
	virtual void			SetDescription(SString * val = NULL);
	virtual const SString * SSDescription(const SString * def = NULL) const;

private:
	Sector					sector_type;			//	sector type (move/hide)
	Flags					flags;					//	DEATH,DARK ... etc
	UInt8					light;					//	Number of lightsources in room

public:
	LList<Character *>		people;					//	List of characters in room
	SString *				description;

	friend class REdit;
};

inline Sector &			RoomInterior::SectorType(void) { return sector_type; }
inline Sector &			RoomInterior::SetSectorType(Sector val) { return (sector_type = val); }
inline Flags &			RoomInterior::RoomFlags(void) { return flags; }
inline Flags &			RoomInterior::SetRoomFlags(Flags val) { flags = val; return flags; }
inline UInt8 &			RoomInterior::Light(void) { return light; }
inline UInt8 &			RoomInterior::Light(SInt8 val) { light = val; return light; }
inline UInt8 &			RoomInterior::ChangeLight(SInt8 val) { light += val; return light; }

inline const char * 	RoomInterior::Description(const char *def) const 
	{ return (description ? description->Data() : def); }
inline void				RoomInterior::SetDescription(const char *val) 
	{	if (description)	SSFree(description);
		description = NULL;
		if (val)	description = SString::Create(val); }
inline void			 	RoomInterior::SetDescription(SString *val) 
	{	if (description)	SSFree(description);
		description = NULL;
		if (val)	description = val->Share();	}
inline const SString * 	RoomInterior::SSDescription(const SString *def) const 
	{ return (description ? description : def);	}

class Ship;

//	Class for Rooms
class Room : public GameData, public RoomInterior, public Editable
{
public:
	friend class REdit;
						Room(void);
						~Room(void);
	
	virtual Type		DataType(void) const;

	const char *		Description(const char *def = NULL) const;
	virtual int			Zone(void) const;
	RoomDirection *		Direction(SInt8 dir);
	RoomDirection *		MakeDirection(SInt8 dir, VNum toroom, Flags doorflags = 0);
	void				RemoveDirection(SInt8 dir);
	
	SInt32				Send(const char *messg, ...) const __attribute__ ((format (printf, 2, 3)));
	void				WavSound(char *filename, int volume, int repeats, int priority, char *type);

//	void				DisplayStat(Character *ch);
//	void				ScriptStat(Character *ch);
	
public:
//	static VNum			Real(VNum vnum);
	static bool			Find(VNum vnum);
	
	void				Parse(char *input);
	static void 		SaveDisk(Room *room, FILE *fp);

//	Scriptable Overrides
public:

	inline virtual VNum 	InRoom(void) const { return NOWHERE; }
	VNum					AbsoluteRoom(void) const;

    int						Realm(void);

	virtual void 			AtCommand(VNum target, char * argument);
   
	virtual Character *		TargetChar(const char *arg) const;
	virtual Character *		TargetCharRoom(const char *arg) const;
	virtual Object *		TargetObj(const char *arg) const;
	virtual Object *		TargetObjList(const char * arg, LList<Object *> &list) const;
	virtual Room *			TargetRoom(const char *arg) const;

	virtual void			Echo(char * arg) const;
	virtual void			EchoAt(char * arg, GameData * target) const;
	virtual void			EchoAround(char * arg, GameData * target) const;
	virtual void			EchoDistance(char * arg, GameData * target) const;

public:
	LList<ExtraDesc *>	ex_description;			//	for examine/look
	
	SPECIAL(*func);
	char					*farg;				//	specfunc arg
	
	LList<Ship *>		ships;
    
	UInt16				zone;					//	Room zone (for resetting)
    SInt16				coordx;
    SInt16				coordy;

	enum Vars {
		RoomName,
		RoomDesc,
		RoomFlagList,
		RoomSector,
		RoomExitNumber,
		RoomExitDesc,
		RoomExitKeyword,
		RoomExitKey,
		RoomExitFlags,
        RoomExitMaterial,
        RoomExitDR,
        RoomExitHP,
        RoomExitPick,
        RoomCoordX,
        RoomCoordY,
		MAXVAR
	};

	Ptr	GetPointer(int arg);
	virtual const struct olc_fields * Constraints(int arg);

// CHANGEPOINT:  When we have real vehicles and ID-based inrooms, this will need
// to be private. -DH
// private:
	RoomDirection *		dir_option[NUM_OF_DIRS];	//	Directions


};

extern Map<VNum, Room>world;

//	Global data from rooms.cp
extern Sunlight		sunlight;

//	Global data pertaining to rooms
extern VNum			mortal_start_room;
extern VNum			immort_start_room;
extern VNum			frozen_start_room;
extern VNum			jailed_start_room;


//	Public functions in rooms.cp
void check_start_rooms(void);
//void renum_world(void);


//	Inline Room methods
inline const char *	Room::Description(const char *def) const 
	{	return (description ? SSData(description) : def);	}
inline VNum		Room::AbsoluteRoom(void) const 
	{	return vnum;	}
inline bool 	Room::Find(VNum vnum) 
	{	return world.Find(vnum);	}
inline RoomDirection * Room::Direction(SInt8 dir)
	{	if (dir >= 0 && dir < NUM_OF_DIRS)	return dir_option[dir];
	    else	return NULL;	}
inline int	Room::Zone(void) const { return zone; }
inline Type		Room::DataType(void) const			{ return Datatypes::Room; }

inline Character *	Room::TargetCharRoom(const char *arg) const
{	if (*arg == UID_CHAR)
		return get_char(arg);
	else
		return get_char_room(arg, Virtual());	}

inline Object *	Room::TargetObjList(const char * arg, LList<Object *> &list) const
{	if (*arg == UID_CHAR)
		return get_obj(arg);
	else 
		return get_obj_in_list(arg, list);	}


#endif

