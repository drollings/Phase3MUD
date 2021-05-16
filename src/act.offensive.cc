/* ************************************************************************
*   File: act.offensive.c                               Part of CircleMUD *
*  Usage: player-level commands of an offensive nature                    *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author: sfn $
*  $Date: 2001/02/04 15:57:56 $
*  $Revision: 1.16 $
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
#include "db.h"
#include "help.h"
#include "event.h"
#include "constants.h"
#include "skills.h"
#include "combat.h"
#include "sound.h"

/* extern functions */
extern void add_follower(Character *ch, Character *leader);

/* prototypes */
ACMD(do_assist);
ACMD(do_hit);
ACMD(do_order);
ACMD(do_flee);
ACMD(do_magic);

extern int combat_move_chance(Character *ch, int move);

extern int find_first_step(VNum src, char *target, Flags edgetype);
extern int find_first_step(VNum src, VNum target, Flags edgetype);


ACMD(do_assist)
{
	Character *helpee = NULL, *opponent, *k, *follower;
	LListIterator<Character *>	iter;

	if (FIGHTING(ch)) {
		ch->Send("You're already fighting!  How can you assist someone else?\r\n");
		return;
	}
	one_argument(argument, arg);

	if (AFF_FLAGGED(ch, AffectBit::Group)) {
		if (!(k = ch->master))
			k = ch;

		if (k && AFF_FLAGGED(k, AffectBit::Group) && FIGHTING(k) &&
				(IN_ROOM(k) == IN_ROOM(ch)) && (k != ch))
			helpee = k;
		else 
			START_ITER(iter, follower, Character *, ch->followers)
				if (AFF_FLAGGED(follower, AffectBit::Group) && FIGHTING(follower) &&
						(IN_ROOM(follower) == IN_ROOM(ch)) && (follower != ch)) {
					helpee = follower;
					break;
				}
	}
	if ((!helpee) && (!*arg)) {
		ch->Send("Whom do you wish to assist?\r\n");
		return;
	} else if (!(helpee = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
		ch->Send(NOPERSON);
		return;
	}

	if (helpee == ch)
		ch->Send("You can't help yourself any more than this!\r\n");
	else {
		opponent = FIGHTING(helpee);

		if (!opponent)
			act("But nobody is fighting $M!", FALSE, ch, 0, helpee, TO_CHAR);
		else if (opponent == ch)
			act("To assist $M, just drop your weapon and remove your armor...",
		FALSE, ch, NULL, opponent, TO_CHAR);
		else if (!CAN_SEE(ch, opponent))
			act("You can't see who is fighting $M!", FALSE, ch, 0, helpee, TO_CHAR);
		else if (!IS_NPC(opponent))	/* prevent accidental pkill */
			act("Use 'murder' if you really want to attack $N.", FALSE,
		ch, 0, opponent, TO_CHAR);
		else {
			ch->Send("You join the fight!\r\n");
			act("$N assists you!", 0, helpee, 0, ch, TO_CHAR);
			act("$n assists $N.", FALSE, ch, 0, helpee, TO_NOTVICT);
			ch->Hit(opponent);
		}
	}
}


ACMD(do_hit)
{
	Character *vict, *follower;
	Attack *attackmove = NULL;
	int i = -1, location;

	ACMD(do_assist);

	half_chop(argument, arg, argument);

	if (!*arg)
		ch->Send("Hit who?\r\n");
	else if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM)))
		ch->Send("They don't seem to be here.\r\n");
	else if (vict == ch) {
		ch->Send("You hit yourself...OUCH!.\r\n");
		act("$n hits $mself, and says OUCH!", FALSE, ch, 0, vict, TO_ROOM);
	} else if (AFF_FLAGGED(ch, AffectBit::Charm) && (ch->master == vict))
		act("$N is just such a good friend, you simply can't hit $M.", FALSE, ch, 0, vict, TO_CHAR);
	else {
		/* require murder to attack PCs, to be safe */
			if (!IS_NPC(vict) && !IS_NPC(ch) && (subcmd != SCMD_MURDER)) {
				ch->Send("Use 'murder' to hit another player.\r\n");
				return;
			}
			// if (AFF_FLAGGED(ch, AffectBit::Charm) && !IS_NPC(ch->master) && !IS_NPC(vict))
			// 	return;			/* you can't order a charmed pet to attack a player */

		if (!Combat::Valid(ch, vict))
			return;

		attackmove = new Attack;

		attackmove->move = Combat::Hit;
		attackmove->WeaponMove(ch, subcmd);
		attackmove->chance = SKILLCHANCE(ch, attackmove->skillnum, NULL);

		half_chop(argument, arg, argument);
		
		if (arg && *arg) {
			// The player typed "hit <victim> location"
			for (i = 0; i < NUM_WEARS; ++i) {
				if (!strncmp(arg, GET_WEARSLOT(i, GET_BODYTYPE(vict)).keywords, strlen(arg)))
					break;
			}
			if (i == NUM_WEARS) {
				act("'$T'?  There's no such location to target.", FALSE, ch, 0, arg, TO_CHAR);
				i = -2;		// We don't want -1, that'll give "You do the best you can."
			} else {
				attackmove->location = i;
				if (attackmove->chance >= 0) {
					attackmove->chance -= abs(7 - GET_WEARSLOT(i, GET_BODYTYPE(vict)).hitlocation) + 2;
				}
			}
		}
	
		if ((GET_POS(ch) == POS_STANDING) && ((vict != FIGHTING(ch)) || (i != -1))) {
			ch->Hit(vict, attackmove);
			if (PURGED(ch) || PURGED(vict)) {
				if (attackmove)		delete attackmove;
				return;
			}
			START_ITER(iter, follower, Character *, ch->followers) {
				if (PRF_FLAGGED(follower, Preference::AutoAssist) &&
					 (IN_ROOM(follower) == IN_ROOM(ch)))
					do_assist(follower, (char *) ch->Name(), 0, "", 0);
			}
			WAIT_STATE(ch, PULSE_VIOLENCE - 1);
		} else
			ch->Send("You do the best you can!\r\n");
	}
	
	if (attackmove)
		delete attackmove;
}



ACMD(do_slay)
{
	Character *vict;

	if ((!IS_SET(PRF_FLAGS(ch), Preference::Invuln) || (IS_IMMPUN(ch)) || 
		 IS_NPC(ch))) {
		if (SKILLCHANCE(ch, SPHERE_NECRO, NULL) != SKILL_NOCHANCE) {
	        do_magic(ch, argument, cmd, command, 1);
		}
		else
			do_hit(ch, argument, cmd, "hit", subcmd);
		return;
	}
	one_argument(argument, arg);

	if (str_cmp(command, "slay"))
		ch->Send("You must type slay, and no less, to kill someone.\r\n");
	else if (!*arg) {
		ch->Send("Slay whom?\r\n");
	} else {
		if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM)))
			ch->Send("They aren't here.\r\n");
		else if (ch == vict) {
			if (Number(0, 5))
				ch->Send("Your mother would be so sad.. :(\r\n");
			else {
				act("You chop yourself to pieces, you silly arse!", FALSE, ch, 0, vict, TO_CHAR);
				act("$n chops $mself to pieces!	Silly arse.", FALSE, ch, 0, vict, TO_ROOM);
				vict->Die(ch);
			}
		} else if (GET_TRUST(ch) < GET_TRUST(vict)) {
			ch->Send("Not bloody likely!\r\n");
		} else {
			act("You chop $M to pieces!  Ah!  The blood!", FALSE, ch, 0, vict, TO_CHAR);
			act("$N chops you to pieces!", FALSE, vict, 0, ch, TO_CHAR);
			act("$n chops $N to pieces!  Ah!  The blood!", FALSE, ch, 0, vict, TO_NOTVICT);
			vict->Die(ch);
		}
	}
}


