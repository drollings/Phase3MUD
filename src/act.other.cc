/* ************************************************************************
*   File: act.other.c                                   Part of CircleMUD *
*  Usage: Miscellaneous player-level commands                             *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */



#include "structs.h"
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
#include "help.h"
#include "db.h"
#include "house.h"
#include "affects.h"
#include "constants.h"
#include "spells.h"

#include <sys/stat.h>

/* extern variables */
extern int pt_allowed;
extern int nameserver_is_slow;
extern int max_filesize;
extern int free_rent;

/* extern procedures */
SPECIAL(shop_keeper);
void Crash_rentsave(Character * ch);
ACMD(do_gen_comm);
void list_skills(Character *ch, bool show_known_only = true, Flags type = 0);
void perform_immort_vis(Character *ch);
int command_mtrigger(Character *actor, char *cmd, char *argument);
int command_otrigger(Character *actor, char *cmd, char *argument);
int command_wtrigger(Character *actor, char *cmd, char *argument);
int sit_otrigger(Object *obj, Character *actor);
void quit_otrigger(Object *obj);
void quit_otrigger(LList<Object *> &objList);
extern int can_drop_obj(Character *ch, Object *obj, SInt8 mode, const char *sname);

/* prototypes */
int perform_group(Character *ch, Character *vict);
void print_group(Character *ch);

ACMD(do_quit);
ACMD(do_save);
ACMD(do_not_here);
ACMD(do_practice);
ACMD(do_visible);
ACMD(do_title);
ACMD(do_group);
ACMD(do_ungroup);
ACMD(do_report);
ACMD(do_wimpy);
ACMD(do_display);
ACMD(do_gen_write);
ACMD(do_gen_tog);
ACMD(do_afk);
ACMD(do_watch);
ACMD(do_promote);
ACMD(do_prompt);
ACMD(do_nonewbie);



ACMD(do_quit) {
	// VNum save_room;	// This is covered in Character::Save() now.
	Descriptor *d;

	if (IS_NPC(ch) || !ch->desc)
		return;

	if (str_cmp(command, "quit") && !IS_STAFF(ch))
		ch->Send("You have to type quit - no less, to quit!\r\n");
	else if (!free_rent && (!arg || !*arg || str_cmp(arg, "yes")))
		ch->Send("By quitting, you will drop all of your equipment.	Type \"quit yes\" if you\r\n"
		 "are sure you want to quit, or go find an inn to rent at to save your equipment.\r\n");
	else if (FIGHTING(ch))
		ch->Send("No way!  You're fighting for your life!\r\n");
	else if (GET_POS(ch) < POS_STUNNED) {
		ch->Send("You die before your time...\r\n");
		ch->Die(NULL);
	} else {
		if (!GET_INVIS_LEV(ch))
			act("$n has left the game.", TRUE, ch, 0, 0, TO_ROOM);
		mudlogf(NRM, ch, TRUE,  "%s has quit the game.", ch->Name());
		ch->Send("Farewell, friend.  Return again soon!\r\n");
		if (PLR_FLAGGED(ch, PLR_AFK)) {
			REMOVE_BIT(PLR_FLAGS(ch), PLR_AFK);
		}

		//	kill off all sockets connected to the same player as the one who is
		//	trying to quit.  Helps to maintain sanity as well as prevent duping.
		START_ITER(iter, d, Descriptor *, descriptor_list) {
			if (d == ch->desc)
				continue;
			if (d->character && (GET_IDNUM(d->character) == GET_IDNUM(ch)))
				STATE(d) = CON_DISCONNECT;
		}
		GET_LOADROOM(ch) = IN_ROOM(ch);

		quit_otrigger(ch->contents);

		for (SInt32 i = 0; i < NUM_WEARS; i++)
			if (GET_EQ(ch, i))	quit_otrigger(GET_EQ(ch, i));

		if (free_rent)		Crash_rentsave(ch);
		else				ch->Send("&cRDropping all your remaining inventory...&c0\r\n");

		ch->Extract();		//	Char is saved in extract char

		/* If someone is quitting in their house, let them load back here */

		// No longer necessary.  Characters are saved in the room they quit in,
		// difference being that houses do not have the one hour timeout. -DH

		// if (ROOM_FLAGGED(save_room, ROOM_HOUSE))
		// 	ch->Save(save_room);
	}
}



ACMD(do_save) {
	if (IS_NPC(ch) || !ch->desc)
		return;

	if (command) {
		act("Saving $n.", FALSE, ch, 0, 0, TO_CHAR);
	}
	ch->Save(ch->AbsoluteRoom());
	Crash_crashsave(ch);
	if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_HOUSE_CRASH))
		House::CrashSave(world[IN_ROOM(ch)].Virtual());
}


/* generic function for commands which are normally overridden by
   special procedures - i.e., shop commands, mail commands, etc. */
ACMD(do_not_here) {
	ch->Send("Sorry, but you cannot do that here!\r\n");
}


void check_thief(Character * ch, Character * victim)
{
	extern int pt_allowed;

	if (pt_allowed || (ch == victim) || NO_STAFF_HASSLE(ch))
		return;

	if (ch->GetRelation(victim) == RELATION_ENEMY)
		return;

	if (!IS_NPC(ch)) {
		SET_BIT(PLR_FLAGS(ch), PLR_THIEF);
		ch->Send("If you want to be a thief, so be it...\r\n");
		log("PC Thief bit set on %s for stealing from %s at %s.",
			ch->Name(), victim->Name(), world[IN_ROOM(victim)].Name());
	}
}


ACMD(do_steal)
{
	Character *vict;
	Object *obj;
	char vict_name[MAX_INPUT_LENGTH], obj_name[MAX_INPUT_LENGTH];
	int roll, gold, eq_pos, pcsteal = 0, ohoh = 0;
	extern int pt_allowed;

	argument = one_argument(argument, obj_name);
	one_argument(argument, vict_name);

	if (!(vict = get_char_vis(ch, vict_name, FIND_CHAR_ROOM))) {
		ch->Send("Steal what from who?\r\n");
		return;
	} else if (vict == ch) {
		ch->Send("Come on now, that's rather stupid!\r\n");
		return;
	}

	if (!Combat::Valid(ch, vict))
		return;

	/* Piano Added a peaceful rooms check */
	if (ch != vict && ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
		ch->Send("This room just has such a peaceful, easy feeling...\r\n");
		return;
	}

	/* Piano Added a check for player Thiefs and to set a Thief flag	 */
	if (!pt_allowed)
		check_thief(ch, vict);

	roll = CHAR_SKILL(ch).RollSkill(ch, SKILL_STEAL);
	CHAR_SKILL(ch).RollToLearn(ch, SKILL_STEAL, roll);

	if (GET_POS(vict) < POS_SLEEPING)
		roll = 1;		/* ALWAYS SUCCESS */

	if (!pt_allowed && !IS_NPC(vict))
		pcsteal = 1;

	/* NO NO With Imp's and Shopkeepers, and if player thieving is not allowed */
	if (IS_STAFF(vict) || pcsteal || GET_MOB_SPEC(vict) == shop_keeper)
		roll = -1;		/* Failure */

	if (str_cmp(obj_name, "coins") && str_cmp(obj_name, "gold")) {

		if (!(obj = get_obj_in_list_vis(vict, obj_name, vict->contents))) {

			for (eq_pos = 0; eq_pos < NUM_WEARS; eq_pos++) {
				if (GET_EQ(vict, eq_pos) && (isname(obj_name, GET_EQ(vict, eq_pos)->Name())) &&
					CAN_SEE_OBJ(ch, GET_EQ(vict, eq_pos))) {
					obj = GET_EQ(vict, eq_pos);
					break;
				}

			if (!obj) {
				act("$E hasn't got that item.", FALSE, ch, 0, vict, TO_CHAR);
				return;
			} else {			/* It is equipment */
				if ((GET_POS(vict) > POS_STUNNED)) {
					ch->Send("Steal the equipment now?  Impossible!\r\n");
					return;
				} else {
					act("You unequip $p and steal it.", FALSE, ch, obj, 0, TO_CHAR);
					act("$n steals $p from $N.", FALSE, ch, obj, vict, TO_NOTVICT);
					unequip_char(vict, eq_pos)->ToChar(ch);
				}
			}
		}
	} else {			/* obj found in inventory */

			if (!OBJ_FLAGGED(obj, ITEM_PROXIMITY)) {

				roll -= (GET_OBJ_WEIGHT(obj)) / 5;	/* Make heavy harder */

				if (AWAKE(vict) && (roll < 0)) {
		ohoh = TRUE;
		act("Oops..", FALSE, ch, 0, 0, TO_CHAR);
		act("$n tried to steal something from you!", FALSE, ch, 0, vict, TO_VICT);
		act("$n tries to steal something from $N.", TRUE, ch, 0, vict, TO_NOTVICT);
				} else {			/* Steal the item */
		if ((IS_CARRYING_N(ch) + 1 < CAN_CARRY_N(ch))) {
			if ((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) < CAN_CARRY_W(ch)) {
				obj->FromChar();
				obj->ToChar(ch);
				act("Got it!", FALSE, ch, 0, 0, TO_CHAR);
			}
		} else
			ch->Send("You cannot carry that much.\r\n");
				}
			} else {
				act("$p sends a jolt through your fingers!", FALSE, ch, obj, 0, TO_CHAR);
				roll += 2;
				roll += (GET_OBJ_WEIGHT(obj) / 10);	/* Make heavy harder */
				if (AWAKE(vict) && (roll < 0))
		ohoh = TRUE;
			}
		}
	} else {			/* Steal some coins */
		if (AWAKE(vict) && (roll < 0)) {
			ohoh = TRUE;
			act("Oops..", FALSE, ch, 0, 0, TO_CHAR);
			act("You discover that $n has $s hands in your wallet.", FALSE, ch, 0, vict, TO_VICT);
			act("$n tries to steal gold from $N.", TRUE, ch, 0, vict, TO_NOTVICT);
		} else {
			/* Steal some gold coins */
			gold = (int) ((GET_GOLD(vict) * Number(1, 10)) / 100);
			gold = MIN(1782, gold);
			if (gold > 0) {
				GET_GOLD(ch) += gold;
				GET_GOLD(vict) -= gold;
				if (gold > 1)	ch->Sendf("Bingo!	You got %d gold coins.\r\n", gold);
				else 			ch->Send("You manage to swipe a solitary gold coin.\r\n");
			} else {
				ch->Send("You couldn't get any gold...\r\n");
			}
		}
	}

	if (ohoh && IS_NPC(vict) && AWAKE(vict))
		vict->Hit(ch);
}


