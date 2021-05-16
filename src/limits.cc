/* ************************************************************************
*   File: limits.c                                      Part of CircleMUD *
*  Usage: limits & gain funcs for HMV, exp, hunger/thirst, idle time      *
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
#include "characters.h"
#include "objects.h"
#include "rooms.h"
#include "comm.h"
#include "scripts.h"
#include "db.h"
#include "handler.h"
#include "find.h"
#include "event.h"
#include "combat.h"


extern int use_autowiz;
extern int min_wizlist_lev;
void Crash_rentsave(Character *ch);

void hour_update(void);
int graf(int age, int p0, int p1, int p2, int p3, int p4, int p5, int p6);
void check_autowiz(Character * ch);

void CheckRegenRates(Character *ch);

void timer_otrigger(Object *obj);

extern Map<VNum, Room>world;

/* When age < 15 return the value p0 */
/* When age in 15..29 calculate the line between p1 & p2 */
/* When age in 30..44 calculate the line between p2 & p3 */
/* When age in 45..59 calculate the line between p3 & p4 */
/* When age in 60..79 calculate the line between p4 & p5 */
/* When age >= 80 return the value p6 */
int graf(int age, int p0, int p1, int p2, int p3, int p4, int p5, int p6) {
	if (age < 15)
		return (p0);		/* < 15   */
	else if (age <= 29)
		return (int) (p1 + (((age - 15) * (p2 - p1)) / 15));	/* 15..29 */
	else if (age <= 44)
		return (int) (p2 + (((age - 30) * (p3 - p2)) / 15));	/* 30..44 */
	else if (age <= 59)
		return (int) (p3 + (((age - 45) * (p4 - p3)) / 15));	/* 45..59 */
	else if (age <= 79)
		return (int) (p4 + (((age - 60) * (p5 - p4)) / 20));	/* 60..79 */
	else
		return (p6);		/* >= 80 */
}


//	Player point types for events
#define REGEN_HIT      0
#define REGEN_MANA     1
#define REGEN_MOVE     2


class RegenEvent : public Event {
public:
						RegenEvent(time_t when, Character *c, SInt32 t);
						~RegenEvent(void);
					
	Character				*ch;
	SInt32		type;

	time_t					Execute(void);
};


RegenEvent::RegenEvent(time_t when, Character *c, SInt32 t) : Event(when), ch(c), type(t)
{
	GET_POINTS_EVENT(ch, type) = this;
}

	
RegenEvent::~RegenEvent(void)
{
	GET_POINTS_EVENT(ch, type) = NULL;
}

 
time_t	RegenEvent::Execute(void)
{
	SInt32		gain;
	
	if (GET_CLASS(ch) == CLASS_GHOST) {
		return 5; // They're really not supposed to regenerate.
	}

    if ((GET_MOVE(ch) < 0) && (GET_POS(ch) > POS_SLEEPING)) {
    	ch->Send("You collapse from exhaustion!\r\n");
		act("$n collapses from exhaustion!", FALSE, ch, NULL, NULL, TO_ROOM);
		ch->StopFighting();
		GET_POS(ch) = POS_SLEEPING;
		// Remove actions
		if (GET_ACTION(ch))	{
			GET_ACTION(ch)->Cancel();
		}
    }

	//	No help for the dying
	if ((GET_POS(ch) >= POS_STUNNED) && (GET_COND(ch, FULL) != 0) && (GET_COND(ch, THIRST) != 0)) {	
		switch (type) {
			case REGEN_HIT:
				// if (IS_SYNTHETIC(ch))
				//	break;
				
				if (!FIGHTING(ch))
					GET_HIT(ch) = MIN(GET_HIT(ch) + 1, (int)GET_MAX_HIT(ch));
				
				if (GET_POS(ch) <= POS_STUNNED)
					ch->UpdatePos();
				
				if (GET_HIT(ch) < GET_MAX_HIT(ch)) {
					gain = hit_gain(ch);
					return (/* PULSES_PER_MUD_HOUR / */ (gain ? gain : 1));
				}
				break;
			case REGEN_MANA:
				// if (!FIGHTING(ch))
				GET_MANA(ch) = MIN(GET_MANA(ch) + 1, GET_MAX_MANA(ch));
			  
				if (GET_MANA(ch) < GET_MAX_MANA(ch)) {
					gain = mana_gain(ch);
					return (/*PULSES_PER_MUD_HOUR / */(gain ? gain : 1));
				}
				break;
			case REGEN_MOVE:
				// if (!FIGHTING(ch))
				GET_MOVE(ch) = MIN(GET_MOVE(ch) + 1, (int)GET_MAX_MOVE(ch));
				
				if (GET_MOVE(ch) < GET_MAX_MOVE(ch)) {
					gain = move_gain(ch);
					return (/* PULSES_PER_MUD_HOUR / */(gain ? gain : 1));
				}
				break;
			default:
				log("SYSERR:  Unknown points event type %d", type);
				GET_POINTS_EVENT(ch, type) = NULL;
				return 0;
		}
	}
	GET_POINTS_EVENT(ch, type) = NULL;
	return 0;
}