ACMD(do_order) {
	char	*name = get_buffer(MAX_INPUT_LENGTH),
			*message = get_buffer(MAX_INPUT_LENGTH),
			*buf = get_buffer(MAX_INPUT_LENGTH);
	int found = FALSE;
	int org_room;
	Character *vict = NULL, *follower = NULL;

	half_chop(argument, name, message);

	if (!*name || !*message)
		ch->Send("Order who to do what?\r\n");
	else if (!(vict = get_char_vis(ch, name, FIND_CHAR_ROOM)) && !is_abbrev(name, "followers"))
		ch->Send("That person isn't here.\r\n");
	else if (ch == vict)
		ch->Send("You obviously suffer from skitzofrenia.\r\n");
	else {
		if (AFF_FLAGGED(ch, AffectBit::Charm)) {
			ch->Send("Your superior would not aprove of you giving orders.\r\n");
		} else if (vict) {
			act("$N orders you to '$t'", FALSE, vict, (Object *)message, ch, TO_CHAR);
			act("$n gives $N an order.", FALSE, ch, 0, vict, TO_NOTVICT);

			if ((vict->master != ch) || !AFF_FLAGGED(vict, AffectBit::Charm))
				act("$n has an indifferent look.", FALSE, vict, 0, 0, TO_ROOM);
			else {
				ch->Send(OK);
				if (GET_INT(vict) < 4)	act("$n has a blank look.", FALSE, follower, 0, 0, TO_ROOM);
				else	command_interpreter(vict, message);
			}
		} else {			/* This is order "followers" */
			act("$n issues the order '$t'.", FALSE, ch, (Object *)message, vict, TO_ROOM);

			org_room = IN_ROOM(ch);

			START_ITER(iter, follower, Character *, ch->followers) {
				if ((org_room == IN_ROOM(follower)) && AFF_FLAGGED(follower, AffectBit::Charm)) {
					found = TRUE;
					if (GET_INT(follower) < 4) {
						act("$n has a blank look.", FALSE, follower, 0, 0, TO_ROOM);
					} else
						command_interpreter(follower, message);
				}
			}
			if (found)	ch->Send(OK);
			else		ch->Send("Nobody here is a loyal subject of yours!\r\n");
		}
	}
	release_buffer(name);
	release_buffer(message);
	release_buffer(buf);
}


void perform_flee(Character *ch, int &attempt)
{
	Character *fighting = NULL;
	LListIterator<Character *> iter;
	Character *tch;

	act("$n panics, and attempts to flee!", TRUE, ch, 0, 0, TO_ROOM);
	fighting = FIGHTING(ch);
	if (do_simple_move(ch, attempt, TRUE)) {
		ch->Send("You flee head over heels.\r\n");
		sound_to_char(ch, "runaway.wav", 100, 1, 100, "combat");
		if (fighting) {
			if (FIGHTING(fighting) == ch)	fighting->StopFighting();
			ch->StopFighting();
		}

		// Daniel Rollings AKA Daniel Houghton's new code:	Make mobs hunt fleeing foes 
		// if (fighting && IS_NPC(fighting) && MOB_FLAGGED(fighting, MOB_HUNTER)) {
		//			HUNTING(fighting) = GET_ID(ch);
		// }

		/* take care of everyone riding */
		if (RIDER(ch)) {
			START_ITER(iter, tch, Character *, ch->riders) {
				act("Your mount panics and bolts!", FALSE, tch, NULL, NULL, TO_CHAR);
				fighting = FIGHTING(tch);
				if (!riding_check(tch, ch, MNT_FLEE) || !do_simple_move(tch, attempt, TRUE))
					ch->RemoveRider(tch, MNT_FLEE);
				else {
					if (fighting) {
						if (FIGHTING(fighting) == tch)
							fighting->StopFighting();
						tch->StopFighting();
					}
				}
			}
		}
	}
}

ACMD(do_flee)
{
	int i, attempt;

	int riding_check(Character * ch, Character * mount, int type);

	if (AFF_FLAGGED(ch, AffectBit::Berserk)) {
		ch->Send("AHHHH!!!	KILL! KILL! KILL!\r\n");
		return;
	} else if (Affect::AffectedBy(ch, SPELL_BIND)) {
		ch->Send("Cords of magic hold you fast!  You cannot escape!\r\n");
		return;
	} else if (GET_POS(ch) < POS_STANDING) {
		ch->Send("PANIC!	But you must be on your feet to flee!\r\n");
		return;
	} else if (RIDING(ch) && (RIDER(RIDING(ch)) == ch)) {
		do_flee(RIDING(ch), "", cmd, "", subcmd);
	} else if (RIDING(ch)) {
		act("You can't flee without dismounting first!", FALSE, ch, NULL, NULL, TO_CHAR);
	} else {
		STALKING(ch) = 0;	// Why should they keep stalking?

		for (i = 0; i < 6; ++i) {
			attempt = Number(0, NUM_OF_DIRS - 1);	/* Select a random direction */
			if (CAN_GO(ch, attempt) && !EXIT_FLAGGED(EXIT(ch, attempt), Exit::Vista) && 
			   (!IS_NPC(ch) || !IS_SET(ROOM_FLAGS(EXIT(ch, attempt)->to_room), ROOM_NOMOB))) {
				perform_flee(ch, attempt);
				return;
			}
		}
	}
	act("$n tries to flee, but can't!", TRUE, ch, 0, 0, TO_ROOM);
	ch->Send("PANIC!	You couldn't escape!\r\n");
}


ACMD(do_rescue)
{
	Character *vict, *tmp_ch;
	SInt16 diceroll;

	one_argument(argument, arg);

	if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
		ch->Send("Whom do you want to rescue?\r\n");
		return;
	}
	if (vict == ch) {
		ch->Send("What about fleeing instead?\r\n");
		return;
	}
	if (FIGHTING(ch) == vict) {
		ch->Send("How can you rescue someone you are trying to kill?\r\n");
		return;
	}
	START_ITER(iter, tmp_ch, Character *, world[IN_ROOM(ch)].people) {
		if (FIGHTING(tmp_ch) == vict)
			break;
	}

	if (!tmp_ch) {
		act("But nobody is fighting $M!", FALSE, ch, 0, vict, TO_CHAR);
		return;
	} else if (Affect::AffectedBy(vict, SPELL_BIND)) {
		act("You can't get between $N and $S foes!", FALSE, ch, 0, vict, TO_CHAR);
	} else {

		diceroll = CHAR_SKILL(ch).RollSkill(ch, SKILL_RESCUE);
		CHAR_SKILL(ch).RollToLearn(ch, SKILL_RESCUE, diceroll + SKILLCHANCE(ch, SKILL_RESCUE, NULL));

		if (diceroll < 0) {
			ch->Send("You fail the rescue!\r\n");
			return;
		}

		ch->Send("Banzai!  To the rescue...\r\n");
		act("You are rescued by $N, you are confused!", FALSE, vict, 0, ch, TO_CHAR);
		act("$n heroically rescues $N!", FALSE, ch, 0, vict, TO_NOTVICT);

		if (FIGHTING(vict) == tmp_ch)
			vict->StopFighting();
		if (FIGHTING(tmp_ch))
			tmp_ch->StopFighting();
		if (FIGHTING(ch))
			ch->StopFighting();

		ch->Fight(tmp_ch);
		tmp_ch->Fight(ch);

		WAIT_STATE(vict, 2 * PULSE_VIOLENCE);
	}

}


// Used to make sure that a given combat move is a valid command for a 
// character, and find the target.
Character *combat_assertion(Character * ch, char *argument, int subcmd,
					SInt16 & skillnum, SInt8 & prob)
{
	Character *vict = NULL;

	if (!IS_WEAPONSKILL(skillnum))
		skillnum = combat_moves[subcmd].skillnum;

	if (VALID_SKILL(skillnum) && !SKL_IS_STAT(skillnum)) {
		prob = SKILLCHANCE(ch, skillnum, NULL);
		if (prob == SKILL_NOCHANCE) {
			ch->Sendf("%s:  You aren't trained to do that effectively!\r\n", Skill::Name(skillnum));
			return (NULL);
		}
	}

	one_argument(argument, arg);

	if (!*arg) {
		if (FIGHTING(ch)) {
			vict = FIGHTING(ch);
		} else {
			ch->Send("You must specify a target if you're not already fighting!\r\n");
			return (NULL);
		}
	} else {
		if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
			ch->Send("They aren't here.\r\n");
			return (NULL);
		} else if (ch == vict) {
			if ((skillnum == SKILL_BACKSTAB) || (skillnum == SKILL_STRANGLE)) {
				ch->Send("How can you sneak up on yourself?\r\n");
				return (NULL);
			} else {
				ch->Send("Aren't we funny today...\r\n");
				return (NULL);
			}
		}
	}
	
	return (vict);
}


