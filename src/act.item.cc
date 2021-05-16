/* ************************************************************************
*   File: act.item.c                                    Part of CircleMUD *
*  Usage: object handling routines -- get/drop and container handling     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */


#include "characters.h"
#include "rooms.h"
#include "objects.h"
#include "descriptors.h"
#include "utils.h"
#include "buffer.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "find.h"
#include "db.h"
#include "event.h"
#include "boards.h"
#include "affects.h"
#include "constants.h"

/* extern variables */
extern int drink_aff[][3];
extern char *drinknames[];
extern VNum donation_room_1;
#if 0
extern VNum donation_room_2;  /* uncomment if needed! */
extern VNum donation_room_3;  /* uncomment if needed! */
#endif

int drop_otrigger(Object *obj, Character *actor);
int drop_wtrigger(Object *obj, Character *actor);
int get_otrigger(Object *obj, Character *actor);
int give_otrigger(Object *obj, Character *actor, Character *victim);
int receive_mtrigger(Character *ch, Character *actor, Object *obj);
int wear_otrigger(Object *obj, Character *actor, int where);
int consume_otrigger(Object *obj, Character *actor);
int remove_otrigger(Object *obj, Character *actor);
void load_otrigger(Object *obj);
int wear_message_number (Object *obj, int where);


/* functions */
int can_take_obj(Character * ch, Object * obj);
void get_check_money(Character * ch, Object * obj);
int perform_get_FromRoom(Character * ch, Object * obj);
void get_FromRoom(Character * ch, char *arg);
Character *give_find_vict(Character * ch, char *arg);
void weight_change_object(Object * obj, int weight);
void name_from_drinkcon(Object * obj);
void name_to_drinkcon(Object * obj, int type);
void wear_message(Character * ch, Object * obj, int where);
void perform_wear(Character * ch, Object * obj, int where);
int find_eq_pos(Character * ch, Object * obj, char *arg, SInt8 bottomslot = 0, SInt8 topslot = NUM_WEARS);
void perform_remove(Character * ch, int pos);
void perform_put(Character * ch, Object * obj, Object * cont);
void perform_get_from_container(Character * ch, Object * obj, Object * cont, int mode);
int perform_drop(Character * ch, Object * obj, SInt8 mode, const char *sname, VNum RDR);
void perform_give(Character * ch, Character * vict, Object * obj);
void get_from_container(Character * ch, Object * cont, char *arg, int mode);
bool can_hold(Object *obj);
ACMD(do_put);
ACMD(do_get);
ACMD(do_drop);
ACMD(do_give);
ACMD(do_drink);
ACMD(do_eat);
ACMD(do_pour);
ACMD(do_wear);
ACMD(do_wield);
ACMD(do_grab);
ACMD(do_remove);
ACMD(do_pull);
ACMD(do_reload);


/* A quick routine to destroy items with the PROXIMITY flag - DH */
void kill_prox_item(Character * ch, Object * obj) {
	sprintf(buf, "$p crackles, hisses, and fades out of existence!");
	act(buf, FALSE, ch, obj, 0, TO_CHAR | TO_ROOM);
	obj->FromChar();
	obj->Extract();
}


/* Draw and sheath imported from Archipelago by Daniel Rollings AKA Daniel Houghton */
void	 perform_sheath(Character *ch, Object *obj,
		 Object *cont)
{
	if (cont->contents.Count()) {
		act("$P seems to have something in it already.",
				FALSE, ch, obj, cont, TO_CHAR);
		return;
	}

	if (OBJ_FLAGGED(obj, ITEM_NODROP)) {
		sprintf(buf, "For some reason, you can't let go of $p!");
		act(buf, FALSE, ch, obj, 0, TO_CHAR);
		return;
	}

	if (OBJ_FLAGGED(obj, ITEM_PROXIMITY)) {
		kill_prox_item(ch, obj);
		return;
	}

	act("You slip $p into $P.", FALSE, ch, obj, cont, TO_CHAR);
	act("$n slips $p into $P.", FALSE, ch, obj, cont, TO_ROOM);
	obj->FromChar();
	obj->ToObj(cont);
}


void	perform_unsheath(Character *ch, Object *weapon,
					 Object *scabbard)
{
	int where = -1;	// A negative value indicates no position found.

	weapon->FromObj();
	act ("You slide $p from $P.",FALSE,ch, weapon, scabbard, TO_CHAR);
	act ("$n slides $p from $P.",FALSE,ch, weapon, scabbard, TO_ROOM);
	weapon->ToChar(ch);

	if (CAN_WEAR(weapon, ITEM_WEAR_WIELD))
		if ((where = find_eq_pos(ch, weapon, 0)) >= 0)
			perform_wear(ch, weapon, where);
	else {
		ch->Send("You cannot wield it for some reason...");
	}
}


ACMD(do_sheath)
{
	Object *obj = 0, *tmp_obj = 0, *weapon = NULL;
	int i, pos = -1;
	SInt8 found_scabbard = FALSE;

	half_chop(argument, arg, buf);

	// First, let's locate the weapon.
	if (*arg) {
		if (!(weapon = get_object_in_equip_vis(ch, arg, ch->equipment, &pos))) {
			ch->Sendf("You don't seem to be wielding %s %s.\r\n", AN(arg), arg);
			return;
		}
	} else {
		for (i = WEAR_HAND_R; i < NUM_WEARS; ++i) {
			if ((GET_EQ(ch, i)) && (GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_WEAPON)) {
				pos = i;
				break;
			}
		}

		if (pos == -1) {
		ch->Send("You can't do that, you aren't wielding anything!\r\n");
		return;
	}

		weapon = GET_EQ(ch, pos);
	}

	if (GET_OBJ_TYPE(weapon) != ITEM_WEAPON) {
		ch->Send("But that's not a weapon you can sheath!\r\n");
		return;
	}

	// Now let's locate the scabbard.
	for (i = 0; i < NUM_WEARS; i++) {
		if (GET_EQ(ch, i) && (GET_EQ(ch, i))->CanBeSeen(ch) &&
			 (GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_WEAPONCON)) {
			if ((GET_OBJ_VAL(GET_EQ(ch, i), 3) == GET_OBJ_VAL(weapon, 0)) && !(GET_EQ(ch, i)->contents.Count())) {
				found_scabbard = TRUE;
				obj = GET_EQ(ch, i);
				break;
			}
		}
	}

	if (!found_scabbard) {
		START_ITER(iter, tmp_obj, Object *, ch->contents) {
			if (tmp_obj->CanBeSeen(ch) && (GET_OBJ_TYPE(tmp_obj) == ITEM_WEAPONCON)) {
				if ((GET_OBJ_VAL(tmp_obj, 3) == GET_OBJ_VAL(weapon, 0)) && !tmp_obj->contents.Count()) {
					found_scabbard = TRUE;
					obj = tmp_obj;
					break;
				}
			}
		}
	}

	if (pos == -1) {
		ch->Send("You screwed up!\r\n");
		return;
	}

	tmp_obj = unequip_char(ch, pos);
	tmp_obj->ToChar(ch);

	if (!found_scabbard) {
		act("You don't have a sheath for $p, so you remove it instead.", FALSE, ch, weapon, 0, TO_CHAR);
		return;
	}

//	 if (obj->action_description)
//		 act(ss_data(obj->action_description), FALSE, ch, weapon, obj, TO_ROOM);

	perform_sheath(ch,tmp_obj, obj);
}


ACMD(do_draw)
{
	Object *obj = 0, *tmp_obj = 0;
	int i;
	bool found = 0;
	ACMD(do_wield);
	LListIterator<Object *>	iter;

	one_argument(argument, arg);

	if (!*arg) {
		for (i = 0; i < NUM_WEARS; i++)
			if (GET_EQ(ch, i) && (GET_EQ(ch, i))->CanBeSeen(ch) &&
			   (GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_WEAPONCON))
				if ((GET_EQ(ch, i))->contents.Count()) {
					found = TRUE;
					obj = (GET_EQ(ch, i));
					break;
				}

		if (!found) {
			ch->Send("You don't seem to be wearing a scabbard with a weapon.\r\n");
			return;
		}

	} else {
		for (i = 0; i < NUM_WEARS; ++i)
			if ((GET_EQ(ch, i)) && GET_EQ(ch, i)->CanBeSeen(ch) &&
			   (GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_WEAPONCON))
				if (GET_EQ(ch, i)->contents.Count() && isname(arg, (GET_EQ(ch, i))->contents.Top()->Keywords())) {
					found = TRUE;
					obj = (GET_EQ(ch, i));
				}
		if (!found)
			START_ITER(iter, tmp_obj, Object *, ch->contents)
				if (CAN_SEE_OBJ(ch, tmp_obj) && (GET_OBJ_TYPE(tmp_obj) == ITEM_WEAPONCON))
					if (tmp_obj->contents.Count() && isname(arg, tmp_obj->contents.Top()->Keywords())) {
						found = TRUE;
						obj = tmp_obj;
						break;
					}

		if (!found) {
			do_wield(ch, "", 0, "wield", 0);
			return;
		}
	}

	if (!found) {
		ch->Send("You don't have a scabbard to draw from.\r\n");
		return;
	}
	//	if (obj->actionDesc)
	//	act(SSData(obj->actionDesc), FALSE, ch, obj, obj->contents.Top(), TO_ROOM);
	perform_unsheath(ch, obj->contents.Top(), obj);
	return;
}


void perform_put(Character * ch, Object * obj, Object * cont) {

	if (!drop_otrigger(obj, ch))
		return;

	if (PURGED(obj) || PURGED(ch) || (obj->CarriedBy() != ch))
		return;

	if (OBJ_FLAGGED(obj, ITEM_NODROP) && !IS_IMMORTAL(ch)) {
		sprintf(buf, "You can't let go of that, it must be CURSED!");
		act(buf, FALSE, ch, obj, 0, TO_CHAR);
		return;
	}

	if (OBJ_FLAGGED(obj, ITEM_PROXIMITY)) {
		kill_prox_item(ch, obj);
		return;
	}

	if ((GET_OBJ_WEIGHT(cont) + GET_OBJ_WEIGHT(obj)) > GET_OBJ_VAL(cont, 0))
		act("$p won't fit in $P.", FALSE, ch, obj, cont, TO_CHAR);
	else {
		obj->FromChar();
		obj->ToObj(cont);
		act("You put $p in $P.", FALSE, ch, obj, cont, TO_CHAR);
		act("$n puts $p in $P.", TRUE, ch, obj, cont, TO_ROOM);
	}
}


/* The following put modes are supported by the code below:

	1) put <object> <container>
	2) put all.<object> <container>
	3) put all <container>

	<container> must be in inventory or on ground.
	all objects to be put into container must be in inventory.
*/