ACMD(do_practice)
{
	SInt8	known = TRUE, 
			parameter, 
			badarg = FALSE;
	Flags 	types,
			normtypes;

	types = normtypes = (Skill::GENERAL | Skill::MOVEMENT | Skill::WEAPON | Skill::COMBAT | 
					Skill::RANGEDCOMBAT | Skill::STEALTH | Skill::ENGINEERING);

	skip_spaces(argument);

	if (*argument) {
		half_chop(argument, arg, buf);

		while (*arg != '\0') {
			types = 0;
			if (is_abbrev(arg, "all")) {
				known = false;
			} else {
				parameter = search_block(arg, Skill::namedtypes, FALSE);
				if (parameter == -1) {
					badarg = true;
					break;
				} else {
					SET_BIT(types, (1 << parameter));
				}
			}

			half_chop(buf, arg, buf);
		}

		if (badarg) {
			ch->Sendf("Usage:\r\n"
			"  practice [all | skill | ordination | sphere | root | method | weapons]\r\n"
			"    or\r\n"
			"  practice [skill name] if you are in a guild.\r\n");
			return;
		}
	}

	if (!types)		types = normtypes;

	if (!GET_PRACTICES(ch))
		ch->Send("&cGYou have no training sessions remaining.&c0\r\n");
	else
		ch->Sendf("&cGYou have &cY%d&cG training session%s remaining.&c0\r\n",
						GET_PRACTICES(ch), (GET_PRACTICES(ch) == 1 ? "" : "s"));

	ch->Send("&cGYou know of the following skills:\r\n\n"
	"&cWSkill                     Base           Known    Chance  Proficiency\r\n"
	"&cL----------------------------------------------------------------------&c0\r\n");
	list_skills(ch, known, types);
}


ACMD(do_spells)
{
	SInt8 showall = FALSE;
	Flags types = (subcmd ? Skill::PRAYER : Skill::SPELL);

	one_argument(argument, arg);

	if ((*arg) && (!str_cmp(arg, "all")))
		showall = TRUE;

	ch->Sendf("&cGYou know of the following %ss:\r\n\n"
	    "&cWSpell                     Base                    Mod  Proficiency\r\n"
		"&cL----------------------------------------------------------------------&c0\r\n",
		(subcmd ? "prayer" : "spells"));
	list_skills(ch, showall, types);
}


ACMD(do_bandage)
{
	Character *vict = NULL;
	SInt16 diceroll;
	SInt8 chance;

	one_argument(argument, buf);

	if (!*buf) {
		ch->Send("Bandage whom?\r\n");
		return;
	}

	if (!(vict = get_char_vis(ch, buf, FIND_CHAR_ROOM))) {
		ch->Send(NOPERSON);
		return;
	}

	if (GET_HIT(vict) > 0) {
		ch->Send("Bandaging won't do much for them right now.\r\n");
		return;
	}

	chance = SKILLCHANCE(ch, SKILL_FIRST_AID, NULL);
	chance -= (1 - GET_HIT(vict)) / 2;

	diceroll = CHAR_SKILL(ch).RollSkill(ch, SKILL_FIRST_AID, chance);
	CHAR_SKILL(ch).RollToLearn(ch, SKILL_FIRST_AID, diceroll);

	if (diceroll < 0) {
		ch->Send("You fail to staunch the flow of blood.\r\n");
	} else {
		act("You bandage some of $N's wounds.", FALSE, ch, 0, vict, TO_CHAR);
		act("$N bandages $s wounds.", TRUE, vict, 0, ch, TO_NOTVICT);
		AlterHit(vict, -5);
		vict->UpdatePos();
	if (GET_POS(vict) < POS_RESTING)
			GET_POS(vict) = POS_RESTING;
	}

	WAIT_STATE(ch, PULSE_VIOLENCE * 2);

}


ACMD(do_skillinfo)
{
	SInt16 skillnum;
	int i, j, chance;
	Flags positions = 0;
	char *ptr;

	extern const char *skillbases[];
	extern UInt8 skill_mod_to_pts(SInt16 skillnum, SInt8 mod);
	extern int avg_base_chance(Character *ch, SInt16 skillnum);

	// Replace all underscores with spaces
	while ((ptr = strchr(argument, '\'')) != NULL)
		*ptr = ' ';

	skip_spaces(argument);
	skillnum = Skill::Number(argument);

	if (skillnum < 1) {
		ch->Send("Not a valid skill.\r\n");
		return;
	}

	if (GET_TRUST(ch) > 0) {
		sprintf(buf, "&cc&c+Skill #&cC%d&c- : '&cG%s&c-'\r\n", skillnum, Skill::Name(skillnum));

		// sprintf(buf + strlen(buf), "StartMod: &cC%d&c-	Maxincr: &cC%d&c-	",
		//				 skill_info[skillnum]->startmod, skill_info[skillnum]->maxincr);
		positions = (unsigned long) Skills[skillnum]->min_position;
		sprintbit(Skills[skillnum]->target, Skill::targeting, buf1);
		sprintbit(positions, position_types, buf2);
		sprintf(buf + strlen(buf), "Targeting: &cY%s&c-  Violent: &cC%d&c-  Position: &cY%s&c-\r\n",
						buf1, Skills[skillnum]->violent, buf2);
	} else {
		sprintf(buf, "&cc&c+Skill : '&cG%s&c-'  ", Skill::Name(skillnum));
	}

	sprintbit(Skills[skillnum]->stats, skillbases, buf1);
	sprintf(buf + strlen(buf), "Base: &cY%s&c-\r\n", buf1);

	for (i = 0; i < Skills[skillnum]->prerequisites.Count(); ++i) {
		sprintf(buf + strlen(buf),
			 "Prerequisite : &cY%-10s&c-    Points   : &cC%d&c-\r\n",
			 Skill::Name(Skills[skillnum]->prerequisites[i].skillnum),
			 Skills[skillnum]->prerequisites[i].chance);
	}

	for (i = 0; i < Skills[skillnum]->anrequisites.Count(); ++i) {
		sprintf(buf + strlen(buf),
			 "Anrequisite  : &cY%-10s&c-    Points   : &cC%d&c-\r\n",
			 Skill::Name(Skills[skillnum]->anrequisites[i].skillnum),
			 Skills[skillnum]->anrequisites[i].chance);
	}

	for (i = 0; i < Skills[skillnum]->defaults.Count(); ++i) {
		sprintf(buf + strlen(buf),
			 "Default      : &cY%-10s&c-    Modifier : &cC%d&c-\r\n",
			 Skill::Name(Skills[skillnum]->defaults[i].skillnum),
			 Skills[skillnum]->defaults[i].chance);
	}

	if (GET_TRUST(ch) > 0) {
		sprintf(buf + strlen(buf), "Delay: &cC%d&c-  Lag: &cC%d&c-\r\n",
						Skills[skillnum]->delay, Skills[skillnum]->lag);
	}

	if (LEARNABLE_SKILL(skillnum)) {
		strcat(buf, "Cost  Modifier  Chance\r\n");
		for (i = Skills[skillnum]->startmod, j = 0;
			 j < 8;
			 ++j) {

			chance = avg_base_chance(ch, skillnum) + i + j;

			sprintf(buf + strlen(buf), "  &cC%2d&c-  &cC%8d&c-  &cC%5d&c-\r\n",
						skill_mod_to_pts(skillnum, i + j), i + j, chance);
		}
	}

	strcat(buf, "&c0");
	ch->Send(buf);
}


