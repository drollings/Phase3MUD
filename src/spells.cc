/* ************************************************************************
*	 File: spells.c																			Part of CircleMUD *
*	Usage: Implementation of "manual spells".	Circle 2.2 spell compat.		*
*																																				 *
*	All rights reserved.	See license.doc for complete information.				*
*																																				 *
*	Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*	CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.							 *
*																																				 *
*	$Author: sfn $
*	$Date: 2001/09/03 22:28:42 $
*	$Revision: 1.8 $
************************************************************************ */


#include "structs.h"
#include "mud.base.h"
#include "characters.h"
#include "objects.h"
#include "rooms.h"
#include "scripts.h"
#include "index.h"
#include "utils.h"
#include "comm.h"
#include "spells.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "constants.h"
#include "config.h"
#include "extradesc.h"
#include "buffer.h"
// #include "world.h"

// extern Object *object_list;
// extern Character *character_list;
extern struct cha_app_type cha_app[];
extern struct int_app_type int_app[];

// extern struct descriptor_data *descriptor_list;
extern struct zone_data *zone_table;

extern int mini_mud;
extern int pk_allowed;
extern int summon_allowed;
extern int sleep_allowed;
extern int charm_allowed;
extern int roomaffect_allowed;

extern struct default_mobile_stats *mob_defaults;
extern char weapon_verbs[];
extern int *max_ac_applys;
extern struct apply_mod_defaults *apmd;

void clearMemory(Character * ch);
extern void CheckRegenRates(Character *ch);

void weight_change_object(Object * obj, int weight);
void add_follower(Character * ch, Character * leader);
void affect_to_obj(Object * obj, Affect * af, long duration);
int mag_savingthrow(Character * ch, SInt16 type, SInt16 mod = 0);

extern void concentration_to_character(Character *ch, Concentration *con, long interval);


/*
 * Special spells appear below.
 */

#define PORTAL 21

ASPELL(spell_portal)
{
	Affect *af, *af2;
	Concentration *con = NULL, *con2= NULL;

	/* create a magic portal */
	Object *tmp_obj, *tmp_obj2;
	ExtraDesc *ed;
	Room *rp = NULL, *nrp = NULL;
//	Character *tmp_ch = NULL;
	VNum to_room = NOWHERE, to_where = NOWHERE;
	int i;
	bool broken = false;
	char buf[512];

	extern Character * get_char_vis(const Character *ch, const char *name, Flags mode);

	skip_spaces(arg);

	if (!*arg) {
		to_room = mortal_start_room;
		nrp = &world[mortal_start_room];
	} else	{

		const char *destinations[] = {
			"home",
			"newhaven",
			"freedale",
			"callista",
			"downs",
			"arena",
			"lonoya",
			"\n"
		};

		const VNum dest_rooms[] = {
			mortal_start_room,
			3039,
			8800,
			8000,
			14000,
			19500,
			3710,
			-1
		};

		to_where = search_block(arg, destinations, TRUE);

		if (to_where >= 0) {
			to_room = dest_rooms[to_where];
// 		} else {
// 			tmp_ch = get_char_vis(ch, arg, FIND_CHAR_WORLD);
// 			if (!tmp_ch) {
// 				ch->Send("Cannot find the target of your spell!\r\n");
// 				return;
// 			}
// 			if (tmp_ch == ch) {
// 				ch->Send("You cannot cast this spell upon yourself!\r\n");
// 				return;
// 			}
// 			to_room = IN_ROOM(tmp_ch);
		}

		if (to_room != NOWHERE)
			nrp = &world[to_room];
	}

	tmp_obj = Object::Read(PORTAL);

	/*
		check target room for legality.
	 */
	if ((IN_ROOM(ch) == NOWHERE) || !tmp_obj || !nrp) {
		ch->Send("The magic fails.\r\n");
		if (tmp_obj)
			tmp_obj->Extract();
		return;
	}

// 	if (tmp_ch) {
// 		if (IS_NPC(tmp_ch))
// 			broken = (!MOB_FLAGGED(tmp_ch, MOB_NOSUMMON));
// 		// else
// 		//	 broken = (!PRF_FLAGGED(tmp_ch, Preference::Summonable));
//
// 		if (broken || (GET_TRUST(ch) < GET_TRUST(tmp_ch))) {
// 			ch->Send("Your magic fails to take hold.\n\r");
// 			broken = TRUE;
// 		}
//
// 		//	(world[ch->in_room]->zone != world[tmp_ch->in_room]->zone)) {
//
// 		else if ((!IS_NPC(ch) || AFF_FLAGGED(ch, AffectBit::Charm)) && IS_NPC(tmp_ch)) {
// 			ch->Send("Your ambition exceeds your grasp, and the portal flickers out of sight.\r\n");
// 			broken = TRUE;
// 		}
// 	}

	if ((ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOMAGIC)) ||
			(ROOM_FLAGGED(to_room, ROOM_NOMAGIC))) {
		ch->Send("A rift forms, flickers violently and closes.\n\r");
		broken = TRUE;
	}

	else if ((GET_TRUST(ch) < 6) && (ROOM_FLAGGED(to_room, ROOM_GODROOM) ||
			ROOM_FLAGGED(to_room, ROOM_HOUSE))) {
		ch->Send("The magic to travel there exceeds your grasp!\n\r");
		broken = TRUE;
	}

	else if ((ROOM_FLAGGED(IN_ROOM(ch), ROOM_TUNNEL)) ||
					 (ROOM_FLAGGED(to_room, ROOM_TUNNEL))) {
		ch->Send("There is no room in here to summon!\n\r");
		broken = TRUE;
	}

	else if (zone_table[world[ch->AbsoluteRoom()].zone].realm !=
			 zone_table[world[to_room].zone].realm) {
		ch->Send("The magic to travel there exceeds your grasp!\n\r");
		broken = TRUE;
	}

	if (broken) {
		tmp_obj->Extract();
		return;
	}

	sprintf(buf, "Through the mists of the portal, you can faintly see %s", nrp->Name());

	ed = new ExtraDesc();
	ed->keyword = SString::Create(tmp_obj->Name());
	ed->description = SString::Create(buf);
	tmp_obj->exDesc.Append(ed);

	GET_OBJ_VAL(tmp_obj, 0) = level/2;
	GET_OBJ_VAL(tmp_obj, 1) = to_room;

	af = new Affect(SPELL_PORTAL, 0, Affect::None, 0);
	af->Join(tmp_obj, 0, false, false, false, false);

	con = new Concentration;
	con->Join(ch, PULSE_VIOLENCE, tmp_obj, af);

	tmp_obj->ToRoom(IN_ROOM(ch));

	act("$p suddenly appears.", TRUE, ch, tmp_obj, 0, TO_ROOM);
	act("$p suddenly appears.", TRUE, ch, tmp_obj, 0, TO_CHAR);

