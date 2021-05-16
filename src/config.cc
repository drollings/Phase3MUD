/* ************************************************************************
*   File: config.c                                      Part of CircleMUD *
*  Usage: Configuration of various aspects of CircleMUD operation         *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define __CONFIG_C__



#include "types.h"
#include "structs.h"

extern const VNum NOWHERE;


//#define TRUE	1
#define YES		1
#define FALSE	0
#define NO		0

/*
 * Below are several constants which you can change to alter certain aspects
 * of the way CircleMUD acts.  Since this is a .c file, all you have to do
 * to change one of the constants (assuming you keep your object files around)
 * is change the constant in this file and type 'make'.  Make will recompile
 * this file and relink; you don't have to wait for the whole thing to
 * recompile as you do if you change a header file.
 *
 * I realize that it would be slightly more efficient to have lots of
 * #defines strewn about, so that, for example, the autowiz code isn't
 * compiled at all if you don't want to use autowiz.  However, the actual
 * code for the various options is quite small, as is the computational time
 * in checking the option you've selected at run-time, so I've decided the
 * convenience of having all your options in this one file outweighs the
 * efficency of doing it the other way.
 *
 */

/****************************************************************************/
/****************************************************************************/


/* GAME PLAY OPTIONS */

/*
 * pk_allowed sets the tone of the entire game.  If pk_allowed is set to
 * NO, then players will not be allowed to kill, summon, charm, or sleep
 * other players, as well as a variety of other "asshole player" protections.
 * However, if you decide you want to have an all-out knock-down drag-out
 * PK Mud, just set pk_allowed to YES - and anything goes.
 */
const char *LOGFILE = "";

int system_b = YES;

int max_exp_gain = 1000;
int max_exp_loss = 1000;

/* is playerthieving allowed? */
int pt_allowed = YES;

/* minimum level a player must be to shout/holler/gossip/music */
int level_can_shout = 1;

/* number of movement points it costs to holler */
int holler_move_cost = 0;

/* number of tics (usually 75 seconds) before PC/NPC corpses decompose */
int max_npc_corpse_time = 6;
int max_pc_corpse_time = 0;

/* should items in death traps automatically be junked? */
int dts_are_dumps = YES;

/* "okay" etc. */
const char *OK = "Okay.\r\n";
const char *NOPERSON = "No-one by that name here.\r\n";
const char *NOEFFECT = "Nothing seems to happen.\r\n";

/****************************************************************************/
/****************************************************************************/


/* RENT/CRASHSAVE OPTIONS */

/*
 * Should the game automatically save people?  (i.e., save player data
 * every 4 kills (on average), and Crash-save as defined below.
 */
int auto_save = YES;

/*
 * if auto_save (above) is yes, how often (in minutes) should the MUD
 * Crash-save people's objects?   Also, this number indicates how often
 * the MUD will Crash-save players' houses.
 */
UInt32 autosave_time = 5;

/* Lifetime of crashfiles and forced-rent (idlesave) files in days */
int crash_file_timeout = 10;

/* Lifetime of normal rent files in days */
int rent_file_timeout = 30;


int free_rent = YES;

/****************************************************************************/
/****************************************************************************/


/* ROOM NUMBERS */

/* virtual number of room that mortals should enter at */
VNum mortal_start_room = 3039;

/* virtual number of room that immorts should enter at by default */
VNum immort_start_room = 1204;

/* virtual number of room that frozen players should enter at */
VNum frozen_start_room = 1202;

/* virtual number of room that jailed players should enter at */
VNum jailed_start_room = 50;

/*
 * virtual numbers of donation rooms.  note: you must change code in
 * do_drop of act.item.c if you change the number of non-NOWHERE
 * donation rooms.
 */
VNum donation_room_1 = 3086;
VNum donation_room_2 = NOWHERE;	/* unused - room for expansion */
VNum donation_room_3 = NOWHERE;	/* unused - room for expansion */


/****************************************************************************/
/****************************************************************************/


/* GAME OPERATION OPTIONS */

/*
 * This is the default port the game should run on if no port is given on
 * the command-line.  NOTE WELL: If you're using the 'autorun' script, the
 * port number there will override this setting.  Change the PORT= line in
 * instead of (or in addition to) changing this.
 */
int DFLT_PORT = 7777;

/* maximum number of players allowed before game starts to turn people away */
int MAX_PLAYERS = 300;

