/* ************************************************************************
*   File: spell_parser.c                                Part of CircleMUD *
*  Usage: top-level magic routines; outside points of entry to magic sys. *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author: sfn $
*	$Date: 2001/02/04 15:57:58 $
*	$Revision: 1.13 $
************************************************************************ */

#include "structs.h"
#include "mud.base.h"
#include "characters.h"
#include "objects.h"
#include "rooms.h"
#include "combat.h"
#include "scripts.h"
#include "utils.h"
#include "interpreter.h"
#include "spells.h"
#include "handler.h"
#include "find.h"
#include "comm.h"
#include "db.h"
#include "event.h"
#include "buffer.h"

// extern void skill_learn(Character *ch, int skill);
// extern SInt8 skill_chance(Character *ch, int skill);
extern int in_melee_range(Character *ch, Character *victim, int attacktype);
extern void perform_act(const char *orig, const Character *ch, const MUDObject *obj,
		    CPtr vict_obj, Character *to);
// extern EVENTFUNC(concentrate_event);
void maintenance_mana_cost(Character *ch, int cost, bool &broken);

extern Character *get_char_vis(const Character * ch, const char *name);
extern int cast_spell(Character *ch, Character *tch, Object *tobj, SpellData *spell);


SpellData::SpellData(void) : mana(0), spellnum(0), hp(0), isprayer(false) {}
SpellData::~SpellData(void) {}


ACMD(do_magic);

/*
 * This arrangement is pretty stupid, but the number of skills is limited by
 * the playerfile.	We can arbitrarily increase the number of skills by
 * increasing the space in the playerfile. Meanwhile, this should provide
 * ample slots for skills.
 */


struct syllable syls[] = {
	{" ", " "},
	{"ar", "abra"},
	{"au", "aorva"},
	{"ate", "i"},
	{"cau", "kada uli"},
	{"blind", "gul nashfe"},
	{"bur", "mosa"},
	{"cu", "judi"},
	{"de", "oculo"},
	{"dis", "malor"},
	{"ect", "kamina"},
	{"en", "uns"},
	{"gro", "cra"},
	{"light", "dies"},
	{"lo", "hi"},
	{"magi", "nysh kari"},
	{"mon", "xibar"},
	{"mor", "zak"},
	{"move", "sido"},
	{"ness", "lacri"},
	{"ning", "iyya"},
	{"of", "viy"},
	{"or", "uhl"},
	{"per", "duda"},
	{"ra", "gru"},
	{"re", "candus"},
	{"son", "sabru"},
	{"tect", "shavra"},
	{"tri", "cula"},
	{"ven", "nofo"},
	{"word", "nishin"},
	{"rech", "santn"},
	{"phan", "raji"},
	{"hol", "olm"},
	{"aci", "losudo"},
	{"psy", "vashri"},
	{"fea", "yum"},
	{"fli", "con"},
	{"a", "ia"}, {"b", "sh"}, {"c", "q"}, {"d", "m"}, {"e", "iy"}, {"f", "ch"},
	{"g", "b"}, {"h", "f"}, {"i", "au"}, {"j", "l"}, {"k", "t"}, {"l", "r"},
	{"m", "v"}, {"n", "k"}, {"o", "a"}, {"p", "r"}, {"q", "w"}, {"r", "s"},
	{"s", "g"}, {"t", "h"}, {"u", "e"}, {"v", "z"}, {"w", "x"}, {"x", "n"},
	{"y", "o"}, {"z", "k"}, {"", ""}

};


struct syllable drunk_syls[] = {
	{" ", " "},
	{"sh", "shhh"},
	{"ch", "chsh"},
	{"ct", "ksh"},
	{"tion", "shun"},
	{"sion", "shun"},
	{"a", "a"}, {"b", "b"}, {"c", "c"}, {"d", "d"}, {"e", "e"}, {"f", "ff"},
	{"g", "g"}, {"h", "h"}, {"i", "i"}, {"j", "j"}, {"k", "k"}, {"l", "ll"},
	{"m", "m"}, {"n", "n"}, {"o", "o"}, {"p", "p"}, {"q", "q"}, {"r", "rr"},
	{"s", "sh"}, {"t", "t"}, {"u", "uh"}, {"v", "v"}, {"w", "w"}, {"x", "x"},
	{"y", "y"}, {"z", "zzz"}, {"", ""}

};


namespace ManualSpells {
	const char * Names[] = {
		"charm",
		"create water",
		"detect poison",
		"enchant weapon",
		"identify",
		"locate object",
		"summon",
		"word of recall",
		"recharge",
		"planeshift",
		"teleport",
		"horrific illusion",
		"portal",
		"bind",
		"raise dead",
		"reincarnate",
		"\n"
	};


	enum {
		Charm,
		CreateWater,
		DetectPoison,
		EnchantWeapon,
		Identify,
		LocateObject,
		Summon,
		WordOfRecall,
		Recharge,
		Planeshift,
		Teleport,
		HorrificIllusion,
		Portal,
		Bind,
		RaiseDead,
		Reincarnate,
		MAX
	};
}


#define SINFO Skills[spellnum]

// An attempt to fix a stupid link error. -DH
/*
 * cast_spell is used generically to cast any spoken spell, assuming we
 * already have the target char/obj and spell number.	It checks all
 * restrictions, etc., prints the words, etc.
 *
 * Entry point for NPC casts.	Recommended entry point for spells cast
 * by NPCs via specprocs.
 */

