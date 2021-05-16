/************************************************************************
 * OasisOLC - oedit.c                                              v1.5 *
 * Copyright 1996 Harvey Gilpin.                                        *
 * Original author: Levork                                              *
 ************************************************************************/


#include "characters.h"
#include "descriptors.h"
#include "objects.h"
#include "rooms.h"
#include "comm.h"
#include "utils.h"
#include "scripts.h"
#include "db.h"
#include "boards.h"
#include "shop.h"
#include "olc.h"
#include "interpreter.h"
#include "buffer.h"
#include "extradesc.h"
#include "constants.h"
#include "skills.h"
#include "combat.h"

#include "oedit.h"


/*------------------------------------------------------------------------*/

//	External variable declarations.
extern struct zone_data *zone_table;
extern int top_of_zone_table;

extern const char *weapon_types[];
extern const char *vehicle_bits[];


/*------------------------------------------------------------------------*/


int oedit_wizard_type_menu(Descriptor *d, Object *obj);
int oedit_wizard_type_parse(Descriptor *d, const char *arg, Object *obj);


//	Handy macros.
#define S_PRODUCT(s, i) ((s)->producing[(i)])
#define OBJTYPE_PROMPT (obj_values_tab[GET_OBJ_TYPE(obj)][mode - Object::ObjVal1].prompt)

/*------------------------------------------------------------------------*/


OEdit::OEdit(Descriptor *d, VNum newVNum) : Editor(d), desc(NULL) {
	obj = new Object;
	
	obj->SetKeywords("unfinished object");
	obj->SetSDesc("An unfinished object is lying here.");
	obj->SetName("an unfinished object");

	obj->Virtual(number = newVNum);
	
	func = NULL;

	Menu();
}


OEdit::OEdit(Descriptor *d, VNum newVNum, IndexData<Object> &objEntry) : Editor(d), triggers(objEntry.triggers), desc(NULL) {
	Object *proto = objEntry.proto;

	obj = new Object(*proto);

	if (obj->Virtual() != newVNum) {
		if (Object::Find(newVNum))		save = OLC_SAVE_OVERWRITE;
		else							save = OLC_SAVE_COPY;
	}

	func = Object::Index[obj->Virtual()].func;

	obj->Virtual(number = newVNum);
	
	//	Copy all strings over.
	obj->SetKeywords(proto->SSKeywords() ? proto->Keywords() : "undefined");
	obj->SetName(proto->SSName() ? proto->Name() : "undefined");
	obj->SetSDesc(proto->SSSDesc() ? proto->SDesc() : "undefined");

	//	Extra descriptions if necessary.
	obj->exDesc.Erase();
	if (proto->exDesc.Count()) {
		ExtraDesc *	extradesc;
		LListIterator<ExtraDesc *>	descIter(proto->exDesc);
		while ((extradesc = descIter.Next())) {
			obj->exDesc.Append(new ExtraDesc(*extradesc));
		}
	}

	triggers = objEntry.triggers;

	Menu();
}


#define ZCMD zone_table[zone].cmd[cmd_no]

void OEdit::SaveInternally(void) {
//	int		zone, cmd_no, shop;
	bool	found = false;
	Object	*old, *listObject;
//	Descriptor *dsc;
	LListIterator<Object *>	iter(Objects);
	
	REMOVE_BIT(GET_OBJ_EXTRA(obj), ITEM_DELETED);

	/* write to internal tables */
	if (Object::Find(number)) {
		delete Object::Index[number].proto;
		Object::Index[number].func = func;
		Object::Index[number].triggers = triggers;
		Object::Index[number].proto = obj;
	} else {
		Object::Index[number].vnum = number;
		Object::Index[number].number = 0;
		Object::Index[number].func = func;
		Object::Index[number].triggers = triggers;
		Object::Index[number].proto = obj;
	}

	if (GET_OBJ_TYPE(obj) == ITEM_BOARD)
		Board::Save(number);

	olc_add_to_save_list(zone_table[zoneNum].number, OLC_OEDIT);
}


