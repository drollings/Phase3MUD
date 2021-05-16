/* ************************************************************************
*  File: cmdtab.c                                       Part of CircleMUD *
*                                                                         *
*  Usage: The command table                                               *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author: sfn $
*  $Date: 2000/11/18 08:48:54 $
*  $Revision: 1.22 $
************************************************************************ */

#include "structs.h"
#include "olc.h"
#include "interpreter.h"
#include "combat.h"

/* prototypes for all do_x functions. */
ACMD(do_afk);
ACMD(do_action);
ACMD(do_advance);
ACMD(do_alias);
ACMD(do_allow);
ACMD(do_assist);
ACMD(do_at);
ACMD(do_backstab);
ACMD(do_ban);
//ACMD(do_bash);
//ACMD(do_circle);
ACMD(do_commands);
ACMD(do_combatprofile);
ACMD(do_consider);
ACMD(do_date);
ACMD(do_dc);
ACMD(do_diagnose);
ACMD(do_display);
ACMD(do_drink);
ACMD(do_drop);
ACMD(do_drive);
ACMD(do_eat);
ACMD(do_echo);
ACMD(do_enter);
ACMD(do_equipment);
ACMD(do_examine);
ACMD(do_exits);
ACMD(do_flee);
ACMD(do_follow);
ACMD(do_force);
ACMD(do_gecho);
ACMD(do_gen_comm);
ACMD(do_gen_door);
ACMD(do_gen_ps);
ACMD(do_gen_tog);
ACMD(do_gen_write);
ACMD(do_get);
ACMD(do_give);
ACMD(do_gold);
ACMD(do_goto);
ACMD(do_grab);
ACMD(do_group);
ACMD(do_gsay);
ACMD(do_hcontrol);
ACMD(do_help);
ACMD(do_hide);
ACMD(do_hit);
ACMD(do_house);
ACMD(do_insult);
ACMD(do_inventory);
ACMD(do_invis);
//ACMD(do_kick_bite);
ACMD(do_slay);
ACMD(do_last);
ACMD(do_leave);
ACMD(do_levels);
ACMD(do_load);
ACMD(do_look);
ACMD(do_move);
ACMD(do_not_here);
ACMD(do_olc);
ACMD(do_order);
ACMD(do_page);
// ACMD(do_pkill);
ACMD(do_poofset);
ACMD(do_pour);
ACMD(do_practice);
ACMD(do_purge);
ACMD(do_put);
ACMD(do_qcomm);
ACMD(do_quit);
ACMD(do_reboot);
ACMD(do_reload);
ACMD(do_remove);
ACMD(do_reply);
ACMD(do_report);
ACMD(do_rescue);
ACMD(do_respond);
ACMD(do_rest);
ACMD(do_restore);
ACMD(do_return);
ACMD(do_save);
ACMD(do_say);
ACMD(do_sayto);
ACMD(do_score);
ACMD(do_send);
ACMD(do_set);
ACMD(do_show);
ACMD(do_shoot);
ACMD(do_shutdown);
ACMD(do_sit);
ACMD(do_skillset);
ACMD(do_sleep);
ACMD(do_sneak);
ACMD(do_snoop);
//ACMD(do_spec_comm);
ACMD(do_stand);
ACMD(do_stat);
ACMD(do_switch);
ACMD(do_syslog);
ACMD(do_throw);
ACMD(do_pull);
ACMD(do_teleport);
ACMD(do_tell);
ACMD(do_time);
ACMD(do_title);
ACMD(do_toggle);
ACMD(do_track);
ACMD(do_trans);
ACMD(do_unban);
ACMD(do_ungroup);
//ACMD(do_use);
ACMD(do_users);
ACMD(do_visible);
ACMD(do_vnum);
ACMD(do_vstat);
ACMD(do_wake);
ACMD(do_watch);
ACMD(do_wear);
ACMD(do_weather);
ACMD(do_where);
ACMD(do_whisper);
ACMD(do_who);
ACMD(do_wield);
ACMD(do_wimpy);
ACMD(do_wizlock);
ACMD(do_wiznet);
ACMD(do_wizutil);
ACMD(do_write);
ACMD(do_zreset);

ACMD(do_call);
ACMD(do_forget);

ACMD(do_zallow);
ACMD(do_zdeny);
ACMD(do_ostat);
ACMD(do_mstat);
ACMD(do_rstat);
ACMD(do_zstat);
ACMD(do_zlist);
ACMD(do_liblist);

ACMD(do_tedit);

ACMD(do_push_drag);

ACMD(do_depiss);
ACMD(do_repiss);

ACMD(do_hunt);

ACMD(do_mount);
ACMD(do_dismount);

ACMD(do_vwear);

// ACMD(do_reward);
ACMD(do_delete);

ACMD(do_copyover);

ACMD(do_buffer);
ACMD(do_overflow);

ACMD(do_attach);
ACMD(do_detach);

// Clan ACMDs
ACMD(do_clan);
// ACMD(do_apply);
// ACMD(do_resign);
// ACMD(do_enlist);
// ACMD(do_boot);
// ACMD(do_clanlist);
// ACMD(do_clanstat);
// ACMD(do_members);
// ACMD(do_forceenlist);
// ACMD(do_clanwho);
// ACMD(do_clanpromote);
// ACMD(do_clandemote);
// ACMD(do_deposit);

ACMD(do_prompt);
ACMD(do_nonewbie);

ACMD(do_move_element);

ACMD(do_string);

ACMD(do_massroomsave);

ACMD(do_path);

ACMD(do_calm);
ACMD(do_wizcall);
ACMD(do_wizassist);

