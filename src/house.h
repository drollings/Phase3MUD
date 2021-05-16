
#ifndef __HOUSE_H__
#define __HOUSE_H__

#include "types.h"
#include "stl.llist.h"
#include "stl.vector.h"

#define MAX_HOUSES	100
#define MAX_GUESTS	10


#define HOUSE_PRIVATE	0


class Object;
class Character;


class House {
public:
					House(void);
					~House(void);
	
	VNum			vnum;
	
	time_t			created, lastPayment;
	IDNum			owner, maker;
	Vector<IDNum>	guests;
	SInt8			mode;

public:
	void			Parser(char *block);
	void			ListGuests(char *buf, bool quiet);
	
	static void		Boot(void);
	static void		SaveHouses(void);
	
	static bool		GetFilename(VNum house, char *filename);
	static void		DeleteFile(VNum house);
	
	static void		SaveAll(void);
	static void		CrashSave(VNum house);
	static void		Save(Object * obj, FILE * fp, int locate);
	static bool		Load(VNum vnum);
	
	static void		RestoreWeight(Object * obj);
	
	static bool		CanEnter(Character *ch, VNum house);	

	static void		Listrent(Character *ch, VNum house);
	
	static House *	Find(VNum house);
};


extern LList<House *>	Houses;


#define TOROOM(room, dir) (world[room].dir_option[dir] ? world[room].dir_option[dir]->to_room : NOWHERE)

#endif