void OEdit::Finish(FinishMode mode) {
	if (mode == All)		delete obj;
	
	if (d->character) {
		STATE(d) = CON_PLAYING;
		act("$n stops using OLC.", TRUE, d->character, 0, 0, TO_ROOM);
	}
	Editor::Finish();
}


/**************************************************************************
 Menu functions 
 **************************************************************************/

/* For container flags */
void OEdit::DispContainerFlagsMenu(void) {
	char *buf = get_buffer(MAX_STRING_LENGTH);


	sprintbit(GET_OBJ_VAL(obj, 1), container_bits, buf);
	d->Writef(
#if defined(CLEAR_SCREEN)
			"\x1B[H\x1B[J"
#endif
			"&cG1&c0) CLOSEABLE\r\n"
			"&cG2&c0) PICKPROOF\r\n"
			"&cG3&c0) CLOSED\r\n"
			"&cG4&c0) LOCKED\r\n"
			"Container flags: &cC%s&c0\r\n"
			"Enter flag, 0 to quit : ",
			buf);
	release_buffer(buf);
}


/* For extra descriptions */
void OEdit::DispExtradescMenu(void) {
	bool	next = false;
	
	LListIterator<ExtraDesc *>	descIter(obj->exDesc);
	ExtraDesc *	temp;
	while ((temp = descIter.Next())) {
		if (temp == desc) {
			next = descIter.Peek();
			break;
		}
	}
	
	d->Writef(
#if defined(CLEAR_SCREEN)
			"\x1B[H\x1B[J"
#endif
			"-- Extra desc menu\r\n"
			"&cG1&c0) Keyword: &cY%s\r\n"
			"&cG2&c0) Description:\r\n"
			"&cY%s\r\n"
			"&cG3&c0) Goto next description: %s\r\n"
			"&cG0&c0) Quit\r\n"
			"Enter choice : ",
			SSData(desc->keyword) ? SSData(desc->keyword) : "<NONE>",
			SSData(desc->description) ? SSData(desc->description) : "<NONE>",
			next ? "Set." : "<NOT SET>");

	mode = ExtradescMenu;
}

/* Ask for *which* apply to edit */
void OEdit::DispPromptApplyMenu(void) {
	char *buf = get_buffer(256);
	
#if defined(CLEAR_SCREEN)
	d->Write("\x1B[H\x1B[J");
#endif
	for (SInt32 counter = 0; counter < MAX_OBJ_AFFECT; counter++) {
		if (obj->affectmod[counter].modifier) {
			sprinttype(obj->affectmod[counter].location, apply_types, buf);
			d->Writef(" &cG%d&c0) &cY%+d &c0to &cY%s\r\n",
					counter + 1, obj->affectmod[counter].modifier, buf);
		} else {
			d->Writef(" &cG%d&c0) &cCNone.\r\n", counter + 1);
		}
	}
	release_buffer(buf);
	
	d->Write("\r\n&c0Enter affection to modify (0 to quit): ");
	
	mode = PromptApply;
}

/* The actual apply to set */
void OEdit::DispApplyMenu(void) {
	SInt32	columns = 0;
	
#if defined(CLEAR_SCREEN)
	d->Write("\x1B[H\x1B[J");
#endif
	for (SInt32 counter = 0; counter < NUM_APPLIES; counter++) {
		d->Writef("&cG%2d&c0) &cC%-20.20s %s", counter, apply_types[counter],
					!(++columns % 2) ? "\r\n" : "");
	}
	d->Write("\r\n&c0Enter apply type (0 is no apply): ");
	
	mode = ApplyMenu;
}


void OEdit::DispVehicleFlagsMenu(void) {
	char *buf = get_buffer(MAX_STRING_LENGTH);

	sprintbit(GET_OBJ_VAL(obj, 1), vehicle_bits, buf);
	d->Writef(
#if defined(CLEAR_SCREEN)
			"\x1B[H\x1B[J"
#endif
			"&cG1&c0) Ground (rough terrain)\r\n"
			"&cG2&c0) Air\r\n"
			"&cG3&c0) Space (Upper atmosphere)\r\n"
			"&cG4&c0) Deep space (Interstellar space)\r\n"
			"&cG5&c0) Water surface\r\n"
			"&cG6&c0) Underwater\r\n"
			"Vehicle flags: &cC%s&c0\r\n"
			"Enter flag, 0 to quit : ",
			buf);
	release_buffer(buf);
}


