#ifndef __INTERNAL_DEFS_H__
#define __INTERNAL_DEFS_H__

const SInt32 OPT_USEC =			100000;		//	10 passes per second
const SInt32 PASSES_PER_SEC =	(1000000 / OPT_USEC);
#define RL_SEC			* PASSES_PER_SEC

const SInt32 PULSE_ZONE =		10 RL_SEC;
const SInt32 PULSE_MOBILE =		1 RL_SEC;
const SInt32 PULSE_POINTS =		25 RL_SEC;	//	WILL REMOVE
const SInt32 PULSE_VIOLENCE	=	4 RL_SEC;	//	WILL REMOVE ?
const SInt32 PULSE_BUFFER =		5 RL_SEC;
const SInt32 PULSE_SCRIPT =		6 RL_SEC;
const SInt32 PULSE_EVENT =		1 RL_SEC;

const UInt32 TICK_WRAP_COUNT =	10;

const SInt32 MOBILE_PERCENT	=	20;			//	Do 1 in 5 of specials per mobile pulse.

//	Variables for the output buffering system */
const SInt32 MAX_SOCK_BUF =		(12 * 1024);	//	Size of kernel's sock buf
const SInt32 MAX_PROMPT_LENGTH =512;			//	Max length of prompt
const SInt32 GARBAGE_SPACE =	32;				//	Space for **OVERFLOW** etc
const SInt32 SMALL_BUFSIZE =	1024;			//	Static output buffer size
//	Max amount of output that can be buffered
const SInt32 LARGE_BUFSIZE =	(MAX_SOCK_BUF - GARBAGE_SPACE - MAX_PROMPT_LENGTH);

#define USE_CIRCLE_SOCKET_BUF	1

const UInt32 HISTORY_SIZE =		5;
const UInt32 MAX_STRING_LENGTH = 8192;
const UInt32 MAX_INPUT_LENGTH = 256;			//	Max length per *line* of input
const UInt32 MAX_RAW_INPUT_LENGTH = 512;			//	Max size of *raw* input
const SInt32 MAX_MESSAGES =		60;

class Character;
#define SPECIAL(name)	int (name)(Character *ch, Ptr me, const char * cmd, char *argument)

namespace Realm {
	enum {
		Nyrilis,
		Earth,
		Olodris,
		Space,
		Astral,
		Shadow,
		Ethereal,
		MAX
	};
}

// For ranged functions
#define MAX_DISTANCE 3

/* misc editor defines **************************************************/

/* format modes for format_text */
enum {
	FORMAT_INDENT =		(1 << 0)
};

enum {
	REAL,
	VIRTUAL
};


/* Positions bitflags */
#define P_DE	   (1 << POS_DEAD)
#define P_MO	   (1 << POS_MORTALLYW)
#define P_IN	   (1 << POS_INCAP)
#define P_SU	   (1 << POS_STUNNED)
#define P_SL	   (1 << POS_SLEEPING)
#define P_RE	   (1 << POS_RESTING)
#define P_SI	   (1 << POS_SITTING)
#define P_FI	   (1 << POS_FIGHTING)
#define P_ST	   (1 << POS_STANDING)
#define P_RI	   (1 << POS_RIDING)
#define P_AUD      (1 << POS_AUDIBLE)
#define P_MOB      (1 << POS_MOBILE)


/* Some abbreviations for the positions */
#define P_ANY	(P_DE | P_MO | P_IN | P_SU | P_SL | P_RE | P_SI | P_FI | P_ST | P_RI)
#define P_AWAKE	(P_RE | P_SI | P_FI | P_ST | P_RI)
#define P_NOTFI (P_RE | P_SI | P_ST | P_RI)
#define P_NOTRI (P_RE | P_SI | P_ST)

// OLC-related defines
enum {
	OLC_REDIT = 1,
	OLC_OEDIT,
	OLC_ZEDIT,
	OLC_MEDIT,
	OLC_SEDIT,
	OLC_AEDIT,
	OLC_HEDIT,
	OLC_SCRIPTEDIT,
	OLC_CEDIT,
	OLC_SKILLEDIT
};


#define EDIT_APPEND			(0 << 1)
#define EDIT_ONELINE		(0 << 2)

#endif