/* maximum size of bug, typo and idea files in bytes (to prevent bombing) */
int max_filesize = 50000;

/* maximum number of password attempts before disconnection */
int max_bad_pws = 3;

/*
 * Some nameservers are very slow and cause the game to lag terribly every 
 * time someone logs in.  The lag is caused by the gethostbyaddr() function
 * which is responsible for resolving numeric IP addresses to alphabetic names.
 * Sometimes, nameservers can be so slow that the incredible lag caused by
 * gethostbyaddr() isn't worth the luxury of having names instead of numbers
 * for players' sitenames.
 *
 * If your nameserver is fast, set the variable below to NO.  If your
 * nameserver is slow, of it you would simply prefer to have numbers
 * instead of names for some other reason, set the variable to YES.
 *
 * You can experiment with the setting of nameserver_is_slow on-line using
 * the SLOWNS command from within the MUD.
 */

int nameserver_is_slow = NO;


const char *ANSI = "\r\nDoes your terminal support ANSI color? [Y/n] ";

const char *MENU =
"\r\n"
" STRUGGLE FOR NYRILIS\r\n\n"
" 0)   Disconnect from the game\r\n"
" 1)   Join the Struggle\r\n"
" 2)   Enter description\r\n"
" 3)   Read the background story\r\n"
" 4)   Change password\r\n"
" 5)   Delete this character\r\n\n\n"
" Make your choice: ";

#if 0
"\r\n"
"\x1B[31m (TLO)  Welcome to Tower of the Lost Order   (TLO)  \x1B[0m\r\n"
"\x1B[33m _________________________________________________  \x1B[0m\r\n"
"\x1B[35m 0) \x1B[33m| \x1B[32mExit from the Order.      :-< \x1B[37m   (Get Lost!)\x1B[0m\r\n"
"\x1B[35m 1) \x1B[33m| \x1B[32mEnter the Order.         >;-) \x1B[36m    (Be Brave)\x1B[0m\r\n"
"\x1B[35m 2) \x1B[33m| \x1B[37mEnter description             \x1B[35m(Say Hi 2 Mom)\x1B[0m\r\n"
"\x1B[35m 3) \x1B[33m| \x1B[37mRead the background story.    \x1B[34m   (About LOD)\x1B[0m\r\n"
"\x1B[35m 4) \x1B[33m| \x1B[31mChange password.              \x1B[33m(LOD Security)\x1B[0m\r\n"
"\x1B[35m 5) \x1B[33m| \x1B[31mDelete this character.      \x1B[32m(Commit Suicide)\x1B[0m\r\n"
"\x1B[35m 6) \x1B[33m| \x1B[36mWho's Online Lost Order.       \x1B[31m(Nosy Person)\x1B[0m\r\n"
"\x1B[33m ___|_____________________________________________  \x1B[0m\r\n"
"\r\n"
"            Make thy choice: ";
#endif

const char *PLAINMENU =
"\r\n"
" STRUGGLE FOR NYRILIS\r\n\n"
" 0)   Disconnect from the game\r\n"
" 1)   Join the Struggle\r\n"
" 2)   Enter description\r\n"
" 3)   Read the background story\r\n"
" 4)   Change password\r\n"
" 5)   Delete this character\r\n\n\n"
" Make your choice: ";

const char *GREETINGS =

"\r\n\r\n"
"\E[1;37m                                     ^ \r\n"
"\E[1;37m                                   / | \\ \r\n"
"\E[0;36m                 S T R U \E[0;36mG \E[0;36mG L E   \E[1;37mF O R    \E[0;36mN Y R \E[0;36mI L I S\r\n"
"\E[1;37m                                 /   |   \\ \r\n"
"\E[1;37m                                /    |    \\ \r\n"
"\E[1;30m                               /     |     \\ \r\n"
"\E[1;30m                              /      |      \\ \E[0;0m\r\n"
"\r\n"
"             An immersive role-playing multiplayer adventure.\r\n"
"\r\n"
"Based on LexiMUD, by Chris Jacobson, and on CircleMUD, by Jeremy Elson and\r\n"
"George Greer.  Other contributions made by too many coders to list!\r\n"
"\r\n"
"A derivative of DikuMUD (GAMMA 0.0), created by  Hans Henrik Staerfeldt, Katja\r\n"
"Nyboe, Tom Madsen,  Michael Seifert, and Sebastian Hammer.\r\n";
#if 0
"\r\n\r\n"
"\x1B[33m                __                         _____                       \n\r"
"               / /    ____  ___  _____    /___ / ___   ___   ____  ___ \n\r"
"              / /    /__ / /__/ /_  _/   //  // /__ ) /__ ) / /   /__ )\n\r"
"             / /__  //_// __))   / /    //__// //_// //_// / /-  //_// \n\r"
"            /____/ /___/ /__/   /_/    /____/ // )) /___/ /_/__ // ))  \n\r\r\n"
"                                                          Mud (v 1.0)  \r\n"
"           By: the Tower of the Lost Order Team\x1B[0m\r\n"
"\r\n\x1B[35m"
"                            Based on CircleMUD 3.0,\r\n"
"                            Created by Jeremy Elson\r\n"
"\r\n\x1B[36m"
"                      A derivative of DikuMUD (GAMMA 0.0)\r\n"
"                                  Created by\r\n"
"                     Hans Henrik Staerfeldt, Katja Nyboe,\r\n"
"               Tom Madsen, Michael Seifert, and Sebastian Hammer\x1B[0m\r\n"
"\r\n\r\n";
#endif


