/***************************************************************************\
[*]    __     __  ___                ___  | LexiMUD is a C++ MUD codebase [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ |        for Sci-Fi MUDs        [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / | Copyright(C) 1997-1999        [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  | All rights reserved           [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   | Chris Jacobson (FearItself)   [*]
[*]      A Creation of the AvP Team!      | fear@technologist.com         [*]
[-]---------------------------------------+-------------------------------[-]
[*] File : clans.h                                                        [*]
[*] Usage: Clan module header                                             [*]
\***************************************************************************/


#ifndef __CLANS_H__
#define __CLANS_H__


#include "index.h"
#include "stl.vector.h"


class Clan {
public:
						Clan(void);
						Clan(const Clan &clan);
						~Clan(void);

public:
	enum { NumRanks = 10 };

	struct Rank {
		enum {
			Applicant,
			Member,
			Commander = NumRanks - 2,	
			Leader = NumRanks - 1
		};
	};

public:
	VNum				vnum;
	
	char *				name;
	char *				description;
	char *				ranks[Clan::NumRanks - 1];
	IDNum				owner;
	VNum				room;
	SInt32				savings;
	Vector<IDNum>		members;

	bool				AddMember(UInt32 id);
	bool				RemoveMember(UInt32 id);
	bool				IsMember(UInt32 id);
	
public:
	static Map<VNum, Clan>Clans;
	
public:
//	static RNum			Real(VNum vnum);
	static bool			Find(VNum vnum);
	static void			Save(void);
	
	static void			Parse(FILE *fl, VNum virtual_nr, char *filename);
	static void			Load(void);
};


inline bool Clan::Find(VNum vnum)		{	return Clans.Map::Find(vnum);		}
inline bool Clan::IsMember(UInt32 id)	{	return (members.Contains(id));		}


#define GET_CLAN_LEVEL(ch)		((ch)->general.misc.clanrank)

#endif

