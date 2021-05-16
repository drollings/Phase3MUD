/*************************************************************************
*   File: socials.h                  Part of Aliens vs Predator: The MUD *
*  Usage: Header file for socials                                        *
*************************************************************************/

#ifndef __SOCIALS_H__
#define __SOCIALS_H__

#include "types.h"
#include "stl.map.h"
#include "stl.vector.h"
#include "character.defs.h"

class Social {
public:
						Social(void);
//						~Social(void);

	void				Clear(void);

	VNum				act_nr;
	char *				command;				//	holds copy of activating command
	char *				sort_as;				//	holds a copy of a similar command or
												//	abbreviation to sort by for the parser
	bool				hide;					//	Hide Invisible?
	Flags				victim_position;		//	Position of victim
	Flags				char_position;			//	Position of char
	SInt32				min_level_char;         //	Minimum level of socialing char

	//	No argument was supplied
	char *				char_no_arg;
	char *				others_no_arg;

	//	An argument was there, and a victim was found
	char *				char_found;
	char *				others_found;
	char *				vict_found;

	//	An argument was there, as well as a body part, and a victim was found
	char *				char_body_found;
	char *				others_body_found;
	char *				vict_body_found;

	//	An argument was there, but no victim was found
	char *				not_found;

	//	The victim turned out to be the character
	char *				char_auto;
	char *				others_auto;

	//	If the char cant be found search the char's inven and do these:
	char *				char_obj_found;
	char *				others_obj_found;

public:
	static void			Boot(void);
	static Map<VNum, Social *>Messages;
	static Social *		Find(VNum nr) { return Messages.Find(nr) ? Messages[nr] : NULL; }

	static Vector<Social>internal;
};


#endif
