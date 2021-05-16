/*************************************************************************
*   File: characters.c++             Part of Aliens vs Predator: The MUD *
*  Usage: Primary code for characters                                    *
*************************************************************************/


#include "structs.h"
#include "screen.h"
#include "combat.h"
// #include "utils.h"
#include "scripts.h"
#include "descriptors.h"
#include "rooms.h"
#include "characters.h"
#include "objects.h"
#include "comm.h"
#include "buffer.h"
#include "files.h"
#include "interpreter.h"
#include "affects.h"
#include "db.h"
#include "clans.h"
#include "opinion.h"
#include "alias.h"
#include "event.h"
#include "handler.h"
#include "find.h"
#include "strings.h"
#include "skills.h"
#include "track.h"
#include "constants.h"
#include "eventdefines.h"
#include "queue.h"

// External functions
int death_mtrigger(Character *ch, Character *actor);
void clearMemory(Character *ch);
void strip_string(char *buffer);
int find_name(const char *name);
void tag_argument(char *argument, char *tag);
void check_autowiz(Character * ch);
void CheckRegenRates(Character *ch);
extern void load_otrigger(Object *obj);

void MakeSpirit(Character *ch, int clss);

extern int olc_edit_bitvector(Descriptor *d, Flags *result, char *argument,
		       Flags old_bv, char *name, const char **values, Flags mask);

extern int newb_eq;
extern int max_exp_gain;
extern int max_exp_loss;
extern int strip_last_char(char *line, char letter);

extern Queue *EventQueue;

LList<Character *>	Characters;			//	global linked list of chars
LList<Character *>	PurgedChars;		//	list of chars to be deleted
Index<Character>	Character::Index;			//	index table for mobile file
Vector<PlayerIndex>	player_table;	/* index to plr file	 */
SInt32 top_of_p_table = 0;		/* ref to top of table		 */

PlayerData	dummy_player;		//	dummy spec area for mobs
MobData		dummy_mob;			//	dummy spec area for players


PlayerData::PlayerData(void) : afk(NULL), prompt(NULL), host(NULL), email(NULL),
		poofin(NULL), poofout(NULL), last_tell(0), idnum(0), timer(0) {
	memset(&special, 0, sizeof(struct PlayerSpecialData));
	memset(&time, 0, sizeof(time_data));
	passwd[0] = '\0';
	special.load_room = NOWHERE;
}


PlayerData::~PlayerData(void) {
	Alias *a;
	while ((a = aliases.Top())) {
		aliases.Remove(a);
		delete a;
	}
	if (poofin)		free(poofin);
	if (poofout)	free(poofout);
	if (host)		free(host);
	if (prompt)		free(prompt);
	if (afk)		free(afk);
	if (email)		free(email);
}


MobData::MobData(void) : attack_type(0), wait_state(0), hunting(0), seeking(0),
		roomseeking(0), disposition(0),  default_pos(POS_STANDING),
		last_direction(0), tick(0), hates(NULL), fears(NULL), type(0),
		numhitdice(0), sizehitdice(0), modhitdice(1) {
	memset(&damage, 0, sizeof(Dice));
}


MobData::MobData(const MobData &mob) : attack_type(mob.attack_type),
		wait_state(0),  damage(mob.damage), hunting(0), seeking(0),
		roomseeking(0), disposition(mob.disposition),
		default_pos(mob.default_pos), last_direction(0), tick(mob.tick),
		hates(mob.hates ? new Opinion(*mob.hates) : NULL),
		fears(mob.fears ? new Opinion(*mob.fears) : NULL),
		type(mob.type), numhitdice(mob.numhitdice), 
		sizehitdice(mob.sizehitdice), modhitdice(mob.modhitdice) {
}


MobData::~MobData(void) {
	if (hates)	delete hates;
	if (fears)	delete fears;
}


Character::Character(void) : MUDObject(-1), pfilepos(-1), /*nr(-1),*/
		was_in_room(NOWHERE), player(&dummy_player), mob(&dummy_mob),
		desc(NULL), master(NULL), action(NULL), sitting_on(NULL) {
	weight = 200;
	material = Materials::Flesh;
	for (SInt32 eq = 0; eq < NUM_WEARS; eq++)	equipment[eq] = NULL;
	for (SInt32 i = 0; i < 3; i++)				points_event[i] = NULL;
}


//	Copy constructor
Character::Character(const Character &ch) : MUDObject(ch), pfilepos(-1),
		/* nr(ch.nr), */ was_in_room(NOWHERE), general(ch.general),
		real_abils(ch.real_abils), aff_abils(ch.aff_abils), points(ch.points),
		player(&dummy_player), mob(&dummy_mob), desc(NULL),
		master(NULL), action(NULL), sitting_on(NULL) {
	Affect *aff, *newAff;
	Object *item;
	SkillKnown *skill;

	weight = 200;

	for (SInt32 i = 0; i < 3; i++)
		points_event[i] = NULL;

	mob = (!ch.mob || (ch.mob == &dummy_mob)) ? new MobData() : new MobData(*ch.mob);

	if (!IS_SET(ch.general.act, MOB_ISNPC)) {
		char *buf = get_buffer(MAX_STRING_LENGTH);

		if (IS_STAFF((&ch)))	sprintf(buf, "%s %s", ch.Name(), SSData(ch.general.title));
		else				sprintf(buf, "%s %s", SSData(ch.general.title), ch.Name());
		general.longDesc = SString::Create(buf);
		// general.shortDesc = general.longDesc->Share();

		release_buffer(buf);
	} else {
		// general.name = ch.general.name->Share();
		// general.shortDesc = ch.general.shortDesc->Share();
		general.longDesc = ch.general.longDesc->Share();
		// general.description = ch.general.description->Share();
	}

	MOB_FLAGS(this) = IS_SET(ch.general.act, MOB_ISNPC) ? ch.general.act : MOB_ISNPC;	//	Gaurantee - no duping players, lest to make a mob

    // Copy over the skills
    CHAR_SKILL(&ch).GetTable(&skill);

    while (skill) {
      GET_TOTALPTS(this) += CHAR_SKILL(this).SetSkill(skill->skillnum, NULL, skill->proficient);
      skill = skill->next;
    }

	START_ITER(affect_iter, aff, Affect *, ch.affected) {
		newAff = new Affect(*aff, this);
		affected.Append(newAff);
	}

	for (SInt32 eq = 0; eq < NUM_WEARS; eq++)
		equipment[eq] = ch.equipment[eq] ? new Object(*ch.equipment[eq]) : NULL;

	START_ITER(item_iter, item, Object *, ch.contents) {
		contents.Append(new Object(*item));
	}

	AffectTotal();
}


Character::~Character(void) {
	Concentration *tmp_con;
	bool itsbroken = true;	// Needed for passing by reference

	if (player && (player != &dummy_player)) {
		delete player;
		if (IS_NPC(this))
			log("SYSERR: Mob %s (#%d) had PlayerData allocated!", RealName(), Virtual());
	}
	if (mob && (mob != &dummy_mob))	{
		delete mob;
		if (!IS_NPC(this))
			log("SYSERR: Player %s had MobData allocated!", RealName());
	}

	SSFree(general.title);
	SSFree(general.longDesc);

// 	if (path) {
// 		delete path;
// 		path = NULL;
// 	}

	START_ITER(iter, tmp_con, Concentration *, concentrations) {
		tmp_con->Update(this, itsbroken);
		concentrations.Remove(tmp_con);
		delete tmp_con;
	}
}


// CHANGEPOINT:  This is probably unnecessary.  I'd rather have something that
// automatically went through switches and such. -DH
inline const char * Character::RealName(void) const {
//	return (IS_NPC(this) ? SDesc() : Name());
	return Name();
}


/* Extract a ch completely from the world, and leave his stuff behind */
void Character::Extract(void) {
	Character *k;
	Descriptor *t_desc;
	Concentration *tmp_con;
	Affect *tmp_affect;
	Object *obj;
	int i;
	ACMD(do_return);
	VNum saveroom = NOWHERE;

	if (Purged())
		return;

	if (!IS_NPC(this) && !desc) {
		START_ITER(iter, t_desc, Descriptor *, descriptor_list) {
			if (t_desc->original == this)
				do_return(t_desc->character, "", 0, "return", 0);
		}
	}
	if (InRoom() == NOWHERE) {
		log("SYSERR: NOWHERE extracting char %s in extract_char.", RealName());
		core_dump();
		exit(1);
	} else {
		saveroom = AbsoluteRoom();
		if (!Room::Find(saveroom))	saveroom = NOWHERE;
	}

	if (followers.Count() || master)
		DieFollower();

	if (RIDING(this))
		RemoveRider(this, MNT_EXTRACT);
	if (RIDER(this)) {
		START_ITER(iter, k, Character *, riders)
			RemoveRider(k, MNT_EXTRACT);
	}

#if 0
	if (GRAPPLING(this)) {
		GRAPPLER(GRAPPLING(this)) = NULL;
		GRAPPLING(this) = NULL;
	}

	if (GRAPPLER(this)) {
		GRAPPLING(GRAPPLER(this)) = NULL;
		GRAPPLER(this) = NULL;
	}

	if (OBJ_DRAGGED(ch)) {
		OBJ_DRAGGED_BY(OBJ_DRAGGED(ch)) = NULL;
		OBJ_DRAGGED(ch) = NULL;
	}
#endif

	/* Forget snooping, if applicable */
	if (desc) {
		if (desc->snooping) {
			desc->snooping->snoop_by = NULL;
			desc->snooping = NULL;
		}
		if (desc->snoop_by) {
			desc->snoop_by->Write("Your victim is no longer among us.\r\n");
			desc->snoop_by->snooping = NULL;
			desc->snoop_by = NULL;
		}
	}

	/* transfer objects to room, if any */
	START_ITER(object_iter, obj, Object *, contents) {
		obj->FromChar();
		if (!OBJ_FLAGGED(obj, ITEM_PROXIMITY)) 	obj->ToRoom(InRoom());
		else									obj->Extract();
	}

	/* transfer equipment to room, if any */
	for (i = 0; i < NUM_WEARS; i++) {
		if (GET_EQ(this, i)) {
			Object *	obj = unequip_char(this, i);
			if (OBJ_FLAGGED(obj, ITEM_PROXIMITY))		obj->Extract();
			else
			obj->ToRoom(InRoom());
		}
	}

	if (FIGHTING(this))
		StopFighting();

	START_ITER(combat_iter, k, Character *, CombatList) {
		if (FIGHTING(k) == this)
			k->StopFighting();
	}

	START_ITER(iter, tmp_affect, Affect *, affected) {
		delete tmp_affect;
		affected.Remove(tmp_affect);
	}

	// Get rid of any concentration spells.
	START_ITER(conc_iter, tmp_con, Concentration *, concentrations) {
		delete tmp_con;
		concentrations.Remove(tmp_con);
	}

// 	if (path) {
// 		delete path;
// 		path = NULL;
// 	}

	//	pull the char from the lists
	FromRoom();
	FromWorld();

	//	remove any pending events for this character.
	if (action)	{
		action->Cancel();
		action = NULL;
	}
	EventCleanup();

	if (desc && desc->original)
		do_return(this, "", 0, "return", 0);

	if (!IS_NPC(this))			Save(saveroom);
	else {
		if (Virtual() > -1)		Index[Virtual()].number--;		//	if mobile
		clearMemory(this);		/* Only NPC's can have memory */
		Purged(true);
		PurgedChars.Add(this);
	}

	if (script) {
		script->Extract();
		script = NULL;
	}

	if (desc) {
		if (STATE(desc) != CON_CLOSE) {
			STATE(desc) = CON_MENU;
			desc->Write(desc->termtype == TERM_PLAIN ? PLAINMENU : MENU);
		}
	} else {  /* if a player gets purged from within the game */
		if (!Purged()) {	// We need this check to prevent people from getting added twice to the Purged list
			Purged(true);
			PurgedChars.Add(this);
		}
	}
}


/* place a character in a room */
void Character::ToRoom(VNum room) {
	int i;
	Object *tmp_obj = NULL;
	Character 	*tch;

	if (room < 0 || room > world.Top())
		log("SYSERR: %s->ToRoom(room = %d)", RealName(), room);
	else if (IN_ROOM(this) != NOWHERE)
		log("SYSERR: %s->ToRoom(room = %d): InRoom() == %d", RealName(), room, IN_ROOM(this));
	else if (PURGED(this))
		log("SYSERR: %s->ToRoom(room = %d): Purged() == true", RealName(), room);
	else {
		if (world[room].people.Contains(this)) {
			log("SYSERR: %s->ToRoom(room = %d): world[room].people.Contains(this) == true, InRoom() == %d",
					RealName(), room, IN_ROOM(this));
//			return;
		} else
			world[room].people.Add(this);
		SetRoom(room);

		for (i = 0; i < NUM_WEARS; ++i) {
			if (GET_EQ(this, i) != NULL)
				if (IS_SET(GET_OBJ_EXTRA(GET_EQ(this, i)), ITEM_GLOW)) /* Light is ON */
					world[IN_ROOM(this)].ChangeLight(1);
		}

		START_ITER(iter, tmp_obj, Object *, contents) {
			if (IS_SET(GET_OBJ_EXTRA(tmp_obj), ITEM_GLOW)) /* Light is ON */
				world[IN_ROOM(this)].ChangeLight(1);
		}

		if (AFF_FLAGGED(this, AffectBit::Light))
			world[IN_ROOM(this)].ChangeLight(1);

		if (FIGHTING(this) && (IN_ROOM(this) != IN_ROOM(FIGHTING(this)))) {
			FIGHTING(this)->StopFighting();
			StopFighting();
		}

		if (RIDING(this) && (IN_ROOM(RIDING(this)) != room))
			RIDING(this)->RemoveRider(this, MNT_TOROOM);

		if (NO_STAFF_HASSLE(this))	// Because the rest of the function is only to kill off morts.
			return;

		if (ROOM_FLAGGED(room, ROOM_DEATH)) {
			GET_HIT(this) = -100;
			UpdatePos();
			return;
		}

		if (ROOM_FLAGGED(room, ROOM_GRAVITY) && !AFF_FLAGGED(this, AffectBit::Fly) &&
			!NO_STAFF_HASSLE(this) && !Event::Find(events, typeid(FallingEvent))) {
//			add_event(3, gravity, this, NULL, NULL);
			new FallingEvent(3 RL_SEC, this);
		}

		if (ROOM_FLAGGED(room, ROOM_VACUUM) && !NO_STAFF_HASSLE(this) && !AFF_FLAGGED(this, AffectBit::VacuumSafe)) {
			act("*Gasp* You can't breath!  It's a vacuum!", TRUE, this, 0, 0, TO_CHAR);
			GET_HIT(this) = -100;
			UpdatePos();
			return;
		}
	}
}