int cast_spell(Character *ch, Character *tch, Object *tobj, SpellData *spell)
{
	int spellnum = spell->spellnum;

	if (spellnum < 0 || spellnum > TOP_SKILLO_DEFINE) {
		log("SYSERR: cast_spell trying to call spellnum %d\n", spellnum);
		return 0;
	}

	if (!((1 << GET_POS(ch)) & SINFO->min_position)) {
		switch (GET_POS(ch)) {
		case POS_DEAD:
			ch->Send("Lie still; you are DEAD!!! :-(\r\n");
			break;
		case POS_INCAP:
		case POS_MORTALLYW:
			ch->Send("You are in a pretty bad shape, unable to do anything!\r\n");
			break;
		case POS_STUNNED:
			ch->Send("All you can do right now is think about the stars!\r\n");
			break;
		case POS_SLEEPING:
			ch->Send("You dream about great magical powers.\r\n");
			break;
		case POS_RESTING:
			ch->Send("Nah... You feel too relaxed to do that..\r\n");
			break;
		case POS_SITTING:
			ch->Send("Maybe you should get on your feet first?\r\n");
			break;
		case POS_STANDING:
			ch->Send("You can't do this while standing.\r\n");
			break;
		default:
			break;
		}
		return 0;
	}
	if (!(P_FI & SINFO->min_position) && FIGHTING(ch)) {
		ch->Send("Impossible!  You can't concentrate enough!\r\n");
		return 0;
	}
	if (!(P_RI & SINFO->min_position) && RIDING(ch)) {
		ch->Send("Maybe you should dismount first.\r\n");
		return 0;
	}
	if (AFF_FLAGGED(ch, AffectBit::Charm) && (ch->master == tch)) {
		ch->Send("You are afraid you might hurt your master!\r\n");
		return 0;
	}
	if ((tch != ch) && IS_SET(SINFO->target, TAR_SELF_ONLY)) {
		ch->Send("You can only cast this spell upon yourself!\r\n");
		return 0;
	}
	if ((tch == ch) && IS_SET(SINFO->target, TAR_NOT_SELF)) {
		ch->Send("You cannot cast this spell upon yourself!\r\n");
		return 0;
	}
	if (IS_SET(SINFO->routines, MAG_GROUPS) && !AFF_FLAGGED(ch, AffectBit::Group)) {
		ch->Send("You can't cast this spell if you're not in a group!\r\n");
		return 0;
	}

	if ((tch == ch) && IS_SET(SINFO->target, TAR_NOT_SELF)) {
		ch->Send("You cannot cast this spell upon yourself!\r\n");
		return 0;
	}

	if (IS_SET(SINFO->target, TAR_MELEE) && (!in_melee_range(ch, tch, spellnum))) {
		ch->Send("Your target is not within reach!\r\n");
		return 0;
	}


	return 1;
}




/* say_spell erodes buf, buf1, buf2 */
void say_spell(Character * ch, int spellnum, Character * tch, Object * tobj)
{
	char lbuf[256];

	Character *i;
	int j, ofs = 0;

	*buf = '\0';
	strcpy(lbuf, Skill::Name(spellnum));

	garble_text(lbuf, buf, syls, 250, -10);

#ifdef GOT_RID_OF_IT
	while (*(lbuf + ofs)) {
		for (j = 0; *(syls[j].org); j++) {
			if (!strncmp(syls[j].org, lbuf + ofs, strlen(syls[j].org))) {
				strcat(buf, syls[j].news);
				ofs += strlen(syls[j].org);
			}
		}
	}
#endif

	if (tch != NULL && IN_ROOM(tch) == IN_ROOM(ch)) {
		if (tch == ch)
			sprintf(lbuf, "$n closes $s eyes and utters the words, '%%s'.");
		else
			sprintf(lbuf, "$n stares at $N and utters the words, '%%s'.");
	} else if (tobj != NULL &&
			 ((IN_ROOM(tobj) == IN_ROOM(ch)) || (tobj->CarriedBy() == ch)))
		sprintf(lbuf, "$n stares at $p and utters the words, '%%s'.");
	else
		sprintf(lbuf, "$n utters the words, '%%s'.");

//	sprintf(buf1, lbuf, spells[spellnum]);
	sprintf(buf2, lbuf, buf);

	START_ITER(iter, i, Character *, world[IN_ROOM(ch)].people) {
		if (i == ch || i == tch || !i->desc || !AWAKE(i))
			continue;
//		if (GET_CLASS(ch) == GET_CLASS(i))
//			perform_act(buf1, ch, tobj, tch, i);
//		else
			perform_act(buf2, ch, tobj, tch, i);
	}

	if (tch != NULL && tch != ch && IN_ROOM(tch) == IN_ROOM(ch)) {
		sprintf(buf1, "$n stares at you and utters the words, '%s'.", buf);
//			GET_CLASS(ch) == GET_CLASS(tch) ? spells[spellnum] : buf);
		act(buf1, FALSE, ch, NULL, tch, TO_VICT);
	}
}


bool spell_effect_cvict(int level, Character * ch, Character * victim, SInt16 spellnum)
{
	bool worked = false;
//	static Script	script(Datatypes::Character);
//	static Trigger	trig;
	static char		field[20];
	Property::Scripted		*property;

	if (IS_SET(SINFO->routines, MAG_AFFECTS)) {
		mag_affects(level, ch, victim, spellnum);
		worked = true;
	}

	if (IS_SET(SINFO->routines, MAG_UNAFFECTS)) {
		mag_unaffects(level, ch, victim, spellnum);
		worked = true;
	}

	if (IS_SET(SINFO->routines, MAG_POINTS)) {
		mag_points(level, ch, victim, spellnum);
		worked = true;
	}

	if ((IN_ROOM(ch) == NOWHERE) || (victim && PURGED(victim)))
		return true;

	if (IS_SET(SINFO->routines, MAG_DAMAGE)) {
		mag_damage(level, ch, victim, spellnum);
		worked = true;
	}

	if ((IN_ROOM(ch) == NOWHERE) || (victim && PURGED(victim)))
		return true;

	if (IS_SET(Skills[spellnum]->routines, MAG_SCRIPT)) {
		property = PROPERTY(spellnum, Scripted);
		property->RunScript(ch, victim);
	}

	return worked;
}


/*
 * This function is the very heart of the entire magic system.	All
 * invocations of all types of magic -- objects, spoken and unspoken PC
 * and NPC spells, the works -- all come through this function eventually.
 * This is also the entry point for non-spoken or unrestricted spells.
 * Spellnum 0 is legal but silently ignored here, to make callers simpler.
 */
