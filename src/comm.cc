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
[*] File : comm.cp                                                        [*]
[*] Usage: Main Loop and Communication                                    [*]
\***************************************************************************/

#define __COMM_C__


#include "types.h"
#include "structs.h"
#include "screen.h"

# include <sys/ioctl.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <netdb.h>
#ifndef macintosh
# include <arpa/inet.h>
# include <sys/resource.h>
#else
#include <machine/endian.h>
# define SIGPIPE 13
# define SIGALRM 14
# define isascii(c)		((unsigned)(c) <= 0177)
#endif





#include "db.h"
#include "utils.h"
#include "buffer.h"
#include "descriptors.h"
#include "rooms.h"
#include "characters.h"
#include "objects.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "find.h"
#include "house.h"
#include "ban.h"
#include "olc.h"
#include "event.h"
#include "ident.h"
#include "constants.h"
#include "editor.h"
#include "space.h"

#ifdef HAVE_ARPA_TELNET_H
#include <arpa/telnet.h>
#else
#include "telnet.h"
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

/* externs */
extern int num_invalid;
extern SInt32 circle_restrict;
extern bool mini_mud;
bool no_rent_check = false;
extern int DFLT_PORT;
extern char *DFLT_DIR;
extern char *DFLT_IP;
extern int MAX_PLAYERS;
bool no_external_procs = false;
const char *LOGNAME;

extern struct TimeInfoData time_info;		/* In db.c */
extern char help[];


/* local globals */
#if USE_CIRCLE_SOCKET_BUF
struct txt_block *bufpool = 0;	/* pool of large output buffers */
#endif
int buf_largecount = 0;		/* # of large buffers which exist */
int buf_overflows = 0;		/* # of overflows of output */
int buf_switches = 0;		/* # of switches from small to large buf */
int circle_shutdown = 0;	/* clean shutdown */
int circle_reboot = 0;		/* reboot the game after a shutdown */
int circle_copyover = 0;
int no_specials = 0;		/* Suppress ass. of special routines */
int max_players = 0;		/* max descriptors available */
int tics = 0;			/* for extern checkpointing */
int act_check;
int scheck = 0;			/* for syntax checking mode */
extern int nameserver_is_slow;	/* see config.c */
extern int auto_save;		/* see config.c */
extern UInt32 autosave_time;	/* see config.c */
struct timeval null_time;	/* zero-valued time structure */
FILE *logfile = NULL;
extern char *LOGFILE;

static bool		fCopyOver;			//	Are we booting in copyover mode?
SInt32			mother_desc;		//	Now a global
SInt32			port;

/* functions in this file */
bool get_from_q(struct txt_q *queue, char *dest, int *aliased);
void init_game(UInt16 port);
void signal_setup(void);
void game_loop(int mother_desc);
int init_socket(UInt16 port);
int new_descriptor(int s);
int get_max_players(void);
int process_output(Descriptor *t);
int process_input(Descriptor *t);
void close_socket(Descriptor *d);
timeval operator-(timeval a, timeval b);
timeval operator+(timeval a, timeval b);
void flush_queues(Descriptor *d);
void nonblock(socket_t s);
int perform_subst(Descriptor *t, char *orig, char *subst);
int perform_alias(Descriptor *d, char *orig);
void record_usage(void);
char *make_prompt(Descriptor *point);
void check_idle_passwords(void);
void heartbeat(int pulse);
void copyover_recover(void);
int set_sendbuf(socket_t s);
void sub_write_to_char(Character *ch, char *tokens[], Ptr otokens[], SInt8 type[]);
void proc_color(char *inbuf, int color_lvl, Color default_color = COL_OFF, bool is_html = false);
char *prompt_str(Character *ch);
void perform_act(const char *orig, const Character *ch, const Object *obj,
		    CPtr vict_obj, Character *to);
void setup_log(const char *filename, int fd);
int open_logfile(const char *filename, FILE *stderr_fp);

RETSIGTYPE checkpointing(int);
RETSIGTYPE reread_wizlists(int);
RETSIGTYPE unrestrict_game(int);
RETSIGTYPE hupsig(int);
RETSIGTYPE reap(int sig);
ssize_t perform_socket_read(socket_t desc, char *read_point,size_t space_left);
ssize_t perform_socket_write(socket_t desc, const char *txt,size_t length);

#ifdef macintosh
void gettimeofday(struct timeval *t, struct timezone *dummy);
extern "C" {
	char * inet_ntoa(struct in_addr inaddr);
	struct in_addr inet_addr(char *address);
}
#endif

/* extern fcnts */
void reboot_wizlists(void);
void boot_db(void);
void boot_world(void);
void zone_update(void);
void point_update(void);		//	In limits.c
void hour_update(void);			//	In limits.c
void free_purged_lists();		//	In handler.c
void character_update(UInt32 pulse);
void string_add(Descriptor *d, char *str);
void perform_violence(void);
void show_string(Descriptor *d, char *input);
void weather_and_time(int mode);
void act_mtrigger(Character *ch, const char *str, const Character *actor, 
		const Character *victim, const MUDObject *object, const Object *target, const char *arg);
void script_trigger_check(void);

UInt32 pulse;


/********************************************************************
*	main game loop and related stuff								*
*********************************************************************/

/* GUSI doesn't have gettimeofday, so we'll simulate it. */
#ifdef macintosh
void gettimeofday(struct timeval *t, struct timezone *dummy) {}
#endif


int enter_player_game(Descriptor *d);

/* Reload players after a copyover */
void copyover_recover(void) {
	Descriptor *d;
	FILE *fp;
	char *host = get_buffer(1024);
	int desc, player_i;
	int fOld;
	char *name = get_buffer(MAX_INPUT_LENGTH);

	log ("Copyover recovery initiated");

	if (!(fp = fopen (COPYOVER_FILE, "r"))) { /* there are some descriptors open which will hang forever then ? */
		perror ("copyover_recover:fopen");
		log ("Copyover file not found. Exitting.\r\n");
		exit (1);
	}

	unlink (COPYOVER_FILE); /* In case something crashes - doesn't prevent reading  */

	for (;;) {
		fOld = TRUE;
		fscanf (fp, "%d %s %s\n", &desc, name, host);
		if (desc == -1)
			break;

		/* Write something, and check if it goes error-free */
		if (write_to_descriptor (desc, "\r\nRestoring from copyover...\r\n") < 0) {
			close (desc); /* nope */
			continue;
		}

		d = new Descriptor(desc);
		
		strcpy(d->host, host);
		descriptor_list.Add(d);
	
		STATE(d) = CON_CLOSE;

		/* Now, find the pfile */

		d->character = new Character();
		d->character->desc = d;

		if ((player_i = d->character->Load(name)) >= 0) {
			GET_PFILEPOS(d->character) = player_i;
			if (!PLR_FLAGGED(d->character, PLR_DELETED))
				REMOVE_BIT(PLR_FLAGS(d->character),PLR_WRITING | PLR_MAILING);
			else
				fOld = FALSE;
		} else
			fOld = FALSE;

		if (!fOld) { /* Player file not found?! */
			write_to_descriptor (desc, "\r\nSomehow, your character was lost in the copyover. Sorry.\r\n");
			close_socket (d);
		} else { /* ok! */
			write_to_descriptor (desc, "\r\nCopyover recovery complete.\r\n");
			enter_player_game(d);
			STATE(d) = CON_PLAYING;
			look_at_room(d->character, 0);
		}
//		save_char(d->character, NOWHERE);
	}

	fclose (fp);
	release_buffer(name);
	release_buffer(host);
}



int main(int argc, char **argv) {
	int pos = 1;
	char *dir;

	Buffer::Init();

	port = DFLT_PORT;
	dir = DFLT_DIR;

	/* Be nice to make this a command line option but the parser uses log() */
//	if (!LOGFILE || *LOGFILE == '\0')
//		logfile = stderr;
//	else
//		logfile = freopen(LOGFILE, "w", stderr);

//	if (!logfile) {
//		printf("Error opening log file %s: %s\n", LOGFILE && *LOGFILE ? LOGFILE : "stderr", strerror(errno));
//		exit(1);
//	}

	while ((pos < argc) && (*(argv[pos]) == '-')) {
		switch (*(argv[pos] + 1)) {
// 			case 'o':
// 				if (*(argv[pos] + 2))	LOGNAME = argv[pos] + 2;
// 				else if (++pos < argc)	LOGNAME = argv[pos];
// 				else {
// 					puts("SYSERR: File name to log to expected after option -o.");
// 					exit(1);
// 				}
			case 'C': /* -C<socket number> - recover from copyover, this is the control socket */
				fCopyOver = TRUE;
				mother_desc = atoi(argv[pos]+2);
				no_rent_check = true;
				break;
			case 'd':
				if (*(argv[pos] + 2))		dir = argv[pos] + 2;
				else if (++pos < argc)		dir = argv[pos];
				else {
					puts("SYSERR: Directory arg expected after option -d.");
					exit(1);
				}
				break;
			case 'm':
				mini_mud = true;
				no_rent_check = true;
				puts("Running in minimized mode & with no rent check.");
				break;
			case 'c':
				scheck = 1;
				puts("Syntax check mode enabled.");
				break;
			case 'q':
				no_rent_check = true;
				puts("Quick boot mode -- rent check supressed.");
				break;
			case 'r':
				circle_restrict = 1;
				puts("Restricting game -- no new players allowed.");
				break;
			case 's':
				no_specials = 1;
				puts("Suppressing assignment of special routines.");
				break;
			case 'v':
				if (*(argv[pos] + 2))		Buffer::options = atoi(argv[pos] + 2);
				else if (++pos < argc)		Buffer::options = atoi(argv[pos]);
				else {
					puts("SYSERR: Number expected after option -v.");
					exit(1);
				}
			case 'x':
				no_external_procs = true;
				break;
			default:
				printf("SYSERR: Unknown option -%c in argument string.", *(argv[pos] + 1));
				break;
		}
	pos++;
	}

	if (pos < argc) {
		if (!isdigit(*argv[pos])) {
			printf("Usage: %s [-c] [-m] [-q] [-r] [-s] [-x] [-v val] [-d pathname] [port #]", argv[0]);
			exit(1);
		} else if ((port = atoi(argv[pos])) <= 1024) {
			printf("SYSERR: Illegal port number.");
			exit(1);
		}
	}

	//	All arguments have been parsed, try to open log file.
	setup_log(LOGNAME, STDERR_FILENO);

//	log(circlemud_version);

 	if (chdir(dir) < 0) {
		perror("SYSERR: Fatal error changing to data directory");
		exit(1);
	}
	log("Using %s as data directory.", dir);

	// log("Signal trapping.");
	// signal_setup();
	
	Identd::Startup();
	
	if (scheck) {
		boot_world();
		log("Done.");
		exit(0);
	} else {
//#ifndef macintosh
//		pid_t	pid = 0;
//		if (!no_external_procs && ((pid = fork()) < 0)) {
//			log("SYSERR: Unable to start MUD Server!");
//		} else if (pid > 0) {
//			log("Starting MUD Server (%d)", pid);
//			waitpid(pid, NULL, 0);
//			log("Closing MUDServer");
//			exit(0);
//		} else {
//#endif
			log("Running game on port %d.", port);
			init_game(port);
//#ifndef macintosh
//			log("Shutting down game port.");
//			if (!no_external_procs)
//				exit(0);
//		}
//#endif
	}
	
	Identd::Shutdown();
	Buffer::Exit();
	
	if (circle_copyover) {
#define EXE_FILE "bin/lexi" /* maybe use argv[0] but it's not reliable */

		char buf[256], buf2[64];

		/* Close reserve and other always-open files and release other resources */

		/* exec - descriptors are inherited */
		
		sprintf (buf, "%d", port);
		sprintf (buf2, "-C%d", mother_desc);
		
		/* Ugh, seems it is expected we are 1 step above lib - this may be dangerous! */
		chdir ("..");
		
#ifndef macintosh
//		pid_t parent = getppid();
//	#if defined(SIGCLD)
//		kill(parent, SIGCLD);
//	#elif define(SIGCHLD)
//		kill(parent, SIGCHLD);
//	#endif
		
		execl (EXE_FILE, "circle", buf2, buf, NULL);
#endif
	}
	
	return 0;
}