ACMD(do_put) {
	// char *	arg1 = get_buffer(MAX_INPUT_LENGTH),
	// 	 *	arg2 = get_buffer(MAX_INPUT_LENGTH);
	char *	arg1 = buf1,
		 *	arg2 = buf2;
	Object *obj, *cont;
	Character *tmp_char;
	int obj_dotmode, cont_dotmode, found = 0;

	two_arguments(argument, arg1, arg2);
	obj_dotmode = find_all_dots(arg1);
	cont_dotmode = find_all_dots(arg2);

	if (!*arg1)
		ch->Send("Put what in what?\r\n");
	else if (cont_dotmode != FIND_INDIV)
		ch->Send("You can only put things into one container at a time.\r\n");
	else if (!*arg2) {
		ch->Sendf("What do you want to put %s in?\r\n",
			((obj_dotmode == FIND_INDIV) ? "it" : "them"));
	} else {
		generic_find(arg2, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP, ch, &tmp_char, &cont);
		if (!cont) {
			ch->Sendf("You don't see %s %s here.\r\n", AN(arg2), arg2);
			return;
		}
		switch (GET_OBJ_TYPE(cont)) {
		case ITEM_CONTAINER:
			if (OBJVAL_FLAGGED(cont, CONT_CLOSED)) {
				ch->Send("You'd better open it first!\r\n");
				return;
			}
			break;
		case ITEM_WEAPONCON:
			break;
		default:
			act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
			return;
		}
	}

	if (obj_dotmode == FIND_INDIV) {	/* put <obj> <container> */
		if (!(obj = get_obj_in_list_vis(ch, arg1, ch->contents))) {
			ch->Sendf("You aren't carrying %s %s.\r\n", AN(arg1), arg1);
		} else if (obj == cont)
			ch->Send("You attempt to fold it into itself, but fail.\r\n");
		else
			perform_put(ch, obj, cont);
	} else {
		START_ITER(iter, obj, Object *, ch->contents) {
			if ((obj != cont) && obj->CanBeSeen(ch) &&
			   (obj_dotmode == FIND_ALL || isname(arg1, obj->Keywords()))) {
				found = 1;
				perform_put(ch, obj, cont);
			}
		}
		if (!found) {
			if (obj_dotmode == FIND_ALL)
				ch->Send("You don't seem to have anything to put in it.\r\n");
			else {
				ch->Sendf("You don't seem to have any %ss.\r\n", arg1);
			}
		}
	}
}



int can_take_obj(Character * ch, Object * obj) {
	bool in_inventory = false;
	Object * tmp_obj;

	if (obj->InObj()) {
		tmp_obj = (Object *) obj->Inside();
		while (tmp_obj->InObj()) {
			tmp_obj = (Object *) tmp_obj->Inside();
		}
		if (tmp_obj->CarriedBy() || tmp_obj->WornBy())
			in_inventory = true;
	}

	if (ch->material == Materials::Ether && obj->material != Materials::Ether)
		act("$p slips through your grasp.", FALSE, ch, obj, 0, TO_CHAR);
	else if (!IS_STAFF(ch) && IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
		act("$p: you can't carry that many items.", FALSE, ch, obj, 0, TO_CHAR);
	else if ((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) > CAN_CARRY_W(ch) && !in_inventory && !(IS_IMMORTAL(ch)))
		act("$p: you can't carry that much weight.", FALSE, ch, obj, 0, TO_CHAR);
	else if (!(CAN_WEAR(obj, ITEM_WEAR_TAKE)) && (!(IS_IMMORTAL(ch)) || (GET_TRUST(ch) < TRUST_GRGOD)))
		act("$p: you can't take that!", FALSE, ch, obj, 0, TO_CHAR);
	else
		return 1;

	return 0;
}


void get_check_money(Character * ch, Object * obj)
{
	if ((GET_OBJ_TYPE(obj) == ITEM_MONEY) && (GET_OBJ_VAL(obj, 0) > 0)) {
		obj->FromChar();
		if (GET_OBJ_VAL(obj, 0) > 1) {
			sprintf(buf, "There were %d coins.\r\n", GET_OBJ_VAL(obj, 0));
			ch->Send(buf);
		}
		GET_GOLD(ch) += GET_OBJ_VAL(obj, 0);
		obj->Extract();
	}
}


void perform_get_from_container(Character * ch, Object * obj, Object * cont, int mode) {
	if (mode == FIND_OBJ_INV || can_take_obj(ch, obj)) {
		if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
			act("$p: you can't hold any more items.", FALSE, ch, obj, 0, TO_CHAR);
		else if (get_otrigger(obj, ch)) {
			if (PURGED(obj) || PURGED(ch) || (obj->InObj() && PURGED(obj->Inside())))
				return;
			obj->FromObj();
			obj->ToChar(ch);
			act("You get $p from $P.", FALSE, ch, obj, cont, TO_CHAR);
			act("$n gets $p from $P.", TRUE, ch, obj, cont, TO_ROOM);
			get_check_money(ch, obj);
			if ((GET_OBJ_TYPE(cont) == ITEM_CONTAINER) && cont->value[3] && !cont->contents.Count())
				cont->timer = 1;
		}
	}
}


void get_from_container(Character * ch, Object * cont, char *arg, int mode) {
	Object *obj;
	int obj_dotmode, found = 0;

	obj_dotmode = find_all_dots(arg);

	if (OBJVAL_FLAGGED(cont, CONT_CLOSED))
		act("$p is closed.", FALSE, ch, cont, 0, TO_CHAR);
	else if (obj_dotmode == FIND_INDIV) {
		if (!(obj = get_obj_in_list_vis(ch, arg, cont->contents))) {
			char *buf = get_buffer(128);
			sprintf(buf, "There doesn't seem to be %s %s in $p.", AN(arg), arg);
			act(buf, FALSE, ch, cont, 0, TO_CHAR);
			release_buffer(buf);
		} else
			perform_get_from_container(ch, obj, cont, mode);
	} else {
		if (obj_dotmode == FIND_ALLDOT && !*arg) {
			ch->Send("Get all of what?\r\n");
			return;
		}
		START_ITER(iter, obj, Object *, cont->contents) {
			if (obj->CanBeSeen(ch) &&
					(obj_dotmode == FIND_ALL || isname(arg, obj->Keywords()))) {
				found = 1;
				perform_get_from_container(ch, obj, cont, mode);
			}
		}
		if (!found) {
			if (obj_dotmode == FIND_ALL)
				act("$p seems to be empty.", FALSE, ch, cont, 0, TO_CHAR);
			else {
				char *buf = get_buffer(128);
				sprintf(buf, "You can't seem to find any %ss in $p.", arg);
				act(buf, FALSE, ch, cont, 0, TO_CHAR);
				release_buffer(buf);
			}
		}
	}
}


int perform_get_FromRoom(Character * ch, Object * obj) {
	if (can_take_obj(ch, obj) && get_otrigger(obj, ch)) {
		if (PURGED(obj) || PURGED(ch) || (IN_ROOM(obj) != IN_ROOM(ch)))
			return 0;
		obj->FromRoom();
		obj->ToChar(ch);
		act("You get $p.", FALSE, ch, obj, 0, TO_CHAR);
		act("$n gets $p.", TRUE, ch, obj, 0, TO_ROOM);
		get_check_money(ch, obj);
		REMOVE_BIT(GET_OBJ_EXTRA(obj), ITEM_HIDDEN);
		return 1;
	}
	return 0;
}


void get_FromRoom(Character * ch, char *arg) {
	Object *obj;
	int dotmode, found = 0;

	dotmode = find_all_dots(arg);

	if (dotmode == FIND_INDIV) {
		if (!(obj = get_obj_in_list_vis(ch, arg, world[IN_ROOM(ch)].contents))) {
			ch->Sendf("You don't see %s %s here.\r\n", AN(arg), arg);
		} else
			perform_get_FromRoom(ch, obj);
	} else {
		if (dotmode == FIND_ALLDOT && !*arg) {
			ch->Send("Get all of what?\r\n");
			return;
		}
		START_ITER(iter, obj, Object *, world[IN_ROOM(ch)].contents) {
			if (obj->CanBeSeen(ch) && (!OBJ_FLAGGED(obj, ITEM_HIDDEN) || IS_STAFF(ch)) &&
				(dotmode == FIND_ALL || isname(arg, obj->Keywords()))) {
				found = 1;
				if (IN_ROOM(obj) != -1) {
					perform_get_FromRoom(ch, obj);
				} else {
					core_dump();
				}
			}
		}
		if (!found) {
			if (dotmode == FIND_ALL)
				ch->Send("There doesn't seem to be anything here.\r\n");
			else {
				act("You don't see any $ts here.\r\n", FALSE, ch, (Object *)arg, 0, TO_CHAR);
			}
		}
	}
}



ACMD(do_get) {
	char *arg1 = get_buffer(MAX_INPUT_LENGTH);
	char *arg2 = get_buffer(MAX_INPUT_LENGTH);

	int cont_dotmode, found = 0, mode;
	Object *cont;
	Character *tmp_char;

	SInt8 prev_encumbrance = GET_ENCUMBRANCE(ch, IS_CARRYING_W(ch)), encumbrance;

	two_arguments(argument, arg1, arg2);

	if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
		ch->Send("Your arms are already full!\r\n");
	else if (!*arg1)
		ch->Send("Get what?\r\n");
	else if (!*arg2)
		get_FromRoom(ch, arg1);
	else {
		cont_dotmode = find_all_dots(arg2);

		if (cont_dotmode == FIND_INDIV) {
			mode = generic_find(arg2, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP, ch, &tmp_char, &cont);
			if (!cont)		ch->Sendf("You don't have %s %s.\r\n", AN(arg2), arg2);
			else {
				switch (GET_OBJ_TYPE(cont)) {
				case ITEM_CONTAINER:
					if (OBJVAL_FLAGGED(cont, CONT_CLOSED)) {
						ch->Send("You'd better open it first!\r\n");
						return;
					}
					break;
				case ITEM_WEAPONCON:
					break;
				default:
					act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
					return;
				}
				get_from_container(ch, cont, arg1, mode);
			}
		} else {
			if (cont_dotmode == FIND_ALLDOT && !*arg2) {
				ch->Send("Get from all of what?\r\n");
				return;
			}
			LListIterator<Object *>	iter(ch->contents);
			while ((cont = iter.Next())) {
				if (cont->CanBeSeen(ch) && (cont_dotmode == FIND_ALL || isname(arg2, cont->Keywords()))) {
					if ((GET_OBJ_TYPE(cont) == ITEM_CONTAINER) ||
						(GET_OBJ_TYPE(cont) == ITEM_WEAPONCON)) {
						found = 1;
						get_from_container(ch, cont, arg1, FIND_OBJ_INV);
					} else if (cont_dotmode == FIND_ALLDOT) {
						found = 1;
						act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
					}
				}
			}
			iter.Restart(world[IN_ROOM(ch)].contents);
			while ((cont = iter.Next())) {
				if (cont->CanBeSeen(ch) && (cont_dotmode == FIND_ALL || isname(arg2, cont->Keywords())))
					if ((GET_OBJ_TYPE(cont) == ITEM_CONTAINER) ||
						(GET_OBJ_TYPE(cont) == ITEM_WEAPONCON)) {
						get_from_container(ch, cont, arg1, FIND_OBJ_ROOM);
						found = 1;
					} else if (cont_dotmode == FIND_ALLDOT) {
						act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
						found = 1;
					}
			}
			if (!found) {
				if (cont_dotmode == FIND_ALL)
					ch->Send("You can't seem to find any containers.\r\n");
				else
					ch->Sendf("You can't seem to find any %ss here.\r\n", arg2);
			}
		}
	}
	release_buffer(arg1);
	release_buffer(arg2);

	encumbrance = GET_ENCUMBRANCE(ch, IS_CARRYING_W(ch));
	if (prev_encumbrance != encumbrance)
		ch->Send(encumbrance_shift[encumbrance][(encumbrance < prev_encumbrance)]);
}


void perform_drop_gold(Character * ch, int amount, SInt8 mode, SInt16 RDR)
{
	Object *obj;

	int drop_wtrigger(Object *obj, Character *actor);

	if (amount <= 0)
		ch->Send("Heh heh heh.. we are jolly funny today, eh?\r\n");

	// Daniel Rollings AKA Daniel Houghton's extra fix to prevent newbie rape
	else if (GET_LEVEL(ch) <= 1) {
		ch->Send("No... better hold on to your money for now.\r\n");
		return;
	} else if (GET_GOLD(ch) < amount)
		ch->Send("You don't have that many coins!\r\n");
	else {
		if (mode != SCMD_JUNK) {
			WAIT_STATE(ch, PULSE_VIOLENCE);	/* to prevent coin-bombing */
			obj = create_money(amount);
			if (mode == SCMD_DONATE) {
				ch->Send("You throw some gold into the air where it disappears in a puff of smoke!\r\n");
				act("$n throws some gold into the air where it disappears in a puff of smoke!", FALSE, ch, 0, 0, TO_ROOM);
				obj->ToRoom(RDR);
				act("$p suddenly appears in a puff of orange smoke!", 0, 0, obj, 0, TO_ROOM);
			} else {
				if (!drop_wtrigger(obj, ch))
					return;
				ch->Send("You drop some gold.\r\n");
				sprintf(buf, "$n drops %s.", money_desc(amount));
				act(buf, TRUE, ch, 0, 0, TO_ROOM);
				obj->ToRoom(ch->InRoom());
			}
		} else {
			sprintf(buf, "$n drops %s which disappears in a puff of smoke!",
				money_desc(amount));
			act(buf, FALSE, ch, 0, 0, TO_ROOM);
			ch->Send("You drop some gold which disappears in a puff of smoke!\r\n");
		}
		GET_GOLD(ch) -= amount;
	}
}


int can_drop_obj(Character *ch, Object *obj, SInt8 mode, const char *sname)
{
	int drop_otrigger(Object *obj, Character *actor);
	int drop_wtrigger(Object *obj, Character *actor);


	if (OBJ_FLAGGED(obj, ITEM_NODROP) && !IS_IMMORTAL(ch)) {
		sprintf(buf, "For some reason, you can't %s $p!", sname);
		act(buf, FALSE, ch, obj, 0, TO_CHAR);
		return 0;
	}

	if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && GET_OBJ_VAL(obj, 3) && mode == SCMD_JUNK) {
		act("You cannot dispose of $p that way!", FALSE, ch, obj, 0, TO_CHAR);
		return 0;
	}

	if (!drop_otrigger(obj, ch))
		return 0;

	if (PURGED(obj) || PURGED(ch) || (obj->CarriedBy() != ch))
		return 0;

	if ((mode == SCMD_DROP) && !drop_wtrigger(obj, ch))
		return 0;

	if (PURGED(obj) || PURGED(ch) || (obj->CarriedBy() != ch))
		return 0;

	return 1;
}


