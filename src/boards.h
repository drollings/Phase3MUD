/***************************************************************************\
[*]    __     __  ___                ___  | LexiMUD is a C++ MUD codebase [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ |        for Sci-Fi MUDs        [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / | Copyright(C) 1997-1999        [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  | All rights reserved           [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   | Chris Jacobson (FearItself)   [*]
[*]      A Creation of the AvP Team!      | fear@technologist.com         [*]
[-]---------------------------------------+-------------------------------[-]
[*] Based on CircleMUD, Copyright 1993-94, and DikuMUD, Copyright 1990-91 [*]
[-]-----------------------------------------------------------------------[-]
[*] File : boards.h                                                       [*]
[*] Usage: Message Board Engine header                                    [*]
\***************************************************************************/



#include "types.h"
#include "stl.llist.h"


#define MAX_MESSAGE_LENGTH	4096	/* arbitrary -- change if needed */

#define BOARD_MAGIC	1048575	/* arbitrary number - see modify.c */

class BoardMessage {
public:
	IDNum			id;
	IDNum			poster;
	time_t			timestamp;
	char *			subject;
	char *			data;
};

/* sorry everyone, but inorder to allow for board memory, I had to make
   this in the following nasty way.  Sorri. */
//struct board_memory_type {
//	int timestamp;
//	int reader;
//	struct board_memory_type *next;
//};


class Board {
public:
	SInt32			Save(void);

protected:
	LList<BoardMessage *>	messages;
	IDNum			lastId;
	VNum			vnum;
	SInt32			readLevel;		//	min level to read messages
	SInt32			writeLevel;		//	min level to write messages
	SInt32			removeLevel;	//	min level to remove messages
//	struct board_memory_type *memory[301];

public:
	static void		InitBoards(void);
	static Board *	Create(VNum vnum);
	static void		Parse(FILE *fl, Board *board, char *filename, UInt32 id);
	static void		LookAt(void);
	
	static Board *	Find(VNum vnum);
	
	static SInt32	Show(VNum vnum, Character *ch);
	static SInt32	DisplayMessage(VNum vnum, Character *ch, IDNum arg);
	static void		WriteMessage(VNum vnum, Character *ch, char *arg);
	static void		RespondMessage(VNum bvnum, Character *ch, IDNum mnum);
	static SInt32	RemoveMessage(VNum vnum, Character *ch, IDNum arg);
	static SInt32	Save(VNum vnum);
};

#define MESG_POSTER(i)		(i->poster)
#define MESG_TIMESTAMP(i)	(i->timestamp)
#define MESG_SUBJECT(i)		(i->subject)
#define MESG_DATA(i)		(i->data)
#define MESG_ID(i)			(i->id)

//#define MEMORY_TIMESTAMP(i) (i->timestamp)
//#define MEMORY_READER(i) (i->reader)
//#define MEMORY_NEXT(i) (i->next)