ACMD(do_visible) {
	if (IS_STAFF(ch)) {
		perform_immort_vis(ch);
		return;
	}

	if ((!AFF_FLAGGED(ch, AffectBit::Invisible)) &&
		 (!AFF_FLAGGED(ch, AffectBit::Camoflauge)) &&
		 !GET_INVIS_LEV(ch)) {
		ch->Send("You are already visible.\r\n");
		return;
	}

	if AFF_FLAGGED(ch, AffectBit::Invisible) {
		ch->Send("You break the spell of invisibility.\r\n");
	}
	if AFF_FLAGGED(ch, AffectBit::Camoflauge) {
		ch->Send("You come out of camoflauge.\r\n");
	}

	ch->Appear();
}



ACMD(do_title) {
	char *buf = get_buffer(MAX_INPUT_LENGTH);

	skip_spaces(argument);
	strcpy(buf, argument);
	delete_doubledollar(buf);

	if (IS_NPC(ch))
		ch->Send("Your title is fine... go away.\r\n");
	else if (PLR_FLAGGED(ch, PLR_NOTITLE))
		ch->Send("You can't title yourself -- you shouldn't have abused it!\r\n");
	else if (strstr(argument, "(") || strstr(argument, ")"))
		ch->Send("Titles can't contain the ( or ) characters.\r\n");
	else if (strlen(argument) > MAX_TITLE_LENGTH) {
		ch->Sendf("Sorry, titles can't be longer than %d characters.\r\n", MAX_TITLE_LENGTH);
	} else {
		ch->SetTitle(buf);
		act("Okay, you're now $n $T.", FALSE, ch, 0, GET_TITLE(ch), TO_CHAR);
	}
	release_buffer(buf);
}


int perform_group(Character *ch, Character *vict)
{
	if (AFF_FLAGGED(vict, AffectBit::Group) || !CAN_SEE(ch, vict))
		return 0;

	SET_BIT(AFF_FLAGS(vict), AffectBit::Group);
	if (ch != vict)
		act("$N is now a member of your group.", FALSE, ch, 0, vict, TO_CHAR);
	act("You are now a member of $n's group.", FALSE, ch, 0, vict, TO_VICT);
	act("$N is now a member of $n's group.", FALSE, ch, 0, vict, TO_NOTVICT);
	return 1;
}


void print_group(Character *ch)
{
	Character *k, *follower;

	if (!AFF_FLAGGED(ch, AffectBit::Group))
		ch->Send("But you are not the member of a group!\r\n");
	else {
		ch->Send("Your group consists of:\r\n");

		k = (ch->master ? ch->master : ch);

		if (AFF_FLAGGED(k, AffectBit::Group)) {
			sprintf(buf, "    [%3dH %3dM %3dV] $N  Position: %s (Head of group)",
				GET_HIT(k), GET_MANA(k), GET_MOVE(k), group_positions[GROUP_POS(k)]);
			act(buf, FALSE, ch, 0, k, TO_CHAR);
		}

		START_ITER(iter, follower, Character *, k->followers) {
			if (!AFF_FLAGGED(follower, AffectBit::Group))
				continue;

			sprintf(buf, "    [%3dH %3dM %3dV] $N  Position: %s",
				GET_HIT(follower), GET_MANA(follower), GET_MOVE(follower),
                group_positions[GROUP_POS(follower)]);
			act(buf, FALSE, ch, 0, follower, TO_CHAR);
		}
	}
}



ACMD(do_group)
{
	Character *vict, *follower;
	int found = 0;
	SInt8 position_move = 0;

	one_argument(argument, buf);

	if (!*buf) {
		print_group(ch);
		return;
	}

	if (is_abbrev(buf, "front")) {
		position_move = 1;
	} else if (is_abbrev(buf, "back")) {
		position_move = 2;
	}

	if (position_move) {
		if (!AFF_FLAGGED(ch, AffectBit::Group)) {
			ch->Send("But you are not a member of any group!\r\n");
			return;
		}
		if (Affect::AffectedBy(ch, SPELL_BIND)) {
			ch->Send("Sinewy cords of magical force hold you fast!  You can't break free!\r\n");
			return;
		}
		if (FIGHTING(ch)) {
			ch->Send("No way!	You're fighting for your life!\r\n");
			return;
		}
	}

	switch (position_move) {
	case 1:
		if (GROUP_POS(ch)) {
			act("You move to the front of the group.", FALSE, ch, 0, 0, TO_CHAR);
			act("$n moves to the front of the group.", FALSE, ch, 0, 0, TO_ROOM);
			GROUP_POS(ch) = 0;
		} else {
			ch->Send("You are already at the front of the group!\r\n");
		}
		return;
	case 2:
		if (!GROUP_POS(ch)) {
			act("You move to the back of the group.", FALSE, ch, 0, 0, TO_CHAR);
			act("$n moves to the back of the group.", FALSE, ch, 0, 0, TO_ROOM);
			GROUP_POS(ch) = 1;
		} else {
			ch->Send("You are already at the back of the group!\r\n");
		}
		return;
	}

	if (ch->master) {
		act("You can not enroll group members without being head of a group.", FALSE, ch, 0, 0, TO_CHAR);
		return;
	}

	if (!str_cmp(buf, "all")) {
		perform_group(ch, ch);
		START_ITER(iter, follower, Character *, ch->followers)
			found += perform_group(ch, follower);
		if (!found)
			ch->Send("Everyone following you is already in your group.\r\n");
		return;
	}

	if (!(vict = get_char_vis(ch, buf, FIND_CHAR_ROOM)))
		ch->Send(NOPERSON);
	else if ((vict->master != ch) && (vict != ch))
		act("$N must follow you to enter your group.", FALSE, ch, 0, vict, TO_CHAR);
	else {
		if (!AFF_FLAGGED(vict, AffectBit::Group))
			perform_group(ch, vict);
		else {
			if (ch != vict)
	act("$N is no longer a member of your group.", FALSE, ch, 0, vict, TO_CHAR);
			act("You have been kicked out of $n's group!", FALSE, ch, 0, vict, TO_VICT);
			act("$N has been kicked out of $n's group!", FALSE, ch, 0, vict, TO_NOTVICT);
			REMOVE_BIT(AFF_FLAGS(vict), AffectBit::Group);
		}
	}
}