int call_magic(Character * caster, Character * cvict,
				 Object * ovict, int spellnum, int level,
				 int casttype, char *arg)
{
	if (spellnum < 1 || spellnum > TOP_SKILLO_DEFINE)
		return 0;

	if ((IN_ROOM(caster) == NOWHERE) ||
		 (cvict && PURGED(cvict)) ||
		 (ovict && PURGED(ovict)))
		return 0;

	if (ROOM_FLAGGED(IN_ROOM(caster), ROOM_NOMAGIC) && !IS_STAFF(caster)) {
		caster->Send("Your magic fizzles out and dies.\r\n");
		act("$n's magic fizzles out and dies.", FALSE, caster, 0, 0, TO_ROOM);
		return 0;
	}

	if (!IS_STAFF(caster) && IS_SET(ROOM_FLAGS(IN_ROOM(caster)), ROOM_PEACEFUL) &&
			(SINFO->violent || IS_SET(SINFO->routines, MAG_DAMAGE))) {
		caster->Send("A flash of white light fills the room, "
					"dispelling your violent magic!\r\n");
		act("White light from no particular source suddenly fills "
			"the room, then vanishes.", FALSE, caster, 0, 0, TO_ROOM);
		return 0;
	}

	if (IS_SET(SINFO->routines, MAG_MATERIALS))
		if (!mag_materials(level, caster, cvict, spellnum, TRUE, TRUE))
			return 0;

	if (IS_SET(SINFO->routines, MAG_MANUAL)) {
		Property::Manual	*property = PROPERTY(spellnum, Manual);

		if (property) {
			switch (property->routine) {
			case ManualSpells::Charm:				MANUAL_SPELL(spell_charm); break;
			case ManualSpells::CreateWater:			MANUAL_SPELL(spell_create_water); break;
			case ManualSpells::DetectPoison:		MANUAL_SPELL(spell_detect_poison); break;
			case ManualSpells::EnchantWeapon:		MANUAL_SPELL(spell_enchant_weapon); break;
			case ManualSpells::Identify:			MANUAL_SPELL(spell_identify); break;
			case ManualSpells::LocateObject:		MANUAL_SPELL(spell_locate_object); break;
			case ManualSpells::Summon:				MANUAL_SPELL(spell_summon); break;
			case ManualSpells::WordOfRecall:		MANUAL_SPELL(spell_recall); break;
			case ManualSpells::Recharge:			MANUAL_SPELL(spell_recharge); break;
			case ManualSpells::Planeshift:			MANUAL_SPELL(spell_planeshift); break;
			case ManualSpells::Teleport:			MANUAL_SPELL(spell_teleport); break;
			case ManualSpells::HorrificIllusion:	MANUAL_SPELL(spell_horrific_illusion); break;
			case ManualSpells::Portal:				MANUAL_SPELL(spell_portal); break;
			case ManualSpells::Bind:				MANUAL_SPELL(spell_bind); break;
			case ManualSpells::RaiseDead:			MANUAL_SPELL(spell_raise_dead); break;
			case ManualSpells::Reincarnate:			MANUAL_SPELL(spell_reincarnate); break;
			default:								break;
			}
		}
	}

	if ((IN_ROOM(caster) == NOWHERE) || (cvict && PURGED(cvict)) || (ovict && PURGED(ovict)))
		return 1;

	if (IS_SET(SINFO->routines, MAG_ROOM))
		mag_room(level, caster, spellnum);

	if (IS_SET(SINFO->routines, MAG_SUMMONS))
		mag_summons(level, caster, ovict, spellnum);

	if (IS_SET(SINFO->routines, MAG_CREATIONS))
		mag_creations(level, caster, spellnum);

	if (IS_SET(SINFO->routines, MAG_AREAS))
		mag_areas(level, caster, cvict, spellnum);
	else if (cvict) 
		return spell_effect_cvict(level, caster, cvict, spellnum);
	else if (IS_SET(SINFO->routines, MAG_ALTER_OBJS))
		mag_alter_objs(level, caster, ovict, spellnum);

	return 1;
}


/*
 * mag_objectmagic: This is the entry-point for all magic items.	This should
 * only be called by the 'quaff', 'use', 'recite', etc. routines.
 *
 * For reference, object values 0-3:
 * staff	- [0]	level	[1] max charges	[2] num charges	[3] spell num
 * wand	 - [0]	level	[1] max charges	[2] num charges	[3] spell num
 * scroll - [0]	level	[1] spell num	[2] spell num	[3] spell num
 * potion - [0] level	[1] spell num	[2] spell num	[3] spell num
 *
 * Staves and wands will default to level 14 if the level is not specified;
 * the DikuMUD format did not specify staff and wand levels in the world
 * files (this is a CircleMUD enhancement).
 */