//	Subtract amount of HP from ch's current and start points event
void AlterHit(Character *ch, SInt32 amount) {
	SInt32		time;
	SInt32		gain;
	
	GET_HIT(ch) = MIN(GET_HIT(ch) - amount, (int)GET_MAX_HIT(ch));
	
	ch->UpdatePos();
	
	if (GET_POS(ch) < POS_INCAP)  // || (GET_RACE(ch) == RACE_SYNTHETIC))
		return;
	
	if ((GET_HIT(ch) < GET_MAX_HIT(ch)) && !GET_POINTS_EVENT(ch, REGEN_HIT)) {
		gain = hit_gain(ch);
		time = /* PULSES_PER_MUD_HOUR / */ (gain ? gain : 1);
		new RegenEvent(time, ch, REGEN_HIT);
	}
	
	if (amount >= 0) {
		ch->UpdatePos();
	}
}

/* 
//	Subtracts amount of mana from ch's current and starts points event
void AlterMana(Character *ch, SInt32 amount) {
	struct RegenEvent *regen;
	SInt32	time;
	SInt32	gain;

	GET_MANA(ch) = MIN(GET_MANA(ch) - amount, GET_MAX_MANA(ch));
	
	if ((GET_MANA(ch) < GET_MAX_MANA(ch)) && !GET_POINTS_EVENT(ch, REGEN_MANA)) {
		if (GET_POS(ch) >= POS_STUNNED) {
			CREATE(regen, struct RegenEvent, 1);
			regen->ch = ch;
			regen->type = REGEN_MANA;
			gain = mana_gain(ch);
			time = PULSES_PER_MUD_HOUR / (gain ? gain : 1);
			GET_POINTS_EVENT(ch, REGEN_MANA) = new Event(regen_event, regen, time, NULL);
		}
	}
}
*/


void AlterMana(Character *ch, SInt32 amount) {
	SInt32		time;
	SInt32		gain;

	GET_MANA(ch) = MIN(GET_MANA(ch) - amount, (int)GET_MAX_MANA(ch));
	
	if ((GET_MANA(ch) < GET_MAX_MANA(ch)) && !GET_POINTS_EVENT(ch, REGEN_MANA)) {
		if (GET_POS(ch) >= POS_STUNNED) {
			gain = mana_gain(ch);
			time = /* PULSES_PER_MUD_HOUR / */ (gain ? gain : 1);
			new RegenEvent(time, ch, REGEN_MANA);
		}
	}
}


