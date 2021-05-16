/* ************************************************************************
*   File: objsave.c                                     Part of CircleMUD *
*  Usage: loading/saving player objects for rent and crash-save           *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */




#include "structs.h"
#include "buffer.h"
#include "descriptors.h"
#include "characters.h"
#include "objects.h"
#include "comm.h"
#include "scripts.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"
#include "files.h"


/* local functions */
void Crash_restore_weight(Object * obj);
void Crash_extract_objs(Object * obj);
int Crash_is_unrentable(Object * obj);
void Crash_extract_norents(Object * obj);
void Crash_rentsave(Character * ch);
void Crash_save(Object * obj, FILE * fp, int locate, Character *ch = NULL);
void auto_equip(Character *ch, Object *obj, int locate);
void Crash_extract_norents_from_equipped(Character * ch);
void Crash_crashsave(Character * ch);


void Crash_listrent(Character * ch, char *name) {
	FILE *fl;
	char *fname = get_buffer(MAX_INPUT_LENGTH), *buf, *line;
	Object *obj;
	SInt32	obj_vnum, location;
	bool	read_file = true;

	if (!get_filename(name, fname)) {
		release_buffer(fname);
		return;
	}
	if (!(fl = fopen(fname, "rb"))) {
		ch->Sendf("%s has no rent file.\r\n", name);
		release_buffer(fname);
		return;
	}
	
	buf = get_buffer(MAX_STRING_LENGTH);
	line = get_buffer(MAX_INPUT_LENGTH);
	
	sprintf(buf, "%s\r\n", fname);
	
	get_line(fl, line);
	while (get_line(fl, line) && read_file) {
		switch (*line) {
			case '#':
				if (sscanf(line, "#%d %d", &obj_vnum, &location) < 1) {
					log("Player object file %s is damaged: no vnum", fname);
					read_file = false;
				} else {
					if (!(obj = Object::Read(obj_vnum))) {
						obj = new Object();
						obj->SetKeywords("Empty.");
						obj->SetName("Empty.");
						obj->SetSDesc("Empty.");
					}
					if (obj->Load(fl, fname, ch)) {
						sprintf(buf + strlen(buf), " [%5d] <%2d> %-20s\r\n", obj->Virtual(),
								location, obj->Name());
					} else {
						log("Player object file %s is damaged: failed to load obj #%d", fname, obj_vnum);
						read_file = false;
					}
					obj->Extract();
				}
				break;
			case '$':
				read_file = false;
				break;
			default:
				log("Player object file %s is damaged: bad line %s", fname, line);
				read_file = false;
		}
	}
	page_string(ch->desc, buf, true);
	release_buffer(fname);
	release_buffer(buf);
	release_buffer(line);
	fclose(fl);
}


 /* so this is gonna be the auto equip (hopefully) */