void mag_objectmagic(Character * ch, Object * obj,
							char *argument)
{
	int i, k;
	Character *tch = NULL, *next_tch;
	Object *tobj = NULL;
	char strtochar[MAX_INPUT_LENGTH], strtoroom[MAX_INPUT_LENGTH];

	*strtochar = '\0';
	*strtoroom = '\0';

	// Set up the action descriptions	// CHANGEPOINT
	// if (obj->SSADesc())	adesc_lines(obj->ADesc(), strtochar, strtoroom);

	one_argument(argument, arg);

	k = generic_find(arg, FIND_CHAR_ROOM | FIND_OBJ_INV | FIND_OBJ_ROOM |
			 FIND_OBJ_EQUIP, ch, &tch, &tobj);

	switch (GET_OBJ_TYPE(obj)) {
	case ITEM_STAFF:
		if (!*strtochar) 	strcpy(strtochar, "You hold $p high in the air...");
		if (!*strtoroom) 	strcpy(strtoroom, "$n holds $p high in the air.");

		act(strtochar, FALSE, ch, obj, 0, TO_CHAR);
		act(strtoroom, FALSE, ch, obj, 0, TO_ROOM);

		if (GET_OBJ_VAL(obj, 2) <= 0) {
			act("It seems powerless.", FALSE, ch, obj, 0, TO_CHAR);
			act("Nothing seems to happen.", FALSE, ch, obj, 0, TO_ROOM);
		} else {
			GET_OBJ_VAL(obj, 2)--;
			WAIT_STATE(ch, PULSE_VIOLENCE);
			START_ITER(iter, tch, Character *, world[IN_ROOM(ch)].people) {
				if (ch == tch)	continue;
				if (GET_OBJ_VAL(obj, 0))
					call_magic(ch, tch, NULL, GET_OBJ_VAL(obj, 3), GET_OBJ_VAL(obj, 0), CAST_STAFF, NULL);
				else
					call_magic(ch, tch, NULL, GET_OBJ_VAL(obj, 3), DEFAULT_STAFF_LVL, CAST_STAFF, NULL);
			}
		}
		break;
	case ITEM_WAND:
		if (k == FIND_CHAR_ROOM) {
			if (tch == ch) {
				if (!*strtochar)	strcpy(strtochar, "You point $p at yourself.");
				if (!*strtoroom)	strcpy(strtoroom, "$n points $p at $mself.");

				act(strtochar, FALSE, ch, obj, 0, TO_CHAR);
				act(strtoroom, FALSE, ch, obj, 0, TO_ROOM);
			} else {
				if (!*strtochar)	strcpy(strtochar, "You point $p at $N.");
				if (!*strtoroom) 	strcpy(strtoroom, "$n points $p at $N.");

				act(strtochar, FALSE, ch, obj, tch, TO_CHAR);
				act(strtoroom, FALSE, ch, obj, tch, TO_ROOM);
			}
		} else if (tobj != NULL) {
			if (!*strtochar)	strcpy(strtochar, "You point $p at $P.");
			if (!*strtoroom)	strcpy(strtoroom, "$n points $p at $P.");

			act(strtochar, FALSE, ch, obj, tobj, TO_CHAR);
			act(strtoroom, TRUE, ch, obj, tobj, TO_ROOM);
		} else {
			act("At what should $p be pointed?", FALSE, ch, obj, NULL, TO_CHAR);
			return;
		}

		if (GET_OBJ_VAL(obj, 2) <= 0) {
			act("It seems powerless.", FALSE, ch, obj, 0, TO_CHAR);
			act("Nothing seems to happen.", FALSE, ch, obj, 0, TO_ROOM);
			return;
		}
		GET_OBJ_VAL(obj, 2)--;
		WAIT_STATE(ch, PULSE_VIOLENCE);
		if (GET_OBJ_VAL(obj, 0))
			call_magic(ch, tch, tobj, GET_OBJ_VAL(obj, 3), GET_OBJ_VAL(obj, 0), CAST_WAND, NULL);
		else	call_magic(ch, tch, tobj, GET_OBJ_VAL(obj, 3),
		 DEFAULT_WAND_LVL, CAST_WAND, NULL);
		break;
	case ITEM_SCROLL:
		if (*arg) {
			if (!k) {
				act("There is nothing to here to affect with $p.", FALSE, ch, obj, NULL, TO_CHAR);
				return;
			}
		} else
			tch = ch;

		if (!*strtochar)	strcpy(strtochar, "You recite $p which dissolves.");
		if (!*strtoroom)	strcpy(strtoroom, "$n recites $p.");

		act(strtochar, TRUE, ch, obj, 0, TO_CHAR);
		act(strtoroom, FALSE, ch, obj, NULL, TO_ROOM);

		for (i = 1; i < 4; i++) {
			if (!(call_magic(ch, tch, tobj, GET_OBJ_VAL(obj, i),
					 GET_OBJ_VAL(obj, 0), CAST_SCROLL, NULL)))
			break;
		}

		if (obj != NULL)	obj->Extract();
		WAIT_STATE(ch, PULSE_VIOLENCE);
		break;
	case ITEM_POTION:
		tch = ch;
		if ((GET_COND(ch, DRUNK) > 10) && (GET_COND(ch, THIRST) > 0)) {
			/* The pig is drunk */
			ch->Send("You can't seem to get close enough to your mouth.\r\n");
			act("$n tries to drink but misses $s mouth!", TRUE, ch, 0, 0, TO_ROOM);
			return;
		}
		if (GET_COND(ch, THIRST) > 23) {
			ch->Send("Your stomach can't contain anymore!\r\n");
			return;
		}

		if (!*strtochar)	strcpy(strtochar, "You quaff $p.");
		if (!*strtoroom)	strcpy(strtoroom, "$n quaffs $p.");

		act(strtochar, TRUE, ch, obj, 0, TO_CHAR);
		act(strtoroom, FALSE, ch, obj, NULL, TO_ROOM);

		if (GET_COND(ch, THIRST) >= 0)	(GET_COND(ch, THIRST)) += 1;
		if (GET_COND(ch, THIRST) > 20)	ch->Send("You don't feel thirsty any more.\r\n");

		WAIT_STATE(ch, PULSE_VIOLENCE);

		for (i = 1; i < 4; i++) {
			if (!(call_magic(ch, ch, NULL, GET_OBJ_VAL(obj, i),
					 GET_OBJ_VAL(obj, 0), CAST_POTION, NULL)))
				break;
		}

		if (obj != NULL)	obj->Extract();
		break;
	default:
		log("SYSERR: Unknown object_type %d in mag_objectmagic", GET_OBJ_TYPE(obj));
		break;
	}
}


/*
 * finds the target of spell spellnum cast by ch with arg
 * if found, tobj or tch is set to the target.	returns
 * TRUE if target is found, else FALSE
 */
int find_spell_target(Character *ch, char *arg, int spellnum,
					Character **tch, Object **tobj)
{
	int i, j, target = FALSE;

	if (IS_SET(SINFO->target, TAR_IGNORE)) {
		target = TRUE;
	} else if (arg != NULL && *arg) {
		if (!target && (IS_SET(SINFO->target, TAR_CHAR_ROOM))) {
			if ((*tch = get_char_vis(ch, arg, FIND_CHAR_ROOM)) != NULL)
				target = TRUE;
		}
		if (!target && IS_SET(SINFO->target, TAR_CHAR_WORLD))
			if ((*tch = get_char_vis(ch, arg, FIND_CHAR_WORLD)))
				target = TRUE;

		if (!target && IS_SET(SINFO->target, TAR_OBJ_INV))
			if ((*tobj = get_obj_in_list_vis(ch, arg, ch->contents)))
				target = TRUE;

		if (!target && IS_SET(SINFO->target, TAR_OBJ_EQUIP))
			if ((*tobj = get_object_in_equip_vis(ch, arg, ch->equipment, &j)))
				target = TRUE;

		if (!target && IS_SET(SINFO->target, TAR_OBJ_ROOM))
			if ((*tobj = get_obj_in_list_vis(ch, arg, world[IN_ROOM(ch)].contents)))
				target = TRUE;

		if (!target && IS_SET(SINFO->target, TAR_OBJ_WORLD))
			if ((*tobj = get_obj_vis(ch, arg)))
				target = TRUE;

	} else {			/* if target string is empty */
		if (!target && IS_SET(SINFO->target, TAR_FIGHT_SELF))
			if (FIGHTING(ch) != NULL) {
				*tch = ch;
				target = TRUE;
			}
		if (!target && IS_SET(SINFO->target, TAR_FIGHT_VICT))
			if ((FIGHTING(ch) != NULL) && (IN_ROOM(ch) == IN_ROOM(FIGHTING(ch))) &&
		!IS_SET(SINFO->target, TAR_CHAR_WORLD)) {
				/* extra checks made to make sure target hasn't fled */
				*tch = FIGHTING(ch);
				target = TRUE;
			}
		/* if no target specified, and the spell isn't violent, default to self */
		if (!target && IS_SET(SINFO->target, TAR_CHAR_ROOM) && !SINFO->violent) {
			*tch = ch;
			target = TRUE;
		}
	}

	return target;
}


void cleric_sphere_cost(Character *ch, SInt16 &defaultnum, SInt16 &spellnum)
{
	SInt16 defaultpts;

	defaultpts = CHAR_SKILL(ch).GetSkillPts(defaultnum, NULL);
	defaultpts -= Skills[spellnum]->mana / Number(3, 10);
	defaultpts = MAX(defaultpts, 0);

	SET_SKILL(ch, defaultnum, defaultpts);
}


class SpellEvent : public ActionEvent {
public:
						SpellEvent(time_t when, Character &caster, SpellData &spl, Character *victim, Object *obj) :
						ActionEvent(when, caster), spell(&spl), tch(victim), tobj(obj) {}
					