ACMD(do_aim);
ACMD(do_shoot);
ACMD(do_throw);
ACMD(do_pull);
ACMD(do_draw);
ACMD(do_sheath);
ACMD(do_light);
ACMD(do_skillinfo);
ACMD(do_bandage);
ACMD(do_berserk);
ACMD(do_bladesong);
ACMD(do_cast);
ACMD(do_pray);
ACMD(do_camoflauge);
// ACMD(do_clan);

ACMD(do_concentrate);
ACMD(do_magic);
ACMD(do_meditate);

// ACMD(do_ctell);
ACMD(do_disarm);
ACMD(do_exittrace);
ACMD(do_melee);
ACMD(do_fly);
ACMD(do_combat);
// ACMD(do_home);
// ACMD(do_hostnames);
ACMD(do_invuln);
ACMD(do_land);
// ACMD(do_name);
ACMD(do_poof);
ACMD(do_preference);
ACMD(do_use);
ACMD(do_highfive);
// ACMD(do_rune);
ACMD(do_search);
ACMD(do_melee);
ACMD(do_skillroll);
ACMD(do_split);
ACMD(do_spells);
ACMD(do_stalk);
ACMD(do_steal);
ACMD(do_surrender);
// ACMD(do_syswho);
// ACMD(do_sysps);
ACMD(do_telepathy);
ACMD(do_use);
// ACMD(do_whois);
ACMD(do_scan);
ACMD(do_pagelength);
ACMD(do_rdig);

ACMD(do_coredump);

struct command_info *complete_cmd_info;

/* This is the Master Command List(tm).

 * You can put new commands in, take commands out, change the order
 * they appear in, etc.  You can adjust the "priority" of commands
 * simply by changing the order they appear in the command list.
 * For example, if you want "as" to mean "assist" instead of "ask",
 * just put "assist" above "ask" in the Master Command List(tm).
 *
 * In general, utility commands such as "at" should have high priority;
 * infrequently used and dangerously destructive commands should have low
 * priority.
 */

struct command_info cmd_info[] = {
  { "RESERVED", "RESERVED", 0, 0, 0, 0 },	/* this must be first -- for specprocs */

  /* directions must come before other commands but after RESERVED */
  { "north"    , "n"    , P_ST | P_RI , do_move     , 0, 0, SCMD_NORTH },
  { "east"     , "e"     , P_ST | P_RI , do_move     , 0, 0, SCMD_EAST },
  { "south"    , "s"    , P_ST | P_RI , do_move     , 0, 0, SCMD_SOUTH },
  { "west"     , "w"     , P_ST | P_RI , do_move     , 0, 0, SCMD_WEST },
  { "up"       , "u"       , P_ST | P_RI , do_move     , 0, 0, SCMD_UP },
  { "down"     , "d"     , P_ST | P_RI , do_move     , 0, 0, SCMD_DOWN },

  /* now, the main list */
  { "at"       , "a"	       , P_ANY       , do_at       , STAFF_CMD, Staff::Game, 0 },
  { "advance"  , "a"	  , P_ANY       , do_advance  , STAFF_CMD, Staff::Extra, 0 },
  { "aim"      , "a"	      , P_ST|P_NOTFI, do_aim      , 0, 0, 0 },
  { "alias"    , "a"	    , P_ANY       , do_alias    , 0, 0, 0 },
  { "allow"    , "a"	   , P_ANY       , do_allow    , STAFF_CMD, Staff::Allow, SCMD_ALLOW },
  { "assist"   , "a"	   , P_FI|P_ST|P_RI, do_assist   , 1, 0, 0 },
//  { "ask"      , "a"	      , P_AWAKE | P_AUD | P_MOB, do_spec_comm, 0, 0, SCMD_ASK },
//  { "auction"  , "a"	  , P_SL|P_AWAKE | P_AUD, do_gen_comm , 0, 0, SCMD_AUCTION },

  { "ban"      , "b"	      , P_ANY       , do_ban      , STAFF_CMD, Staff::Admin, 0 },
  { "balance"  , "b"	  , P_ST        , do_not_here , 1, 0, 0 },
  { "kick"     , "k"	  , P_FI|P_ST|P_RI, do_combat   , 1, 0, Combat::Kick },
  { "bash"     , "b"	  , P_FI | P_ST , do_combat    , 1, 0, Combat::Bash },
  { "backstab"     , "b"	  , P_NOTFI | P_ST , do_backstab    , 1, 0, 0 },

  { "bandage"  , "b"	  , P_SI | P_ST , do_bandage  , 0, 0, 0 },
  { "berserk"  , "b"	  , P_ST        , do_berserk  , 0, 0, 0 },
  { "bladesong" , "b"	 , P_FI | P_ST, do_bladesong, 0, 0, Combat::Bladesong },
//  { "builder"  , "b"	  , P_ANY       , do_gen_ps   , STAFF_CMD, Staff::General, SCMD_BUILDER },
  { "buy"      , "b"	      , P_ST        , do_not_here , 0, 0, 0 },
  { "bug"      , "b"	      , P_ANY       , do_gen_write, 0, 0, SCMD_BUG },

