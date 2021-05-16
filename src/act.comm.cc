/* ************************************************************************
*   File: act.comm.c                                    Part of CircleMUD *
*  Usage: Player-level communication commands                             *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */


#include "mud.base.h"
#include "characters.h"
#include "rooms.h"
#include "objects.h"
#include "descriptors.h"
#include "utils.h"
#include "buffer.h"
#include "comm.h"
#include "interpreter.h"
#include "find.h"
#include "db.h"
#include "boards.h"
#include "clans.h"
#include "constants.h"

void speech_mtrigger(Character *actor, char *str);
void speech_otrigger(Character *actor, char *str);
void speech_wtrigger(Character *actor, char *str);


/* extern variables */
extern int level_can_shout;
extern int holler_move_cost;

/* local functions */
void perform_tell(Character *ch, Character *vict, char *arg);
bool is_tell_ok(Character *ch, Character *vict);
ACMD(do_say);
ACMD(do_sayto);
ACMD(do_gsay);
ACMD(do_tell);
ACMD(do_reply);
//ACMD(do_spec_comm);
ACMD(do_whisper);
ACMD(do_write);
ACMD(do_page);
ACMD(do_gen_comm);
ACMD(do_qcomm);


ACMD(do_say) {
	char *arg;
	char *verb, *s;
	bool hide = false;

	skip_spaces(argument);

	if (!*argument)
		ch->Send("Yes, but WHAT do you want to say?\r\n");
	else if (GET_POS(ch) == POS_SLEEPING)
		ch->Send("In your dreams, or what?\r\n");
	else {
//		Character *vict;
		arg = get_buffer(MAX_STRING_LENGTH);
		
/*		any_one_name(argument, arg);
		
		if ((vict = get_char_room(arg, IN_ROOM(ch))))
			argument = any_one_name(argument, arg);
		
*/		
		strcpy(arg, argument);
		
		s = arg + strlen(arg);
		while (isspace(*(--s)));
		*(s + 1) = '\0';
		
		switch (*s) {
			case '?':	verb = "ask";		break;
			case '!':	verb = "exclaim";	break;
			default:	verb = "say";		break;
		}
		
		if (ch->material == Materials::Ether)
			hide = true;

		sprintf(buf, "%s&cB$n %ss, '%s%s&cB'&c0", ((subcmd == SCMD_OOC) ? "&cW(OOC) " : ""),
	            verb, colorcodes[GET_SAYCOLOR(ch)], arg);
		act(buf, hide, ch, 0, 0, TO_ROOM);

		if (PRF_FLAGGED(ch, Preference::NoRepeat))	ch->Send(OK);
		else {
			delete_doubledollar(arg);
			ch->Sendf("%s&cBYou %s, '%s%s&cB'&c0\r\n", ((subcmd == SCMD_OOC) ? "&cW(OOC) " : ""),
					verb, colorcodes[GET_SAYCOLOR(ch)], arg);
		}

		release_buffer(arg);
	}
	speech_mtrigger(ch, argument);
	speech_otrigger(ch, argument);
	speech_wtrigger(ch, argument);
}