//	Init sockets, run game, and cleanup sockets
void init_game(UInt16 port) {
	circle_srandom(time(0));

	log("Finding player limit.");
	max_players = get_max_players();

	if (!fCopyOver) {	//	If copyover mother_desc is already set up
		log("Opening mother connection.");
		mother_desc = init_socket(port);
	}
	
	Event::Initialize();
	
	boot_db();

	log("Signal trapping.");
	signal_setup();
	
	if (fCopyOver) /* reload players */
		copyover_recover();

	log("Entering game loop.");

	game_loop(mother_desc);
	
	
	if (!circle_copyover) {
		Crash_save_all();

		log("Closing all sockets.");
		while (descriptor_list.Top())
			close_socket(descriptor_list.Top());

		CLOSE_SOCKET(mother_desc);
	}
/*
	if (circle_reboot != 2 && olc_save_list) { // Don't save zones.
		struct olc_save_info *entry, *next_entry;
		for (entry = olc_save_list; entry; entry = next_entry) {
			next_entry = entry->next;
			if (entry->type < 0 || entry->type > 4)
				log("OLC: Illegal save type %d!", entry->type);
			else if (entry->zone < 0)
				log("OLC: Illegal save zone %d!", entry->zone);
			else {
				log("OLC: Reboot saving %s for zone %d.",
						save_info_msg[(int)entry->type], entry->zone);
				switch (entry->type) {
					case OLC_SAVE_ROOM:		REdit::save_to_disk(entry->zone); break;
					case OLC_SAVE_OBJ:		oedit_save_to_disk(entry->zone); break;
					case OLC_SAVE_MOB:		medit_save_to_disk(entry->zone); break;
					case OLC_SAVE_ZONE:		zedit_save_to_disk(entry->zone); break;
					case OLC_SAVE_SHOP:		sedit_save_to_disk(entry->zone); break;
					case OLC_SAVE_ACTION:	aedit_save_to_disk(entry->zone); break;
					case OLC_SAVE_HELP:		hedit_save_to_disk(entry->zone); break;
					default:				log("Unexpected olc_save_list->type"); break;
				}
			}
		}
	}
*/

	if (circle_reboot) {
		log("Rebooting.");
		exit(52);			/* what's so great about HHGTTG, anyhow? */
	} else if (!circle_copyover)
		log("Normal termination of game.");
}



/*
 * init_socket sets up the mother descriptor - creates the socket, sets
 * its options up, binds it, and listens.
 */
int init_socket(UInt16 port) {
	int s;
	int opt;
	struct sockaddr_in sa;

  /*
   * Should the first argument to socket() be AF_INET or PF_INET?  I don't
   * know, take your pick.  PF_INET seems to be more widely adopted, and
   * Comer (_Internetworking with TCP/IP_) even makes a point to say that
   * people erroneously use AF_INET with socket() when they should be using
   * PF_INET.  However, the man pages of some systems indicate that AF_INET
   * is correct; some such as ConvexOS even say that you can use either one.
   * All implementations I've seen define AF_INET and PF_INET to be the same
   * number anyway, so ths point is (hopefully) moot.
   */

	if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Error creating socket");
		exit(1);
	}


# if defined(SO_REUSEADDR)
	opt = 1;
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt)) < 0) {
		perror("SYSERR: setsockopt REUSEADDR");
		exit(1);
	}
	
# endif

	set_sendbuf(s);

# if defined(SO_LINGER) && !defined(macintosh)
	{
		struct linger ld;

		ld.l_onoff = 0;
		ld.l_linger = 0;
		if (setsockopt(s, SOL_SOCKET, SO_LINGER, (char *) &ld, sizeof(ld)) < 0)
			perror("SYSERR: setsockopt LINGER");
	}
# endif
	
	memset(&sa, 0, sizeof(sa));
	
	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	sa.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(s, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
		perror("SYSERR: bind");
		CLOSE_SOCKET(s);
		exit(1);
	}
	nonblock(s);
	listen(s, 5);
	return s;
}


int get_max_players(void) {
	return MAX_PLAYERS;
	int max_descs = 0;
	const char *method;

/*
 * First, we'll try using getrlimit/setrlimit.  This will probably work
 * on most systems.
 */
#if defined (RLIMIT_NOFILE) || defined (RLIMIT_OFILE)
#if !defined(RLIMIT_NOFILE)
#define RLIMIT_NOFILE RLIMIT_OFILE
#endif
	{
		struct rlimit limit;

		/* find the limit of file descs */
		method = "rlimit";
		if (getrlimit(RLIMIT_NOFILE, &limit) < 0) {
			perror("SYSERR: calling getrlimit");
			exit(1);
		}
		/* set the current to the maximum */
		limit.rlim_cur = limit.rlim_max;
		if (setrlimit(RLIMIT_NOFILE, &limit) < 0) {
			perror("SYSERR: calling setrlimit");
			exit(1);
		}
#ifdef RLIM_INFINITY
		if (limit.rlim_max == RLIM_INFINITY)
			max_descs = MAX_PLAYERS + NUM_RESERVED_DESCS;
		else
			max_descs = MIN(MAX_PLAYERS + NUM_RESERVED_DESCS, (int)limit.rlim_max);
#else
		max_descs = MIN(MAX_PLAYERS + NUM_RESERVED_DESCS, (int)limit.rlim_max);
#endif
	}

#elif defined (OPEN_MAX) || defined(FOPEN_MAX)
#if !defined(OPEN_MAX)
#define OPEN_MAX FOPEN_MAX
#endif
	method = "OPEN_MAX";
	max_descs = OPEN_MAX;		/* Uh oh.. rlimit didn't work, but we have
								 * OPEN_MAX */
#elif defined (POSIX)
	/*
	 * Okay, you don't have getrlimit() and you don't have OPEN_MAX.  Time to
	 * use the POSIX sysconf() function.  (See Stevens' _Advanced Programming
	 * in the UNIX Environment_).
	 */
	method = "POSIX sysconf";
	errno = 0;
	if ((max_descs = sysconf(_SC_OPEN_MAX)) < 0) {
		if (errno == 0)
			max_descs = MAX_PLAYERS + NUM_RESERVED_DESCS;
		else {
			perror("SYSERR: Error calling sysconf");
			exit(1);
		}
	}
#else
	method = "random guess";
	/* if everything has failed, we'll just take a guess */
	max_descs = MAX_PLAYERS + NUM_RESERVED_DESCS;
#endif

	/* now calculate max _players_ based on max descs */
	max_descs = MIN(MAX_PLAYERS, max_descs - NUM_RESERVED_DESCS);

	if (max_descs <= 0) {
		log("SYSERR: Non-positive max player limit!  (Set at %d using %s).", max_descs, method);
		exit(1);
	}
	log("   Setting player limit to %d using %s.", max_descs, method);
	return max_descs;
}



/*
 * game_loop contains the main loop which drives the entire MUD.  It
 * cycles once every 0.10 seconds and is responsible for accepting new
 * new connections, polling existing connections for input, dequeueing
 * output and sending it out to players, and calling "heartbeat" functions
 * such as mobile_activity().
 */
void game_loop(int mother_desc) 
{
	fd_set input_set, output_set, exc_set, null_set;
	struct timeval last_time, before_sleep, opt_time, process_time, now, timeout;
	char *comm;
	Descriptor *d;
	Editor *tmpeditor = NULL;
	SInt32	missed_pulses = 0, maxdesc = 0, aliased;

	/* initialize various time values */
	null_time.tv_sec = 0;
	null_time.tv_usec = 0;
	opt_time.tv_usec = OPT_USEC;
	opt_time.tv_sec = 0;
	FD_ZERO(&null_set);

	gettimeofday(&last_time, (struct timezone *) 0);
	
	LListIterator<Descriptor *>	iter(descriptor_list);

  /* The Main Loop.  The Big Cheese.  The Top Dog.  The Head Honcho.  The.. */
	while (!circle_shutdown) {
/* Sleep if we don't have any connections */
		if (descriptor_list.Count() == 0) {
//			log("No connections.  Going to sleep.");
//			Removed to prevent log spam, because of IMC
			FD_ZERO(&input_set);
			FD_ZERO(&output_set);	//	IMC
			FD_ZERO(&exc_set);		//	IMC
			FD_SET(mother_desc, &input_set);
			
			if (select(mother_desc + 1, &input_set, (fd_set *) 0, (fd_set *) 0, NULL) < 0) {
				if (errno == EINTR)	log("Waking up to process signal.");
				else				perror("SYSERR: Select coma");
			}// else
			//	log("New connection.  Waking up.");
			gettimeofday(&last_time, (struct timezone *) 0);
		}
		//	Set up the input, output, and exception sets for select().
		FD_ZERO(&input_set);
		FD_ZERO(&output_set);
		FD_ZERO(&exc_set);
		FD_SET(mother_desc, &input_set);

		maxdesc = mother_desc;

		iter.Reset();
		while ((d = iter.Next())) {
			if (d->descriptor > maxdesc)
				maxdesc = d->descriptor;
			FD_SET(d->descriptor, &input_set);
			FD_SET(d->descriptor, &output_set);
			FD_SET(d->descriptor, &exc_set);
		}
		
		/*
		 * At this point, we have completed all input, output and heartbeat
		 * activity from the previous iteration, so we have to put ourselves
		 * to sleep until the next 0.1 second tick.  The first step is to
		 * calculate how long we took processing the previous iteration.
		 */

		gettimeofday(&before_sleep, (struct timezone *) 0); /* current time */
		process_time = before_sleep - last_time;

		/*
		 * If we were asleep for more than one pass, count missed pulses and sleep
		 * until we're resynchronized with the next upcoming pulse.
		 */
		if (process_time.tv_sec == 0 && process_time.tv_usec < OPT_USEC) {
			missed_pulses = 0;
		} else {
			missed_pulses = process_time.tv_sec RL_SEC;
			missed_pulses += process_time.tv_usec / OPT_USEC;
			process_time.tv_sec = 0;
			process_time.tv_usec = process_time.tv_usec % OPT_USEC;
		}

		/* Calculate the time we should wake up */
		last_time = before_sleep + (opt_time - process_time);

		/* Now keep sleeping until that time has come */
		gettimeofday(&now, (struct timezone *) 0);
		timeout = last_time - now;

		/* Go to sleep */
		do {
			if (select(0, (fd_set *) 0, (fd_set *) 0, (fd_set *) 0, &timeout) < 0) {
				if (errno != EINTR) {
					perror("SYSERR: Select sleep");
					exit(1);
				}
			}
			gettimeofday(&now, (struct timezone *) 0);
			timeout = last_time - now;
		} while (timeout.tv_usec || timeout.tv_sec);

		/* Poll (without blocking) for new input, output, and exceptions */
		if (select(maxdesc + 1, &input_set, &output_set, &exc_set, &null_time) < 0) {
			perror("SYSERR: Select poll");
			return;
		}
		
		//	Accept new connections
		if (FD_ISSET(mother_desc, &input_set))
			new_descriptor(mother_desc);
		
		//	Check for completed lookups
		if (Identd::lookups.Count() > 0)
			Identd::Receive();
		
		//	Kick out the freaky folks in the exception set
		iter.Reset();
		while ((d = iter.Next())) {
			if (FD_ISSET(d->descriptor, &exc_set)) {
				FD_CLR(d->descriptor, &input_set);
				FD_CLR(d->descriptor, &output_set);
				close_socket(d);
			}
		}
		
		//	Process descriptors with input pending
		iter.Reset();
		while ((d = iter.Next())) {
			if (FD_ISSET(d->descriptor, &input_set))
				if (process_input(d) < 0)
					close_socket(d);
		}
		
		/* Process commands we just read from process_input */
		comm = get_buffer(MAX_STRING_LENGTH);
		iter.Reset();
		while ((d = iter.Next())) {
			d->wait -= (d->wait > 0);
			if (d->wait > 0)							continue;
			if (!get_from_q(&d->input, comm, &aliased))	continue;
			if (d->character) {
			//	Reset the idle timer & pull char back from void if necessary
				d->character->player->timer = 0;
				if ((STATE(d) == CON_PLAYING) && (GET_WAS_IN(d->character) != NOWHERE)) {
					if (IN_ROOM(d->character) != NOWHERE)
						d->character->FromRoom();
					d->character->ToRoom(GET_WAS_IN(d->character));
					GET_WAS_IN(d->character) = NOWHERE;
					act("$n has returned.", TRUE, d->character, 0, 0, TO_ROOM);
				}
			}
			d->wait = 1;
			d->has_prompt = 0;

			if (d->showstr_count)				show_string(d, comm);	//	Reading something w/ pager
			else if (d->Edit()) {
				tmpeditor = (Editor *) d->Edit();
				tmpeditor->Parse(comm);	//	Writing boards, mail, etc.
			}
			else if (STATE(d) != CON_PLAYING)	nanny(d, comm);			//	in menus, etc.
			else {														//	We're playing normally
				if (aliased)	d->has_prompt = 1;			//	to prevent recursive aliases
				else if (perform_alias(d, comm))			//	run it through aliasing system
					get_from_q(&d->input, comm, &aliased);
				command_interpreter(d->character, comm);	//	send it to interpreter
			}
		}
		release_buffer(comm);
		
		//	send queued output out to the operating system (ultimately to user)
		iter.Reset();
		while ((d = iter.Next())) {
			if (*(d->output) && FD_ISSET(d->descriptor, &output_set)) {
				//	Output for this player is ready
				if (process_output(d) < 0)		close_socket(d);
				else							d->has_prompt = 1;
			}
		}
		
		if (circle_copyover)	break;
		
		//	Print prompts for other descriptors who had no other output
		//	Merged these together, also.  Seems safe to also do the ELSE, because
		//	if the previous IF is called, either d would be removed from list,
		//	or d->has_prompt would be set, therefore would be skipped during
		//	prompt making
		iter.Reset();
		while ((d = iter.Next())) {
			if (!d->has_prompt && (STATE(d) != CON_DISCONNECT)) {
				write_to_descriptor(d->descriptor, make_prompt(d));
				d->has_prompt = 1;
			}
		}

		/* kick out folks in the CON_CLOSE state */
		iter.Reset();
		while ((d = iter.Next())) {
			if ((STATE(d) == CON_CLOSE) || (STATE(d) == CON_DISCONNECT))
				close_socket(d);
		}

		//	Now, we execute as many pulses as necessary--just one if we haven't
		//	missed any pulses, or make up for lost time if we missed a few
		//	pulses by sleeping for too long.
		missed_pulses++;

		if (missed_pulses <= 0) {
			log("SYSERR: **BAD** MISSED_PULSES IS NONPOSITIVE (%d), TIME GOING BACKWARDS!!!", missed_pulses / PASSES_PER_SEC);
			missed_pulses = 1;
		}

		/* If we missed more than 30 seconds worth of pulses, forget it */
		if (missed_pulses > (30 RL_SEC)) {
			log("SYSERR: Missed more than 30 seconds worth of pulses");
			missed_pulses = 30 RL_SEC;
		}

		/* Now execute the heartbeat functions */
		while (missed_pulses--)		heartbeat(++pulse);

		tics++;		//	Update tics for deadlock protection (UNIX only)
	}
}