/* Portal on the other side. Rewrite : (JD) for TLO */
	 tmp_obj2 = Object::Read(PORTAL);
	 if (!tmp_obj2) {
		 ch->Send("The magic fails\n\r");
		 tmp_obj2->Extract();
		 return;
	 }
	sprintf(buf, "Through the mists of the portal, you can faintly see %s", world[IN_ROOM(ch)].Name());

	ed = new ExtraDesc();
	ed->keyword = SString::Create(tmp_obj2->Name());
	ed->description = SString::Create(buf);
	tmp_obj2->exDesc.Append(ed);

	GET_OBJ_VAL(tmp_obj2, 0) = level/2;
	GET_OBJ_VAL(tmp_obj2, 1) = IN_ROOM(ch);

	af2 = new Affect(SPELL_PORTAL, 0, Affect::None, 0);
	af2->Join(tmp_obj2, 0, false, false, false, false);

	con2 = new Concentration;
	con2->Join(ch, PULSE_VIOLENCE, tmp_obj2, af2);

	tmp_obj2->ToRoom(to_room);

	if (world[to_room].people.Count()) {
		act("$p appears from within itself.",
			TRUE, world[to_room].people.Bottom(), tmp_obj2, 0, TO_ROOM);
		act("$p appears from within itself.",
			TRUE, world[to_room].people.Bottom(), tmp_obj2, 0, TO_CHAR);
	}
}


ASPELL(spell_recharge)
{
	int restored_charges = 0, explode = 0;

	if (ch == NULL || obj == NULL)
		return;

	/* This is on their Mud, comment out for right now!
	if (GET_OBJ_EXTRA(obj) == ITEM_NO_RECHARGE) {
		ch->Send("This item cannot be recharged.\r\n");
		return;
	}
	*/
	if (GET_OBJ_TYPE(obj) == ITEM_WAND) {
		if (GET_OBJ_VAL(obj, 2) < GET_OBJ_VAL(obj, 1)) {
			ch->Send("You attempt to recharge the wand.\r\n");
			restored_charges = Number(1, 10);
			GET_OBJ_VAL(obj, 2) += restored_charges;
			GET_OBJ_VAL(obj, 1) -= MAX((restored_charges - (level / 2)), 0);
			if (GET_OBJ_VAL(obj, 2) > GET_OBJ_VAL(obj, 1)) {
			ch->Send("The wand is overcharged and explodes!\r\n");
			act("$n overcharges $p and it explodes!", TRUE, ch, obj, 0, TO_NOTVICT);
			explode = dice(GET_OBJ_VAL(obj, 2), 2);
			GET_HIT(ch) -= explode;
			ch->UpdatePos();
			obj->Extract();
			return;
			} else {
			sprintf(buf, "You restore %d charges to the wand.\r\n", restored_charges);
			ch->Send(buf);
			return;
			}
		} else {
			ch->Send("That item is already at full charge!\r\n");
			return;
		}
	} else if (GET_OBJ_TYPE(obj) == ITEM_STAFF) {
		if (GET_OBJ_VAL(obj, 2) < GET_OBJ_VAL(obj, 1)) {
			ch->Send("You attempt to recharge the staff.\r\n");
			restored_charges = Number(1, 3);
			GET_OBJ_VAL(obj, 2) += restored_charges;
			GET_OBJ_VAL(obj, 1) -= MAX((restored_charges - (level / 2)), 0);
			if(GET_OBJ_VAL(obj, 2) > GET_OBJ_VAL(obj, 1)) {
				ch->Send("The staff is overcharged and explodes!\r\n");
				act("$n overcharges $p and it explodes!", TRUE, ch, obj, 0, TO_NOTVICT);
				explode = dice(GET_OBJ_VAL(obj, 2), 3);
				GET_HIT(ch) -= explode;
				ch->UpdatePos();
				obj->Extract();
				return;
			}
			else {
				sprintf(buf, "You restore %d charges to the staff.\r\n", restored_charges);
				ch->Send(buf);
				return;
			}
		}
		else {
			ch->Send("That item is already at full charge!\r\n");
			return;
		}
	}
}