void OEdit::DispValues(void) {
	char selection[80], *ptr;
	SInt8 objtype = GET_OBJ_TYPE(obj);
	SInt8 objval = 0;
	const char **values = NULL;
	
	*buf = '\0';

	while (objval < NUM_OBJ_VALUES) {
		if (*obj_values_tab[objtype][objval].prompt != '\n') {
			strcpy(selection, obj_values_tab[objtype][objval].prompt);
			if ((ptr = strchr(selection, '(')) != NULL)		*ptr = '\0';
		
			sprintf(buf + strlen(buf), "&cG%d&c0) %-30s : ", 
							objval + 1, selection);
				
			values = obj_values_tab[objtype][objval].values;
	
			switch (obj_values_tab[objtype][objval].type) {
			case Datatypes::Value:
			case Datatypes::SpellValue:
				if (GET_OBJ_VAL(obj, objval) < 0)	strcat(buf, "&cC-\r\n");
				else {
					sprintf(buf + strlen(buf), "&cC%s\r\n", values[GET_OBJ_VAL(obj, objval)]);
				}
				break;
			case Datatypes::DiffValue:
				if (GET_OBJ_VAL(obj, objval) < 0)	strcat(buf, "&cC-\r\n");
				else {
					sprintf(buf + strlen(buf), "&cC%s\r\n", 
					values[GET_OBJ_VAL(obj, objval) + obj_values_tab[objtype][objval].min]);
				}
				break;
			default:
				sprintf(buf + strlen(buf), "&cC%d\r\n", GET_OBJ_VAL(obj, objval));
				break;
			}
		}
		++objval;
	}

	d->Write(buf);
}


void OEdit::DispValueMenu(void) {
	mode = ValueMenu;
	d->Write("&cYObject Values&c0\r\n\n");

	DispValues();
	d->Write(
			"\r\n&cGQ&c0) Exit Value Editor\r\n"
			"Choose a value: ");
}


void OEdit::DispTypesMenu(void) {
#if defined(CLEAR_SCREEN)
	d->Write("\x1B[H\x1B[J");
#endif
	mode = Object::ObjType;
	obj->Edit(d, "", mode, "");
	d->Write("\r\n&cGQ&c0) Exit Type Menu\r\n"
				"Choose an object type: ");
}


void OEdit::DispExtraMenu(void) {
#if defined(CLEAR_SCREEN)
	d->Write("\x1B[H\x1B[J");
#endif
	mode = Object::ObjExtraFlags;
	obj->Edit(d, "", mode, "");
}


void OEdit::DispWearMenu(void) {
#if defined(CLEAR_SCREEN)
	d->Write("\x1B[H\x1B[J");
#endif
	mode = Object::ObjWearFlags;
	obj->Edit(d, "", mode, "");
}