void AlterMove(Character *ch, SInt32 amount) {
	SInt32		time;
	SInt32		gain;

	if (GET_MATERIAL(ch) == Materials::Ether || 
		GET_MATERIAL(ch) == Materials::Shadow) {
		GET_MOVE(ch) = GET_MAX_MOVE(ch);
		return;
	}

	GET_MOVE(ch) = MIN(MAX(GET_MOVE(ch) - amount, -100), (int)GET_MAX_MOVE(ch));
	
    if ((GET_MOVE(ch) < 0) && (GET_POS(ch) > POS_SLEEPING)) {
    	ch->Send("You collapse from exhaustion!\r\n");
		act("$n collapses from exhaustion!", FALSE, ch, NULL, NULL, TO_ROOM);
		ch->StopFighting();
		GET_POS(ch) = POS_SLEEPING;
		// Remove actions
		if (GET_ACTION(ch))	{
			GET_ACTION(ch)->Cancel();
		}
    }

	if ((GET_MOVE(ch) < GET_MAX_MOVE(ch)) && !GET_POINTS_EVENT(ch, REGEN_MOVE)) {
		if (GET_POS(ch) >= POS_STUNNED) {
			gain = move_gain(ch);
			time = /* PULSES_PER_MUD_HOUR / */ (gain ? gain : 1);
			new RegenEvent(time, ch, REGEN_MOVE);
		}
	}
}


int hit_gain(Character * ch) {	//	Hitpoint gain pr. game hour
	float gain;

//	if (IS_NPC(ch)) {
//		gain = 3;
//	} else {
		//	5 - 500 pulses
		gain = 1200 / (GET_STAT(ch, Con) ? GET_STAT(ch, Con) : 1); //	3 * graf(age(ch).year, 8, 12, 20, 32, 16, 10, 4);
		
		//	Race/Level calculations
//		if (IS_ALIEN(ch))	gain /= 1.5;

		//	Skill/Spell calculations

		//	Position calculations
		switch (GET_POS(ch)) {
			case POS_SLEEPING:	gain /= 3;			break;		//	Triple
			case POS_RESTING:	gain /= 2;			break;		//	Double
			case POS_SITTING:	gain /= 1.5;		break;		//	One and a half
			default:								break;
		}
//	}

	if (AFF_FLAGGED(ch, AffectBit::Poison))		gain *= 2;	//	Half

	if (ADVANTAGED(ch, Advantages::FastHpRegen))	gain /= 2;

	return (int)gain;
}


int mana_gain(Character * ch) {
	float gain;

//	if (IS_NPC(ch)) {
//		gain = 5;
//	} else {
		//	10 - 1000 pulses
		gain = 1000 / (GET_STAT(ch, Int) ? GET_STAT(ch, Int) : 1); //	graf(age(ch).year, 16, 20, 24, 20, 16, 12, 10);
		
		//	Class/Level calculations
		// if (IS_SYNTHETIC(ch))	gain = 5;

		//	Skill/Spell calculations

		//	Position calculations
		switch (GET_POS(ch)) {
			case POS_SLEEPING:	gain /= 3;			break;		//	Triple
			case POS_RESTING:	gain /= 2;			break;		//	Double
			case POS_SITTING:	gain /= 1.5;		break;		//	One and a half
			default:								break;
		}

		if (ADVANTAGED(ch, Advantages::FastManaRegen))
			gain /= 2;

	return (int)gain;
}


int move_gain(Character * ch) {
	float gain;

//	if (IS_NPC(ch)) {
//		gain = 5;
//	} else {
		//	10 - 1000 pulses
		gain = 300 / (GET_STAT(ch, Con) ? GET_STAT(ch, Con) : 1); //	graf(age(ch).year, 16, 20, 24, 20, 16, 12, 10);
		
		//	Class/Level calculations
		// if (IS_SYNTHETIC(ch))	gain = 5;

		//	Skill/Spell calculations

		//	Position calculations
		switch (GET_POS(ch)) {
			case POS_SLEEPING:	gain /= 3;			break;		//	Triple
			case POS_RESTING:	gain /= 2;			break;		//	Double
			case POS_SITTING:	gain /= 1.5;		break;		//	One and a half
			default:								break;
		}

		if (GET_MAX_HIT(ch) >= (GET_HIT(ch) * 2))
	        	gain *= 2;

		if (AFF_FLAGGED(ch, AffectBit::Poison))
			gain *= 2;


		if (ADVANTAGED(ch, Advantages::FastMoveRegen))	gain /= 2;
		// if (!IS_SYNTHETIC(ch)) { if (AFF_FLAGGED(ch,
		//		AffectBit::Poison))
		//			gain *= 2;
//			if ((GET_COND(ch, FULL) == 0) || (GET_COND(ch, THIRST) == 0))
//				gain *= 2;
		// }
//	}
	return (int)gain;
}