ASPELL(spell_create_water)
{
	int water;

	void name_to_drinkcon(Object * obj, int type);
	void name_from_drinkcon(Object * obj);

	if (ch == NULL || obj == NULL)
		return;
	level = MAX(MIN(level, MAX_LEVEL), 1);

	if (GET_OBJ_TYPE(obj) == ITEM_DRINKCON) {
		if ((GET_OBJ_VAL(obj, 2) != LIQ_WATER) && (GET_OBJ_VAL(obj, 1) != 0)) {
			name_from_drinkcon(obj);
			GET_OBJ_VAL(obj, 2) = LIQ_SLIME;
			name_to_drinkcon(obj, LIQ_SLIME);
		} else {
			water = MAX(GET_OBJ_VAL(obj, 0) - GET_OBJ_VAL(obj, 1), 0);
			if (water > 0) {
				if (GET_OBJ_VAL(obj, 1) >= 0)
					name_from_drinkcon(obj);
				GET_OBJ_VAL(obj, 2) = LIQ_WATER;
				GET_OBJ_VAL(obj, 1) += water;
				name_to_drinkcon(obj, LIQ_WATER);
				weight_change_object(obj, water);
				act("$p is filled.", FALSE, ch, obj, 0, TO_CHAR);
			}
		}
	}
}


ASPELL(spell_recall)
{
	extern SInt16 r_mortal_start_room;

	if (victim == NULL)
		return;
	if (IS_NPC(victim) || (AFF_FLAGGED(ch, AffectBit::Charm) && (ch != victim->master))) {
		ch->Send("You fail.\r\n");
		return;
	}

	act("$n disappears.", TRUE, victim, 0, 0, TO_ROOM);
	victim->FromRoom();
	victim->ToRoom(r_mortal_start_room);
	act("$n appears in the middle of the room.", TRUE, victim, 0, 0, TO_ROOM);
	if (!IS_NPC(victim))
	look_at_room(victim, 0);
}


ASPELL(spell_planeshift)
{
	extern int astral_start_room;

	if (victim == NULL || (IS_NPC(victim) && (!AFF_FLAGGED(ch, AffectBit::Charm))) ||
		(!Room::Find(astral_start_room)))
		return;

	act("$n disappears in a flash of light.", TRUE, victim, 0, 0, TO_ROOM);
	victim->FromRoom();
	victim->ToRoom(astral_start_room);
	act("$n appears in a burst of scintillating light.", TRUE, victim, 0, 0, TO_ROOM);
	look_at_room(victim, 0);
}


ASPELL(spell_teleport)
{
#ifdef GOT_RID_OF_IT
	VNum to_room;

	if (victim == NULL)
		return;

	do {
		to_room = Number(0, top_of_world);
	} while ((!world[to_room]) || ROOM_FLAGGED(to_room, ROOM_GODROOM | ROOM_PRIVATE | ROOM_DEATH));

	act("$n slowly fades out of existence and is gone.",
			FALSE, victim, 0, 0, TO_ROOM);
	char_from_room(victim);
	char_to_room(victim, to_room);
	act("$n slowly fades into existence.", FALSE, victim, 0, 0, TO_ROOM);
	look_at_room(victim, 0);
#endif
}

#define SUMMON_FAIL "You failed.\r\n"

ASPELL(spell_summon)
{
	if (ch == NULL || victim == NULL || ch == victim)
		return;

	if ((GET_LEVEL(victim) > (GET_LEVEL(ch) + 3)) || IS_STAFF(victim)) {
		ch->Send(SUMMON_FAIL);
		return;
	}

	if (IS_NPC(victim) || (AFF_FLAGGED(ch, AffectBit::Charm) && (ch == victim->master))) {
		ch->Send(SUMMON_FAIL);
		return;
	}

	/* (FIDO) Changed from pk_allowed to summon_allowed */
	if (!summon_allowed) {
		ch->Send("The magic unravels before you can complete the spell!\r\n");
		return;
	}

	if (!pk_allowed) {
		if (MOB_FLAGGED(victim, MOB_AGGRESSIVE)) {
			act("As the words escape your lips and $N travels\r\n"
			"through time and space towards you, you realize that $E is\r\n"
			"aggressive and might harm you, so you wisely send $M back.",
			FALSE, ch, 0, victim, TO_CHAR);
			return;
		}
		if (Affect::AffectedBy(victim, SPELL_BIND)) {
			act("You are unable to wrest $N free of the spell holding $M.", FALSE, ch, 0, victim, TO_CHAR);
			return;
		}
		if (!IS_NPC(victim) && !PRF_FLAGGED(victim, Preference::Summonable) &&
			!PLR_FLAGGED(victim, PLR_KILLER)) {
			sprintf(buf, "%s just tried to summon you to: %s.\r\n"
				"%s failed because you have summon protection on.\r\n"
				"Type PREF SUMMON to allow other players to summon you.\r\n",
				ch->Name(), world[IN_ROOM(ch)].Name(),
				(GET_SEX(ch) == Male) ? "He" : "She");
			victim->Send(buf);

			sprintf(buf, "You failed because %s has summon protection on.\r\n",
				ch->Name());
			ch->Send(buf);

			sprintf(buf, "%s failed summoning %s to %s.",
				ch->Name(), ch->Name(), world[IN_ROOM(ch)].Name());
			mudlog(buf, BRF, 0, TRUE);
			return;
		}
	}

	if (MOB_FLAGGED(victim, MOB_NOSUMMON) ||
			(IS_NPC(victim) && mag_savingthrow(victim, SAVING_SPELL))) {
		ch->Send(SUMMON_FAIL);
		return;
	}

	act("$n disappears suddenly.", TRUE, victim, 0, 0, TO_ROOM);

	victim->FromRoom();
	victim->ToRoom(IN_ROOM(ch));

	act("$n arrives suddenly.", TRUE, victim, 0, 0, TO_ROOM);
	act("$n has summoned you!", FALSE, ch, 0, victim, TO_VICT);
	look_at_room(victim, 0);
}