  { "cast"     , "c"	     , P_FI|P_ST|P_RI|P_AUD, do_cast     , 1, 0, 0 },
  { "call"     , "c"	     , P_ANY       , do_call     , 1, 0, 0 },
  { "calm"     , "c"	     , P_ANY       , do_calm     , STAFF_CMD, Staff::General, 0},
  { "camoflauge" , "c"	 , P_ST        , do_camoflauge, 1, 0, 0 },
  { "chat"   , "c"	   , P_ANY       , do_gen_comm , 0, 0, SCMD_GOSSIP },
  { "check"    , "c"	    , P_ST        , do_not_here , 1, 0, 0 },
  { "circle"   , "c"	   , P_FI | P_ST , do_circle   , 1, 0, Combat::Circle },
  { "clan"     , "c"	     , P_ANY       , do_clan     , 1, 0, 0 },
  { "clear"    , "c"	    , P_ANY       , do_gen_ps   , 0, 0, SCMD_CLEAR },
  { "close"    , "c"	    , P_ST|P_FI   , do_gen_door , 0, 0, SCMD_CLOSE },
  { "cls"      , "c"	      , P_ANY       , do_gen_ps   , 0, 0, SCMD_CLEAR },
//  { "color"    , "c"	    , P_ANY	     , do_color    , 0, 0, 0 },
  { "consider" , "c"	 , P_AWAKE     , do_consider , 0, 0, 0 },
  { "commands" , "c"	 , P_ANY       , do_commands , 0, 0, SCMD_COMMANDS },
  { "compact"  , "c"	  , P_ANY       , do_gen_tog  , 0, 0, SCMD_COMPACT },
  { "concentrate", "c"	, P_ANY     , do_concentrate, 0, 0, 0 },
  { "credits"  , "c"	  , P_ANY       , do_gen_ps   , 0, 0, SCMD_CREDITS },
//   { "ctell"    , "c"	    , P_AWAKE     , do_ctell    , 0, 0, 0 },

  { "date"     , "d"	     , P_ANY       , do_date     , 0, 0, SCMD_DATE },
  { "dc"       , "d"	       , P_ANY       , do_dc       , STAFF_CMD, Staff::Admin, 0 },
  { "deny"     , "d"	   , P_ANY       , do_allow    , STAFF_CMD, Staff::Admin, SCMD_DENY },
  { "deposit"  , "d"	  , P_ST        , do_not_here , 1, 0, 0 },
  { "diagnose" , "d"	 , P_AWAKE     , do_diagnose , 0, 0, 0 },
  { "disarm"   , "d"	   , P_FI|P_ST|P_RI, do_disarm   , 0, 0, Combat::Disarm },
  { "dismount" , "d"	 , P_AWAKE | P_MOB     , do_dismount , 0, 0, 0 },
  { "display"  , "d"	  , P_ANY       , do_display  , 0, 0, 0 },
  { "donate"   , "d"	   , P_AWAKE | P_MOB     , do_drop     , 0, 0, SCMD_DONATE },
  { "douse"    , "d"	    , P_AWAKE | P_MOB  , do_light    , 0, 0, SCMD_DOUSE_OBJ },
  { "drag"     , "d"	     , P_FI|P_ST     , do_push_drag     , 0, 0, SCMD_DRAG },
  { "draw"     , "d"	     , P_AWAKE | P_MOB , do_draw     , 0, 0, 0 },
  { "drink"    , "d"	    , P_NOTFI     , do_drink    , 0, 0, SCMD_DRINK },
  { "drive"    , "d"	     , P_NOTFI     , do_drive    , 0, 0, 0 },
  { "drop"     , "d"	     , P_AWAKE | P_MOB , do_drop     , 0, 0, SCMD_DROP },

  { "eat"      , "e"	      , P_NOTFI     , do_eat      , 0, 0, SCMD_EAT },
  { "echo"     , "e"	     , P_ANY       , do_echo     , STAFF_CMD, Staff::Game, SCMD_ECHO },
  { "emote"    , "e"	    , P_AWAKE | P_AUD | P_MOB, do_echo     , 1, 0, SCMD_EMOTE },
  { ":"        , ":"		, P_AWAKE | P_AUD, do_echo      , 1, 0, SCMD_EMOTE },
  { "enter"    , "e"	    , P_ST        , do_enter    , 0, 0, 0 },
  { "equipment", "e"	, P_ANY       , do_equipment, 0, 0, 0 },
  { "exits"    , "e"	    , P_AWAKE     , do_exits    , 0, 0, 0 },
  { "exittrace", "e"	, P_ANY       , do_exittrace, STAFF_CMD, (Staff::OLCAdmin | Staff::Rooms), 0 },
  { "examine"  , "e"	  , P_AWAKE | P_MOB     , do_examine  , 0, 0, 0 },

  { "force"    , "f"	    , P_ANY       , do_force    , STAFF_CMD, Staff::Force, 0 },
  { "forget"   , "f"	     , P_ANY       , do_forget   , 1, 0, 0 },
  { "fill"     , "f"	     , P_ST        , do_pour     , 0, 0, SCMD_FILL },
  { "flee"     , "f"	     , P_FI|P_ST|P_RI, do_flee     , 1, 0, 0 },
  { "flip"     , "f"	     , P_FI|P_ST|P_RI, do_melee    , 1, 0, Combat::Flip },
  { "fly"      , "f"	      , P_FI|P_ST   , do_fly      , 0, 0, 0 },
  { "follow"   , "f"	   , P_AWAKE | P_MOB , do_follow   , 0, 0, 0 },
  { "freeze"   , "f"	   , P_ANY       , do_wizutil  , STAFF_CMD, Staff::Security, SCMD_FREEZE },

