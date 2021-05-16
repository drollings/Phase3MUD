
#ifndef __ROOM_DEFS_H__
#define __ROOM_DEFS_H__

/* The cardinal directions: used as index to room_data.dir_option[] */
enum {
	NORTH,
	EAST,
	SOUTH,
	WEST,
	UP,
	DOWN,
	NUM_OF_DIRS
};


/* Room flags: used in room_data.room_flags */
#define ROOM_DARK			(1 << 0)	//	Dark
#define ROOM_DEATH			(1 << 1)	//	Death trap
#define ROOM_NOMOB			(1 << 2)	//	MOBs not allowed
#define ROOM_INDOORS		(1 << 3)	//	Indoors
#define ROOM_PEACEFUL		(1 << 4)	//	Violence not allowed
#define ROOM_SOUNDPROOF		(1 << 5)	//	Shouts, gossip blocked
#define ROOM_NOTRACK		(1 << 6)	//	Track won't go through
#define ROOM_PARSE			(1 << 7)	//	Use String Parser
#define ROOM_TUNNEL			(1 << 8)	//	room for only 1 pers
#define ROOM_PRIVATE		(1 << 9)	//	Can't teleport in
#define ROOM_GODROOM		(1 << 10)	//	Staff only allowed
#define ROOM_HOUSE			(1 << 11)	//	(R) Room is a house
#define ROOM_HOUSE_CRASH	(1 << 12)	//	(R) House needs saving
#define ROOM_NOMAGIC		(1 << 13)
// #define ROOM_MAP			(1 << 14)
#define ROOM_BFS_MARK		(1 << 15)	//	(R) breath-first srch mrk
#define ROOM_NOVEHICLE		(1 << 16)	//	No vehicles allowed
#define ROOM_GRGODROOM		(1 << 17)	//	GRGODs or higher only
#define ROOM_GRAVITY		(1 << 18)	//	Room has gravity, players fall down :-)
#define ROOM_MAP			(1 << 19)	//	Room has gravity, players fall down :-)
#define ROOM_VACUUM			(1 << 20)	//	GASP!  No...air...
#define ROOM_JAIL			(1 << 21)   //  Room is jail

#define	ROOM_DELETED		(1 << 31)	//	Room is deleted

/* Room affections */
#define RAFF_FOG		(1 << 0)   /* The Wall of Fog Spell	*/

/* Exit info: used in room_data.dir_option.exit_info */
namespace Exit {
	enum {
		Door		= (1 << 0),
		Closed		= (1 << 1),
		Locked		= (1 << 2),
		Pickproof	= (1 << 3),
		Hidden		= (1 << 4),
		NoShoot		= (1 << 5),
		Vista		= (1 << 6),		//	Window
		NoMob		= (1 << 7),
		NoVehicles	= (1 << 8),
        SeeThrough	= (1 << 9),
        Automatic	= (1 << 10)
	};
}


//	Sector types: used in Room.sector_type
enum Sector {
	SECT_INSIDE,			//	Indoors
	SECT_CITY,				//	In a city
	SECT_FIELD,				//	In a field
	SECT_FOREST,			//	In a forest
	SECT_HILLS,				//	In the hills
	SECT_MOUNTAIN,			//	On a mountain
	SECT_WATER_SWIM,		//	Shallow (<= 6') water
	SECT_WATER_NOSWIM,		//	Deep (> 6') - need a boat or swim
	SECT_UNDERWATER,		//	Underwater
	SECT_FLYING,			//	Wheee!
	SECT_ROAD,
	SECT_DESERT,
	SECT_JUNGLE,
	SECT_SWAMP,
	SECT_ARCTIC,
	SECT_HIGH_MOUNTAIN,
	SECT_UNDERGROUND,
	SECT_VACUUM,
	SECT_SPACE,				//	Guess...
	SECT_DEEPSPACE,
	NUM_ROOM_SECTORS		//	Guess again!
};


//	Sun state for sunlight
enum Sunlight {
	SUN_DARK,
	SUN_RISE,
	SUN_LIGHT,
	SUN_SET
};


/* Sky conditions for weather */
enum Sky {
	CLOUDLESS,
	CLOUDY,
	RAINING,
	LIGHTNING
};


#define MAX_DOORDESC_LENGTH		512

#endif