void heartbeat(int pulse) {
	static UInt32 mins_since_crashsave = 0;
	
	Event::Process();
	
	if (!(pulse % PULSE_ZONE))					zone_update();
	if (!(pulse % (15 RL_SEC)))					check_idle_passwords();		/* 15 seconds */
	if (!(pulse % PULSE_MOBILE))				character_update(pulse);
	if (!(pulse % PULSE_VIOLENCE))				perform_violence();
	if (!(pulse % PULSE_SCRIPT))				script_trigger_check();
	
	if (!(pulse % (SECS_PER_MUD_HOUR * PASSES_PER_SEC))) {
		weather_and_time(!Number(0, 3));
		hour_update();
		free_purged_lists();
	}
	if (!(pulse % PULSE_POINTS))				point_update();
	/* Clear out all the global buffers now in case someone forgot. */
	if (!(pulse % PULSE_BUFFER))				Buffer::ReleaseAll();
	
	// CHANGEPOINT:  This is related to space.C
	// if (!(pulse % 1))			MoveShips();
	// if (!(pulse % 10))			UpdateSpace();
	
	if (auto_save && !(pulse % (60 RL_SEC))) {	/* 1 minute */
		if (++mins_since_crashsave >= autosave_time) {
			mins_since_crashsave = 0;
			Crash_save_all();
			House::SaveAll();
//			clans_save_to_disk();
		}
	}
	
	if (!(pulse % (5 * 60 RL_SEC)))	record_usage();				/* 5 minutes */
}


/********************************************************************
*	general utility stuff (for local use)							*
*********************************************************************/

/*
 *	new code to calculate time differences, which works on systems
 *	for which tv_usec is unsigned (and thus comparisons for something
 *	being < 0 fail).  Based on code submitted by ss@sirocco.cup.hp.com.
 */

/*
 * code to return the time difference between a and b (a-b).
 * always returns a nonnegative value (floors at 0).
 */
//	Return time difference, floor at 0.  (a - b)
timeval operator-(timeval a, timeval b) {
	timeval rslt;

	if (a.tv_sec < b.tv_sec)				return null_time;
	else if (a.tv_sec == b.tv_sec) {
		if (a.tv_usec < b.tv_usec)			return null_time;
		else {
			rslt.tv_sec = 0;
			rslt.tv_usec = a.tv_usec - b.tv_usec;
			return rslt;
		}
	} else {			//	a->tv_sec > b->tv_sec
		rslt.tv_sec = a.tv_sec - b.tv_sec;
		if (a.tv_usec < b.tv_usec) {
			rslt.tv_usec = a.tv_usec + 1000000 - b.tv_usec;
			rslt.tv_sec--;
		} else
			rslt.tv_usec = a.tv_usec - b.tv_usec;
		return rslt;
	}
}


//	Add two time values
timeval operator+(timeval a, timeval b) {
	timeval rslt;

	rslt.tv_sec = a.tv_sec + b.tv_sec;
	rslt.tv_usec = a.tv_usec + b.tv_usec;

	while (rslt.tv_usec >= 1000000) {
		rslt.tv_usec -= 1000000;
		rslt.tv_sec++;
	}

	return rslt;
}


void record_usage(void) {
	int sockets_playing = 0;
	Descriptor *d;
	LListIterator<Descriptor *>	iter(descriptor_list);
	
	while ((d = iter.Next()))
		if (STATE(d) == CON_PLAYING)
			sockets_playing++;

	log("nusage: %-3d sockets connected, %-3d sockets playing",
			descriptor_list.Count(), sockets_playing);

#ifdef RUSAGE
	{
		struct rusage ru;

		getrusage(RUSAGE_SELF, &ru);
		log("rusage: user time: %d sec, system time: %d sec, max res size: %d",
				ru.ru_utime.tv_sec, ru.ru_stime.tv_sec, ru.ru_maxrss);
	}
#endif
}


//	Turn off echoing (specific to telnet client)
void echo_off(Descriptor *d) {
	char off_string[] = {
		(char) IAC,
		(char) WILL,
		(char) TELOPT_ECHO,
		(char) 0,
	};

	d->Write(off_string);
}


//	Turn on echoing (specific to telnet client)
void echo_on(Descriptor *d) {
	char on_string[] = {
		(char) IAC,
		(char) WONT,
		(char) TELOPT_ECHO,
		(char) TELOPT_NAOFFD,
		(char) TELOPT_NAOCRD,
		(char) 0,
	};

	d->Write(on_string);
}


char *colorpoints(int percent)
{
  static char buf[8];

  if (percent >= 90)
    strcpy(buf, "&cg");
  else if ((percent >= 50) && (percent <= 89))
    strcpy(buf, "&cy");
  else if ((percent >= 30) && (percent <= 49))
    strcpy(buf, "&cm");
  else
    strcpy(buf, "&cr");

    return(buf);
}


char *colorsays(int percent)
{
  static char buf[20];

    sprintf(buf, "%s",
                  (percent >= 90 ? "&cgunscathed" :
                   percent >= 75 ? "&cyscratched" :
                   percent >= 50 ? "&cmbeaten-up" :
                   percent >= 25 ? "&crbloody"    : "&crnear death"));
    return(buf); 
}                


char *prompt_str(Character *ch)
{
	register char *place;
	char *orig, *i;
	static char outbuf[MAX_STRING_LENGTH];
	Character *victim = NULL, *tank = NULL;
	int perc;  
  
	if (!IS_MOB(ch)) {
		if (GET_PROMPT(ch) && *GET_PROMPT(ch)) {
			strcpy(buf1, GET_PROMPT(ch));
		} else {
			strcpy(buf1, "$I$chH $cmM $cvV $o$t&c0-> ");
		}
	} else {
		strcpy(buf1, "M $chH $cmM $cvV $o$t&c0-> ");
	}

	*buf2 = '\0';		// We're using buf2 for a temporary buffer; clear it.
	orig = buf1;		// Set up the pointer for the original string
	place = outbuf;	// Set up the pointer for the output string
  
	if (FIGHTING(ch) && !PURGED(FIGHTING(ch)) && 
		 (IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))) {
		victim = FIGHTING(ch);
	}
  
	if (victim && FIGHTING(victim) && !PURGED(FIGHTING(victim)) && 
		 (IN_ROOM(victim) == IN_ROOM(FIGHTING(victim)))) {
		tank = FIGHTING(victim);
	}
  
	while (*orig) {
		if (*orig == '$' && *(++orig)) {
			switch (*orig) {
			case 'i': // Are we invisible?
				if (GET_INVIS_LEV(ch))
					sprintf(buf2, "%d", GET_INVIS_LEV(ch));
				if (GET_STAFF_INVIS(ch))
					sprintf(buf2 + strlen(buf2), "%s%d ", GET_INVIS_LEV(ch) ? "/" : "", GET_STAFF_INVIS(ch));
				break;	 
			case 'I':	
				if (GET_INVIS_LEV(ch))
					sprintf(buf2, "i%d ", GET_INVIS_LEV(ch));
				if (GET_STAFF_INVIS(ch))
					sprintf(buf2 + strlen(buf2), "I%d ", GET_STAFF_INVIS(ch));
				break;	 
				case 'h': // current hitp
				sprintf(buf2, "%d", GET_HIT(ch));
					break;
				case 'H': // maximum hitp
				sprintf(buf2, "%d", GET_MAX_HIT(ch));
				break;	 
			case 'm': // current mana
				sprintf(buf2, "%d", GET_MANA(ch));
				break;	 
			case 'M': // maximum mana
				sprintf(buf2, "%d", GET_MAX_MANA(ch));
					break;
				case 'v': // current moves
				sprintf(buf2, "%d", GET_MOVE(ch));
					break;
				case 'V': // maximum moves
				sprintf(buf2, "%d", GET_MAX_MOVE(ch));
				break;	 
			case 'C':	
			case 'c':	
				++orig;	 
				switch (LOWER(*orig)) {
				case 'h':
					perc = (int) (100 * GET_HIT(ch)) / MAX(GET_MAX_HIT(ch), 1);
					sprintf(buf2, "%s%d", colorpoints(perc), GET_HIT(ch));
					break; 
				case 'm':
					perc = (int) (100 * GET_MANA(ch)) / MAX(GET_MAX_MANA(ch), 1);
					sprintf(buf2, "%s%d", colorpoints(perc), GET_MANA(ch));
					break; 
				case 'v':
					perc = (int) (100 * GET_MOVE(ch)) / MAX(GET_MAX_MOVE(ch), 1);
					sprintf(buf2, "%s%d", colorpoints(perc), GET_MOVE(ch));
					break; 
				default :
					break; 
				}
					break;
				case 'P':
				case 'p': // percentage of hitp/move/mana
				++orig;	 
				switch (LOWER(*orig)) {
						case 'h':
					perc = (int) (100 * GET_HIT(ch)) / MAX(GET_MAX_HIT(ch), 1);
					break; 
				case 'm':
					perc = (int) (100 * GET_MANA(ch)) / MAX(GET_MAX_MANA(ch), 1);
							break;
						case 'v':
					perc = (int) (100 * GET_MOVE(ch)) / MAX(GET_MAX_MOVE(ch), 1);
					break; 
				case 'x':
					perc = (100 * GET_EXP(ch)) / EXP_TO_LEVEL;
							break;
						default :
							perc = 0;
							break;
					}
				sprintf(buf2, "%s%d%%", colorpoints(perc), perc);
					break;
			case 'o':	
				if (victim) {
					perc = (int) (100 * GET_HIT(victim)) / MAX(GET_MAX_HIT(victim), 1);
					sprintf(buf2, ": %s &cb(%s&cb)&c0 ", PERS(victim, ch), colorsays(perc));
				} else { 
					++orig; 
					continue;
				}				
				break;	 
			case 'O': // opponent
				if (victim) {
					perc = (int) (100 * GET_HIT(victim)) / MAX(GET_MAX_HIT(victim), 1);
					sprintf(buf2, ": %s %s&c0 ", PERS(victim, ch), colorsays(perc));
					} else {
					++orig; 
						continue;
					}
					break;
			case 'x': // current exp
				sprintf(buf2, "%d", GET_EXP(ch));
				break;	 
			case 'X': // exp to level
				sprintf(buf2, "%d", (EXP_TO_LEVEL - GET_EXP(ch)));
				break;	 
			case 'g': // gold on hand
				sprintf(buf2, "%d", GET_GOLD(ch));
				break;	 
			case 'G': // gold in bank
				sprintf(buf2, "%d", GET_BANK_GOLD(ch));
				break;	 
				case 'T':
				if (victim && (tank = FIGHTING(victim)) && tank != ch) {
					perc = (int) (100 * GET_HIT(tank)) / MAX(GET_MAX_HIT(tank), 1);
					sprintf(buf2, ": %s %s&c0 ", PERS(tank, ch), colorsays(perc));
				} else { 
					++orig;
					continue;
				}				
				break;	 
				case 't': // tank
				if (victim && (tank = FIGHTING(victim)) && tank != ch) {
					perc = (int) (100 * GET_HIT(tank)) / MAX(GET_MAX_HIT(tank), 1);
					sprintf(buf2, ": %s &cb(%s&cb)&c0 ", PERS(tank, ch), colorsays(perc));
					} else {
					++orig; 
						continue;
					}
					break;

				case '_':
				strcpy(buf2, "\r\n");
					break;
			case '$':	
				strcpy(buf2, "$");
				++orig;	 
					continue;
				break;	 

				default :
				++orig;	 
					continue;
				break;	 
			}

			i = buf2;

			while ((*place = *(i++)))
				++place;
			++orig;

			*buf2 = '\0';

		} else if (!(*(place++) = *(orig++)))
			break;
		
	}
  
	*place = '\0';

	return (outbuf);
  
}