/* move a player out of a room */
void Character::FromRoom() {
	int i;
	Object *tmp_obj = NULL;
	Character 	*tch;

	if (PURGED(this)) {
		log("SYSERR: %s->Purged() == TRUE in Character::FromRoom()", RealName());
		core_dump();
		return;
	}

	if (IN_ROOM(this) == NOWHERE) {
		log("SYSERR: %s->InRoom() == NOWHERE in Character::FromRoom()", RealName());
		return;
	}

	CHAR_WATCHING(this) = 0;

	if (FIGHTING(this)) {
		if (IS_NPC(this) || desc) {
			if ((FIGHTING(this)->GetRelation(this) != RELATION_FRIEND)) {
				FIGHTING(this)->GainExp(Combat::CalcReward(FIGHTING(this), this));
			}
		}
		StopFighting();
	}

	// Daniel Rollings AKA Daniel Houghton's addition:  Unless I find this to be redundant, this
	// code assures that nobody in the room continues to fight this character.
	START_ITER(chiter, tch, Character *, world[IN_ROOM(this)].people) {
		if (FIGHTING(tch) == this)		tch->StopFighting();
	}

	for (i = 0; i < NUM_WEARS; ++i) {
		if (GET_EQ(this, i) != NULL)
			if (IS_SET(GET_OBJ_EXTRA(GET_EQ(this, i)), ITEM_GLOW)) /* Light is ON */
				world[IN_ROOM(this)].ChangeLight(-1);
	}

	START_ITER(iter, tmp_obj, Object *, contents) {
		if (IS_SET(GET_OBJ_EXTRA(tmp_obj), ITEM_GLOW)) /* Light is ON */
			world[IN_ROOM(this)].ChangeLight(-1);
	}

	if (AFF_FLAGGED(this, AffectBit::Light))
		world[IN_ROOM(this)].ChangeLight(-1);

	world[IN_ROOM(this)].people.Remove(this);
	SetRoom(NOWHERE);
}


void Character::Die(Character *killer) {
	Affect 		*aff;

	if (!IS_NPC(this)) {
		char *buf = get_buffer(MAX_INPUT_LENGTH);
		if (!killer || this == killer)
			sprintf(buf, "&cY[&cBINFO&cY]&c0 %s killed at %s&c0.\r\n", Name(), world[IN_ROOM(this)].Name());
		else			sprintf(buf, "&cY[&cBINFO&cY]&c0 %s killed by %s&c0 at %s&c0.\r\n", Name(), killer->Name(), world[IN_ROOM(this)].Name());
		Combat::Announce(buf);
	  	release_buffer(buf);
	}

  	if (killer) {
  		WAIT_STATE(killer, 0);
		if (FIGHTING(killer) == this) {
			killer->StopFighting();
		}
	}

	if (FIGHTING(this)) {
		if (FIGHTING(this) == this) {
			FIGHTING(this)->StopFighting();
		}
		StopFighting();
	}

	START_ITER(iter, aff, Affect *, affected) {
		aff->Remove(this);
	}

		if (death_mtrigger(this, killer)) {
			Combat::DeathCry(this);
		}

	switch (GET_MATERIAL(this)) {
	case Materials::Undefined:
	case Materials::Flesh:
	case Materials::Veggy:
	case Materials::DragonHide:	// This is because a character's material is really the outer covering
	case Materials::Jelly:
		Combat::MakeCorpse(this);
		break;
	default:
		break;
	}

#ifdef GOT_RID_OF_IT
	if (killer && (this != killer)) {
		if (IS_NPC(this))	killer->player->special.MKills++;
		else				killer->player->special.PKills++;

		if (IS_NPC(killer))	player->special.MDeaths++;
		else				player->special.PDeaths++;
	}
#endif

	if (IS_NPC(this))
		Extract();
	else {
		REMOVE_BIT(PLR_FLAGS(this), PLR_KILLER | PLR_THIEF);

		if (GET_MORTALITY(this) == MORTALITY_MORTAL) {
					
			switch (GET_MATERIAL(this)) {
			case Materials::Ether:
			case Materials::Shadow:
				SET_BIT(PLR_FLAGS(this), PLR_DELETED);
				if (desc)
					STATE(desc) = CON_CLOSE;
				Extract();
				return;
			default:
				break;
			}

			MakeSpirit(this, CLASS_GHOST);

			if (desc) {
				desc->inbuf[0] = '\0';
			desc->Write("It's all going hazy... you feel your flesh give you up, and you feel\r\n"
						"light, airy... insubstantial.\r\n");
			}

			GET_LOADROOM(this) = AbsoluteRoom();
			Extract();
		} else {
			EventCleanup();

			FromRoom();
			ToRoom(desc ? StartRoom() : 1);

			GET_HIT(this) = 1;
			GET_MANA(this) = 1;
			GET_MOVE(this) = 1;
			UpdatePos();
			CheckRegenRates(this);

			if (desc)	desc->inbuf[0] = '\0';
			act("$n has returned from the dead.", TRUE, this, 0, 0, TO_ROOM);
			GET_POS(this) = POS_SITTING;
			look_at_room(this, 0);
		}
	}
}


inline void Character::UpdateObjects(void) {
#ifdef GOT_RID_OF_IT
	int i, pos;

	for (pos = WEAR_HAND_R; pos <= WEAR_HAND_L; pos++) {
		if (GET_EQ(this, pos)) {
			if (GET_OBJ_TYPE(GET_EQ(this, pos)) == ITEM_LIGHT) {
				if (GET_OBJ_VAL(GET_EQ(this, pos), 2) > 0) {
					i = --GET_OBJ_VAL(GET_EQ(this, pos), 2);
					if (i == 1) {
						act("Your light begins to flicker and fade.", FALSE, this, 0, 0, TO_CHAR);
						act("$n's light begins to flicker and fade.", FALSE, this, 0, 0, TO_ROOM);
					} else if (i == 0) {
						act("Your light sputters out and dies.", FALSE, this, 0, 0, TO_CHAR);
						act("$n's light sputters out and dies.", FALSE, this, 0, 0, TO_ROOM);
						world[IN_ROOM(this)].light--;
					}
				}
			}
		}
	}
//	for (i = 0; i < NUM_WEARS; i++)
//		if (GET_EQ(this, i))
//			GET_EQ(this, i)->update(2);
//
//	if (carrying)
//		carrying->update(1);
#endif
}


//	This updates a character by subtracting everything he is affected by
//	restoring original abilities, and then affecting all again
void Character::AffectTotal(void) {
	Affect *af;
	int i, j;
	LListIterator<Affect *>		iter(affected);

	//	Unaffect everything
	for (i = 0; i < NUM_WEARS; i++) {
		if (equipment[i])
			for (j = 0; j < MAX_OBJ_AFFECT; j++)
				AffectModify(equipment[i]->affectmod[j].location,
						equipment[i]->affectmod[j].modifier, equipment[i]->affectbits, false);
	}
	while ((af = iter.Next()))
		af->Modify(this, false);

	//	Restore original
	aff_abils = real_abils;

	//	Re-apply effects
	for (i = 0; i < NUM_WEARS; i++) {
		if (GET_EQ(this, i))
			for (j = 0; j < MAX_OBJ_AFFECT; j++)
				AffectModify(GET_EQ(this, i)->affectmod[j].location,
						GET_EQ(this, i)->affectmod[j].modifier, GET_EQ(this, i)->affectbits, true);
	}

	iter.Reset();
	while ((af = iter.Next()))
		af->Modify(this, true);

	/* Make certain values are between 0..200, not < 0 and not > 200! */
	i = 99;

	aff_abils.Str = RANGE(0, (int)aff_abils.Str, i);
	aff_abils.Int = RANGE(0, (int)aff_abils.Int, i);
	aff_abils.Wis = RANGE(0, (int)aff_abils.Wis, i);
	aff_abils.Dex = RANGE(0, (int)aff_abils.Dex, i);
	aff_abils.Con = RANGE(0, (int)aff_abils.Con, i);
	aff_abils.Cha = RANGE(0, (int)aff_abils.Cha, i);

	passive_defense = RANGE(0, (int)passive_defense, i);
	damage_resistance = RANGE(0, (int)damage_resistance, i);

	CheckRegenRates(this);
}


/* enter a character in the world (place on lists) */
void Character::ToWorld(void) {
	Characters.Add(this);
}


/* remove a character from the world lists */
void Character::FromWorld(void) {
	Characters.Remove(this);
}


SInt32 Character::Send(const char *messg) {
	if (desc)		return desc->Write(messg);
	else			act(messg, FALSE, this, 0, 0, TO_CHAR);

	return 0;
}


/*
 * If missing vsnprintf(), remove the 'n' and remove MAX_STRING_LENGTH.
 */
SInt32 Character::Sendf(const char *messg, ...) {
	SInt32	length = 0;
	char *	send_buf = get_buffer(MAX_STRING_LENGTH);
	va_list args;

	if (!messg || !*messg)
		return 0;

	va_start(args, messg);
	vsprintf(send_buf, messg, args);
	va_end(args);

	if (desc)		length = desc->Write(send_buf);
	else			act(send_buf, FALSE, this, 0, 0, TO_CHAR);

	release_buffer(send_buf);

	return length;
}


SInt32 Character::MergeSend(const char *messg) {
	if (desc)		return desc->MergeWrite(messg);
	else			act(messg, FALSE, this, 0, 0, TO_CHAR);

	return 0;
}


/*
 * If missing vsnprintf(), remove the 'n' and remove MAX_STRING_LENGTH.
 */
SInt32 Character::MergeSendf(const char *messg, ...) {
	SInt32	length = 0;
	char *	send_buf = get_buffer(MAX_STRING_LENGTH);
	va_list args;

	if (!messg || !*messg)
		return 0;

	va_start(args, messg);
	vsprintf(send_buf, messg, args);
	va_end(args);

	if (desc)		length = desc->MergeWrite(send_buf);
	else			act(send_buf, FALSE, this, 0, 0, TO_CHAR);

	release_buffer(send_buf);

	return length;
}