ASPELL(spell_relocate)
{
	if (ch == NULL || victim == NULL || ch != victim)
		return;

// CHANGEPOINT:	Gotta decide on new conditions for failure.
//	 if (GET_LEVEL(victim) > MIN(TRUST_IMMORT - 1, level + 3)) {
//		 ch->Send(SUMMON_FAIL);
//		 return;
//	 }

	/* (FIDO) Changed from pk_allowed to summon_allowed */
	if (!summon_allowed) {
		if (MOB_FLAGGED(victim, MOB_AGGRESSIVE)) {
			act("As the words escape your lips and a rift travels\r\n"
					"through time and space towards $N, you realize that $E is\r\n"
					"aggressive and might harm you, so you wisely close it..",
					FALSE, ch, 0, victim, TO_CHAR);
			return;
		}
		if (!IS_NPC(victim) && !PRF_FLAGGED(victim, Preference::Summonable) &&
				!PLR_FLAGGED(victim, PLR_KILLER)) {
			sprintf(buf, "%s just tried to relocate to you : %s.\r\n"
							"%s failed because you have summon protection on.\r\n"
							"Type NOSUMMON to allow other players to relocate to you.\r\n",
							GET_NAME(ch), world[IN_ROOM(ch)].Name(),
							(GET_SEX(ch) == Male) ? "He" : "She");
			victim->Send(buf);

			sprintf(buf, "You failed because %s has summon protection on.\r\n", victim->Name());
			ch->Send(buf);

			mudlogf(BRF, NULL, TRUE, "%s failed relocating to %s at %s.",
					GET_NAME(ch), GET_NAME(victim), world[IN_ROOM(ch)].Name());
			return;
		}
	}

	act("$n opens a portal and steps through it.", TRUE, victim, 0, 0, TO_ROOM);
	act("You open a portal and step through.", FALSE, ch, 0, 0, TO_CHAR);

	ch->FromRoom();
	ch->ToRoom(victim->InRoom());

	act("A portal opens and $n steps out.", TRUE, ch, 0, 0, TO_ROOM);
	look_at_room(ch, 0);
}

ASPELL(spell_locate_object)
{
	Object *i;
	int j;

	skip_spaces(arg);
	j = level >> 1;

	START_ITER(iter, i, Object *, Objects) {
		if (!isname(arg, i->Name()))
			continue;

		if (!CAN_SEE_OBJ(ch, i))
			continue;

		if (i->CarriedBy())
			sprintf(buf, "%s is being carried by %s.\n\r", i->Name(), PERS(i->CarriedBy(), ch));
		else if (IN_ROOM(i) != NOWHERE)
			sprintf(buf, "%s is in %s.\n\r", i->Name(), world[IN_ROOM(i)].Name());
		else if (i->InObj())
			sprintf(buf, "%s is in %s.\n\r", i->Name(), i->InObj()->Name());
		else if (i->WornBy())
			sprintf(buf, "%s is being worn by %s.\n\r",
				i->Name(), PERS(i->WornBy(), ch));
		else
			sprintf(buf, "%s's location is uncertain.\n\r", i->Name());

		CAP(buf);
		ch->Send(buf);
		j--;
	}

	if (j == level >> 1)
		ch->Send("You sense nothing.\n\r");
}



ASPELL(spell_charm)
{
	Affect *aff;
	Concentration *con = NULL;
	long duration;

	if (victim == NULL || ch == NULL)
		return;

	if (victim == ch)
		ch->Send("You like yourself even better!\r\n");
	// else if (!IS_NPC(victim)) {
	//	 if ((charm_allowed == 0) && (!PRF_FLAGGED(victim, Preference::Summonable)))
	//			 ch->Send("You fail because SUMMON protection is on!\r\n");
	// }
	else if (AFF_FLAGGED(victim, AffectBit::Sanctuary))
		ch->Send("Your victim is protected by sanctuary!\r\n");
	else if (MOB_FLAGGED(victim, MOB_NOCHARM))
		ch->Send("Your victim resists!\r\n");
	else if (AFF_FLAGGED(ch, AffectBit::Charm))
		ch->Send("You can't have any followers of your own!\r\n");
	else if (AFF_FLAGGED(victim, AffectBit::Charm) ||
					 // level < GET_LEVEL(victim) ||
		 IS_STAFF(victim))
		ch->Send("You fail.\r\n");
	/* player charming another player - no legal reason for this */
	/* (FIDO) Changed from pk_allowed to charm_allowed */
	else if (!charm_allowed && !IS_NPC(victim))
		ch->Send("You fail - shouldn't be doing it anyway.\r\n");
	else if (circle_follow(victim, ch))
		ch->Send("Sorry, following in circles can not be allowed.\r\n");
	else if (mag_savingthrow(victim, SAVING_PARA))
		ch->Send("Your victim resists!\r\n");
	else {
		if (victim->master)
			victim->StopFollower();

		add_follower(victim, ch);

		if (GET_INT(victim))	duration = 24 * 18 / GET_INT(victim);
		else					duration = 24 * 18;

		aff = new Affect(SPELL_CHARM, 0, Affect::None, AffectBit::Charm);
		aff->Join(victim, duration, false, false, false, false);

		act("Isn't $n just such a nice fellow?", FALSE, ch, 0, victim, TO_VICT);
		if (IS_NPC(victim)) {
			REMOVE_BIT(MOB_FLAGS(victim), MOB_AGGRESSIVE);
			REMOVE_BIT(MOB_FLAGS(victim), MOB_SPEC);
		}

		con = new Concentration;
		con->Join(ch, PULSE_VIOLENCE, victim, aff);
	}
}