  { "get"      , "g"	      , P_AWAKE | P_MOB     , do_get      , 0, 0, 0 },
  { "give"     , "g"	     , P_AWAKE | P_MOB     , do_give     , 0, 0, 0 },
  { "goto"     , "g"	     , P_ANY       , do_goto     , STAFF_CMD, Staff::General, 0 },
  { "gold"     , "g"	     , P_AWAKE     , do_gold     , 0, 0, 0 },
  { "gossip"   , "g"	   , P_ANY       , do_gen_comm , 0, 0, SCMD_GOSSIP },
  { "group"    , "g"	    , P_AWAKE     , do_group    , 1, 0, 0 },
  { "grab"     , "g"	     , P_AWAKE | P_MOB     , do_grab     , 0, 0, 0 },
  { "grats"    , "g"	    , P_AWAKE|P_SL, do_gen_comm , 0, 0, SCMD_GRATZ },
  { "gsay"     , "g"	     , P_AWAKE | P_MOB | P_AUD, do_gsay     , 0, 0, 0 },
  { "gtell"    , "g"	    , P_AWAKE | P_MOB | P_AUD, do_gsay     , 0, 0, 0 },

  { "headbutt"     , "h"	     , P_FI|P_ST, do_combat   , 1, 0, Combat::Headbutt },
  { "help"     , "h"	     , P_ANY       , do_help     , 0, 0, SCMD_HELP },
  { "handbook" , "h"	 , P_ANY       , do_gen_ps   , STAFF_CMD, Staff::General, SCMD_HANDBOOK },
  { "hcontrol" , "h"	 , P_ANY       , do_hcontrol , STAFF_CMD, Staff::Houses, 0 },
  { "hide"     , "h"	     , P_ST        , do_hide     , 1, 0, 0 },
  { "hit"      , "h"	      , P_FI|P_ST|P_RI|P_MOB, do_hit      , 0, 0, SCMD_HIT },
  { "hold"     , "h"	     , P_AWAKE | P_MOB     , do_grab     , 1, 0, 0 },
//  { "holler"   , "h"	   , P_AWAKE | P_AUD, do_gen_comm , 1, 0, SCMD_HOLLER },
//   { "home"     , "h"	     , P_ANY       , do_home     , STAFF_CMD, Staff::Admin, 0 },
//  { "hostnames", "h"	, P_ANY       , do_hostnames, STAFF_CMD, Staff::Admin, 0 },
  { "house"    , "h"	    , P_AWAKE     , do_house    , 0, 0, 0 },

  { "inventory", "i"	, P_ANY       , do_inventory, 0, 0, 0 },
  { "idea"     , "i"	     , P_ANY       , do_gen_write, 0, 0, SCMD_IDEA },
  { "imotd"    , "i"	    , P_ANY       , do_gen_ps   , STAFF_CMD, Staff::General, SCMD_IMOTD },
  { "immlist"  , "i"	  , P_ANY       , do_gen_ps   , 0, 0, SCMD_IMMLIST },
  { "info"     , "i"	     , P_ANY       , do_gen_ps   , 0, 0, SCMD_INFO },
  { "insult"   , "i"	   , P_AWAKE | P_MOB | P_AUD, do_insult   , 0, 0, 0 },
  { "invis"    , "i"	    , P_ANY       , do_invis    , STAFF_CMD, Staff::Chars, 0 },
  { "invuln"   , "i"	   , P_ANY	     , do_invuln   , STAFF_CMD, Staff::Invuln, 0 },
  { "ironfist" , "i"	 , P_FI|P_ST|P_RI, do_combat   , 1, 0, Combat::IronFist },

  { "judge"    , "j"	    , P_ANY       , do_wizutil  , STAFF_CMD, Staff::Security, SCMD_JUDGE },
  { "junk"     , "j"	     , P_AWAKE | P_MOB     , do_drop     , 0, 0, SCMD_JUNK },

  { "kill"      , "k"	      , P_FI|P_ST|P_RI, do_hit     , 0, 0, SCMD_HIT },
//  { "kick"     , "k"	     , P_FI | P_ST , do_kick     , 1, 0, 0 },

  { "look"     , "l"	     , P_AWAKE     , do_look     , 0, 0, SCMD_LOOK },
  { "land"     , "l"	     , P_FI|P_ST   , do_land     , 0, 0, 0 },
  { "last"     , "l"	     , P_ANY       , do_last     , STAFF_CMD, Staff::General, 0 },
  { "leave"    , "l"	    , P_ST | P_RI , do_leave    , 0, 0, 0 },
  { "list"     , "l"	     , P_ST        , do_not_here , 0, 0, 0 },
  { "light"    , "l"	    , P_AWAKE | P_MOB     , do_light    , 0, 0, SCMD_LIGHT_OBJ },
  { "lock"     , "l"	     , P_ST        , do_gen_door , 0, 0, SCMD_LOCK },
  { "load"     , "l"	     , P_ANY       , do_load     , STAFF_CMD, Staff::Game, 0 },

  { "olc"      , "o"	      , P_ANY       , do_olc     , STAFF_CMD, (Staff::Rooms | Staff::Mobiles | Staff::Objects | Staff::Scripts | Staff::Shops | Staff::OLCAdmin), 0 },

	{ "aedit"		, "aed"			, P_ANY		, do_olc		, STAFF_CMD	, Staff::Socials, OLC_AEDIT},
	{ "cedit"		, "ced"			, P_ANY		, do_olc		, STAFF_CMD	, Staff::Clans, OLC_CEDIT},
	{ "hedit"		, "hed"			, P_ANY		, do_olc		, STAFF_CMD	, Staff::Help, OLC_HEDIT},
	{ "medit"    	, "med"	    	, P_ANY		, do_olc		, STAFF_CMD	, Staff::Mobiles, OLC_MEDIT },
	{ "oedit"    	, "oed"	    	, P_ANY		, do_olc		, STAFF_CMD	, Staff::Objects, OLC_OEDIT },
	{ "redit"    	, "red"	    	, P_ANY		, do_olc		, STAFF_CMD	, Staff::Rooms, OLC_REDIT },
	{ "sedit"		, "sed"			, P_ANY		, do_olc		, STAFF_CMD	, Staff::Shops, OLC_SEDIT},
	{ "tedit"		, "ted"			, P_ANY		, do_tedit		, STAFF_CMD	, Staff::Admin, 0 },
	{ "zedit"    	, "zed"			, P_ANY		, do_olc     	, STAFF_CMD	, Staff::Rooms, OLC_ZEDIT },
	{ "trigedit"	, "triged"		, P_ANY		, do_olc		, STAFF_CMD	, Staff::Scripts, OLC_SCRIPTEDIT},
	{ "skilledit"	, "skilled"		, P_ANY		, do_olc		, STAFF_CMD	, Staff::SkillEd, OLC_SKILLEDIT},