ACMD(do_sayto) 
{
	Character *vict;
	char *action_sing, *action_plur, *action_others;
	char *buf = get_buffer(MAX_STRING_LENGTH);
	char *s, *verbplural, *verbsingular;

	argument = one_argument(argument, buf);

	if (!*buf || !*argument) 
	{
		ch->Sendf("Whom do you want to speak to.. and what??\r\n");
	}
	else if (!(vict = get_char_vis(ch, buf, FIND_CHAR_ROOM)))
		ch->Send(NOPERSON);
	else 
	{
	   	s = argument + strlen(argument);
		while (isspace(*(--s)));
		*(s + 1) = '\0';

		switch (*s) 
		{
			case '?':	
				verbplural = "asks";
				verbsingular = "ask";
				break;
			case '!':
				verbplural = "exclaims to";
				verbsingular = "exclaim to";	
				break;
			default:
				verbplural = "says to";
				verbsingular = "say to";
				break;
		}
		sprintf(buf, "&cB$n %s you, '%s%s&cB' &c0", verbplural, colorcodes[GET_SAYCOLOR(ch)], argument);
		act(buf, FALSE, ch, 0, vict, TO_VICT);
		if (PRF_FLAGGED(ch, Preference::NoRepeat))
			ch->Send(OK);
		else 
		{
			sprintf(buf, "&cBYou %s $N, '%s%s&cB'&c0", verbsingular, colorcodes[GET_SAYCOLOR(ch)], argument);
			act(buf, FALSE, ch, 0, vict, TO_CHAR);
		}
	   	sprintf(buf, "&cB$n %s $N, '%s%s&cB' &c0", verbplural, colorcodes[GET_SAYCOLOR(ch)], argument);
		act(buf, FALSE, ch, 0, vict, TO_NOTVICT);
	}
	release_buffer(buf);
	speech_mtrigger(ch, argument);
	speech_otrigger(ch, argument);
	speech_wtrigger(ch, argument);

}