/* display main menu */
void OEdit::Menu(void) {
	/*. Build buffers for first part of menu .*/
	sprinttype(GET_OBJ_TYPE(obj), item_types, buf1);
	sprintbit(GET_OBJ_EXTRA(obj), extra_bits, buf2);
	sprintbit(GET_OBJ_WEAR(obj), wear_bits, buf3);

	/*. Build first half of menu .*/
	d->Writef(
#if defined(CLEAR_SCREEN)
				"\x1B[H\x1B[J"
#endif
				"-- Item Number:  [&cC%d&c0]    %s%s\r\n"
				"&cG1&c0) Keywords : &cY%s\r\n"
				"&cG2&c0) Name     : &cY%s\r\n"
				"&cG3&c0) S-Desc   : &cY%s\r\n"
//				"&cG4&c0) A-Desc   :-\r\n&cY%s\r\n"
				"&cG4&c0) Type        : &cC%s\r\n"
				"&cG5&c0) Extra flags : &cC%s\r\n"
				"&cG6&c0) Wear flags  : &cC%s\r\n",
		number, 
		(save == OLC_SAVE_COPY) ? "&cW&bb[ Copy ]&c0" : "",
		(save == OLC_SAVE_OVERWRITE) ? "&cW&br[ OVERWRITING ]&c0" : "",
		obj->Keywords(), obj->Name(), obj->SDesc(),
//		SSData(obj->actionDesc) ?  SSData(obj->actionDesc) : "<not set>",
		buf1, buf2, buf3);
	/*. Build second half of menu .*/


	d->Writef(
				"&cG7&c0) Weight      : &cC%-14d &cGA&c0) Passive Defense : &cC%d\r\n"
				"&cG8&c0) Cost        : &cC%-14d &cGB&c0) Damage Resist   : &cC%d\r\n"
				"&cG9&c0) Timer       : &cC%-14d &cGC&c0) Material        : &cC%s\r\n",
			GET_OBJ_WEIGHT(obj), 
			GET_PD(obj), 
			GET_OBJ_COST(obj), 
			GET_DR(obj), 
			GET_OBJ_TIMER(obj), 
			material_types[GET_MATERIAL(obj)]);

	d->Writef(
				"&cGD&c0) Values      : &cC%d %d %d %d %d %d %d %d\r\n",
			GET_OBJ_VAL(obj, 0), GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2), GET_OBJ_VAL(obj, 3),
			GET_OBJ_VAL(obj, 4), GET_OBJ_VAL(obj, 5), GET_OBJ_VAL(obj, 6), GET_OBJ_VAL(obj, 7));
	
	if (d->Original() && STF_FLAGGED(d->Original(), Staff::OLCAdmin)) {
		d->Write("&cGE&c0) Applies menu\r\n");
		sprintbit(obj->affectbits, affected_bits, buf1);
		d->Writef("&cGF&c0) Affects     : &cC%s\r\n", buf1);
	}

	d->Writef(
				"&cGG&c0) Extra descriptions menu\r\n"
				"&cGP&c0) Special procedure : &cC%s\r\n",
					(func == NULL) ? "None" : spec_obj_name(func));
  	
	*buf1 = '\0';
	for (int i = 0; i < triggers.Count(); ++i) {
		sprintf(buf1 + strlen(buf1), "%d ", triggers[i]);
	}
	d->Writef("&cGS&c0) Scripts     : &cC%s\r\n"
				 "&cGQ&c0) Quit\r\n"
				 "Enter choice : ",
			*buf1 ? buf1 : "None");

	mode = MainMenu;
}

/*-------------------------------------------------------------------*/
/*. Display aff-flags menu .*/

void OEdit::DispAffectsMenu(void) {
#if defined(CLEAR_SCREEN)
	d->Write("\x1B[H\x1B[J");
#endif
	mode = Object::ObjAffFlags;
	obj->Edit(d, "", mode, "");
}


/*-------------------------------------------------------------------*/
/*. Display scripts attached .*/

void OEdit::DispScriptsMenu(void) {
	d->Write("-- Scripts :\r\n");
	for (int i = 0; i < triggers.Count(); i++) {
		Trigger *	trigger = Trigger::Find(triggers[i]) ? Trigger::Index[triggers[i]].proto : NULL;
		d->Writef("&cG%2d&c0) [&cC%5d&c0] %s\r\n", i + 1, triggers[i], trigger ?
				(SSData(trigger->name) ? SSData(trigger->name) : "(Unnamed)") : "(Nonexistant)");
	}
	if (!triggers.Count())
		d->Write("No scripts attached.\r\n");
	d->Write("&cGA&c0) Attach Script\r\n"
			 "&cGR&c0) Detach Script\r\n"
			 "&cGQ&c0) Exit Menu\r\n"
			 "Enter choice:  ");
	mode = TriggersMenu;
}


/***************************************************************************
 main loop (of sorts).. basically interpreter throws all input to here
 ***************************************************************************/


