/*************************************************************************
*   File: descriptors.h              Part of Aliens vs Predator: The MUD *
*  Usage: Header file for descriptors                                    *
*************************************************************************/


#ifndef	__DESCRIPTORS_H__
#define __DESCRIPTORS_H__

#include "types.h"
#include "screen.h"

#include <netinet/in.h>

#include "internal.defs.h"
#include "character.defs.h"
#include "stl.llist.h"

#include <stdarg.h>


class Editor;
class Character;

//	Types

//	Modes of connectedness: used by descriptor_data.state
enum State {
	CON_PLAYING,		//	0 - Playing - Nominal state
	CON_CLOSE,			//	Disconnecting
	CON_GET_NAME,		//	By what name ..?
	CON_NAME_CNFRM,		//	Did I get that right, x?
	CON_PASSWORD,		//	Password:
	CON_NEWPASSWD,		//	5 - Give me a password for x
	CON_CNFPASSWD,		//	Please retype password:
	CON_SDESC,			//	Enter a short description:
	CON_QSEX,			//	Sex?
	CON_QRACE,			//	Race?
	CON_RMOTD,			//	PRESS RETURN after MOTD
	CON_MENU,			//	10 - Your choice: (main menu)
	CON_EXDESC,			//	Enter a new description:
	CON_CHPWD_GETOLD,	//	Changing passwd: get old
	CON_CHPWD_GETNEW,	//	Changing passwd: get new
	CON_CHPWD_VRFY,		//	Verify new password
	CON_DELCNF1,		//	15 - Delete confirmation 1
	CON_DELCNF2,		//	Delete confirmation 2
	CON_NEWPLAYMESG,	//  Reading newplayer message
	// NOTE:  The OLC modes must be in the same order as the OLC_ defines in internal.defs.h!
	CON_REDIT,			//	OLC mode - room edit	
	CON_OEDIT,			//	OLC mode - object edit
	CON_ZEDIT,			//	OLC mode - zone info edit
	CON_MEDIT,			//	20 - OLC mode - mobile edit
	CON_SEDIT,			//	OLC mode - shop edit
	CON_AEDIT,			//	OLC mode - action edit
	CON_HEDIT,			//	OLC mode - help edit
	CON_SCRIPTEDIT,		//	OLC mode- Script edit
	CON_SKILLEDIT,		//	OLC mode- Skill/spell edit
	CON_CEDIT,			//	OLC mode - Clan Edit

	CON_TEXTED,			//	OLC mode - Text edit
	CON_DISCONNECT,		//	Disconnect user
	CON_IDENT,			//	Identifying...
	CON_GETEMAIL,		//	Fetch email address
	CON_CGINFO,			//	Character generator
	CON_QSAVE,			//	Character generator
	CON_QSTR,			//	Character generator
	CON_QINT,			//	Character generator
	CON_QWIS,			//	Character generator
	CON_QDEX,			//	Character generator
	CON_QCON,			//	Character generator
	CON_QCHA,			//	Character generator
	CON_QALIGN,			//	Character generator
	CON_QLAWFUL,			//	Character generator
	CON_QHAIR,			//	Character generator
	CON_QEYES,			//	Character generator
	CON_QADV,			//	Character generator
	CON_QDISADV,		//	Character generator
	CON_QANSI,
	CON_QCLASS
    
};


enum {
	TERM_PLAIN,
	TERM_ANSI,
	TERM_VT100
};

//	Base declarations

struct txt_block {
	char *	text;
	int		aliased;
	struct txt_block *next;
};


struct txt_q {
	struct txt_block *head;
	struct txt_block *tail;
};

class Descriptor {
public:
						Descriptor(UInt32 desc);
						~Descriptor(void);
	
	Character *			Original(void);
	
	SInt32				Write(const char *fmt);
	SInt32				Writef(const char *fmt, ...) __attribute__ ((format (printf, 2, 3)));
	SInt32				MergeWrite(const char *fmt);
	SInt32				MergeWritef(const char *fmt, ...) __attribute__ ((format (printf, 2, 3)));
//	SInt32				VAWrite(const char *fmt, va_list ap);

public:
	socket_t			descriptor;				//	file descriptor for socket
	char				host[HOST_LENGTH+1];	//	hostname
	UInt8				bad_pws;				//	number of bad pw attemps this login
	UInt8				idle_tics;				//	tics idle at password prompt
	State				connected;				//	mode of 'connectedness'
	SInt32				wait;					//	wait for how many loops
	SInt32				desc_num;				//	unique num assigned to desc
	time_t				login_time;				//	when the person connected
	
	//	Paged Mode
	char *				showstr_head;			//	for keeping track of an internal str
	char **				showstr_vector;			//	for paging through texts
	UInt8				showstr_count;			//	number of pages to page through
	UInt8				showstr_page;			//	which page are we currently showing?

	Color				default_color;			// Default for &c0 in proc_color
	
	bool				has_prompt;				//	control of prompt-printing

	//	I/O
	char				inbuf[MAX_RAW_INPUT_LENGTH];	//	buffer for raw input
	char				last_input[MAX_INPUT_LENGTH];	//	the last input
	char				small_outbuf[SMALL_BUFSIZE];	//	standard output buffer
	char **				history;				//	History of commands, for ! mostly.
	UInt32				history_pos;			//	Circular array position.
	char *				output;					//	ptr to the current output buffer
	SInt32				bufptr;					//	ptr to end of current output
	SInt32				bufspace;				//	space left in the output buffer
#if USE_CIRCLE_SOCKET_BUF
	struct txt_block *	large_outbuf;			//	ptr to large buffer, if we need it
#else
	char *				large_outbuf;			//	ptr to large buffer, if we need it.
#endif
	struct txt_q		input;					//	q of unprocessed input
    UInt8				termtype;

	//	Character Data
	Character *			character;				//	linked to char
	Character *			original;				//	original char if switched
	Descriptor *		snooping;				//	Who is this char snooping
	Descriptor *		snoop_by;				//	And who is snooping this char

	const Editor *		Edit(void);
	void				Edit(Editor * new_editor);
	Ptr					Editing(void);
	//	OLC
protected:
	Editor *			editor;
	
public:	
	struct sockaddr_in	saddr;
	char				host_ip[HOST_LENGTH + 1];
	char				host_name[HOST_LENGTH + 1];
	char				user_name[HOST_LENGTH + 1];

friend class Editor;
};

inline Character *	Descriptor::Original(void) 
	{	return this->original ? this->original : this->character;	}

inline 	const Editor *	Descriptor::Edit(void)
	{	return editor;	}

extern LList<Descriptor *>	descriptor_list;

#endif