	SpellData				*spell;
	SafePtr<Character>		tch;
	SafePtr<Object>			tobj;

	time_t					Execute(void);
};


time_t			SpellEvent::Execute(void)
{
	cast_event(ch, tch(), tobj(), spell);
	return 0;
}


class ConcentrateEvent : public ListedEvent {
public:
						ConcentrateEvent(time_t when, Character *caster, Concentration *c) :
						ListedEvent(when, &(caster->events)), ch(caster), concentration(c) {}
					
	Character 				*ch;
	Concentration			*concentration;

	time_t					Execute(void);
};


void maintenance_mana_cost(Character *ch, int cost, bool &broken)
{
	if (cost > GET_MANA(ch)) {
		AlterMana(ch, GET_MANA(ch));
		broken = TRUE;
	} else {
		AlterMana(ch, cost);
	}
}


time_t			ConcentrateEvent::Execute(void)
{
	long interval = PULSE_VIOLENCE;
	bool broken = FALSE;

	if (concentration->Affecting()) {
		switch (concentration->Affecting()->AffType()) {
		case SPELL_CHARM:
			maintenance_mana_cost(ch, 2, broken);
			concentration->Update(ch, broken);
			break;
		case SPELL_PORTAL:
			maintenance_mana_cost(ch, 3, broken);
			concentration->Update(ch, broken);
			break;
		case SPELL_BIND:
			maintenance_mana_cost(ch, 4, broken);
			concentration->Update(ch, broken);
			break;
		}
	}

	if (broken) {
		return 0;
	}

	return interval;	// Lexi's event queue will automatically reschedule.
}


#define CLERICAL_DEFAULT(num) 		(((num) >= FIRST_CSPHERE) && ((num) <= LAST_ORDINATION))
/*
 * do_cast is the entry point for PC-casted spells.	It parses the arguments,
 * determines the spell number and finds a target, throws the die to see if
 * the spell can be cast, checks for sufficient mana and subtracts it, and
 * passes control to cast_spell().
 */

ACMD(do_cast)
{
	Character *tch = NULL;
	Object *tobj = NULL;
	Scriptable *targ = NULL;
	char *s, *t;
	int mana = 0;
	SInt16 spellnum, target = 0, defaultnum = 0, defaulttype = 0;
	SInt8 spellchance, defaultmod;
	SpellData	*spell;

	if (DISADVANTAGED(ch, Disadvantages::NoMagic)) {
		ch->Send("You don't need this silly hocus-pocus.\r\n");
		return;
	}

	if (AFF_FLAGGED(ch, AffectBit::Charm)) {
		ch->Send("Oh, no.  Only your master should cast that!\r\n");
		return;
	}

	/* prevent from casting multiple spells at a time (e.g. by force) */
	if (GET_ACTION(ch)) {
		ch->Send("You are too busy to do this!\r\n");
		return;
	}

	/* get: blank, spell name, target name */
	s = strtok(argument - 1, "'");

	if (s == NULL) {
		ch->Send("Cast what where?\r\n");
		return;
	}

	s = strtok(NULL, "'");
	if (s == NULL) {
		ch->Send("Spell names must be enclosed in the Magic Symbols: '\r\n");
		return;
	}
	t = strtok(NULL, "\0");

	// log("Attempting to cast %s, Targetting %s", s, t);

	/* spellnum = search_block(s, spells, 0); */
	spellnum = Skill::Number(s, Skill::SPELL);
	if (spellnum < 0) {
		spellnum = Skill::Number(s, Skill::DEFAULT);
	}

	if (spellnum < 0) {
		ch->Send("Cast what?!?\r\n");
		return;
	}

	if (!IS_NPC(ch)) {
		defaulttype = Skill::ROOT;
	}

	CHAR_SKILL(ch).FindBestRoll(ch, spellnum, defaultnum, spellchance, defaultmod, defaulttype);

	if (spellchance == SKILL_NOCHANCE) {
		ch->Send("You are unfamiliar with that spell.\r\n");
		if (IS_NPC(ch)) {
			log ("ERROR:	%s (%d) attempting to cast %s without skill.", ch->Name(), ch->Virtual(), s);
		}
		return;
	}

	/* Find the target */
	if (t != NULL) {
		one_argument(strcpy(arg, t), t);
		skip_spaces(t);
	}

	target = find_spell_target(ch, t, spellnum, &tch, &tobj);

	if (!target && (!t || !*t)) {
		sprintf(buf, "Upon %s should the spell be cast?\r\n",
			IS_SET(SINFO->target, TAR_OBJ_ROOM | TAR_OBJ_INV | TAR_OBJ_WORLD) ?
			"what" : "whom");
		ch->Send(buf);
		return;
	}
	if (target && (targ == ch) && SINFO->violent) {
		ch->Send("You shouldn't cast that on yourself -- could be bad for your health!\r\n");
		return;
	}

	if (!target) {
		ch->Send("Cannot find the target of your spell!\r\n");
		return;
	}

	mana = Skills[spellnum]->mana;

	if ((mana > 0) && (ENERGY(ch).Check(ch, defaultnum) < mana) && !IS_STAFF(ch) && !IS_NPC(ch)) {
		ch->Sendf("You haven't the %s energy to cast that spell!  Required: %d\r\n", 
				Skill::Name(defaultnum), mana);
		return;
	}

	spell = new SpellData;

	spell->mana = mana;
	spell->spellnum = spellnum;
	spell->hp = GET_HIT(ch);
	spell->isprayer = false;
	spell->arg = arg;

	if (!cast_spell(ch, tch, tobj, spell)) {
		delete spell;
		return;
	}

	/* half mana now, half when cast */
	ENERGY(ch).Draw(ch, defaultnum, mana / 2);
	// AlterMana(ch, mana / 2);

	WAIT_STATE(ch, SINFO->delay + SINFO->lag + 1);

	if (SINFO->delay == 0) {
		cast_event(ch, tch, tobj, spell);
		delete spell;
	} else {
		// CHANGEPOINT:	Replace 0 with a bitvector for event cancellation
		new SpellEvent(SINFO->delay, *ch, *spell, tch, tobj);

		ch->Send("You begin to cast a spell.\r\n");
		act("$n begins to cast a spell.", FALSE, ch, NULL, NULL, TO_ROOM);

		say_spell(ch, spellnum, tch, tobj);
	}
}