#define VANISH(mode) ((mode == SCMD_DONATE || mode == SCMD_JUNK) ? \
		      "  It vanishes in a puff of smoke!" : "")

int perform_drop(Character * ch, Object * obj, SInt8 mode, const char *sname, VNum RDR) {
	char *	buf;
	SInt32	value = 0;

	if (!can_drop_obj(ch, obj, mode, sname))
		return 0;

	if (OBJ_FLAGGED(obj, ITEM_PROXIMITY)) {
		kill_prox_item(ch, obj);
		return 0;
	}

	if (!can_drop_obj(ch, obj, mode, sname))				return 0;

	buf = get_buffer(SMALL_BUFSIZE);

	if (OBJ_FLAGGED(obj, ITEM_NODROP)) {
		sprintf(buf, "You can't %s $p!", sname);
		act(buf, FALSE, ch, obj, 0, TO_CHAR);
#ifdef GOT_RID_OF_IT
	} else if (OBJ_FLAGGED(obj, ITEM_MISSION)) {
		sprintf(buf, "You can't %s $p, it is Mission Equipment.", sname);
		act(buf, FALSE, ch, obj, 0, TO_CHAR);
#endif
	} else {
		sprintf(buf, "You %s $p.%s", sname, VANISH(mode));
		act(buf, FALSE, ch, obj, 0, TO_CHAR);
		sprintf(buf, "$n %ss $p.%s", sname, VANISH(mode));
		act(buf, TRUE, ch, obj, 0, TO_ROOM);
		obj->FromChar();

		if ((mode == SCMD_DONATE) && OBJ_FLAGGED(obj, ITEM_NODONATE))
			mode = SCMD_JUNK;

		switch (mode) {
			case SCMD_DROP:
				obj->ToRoom(IN_ROOM(ch));
				break;
			case SCMD_DONATE:
				obj->ToRoom(RDR);
				act("$p suddenly appears in a puff a smoke!", FALSE, 0, obj, 0, TO_ROOM);
				break;
			case SCMD_JUNK:
				value = RANGE(1, GET_OBJ_COST(obj) / 16, 200);
				obj->Extract();
				break;
			default:
				log("SYSERR: perform_drop(ch = %s, obj = %s, mode = %d, sname = %s, RDR = %d): Invalid mode",
						ch->RealName(), obj->Name(), mode, sname, RDR);
				break;
		}
	}
	release_buffer(buf);
	return value;
}


ACMD(do_drop) {
	Object *	obj;
	VNum		RDR = 0;
	SInt8		mode = SCMD_DROP;
	int			dotmode, amount = 0;
	const char *sname;
	char *		arg;

	SInt8 prev_encumbrance = GET_ENCUMBRANCE(ch, IS_CARRYING_W(ch)), encumbrance;

	switch (subcmd) {
		case SCMD_JUNK:
			sname = "junk";
			mode = SCMD_JUNK;
			break;
		case SCMD_DONATE:
			sname = "donate";
			mode = SCMD_DONATE;
			switch (Number(0, 2)) {
				case 0:
					mode = SCMD_JUNK;
					break;
				case 1:
				case 2:
					RDR = donation_room_1;
					break;
//				case 3: RDR = Room::Real(donation_room_2); break;
//				case 4: RDR = Room::Real(donation_room_3); break;
			}
			if (!Room::Find(RDR)) {
				ch->Send("Sorry, you can't donate anything right now.\r\n");
				return;
			}
			break;
		default:
			sname = "drop";
			break;
	}
	arg = get_buffer(MAX_INPUT_LENGTH);

	argument = one_argument(argument, arg);

	if (!*arg)
		ch->Sendf("What do you want to %s?\r\n", sname);
	else if (is_number(arg)) {
		amount = atoi(arg);
		argument = one_argument(argument, arg);
//		if (!str_cmp("coins", arg) || !str_cmp("coin", arg))
//			perform_drop_gold(ch, amount, mode, RDR);
//		else {
			//	code to drop multiple items.  anyone want to write it? -je
			ch->Send("Sorry, you can't do that to more than one item at a time.\r\n");
//		}
	} else {
		dotmode = find_all_dots(arg);

		/* Can't junk or donate all */
		if ((dotmode == FIND_ALL) && (subcmd == SCMD_JUNK || subcmd == SCMD_DONATE)) {
			if (subcmd == SCMD_JUNK)
				ch->Send("Go to the dump if you want to junk EVERYTHING!\r\n");
			else
				ch->Send("Go do the donation room if you want to donate EVERYTHING!\r\n");
		} else if (dotmode == FIND_ALL) {
			if (!ch->contents.Count())
				ch->Send("You don't seem to be carrying anything.\r\n");
			else {
				START_ITER(iter, obj, Object *, ch->contents) {
					if (obj->CanBeSeen(ch))
						amount += perform_drop(ch, obj, mode, sname, RDR);
				}
			}
		} else if (dotmode == FIND_ALLDOT) {
			if (!*arg)
				ch->Sendf("What do you want to %s all of?\r\n", sname);
			else if (!(obj = get_obj_in_list_vis(ch, arg, ch->contents)))
				ch->Sendf("You don't seem to have any %ss.\r\n", arg);
			else {
				START_ITER(iter, obj, Object *, ch->contents) {
					if (obj->CanBeSeen(ch) && isname(arg, obj->Keywords()))
						amount += perform_drop(ch, obj, mode, sname, RDR);
				}
			}
		} else if (!(obj = get_obj_in_list_vis(ch, arg, ch->contents)))
			ch->Sendf("You don't seem to have %s %s.\r\n", AN(arg), arg);
		else
			amount += perform_drop(ch, obj, mode, sname, RDR);
	}

//	if (amount && (subcmd == SCMD_JUNK)) {
//		act("You trash $p.", TRUE, ch, obj, 0, TO_CHAR);
//		act("$n trashes $p.", TRUE, ch, obj, 0, TO_ROOM);
//		ch->Send("You have been rewarded by the gods!\r\n");
//		act("$n has been rewarded by the gods!", TRUE, ch, 0, 0, TO_ROOM);
//		GET_MISSION_POINTS(ch) += amount;
//	}
	release_buffer(arg);

	encumbrance = GET_ENCUMBRANCE(ch, IS_CARRYING_W(ch));
	if (prev_encumbrance != encumbrance)
		ch->Send(encumbrance_shift[encumbrance][(encumbrance < prev_encumbrance)]);
}