ACMD(do_ungroup)
{
	Character *tch, *follower;
	void stop_follower(Character * ch);

	one_argument(argument, buf);

	if (!*buf) {
		if (ch->master || !(AFF_FLAGGED(ch, AffectBit::Group))) {
			ch->Send("But you lead no group!\r\n");
			return;
		}
		sprintf(buf2, "%s has disbanded the group.\r\n", ch->Name());
		START_ITER(iter, follower, Character *, ch->followers) {
			if (AFF_FLAGGED(follower, AffectBit::Group)) {
				REMOVE_BIT(AFF_FLAGS(follower), AffectBit::Group);
				follower->Send(buf2);
				// if (!AFF_FLAGGED(f->follower, AffectBit::Charm))
				//	 stop_follower(f->follower);
			}
		}

		REMOVE_BIT(AFF_FLAGS(ch), AffectBit::Group);
		ch->Send("You disband the group.\r\n");
		return;
	}
	if (!(tch = get_char_vis(ch, buf, FIND_CHAR_ROOM))) {
		ch->Send("There is no such person!\r\n");
		return;
	}
	if (tch->master != ch) {
		ch->Send("That person is not following you!\r\n");
		return;
	}

	if (!AFF_FLAGGED(tch, AffectBit::Group)) {
		ch->Send("That person isn't in your group.\r\n");
		return;
	}

	REMOVE_BIT(AFF_FLAGS(tch), AffectBit::Group);

	act("$N is no longer a member of your group.", FALSE, ch, 0, tch, TO_CHAR);
	act("You have been kicked out of $n's group!", FALSE, ch, 0, tch, TO_VICT);
	act("$N has been kicked out of $n's group!", FALSE, ch, 0, tch, TO_NOTVICT);

	if (!AFF_FLAGGED(tch, AffectBit::Charm))
		tch->StopFollower();
}


ACMD(do_report) {
	Character *k, *follower;
	char *buf;

	if (!AFF_FLAGGED(ch, AffectBit::Group)) {
		ch->Send("But you are not a member of any group!\r\n");
		return;
	}

	buf = get_buffer(128);

	sprintf(buf, "%s reports: %d/%dH, %d/%dV\r\n", ch->Name(), GET_HIT(ch), GET_MAX_HIT(ch),
			GET_MOVE(ch), GET_MAX_MOVE(ch));

	CAP(buf);

	k = (ch->master ? ch->master : ch);

	START_ITER(iter, follower, Character *, k->followers)
		if (AFF_FLAGGED(follower, AffectBit::Group) && (follower != ch))
			follower->Send(buf);
	if (k != ch)
		k->Send(buf);
	ch->Send("You report to the group.\r\n");
	release_buffer(buf);
}


ACMD(do_use)
{
	Object *mag_item;
	int equipped = 1, found = FALSE;

	half_chop(argument, arg, buf);
	if (!*arg) {
		sprintf(buf2, "What do you want to %s?\r\n", CMD_NAME);
		ch->Send(buf2);
		return;
	}

	mag_item = GET_EQ(ch, WEAR_HAND_R);
	if (mag_item)
		if (isname(arg, mag_item->Keywords()))
			found = TRUE;
	if (!found) {
		mag_item = GET_EQ(ch, WEAR_HAND_L);
		if (mag_item)
			if (isname(arg, mag_item->Keywords()))
				found = TRUE;
	}

	if (!found) {
		switch (subcmd) {
		case SCMD_RECITE:
		case SCMD_QUAFF:
			equipped = 0;
			if (!(mag_item = get_obj_in_list_vis(ch, arg, ch->contents))) {
				sprintf(buf2, "You don't seem to have %s %s.\r\n", AN(arg), arg);
				ch->Send(buf2);
				return;
			}
			break;
		case SCMD_USE:
			sprintf(buf2, "You don't seem to be holding %s %s.\r\n", AN(arg), arg);
			ch->Send(buf2);
			return;
			break;
		default:
			log("SYSERR: Unknown subcmd %d passed to do_use", subcmd);
			return;
			break;
		}
	}
	switch (subcmd) {
	case SCMD_QUAFF:
		if (GET_OBJ_TYPE(mag_item) != ITEM_POTION) {
			ch->Send("You can only quaff potions.\r\n");
			return;
		}
		break;
	case SCMD_RECITE:
		if (GET_OBJ_TYPE(mag_item) != ITEM_SCROLL) {
			ch->Send("You can only recite scrolls.\r\n");
			return;
		}
		break;
	case SCMD_USE:
		if ((GET_OBJ_TYPE(mag_item) != ITEM_WAND) &&
			(GET_OBJ_TYPE(mag_item) != ITEM_STAFF)) {
			if (!command_wtrigger(ch, "pull", arg) &&
					!command_mtrigger(ch, "pull", arg) &&
					!command_otrigger(ch, "pull", arg)) {
				ch->Send("You can't seem to figure out how to use it.\r\n");
				return;
			}
		}
		break;
	}

	mag_objectmagic(ch, mag_item, buf);
}


ACMD(do_wimpy) {
	int wimp_lev;
	char * arg;

	if (IS_NPC(ch))
		return;

	arg = get_buffer(MAX_INPUT_LENGTH);

	one_argument(argument, arg);

	if (!*arg) {
		if (GET_WIMP_LEV(ch))
			ch->Sendf("Your current wimp level is %d hit points.\r\n", GET_WIMP_LEV(ch));
		else
			ch->Send("At the moment, you're not a wimp.	(sure, sure...)\r\n");
	} else if (isdigit(*arg)) {
		if ((wimp_lev = atoi(arg))) {
			if (wimp_lev < 0)
				ch->Send("Heh, heh, heh.. we are jolly funny today, eh?\r\n");
			else if (wimp_lev > GET_MAX_HIT(ch))
				ch->Send("That doesn't make much sense, now does it?\r\n");
			else if (wimp_lev > (GET_MAX_HIT(ch) / 2))
				ch->Send("You can't set your wimp level above half your hit points.\r\n");
			else {
				ch->Sendf("Okay, you'll wimp out if you drop below %d hit points.\r\n", wimp_lev);
				GET_WIMP_LEV(ch) = wimp_lev;
			}
		} else {
			ch->Send("Okay, you'll now tough out fights to the bitter end.\r\n");
			GET_WIMP_LEV(ch) = 0;
		}
	} else
		ch->Send("Specify at how many hit points you want to wimp out at.	(0 to disable)\r\n");
	release_buffer(arg);
}


ACMD(do_display)
{
	char arg[MAX_INPUT_LENGTH];
	int i, x;

	const char *def_prompts[][2] = {
		{ "tlo"				, "$I$chH $cmM $cvV&c0 $o$t&c0-> " },
		{ "Stock Circle"	, "&cg$hH $mM $vV&c0> " },
		{ "Standard"		, "&cR$ph&crhp &cC$pm&ccmp &cG$pv&cgmv&c0> " },
		{ "\n"				, "\n" }
	};

	one_argument(argument, arg);

	if (!arg || !*arg) {
		ch->Send("The following pre-set prompts are available...\r\n");
		for (i = 0; *def_prompts[i][0] != '\n'; i++) {
			sprintf(buf, "	%d. %-25s	%s\r\n", i, def_prompts[i][0], def_prompts[i][1]);
			ch->Send(buf);
		}
		ch->Send("Usage: display <number>\r\n"
				 "To create your own prompt, use _prompt <str>_.\r\n");
		return;
	} else if (!isdigit(*arg)) {
		ch->Send("Usage: display <number>\r\n");
		ch->Send("Type 'display' without arguments for a list of preset prompts.\r\n");
		return;
	} else if (!isdigit(*arg)) {
		ch->Send("Usage: display <number>\r\n");
		ch->Send("Type 'display' without arguments for a list of preset prompts.\r\n");
		return;
	}

	i = atoi(arg);

	if (i < 0) {
		ch->Send("The number cannot be negative.\r\n");
			return;
		}

	for (x = 0; *def_prompts[x][0] != '\n'; x++);

	if (i >= x) {
		sprintf(buf, "The range for the prompt number is 0-%d.\r\n", x);
		ch->Send(buf);
		return;
	}

	if (GET_PROMPT(ch))
		free(GET_PROMPT(ch));
	GET_PROMPT(ch) = str_dup(def_prompts[i][1]);

	sprintf(buf, "Set your prompt to the %s preset prompt.\r\n", def_prompts[i][0]);
	ch->Send(buf);
}


ACMD(do_prompt)
{
	skip_spaces(argument);

	if (!*argument) {
		sprintf(buf, "Your prompt is currently: %s\r\n", (GET_PROMPT(ch) ? GET_PROMPT(ch) : "n/a"));
		ch->Send(buf);
		return;
	}

	delete_doubledollar(argument);

	if (GET_PROMPT(ch))
		free(GET_PROMPT(ch));
	GET_PROMPT(ch) = str_dup(argument);

	sprintf(buf, "Okay, set your prompt to: %s\r\n", GET_PROMPT(ch));
	ch->Send(buf);
}