// Special bladesong manuever
ACMD(do_bladesong)
{
	Object *weapon = NULL;
	SInt16 diceroll;
	SInt8 hand, prob;
	unsigned long wait;

	if (GET_RACE(ch) != RACE_ELF) {
		ch->Send("Your essence does not permit you to do that.\r\n");
		return;
	}
	prob = SKILLCHANCE(ch, SKILL_BLADESONG, NULL);
	
	if (prob == SKILL_NOCHANCE) {
		ch->Send("You have not been trained in the way of bladesong.\r\n");
		return;
	}
	if (FIGHTING(ch) == NULL) {
		ch->Send("Bladesong really isn't useful right at this moment.	Save it for combat.\r\n");
		return;
	}
	if (GET_MANA(ch) < 10) {
		ch->Send("You have not the mana for bladesong!\r\n");
		return;
	}
	hand = find_eqtype_pos(ch, ITEM_WEAPON, WEAR_HAND_R, NUM_WEARS, FALSE);
	if (hand < 0) {
		ch->Send("You have no weapon to cause to sing!\r\n");
		return;
	}
	GET_MANA(ch) -= 10;
	weapon = GET_EQ(ch, hand);

	diceroll = CHAR_SKILL(ch).RollSkill(ch, SKILL_BLADESONG, prob);
	CHAR_SKILL(ch).RollToLearn(ch, SKILL_BLADESONG, diceroll + prob);

	if (diceroll < 0) {
		act("Your essence fails to touch $p.", FALSE, ch, weapon, NULL, TO_CHAR);
		return;
	}

	act("Your $o sings with ethereal tones!", FALSE, ch, weapon, NULL, TO_CHAR);
	act("$n's $o sings with ethereal tones!", FALSE, ch, weapon, NULL, TO_ROOM);

	COMBAT_PT_UPDATE(ch);

	wait = (combat_moves[subcmd].speed * PULSE_VIOLENCE) / 5;

	// A value of -1 indicates that this was called by the profile.
	if (cmd != -1) { 
		if (wait > 0) {
			WAIT_STATE(ch, wait);
		}
	}
}


// A feint maneuver for fighting.
ACMD(do_feint)
{
	Character *vict = NULL;
	unsigned long wait;
	SInt16 nextattack = 0;
	SInt16 skillnum = 0, diceroll;
	SInt8 i = 0;
	SInt8 prob, feintprob = 0;
	int prevhitroll;

	vict = combat_assertion(ch, argument, subcmd, skillnum, prob);

	if (vict == NULL)
		return;

	skillnum = combat_moves[subcmd].skillnum;

	while (i < 8) {
		// Grab the next offensive move.
		nextattack = ch->combat_profile.CombatMove(COMBAT_OFFENSIVE);
		if ((nextattack == 0) || (nextattack != subcmd))
			i = 8;
		++i;
	}

	if ((nextattack == 0) || (nextattack == subcmd)) {
		ch->Send("You flail about and look quite silly, feinting but not attacking.\r\n");
		act("$n flails about wildly, attempting to feint!", TRUE, ch, 0, vict, TO_ROOM);
		return;
		}
	wait = (combat_moves[subcmd].speed * PULSE_VIOLENCE) / 10;

	diceroll = CHAR_SKILL(ch).RollSkill(ch, skillnum);
	CHAR_SKILL(ch).RollToLearn(ch, skillnum, diceroll + SKILLCHANCE(ch, skillnum, NULL));

	if (diceroll >= 0) {
		feintprob = 2;
		ch->MergeSend("&c+&cWYou feint!&c-	");
		// act("$n feints!", TRUE, ch, 0, 0, TO_ROOM);
		vict->combat_profile.defense_pts -= 2;
	}

	// This seems awfully hackish to me.	Oh, what the hell.
	GET_HITROLL(ch) += feintprob;

	// The subroutine should test for the -1 cmd variable
	if (*combat_moves[nextattack].command_pointer != NULL) {
		(*combat_moves[nextattack].command_pointer) // Perform the attack!
		(ch, "", -1, combat_moves[nextattack].move, 
		combat_moves[nextattack].subcmd);
	}

	GET_HITROLL(ch) -= feintprob;

	ch->combat_profile.CombatCost(ch, COMBAT_OFFENSIVE, combat_moves[subcmd].speed);

	// A value of -1 indicates that this was called by the profile.
	if (cmd != -1) { 
		if (combat_moves[subcmd].speed > 0) {
			WAIT_STATE(ch, wait);
		}
	}
}


// A circle maneuver for fighting.
ACMD(do_circle)
{
	Character *vict;
	SInt16 nextattack = 0, diceroll;
	SInt8 i = 0;
	SInt8 temp_defense = 0;
	Attack attack;

	vict = combat_assertion(ch, argument, subcmd, attack.skillnum, attack.chance);
	attack.skillnum = SKILL_CIRCLE;
	attack.message = SKILL_CIRCLE;
					 
	if (vict == NULL)
		return;

	diceroll = CHAR_SKILL(ch).RollSkill(ch, attack.skillnum);
	CHAR_SKILL(ch).RollToLearn(ch, attack.skillnum, diceroll + SKILLCHANCE(ch, attack.skillnum, NULL));

	if (diceroll < 0) {
		Combat::AttackMessage(ch, vict, &attack);
		return;
	} else {
		attack.succeed_by = 1;
		Combat::AttackMessage(ch, vict, &attack);
	}

	while (i < 8) {
		// Grab the next offensive move.
		nextattack = ch->combat_profile.CombatMove(COMBAT_OFFENSIVE);
		if ((nextattack == 0) || (nextattack != subcmd))
			i = 8;
		++i;
	}

	if ((nextattack == 0) || (nextattack == subcmd)) {
		return;
	}
	temp_defense = vict->combat_profile.defense_pts;
	vict->combat_profile.defense_pts = -1;

	ch->combat_profile.CombatCost(ch, COMBAT_OFFENSIVE, combat_moves[subcmd].speed);

	// The subroutine should test for the -1 cmd variable
	if (*combat_moves[nextattack].command_pointer != NULL) {
		(*combat_moves[nextattack].command_pointer) // Perform the attack!
		(ch, "", -1, combat_moves[nextattack].move, 
		combat_moves[nextattack].subcmd);
	}

	vict->combat_profile.defense_pts = temp_defense;

	// A value of -1 indicates that this was called by the profile.
	if (cmd != -1) { 
		if (combat_moves[subcmd].speed > 0) {
			WAIT_STATE(ch, PULSE_VIOLENCE);
		}
	}
}


// A support routine for shield blocking and bashing.	Searches for and
// returns a pointer to an equipped shield.
Object *grab_shield(Character * ch, Attack * attackmove)
{
	SInt8 hand = WEAR_HAND_R - 1;
	Object *shield = NULL;

	do { 
		hand = find_eqtype_pos(ch, ITEM_ARMOR, hand + 1, NUM_WEARS, FALSE);
		if ((hand > 0) && CAN_WEAR(GET_EQ(ch, hand), ITEM_WEAR_SHIELD)) {
			shield = GET_EQ(ch, hand);
			attackmove->hand = hand;
			break;
		}
	} while ((hand >= 0) && (hand <= NUM_WEARS));

	return (shield);
}


