/***************************************************************************\
[*]    __     __  ___                ___  | LexiMUD is a C++ MUD codebase [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ |        for Sci-Fi MUDs        [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / | Copyright(C) 1997-1999        [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  | All rights reserved           [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   | Chris Jacobson (FearItself)   [*]
[*]      A Creation of the AvP Team!      | fear@technologist.com         [*]
[-]---------------------------------------+-------------------------------[-]
[*] File : ban.h                                                          [*]
[*] Usage: Host Ban Services                                              [*]
\***************************************************************************/


#ifndef __BAN_H__
#define __BAN_H__

#include "types.h"
#include "character.defs.h"
#include "stl.vector.h"

const SInt32 BANNED_SITE_LENGTH = 50;

class Ban {
public:
	enum BanType { Not, New, Select, All };
						Ban(void);
						Ban(const char *site, const char *name, SInt32 type, time_t date = 0);
						~Ban(void);
						
	char *				mSite;	//	[BANNED_SITE_LENGTH+1];
	IDNum				mID;	//	[MAX_NAME_LENGTH+1];
	time_t				mDate;
	BanType				mType;

public:
	static void			Load(void);
	static BanType		Banned(char *hostname);
	static bool			ValidName(const char *name);
	static void			ReadInvalids(void);
	
	static void			Write(void);
};

extern Vector<Ban *>	bans;


#endif

