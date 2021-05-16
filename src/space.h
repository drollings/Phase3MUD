#include "types.h"
#include "mud.base.h"
#include "stl.llist.h"
#include "index.h"

class Point {			//	Concrete 3D Point class
public:
	float				x, y, z;
	
						Point(float xVal = 0, float yVal = 0, float zVal = 0) :
								x(xVal), y(yVal), z(zVal) { }
						Point(const Point &a) : x(a.x), y(a.y), z(a.z) { }
	
	Point &				operator+=(const Point &b);
	Point &				operator-=(const Point &b);
	float				Pythagorean(void) const;
	static float		Distance(const Point &a, const Point &b);
};
Point operator-(const Point &a, const Point &b);
Point operator+(const Point &a, const Point &b);
Point operator/(const Point &a, const Point &b);
Point operator/(const Point &a, float b);
Point operator*(const Point &a, const Point &b);
Point operator*(const Point &a, float b);


class StellarObject {	//	Base class for stars, planets, etc.
public:
	enum StellarObjectType { Base, Star, Planet };
						StellarObject(StellarObjectType objectType = Base);
	virtual				~StellarObject(void);
						
	char *				name;
//	char *				file;
	Point				position;
	
	SInt32				gravity;
	StellarObjectType	type;
};


class Star : public StellarObject {
public:
						Star(void);
	virtual				~Star(void);
};



class Dock {
public:
						Dock(void);
						~Dock(void);
						
	VNum				dockVNum;
	char *				name;
};


class Planet : public StellarObject {
public:
						Planet(void);
	virtual				~Planet(void);
						
	Vector<Dock *>		docks;
};


class Ship;
class StarSystem {
public:
	SInt32				Send(Ship *from, Ship *ignore, const char *fmt, ...)
								__attribute__ ((format (printf, 4, 5)));


	//	Appearance
	char *				name;
	VNum				number;
	
	//	Internal Data
	char *				file;
	
	//	Location
	Point				position;
	
	//	Contents
	LList<Ship *>		ships;
	LList<Star *>		stars;
	LList<Planet *>		planets;
	
	void				Update(void);
	void				Write(void);
	void				Parser(char *input);

public:
	const char *		Name(void);

public:
	static Map<VNum, StarSystem>	_Index;
	
	static bool			Find(VNum vnum) { return _Index.Find(vnum); }
//	static VNum			Top;
	
//	static VNum			Real(VNum vnum);
	
	
	static VNum			FromName(const char *name);
	static VNum			FromVNum(VNum vnum);
	
	static void			WriteList(void);
};


inline const char *StarSystem::Name(void)		{	return name;		}


class Ship : public MUDObject {
public:
	enum ShipType { Civilian, Mob };
	enum Kind { Fighter, Midsize, Capital, Platform, Flight, Hover, Boat, Wheeled, Tread, Walker };
	enum State { Docked, Ready, Busy1, Busy2, Busy3, Refuel, Launch1, Launch2, Land1, Land2,
				PreHyper, Hyperspace, Disabled, Flying };
	enum { ShipTypes = Mob + 1, ShipKinds = Walker + 1, ShipStates = Flying + 1};
public:
						Ship(void);
	virtual				~Ship(void);
						
	virtual const char *Name(void) const;
	virtual void		Extract(void);
	void				Save(void);
	
	void				Parser(char *input);

	void				Move(void);
	void				Update(void);
	
	SInt32				SendCockpit(const char *fmt, ...) __attribute__ ((format (printf, 2, 3)));
	SInt32				Send(const char *fmt, ...) __attribute__ ((format (printf, 2, 3)));

	bool				Facing(const Point &target);
	
//	static Ship *		FindInRoom(RNum room, const char *name);
	static Ship *		FindInList(const char *name, LList<Ship *> &list);

	void				ToSystem(VNum newSystem);
	void				FromSystem(void);
	virtual void		ToRoom(VNum newRoom);
	virtual void		FromRoom(void);
	
	void				Launch(void);
	void				Orbit(void);
	void				Hyper(void);
	void				EnterOrbit(void);
	void				Land(void);
	
	void				Destroy(Character *killer = NULL);
	void				Reset(void);
public:
	//	Appearance
	char *				name;
	char *				description;
	
	//	Internal Data
	char *				file;
	char *				home;
	char *				dest;
	State				state;
//	SInt32				collision;
	
	//	Data
	Kind				kind;
	ShipType			type;
	SInt16				hyperSpeed, realSpeed;
	
	//	Controllers
	char *				owner;
	char *				pilot;
	
	//	Location
	VNum				starsystem;	//	Current system or -1
	VNum				shipyard;	//	Starting room
	VNum				lastDock;
	SInt16				speed;
	Point				position, heading;
	SInt32				hyperDistance;
	VNum				targetSystem;
	
	//	Rooms
	VNum				firstRoom, lastRoom;
	VNum				cockpit, pilotSeat;
	VNum				enterance;

public:
	static LList<Ship *>ships;
	
	static Ship *		ByPilot(const char *name);
	static Ship *		ByCockpit(VNum seat);
	static Ship *		ByPilotSeat(VNum seat);
//	static Ship *		ByGunSeat(VNum seat);
//	static Ship *		ByEngine(VNum seat);
//	static Ship *		ByTurret(VNum seat);
	static Ship *		ByEnterance(VNum seat);
//	static Ship *		ByHangar(VNum seat);

	static void			ShowShips(Character *ch, LList<Ship *> &shipList);
	
	static Index<Ship>	_Index;
};


inline const char *Ship::Name(void) const {	return this->name;					}


void UpdateSpace(void);
void MoveShips(void);