// Used for special melee combat attacks
ACMD(do_melee)
{
	Character *vict;
	unsigned long wait;
	Attack *attack = NULL;
	Object *shield = NULL;
	SInt16 skillnum = 0;
	SInt8 prob = -120;

	vict = combat_assertion(ch, argument, subcmd, skillnum, prob);
					 
	if (vict == NULL)
		return;

	attack = new Attack;
	
	attack->move = subcmd;
	attack->dam = str_dam(GET_STR(ch), attack->move) + GET_DAMROLL(ch);
	prob -= (int) (GET_WEIGHT(vict) / GET_WEIGHT(ch)) * 2;
	attack->skillnum = skillnum;
	attack->chance = prob;
	attack->response = SKILLROLL(vict, SKILL_DODGE);
	attack->speed = combat_moves[attack->move].speed;
	attack->message = combat_moves[attack->move].message;

	// CHANGEPOINT : pass die roll, not difference!
	CHAR_SKILL(vict).RollToLearn(vict, SKILL_DODGE, attack->response);
	
	wait = (attack->speed * PULSE_VIOLENCE) / 10;
	
	// Some special adjustments made before the attack.
	switch (attack->move) {
	case Combat::Bash:
		break;
	case Combat::ShieldBash:
		shield = grab_shield(ch, attack);
		if (shield == NULL) {
			ch->Send("You have no shield to bash with!\r\n");
			return;
		}
		attack->dam += GET_OBJ_VAL(shield, 0) + GET_OBJ_WEIGHT(shield);
		attack->damtype = GET_OBJ_VAL(shield, 1);
		attack->mod = GET_OBJ_VAL(shield, 2) + GET_OBJ_WEIGHT(shield);
		break;
	case Combat::Flip:
		if (find_eqtype_pos(ch, ITEM_WEAPON, WEAR_HAND_R, NUM_WEARS, FALSE) >= 0)
			attack->chance /= 2;
		if (attack->chance > 3)
			attack->dam += attack->chance / 3;
		break;
	}

	attack->succeed_by = CHAR_SKILL(ch).RollSkill(ch, attack->skillnum, attack->chance);
	CHAR_SKILL(ch).RollToLearn(ch, attack->skillnum, attack->chance + attack->succeed_by);

	ch->combat_profile.CombatCost(ch, COMBAT_OFFENSIVE, combat_moves[attack->move].speed);

	// CHANGEPOINT:	This isn't quite the elegance I hoped for, but oh well. -DH
	if (attack->response >= 0)
		attack->succeed_by = -1;

	switch (attack->move) {
	case Combat::ShieldBash:
	case Combat::Bash:
		if (attack->succeed_by >= 0) {
			attack->dam = MAX(attack->dam - GET_DR(vict), 1);
			vict->Damage(ch, attack);
			if (GET_POS(vict) > POS_SITTING)
				GET_POS(vict) = POS_SITTING;
			WAIT_STATE(vict, PULSE_VIOLENCE);
		} else { // Ooops, we failed.
			attack->dam = 0;
			vict->Damage(ch, attack);
			GET_POS(ch) = POS_SITTING;
		}
		break;
	case Combat::Flip:
		if (attack->succeed_by >= 0) {
			attack->dam = MAX(attack->dam - GET_DR(vict), 1);
			vict->Damage(ch, attack);
			if (GET_POS(vict) > POS_RESTING)
				GET_POS(vict) = POS_RESTING;
			WAIT_STATE(vict, PULSE_VIOLENCE * 2);
		} else { // Ooops, we failed.
			attack->dam = 0;
			vict->Damage(ch, attack);
			GET_POS(ch) = POS_RESTING;
			wait += wait / 2;
		}
		break;
	}

	// A value of -1 indicates that this was called by the profile.
	if (cmd != -1) { 
		if (attack->speed > 0) {
			WAIT_STATE(ch, wait + PULSE_VIOLENCE);
		}
	}
	if (attack)
		delete attack;

}


// Used for barehanded and similar combat moves like kick, punch, etc.
ACMD(do_combat)
{
	Character *vict;
	unsigned long wait;
	Attack *attackmove = NULL;
	SInt16 skillnum = 0;
	SInt8 prob, eqslot = -1;

	vict = combat_assertion(ch, argument, subcmd, skillnum, prob);
					 
	if (vict == NULL)
		return;

	attackmove = new Attack;
	
	attackmove->move = subcmd;
	attackmove->dam = str_dam(GET_STR(ch), attackmove->move) + GET_DAMROLL(ch);
	attackmove->skillnum = skillnum;
	attackmove->chance = prob;
	attackmove->message = skillnum;

	if (!attackmove->WeaponMove(ch, subcmd)) { // No weapon?
		if (IS_NPC(ch)) {
		// if ((IS_NPC(ch)) && ((message == TYPE_HIT) || (message == TYPE_PUNCH))) {
			attackmove->message = TYPE_HIT + GET_ATTACKTYPE(ch);
			switch (attackmove->message) {
			case TYPE_CLAW:
				attackmove->damtype = Damage::Slashing;
				break;
			case TYPE_MAUL:
				attackmove->damtype = Damage::Piercing;
				break;
			case TYPE_BITE:
				attackmove->damtype = Damage::Slashing;
				break;
			}
		}
	}

	
	wait = (attackmove->speed * PULSE_VIOLENCE) / 5;

	// Some special adjustments made before the attack.
	switch (attackmove->move) {
	case Combat::Punch:
		attackmove->dam /= 2;
		eqslot = find_eqtype_pos(ch, ITEM_ARMOR, WEAR_HANDS, WEAR_HANDS_2, TRUE);
		if (eqslot > -1) {
			attackmove->dam += GET_OBJ_VAL(GET_EQ(ch, eqslot), 0);
			attackmove->damtype = MAX(GET_OBJ_VAL(GET_EQ(ch, eqslot), 1), Damage::Basic);
			// attackmove->message = GET_OBJ_VAL(GET_EQ(ch, eqslot), 2) + TYPE_HIT;
		}
#ifdef GOT_RID_OF_IT	 // This is redundant to code in WeaponMove, and cause anomalous messages for headbutt
		 else {
			if (IS_NPC(ch)) {
				attackmove->message = TYPE_HIT + ch->mob_specials.attack_type;
				switch (attackmove->message) {
				case TYPE_CLAW:
					attackmove->damtype = Damage::Slashing;
					break;
				}
			}
		}
#endif
		break;
	case Combat::IronFist:
		if (attackmove->chance > 3)
			attackmove->dam += attackmove->chance;
		if (attackmove->weapon)
			attackmove->chance /= 2;
		break;
	case Combat::Headbutt:
		if (GET_EQ(ch, WEAR_HEAD)) {
			attackmove->dam += GET_DR(GET_EQ(ch, WEAR_HEAD));

			if (GET_OBJ_TYPE(GET_EQ(ch, WEAR_HEAD)) == ITEM_ARMOR) {
				attackmove->dam += GET_OBJ_VAL(GET_EQ(ch, WEAR_HEAD), 0);
				attackmove->damtype = MAX(GET_OBJ_VAL(GET_EQ(ch, WEAR_HEAD), 1), Damage::Basic);
			}
		}
		break;
	case Combat::Kick:
		if (GET_EQ(ch, WEAR_FEET)) {
			attackmove->dam += GET_DR(GET_EQ(ch, WEAR_FEET));

			if (GET_OBJ_TYPE(GET_EQ(ch, WEAR_FEET)) == ITEM_ARMOR) {
				attackmove->dam += GET_OBJ_VAL(GET_EQ(ch, WEAR_FEET), 0);
				attackmove->damtype = MAX(GET_OBJ_VAL(GET_EQ(ch, WEAR_FEET), 1), Damage::Basic);
			}
		}
		break;

	}

	ch->Hit(vict, attackmove);

	if (PURGED(ch) || PURGED(vict)) {
		if (attackmove)		delete attackmove;
		return;
	}

	// Special effects based on the success of the move.
	switch (attackmove->move) {
	case Combat::Kick:
	case Combat::KarateKick:
		if (attackmove->succeed_by <= SKILL_CRIT_FAILURE) {
			ch->Send("It's no good!	You fall clumsily to the ground!\r\n");
			act("$n falls clumsily to the ground!", FALSE, ch, 0, vict, TO_ROOM);
			wait *= 2;
			GET_POS(ch) = POS_SITTING;
		} else if ((attackmove->succeed_by >= SKILL_CRIT_SUCCESS) && 
							(attackmove->response < 0)) {
			if (GET_WEIGHT(vict) <= (GET_STR(ch) * 15)) {
				act("You really floored $N with that kick!", 0, ch, 0, vict, TO_CHAR);
				vict->Send("Oof!  That kick really floored you!\r\n");
				act("$n's kick knocks $N off $S feet!", FALSE, ch, 0, vict, TO_NOTVICT);
				if (GET_POS(vict) > POS_SITTING)
					GET_POS(vict) = POS_SITTING;
				WAIT_STATE(vict, PULSE_VIOLENCE);
			}
			if (attackmove->move == Combat::KarateKick) {
				CHAR_SKILL(ch).RollToLearn(ch, SKILL_KARATE, attackmove->succeed_by + prob);
			} else {
				CHAR_SKILL(ch).RollToLearn(ch, SKILL_BRAWLING, attackmove->succeed_by + prob);
			}
		}
		break;
	case Combat::Punch:
	case Combat::KarateChop:
		if ((attackmove->succeed_by >= 0) && (attackmove->response < 0) && 
				(attackmove->dam > 2) && 
				((attackmove->location == WEAR_HEAD) || 
				(attackmove->location == WEAR_EYES) ||
				(attackmove->location == WEAR_BODY) ||
				(attackmove->location == WEAR_FACE)) && 
				(SKILLROLL(vict, STAT_CON) < 0)) {
			act("Your punch knocks the daylights out of $N!", 0, ch, 0, vict, TO_CHAR);
			act("$n's punch knocks the daylights out of you!", 0, ch, 0, vict, TO_VICT);
			act("$n's punch knocks the daylights out of $N!", FALSE, ch, 0, vict, TO_NOTVICT);
			if (GET_POS(vict) > POS_RESTING)
				GET_POS(vict) = POS_RESTING;
			AlterMove(vict, 10);
			WAIT_STATE(vict, wait * 2);
			if (attackmove->move == Combat::KarateChop) {
				CHAR_SKILL(ch).RollToLearn(ch, SKILL_KARATE, attackmove->succeed_by + prob);
			} else {
				CHAR_SKILL(ch).RollToLearn(ch, SKILL_BRAWLING, attackmove->succeed_by + prob);
			}
		}
		break;
	case Combat::IronFist:
		if ((attackmove->succeed_by >= 0) && (attackmove->response < 0) && 
				(attackmove->dam > 2) && 
				((attackmove->location == WEAR_HEAD) || 
				(attackmove->location == WEAR_EYES) ||
				(attackmove->location == WEAR_BODY) ||
				(attackmove->location == WEAR_FACE))) {
			act("Your iron fist knocks the daylights out of $N!", 0, ch, 0, vict, TO_CHAR);
			act("$n's iron fist knocks the daylights out of you!", 0, ch, 0, vict, TO_VICT);
			act("$n's iron fist knocks the daylights out of $N!", FALSE, ch, 0, vict, TO_NOTVICT);
			if (GET_POS(vict) > POS_RESTING)
				GET_POS(vict) = POS_RESTING;
			AlterMove(vict, 15);
			WAIT_STATE(vict, wait * 2);
			CHAR_SKILL(ch).RollToLearn(ch, SKILL_KARATE, attackmove->succeed_by);
		}
		break;
	case Combat::Headbutt:
		if (attackmove->dam > 6) {
			SInt8 ch_conroll = GET_CON(ch) - GET_DR(vict);
			SInt8 vict_conroll = GET_CON(vict) - GET_DR(ch);

			if (GET_EQ(vict, attackmove->location))
				ch_conroll -= GET_DR(GET_EQ(vict, attackmove->location));
			if (((attackmove->location == WEAR_HEAD) ||
			(attackmove->location == WEAR_EYES) ||
			(attackmove->location == WEAR_FACE)) &&
			GET_EQ(ch, WEAR_HEAD))
					vict_conroll -= GET_DR(GET_EQ(ch, WEAR_HEAD));
			if (CHAR_SKILL(vict).RollSkill(vict, STAT_CON, vict_conroll) < 0) {
				vict->Send("Ouch!  That headbutt knocked the daylights out of you!\r\n");
				act("That headbutt was a bit much for $n!", FALSE, vict, 0, 0, TO_ROOM);
				WAIT_STATE(vict, wait * 2);
				CHAR_SKILL(ch).LearnSkill(SKILL_BRAWLING, NULL, 1);
			}
			if (GET_EQ(vict, attackmove->location))
				ch_conroll -= GET_DR(GET_EQ(vict, attackmove->location));
			if (CHAR_SKILL(ch).RollSkill(ch, STAT_CON, ch_conroll) < 0) {
				wait *= 2;
				ch->Send("Ouch!	What a pounding headache!\r\n");
				act("That headbutt was a bit much for $n!", FALSE, ch, 0, 0, TO_ROOM);
			}
		}

		break;
	}

	// A value of -1 indicates that this was called by the profile.
	if (cmd != -1) { 
		if (attackmove->speed > 0) {
			WAIT_STATE(ch, wait);
		}
	}
	if (attackmove)
		delete attackmove;

}