ACMD(do_gen_write) {
	FILE *fl;
	char *tmp, *filename, *buf;
	struct stat fbuf;
	time_t ct;

	switch (subcmd) {
		case SCMD_BUG:		filename = BUG_FILE;		break;
		case SCMD_TYPO:		filename = TYPO_FILE;		break;
		case SCMD_IDEA:		filename = IDEA_FILE;		break;
		case SCMD_WARNING:	filename = WARNING_FILE;	break;
		default:										return;
	}

	ct = time(0);
	tmp = asctime(localtime(&ct));

	if (IS_NPC(ch)) {
		ch->Send("Monsters can't have ideas - Go away.\r\n");
		return;
	}

	skip_spaces(argument);

	if (!*argument) {
		ch->Send("That must be a mistake...\r\n");
		return;
	}

	buf = get_buffer(MAX_INPUT_LENGTH);
	strcpy(buf, argument);
	delete_doubledollar(buf);

	mudlogf(NRM, ch, FALSE,	"%s %s: %s", ch->Name(), CMD_NAME, buf);

	if (stat(filename, &fbuf) < 0)				perror("Error statting file");
	else if (fbuf.st_size >= max_filesize)		ch->Send("Sorry, the file is full right now...\r\n");
	else if (!(fl = fopen(filename, "a"))) {
		perror("do_gen_write");
		ch->Send("Could not open the file.	Sorry.\r\n");
	} else {
		fprintf(fl, "%-8s (%6.6s) [%5d] %s\n", ch->Name(), (tmp + 4), world[IN_ROOM(ch)].Virtual(), buf);
		fclose(fl);
		ch->Send("Okay.	Thanks!\r\n");
	}
	release_buffer(buf);
}



#define TOG_OFF 0
#define TOG_ON	1

#define PRF_TOG_CHK(ch,flag) ((TOGGLE_BIT(PRF_FLAGS(DESC_ORIG(ch)), (flag))) & (flag))
#define CHN_TOG_CHK(ch,flag) ((TOGGLE_BIT(CHN_FLAGS(DESC_ORIG(ch)), (flag))) & (flag))

ACMD(do_gen_tog) {
	Flags	result;

	const char *tog_messages[][2] = {
		{"You are now safe from summoning by other players.",
		"You may now be summoned by other players."},
		{"Nohassle disabled.",
		"Nohassle enabled."},
		{"Brief mode off.",
		"Brief mode on."},
		{"Compact mode off.",
		"Compact mode on."},
		{"You can now hear tells.",
		"You are now deaf to tells."},
		{"You can now hear music.",
		"You are now deaf to music."},
		{"You can now hear shouts.",
		"You are now deaf to shouts."},
		{"You can now hear chat.",
		"You are now deaf to chat."},
		{"You can now hear the congratulation messages.",
		"You are now deaf to the congratulation messages."},
		{"You can now hear the Wiz-channel.",
		"You are now deaf to the Wiz-channel."},
		{"You are no longer part of the Mission.",
		"Okay, you are part of the Mission!"},
		{"You will no longer see the room flags.",
		"You will now see the room flags."},
		{"You will now have your communication repeated.",
		"You will no longer have your communication repeated."},
		{"HolyLight mode off.",
		"HolyLight mode on."},
		{"Nameserver_is_slow changed to NO; IP addresses will now be resolved.",
		"Nameserver_is_slow changed to YES; sitenames will no longer be resolved."},
		{"Autoexits disabled.",
		"Autoexits enabled."},
		{"You will now receive game broadcasts.",
		"You will no longer receive game broadcasts."},
		{"You will no longer see color.",
		"You will now see color."},
		{"You can now hear racial communications.",
		"You are now deaf to racial communications."},
		{"You will no longer automatically assist your group.",
		"You will now automatically assist your group."},
		{"MUD sound protocol deactivated.",
		"MUD sound protocol activated."},
		{"You will now slay surrendered foes.",
		"You will now pardon surrendered foes."}
	};


	if (IS_NPC(ch))
		return;

// CHANGEPOINT:	We have to revamp this.
	switch (subcmd) {
		case SCMD_NOSUMMON:	result = PRF_TOG_CHK(ch, Preference::Summonable);	break;
		case SCMD_NOHASSLE:	result = PRF_TOG_CHK(ch, Preference::NoHassle);		break;
		case SCMD_BRIEF:	result = PRF_TOG_CHK(ch, Preference::Brief);		break;
		case SCMD_COMPACT:	result = PRF_TOG_CHK(ch, Preference::Compact);		break;
		case SCMD_NOTELL:	result = CHN_TOG_CHK(ch, Channel::NoTell);			break;
//		case SCMD_NOMUSIC:	result = CHN_TOG_CHK(ch, Channel::NoMusic);			break;
		case SCMD_DEAF:		result = CHN_TOG_CHK(ch, Channel::NoShout);			break;
		case SCMD_NOGOSSIP:	result = CHN_TOG_CHK(ch, Channel::NoGossip);			break;
		case SCMD_NOGRATZ:	result = CHN_TOG_CHK(ch, Channel::NoGratz);			break;
		case SCMD_NOWIZ:	result = CHN_TOG_CHK(ch, Channel::NoWiz);			break;
		case SCMD_QUEST:	result = CHN_TOG_CHK(ch, Channel::Quest);			break;
		case SCMD_ROOMFLAGS:result = PRF_TOG_CHK(ch, Preference::RoomFlags);	break;
		case SCMD_NOREPEAT:	result = PRF_TOG_CHK(ch, Preference::NoRepeat);		break;
		case SCMD_HOLYLIGHT:result = PRF_TOG_CHK(ch, Preference::HolyLight);	break;
//		case SCMD_SLOWNS:	result = (nameserver_is_slow = !nameserver_is_slow);	break;
		case SCMD_AUTOEXIT:	result = PRF_TOG_CHK(ch, Preference::AutoExit);		break;
//		case SCMD_NOINFO:	result = CHN_TOG_CHK(ch, Channel::NoInfo);			break;
		case SCMD_AUTOASSIST:	result = PRF_TOG_CHK(ch, Preference::AutoAssist);		break;
		case SCMD_COLOR:	result = PRF_TOG_CHK(ch, Preference::Color);		break;
		case SCMD_SOUND:	result = PRF_TOG_CHK(ch, Preference::Sound);		break;
		case SCMD_MERCIFUL:	result = PRF_TOG_CHK(ch, Preference::Merciful);		break;
//		case SCMD_NORACE:	result = CHN_TOG_CHK(ch, Channel::NoRace);			break;
		default:	log("SYSERR: Unknown subcmd %d in do_gen_toggle", subcmd);	return;
	}

	if (result)		ch->Sendf("%s\r\n", tog_messages[subcmd][TOG_ON]);
	else			ch->Sendf("%s\r\n", tog_messages[subcmd][TOG_OFF]);

	return;
}


ACMD(do_afk) {
	if (IS_NPC(ch))
		return;

	skip_spaces(argument);

	if (!GET_AFK(ch)) {
		GET_AFK(ch) = str_dup(*argument ? argument : "I am AFK!");
		SET_BIT(PLR_FLAGS(ch), PLR_AFK);
		act("$n has just gone AFK:  $T", TRUE, ch, 0, GET_AFK(ch), TO_ROOM);
        act("You are now AFK.", FALSE, ch, 0, 0, TO_CHAR);
	} else {
		free(GET_AFK(ch));
		GET_AFK(ch) = NULL;
		REMOVE_BIT(PLR_FLAGS(ch), PLR_AFK);
        act("$n just returned to this world.", TRUE, ch, 0, 0, TO_ROOM);
        act("Ok, I guess that means you're back.", FALSE, ch, 0, 0, TO_CHAR);
	}

	return;
}


ACMD(do_pagelength)
{
	int size;

	one_argument(argument, arg);

	if (!*arg) {
		if (GET_PAGE_LENGTH(ch)) {
			sprintf(buf, "Your page length is currently set at %d lines.\r\n",
				GET_PAGE_LENGTH(ch));
			ch->Send(buf);
		} else
			ch->Send("Your pager is currently turned off.\r\n");
	} else if (!strn_cmp(arg, "default", strlen(arg))) {
		GET_PAGE_LENGTH(ch) = DEFAULT_PAGE_LENGTH;
		ch->Send("Page length set to default value.\r\n");
	} else if (!strn_cmp(arg, "off", strlen(arg))) {
		GET_PAGE_LENGTH(ch) = 0;
		ch->Send("Output will no longer be paged.\r\n");
	} else {
		size = atoi(arg);
		if ((size < 5) || (size > 128))
			ch->Send("Page length must be off, default, or a number between 5 and 127.\r\n");
		else {
			GET_PAGE_LENGTH(ch) = size;
			sprintf(buf, "Page length set to %d lines.\r\n", size);
			ch->Send(buf);
		}
	}
}


