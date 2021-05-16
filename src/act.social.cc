/* ************************************************************************
*   File: act.social.c                                  Part of CircleMUD *
*  Usage: Functions to handle socials                                     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "characters.h"
#include "descriptors.h"
#include "socials.h"
#include "rooms.h"
#include "objects.h"
#include "utils.h"
#include "buffer.h"
#include "comm.h"
#include "interpreter.h"
#include "find.h"
#include "db.h"

/* extern variables */
extern struct command_info cmd_info[];

/* extern functions */
char *fread_action(FILE * fl, int nr);

/* local functions */
void create_command_list(void);
ACMD(do_action);
ACMD(do_insult);


#define NUM_RESERVED_CMDS	6

void Social::Clear(void) {
	if (command)			free(command);
	if (sort_as)			free(sort_as);
	if (char_no_arg)		free(char_no_arg);
	if (others_no_arg)		free(others_no_arg);
	if (char_found)			free(char_found);
	if (others_found)		free(others_found);
	if (vict_found)			free(vict_found);
	if (char_body_found)	free(char_body_found);
	if (others_body_found)	free(others_body_found);
	if (vict_body_found)	free(vict_body_found);
	if (not_found)			free(not_found);
	if (char_auto)			free(char_auto);
	if (others_auto)		free(others_auto);
	if (char_obj_found)		free(char_obj_found);
	if (others_obj_found)	free(others_obj_found);

	memset(this, 0, sizeof(Social));	// Won't this NULL member methods?
}


