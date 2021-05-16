
#ifndef __TRACK_H__
#define __TRACK_H__


#include "types.h"
#include "room.defs.h"

extern const VNum NOWHERE;

class Character;


#ifdef GOT_RID_OF_IT
struct TrackStep {
				TrackStep(void) : room(NOWHERE), dir(0) { }
				TrackStep(const TrackStep &step) : room(step.room), dir(step.dir) { }
				
	VNum		room;
	UInt8		dir;
};


class Path {
public:
						Path(void);
						~Path(void);
	SInt32				Dir(VNum start);
	
	
//	primary entry points, these are the easiest to use the tracking package

	enum {
		Global		= (1 << 0),
		ThruDoors	= (1 << 1)
	};
	
//	build a path to either a named character, or a specific character,
//	path_to_name locates a character for whom is_name(name) returns
//	true, path_to_full_name locates a character with the exact name
//	specified
	static Path *		ToName(VNum start, char *name, SInt32 depth, Flags flags);
	static Path *		ToFullName(VNum start, char *name, SInt32 depth, Flags flags);
	static Path *		ToChar(VNum start, Character *ch, SInt32 depth, Flags flags);
	static Path *		ToRoom(VNum start, VNum dest, SInt32 depth, Flags flags);
	
/* a lower level function to build a path, perhaps based on a more
   complicated predicate then path_to*_char */
   
	typedef SInt32		(*BuildPathFunc)(VNum start, Ptr data);

	static Path *		Build(VNum start, Path::BuildPathFunc predicate, Ptr data, SInt32 depth, Flags flags);
	static Path *		Rebuild(VNum start, Path *path);
	
	SInt32				depth;
	Flags				flags;
	SInt32				count;
	Character *			victim;
	SInt32				dest;
	TrackStep*			moves;
};



/* do ongoing maintenance of an in-progress track */
SInt32	Track(Character* ch);

//ACMD(do_path);
#else
namespace Path {
	enum {
		Global		= (1 << 0),
		ThruDoors	= (1 << 1)
	};
}
#endif

#endif

