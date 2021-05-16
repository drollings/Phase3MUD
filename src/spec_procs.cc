/* ************************************************************************
*	 File: spec_procs.c																	Part of CircleMUD *
*	Usage: implementation of special procedures for mobiles/objects/rooms	*
*																																				 *
*	All rights reserved.	See license.doc for complete information.				*
*																																				 *
*	Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*	CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.							 *
************************************************************************ */


#include "structs.h"
#include "utils.h"
#include "buffer.h"
#include "rooms.h"
#include "characters.h"
#include "objects.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "find.h"
#include "db.h"
#include "skills.h"
#include "spells.h"

/*	 external vars	*/
extern struct TimeInfoData time_info;
extern char *skills[];
extern int guild_info[][3];
extern struct skill_info_type skill_info[];

/* extern functions */
void add_follower(Character * ch, Character * leader);
char *fname(char *namelist);
ACMD(do_say);
ACMD(do_drop);
ACMD(do_gen_door);
extern int cast_spell(Character *ch, Character *tch, Object *tobj, SpellData *spell);


struct social_type {
	char *cmd;
	int next_line;
};

/* local functions */
const char *how_good(int percent);
void list_skills(Character * ch);
SPECIAL(guild);
SPECIAL(dump);
SPECIAL(mayor);
SPECIAL(snake);
SPECIAL(thief);
SPECIAL(magic_user);
SPECIAL(guild_guard);
SPECIAL(puff);
SPECIAL(fido);
SPECIAL(janitor);
SPECIAL(cityguard);
SPECIAL(pet_shops);
SPECIAL(bank);


SPECIAL(guild_proc);


/* ********************************************************************
*	Special procedures for mobiles																		 *
******************************************************************** */


const char *how_good(int percent) {
	if (percent == 0)			return " (not learned)";
	else if (percent <= 10)		return " (awful)";
	else if (percent <= 20)		return " (bad)";
	else if (percent <= 40)		return " (poor)";
	else if (percent <= 55)		return " (average)";
	else if (percent <= 70)		return " (fair)";
	else if (percent <= 80)		return " (good)";
	else if (percent <= 85)		return " (very good)";
	else						return " (superb)";
}


SPECIAL(dump) {
	Object *k;
	int value = 0;
	LListIterator<Object *>	iter(world[IN_ROOM(ch)].contents);

	while ((k = iter.Next())) {
		act("$p vanishes in a puff of smoke!", FALSE, 0, k, 0, TO_ROOM);
		k->Extract();
	}

	if (!CMD_IS("drop"))
		return 0;

	do_drop(ch, argument, 0, "drop", 0);

	iter.Reset();
	while((k = iter.Next())) {
		act("$p vanishes in a puff of smoke!", FALSE, 0, k, 0, TO_ROOM);
		value += RANGE(1, GET_OBJ_COST(k) / 10, 50);
		k->Extract();
	}
	return 1;
}


#if 0
SPECIAL(mayor)
{
	ACMD(do_gen_door);

	static char open_path[] =
	"W3a3003b33000c111d0d111Oe333333Oe22c222112212111a1S.";

	static char close_path[] =
	"W3a3003b33000c111d0d111CE333333CE22c222112212111a1S.";

	static char *path;
	static int index;
	static int move = FALSE;

	if (!move) {
		if (time_info.hours == 6) {
			move = TRUE;
			path = open_path;
			index = 0;
		} else if (time_info.hours == 20) {
			move = TRUE;
			path = close_path;
			index = 0;
		}
	}
	if (cmd || !move || (GET_POS(ch) < POS_SLEEPING) ||
			(GET_POS(ch) == POS_FIGHTING))
		return FALSE;

	switch (path[index]) {
	case '0':
	case '1':
	case '2':
	case '3':
		perform_move(ch, path[index] - '0', 1);
		break;

	case 'W':
		GET_POS(ch) = POS_STANDING;
		act("$n awakens and groans loudly.", FALSE, ch, 0, 0, TO_ROOM);
		break;

	case 'S':
		GET_POS(ch) = POS_SLEEPING;
		act("$n lies down and instantly falls asleep.", FALSE, ch, 0, 0, TO_ROOM);
		break;

	case 'a':
		act("$n says 'Hello Honey!'", FALSE, ch, 0, 0, TO_ROOM);
		act("$n smirks.", FALSE, ch, 0, 0, TO_ROOM);
		break;

	case 'b':
		act("$n says 'What a view!	I must get something done about that dump!'",
	FALSE, ch, 0, 0, TO_ROOM);
		break;

	case 'c':
		act("$n says 'Vandals!	Youngsters nowadays have no respect for anything!'",
	FALSE, ch, 0, 0, TO_ROOM);
		break;

	case 'd':
		act("$n says 'Good day, citizens!'", FALSE, ch, 0, 0, TO_ROOM);
		break;

	case 'e':
		act("$n says 'I hereby declare the bazaar open!'", FALSE, ch, 0, 0, TO_ROOM);
		break;

	case 'E':
		act("$n says 'I hereby declare Midgaard closed!'", FALSE, ch, 0, 0, TO_ROOM);
		break;

	case 'O':
		do_gen_door(ch, "gate", 0, SCMD_UNLOCK);
		do_gen_door(ch, "gate", 0, SCMD_OPEN);
		break;

	case 'C':
		do_gen_door(ch, "gate", 0, SCMD_CLOSE);
		do_gen_door(ch, "gate", 0, SCMD_LOCK);
		break;

	case '.':
		move = FALSE;
		break;

	}

	index++;
	return FALSE;
}
#endif

