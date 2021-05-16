/* ************************************************************************
*	 File: magic.c																			 Part of CircleMUD *
*	Usage: low-level functions for magic; spell template code							*
*																																				 *
*	All rights reserved.	See license.doc for complete information.				*
*																																				 *
*	Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*	CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.							 *
*																																				 *
*	$Author: sfn $
*	$Date: 2001/02/04 15:57:57 $
*	$Revision: 1.15 $
************************************************************************ */


#include "structs.h"
#include "mud.base.h"
#include "objects.h"
#include "rooms.h"
#include "characters.h"
#include "descriptors.h"
#include "combat.h"
#include "scripts.h"
#include "config.h"
#include "utils.h"
#include "comm.h"
#include "spells.h"
#include "handler.h"
#include "event.h"
#include "db.h"
#include "constants.h"
#include "buffer.h"
#include "property.h"

class MethodEvent;

extern struct default_mobile_stats *mob_defaults;
extern char weapon_verbs[];
extern struct apply_mod_defaults *apmd;
inline void calc_delay(Character *ch, MethodEvent *energymethod, SInt16 &delay);
void eval_expr(char *line, char *result, Scriptable * go, Script *sc, Trigger *trig);

void clearMemory(Character * ch);

void weight_change_object(Object * obj, int weight);
int in_melee_range(Character * ch, Character * victim, int attacktype);
extern void add_follower(Character *ch, Character *leader);
extern void CheckRegenRates(Character *ch);
extern SInt8 find_hit_location(Character *victim, SInt8 modifier);
extern void add_var(LList<TrigVarData *> &var_list, const char *name, const char *value, SInt32 id);

extern bool spell_effect_cvict(int level, Character * ch, Character * victim, SInt16 spellnum);


Vector<Method> Method::Methods;

METHOD_FUNC(method_start);
METHOD_FUNC(method_end);
METHOD_FUNC(method_disrupt);


const char * Method::func_names[] = {
	"start",
	"end",
	"disrupt",
	"\n"
};


SInt32 * Method::funcs[] = {
	(SInt32 *) method_start,
	(SInt32 *) method_end,
	(SInt32 *) method_disrupt
};


int mag_savingthrow(Character * ch, SInt16 type, SInt16 mod = 0)
{
	int save;

	switch (type) {
	case 0:		// Paralyzation
		save = (GET_DEX(ch) + GET_WIS(ch)) / 2;
		break;
	case 1:		// Rod
	case 3:		// Breath Weapon
		save = GET_DEX(ch);
		break;
	case 2:		// Petrification
		save = (GET_STR(ch) + GET_DEX(ch)) / 2;
		break;
	case 4:		// Spell
		save = (GET_INT(ch) + GET_WIS(ch)) / 2;
		break;
	default:
		save = GET_WIS(ch);
		break;
	}

	if (save < (dice(3, 6) + mod))
		return TRUE;
	else
		return FALSE;
}




/*
 *	mag_materials:
 *	Checks for up to 3 vnums (spell reagents) in the player's inventory.
 *
 * No spells implemented in Circle 3.0 use mag_materials, but you can use
 * it to implement your own spells which require ingredients (i.e., some
 * heal spell which requires a rare herb or some such.)
 */
int mag_materials(	int level, Character * ch, Character * victim, 
					SInt16 spellnum, bool extract, bool verbose)
{
	Property::Materials	*property = PROPERTY(spellnum, Materials);

	SInt32 i;
	Object *tobj;
	Vector<int>		objects = property->components;
	LList<Object *>		components;

	if (!property) {
		sprintf(buf, "Material properties for spell %s have not been defined!", Skill::Name(spellnum));
		mudlog(buf, BRF, 0, TRUE);
		return false;
	}
	
	if (objects.Count() > 0) {
		START_ITER(iter, tobj, Object *, ch->contents) {
			if (tobj->CanBeSeen(ch)) {
				for (i = objects.Count() - 1; i >= 0; --i) {
					if ((objects[i] > 0) && (GET_OBJ_NUM(tobj) == objects[i])) {
						objects[i] = -1;
						components.Append(tobj);
					}
				}
			}
		}

		// Check to see if any components were found missing
		for (i = objects.Count() - 1; i >= 0; --i) {
			if (objects[i] > 0) {
				if (verbose) {
					switch (Number(0, 2)) {
					case 0:
						ch->Send("A wart sprouts on your nose.\r\n");
						break;
					case 1:
						ch->Send("Your hair falls out in clumps.\r\n");
						break;
					case 2:
						ch->Send("A huge corn develops on your big toe.\r\n");
						break;
					}
				}
				return false;
			}
		}

		// Extract reagents
		if (extract) {
			START_ITER(iter, tobj, Object *, components) {
				tobj->FromChar();
				tobj->Extract();
			}
		}

		if (verbose) {
			ch->Send("A puff of smoke rises from your pack.\r\n");
			act("A puff of smoke rises from $n's pack.", TRUE, ch, NULL, NULL, TO_ROOM);
		}
		
		return true;

	} else {
		return false;
	}
}