// While do_hit could handle this, there'd be the overhead of processing
// arguments.	This is designed for the combat profile, to be a faster, leaner
// alternative for combat moves related to weapons.
ACMD(do_backstab)
{
	Attack *attackmove = NULL;
	Character *vict = NULL;
	Object *weapon = NULL;
	SInt16 skillnum = 0;
	SInt16 diceroll = -120;
	SInt8 hand = WEAR_HAND_R - 1, temphand = -1;
	SInt8 prob = SKILL_NOCHANCE;


	do { 
		hand = find_eqtype_pos(ch, ITEM_WEAPON, hand + 1, WEAR_HAND_4, FALSE);
		if ((hand > 0) && ((GET_OBJ_VAL(GET_EQ(ch, hand), 0) == PROFICIENCY_KNIFE) || 
			 (GET_OBJ_VAL(GET_EQ(ch, hand), 0) == PROFICIENCY_SHORTSWORD))) {
			weapon = GET_EQ(ch, hand);
			temphand = hand;
			break;
		}
	} while ((hand >= 0) && (hand <= NUM_WEARS));

	if (!weapon) {
		ch->Send("You must wield a piercing weapon for this to work!\r\n");
		return;
	}

	vict = combat_assertion(ch, argument, subcmd, skillnum, prob);

	// We actually still want these zeroed out so they are set properly later.
	prob = SKILL_NOCHANCE;
		
	if (vict == NULL)
		return;

	attackmove = new Attack;

	attackmove->hand = temphand;
	attackmove->weapon = weapon;

	switch (temphand) {
	case WEAR_HAND_R: 
		attackmove->skillnum = SKILL_WEAPON_R; 
		attackmove->move = Combat::ThrustRight;
		break;
	case WEAR_HAND_L: 
		attackmove->skillnum = SKILL_WEAPON_L; 
		attackmove->move = Combat::ThrustLeft;
		break;
	case WEAR_HAND_3: 
		attackmove->skillnum = SKILL_WEAPON_3; 
		attackmove->move = Combat::ThrustThird;
		break;
	case WEAR_HAND_4: 
		attackmove->skillnum = SKILL_WEAPON_4; 
		attackmove->move = Combat::ThrustFourth;
		break;
	}

	attackmove->dam = str_dam(GET_STR(ch), Combat::ThrustRight) + GET_DAMROLL(ch);

	if (!attackmove->WeaponMove(ch, attackmove->move)) { // No weapon?
		delete attackmove;
		ch->Send("You must wield a piercing weapon for this to work!\r\n");
		return;
	}

	if (GET_POS(vict) <= POS_SLEEPING) {
		attackmove->mod = 100;
		attackmove->location = 6;
	} else if (IS_NPC(vict) && MOB_FLAGGED(vict, MOB_AWARE)) {
		diceroll = -120;
	} else {
		diceroll = CHAR_SKILL(ch).RollSkill(ch, SKILL_BACKSTAB);

		if (diceroll > -10) {
			if (AFF_FLAGGED(ch, AffectBit::Sneak)) {
				diceroll += 2;
			}
			if (FIGHTING(vict) && FIGHTING(vict) != ch) {
				diceroll += 2;
			}
		}


		if (diceroll >= 0) {
			CHAR_SKILL(ch).RollToLearn(ch, SKILL_BACKSTAB, diceroll);
			attackmove->location = 6;
			attackmove->mod = 100;
		}
	}

	attackmove->succeed_by = diceroll;
	attackmove->message = SKILL_BACKSTAB;
	attackmove->dam *= MAX(attackmove->chance / 2, 2);

	ch->Hit(vict, attackmove);

	if (PURGED(ch) || PURGED(vict)) {
		if (attackmove)		delete attackmove;
		return;
	}

	if ((attackmove->response < 0) || (attackmove->succeed_by >= 9)) {
		CHAR_SKILL(ch).RollToLearn(ch, SKILL_BACKSTAB, diceroll);
	}

	if (cmd != -1) // Was this called by the Combat Profile?
		WAIT_STATE(ch, PULSE_VIOLENCE);

	delete attackmove;
}