const char *PLAINGREETINGS =

"\r\n"
"               S T R U G G L E     F O R     N Y R I L I S\r\n\r\n\r\n"
"             An immersive role-playing multiplayer adventure.\r\n"
"	   \r\n"
"Based on LexiMUD, by Chris Jacobson, and on CircleMUD, by Jeremy Elson and\r\n"
"George Greer.  Other contributions made by too many coders to list!\r\n"
"\r\n"
"A derivative of DikuMUD (GAMMA 0.0), created by Hans Henrik Staerfeldt, Katja\r\n"
"Nyboe, Tom Madsen,  Michael Seifert, and Sebastian Hammer.\r\n";

#if 0
"\r\n\r\n"
"                __                         _____                       \n\r"
"               / /    ____  ___  _____    /___ / ___   ___   ____  ___ \n\r"
"              / /    /__ / /__/ /_  _/   //  // /__ ) /__ ) / /   /__ )\n\r"
"             / /__  //_// __))   / /    //__// //_// //_// / /-  //_// \n\r"
"            /____/ /___/ /__/   /_/    /____/ // )) /___/ /_/__ // ))  \n\r\r\n"
"                                                          Mud (v 1.0)  \r\n"
"           By: the Tower of the Lost Order Team\r\n"
"\r\n"
"                            Based on CircleMUD 3.0,\r\n"
"                            Created by Jeremy Elson\r\n"
"\r\n"
"                      A derivative of DikuMUD (GAMMA 0.0)\r\n"
"                                  Created by\r\n"
"                     Hans Henrik Staerfeldt, Katja Nyboe,\r\n"
"               Tom Madsen, Michael Seifert, and Sebastian Hammer\r\n"
"\r\n\r\n";
#endif


const char *WELC_MESSG =
"\r\n"
"Welcome to Struggle for Nyrilis.  Good luck on your travels!"
"\r\n\r\n";

const char *START_MESSG =
"Welcome!  You now have the opportunity to enter the Struggle for Nyrilis\r\n"
"with your new character.  You can now experience roleplay with people from\r\n"
"around the world as your character gains skills and finds new treasures and\r\n"
"equipment.\r\n\r\n"
"Check http://sfn.zenonline.com if you haven't already for important\r\n"
"background information and playing tips, and make sure to use the \"help\" and\r\n"
"\"policies\" commands to learn more about this MUD.\r\n\r\n"
"Most of all, enjoy your stay!\r\n";

/****************************************************************************/
/****************************************************************************/


/* AUTOWIZ OPTIONS */

/*
 * Should the game automatically create a new wizlist/immlist every time
 * someone immorts, or is promoted to a higher (or lower) god level?
 * NOTE: this only works under UNIX systems.
 */
int use_autowiz = NO;
// int no_rent_check = 0;          /* skip rent check on boot?      */

const char *DFLT_IP = NULL; /* bind to all interfaces */
/* const char *DFLT_IP = "192.168.1.1";  -- bind only to one interface */

/* default directory to use as data directory */
const char *DFLT_DIR = "lib";

int newb_eq = YES;

int sleep_allowed = YES;
int roomaffect_allowed = YES;
int summon_allowed = YES;
int astral_start_room = 13001;
int r_mortal_start_room = 3039;
int charm_allowed = YES;
int pk_allowed = YES;