char *make_prompt(Descriptor *d) {
	static char prompt[512];
	Editor * tmpeditor = NULL;
	//	Note, prompt is truncated at MAX_PROMPT_LENGTH chars (structs.h )
	
	//	reversed these top 2 if checks so that page_string() would work in
	//	the editor */
	if (d->showstr_count) {
		sprintf(prompt,
		"\r&c+&b+&cW&bb[ Return to continue, (q)uit, (r)efresh, (b)ack, or page number (%d/%d) ]&b0&c-&b-",
				d->showstr_page, d->showstr_count);
//		write_to_descriptor(d->descriptor, prompt);
	} else if (d->Edit()) {
		tmpeditor = (Editor *) d->Edit();
		strcpy(prompt, tmpeditor->Prompt());
	} else if (STATE(d) == CON_PLAYING) {
		*prompt = '\0';

		if (!IS_NPC(d->character) && GET_AFK(d->character))
			strcat(prompt, "&cC[AFK]&c0");
		strcat(prompt, prompt_str(d->character));
		if(d->character)
			proc_color(prompt, PRF_FLAGGED(d->character, Preference::Color)); 
	} else
		*prompt = '\0';
	
	return prompt;
}


void write_to_q(const char *txt, struct txt_q *queue, int aliased) {
	struct txt_block *new_text;

	CREATE(new_text, struct txt_block, 1);
	new_text->text = str_dup(txt);
	new_text->aliased = aliased;

	//	queue empty?
	if (!queue->head) {
		new_text->next = NULL;
		queue->head = queue->tail = new_text;
	} else {
		queue->tail->next = new_text;
		queue->tail = new_text;
		new_text->next = NULL;
	}
}



bool get_from_q(struct txt_q *queue, char *dest, int *aliased) {
	struct txt_block *tmp;

	//	queue empty?
	if (!queue->head)
		return false;

	tmp = queue->head;
	strcpy(dest, queue->head->text);
	*aliased = queue->head->aliased;
	queue->head = queue->head->next;

	FREE(tmp->text);
	FREE(tmp);

	return true;
}



/* Empty the queues before closing connection */
void flush_queues(Descriptor *d) {
	int dummy;
	char *buf2 = get_buffer(MAX_STRING_LENGTH);
 
	//	As I understand this, it puts the buffer back on the list.
	//	So we don't need this anymore as it is.

	if (d->large_outbuf) {
#if USE_CIRCLE_SOCKET_BUF
		d->large_outbuf->next = bufpool;
		bufpool = d->large_outbuf;
#else
		release_buffer(d->large_outbuf);
		d->output = d->small_outbuf;
#endif
	}
	while (get_from_q(&d->input, buf2, &dummy));
	release_buffer(buf2);
}


/********************************************************************
*	socket handling													*
********************************************************************/



/* Sets the kernel's send buffer size for the descriptor */
int set_sendbuf(socket_t s) {
#ifdef SO_SNDBUF
	int opt = MAX_SOCK_BUF;
	if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char *) &opt, sizeof(opt)) < 0) {
		perror("SYSERR: setsockopt SNDBUF");
		return -1;
	}
#endif
	return 0;
}


int new_descriptor(int s) {
	socket_t desc;
	socklen_t i;
	Descriptor *newd;
	struct sockaddr_in peer;

	/* accept the new connection */
	i = sizeof(peer);
	if ((desc = accept(s, (struct sockaddr *) &peer, &i)) == INVALID_SOCKET) {
		perror("SYSERR: accept");
		return -1;
	}
	
	/* keep it from blocking */
	nonblock(desc);

	/* set the send buffer size if available on the system */
	if (set_sendbuf(desc) < 0) {
		CLOSE_SOCKET(desc);
		return 0;
	}

	/* make sure we have room for it */
	if (descriptor_list.Count() >= max_players) {
		write_to_descriptor(desc, "Sorry, Struggle for Nyrilis is full right now... please try again later!\r\n");
		CLOSE_SOCKET(desc);
		return 0;
	}
  
  /* Make sure the socket is valid... */

	if ((write_to_descriptor(desc, "Validating socket, looking up hostname.")) < 0) {
		//close(desc);
		CLOSE_SOCKET(desc);
		return 0;
	}

	/* create a new descriptor */
//	CREATE(newd, Descriptor, 1);
//	memset((char *) newd, 0, sizeof(Descriptor));
	
	newd = new Descriptor(desc);

	/* prepend to list */
	descriptor_list.Add(newd);

	STATE(newd) = CON_QANSI;
	newd->saddr = peer;
	strcpy(newd->host_ip, inet_ntoa(newd->saddr.sin_addr));
	strcpy(newd->host	, newd->host_ip);
	Identd::Lookup(newd);
	if (STATE(newd) != CON_QANSI)
		write_to_descriptor(desc, "  This may take a moment.");
	write_to_descriptor(desc, "\r\n");

	return 0;
}


/*
 * Send all of the output that we've accumulated for a player out to
 * the player's descriptor.
 * FIXME - This will be rewritten before 3.1, this code is dumb.
 */
int process_output(Descriptor *t) {
	char i[MAX_SOCK_BUF];
	int written = 0, offset, result;

	strcpy(i, "\r\n");	//	we may need this \r\n for later -- see below

	strcpy(i + 2, t->output);	//	now, append the 'real' output

	if (t->bufptr < 0)	//	if we're in the overflow state, notify the user
		strcat(i, "**OVERFLOW**\r\n");

	//	add the extra CRLF if the person isn't in compact mode
	if ((STATE(t) == CON_PLAYING) && t->character && !PRF_FLAGGED(t->character, Preference::Compact))
		strcat(i + 2, "\r\n");

	//	add a prompt
	strncat(i + 2, make_prompt(t), MAX_PROMPT_LENGTH);

	if (t->character)
		proc_color(i, PRF_FLAGGED(t->character, Preference::Color));

	//	 now, send the output.  If this is an 'interruption', use the prepended
	//	CRLF, otherwise send the straight output sans CRLF.
//	if (t->has_prompt)	result = write_to_descriptor(t->descriptor, i);
//	else				result = write_to_descriptor(t->descriptor, i + 2);
	if (t->has_prompt)	offset = 0;
	else				offset = 2;

	result = write_to_descriptor(t->descriptor, i + offset);
	written = result >= 0 ? result : -result;
	
	//	handle snooping: prepend "% " and send to snooper
	if (t->snoop_by)
		t->snoop_by->Writef("%% %s%%%%", t->output);
	
	//	if we were using a large buffer, put the large buffer on the buffer pool
	//	and switch back to the small one
	if (t->large_outbuf) {
#if USE_CIRCLE_SOCKET_BUF
		t->large_outbuf->next = bufpool;
		bufpool = t->large_outbuf;
		t->large_outbuf = NULL;
#else
		release_buffer(t->large_outbuf);
#endif
		t->output = t->small_outbuf;
	}
	/* reset total bufspace back to that of a small buffer */
	t->bufspace = SMALL_BUFSIZE - 1;
	t->bufptr = 0;
	*(t->output) = '\0';
	
//	return result;
	if (result == 0)		return -1;		//	Error, cut off.
	if (result > 0)			return 1;		//	Normal case
	
	//	Blocked...
	t->Writef("%s", i + written + offset);
	return 0;
}



/*
 * perform_socket_write: takes a descriptor, a pointer to text, and a
 * text length, and tries once to send that text to the OS.  This is
 * where we stuff all the platform-dependent stuff that used to be
 * ugly #ifdef's in write_to_descriptor().
 *
 * This function must return:
 *
 * -1  If a fatal error was encountered in writing to the descriptor.
 *  0  If a transient failure was encountered (e.g. socket buffer full).
 * >0  To indicate the number of bytes successfully written, possibly
 *     fewer than the number the caller requested be written.
 *
 * Right now there are two versions of this function: one for Windows,
 * and one for all other platforms.
 */

/* perform_socket_write for all Non-Windows platforms */
ssize_t perform_socket_write(socket_t desc, const char *txt, size_t length) {
	ssize_t result;

	result = write(desc, txt, length);

	if (result > 0) {
		/* Write was successful. */
		return result;
	}

	if (result == 0) {
		/* This should never happen! */
		log("SYSERR: Huh??  write() returned 0???  Please report this!");
		return -1;
	}

	/*
	 * result < 0, so an error was encountered - is it transient?
	 * Unfortunately, different systems use different constants to
	 * indicate this.
	 */

#ifdef EAGAIN		/* POSIX */
	if (errno == EAGAIN)
		return 0;
#endif

#ifdef EWOULDBLOCK	/* BSD */
	if (errno == EWOULDBLOCK)
		return 0;
#endif

#ifdef EDEADLK		/* Macintosh */
	if (errno == EDEADLK)
		return 0;
#endif

	/* Looks like the error was fatal.  Too bad. */
	return -1;
}