void auto_equip(Character *ch, Object *obj, int locate) {
	int j;
 
	if (locate > 0) { /* was worn */
		switch (j = locate-1) {
			case WEAR_FINGER_R:
			case WEAR_FINGER_L:
			case WEAR_FINGER_3:
			case WEAR_FINGER_4:	if (!CAN_WEAR(obj,ITEM_WEAR_FINGER))	locate = 0;		break;
			case WEAR_NECK_1:	if (!CAN_WEAR(obj,ITEM_WEAR_NECK))		locate = 0;		break;
			case WEAR_NECK_2:	if (!CAN_WEAR(obj,ITEM_WEAR_NECK))		locate = 0;		break;
			case WEAR_BODY:		if (!CAN_WEAR(obj,ITEM_WEAR_BODY))		locate = 0;		break;
			case WEAR_HEAD:		if (!CAN_WEAR(obj,ITEM_WEAR_HEAD))		locate = 0;		break;
			case WEAR_FACE:		if (!CAN_WEAR(obj,ITEM_WEAR_FACE))		locate = 0;		break;
			case WEAR_EYES:		if (!CAN_WEAR(obj,ITEM_WEAR_EYES))		locate = 0;		break;
			case WEAR_EAR_R:	if (!CAN_WEAR(obj,ITEM_WEAR_EAR))		locate = 0;		break;
			case WEAR_EAR_L:	if (!CAN_WEAR(obj,ITEM_WEAR_EAR))		locate = 0;		break;
			case WEAR_LEGS:		if (!CAN_WEAR(obj,ITEM_WEAR_LEGS))		locate = 0;		break;
			case WEAR_FEET:		if (!CAN_WEAR(obj,ITEM_WEAR_FEET))		locate = 0;		break;
			case WEAR_HANDS:	if (!CAN_WEAR(obj,ITEM_WEAR_HANDS))		locate = 0;		break;
			case WEAR_HANDS_2:	if (!CAN_WEAR(obj,ITEM_WEAR_HANDS))		locate = 0;		break;
			case WEAR_ARMS:		if (!CAN_WEAR(obj,ITEM_WEAR_ARMS))		locate = 0;		break;
			case WEAR_ARMS_2:	if (!CAN_WEAR(obj,ITEM_WEAR_ARMS))		locate = 0;		break;
			case WEAR_ABOUT:	if (!CAN_WEAR(obj,ITEM_WEAR_ABOUT))		locate = 0;		break;
			case WEAR_WAIST:	if (!CAN_WEAR(obj,ITEM_WEAR_WAIST))		locate = 0;		break;
			case WEAR_BACK:		if (!CAN_WEAR(obj,ITEM_WEAR_BACK))		locate = 0;		break;
			case WEAR_ONBELT:	if (!CAN_WEAR(obj,ITEM_WEAR_ONBELT))		locate = 0;		break;
			case WEAR_WRIST_R:
			case WEAR_WRIST_L:
			case WEAR_WRIST_3:
			case WEAR_WRIST_4:	if (!CAN_WEAR(obj,ITEM_WEAR_WRIST))		locate = 0;		break;
			case WEAR_HAND_R:	
			case WEAR_HAND_L:	
			case WEAR_HAND_3:	
			case WEAR_HAND_4:	if (!CAN_WEAR(obj, ITEM_WEAR_TAKE | ITEM_WEAR_SHIELD | ITEM_WEAR_WIELD))
									locate = 0;
								break;
			default:			locate = 0;
	}
	if (locate > 0)
		if (!GET_EQ(ch,j)) {
			/* check ch's race to prevent $M from being zapped through auto-equip */
			if (!ch->CanUse(obj))
				locate = 0;
			else
				equip_char(ch, obj, j);
		} else  /* oops - saved player with double equipment[j]? */
			locate = 0;
	}
	if (locate <= 0)
		obj->ToChar(ch);
}
 
 
 
#define MAX_BAG_ROW 5
/* should be enough - who would carry a bag in a bag in a bag in a bag in a bag in a bag ?!? */