void perform_give(Character * ch, Character * vict,  Object * obj) {

#if 0
	if (OBJ_FLAGGED(obj, ITEM_NODROP) && !IS_IMMORTAL(ch)) {
		act("You can't let go of $p!!	Yeech!", FALSE, ch, obj, 0, TO_CHAR);
		return;
	}
	if (OBJ_FLAGGED(obj, ITEM_PROXIMITY)) {
		kill_prox_item(ch, obj);
		return;
	}

	if (!give_otrigger(obj, ch, vict))	return;
	if (PURGED(obj) || PURGED(ch) || PURGED(vict) || (obj->CarriedBy() != ch))	return;
	if (!receive_mtrigger(vict, ch, obj))	return;
	if (PURGED(obj) || PURGED(ch) || PURGED(vict) || (obj->CarriedBy() != ch))	return;

	if (OBJ_FLAGGED(obj, ITEM_NODROP))
		act("You can't let go of $p!!  Yeech!", FALSE, ch, obj, 0, TO_CHAR);
	else if (IS_CARRYING_N(vict) >= CAN_CARRY_N(vict))
		act("$N seems to have $S hands full.", FALSE, ch, 0, vict, TO_CHAR);
	else if (GET_OBJ_WEIGHT(obj) + IS_CARRYING_W(vict) > CAN_CARRY_W(vict))
		act("$E can't carry that much weight.", FALSE, ch, 0, vict, TO_CHAR);
	else if (!vict->CanUse(obj))
		act("$E can't figure what $p does, so you keep it.", FALSE, ch, obj, vict, TO_CHAR);
	else {
		obj->FromChar();
		obj->ToChar(vict);
		act("You give $p to $N.", FALSE, ch, obj, vict, TO_CHAR);
		act("$n gives you $p.", FALSE, ch, obj, vict, TO_VICT);
		act("$n gives $p to $N.", TRUE, ch, obj, vict, TO_NOTVICT);
	}
#endif
	int give_otrigger(Object *obj, Character *actor,
				Character *victim);
	int receive_mtrigger(Character *ch, Character *actor,
					 Object *obj);

	if (OBJ_FLAGGED(obj, ITEM_NODROP) && !IS_IMMORTAL(ch)) {
		act("You can't let go of $p!!	Yeech!", FALSE, ch, obj, 0, TO_CHAR);
		return;
	}
	if (vict->material == Materials::Ether && obj->material != Materials::Ether) {
		act("$N cannot take hold of $p.", FALSE, ch, obj, vict, TO_CHAR);
		return;
	}
	if (OBJ_FLAGGED(obj, ITEM_PROXIMITY)) {
		kill_prox_item(ch, obj);
		return;
	}
	if (IS_CARRYING_N(vict) >= CAN_CARRY_N(vict)) {
		act("$N seems to have $S hands full.", FALSE, ch, 0, vict, TO_CHAR);
		return;
	}
	if (GET_OBJ_WEIGHT(obj) + IS_CARRYING_W(vict) > CAN_CARRY_W(vict)) {
		act("$E can't carry that much weight.", FALSE, ch, 0, vict, TO_CHAR);
		return;
	}
	if (!give_otrigger(obj, ch, vict))
		return;

	if (PURGED(obj) || PURGED(ch) || PURGED(vict) || (obj->CarriedBy() != ch))
		return;

	if (!receive_mtrigger(vict, ch, obj))
		return;

	if (PURGED(obj) || PURGED(ch) || PURGED(vict) || (obj->CarriedBy() != ch))
		return;

	obj->FromChar();
	obj->ToChar(vict);

	act("You give $p to $N.", FALSE, ch, obj, vict, TO_CHAR);
	act("$n gives you $p.", FALSE, ch, obj, vict, TO_VICT);
	act("$n gives $p to $N.", TRUE, ch, obj, vict, TO_NOTVICT);
}


/* utility function for give */
Character *give_find_vict(Character * ch, char *arg)
{
	Character *vict;

	if (!*arg) {
		ch->Send("To who?\r\n");
		return NULL;
	} else if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
		ch->Send(NOPERSON);
		return NULL;
	} else if (vict == ch) {
		ch->Send("What's the point of that?\r\n");
		return NULL;
	} else
		return vict;
}


void perform_give_gold(Character * ch, Character * vict, int amount)
{
	void bribe_mtrigger(Character *ch, Character *actor, int amount);

	if (amount <= 0) {
		ch->Send("Heh heh heh ... we are jolly funny today, eh?\r\n");
		return;
	}
	// Daniel Rollings AKA Daniel Houghton's extra fix to prevent newbie rape
	if (GET_LEVEL(ch) <= 1) {
		ch->Send("No... better hold on to your money for now.\r\n");
		return;
	}
	if ((GET_GOLD(ch) < amount) &&
			(IS_NPC(ch) || !IS_IMMORTAL(ch) || (GET_TRUST(ch) < TRUST_GOD))) {
		ch->Send("You don't have that many coins!\r\n");
		return;
	}
	ch->Send(OK);

	sprintf(buf, "$n gives you %d gold coins.", amount);
	act(buf, FALSE, ch, 0, vict, TO_VICT);
	sprintf(buf, "$n gives %s to $N.", money_desc(amount));
	act(buf, TRUE, ch, 0, vict, TO_NOTVICT);
	if (IS_NPC(ch) || !IS_IMMORTAL(ch) || (GET_TRUST(ch) < TRUST_GOD))
		GET_GOLD(ch) -= amount;
	GET_GOLD(vict) += amount;

	bribe_mtrigger(vict, ch, amount);
}


ACMD(do_give) {
	int amount, dotmode;
	Character *vict = NULL;
	Object *obj, *next_obj;
	LListIterator<Object *>	iter;
	SInt8 ch_prev_encumbrance = GET_ENCUMBRANCE(ch, IS_CARRYING_W(ch)),
			 vict_prev_encumbrance = 0,
			 encumbrance;

	argument = one_argument(argument, arg);

	if (!*arg)
		ch->Send("Give what to who?\r\n");
	else if (is_number(arg)) {
		amount = atoi(arg);
		argument = one_argument(argument, arg);
		if (!str_cmp("coins", arg) || !str_cmp("coin", arg)) {
			argument = one_argument(argument, arg);
			if ((vict = give_find_vict(ch, arg))) {
				if (!IS_NPC(ch) && !IS_NPC(vict) && (GET_TRUST(ch)) && (!IS_SET(STF_FLAGS(ch), Staff::Admin)) && (!(GET_TRUST(vict)))) {
					ch->Send("Builders and staff can not give money to mortals!\r\n");
					sprintf(buf, "%s tried to give coins to %s", GET_NAME(ch), GET_NAME(vict));
					log(buf);
					return;
				}
				vict_prev_encumbrance = GET_ENCUMBRANCE(vict, IS_CARRYING_W(vict)),
				perform_give_gold(ch, vict, amount);
			}
			return;
		} else {
			/* code to give multiple items.	anyone want to write it? -je */
			ch->Send("You can't give more than one item at a time.\r\n");
			return;
		}
	} else {
		one_argument(argument, buf1);
		if (!(vict = give_find_vict(ch, buf1))) {
			return;
		} else {
			vict_prev_encumbrance = GET_ENCUMBRANCE(vict, IS_CARRYING_W(vict));
			if ((GET_TRUST(ch)) && (!IS_SET(STF_FLAGS(ch), Staff::Admin)) && (!(GET_TRUST(vict)))) {
				ch->Send("Builders and immortals can not give objects to mortals!\r\n");
				sprintf(buf, "%s tried to give an object to %s", GET_NAME(ch), GET_NAME(vict));
				log(buf);
				return;
			}
		}
		dotmode = find_all_dots(arg);
		if (dotmode == FIND_INDIV) {
			if (!(obj = get_obj_in_list_vis(ch, arg, ch->contents))) {
				sprintf(buf, "You don't seem to have %s %s.\r\n", AN(arg), arg);
				ch->Send(buf);
			} else
				perform_give(ch, vict, obj);
		} else {
			if (dotmode == FIND_ALLDOT && !*arg) {
				ch->Send("All of what?\r\n");
				return;
			}
			if (ch->contents.Count() <= 0)
				ch->Send("You don't seem to be holding anything.\r\n");
			else {
				START_ITER(iter, obj, Object *, ch->contents) {
					if (CAN_SEE_OBJ(ch, obj) && (dotmode == FIND_ALL || isname(arg, obj->Keywords())))
						perform_give(ch, vict, obj);
				}
			}
		}
	}

	encumbrance = GET_ENCUMBRANCE(ch, IS_CARRYING_W(ch));
	if (ch_prev_encumbrance != encumbrance)
		ch->Send(encumbrance_shift[encumbrance][(encumbrance < ch_prev_encumbrance)]);

	if (vict) {
		encumbrance = GET_ENCUMBRANCE(vict, IS_CARRYING_W(vict));
		if (vict_prev_encumbrance != encumbrance)
			vict->Send(encumbrance_shift[encumbrance][(encumbrance < vict_prev_encumbrance)]);
	}
}



void weight_change_object(Object * obj, int weight) {
	Object *tmp_obj = NULL;
	Object *loop_obj = NULL;
	Character *tmp_ch = NULL;
	SInt8 prev_encumbrance = 0, encumbrance;

	if (obj->InRoom() != NOWHERE) {
		GET_OBJ_WEIGHT(obj) += weight;
	} else if ((tmp_ch = obj->CarriedBy())) {
		prev_encumbrance = GET_ENCUMBRANCE(tmp_ch, IS_CARRYING_W(tmp_ch));
		obj->FromChar();
		GET_OBJ_WEIGHT(obj) += weight;
		obj->ToChar(tmp_ch);
	} else if ((tmp_obj = obj->InObj())) {

		loop_obj = tmp_obj;
		while (loop_obj->InObj())
			loop_obj = (Object *) loop_obj->Inside();
		if ((tmp_ch = loop_obj->CarriedBy()))
			prev_encumbrance = GET_ENCUMBRANCE(tmp_ch, IS_CARRYING_W(tmp_ch));

		obj->FromObj();
		GET_OBJ_WEIGHT(obj) += weight;
		obj->ToObj(tmp_obj);
	} else {
		log("SYSERR: Unknown attempt to subtract weight from an object.");
	}

	if (tmp_ch) {
		encumbrance = GET_ENCUMBRANCE(tmp_ch, IS_CARRYING_W(tmp_ch));
		if (prev_encumbrance != encumbrance)
			tmp_ch->Send(encumbrance_shift[encumbrance][(encumbrance < prev_encumbrance)]);
	}
}