int write_to_descriptor(socket_t desc, const char *txt) {
	ssize_t bytes_written, total_written = 0;
	size_t total = strlen(txt);

	while (total > 0) {
		bytes_written = perform_socket_write(desc, txt, total);
		if (bytes_written < 0) {
			perror("SYSERR: write_to_descriptor");
			return 0;
		} else if (bytes_written == 0) {
			/*
			 * Temporary failure -- socket buffer full.  For now we'll just
			 * cut off the player, but eventually we'll stuff the unsent
			 * text into a buffer and retry the write later.  JE 30 June 98.
			 * Implemented the anti-cut-off code he wanted. GG 13 Jan 99.
			 */
			log("WARNING: write_to_descriptor: socket write would block");
			return -total_written;
		} else {
			txt += bytes_written;
			total -= bytes_written;
			total_written += bytes_written;
		}
	}

	return total_written;
}


 /*
 * Same information about perform_socket_write applies here. I like
 * standards, there are so many of them. -gg 6/30/98
 */
ssize_t perform_socket_read(socket_t desc, char *read_point, size_t space_left) {
	ssize_t ret;

	ret = read(desc, read_point, space_left);

	/* Read was successful. */
	if (ret > 0)
		return ret;

	/* read() returned 0, meaning we got an EOF. */
	if (ret == 0) {
		log("WARNING: EOF on socket read (connection broken by peer)");
		return -1;
	}

	/*
	 * read returned a value < 0: there was an error
	 */

#ifdef EINTR		/* Interrupted system call - various platforms */
	if (errno == EINTR)
		return 0;
#endif

#ifdef EAGAIN		/* POSIX */
	if (errno == EAGAIN)
		return 0;
#endif

#ifdef EWOULDBLOCK	/* BSD */
	if (errno == EWOULDBLOCK)
		return 0;
#endif /* EWOULDBLOCK */

#ifdef EDEADLK		/* Macintosh */
	if (errno == EDEADLK)
		return 0;
#endif

	/* We don't know what happened, cut them off. */
	perror("SYSERR: process_input: about to lose connection");
	return -1;
}


//	ASSUMPTION: There will be no newlines in the raw input buffer when this
//	function is called.  We must maintain that before returning.
int process_input(Descriptor *t) {
	int buf_length, failed_subst;
	ssize_t bytes_read;
	size_t space_left;
	char *ptr, *read_point, *write_point, *nl_pos = NULL;
	char *tmp;

	//	first, find the point where we left off reading data
	buf_length = strlen(t->inbuf);
	read_point = t->inbuf + buf_length;
	space_left = MAX_RAW_INPUT_LENGTH - buf_length - 1;

	do {
		if (space_left <= 0) {
			log("WARNING: process_input: about to close connection: input overflow");
			return -1;
		}
		bytes_read = perform_socket_read(t->descriptor, read_point, space_left);
		
		if (bytes_read < 0)			return -1;
		else if (bytes_read == 0)	return 0;
		//	at this point, we know we got some data from the read

		*(read_point + bytes_read) = '\0';	/* terminate the string */

		//	search for a newline in the data we just read
		for (ptr = read_point; *ptr && !nl_pos; ptr++)
			if (ISNEWL(*ptr))
				nl_pos = ptr;

		read_point += bytes_read;
		space_left -= bytes_read;

/*
 * on some systems such as AIX, POSIX-standard nonblocking I/O is broken,
 * causing the MUD to hang when it encounters input not terminated by a
 * newline.  This was causing hangs at the Password: prompt, for example.
 * I attempt to compensate by always returning after the _first_ read, instead
 * of looping forever until a read returns -1.  This simulates non-blocking
 * I/O because the result is we never call read unless we know from select()
 * that data is ready (process_input is only called if select indicates that
 * this descriptor is in the read set).  JE 2/23/95.
 */
	}
#if !defined(POSIX_NONBLOCK_BROKEN)
	while (!nl_pos);
#else
	while (0);

	if (!nl_pos)	return 0;
#endif /* POSIX_NONBLOCK_BROKEN */

	/*
	 * okay, at this point we have at least one newline in the string; now we
	 * can copy the formatted data to a new array for further processing.
	 */

	read_point = t->inbuf;
	
	tmp = get_buffer(MAX_INPUT_LENGTH + 8);
	while (nl_pos != NULL) {
		write_point = tmp;
		space_left = MAX_INPUT_LENGTH - 1;

		for (ptr = read_point; (space_left > 0) && (ptr < nl_pos); ptr++) {
			if (*ptr == '\b' || *ptr == 127) {	/* handle backspacing or delete key */
				if (write_point > tmp) {
					if (*(--write_point) == '$') {
						write_point--;
						space_left += 2;
					} else
						space_left++;
				}
			} else if (isascii(*ptr) && isprint(*ptr)) {
				if ((*(write_point++) = *ptr) == '$') {		/* copy one character */
					*(write_point++) = '$';	/* if it's a $, double it */
					space_left -= 2;
				} else
					space_left--;
			}
		}

		*write_point = '\0';

		if ((space_left <= 0) && (ptr < nl_pos)) {
			char *buffer = get_buffer(MAX_INPUT_LENGTH + 64);

			sprintf(buffer, "Line too long.  Truncated to:\r\n%s\r\n", tmp);
			if (write_to_descriptor(t->descriptor, buffer) == 0) {	//	Descriptor::Write ?
				release_buffer(tmp);
				release_buffer(buffer);
				return -1;
			}
			release_buffer(buffer);
		}
		if (t->snoop_by)
			t->snoop_by->Writef("%% %s\r\n", tmp);
		failed_subst = 0;

		if (*tmp == '!' && !(*(tmp + 1)))
			strcpy(tmp, t->last_input);
		else if (*tmp == '!' && *(tmp + 1)) {
			char *commandln = (tmp + 1);
			int starting_pos = t->history_pos,
			cnt = (t->history_pos == 0 ? HISTORY_SIZE - 1 : t->history_pos - 1);

			skip_spaces(commandln);
			for (; cnt != starting_pos; cnt--) {
				if (t->history[cnt] && is_abbrev(commandln, t->history[cnt])) {
					strcpy(tmp, t->history[cnt]);
					strcpy(t->last_input, tmp);
					t->Writef("%s\r\n", tmp);
					break;
				}
				if (cnt == 0)	/* At top, loop to bottom. */
					cnt = HISTORY_SIZE;
			}
		} else if (*tmp == '^') {
			if (!(failed_subst = perform_subst(t, t->last_input, tmp)))
				strcpy(t->last_input, tmp);
		} else {
			strcpy(t->last_input, tmp);
			if (t->history[t->history_pos])
				free(t->history[t->history_pos]);		//	Clear the old line
			t->history[t->history_pos] = str_dup(tmp);	//	Save the new
			if (++t->history_pos >= HISTORY_SIZE)		//	Wrap to top
				t->history_pos = 0;
		}

		if (!failed_subst)
			write_to_q(tmp, &t->input, 0);

		/* find the end of this line */
		while (ISNEWL(*nl_pos))
			nl_pos++;

		/* see if there's another newline in the input buffer */
		read_point = ptr = nl_pos;
		for (nl_pos = NULL; *ptr && !nl_pos; ptr++)
			if (ISNEWL(*ptr))
				nl_pos = ptr;
	}

	/* now move the rest of the buffer up to the beginning for the next pass */
	write_point = t->inbuf;
	while (*read_point)
		*(write_point++) = *(read_point++);
	*write_point = '\0';

	release_buffer(tmp);
	return 1;
}



/*
 * perform substitution for the '^..^' csh-esque syntax
 * orig is the orig string (i.e. the one being modified.
 * subst contains the substition string, i.e. "^telm^tell"
 */
int perform_subst(Descriptor *t, char *orig, char *subst) {
	char *new_t = get_buffer(MAX_INPUT_LENGTH + 5);
	char *first, *second, *strpos;

	//	first is the position of the beginning of the first string (the one
	//	to be replaced
	first = subst + 1;

	//	now find the second '^'
	if (!(second = strchr(first, '^'))) {
		t->Write("Invalid substitution.\r\n");
		release_buffer(new_t);
		return 1;
	}
	//	terminate "first" at the position of the '^' and make 'second' point
	//	to the beginning of the second string
	*(second++) = '\0';

	//	now, see if the contents of the first string appear in the original
	if (!(strpos = strstr(orig, first))) {
		t->Write("Invalid substitution.\r\n");
		release_buffer(new_t);
		return 1;
	}
	//	now, we construct the new string for output.

	//	first, everything in the original, up to the string to be replaced
	strncpy(new_t, orig, (strpos - orig));
	new_t[(strpos - orig)] = '\0';

	//	now, the replacement string
	strncat(new_t, second, (MAX_INPUT_LENGTH - strlen(new_t) - 1));

	//	now, if there's anything left in the original after the string to replaced,
	//	copy that too.
	if (((strpos - orig) + strlen(first)) < strlen(orig))
		strncat(new_t, strpos + strlen(first), (MAX_INPUT_LENGTH - strlen(new_t) - 1));

	//	terminate the string in case of an overflow from strncat
	new_t[MAX_INPUT_LENGTH - 1] = '\0';
	strcpy(subst, new_t);
  
	release_buffer(new_t);
	return 0;
}



void close_socket(Descriptor *d) {
	descriptor_list.Remove(d);
	Editor * tmpeditor = NULL;
	
	CLOSE_SOCKET(d->descriptor);
	flush_queues(d);

	/* Forget snooping */
	if (d->snooping)
		d->snooping->snoop_by = NULL;

	if (d->snoop_by) {
		d->snoop_by->Write("Your victim is no longer among us.\r\n");
		d->snoop_by->snooping = NULL;
	}

	while (d->Edit()) {
		tmpeditor = (Editor *) d->Edit();
		tmpeditor->Finish();
	}
	
	if (d->character) {
// 		if (PLR_FLAGGED(d->character, PLR_MAILING) && d->str) {
// 			if (*d->str)	free(*d->str);
// //			free (d->str);
// 			*d->str = NULL;
// 			d->str = NULL;
// 		}
		if ((STATE(d) == CON_PLAYING) || (STATE(d) == CON_DISCONNECT)) {
			if (IN_ROOM(d->character) != NOWHERE)
				act("$n has lost $s link.", TRUE, d->character, 0, 0, TO_ROOM);
			if (!IS_NPC(d->character)) {
				d->character->Save(d->character->AbsoluteRoom());
				mudlogf( NRM, NULL, TRUE,  "Closing link to: %s.", d->character->RealName() ? d->character->RealName() : "UNDEFINED");
				d->character->desc = NULL;
			}
			
			if (d->character->player->host)
				FREE(d->character->player->host);
			d->character->player->host = str_dup(d->host);
		} else {
			mudlogf(CMP, NULL, TRUE, "Losing player: %s.",
					d->character->SSName() ? d->character->Name() : "<null>");

			if (!PURGED(d->character)) {
				if (IN_ROOM(d->character) != NOWHERE)
					d->character->Extract();
				else {
					d->character->Purged(true);
					PurgedChars.Add(d->character);
				}
			}
		}
	} else
		mudlog("Losing descriptor without char.", CMP, NULL, TRUE);

	/* JE 2/22/95 -- part of my unending quest to make switch stable */
	if (d->original && d->original->desc)
		d->original->desc = NULL;

	delete d;
}



void check_idle_passwords(void) {
	Descriptor *d;

	START_ITER(iter, d, Descriptor *, descriptor_list) {
		if ((STATE(d) != CON_PASSWORD) && (STATE(d) != CON_GET_NAME) && (STATE(d) != CON_QANSI))
			continue;
		if (++d->idle_tics > 2) {
			echo_on(d);
			d->Write("\r\nTimed out... goodbye.\r\n");
			STATE(d) = CON_CLOSE;
		}
	}
}



/*
 * I tried to universally convert Circle over to POSIX compliance, but
 * alas, some systems are still straggling behind and don't have all the
 * appropriate defines.  In particular, NeXT 2.x defines O_NDELAY but not
 * O_NONBLOCK.  Krusty old NeXT machines!  (Thanks to Michael Jones for
 * this and various other NeXT fixes.)
 */

#ifndef O_NONBLOCK
#define O_NONBLOCK O_NDELAY
#endif