// New default color preference for communication! -DH
ACMD(do_saycolor)
{
	UInt8 i = 0, valid_arg = FALSE;
	char extracolor[MAX_INPUT_LENGTH];

	two_arguments(argument, arg, extracolor);

	if (*arg) {
		for (i = 0; (!valid_arg) && (*colornames[i] != '\n'); i++)
			if (!strcmp(arg, colornames[i])) {
				valid_arg = TRUE;
				--i;
			}

		if (valid_arg) {
			switch (i) {
			case 0:
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
			case 11:
			case 12:
			case 13:
			case 14:
			case 15:
			case 16:
			case 17:
			case 18:
				GET_SAYCOLOR(ch) = i;
				ch->Sendf("Your communication color is now %s%s&c0.\r\n",
					colorcodes[GET_SAYCOLOR(ch)], colornames[GET_SAYCOLOR(ch)]);
				break;
			default:
				valid_arg = FALSE;
			}
		}
	} else {
		ch->Sendf("Your communication color is currently %s%s&c0.\r\n",
			 colorcodes[GET_SAYCOLOR(ch)], colornames[GET_SAYCOLOR(ch)]);
	}

	if (!valid_arg) {
		char *buf4 = get_buffer(MAX_INPUT_LENGTH);
		strcpy(buf4, "Valid saycolor arguments are:	\r\noff    ");
		for (i = 3; i <= 18; ++i) {
			if (i == 10)	strcat(buf4, "\r\n");
			else	sprintf(buf4 + strlen(buf4), "%s%-8.8s ",
							colorcodes[i], colornames[i]);
		}
		strcat(buf4, "\r\n");
		ch->Send(buf4);
		release_buffer(buf4);
	}

}


class SearchEvent : public ActionEvent {
public:
						SearchEvent(time_t when, Character &tch, MUDObject *tobj) :
						ActionEvent(when, tch), targ(tobj) {}
					
	SafePtr<MUDObject>		targ;

	time_t					Execute(void);
};


time_t			SearchEvent::Execute(void)
{
	int				i;
	Character		*vict = NULL;
	Object			*obj = NULL;
	IDNum			target = 0;
    VNum 			room = NOWHERE;
	SInt8			chance;
	bool			found = false;

	// CHANGEPOINT:	Add modifiers for different situations.
	chance = GET_INT(ch);
	target = targ->ID();

	sprintf(buf, "%c%d", UID_CHAR, target);
	if (!(get_obj_in_list_vis(ch, buf, ch->contents) ||
		get_obj_in_list_vis(ch, buf, world[IN_ROOM(ch)].contents))) {

		targ.Clear();
	}

	if (targ) {	/* nothing to find on objects, yet */
		act("You didn't find anything new about $o.", false, ch, targ(), NULL, TO_CHAR);
	} else {
		if (IN_ROOM(ch) == NOWHERE) {
			log("SYSERR: Character in NOWHERE when searching.");
		    return 0;
	    }

	    if (&world[IN_ROOM(ch)] == find_room(target)) {
			for (i = 0; i < NUM_OF_DIRS; ++i) {
				if (EXIT(ch, i) && IS_SET(EXIT(ch, i)->exit_info, Exit::Hidden) && 
					(SKILLROLL(ch, SKILL_SEARCH) < 0)) {
					sprintf(buf, "You find a hidden %s to the %s.",
						EXIT(ch, i)->Keyword() ? fname(EXIT(ch, i)->Keyword()) : "exit", dirs[i]);
					act(buf, FALSE, ch, NULL, NULL, TO_CHAR);
					found = TRUE;
				}
			}

			/* hidden objects */
			START_ITER(obj_iter, obj, Object *, world[IN_ROOM(ch)].contents) {
				if (CAN_SEE_OBJ(ch, obj) && OBJ_FLAGGED(obj, ITEM_HIDDEN) && (dice(3, 6) < chance)) {
					act("You discover $p hidden here.", false, ch, targ(), NULL, TO_CHAR);
					found = true;
				}
			}
			/* hidden mobs */
			START_ITER(ch_iter, vict, Character *, world[IN_ROOM(ch)].people) {
				if (AFF_FLAGGED(vict, AffectBit::Hide)) {
					/* remove the hide so it doesn't mess up with CAN_SEE */
					REMOVE_BIT(AFF_FLAGS(vict), AffectBit::Hide);
					if (CAN_SEE(ch, vict) && (SKILLROLL(ch, SKILL_SEARCH) >= 0))
						/* make a skill check for victim */
						if (SKILLROLL(vict, SKILL_HIDE) <= 0) {
							act("You discover $N hiding here.", false, ch, NULL, vict, TO_CHAR);
							act("$n has discovered you hiding!", true, ch, NULL, vict, TO_VICT);
							found = true;
							continue;
						}

					SET_BIT(AFF_FLAGS(vict), AffectBit::Hide);
				}
			}

      		if (!found) {
				act("You didn't find anything unusual from your searching.", 
					false, ch, NULL, NULL, TO_CHAR);
			}
		} else {
      		act("You can't find what you were searching!", false, ch, NULL, NULL, TO_CHAR);
		}
	}
	
	return 0;
}


ACMD(do_search)
{
	MUDObject	*target = NULL;
	char buf[100], *name = NULL;
	Object *obj = NULL;

	if (GET_ACTION(ch)) {
		act("You are already busy!", FALSE, ch, NULL, NULL, TO_CHAR);
		return;
	}

	if (!argument || !*argument) {
		name = "the room";
	}

	else {
		one_argument(argument, arg);

		if		((obj = get_obj_in_list_vis(ch, arg, ch->contents)));
		else 	obj = get_obj_in_list_vis(ch, arg, world[IN_ROOM(ch)].contents);
	}

	if (obj) {
		target = obj;
		name = "$p";
	} else {
		if (!str_cmp(arg, "room")) {
			name = "the room";
		}
	}

	if (!name)
		act("You can't find what you are looking for.", FALSE, ch, NULL, NULL, TO_CHAR);

	else {
		new SearchEvent(4 RL_SEC, *ch, target);

		// NOTE:  The wait state is no longer needed as the event will be cancelled by
        // action, like the hide vector.

		// CHANGEPOINT:	20 and 41 are purely arbitrary, based on old numbers.	Change ASAP!
		// CHANGEPOINT 0 in last argument should be replaced by flags for event
		// cancellation.
		// WAIT_STATE(ch, 41);

		sprintf(buf, "You start to search %s.", name);
		act(buf, FALSE, ch, obj, NULL, TO_CHAR);
		sprintf(buf, "$n starts to search %s.", name);
		act(buf, TRUE, ch, obj, NULL, TO_ROOM);
		return;
	}
}


int riding_check(Character *ch, Character *mount, int type)
{
	/*
	 * The formula is way to complex, but it fits the definition.
	 * Maybe throwing in Newton's Method would make it perfect.
	 *
	 * raw skill counts for (75% * skill).	disposition counts for the
	 * remainder, up to 100%.	The max chance is 110%, which goes down
	 * if you are in combat.	Both the skill factor and disposition are
	 * scaled so a better mount or more skill always helps (until you hit
	 * the max of 98%)
	 */

	// This shouldn't be used some much for just moving around.
	if ((type == MNT_MOVE) && (Number(0, 10)))
		return 1;

	int roll;

	// CHANGEPOINT:	Chance mount disposition values and modifier for this skill
	roll = SKILLROLL(ch, SKILL_RIDE) + GET_DISPOSITION(mount) / 50;

	if (roll < 0)		return 0;
	else				return 1;
}


void MUDObject::AddRider(Character * ch)
{
	riders.Add(ch);
	ch->SetMount(this);
}