void name_from_drinkcon(Object * obj) {
	int i;
	char *new_name;

	for (i = 0; (*(obj->Name() + i) != ' ') && (*(obj->Name() + i) != '\0'); i++);

	if (*(obj->Name() + i) == ' ') {
		new_name = str_dup(obj->Name() + i + 1);
		obj->SetName(new_name);
		free(new_name);
	}
}


void name_to_drinkcon(Object * obj, int type) {
	char *new_name;

	CREATE(new_name, char, strlen(obj->Name()) + strlen(drinknames[type]) + 2);
	sprintf(new_name, "%s %s", drinknames[type], obj->Name());
	SSFree(obj->name);
	obj->name = SString::Create(new_name);
	free(new_name);
}



ACMD(do_drink) {
	Object *temp;
	Affect *af;
	int amount, weight;
	int on_ground = 0;
	char *arg = get_buffer(MAX_INPUT_LENGTH);

	one_argument(argument, arg);

	if (!*arg) {
		ch->Send("Drink from what?\r\n");
		release_buffer(arg);
		return;
	}
	if (!(temp = get_obj_in_list_vis(ch, arg, ch->contents))) {
		if (!(temp = get_obj_in_list_vis(ch, arg, world[IN_ROOM(ch)].contents))) {
			act("You can't find it!", FALSE, ch, 0, 0, TO_CHAR);
			release_buffer(arg);
			return;
		} else
			on_ground = 1;
	}
	release_buffer(arg);
	if ((GET_OBJ_TYPE(temp) != ITEM_DRINKCON) &&
			(GET_OBJ_TYPE(temp) != ITEM_FOUNTAIN)) {
		ch->Send("You can't drink from that!\r\n");
		return;
	}
	if (on_ground && (GET_OBJ_TYPE(temp) == ITEM_DRINKCON)) {
		ch->Send("You have to be holding that to drink from it.\r\n");
		return;
	}
	if ((GET_COND(ch, DRUNK) > 10) && (GET_COND(ch, THIRST) > 0)) {
		/* The pig is drunk */
		ch->Send("You can't seem to get close enough to your mouth.\r\n");
		act("$n tries to drink but misses $s mouth!", TRUE, ch, 0, 0, TO_ROOM);
		return;
	}
	if (!GET_OBJ_VAL(temp, 1)) {
		ch->Send("It's empty.\r\n");
		return;
	}

	if (!consume_otrigger(temp, ch))
		return;

	if ((GET_COND(ch, FULL) > 20) && (GET_COND(ch, THIRST) > 0)) {
		ch->Send("Your stomach can't contain anymore!\r\n");
		return;
	}

	if (subcmd == SCMD_DRINK) {
		act("$n drinks $T from $p.", TRUE, ch, temp, drinks[GET_OBJ_VAL(temp, 2)], TO_ROOM);

		act("You drink the $T.\r\n", FALSE, ch, 0, drinks[GET_OBJ_VAL(temp, 2)], TO_CHAR);

		if (drink_aff[GET_OBJ_VAL(temp, 2)][DRUNK] > 0)
			amount = (25 - GET_COND(ch, THIRST)) / (drink_aff[GET_OBJ_VAL(temp, 2)][DRUNK] * 10);
		else
			amount = Number(1, 3);

	} else {
		act("$n sips from $p.", TRUE, ch, temp, 0, TO_ROOM);
		act("It tastes like $t.\r\n", FALSE, ch, (Object *)drinks[GET_OBJ_VAL(temp, 2)], 0, TO_CHAR);
		amount = 1;
	}

	amount = MIN(amount, GET_OBJ_VAL(temp, 1));

	/* You can't subtract more than the object weighs */
	weight = MIN(amount, GET_OBJ_WEIGHT(temp));

	weight_change_object(temp, -weight);	/* Subtract amount */

	gain_condition(ch, DRUNK, (int) ((int) drink_aff[GET_OBJ_VAL(temp, 2)][DRUNK] * amount));
	gain_condition(ch, FULL, (int) ((int) drink_aff[GET_OBJ_VAL(temp, 2)][FULL] * amount));
	gain_condition(ch, THIRST, (int) ((int) drink_aff[GET_OBJ_VAL(temp, 2)][THIRST] * amount));

	if (GET_COND(ch, DRUNK) > 10)
		ch->Send("You feel drunk.\r\n");

	if (GET_COND(ch, THIRST) > 20)
		ch->Send("You don't feel thirsty any more.\r\n");

	if (GET_COND(ch, FULL) > 20)
		ch->Send("You are full.\r\n");

	if ((GET_OBJ_VAL(temp, 3) > 0) && !IS_IMMORTAL(ch)) {
    	/* The shit was poisoned ! */
		ch->Send("Oops, it tasted rather strange!\r\n");
		act("$n chokes and utters some strange sounds.", TRUE, ch, 0, 0, TO_ROOM);
		af = new Affect(SPELL_POISON, 0, Affect::None, AffectBit::Poison);
		af->Join(ch, amount * 3, false, false, false, false);
	}

	/* empty the container, and no longer poison. */
	GET_OBJ_VAL(temp, 1) -= amount;
	if (!GET_OBJ_VAL(temp, 1)) {	/* The last bit */
		GET_OBJ_VAL(temp, 2) = 0;
		GET_OBJ_VAL(temp, 3) = 0;
		name_from_drinkcon(temp);
	}
	return;
}



ACMD(do_eat)
{
	Object *food;
	Affect *af;
	int amount;
	char *arg = get_buffer(MAX_INPUT_LENGTH);
	SInt8 prev_encumbrance = GET_ENCUMBRANCE(ch, IS_CARRYING_W(ch)), encumbrance;

	one_argument(argument, arg);

	if (!*arg) {
		ch->Send("Eat what?\r\n");
		release_buffer(arg);
		return;
	}
	if (!(food = get_obj_in_list_vis(ch, arg, ch->contents))) {
		ch->Sendf("You don't seem to have %s %s.\r\n", AN(arg), arg);
		release_buffer(arg);
		return;
	}
	release_buffer(arg);
	if (subcmd == SCMD_TASTE && ((GET_OBJ_TYPE(food) == ITEM_DRINKCON) ||
			(GET_OBJ_TYPE(food) == ITEM_FOUNTAIN))) {
		do_drink(ch, argument, 0, "drink", SCMD_SIP);
		return;
	}
	if ((GET_OBJ_TYPE(food) != ITEM_FOOD) && !IS_STAFF(ch)) {
		ch->Send("You can't eat THAT!\r\n");
		return;
	}

	if (!consume_otrigger(food, ch))
		return;

	if (PURGED(food) || PURGED(ch) || (food->CarriedBy() != ch))
		return;

	if (GET_COND(ch, FULL) > 20) {/* Stomach full */
		act("You are too full to eat more!", FALSE, ch, 0, 0, TO_CHAR);
		return;
	}
	if (subcmd == SCMD_EAT) {
		act("You eat the $o.", FALSE, ch, food, 0, TO_CHAR);
		act("$n eats $p.", TRUE, ch, food, 0, TO_ROOM);
	} else {
		act("You nibble a little bit of the $o.", FALSE, ch, food, 0, TO_CHAR);
		act("$n tastes a little bit of $p.", TRUE, ch, food, 0, TO_ROOM);
	}

	amount = (subcmd == SCMD_EAT ? GET_OBJ_VAL(food, 0) : 1);

	gain_condition(ch, FULL, amount);

	if (GET_COND(ch, FULL) > 20)
		act("You are full.", FALSE, ch, 0, 0, TO_CHAR);

	if (GET_OBJ_VAL(food, 3) && !IS_IMMORTAL(ch)) {
		/* The shit was poisoned ! */
		ch->Send("Oops, that tasted rather strange!\r\n");
		act("$n coughs and utters some strange sounds.", FALSE, ch, 0, 0, TO_ROOM);

		af = new Affect(SPELL_POISON, 0, Affect::None, AffectBit::Poison);
		af->Join(ch, amount * 2, FALSE, FALSE, FALSE, FALSE);
	}

	if (subcmd == SCMD_EAT)
		food->Extract();
	else if (!(--GET_OBJ_VAL(food, 0))) {
		ch->Send("There's nothing left now.\r\n");
		food->Extract();
	}

	encumbrance = GET_ENCUMBRANCE(ch, IS_CARRYING_W(ch));
	if (prev_encumbrance != encumbrance)
		ch->Send(encumbrance_shift[encumbrance][(encumbrance < prev_encumbrance)]);

}