void OEdit::Parse(char *arg) {
	int max_val, min_val, i;
	Editor * tmpeditor = NULL;
	
	switch (mode) {
		case ConfirmSave:
			switch (*arg) {
				case 'y':
				case 'Y':
					d->Write("Saving object to memory.\r\n");
					mudlogf(NRM, d->character, true,  "OLC: %s edits obj %d", d->character->RealName(), number);
					SaveInternally();
					Finish(Structs);
					break;
				case 'n':
				case 'N':
					/*. Cleanup all .*/
					Finish(All);
					break;
				default:
					d->Write("Invalid choice!\r\n");
					switch (save) {
					case OLC_SAVE_COPY:
						d->Write("&cYDo you wish to save this copied object internally? : &c0");
						break;
					case OLC_SAVE_OVERWRITE:
						d->Write("&cRYou're about to copy over a previously existing object!\a&c0\r\n");
					default:
						d->Write("Do you wish to save this object internally? : ");
					}
					return;
			}
			return;
		case MainMenu:
			/* throw us out to whichever edit mode based on user input */
			switch (*arg) {
				case 'q':
				case 'Q':
					if (save) { /*. Something has been modified .*/
						d->Write("Do you wish to save this object internally? (y/n) : ");
						mode = ConfirmSave;
					} else 
						Finish(All);
					return;
				case '1':
					d->Write("\r\nEnter keywords :\r\n] ");
					mode = Object::ObjKeywords;
					break;
				case '2':
					d->Write("\r\nEnter name :\r\n] ");
					mode = Object::ObjName;
					break;
				case '3':
					d->Write("\r\nEnter short desc :\r\n] ");
					mode = Object::ObjSDesc;
					--i;
					break;
// 				case '4':
// 					d->Write("\r\nEnter action desc (SPECIAL PURPOSE) :\r\n] ");
// 					mode = Object::ObjADesc;
// 					if (!save)	save = OLC_SAVE_YES;
// 						obj->Edit(d, "", mode, "");
// 					return;
				case '4':
					DispTypesMenu();
					break;
				case '5':
					DispExtraMenu();
					break;
				case '6':
					DispWearMenu();
					break;
				case '7':
					d->Write("Enter weight: ");
					mode = Object::ObjWeight;
					break;
				case '8':
					d->Write("Enter cost: ");
					mode = Object::ObjCost;
					break;
				case '9':
					d->Write("Enter timer: ");
					mode = Object::ObjTimer;
					break;
				case 'a':
				case 'A':
					d->Write("Enter passive defense (ability to deflect objects): ");
					mode = Object::ObjPD;
					break;
				case 'b':
				case 'B':
					d->Write("Enter damage resistance (absorption of damage): ");
					mode = Object::ObjDR;
					break;
				case 'c':
				case 'C':
					mode = Object::ObjMaterial;
					obj->Edit(d, "", mode, "");
					d->Write("Enter material type: ");
					break;
				case 'd':
				case 'D':
					DispValueMenu();
					break;
				case 'e':
				case 'E':
					if (d->Original() && STF_FLAGGED(d->Original(), Staff::OLCAdmin))
						DispPromptApplyMenu();
					else
						Menu();
					break;
				case 'f':
				case 'F':
					if (d->Original() && STF_FLAGGED(d->Original(), Staff::OLCAdmin))
						DispAffectsMenu();
					else
						Menu();
					break;
				case 'g':
				case 'G':
					/* if extra desc doesn't exist . */
					if (!obj->exDesc.Count()) {
						obj->exDesc.Append(new ExtraDesc);
					}
					desc = obj->exDesc.Top();
					DispExtradescMenu();
					break;
				case 'p':
				case 'P':
					DispSpecProcMenu(d, obj);
					mode = SpecProcMenu;
					d->Write("Enter the name of the desired specproc, or \"none\" to clear: ");
					return;
				case 's':
				case 'S':
					DispScriptsMenu();
					break;
				default:
					Menu();
					break;
			}
			if (!save)	save = OLC_SAVE_YES;
			return;			/* end of OEDIT_MAIN_MENU */

		case Object::ObjKeywords:
			obj->SetKeywords(arg);
			break;

		case Object::ObjName:
			obj->SetName(arg);
			break;

		case Object::ObjSDesc:
			obj->SetSDesc(arg);
			break;

		case Object::ObjCost:
		case Object::ObjWeight:
		case Object::ObjTimer:
		case Object::ObjPD:
		case Object::ObjDR:
		case Object::ObjMaterial:
			switch (*arg && obj->Edit(d, "", mode, arg)) {
			case 0: break;
			default: 
				if (!save)	save = OLC_SAVE_YES;
			}
			Menu(); 
			return;

		case Object::ObjType:
			switch (*arg && obj->Edit(d, "", mode, arg)) {
			case 0: break;
			default: 
				if (!save)	save = OLC_SAVE_YES;
				
			}
			if (oedit_wizard_type_menu(d, obj)) {
				mode = WizardType;
			} else {
				Menu();
			}
			return;

		case Object::ObjExtraFlags:
		case Object::ObjWearFlags:
		case Object::ObjAffFlags:
			switch (*arg && obj->Edit(d, "", mode, arg)) {
				case 0: Menu(); return;
				default: 
					if (!save)	save = OLC_SAVE_YES;
					obj->Edit(d, "", mode, "");
					return;
			}

		case ValueMenu:
			switch (*arg) {
			case '0':
			case 'q':
			case 'Q': Menu(); return;
			}
	
			value = atoi(arg);

			if ((value >= 1) && (value <= 8)) {
				mode = Object::ObjVal1 - 1 + value;
				if (*OBJTYPE_PROMPT == '\n') {
					DispValueMenu(); return;
				}
				obj->Edit(d, "", mode, "");
				d->Writef("Enter value - %s : ", OBJTYPE_PROMPT);
				return;
			}
			break;
  
		case Object::ObjVal1:
		case Object::ObjVal2:
		case Object::ObjVal3:
		case Object::ObjVal4:
		case Object::ObjVal5:
		case Object::ObjVal6:
		case Object::ObjVal7:
		case Object::ObjVal8:
			switch (*arg) {
				case '\0':
				case 'q':
				case 'Q': DispValueMenu(); return;
			}
			if (!save)
				save = OLC_SAVE_YES;
				obj->Edit(d, "", mode, arg);
			DispValueMenu();
			return;
			
		case PromptApply:
			i = atoi(arg);
			if (i == 0)
				break;
			else if (i < 0 || i > MAX_OBJ_AFFECT)
				DispPromptApplyMenu();
			else {
				value = i - 1;
				mode = ApplyMenu;
				DispApplyMenu();
			}
			return;

		case ApplyMenu:
			i = atoi(arg);
			if (i == 0) {
				obj->affectmod[value].location = Affect::None;
				obj->affectmod[value].modifier = 0;
				DispPromptApplyMenu();
			} else if (i < 0 || i >= NUM_APPLIES)
				DispApplyMenu();
			else {
				obj->affectmod[value].location = static_cast<Affect::Location>(i);
				d->Write("Enter value: ");
				mode = ApplyMod;
			}
			return;

		case ApplyMod:
			i = atoi(arg);
			obj->affectmod[value].modifier = i;
			DispPromptApplyMenu();
			if (!save)	save = OLC_SAVE_YES;
			return;

		case WizardType:
			if (oedit_wizard_type_parse(d, arg, obj)) {
				if (!save)	save = OLC_SAVE_YES;
				Menu();
			} else {
				oedit_wizard_type_menu(d, obj);
			}
				return;

		case ExtradescKey:
			SSFree(desc->keyword);
			if (*arg)	desc->keyword = SString::Create(arg);
			else		desc->keyword = NULL;
			DispExtradescMenu();
			return;

		case ExtradescMenu:
			i = atoi(arg);
			switch (i) {
				case 0:	/* if something got left out */
					if (!SSData(desc->keyword) || !SSData(desc->description)) {
						obj->exDesc.Remove(desc);
						desc->Free();
						desc = NULL;
					}
/*
					if (!SSData(desc->keyword) || !SSData(desc->description)) {
						ExtraDesc **tmp_desc;

//						SSFree(desc->keyword);
//						SSFree(desc->description);

						for(tmp_desc = &(obj->exDesc); *tmp_desc; tmp_desc = &((*tmp_desc)->next))
							if(*tmp_desc == desc) {
								*tmp_desc = NULL;
								break;
							}
						delete desc;
						desc = NULL;
					}
*/
					break;

				case 1:
					mode = ExtradescKey;
					d->Write("Enter keywords, separated by spaces :-\r\n| ");
					return;

				case 2:
					mode = ExtradescDesc;
					d->Write("Enter the extra description: (/s saves /h for help)\r\n\r\n");
					tmpeditor = new SStringEditor(d, &(desc->description), MAX_MESSAGE_LENGTH);
					d->Edit(tmpeditor);
					if (!save)	save = OLC_SAVE_YES;
					return;

				case 3:
					if (!SSData(desc->keyword) || !SSData(desc->description))
						d->Write("You can't edit the next extra desc without completing this one.\r\n");
					else {
						ExtraDesc *	newDesc = new ExtraDesc;
						obj->exDesc.InsertAfter(newDesc, desc);
						desc = newDesc;
					}
				/*. No break - drop into default case .*/
				default:
					DispExtradescMenu();
					return;
			}
			break;
			
		case TriggersMenu:
		case TriggerAdd:
		case TriggerPurge:
			ParseScriptsMenu(d, arg, triggers);
			return;
		
		case SpecProcMenu:
			if (*arg) {
				ParseSpecProcMenu(d, arg, obj);
				Menu();
				save = OLC_SAVE_YES;
			} else {
				Menu();
			}
			return;

		default:
			mudlogf(BRF, NULL, true, "SYSERR: Reached default case in oedit_parse()!  Case: (%d)", mode);
			d->Write("Oops...\r\n");
			break;
	}

	/*. If we get here, we have changed something .*/
	if (!save)	save = OLC_SAVE_YES;
	Menu();
}