// While do_hit could handle this, there'd be the overhead of processing
// arguments.	This is designed for the combat profile, to be a faster, leaner
// alternative for combat moves related to weapons.
ACMD(do_weapon)
{
	Attack *attackmove = NULL;
	Character *fighting = NULL;

	if (FIGHTING(ch) == NULL) // A little timesaver to prevent what would be a blank attack.
		return;

	attackmove = new Attack;

	attackmove->move = subcmd;
	attackmove->dam = str_dam(GET_STR(ch), attackmove->move) + GET_DAMROLL(ch);

	// WeaponMove already takes care of this.	This is redundant.
	// if (!attackmove->WeaponMove(ch, subcmd)) { // No weapon?
	//	 delete attackmove;
	//	 do_combat(ch, "punch", "", -1, Combat::Punch);
	//	 return;
	// }

	if (!attackmove->WeaponMove(ch, subcmd)) { // No weapon?
		if (IS_NPC(ch)) {
		// if ((IS_NPC(ch)) && ((message == TYPE_HIT) || (message == TYPE_PUNCH))) {
			attackmove->message = TYPE_HIT + GET_ATTACKTYPE(ch);
			switch (attackmove->message) {
			case TYPE_CLAW:
				attackmove->damtype = Damage::Slashing;
				break;
			case TYPE_MAUL:
				attackmove->damtype = Damage::Piercing;
				break;
			case TYPE_BITE:
				attackmove->damtype = Damage::Slashing;
				break;
			}
		}
	}

	attackmove->chance = SKILLCHANCE(ch, attackmove->skillnum, NULL);

	fighting = FIGHTING(ch);
	ch->Hit(FIGHTING(ch), attackmove);

	if (PURGED(ch) || PURGED(fighting)) {
		delete attackmove;
		return;
	}

#if 0
		sprintf(buf, "%s doing combat move %d (%s), success %d, response %d", ch->Name(), subcmd, combat_moves[subcmd].move,
				attackmove->succeed_by, attackmove->response);
		ch->Inside()->Echo(buf);
#endif

	if (cmd != -1) // Was this called by the Combat Profile?
		WAIT_STATE(ch, PULSE_VIOLENCE);

	delete attackmove;
}


// This is not meant to be used outside of the combat profile.
ACMD(do_riposte)
{
	SInt16 i = 0, nextattack = 0;
	SInt8 hand;

	switch (subcmd) {
	case Combat::RiposteFourth:
		hand = WEAR_HAND_4;
		nextattack = Combat::SwingFourth;
		break;
	case Combat::RiposteThird:
		hand = WEAR_HAND_3;
		nextattack = Combat::SwingThird;
		break;
	case Combat::RiposteLeft:
		hand = WEAR_HAND_L;
		nextattack = Combat::SwingLeft;
		break;
	case Combat::RiposteRight:
		hand = WEAR_HAND_R;
		nextattack = Combat::SwingRight;
		break;
	}

	if (swing_or_thrust(ch, hand)) {
		++nextattack;
	}

	ch->combat_profile.ZeroPoints(COMBAT_DEFENSIVE);
	do_weapon(ch, "", -1, "", combat_moves[nextattack].subcmd);
}


ACMD(do_berserk)
{
	SInt16 diceroll;
	SInt8 chance;
	Affect *af;
	struct skill_store *i = NULL;

	if (Affect::AffectedBy(ch, SKILL_BERSERK)) {
		ch->Send("You already are.\r\n");
		return;
	}

	chance = SKILLCHANCE(ch, SKILL_BERSERK, NULL);

	if (chance == SKILL_NOCHANCE) {
		ch->Send("Don't you think you should learn the skill first?\r\n");
		return;
	}

	diceroll = CHAR_SKILL(ch).RollSkill(ch, SKILL_BERSERK, chance);
	CHAR_SKILL(ch).RollToLearn(ch, SKILL_BERSERK, diceroll);

	if (diceroll < 0) {
		ch->Send("You fail.\r\n");
		AlterMove(ch, 10);
		WAIT_STATE(ch, PULSE_VIOLENCE - 1);
	} else {
		ch->Send("You seethe momentarily, then lose your senses.  Nothing can stop you!\r\n");
		act("$n flies into a rage!  You see fire in $s eyes!\r\n", TRUE, ch, 0, 0, TO_ROOM);
		AlterMove(ch, 15);
		WAIT_STATE(ch, PULSE_VIOLENCE - 1);
		
		af = new Affect(SKILL_BERSERK, 0, Affect::None, AffectBit::Berserk);
		af->Join(ch, PULSES_PER_MUD_HOUR, false, false, false, false);
	}
}


/* utility function for finding position weapon to disarm */
int find_disarm_target(Character *ch, Character *vict)
{
	int location = -1;

	if (GET_EQ(vict, WEAR_HAND_R) &&
			(GET_OBJ_TYPE(GET_EQ(vict, WEAR_HAND_R)) == ITEM_WEAPON) &&
			CAN_SEE_OBJ(vict, GET_EQ(vict, WEAR_HAND_R)))
		location = WEAR_HAND_R;
	else if (GET_EQ(vict, WEAR_HAND_L) &&
		 (GET_OBJ_TYPE(GET_EQ(vict, WEAR_HAND_L)) == ITEM_WEAPON) &&
		 CAN_SEE_OBJ(vict, GET_EQ(vict, WEAR_HAND_L)))
		location = WEAR_HAND_L;

	return location;
}


ACMD(do_disarm)
{
	Character *vict;
	Object *weapon;
	int modifier = 0, location;
	SInt16 diceroll;
	SInt8 prev_encumbrance = 0, encumbrance;
	unsigned long wait;

	one_argument(argument, arg);

	/* find the victim */
	if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
		if (FIGHTING(ch) && (IN_ROOM(FIGHTING(ch)) == IN_ROOM(ch))) {
			vict = FIGHTING(ch);
		} else {
			ch->Send("Disarm who?\r\n");
			return;
		}
	}
	if (vict == ch) {
		ch->Send("Try remove weapon.  It works better.\r\n");
		return;
	}

	/* check if victim is armed */
	if ((location = find_disarm_target(ch, vict)) < 0) {
		act("$N isn't armed!", FALSE, ch, NULL, vict, TO_CHAR);
		return;
	}
	if (!Combat::Valid(ch, vict))
		return;

	prev_encumbrance = GET_ENCUMBRANCE(vict, IS_CARRYING_W(vict));

	modifier = MAX(SKILLROLL(vict, SKILL_COMBAT_TECHNIQUE), 0);

	diceroll = SKILLROLL(ch, SKILL_DISARM);
	CHAR_SKILL(ch).RollToLearn(ch, SKILL_DISARM, diceroll);

	if ((diceroll - modifier) >= 0) {
		weapon = unequip_char(vict, location);
		weapon->ToRoom(IN_ROOM(vict));
		act("You send $N's $o flying from $S grasp!", FALSE, ch, weapon, vict, TO_CHAR);
		act("$n sends your $o flying from your grasp!", FALSE, ch, weapon, vict, TO_VICT);
		act("$n sends $N's $o flying from $S grasp!", FALSE, ch, weapon, vict, TO_NOTVICT);
	} else {
		act("You are unable to disarm $N.", FALSE, ch, NULL, vict, TO_CHAR);
		act("$n tries unsuccessfully to disarm you!", FALSE, ch, NULL, vict, TO_VICT);
		act("$n tries unsuccessfully to disarm $N.", TRUE, ch, NULL, vict, TO_NOTVICT);		
	}

	// A value of -1 indicates that this was called by the profile.
	if (cmd != -1) { 
		wait = (combat_moves[Combat::Disarm].speed * PULSE_VIOLENCE);
		WAIT_STATE(ch, wait + PULSE_VIOLENCE);
	}

	encumbrance = GET_ENCUMBRANCE(vict, IS_CARRYING_W(vict));
	if (prev_encumbrance != encumbrance)
		vict->Send(encumbrance_shift[encumbrance][(encumbrance < prev_encumbrance)]);

	// Changepoint:
	// WAIT_STATE(ch, Skill::Skills[SKILL_DISARM].lag * (PULSE_VIOLENCE / 5));
	if (!FIGHTING(ch))
		ch->Fight(vict);
}


ACMD(do_aim)
{ 
	Character *target = NULL;
	int exception = FALSE, dir = -1;

	target = Ranged::AcquireTarget(ch, argument, dir, exception);
					
	if (target) {
		if (target == ch) {
			ch->Send("No!  Not a good idea!\r\n");
			return;
		}
		act("You take aim at $N!", FALSE, ch, NULL, target, TO_CHAR);
		if (IN_ROOM(ch) == IN_ROOM(target)) {
			act("$n takes aim at you!", TRUE, ch, NULL, target, TO_VICT);
			act("$n takes aim at $N!", TRUE, ch, NULL, target, TO_NOTVICT);
		}
		AIMING(ch) = GET_ID(target);
		CHAR_WATCHING(ch) = dir + 1;
	} else {
		if (!exception)
			act("They're not here.", FALSE, ch, NULL, NULL, TO_CHAR);
		return;
	}

	WAIT_STATE(ch, PULSE_VIOLENCE);
}