  { "zstat"    , "z"	    , P_ANY       , do_zstat   , STAFF_CMD, Staff::General, 0 },
  { "zallow"   , "z"	   , P_ANY       , do_zallow  , STAFF_CMD, Staff::OLCAdmin, 0 },
  { "zdeny"    , "z"	    , P_ANY       , do_zdeny   , STAFF_CMD, Staff::OLCAdmin, 0 },
  { "rdig"     , "r"	     , P_ANY       , do_rdig    , STAFF_CMD, Staff::Rooms, 0 },

  { "meditate" , "m"	     , P_AWAKE   , do_meditate     , 0, 0, 0 },

  { "magic"    , "m"	     , P_ANY     , do_magic     , 0, 0, 0 },
  { "chant"    , "c"	     , P_NOTFI | P_AUD     , do_magic     , 0, 0, 1 },
  { "gesture"  , "g"	     , P_AWAKE | P_MOB     , do_magic     , 0, 0, 1 },
  { "etch"     , "e"	     , P_NOTFI     , do_magic     , 0, 0, 1 },

  { "motd"     , "m"	     , P_ANY       , do_gen_ps   , 0, 0, SCMD_MOTD },
  { "mount"    , "m"	    , P_ST        , do_mount    , 0, 0, 0 },
  { "mail"     , "m"	     , P_ST        , do_not_here , 1, 0, 0 },
  { "mute"     , "m"	     , P_ANY       , do_wizutil  , STAFF_CMD, Staff::Security, SCMD_SQUELCH },
  { "murder"   , "m"	   , P_FI|P_ST|P_RI, do_hit      , 0, 0, SCMD_MURDER },

//  { "name"     , "n"	     , P_ANY       , do_name     , 0, 0, 0 },
  { "news"     , "n"	     , P_ANY       , do_gen_ps   , 0, 0, SCMD_NEWS },

  { "ooc"      , "ooc"	      , P_AWAKE | P_AUD, do_say      , 0, 0, SCMD_OOC },
  //  { "olc"      , "o"	      , P_ANY       , do_olc      , STAFF_CMD, Staff::OLCAdmin, 0 }, //
  { "order"    , "o"	    , P_AWAKE | P_MOB | P_AUD, do_order    , 1, 0, 0 },
  { "offer"    , "o"	    , P_ST        , do_not_here , 1, 0, 0 },
  { "open"     , "o"	     , P_ST|P_FI   , do_gen_door , 0, 0, SCMD_OPEN },

  { "put"      , "p"	      , P_AWAKE | P_MOB     , do_put      , 0, 0, 0 },
  { "page"     , "p"	     , P_ANY       , do_page     , STAFF_CMD, Staff::Admin, 0 },
  { "pick"     , "p"	     , P_ST        , do_gen_door , 1, 0, SCMD_PICK },
  { "policy"   , "p"	   , P_ANY       , do_gen_ps   , 0, 0, SCMD_POLICIES },
  { "poof"     , "p"	     , P_ANY       , do_poof     , STAFF_CMD, Staff::General, 0 },
  { "poofin"   , "p"	   , P_ANY       , do_poofset  , STAFF_CMD, Staff::General, SCMD_POOFIN },
  { "poofout"  , "p"	  , P_ANY       , do_poofset  , STAFF_CMD, Staff::General, SCMD_POOFOUT },
  { "pour"     , "p"	     , P_NOTFI     , do_pour     , 0, 0, SCMD_POUR },
  { "practice" , "p"	 , P_AWAKE | P_MOB     , do_practice , 1, 0, 0 },
  { "pray"     , "p"	     , P_FI|P_ST|P_RI|P_AUD, do_pray, 1, 0, 1 },
  { "prayers"  , "p"	   , P_ANY         , do_spells   , 1, 0, 1 },
  { "prefs"    , "p"	    , P_ANY       , do_preference , 0, 0, 0 },
  { "prompt"   , "p"	   , P_ANY       , do_prompt   , 0, 0, 0 },
  { "push"     , "p"	   , P_ANY       , do_push_drag   , 0, 0, SCMD_PUSH },
  { "punch"    , "p"	     , P_FI|P_ST|P_RI, do_combat   , 1, 0, Combat::Punch },
  { "purge"    , "p"	    , P_ANY       , do_purge    , STAFF_CMD, Staff::Game, 0 },

  { "quaff"    , "q"	    , P_NOTFI     , do_use      , 0, 0, SCMD_QUAFF },
  { "qecho"    , "q"	    , P_ANY | P_AUD, do_qcomm    , STAFF_CMD, Staff::Game, SCMD_QECHO },
  { "quest"    , "q"	    , P_ANY       , do_gen_tog  , 0, 0, SCMD_QUEST },
  { "quit"     , "quit"	     , P_ANY       , do_quit     , 0, 0, SCMD_QUIT },
  { "qsay"     , "q"	     , P_AWAKE | P_AUD, do_qcomm    , 0, 0, SCMD_QSAY },