/* ********************************************************************
*	General special procedures for mobiles														 *
******************************************************************** */


SPECIAL(snake)
{
	if (cmd)
		return FALSE;

	if (GET_POS(ch) != POS_FIGHTING)
		return FALSE;

	if (FIGHTING(ch) && (IN_ROOM(FIGHTING(ch)) == IN_ROOM(ch)) &&
			(Number(0, 42 - GET_LEVEL(ch)) == 0)) {
		act("$n bites $N!", 1, ch, 0, FIGHTING(ch), TO_NOTVICT);
		act("$n bites you!", 1, ch, 0, FIGHTING(ch), TO_VICT);
/*		call_magic(ch, FIGHTING(ch), 0, SPELL_POISON, GET_LEVEL(ch), CAST_SPELL); */
		return TRUE;
	}
	return FALSE;
}



SPECIAL(magic_user)
{
	Character *vict;
	char *buf;
	SpellData *spell;

	if (cmd || GET_POS(ch) != POS_FIGHTING)
		return FALSE;

	/* pseudo-randomly choose someone in the room who is fighting me */
	START_ITER(iter, vict, Character *, world[IN_ROOM(ch)].people) {
		if (FIGHTING(vict) == ch && !Number(0, 4))
			break;
	}

	/* if I didn't pick any of those, then just slam the guy I'm fighting */
	if (vict == NULL)
		vict = FIGHTING(ch);

	spell = new SpellData;
	
	spell->mana = 0;
	spell->hp = GET_HIT(ch);
	spell->isprayer = false;
	spell->spellnum = 0;

	buf = get_buffer(20);
	sprintf(buf, "%c%d", UID_CHAR, vict ? vict->ID() : 0);
	spell->arg = buf;
	release_buffer(buf);

	if ((GET_LEVEL(ch) > 13) && (Number(0, 10) == 0))
		spell->spellnum = SPELL_SLEEP;

	if ((GET_LEVEL(ch) > 7) && (Number(0, 8) == 0))
		spell->spellnum = SPELL_BLINDNESS;

	if ((GET_LEVEL(ch) > 12) && (Number(0, 12) == 0)) {
		if (IS_EVIL(ch))
			spell->spellnum = SPELL_ENERGY_DRAIN;
		else if (IS_GOOD(ch))
			spell->spellnum = SPELL_DISPEL_EVIL;
	}
	if (Number(0, 4))
		return TRUE;

	switch (GET_LEVEL(ch)) {
	case 4:
	case 5:
		spell->spellnum = SPELL_MAGIC_MISSILE;
		break;
	case 6:
	case 7:
		spell->spellnum = SPELL_CHILL_TOUCH;
		break;
	case 8:
	case 9:
		spell->spellnum = SPELL_BURNING_HANDS;
		break;
	case 10:
	case 11:
		spell->spellnum = SPELL_SHOCKING_GRASP;
		break;
	case 12:
	case 13:
		spell->spellnum = SPELL_LIGHTNING_BOLT;
		break;
	case 14:
	case 15:
	case 16:
	case 17:
		spell->spellnum = SPELL_COLOR_SPRAY;
		break;
	default:
		spell->spellnum = SPELL_FIREBALL;
		break;
	}
	if (spell->spellnum)	cast_spell(ch, vict, NULL, spell);
	return TRUE;

}


