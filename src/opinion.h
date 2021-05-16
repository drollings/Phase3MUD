/*************************************************************************
*   File: opinion.h                  Part of Aliens vs Predator: The MUD *
*  Usage: Header file for opinion system                                 *
*************************************************************************/

#ifndef __OPINION_H__
#define __OPINION_H__


#include "types.h"
#include "utils.h"
#include "stl.llist.h"


enum {
	OP_NEUTRAL	= (1 << 0),
	OP_MALE		= (1 << 1),
	OP_FEMALE	= (1 << 2)
};


enum {
	OP_CHAR		= (1 << 0),
	OP_SEX		= (1 << 1),
	OP_RACE		= (1 << 2),
	OP_VNUM		= (1 << 3)
};


class Opinion {
public:
					Opinion(void);
					Opinion(const Opinion &old);
					~Opinion(void);
	void			Clear(void);
	bool			RemChar(IDNum id);
	void			RemOther(Flags type);
	bool			AddChar(IDNum id);
	void			AddOther(Flags type, UInt32 param);
	bool			Contains(IDNum id);
	bool			IsTrue(Character *ch);
	Character *		FindTarget(Character *ch);

	LList<IDNum>	charlist;
	Flags			sex;
	Flags			race;
	VNum			vnum;
	
	Flags			active;
};


inline bool Opinion::Contains(IDNum id)	{ return charlist.Contains(id);	}


#endif