void Object::AddRider(Character * ch)
{
	Character *tch = NULL;

	if ((tch = get_char_on_obj(this)))
		act("$N is already mounted on $p.", TRUE, ch, this, tch, TO_CHAR);
	else if (!IS_MOUNTABLE(this))
		act("You can't mount $p.", TRUE, ch, this, 0, TO_CHAR);
	else if (sit_otrigger(this, ch)) {
		act("You mount $p.", TRUE, ch, this, 0, TO_CHAR);
		act("$n mounts $p.", TRUE, ch, this, 0, TO_ROOM);
		GET_POS(ch) = POS_SITTING;
		riders.Add(ch);
		ch->SetMount(this);
	}
}


void Character::AddRider(Character * ch)
{
	if (GET_MAX_RIDERS(this) < 1)
		act("You can't ride that!", FALSE, ch, NULL, NULL, TO_CHAR);
	else if (!IS_HUMANOID(ch) || (GET_MAX_RIDERS(ch) > 0))
		act("You can't ride things!", FALSE, ch, NULL, NULL, TO_CHAR);
	else {
		/* check if there's enough room left */
		if (riders.Count() >= GET_MAX_RIDERS(this))
			act("$N can't carry any more people.", FALSE, ch, NULL, this, TO_CHAR);
		else if (riding_check(ch, this, MNT_MOUNT)) {
			riders.Add(ch);
			ch->SetMount(this);
			act("$n hops on the back of $N.", TRUE, ch, NULL, this, TO_NOTVICT);
			act("$n hops on your back.", FALSE, ch, NULL, this, TO_VICT);
			act("You hop on the back of $N.", FALSE, ch, NULL, this, TO_CHAR);
		}

		else {
			GET_POS(ch) = POS_SITTING;
			act("$n tries to ride $N, but falls on $s butt.", TRUE, ch, NULL, this, TO_NOTVICT);
			act("$n tries to hop on your back, but falls on $s butt.", FALSE, ch, NULL, this, TO_VICT);
			act("You try to ride $N, but you fall off and land on your butt!", FALSE, ch, NULL, this, TO_CHAR);
			WAIT_STATE(ch, PULSE_VIOLENCE / 2);
		}
	}
}


void MUDObject::RemoveRider(Character * ch, int type)
{
	GET_POS(ch) = POS_STANDING;
	riders.Remove(ch);
	ch->SetMount(NULL);
}


void Character::RemoveRider(Character * ch, int type)
{
	GET_POS(ch) = POS_STANDING;

	/* update riding list */
	riders.Remove(ch);

	/* send messages */
	switch (type) {
	case MNT_DISMOUNT:
		act("You dismount $N.", FALSE, ch, NULL, this, TO_CHAR);
		act("$n hops off your back.", FALSE, ch, NULL, this, TO_VICT);
		act("$n dismounts $N.", TRUE, ch, NULL, this, TO_NOTVICT);
		break;

	case MNT_FLEE:
	case MNT_MOVE:
	case MNT_COMBAT:
		act("You fall off of $N's back.", FALSE, ch, NULL, this, TO_CHAR);
		act("$n falls off your back.", FALSE, ch, NULL, this, TO_VICT);
		act("$n falls off of $N's back.", TRUE, ch, NULL, this, TO_NOTVICT);
		GET_POS(ch) = POS_SITTING;
		break;

	default:
		act("You no longer ride $N.", FALSE, ch, NULL, this, TO_CHAR);
		act("$n no longer rides you.", FALSE, ch, NULL, this, TO_VICT);
		act("$n no longer rides $N.", TRUE, ch, NULL, this, TO_NOTVICT);
		break;
	}

	/* FIXME - take care of falling off in mid air */
	ch->SetMount(NULL);
}


ACMD(do_mount)
{
	MUDObject * target;
	Object *vehicle;
	Character *mount, *tch;

	one_argument(argument, arg);

	if (ch->Riding())
		act("You are already mounted on $N!", FALSE, ch, NULL, ch->Riding(), TO_CHAR);
	else if (ch->SittingOn())
		act("You are already sitting on $p!", FALSE, ch, ch->SittingOn(), NULL, TO_CHAR);
	else if (!arg || !*arg)
		act("What would you like to mount?", FALSE, ch, NULL, NULL, TO_CHAR);

	target = get_char_vis(ch, arg, FIND_CHAR_ROOM);

	if (!target) {
		target = get_obj_in_list_vis(ch, arg, world[IN_ROOM(ch)].contents);
	}

	if (!target) {
		act("You don't see that here.", TRUE, ch, 0, 0, TO_CHAR);
		return;
	}

	target->AddRider(ch);
}


ACMD(do_dismount)
{
	int has_boat(Character *ch);

	/* need check for flying sector */
	if (ch->Mounted() == NULL)
		act("But you aren't riding anything!", FALSE, ch, NULL, NULL, TO_CHAR);

	/* if this room is air, check for fly */
	else if ((SECT(IN_ROOM(ch)) == SECT_FLYING) && !AFF_FLAGGED(ch, AffectBit::Fly))
		act("You would have to fly to dismount here.", false, ch, NULL, NULL, TO_CHAR);

	/* if this room needs a boat, check for one */
	else if ((SECT(IN_ROOM(ch)) == SECT_WATER_NOSWIM) && !has_boat(ch))
		act("You need a boat to dismount here.", FALSE, ch, NULL, NULL, TO_CHAR);

	else {
		ch->Mounted()->RemoveRider(ch, MNT_DISMOUNT);
		ch->SetMount(NULL);
	}

}


ACMD(do_hide)
{
	Object *obj;
	SInt16 diceroll;
	Affect				*aff = NULL;

	if (!argument || !*argument) {	/* hide yourself */
	ch->Send("You attempt to hide yourself.\r\n");

	if (AFF_FLAGGED(ch, AffectBit::Hide))
		REMOVE_BIT(AFF_FLAGS(ch), AffectBit::Hide);

		diceroll = CHAR_SKILL(ch).RollSkill(ch, SKILL_HIDE);
		CHAR_SKILL(ch).RollToLearn(ch, SKILL_HIDE, diceroll);

		if (diceroll < 0)	return;
		aff = new Affect(SKILL_HIDE, 0, Affect::None, AffectBit::Hide, NULL);
		aff->Join(ch, diceroll * PULSE_VIOLENCE, false, false, false, false);
	}

	else { /* hide an object */
		one_argument(argument, arg);

		if (!(obj = get_obj_in_list_vis(ch, arg, ch->contents))) {
			sprintf(buf, "You don't seem to have %s %s.\r\n", AN(arg), arg);
			ch->Send(buf);
		}
		else if (can_drop_obj(ch, obj, SCMD_DROP, "")) {
			obj->FromChar();
			obj->ToRoom(IN_ROOM(ch));
			act("You hide $p in the room.", FALSE, ch, obj, NULL, TO_CHAR);
			act("$n tries to hides $p in the room.", TRUE, ch, obj, NULL, TO_ROOM);

		diceroll = CHAR_SKILL(ch).RollSkill(ch, SKILL_HIDE);
		CHAR_SKILL(ch).RollToLearn(ch, SKILL_HIDE, diceroll);
		if (diceroll >= 0)
			SET_BIT(GET_OBJ_EXTRA(obj), ITEM_HIDDEN);
		}
	}
}


ACMD(do_sneak)
{
	SInt16 diceroll;
	Affect				*aff = NULL;

	ch->Send("Okay, you'll try to move silently for a while.\r\n");

	WAIT_STATE(ch, PULSE_VIOLENCE / 2);

	diceroll = CHAR_SKILL(ch).RollSkill(ch, SKILL_SNEAK);
	if (diceroll >= 0) { 
		CHAR_SKILL(ch).RollToLearn(ch, SKILL_SNEAK, diceroll);
		aff = new Affect(SKILL_SNEAK, 0, Affect::None, AffectBit::Sneak, NULL);
		aff->Join(ch, diceroll * PULSE_VIOLENCE, false, false, false, false);
	}
}


ACMD(do_camoflauge)
{
	SInt16 diceroll;
	Affect				*aff = NULL;

	ch->Send("You attempt to camoflauge yourself.\r\n");

	WAIT_STATE(ch, PULSE_VIOLENCE / 2);

	diceroll = CHAR_SKILL(ch).RollSkill(ch, SKILL_CAMOFLAUGE);
	if (diceroll >= 0) {
		CHAR_SKILL(ch).RollToLearn(ch, SKILL_CAMOFLAUGE, diceroll);
		aff = new Affect(SKILL_CAMOFLAUGE, 0, Affect::None, AffectBit::Camoflauge, NULL);
		aff->Join(ch, diceroll * PULSE_VIOLENCE, false, false, false, false);
	}
}