/* new save_char that writes an ascii pfile */
void Character::Save(VNum load_room) {
	FILE *outfile;
	char *outname, *bits, *buf;
	const char *temp;
	int i;
	Affect *aff;
	Object *char_eq[NUM_WEARS];
	Alias *a;
	Trigger *trig;
	SInt32 hit, mana, move;
	SkillKnown *tskill = NULL;
	CallName *name = NULL;
	Flags	affectmask = 0;
	int new_affected_by;


	if (IS_NPC(this) || GET_PFILEPOS(this) < 0)
		return;

	/*
	 * save current values, because unequip_char will call
	 * affect_total(), which will reset points to lower values
	 * if player is wearing +points items.  We will restore old
	 * points level after reequiping.
	 */
	hit = GET_HIT(this);
	mana = GET_MANA(this);
	move = GET_MOVE(this);


	outname = get_buffer(MAX_INPUT_LENGTH);
	bits = get_buffer(MAX_INPUT_LENGTH);

	temp = Name();
	for (i = 0; (*(bits + i) = LOWER(*(temp + i))); i++);

	sprintf(outname, PLR_PREFIX "%c" SEPERATOR "%s", *bits, bits);

	if (!(outfile = fopen("pfile.out", "w"))) {
		perror(outname);
		release_buffer(outname);
		release_buffer(bits);
		return;
	}
	/* Unequip everything a character can be affected by */
	for (i = 0; i < NUM_WEARS; i++) {
		char_eq[i] = GET_EQ(this, i) ? unequip_char(this, i) : NULL;
	}

	if(RealName())			fprintf(outfile, "Name: %s\n", RealName());
	if(RealName())			fprintf(outfile, "Keyw: %s\n", Keywords());
	if(RealName())			fprintf(outfile, "SDsc: %s\n", SDesc());
	if(GET_PASSWD(this))	fprintf(outfile, "Pass: %s\n", GET_PASSWD(this));
	if(GET_TITLE(this))		fprintf(outfile, "Titl: %s\n", GET_TITLE(this));
	if(SSLDesc()) {
		buf = get_buffer(MAX_STRING_LENGTH);
		strcpy(buf, LDesc());
		strip_string(buf);
		fprintf(outfile, "Desc:\n%s~\n", buf);
		release_buffer(buf);
	}
	if (GET_PROMPT(this))	fprintf(outfile, "Prmp: %s\n", GET_PROMPT(this));
	fprintf(outfile, "Sex : %d\n", GET_SEX(this));
	fprintf(outfile, "Mort: %d\n", GET_MORTALITY(this));
	fprintf(outfile, "Race: %d\n", GET_RACE(this));
	fprintf(outfile, "Clss: %d\n", GET_CLASS(this));
	fprintf(outfile, "Levl: %d\n", GET_LEVEL(this));
	fprintf(outfile, "Trst: %d\n", GET_TRUST(this));
	fprintf(outfile, "Brth: %ld\n", player->time.birth);
	fprintf(outfile, "Plyd: %ld\n", player->time.played + (time(0) - player->time.logon));
	fprintf(outfile, "Last: %ld\n", player->time.logon);
	fprintf(outfile, "Host: %s\n", (desc) ? desc->host : player->host);
	if (player->email)	fprintf(outfile, "EMal: %s\n", player->email);
	fprintf(outfile, "Hite: %d\n", GET_HEIGHT(this));
	fprintf(outfile, "Wate: %d\n", GET_WEIGHT(this));
	fprintf(outfile, "Mtrl: %d\n", material);

	fprintf(outfile, "Id  : %d\n", player->idnum);
	if(general.act) {
		sprintbits(general.act, bits);
		fprintf(outfile, "Act : %s\n", bits);
//			fprintf(outfile, "Act : %d\n", general.act);
	}
	if (PLR_FLAGGED(this, PLR_LOADROOM))
		load_room = player->special.load_room;

	// Fix to prevent players from emerging in limbo and similar -DH
	if (load_room <= 3) {
		load_room = GET_WAS_IN(this);
	}

	general.skills.GetTable(&tskill);

	if (tskill) {
		fprintf(outfile, "Skil:\n");
		do {
			fprintf(outfile, "%d %d %d\n", tskill->skillnum, tskill->proficient, tskill->theory);
			tskill = tskill->next;
		} while (tskill);
		fprintf(outfile, "0 0 0\n");
	}

	if (SCRIPT(this)) {
		fprintf(outfile, "Trig:\n");
	 	LListIterator<Trigger *>	triggers;

		triggers.Restart(TRIGGERS(SCRIPT(this)));

		while ((trig = triggers.Next())) {
			fprintf(outfile, "%d\n", trig->Virtual());
		}
		fprintf(outfile, "0\n");
	}

	fprintf(outfile, "Cmbt: ");
	combat_profile.Save(outfile);
	fprintf(outfile, "\n");

	if (ENERGY(this).Pools.Count() && !IS_STAFF(this)) {
		fprintf(outfile, "Ener:\n");
        ENERGY(this).Save(this, outfile);
	}

	if (player->special.wimp_level)		fprintf(outfile, "Wimp: %d\n", player->special.wimp_level);

//	if ((load_room != NOWHERE) && Room::Find(load_room)) {
	if ((load_room != NOWHERE) && Room::Find(load_room)) {
		fprintf(outfile, "Room: %d\n", load_room);
	}

	if(player->special.preferences) {
		sprintbits(player->special.preferences, bits);
		fprintf(outfile, "Pref: %s\n", bits);
	}
	if(player->special.channels) {
		sprintbits(player->special.channels, bits);
		fprintf(outfile, "Chan: %s\n", bits);
	}

	if(player->special.bad_pws)			fprintf(outfile, "Badp: %d\n", player->special.bad_pws);

	if(!IS_STAFF(this)) {
		if(general.conditions[0])		fprintf(outfile, "Hung: %d\n", general.conditions[0]);
		if(general.conditions[1])		fprintf(outfile, "Thir: %d\n", general.conditions[1]);
		if(general.conditions[2])		fprintf(outfile, "Drnk: %d\n", general.conditions[2]);
	}
	if(points.pracs)			fprintf(outfile, "Prac: %d\n", points.pracs);
	if(Clan::Find(general.misc.clannum))fprintf(outfile, "Clan: %d %d\n", general.misc.clannum, general.misc.clanrank);

#ifdef GOT_RID_OF_IT
	fprintf(outfile, "Kill: %d %d %d %d\n",	player->special.PKills, player->special.MKills,
			player->special.PDeaths, player->special.MDeaths);
	fprintf(outfile, "Scor: %0.3f\n", player->special.killScore);
#endif

	fprintf(outfile, "Str : %d\n", real_abils.Str);
	fprintf(outfile, "Int : %d\n", real_abils.Int);
	fprintf(outfile, "Wis : %d\n", real_abils.Wis);
	fprintf(outfile, "Dex : %d\n", real_abils.Dex);
	fprintf(outfile, "Con : %d\n", real_abils.Con);
	fprintf(outfile, "Cha : %d\n", real_abils.Cha);

	fprintf(outfile, "Hit : %d/%d\n", points.hit, points.max_hit);
	fprintf(outfile, "Mana: %d/%d\n", points.mana, points.max_mana);
	fprintf(outfile, "Move: %d/%d\n", points.move, points.max_move);
	fprintf(outfile, "Exp : %d\n", points.exp);

	if (damage_resistance)		fprintf(outfile, "DR  : %d\n", damage_resistance);
	if (passive_defense)		fprintf(outfile, "PD  : %d\n", passive_defense);

	if (points.hitroll)		fprintf(outfile, "Hrol: %d\n", points.hitroll);
	if (points.damroll)		fprintf(outfile, "Drol: %d\n", points.damroll);

	fprintf(outfile, "Body: %d\n", general.bodytype);
	fprintf(outfile, "Totl: %d\n", general.totalpts);
	fprintf(outfile, "ExpM: %d\n", general.exp_modifier);
	fprintf(outfile, "Algn: %d\n", general.alignment);
	fprintf(outfile, "Lawf: %d\n", general.lawfulness);

	if (points.gold)					fprintf(outfile, "Gold: %d\n", points.gold);
	if (points.bank_gold)				fprintf(outfile, "Bank: %d\n", points.bank_gold);
	if (player->special.saycolor)		fprintf(outfile, "Sayc: %d\n", player->special.saycolor);
	if (player->special.page_length)	fprintf(outfile, "PLen: %d\n", player->special.page_length);
	if (player->special.staff_invis)	fprintf(outfile, "SInv: %d\n", player->special.staff_invis);
	if (player->special.freeze_level)	fprintf(outfile, "Frez: %d\n", player->special.freeze_level);

	if (general.advantages)	{
		sprintbits(general.advantages, bits);
		fprintf(outfile, "Adv : %s\n", bits);
	}
	if (general.disadvantages)	{
		sprintbits(general.disadvantages, bits);
		fprintf(outfile, "DAdv: %s\n", bits);
	}

	if (player->poofin)		fprintf(outfile, "Pfin: %s\n", player->poofin);
	if (player->poofout)	fprintf(outfile, "Pfou: %s\n", player->poofout);

	if (player->special.staff_flags) {
		sprintbits(player->special.staff_flags, bits);
		fprintf(outfile, "Stf : %s\n", bits);
	}

	// Save names of people and objects
	if (player->CalledNames.Count() > 0) {
		MapIterator<IDNum, CallName>	iter(player->CalledNames);
		while ((name = iter.Next())) {
			if (name->ID() >= 200000)		continue;
			fprintf(outfile, "Call: %d %s\n", name->ID(), name->Value());
		}
	}

	//	save aliases
	START_ITER(alias_iter, a, Alias *, player->aliases) {
		fprintf(outfile, "Alis: %s %s\n", a->command, a->replacement);
	}

	fprintf(outfile, "Affs:\n");
	START_ITER(affect_iter, aff, Affect *, affected) {
		if (aff->AffType() && (!aff->Caster())) {
			fprintf(outfile, "%d %d %d %d %d\n", aff->AffType(), aff->Time(),
					aff->AffModifier(), aff->AffLocation(), (int)aff->AffFlags());
			affectmask |= aff->AffFlags();
		}
	}
	fprintf(outfile, "0 0 0 0 0\n");

	affectmask |= AffectBit::Group;
	new_affected_by = affectbits;
	new_affected_by &= ~(affectmask);		// Must mask out some stuff.

	if (new_affected_by) {
		sprintbits(new_affected_by, bits);
		fprintf(outfile, "Aff : %s\n", bits);
	}

//		Re-equip
	for (i = 0; i < NUM_WEARS; i++) {
		if (char_eq[i])
			equip_char(this, char_eq[i], i);
	}

	/* restore our points to previous level */
	GET_HIT(this) = hit;
	GET_MANA(this) = mana;
	GET_MOVE(this) = move;

	fclose(outfile);

	sprintf(buf3, "mv pfile.out %s", outname);
	system(buf3);

	release_buffer(outname);
	release_buffer(bits);
}


  /* Load a char, TRUE if loaded, FALSE if not */
SInt32 Character::Load(char *charname) {
	SInt32		id;
	SInt32		num[5];
	FILE *		fl;
	char *		buf2 = NULL;

	if (!player || (player == &dummy_player))
		player = new PlayerData;

	if((id = find_name(charname)) < 0)		return (-1);
	else {
		char *filename = get_buffer(40);
		char *buf = get_buffer(128);
		strcpy(buf, player_table[id].name);
		for (SInt32 i = 0; (buf[i] = LOWER(buf[i])); i++);

		sprintf(filename, PLR_PREFIX "%c" SEPERATOR "%s", *buf, buf);

		if(!(fl = fopen(filename, "r"))) {
			mudlogf(NRM, NULL, TRUE,  "SYSERR: Couldn't open player file %s", filename);
			release_buffer(filename);
			release_buffer(buf);
			return (-1);
		}
		char *line = get_buffer(MAX_INPUT_LENGTH+1);
		char *tag = get_buffer(6);

		while(get_line(fl, line)) {
			tag_argument(line, tag);
			num[0] = atoi(line);
			switch (*tag) {
			case 'A':
				if (!strcmp(tag, "Act "))	general.act = asciiflag_conv(line);
				else if (!strcmp(tag, "Adv "))	general.advantages = asciiflag_conv(line);
				else if (!strcmp(tag, "Aff "))	affectbits = asciiflag_conv(line);
				else if (!strcmp(tag, "Affs")) {
					SInt32	i = 0;
					Affect *aff;
					do {
						get_line(fl, line);
						sscanf(line, " %d %d %d %d %d", num, num + 1, num + 2, num + 3, num + 4);
						if (num != 0) {
							aff = new Affect((num[0]), num[2],
									static_cast<Affect::Location>(num[3]), num[4]);
							if (aff->AffType())	aff->ToThing(this, num[1]);
							else				delete aff;
						}
						i++;
					} while (num && i < MAX_AFFECT);
				}
				else if (!strcmp(tag, "Alis")) {
					char *cmd = get_buffer(MAX_INPUT_LENGTH);
					char *repl = get_buffer(MAX_INPUT_LENGTH);
					half_chop(line, cmd, repl);
					Alias *	a = new Alias(cmd, repl);
					player->aliases.Add(a);
					release_buffer(cmd);
					release_buffer(repl);
				}
				else if (!strcmp(tag, "Algn"))	general.alignment = num[0];
				/* Default */
				else	log("SYSERR: Unknown tag %s in pfile %s", tag, charname);
				break;
			case 'B':
				if (!strcmp(tag, "Badp"))	player->special.bad_pws = num[0];
				else if (!strcmp(tag, "Bank"))	points.bank_gold = num[0];
				else if (!strcmp(tag, "Brth"))	player->time.birth = num[0];
				else if (!strcmp(tag, "Body"))	general.bodytype = (Bodytype) num[0];
				/* Default */
				else	log("SYSERR: Unknown tag %s in pfile %s", tag, charname);
				break;
			case 'C':
				if (!strcmp(tag, "Call")) {
					char *id = get_buffer(MAX_INPUT_LENGTH);
					char *callname = get_buffer(MAX_INPUT_LENGTH);
					half_chop(line, id, callname);
					IDNum idnum = atoi(id);
					player->CalledNames[idnum].Set(idnum, callname);
					release_buffer(id);
					release_buffer(callname);
				}
				else if (!strcmp(tag, "Clss"))	general.clss = (Class) num[0];
				else if (!strcmp(tag, "Cha "))	real_abils.Cha = num[0];
				else if (!strcmp(tag, "Chan"))	player->special.channels = asciiflag_conv(line);
				else if (!strcmp(tag, "Clan")) {
					sscanf(line, " %d %d", num, num + 1);
					if (Clan::Find(num[0]) && Clan::Clans[num[0]].IsMember(GET_IDNUM(this))) {
						general.misc.clannum = num[0];
						general.misc.clanrank = num[1];
					}
				}
				else if (!strcmp(tag, "Cmbt"))	combat_profile.Read(line);
				else if (!strcmp(tag, "Con "))	real_abils.Con = num[0];
				/* Default */
				else	log("SYSERR: Unknown tag %s in pfile %s", tag, charname);
				break;
			case 'D':
				if (!strcmp(tag, "DAdv"))		general.disadvantages = asciiflag_conv(line);
				else if (!strcmp(tag, "Desc"))	general.longDesc = SString::fread(fl, line, filename);
				else if (!strcmp(tag, "Dex "))	real_abils.Dex = num[0];
				// else if (!strcmp(tag, "Die "))	player->special.last_died = num[0];
				else if (!strcmp(tag, "Drnk"))	general.conditions[2] = num[0];
				else if (!strcmp(tag, "Drol"))	points.damroll = num[0];
				else if (!strcmp(tag, "DR  "))	damage_resistance = num[0];
				/* Default */
				else	log("SYSERR: Unknown tag %s in pfile %s", tag, charname);
				break;
			case 'E':
				if (!strcmp(tag, "EMal"))	player->email = str_dup(line);
				else if (!strcmp(tag, "Exp "))	points.exp = num[0];
				else if (!strcmp(tag, "ExpM"))	general.exp_modifier = num[0];
				else if (!strcmp(tag, "Ener")) {
					do {
						get_line(fl, line);
						sscanf(line, " %d %d %d", num, num + 1, num + 2);
						if (num[0]) {
                        	ENERGY(this).Charge(this, num[0], num[1], num[2]);
						}
					} while (num[0]);
				}
				/* Default */
				else	log("SYSERR: Unknown tag %s in pfile %s", tag, charname);
				break;
			case 'F':
				if (!strcmp(tag, "Frez"))	player->special.freeze_level = num[0];
				/* Default */
				else	log("SYSERR: Unknown tag %s in pfile %s", tag, charname);
				break;
			case 'G':
				if (!strcmp(tag, "Gold"))	points.gold = num[0];
				/* Default */
				else	log("SYSERR: Unknown tag %s in pfile %s", tag, charname);
				break;
			case 'H':
				if (!strcmp(tag, "Hit ")) {
					sscanf(line, " %d / %d", num, num + 1);
					points.hit = num[0];
					points.max_hit = num[1];
				}
				else if (!strcmp(tag, "Hite"))	GET_HEIGHT(this) = num[0];
				else if (!strcmp(tag, "Host"))	player->host = str_dup(line);
				else if (!strcmp(tag, "Hrol"))	points.hitroll = num[0];
				else if (!strcmp(tag, "Hung"))	general.conditions[0] = num[0];
				/* Default */
				else	log("SYSERR: Unknown tag %s in pfile %s", tag, charname);
				break;
			case 'I':
				if (!strcmp(tag, "Id  "))	player->idnum = num[0];
				else if (!strcmp(tag, "Int "))	real_abils.Int = num[0];
				/* Default */
				else	log("SYSERR: Unknown tag %s in pfile %s", tag, charname);
				break;
			case 'K':
				if (!strcmp(tag, "Keyw")) {
					SetKeywords(line);
				}
				break;
			case 'L':
				if (!strcmp(tag, "Last"))	player->time.logon = num[0];
				else if (!strcmp(tag, "Lawf"))	general.lawfulness = num[0];
				else if (!strcmp(tag, "Levl"))	GET_LEVEL(this) = num[0];
				/* Default */
				else	log("SYSERR: Unknown tag %s in pfile %s", tag, charname);
				break;
			case 'M':
				if (!strcmp(tag, "Mana")) {
					sscanf(line, " %d / %d", num, num + 1);
					points.mana = num[0];
					points.max_mana = num[1];
				}
				else if (!strcmp(tag, "Move")) {
					sscanf(line, " %d / %d", num, num + 1);
					points.move = num[0];
					points.max_move = num[1];
				}
				else if (!strcmp(tag, "Mort"))	general.mortality = (Mortality) num[0];
				else if (!strcmp(tag, "Mtrl"))	material = (::Materials::Material) num[0];
				/* Default */
				else	log("SYSERR: Unknown tag %s in pfile %s", tag, charname);
				break;
			case 'N':
				if (!strcmp(tag, "Name")) {
					SetName(line);
					SetKeywords(Name());
				}
				/* Default */
				else	log("SYSERR: Unknown tag %s in pfile %s", tag, charname);
				break;
			case 'P':
				if (!strcmp(tag, "Pass")) {
					strncpy(player->passwd, line, MAX_PWD_LENGTH);
					player->passwd[MAX_PWD_LENGTH] = '\0';
				}
				else if (!strcmp(tag, "PD  "))	passive_defense = num[0];
				else if (!strcmp(tag, "Pfin"))	player->poofin = str_dup(line);
				else if (!strcmp(tag, "Pfou"))	player->poofout = str_dup(line);
				else if (!strcmp(tag, "PLen"))	player->special.page_length = num[0];
				else if (!strcmp(tag, "Plyd"))	player->time.played = num[0];
				else if (!strcmp(tag, "Prac"))	points.pracs = num[0];
				else if (!strcmp(tag, "Pref"))	player->special.preferences = asciiflag_conv(line);
				else if (!strcmp(tag, "Prmp"))	player->prompt = str_dup(line);
				/* Default */
				else	log("SYSERR: Unknown tag %s in pfile %s", tag, charname);
				break;
			case 'R':
				if (!strcmp(tag, "Race"))	GET_RACE(this) = (Race)num[0];
				else if (!strcmp(tag, "Room"))	player->special.load_room = num[0];
				/* Default */
				else	log("SYSERR: Unknown tag %s in pfile %s", tag, charname);
				break;
			case 'S':
				if (!strcmp(tag, "Sayc"))		player->special.saycolor = num[0];
				else if (!strcmp(tag, "Sex "))	GET_SEX(this) = (Sex)num[0];
				else if (!strcmp(tag, "SInv"))	player->special.staff_invis = num[0];
				else if (!strcmp(tag, "Skil")) {
					do {
						get_line(fl, line);
						sscanf(line, " %d %d %d", num, num + 1, num + 2);
						if (num[0])		general.skills.SetSkill(num[0], NULL, num[1], num[2]);
					} while (num[0]);
				}
				else if (!strcmp(tag, "Str "))	real_abils.Str = num[0];
				else if (!strcmp(tag, "Stf "))	player->special.staff_flags = asciiflag_conv(line);
				else if (!strcmp(tag, "SDsc")) {
					SetSDesc(line);
				}
				/* Default */
				else	log("SYSERR: Unknown tag %s in pfile %s", tag, charname);
				break;
			case 'T':
				if (!strcmp(tag, "Thir"))	general.conditions[1] = num[0];
				else if (!strcmp(tag, "Titl"))	general.title = SString::Create(line);
				else if (!strcmp(tag, "Trst"))	RANGE(0, player->special.trust = num[0], MAX_TRUST);
				else if (!strcmp(tag, "Totl"))	general.totalpts = num[0];
				else if (!strcmp(tag, "Trig")) {
					do {
						get_line(fl, line);
						sscanf(line, "%d", num);
						if (num[0] && Trigger::Find(num[0])) {
							if (!SCRIPT(this))
								SCRIPT(this) = new Script(Datatypes::Character);
							add_trigger(SCRIPT(this), Trigger::Read(num[0]));
						}
					} while (num[0]);
				}
				/* Default */
				else	log("SYSERR: Unknown tag %s in pfile %s", tag, charname);
				break;
			case 'W':
				if (!strcmp(tag, "Wate"))	GET_WEIGHT(this) = num[0];
				else if (!strcmp(tag, "Wimp"))	player->special.wimp_level = num[0];
				else if (!strcmp(tag, "Wis "))	real_abils.Wis = num[0];
				/* Default */
				else	log("SYSERR: Unknown tag %s in pfile %s", tag, charname);
				break;
			default:
				/* Default */
				log("SYSERR: Unknown tag %s in pfile %s", tag, charname);
				break;
			}
		}
		release_buffer(buf);
		release_buffer(line);
		release_buffer(filename);
		release_buffer(tag);
	}
	fclose(fl);


	// Final data initialization
	aff_abils = real_abils;	// Copy abil scores
	general.misc.carry_weight = 0;
	general.misc.carry_items = 0;

	/* initialization for imms */
	if (IS_STAFF(this)) {
		// for (SInt32 i = 1; i <= MAX_SKILLS; i++)
		// 	SET_SKILL(this, i, 100);
		for (int i = 0; i < 3; ++i)
			general.conditions[i] = -1;
	}

	/*
	 * If you're not poisoned and you've been away for more than an hour of
	 * real time, we'll set your HMV back to full
	 */
	if (!AFF_FLAGGED(this, AffectBit::Poison) && ((time(0) - player->time.logon) >= SECS_PER_REAL_HOUR)) {
		GET_HIT(this) = GET_MAX_HIT(this);
		GET_MOVE(this) = GET_MAX_MOVE(this);
	}

	ID(GET_IDNUM(this));
	if (!SSName() || (GET_LEVEL(this) < 1))
		return (-1);

	if (IS_NPC(this))
		general.act = 0;

	GET_TOTALPTS(this) = sum_char_pts(this);

	return (id);
}