// The wizard routines are strictly procedural to make recompiling easier.
// Return of 0 indicates no templates found.
int oedit_wizard_type_menu(Descriptor *d, Object *obj) {
	const VNum bottom = 0, top = 99;
	int found = 0;
	Index<Object>::Iterator	iter(Object::Index);
	IndexData<Object> *o;

	*buf = '\0';

	while ((o = iter.Next())) {
		if ((o->vnum >= bottom) && (o->vnum <= top) && o->proto->type == obj->type) {
			sprintf(buf + strlen(buf), "&cG%3d&c0) %-20.20s%s", ++found, o->proto->Name(),
					(found % 3 ? "  " : "\r\n"));
		}
	}
	
	if (!found)				return 0;
	else if (found % 3) 	strcat(buf, "\r\n");
	
	d->Write(buf);
	d->Write("Select an object similar to that which you are making.\r\n\n");
	
	return 1;
}


int oedit_wizard_type_parse(Descriptor *d, const char *arg, Object *obj) {
	const VNum bottom = 0, top = 99;
	int choose, i;
	Index<Object>::Iterator	iter(Object::Index);
	IndexData<Object> *o;
	Object *copyfrom = NULL;

	choose = atoi(arg);
	
	if (choose <= 0) {
		return 0;
	}

	while ((o = iter.Next())) {
		if ((o->vnum >= bottom) && (o->vnum <= top) && GET_OBJ_TYPE(o->proto) == obj->type) {
			if (--choose == 0) {
				copyfrom = o->proto;
				break;
			}
		}
	}

	if (choose)		return 0;

	for (i = 0; i < NUM_OBJ_VALUES; ++i) {
		obj->value[i] = copyfrom->value[i];
	}

	obj->material = copyfrom->material;
	obj->weight = copyfrom->weight;
	obj->cost = copyfrom->cost;
	obj->wear = copyfrom->wear;
	obj->extra = copyfrom->extra;
	obj->passive_defense = copyfrom->passive_defense;
	obj->damage_resistance = copyfrom->damage_resistance;
	
	return 1;
}