ACMD(do_shoot)
{
	Character *target = NULL;
	Object *weapon = NULL, *missile = NULL;
	int exception = FALSE, dir = -1, distance = 0;
	int weaponhand = -1, missilehand = -1;
	Attack *attack = new Attack;

	if (AIMING(ch)) {
		target = find_char(AIMING(ch));
		if (!target) {
			AIMING(ch) = 0;
			ch->Send("They're not around.\r\n");
			return;
		}
		dir = find_first_step(IN_ROOM(ch), IN_ROOM(target), 0);
	} else {
		target = Ranged::AcquireTarget(ch, argument, dir, exception);
		if ((!target) && (!exception)) {
			act("You don't see such a person.", FALSE, ch, NULL, NULL, TO_CHAR);
			return;
		}
		attack->chance -= 4;
	}

	if (!target) {
		if (!exception)
			act("They're not here.", FALSE, ch, NULL, NULL, TO_CHAR);
				return;
		}

	distance = Ranged::DistanceToTarget(ch, target, 3, dir);
	if (!distance) {
		ch->Send("They seem to have wandered off!\r\n");
				return;
			}
	// ch->Sendf("Distance: %d	Dir: %s\r\n", distance, (dir >= 0 ? thedirs[dir] : "NONE"));
	
	// First, find the firing weapon.
	weaponhand = find_eqtype_pos(ch, ITEM_FIREWEAPON, WEAR_HAND_R, NUM_WEARS, FALSE);
	if (weaponhand >= 0)
		weapon = GET_EQ(ch, weaponhand);
	else {
		act("You must wield a fire weapon and hold a missile.", FALSE, ch, NULL, NULL, TO_CHAR);
		return;
	}

	// Is this weapon loadable?
	if (GET_OBJ_VAL(weapon, 4) == 0) {	// No, ammo is handheld
		missilehand = find_eqtype_pos(ch, ITEM_MISSILE, WEAR_HAND_R, NUM_WEARS, FALSE);
		if (missilehand >= 0) {
			missile = GET_EQ(ch, missilehand);
		} else {
			START_ITER(iter, missile, Object *, ch->contents) {
				if ((GET_OBJ_TYPE(missile) == ITEM_MISSILE) && 
				   (GET_OBJ_VAL(weapon, 0) == GET_OBJ_VAL(missile, 0)))
				break;
			}

			if (!missile)	missile = Ranged::GrabMissile(ch, weapon);
		}
	
		if ((!weapon) || (!missile) || (GET_OBJ_VAL(weapon, 0) != GET_OBJ_VAL(missile, 0))) {
			act("You must wield a fire weapon and hold a missile.", FALSE, ch, NULL, NULL, TO_CHAR);
			return;
		}
		attack->weapon = missile;

		if (missilehand >= 0) {
			unequip_char(ch, missilehand);
		} else {
			missile->FromChar();
		}
	} else {	// Weapon is loadable, check for rounds.
		if (GET_OBJ_VAL(weapon, 5) <= 0) {
			act("Your weapon seems to be spent.", FALSE, ch, NULL, NULL, TO_CHAR);
			return;
		}
		attack->weapon = weapon;
		attack->speed = GET_OBJ_VAL(weapon, 7);
	}

	attack->skillnum = GET_OBJ_VAL(attack->weapon, 0);
	attack->dam = GET_DAMROLL(ch);

	switch (attack->skillnum) {
	case PROFICIENCY_CROSSBOW: 
		attack->message = TYPE_BOLT; 
		break;
	case PROFICIENCY_SLING: 
		attack->message = TYPE_ROCK; 
		attack->dam += str_dam(GET_STR(ch), Combat::SwingRight);
		break;
	case PROFICIENCY_GUN: 
		attack->message = TYPE_BLAST; 
		break;
	case PROFICIENCY_BEAMGUN: 
		attack->message = TYPE_BLAST; 
		break;
	case PROFICIENCY_THROW: 
		attack->message = TYPE_ROCK; 
		attack->dam += str_dam(GET_STR(ch), Combat::SwingRight);
		break;
	default:
	case PROFICIENCY_BOW: 
		attack->message = TYPE_ARROW; 
		attack->dam += str_dam(GET_STR(ch), Combat::SwingRight);
		break;
	}

	
	if (weapon)
		attack->dam += GET_OBJ_VAL(weapon, 2);
	if (missile) {
		attack->dam += GET_OBJ_VAL(missile, 2);
		attack->dam += GET_OBJ_WEIGHT(missile);
		attack->damtype = GET_OBJ_VAL(missile, 3);
	}

	attack->chance = SKILLCHANCE(ch, attack->skillnum, NULL);
	attack->mod = -distance;

	switch (attack->skillnum) {
	case PROFICIENCY_GUN: 		// We do messages here since skillnums are taken care of,
	case PROFICIENCY_BEAMGUN: 		// and we don't have message types from these object's values.
		attack->message = TYPE_BLAST;	
		break;				
	case PROFICIENCY_BOW: 
		attack->message = PROFICIENCY_BOW;	
		break;
	case PROFICIENCY_CROSSBOW: 
		attack->message = PROFICIENCY_CROSSBOW;	
		break;
	case PROFICIENCY_SLING: 
		attack->message = PROFICIENCY_SLING;	
		break;
	default: 
		break;
	}

	switch (GET_OBJ_TYPE(attack->weapon)) {
	case ITEM_FIREWEAPON: 
		if (GET_OBJ_VAL(attack->weapon, 5) > 0) {
			--GET_OBJ_VAL(attack->weapon, 5);
		}
		break;
	}

	if (Ranged::FireMissile(ch, target, attack, dir) && missile) {
		if (attack->succeed_by >= 0) {
			switch (GET_OBJ_TYPE(missile)) {
			case ITEM_MISSILE:
				if (!Number(0, 3)) {
					missile->Extract();
					break;
				}
			default:
				if (!PURGED(target))	missile->ToChar(target);
				break;
			}
		} else {
			switch (GET_OBJ_TYPE(missile)) {
			case ITEM_MISSILE:
			if (Number(0, 2)) {
				missile->Extract();
			break;
			}
			default:
			missile->ToRoom(IN_ROOM(target));
			}
		}
	}

	delete attack;
}