/*
 * Every spell that does damage comes through here.	This calculates the
 * amount of damage, adds in any modifiers, determines what the saves are,
 * tests for save and calls damage().
 */

void mag_damage(int level, Character * ch, Character * victim, int spellnum)
{
	Property::Damage	*property = PROPERTY(spellnum, Damage);

	Attack attack;
	static Script	script(Datatypes::Character);
	static Trigger	trig;
	static char		field[20];
	
	if (victim == NULL || ch == NULL)	return;

	if (!property) {
		sprintf(buf, "Damage properties for spell %s have not been defined!", Skill::Name(spellnum));
		mudlog(buf, BRF, 0, TRUE);
		return;
	}
	
	attack.damtype = property->damagetype;

	if (property->hitslocation)
		attack.location = find_hit_location(victim, 0);
	
	if (property->numdice) {
		attack.dam = dice(property->numdice, property->sizedice) + property->modifier;
	}

	if (property->formula) {
		sprintf(field, "%c%d", UID_CHAR, GET_ID(ch));
		add_var(trig.variables, "self", field, 0);
		sprintf(field, "%c%d", UID_CHAR, GET_ID(victim));
		add_var(trig.variables, "victim", field, 0);
		eval_expr(property->formula, buf, ch, &script, &trig);
		trig.variables.Clear();
		script.context = 0;

		attack.dam += atoi(buf);
	}
	
	if (IS_EVIL(ch) && Affect::AffectedBy(victim, SPELL_PROT_FROM_EVIL) && (!IS_EVIL(victim)))
		attack.dam = (attack.dam * 4) / 5;

	if (IS_EVIL(ch) && Affect::AffectedBy(ch, SPELL_PROT_FROM_EVIL))
		attack.dam = (attack.dam * 4) / 5;

	/* divide damage by two if victim makes his saving throw */
	// if (mag_savingthrow(victim, savetype))
	//	attack.dam /= 2;

	// CHANGEPOINT:	We need to block casting and other spell routines based
	// on range.	Possibly the skilltable is a good place for this.
	if (IS_SET(Skills[spellnum]->target, TAR_MELEE) && 
			(!in_melee_range(ch, victim, spellnum))) {
		ch->Send("Your target is no longer within reach!\r\n");
		return;
	}

	attack.message = spellnum;

	// ch->Sendf("Damage: %d	Damtype: %d, Location %d\r\n", 
	//							 dam, damtype, attack.location);
	/* and finally, inflict the damage */
	victim->Damage(ch, &attack);
}


/*
 * Every spell that does an affect comes through here.	This determines
 * the effect, whether it is added or replacement, whether it is legal or
 * not, etc.
 *
 * affect_join(vict, aff, duration, add_dur, avg_dur, add_mod, avg_mod)
*/