  { "rest"     , "r"	     , P_SI | P_ST , do_rest     , 0, 0, 0 },
  { "read"     , "r"	     , P_AWAKE     , do_look     , 0, 0, SCMD_READ },
  { "reload"   , "r"	   , P_ANY       , do_reload   , 0, 0, 0 },
  { "reloadfile" , "r"	 , P_ANY     , do_reboot   , STAFF_CMD, Staff::Coder, 0 },
  { "recite"   , "r"	   , P_AWAKE | P_MOB     , do_use      , 0, 0, SCMD_RECITE },
  { "receive"  , "r"	  , P_ST        , do_not_here , 1, 0, 0 },
  { "remove"   , "r"	   , P_AWAKE | P_MOB     , do_remove   , 0, 0, 0 },
  { "rent"     , "r"	     , P_ST        , do_not_here , 1, 0, 0 },
  { "report"   , "r"	   , P_AWAKE | P_AUD, do_report   , 0, 0, 0 },
  { "reply"    , "r"	    , P_AWAKE	, do_reply    , 0, 0, 0 },
  { "reroll"   , "r"	   , P_ANY       , do_wizutil  , STAFF_CMD, Staff::Admin, SCMD_REROLL },
  { "rescue"   , "r"	   , P_FI | P_ST , do_rescue   , 1, 0, 0 },
  { "respond"	, "r"	   , P_AWAKE     , do_respond   , 0, 0, 0 },
  { "restore"  , "r"	  , P_ANY       , do_restore  , STAFF_CMD, Staff::Chars, 0 },
  { "return"   , "r"	   , P_ANY       , do_return   , 0, 0, 0 },
  { "ride"     , "r"	     , P_ST        , do_mount    , 0, 0, 0 },

  { "roomflags", "r"	, P_ANY       , do_gen_tog  , STAFF_CMD, Staff::General, SCMD_ROOMFLAGS },
//   { "rune"     , "r"	     , P_FI|P_ST|P_RI|P_AUD, do_rune, 0, 0, 0 },

  { "say"      , "s"	      , P_AWAKE | P_MOB | P_AUD, do_say      , 0, 0, SCMD_SAY },
  { "'"        , "'"		, P_AWAKE | P_MOB | P_AUD, do_say      , 0, 0, SCMD_SAY },
  { "sayto"	   , "s"		, P_AWAKE | P_MOB | P_AUD, do_sayto	   , 0, 0, 0 },
  { ","        , ","		, P_AWAKE | P_MOB | P_AUD, do_sayto    , 0, 0, 0 },
  { "save"     , "save"     , P_AWAKE|P_SL, do_save     , 0, 0, 0 },
  { "score"    , "s"	    , P_ANY       , do_score    , 0, 0, 0 },
  { "search"   , "s"	   , P_ST        , do_search   , 0, 0, 0 },
  { "sell"     , "s"	     , P_ST        , do_not_here , 0, 0, 0 },
  { "send"     , "s"	     , P_ANY       , do_send     , STAFF_CMD, Staff::Chars, 0 },
  { "set"      , "set"	      , P_ANY       , do_set      , STAFF_CMD, Staff::Admin, 0 },
  { "sheath"   , "s"	   , P_AWAKE | P_MOB     , do_sheath   , 0, 0, 0 },
  { "shout"    , "s"	    , P_AWAKE | P_MOB | P_AUD, do_gen_comm , 0, 0, SCMD_SHOUT },
  { "shieldbash", "s"	, P_FI | P_ST, do_melee    , 1, 0, Combat::ShieldBash },
  { "show"     , "s"	     , P_ANY       , do_show     , STAFF_CMD, Staff::General, 0 },
  { "shutdown" , "shutdown"	 , P_ANY       , do_shutdown , STAFF_CMD, Staff::Admin, SCMD_SHUTDOWN },
  { "sip"      , "s"	      , P_NOTFI     , do_drink    , 0, 0, SCMD_SIP },
  { "sit"      , "s"	      , P_AWAKE | P_MOB     , do_sit      , 0, 0, 0 },
  { "skillroll", "skillr"	, P_ANY       , do_skillroll, STAFF_CMD, Staff::General, 0 },
  { "skillset" , "skillse"	 , P_ANY       , do_skillset , STAFF_CMD, Staff::Admin, 0 },
  { "sleep"    , "s"	    , P_RE|P_SI|P_ST, do_sleep    , 0, 0, 0 },
  { "sneak"    , "s"	    , P_ST          , do_sneak    , 1, 0, 0 },
  { "snoop"    , "s"	    , P_ANY         , do_snoop    , STAFF_CMD, Staff::Security, 0 },
  { "socials"  , "s"	  , P_ANY         , do_commands , 0, 0, SCMD_SOCIALS },
  { "split"    , "s"	    , P_AWAKE | P_MOB       , do_split    , 1, 0, 0 },
  { "spells"   , "s"	   , P_ANY         , do_spells   , 1, 0, 0 },
  { "stalk"    , "s"	    , P_ST          , do_stalk    , 0, 0, 0 },
  { "stand"    , "s"	    , P_AWAKE | P_MOB       , do_stand    , 0, 0, 0 },
  { "stat"     , "s"	     , P_ANY         , do_stat     , STAFF_CMD, Staff::General, SCMD_STAT },
  { "steal"    , "s"	    , P_ST          , do_steal    , 1, 0, 0 },
  { "surrender", "surr"	, P_AWAKE | P_MOB       , do_surrender, 0, 0, 0 },
  { "switch"   , "s"	   , P_ANY         , do_switch   , STAFF_CMD, Staff::Chars, 0 },
  { "syslog"   , "s"	   , P_ANY         , do_syslog   , STAFF_CMD, Staff::Coder, 0 },
//   { "syswho"   , "s"	   , P_ANY         , do_syswho   , STAFF_CMD, Staff::Coder, 0 },
//   { "sysps"    , "s"	    , P_ANY         , do_sysps    , STAFF_CMD, Staff::Coder, 0 },