/* Called when a character that follows/is followed dies */
void Character::DieFollower(void) {
	Character *		follower;

	if (master)
		StopFollower();

	START_ITER(iter, follower, Character *, followers) {
		follower->StopFollower();
	}
}


/* Called when stop following persons, or stopping charm */
/* This will NOT do if a character quits/dies!!          */
void Character::StopFollower(void) {
	if (!master) {
		core_dump();
		return;
	}

	if (AFF_FLAGGED(this, AffectBit::Charm)) {
		act("You realize that $N is a jerk!", FALSE, this, 0, master, TO_CHAR);
		act("$n realizes that $N is a jerk!", FALSE, this, 0, master, TO_NOTVICT);
		act("$n hates your guts!", FALSE, this, 0, master, TO_VICT);
		if (Affect::AffectedBy(this, SPELL_CHARM))
			Affect::FromThing(this, SPELL_CHARM);
	} else {
		act("You stop following $N.", FALSE, this, 0, master, TO_CHAR);
		act("$n stops following $N.", TRUE, this, 0, master, TO_NOTVICT);
		act("$n stops following you.", TRUE, this, 0, master, TO_VICT);
	}

//	if (master->followers->follower == this) {	/* Head of follower-list? */
//		k = master->followers;
//		master->followers = k->next;
//		FREE(k);
//	} else {			/* locate follower who is not head of list */
//		for (k = master->followers; k->next->follower != this; k = k->next);
//
//		j = k->next;
//		k->next = j->next;
//		FREE(j);
//	}
	master->followers.Remove(this);
	master = NULL;
	REMOVE_BIT(AFF_FLAGS(this), AffectBit::Charm | AffectBit::Group);
	GROUP_POS(this) = 0;
}



#define READ_TITLE(ch, lev) (GET_SEX(ch) != Female ?   \
	titles[(int)GET_RACE(ch)][lev].title_m :  \
	titles[(int)GET_RACE(ch)][lev].title_f)


void Character::SetTitle(char *title) {

	if (general.title)	SSFree(general.title);
	general.title = NULL;

	if (title) {
		if (strlen(title) > MAX_TITLE_LENGTH)	title[MAX_TITLE_LENGTH] = '\0';
		general.title = SString::Create(title);
	} else {
		general.title = SString::Create("the adventurer");
	}
}


void Character::SetLevel(UInt16 level) {
	SInt32	diff;

	// if (IS_NPC(this))				return;

	if (level > MAX_LEVEL)			return;
	if (level == GET_LEVEL(this))	return;

	diff = level - GET_LEVEL(this);
	GET_LEVEL(this) = level;

	if (diff == 1 || diff == -1)	Sendf("You %s a level!\r\n", diff == 1 ? "rise" : "drop");
	else							Sendf("You %s %d levels!\r\n", diff > 0 ? "rise" : "drop", abs(diff));

	if (GET_EXP(this) < 0)			GET_EXP(this) = 0;

	if ((diff >= 1) && (GET_LEVEL(this) >= GET_MAX_LEVEL(this))) {
		GET_PRACTICES(this) += MAX(1, (GET_INT(this) + GET_WIS(this)) / 6) * diff;
	}

	// SetTitle(NULL);
}


void Character::UpdatePos(void) {

	Position prev_pos = GET_POS(this);

	if ((GET_HIT(this) > 0) && (GET_POS(this) > POS_STUNNED))	return;
	else if (GET_HIT(this) > 0) {
		if (GET_POS(this) > POS_STUNNED)						GET_POS(this) = POS_STANDING;
		else if (!AFF_FLAGGED(this, AffectBit::Immobilized))	GET_POS(this) = POS_SITTING;
		if (prev_pos <= POS_STUNNED && GET_CLASS(this) != CLASS_GHOST) {
			Send("You struggle to an upright position.\r\n");
			act("$n struggles to an upright position.", false, this, 0, 0, TO_ROOM);
		}
	}
	else if (GET_HIT(this) <= HIT_DEAD)			GET_POS(this) = POS_DEAD;
	else if (GET_HIT(this) <= HIT_MORTALLYW)	GET_POS(this) = POS_MORTALLYW;
	else if (GET_HIT(this) <= HIT_INCAP)		GET_POS(this) = POS_INCAP;
	else										GET_POS(this) = POS_STUNNED;

}


/* Get the start room number */
VNum Character::StartRoom(void) {
	VNum	load_room;

	if ((load_room = GET_LOADROOM(this)) != NOWHERE)
		load_room = Room::Find(load_room) ? load_room : NOWHERE;

	if ((load_room == NOWHERE) && Clan::Find(GET_CLAN(this)) && (GET_CLAN_LEVEL(this) >= Clan::Rank::Member))
		load_room = Room::Find(Clan::Clans[GET_CLAN(this)].room) ? Clan::Clans[GET_CLAN(this)].room : NOWHERE;

	if (load_room == NOWHERE) {
		if (IS_STAFF(this))			load_room = immort_start_room;
		// else						load_room = start_rooms[GET_RACE(this)];
	}

	if (load_room == NOWHERE || !Room::Find(load_room))
		load_room = mortal_start_room;

	if (!PLR_FLAGGED(this, PLR_LOADROOM))
		GET_LOADROOM(this) = NOWHERE;

	if (PLR_FLAGGED(this, PLR_JAILED))
		load_room = jailed_start_room;

	if (PLR_FLAGGED(this, PLR_FROZEN))
		load_room = frozen_start_room;

	return load_room;
}


Relation Character::GetRelation(const Character *target) const {
	VNum	chClan, tarClan;

#ifdef GOT_RID_OF_IT
	if (NO_STAFF_HASSLE(this) || NO_STAFF_HASSLE(target))
		return RELATION_FRIEND;						//	One is a Staff

	if (IS_TRAITOR(this) ^ IS_TRAITOR(target))
		return RELATION_ENEMY;						//	One is a traitor, other is not
#endif

	if (is_same_group(this, target)) {
		return RELATION_FRIEND;
	}

	chClan = (GET_CLAN_LEVEL(this) >= Clan::Rank::Member && Clan::Find(GET_CLAN(this))) ? GET_CLAN(this) : -1;
	tarClan = (GET_CLAN_LEVEL(target) >= Clan::Rank::Member && Clan::Find(GET_CLAN(target))) ? GET_CLAN(this) : -1;

	// Possibility:  Opposed alignments?

	if (chClan != -1 && chClan == tarClan) {
		Echo("Same clan.");
		return RELATION_FRIEND;
	}

	if (IS_NPC(this)) {
		if (mob->hates && mob->hates->Contains(GET_ID(target))) {
			return RELATION_ENEMY;
		} else if (Virtual() == target->Virtual()) {	// We'll assume that mobs are friendly to others of the same vnum
			return RELATION_FRIEND;
		} else if ((IS_GOOD(target) && MOB_FLAGGED(this, MOB_AGGR_GOOD)) ||
			(IS_NEUTRAL(target) && MOB_FLAGGED(this, MOB_AGGR_NEUTRAL)) ||
			(IS_EVIL(target) && MOB_FLAGGED(this, MOB_AGGR_EVIL))) {
			return RELATION_ENEMY;						// Hostile to alignment
		} else if (MOB_FLAGGED(this, MOB_HELPER_RACE) && (GET_RACE(target) == GET_RACE(this))) {
			return RELATION_FRIEND;
		} else if (MOB_FLAGGED(this, MOB_HELPER_ALIGN) &&
			(IS_GOOD(this) && IS_GOOD(target)) ||
			(IS_NEUTRAL(this) && IS_NEUTRAL(target)) ||
			(IS_EVIL(this) && IS_EVIL(target))) {
			return RELATION_FRIEND;						// Friendly to alignment
		}
	}

	return RELATION_NEUTRAL;
}


bool Character::LightOk(void) const {
	if (AFF_FLAGGED(this, AffectBit::Blind))
		return false;

	if (!IS_LIGHT(IN_ROOM(this)) && !AFF_FLAGGED(this, AffectBit::Infravision))
		return false;

	return true;
}


bool Character::InvisOk(const Character *target) const {
	if (AFF_FLAGGED(target, AffectBit::Invisible) &&
		!AFF_FLAGGED(this, AffectBit::DetectInvis) && !IS_STAFF(this))
		return false;

	if (AFF_FLAGGED(target, AffectBit::Hide) &&
		!AFF_FLAGGED(this, AffectBit::SenseLife) && !IS_STAFF(this))
		return false;

	return true;
}


bool Character::CanSeeStaff(const Character *target) const {
	if (!target)								return false;
	if (SELF(this, target))						return true;	//	this == target
	if (!IS_STAFF(target))						return true;

	if (GET_TRUST(this) < GET_STAFF_INVIS(target))	return false;

	return true;
}


#if 0
bool Character::CanSee(const Object *target) const {
	if (!target)									return false;
	if (PRF_FLAGGED(this, Preference::HolyLight))	return true;
	if (!LightOk())									return false;

	if (OBJ_FLAGGED(target, ITEM_INVISIBLE) && !AFF_FLAGGED(this, AffectBit::DetectInvis))
		return false;

	if (target->Inside() != Inside()) {
		if (target->InObj() && !CanSee(target->InObj()))
			return false;

		if (target->CarriedBy() && !CanSee(target->CarriedBy()))
			return false;
	}

	return true;
}
#endif


bool Character::CanUse(const Object *obj) const {
	// Changepoint:  We may want this sort of thing.
	return 1;
#ifdef GOT_RID_OF_IT
	if (NO_STAFF_HASSLE(this))	return true;
	return !((OBJ_FLAGGED(obj, ITEM_ANTI_HUMAN) && IS_HUMAN(this)) ||
			(OBJ_FLAGGED(obj, ITEM_ANTI_SYNTHETIC) && IS_SYNTHETIC(this)) ||
			(OBJ_FLAGGED(obj, ITEM_ANTI_PREDATOR) && IS_PREDATOR(this)) ||
			(!OBJ_FLAGGED(obj, ITEM_ALIEN) && IS_ALIEN(this)));
#endif
}

/*
RNum Character::Real(VNum vnum) {
	RNum	bot = 0,
			top = top_of_mobt;

	//	First binary search
	for (;;) {
		RNum mid = (bot + top) / 2;

		if (Character::Index[mid].vnum == vnum)	return mid;
		if (bot >= top)						break;
		if (Character::Index[mid].vnum > vnum)		top = mid - 1;
		else								bot = mid + 1;
	}

	//	Then, if not found, linear
	for (RNum nr = 0; nr <= top_of_mobt; nr++)
		if(Character::Index[nr].vnum == vnum)
			return nr;

	return -1;
}
*/