/* ********************************************************************
*	Special procedures for mobiles																			*
******************************************************************** */

SPECIAL(puff)
{
	if (cmd)
		return (0);

	switch (Number(0, 60)) {
	case 0:
		do_say(ch, "My god!	It's full of stars!", 0, "say", 0);
		return (1);
	case 1:
		do_say(ch, "How'd all those fish get up here?", 0, "say", 0);
		return (1);
	case 2:
		do_say(ch, "I'm a very female dragon.", 0, "say", 0);
		return (1);
	case 3:
		do_say(ch, "I've got a peaceful, easy feeling.", 0, "say", 0);
		return (1);
	default:
		return (0);
	}
}


SPECIAL(fido)
{

	Object *i, *temp;

	if (cmd || !AWAKE(ch))
		return (FALSE);

	START_ITER(roomobj_iter, i, Object *, world[IN_ROOM(ch)].contents) {
		if (GET_OBJ_TYPE(i) == ITEM_CONTAINER && GET_OBJ_VAL(i, 3)) {
			act("$n savagely devours a corpse.", FALSE, ch, 0, 0, TO_ROOM);
			START_ITER(cont_iter, temp, Object *, i->contents) {
				temp->FromObj();
				temp->ToRoom(IN_ROOM(ch));
			}
			i->Extract();
			return (TRUE);
		}
	}
	return (FALSE);
}


SPECIAL(janitor)
{
	Object *i;

	if (cmd || !AWAKE(ch))
		return (FALSE);

	START_ITER(roomobj_iter, i, Object *, world[IN_ROOM(ch)].contents) {
		if (!CAN_WEAR(i, ITEM_WEAR_TAKE))
			continue;
		if (GET_OBJ_TYPE(i) != ITEM_DRINKCON && GET_OBJ_COST(i) >= 15)
			continue;
		act("$n picks up some trash.", FALSE, ch, 0, 0, TO_ROOM);
		i->FromRoom();
		i->ToChar(ch);
		return TRUE;
	}

	return FALSE;
}


SPECIAL(cityguard)
{
	Character *tch, *evil;
	int max_evil;

	if (cmd || !AWAKE(ch) || FIGHTING(ch))
		return FALSE;

	max_evil = 1000;
	evil = 0;

	START_ITER(iter, tch, Character *, world[IN_ROOM(ch)].people) {
		if (!IS_NPC(tch) && CAN_SEE(ch, tch) && PLR_FLAGGED(tch, PLR_KILLER)) {
			act("$n screams, 'Murderer!!!'", FALSE, ch, 0, 0, TO_ROOM);
			ch->Hit(tch);
			return (TRUE);
		}
	}

	return (FALSE);
}


#define PET_PRICE(pet) (GET_LEVEL(pet) * 300)

SPECIAL(pet_shops)
{
	char buf[MAX_STRING_LENGTH], pet_name[256];
	int pet_room;
	Character *pet;

	pet_room = IN_ROOM(ch) + 1;

	if (CMD_IS("list")) {
		ch->Send("Available pets are:\r\n");
		START_ITER(iter, pet, Character *, world[pet_room].people) {
			ch->Sendf("%8d - %s\r\n", PET_PRICE(pet), pet->Name());
		}
		return (TRUE);
	} else if (CMD_IS("buy")) {

		argument = one_argument(argument, buf);
		argument = one_argument(argument, pet_name);

		if (!(pet = get_char_room(buf, pet_room))) {
			ch->Send("There is no such pet!\r\n");
			return (TRUE);
		}
		if (GET_GOLD(ch) < PET_PRICE(pet)) {
			ch->Send("You don't have enough gold!\r\n");
			return (TRUE);
		}
		GET_GOLD(ch) -= PET_PRICE(pet);

		pet = Character::Read(pet->Virtual());
		GET_EXP(pet) = 0;
		SET_BIT(AFF_FLAGS(pet), AffectBit::Charm);

		if (*pet_name) {
			sprintf(buf, "%s %s", pet->Keywords(), pet_name);
			/* FREE(pet->player.name); don't free the prototype! */
			pet->SetKeywords(buf);

			sprintf(buf, "%sA small sign on a chain around the neck says 'My name is %s'\r\n",
				pet->LDesc(), pet_name);
			/* FREE(pet->player.description); don't free the prototype! */
			pet->SetLDesc(buf);
		}
		pet->ToRoom(IN_ROOM(ch));
		add_follower(pet, ch);

		/* Be certain that pets can't get/carry/use/wield/wear items */
		IS_CARRYING_W(pet) = 1000;
		IS_CARRYING_N(pet) = 100;

		ch->Send("May you enjoy your pet.\r\n");
		act("$n buys $N as a pet.", FALSE, ch, 0, pet, TO_ROOM);

		return 1;
	}
	/* All commands except list and buy */
	return 0;
}