int Crash_load(Character * ch) {
/* return values:
	0 - successful load, keep char in rent room.
	1 - load failure or load of crash items -- put char in temple.
*/
	FILE *fl;
	char *fname = get_buffer(MAX_STRING_LENGTH);
	Object *obj, *obj1;
	LList<Object *>	cont_row[MAX_BAG_ROW];
	SInt32	locate, j, renttime, obj_vnum;
	bool	read_file = true;
//	bool	result = true;
	char *line;

	if (!get_filename(ch->Name(), fname)) {
		release_buffer(fname);
		return 1;
	}
	if (!(fl = fopen(fname, "r+b"))) {
		if (errno != ENOENT) {	/* if it fails, NOT because of no file */
			log("SYSERR: READING OBJECT FILE %s (5)", fname);
			ch->Send("\r\n********************* NOTICE *********************\r\n"
					 "There was a problem loading your objects from disk.\r\n"
					 "Contact Staff for assistance.\r\n");
		}
		mudlogf(NRM, ch, TRUE,  "%s entering game with no equipment.", ch->RealName());
		release_buffer(fname);
		return 1;
	}
	
	line = get_buffer(MAX_INPUT_LENGTH);
	if (get_line(fl, line))
		renttime = atoi(line);

	mudlogf( NRM, ch, TRUE,  "%s entering game.", ch->RealName());

//	for (j = 0;j < MAX_BAG_ROW;j++)
//		cont_row[j] = NULL; /* empty all cont lists (you never know ...) */

	while (get_line(fl, line) && read_file) {
		switch (*line) {
			case '#':
				locate = 0;
				if (sscanf(line, "#%d %d", &obj_vnum, &locate) < 1) {
					log("Player object file %s is damaged: no vnum", fname);
					read_file = false;
				} else {
					if ((obj_vnum == -1) || !Object::Find(obj_vnum)) {
						obj = new Object();
						obj->SetKeywords("Empty.");
						obj->SetName("Empty.");
						obj->SetSDesc("Empty.\r\n");
					} else
						obj = Object::Read(obj_vnum);
					
					read_file = obj->Load(fl, fname, ch);
					auto_equip(ch, obj, locate);

					if (locate > 0) { /* item equipped */
						for (j = MAX_BAG_ROW-1;j > 0;j--)
							if (cont_row[j].Count()) { /* no container -> back to ch's inventory */
//								for (;cont_row[j];cont_row[j] = obj1) {
//									obj1 = cont_row[j]->next_content;
//									cont_row[j]->ToChar(ch);
//								}
//								cont_row[j] = NULL;
								while((obj1 = cont_row[j].Top())) {
									cont_row[j].Remove(obj1);
									obj1->ToChar(ch);
								}
							}
						if (cont_row[0].Count()) {		//	content list existing
							if (IS_CONTAINER(obj)) {
								/* rem item ; fill ; equip again */
								if (GET_EQ(ch, locate - 1))		// KLUDGE!  Just avoiding a bug.  TODO:  Fix it.
									obj = unequip_char(ch, locate-1);
//								for (;cont_row[0];cont_row[0] = obj1) {
//									obj1 = cont_row[0]->next_content;
//									cont_row[0]->ToObj(obj);
//								}
								while((obj1 = cont_row[0].Top())) {
									cont_row[0].Remove(obj1);
									if (obj)	obj1->ToObj(obj);
									else		obj1->ToChar(ch);
								}
								if (obj)	equip_char(ch, obj, locate-1);
							} else { /* object isn't container -> empty content list */
//								for (;cont_row[0];cont_row[0] = obj1) {
//									obj1 = cont_row[0]->next_content;
//									cont_row[0]->ToChar(ch);
//								}
//								cont_row[0] = NULL;
								while((obj1 = cont_row[0].Top())) {
									cont_row[0].Remove(obj1);
									obj1->ToChar(ch);
								}
							}
						}
					} else { /* locate <= 0 */
						for (j = MAX_BAG_ROW-1;j > -locate;j--)
							if (cont_row[j].Count()) { /* no container -> back to ch's inventory */
//								for (;cont_row[j];cont_row[j] = obj1) {
//									obj1 = cont_row[j]->next_content;
//									cont_row[j]->ToChar(ch);
//								}
//								cont_row[j] = NULL;
								while((obj1 = cont_row[j].Top())) {
									cont_row[j].Remove(obj1);
									obj1->ToChar(ch);
								}
							}

						if (j == -locate && cont_row[j].Count()) { /* content list existing */
							if (IS_CONTAINER(obj)) {
								/* take item ; fill ; give to char again */
								obj->FromChar();
//								for (;cont_row[j];cont_row[j] = obj1) {
//									obj1 = cont_row[j]->next_content;
//									cont_row[j]->ToObj(obj);
//								}
								while ((obj1 = cont_row[j].Top())) {
									cont_row[j].Remove(obj1);
									obj1->ToObj(obj);
								}
								obj->ToChar(ch); /* add to inv first ... */
							} else { /* object isn't container -> empty content list */
//								for (;cont_row[j];cont_row[j] = obj1) {
//									obj1 = cont_row[j]->next_content;
//									cont_row[j]->ToChar(ch);
//								}
//								cont_row[j] = NULL;
								while ((obj1 = cont_row[j].Top())) {
									cont_row[j].Remove(obj1);
									obj1->ToChar(ch);
								}
							}
						}

						if (locate < 0 && locate >= -MAX_BAG_ROW) {
							/*	let obj be part of content list but put it at the list's end
							 *	thus having the items
							 *	in the same order as before renting
							 */
							obj->FromChar();
//							if ((obj1 = cont_row[-locate-1].Top())) {
//								while (obj1->next_content)
//									obj1 = obj1->next_content;
//								obj1->next_content = obj;
//							} else
//								cont_row[-locate-1] = obj;
							cont_row[-locate-1].Prepend(obj);
						}
					}
				}
				break;
			case '$':
				read_file = false;
				break;
			default:
				log("Player object file %s is damaged: bad line %s", fname, line);
				read_file = false;
				fclose(fl);
				return 1;
		}
	}
	fclose(fl);

	release_buffer(fname);
	release_buffer(line);
	
	return 0;
}


void Crash_save(Object * obj, FILE * fp, int locate, Character *ch = NULL) {
	Object *tmp;

	START_ITER(iter, tmp, Object *, obj->contents)
		Crash_save(tmp, fp, MIN(0,locate)-1, ch);
	obj->Save(fp, locate, ch);
	for (tmp = obj->InObj(); tmp; tmp = tmp->InObj())
		GET_OBJ_WEIGHT(tmp) -= GET_OBJ_WEIGHT(obj);
}