/* create a new mobile from a prototype */
Character *Character::Read(VNum nr) {
	static UInt32 mob_tick_count = 0;
	Character *mob;
	Trigger *trig;
    SkillKnown *tskill = NULL;

	if (!Character::Find(nr)) {
		log("Mobile (V) %d does not exist in database.", nr);
		return NULL;
	}

	mob = new Character(*Index[nr].proto);

    mob->points.max_hit = dice(mob->mob->numhitdice, mob->mob->sizehitdice) + mob->mob->modhitdice;
    mob->points.max_mana = GET_INT(mob) * 10;
    mob->points.max_move = GET_STR(mob) * 10;

    mob->points.hit = mob->points.max_hit;
    mob->points.mana = mob->points.max_mana;
    mob->points.move = mob->points.max_move;

    GET_TOTALPTS(mob) = sum_char_pts(mob);

	mob->mob->tick = mob_tick_count++;

	if (mob_tick_count == TICK_WRAP_COUNT)
		mob_tick_count=0;

	mob->ToWorld();

	Index[nr].number++;

    if (!IS_SIMPLE_MOB(mob)) {
      // Set the tskills pointer.
      (Index[nr].proto)->general.skills.GetTable(&tskill);

      for (; tskill; tskill = tskill->next) {
        SET_SKILL(mob, tskill->skillnum, tskill->proficient);
      }
    }

	/* add triggers */
	for (int i = 0; i < Index[nr].triggers.Count(); i++) {
		VNum	vnum = Index[nr].triggers[i];
		if (!Trigger::Find(vnum))
			log("SYSERR: Invalid trigger %d assigned to mob %d", vnum, nr);
		else {
			trig = Trigger::Read(vnum);
			if (!SCRIPT(mob))
				SCRIPT(mob) = new Script(Datatypes::Character);
			add_trigger(SCRIPT(mob), trig); //, 0);
		}
	}

	mob->ID(max_id++);

	return mob;
}


void Character::Appear(void) {
	if (Affect::AffectedBy(this, SPELL_INVISIBLE))
		Affect::FromThing(this, SPELL_INVISIBLE);

	REMOVE_BIT(AFF_FLAGS(this), AffectBit::Invisible | AffectBit::Hide | AffectBit::Camoflauge);
	REMOVE_BIT(PRF_FLAGS(this), Preference::StaffInvis | Preference::AdminInvis);

	GET_STAFF_INVIS(this) = 0;

	if (!IS_STAFF(this))	act("$n slowly fades into existence.", false, this, 0, 0, TO_ROOM);
	else				act("$n suddenly appears out of nowhere!", false, this, 0, 0, TO_ROOM);

	SetInvis(0);
}


Character *	Character::TargetChar(const char *arg) const
	{ return get_char_vis(this, arg, FIND_CHAR_WORLD);	}


Object *	Character::TargetObj(const char *arg) const
	{ return get_obj_vis(this, arg);	}


Room *		Character::TargetRoom(const char *arg) const {
	VNum		location = NOWHERE;
	Character *	targetMob;
	Object *	targetObj;

	if (is_number(arg) && !strchr(arg, '.'))	location = atoi(arg);
	else if ((targetMob = TargetChar(arg)))	location = IN_ROOM(targetMob);
	else if ((targetObj = TargetObj(arg)))	location = IN_ROOM(targetObj);

	return (Room::Find(location) ? &world[location] : NULL);
}


//	Changepoint:  Put this elsewhere?
//	We could make the viewer GameData, but that's a lot of busy work for very
//	little gain. -DH
bool GameData::CanBeSeen(const Character *viewer) const {
	if (this == viewer)							return true;	//	this == target

	if (!PRF_FLAGGED(viewer, Preference::HolyLight)) {
		// if (MOB_FLAGGED(target, MOB_PROGMOB))	return false;	//	ProgMob check
		if (!viewer->LightOk())					return false;	//	Enough light to see
	}

	return true;
}


//	Changepoint:  Put this elsewhere?
//	We could make the viewer GameData, but that's a lot of busy work for very
//	little gain. -DH
bool MUDObject::CanBeSeen(const Character *viewer) const {
	if (this == viewer)							return true;	//	this == target

	if (!PRF_FLAGGED(viewer, Preference::HolyLight)) {
		if ((material == Materials::Ether) && (viewer->material != Materials::Ether))
			return false;
		// if (MOB_FLAGGED(target, MOB_PROGMOB))	return false;	//	ProgMob check
		if (!viewer->LightOk())					return false;	//	Enough light to see
	}

	return true;
}


//	Changepoint:  Put this elsewhere?
//	We could make the viewer GameData, but that's a lot of busy work for very
//	little gain. -DH
bool Character::CanBeSeen(const Character *viewer) const {
	if (this == viewer)							return true;	//	this == target

	if (!PRF_FLAGGED(viewer, Preference::HolyLight)) {
		if ((material == Materials::Ether) && (viewer->material != Materials::Ether))
			return false;
		// if (MOB_FLAGGED(target, MOB_PROGMOB))	return false;	//	ProgMob check
		if (!viewer->LightOk())					return false;	//	Enough light to see
	}

	if (!viewer->CanSeeStaff((Character *) this))	return false;	//	Invisibility level
	if (!viewer->InvisOk((Character *) this))		return false;	//	Invis check

	return true;
}


UInt32 Character::PointsSpent(void)
{
	Affect *af;
	int i, j;
	UInt32 sum, savehp = 0, savemana = 0, savemove = 0;
	LListIterator<Affect *>		iter(this->affected);

	//	Unnaffect everything
	for (i = 0; i < NUM_WEARS; i++) {
		if (equipment[i])
			for (j = 0; j < MAX_OBJ_AFFECT; ++j)
				this->AffectModify(equipment[i]->affectmod[j].location,
						equipment[i]->affectmod[j].modifier, equipment[i]->affectbits, false);
	}
	while ((af = iter.Next()))
		af->Modify(this, false);

	//	Restore original
	aff_abils = real_abils;

	//	Now that we've rid the character of affects, figure out what they're worth.
	sum = sum_char_pts(this);

	//	Re-apply effects
	for (i = 0; i < NUM_WEARS; i++) {
		if (GET_EQ(this, i))
			for (j = 0; j < MAX_OBJ_AFFECT; j++)
				this->AffectModify(GET_EQ(this, i)->affectmod[j].location,
						GET_EQ(this, i)->affectmod[j].modifier, GET_EQ(this, i)->affectbits, true);
	}

	iter.Reset();
	while ((af = iter.Next()))
		af->Modify(this, true);

	GET_HIT(this) = savehp;
	GET_MANA(this) = savemana;
	GET_MOVE(this) = savemove;

	return sum;
}


void Character::SetMount(MUDObject * thing)
{
	sitting_on = thing;
}


void Character::GainExp(int gain)
{
	int num_levels = 0;

	if (GET_LEVEL(this) >= MAX_LEVEL)
		return;

	if (!IS_NPC(this) && (GET_LEVEL(this) < 1))
		return;

	if (gain > 0) {
		GET_EXP(this) += MIN(gain, max_exp_gain);
		while ((GET_EXP(this) >= EXP_TO_LEVEL) && GET_LEVEL(this) < MAX_LEVEL) {
			GET_EXP(this) -= EXP_TO_LEVEL;
			++num_levels;
		}


		if (num_levels == 1) {
			Send("You rise a level!\r\n");
			AdvanceLevel(num_levels);
			if (!IS_NPC(this)) {
				mudlogf(NRM, this, true, "%s advanced to level %d", GET_NAME(this), GET_LEVEL(this));
			}
		} else if (num_levels > 1) {
			Sendf("You rise %d levels!\r\n", num_levels);
			AdvanceLevel(num_levels);
			if (!IS_NPC(this)) {
				mudlogf(NRM, this, true, "%s advanced %d levels to level %d", GET_NAME(this), num_levels, GET_LEVEL(this));
			}
			core_dump();
		}

	} else if (gain < 0) {
		gain = MAX(max_exp_loss, gain);	/* Cap max exp lost per death */
		GET_EXP(this) -= gain;
		if (GET_EXP(this) < 0)
			GET_EXP(this) = 0;
	}
}


void Character::AdvanceLevel(int amount)
{
	static const int pracgain = 2 + MAX(1, (GET_INT(this) + GET_WIS(this)) / 6);

	GET_LEVEL(this) += amount;

	if (GET_LEVEL(this) > GET_MAX_LEVEL(this)) {
		GET_MAX_LEVEL(this) = GET_LEVEL(this);
	}

	GET_PRACTICES(this) += pracgain * amount;
	/* start regening new points */
	AlterHit(this, 0);
	AlterMana(this, 0);
	AlterMove(this, 0);

	GET_TOTALPTS(this) = sum_char_pts(this);

	Save(AbsoluteRoom());
}


void str_swing_dice(SInt16 strength, SInt8 &num, SInt8 &dicemod)
{
	SInt8 i;		// Number of dice involved

	for (num = 1, i = 6, dicemod = -5; i <= strength; ++i) {
		if ((++dicemod) == 3) {
			++num;
			dicemod = -1;
		}
	}
}


void str_thrust_dice(SInt16 strength, SInt8 &num, SInt8 &dicemod)
{
	SInt8 i;		// Number of dice involved

	for (num = 1, i = 0, dicemod = -7; i <= strength; ++i) {
		if ((i % 2 ? ++dicemod : 0) == 3) {
			++num;
			dicemod = -1;
		}
	}
}


int str_dam(SInt16 strength, SInt16 move)
{
	SInt8 dicemod,
			 size = 6,
			 num = 1;		// Number of dice involved

	switch (move) {
	case Combat::ThrustRight:
	case Combat::ThrustLeft:
	case Combat::ThrustThird:
	case Combat::ThrustFourth:
	case Combat::Punch:
	case Combat::Bash:
		str_thrust_dice(strength, num, dicemod);
		break;
	case Combat::SwingRight:
	case Combat::SwingLeft:
	case Combat::SwingThird:
	case Combat::SwingFourth:
	case Combat::Kick:
	default:
		str_swing_dice(strength, num, dicemod);
			break;
	}

	return (MAX((dice(num, size) + dicemod), 1));

}


SInt8 rate_encumbrance(Character *ch, int weight)
{
	int encumbrance;
	
	switch (GET_MATERIAL(ch)) {
	case Materials::Ether:
	case Materials::Shadow:
		return 0;
	default:
		break;
	}


	encumbrance = IS_CARRYING_W(ch) / MAX(GET_STR(ch), (int) 1);

	if (encumbrance < 2)			return 0;
	else if (encumbrance < 4)		return 1;
	else if (encumbrance <= 6)		return 2;
	else if (encumbrance <= 12)		return 3;
	else if (encumbrance <= 20)		return 4;
	else if (encumbrance <= 30)		return 5;
	else							return 6;
}


// Returns cost of a given stat level
int stat_cost(int level)
{
	int cost;

	if (level < 0)		level = 0;

	if (level > 20) 	cost = stat_cost_table[20] + (25 * (level - 20));
	else				cost = stat_cost_table[level];

	return (cost);
}


// Returns stat for a given number of points
int stat_ptlevel(int cost)
{
	int level = 0;

	while (stat_cost(level + 1) < cost)
		++level;

	return (level);
}


// This routine used to calculate the cost given a level
int generic_cost(int level, SInt16 start, SInt16 mod)
{
	int cost;

	if (level == 0) {
		return 0;
	}

	if (level < 0)		cost = ((level + 1) * mod) - start;
	else				cost = ((level - 1) * mod) + start;

	return (cost);
}


// This routine used to calculate the level given a cost
int generic_ptlevel(int cost, SInt16 start, SInt16 mod)
{
	if (cost == 0)
		return 0;

	int level = 0;
	bool negative = FALSE;

	if (cost < 0) {
		cost = -cost;
		negative = TRUE;
	}

	if (cost > start) {
		cost -= start;
		level = 1;
	} else {
		return 0;
	}

	level += (cost / mod);

	if (negative)
		level = -level;

	return (level);
}


// Returns character points spent on all abilities
int sum_char_pts(Character *ch)
{
	int sum = 0, i,
			variant = 0;	// This is used to help augment differences because of HP

	// Add in points for stats
	sum += stat_cost(ch->real_abils.Str - mod_stat_table[GET_RACE(ch)][0]);
	sum += stat_cost(ch->real_abils.Int - mod_stat_table[GET_RACE(ch)][1]);
	sum += stat_cost(ch->real_abils.Wis - mod_stat_table[GET_RACE(ch)][2]);
	sum += stat_cost(ch->real_abils.Dex - mod_stat_table[GET_RACE(ch)][3]);
	sum += stat_cost(ch->real_abils.Con - mod_stat_table[GET_RACE(ch)][4]);
	sum += stat_cost(ch->real_abils.Cha - mod_stat_table[GET_RACE(ch)][5]);

	// Now, add in the points for hit points, mana, move...
	sum += (((GET_MAX_HIT(ch)) - (GET_CON(ch) * 10)) / 5);
	sum += (((GET_MAX_MANA(ch)) - (GET_INT(ch) * 10)) / 5);
	sum += (((GET_MAX_MOVE(ch)) - (GET_STR(ch) * 10)) / 5);

	// Account for default skills for mobs.	This is crude.
	if (IS_NPC(ch))
		sum += GET_LEVEL(ch) * 5;

	sum += generic_cost(GET_DR(ch), 10, 10);
	sum += generic_cost(GET_DAMROLL(ch), 5, 5);
	sum += generic_cost(GET_HITROLL(ch), 5, 5);

	// An arbitrary modifier in case there are unquantifiable elements, i.e.
	// scripts.
	sum += GET_EXP_MOD(ch);

	// Get the total number of practice points invested in skills
	sum += CHAR_SKILL(ch).TotalPoints(ch);

	sum += GET_PRACTICES(ch);

	for (i = 0; i < 32; ++i) {
		if (ADVANTAGED(ch, (1 << i)))
			sum += advantage_data[i].cost;
	}

	for (i = 0; i < 32; ++i) {
		if (DISADVANTAGED(ch, (1 << i)))
			sum += disadvantage_data[i].cost;
	}

	variant = (sum * GET_MAX_HIT(ch)) / MAX((GET_CON(ch) * 10), 1);
	if (variant > sum)
		sum = (sum + variant + sum) / 3;

	return sum;
}


SInt16 get_weapon_skill(Character *ch, SInt8 hand = 0)
{
	if (hand && GET_EQ(ch, hand) && GET_OBJ_TYPE(GET_EQ(ch, hand)) == ITEM_WEAPON)
		return (GET_OBJ_VAL(GET_EQ(ch, hand), 0));
	else
		return (0);

	for (hand = WEAR_HAND_R; hand <= WEAR_HAND_4; ++hand) {
		if (GET_EQ(ch, hand) && (GET_OBJ_TYPE(GET_EQ(ch, hand)) == ITEM_WEAPON))
			return (GET_OBJ_VAL(GET_EQ(ch, hand), 0));
	}

	return (0);
}


const SInt16 stat_cost_table[] = {
	-100,
	-80,
	-70,
	-60,
	-50,
	-40,
	-30,
	-20,
	-15,
	-10,
	0,				// 10 is norm!
	10,
	20,
	30,
	45,
	60,
	80,
	100,
	125,
	150,
	175
};


const char *advantages[] = {
	"LUCKY",
	"FAST_HP_REGEN",
	"FAST_MANA_REGEN",
	"FAST_MOVE_REGEN",
	"SLOW_METABOLISM",
	"INFRAVISION",
	"FLIGHT",
	"NATURE_RECHARGE",
	"AMBIDEXTROUS",
	"GROUP_MAGIC_BONUS",
	"RUNE_AWARE",
	"COMBAT_REFLEXES",
	"SAPIENT",
	"\n"
};