void nonblock(socket_t s) {
	int flags;

	flags = fcntl(s, F_GETFL, 0);
	flags |= O_NONBLOCK;
	if (fcntl(s, F_SETFL, flags) < 0) {
		perror("SYSERR: Fatal error executing nonblock (comm.c)");
		exit(1);
	}
}
/********************************************************************
*	signal-handling functions (formerly signals.c)					*
********************************************************************/

RETSIGTYPE checkpointing(int unused) {
	if (!tics) {
		log("SYSERR: CHECKPOINT shutdown: tics not updated. (Infinite Loop Suspected)");
//		raise(SIGSEGV);
		abort();
	} else
		tics = 0;
}


RETSIGTYPE reread_wizlists(int unused) {
	mudlog("Signal received - rereading wizlists.", CMP, NULL, TRUE);
	reboot_wizlists();
}


RETSIGTYPE unrestrict_game(int unused) {
	mudlog("Received SIGUSR2 - completely unrestricting game (emergent)", BRF, NULL, TRUE);
	
	for (int i = 0; i < bans.Count(); i++)
		delete bans[i];
	
	bans.Clear();

	circle_restrict = 0;
	num_invalid = 0;
#ifdef _PTHREADS
	pthread_exit(NULL);
#endif
}


RETSIGTYPE hupsig(int unused) {
	log("SYSERR: Received SIGHUP, SIGINT, or SIGTERM.  Shutting down...");
	exit(0);		//	perhaps something more elegant should substituted
}

/*
 * This is an implementation of signal() using sigaction() for portability.
 * (sigaction() is POSIX; signal() is not.)  Taken from Stevens' _Advanced
 * Programming in the UNIX Environment_.  We are specifying that all system
 * calls _not_ be automatically restarted for uniformity, because BSD systems
 * do not restart select(), even if SA_RESTART is used.
 *
 * Note that NeXT 2.x is not POSIX and does not have sigaction; therefore,
 * I just define it to be the old signal.  If your system doesn't have
 * sigaction either, you can use the same fix.
 *
 * SunOS Release 4.0.2 (sun386) needs this too, according to Tim Aldric.
 */

#ifndef POSIX
#define my_signal(signo, func) signal(signo, func)
#else
sigfunc *my_signal(int signo, sigfunc * func);

sigfunc *my_signal(int signo, sigfunc * func) {
	struct sigaction act, oact;

	act.sa_handler = func;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
#ifdef SA_INTERRUPT
	act.sa_flags |= SA_INTERRUPT;	/* SunOS */
#endif

	if (sigaction(signo, &act, &oact) < 0)
		return SIG_ERR;

	return oact.sa_handler;
}
#endif				/* NeXT */


/* clean up our zombie kids to avoid defunct processes */
RETSIGTYPE reap(int sig) {
//	log("Beginning reap...");
//	while (waitpid(-1, NULL, WNOHANG) > 0);
//	log("Finishing reap...");
//	my_signal(SIGCHLD, reap);
//	struct rusage	ru;
//	wait3(NULL, WNOHANG, &ru);
}


void signal_setup(void) {
#ifndef macintosh
	struct itimerval itime;
	struct timeval interval;

	//	Emergency unrestriction procedure
	my_signal(SIGUSR2, unrestrict_game);

	//	Deadlock Protection
	interval.tv_sec = 60;
	interval.tv_usec = 0;
	itime.it_interval = interval;
	itime.it_value = interval;
	setitimer(ITIMER_VIRTUAL, &itime, NULL);
	my_signal(SIGVTALRM, checkpointing);

	/* just to be on the safe side: */
	my_signal(SIGHUP, hupsig);
//	my_signal(SIGCHLD, reap);
#if	defined(SIGCLD)
	my_signal(SIGCLD, reap);
#elif defined(SIGCHLD)
	my_signal(SIGCHLD, reap);
#endif

#endif
//	my_signal(SIGABRT, hupsig);
//	my_signal(SIGFPE, hupsig);
//	my_signal(SIGILL, hupsig);
//	my_signal(SIGSEGV, hupsig);
	my_signal(SIGINT, hupsig);
	my_signal(SIGTERM, hupsig);
	my_signal(SIGPIPE, SIG_IGN);
	my_signal(SIGALRM, SIG_IGN);
}

/* ****************************************************************
*       Public routines for system-to-player-communication        *
**************************************************************** */


//void Character *ch->Send(const char *messg) {
//	if (!messg || !ch)
//		return;
//	ch->Send("%s", messg);
//	if (ch->desc)	ch->desc->Write("%s", messg);
//	else			act(messg, FALSE, ch, 0, 0, TO_CHAR);
//}


void send_to_all(const char *messg) {
	Descriptor *i;

	if (!messg || !*messg)
		return;
	
	LListIterator<Descriptor *>	iter(descriptor_list);
	while ((i = iter.Next())) {
		if (STATE(i) == CON_PLAYING)
			i->Write(messg);
	}
}


void send_to_zone(char *messg, int zone_rnum) {
	Descriptor *i;

	if (!messg || !*messg)
		return;

	LListIterator<Descriptor *>	iter(descriptor_list);
	while ((i = iter.Next())) {
		if ((STATE(i) == CON_PLAYING) && i->character && AWAKE(i->character) &&
				!PLR_FLAGGED(i->character, PLR_WRITING) &&
				(IN_ROOM(i->character) != NOWHERE) &&
				(world[IN_ROOM(i->character)].zone == zone_rnum))
			i->Write(messg);
	}
}


void send_to_outdoor(int realm, const char *messg, ...)
{
	Descriptor *i;
	va_list args;
	char send_buf[MAX_STRING_LENGTH];

	if (!messg || !*messg)
		return;

	va_start(args, messg);
	vsnprintf(send_buf, MAX_STRING_LENGTH, messg, args);
	va_end(args);

	LListIterator<Descriptor *>	iter(descriptor_list);
	while ((i = iter.Next())) {
		if (!i->connected && i->character && (STATE(i) == CON_PLAYING) &&
			AWAKE(i->character) && IS_SOMEWHERE(i->character) &&
			OUTSIDE(i->character) && (realm == world[IN_ROOM(i->character)].Realm()))
		i->Write(send_buf);
	}
}


void send_to_players(Character *ch, const char *messg) {
	Descriptor *i;

	if (!messg || !*messg)
		return;

	LListIterator<Descriptor *>	iter(descriptor_list);
	while ((i = iter.Next())) {
		if ((STATE(i) == CON_PLAYING) && i->character && i->character != ch &&
				!PLR_FLAGGED(i->character, PLR_WRITING))
			i->Write(messg);
	}
}


//void send_ToRoom(const char *messg, int room) {
//	Character *i;

//	if (!messg || !*messg || room == NOWHERE)
//		return;
	
//	START_ITER(iter, i, Character *, world[room].people) {
//		if (i->desc && (STATE(i->desc) == CON_PLAYING) && !PLR_FLAGGED(i, PLR_WRITING) && AWAKE(i))
//			i->desc->Write("%s", messg);
//	} END_ITER(iter);
//}


void send_to_playersf(Character *ch, const char *messg, ...) {
	Descriptor *i;
	char *send_buf;
	va_list args;

	if (!messg || !*messg)
		return;
 
 	send_buf = get_buffer(MAX_STRING_LENGTH);
 	
	va_start(args, messg);
	vsprintf(send_buf, messg, args);
	va_end(args);

	LListIterator<Descriptor *>	iter(descriptor_list);
	while ((i = iter.Next())) {
		if ((STATE(i) == CON_PLAYING) && i->character && i->character != ch &&
				!PLR_FLAGGED(i->character, PLR_WRITING))
			i->Write(send_buf);
	}
				
	release_buffer(send_buf);
}


void send_to_outdoorf(int realm, const char *messg, ...) {
	Descriptor *i;
	va_list args;
	char *send_buf;
 
	if (!messg || !*messg)
		return;
 	
 	send_buf = get_buffer(MAX_STRING_LENGTH);
 	
	va_start(args, messg);
	vsprintf(send_buf, messg, args);
	va_end(args);

	LListIterator<Descriptor *>	iter(descriptor_list);
	while ((i = iter.Next())) {
		if ((STATE(i) == CON_PLAYING) && i->character && AWAKE(i->character) &&
				OUTSIDE(i->character) && !PLR_FLAGGED(i->character, PLR_WRITING) &&
				(realm == world[IN_ROOM(i->character)].Realm()))
			i->Write(send_buf);
	}
	
	release_buffer(send_buf);
}


const char *ACTNULL = "<NULL>";

#define CHECK_NULL(pointer, expression) \
	if (!(pointer)) i = ACTNULL; else i = (expression);


/* higher-level communication: the act() function */
void perform_act(const char *orig, const Character *ch, const MUDObject *obj, CPtr vict_obj,
		Character *to) {
	const char *i = NULL;
	
	char *	lbuf = get_buffer(MAX_STRING_LENGTH);
	char *	buf = lbuf;
	
	Relation	relation = RELATION_NONE;
	const Character *	victim = NULL;
	const Object *		target = NULL;
	const char *		arg = NULL;
	int 				k = 0;

	for (;;) {
		if (*orig == '$') {
			switch (*(++orig)) {
				case 'n':
					i = PERS(ch, to);
					if (ch)		relation = ch->GetRelation(to);
					break;
				case 'N':
					victim = static_cast<const Character *>(vict_obj);
					CHECK_NULL(victim, PERS(victim, to));
					if (victim)	relation = to->GetRelation(victim);
					break;
				case 'm':
					i = HMHR(ch);
					break;
				case 'M':
					victim = static_cast<const Character *>(vict_obj);
					CHECK_NULL(victim, HMHR(victim));
					break;
				case 's':
					i = HSHR(ch);
					break;
				case 'S':
					victim = static_cast<const Character *>(vict_obj);
					CHECK_NULL(vict_obj, HSHR(victim));
					break;
				case 'e':
					i = HSSH(ch);
					break;
				case 'E':
					victim = static_cast<const Character *>(vict_obj);
					CHECK_NULL(victim, HSSH(victim));
					break;
				case 'o':
					CHECK_NULL(obj, OBJN(obj, to));
					break;
				case 'O':
					target = static_cast<const Object *>(vict_obj);
					CHECK_NULL(target, OBJN(target, to));
					break;
				case 'p':
					CHECK_NULL(obj, OBJS(obj, to));
					break;
				case 'P':
					target = static_cast<const Object *>(vict_obj);
					CHECK_NULL(target, OBJS(target, to));
					break;
				case 'a':
					CHECK_NULL(obj, SANA(obj));
					break;
				case 'A':
					target = static_cast<const Object *>(vict_obj);
					CHECK_NULL(target, SANA(target));
					break;
				case 'T':
					arg = static_cast<const char *>(vict_obj);
					CHECK_NULL(arg, arg);
					break;
				case 't':
					CHECK_NULL(obj, reinterpret_cast<const char *>(obj));
					break;
				case 'F':
					CHECK_NULL(vict_obj, fname(static_cast<const char *>(vict_obj)));
					break;
				case '%':
					if (to != ch)	i = "s";
					break;
				case '$':
					i = "$";
					break;
				default:
					log("SYSERR: Illegal $-code to act(): %c", *orig);
					log("SYSERR: %s", orig);
					break;
			}
#ifdef GOT_RID_OF_IT
			if (i) {
				if (relation != RELATION_NONE) {
					*buf++ = '&';
					*buf++ = 'c';
					*buf++ = relation_colors[relation];
				}
				
				while ((*buf = *(i++)))
					buf++;
				
				if (relation != RELATION_NONE) {
					*buf++ = '&';
					*buf++ = 'c';
					*buf++ = '0';
				}
				relation = RELATION_NONE;
			}
#else
			while ((*buf = *(i++)))
				buf++;
#endif
			orig++;
		} else if (!(*(buf++) = *(orig++)))
			break;
	}
	
	*(--buf) = '\r';
	*(++buf) = '\n';
	*(++buf) = '\0';

	// Daniel Rollings AKA Daniel Houghton's experimental change to prevent color codes from
	// interfering with act capitalization
	if ((lbuf[k] == '&') && (lbuf[k + 1] && lbuf[k + 1] != '&')) {
		while ((lbuf[k + 3] == '&') && (lbuf[k + 4] && lbuf[k + 4] != '&'))
			k += 3;
		lbuf[k + 3] = UPPER(lbuf[k + 3]);
	}

	if (to->desc)
		to->desc->MergeWrite(CAP(lbuf));
		// to->desc->Write(CAP(lbuf));
	if (act_check && (to != ch))
		act_mtrigger(to, lbuf, ch, victim, obj, target, arg);
	release_buffer(lbuf);
}