void mag_affects(int level, Character *ch, Character *victim, int spellnum, int concentrate = 0)
{
	Property::Affects	*property = PROPERTY(spellnum, Affects);
	AffectEditable		*ae = NULL;
	Affect				*aff = NULL;
	Concentration		*con = NULL;
	int 				i;
	SInt8				encumbrance, prev_encumbrance;
	bool				is_innate = false;

	Property::Concentrate	*cproperty = PROPERTY(spellnum, Concentrate);
	Character				*concentrator = NULL;
	
	if (victim == NULL || ch == NULL)	return;

	if (!property) {
		sprintf(buf, "Affect properties for spell %s have not been defined!", Skill::Name(spellnum));
		mudlog(buf, BRF, 0, TRUE);
		return;
	}
	
	prev_encumbrance = GET_ENCUMBRANCE(victim, IS_CARRYING_W(victim));

	//	If this is a mob that has this affect set in its mob file, do not
	//	perform the affect.	This prevents people from un-sancting mobs
	//	by sancting them and waiting for it to fade, for example.
	if (IS_NPC(victim) && !Affect::AffectedBy(victim, spellnum)) {
		for (i = property->affects.Count() - 1; i >= 0; --i) {
			if (AFF_FLAGGED(victim, property->affects[i].flags)) {
				ch->Send(NOEFFECT);
				return;
			}
		}
	}

	//	If the victim is already affected by this spell, and the spell does
	//	not have an accumulative effect, then fail the spell.
	if (Affect::AffectedBy(victim, spellnum) && 
	   !(property->accum_duration || property->accum_affect)) {
		ch->Send(NOEFFECT);
		return;
	}

	/*
	 *	If the victim has that spell affect set innate, then make sure that the
	 *	spell is treated like the above, which fails the spell.
	 */
	START_ITER(iter, aff, Affect *, AFFECTS(victim)) {
		if (spellnum == aff->AffType() && !(aff->Time())) {
			is_innate = true;
			break;
		}
	}

	if (Affect::AffectedBy(victim, spellnum) && is_innate) {
		ch->Send(NOEFFECT);
		return;
	}

	if (cproperty) {
		concentrator = ch;
	}

	for (i = property->affects.Count() - 1; i >= 0; --i) {
		ae = &(property->affects[i]);

		if (IS_SET(ae->flags, AffectBit::Sleep)) {
			if (MOB_FLAGGED(victim, MOB_NOSLEEP)) {
				ch->Send(NOEFFECT);
				return;
			} else {
				GET_POS(victim) = POS_SLEEPING;
			}
		}

		if (ae->flags || (ae->location != Affect::None)) {
			aff = new Affect(spellnum, ae->modifier, (Affect::Location) ae->location, 
					  (Flags) ae->flags, concentrator);
			aff->Join(victim, ae->duration * PULSES_PER_MUD_HOUR, 
					property->accum_duration, false, property->accum_affect, false);
		}

		if (aff && IS_SET(aff->AffFlags(), AffectBit::Invisible)) {
			victim->SetInvis(level);
		}

		if (cproperty) {
			con = new Concentration;
			con->Join(ch, cproperty->interval * PULSE_VIOLENCE, victim, aff);
		}
	}

	if (property->tovict != NULL)
		act(property->tovict, FALSE, victim, 0, ch, TO_CHAR);
	if (property->toroom != NULL)
		act(property->toroom, true, victim, 0, ch, TO_ROOM);

	if (ch != victim && Skills[spellnum]->violent && (GET_POS(victim) > POS_SLEEPING))
		victim->Hit(ch);	/* Thats not nice! */

	encumbrance = GET_ENCUMBRANCE(victim, IS_CARRYING_W(victim));
	if (prev_encumbrance != encumbrance)
		victim->Send(encumbrance_shift[encumbrance][(encumbrance < prev_encumbrance)]);


}


/*
 * This function is used to provide services to mag_groups.	This function
 * is the one you should change to add new group spells.
 */

void perform_mag_groups(int level, Character * ch,
			Character * tch, int spellnum)
{
	switch (spellnum) {
		case SPELL_GROUP_HEAL:
		mag_points(level, ch, tch, SPELL_HEAL);
		break;
	case SPELL_GROUP_ARMOR:
		mag_affects(level, ch, tch, SPELL_ARMOR);
		break;
	case SPELL_GROUP_RECALL:
		spell_recall(level, ch, tch, NULL, NULL);
		break;
	case SPELL_GROUP_PLANESHIFT:
		spell_planeshift(level, ch, tch, NULL, NULL);
		break;
	}
}


/*
 * Every spell that affects an area (room) runs through here.	These are
 * generally offensive spells.	This calls mag_damage to do the actual
 * damage -- all spells listed here must also have a case in mag_damage()
 * in order for them to work.
 *
 *	area spells have limited targets within the room.
*/

