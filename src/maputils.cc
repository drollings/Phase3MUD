/*****************************************************
 * maputils.c --- implementation file for ascii maps *
 *							 *
 * Kyle Goodwin, (c) 1998 All Rights Reserved			 *
 * vbmasta@earthlink.net - Head Implemenor FirocMUD	*
 *****************************************************/

#include "structs.h"
#include "buffer.h"
#include "characters.h"
#include "rooms.h"
#include "utils.h"
#include "maputils.h"

int mapnums[MAP_ROWS][MAP_COLS];

char *map_chars[] =
{
	"&c0 ",			/* Inside, not used on map */
	"&cW#",			/* City, used for entances to zones */
	"&cg.",			/* Field */
	"&cg*",			/* Forrest */
	"&cy^",			/* Hills */
	"&cLM",			/* Mountains */
	"&cc~",			/* Water */
	"&cB~",			/* Water */
	"&cb~",			/* Water */
	"&c0 ",			/* Flying, not used on map */
	"&cw+",			/* Road */
	"&cY:",			/* Desert */
	"&cGT",			/* Jungle */
	"&ccw",			/* Swamp */
	"&cW_",			/* Arctic */
	"&cWM",			/* Impassable Mountains */
	"&cL.",			/* Underground */
	" ",													/* Vacuum */
	" ",													/* Space */
	" ",													/* Deepspace */
	"&cM@",			/* Player */
	"&cR@",			/* NPC */
	"\n"
};


void getmapchar(int rnum, Character *ch)
{
	char *mapchar;

	if (!Room::Find(rnum)) {
		log("Illegal room number referenced in getmapchar()");
		return;
	}

	// sprintf(buf, "MAP:	%d", rnum);
	// log(buf);

	*buf3 = '\0';

	sprintf(buf3, "%s", map_chars[world[rnum].SectorType()]);

//	 if (world[rnum].people) {
//		 if (IS_NPC(world[rnum].people))
//			 sprintf(buf3, "%s", map_chars[NUM_SECTOR_TYPES]);
//		 else
//			 sprintf(buf3, "%s", map_chars[NUM_SECTOR_TYPES + 1]);
//	 } else {
//		 sprintf(buf3, "%s", map_chars[world[rnum].sector_type]);
//	 }

}

MapStruct findcoord(int rnum)
{
	SInt16 x = 0;
	SInt16 y = 0;

	MapStruct coords;
	
	coords.x = -1;
	coords.y = -1;

	if (!Room::Find(rnum)) {
		log("SYSERR: findcoord for non-existent room");
	}

	if (((world[rnum].coordx < 0) || (world[rnum].coordx >= MAP_ROWS)) ||
			((world[rnum].coordy < 0) || (world[rnum].coordy >= MAP_COLS))) {
		log("SYSERR: findcoord for non-map rnum");
	}
	
	coords.x = world[rnum].coordx;
	coords.y = world[rnum].coordy;

	return coords;

}


void printmap(int rnum, Character *ch)
{
	int x = 0;
	int y = 0;
	int sightradius = 3;
	char buf[1024];

	MapStruct coord;

	if (!Room::Find(rnum)) {
		return;
	}

	coord = findcoord(rnum);
	*buf3 = '\0';

	if (IS_IMMORTAL(ch))
		sightradius = 5;

	strcpy(buf, "\r\n\t");

	for (y = coord.y - sightradius; y <= coord.y + sightradius; ++y) {
		for (x = coord.x - sightradius; x <= coord.x + sightradius; ++x) {
			if ((x == coord.x) && (y == coord.y)) {
				strcat(buf, "&c+&cW@&c-");
			} else if ((x < 0) || (y < 0) || (y >= MAP_ROWS) || (x >= MAP_COLS)) {
			 	strcat(buf, " ");
			} else {
				getmapchar(mapnums[y % MAP_ROWS][x % MAP_COLS], ch);
				strcat(buf, buf3);
			}
		}
		strcat(buf, "\r\n\t");
	}
	strcat(buf, "&c0\r\n");
	ch->Send(buf);
	if (IS_IMMORTAL(ch))
		ch->Sendf("Coordinates:	X:%d Y:%d\r\n", coord.x, coord.y);
}