#define SENDOK(ch) ((IS_NPC(ch) || ((ch)->desc && (STATE(ch->desc) == CON_PLAYING))) && (AWAKE(ch) || tosleep) && \
		    !PLR_FLAGGED((ch), PLR_WRITING))


#ifdef ACT_DEBUG
void the_act(const char *str, bool hide_invisible, const Character *ch, const MUDObject *obj,
		CPtr vict_obj, int type, const char *who, UInt16 line)
#else
void act(const char *str, int hide_invisible, const Character *ch, const MUDObject *obj, CPtr vict_obj, int type)
#endif
{
	Character *		to;
	Descriptor *	i;
	VNum			room;
	bool 			tosleep = false;
	
	if (!str || !*str) {
		return;
	}
	
	if (IS_SET(type, TO_SLEEP))
		tosleep = true;
	
	//	If this bit is set, the act triggers are not checked.
	act_check = !IS_SET(type, TO_TRIG);
	
	if (IS_SET(type, TO_CHAR) && ch && SENDOK(ch))
		perform_act(str, ch, obj, vict_obj, const_cast<Character *>(ch));
	
	if (IS_SET(type, TO_VICT) && (to = static_cast<Character *>(const_cast<Ptr>(vict_obj))) && SENDOK(to) &&
		!((ch->material == Materials::Ether) && hide_invisible && !(ch->CanBeSeen(to))))
		perform_act(str, ch, obj, vict_obj, to);
	
	if (IS_SET(type, TO_ZONE | TO_GAME)) {
		START_ITER(iter, i, Descriptor *, descriptor_list) {
			if ((ch->material == Materials::Ether) && hide_invisible && !(ch->CanBeSeen(to)))
				continue;
			if (i->character && SENDOK(i->character) && (i->character != ch) &&
					!(hide_invisible && ch && !ch->CanBeSeen(i->character)) &&
					((IS_SET(type, TO_GAME)) || (world[IN_ROOM(ch)].zone == world[IN_ROOM(i->character)].zone)))
				perform_act(str, ch, obj, vict_obj, i->character);
		}
	}
	
	
	if (IS_SET(type, TO_ROOM | TO_NOTVICT)) {
		if (ch && (IN_ROOM(ch) != NOWHERE)) {
			room = IN_ROOM(ch);
		} else if (obj && (IN_ROOM(obj) != NOWHERE)) {
			room = IN_ROOM(obj);
		} else {
#ifdef ACT_DEBUG
			log("SYSERR: no valid target to act() called by %s:%d", who, line);
#else
			log("SYSERR: no valid target to act()!");
#endif
			log("SYSERR: \"%s\"", str);
			return;
		}
		
		START_ITER(iter, to, Character *, world[room].people) {
			if (SENDOK(to) && !(hide_invisible && ch && !ch->CanBeSeen(to)) &&
					(to != ch) && (IS_SET(type, TO_ROOM) || (to != vict_obj)))
				perform_act(str, ch, obj, vict_obj, to);
		}
	}
}


void sub_write_to_char(Character *ch, char *tokens[], Ptr otokens[], SInt8 type[]) {
	char *	string = get_buffer(MAX_STRING_LENGTH);
	int		i;

	for (i = 0; tokens[i + 1]; i++) {
		strcat(string, tokens[i]);

		switch (type[i]) {
		// Cases for mobiles (designated by the `)
		case 'n':
			if (!otokens[i])			strcat(string, "someone");
			else if (otokens[i] == ch)	strcat(string, "you");
			else						strcat(string, PERS(static_cast<Character *>(otokens[i]), ch));
			break;

		case 't':
			if (!otokens[i])			strcat(string, "someone");
			else if (otokens[i] == ch)	strcat(string, "you");
			else						strcat(string, PERS(static_cast<Character *>(otokens[i]), ch));
			break;

		case 'z':
			if (!otokens[i])			strcat(string, "someone's");
			else if (otokens[i] == ch)	strcat(string, "your");
			else {
				strcat(string, PERS(static_cast<Character *>(otokens[i]), ch));
				strcat(string, "'s");
			}
			break;

		case 'r':
			if (!otokens[i] || !CAN_SEE(ch, (Character *) otokens[i]))
				strcat(string, "its");
			else if (otokens[i] == ch)	strcat(string, "your");
			else						strcat(string, HSHR(static_cast<Character *>(otokens[i])));
			break;
		
		case 'a':
			if (!otokens[i] || !CAN_SEE(ch, static_cast<Character *>(otokens[i])))	
				strcat(string, "it is");
			else if (otokens[i] == ch)	strcat(string, "you are");
			else {
				strcat(string, PERS(static_cast<Character *>(otokens[i]), ch));
				strcat(string, " is");
			}
			break;
		
		case 's':
			if (!otokens[i] || !CAN_SEE(ch, (Character *) otokens[i]))
				strcat(string, "its");
			else if (otokens[i] == ch)	strcat(string, "yours");
			else						strcat(string, HSHRS(static_cast<Character *>(otokens[i])));
			break;
		
		case 'e':
			if (!otokens[i] || !CAN_SEE(ch, (Character *) otokens[i]))	
				strcat(string, "it");
			else if (otokens[i] == ch)	strcat(string, "you");
			else						strcat(string, HSSH(static_cast<Character *>(otokens[i])));
			break;
			
		case 'm':
			if (!otokens[i] || !CAN_SEE(ch, static_cast<Character *>(otokens[i])))
				strcat(string, "it");
			else if (otokens[i] == ch)	strcat(string, "you");
			else						strcat(string, HMHR(static_cast<Character *>(otokens[i])));
			break;

		// Cases for objects (designated by the #)
		case 'o':
			if (!otokens[i])			strcat(string, "something");
			else						strcat(string, OBJN(static_cast<Object *>(otokens[i]), ch));
			break;

		case 'p':
			if (!otokens[i])			strcat(string, "something");
			else						strcat(string, OBJS(static_cast<Object *>(otokens[i]), ch));
			break;
		}

	}

	ch->Sendf("%s%s\r\n", CAP(string), tokens[i]);
	release_buffer(string);
}


void sub_write(char *arg, Character *ch, bool find_invis, int targets, bool tosleep = false) {
	char	*str = get_buffer(MAX_STRING_LENGTH);
	char	*name = get_buffer(MAX_INPUT_LENGTH);
	SInt8	type[256];
	char *	tokens[256], *s, *p;
	Ptr		otokens[256];
	Object *obj;
	Character *to;
	int i, tmp;

	if (arg) {
		tokens[0] = str;

		for (i = 0, p = arg, s = str; *p;) {
			switch (*p) {
				case '`':	//	get char_data, move to next token
					p++;
					if (*p) {
						type[i] = *p;
						*s = '\0';
						p = any_one_name(++p, name);
						otokens[i] = find_invis ? get_char(name) : get_char_vis(ch, name, FIND_CHAR_ROOM);
						tokens[++i] = ++s;
					}
					break;

				case '#':	//	get obj_data, move to next token
					p++;
					if (*p) {
						type[i] = *p;
						*s = '\0';
						p = any_one_name(++p, name);
						otokens[i] =
								find_invis ? (obj = get_obj(name)) :
								((obj = get_obj_in_list_vis(ch, name, world[IN_ROOM(ch)].contents)) ?
									obj : (obj = get_object_in_equip_vis(ch, name, ch->equipment, &tmp)) ?
									obj : (obj = get_obj_in_list_vis(ch, name, ch->contents)));
						otokens[i] = obj;
						tokens[++i] = ++s;
					}
					break;

				case '\\':
					p++;
					*s++ = *p++;
					break;

				default:
					*s++ = *p++;
			}
		}

		*s = '\0';
		tokens[++i] = NULL;

		if (IS_SET(targets, TO_CHAR) && SENDOK(ch))
			sub_write_to_char(ch, tokens, otokens, type);

		if (IS_SET(targets, TO_ROOM)) {
			LListIterator<Character *>	iter(world[IN_ROOM(ch)].people);
			while ((to = iter.Next())) {
				if (to != ch && SENDOK(to) && ch->CanBeSeen(to))
					sub_write_to_char(to, tokens, otokens, type);
			}
		}
	}
	release_buffer(str);
	release_buffer(name);
}


/* Prefer the file over the descriptor. */
void setup_log(const char *filename, int fd) {
	FILE *s_fp;
	
#if defined(__MWERKS__) || defined(__GNUC__)
	s_fp = stderr;
#else
	if ((s_fp = fdopen(STDERR_FILENO, "w")) == NULL) {
		puts("SYSERR: Error opening stderr, trying stdout.");

		if ((s_fp = fdopen(STDOUT_FILENO, "w")) == NULL) {
			puts("SYSERR: Error opening stdout, trying a file.");

			/* If we don't have a file, try a default. */
			if (filename == NULL || *filename == '\0')	filename = "log/syslog";
		}
	}
#endif

	if (filename == NULL || *filename == '\0') {
		/* No filename, set us up with the descriptor we just opened. */
		logfile = s_fp;
		puts("Using file descriptor for logging.");
		return;
	}

	/* We honor the default filename first. */
	if (open_logfile(filename, s_fp))		return;

	/* Well, that failed but we want it logged to a file so try a default. */
//	if (open_logfile("log/syslog", s_fp))	return;
	
	/* Ok, one last shot at a file. */
	if (open_logfile("syslog", s_fp))		return;

	/* Erp, that didn't work either, just die. */
	puts("SYSERR: Couldn't open anything to log to, giving up.");
	exit(1);
}


int open_logfile(const char *filename, FILE *stderr_fp) {
	if (stderr_fp)	/* freopen() the descriptor. */
	logfile = freopen(filename, "w", stderr_fp);
	else
		logfile = fopen(filename, "w");

	if (logfile) {
		printf("Using log file '%s'%s.\n", filename, stderr_fp ? " with redirection" : "");
		return TRUE;
	}

	printf("SYSERR: Error opening file '%s': %s\n", filename, strerror(errno));
	return FALSE;
}


void GameData::EchoZone(char * arg, int tozone = -1) const
{
	char * buf = get_buffer(512);
	strcpy(buf, arg);
	strcat(buf, "\r\n"); 
	send_to_zone(buf, tozone);
	release_buffer(buf);
}


void		Character::Echo(char * arg) const
{
	act(arg, FALSE, this, 0, 0, TO_ROOM);
}


void		Character::EchoAt(char * arg, GameData * target) const
{
	if (target->DataType() == Datatypes::Character)
		((Character *) target)->Send(arg);
}


void		Character::EchoAround(char * arg, GameData * target) const
{
	target->Echo(arg);
}


const GameData *	MUDObject::AbsoluteInside(void)	const
{
	const GameData *place = this;
	
	while (place->Inside()) {
		place = place->Inside();
	}
	
	return place;
}


void		MUDObject::Echo(char * arg) const
{
	//	Inside()->Echo(arg);
	world[AbsoluteRoom()].Echo(arg);
}


void		MUDObject::EchoAt(char * arg, GameData * target) const
{
	if (target->DataType() == Datatypes::Character)
		((Character *) target)->Send(arg);
}


void		MUDObject::EchoAround(char * arg, GameData * target) const
{
	target->Echo(arg);
}


void		MUDObject::EchoDistance(char * arg, GameData * target) const
{
#if 0
	if (target != (GameData *) this)
		target->EchoDistance(arg, target);
	else
		Inside()->EchoDistance(arg, (GameData *) Inside());
#endif
	if (Room::Find(InRoom())) {
		world[InRoom()].EchoDistance(arg, NULL);
	}
}