ASPELL(spell_bind)
{
	Affect *aff = NULL, *aff2 = NULL;
	Concentration *con = NULL, *con2 = NULL;
	long duration;
	bool tmp;	// Temporary value used for the Updates at the end.

	if (victim == NULL || ch == NULL)
		return;

	if (victim == ch)
		ch->Send("You're quite attached to yourself already!\r\n");
    else if (Affect::AffectedBy(ch, SPELL_MAGE_SHIELD))
		act("Your spell's energy reflects harmlessly off of $n's mage shield!", TRUE, ch, 0, victim, TO_CHAR);
	else if (IS_STAFF(victim))
		ch->Send("False.\r\n");
	else if (mag_savingthrow(victim, SAVING_PARA))
		ch->Send("Your victim resists!\r\n");
	else {
		if (victim->master)
			victim->StopFollower();

		aff = new Affect(SPELL_BIND, 0, Affect::None, 0);
		aff->Join(ch, 0, false, false, false, false);

		con = new Concentration;
		con->Join(ch, PULSE_VIOLENCE, ch, aff);

		aff2 = new Affect(SPELL_BIND, 0, Affect::None, 0);
		aff2->Join(victim, 0, false, false, false, false);

		con2 = new Concentration;
		con2->Join(ch, PULSE_VIOLENCE, victim, aff2);

		act("Tendrils of magical energy fasten themselves about $N!", TRUE, ch, 0, victim, TO_CHAR);
		victim->Send("Tendrils of magical energy fasten themselves about you!\r\n");
		act("Tendrils of magical energy fasten themselves about $N!", TRUE, ch, 0, victim, TO_NOTVICT);
	}
}


ASPELL(spell_peace)
{
	Character *temp;

	act("$n tries to stop the fight.", TRUE, ch, 0, 0, TO_ROOM);
	act("You try to stop the fight.", FALSE, ch, 0, 0, TO_CHAR);

	if (IS_EVIL(ch))	return;

	START_ITER(iter, temp, Character *, world[IN_ROOM(ch)].people) {
		if (FIGHTING(temp)) {
			temp->StopFighting();

			if (IS_NPC(temp))	clearMemory(temp);

			if (ch != temp) {
				act("$n stops fighting.", TRUE, temp, 0, 0, TO_ROOM);
				act("You stop fighting.", TRUE, temp, 0, 0, TO_CHAR);
			}
		}
	}

	return;
}


ASPELL(spell_identify)
{
	int found;
	Affect *affect;

	TimeInfoData age(Character * ch);

	extern void objval_info(Character *ch, Object *j);

	if (obj) {
		ch->Send("You feel informed:\r\n");
		sprintf(buf, "Object '%s', Item type: ", obj->Name());
		sprinttype(GET_OBJ_TYPE(obj), item_types, buf2);
		strcat(buf, buf2);
		strcat(buf, "\r\n");
		ch->Send(buf);

		if (obj->affectbits) {
			ch->Send("Item will give you following abilities:	");
			sprintbit(obj->affectbits, affected_bits, buf);
			strcat(buf, "\r\n");
			ch->Send(buf);
		}
		ch->Send("Item is: ");
		sprintbit(GET_OBJ_EXTRA(obj), extra_bits, buf);
		strcat(buf, "\r\n");
		ch->Send(buf);

		sprintf(buf, "Weight: %d, Value: %d\r\n", GET_OBJ_WEIGHT(obj), GET_OBJ_COST(obj));
		ch->Send(buf);

		sprintf(buf, "Passive Defense: %d	Damage Resistance: %d\r\n",
					GET_PD(obj), GET_DR(obj));
		ch->Send(buf);

		objval_info(ch, obj);

#ifdef GOT_RID_OF_IT
		switch (GET_OBJ_TYPE(obj)) {
		case ITEM_SCROLL:
		case ITEM_POTION:
			sprintf(buf, "This %s casts: ", item_types[(int) GET_OBJ_TYPE(obj)]);

			if (GET_OBJ_VAL(obj, 1) >= 1)	sprintf(buf, "%s %s", buf, Skill::Name(GET_OBJ_VAL(obj, 1)));
			if (GET_OBJ_VAL(obj, 2) >= 1)	sprintf(buf, "%s %s", buf, Skill::Name(GET_OBJ_VAL(obj, 2)));
			if (GET_OBJ_VAL(obj, 3) >= 1)	sprintf(buf, "%s %s", buf, Skill::Name(GET_OBJ_VAL(obj, 3)));
			sprintf(buf, "%s\r\n", buf);
			ch->Send(buf);
			break;
		case ITEM_WAND:
		case ITEM_STAFF:
			sprintf(buf, "This %s casts: ", item_types[(int) GET_OBJ_TYPE(obj)]);
			sprintf(buf, "%s %s\r\n", buf, Skill::Name(GET_OBJ_VAL(obj, 3)));
			sprintf(buf, "%sIt has %d maximum charge%s and %d remaining.\r\n", buf,
				GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 1) == 1 ? "" : "s",
				GET_OBJ_VAL(obj, 2));
			ch->Send(buf);
			break;
		case ITEM_WEAPON:
			// sprintf(buf, "", );
			sprintf(buf, "Damage Dice is '%dD%d'", GET_OBJ_VAL(obj, 1),
				GET_OBJ_VAL(obj, 2));
			sprintf(buf, "%s for an average per-round damage of %.1f.\r\n", buf,
				(((GET_OBJ_VAL(obj, 2) + 1) / 2.0) * GET_OBJ_VAL(obj, 1)));
			ch->Send(buf);
			break;
		}
#endif

		found = FALSE;

		START_ITER(iter, affect, Affect *, obj->affected) {
			if ((affect->AffLocation() != Affect::None) && (affect->AffModifier() != 0)) {
				if (!found) {
					ch->Send("Can affect you as :\r\n");
					found = TRUE;
				}
				sprinttype(affect->AffLocation(), apply_types, buf2);
				sprintf(buf, "   Affects: %s By %d\r\n", buf2, affect->AffModifier());
				ch->Send(buf);
			}
		}

	} else if (victim) {		/* victim */
		sprintf(buf, "Name: %s\r\n", victim->Name());
		ch->Send(buf);
		if (!IS_NPC(victim)) {
			sprintf(buf, "%s is %d years, %d months, %d days and %d hours old.\r\n",
				victim->Name(), age(victim).year, age(victim).month,
				age(victim).day, age(victim).hours);
			ch->Send(buf);
		}
		sprintf(buf, "Height %d cm, Weight %d pounds\r\n",
			GET_HEIGHT(victim), GET_WEIGHT(victim));
		sprintf(buf, "%sLevel: %d, Hits: %d, Mana: %d\r\n", buf,
			GET_LEVEL(victim), GET_HIT(victim), GET_MANA(victim));
		sprintf(buf, "%sPD: %d, DR: %d, Hitroll: %d, Damroll: %d\r\n", buf,
			GET_PD(victim), GET_DR(victim), GET_HITROLL(victim), GET_DAMROLL(victim));
		sprintf(buf, "%sStr: %d, Int: %d, Wis: %d, Dex: %d, Con: %d, Cha: %d\r\n",
			buf, GET_STR(victim), GET_INT(victim), GET_WIS(victim),
			GET_DEX(victim), GET_CON(victim), GET_CHA(victim));
		ch->Send(buf);

	}
}