ACMD(do_telepathy)
{
	Character *vict = NULL;

	half_chop(argument, buf, buf2);

	if (!PLR_FLAGGED(ch, PLR_TELEPATHY))
		ch->Send("You try really hard to think, but only end up hurting yourself,\r\n");
	else if (!*buf || !*buf2)
		ch->Send("You reflect upon your mental power.\r\n");
	else if (!(vict = get_char_vis(ch, buf, FIND_CHAR_WORLD)))
		ch->Send("You cannot sense that entity.\r\n");
	else if (ch == vict)
		ch->Send("Your thoughts ring clearly in your head.\r\n");
	else if (PLR_FLAGGED(vict, PLR_WRITING))
		ch->Send("Your words reach a numbed mind.\r\n");
	else {
		sprintf(buf, "%s", buf2);	// Include Telepathy header in future
		act(buf, FALSE, ch, 0, vict, TO_VICT | TO_SLEEP);	
		sprintf(buf, "You send %s into $N's mind.", buf2);
		act(buf, FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
	}
}


ACMD(do_gsay) {
	Character *k, *follower;

	// delete_doubledollar(argument);
	skip_spaces(argument);

	if (!AFF_FLAGGED(ch, AffectBit::Group))
		ch->Send("But you are not the member of a group!\r\n");
	else if (!*argument)
		ch->Send("Yes, but WHAT do you want to group-say?\r\n");
	else {
		k = ch->master ? ch->master : ch;

	    sprintf(buf, "&cc$n tells the group, '%s%s&cc'&c0", 
			    colorcodes[GET_SAYCOLOR(ch)], argument);

		if (AFF_FLAGGED(k, AffectBit::Group) && (k != ch))
			act(buf, FALSE, ch, 0, k, TO_VICT | TO_SLEEP);
		
		LListIterator<Character *>	iter(k->followers);
		while ((follower = iter.Next())) {
			if (AFF_FLAGGED(follower, AffectBit::Group) && (follower != ch))
				act(buf, FALSE, ch, 0, follower, TO_VICT | TO_SLEEP);
		}
		
		if (PRF_FLAGGED(ch, Preference::NoRepeat))
			ch->Send(OK);
		else {
			delete_doubledollar(argument);
			sprintf(buf, "&ccYou tell the group, '%s%s&cc'&c0", 
			colorcodes[GET_SAYCOLOR(ch)], argument);
			act(buf, FALSE, ch, 0, 0, TO_CHAR | TO_SLEEP);
		}
	}
}


void perform_tell(Character *ch, Character *vict, char *arg) 
{
	char * buf = get_buffer(MAX_STRING_LENGTH);

	// delete_doubledollar(arg);

	sprintf(buf, "&cc$n tells you, '%s%s&cc'&c0", colorcodes[GET_SAYCOLOR(ch)], arg);
	act(buf, FALSE, ch, 0, vict, TO_VICT | TO_SLEEP);	


	if (PRF_FLAGGED(ch, Preference::NoRepeat)) {
		if (PLR_FLAGGED(vict, PLR_AFK))	ch->Sendf("%s is AFK.\r\n", vict->Name());
		ch->Send(OK);
	} else {
		delete_doubledollar(arg);
		sprintf(buf, "&ccYou tell $N%s, '%s%s&cc'&c0", PLR_FLAGGED(vict,PLR_AFK) ? " (AFK)" : "", 
				colorcodes[GET_SAYCOLOR(ch)], arg);
		act(buf, FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
	}

	if (!IS_NPC(vict))
		GET_LAST_TELL(vict) = GET_ID(ch);
        
	release_buffer(buf);
}


bool is_tell_ok(Character *ch, Character *vict) {
	if (ch == vict)
		ch->Send("You try to tell yourself something.\r\n");
	else if (CHN_FLAGGED(ch, Channel::NoTell))
		ch->Send("You can't tell other people while you have notell on.\r\n");
	else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SOUNDPROOF))
		ch->Send("The walls seem to absorb your words.\r\n");
	else if (!IS_NPC(vict) && !vict->desc)        /* linkless */
		act("$E's linkless at the moment.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
	else if (PLR_FLAGGED(vict, PLR_WRITING))
		act("$E's writing a message right now; try again later.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
	else if (CHN_FLAGGED(vict, Channel::NoTell) || ROOM_FLAGGED(IN_ROOM(vict), ROOM_SOUNDPROOF))
		act("$E can't hear you.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
	else
		return true;
	return false;
}



/*
 * Yes, do_tell probably could be combined with whisper and ask, but
 * called frequently, and should IMHO be kept as tight as possible.
 */
ACMD(do_tell) {
	Character *vict;
	char *buf = get_buffer(MAX_STRING_LENGTH);

    argument = one_argument(argument, buf);

	if (!*buf || !*argument)
		ch->Send("Who do you wish to tell what??\r\n");
	else if (!(vict = get_char_vis(ch, buf, FIND_CHAR_WORLD)))
		ch->Send(NOPERSON);
	else if (is_tell_ok(ch, vict)) {
		if (!IS_NPC(vict) && GET_AFK(vict)) {
			act("$N is AFK and may not hear your tell.\r\n"
				"MESSAGE: $t\r\n", TRUE, ch, (Object *)GET_AFK(vict), vict, TO_CHAR);
		}
		perform_tell(ch, vict, argument);
	}
	
	release_buffer(buf);
}


ACMD(do_reply) {
	Character *tch;
	
	if (IS_NPC(ch))
		return;
	
	skip_spaces(argument);

	if (!GET_LAST_TELL(ch))				ch->Send("You have no-one to reply to!\r\n");
	else if (!*argument)				ch->Send("What is your reply?\r\n");
	else if (!(tch = find_char(GET_LAST_TELL(ch)))) {
		ch->Send("They are no longer playing.\r\n");
		GET_LAST_TELL(ch) = NOBODY;
	} else if (is_tell_ok(ch, tch))	{
		perform_tell(ch, tch, argument);
	}
}


ACMD(do_whisper) {
	Character *vict;
	char *action_sing, *action_plur, *action_others;
	char *buf = get_buffer(MAX_STRING_LENGTH);

	argument = one_argument(argument, buf);

	if (!*buf || !*argument) {
		ch->Sendf("Whom do you want to whisper to.. and what??\r\n");
	} else if (!(vict = get_char_vis(ch, buf, FIND_CHAR_ROOM)))
		ch->Send(NOPERSON);
	else if (vict == ch)
		ch->Send("You can't get your mouth close enough to your ear...\r\n");
	else {
	    sprintf(buf, "&cB$n whispers to you, '%s%s&cB'&c0", colorcodes[GET_SAYCOLOR(ch)], argument);
		act(buf, FALSE, ch, 0, vict, TO_VICT);
		if (PRF_FLAGGED(ch, Preference::NoRepeat))
			ch->Send(OK);
		else {
			sprintf(buf, "&cBYou whisper to $N, '%s%s&cB'&c0", colorcodes[GET_SAYCOLOR(ch)], argument);
			act(buf, FALSE, ch, 0, vict, TO_CHAR);
		}
		act("$n whispers something to $N.", FALSE, ch, 0, vict, TO_NOTVICT);
	}
	release_buffer(buf);
}


#define MAX_NOTE_LENGTH 1000	/* arbitrary */

ACMD(do_write) {
	Object *paper = NULL, *pen = NULL, *obj, *held;
	char *papername, *penname;
	int temp = 0;
	Editor * tmpeditor = NULL;

	if (!ch->desc)
		return;

	/* before we do anything, lets see if there's a board involved. */
	if(!(obj = get_obj_in_list_type(ITEM_BOARD, ch->contents)))
		obj = get_obj_in_list_type(ITEM_BOARD, world[IN_ROOM(ch)].contents);
	
	if(obj)                /* then there IS a board! */
		Board::WriteMessage(obj->Virtual(), ch, argument);
	else {
		char *buf1 = get_buffer(MAX_INPUT_LENGTH);
		char *buf2 = get_buffer(MAX_INPUT_LENGTH);
		
		papername = buf1;
		penname = buf2;

		two_arguments(argument, papername, penname);

		if (!*papername)		/* nothing was delivered */
			ch->Send("Write?  With what?  ON what?  What are you trying to do?!?\r\n");
		else if (*penname) {		/* there were two arguments */
			if (!(paper = get_obj_in_list_vis(ch, papername, ch->contents)))
				ch->Sendf("You have no %s.\r\n", papername);
			else if (!(pen = get_obj_in_list_vis(ch, penname, ch->contents)))
				ch->Sendf("You have no %s.\r\n", penname);
		} else {		/* there was one arg.. let's see what we can find */
			if (!(paper = get_obj_in_list_vis(ch, papername, ch->contents)))
				ch->Sendf("There is no %s in your inventory.\r\n", papername);
			else if (GET_OBJ_TYPE(paper) == ITEM_PEN) {	/* oops, a pen.. */
				pen = paper;
				paper = NULL;
			} else if (GET_OBJ_TYPE(paper) != ITEM_NOTE) {
				ch->Send("That thing has nothing to do with writing.\r\n");
				release_buffer(buf1);
				release_buffer(buf2);
				return;
			}
			/* One object was found.. now for the other one. */
			held = GET_EQ(ch, WEAR_HAND_R);
			if (!held || (GET_OBJ_TYPE(held) != (pen ? ITEM_NOTE : ITEM_PEN))) {
				held = GET_EQ(ch, WEAR_HAND_L);
				if (!held || (GET_OBJ_TYPE(held) != (pen ? ITEM_NOTE : ITEM_PEN))) {
					ch->Sendf("You can't write with %s %s alone.\r\n", AN(papername), papername);
					pen = NULL;
				}
			}
			if (held && !held->CanBeSeen(ch))
				ch->Send("The stuff in your hand is invisible!  Yeech!!\r\n");
			else if (pen)	paper = held;
			else			pen = held;
		}
		
		if (paper && pen) {
			/* ok.. now let's see what kind of stuff we've found */
			if (GET_OBJ_TYPE(pen) != ITEM_PEN)
				act("$p is no good for writing with.", FALSE, ch, pen, 0, TO_CHAR);
			else if (GET_OBJ_TYPE(paper) != ITEM_NOTE)
				act("You can't write on $p.", FALSE, ch, paper, 0, TO_CHAR);
			else if (paper->actionDesc)
				ch->Send("There's something written on it already.\r\n");
			else {
				ch->Send("Write your note.  (/s saves /h for help)\r\n");
				tmpeditor = new TextEditor(ch->desc, &paper->actionDesc->str, MAX_NOTE_LENGTH);
				ch->desc->Edit(tmpeditor);
				act("$n begins to jot down a note.", TRUE, ch, 0, 0, TO_ROOM);
			}
		}
		release_buffer(buf1);
		release_buffer(buf2);
	}
}


ACMD(do_page) {
	Descriptor *d;
	Character *vict;
	char *arg1 = get_buffer(MAX_INPUT_LENGTH);

	argument = one_argument(argument, arg1);

	if (IS_NPC(ch))		ch->Send("Monsters can't page.. go away.\r\n");
	else if (!*arg1)	ch->Send("Whom do you wish to page?\r\n");
	else {
		if (!str_cmp(arg1, "all")) {
			if (STF_FLAGGED(ch, Staff::Admin | Staff::Game | Staff::Security)) {
				START_ITER(iter, d, Descriptor *, descriptor_list) {
					if ((STATE(d) == CON_PLAYING) && d->character)
						act("\b\b*$n* $t\r\n", FALSE, ch, (Object *)argument, d->character, TO_VICT);
				}
			} else
				ch->Send("You are not privileged enough to do that!\r\n");
		} else if ((vict = get_char_vis(ch, arg1, FIND_CHAR_WORLD)) != NULL) {
			act("\b\b*$n* $t\r\n", FALSE, ch, (Object *)argument, vict, TO_VICT);
			if (PRF_FLAGGED(ch, Preference::NoRepeat))
				ch->Send(OK);
			else
				act("\b\b-$N-> $t\r\n", FALSE, ch, (Object *)argument, vict, TO_CHAR);
		} else
			ch->Send("There is no such player!\r\n");
	}
	release_buffer(arg1);
}


/**********************************************************************
 * generalized communication func, originally by Fred C. Merkel (Torg) *
  *********************************************************************/

class NewChannel {
public:
	char *			name;
	char *			command;
	char *			cannot;
	char *			changeflag;
	char *			msg;
	char *			chmsg;
	int				flags;
};


ACMD(do_gen_comm) {
	char *sendout;
	/* Array of flags which must _not_ be set in order for comm to be heard */
	const UInt32 channels[] = {
		Channel::NoShout,
		Channel::NoGossip,
		Channel::NoSound,
		Channel::NoGratz,
		Channel::NoClan,
		0,
		0
	};

	/*
	 * com_msgs: [0] Message if you can't perform the action because of noshout
	 *           [1] name of the action
	 *           [2] message if you're not on the channel
	 *           [3] a color string.
	 */
	static const char *com_msgs[][5] = {
		{	"You cannot shout!!\r\n",
			"shout",
			"Turn off your noshout flag first!\r\n",
			"&cY$n shouts, '$t&cY'&c0",
			"&cYYou shout, '$T&cY'&c0",
		},

		{	"You cannot chat!!\r\n",
			"chat",
			"You aren't even on the channel!\r\n",
			"&cY$n chats, '$t&cY'&c0",
			"&cYYou chat, '$T&cY'&c0",
		},

		{	"You cannot congratulate!\r\n",
			"congrat",
			"You aren't even on the channel!\r\n",
			"&cG$n congratulates, '$t&cG'&c0",
			"&cGYou congratulate, '$T&cG'&c0",
		},
		{	"You cannot converse with your clan!\r\n",
			"clanchat",
			"You aren't even on the channel!\r\n",
			"&cG[&cMCLAN&cG] $n&cM: $t&c0",
			"&cG[&cMCLAN&cG] -> &cM$T&c0",
		},
		{
			"You cannot chat with others on the quest!\r\n",
			"quest",
			"",
			"&cG[&cMQUEST&cG] $n&cM: $t&c0",
			"&cG[&cMQUEST&cG] -> &cM$T&c0",
		}
	};

	skip_spaces(argument);
	
	if (PLR_FLAGGED(ch, PLR_NOSHOUT))				ch->Send(com_msgs[subcmd][0]);
	else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SOUNDPROOF) && !NO_STAFF_HASSLE(ch))
		ch->Send("The walls seem to absorb your words.\r\n");
#ifdef GOT_RID_OF_IT
// CHANGEPOINT
	else if (subcmd != SCMD_MISSION && CHN_FLAGGED(ch, channels[subcmd]))
		ch->Send(com_msgs[subcmd][2]);
	else if (subcmd == SCMD_MISSION && !CHN_FLAGGED(ch, Channel::Quest))
		ch->Send("You aren't even part of the quest!\r\n");
#endif
	else if (subcmd == SCMD_CLAN && !Clan::Find(GET_CLAN(ch)))
		ch->Send("You aren't even a clan member!\r\n");
	else if (!*argument)
		ch->Sendf("Yes, %s, fine, %s we must, but WHAT???\r\n", com_msgs[subcmd][1], com_msgs[subcmd][1]);
	else {
//		char *buf = get_buffer(MAX_STRING_LENGTH);
		Descriptor *i;
	
		delete_doubledollar(argument);
		sendout = get_buffer(MAX_STRING_LENGTH);
		
		sprintf(sendout, "%s%s", colorcodes[GET_SAYCOLOR(ch)], argument);
		
		/* first, set up strings to be given to the communicator */
		if (PRF_FLAGGED(ch, Preference::NoRepeat))	ch->Send(OK);
		else	act(com_msgs[subcmd][4], false, ch, 0, sendout, TO_CHAR | TO_SLEEP);
		
//		sprintf(buf, com_msgs[subcmd][3], argument);

		/* now send all the strings out */
		LListIterator<Descriptor *>	iter(descriptor_list);
		while ((i = iter.Next())) {
			if ((STATE(i) == CON_PLAYING) && (i != ch->desc) && i->character &&
					!CHN_FLAGGED(i->character, channels[subcmd]) &&
					!PLR_FLAGGED(i->character, PLR_WRITING) &&
					!ROOM_FLAGGED(IN_ROOM(i->character), ROOM_SOUNDPROOF)) {
	
				if ((subcmd == SCMD_SHOUT) &&
						((world[IN_ROOM(ch)].zone != world[IN_ROOM(i->character)].zone) ||
						GET_POS(i->character) < POS_RESTING))
					continue;
				if ((subcmd != SCMD_GOSSIP) && (ch->material == Materials::Ether) && 
					(i->character->material != Materials::Ether))
					continue;
				if ((subcmd == SCMD_CLAN) && 
						((GET_CLAN(ch) != GET_CLAN(i->character)) || 
						(GET_CLAN_LEVEL(i->character) < Clan::Rank::Member)))
					continue;
				if ((subcmd == SCMD_QUEST) && !CHN_FLAGGED(i->character, Channel::Quest))
					continue;

				if ((PLR_FLAGGED(ch, PLR_DOZY)) && (STF_FLAGGED(i->character, Staff::Admin)))
					i->Write("&c+&cW<DOZY>&c- ");
				
				act(com_msgs[subcmd][3], false, ch, reinterpret_cast<Object *>(sendout), i->character, TO_VICT | TO_SLEEP);
			}
		}
//		release_buffer(buf);
		release_buffer(sendout);
	}
}


ACMD(do_qcomm) {
	Descriptor *i;
	
	skip_spaces(argument);

	if (!CHN_FLAGGED(ch, Channel::Quest))
		ch->Send("You aren't even on the quest channel!\r\n");
	else if (!*argument)
		ch->Send("Yes, fine, %s we must, but WHAT??\r\n");
	else {
		if (PRF_FLAGGED(ch, Preference::NoRepeat))
			ch->Send(OK);
		else
			act(argument, FALSE, ch, 0, 0, TO_CHAR);

		LListIterator<Descriptor *>	iter(descriptor_list);
		while ((i = iter.Next())) {
			if ((STATE(i) == CON_PLAYING) && i != ch->desc && CHN_FLAGGED(i->character, Channel::Quest))
				act(argument, 0, ch, 0, i->character, TO_VICT | TO_SLEEP);
		}
	}
}