ACMD(do_pray)
{
	Character *tch = NULL;
	Object *tobj = NULL;
	char *s, *t;
	int mana = 0;
	SInt16 spellnum, target = 0, defaultnum = 0, defaulttype = 0;
	SInt8 spellchance, defaultmod;
	SpellData *spell = NULL;

	if (DISADVANTAGED(ch, Disadvantages::Atheist)) {
		ch->Send("You feel foolish praying like this.\r\n");
		return;
	}

	if (AFF_FLAGGED(ch, AffectBit::Charm)) {
		ch->Send("You don't feel very motivated to do that right now!\r\n");
		return;
	}

	/* prevent from casting multiple spells at a time (e.g. by force) */
	if (GET_ACTION(ch)) {
		ch->Send("You are too busy to do this!\r\n");
		return;
	}

	/* get: blank, spell name, target name */
	s = strtok(argument - 1, "'");

	if (s == NULL) {
		ch->Send("Pray for what, and where?\r\n");
		return;
	}

	s = strtok(NULL, "'");
	if (s == NULL) {
		ch->Send("Prayers must be enclosed in the Holy Symbols: '\r\n");
		return;
	}
	t = strtok(NULL, "\0");

	// log("Attempting to cast %s, Targetting %s", s, t);

	/* spellnum = search_block(s, spells, 0); */
	spellnum = Skill::Number(s, Skill::PRAYER);
	if (spellnum < 0) {
		spellnum = Skill::Number(s, Skill::DEFAULT);
	}

	if (spellnum < 0) {
		ch->Send("Pray for what to happen?\r\n");
		return;
	}

	if (!IS_NPC(ch)) {
		defaulttype = Skill::SPHERE;
	}

	CHAR_SKILL(ch).FindBestRoll(ch, spellnum, defaultnum, spellchance, defaultmod, defaulttype);

	if (spellchance == SKILL_NOCHANCE) {
		ch->Send("You have not been granted that power.\r\n");
		if (IS_NPC(ch)) {
			log ("ERROR:	%s (%d) attempting to cast %s without skill.", ch->Name(), ch->Virtual(), s);
		}
		return;
	}

	/* Find the target */
	if (t != NULL) {
		one_argument(strcpy(arg, t), t);
		skip_spaces(t);
	}

	target = find_spell_target(ch, t, spellnum, &tch, &tobj);

	if (!target && (!t || !*t)) {
		strcpy(buf2, "Appeal to the heavens regarding %s?\r\n");
		sprintf(buf, buf2, IS_SET(SINFO->target, TAR_OBJ_ROOM | TAR_OBJ_INV | TAR_OBJ_WORLD) ? "what" : "whom");
		ch->Send(buf);
		return;
	}
	if (target && (tch == ch) && SINFO->violent) {
		ch->Send("Bringing that upon yourself could be calamitous!\r\n");
		return;
	}

	if (!target) {
		ch->Send("Cannot find the object of your prayer!\r\n");
		return;
	}

	spell = new SpellData;

	spell->mana = mana;
	spell->spellnum = spellnum;
	spell->hp = GET_HIT(ch);
	spell->isprayer = false;
	spell->arg = arg;

	if (!cast_spell(ch, tch, tobj, spell)) {
		delete spell;
		return;
	}

	/* half mana now, half when cast */
	ENERGY(ch).Draw(ch, defaultnum, mana / 2);
	// AlterMana(ch, mana / 2);

	WAIT_STATE(ch, SINFO->delay + SINFO->lag + 1);

	if (SINFO->delay == 0) {
		cast_event(ch, tch, tobj, spell);
		delete spell;
	} else {
		// CHANGEPOINT:	Replace 0 with a bitvector for event cancellation
		new SpellEvent(SINFO->delay, *ch, *spell, tch, tobj);

		ch->Send("You beseech the Powers for aid.\r\n");
		act("$n beseeches the Powers for aid.", FALSE, ch, NULL, NULL, TO_ROOM);

		say_spell(ch, spellnum, tch, tobj);
	}
}