/* Routine to find an available affect slot and modify it */
int enchant_affect(Character * ch, Object * obj,
			 Affect::Location applywhat, SInt16 modifier, UInt8 divisor)
{
#ifdef GOT_RID_OF_IT
// CHANGEPOINT: We'll need to make some way for the affect to be more manipulable
	int i, j;
	extern void affect_modify(Character * ch, SInt8 loc, SInt8 mod,
					long bitv, bool add);
	Affect *affect = NULL;

	START_ITER(iter, affect, Affect *, obj->affected) {
		if ((affect->AffLocation() == Affect::None) || (affect->AffLocation() == applywhat))
			break;
	}

	if (!affect)
		return 0;


	for (i = 0; i < NUM_WEARS; i++)
		if (GET_EQ(ch, i) == obj) {
			affect_modify(ch, GET_EQ(ch, i)->affected[j].location,
			GET_EQ(ch, i)->affected[j].modifier,
			GET_EQ(ch, i)->obj_flags.bitvector, FALSE);
			break;
		}

	obj->affected[j].location = applywhat;
	obj->affected[j].modifier += modifier;

	if (i < NUM_WEARS)
		affect_modify(ch, GET_EQ(ch, i)->affected[j].location,
		GET_EQ(ch, i)->affected[j].modifier,
		GET_EQ(ch, i)->obj_flags.bitvector, TRUE);

	/* Return the modifier divided by the divisor, which allows for
		 certain types of enchantments incrementing "i" in the spell
		 by more or less, making certain plusses (or weapons?) more/less
		 risky. */
	return (obj->affected[j].modifier / MAX(divisor, 1));
#endif

	return 0;
}


// Generic routine to improve objects of various types
int enchant_object(Character * ch, Object * obj)
{
	int plus = 0;

	switch (GET_OBJ_TYPE(obj)) {
	case ITEM_WEAPON:
		plus = Number(0, 9);
		if (GET_OBJ_VAL(obj, 2) > -1)
			plus += ++GET_OBJ_VAL(obj, 1);
		if (GET_OBJ_VAL(obj, 5) > -1)
			plus += ++GET_OBJ_VAL(obj, 4);
		break;
	case ITEM_ARMOR:
		switch (Number(0, 1)) {
		case 0: plus = ++GET_PD(obj); break;
		case 1: plus = ++GET_DR(obj); break;
		}
		break;
	case ITEM_WAND:
	case ITEM_POTION:
		plus = ++GET_OBJ_VAL(obj, 0); break;
	case ITEM_STAFF:
		plus = ++GET_OBJ_VAL(obj, 0) * 2; break;
	case ITEM_LIGHT:
		if (GET_OBJ_VAL(obj, 2) > -1)
			plus = (GET_OBJ_VAL(obj, 2) + 10) / 2; break;
	}

	return (plus);
}


