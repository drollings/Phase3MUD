/*****************************************************
 * maputils.h --- implementation file for ascii maps *
 *						   *
 * Kyle Goodwin, (c) 1998 All Rights Reserved	     *
 * vbmasta@earthlink.net - Head Implemenor FirocMUD  *
 *****************************************************/

#define MAP_ROWS      100
#define MAP_COLS      100

struct mapstruct {
   SInt16 x;
   SInt16 y;
};

typedef struct mapstruct MapStruct;

MapStruct findcoord(int rnum);

// extern void load_surface_map(void);

extern void getmapchar(int rnum, Character * ch);

extern void printmap(int rnum, Character * ch);

extern int mapnums[MAP_ROWS][MAP_COLS];


