/*************************************************************************
*   File: olc.h                      Part of Aliens vs Predator: The MUD *
*  Usage: Header file for the OLC System                                 *
*************************************************************************/

#ifndef __OLC_H__
#define __OLC_H__

#include "types.h"

class Character;
class Trigger;
class ExtraDesc;

//	Include a short explanation of each field in your zone files,
#undef ZEDIT_HELP_IN_FILE

//	Clear the screen before certain Oasis menus
#if 0
#define CLEAR_SCREEN	0
#endif

#ifdef GOT_RID_OF_IT

//	Macros, defines, structs and globals for the OLC suite.
#define NUM_ROOM_FLAGS 		21
#define NUM_ROOM_SECTORS	12

#define NUM_MOB_FLAGS		18
#define NUM_EXIT_FLAGS		10
#define NUM_AFF_FLAGS		18
#define NUM_WEAPON_TYPES	8

#define NUM_WTRIG_TYPES		9
#define NUM_OTRIG_TYPES		15
#define NUM_MTRIG_TYPES		17

#define NUM_AMMO_TYPES		14

#define NUM_ITEM_TYPES		31
#define NUM_ITEM_FLAGS		16
#define NUM_ITEM_WEARS 		16
#define NUM_APPLIES			19
#define NUM_LIQ_TYPES 		1
#define NUM_POSITIONS		9
#endif

#define NUM_APPLIES			19
#define NUM_LIQ_TYPES 		1
#define NUM_ATTACK_TYPES	13

#define OLCCMD(name)   void (name)(Character *ch, int subcmd, char *argument)

const SInt32 NUM_GENDERS =		3;
const SInt32 NUM_SHOP_FLAGS =	2;
const SInt32 NUM_TRADERS = 		3;

/* aedit permissions magic number */
enum {
	HEDIT_PERMISSION = 666,
	CEDIT_PERMISSION = 888,
	AEDIT_PERMISSION = 999,
	SKILLEDIT_PERMISSION = 20000
};

/*. Limit info .*/
#define	MAX_ROOM_NAME		75
#define	MAX_MOB_NAME 		50
#define	MAX_OBJ_NAME 		50
#define	MAX_ROOM_DESC		8192
#define	MAX_EXIT_DESC		8192
#define	MAX_EXTRA_DESC		8192
#define	MAX_MOB_DESC 		8192
#define	MAX_OBJ_DESC 		8192
#define	MAX_HELP_KEYWORDS	75
#define	MAX_HELP_ENTRY		4096

//	Utilities exported from olc.c.
void strip_string(char *str);
void olc_add_to_save_list(int zone, UInt8 type);
void olc_remove_from_save_list(int zone, UInt8 type);
bool get_zone_perms(Character * ch, int rnum);
int real_zone(int number);
void add_to_index(SInt32 znum, const char *type);

struct olc_save_info {
	SInt16			zone;
	UInt8			type;
	olc_save_info *	next;
};


extern struct olc_save_info *olc_save_list;

/* Daniel Rollings AKA Daniel Houghton's OLC_SAVE states */
enum {
	OLC_SAVE_NO,
	OLC_SAVE_YES,
	OLC_SAVE_COPY,
	OLC_SAVE_OVERWRITE
};

struct olc_fields {
	const char *name;				// Field name
	SInt32 type;				// Type of integer variable (EDT_UINT8...)
	SInt32 what;				// Kind of var (bv, int, string), Datatypes::??
	UInt32 flags;			// Type of flags to pass to the editor
	long min;				// Min value (integers only)
	long max;				// Max value (int), leng (str), mask (bv)
	const char **values;	// List of values, terminated with '\n'
};


// CHANGEPOINT:  Move this elsewhere
namespace InputType {
	enum {
		Any,
		Numeric,
		String
	};
}
#endif