ASPELL(spell_enchant_weapon)
{
	int i;
	Attack attack;

	if (ch == NULL || obj == NULL)
		return;

	if ((GET_OBJ_TYPE(obj) != ITEM_WEAPON) &&
		 (GET_OBJ_TYPE(obj) != ITEM_MISSILE))
		return;

	/* Find the locations for the HITROLL and DAMROLL */

	if (IS_GOOD(ch)) {
		if (IS_SET(GET_OBJ_EXTRA(obj), ITEM_ANTI_GOOD)) {
			sprintf(buf, "$p zaps you!	Ouch!");
			act(buf, FALSE, ch, obj, 0, TO_CHAR);
			attack.dam = dice(3, 4);
			ch->Damage(ch, &attack);
			return;
		}
		SET_BIT(GET_OBJ_EXTRA(obj), ITEM_ANTI_EVIL);
		sprintf(buf, "$p glows blue.");
	} else if (IS_EVIL(ch)) {
		if (IS_SET(GET_OBJ_EXTRA(obj), ITEM_ANTI_EVIL)) {
			sprintf(buf, "$p zaps you!	Ouch!");
			act(buf, FALSE, ch, obj, 0, TO_CHAR);
			attack.dam = dice(3, 4);
			ch->Damage(ch, &attack);
			return;
		}
		SET_BIT(GET_OBJ_EXTRA(obj), ITEM_ANTI_GOOD);
		sprintf(buf, "$p glows red.");
	} else {
		sprintf(buf, "$p glows yellow.");
		switch(Number(0, 3)) {
		case 2: SET_BIT(GET_OBJ_EXTRA(obj), ITEM_ANTI_GOOD); break;
		case 3: SET_BIT(GET_OBJ_EXTRA(obj), ITEM_ANTI_EVIL); break;
		default: break;
		}
	}

	i = 0;

	// i += enchant_affect(ch, obj, Affect::Hitroll, Number(1, 2), 1);
	// i += enchant_affect(ch, obj, Affect::Damroll, Number(1, 2), 1);

	i += enchant_affect(ch, obj, Affect::Hitroll, MAX(Number(-1, 1), 0), 1);
	i += enchant_object(ch, obj);

	SET_BIT(GET_OBJ_EXTRA(obj), ITEM_NODROP);
	SET_BIT(GET_OBJ_EXTRA(obj), ITEM_PROXIMITY);

	if (IS_SET(GET_OBJ_EXTRA(obj), ITEM_MAGIC)) {
		switch(MAX((Number(i, i * 100) / (level * 3)), 1)) {
		case 1:
			break;
		case 2:
		case 3:
		case 4:
			switch (Number(0, 7)) {
			case 0: enchant_affect(ch, obj, Affect::Hit, Number(i * -8, i * -1), 1); break;
			case 1: enchant_affect(ch, obj, Affect::Mana, Number(i * -8, i * -1), 1); break;
			case 2: enchant_affect(ch, obj, Affect::Move, Number(i * -8, i * -1), 1); break;
			case 3: enchant_affect(ch, obj, Affect::SavingFire, Number(i * -8, i * -1), 1); break;
			case 4: enchant_affect(ch, obj, Affect::SavingElectricity, Number(i * -8, i * -1), 1); break;
			case 5: enchant_affect(ch, obj, Affect::SavingFreezing, Number(i * -8, i * -1), 1); break;
			case 6: enchant_affect(ch, obj, Affect::SavingVim, Number(i * -8, i * -1), 1); break;
			case 7: enchant_affect(ch, obj, Affect::SavingSpell, Number(i * -8, i * -1), 1); break;
			}
			break;
		case 5:
		case 6:
		case 7:
			switch(Number(0, 8)) {
			case 0:
			case 1:
			case 2: SET_BIT(GET_OBJ_EXTRA(obj), ITEM_HUM); break;
			case 3:
			case 4:
			case 5: SET_BIT(GET_OBJ_EXTRA(obj), ITEM_GLOW); break;
			case 6:
			case 7:
			case 8: SET_BIT(GET_OBJ_EXTRA(obj), ITEM_INVISIBLE); break;
			}
			break;
		case 8:
		case 9:
		case 10:
			switch (Number(0, 4)) {
			case 0: enchant_affect(ch, obj, Affect::Str, Number(i / -2, -1), 1); break;
			case 1: enchant_affect(ch, obj, Affect::Int, Number(i / -2, -1), 1); break;
			case 2: enchant_affect(ch, obj, Affect::Wis, Number(i / -2, -1), 1); break;
			case 3: enchant_affect(ch, obj, Affect::Dex, Number(i / -2, -1), 1); break;
			case 4: enchant_affect(ch, obj, Affect::Con, Number(i / -2, -1), 1); break;
			}
			break;
		default:
			sprintf(buf, "$p suddenly glows violently and evaporates in a brilliant flash!");
			act(buf, FALSE, ch, obj, 0, TO_CHAR);
			act(buf, FALSE, ch, obj, 0, TO_ROOM);
			act("&cRYour flesh is seared by the chaotic magic!&c0", FALSE, ch, obj, 0, TO_CHAR);
			act("$n's flesh is seared by the chaotic magic!", FALSE, ch, obj, 0, TO_ROOM);
			attack.dam = dice(i, i * 5);
			ch->Damage(ch, &attack);
			obj->Extract();
			return;
		}
	}

	act(buf, FALSE, ch, obj, 0, TO_CHAR);
	act(buf, FALSE, ch, obj, 0, TO_ROOM);
	SET_BIT(GET_OBJ_EXTRA(obj), ITEM_MAGIC);

}


ASPELL(spell_detect_poison)
{
	if (victim) {
		if (victim == ch) {
			if (AFF_FLAGGED(victim, AffectBit::Poison))
				ch->Send("You can sense poison in your blood.\r\n");
			else
				ch->Send("You feel healthy.\r\n");
		} else {
			if (AFF_FLAGGED(victim, AffectBit::Poison))
				act("You sense that $E is poisoned.", FALSE, ch, 0, victim, TO_CHAR);
			else
				act("You sense that $E is healthy.", FALSE, ch, 0, victim, TO_CHAR);
		}
	}

	if (obj) {
		switch (GET_OBJ_TYPE(obj)) {
		case ITEM_DRINKCON:
		case ITEM_FOUNTAIN:
		case ITEM_FOOD:
			if (GET_OBJ_VAL(obj, 3))
	act("You sense that $p has been contaminated.",FALSE,ch,obj,0,TO_CHAR);
			else
	act("You sense that $p is safe for consumption.", FALSE, ch, obj, 0,
			TO_CHAR);
			break;
		default:
			ch->Send("You sense that it should not be consumed.\r\n");
		}
	}
}

