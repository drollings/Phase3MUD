#include "types.h"
#include "mud.base.h"
#include "space.h"

class StarSystem;

class Ship {
public:
	
	void				Move(void);
	
public:
	//	Appearance
	char *				name;
	char *				description;
	
	//	Internal Data
	char *				file;
	char *				home;
	char *				dest;
	
	//	Controllers
	char *				owner;
	char *				pilot;
	
	//	Location
	StarSystem *		starsystem;
	SInt32				speed;
	Point				position, heading;
	
	//	Rooms
	struct Rooms {
		VNum			first, last;
		VNum			cockpit;
	};
	
};