int cast_event(Character *ch, Character *tch, Object *tobj, SpellData *spell)
{
	int mana, encumbrance_penalty = 0;
	SInt16 spellnum, defaultnum, defaulttype = 0;
	SInt8 found, chance, diceroll, defaultmod;
	SInt16 conc_breakchance = 0, conc_breaktype = 0;
	SkillKnown *skilldat = NULL;
	Concentration *tmp_con = NULL;
	bool isprayer;

	*arg = '\0';

	spellnum = spell->spellnum;
	mana = spell->mana;

	if (tch)	sprintf(arg, "%c%d", UID_CHAR, tch->ID());
	else if (tobj)	sprintf(arg, "%c%d", UID_CHAR, tobj->ID());

	found = find_spell_target(ch, arg, spellnum, &tch, &tobj);
	isprayer = spell->isprayer;
	strcpy(arg, spell->arg.c_str());

	/* find and make sure the target is still available */
	if (!found) {
		act("Your target is no longer available!", FALSE, ch, NULL, NULL, TO_CHAR);
		return 0;
	}

	/* check if character is still in casting position */
	if (!((1 << GET_POS(ch)) & SINFO->min_position)) {
		act("All that jostling broke your concentration!", FALSE, ch, NULL, NULL, TO_CHAR);
		return 0;
	}

	if (!(P_FI & SINFO->min_position) && FIGHTING(ch)) {
		act("You lose focus in the heat of the battle!", FALSE, ch, NULL, NULL, TO_CHAR);
		return 0;
	}

	if (!(P_RI & SINFO->min_position) && RIDING(ch)) {
		act("Your mount's jostling breaks your concentration!", FALSE, ch, NULL, NULL, TO_CHAR);
		return 0;
	}

	if (GET_HIT(ch) < (spell->hp)) {
		act("Your stinging wounds prevent you from focusing!", FALSE, ch, NULL, NULL, TO_CHAR);
		return 0;
	}

	if (IS_SET(SINFO->routines, MAG_GROUPS) && !AFF_FLAGGED(ch, AffectBit::Group)) {
		act("You are no longer part of a group!", FALSE, ch,
			NULL, NULL, TO_CHAR);
		return 0;
	}

	conc_breakchance = MAX(Number(-2, ch->concentrations.Count()), 0);

	if (conc_breakchance > 0) {
		switch (conc_breakchance) {
		case 0:		break;
		case 1:
			{
				START_ITER(iter, tmp_con, Concentration *, ch->concentrations) {
					if ((--conc_breakchance == 0)) {
						ch->concentrations.Remove(tmp_con);
						break;
					}
				}
			}
			break;
		case 2:
			ch->Send("You fail to maintain concentration on so many things at once!\r\n");
			return 0;
		default:
			{
				START_ITER(iter, tmp_con, Concentration *, ch->concentrations) {
					ch->concentrations.Remove(tmp_con);
					break;
				}
			}
			return 0;
		}
	}

	if (!IS_NPC(ch)) {
		defaulttype = spell->isprayer ? 0 : Skill::ROOT;
	}

	CHAR_SKILL(ch).FindBestRoll(ch, spellnum, defaultnum, chance, defaultmod, defaulttype);

	if ((mana > 0) && (ENERGY(ch).Check(ch, defaultnum) < mana) && !IS_STAFF(ch) && !IS_NPC(ch)) {
		switch (spell->isprayer) {
		case true:	ch->Send("You have not been granted that power!  Required: %d\r\n"); break;
		default: 	ch->Send("You haven't the energy left to finish your spell!\r\n");	break;
		}
		return 0;
	}

	if (CLERICAL_DEFAULT(defaultnum))
		diceroll = chance - dice(3, 6);
	else {
		encumbrance_penalty = GET_ENCUMBRANCE(ch, IS_CARRYING_W(ch));

		if (encumbrance_penalty < 0) {
			encumbrance_penalty = 0;
		} else {
			chance -= encumbrance_penalty;
		}

		diceroll = CHAR_SKILL(ch).RollSkill(ch, spellnum, defaultnum, chance);
		CHAR_SKILL(ch).RollToLearn(ch, defaultnum, diceroll);

		if ((diceroll < 0) && (diceroll >= -encumbrance_penalty)) {
			ch->Sendf("You are too encumbered to finish your %s!\r\n", spell->isprayer ? "prayer" : "spell");
			return 0;
		}
	}

	if (diceroll < 0) {
		WAIT_STATE(ch, PULSE_VIOLENCE);

		if (!tch || !Combat::SkillMessage(0, ch, tch, spellnum)) {
			switch (spell->isprayer) {
			case true:	ch->Send("You sense no response to your appeal.\r\n"); break;
			default: 	ch->Send("You lost your focus!\r\n");	break;
			}
		}
		if (SINFO->violent && tch && IS_NPC(tch))
			Combat::CheckFighting(ch, tch);
	} else {
		if (spell->isprayer) {
			strcpy(buf3, "Divine power rushes through you!");
			if (!IS_NPC(ch) && SKL_IS_CSPHERE(defaultnum))
				cleric_sphere_cost(ch, defaultnum, spellnum);
		} else {
			strcpy(buf3, "You complete your spell.");
		}

		act(buf3, FALSE, ch, NULL, NULL, TO_CHAR);
		/*say_spell(ch, spellnum, tch, tobj);*/

		// CHANGEPOINT:	GET_INT below was GET_LEVEL.	Find a better substitute!
		/* call magic returns 1 on success; subtract mana */
		// If the cast succeeds by ten or more on their skillroll
		// (a critical success), they lose no mana at this stage.
		if (call_magic(ch, tch, tobj, spellnum, MAX(chance, 1), CAST_SPELL, arg)) {
			if ((!spell->isprayer) && (diceroll < 10) && (mana > 0) && !IS_NPC(ch))
				ENERGY(ch).Draw(ch, defaultnum, mana);
				// AlterMana(ch, mana);
		}
	}

	return 0;
}


// Concentration breaking routine
int do_break_concentration(Character *ch, int type)
{
	Concentration *temp_con = NULL;
	bool itsbroken = true;	// Needed for passing by reference
	int brokeone = FALSE;

	START_ITER(iter, temp_con, Concentration *, ch->concentrations) {
		if (temp_con->affect && (temp_con->affect->type == type)) {
			ch->concentrations.Remove(temp_con);
			iter.Reset();
			temp_con->Update(ch, itsbroken);
			brokeone = TRUE;
		}
	}

	return brokeone;
}


ACMD(do_concentrate)
{
	int type = 0;
	Concentration *tmp_con = NULL;


	*buf2 = '\0';
	*buf3 = '\0';

	skip_spaces(argument);

	if (!*argument) {
		ch->Send("Concentrations:\r\n");
		if (ch->concentrations.Count()) {
			START_ITER(iter, tmp_con, Concentration *, ch->concentrations) {
				if (tmp_con->affect)	strcpy(buf3, Skill::Name(tmp_con->affect->type));
				else					strcpy(buf3, "n/a");

				if (tmp_con->target) 	strcpy(buf2, tmp_con->target->CalledName(ch));

				ch->Sendf("  Spell:  %-20s Target:  &cC%-21s&c0\r\n", buf3, buf2);
			}
		} else {
			ch->Send("None.\r\n");
			return;
		}

	} else {
		half_chop(argument, arg, buf3);
		if (is_abbrev(arg, "break")) {
			type = Skill::Number(buf3);

			if (type < 1) {
				ch->Send("This isn't a valid spell to break.\r\n");
				return;
			}
			if (do_break_concentration(ch, type))
				ch->Sendf("You break the spell of %s.\r\n", Skill::Name(type));
		} else {
        	do_magic(ch, argument, cmd, command, 1);
        }
	}

}


Concentration::Concentration() : affect(NULL), target(NULL), event(NULL)
{
}


void Concentration::BreakConcentration()
{
	Character *target_char = NULL;
	Object *target_obj = NULL;
	Room *target_room = NULL;
	Property::Concentrate	*conproperty = NULL;

	if ((affect) && (target)) {
		if ((affect->caster) && VALID_SKILL(affect->type)) {
			conproperty = PROPERTY(affect->type, Concentrate);
			if (conproperty->wearoff)	
				affect->caster->Send(conproperty->wearoff);
		}

		affect->caster = NULL; // Prevent recursive deletion of this object

		switch (target->DataType()) {
		case Datatypes::Character:
			target_char = (Character *) target;
			affect->Remove(target_char);
			break;
		case Datatypes::Object:
			target_obj = (Object *) target;
			switch (affect->type) {
				case SPELL_PORTAL:
					if ((IN_ROOM(target_obj) != NOWHERE) && (world[IN_ROOM(target_obj)].people.Count())) {
						act("$p vanishes in a cloud of smoke!",
							FALSE, world[IN_ROOM(target_obj)].people.Bottom(), target_obj, 0, TO_ROOM);
						act("$p vanishes in a cloud of smoke!",
							FALSE, world[IN_ROOM(target_obj)].people.Bottom(), target_obj, 0, TO_CHAR);
					}
					target_obj->Extract();
					break;
			}
			break;
		case Datatypes::Room:
			break;
		default:
			break;
		}
	}
	
}