  { "tell"     , "t"	     , P_ANY, do_tell     , 0, 0, 0 },
  { "telepath"     , "t"	     , P_AWAKE | P_AUD, do_telepathy     , 0, 0, 0 },
  { "take"     , "t"	     , P_AWAKE | P_MOB       , do_get      , 0, 0, 0 },
  { "taste"    , "t"	    , P_NOTFI       , do_eat      , 0, 0, SCMD_TASTE },
  { "teleport" , "t"	 , P_ANY         , do_teleport , STAFF_CMD, Staff::Wiznet, 0 },
  { "think"    , "t"	    , P_ANY         , do_wiznet   , STAFF_CMD, Staff::Wiznet, 0 },
  { ";"        , ";"	    , P_ANY         , do_wiznet   , STAFF_CMD, Staff::General, 0 },
  { "thaw"     , "t"	     , P_ANY         , do_wizutil  , STAFF_CMD, Staff::Security, SCMD_THAW },
  { "title"    , "t"	    , P_ANY         , do_title    , 0, 0, 0 },
  { "time"     , "t"	     , P_ANY         , do_time     , 0, 0, 0 },
  { "track"    , "t"	    , P_ST          , do_track    , 0, 0, 0 },
  { "transfer" , "t"	 , P_ANY         , do_trans    , STAFF_CMD, Staff::Chars, 0 },
  { "typo"     , "t"	     , P_ANY         , do_gen_write, 0, 0, SCMD_TYPO },

  { "unlock"   , "u"	   , P_ST          , do_gen_door , 0, 0, SCMD_UNLOCK },
  { "ungroup"  , "u"	  , P_FI|P_ST|P_RI, do_ungroup  , 0, 0, 0 },
  { "unban"    , "u"	    , P_ANY         , do_unban    , STAFF_CMD, Staff::Security, 0 },
  { "unaffect" , "u"	 , P_ANY         , do_wizutil  , STAFF_CMD, Staff::Admin, SCMD_UNAFFECT },
  { "uptime"   , "u"	   , P_ANY         , do_date     , STAFF_CMD, Staff::General, SCMD_UPTIME },
  { "use"      , "u"	      , P_AWAKE | P_MOB       , do_use      , 1, 0, SCMD_USE },
  { "users"    , "u"	    , P_ANY         , do_users    , STAFF_CMD, Staff::Admin, 0 },

  { "value"    , "v"	    , P_ST          , do_not_here , 0, 0, 0 },
  { "version"  , "v"	  , P_ANY         , do_gen_ps   , 0, 0, SCMD_VERSION },
  { "visible"  , "v"	  , P_AWAKE | P_MOB       , do_visible  , 1, 0, 0 },
  { "vnum"     , "v"	     , P_ANY         , do_vnum     , STAFF_CMD, Staff::General, 0 },
  { "vstat"    , "v"	    , P_ANY         , do_vstat    , STAFF_CMD, Staff::General, 0 },

  { "wake"     , "w"	     , P_SL | P_AWAKE, do_wake     , 0, 0, 0 },
  { "watch"    , "w"	     , P_AWAKE | P_MOB		, do_watch     , 0, 0, 0 },
  { "wear"     , "w"	     , P_AWAKE | P_MOB       , do_wear     , 0, 0, 0 },
  { "weather"  , "w"	  , P_AWAKE       , do_weather  , 0, 0, 0 },
  { "who"      , "w"	      , P_ANY         , do_who      , 0, 0, 0 },
  { "whoami"   , "w"	   , P_ANY         , do_gen_ps   , 0, 0, SCMD_WHOAMI },
//   { "whois"    , "w"	    , P_ANY         , do_whois    , 0, 0, 0 },
  { "where"    , "w"	    , P_AWAKE       , do_where    , 0, 0, 0 },
  { "whisper"  , "w"	  , P_AWAKE | P_MOB | P_AUD, do_whisper, 0, 0, 0 },
  { "wield"    , "w"	    , P_AWAKE | P_MOB       , do_wield    , 0, 0, 0 },
  { "wimpy"    , "w"	    , P_ANY         , do_wimpy    , 0, 0, 0 },
  { "withdraw" , "w"		, P_ST          , do_not_here , 1, 0, 0 },
  { "wizlist"  , "w"	  , P_ANY         , do_gen_ps   , 0, 0, SCMD_WIZLIST },
  { "wizlock"  , "wizlock"	  , P_ANY         , do_wizlock  , STAFF_CMD, Staff::Admin, 0 },
  { "write"    , "w"	    , P_RE|P_SI|P_ST, do_write    , 1, 0, 0 },


  { "zreset"   , "z"	   , P_ANY         , do_zreset   , STAFF_CMD, (Staff::Rooms | Staff::Mobiles | Staff::Objects | Staff::Scripts | Staff::Shops | Staff::OLCAdmin), 0 },

