/* ************************************************************************
*   File: comm.h                                        Part of CircleMUD *
*  Usage: header file: prototypes of public communication functions       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */


#include "types.h"
#include "screen.h"

class Descriptor;
class Character;
class MUDObject;

#define NUM_RESERVED_DESCS	8

#define COPYOVER_FILE "copyover.dat"


/* comm.c */
/*
 * Backward compatibility macros.
 */
/*
#define send_to_all(txt)			send_to_allf(txt)
#define send_to_char(txt, ch)		send_to_charf(ch, txt)
#define send_to_room(txt, ch)		send_to_roomf(ch, txt)
#define send_to_outdoor(txt)		send_to_outdoorf(txt)
#define send_to_players(ch, txt)	send_to_playersf(ch, txt)
*/
void  send_to_outdoor(int realm, const char *messg, ...)
  __attribute__ ((format (printf, 2, 3)));
void	send_to_outdoorf(int realm, const char *messg, ...)
  __attribute__ ((format (printf, 2, 3)));
void	send_to_playersf(Character *ch, const char *messg, ...)
  __attribute__ ((format (printf, 2, 3)));
void	send_to_all(const char *messg);
void	send_to_players(Character *ch, const char *messg);
void	send_to_zone(char *messg, int zone_rnum);

void	close_socket(Descriptor *d);
void	echo_off(Descriptor *d);
void	echo_on(Descriptor *d);


#define ACT_DEBUG

#ifdef ACT_DEBUG
void	the_act(const char *str, bool hide_invisible, const Character *ch, const MUDObject *obj,
				CPtr vict_obj, int type, const char *who, UInt16 line);
#define	act(str, hide_invisible, ch, obj, vict_obj, type) \
	the_act(str, hide_invisible, ch, obj, vict_obj, type, __FUNCTION__, __LINE__)
#else
void	act(const char *str, bool hide_invisible, const Character *ch, const MUDObject *obj,
				CPtr vict_obj, int type);
#endif

void sub_write(char *arg, Character *ch, bool find_invis, int targets, bool tosleep = false);

#define TO_ROOM		(1 << 0)
#define TO_VICT		(1 << 1)
#define TO_NOTVICT	(1 << 2)
#define TO_CHAR		(1 << 3)
#define TO_ZONE		(1 << 4)
#define TO_GAME		(1 << 5)
#define TO_TRIG		(1 << 7)
#define TO_SLEEP	(1 << 8)	/* to char, even if sleeping */

int		write_to_descriptor(socket_t desc, const char *txt);
void	write_to_q(const char *txt, struct txt_q *queue, int aliased);
void	page_string(Descriptor *d, const char *str, bool keep_internal);

#define USING_SMALL(d)	((d)->output == (d)->small_outbuf)
#define USING_LARGE(d)  ((d)->output == (d)->large_outbuf)

#define DEFAULT_PAGE_LENGTH	22   /* default number of lines in a page */

#if 0
#ifdef __linux__
typedef RETSIGTYPE sigfunc(int);
#else
typedef RETSIGTYPE sigfunc();
#endif
#else
typedef RETSIGTYPE sigfunc(int);
#endif

extern FILE *logfile;	/* Where to send messages from log() */