void mag_areas(int level, Character * ch, Character * victim, int spellnum)
{
	Property::Areas		*property = PROPERTY(spellnum, Areas);
	bool	valid;
	int		number_affected;

	/* Daniel Rollings AKA Daniel Houghton's enhancement of mag_areas: exclude groupmembers */
	Character *tch, *k;

	if (ch == NULL)			return;

	if (!property) {
		sprintf(buf, "Area properties for spell %s have not been defined!", Skill::Name(spellnum));
		mudlog(buf, BRF, 0, TRUE);
		return;
	}
	
	if (ch->master != NULL)	k = ch->master;
	else					k = ch;

	if (property->tochar != NULL)	act(property->tochar, FALSE, ch, 0, 0, TO_CHAR);
	if (property->toroom != NULL)	act(property->toroom, FALSE, ch, 0, 0, TO_ROOM);

	number_affected = property->how_many;

	START_ITER(iter, tch, Character *, world[ch->InRoom()].people) {

		if (!number_affected)		break;

		// 	This skips: 1: the caster (if affect_caster not on)
		//				2: immortals
		//				3: members of the caster's group
		// 		
		// 	If the spell is not affect_fighting, 
		//				4: if no pk on this mud, skips over all players except
		//					murderers, 			  
		//				5: the caster/master's pets (charmed NPCs)
		// 	Otherwise only those fighting the caster or his/her groupmembers is
		//	affected.

		/* If affect_caster not set, skip the caster */
		if ((!IS_SET(property->flags, 1 << Property::Areas::Caster) && (tch == ch)))	continue;

		/* If tch is a groupmember or charmed follower of the caster 
			 or the caster's master, skip */

		if (IS_SET(property->flags, 1 << Property::Areas::MissFlying) && 
			 (AFF_FLAGGED(ch, AffectBit::Fly) || (Affect::AffectedBy(ch, SPELL_LEVITATE))))
			continue;

		valid = false;	// We're going to assume a skip unless proven otherwise

		if (IS_SET(property->flags, 1 << Property::Areas::CasterGroup) && 
			((AFF_FLAGGED(tch, AffectBit::Group) || AFF_FLAGGED(tch, AffectBit::Charm)) && 
			(k == tch->master)))
			valid = true;

		if (!valid && (tch != victim) && (!IS_SET(property->flags, 1 << Property::Areas::Bystander))) {
			if (IS_SET(property->flags, Property::Areas::Fighting) && (tch == FIGHTING(ch)))
				valid = true;

			if (IS_SET(property->flags, 1 << Property::Areas::FightingCaster) && (FIGHTING(tch) == ch))
				valid = true;

			if (IS_SET(property->flags, 1 << Property::Areas::FightingGroup) && FIGHTING(tch) &&
				is_same_group(ch, FIGHTING(tch)))
				valid = true;

			if (IS_SET(property->flags, 1 << Property::Areas::TargetGroup) && FIGHTING(tch) &&
				is_same_group(tch, victim))
				valid = true;
		} else {
			if ((ch != victim) && !is_same_group(ch, victim))
				valid = true;
		}
		
		if (!valid)		continue;

		if (IS_SET(Skills[spellnum]->routines, MAG_DAMAGE)) {
			/* Skip unpunished immortals */
			if (!IS_NPC(tch) && ((GET_TRUST(tch) >= TRUST_IMMORTAL) && (!IS_IMMPUN(ch))))
				continue;
			/* If no roomaffects or allowed, or if PKilling is not allowed and
			 the caster and foe are both non PK'ing players, skip */
			if (!IS_NPC(ch) && !IS_NPC(tch)) {
				if (!roomaffect_allowed)
					continue;
				if (!pk_allowed && (!PLR_FLAGGED(ch, PLR_KILLER) || !PLR_FLAGGED(tch, PLR_KILLER)))
					continue;
			}
		}

		--number_affected;

		spell_effect_cvict(level, ch, tch, spellnum);
	}
}


void mag_summons(int level, Character * ch, Object * obj,
					int spellnum)
{
	Property::Summons	*property = PROPERTY(spellnum, Summons);

	Character			*mob = NULL;
	Object 				*tobj;
	int					i;

	if (ch == NULL)		return;

	if (!property) {
		sprintf(buf, "Summon properties for spell %s have not been defined!", Skill::Name(spellnum));
		mudlog(buf, BRF, 0, TRUE);
		return;
	}
	
	if (AFF_FLAGGED(ch, AffectBit::Charm)) {
		ch->Send("You are too giddy to have any followers!\r\n");
		return;
	}

	if (property->corpse) {
		if ((obj == NULL) || (GET_OBJ_TYPE(obj) != ITEM_CONTAINER) || (!GET_OBJ_VAL(obj, 3))) {
			act(property->fail, FALSE, ch, 0, 0, TO_CHAR);
			return;
		}
	}

	for (i = 0; i < property->how_many; ++i) {
		if (!(mob = Character::Read(property->tosummon))) {
			ch->Send("Sorry, this mob is not available (please report).\r\n");
			sprintf(buf, "SYSERR: Player attempting to mag_summon() non-existing mob %d", property->tosummon);
			mudlog(buf, BRF, 0, TRUE);
			return;
		}
		mob->ToRoom(ch->InRoom());
		IS_CARRYING_W(mob) = 0;
		IS_CARRYING_N(mob) = 0;
		SET_BIT(AFF_FLAGS(mob), AffectBit::Charm);
		add_follower(mob, ch);
		act(property->tochar, FALSE, ch, 0, mob, TO_ROOM);
		act(property->toroom, FALSE, ch, 0, mob, TO_ROOM);
	}

	if (property->corpse) {
		START_ITER(iter, tobj, Object *, obj->contents) {
			tobj->FromObj();
			tobj->ToChar(mob);
		}
		obj->Extract();
	}
}