#ifdef GOT_RID_OF_IT
ASPELL(spell_cancellation)
{
	int i;
	int found;

	struct time_info_data age(Character * ch);

	extern char *item_types[];
	extern char *extra_bits[];
	extern char *apply_types[];
	extern char *affected_bits[];

	if (obj) {
	} else if (victim) {		/* victim */
		if (vict->affected) {
			while (vict->affected && (!found))
				if ((!Number(0, 2)) {
					affect_remove(vict, vict->affected);
					found = TRUE;
				}
			if (found) {
				send_to_char("There is a brief flash of light!\r\n"
										 "You feel slightly different.\r\n", vict);
				ch->Send("All spells removed.\r\n");
			} else {
				ch->Send("Nothing happens.\r\n");
			}
		} else {
			ch->Send("Nothing happens.\r\n");
			return;
		}
	}
}
#endif


ASPELL(spell_horrific_illusion)
{
	char to_ch[120], to_vict[120], to_room[120];
	extern ACMD(do_flee);

	*to_ch = '\0';
	*to_vict = '\0';
	*to_room = '\0';

	if (ch == NULL || victim == NULL)
		return;

	if (mag_savingthrow (victim, SAVING_SPELL)) {
		 ch->Send("The illusion will not form.\r\n");
		 return;
	}

	switch (Number(0, 5)) {
	case 0:
		strcpy (to_ch, "$N bursts into imaginary flames!");
		strcpy (to_vict, "&cRYou erupt into flames!  They won't go out!&c0");
		strcpy (to_room, "$n screams in agony for no good reason.");
		break;
	case 1:
		strcpy (to_ch, "$N's skin crawls with imaginary spiders!");
		strcpy (to_vict, "&cRHuge poisonous spiders crawl all over you!&c0");
		strcpy (to_room, "$n screams in horror for no good reason.");
		break;
	case 2:
		strcpy (to_ch, "In your mind, $N's flesh turns to liquid!");
		strcpy (to_vict, "&cRYour flesh begins to drip off your bones!&c0");
		strcpy (to_room, "$n screams at the top of $s lungs.");
		break;
	case 3:
		strcpy (to_ch, "$N's mouth begins to fill with maggots!");
		strcpy (to_vict, "&cRYou start to choke as your mouth fills up with maggots!&c0");
		strcpy (to_room, "$n begins to choke and cough.");
		break;
	case 4:
		strcpy (to_ch, "You imagine $N's limbs falling off.");
		strcpy (to_vict, "&cRYour arms begin to fall apart!&c0");
		strcpy (to_room, "$n begins to scream and wave their arms in the air wildly.");
		break;
	default:
		strcpy (to_ch, "You disembowel $N with your eyes!");
		strcpy (to_vict, "&cRYour stomach suddenly bursts open and your guts fall to the ground!&c0");
		strcpy (to_room, "$n screams in terror and clutches their stomach.");
	}

	if (to_ch != NULL)
		act(to_ch, FALSE, ch, 0, victim, TO_CHAR);
	if (to_vict != NULL)
		act(to_vict, FALSE, victim, 0, ch, TO_CHAR);
	if (to_room != NULL)
		act(to_room, TRUE, victim, 0, ch, TO_ROOM);

	victim->Send("You panic, and attempt to flee!\r\n");
	do_flee(victim, "flee", 0, "", 0);

}


void Resurrect(Character *ch, Object *obj, Character *mob)
{
	Object *o = NULL;

	LListIterator<Object *>	iter(obj->contents);
	while ((o = iter.Next())) {
		o->FromObj();
		o->ToChar(mob);
	}

	mob->material = (obj->material == Materials::Bone ? Materials::Flesh : obj->material);
	if (!IS_NPC(mob)) {
		GET_CLASS(mob) = CLASS_UNDEFINED;
	}

	GET_POS(mob) = POS_RESTING;
	GET_COND(mob, THIRST) = 0;
	GET_COND(mob, FULL) = 0;

	if (!IS_NPC(mob))	Crash_crashsave(mob);
}


inline void perform_raise_dead(Character *ch, Object *obj, Character *mob) {
	Resurrect(ch, obj, mob);
	act("$N stirs, and comes to life!", FALSE, ch, 0, mob, TO_ROOM);
	act("$N stirs, and comes to life!", FALSE, ch, 0, mob, TO_CHAR);
	mob->Send("A strong force tugs at you, bringing you back within the confines of your\r\n"
		"newly regenerated flesh.\r\n");
	GET_HIT(mob) = 1;
	GET_MANA(mob) = 1;
	GET_MOVE(mob) = 1;
	CheckRegenRates(mob);
	obj->Extract();
}


ASPELL(spell_raise_dead)
{
	IDNum		id = 0;
	VNum		vnum = 0;
	Character	*mob = NULL;

	if (obj) {
		if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && GET_OBJ_VAL(obj, 3)) {
			if (GET_OBJ_VAL(obj, 3) > 0)		vnum = GET_OBJ_VAL(obj, 3);
			else								id = -(GET_OBJ_VAL(obj, 3));

			if (id) {
				if ((mob = find_char(id))) {
					perform_raise_dead(ch, obj, mob);
					mob->Save(mob->AbsoluteRoom());
					Crash_crashsave(mob);
				} else {
					ch->Send("The corpse's spirit does not answer your call.\r\n");
				}
			} else {
				if (!Character::Find(vnum)) {
					ch->Send("The corpse still shows no sign of life.\r\n");
				} else {
					mob = Character::Read(vnum);
					mob->ToRoom(IN_ROOM(ch));
					perform_raise_dead(ch, obj, mob);
				}
			}
		} else {
			ch->Send("That is not a corpse.\r\n");
		}
	}
}


ASPELL(spell_reincarnate)
{
}