ACMD(do_action) {
	Character *vict;
	Object *targ;
	char *buf, *buf2;
	Social *	action;
	bool	hide = false;

	if (!(action = Social::Find(cmd))) {
		ch->Send("That action is not supported.\r\n");
		return;
	}

	buf = get_buffer(MAX_INPUT_LENGTH);
	buf2 = get_buffer(MAX_INPUT_LENGTH);

	if (action->hide || ch->material == Materials::Ether)
		hide = true;

	half_chop(argument, buf, buf2);

	if ((!action->char_body_found) && (*buf2)) {
		ch->Send("Sorry, this social does not support body parts.\r\n");
	} else if (!action->char_found || !*buf) {
		act(action->char_no_arg, false, ch, 0, 0, TO_CHAR);
		act(action->others_no_arg, hide, ch, 0, 0, TO_ROOM);
	} else if (!(vict = get_char_vis(ch, buf, FIND_CHAR_ROOM))) {
		if ((action->char_obj_found) &&
				((targ = get_obj_in_list_vis(ch, buf, ch->contents)) ||
				(targ = get_obj_in_list_vis(ch, buf, world[IN_ROOM(ch)].contents)))) {
			act(action->char_obj_found, hide, ch, targ, 0, TO_CHAR);
			act(action->others_obj_found, hide, ch, targ, 0, TO_ROOM);
		} else
			act(action->not_found, false, ch, 0, 0, TO_CHAR);
	} else if (vict == ch) {
		act(action->char_auto, false, ch, 0, 0, TO_CHAR);
		act(action->others_auto, hide, ch, 0, 0, TO_ROOM);
	} else {
		if (!((1 << GET_POS(ch)) & complete_cmd_info[cmd].position))
			act("$N is not in a proper position for that.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
		else {
			if (*buf2)  {
				act(action->char_body_found, 0, ch, (Object *)buf2, vict, TO_CHAR | TO_SLEEP);
				act(action->others_body_found, hide, ch, (Object *)buf2, vict, TO_NOTVICT);
				act(action->vict_body_found, hide, ch, (Object *)buf2, vict, TO_VICT);
			} else  {
				act(action->char_found, 0, ch, 0, vict, TO_CHAR | TO_SLEEP);
				act(action->others_found, hide, ch, 0, vict, TO_NOTVICT);
				act(action->vict_found, hide, ch, 0, vict, TO_VICT);
			}
		}
	}
	release_buffer(buf);
	release_buffer(buf2);
}



ACMD(do_insult) {
	Character *victim;
	char *arg = get_buffer(MAX_INPUT_LENGTH);

	one_argument(argument, arg);

	if (*arg) {
		if (!(victim = get_char_vis(ch, arg, FIND_CHAR_ROOM)))
			ch->Send("Can't hear you!\r\n");
		else {
			if (victim != ch) {
				act("You insult $N.\r\n", TRUE, ch, 0, victim, TO_CHAR);

				switch (Number(0, 2)) {
				case 0:
					if (GET_SEX(ch) == Male) {
						if (GET_SEX(victim) == Male)
							act("$n accuses you of fighting like a woman!", FALSE, ch, 0, victim, TO_VICT);
						else
							act("$n says that women can't fight.", FALSE, ch, 0, victim, TO_VICT);
					} else {		/* Ch == Woman */
						if (GET_SEX(victim) == Male)
							act("$n accuses you of having the smallest... (brain?)",
									FALSE, ch, 0, victim, TO_VICT);
						else
							act("$n tells you that you'd lose a beauty contest against a troll.",
									FALSE, ch, 0, victim, TO_VICT);
					}
					break;
				case 1:
					act("$n calls your mother a bitch!", FALSE, ch, 0, victim, TO_VICT);
					break;
				default:
					act("$n tells you to get lost!", FALSE, ch, 0, victim, TO_VICT);
					break;
				}			/* end switch */

				act("$n insults $N.", TRUE, ch, 0, victim, TO_NOTVICT);
			} else {			/* ch == victim */
				ch->Send("You feel insulted.\r\n");
			}
		}
	} else
		ch->Send("I'm sure you don't want to insult *everybody*...\r\n");
	release_buffer(arg);
}


char *fread_action(FILE * fl, int nr) {
	char	*buf = get_buffer(MAX_STRING_LENGTH),
			*rslt;

	fgets(buf, MAX_STRING_LENGTH, fl);
	if (feof(fl)) {
		log("SYSERR: fread_action - unexpected EOF near action #%d", nr);
		exit(1);
	}
	if (*buf == '#') {
		release_buffer(buf);
		return (NULL);
	} else {
		*(buf + strlen(buf) - 1) = '\0';
		rslt = str_dup(buf);
		release_buffer(buf);
		return (rslt);
	}
}


Social::Social(void)
// : act_nr(0), command(NULL), sort_as(NULL), hide(false),
//		min_victim_position(POS_DEAD), min_char_position(POS_DEAD), min_level_char(0),
//		char_no_arg(NULL), others_no_arg(NULL), char_found(NULL), others_found(NULL),
//		vict_found(NULL), char_body_found(NULL), others_body_found(NULL),
//		vict_body_found(NULL), not_found(NULL), char_auto(NULL), others_auto(NULL),
//		char_obj_found(NULL), others_obj_found(NULL)
{
memset(this, 0, sizeof(Social));
}


Map<VNum, Social *>	Social::Messages;
Vector<Social>		Social::internal;


void Social::Boot(void) {
	FILE *fl;
	char *	next_soc = get_buffer(MAX_STRING_LENGTH),
		 *	sorted = get_buffer(MAX_INPUT_LENGTH);

	if (!(fl = fopen(SOCMESS_FILE, "r"))) {	//	open social file
		log("SYSERR: Can't open socials file '%s'", SOCMESS_FILE);
		exit(1);
	}

	//	Now read 'em
	int		nr = 0;
	int		hide, min_char_pos, min_pos, min_lvl;
	Social	social;

	for (;;) {
		fscanf(fl, " %s ", next_soc);
		if (*next_soc == '$') break;
		if (fscanf(fl, " %s %d %d %d %d \n", sorted, &hide, &min_char_pos, &min_pos, &min_lvl) != 5) {
			log("SYSERR: Format error in social file near social '%s'", next_soc);
			exit(1);
		}
		//	read the stuff
		social.command = str_dup(next_soc+1);
		social.sort_as = str_dup(social.command);
		social.hide = hide;
		social.char_position = (Flags)min_char_pos;
		social.victim_position = (Flags)min_pos;
		social.min_level_char = min_lvl;

		social.char_no_arg = fread_action(fl, nr);
		social.others_no_arg = fread_action(fl, nr);
		social.char_found = fread_action(fl, nr);

		social.others_found = fread_action(fl, nr);
		social.vict_found = fread_action(fl, nr);
		social.not_found = fread_action(fl, nr);
		social.char_auto = fread_action(fl, nr);
		social.others_auto = fread_action(fl, nr);
		social.char_body_found = fread_action(fl, nr);
		social.others_body_found = fread_action(fl, nr);
		social.vict_body_found = fread_action(fl, nr);
		social.char_obj_found = fread_action(fl, nr);
		social.others_obj_found = fread_action(fl, nr);

		Social::internal.SetSize(nr + 1);
		Social::internal[nr++] = social;
	}

	//	close file
	fclose(fl);
	release_buffer(next_soc);
	release_buffer(sorted);
}


/* this function adds in the loaded socials and assigns them a command # */
void create_command_list(void)  {
	int i, j, k;
	Social temp;

	/* free up old command list */
	if (complete_cmd_info) free(complete_cmd_info);
	complete_cmd_info = NULL;

	/* re check the sort on the socials */
	for (j = 0; j < Social::internal.Count(); j++) {
		k = j;
		for (i = j + 1; i <= Social::internal.Count(); i++)
			if (str_cmp(Social::internal[i].sort_as, Social::internal[k].sort_as) < 0)
				k = i;
		if (j != k) {
			temp = Social::internal[j];
			Social::internal[j] = Social::internal[k];
			Social::internal[k] = temp;
		}
	}

	/* count the commands in the command list */
	i = 0;
	while(*cmd_info[i].command != '\n') ++i;
	++i;

	CREATE(complete_cmd_info, struct command_info, Social::internal.Count() + i + 2);

	/* this loop sorts the socials and commands together into one big list */
	i = 0;
	j = 0;
	k = 0;
	while ((*cmd_info[i].command != '\n') || (j <= Social::internal.Count()))  {
		if ((i < NUM_RESERVED_CMDS) || (j > Social::internal.Count()) ||
				(str_cmp(cmd_info[i].sort_as, Social::internal[j].sort_as) < 1))
			complete_cmd_info[k++] = cmd_info[i++];
		else {
			Social::Messages[k] = &Social::internal[j];
			Social::internal[j].act_nr				= k;
			complete_cmd_info[k].command			= Social::internal[j].command;
			complete_cmd_info[k].sort_as			= Social::internal[j].sort_as;
			complete_cmd_info[k].position			= Social::internal[j].char_position;
			complete_cmd_info[k].command_pointer	= do_action;
			complete_cmd_info[k].minimum_level    	= Social::internal[j++].min_level_char;
			complete_cmd_info[k++].subcmd			= 0;
		}
	}
	complete_cmd_info[k].command			= str_dup("\n");
	complete_cmd_info[k].sort_as			= str_dup("zzzzzzz");
	complete_cmd_info[k].position			= P_ANY;
	complete_cmd_info[k].command_pointer	= NULL;
	complete_cmd_info[k].minimum_level		= 0;
	complete_cmd_info[k].subcmd				= 0;

	log("Command info rebuilt, %d total commands.", k);
}


ACMD(do_highfive)
{
	Character *vict;
	Descriptor *i;

	if (argument) {
		one_argument(argument, buf);
		if ((vict = get_char_vis(ch,buf, FIND_CHAR_ROOM))) {
			if (IS_STAFF(ch) && IS_STAFF(vict) && !PLR_FLAGGED(ch, PLR_NOWIZLIST) &&
		!PLR_FLAGGED(vict, PLR_NOWIZLIST)) {
	sprintf(buf1, "Lightning illuminates the skies as $n and %s high five.",
		vict->Name());
	strcpy(buf2,
		"Lightning illuminates the skies as $n and someone high five.");

	START_ITER(iter, i, Descriptor *, descriptor_list) {
		if (!i->connected && i->character &&
				!PLR_FLAGGED(i->character, PLR_WRITING) &&
				!ROOM_FLAGGED(i->character->InRoom(), ROOM_SOUNDPROOF) &&
				(i->character != ch) && (i->character != vict)) {
			if( CAN_SEE(i->character, vict))
				act(buf1, FALSE, ch, 0, i->character, TO_VICT | TO_SLEEP);
			else
				act(buf2, FALSE, ch, 0, i->character, TO_VICT | TO_SLEEP);
		}
	}
	if (ch == vict)
		act("Lightning illuminates the skies as you high five yourself.",
				FALSE, ch, 0, 0, TO_CHAR);
	else {
		act("Lightning illuminates the skies as you and $N high five.",
				FALSE, ch, 0, vict, TO_CHAR);
		act("Lightning illuminates the skies as $n gives you a high five.",
				FALSE, ch, 0, vict, TO_VICT | TO_SLEEP);
	}
			} else {
	if (ch == vict) {
		act("Don't you have any friends???", FALSE, ch, 0, 0, TO_CHAR);
	} else {
		act("$n gives you a high five.", TRUE, ch, 0, vict, TO_VICT);
		act("You give a hearty high five to $N.", TRUE, ch, 0, vict, TO_CHAR);
		act("$n and $N do a high five.", TRUE, ch, 0, vict, TO_NOTVICT);
	}
			}
		} else
			ch->Send("I don't see anyone here like that.\r\n");

	}
}