void CheckRegenRates(Character *ch) {
	SInt32		gain, type;
    SInt32		time;
	
	if (GET_POS(ch) <= POS_INCAP)
		return;
	
	for (type = REGEN_HIT; type <= REGEN_MOVE; ++type) {
		switch (type) {
			case REGEN_HIT:
				if (GET_HIT(ch) >= GET_MAX_HIT(ch))		continue;
				else 									gain = hit_gain(ch);
				break;
			case REGEN_MANA:
				if (GET_MANA(ch) >= GET_MAX_MANA(ch))	continue;
				else									gain = mana_gain(ch);
				break;
			case REGEN_MOVE:
				if (GET_MOVE(ch) >= GET_MAX_MOVE(ch))	continue;
				else									gain = move_gain(ch);
				break;
			default:
				continue;
		}
		time = /* PULSES_PER_MUD_HOUR / */ (gain ? gain : 1);
	
		if (!GET_POINTS_EVENT(ch, type) || (time < GET_POINTS_EVENT(ch, type)->Time())) {
			if (GET_POINTS_EVENT(ch, type))
				GET_POINTS_EVENT(ch, type)->Cancel();
			
			new RegenEvent(time, ch, type);
		}
	}
}


#define READ_TITLE(ch, lev) (GET_SEX(ch) == SEX_MALE ?   \
	titles[(int)GET_RACE(ch)][lev].title_m :  \
	titles[(int)GET_RACE(ch)][lev].title_f)


void gain_condition(Character * ch, int condition, int value) 
{
	bool intoxicated;

	if (GET_COND(ch, condition) == -1)	/* No change */
		return;

	intoxicated = (GET_COND(ch, DRUNK) > 0);

	GET_COND(ch, condition) += value;

	if ((value > 0) && (condition != DRUNK) &&
			(GET_COND(ch, condition) == value))
		CheckRegenRates(ch);

	GET_COND(ch, condition) = RANGE(0, GET_COND(ch, condition), 24);

	if (GET_COND(ch, condition) || PLR_FLAGGED(ch, PLR_WRITING))
		return;

	switch (condition) {
	case FULL:
		ch->Send("You are hungry.\r\n");
		if (GET_HIT(ch) > 1)	AlterHit(ch, 1);
		return;
	case THIRST:
		ch->Send("You are thirsty.\r\n");
		if (GET_HIT(ch) > 1)	AlterHit(ch, 1);
		return;
	case DRUNK:
		if (intoxicated)
			ch->Send("You are now sober.\r\n");
		return;
	default:
		break;
	}
}


void check_idling(Character * ch) {
	Character *tch;
	if (++(ch->player->timer) > 8)
		if (GET_WAS_IN(ch) == NOWHERE && IN_ROOM(ch) != NOWHERE) {
			GET_WAS_IN(ch) = IN_ROOM(ch);
			if (FIGHTING(ch)) {
				ch->StopFighting();
				ch->StopFoesFighting();
			}
			act("$n disappears into the void.", TRUE, ch, 0, 0, TO_ROOM);
			ch->Send("You have been idle, and are pulled into a void.\r\n");
			ch->Save(ch->AbsoluteRoom());
			Crash_crashsave(ch);
			ch->FromRoom();
			ch->ToRoom(3);
		} else if (ch->player->timer > 48) {
			if (IN_ROOM(ch) != NOWHERE)
				ch->FromRoom();
			else
				mudlogf(BRF, ch, TRUE, "%s in NOWHERE when idle-extracted.", ch->RealName());
			ch->ToRoom(3);
			if (ch->desc) {
				STATE(ch->desc) = CON_DISCONNECT;
				ch->desc->character = NULL;
				ch->desc = NULL;
			}
			Crash_rentsave(ch);
			mudlogf(CMP, ch, TRUE, "%s idle-extracted.", ch->RealName());
			ch->Extract();
		}
}