void mag_points(int level, Character * ch, Character * victim,
				 int spellnum)
{
	Property::Points	*property = PROPERTY(spellnum, Points);

	int						hit = 0, mana = 0, move = 0;
	
	bool is_change = false;

	if (ch == NULL || victim == NULL)		return;

	if (!property) {
		sprintf(buf, "Point properties for spell %s have not been defined!", Skill::Name(spellnum));
		mudlog(buf, BRF, 0, TRUE);
		return;
	}
	
	if (property->hitnumdice) {
		hit = dice(property->hitnumdice, property->hitsizedice) + property->hit;
	}

	if (property->mananumdice) {
		mana = dice(property->mananumdice, property->manasizedice) + property->mana;
	}

	if (property->movenumdice) {
		move = dice(property->movenumdice, property->movesizedice) + property->move;
	}

	/* Daniel Rollings AKA Daniel Houghton's revision:	Changed healing spells to to_char format */

/* If there are changes in HP or MP, perform them */
	if ( ((hit != 0) && (GET_HIT(victim) < GET_MAX_HIT(victim))) ||
			((mana != 0) && (GET_MANA(victim) < GET_MAX_MANA(victim))) ||
			((move != 0) && (GET_MOVE(victim) < GET_MAX_MOVE(victim))) ) {
		AlterHit(victim, -hit);
		AlterMana(victim, -mana);
		AlterMove(victim, -move);
		is_change = true;
	}

	if (!is_change)
		ch->Send(NOEFFECT);
	else {
		if (property->tochar != NULL)	act(property->tochar, FALSE, ch, 0, 0, TO_CHAR);
		if (property->tovict != NULL)	act(property->tovict, FALSE, victim, 0, ch, TO_CHAR);
		if (property->toroom != NULL)	act(property->toroom, TRUE, victim, 0, ch, TO_ROOM);
	}
}


void mag_alter_objs(int level, Character * ch, Object * obj,
						 int spellnum)
{
	char *to_char = NULL;
	char *to_room = NULL;

	if (obj == NULL)	return;

	switch (spellnum) {
		case SPELL_BLESS:
			if (!OBJ_FLAGGED(obj, ITEM_BLESS) &&
		(GET_OBJ_WEIGHT(obj) <= 5 * level)) {
	SET_BIT(GET_OBJ_EXTRA(obj), ITEM_BLESS);
	to_char = "$p glows briefly.";
			}
			break;
		case SPELL_CURSE:
			if (!OBJ_FLAGGED(obj, ITEM_NODROP)) {
	SET_BIT(GET_OBJ_EXTRA(obj), ITEM_NODROP);
	if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
		GET_OBJ_VAL(obj, 2)--;
	to_char = "$p briefly glows red.";
			to_room = "$p in $n's hands briefly glows red.";
			}
			break;
		case SPELL_INVISIBLE:
			if (!OBJ_FLAGGED(obj, ITEM_NOINVIS | ITEM_INVISIBLE)) {
				SET_BIT(obj->extra, ITEM_INVISIBLE);
				to_char = "$p vanishes.";
			to_room = "$p in $n's hands fades out of existance!";
			}
			break;
		case SPELL_POISON:
			if (((GET_OBJ_TYPE(obj) == ITEM_DRINKCON) ||
				 (GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN) ||
				 (GET_OBJ_TYPE(obj) == ITEM_FOOD)) && !GET_OBJ_VAL(obj, 3)) {
			GET_OBJ_VAL(obj, 3) = 1 + level / 15;
			to_char = "$p steams briefly.";
			to_room = "$p steams briefly.";
			}
			break;
		case SPELL_REMOVE_CURSE:
			if (OBJ_FLAGGED(obj, ITEM_NODROP)) {
				REMOVE_BIT(obj->extra, ITEM_NODROP);
				if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
					GET_OBJ_VAL(obj, 2)++;
				to_char = "$p briefly glows blue.";
			to_room = "$p in $n's hands briefly glows blue.";
			}
			break;
		case SPELL_REMOVE_POISON:
			if (((GET_OBJ_TYPE(obj) == ITEM_DRINKCON) ||
				 (GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN) ||
				 (GET_OBJ_TYPE(obj) == ITEM_FOOD)) && GET_OBJ_VAL(obj, 3)) {
				GET_OBJ_VAL(obj, 3) = 0;
				to_char = "$p steams briefly.";
			}
			break;
	}

	if (to_char == NULL)
		ch->Send(NOEFFECT);
	else
		act(to_char, TRUE, ch, obj, 0, TO_CHAR);

	if (to_room != NULL)
		act(to_room, TRUE, ch, obj, 0, TO_ROOM);
	else if (to_char != NULL)
		act(to_char, TRUE, ch, obj, 0, TO_ROOM);

}