ACMD(do_fly)
{
	SInt8 fly_ok = FALSE, req_move = TRUE;

	if (ADVANTAGED(ch, Advantages::Flight))
		fly_ok = TRUE;

	if (Affect::AffectedBy(ch, SPELL_FLY)) {
		req_move = FALSE;
		fly_ok = TRUE;
	}

	if (!fly_ok) {
		ch->Sendf("You flap your %s, but do not leave the ground.\r\n",
					GET_WEARSLOT(WEAR_ARMS, GET_BODYTYPE(ch)).partname);
		return;
	}

	if (req_move && (GET_MOVE(ch) < 5)) {
		ch->Send("You are too tired to fly!\r\n");
		return;
	}

	if (AFF_FLAGGED(ch, AffectBit::Fly)) {
		ch->Send("You are already flying!\r\n");
		return;
	} else {
		ch->Send("You take to the air!\r\n");
		act("$n takes to the air!", TRUE, ch, 0, 0, TO_ROOM);
		SET_BIT(AFF_FLAGS(ch), AffectBit::Fly);
	}
}


ACMD(do_land)
{
	if (AFF_FLAGGED(ch, AffectBit::Fly)) {
		if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_GRAVITY)) {
			ch->Send("There's nowhere here to land!\r\n");
		} else {
			ch->Send("You touch down, landing on the ground.\r\n");
			act("$n touches down, landing on the ground.", TRUE, ch, 0, 0, TO_ROOM);
			if (!Affect::AffectedBy(ch, SPELL_FLY))
				REMOVE_BIT(AFF_FLAGS(ch), AffectBit::Fly);
		}
	} else {
		ch->Send("You're not quite flying at the moment.\r\n");
		return;
	}
}


ACMD(do_split)
{
	int amount, num, share;
	Character *k, *follower;

	if (IS_NPC(ch))
		return;

	one_argument(argument, buf);

	if (is_number(buf)) {
		amount = atoi(buf);
		if (amount <= 0) {
			ch->Send("Sorry, you can't do that.\r\n");
			return;
		}
		if (amount > GET_GOLD(ch)) {
			ch->Send("You don't seem to have that much gold to split.\r\n");
			return;
		}
		k = (ch->master ? ch->master : ch);

		if (AFF_FLAGGED(k, AffectBit::Group) && (k->InRoom() == ch->InRoom()))
			num = 1;
		else
			num = 0;

		START_ITER(iter, follower, Character *, k->followers) {
			if (AFF_FLAGGED(follower, AffectBit::Group) &&
				(!IS_NPC(follower)) && (follower->InRoom() == ch->InRoom()))
			++num;
		}

		if (num && AFF_FLAGGED(ch, AffectBit::Group))
			share = amount / num;
		else {
			ch->Send("With whom do you wish to share your gold?\r\n");
			return;
		}

		GET_GOLD(ch) -= share * (num - 1);

		if (AFF_FLAGGED(k, AffectBit::Group) && (k->InRoom() == ch->InRoom()) && !(IS_NPC(k)) && k != ch) {
			GET_GOLD(k) += share;
			sprintf(buf, "%s splits %d coins; you receive %d.\r\n", ch->Name(), amount, share);
			k->Send(buf);
		}

		iter.Reset();

		while ((follower = iter.Next())) {
			if (AFF_FLAGGED(follower, AffectBit::Group) && (follower != ch) &&
				(!IS_NPC(follower)) && (follower->InRoom() == ch->InRoom())) {
				GET_GOLD(follower) += share;
				sprintf(buf, "%s splits %d coins; you receive %d.\r\n", ch->Name(), amount, share);
				follower->Send(buf);
			}
		}

		sprintf(buf, "You split %d coins among %d members -- %d coins each.\r\n",
			amount, num, share);
		ch->Send(buf);
	} else {
		ch->Send("How many coins do you wish to split with your group?\r\n");
		return;
	}
}


#define PREF_GODCMD(cmd)   (pref_cmds[cmd].minimum_level == STAFF_CMD)
#define PREF_NPCCMD(cmd)   (pref_cmds[cmd].minimum_level == MOB_CMD)
// CHANGEPOINT:  Daniel Rollings AKA Daniel Houghton's beginnings of a unified preference screen.
// It's bloody inefficient right now, but it'll do.
ACMD(do_preference)
{
	int j = 0;
	SInt8 valid_arg = FALSE;

	ACMD(do_toggle);
	ACMD(do_gen_tog);
	ACMD(do_display);
	ACMD(do_prompt);
	ACMD(do_pagelength);
	ACMD(do_saycolor);
	ACMD(do_invuln);
	ACMD(do_wimpy);

	const struct preference_info {
		char *command;
		ACMD(*command_pointer);
		SInt16 minimum_level;
		long godcmd;
		int	subcmd;
	} pref_cmds[] = {
		{ "autoassist"	, do_gen_tog	, 0, 0, SCMD_AUTOASSIST},
		{ "autoexits"	, do_gen_tog	, 0, 0, SCMD_AUTOEXIT },
		{ "brief"		, do_gen_tog	, 0, 0, SCMD_BRIEF },
		{ "chat"		, do_gen_tog	, 1, 0, SCMD_NOGOSSIP },
		{ "color"		, do_gen_tog	, 0, 0, SCMD_COLOR },
		{ "compact"		, do_gen_tog	, 0, 0, SCMD_COMPACT },
		{ "display"		, do_display	, 0, 0, 0 },
		{ "grats"		, do_gen_tog	, 0, 0, SCMD_NOGRATZ },
		{ "holylight"	, do_gen_tog	, STAFF_CMD, Staff::General, SCMD_HOLYLIGHT },
		{ "invuln"		, do_invuln		, STAFF_CMD, Staff::Invuln, 0 },
		{ "mercy"		, do_gen_tog	, 0, 0, SCMD_MERCIFUL},
		{ "nohassle"	, do_gen_tog	, STAFF_CMD, Staff::General, SCMD_NOHASSLE },
		{ "pagelength"	, do_pagelength	, 0, 0, 0 },
		{ "prompt"		, do_prompt		, 0, 0, 0 },
		{ "repeat"		, do_gen_tog	, 0, 0, SCMD_NOREPEAT },
		{ "roomflags"	, do_gen_tog	, STAFF_CMD, Staff::General, SCMD_ROOMFLAGS },
		{ "saycolor"	, do_saycolor	, 0, 0, 0 },
		{ "shout"		, do_gen_tog	, 1, 0, SCMD_DEAF },
		{ "sound"		, do_gen_tog	, 0, 0, SCMD_SOUND},
		{ "summonable"	, do_gen_tog	, 1, 0, SCMD_NOSUMMON },
		{ "tells"		, do_gen_tog	, 1, 0, SCMD_NOTELL },
		{ "think"		, do_gen_tog	, STAFF_CMD, Staff::Wiznet, SCMD_NOWIZ },
		{ "wimpy"		, do_wimpy		, 0, 0, 0 },
		{ "\n", NULL, 0, 0, 0 }		/* this must be last */
	};

	half_chop(argument, arg, buf2);

	if (!(*arg)) {	// Player typed no argument, act like old "toggle" command
		do_toggle(ch, "toggle", 0, buf2, 0);
		return;
	}

	for (j = 0; (*(pref_cmds[j].command) != '\n') && (!valid_arg); j++) {
		if ((is_abbrev(arg, pref_cmds[j].command)) &&
			 (GET_LEVEL(ch) >= pref_cmds[j].minimum_level &&
			 (!PREF_GODCMD(j) || (pref_cmds[j].godcmd & STF_FLAGS(ch))) &&
			 (!PREF_NPCCMD(j) || IS_NPC(ch)))) {
			valid_arg = TRUE;
			break;
		}
	}

	if (valid_arg) {
		((*pref_cmds[j].command_pointer)
		 (ch, buf2, j, pref_cmds[j].command, pref_cmds[j].subcmd));
	} else {
		ch->Sendf("Invalid argument:	'%s'\r\n", arg);
	}
}
#undef PREF_GODCMD
#undef PREF_GODCMD2
#undef PREF_NPCCMD