void hour_update(void) {
	Character *i;
	Object *j, *jj;

	/* characters */
	LListIterator<Character *>	characters(Characters);
	while ((i = characters.Next())) {
		// if (GET_RACE(i) != RACE_SYNTHETIC) {
		// 	gain_condition(i, FULL, -1);
		// 	gain_condition(i, DRUNK, -1);
		// 	gain_condition(i, THIRST, -1);
		// }
		if (!IS_NPC(i)) {
			if (!IS_STAFF(i)) {
				check_idling(i);
				gain_condition(i, FULL, -1);
				gain_condition(i, DRUNK, -1);
				gain_condition(i, THIRST, -1);
			}
			// i->UpdateObjects();
		}
	}
	
	/* objects */
	LListIterator<Object *>	objects(Objects);
	while ((j = objects.Next())) {
		if ((IN_ROOM(j) == NOWHERE) && !j->Inside()) {
			j->Extract();
			continue;
		}
		/* If this is a temporary object */
		if (GET_OBJ_TIMER(j) > 0) {
			GET_OBJ_TIMER(j)--;

			if (!GET_OBJ_TIMER(j)) {
				if (SCRIPT_CHECK(j, OTRIG_TIMER))
					timer_otrigger(j);
				if (PURGED(j))
					continue;
				if (j->CarriedBy())
					act("$p decays in your hands.", FALSE, j->CarriedBy(), j, 0, TO_CHAR);
				else if (j->WornBy())
					act("$p decays.", FALSE, j->WornBy(), j, 0, TO_CHAR);
				else if (IN_ROOM(j) != NOWHERE)
					act("$p falls apart.", FALSE, 0, j, 0, TO_ROOM);
				if (GET_OBJ_TYPE(j) == ITEM_CONTAINER) {
					LListIterator<Object *>	contents(j->contents);
					while ((jj = contents.Next())) {
						jj->FromObj();

						if (j->InObj())					jj->ToObj(j->InObj());
						else if (j->CarriedBy())		jj->ToChar(j->CarriedBy());
						else if (IN_ROOM(j) != NOWHERE)	jj->ToRoom(IN_ROOM(j));
						else if (j->WornBy())			jj->ToChar(j->WornBy());
						else							core_dump();
					}
				}
				j->Extract();
			}
		}
	}
}


/* Update PCs, NPCs, and objects */
void point_update(void) {
	Character *i;
	LListIterator<Character *>	iter(Characters);
	Attack attack;

	attack.damtype = Damage::Suffering;
	
	/* characters */
	while ((i = iter.Next())) {
//		if (AFF_FLAGGED(i, AffectBit::Poison))
//			if (i->Damage(NULL, GET_HIT(ch) / 100, Damage::Suffering) < 0)
//				continue;
		if (GET_POS(i) >= POS_STUNNED) {
			if (GET_POS(i) <= POS_STUNNED)
				i->UpdatePos();
		} else if (GET_POS(i) == POS_INCAP) {
			attack.dam = 1;
			if (i->Damage(NULL, &attack) < 0)
				continue;
		} else if (GET_POS(i) == POS_MORTALLYW) {
			attack.dam = 2;
			if (i->Damage(NULL, &attack) < 0)
				continue;
		}
		
		if (AFF_FLAGGED(i, AffectBit::Fly)) {
			AlterMove(i, 1);
		}

		// Laori Nature spell recharge
		if ((ADVANTAGED(i, Advantages::NatureRecharge)) && 
				(!FIGHTING(i)) && (GET_POS(i) < POS_STANDING)) {
			switch (SECT(IN_ROOM(i))) {
			case SECT_CITY:
			case SECT_ROAD:
			case SECT_UNDERGROUND:
			case SECT_VACUUM:
				break;
			default:
				if (CHAR_SKILL(i).GetSkillPts(CSPHERE_NATURE, NULL) < MIN(GET_LEVEL(i), 250))
					CHAR_SKILL(i).LearnSkill(CSPHERE_NATURE, NULL, 1);
			}
		}
	}
}