void mag_unaffects(int level, Character * ch, Character * victim, int spellnum)
{
	Property::Unaffects	*property = PROPERTY(spellnum, Unaffects);

	if (ch == NULL || victim == NULL)		return;

	if (!property) {
		ch->Sendf("Properties for spell %s have not been defined!", Skill::Name(spellnum));
		return;
	}
	
	if (!Affect::AffectedBy(victim, property->spellnum)) {
		ch->Send(NOEFFECT);
		return;
	}

	Affect::FromThing(victim, property->spellnum);
	if (property->tovict != NULL)	   act(property->tovict, FALSE, victim, 0, ch, TO_CHAR);
	if (property->toroom != NULL)	   act(property->toroom, TRUE, victim, 0, ch, TO_ROOM);

	CheckRegenRates(victim);	/* speed up regen rate immediately. */
}


void mag_creations(int level, Character * ch, int spellnum)
{
	Property::Creations	*property = PROPERTY(spellnum, Creations);
	Object *			tobj;
	int					i;

	if (ch == NULL)		return;

	if (!property) {
		ch->Sendf("Creation properties for spell %s have not been defined!", Skill::Name(spellnum));
		return;
	}
	
	level = MAX(MIN(level, MAX_LEVEL), 1);

	// Check to see if any components were found missing
	for (i = property->objects.Count() - 1; i >= 0; --i) {
		if (property->objects[i] > 0) {
			if (!(tobj = Object::Read(property->objects[i]))) {
				ch->Send("I seem to have goofed.\r\n");
				sprintf(buf, "SYSERR: spell_creations, spell %d, obj %d: obj not found",
						spellnum, property->objects[i]);
				log(buf);
			} else {
				tobj->ToChar(ch);
				act("$n creates $p.", FALSE, ch, tobj, 0, TO_ROOM);
				act("You create $p.", FALSE, ch, tobj, 0, TO_CHAR);
			}
		}
	}
}


void mag_room(int level, Character *ch, int spellnum)
{
	Property::Room		*property = PROPERTY(spellnum, Room);
	int					i;
	VNum				target_room = ch->InRoom();
	Affect				*aff;
	AffectEditable		*ae = NULL;

	if (ch == NULL)					return;

	if (!property) {
		ch->Sendf("Room properties for spell %s have not been defined!", Skill::Name(spellnum));
		return;
	}
	
	if (!Room::Find(target_room))	return;

	level = MAX(MIN(level, MAX_LEVEL), 1);

	for (i = property->affects.Count() - 1; i >= 0; --i) {
		ae = &(property->affects[i]);
	
		if (ae->flags || (ae->location != Affect::None)) {
			// CHANGEPOINT:  We need a copy constructor for this.
			// aff = new Affect(property->affects[i]);
			// aff->Join(&(world[target_room]), property->duration[i] * PULSE_VIOLENCE, false, false, false, false);
		}
	}

	if (property->tochar)
		act(property->tochar, TRUE, ch, 0, 0, TO_CHAR);

	if (property->toroom)	
		act(property->toroom, TRUE, ch, 0, 0, TO_ROOM);
}


/* local structures */
class MethodEvent : public ActionEvent {
public:
						MethodEvent(time_t when, Character &c, SInt8 m, SInt16 a, SInt16 r) :
						ActionEvent(when, c), method(m), amount(a), root(r), stage(0) {}
					
    SInt8			method;
    SInt16			amount;
    SInt16			root;
    SInt8			stage;
	
	time_t			Execute(void);
};