void		Room::Echo(char * arg) const
{
	Send(arg);
	Send("\r\n");
}


void		Room::EchoAt(char * arg, GameData * target) const
{
	if (target->DataType() == Datatypes::Character)
		((Character *) target)->Send(arg);
}


void		Room::EchoAround(char * arg, GameData * target) const
{
	target->Echo(arg);
}


void		Room::EchoDistance(char * arg, GameData * target) const
{
	RoomDirection *	exit;
	VNum 			source, dest;
	SInt32 			door;

	source = AbsoluteRoom();

	for (door = 0; door < NUM_OF_DIRS; door++) {
		if (!(exit = EXITN(source, door)))
			continue;
		if (((dest = exit->to_room) == NOWHERE) || (dest == source) || (!Room::Find(exit->to_room)))
			continue;
		world[dest].Echo(arg);
	}
}


/* r is red, b is blue, etc.	L and l are black, you can easily change to/
	 add in K/k for black, which some people prefer since black is "KEY"
	 as in CMYK. */

						 /* Measurements *//* Not implemented this version */
/* 
	 #define M_FOOT			 "foot"
	 #define M_FEET			 "feet"
	 #define M_cm "centimeter"
	 #define M_m	"meter"
	 #define M_i	"inch"
	 #define M_is "inches"
	 #define M_M	"mile"
	 #define M_p	"pound"
	 #define M_k	"kilogram"
	 #define M_o	"ounce"
	 #define M_g	"gram"
 */

/* Cursor controls */
#define C_UP		"\E[A"
#define C_DOWN	"\E[B"
#define C_RIGHT "\E[C"
#define C_LEFT	"\E[D"
#define C_HOME	"\E[H"
#define C_CLR	 C_HOME"\E[J"

const char * ANSIColors[] =
{
	ANSI_OFF, 
	ANSI_NONE, 

	ANSI_BLACK, ANSI_RED, ANSI_GREEN, ANSI_YELLOW, 
	ANSI_BLUE, ANSI_MAGENTA, ANSI_CYAN, ANSI_WHITE, 

	ANSI_D_BLACK, ANSI_D_RED, ANSI_D_GREEN, ANSI_D_YELLOW, 
	ANSI_D_BLUE, ANSI_D_MAGENTA, ANSI_D_CYAN, ANSI_D_WHITE, 

	ANSI_E_BLACK, ANSI_E_RED, ANSI_E_GREEN, ANSI_E_YELLOW, 
	ANSI_E_BLUE, ANSI_E_MAGENTA, ANSI_E_CYAN, ANSI_E_WHITE, 

	ANSI_FG_OFF,

	ANSI_BK_BLACK, ANSI_BK_RED, ANSI_BK_GREEN,ANSI_BK_YELLOW, 
	ANSI_BK_BLUE, ANSI_BK_MAGENTA,ANSI_BK_CYAN, ANSI_BK_WHITE, 

	C_UP, C_DOWN, C_RIGHT, C_LEFT, 
	C_HOME, C_CLR, 

	STYLE_UNDERLINE, STYLE_FLASH, STYLE_REVERSE
};


#define DIM_DIFFERENCE	(8)
#define BOLD_DIFFERENCE	(16)


const char * HTMLColors[] =
{
	HTML_OFF, 
	HTML_NONE, 

	HTML_BLACK, HTML_RED, HTML_GREEN, HTML_YELLOW, 
	HTML_BLUE, HTML_MAGENTA, HTML_CYAN, HTML_WHITE, 

	HTML_BLACK, HTML_RED, HTML_GREEN, HTML_YELLOW, 
	HTML_BLUE, HTML_MAGENTA, HTML_CYAN, HTML_WHITE, 

	HTML_E_BLACK, HTML_E_RED, HTML_E_GREEN, HTML_E_YELLOW, 
	HTML_E_BLUE, HTML_E_MAGENTA, HTML_E_CYAN, HTML_E_WHITE, 

	HTML_FG_OFF,

	HTML_BK_BLACK, HTML_BK_RED, HTML_BK_GREEN,HTML_BK_YELLOW, 
	HTML_BK_BLUE, HTML_BK_MAGENTA,HTML_BK_CYAN, HTML_BK_WHITE, 

	C_UP, C_DOWN, C_RIGHT, C_LEFT, 
	C_HOME, C_CLR, 

	STYLE_UNDERLINE, STYLE_FLASH, STYLE_REVERSE
};


#define LAST_COLOR	STYLE_REVERSE
#define BUFSIZE		LARGE_BUFSIZE-1
#define START_CHAR '&'		/* a forward slash followed by c or C or b
					 and a number */

/******* You can change START_CHAR to whatever you wish, like '^' *******/


static char out_buf[LARGE_BUFSIZE], insert_text[LARGE_BUFSIZE];

void proc_color(char *inbuf, int color_lvl, Color default_color = COL_OFF, bool is_html = false)
{
	int has_color = (is_html ? 1 : color_lvl);

	/*				has_cursor = ((color_lvl==1) || (color_lvl==3)), */
	Color current_color = COL_UNSET, current_bg = COL_UNSET, 
				stored_bg = COL_UNSET, stored_color = COL_UNSET, 
				color = COL_PREUNSET;

	register int inpos = 0, outpos = 0;
	int remaining, len;

	extern int MAX(int a, int b);

	const char **colorlist = ANSIColors;

	if (is_html)
		colorlist = HTMLColors;

	len = strlen(inbuf);

	if (*inbuf == '\0')
		return;			/* if first char is null */

/* If color level is 1 (sparse), then character will get cursor controls
	 only.	If color level is 2, character will get color codes only.	If
	 color is complete (3), character will get both.	 Color level 0 removes
	 all color codes, of course. =) */


	*out_buf = '\0';
	*insert_text = '\0';

	if (has_color && (default_color > COL_OFF)) {
		strcpy(out_buf, colorlist[default_color]);
		current_color = default_color;
		outpos += strlen(out_buf);
	}

	while (inbuf[inpos] != '\0') {
		remaining = len - inpos;

		if (remaining > 2) {
			if (inbuf[inpos] == START_CHAR) {
	*insert_text = '\0';
	switch (inbuf[inpos + 1]) {
#ifdef GOT_RID_OF_IT
	case START_CHAR:	/* just a slash */
		*insert_text = START_CHAR;
		insert_text[1] = '\0';
		inpos += 1;
		break;
#endif
	case '^':		/* just a slash */
		*insert_text = START_CHAR;
		insert_text[1] = '\0';
		inpos += 2;
		break;
	case 'c':		/* foreground color */
		switch (inbuf[inpos + 2]) {

		case 'l': color = COL_D_BLACK; break;
		case 'L': color = COL_E_BLACK; break;
		case 'r': color = COL_D_RED; break;
		case 'R': color = COL_E_RED; break;
		case 'g': color = COL_D_GREEN; break;
		case 'G': color = COL_E_GREEN; break;
		case 'y': color = COL_D_YELLOW; break;
		case 'Y': color = COL_E_YELLOW; break;
		case 'b': color = COL_D_BLUE; break;
		case 'B': color = COL_E_BLUE; break;
		case 'm':
		case 'p': color = COL_D_MAGENTA; break;
		case 'M':
		case 'P': color = COL_E_MAGENTA; break;
		case 'c': color = COL_D_CYAN; break;
		case 'C': color = COL_E_CYAN; break;
		case 'w': color = COL_D_WHITE; break;
		case 'W': color = COL_E_WHITE; break;
		case 'O':
		case 'o': color = COL_D_BLACK; break;
		case 'U':
		case 'u': color = COL_STYLE_UNDERLINE; break;
		case 'F':
		case 'f': color = COL_STYLE_FLASH; break;
		case 'V':
		case 'v': color = COL_STYLE_REVERSE; break;
		case '+': color = COL_STORE; break;
		case '-': color = COL_RESTORE; break;
		case '0': color = default_color; break;
		default:
			color = COL_NONE;		/* no change */
		}

		switch (color) {
		case COL_STORE:
			stored_color = (Color) MAX(0, current_color);
			break;
		case COL_RESTORE:
			if (stored_color != current_color)
				if (has_color) {
					if (IS_BOLD(stored_color) && (IS_BOLD(current_color)))
						strcpy(insert_text, colorlist[MAX(0, stored_color - BOLD_DIFFERENCE)]);
					else if (IS_DIM(stored_color) && (IS_DIM(current_color) || current_color == COL_OFF))
						strcpy(insert_text, colorlist[MAX(0, stored_color - DIM_DIFFERENCE)]);
					else
						strcpy(insert_text, colorlist[MAX(0, stored_color)]);
				}
			current_color = stored_color;
			stored_color = default_color;
			break;
		default:
			if (color != current_color) {
				if (has_color) {
					if (IS_BOLD(color) && (IS_BOLD(current_color)))
						strcpy(insert_text, colorlist[color - BOLD_DIFFERENCE]);
					else if (IS_DIM(color) && (IS_DIM(current_color) || current_color == COL_OFF))
						strcpy(insert_text, colorlist[color - DIM_DIFFERENCE]);
					else
						strcpy(insert_text, colorlist[color]);
				}
			}
			current_color = color;
			break;
		}
		inpos += 3;
		break;

	case 'b':		/* background color */
		switch (inbuf[inpos + 2]) {
		case 'l': color = COL_BK_BLACK; break;
		case 'r': color = COL_BK_RED; break;
		case 'g': color = COL_BK_GREEN; break;
		case 'y': color = COL_BK_YELLOW; break;
		case 'b': color = COL_BK_BLUE; break;
		case 'm':
		case 'p': color = COL_BK_MAGENTA; break;
		case 'c': color = COL_BK_CYAN; break;
		case 'w': color = COL_BK_WHITE; break;
		case '0': color = COL_OFF; break;
		case '+': color = COL_STORE; break;
		case '-': color = COL_RESTORE; break;
		default:
			color = COL_NONE;		/* no change */
		}

		switch (color) {
		case COL_STORE:
			stored_bg = (Color) MAX(0, current_bg);
			break;
		case COL_RESTORE:
			if (stored_bg != current_bg)
				if (has_color)
		strcpy(insert_text, colorlist[MAX(0, stored_bg)]);
			current_bg = stored_bg;
			break;
		default:
			if (color != current_bg)
				if (has_color)
		strcpy(insert_text, colorlist[color]);
			current_bg = color;
			break;
		}
		inpos += 3;
		break;

#ifdef GOT_RID_OF_IT
	case 'C':		/* cursor control */
		switch (inbuf[inpos + 2]) {
		case 'u': color = 29; break;
		case 'd': color = 30; break;
		case 'r': color = 31; break;
		case 'l': color = 32; break;
		case 'h': color = 33; break;
		case 'c': color = 34; break;
		default:
			color = 1;		/* no change */
		}

		if (has_cursor)
			strcpy(insert_text, colorlist[color]);
		inpos += 3;
		break;

#endif
		/*							case 'm':		// measurement	 *//* not implemented */


	default:
		*insert_text = START_CHAR;
		// insert_text[1] = inbuf[inpos + 1];
		insert_text[1] = '\0';
		inpos += 1;
		break;
	}			/* switch */

	if (color_lvl == 0)
		out_buf[outpos] = '\0';

	if ((strlen(out_buf) + strlen(insert_text)) < BUFSIZE)
		/* don't overfill buffer */
	{
		out_buf[outpos] = '\0';	/* so strcat is not confused by whatever out_buf WAS */
		strcat(out_buf, insert_text);
		outpos = strlen(out_buf);
	}
			}
			/* if char is '/' (START_CHAR) */
			else {
				if (outpos < BUFSIZE) {
					out_buf[outpos] = inbuf[inpos];
					++inpos;
					++outpos;
				}
			}

		}
		/* if remaining > 2 */
		else {
			if (outpos < BUFSIZE) {
				out_buf[outpos] = inbuf[inpos];
				++inpos;
				++outpos;
			}
		}


	}				/* while */


	out_buf[outpos] = '\0';

	/* printf("outbuf: %s\n",out_buf); *//* for debugging */

	strcpy(inbuf, out_buf);

}