ACMD(do_pour) {
	char *arg1 = get_buffer(MAX_INPUT_LENGTH);
	char *arg2 = get_buffer(MAX_INPUT_LENGTH);
	Object *from_obj = NULL, *to_obj = NULL;
	int amount = TRUE;

	two_arguments(argument, arg1, arg2);

	if (subcmd == SCMD_POUR) {
		if (!*arg1) {		/* No arguments */
			act("From what do you want to pour?", FALSE, ch, 0, 0, TO_CHAR);
			amount = FALSE;
		} else if (!(from_obj = get_obj_in_list_vis(ch, arg1, ch->contents))) {
			act("You can't find it!", FALSE, ch, 0, 0, TO_CHAR);
			amount = FALSE;
		} else if (GET_OBJ_TYPE(from_obj) != ITEM_DRINKCON) {
			act("You can't pour from that!", FALSE, ch, 0, 0, TO_CHAR);
			amount = FALSE;
		}
	} else if (subcmd == SCMD_FILL) {
		if (!*arg1) {		/* no arguments */
			ch->Send("What do you want to fill?  And what are you filling it from?\r\n");
			amount = FALSE;
		} else if (!(to_obj = get_obj_in_list_vis(ch, arg1, ch->contents))) {
			ch->Send("You can't find it!");
			amount = FALSE;
		} else if (GET_OBJ_TYPE(to_obj) != ITEM_DRINKCON) {
			act("You can't fill $p!", FALSE, ch, to_obj, 0, TO_CHAR);
			amount = FALSE;
		} else if (!*arg2) {		/* no 2nd argument */
			act("What do you want to fill $p from?", FALSE, ch, to_obj, 0, TO_CHAR);
			amount = FALSE;
		} else if (!(from_obj = get_obj_in_list_vis(ch, arg2, world[IN_ROOM(ch)].contents))) {
			ch->Sendf("There doesn't seem to be %s %s here.\r\n", AN(arg2), arg2);
			amount = FALSE;
		} else if (GET_OBJ_TYPE(from_obj) != ITEM_FOUNTAIN) {
			act("You can't fill something from $p.", FALSE, ch, from_obj, 0, TO_CHAR);
			amount = FALSE;
		}
	}
	if (!amount) {
		release_buffer(arg1);
		release_buffer(arg2);
		return;
	}
	if (GET_OBJ_VAL(from_obj, 1) == 0) {
		act("The $p is empty.", FALSE, ch, from_obj, 0, TO_CHAR);
		release_buffer(arg1);
		release_buffer(arg2);
		return;
	}

	if (subcmd == SCMD_POUR) {	/* pour */
		if (!*arg2) {
			act("Where do you want it?  Out or in what?", FALSE, ch, 0, 0, TO_CHAR);
			amount = FALSE;
		} else if (!str_cmp(arg2, "out")) {
			act("$n empties $p.", TRUE, ch, from_obj, 0, TO_ROOM);
			act("You empty $p.", FALSE, ch, from_obj, 0, TO_CHAR);

			weight_change_object(from_obj, -GET_OBJ_VAL(from_obj, 1)); /* Empty */

			GET_OBJ_VAL(from_obj, 1) = 0;
			GET_OBJ_VAL(from_obj, 2) = 0;
			GET_OBJ_VAL(from_obj, 3) = 0;
			name_from_drinkcon(from_obj);

			amount = FALSE;
		} else if (!(to_obj = get_obj_in_list_vis(ch, arg2, ch->contents))) {
			act("You can't find it!", FALSE, ch, 0, 0, TO_CHAR);
			amount = FALSE;
		} else if ((GET_OBJ_TYPE(to_obj) != ITEM_DRINKCON) &&
				(GET_OBJ_TYPE(to_obj) != ITEM_FOUNTAIN)) {
			act("You can't pour anything into that.", FALSE, ch, 0, 0, TO_CHAR);
			amount = FALSE;
		}
	}
	if (!amount) {
		release_buffer(arg1);
		release_buffer(arg2);
		return;
	}

	if (to_obj == from_obj) {
		act("A most unproductive effort.", FALSE, ch, 0, 0, TO_CHAR);
		amount = FALSE;
	} else if ((GET_OBJ_VAL(to_obj, 1) != 0) &&
			(GET_OBJ_VAL(to_obj, 2) != GET_OBJ_VAL(from_obj, 2))) {
		act("There is already another liquid in it!", FALSE, ch, 0, 0, TO_CHAR);
		amount = FALSE;
	} else if (!(GET_OBJ_VAL(to_obj, 1) < GET_OBJ_VAL(to_obj, 0))) {
		act("There is no room for more.", FALSE, ch, 0, 0, TO_CHAR);
		amount = FALSE;
	}
	if (!amount) {
		release_buffer(arg1);
		release_buffer(arg2);
		return;
	}

	if (subcmd == SCMD_POUR) {
		act("You pour the $T into the $p.", FALSE, ch, to_obj,
				drinks[GET_OBJ_VAL(from_obj, 2)], TO_CHAR);
	} else if (subcmd == SCMD_FILL) {
		act("You gently fill $p from $P.", FALSE, ch, to_obj, from_obj, TO_CHAR);
		act("$n gently fills $p from $P.", TRUE, ch, to_obj, from_obj, TO_ROOM);
	}
	/* New alias */
	if (GET_OBJ_VAL(to_obj, 1) == 0)
		name_to_drinkcon(to_obj, GET_OBJ_VAL(from_obj, 2));

	/* First same type liq. */
	GET_OBJ_VAL(to_obj, 2) = GET_OBJ_VAL(from_obj, 2);

	/* Then how much to pour */
	GET_OBJ_VAL(from_obj, 1) -= (amount = (GET_OBJ_VAL(to_obj, 0) - GET_OBJ_VAL(to_obj, 1)));

	GET_OBJ_VAL(to_obj, 1) = GET_OBJ_VAL(to_obj, 0);

	if (GET_OBJ_VAL(from_obj, 1) < 0) {	/* There was too little */
		GET_OBJ_VAL(to_obj, 1) += GET_OBJ_VAL(from_obj, 1);
		amount += GET_OBJ_VAL(from_obj, 1);
		GET_OBJ_VAL(from_obj, 1) = 0;
		GET_OBJ_VAL(from_obj, 2) = 0;
		GET_OBJ_VAL(from_obj, 3) = 0;
		name_from_drinkcon(from_obj);
	}
	/* Then the poison boogie */
	GET_OBJ_VAL(to_obj, 3) = (GET_OBJ_VAL(to_obj, 3) || GET_OBJ_VAL(from_obj, 3));

	/* And the weight boogie */
	weight_change_object(from_obj, -amount);
	weight_change_object(to_obj, amount);	/* Add weight */

	release_buffer(arg1);
	release_buffer(arg2);
}



// Send a message to the character and the room when objects are equipped.
void wear_message(Character * ch, Object * obj, int where)
{
	char *buf = get_buffer(MAX_INPUT_LENGTH);
	char *buf2 = get_buffer(MAX_INPUT_LENGTH);

	SInt8 bodytype = GET_BODYTYPE(ch);
	SInt8 msgnum = GET_WEARSLOT(where, bodytype).msg_wearon;

	if (where >= WEAR_HAND_R) {
		if (GET_OBJ_TYPE(obj) == ITEM_WEAPON) {
			msgnum = WEAR_MSG_WIELD;
//		 } else if (GET_OBJ_TYPE(obj) == ITEM_LIGHT) {
//			 msgnum = WEAR_MSG_LIGHT;
		} else if (CAN_WEAR(obj, ITEM_WEAR_SHIELD))
			msgnum = WEAR_MSG_STRAPTO;

		if (OBJ_FLAGGED(obj, ITEM_TWO_HAND)) {
			sprintf(buf, "You %s both %ss.", wear_slot_messages[msgnum].to_char,
							GET_WEARSLOT(where, bodytype).partname);
			sprintf(buf2, "$n %s both %ss.", wear_slot_messages[msgnum].to_room,
							GET_WEARSLOT(where, bodytype).partname);
		} else {
			sprintf(buf, "You %s your %s %s.", wear_slot_messages[msgnum].to_char,
							GET_WEARSLOT(where, bodytype).keywords,
							GET_WEARSLOT(where, bodytype).partname);
			sprintf(buf2, "$n %s $s %s %s.", wear_slot_messages[msgnum].to_room,
							GET_WEARSLOT(where, bodytype).keywords,
							GET_WEARSLOT(where, bodytype).partname);
		}

	} else {
			sprintf(buf, "You %s your %s.", wear_slot_messages[msgnum].to_char,
							GET_WEARSLOT(where, bodytype).partname);
			sprintf(buf2, "$n %s $s %s.", wear_slot_messages[msgnum].to_room,
							GET_WEARSLOT(where, bodytype).partname);
	}

	act(buf, FALSE, ch, obj, 0, TO_CHAR);
	act(buf2, TRUE, ch, obj, 0, TO_ROOM);

	release_buffer(buf);
	release_buffer(buf2);
}


// This routine is used to check for the use of two-handed objects when
// placing any object, and properly place a two-handed object.	Returns
// the wear-slot value if valid, -1 if not.
SInt8 check_empty_slot(Character * ch, Object * obj, int &i, SInt8 &bodytype)
{

	// This routine is used to A) Check for the use of two-handed objects when
	// placing any object, and B) properly place a two-handed object.

	// Check for equipped two-handed objects in the previous slot.
	if ((i > WEAR_HAND_R) && (GET_EQ(ch, i - 1)) && (OBJ_FLAGGED(GET_EQ(ch, i - 1), ITEM_TWO_HAND))) {
		return -1;	// There's a two-handed object in the previous slot, forget it.
	}

	// Check next slot if equipping a two-handed object
	if (OBJ_FLAGGED(obj, ITEM_TWO_HAND)) {
		if ((i == (NUM_WEARS - 1)) ||
			 (!CAN_WEAR(obj, GET_WEARSLOT(i + 1, bodytype).wear_bitvector)) ||
			 (GET_EQ(ch, i + 1)))
			return -1;
	}

	return i;		// We passed the checks!

}


void perform_wear(Character * ch, Object * obj, int where)
{
	char *buf = get_buffer(MAX_INPUT_LENGTH);
	int wear_otrigger(Object *obj, Character *actor, int where);
	SInt8 bodytype = GET_BODYTYPE(ch);

	if (GET_EQ(ch, where) || (check_empty_slot(ch, obj, where, bodytype) < 0)) {
		if (where < WEAR_HAND_R) {	// For regular slots, it's a simple string from already_wearing.
			sprintf(buf, "%s %s.\r\n",
					already_wearing[GET_WEARSLOT(where, bodytype).msg_already_wear],
					GET_WEARSLOT(where, bodytype).partname);
		} else {			// Special handling for hand slots.
			sprintf(buf, "Your %ss are already full!\r\n", GET_WEARSLOT(where, bodytype).partname);
		}
		ch->Send(buf);
        release_buffer(buf);
		return;
	}

	if (!wear_otrigger(obj, ch, where)) {
        release_buffer(buf);
		return;
    }

	if (PURGED(obj) || PURGED(ch) || (obj->CarriedBy() != ch)) {
        release_buffer(buf);
		return;
    }

	wear_message(ch, obj, where);
	obj->FromChar();
	equip_char(ch, obj, where);
	release_buffer(buf);
}


// Returns a wear slot number.	Said wear slot will be the last slot with a
// valid bitvector checked before the slot that is found ready (correct
// bitvector, not already equipped).	If said slot is not found, it indicates
// that the player has no slot that can equip the object.
int find_eq_pos(Character * ch, Object * obj, char *arg,
				SInt8 bottomslot = 0, SInt8 topslot = NUM_WEARS)
{
	int where = -1;	// Used for the return value; can be invalid, so the
				//	 "already wearing" message can be returned for that slot.
	SInt8 okaywhere = -1; 	// Used to indicated a valid, unfilled slot.
	SInt8 i;		// Counter
	SInt8 bodytype = GET_BODYTYPE(ch);

	if (!arg || !*arg) {	// Player typed "wear object"
		for (i = bottomslot; (okaywhere < 0) && (i < topslot); i++) {
			if (CAN_WEAR(obj, GET_WEARSLOT(i, bodytype).wear_bitvector)) {
				where = i;
				if (!GET_EQ(ch, i))
					okaywhere = check_empty_slot(ch, obj, where, bodytype);
			}
		}
	} else {	// Player typed "wear object location"
		for (i = 0; (okaywhere < 0) && (i < NUM_WEARS); i++) {
			if (!strncmp(arg, GET_WEARSLOT(i, bodytype).keywords, strlen(arg))) {
				where = i;
				if (CAN_WEAR(obj, GET_WEARSLOT(i, bodytype).wear_bitvector)) {
					if (!GET_EQ(ch, i))
						okaywhere = check_empty_slot(ch, obj, where, bodytype);
				} else {
					ch->Send("You can't wear that THERE!\r\n");
					return ((int) -1);
				}
			}
		}
		if (where < 0) {
			act("'$T'?  What part of your body is THAT?", FALSE, ch, 0, arg, TO_CHAR);
		}

	}

	return ((int) where);
}