time_t MethodEvent::Execute(void) {
	SInt32			chance, mod = 0;
	SInt16			diceroll, total;
    SInt16			skillnum, delay, tmpamount;
	Attack			*attack = NULL;
    Affect			*aff = NULL;

    skillnum = method + FIRST_METHOD;
    
	Method::Methods[method].StageMessage(ch, root, amount, stage);

	tmpamount = amount / MAX(1, Method::Methods[method].Messages.Count());
	stage += 1;

    if (Method::Methods[method].hit)
	    AlterHit(ch, MAX(1, tmpamount * Method::Methods[method].hit / 100));

    if (Method::Methods[method].mana)
	    AlterMana(ch, MAX(1, tmpamount * Method::Methods[method].mana / 100));

    if (Method::Methods[method].move)
	    AlterMove(ch, MAX(1, tmpamount * Method::Methods[method].move / 100));

	if (GET_MANA(ch) < 0) {
    	AlterMove(ch, -(GET_MANA(ch) * 3));
        Method::Methods[method].Disrupt(ch, root, amount);
        return 0;
    }
    
	if (GET_POS(ch) < POS_RESTING) {
        return 0;
    }

    total = ENERGY(ch).Check(ch, root);
    mod = total + amount;
    mod /= MAX(1, Method::Methods[method].ceiling);

	chance = SKILLCHANCE(ch, Method::Methods[method].skill, NULL) - mod;
    chance = RANGE(SKILL_NOCHANCE, chance, 100);
	diceroll = CHAR_SKILL(ch).RollSkill(ch, Method::Methods[method].skill, chance);

	if (diceroll < 0) {
		CHAR_SKILL(ch).RollToLearn(ch, Method::Methods[method].skill, diceroll);
		Method::Methods[method].Disrupt(ch, root, amount);
        return 0;
    }

	if (stage >= Method::Methods[method].Messages.Count()) {
		// This assures precision of the final amount.
    	tmpamount *= Method::Methods[method].Messages.Count() - 1;
        amount -= tmpamount;
		CHAR_SKILL(ch).RollToLearn(ch, Method::Methods[method].skill, diceroll);
		Method::Methods[method].Complete(ch, root, amount);
        return 0;
    }

	ENERGY(ch).Charge(ch, root, tmpamount, method, false);

	calc_delay(ch, this, delay);

	return (delay RL_SEC);
}


void Energy::Draw(Character *ch, SInt16 amount) 
{
	if (mana < amount) {
		mana = 0;
	}

	mana -= amount;
}


void	Energy::Charge(Character *ch, SInt16 amount, bool drain = true)
{
	mana += amount;

	// We add a number here to the amount to prevent the pool from draining right away.
	if (drain) {
		nextdrain = Method::Methods[method].decay;
		if (nextdrain)	nextdrain += 20;
	} else		nextdrain = 0;
}


bool	Energy::Update(Character *ch)
{
	if (mana <= 0)		return false;
	if (!nextdrain)		return true;

	--nextdrain;

	if (nextdrain == 0) {
		--mana;
		if (mana <= 0)		return false;
	    nextdrain = Method::Methods[method].decay;
	}
	
	return true;
}


// Returns the amount of energy needed that we fell short by, 0 if success.
SInt16	EnergyPool::Draw(Character *ch, SInt16 root, SInt16 amount)
{
	Energy *i = NULL;
	SInt16 needed = amount;

	START_ITER(iter, i, Energy *, Pools) {
		if (needed <= 0)
			break;

		if (i->root == root) {
			needed -= i->mana;
			i->Draw(ch, amount);

			if (i->mana == 0) {
				delete i;
				Pools.Remove(i);
			}
		}
	}

	return needed;
}


void	EnergyPool::Charge(Character *ch, SInt16 root, SInt16 amount, SInt16 method, bool drain = true)
{
	Energy *i = NULL, *energy = NULL, *insert_before = NULL;

	START_ITER(iter, i, Energy *, Pools) {
		if (i->root == root) {
			if (i->method == method) {
				i->Charge(ch, amount, drain);
				return;
			} else if (Method::Methods[i->method].decay < Method::Methods[method].decay) {
				insert_before = i;
				break;
			}
		}
	}
	
	// We didn't find a pool of the right type, and need to make one.
	// We'll use an insertion sort.
	energy = new Energy;

	energy->root = root;
	energy->mana = amount;
	energy->method = method;

	if (drain)	energy->nextdrain = Method::Methods[method].decay;
	else		energy->nextdrain = 0;

	if (insert_before)	Pools.InsertBefore(energy, insert_before);
	else				Pools.Append(energy);
}


void	EnergyPool::Save(Character *ch, FILE *outfile)
{
	Energy	*i = NULL;

	START_ITER(iter, i, Energy *, Pools) {
		if (Method::Methods[i->method].decay <= 0)
			fprintf(outfile, "%d %d %d\n", i->root, i->mana, i->method);
	}
	fprintf(outfile, "0 0 0\n");
}


SInt16	EnergyPool::Check(Character *ch, SInt16 root)
{
	SInt16 total = 0;
	Energy *i = NULL;

	START_ITER(iter, i, Energy *, Pools) {
		if (i->root == root) {
			total += i->mana;
		}
	}

	return total;
}


void	EnergyPool::Update(Character *ch)
{
	Energy *i = NULL;

	START_ITER(iter, i, Energy *, Pools) {
		if (!i->Update(ch)) {
			delete i;
			Pools.Remove(i);
		}
	}
}