const char *named_advantages[] = {
	"Lucky",
	"Fast healing",
	"Fast mana recharge",
	"High stamina",
	"Slow metabolism",
	"Infravision",
	"Can fly",
	"Nature affinity",
	"Ambidextrous",
	"Group magic bonus",
	"Markridden",
	"Combat reflexes",
	"Sapient",
	"\n"
};


const char *disadvantages[] = {
	"UNLUCKY",
	"SLOW_HP_REGEN",
	"SLOW_MANA_REGEN",
	"SLOW_MOVE_REGEN",
	"FAST_METABOLISM",
	"NO_MAGIC",
	"ATHEIST",
	"COLDBLOODED",
	"NECRO_DESTROYS_FLESH",
	"\n"
};


const char *named_disadvantages[] = {
	"Unlucky",
	"Slow healing",
	"Slow mana recharge",
	"Low stamina",
	"Fast metabolism",
	"Nonmagical",
	"Atheistic",
	"Cold blooded",
	"Shastick symbiots",
	"\n"
};


const struct advantage_dat advantage_data[] = {
	{ 0, 0, 15 },				// lucky
	{ Advantages::SlowMetabolism, Disadvantages::SlowHpRegen, 10 },				// fastHpRegen
	{ 0, Disadvantages::SlowManaRegen | Disadvantages::NoMagic, 5 },				// fastManaRegen
	{ Advantages::SlowMetabolism, Disadvantages::SlowMoveRegen, 5 },				// fastMoveRegen
	{ (Advantages::FastHpRegen | Advantages::FastMoveRegen | Advantages::Flight), Disadvantages::FastMetabolism, 10 },				// slowMetabolism
	{ 0, 0, 15 },				// infravision
	{ Advantages::SlowMetabolism, 0, 20 },				// flight
	{ 0, 0, 15 },				// natureRecharge
	{ 0, 0, 15 },				// ambidextrous
	{ 0, Disadvantages::NoMagic, 15 },				// groupMagicBonus
	{ 0, Disadvantages::NoMagic, 10 },				// runeAware
	{ 0, 0, 15 },				// combatReflexes
	{ 0, Disadvantages::NoMagic, 15 },				// Sapient
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 }
};


const struct advantage_dat disadvantage_data[] = {
	{ 0, 0, -10 },		// unlucky
	{ Advantages::FastHpRegen, 0, -10 },		// slowHpRegen
	{ Advantages::FastManaRegen, 0, -5 },		// slowManaRegen
	{ Advantages::FastMoveRegen, 0, -5 },		// slowMoveRegen
	{ Advantages::SlowMetabolism, 0, -10 },		// fastMetabolism
	{ Advantages::FastManaRegen | Advantages::GroupMagicBonus | Advantages::RuneAware | Advantages::Sapient, Disadvantages::SlowManaRegen, -20 },		// noMagic
	{ 0, 0, -20 },		// atheist (cannot pray)
	{ 0, Disadvantages::FastMetabolism, -10 },		// coldBlooded
	{ 0, 0, -10 },		// necroDestroysFlesh
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 }
};


const char *mobile_types[] = {
	"simple",
	"extended",
	"\n"
};


const char *group_positions[] = {
	"front",
	"back",
	"\n"
};


const char *lawfulness[] = {
	"Chaotic",
	"Neutral",
	"Lawful"
};

const char *alignment[] = {
	"Evil",
	"Neutral",
	"Good"
};


// Mobiles still use the class types to define "default" skills which they
// possess.  More complex mobs are constructed by defining skills.
const char *class_types[] = {
	"Undefined",
	"Magician",
	"Cleric",
	"Thief",
	"Warrior",
	"Ranger",
	"Assassin",
	"Alchemist",
	"Psionicist",
	"Insect",
	"Mammal",
	"Avian",
	"Fish",
	"Reptile",
	"Zombie",
	"Ghoul",
	"Ghost",
	"Poltergeist",
	"Wraith",
	"Spectre",
	"Vampire",
	"\n"
};


// Array used to filter race advantages
const long race_advantage_filter[NUM_RACES] =
{
	0,																				// Animal
	(Advantages::Flight | Advantages::Infravision | Advantages::NatureRecharge | Advantages::GroupMagicBonus | Advantages::Sapient),		 // Hyu-Mann
	(Advantages::Flight | Advantages::NatureRecharge | Advantages::Infravision | Advantages::FastHpRegen | Advantages::GroupMagicBonus | Advantages::RuneAware | Advantages::Sapient),				// Elf
	(Advantages::Flight | Advantages::NatureRecharge | Advantages::Infravision | Advantages::FastManaRegen | Advantages::GroupMagicBonus | Advantages::RuneAware | Advantages::Sapient),			// Dwomax
	(Advantages::Flight | Advantages::NatureRecharge | Advantages::FastManaRegen | Advantages::GroupMagicBonus | Advantages::RuneAware | Advantages::Sapient),								// Shastick
	(Advantages::Flight | Advantages::NatureRecharge | Advantages::Infravision | Advantages::RuneAware | Advantages::FastManaRegen | Advantages::Ambidextrous | Advantages::SlowMetabolism | Advantages::GroupMagicBonus | Advantages::Sapient),											// Murrash
	(Advantages::NatureRecharge | Advantages::GroupMagicBonus | Advantages::RuneAware),	 // Laori
	(Advantages::Flight | Advantages::NatureRecharge | Advantages::Ambidextrous | Advantages::GroupMagicBonus),		// Thadyri
	(Advantages::Flight | Advantages::NatureRecharge | Advantages::GroupMagicBonus | Advantages::RuneAware | Advantages::Sapient),			// Terran
	(Advantages::Flight | Advantages::NatureRecharge | Advantages::GroupMagicBonus | Advantages::RuneAware | Advantages::Sapient),			// Trulor
	(Advantages::Flight | Advantages::NatureRecharge | Advantages::Infravision),		 // Dragon
	(Advantages::Flight | Advantages::FastManaRegen),			 // Deva
	(Advantages::Flight | Advantages::FastManaRegen),				// Solar
	0,																				// Animal
	0,																				// Animal
	0,																				// Animal
	0,																				// Animal
	0,																				// Animal
	0,																				// Animal
	0,																				// Animal
	0,																				// Animal
	0,																				// Animal
	0																				// Animal
};


// Array to filter racial disadvantages
const long race_disadvantage_filter[NUM_RACES] =
{
	(Disadvantages::NecroDestroysFlesh),			// Animal
	(Disadvantages::NecroDestroysFlesh | Disadvantages::ColdBlooded),			// Hyu-Mann
	(Disadvantages::NecroDestroysFlesh | Disadvantages::NoMagic | Disadvantages::ColdBlooded),			// Elf
	(Disadvantages::NecroDestroysFlesh | Disadvantages::ColdBlooded),			// Dwomax
	(Disadvantages::NecroDestroysFlesh | Disadvantages::ColdBlooded),			// Shastick
	(Disadvantages::NecroDestroysFlesh | Disadvantages::ColdBlooded),			// Murrash
	(Disadvantages::NecroDestroysFlesh | Disadvantages::Atheist | Disadvantages::NoMagic),			// Laori
	(Disadvantages::NecroDestroysFlesh | Disadvantages::NoMagic | Disadvantages::ColdBlooded),			// Thadyri
	(Disadvantages::NecroDestroysFlesh | Disadvantages::NoMagic | Disadvantages::ColdBlooded),			// Terran
	(Disadvantages::NecroDestroysFlesh | Disadvantages::NoMagic | Disadvantages::ColdBlooded),			// Trulor
	(Disadvantages::NecroDestroysFlesh | Disadvantages::ColdBlooded),			// Dragon
	(Disadvantages::NecroDestroysFlesh | Disadvantages::Atheist | Disadvantages::ColdBlooded),			// Deva
	(Disadvantages::NecroDestroysFlesh | Disadvantages::Atheist | Disadvantages::ColdBlooded),			// Solar
	(Disadvantages::NecroDestroysFlesh),			// Animal
	(Disadvantages::NecroDestroysFlesh),			// Animal
	(Disadvantages::NecroDestroysFlesh),			// Animal
	(Disadvantages::NecroDestroysFlesh),			// Animal
	(Disadvantages::NecroDestroysFlesh),			// Animal
	(Disadvantages::NecroDestroysFlesh),			// Animal
	(Disadvantages::NecroDestroysFlesh),			// Animal
	(Disadvantages::NecroDestroysFlesh),			// Animal
	(Disadvantages::NecroDestroysFlesh),			// Animal
	(Disadvantages::NecroDestroysFlesh)			// Animal
};


// Valid classes for PC creation
int raceTclass[NUM_RACES][NUM_CLASSES] = {
 /*                 x, M, C, T, W, R, A, L, V, P, U   */
    /* Animal  */ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    /* Human   */ { 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0 },
    /* Elf     */ { 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0 },
    /* Dwomax  */ { 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0 },
    /* Shastick*/ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    /* Murrash */ { 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0 },
    /* Laori   */ { 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0 },
    /* Thadyri */ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    /* Terran  */ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    /* Trulor  */ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    /* Dragon  */ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    /* Deva    */ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    /* Solar   */ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    /* Animal  */ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    /* Animal  */ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    /* Animal  */ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    /* Animal  */ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    /* Animal  */ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    /* Animal  */ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    /* Animal  */ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    /* Animal  */ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    /* Animal  */ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    /* Animal  */ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};


int parse_race_menu(char arg)
{
	arg = LOWER(arg);

	switch (arg) {
	case 'a':
		return RACE_HYUMANN;
		break;
	case 'b':
		return RACE_ELF;
		break;
	case 'c':
		return RACE_DWOMAX;
		break;
//	case 'd':
//		return RACE_SHASTICK;
//		break;
	case 'e':
		return RACE_MURRASH;
		break;
	case 'f':
		return RACE_LAORI;
		break;
//	case 'g':
//		return RACE_THADYRI;
//		break;
//	case 'h':
//		return RACE_TERRAN;
//		break;
//	case 'i':
//		return RACE_TRULOR;
//		break;
//	case 'j':
//		return RACE_DRAGON;
//		break;
//	case 'k':
//		return RACE_DEVA;
//		break;
//	case 'l':
//		return RACE_SOLAR;
//		break;
	default:
		return RACE_HYUMANN;
		break;
	}
}

// int totalpts(Character *ch)
// {
//	 int iffy	= ((ch)->aff_abils.con);
//			 iffy += ((ch)->aff_abils.wis);
//			 iffy += ((ch)->aff_abils.dex);
//			 iffy += ((ch)->aff_abils.intel);
//			 iffy += ((ch)->aff_abils.str);
//			 iffy += ((ch)->aff_abils.cha);
//	 return (iffy);
// }

int whichrating(int rating)
{
	if (rating <= -500)
		return 0;
	else if (rating < 500)
		return 1;
	return 2;
}

char *cg_info(Character *ch)
{
	const int averagepts = 60;
	const int maxpts = 120;
	int i;

	char *buf2 = get_buffer(MAX_STRING_LENGTH);

	((ch)->points.max_hit) = ((ch)->points.hit) = MAX((GET_CON(ch) * 10), 1);
	((ch)->points.max_mana) = ((ch)->points.mana) = MAX((GET_INT(ch) * 10), 1);
	((ch)->points.max_move) = ((ch)->points.move) = MAX((GET_STR(ch) * 10), 1);

	(ch)->real_abils = (ch)->aff_abils;
	GET_TOTALPTS(ch) = sum_char_pts(ch);

	for (i = 0; i < 32; ++i) {
		if (ADVANTAGED(ch, 1 << i)) {
			if (*buf2) {
				sprintf(buf2 + strlen(buf2), ", %s", named_advantages[i]);
			} else {
				strcpy(buf2, named_advantages[i]);
			}
		}
	}

	for (i = 0; i < 32; ++i) {
		if (DISADVANTAGED(ch, 1 << i)) {
			if (*buf2) {
				sprintf(buf2 + strlen(buf2), ", %s", named_disadvantages[i]);
			} else {
				strcpy(buf2, named_disadvantages[i]);
			}
		}
	}

	// strcpy(buf, "\033[H\033[J\r\n");
	strcpy(buf, "\r\n");

	sprintf(buf + strlen(buf),
		  "\r\n&cGD&c0) Description:  &cY%s&c0\r\n"
		  "   Keywords:  &cY%s&c0\r\n\n"
          "&cG1&c0) Strength       [&cC%3d&c0]    &cGR&c0) Race           [&cC%10s&c0]\r\n"
          "&cG2&c0) Intelligence   [&cC%3d&c0]    &cGS&c0) Sex            [&cC%10s&c0]\r\n"
          "&cG3&c0) Wisdom         [&cC%3d&c0]    &cGL&c0) Lawfulness     [&cC%10s&c0]\r\n"
          "&cG4&c0) Dexterity      [&cC%3d&c0]    &cGA&c0) Alignment      [&cC%10s&c0]\r\n"
          "&cG5&c0) Constitution   [&cC%3d&c0]    &cG+&c0) Advantages\r\n"
          "&cG6&c0) Charisma       [&cC%3d&c0]    &cG-&c0) Disadvantages\r\n\n",
//		  "&cGB&c0) Background:  &cC%s&c0\r\n\n",
		ch->SDesc("unset"),
		ch->Keywords("unset"),
		GET_STR(ch), race_types[GET_RACE(ch)],
		GET_INT(ch), genders[GET_SEX(ch)],
		GET_WIS(ch), lawfulness[whichrating(GET_LAWFULNESS(ch))],
		GET_DEX(ch), alignment[whichrating(GET_ALIGNMENT(ch))],
		GET_CON(ch),
		GET_CHA(ch)
//		class_types[GET_CLASS(ch)]
		);

	if (*buf2) {
		sprintf(buf + strlen(buf), "Attributes:  &cC%s&c0\r\n\n", buf2);
	}

	sprintf(buf + strlen(buf),
          "Hit points         &cY%4d&c0    Available points           &cY%d&c0\r\n"
          "Mana               &cY%4d&c0    Final rating               &cY%d&c0\r\n"
          "Movement points    &cY%4d&c0\r\n\n",
		GET_MAX_HIT(ch), MAX(averagepts - GET_TOTALPTS(ch), 0),
		GET_MAX_MANA(ch), GET_TOTALPTS(ch),
		GET_MAX_MOVE(ch));

	if (GET_TOTALPTS(ch) > maxpts) {
		strcat(buf, "&cY&brYou are above the maximum limit for the point rating.  Lower some stats!&c0&b0\r\n\n");
	} else if (!ch->Keywords()) {
		strcat(buf, "&cRYou must enter a valid short description for your character.&c0\r\n\n");
	} else {
		if (GET_TOTALPTS(ch) > averagepts) {
			sprintf(buf + strlen(buf),
				"&cRYou are &cY%d&cR points above average.\r\n"
        		"Your advancement through levels will be slower than usual.&c0\r\n\n",
				GET_TOTALPTS(ch) - 60);
		}
		// if (raceTclass[(int) GET_RACE(ch)][GET_CLASS(ch)])
		strcat(buf, "&cGQ&c0) Quit the generator, these stats are fine.\r\n\n");
	}

	strcat(buf, "Enter choice : ");

	release_buffer(buf2);
	return buf;
}