void Concentration::Update(Character *ch, bool &broken)
{
	Character *target_char = NULL;
	Object *target_obj = NULL;
	Room *target_room = NULL;
	int type = -1;
	Property::Affects	*affproperty;

	switch (target->DataType()) {
	case Datatypes::Character:
		target_char = (Character *) target;
		break;
	case Datatypes::Object:
		target_obj = (Object *) target;
		break;
	case Datatypes::Room:
		target_room = (Room *) target;
		break;
	}

	if (affect)
		type = affect->type;

	if ((!broken) && (type == SPELL_PORTAL)) {
		if (GET_OBJ_VAL(target_obj, 0) > 0)
			--GET_OBJ_VAL(target_obj, 0);

		switch (GET_OBJ_VAL(target_obj, 0)) {
		case 1:
			if ((IN_ROOM(target_obj) != NOWHERE) && (world[IN_ROOM(target_obj)].people.Count())) {
			act("$p starts to fade!", FALSE, world[IN_ROOM(target_obj)].people.Top(), target_obj, 0, TO_ROOM);
			act("$p starts to fade!", FALSE, world[IN_ROOM(target_obj)].people.Top(), target_obj, 0, TO_CHAR);
			}
			break;
		case 0:
			broken = TRUE;
			break;
		default:
			break;
		}
	}

	if (broken) {
		switch (type) {
		case SPELL_PORTAL:
			if ((IN_ROOM(target_obj) != NOWHERE) && (world[IN_ROOM(target_obj)].people.Count())) {
				act("$p vanishes in a cloud of smoke!", FALSE, world[IN_ROOM(target_obj)].people.Top(), target_obj, 0, TO_ROOM);
				act("$p vanishes in a cloud of smoke!", FALSE, world[IN_ROOM(target_obj)].people.Top(), target_obj, 0, TO_CHAR);
			}
			target_obj->Extract();
			break;
		default:
			if (VALID_SKILL(type))
				affproperty = PROPERTY(type, Affects);

			if (affproperty && affproperty->fromtrig) {
				affproperty->FromScript(affect->caster, target);
			}
			
			if (!PURGED(target)) {
				if (Affect::AffectedBy(target, type))
					Affect::FromThing(target, type);
			}
			break;
		}
	}
}


Concentration::~Concentration()
{
	if (event) {
		event->Cancel();
		event = NULL;
	}

	if (target) {
		BreakConcentration();
		target = NULL;
	}
}


void Concentration::Join(Character *ch, long interval, GameData *conc_target, Affect *aff)
{
	affect = aff;
	target = conc_target;

	aff->caster = ch;

	ToChar(ch, this, interval);
}


void Concentration::ToChar(Character *ch, Concentration *con, long interval)
{
	if (interval > 0) {
		ConcentrateEvent *ae_data = new ConcentrateEvent(interval, ch, this);
	}

	ch->concentrations.Append(con);
}


void Concentration::FromChar(Character *ch, Concentration *con)
{
	ch->concentrations.Remove(con);
	delete con;
}


ACMD(do_magic)
{
	SInt16	method = -1,
    		root = -1,
            amount = 20;

	if (DISADVANTAGED(ch, Disadvantages::NoMagic)) {
		ch->Send("You don't need this silly hocus-pocus.\r\n");
		return;
	}

	if (GET_ACTION(ch)) {
		ch->Send("You are too busy to do this!\r\n");
		return;
	}

	half_chop(argument, arg, argument);

	if (subcmd) {
    	for (method = 0; method < Method::Methods.Count(); ++method) {
			if (is_abbrev(command, Method::Methods[method].Verb())) {
                break;
            }
        }

        if (method == Method::Methods.Count()) {
        	ch->Send("Bug in do_magic, warn the imps!\r\n");
            return;
        }

		if (*arg)
        	root = Skill::Number(arg, Skill::ROOT);

		if (root < 0) {
			ch->Send("And summon what form of energy?\r\n");
			return;
		} else if (SKILLCHANCE(ch, root, NULL) == SKILL_NOCHANCE) {
        	ch->Send("You cannot tap into that power.\r\n");
            return;
		}

		half_chop(argument, arg, argument);

		if (*arg)
			amount = MAX(atoi(arg), 1);

		Method::Methods[method].Start(ch, root, amount);
        return;

	} else {
		ch->Send("Energies summoned:\r\n");

		if (!ENERGY(ch).List(ch)) {
			ch->Send("None.\r\n");
		}
	}
}


ACMD(do_meditate)
{
	SInt16	sphere = -1,
    		ordination = -1,
            amount = 20;
	SInt8	chance = -120,
			mod = 0;
	bool	wrongalign = false;

	if (DISADVANTAGED(ch, Disadvantages::Atheist)) {
		ch->Send("This doesn't do much for you.\r\n");
		return;
	}

	if (GET_ACTION(ch)) {
		ch->Send("You are too busy to do this!\r\n");
		return;
	}

	if (!*argument) {
		ch->Send("And on what do you wish to meditate?\r\n");
		return;
	}

	sphere = Skill::Number(argument, Skill::SPHERE);

	CHAR_SKILL(ch).FindBestRoll(ch, sphere, ordination, chance, mod);

	if (ordination == sphere) {
		ch->Send("You receive no insight from your musings.\r\n");
		return;
	} else if (SKILLCHANCE(ch, ordination, NULL) == SKILL_NOCHANCE) {
		wrongalign = true;
	} else {
		switch (ordination) {
		case ORDINATION_PELENDRA:
			if (GET_ALIGNMENT(ch) < 0)
				wrongalign = true;
			break;
		case ORDINATION_TALVINDAS:
			if (GET_ALIGNMENT(ch) < 500 || GET_LAWFULNESS(ch) < 500)
				wrongalign = true;
			break;
		case ORDINATION_DYMORDIAN:
			if (GET_ALIGNMENT(ch) < -300 || GET_ALIGNMENT(ch) > 600)
				wrongalign = true;
			break;
		case ORDINATION_NYMRIS:
			if (GET_ALIGNMENT(ch) < -500 || GET_ALIGNMENT(ch) > 800 || GET_LAWFULNESS(ch) < 0)
				wrongalign = true;
			break;
		case ORDINATION_KHARSUS:
			if (GET_ALIGNMENT(ch) < -250 || GET_ALIGNMENT(ch) > 500 || GET_LAWFULNESS(ch) < 500)
				wrongalign = true;
			break;
		case ORDINATION_VAADH:
			if (GET_ALIGNMENT(ch) > 0 || GET_LAWFULNESS(ch) < 500)
				wrongalign = true;
			break;
		case ORDINATION_SHESTU:
			if (GET_ALIGNMENT(ch) > -500 || GET_LAWFULNESS(ch) > 0)
				wrongalign = true;
			break;
		}
	}

	if (wrongalign) {
	   	ch->Send("You sense that you have strayed.\r\n");
		return;
	}

	ch->Sendf("Ready to meditate %s through the %s\r\n", Skill::Name(sphere), Skill::Name(ordination));
}