// Returns a wear slot number.	Said wear slot will be an object of type
// "seektype" between bottomslot and topslot.	If random = FALSE, the first
// wear slot found containing a valid object is return; otherwise, all slots
// are searched and one is chosen randomly out of the valid slots.
SInt8 find_eqtype_pos(Character *ch, SInt8 seektype, SInt8 bottomslot = 0,
						SInt8 topslot = NUM_WEARS - 1, bool random = FALSE)
{
	SInt8 i, numobjects = 0;

	if (topslot >= NUM_WEARS) // Sanity check.
		topslot = NUM_WEARS - 1;

	for (i = bottomslot; i <= topslot; ++i) {
		if (GET_EQ(ch, i) && (GET_OBJ_TYPE(GET_EQ(ch, i)) == seektype))
			++numobjects;
		if ((numobjects) && (random == FALSE))
			return (i);
	}

	if (numobjects < 1)
		return (-1);

		numobjects = Number(1, numobjects);

	i = bottomslot;

	while (i <= topslot) {
		if (GET_EQ(ch, i) && (GET_OBJ_TYPE(GET_EQ(ch, i)) == seektype))
			--numobjects;
		if (!numobjects)
			break;
		++i;
	}

	return (i);
}


bool can_hold(Object *obj)
{
	if (CAN_WEAR(obj, ITEM_WEAR_HOLD) ||
		CAN_WEAR(obj, ITEM_WEAR_SHIELD) ||
		GET_OBJ_TYPE(obj) == ITEM_WAND ||
		GET_OBJ_TYPE(obj) == ITEM_STAFF ||
		GET_OBJ_TYPE(obj) == ITEM_SCROLL ||
		GET_OBJ_TYPE(obj) == ITEM_POTION ||
		GET_OBJ_TYPE(obj) == ITEM_LIGHT)
		return TRUE;

	return FALSE;
}


ACMD(do_wear) {
	char *arg1 = get_buffer(MAX_INPUT_LENGTH);
	char *arg2 = get_buffer(MAX_INPUT_LENGTH);
	Object *obj;
	int where = -1, dotmode, items_worn = 0;
	SInt8 prev_encumbrance = GET_ENCUMBRANCE(ch, IS_CARRYING_W(ch)), encumbrance;

	two_arguments(argument, arg1, arg2);

	if (!*arg1)
		ch->Send("Wear what?\r\n");
	else {
		LListIterator<Object *>	iter(ch->contents);
		dotmode = find_all_dots(arg1);

		if (*arg2 && (dotmode != FIND_INDIV))
			ch->Send("You can't specify the same body location for more than one item!\r\n");
		else if (dotmode == FIND_ALL) {
			while((obj = iter.Next())) {
				if (!obj->CanBeSeen(ch))
					continue;
				if ((where = find_eq_pos(ch, obj, NULL)) >= 0) {
					items_worn++;
					perform_wear(ch, obj, where);
				} else if (CAN_WEAR(obj, ITEM_WEAR_WIELD)) {
					sprintf(arg1, "%c%d", UID_CHAR, GET_ID(obj));
					do_wield(ch, arg1, 0, "wield", 0);
				} else if (can_hold(obj)) {
					sprintf(arg1, "%c%d", UID_CHAR, GET_ID(obj));
					do_grab(ch, arg1, 0, "grab", 0);
				}
			}
			if (!items_worn)
				ch->Send("You don't seem to have anything wearable.\r\n");
		} else if (dotmode == FIND_ALLDOT) {
			if (!*arg1)
				ch->Send("Wear all of what?\r\n");
			else if (!(obj = get_obj_in_list_vis(ch, arg1, ch->contents)))
				act("You don't seem to have any $ts.\r\n", FALSE, ch, (Object *)arg1, 0, TO_CHAR);
			else {
				while ((obj = iter.Next())) {
					if (!obj->CanBeSeen(ch) || !isname(arg1, obj->Keywords()))
						continue;
					if ((where = find_eq_pos(ch, obj, NULL)) >= 0)
						perform_wear(ch, obj, where);
					else if (CAN_WEAR(obj, ITEM_WEAR_WIELD)) {
						sprintf(arg1, "%c%d", UID_CHAR, GET_ID(obj));
						do_wield(ch, arg1, 0, "wield", 0);
					} else if (can_hold(obj)) {
						sprintf(arg1, "%c%d", UID_CHAR, GET_ID(obj));
						do_grab(ch, arg1, 0, "grab", 0);
					} else
						act("You can't wear $p.", FALSE, ch, obj, 0, TO_CHAR);
				}
			}
		} else if (!(obj = get_obj_in_list_vis(ch, arg1, ch->contents)))
			ch->Sendf("You don't seem to have %s %s.\r\n", AN(arg1), arg1);
		else if ((where = find_eq_pos(ch, obj, arg2)) >= 0)
			perform_wear(ch, obj, where);
		else if (CAN_WEAR(obj, ITEM_WEAR_WIELD)) {
			sprintf(arg1, "%c%d", UID_CHAR, GET_ID(obj));
			do_wield(ch, arg1, 0, "wield", 0);
		} else if (can_hold(obj)) {
			sprintf(arg1, "%c%d", UID_CHAR, GET_ID(obj));
			do_grab(ch, arg1, 0, "grab", 0);
		} else if (!*arg2)
			act("You can't wear $p.", FALSE, ch, obj, 0, TO_CHAR);
	}
	release_buffer(arg1);
	release_buffer(arg2);

	encumbrance = GET_ENCUMBRANCE(ch, IS_CARRYING_W(ch));
	if (prev_encumbrance != encumbrance)
		ch->Send(encumbrance_shift[encumbrance][(encumbrance < prev_encumbrance)]);
}


ACMD(do_wield) {
	Object *obj;
	char *arg1 = get_buffer(MAX_INPUT_LENGTH);
	char *arg2 = get_buffer(MAX_INPUT_LENGTH);
	SInt8 prev_encumbrance = GET_ENCUMBRANCE(ch, IS_CARRYING_W(ch)), encumbrance;
	int where;

	two_arguments(argument, arg1, arg2);

	if (!*arg1)
		ch->Send("Wield what?\r\n");
	else if (!(obj = get_obj_in_list_vis(ch, arg1, ch->contents)))
		ch->Sendf("You don't seem to have %s %s.\r\n", AN(arg1), arg1);
	else if (GET_OBJ_WEIGHT(obj) > (GET_STR(ch) * (OBJ_FLAGGED(obj, ITEM_TWO_HAND) ? 4 : 2)))
			ch->Send("It's too heavy for you to use.\r\n");
	else {
		if ((where = find_eq_pos(ch, obj, arg2, WEAR_HAND_R, WEAR_HAND_4)) >= 0)
			perform_wear(ch, obj, where);
		else if (!*arg2)
			act("You can't wield $p.", FALSE, ch, obj, 0, TO_CHAR);
	}

	release_buffer(arg1);
	release_buffer(arg2);

	encumbrance = GET_ENCUMBRANCE(ch, IS_CARRYING_W(ch));
	if (prev_encumbrance != encumbrance)
		ch->Send(encumbrance_shift[encumbrance][(encumbrance < prev_encumbrance)]);
}


ACMD(do_grab) {
	Object *obj;
	char * arg1 = get_buffer(MAX_INPUT_LENGTH);
	char * arg2 = get_buffer(MAX_INPUT_LENGTH);
	SInt8 prev_encumbrance = GET_ENCUMBRANCE(ch, IS_CARRYING_W(ch)), encumbrance;

	two_arguments(argument, arg1, arg2);

	if (!*arg1)
		ch->Send("Hold what?\r\n");
	else if (!(obj = get_obj_in_list_vis(ch, arg1, ch->contents)))
		ch->Sendf("You don't seem to have %s %s.\r\n", AN(arg1), arg1);
	else if (!can_hold(obj))
		ch->Send("You can't hold that.\r\n");
	else if (OBJ_FLAGGED(obj, ITEM_TWO_HAND)) {
		if (GET_EQ(ch, WEAR_HAND_R) || GET_EQ(ch, WEAR_HAND_L))
			ch->Send("You need both hands free to hold this.\r\n");
		else
			perform_wear(ch, obj, WEAR_HAND_R);
	} else if (GET_EQ(ch, WEAR_HAND_R) && OBJ_FLAGGED(GET_EQ(ch, WEAR_HAND_R), ITEM_TWO_HAND))
		ch->Send("You're already holding something in both of your hands.\r\n");
	else if (!strn_cmp(arg2, "left", strlen(arg2)))
		perform_wear(ch, obj, WEAR_HAND_L);
	else if (!strn_cmp(arg2, "right", strlen(arg2)))
		perform_wear(ch, obj, WEAR_HAND_R);
	else
		ch->Send("You only have two hands, right and left.\r\n");
	release_buffer(arg1);
	release_buffer(arg2);

	encumbrance = GET_ENCUMBRANCE(ch, IS_CARRYING_W(ch));
	if (prev_encumbrance != encumbrance)
		ch->Send(encumbrance_shift[encumbrance][(encumbrance < prev_encumbrance)]);
}


class BurnLightEvent : public ListedEvent {
public:
						BurnLightEvent(time_t when, Object *tobj) :
						ListedEvent(when, &(tobj->events)), obj(tobj) {}
					
	Object				*obj;

	time_t				Execute(void);
};


time_t			BurnLightEvent::Execute(void)
{
	int				i = 0;
    VNum 			room = NOWHERE;
	Character 		*ch;

	ch = obj->CarriedBy();

	if (!ch)	ch = obj->WornBy();

	room = obj->AbsoluteRoom();

	if ((GET_OBJ_VAL(obj, 2) > 0) && (IS_SET(GET_OBJ_EXTRA(obj), ITEM_GLOW))) {
		i = --GET_OBJ_VAL(obj, 2);
		
		if (i == 1) {
			if (ch) {
				act("Your $o begins to flicker and fade.", FALSE, ch, obj, 0, TO_CHAR);
				act("$n's $o begins to flicker and fade.", FALSE, ch, obj, 0, TO_ROOM);
			} else if (room != NOWHERE) {
				act("$p begins to flicker and fade.", FALSE, 0, obj, 0, TO_ROOM);
			}
		} else if (i == 0) {
			if (ch) {
				act("Your $o sputters out and dies.", FALSE, ch, obj, 0, TO_CHAR);
				act("$n's $o sputters out and dies.", FALSE, ch, obj, 0, TO_ROOM);
			} else if (room != NOWHERE) {
				act("$p sputters out and dies.", FALSE, 0, obj, 0, TO_ROOM);
			}
			if (room != NOWHERE)
				world[room].ChangeLight(-1);
			REMOVE_BIT(GET_OBJ_EXTRA(obj), ITEM_GLOW);
			obj->events.remove(this);
			return 0;
		}

	}
	return (SECS_PER_MUD_HOUR RL_SEC);
}