/*
	 JD Displays Classes multi-class per line
	 Change ST to show how many you want PER line!
 */

void display_classes(Character *ch)
{
	char *buf = get_buffer(MAX_STRING_LENGTH);

	int tmpclass, displayed = 0, perline = 2;

	strcpy(buf, "\r\n&ccSelect your background experience:&c0\r\n");

	for (tmpclass = 0; tmpclass < NUM_CLASSES; ++tmpclass) {
		if (raceTclass[(int) GET_RACE(ch)][tmpclass]) {
			++displayed;
			sprintf(buf + strlen(buf), "%c) %-15s%s",
							'a' + tmpclass, class_types[tmpclass],
							((displayed % perline) ? "		" : "\r\n"));
		}
	}

	strcat(buf, "\nClass: ");

	ch->Send(buf);
	release_buffer(buf);
}


/* Takes all the RACES and checks to see if they have class's assigned
	 to them VIA the raceTclass function.	If so, then it displays the race.
	 If not, then it does not display it.
	 Multi-Class per line added also!	Change st to how many you want per line!
 */

void display_races(Character * ch)
{
	int tmprace, tmpclass;
	bool allowed;
	char *buf = get_buffer(MAX_STRING_LENGTH);

	ch->Send("\r\n&ccSelect a Race:&c0\r\n");
	for (tmprace = 0; tmprace < NUM_RACES; ++tmprace) {
		for (allowed = FALSE, tmpclass = 0; tmpclass < NUM_CLASSES; ++tmpclass)
			if (raceTclass[tmprace][tmpclass])
	allowed = TRUE;

	if (allowed) {
		sprintf(buf, "%c) %-15s%5s\r\n", 'a' - 1 + tmprace, race_types[tmprace],
			((tmprace % 1) ? "\r\n" : "		"));
		ch->Send(buf);
		allowed = FALSE;
	}

	}
	ch->Send("\nRace: ");
	release_buffer(buf);
}


/* Str_test was used to find out where stats where being lost! */

// void str_tester(Character *ch, int num)
// {
//	 if (GET_STR(ch) < 1) {
//		 sprintf(buf, "%2d\r\n", num);
//		 ch->Send(buf);
//	 }
// }


/*
 * Roll the 6 stats for a character... each stat is made of the sum of
 * the best 3 out of 4 rolls of a 6-sided die.	Each class then decides
 * which priority will be given for the best to worst stats.
 */


void roll_real_abils(Character *ch)
{
	int i;
	// int j;
	int k, temp;
	UInt8 table[6];

	GET_RACE(ch) = RACE_HYUMANN;
	GET_SEX(ch) = Male;

	GET_STR(ch) = 8 + Number(0, 5);
	GET_INT(ch) = 8 + Number(0, 5);
	GET_WIS(ch) = 8 + Number(0, 5);
	GET_DEX(ch) = 8 + Number(0, 5);
	GET_CON(ch) = 8 + Number(0, 5);
	GET_CHA(ch) = 8 + Number(0, 5);

	GET_STR(ch) += mod_stat_table[GET_RACE(ch)][0];
	GET_INT(ch) += mod_stat_table[GET_RACE(ch)][1];
	GET_WIS(ch) += mod_stat_table[GET_RACE(ch)][2];
	GET_DEX(ch) += mod_stat_table[GET_RACE(ch)][3];
	GET_CON(ch) += mod_stat_table[GET_RACE(ch)][4];
	GET_CHA(ch) += mod_stat_table[GET_RACE(ch)][5];

	GET_STR(ch) = MIN(MAX(GET_STR(ch), 3), max_stat_table[GET_RACE(ch)][0]);
	GET_INT(ch) = MIN(MAX(GET_INT(ch), 3), max_stat_table[GET_RACE(ch)][1]);
	GET_WIS(ch) = MIN(MAX(GET_WIS(ch), 3), max_stat_table[GET_RACE(ch)][2]);
	GET_DEX(ch) = MIN(MAX(GET_DEX(ch), 3), max_stat_table[GET_RACE(ch)][3]);
	GET_CON(ch) = MIN(MAX(GET_CON(ch), 3), max_stat_table[GET_RACE(ch)][4]);
	GET_CHA(ch) = MIN(MAX(GET_CHA(ch), 3), max_stat_table[GET_RACE(ch)][5]);
}


void cg_edit_advantage(Character *ch, char *argument)
{
	Flags mask = race_advantages[GET_RACE(ch)];
	Flags flags = ADV_FLAGS(ch) | race_advantages[GET_RACE(ch)];
	int i, j;

	mask |= race_advantage_filter[GET_RACE(ch)];

	for (i = 0; i < 32; ++i) {
		if (ADVANTAGED(ch, 1 << i)) {
			mask |= advantage_data[i].advantage_filter;
		}
	}

	for (i = 0; i < 32; ++i) {
		if (DISADVANTAGED(ch, 1 << i)) {
			mask |= disadvantage_data[i].advantage_filter;
		}
	}

	olc_edit_bitvector(ch->desc, &flags, argument, flags, "advantages", named_advantages, mask);

	for (i = 0; i < 32; ++i) {
		if (ADVANTAGED(ch, 1 << i)) {
			ADV_FLAGS(ch) ^= advantage_data[i].advantage_filter;
		}
	}

	ADV_FLAGS(ch) = flags;

	if (GET_RACE(ch) == RACE_LAORI) {
		if (ADVANTAGED(ch, Advantages::Flight))
			GET_BODYTYPE(ch) = BODYTYPE_AVIAN;
		else
			GET_BODYTYPE(ch) = BODYTYPE_HUMANOID;
	}

	for (i = 0; i < 32; ++i) {
		if (ADVANTAGED(ch, 1 << i)) {
			for (j = 0; j < 32; ++j) {
				if (IS_SET(advantage_data[i].advantage_filter, 1 << j) && (ADVANTAGED(ch, 1 << j))) {
					ADV_FLAGS(ch) ^= 1 << j;
				}
			}
		}
	}

}


void cg_edit_disadvantage(Character *ch, char *argument)
{
	Flags mask = race_disadvantages[GET_RACE(ch)];
	Flags flags = DISADV_FLAGS(ch) | race_disadvantages[GET_RACE(ch)];
	int i, j;

	mask |= race_disadvantage_filter[GET_RACE(ch)];

	for (i = 0; i < 32; ++i) {
		if (ADVANTAGED(ch, 1 << i)) {
			mask |= advantage_data[i].disadvantage_filter;
		}
	}

	for (i = 0; i < 32; ++i) {
		if (DISADVANTAGED(ch, 1 << i)) {
			mask |= disadvantage_data[i].disadvantage_filter;
		}
	}

	olc_edit_bitvector(ch->desc, &flags, argument, flags, "disadvantages", named_disadvantages, mask);

	DISADV_FLAGS(ch) = flags;

	for (i = 0; i < 32; ++i) {
		if (DISADVANTAGED(ch, 1 << i)) {
			for (j = 0; j < 32; ++j) {
				if (IS_SET(disadvantage_data[i].disadvantage_filter, 1 << j) && (DISADVANTAGED(ch, 1 << j))) {
					DISADV_FLAGS(ch) ^= 1 << j;
				}
			}
		}
	}
}


/*
 * Gives the new player coins and equipment based on class
 * Eventually, race and hometown should be a factor too.
 */
// Updated to use an ASCII file by Daniel Rollings AKA Daniel Houghton.

void equip_newbie(Character *ch)
{
	FILE *newbie_eq = NULL;
	char line[256];

	SInt32 i, objnum = 0, t[10];
	UInt32 linenum = 0;
	Object *obj;

	if (!newb_eq)
		return;

	sprintf(line, "%s.%s.%s", NEWBIE_EQ_FILE,
			race_types[GET_RACE(ch)],
			class_types[GET_CLASS(ch)]);

	/* give the guy a few coins to start */
	GET_GOLD(ch) = 950 + dice(10, 10);

	if (!(newbie_eq = fopen(NEWBIE_EQ_FILE, "r"))) {
		log("Error opening newbie_eq file!");
		return;
	}

	// Daniel Rollings AKA Daniel Houghton's patch - allow race and class-specific starting equipment
	// by starting with "newbie_eq.race.class", and whittling it down from there as
	// files may not be found.
	while ((*line) && (!newbie_eq)) {
		newbie_eq = fopen(line, "r");
		if (!newbie_eq) {
			if (!strip_last_char(line, '.'))
				*line = '\0';
		}
	}

	if (newbie_eq == NULL) {
		log("Error opening newbie_eq file!");
		return;
	}

	*line = '\0';

	while (*line != '$') {
		get_line(newbie_eq, line);
		++linenum;
		i = sscanf(line, "%d", t);
		if (*line != '$') {
			if (i != 1) {
				sprintf(buf2, "Error while reading line %d of newbie_eq.", linenum);
				log(buf2);
				break;
			}

			objnum = t[0];

			if ((obj = Object::Read(objnum))) {
				obj->ToChar(ch);
				load_otrigger(obj);
			}

		}
	}

	fclose(newbie_eq);
}


/* Some initializations for characters, including initial skills */
void do_start(Character * ch)
{
//	void roll_real_abils(Character * ch);

	GET_LEVEL(ch) = 1;
	GET_MAX_LEVEL(ch) = 1;
	GET_EXP(ch) = 1;

	ch->SetTitle();
//	roll_real_abils(ch);

	GET_WIMP_LEV(ch) = 7;

	GET_MAX_HIT(ch) = (GET_CON(ch) * 10) + Number(0, 9);
	GET_MAX_MANA(ch) = (GET_INT(ch) * 10) + Number(0, 9);
	GET_MAX_MOVE(ch) = (GET_STR(ch) * 5) + (GET_DEX(ch) * 5) + Number(0, 9);

	GET_HIT(ch) = GET_MAX_HIT(ch);
	GET_MANA(ch) = GET_MAX_MANA(ch);
	GET_MOVE(ch) = GET_MAX_MOVE(ch);

	GET_COND(ch, THIRST) = 24;
	GET_COND(ch, FULL) = 24;
	GET_COND(ch, DRUNK) = 0;

	SET_BIT(PRF_FLAGS(ch), Preference::AutoExit | Preference::Merciful);

	ch->UpdatePos();

	equip_newbie(ch);

	if (GET_TOTALPTS(ch) <= 60)
		GET_PRACTICES(ch) = ((60 - GET_TOTALPTS(ch)));

	GET_PRACTICES(ch) += 5;
	GET_TOTALPTS(ch) += 5;

//
// 	GET_PRACTICES(ch) = 2;
//
// 	switch (GET_CLASS(ch)) {
// 	case CLASS_MAGICIAN:		CHAR_SKILL(ch).LearnSkill(SPHERE_VIM, NULL, 3); break;
// 	// case CLASS_CLERIC:			CHAR_SKILL(ch).LearnSkill(SPHERE_VIM, 5); break;
// 	case CLASS_THIEF:			CHAR_SKILL(ch).LearnSkill(SKILL_HIDE, NULL, 3); break;
// 	case CLASS_WARRIOR:			CHAR_SKILL(ch).LearnSkill(SKILL_COMBAT_TECHNIQUE, NULL, 3); break;
// 	case CLASS_RANGER:			CHAR_SKILL(ch).LearnSkill(SKILL_TRACKING, NULL, 3); break;
// 	case CLASS_ASSASSIN:		CHAR_SKILL(ch).LearnSkill(SKILL_HIDE, NULL, 3); break;
// 	case CLASS_ALCHEMIST:		CHAR_SKILL(ch).LearnSkill(SKILL_ALCHEMY, NULL, 3); break;
// 	default:					GET_PRACTICES(ch) = 5; break;
// 	}
}


inline Color	Character::DefColor(void) const
{	if (desc)		return desc->default_color;
	return COL_OFF;		}

inline void		Character::SetDefColor(Color num)
{	if (!desc)		return;
	desc->default_color = num;	}



extern void fprintstring(FILE *fp, const char *fmt, const char *txt);
extern void fprintbits(FILE *fp, const char *fmt, UInt32 bits, const char *bitnames[]);