void Crash_restore_weight(Object * obj) {
	Object *tmp;
	START_ITER(iter, tmp, Object *, obj->contents)
		Crash_restore_weight(tmp);
	if (obj->InObj())
		GET_OBJ_WEIGHT(obj->InObj()) += GET_OBJ_WEIGHT(obj);
}



void Crash_extract_objs(Object * obj) {
	Object *tmp;
	START_ITER(iter, tmp, Object *, obj->contents)
		Crash_extract_objs(tmp);
	obj->Extract();
}


int Crash_is_unrentable(Object * obj) {
	if (!obj)							return 0;
	if (OBJ_FLAGGED(obj, ITEM_NORENT))	return 1;
	return 0;
}


void Crash_extract_norents(Object * obj) {
	Object *tmp;
	START_ITER(iter, tmp, Object *, obj->contents)
		Crash_extract_norents(tmp);
	if (Crash_is_unrentable(obj))
		obj->Extract();
}


/* get norent items from eq to inventory and extract norents out of worn containers */
void Crash_extract_norents_from_equipped(Character * ch) {
	int j;

	for (j = 0;j < NUM_WEARS;j++) {
		if (GET_EQ(ch,j)) {
			if (Crash_is_unrentable(GET_EQ(ch, j)))
				unequip_char(ch,j)->ToChar(ch);
			else
				Crash_extract_norents(GET_EQ(ch,j));
		}
	}
}


void Crash_crashsave(Character * ch) {
	char *buf;
	int j;
	FILE *fp;
	Object *tmp;

	if (IS_NPC(ch))
		return;
	buf = get_buffer(MAX_INPUT_LENGTH);
	if (!get_filename(ch->Name(), buf)) {
		release_buffer(buf);
		return;
	}
	if (!(fp = fopen(buf, "w"))) {
		release_buffer(buf);
		return;
	}
	
	release_buffer(buf);
	
	fprintf(fp, "%ld\n", time(0));

	for (j = 0; j < NUM_WEARS; j++)
		if (GET_EQ(ch,j)) {
			Crash_save(GET_EQ(ch,j), fp, j+1, ch);
			Crash_restore_weight(GET_EQ(ch, j));
		}
	
	LListIterator<Object *>	iter(ch->contents);
	
	while ((tmp = iter.Next())) {
		Crash_save(tmp, fp, 0, ch);
		Crash_restore_weight(tmp);
	}
	
//	START_ITER(save_iter, tmp, Object *, ch->contents) {
//		Crash_save(tmp, fp, 0);
//	} END_ITER(save_iter);
//	START_ITER(weight_iter, tmp, Object *, ch->contents) {
//		Crash_restore_weight(tmp);
//	} END_ITER(weight_iter);

	fclose(fp);
	REMOVE_BIT(PLR_FLAGS(ch), PLR_CRASH);
}


void Crash_rentsave(Character * ch) {
	char *buf;
	int j;
	FILE *fp;
	Object *tmp;

	if (IS_NPC(ch))
		return;
    
	buf = get_buffer(MAX_INPUT_LENGTH);
	if (!get_filename(ch->Name(), buf)) {
		release_buffer(buf);
		return;
	}
	if (!(fp = fopen(buf, "wb"))) {
		release_buffer(buf);
		return;
	}
	release_buffer(buf);
  
	Crash_extract_norents_from_equipped(ch);

	LListIterator<Object *>	iter(ch->contents);

	while ((tmp = iter.Next())) {
		Crash_extract_norents(tmp);
	}

	fprintf(fp, "%ld\n", time(0));

	for (j = 0; j < NUM_WEARS; j++)
		if (GET_EQ(ch,j)) {
			Crash_save(GET_EQ(ch,j), fp, j+1, ch);
			Crash_restore_weight(GET_EQ(ch,j));
			Crash_extract_objs(GET_EQ(ch,j));
		}
	
	iter.Reset();
	while((tmp = iter.Next())) {
		Crash_save(tmp, fp, 0, ch);
		Crash_extract_objs(tmp);
	}
	
	fclose(fp);
}


void Crash_save_all(void) {
	Descriptor *d;
	START_ITER(iter, d, Descriptor *, descriptor_list) {
		if ((STATE(d) == CON_PLAYING) && !IS_NPC(d->character)) {
			if (PLR_FLAGGED(d->character, PLR_CRASH)) {
				Crash_crashsave(d->character);
				d->character->Save(d->character->AbsoluteRoom());
				REMOVE_BIT(PLR_FLAGS(d->character), PLR_CRASH);
			}
		}
	}
}