ACMD(do_light)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	Object *obj = NULL;
	Character *tmp_char = NULL;
    Event * event;
	int tmp_light;
	struct obj_timer_data *next = NULL;

	two_arguments(argument, arg1, arg2);

	if (!*arg1) {
		switch (subcmd) {
		case SCMD_LIGHT_OBJ: ch->Send("Light what?\r\n"); break;
		case SCMD_DOUSE_OBJ: ch->Send("Douse what?\r\n"); break;
		}
		return;
	}

	tmp_light = world[IN_ROOM(ch)].Light();
	world[IN_ROOM(ch)].Light(1);

	generic_find(arg1, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP, ch, &tmp_char, &obj);

	if (!obj) {
		ch->Sendf("You don't seem to have %s %s.\r\n", AN(arg1), arg1);
	} else {
		if (GET_OBJ_TYPE(obj) == ITEM_LIGHT) {
			if (GET_OBJ_VAL(obj, 2) != 0) {
				if (subcmd == SCMD_LIGHT_OBJ) {
					if (IS_SET(GET_OBJ_EXTRA(obj), ITEM_GLOW)) { /* Light is ON */
						ch->Send("It's already lit!\r\n");
					} else {
						SET_BIT(GET_OBJ_EXTRA(obj), ITEM_GLOW);
						tmp_light++;
						act("You light $p.", FALSE, ch, obj, 0, TO_CHAR);
						act("$n lights $p.", FALSE, ch, obj, 0, TO_ROOM);
					    new BurnLightEvent(SECS_PER_MUD_HOUR * PASSES_PER_SEC, obj);
					}
				} else if (subcmd == SCMD_DOUSE_OBJ) {
					if (!IS_SET(GET_OBJ_EXTRA(obj), ITEM_GLOW)) { /* Light is ON */
						ch->Send("It's already out!\r\n");
					} else {
						REMOVE_BIT(GET_OBJ_EXTRA(obj), ITEM_GLOW);
						tmp_light--;
						act("You douse $p.", FALSE, ch, obj, 0, TO_CHAR);
						act("$n douses $p.", FALSE, ch, obj, 0, TO_ROOM);
						event = Event::Find(ch->events, typeid(BurnLightEvent));
                        if (event)
                        	event->Cancel();
					}
				}
			} else
				ch->Send("That light source seems to be expended.\r\n");
		} else {
			ch->Send("Now don't be silly!\r\n");
		}
	}

	world[IN_ROOM(ch)].Light(tmp_light);

}


void perform_remove(Character * ch, int pos)
{
	Object *obj;

	if (!(obj = GET_EQ(ch, pos)))
		log("Error in perform_remove: bad pos %d passed.", pos);
	else if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
		act("$p: you can't carry that many items!", FALSE, ch, obj, 0, TO_CHAR);
	else if (OBJ_FLAGGED(obj, ITEM_NODROP) && !IS_IMMORTAL(ch))
		act("You can't remove $p for some reason!", FALSE, ch, obj, 0, TO_CHAR);
	else if (remove_otrigger(obj, ch)) {
    	// We need to check object stats in case a script did something. -DH
		if (PURGED(obj) || PURGED(ch) || (obj->WornBy() != ch))
			return;
		unequip_char(ch, pos)->ToChar(ch);
		act("You stop using $p.", FALSE, ch, obj, 0, TO_CHAR);
		act("$n stops using $p.", TRUE, ch, obj, 0, TO_ROOM);
	}
}



ACMD(do_remove)
{
	Object *obj = NULL;
	int		i, dotmode;
	IDNum	msg;
	char	*arg = get_buffer(MAX_INPUT_LENGTH);
	bool	found = false;
	SInt8 prev_encumbrance = GET_ENCUMBRANCE(ch, IS_CARRYING_W(ch)), encumbrance;

	one_argument(argument, arg);

	if (!*arg) {
		ch->Send("Remove what?\r\n");
		release_buffer(arg);
		return;
	}

	if (!(obj = get_obj_in_list_type(ITEM_BOARD, ch->contents)))
		obj = get_obj_in_list_type(ITEM_BOARD, world[IN_ROOM(ch)].contents);

	if (obj && isdigit(*arg) && (msg = atoi(arg)))
		Board::RemoveMessage(obj->Virtual(), ch, msg);
	else {
		dotmode = find_all_dots(arg);

		if (dotmode == FIND_ALL) {
			for (i = 0; i < NUM_WEARS; i++)
				if (GET_EQ(ch, i)) {
					perform_remove(ch, i);
					found = true;
				}
			if (!found)
				ch->Send("You're not using anything.\r\n");
		} else if (dotmode == FIND_ALLDOT) {
			if (!*arg)
				ch->Send("Remove all of what?\r\n");
			else {
				for (i = 0; i < NUM_WEARS; i++)
					if (GET_EQ(ch, i) && (GET_EQ(ch, i))->CanBeSeen(ch) &&
							isname(arg, GET_EQ(ch, i)->Keywords())) {
						perform_remove(ch, i);
						found = true;
					}
				if (!found)
					ch->Sendf("You don't seem to be using any %ss.\r\n", arg);
			}
		} else if (!(obj = get_object_in_equip_vis(ch, arg, ch->equipment, &i)))
			ch->Sendf("You don't seem to be using %s %s.\r\n", AN(arg), arg);
		else
			perform_remove(ch, i);
	}
	release_buffer(arg);

	encumbrance = GET_ENCUMBRANCE(ch, IS_CARRYING_W(ch));
	if (prev_encumbrance != encumbrance)
		ch->Send(encumbrance_shift[encumbrance][(encumbrance < prev_encumbrance)]);
}


class GrenadeEvent : public ListedEvent {
public:
					GrenadeEvent(time_t when = 0, Object * tobj = NULL, Character *c = NULL) :
						ListedEvent(when, &(tobj->events)), obj(tobj), ch(c) { }
					
	Object					*obj;
	SafePtr< Character >	ch;
	
	time_t			Execute(void);
};


time_t			GrenadeEvent::Execute(void)
{
	int t = obj->AbsoluteRoom();
	
	//	grenades are activated by pulling the pin - ie, starting an
	//	event. After the pin is pulled the grenade
	//	starts counting down. once it reaches zero, it explodes. */
	
	//	then truly this grenade is nowhere?!?
	if (t <= 0) {
		log("Serious problem, grenade truly in nowhere");
	} else { /* ok we have a room to blow up */
		if (obj->CarriedBy())		obj->FromChar();
		else if (obj->WornBy())		unequip_char(obj->WornBy(), obj->worn_on);
		else if (obj->InObj())		obj->FromObj();
		else if (IN_ROOM(obj))		obj->FromRoom();
		
		obj->ToRoom(t);
		
		if (ROOM_FLAGGED(t, ROOM_PEACEFUL)) {		/* peaceful rooms */
			act("You hear $p explode harmlessly, with a loud POP!", FALSE, 0, obj, 0, TO_ROOM);
		} else {
			act("You hear a loud explosion as $p explodes!\r\n", FALSE, 0, obj, 0, TO_ROOM);

			Combat::Explosion(ch, obj->value[1], obj->value[2], t, TRUE);
		}
	}
	obj->Extract();
	return 0;
}



ACMD(do_pull)
{
	int i = 0;
	Object *grenade = 0;

	one_argument(argument, arg);

	if (!*arg) {
		ch->Send("Pull what?\r\n");
		return;
	}

/* for now only "pull pin" will work, and player must be wielding a grenade */
	if (!str_cmp(arg, "pin"))
		for (i = WEAR_HAND_R; i < NUM_WEARS; i++)
			if (GET_EQ(ch, i) && GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_GRENADE)
				grenade = GET_EQ(ch, i);

	if (grenade) {
		if (GET_OBJ_VAL(grenade, 3)) {
			ch->Send("It's already been activated!\r\n");
			return;
		}

		GET_OBJ_VAL(grenade, 3) = 1;
		act("You pull the pin, the $p is activated!", FALSE, ch, grenade, 0, TO_CHAR);

		new GrenadeEvent(GET_OBJ_VAL(grenade, 0) RL_SEC, grenade, ch);
		return;
	}

	ch->Send("Pull what?\r\n");
}


ACMD(do_reload)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	Character *tmp_char = ch;
	Object *weapon = NULL, *ammo = NULL;

	skip_spaces(argument);

	if (!*argument) {
		ch->Send("Usage for reload:	reload <weapon> <ammo>.\r\n");
		return;
	}

	two_arguments(argument, arg1, arg2);

	generic_find(arg1, FIND_OBJ_INV | FIND_OBJ_EQUIP, ch, &tmp_char, &weapon);
	if (!weapon) {
		ch->Send("You don't see that weapon here.\r\n");
		return;
	}
	if ((GET_OBJ_TYPE(weapon) != ITEM_FIREWEAPON) || (GET_OBJ_VAL(weapon, 4) == 0)) {
		ch->Send("But that is not a proper weapon!\r\n");
		return;
	}

	generic_find(arg2, FIND_OBJ_INV | FIND_OBJ_EQUIP, ch, &tmp_char, &ammo);
	if (!ammo) {
		ch->Send("You don't see those munitions here.\r\n");
		return;
	}
	if ((GET_OBJ_TYPE(ammo) != ITEM_MISSILE) || (GET_OBJ_VAL(ammo, 4) == 0)) {
		ch->Send("But that is not a proper round of ammunition!\r\n");
		return;
	}

	// Is the weapon already full?

	if (GET_OBJ_VAL(ammo, 5) == 0) {
		ch->Send("That round of ammunition is spent.\r\n");
		return;
	}

	if (GET_OBJ_VAL(ammo, 5) >
		 (GET_OBJ_VAL(weapon, 4) - GET_OBJ_VAL(weapon, 5))) {
		ch->Send("That weapon is too full to accept that many more rounds.\r\n");
		return;
	}

	GET_OBJ_VAL(weapon, 5) += GET_OBJ_VAL(ammo, 5);
	GET_OBJ_VAL(ammo, 5) = 0;

	ch->Sendf("You load %s with %s.\r\n", weapon->Name(), ammo->Name());

	ammo->Extract();
}