/* ********************************************************************
*	Special procedures for objects																		 *
******************************************************************** */


SPECIAL(bank)
{
	int amount;

	if (CMD_IS("balance")) {
		if (GET_BANK_GOLD(ch) > 0)
			ch->Sendf("Your current balance is %d coins.\r\n",
				GET_BANK_GOLD(ch));
		else
			ch->Send("You currently have no money deposited.\r\n");
		return 1;
	} else if (CMD_IS("deposit")) {
		if ((amount = atoi(argument)) <= 0) {
			ch->Send("How much do you want to deposit?\r\n");
			return 1;
		}
		if (GET_GOLD(ch) < amount) {
			ch->Send("You don't have that many coins!\r\n");
			return 1;
		}
		GET_GOLD(ch) -= amount;
		GET_BANK_GOLD(ch) += amount;
		ch->Sendf("You deposit %d coins.\r\n", amount);
		act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
		return 1;
	} else if (CMD_IS("withdraw")) {
		if ((amount = atoi(argument)) <= 0) {
			ch->Send("How much do you want to withdraw?\r\n");
			return 1;
		}
		if (GET_BANK_GOLD(ch) < amount) {
			ch->Send("You don't have that many coins deposited!\r\n");
			return 1;
		}
		GET_GOLD(ch) += amount;
		GET_BANK_GOLD(ch) -= amount;
		ch->Sendf("You withdraw %d coins.\r\n", amount);
		act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
		return 1;
	} else
		return 0;
}


void npc_steal(Character * ch, Character * victim)
{
	int gold;

	if (IS_NPC(victim))
		return;
	if (IS_STAFF(victim))
		return;
	if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
		return;

	if (AWAKE(victim) && (Number(0, GET_LEVEL(ch)) == 0)) {
		act("You discover that $n has $s hands in your wallet.", FALSE, ch, 0, victim, TO_VICT);
		act("$n tries to steal gold from $N.", TRUE, ch, 0, victim, TO_NOTVICT);
	} else {
		/* Steal some gold coins */
		gold = (int) ((GET_GOLD(victim) * Number(1, 10)) / 100);
		if (gold > 0) {
			GET_GOLD(ch) += gold;
			GET_GOLD(victim) -= gold;
		}
	}
}


SPECIAL(thief)
{
	Character *cons;

	if (cmd)
		return FALSE;

	if (GET_POS(ch) != POS_STANDING)
		return FALSE;

	START_ITER(iter, cons, Character *, world[IN_ROOM(ch)].people) {
		if (!IS_NPC(cons) && !IS_STAFF(cons) && (!Number(0, 4))) {
			npc_steal(ch, cons);
			return TRUE;
		}
	}

	return FALSE;
}


SPECIAL(portal)
{
  Object *obj = (Object *) me;
  Object *port;
  char obj_name[MAX_STRING_LENGTH];

    if (!CMD_IS("enter")) return FALSE;

    argument = one_argument(argument,obj_name);
    if (!(port = get_obj_in_list_vis(ch, obj_name, world[IN_ROOM(ch)].contents)))
    {
      return(FALSE);
    }

    if (port != obj)
      return(FALSE);

    if (GET_OBJ_VAL(port, 1) <= 0 ||
        GET_OBJ_VAL(port, 1) > 32000) {
      ch->Send("The portal leads nowhere\n\r");
      return TRUE;
    }

//	CHANGEPOINT:
//    if (obj->action_description) {
//      adesc_lines(obj->ADesc(), buf3, buf4);
//    } else {
      strcpy(buf2, "$n enters $p, and vanishes!");
      strcpy(buf3, "You enter $p, and are transported elsewhere!");
//    }

    act(buf2, FALSE, ch, port, 0, TO_ROOM);
    act(buf3, FALSE, ch, port, 0, TO_CHAR);
    ch->FromRoom();
    ch->ToRoom(GET_OBJ_VAL(port, 1));
	look_at_rnum(ch, ch->InRoom(), false);
    act("$n appears from thin air!", FALSE, ch, 0, 0, TO_ROOM);

    return TRUE;
}