  { "slay"     , "s"	     , P_AWAKE | P_MOB       , do_slay     , 0, 0, 0 },
  { "highfive" , "h"	 , P_ST          , do_highfive , 0, 0, 0 },
//   { "pose"     , "p"	     , P_ST          , do_pose     , 0, 0, 0 },
  { "system"   , "system"	   , P_ANY         , do_gecho    , STAFF_CMD, Staff::Admin, 0 },
  { "afk"      , "a"	      , P_ANY, do_afk      , 0, 0, 0 },
  { "scan"     , "s"	     , P_ST | P_RI   , do_scan     , 0, 0, 0 },
  { "immort"   , "immort"	   , P_ANY         , do_wizutil  , STAFF_CMD, Staff::Admin, SCMD_IMMORT },
  { "staff"   , "staff"	   , P_ANY         , do_wizutil  , STAFF_CMD, Staff::Admin, SCMD_STAFF },
  { "wizhelp"  , "w"	  , P_ANY         , do_help     , STAFF_CMD, Staff::General, SCMD_WIZHELP },
  { "pagelength", "p"	,P_ANY         , do_pagelength,0, 0, 0 },
  { "attach"   , "attach"	   , P_ANY         , do_attach   , STAFF_CMD, Staff::Scripts, 0 },
  { "detach"   , "detach"	   , P_ANY         , do_detach   , STAFF_CMD, Staff::Scripts, 0 },
  { "sstat"    , "s"	    , P_ANY         , do_stat     , STAFF_CMD, Staff::General, SCMD_SSTAT},

//   { "masound" , "m"	 , P_ANY          , do_masound , MOB_CMD, 0, 0 },
//   { "mjunk"   , "m"	   , P_ANY          , do_mjunk   , MOB_CMD, 0, 0 },
//   { "mecho"   , "m"	   , P_ANY          , do_mecho   , MOB_CMD, 0, 0 },
//   { "msend"   , "m"	   , P_ANY          , do_msend   , MOB_CMD, 0, 0 },
//   { "mechoat" , "m"	 , P_ANY          , do_msend   , MOB_CMD, 0, 0 },
//   { "mechoaround", "m"	, P_ANY       , do_mechoaround , MOB_CMD, 0, 0 },
//   { "mkill"   , "m"	   , P_ANY          , do_mkill   , MOB_CMD, 0, 0 },
//   { "mload"   , "m"	   , P_ANY          , do_mload   , MOB_CMD, 0, 0 },
//   { "mpurge"  , "m"	  , P_ANY          , do_mpurge  , MOB_CMD, 0, 0 },
//   { "mgoto"   , "m"	   , P_ANY          , do_mgoto   , MOB_CMD, 0, 0 },
//   { "mat"     , "m"	     , P_ANY          , do_mat     , MOB_CMD, 0, 0 },
//   { "mteleport", "m"	, P_ANY          , do_mteleport,MOB_CMD, 0, 0 },
//   { "mtransfer", "m"	, P_ANY          , do_mtransfer,MOB_CMD, 0, 0 },
//   { "mforce"  , "m"	  , P_ANY          , do_mforce  , MOB_CMD, 0, 0 },
//   { "mexp"    , "m"	    , P_ANY          , do_mexp    , MOB_CMD, 0, 0 },
//   { "mhunt"   , "m"	   , P_ANY          , do_mhunt   , MOB_CMD, 0, 0 },
//   { "mseek"   , "m"	   , P_ANY          , do_mseek   , MOB_CMD, 0, 0 },
//   { "mroomseek", "m"	, P_ANY         , do_mroomseek, MOB_CMD, 0, 0 },
//   { "minroomseek", "m"	, P_ANY       , do_minroomseek, MOB_CMD, 0, 0 },
//   { "mdelayed", "m"	, P_ANY          , do_mdelayed, MOB_CMD, 0, 0 },
//   { "mdamage", "m"	, P_ANY           , do_mdamage, MOB_CMD, 0, 0 },


  { "combat"   , "c"	   , P_ANY       , do_combatprofile, 0, 0, 0 },

  { "pull"     , "p"	     , P_ST        , do_pull     , 0, 0, 0 },
  { "shoot"    , "s"	    , P_ST        , do_shoot    , 0, 0, 0 },
  { "throw"    , "t"	    , P_ST        , do_throw    , 0, 0, 0 },
  { "skillinfo" , "s"	    , P_ANY        , do_skillinfo    , 0, 0, 0 },

	{ "delete"		, "d"			, P_ANY		, do_delete		, STAFF_CMD	, Staff::OLCAdmin, SCMD_DELETE },
	{ "undelete"	, "u"		, P_ANY		, do_delete		, STAFF_CMD	, Staff::OLCAdmin, SCMD_UNDELETE },

	{ "massroomsave", "m"	, P_ANY		, do_massroomsave, STAFF_CMD, Staff::Coder, 0 },

	{ "buffer"		, "b"			, P_ANY		, do_buffer		, STAFF_CMD	, Staff::Coder, 0 },
	{ "crash"		, "c"			, P_ANY		, do_overflow	, STAFF_CMD	, Staff::Coder, 0 },
	{ "coredump"	, "coredump"	, P_ANY     , do_coredump	, STAFF_CMD	, Staff::Coder, 0 },

	{ "path"		, "p"			, P_ANY		, do_path		, TRUST_IMMORTAL, 0, 0 },

	{ "warning"		, "w"			, P_ANY		, do_gen_write	, STAFF_CMD	, Staff::Security, SCMD_WARNING },

	{ "copyover"	, "c"		, P_ANY		, do_copyover	, STAFF_CMD	, Staff::Admin, 0 },

	{ "vwear"		, "v"			, P_ANY		, do_vwear		, STAFF_CMD	, Staff::General, 0 },

	{ "string"		, "s"			, P_ANY		, do_string		, STAFF_CMD	, Staff::Chars, 0 },

	{ "quest"		, "q"			, P_ANY		, do_gen_tog	, 0, 0, SCMD_QUEST },

	{ "\n"			, "zzzzzzz"		, POS_DEAD, 0, 0, 0 }	//	This must be last
};