void Character::SaveDisk(Character *mob, FILE *fp)
{
	int num = mob->Virtual();
	SkillKnown *tskill = NULL;
	char *ptr = NULL;

	fprintf(fp, "#%d = {\n", mob->Virtual());
	fprintstring(fp, "\tkeyw = %s;\n", mob->Keywords("undefined"));
	fprintstring(fp, "\tname = %s;\n", mob->Name("undefined"));
	fprintstring(fp, "\tshort = %s;\n", mob->SDesc("undefined"));
	fprintstring(fp, "\tlong = %s;\n", mob->LDesc("undefined"));

	fprintf(fp, "\tsex = %s;\n"
				"\tmort = %s;\n"
				"\trace = %s;\n"
				"\tclass = %s;\n",
				genders[GET_SEX(mob)],
				mortality_types[GET_MORTALITY(mob)],
				race_types[GET_RACE(mob)],
				class_types[GET_CLASS(mob)]);

	if (!IS_SIMPLE_MOB(mob)) {
		fprintf(fp, "\textended = %d;\n", MOB_TYPE(mob));
	} else {
		fprintf(fp, "\tpts = %d;\n", GET_TOTALPTS(mob));
	}

	if (GET_LEVEL(mob))		fprintf(fp, "\tlevel = %d;\n", GET_LEVEL(mob));
	if (GET_ALIGNMENT(mob))	fprintf(fp, "\talign = %d;\n", GET_ALIGNMENT(mob));
	if (GET_GOLD(mob))		fprintf(fp, "\tgold = %d;\n", GET_GOLD(mob));
	if (GET_EXP_MOD(mob))	fprintf(fp, "\texpmod = %d;\n", GET_EXP_MOD(mob));
	if (GET_BODYTYPE(mob))	fprintf(fp, "\tbodytype = %s;\n", bodytypes[GET_BODYTYPE(mob)]);
	if (GET_MATERIAL(mob))	fprintf(fp, "\tmaterial = \"%s\";\n", material_types[GET_MATERIAL(mob)]);
	if (GET_DR(mob))		fprintf(fp, "\tdr = %d;\n", GET_DR(mob));

	if (GET_DISPOSITION(mob))	fprintf(fp, "\tdisposition = %d;\n", GET_DISPOSITION(mob));
	if (GET_MAX_RIDERS(mob))	fprintf(fp, "\tmaxriders = %d;\n", GET_MAX_RIDERS(mob));

	fprintf(fp, "\thp = %dd%d+%d;\n"
				"\tdamage = %dd%d+%d;\n"
				"\tattack = %s;\n",
				mob->mob->numhitdice, mob->mob->sizehitdice, mob->mob->modhitdice,
				GET_NDD(mob), GET_SDD(mob), GET_DAMROLL(mob),
//				Damage::types[GET_ATTACK(mob)]);
				Skill::Name(GET_ATTACK(mob) + TYPE_HIT));
	if (GET_HITROLL(mob))	fprintf(fp, "\thitroll = %d;\n", GET_HITROLL(mob));
	fprintf(fp, "\tposition = %s;\n"
				"\tdefposition = %s;\n",
				positions[GET_POS(mob)],
				positions[GET_DEFAULT_POS(mob)]);
	if (MOB_FLAGS(mob))	fprintbits(fp, "\tflags = { %s };\n", MOB_FLAGS(mob), action_bits);
	if (AFF_FLAGS(mob))	fprintbits(fp, "\taffects = { %s };\n", AFF_FLAGS(mob), affected_bits);

	if (GET_STR(mob) != 10 || GET_INT(mob) != 10 || GET_WIS(mob) != 10 ||
		GET_DEX(mob) != 10 || GET_CON(mob) != 10 || GET_CHA(mob) != 10) {
		fprintf(fp, "\tpoints = {\n");

		if (GET_STR(mob) != 10)	fprintf(fp, "\t\tstr = %d;\n", GET_STR(mob));
		if (GET_INT(mob) != 10)	fprintf(fp, "\t\tint = %d;\n", GET_INT(mob));
		if (GET_WIS(mob) != 10)	fprintf(fp, "\t\twis = %d;\n", GET_WIS(mob));
		if (GET_DEX(mob) != 10)	fprintf(fp, "\t\tdex = %d;\n", GET_DEX(mob));
		if (GET_CON(mob) != 10)	fprintf(fp, "\t\tcon = %d;\n", GET_CON(mob));
		if (GET_CHA(mob) != 10)	fprintf(fp, "\t\tcha = %d;\n", GET_CHA(mob));

		fprintf(fp, "\t};\n");
	}

	if (Clan::Find(GET_CLAN(mob)))	fprintf(fp, "\tclan = %d;\n", GET_CLAN(mob));

	if (mob->mob->hates || mob->mob->fears) {
		fprintf(fp, "\topinions = {\n");

		if (mob->mob->hates) {
			fprintf(fp, "\t\thates = {\n");
			if (mob->mob->hates->sex)	fprintbits(fp, "\t\t\tsexes = { %s };\n", mob->mob->hates->sex, genders);
			if (mob->mob->hates->race)	fprintbits(fp, "\t\t\traces = { %s };\n", mob->mob->hates->race, race_types);
			if (mob->mob->hates->vnum != -1)	fprintf(fp, "\t\t\tvnum = %d;\n", mob->mob->hates->vnum);
			fprintf(fp, "\t\t};\n");
		}
		if (mob->mob->fears) {
			fprintf(fp, "\t\tfears = {\n");
			if (mob->mob->fears->sex)	fprintbits(fp, "\t\t\tsexes = { %s };\n", mob->mob->fears->sex, genders);
			if (mob->mob->fears->race)	fprintbits(fp, "\t\t\traces = { %s };\n", mob->mob->fears->race, race_types);
			if (mob->mob->fears->vnum != -1)	fprintf(fp, "\t\t\tvnum = %d;\n", mob->mob->fears->vnum);
			fprintf(fp, "\t\t};\n");
		}

		fprintf(fp, "\t};\n");
	}

	if (Character::Index[num].triggers.Count()) {
		fprintf(fp, "\ttriggers = { ");
		for (int i = 0; i < Character::Index[num].triggers.Count(); ++i)
			fprintf(fp, "%d ", Character::Index[num].triggers[i]);
		fprintf(fp, "};\n");
	}

    // Save skills
    mob->general.skills.GetTable(&tskill);

    if (tskill) {
		fprintf(fp, "\tskills = {\n");
		do {
			strcpy(buf1, Skill::Name(tskill->skillnum));

			// Replace all underscores with spaces
			while ((ptr = strchr(buf1, ' ')) != NULL)
				*ptr = '_';

			fprintf(fp, "\t\t%s = %d;\n", buf1, tskill->proficient);
			tskill = tskill->next;
		} while (tskill);
		fprintf(fp, "\t};\n");
    }

	if (Character::Index[num].func && Character::Index[num].func != shop_keeper) {
		fprintf(fp, "\tspecproc = %s \"%s\";\n",
			spec_mob_name(Character::Index[num].func),
			Character::Index[num].farg);
	}

	fprintf(fp, "};\n");
}


typedef struct {
	UInt8 strength;
	UInt8 intelligence;
	UInt8 wisdom;
	UInt8 dexterity;
	UInt8 constitution;
	UInt8 charisma;
	UInt8 maxhp;
	UInt8 hitroll;
	UInt8 damroll;
	UInt8 damresist;
} stat_priorities;


// Lists of the priorities of stats for the classes
const stat_priorities mob_stat_list[] = {
//	Str Int Wis Dex Con Cha HP	Hit Dam DR
	{ 11, 25, 18, 19, 12, 15,  0,  0,  0,  0 },  // Standard Magician, 0
	{ 16, 11, 25, 15, 15, 18,  0,  0,  0,  0 },  // Standard Cleric, 1
	{ 16, 18, 11, 25, 15, 15,  0,  0,  0,  0 },  // Standard Thief, 2
	{ 24, 11, 10, 23, 18, 14,  0,  0,  0,  0 },  // Standard Warrior, 3
	{ 18, 15, 16, 22, 16, 15,  0,  0,  0,  0 },  // Standard Ranger, 4
	{ 17, 17, 13, 22, 16, 15,  0,  0,  0,  0 },  // Standard Assassin, 5
	{ 13, 22, 21, 16, 13, 15,  0,  0,  0,  0 },  // Standard Alchemist, 6
	{ 17, 15, 14, 21, 16, 17,  0,  0,  0,  0 },  // Standard Vampire, 7
	{ 14, 20, 22, 14, 14, 16,  0,  0,  0,  0 },  // Standard Psi, 8
	{ 15,  5,  3, 12, 18, 12, 18,  8,  9,  0 },  // Standard Beast, 9
	{ 25, 13, 13, 15, 18, 15, 20,  5,  8, 11 },  // Standard Dragon, 10

	{ 25,  8, 10, 22, 16,  8,  5,  5,  7,  5 },  // Murrash Warrior, 11
	{ 18, 16, 18, 18, 16,  8,  3,  3,  5,  4 }   // Murrash Cleric, 12
};


// Sets stats according to a simple rating.
void render_simple_mob(Character *ch)
{
	if (!IS_SIMPLE_MOB(ch))
		return;

	const stat_priorities *stat_table;
	stat_priorities custom_stat_table;
	int i, 			// iterator
			pts,			// Temporary for points for a single stat
			stat,			// Temporary for points for a single stat
			stat_pts = 0, 		// Number of points to put in stats
			remainder_pts = 0, 	// Number of points to put in levels
			totalpts, 		// Total after removal of XP Bonus
			expbonus;			// Experience bonus

	extern int stat_ptlevel(int cost);
	extern int generic_ptlevel(int cost, SInt16 start, SInt16 mod);
	extern const int mod_stat_table[NUM_RACES][6];

	if (GET_TOTALPTS(ch) <= -360) {
		ch->real_abils.Str = 3;
		ch->real_abils.Int = 3;
		ch->real_abils.Wis = 3;
		ch->real_abils.Dex = 3;
		ch->real_abils.Con = 3;
		ch->real_abils.Cha = 3;
		GET_MAX_HIT(ch) = 30;

		ch->aff_abils = ch->real_abils;
		return;
	}

	switch (GET_RACE(ch)) {
	case RACE_ANIMAL: stat_table = &mob_stat_list[9]; break;
	case RACE_DRAGON: stat_table = &mob_stat_list[10]; break;
	case RACE_MURRASH:
		switch (GET_CLASS(ch)) {
		case CLASS_CLERIC: stat_table = &mob_stat_list[12]; break;
		default: stat_table = &mob_stat_list[11]; break;
		}
		break;

	default:
		switch (GET_CLASS(ch)) {
		case CLASS_MAGICIAN: stat_table = &mob_stat_list[0]; break;
		case CLASS_CLERIC: stat_table = &mob_stat_list[1]; break;
		case CLASS_THIEF: stat_table = &mob_stat_list[2]; break;
		case CLASS_WARRIOR: stat_table = &mob_stat_list[3]; break;
		case CLASS_RANGER: stat_table = &mob_stat_list[4]; break;
		case CLASS_ASSASSIN: stat_table = &mob_stat_list[5]; break;
		case CLASS_ALCHEMIST: stat_table = &mob_stat_list[6]; break;
		case CLASS_VAMPIRE: stat_table = &mob_stat_list[7]; break;
		case CLASS_PSIONICIST: stat_table = &mob_stat_list[8]; break;
		default:
		stat_table = &mob_stat_list[9]; break;
		}
		break;

	}

	expbonus = GET_EXP_MOD(ch);
	GET_EXP_MOD(ch) = 0;
	totalpts = GET_TOTALPTS(ch) - expbonus;

	GET_LEVEL(ch) = 0;
	CHAR_SKILL(ch).ClearSkills();

	remainder_pts = totalpts;

	if (remainder_pts > 60) {
		stat_pts = 60;
		stat_pts += (int) ((remainder_pts - 60) / 2);
	} else {
		// It appears that we are allocating everything
		// into stats, but it won't happen as such.
		// There will be a lot in the remainders.
		stat_pts = remainder_pts;
	}

	remainder_pts -= stat_pts;

	stat_pts += 360;

	pts = ((stat_table->strength * stat_pts) / 100) - 60;
	pts = (int) (((pts / 5.0) + 0.50) * 5);
	stat = stat_ptlevel(pts);
	ch->real_abils.Str = MAX(MIN(stat, 99), 1);

	pts = ((stat_table->intelligence * stat_pts) / 100) - 60;
	pts = (int) (((pts / 5.0) + 0.50) * 5);
	stat = stat_ptlevel(pts);
	ch->real_abils.Int = MAX(MIN(stat, 99), 1);

	pts = ((stat_table->wisdom * stat_pts) / 100) - 60;
	pts = (int) (((pts / 5.0) + 0.50) * 5);
	stat = stat_ptlevel(pts);
	ch->real_abils.Wis = MAX(MIN(stat, 99), 1);

	pts = ((stat_table->dexterity * stat_pts) / 100) - 60;
	pts = (int) (((pts / 5.0) + 0.50) * 5);
	stat = stat_ptlevel(pts);
	ch->real_abils.Dex = MAX(MIN(stat, 99), 1);

	pts = ((stat_table->constitution * stat_pts) / 100) - 60;
	pts = (int) (((pts / 5.0) + 0.50) * 5);
	stat = stat_ptlevel(pts);
	ch->real_abils.Con = MAX(MIN(stat, 99), 1);

	pts = ((stat_table->charisma * stat_pts) / 100) - 60;
	pts = (int) (((pts / 5.0) + 0.50) * 5);
	stat = stat_ptlevel(pts);
	ch->real_abils.Cha = MAX(MIN(stat, 99), 1);

	stat_pts -= 360;

	GET_DR(ch) = generic_ptlevel(((stat_pts * stat_table->damresist) / 100), 10, 10);
	GET_HITROLL(ch) = generic_ptlevel(((stat_pts * stat_table->hitroll) / 100), 5, 5);
	GET_DAMROLL(ch) = generic_ptlevel(((stat_pts * stat_table->damroll) / 100), 5, 5);

	ch->mob->numhitdice = 1;
	ch->mob->sizehitdice = 9;
	ch->mob->modhitdice = ch->real_abils.Con * 10;
	ch->mob->modhitdice += ((stat_table->maxhp * stat_pts) / 100) / 10;

	if (GET_RACE(ch) != RACE_ANIMAL) {
		ch->real_abils.Str = MAX(1, ch->real_abils.Str + mod_stat_table[GET_RACE(ch)][0]);
		ch->real_abils.Int = MAX(1, ch->real_abils.Int + mod_stat_table[GET_RACE(ch)][1]);
		ch->real_abils.Wis = MAX(1, ch->real_abils.Wis + mod_stat_table[GET_RACE(ch)][2]);
		ch->real_abils.Dex = MAX(1, ch->real_abils.Dex + mod_stat_table[GET_RACE(ch)][3]);
		ch->real_abils.Con = MAX(1, ch->real_abils.Con + mod_stat_table[GET_RACE(ch)][4]);
		ch->real_abils.Cha = MAX(1, ch->real_abils.Cha + mod_stat_table[GET_RACE(ch)][5]);
	}

	ch->aff_abils = ch->real_abils;

	GET_HIT(ch) = GET_MAX_HIT(ch) = GET_CON(ch) * 10;
	GET_MANA(ch) = GET_MAX_MANA(ch) = GET_INT(ch) * 10;
	GET_MOVE(ch) = GET_MAX_MOVE(ch) = GET_STR(ch) * 10;

	pts = sum_char_pts(ch);
	remainder_pts = totalpts - MAX(pts, 0);
	// remainder_pts = (totalpts - pts + remainder_pts) / 2;
	// remainder_pts = MAX(remainder_pts, 0);

	GET_LEVEL(ch) = MAX(MIN(remainder_pts / 5, MAX_LEVEL) + 1, 1);
	GET_EXP_MOD(ch) = expbonus;

	// GET_LEVEL(ch) = MIN(GET_LEVEL(ch) + (remainder_pts / 5), MAX_LEVEL);

}


void MakeSpirit(Character *ch, int clss)
{
	GET_CLASS(ch) = (Class) clss;

	switch (clss) {
	case CLASS_GHOST:
	case CLASS_POLTERGEIST:
		GET_MATERIAL(ch) = Materials::Ether;
		break;
	case CLASS_WRAITH:
	case CLASS_SPECTRE:
		GET_MATERIAL(ch) = Materials::Shadow;
		break;
	}

	GET_COND(ch, THIRST) = -1;
	GET_COND(ch, FULL) = -1;
	GET_COND(ch, DRUNK) = -1;

	GET_HIT(ch) = GET_MAX_HIT(ch);
	GET_MANA(ch) = 0;
	GET_MOVE(ch) = GET_MAX_MOVE(ch);

	ch->UpdatePos();
	ENERGY(ch).Clear();
	CheckRegenRates(ch);
}


ActionEvent::ActionEvent(time_t when, Character &tch) : Event(when), ch(&tch) {	tch.action = this;	}
ActionEvent::~ActionEvent(void)	{}
	

time_t	ActionEvent::Run(void) 
{
	time_t	result;
	
	running = true;

	GET_ACTION(ch) = NULL;

	result = Execute();
	running = false;

	if (result)		GET_ACTION(ch) = this;

	return result;
}


void	ActionEvent::Cancel(void)
{
	GET_ACTION(ch) = NULL;
	if (!running) {
		if (queue)	EventQueue->Dequeue(queue);
		queue = NULL;
		delete this;
	}
}