ACMD(do_throw)
{
	Character *target = NULL;
	Object *missile = NULL;
	char arg1[MAX_INPUT_LENGTH], trailer[MAX_INPUT_LENGTH];
	int missilehand = -1;
	int exception = FALSE, dir = -1, distance = 0;
	Attack *attack = new Attack;

	half_chop(argument, arg1, trailer);

	if (*arg1) {
		missile = get_object_in_equip_vis(ch, arg1, ch->equipment, &missilehand);
		if (!missile) {
			missilehand = -1; // The above search left missilehand at the last place it searched.	We need to compensate.
			missile = get_obj_in_list_vis(ch, arg1, ch->contents);
		}
	} else {
		missilehand = find_eqtype_pos(ch, ITEM_GRENADE, WEAR_HAND_R, NUM_WEARS, FALSE);
		if (missilehand >= 0)
			missile = GET_EQ(ch, missilehand);
		if (!missile) {
			ch->Send("You must specify what you want to throw.\r\n");
			return;
		}
	}
		
	if (!missile)	{
		ch->Sendf("You don't seem to have %s %s.\r\n", AN(arg), arg1);
		return;
	}

	if (AIMING(ch)) {
		target = find_char(AIMING(ch));
		if (!target) {
			AIMING(ch) = 0;
			ch->Send("They're not around.\r\n");
			return;
		}
		dir = find_first_step(IN_ROOM(ch), IN_ROOM(target), 0);
	} else {
		target = Ranged::AcquireTarget(ch, trailer, dir, exception);
		if ((!target) && (!exception)) {
			act("You don't see them around.", FALSE, ch, NULL, NULL, TO_CHAR);
			return;
		}
		attack->mod -= 4;
	}
	

	if (!target) {
		if (!exception)		act("They're not here.", FALSE, ch, NULL, NULL, TO_CHAR);
		return;
	}

	distance = Ranged::DistanceToTarget(ch, target, 1, dir);
	if (!distance) {
		ch->Send("They seem to have wandered off!\r\n");
		return;
	}

	attack->weapon = missile;
	switch (GET_OBJ_TYPE(missile)) {
	case ITEM_MISSILE:
		attack->skillnum = GET_OBJ_VAL(missile, 0); 
		attack->damtype = GET_OBJ_VAL(missile, 3);
		attack->dam += GET_OBJ_VAL(missile, 2);
		break;
	case ITEM_WEAPON:
		attack->skillnum = GET_OBJ_VAL(missile, 0);
		attack->damtype = GET_OBJ_VAL(missile, 5);
		attack->message = GET_OBJ_VAL(missile, 6);
		attack->dam += GET_OBJ_VAL(missile, 4);
		break;
	default:
		attack->skillnum = STAT_DEX;
		attack->mod -= 4;
		attack->message = TYPE_POUND;
		attack->dam -= 10;
		break;
	}

	switch (attack->skillnum) {
	case PROFICIENCY_KNIFE: 
		attack->skillnum = PROFICIENCY_KNIFETHROW; 
		break;
	case PROFICIENCY_SPEAR: 
		attack->skillnum = PROFICIENCY_SPEARTHROW; 
		break;
	case PROFICIENCY_AXE: 
		attack->skillnum = PROFICIENCY_AXETHROW; 
		break;
	case PROFICIENCY_GUN: 		// We do messages here since skillnums are taken care of,
	case PROFICIENCY_BEAMGUN: 		// and we don't have message types from these object's values.
		attack->message = TYPE_BLAST;	
		break;				
	case PROFICIENCY_BOW: 
		attack->message = PROFICIENCY_BOW;	
		break;
	case PROFICIENCY_CROSSBOW: 
		attack->message = PROFICIENCY_CROSSBOW;	
		break;
	case PROFICIENCY_SLING: 
		attack->message = PROFICIENCY_SLING;	
		break;
	default: 
		break;
	}

	attack->chance = SKILLCHANCE(ch, attack->skillnum, NULL);
	attack->dam += str_dam(GET_STR(ch), Combat::ThrustRight);
	attack->dam += GET_OBJ_WEIGHT(attack->weapon);

	attack->mod -= distance;

	if (missilehand >= 0) {
		unequip_char(ch, missilehand);
	} else {
		attack->weapon->FromChar();
	}

	if (Ranged::FireMissile(ch, target, attack, dir)) {
		if (attack->succeed_by >= 0) {
			switch (GET_OBJ_TYPE(attack->weapon)) {
			case ITEM_MISSILE: 
				if (!Number(0, 2)) {
					attack->weapon->Extract();
					break;
				}	// We don't hit break if the weapon is not extracted, so it is sent to the room.

			default:
				if (!PURGED(target))	attack->weapon->ToChar(target);
				break;
			}
		} else {
			switch (GET_OBJ_TYPE(attack->weapon)) {
			case ITEM_MISSILE: 
				if (Number(0, 1)) {
					attack->weapon->Extract();
					break;
				}	// We don't hit break if the weapon is not extracted, so it is sent to the room.

			default:
				attack->weapon->ToRoom(IN_ROOM(target));
				break;
			}
		}
	}

	delete attack;
}


// Daniel Rollings AKA Daniel Houghton's new combat profile

#define POSTURE_NORMAL		0
#define POSTURE_ALLOUT_DEFENSE	1
#define POSTURE_ALLOUT_OFFENSE	2
#define POSTURE_CAUTIOUS	3
#define POSTURE_OFFENSE		4

#define IS_OFFENSIVE(i) ((combat_moves[(i)].type == COMBAT_OFFENSIVE) || \
			(combat_moves[(i)].type == COMBAT_BOTH))
#define IS_DEFENSIVE(i) ((combat_moves[(i)].type == COMBAT_DEFENSIVE) || \
			(combat_moves[(i)].type == COMBAT_BOTH))

#define GET_COMBAT_MOVE(ch, i) ((ch)->combat_profile.GetMove(i))


// Really a dummy function to be overridden by scripts.
// CHANGEPOINT:	This really needs to be modifiable.
ACMD(do_surrender)
{
	Character *surrender_to = NULL;
	Affect *af;

	if (FIGHTING(ch)) {
		surrender_to = FIGHTING(ch);
	} else {
		one_argument(argument, arg);
		if (*arg)
			surrender_to = get_char_vis(ch, arg, FIND_CHAR_ROOM);
	}
	
	if (surrender_to) {
		if (AFF_FLAGGED(ch, AffectBit::Charm) && (ch->master)) {
			if (ch->master == surrender_to) {
				ch->Send("You have already placed yourself at their mercy.\r\n");
				return;
			} else {
				act("No way!  Your life for $N!", FALSE, ch, 0, ch->master, TO_CHAR);
				return;
			}
		}

		if (surrender_to == ch) {
			ch->Send("You give in to your overwhelming greatness.\r\n");
			return;
		}
		
		act("You surrender to $N.", FALSE, ch, 0, surrender_to, TO_CHAR);
		act("$n surrenders to $N!", FALSE, ch, 0, surrender_to, TO_ROOM);
		WAIT_STATE(ch, PULSE_VIOLENCE * 2);
		
		if (FIGHTING(ch) == surrender_to)
			ch->StopFighting();

		if (ch->master) {
			ch->StopFollower();
			REMOVE_BIT(AFF_FLAGS(ch), AffectBit::Group);
		}

		if (circle_follow(ch, surrender_to)) {
			surrender_to->StopFollower();
			REMOVE_BIT(AFF_FLAGS(surrender_to), AffectBit::Group);
		}

		add_follower(ch, surrender_to);

		if (!GET_TRUST(ch)) {
			af = new Affect(SPELL_CHARM, 0, Affect::None, AffectBit::Charm);
			af->Join(ch, PULSES_PER_MUD_HOUR, false, false, false, false);
		}

		if ((IS_NPC(surrender_to) ? 
				MOB_FLAGGED(surrender_to, MOB_MERCIFUL) : 
				PRF_FLAGGED(surrender_to, Preference::Merciful))) {
			if (FIGHTING(surrender_to) == ch)
				surrender_to->StopFighting();
		}

		return;
	}
	
	act("Nobody here accepts your surrender.", FALSE, ch, 0, 0, TO_CHAR);
}


int auto_defend(Character *ch)
{
	int i, movesums = 0, pick = 0, defensemove = 0;
	int movechances[100];
	
	movechances[0] = 0;

	if (ch->combat_profile.defense_pts <= 0)
		return 0;

	for (i = 1; *(combat_moves[i].move) != '\n'; ++i) {
		if (IS_DEFENSIVE(i)) {
			movechances[i] = MAX(0, combat_move_chance(ch, i));

			if (movechances[i] > 0) {
				movesums += movechances[i];
			}
		} else {
			movechances[i] = 0;
		}
	}

	pick = Number(0, movesums);
	
	for (i = 1; (!defensemove) && *(combat_moves[i].move) != '\n'; ++i) {
		if ((i == Combat::AutoAttack) || 
				(i == Combat::AutoDefend))
			continue;
		if ((combat_moves[i].skillnum == SKILL_SHIELD) && 
				(!find_eqtype_pos(ch, ITEM_ARMOR, WEAR_HAND_R, NUM_WEARS, FALSE)))
			continue;
		if (movechances[i] >= pick)
			defensemove = i;
		pick -= movechances[i];
	}
	
	return defensemove;
}


ACMD(do_auto_attack)
{
	int i, movesums = 0, pick = 0, attackmove = 0;
	int movechances[100];
	
	movechances[0] = 0;

	if (ch->combat_profile.attack_pts <= 0)
		return;

	for (i = 1; *(combat_moves[i].move) != '\n'; ++i) {
		if (IS_OFFENSIVE(i)) {
			movechances[i] = MAX(0, SKILLCHANCE(ch, combat_moves[i].skillnum, 0));

			if (movechances[i] > 0) {
				movesums += movechances[i];
			}
		} else {
			movechances[i] = 0;
		}
	}

	pick = Number(0, movesums);
	
	for (i = 1; (!attackmove) && *(combat_moves[i].move) != '\n'; ++i) {
		if ((i == Combat::AutoAttack) || (i == Combat::AutoDefend) || (i == Combat::Backstab))
			continue;
		if ((combat_moves[i].skillnum == SKILL_SHIELD) && 
				(!find_eqtype_pos(ch, ITEM_ARMOR, WEAR_HAND_R, NUM_WEARS, FALSE)))
			continue;
		if (movechances[i] >= pick)
			attackmove = i;
		pick -= movechances[i];
	}

	// The subroutine should test for the -1 cmd variable
	if (*combat_moves[attackmove].command_pointer != NULL) {
		(ch, "", -1, combat_moves[attackmove].move, 
		combat_moves[attackmove].subcmd);
	}
	
}