void	EnergyPool::Clear(void)
{
	Pools.Clear();
}


bool	EnergyPool::List(Character *ch)
{
	Energy *i = NULL;
	int found = 0;

	START_ITER(iter, i, Energy *, Pools) {
		ch->Sendf("  %-15s %4d (%s)\r\n", Skill::Name(i->root), i->mana, Skill::Name(i->method + FIRST_METHOD));
		++found;
	}
	
	if (found)	return true;
	else		return false;
}


MethodMessage::~MethodMessage(void)
{
	if (tochar)			delete tochar;
	if (toroom)			delete toroom;
}


Method::~Method(void)
{
	if (startchar)		delete startchar;
	if (startroom)		delete startroom;
	if (verb)			delete verb;
	
	Messages.Clear();
}


void Method::Start(Character *ch, SInt16 root, SInt16 amount, MUDObject *target = NULL)
{
	if (startfunc != NULL) {
		(startfunc)(ch, root, amount, number);
	}
}


void Method::Disrupt(Character *ch, SInt16 root, SInt16 amount, MUDObject *target = NULL)
{
	if (PURGED(ch))
		return;
	    
	if (disruptfunc != NULL) {
		(disruptfunc)(ch, root, amount, number);
	}
}


void Method::Complete(Character *ch, SInt16 root, SInt16 amount, MUDObject *target = NULL)
{
	if (PURGED(ch))
		return;
	    
	if (endfunc != NULL) {
		(endfunc)(ch, root, amount, number);
	}
}


void Method::StageMessage(Character *ch, SInt16 &root, SInt16 &amount, SInt8 &stage, MUDObject *target = NULL)
{
	if (PURGED(ch) || (Methods[number].Messages.Count() == 0))
		return;
	    
	ch->Sendf("%s\r\n", Methods[number].Messages[stage].tochar);
	act(Methods[number].Messages[stage].toroom, Method::Methods[number].endvis, ch, NULL, NULL, TO_ROOM);
}


void calc_delay(Character *ch, MethodEvent *energymethod, SInt16 &delay)
{
	SInt8 &method = energymethod->method;
	SInt8 &build = Method::Methods[method].build;

	// Delay the event.  It doesn't get any quicker for mana amounts less than 10.
	delay = MAX(20, energymethod->amount);
	
	if (build && delay > build) {
		delay += build;
		delay /= 2;
	}
	
	delay /= MAX(1, build);

	// Now to partition the method casting into stages.
	delay = MAX(1, delay / MAX(1, Method::Methods[method].Messages.Count()));
}


METHOD_FUNC(method_start)
{
	SInt16	delay = 1;

	MethodEvent * energymethod = new MethodEvent(delay, *ch, method, amount, root);

	calc_delay(ch, energymethod, delay);

	ch->Sendf("%s\r\n", Method::Methods[method].startchar);
	act(Method::Methods[method].startroom, Method::Methods[method].startvis, ch, NULL, NULL, TO_ROOM);
}


METHOD_FUNC(method_end)
{
	SInt16 diceroll, total;
	SInt32 chance, mod = 0;

	total = ENERGY(ch).Check(ch, root);
	mod = total + amount;
	mod /= MAX(1, Method::Methods[method].ceiling);

	chance = RANGE(SKILL_NOCHANCE, SKILLCHANCE(ch, root, NULL) - mod, 100);
	diceroll = CHAR_SKILL(ch).RollSkill(ch, root, chance);

	if (amount > chance) {
		CHAR_SKILL(ch).RollToLearn(ch, Method::Methods[method].skill, diceroll);
	}

	if (diceroll < 0) {
		Method::Methods[method].Disrupt(ch, root, amount);
		return;
    }    

	ENERGY(ch).Charge(ch, root, amount, method, true);
	
}


METHOD_FUNC(method_disrupt)
{
	ch->Sendf("You miscarry!  Your %s energies are shattered!\r\n", Skill::Name(root));
	act("$n's efforts cease clumsily as $e miscarries.", Method::Methods[method].endvis, ch, NULL, NULL, TO_ROOM);
	ENERGY(ch).Draw(ch, root, amount / 2);
	
	AlterMana(ch, MIN(amount + 10, GET_MAX_MANA(ch) / 4));
	if (GET_MANA(ch) < 0)	AlterMove(ch, -(GET_MANA(ch) * 3));

	if (amount > 40) {
		CHAR_SKILL(ch).RollToLearn(ch, Method::Methods[method].skill, dice(3, 6));
	}

}
