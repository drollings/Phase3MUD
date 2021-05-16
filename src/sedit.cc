/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*  _TwyliteMud_ by Rv.                          Based on CircleMud3.0bpl9 *
*    				                                          *
*  OasisOLC - sedit.c 		                                          *
*    				                                          *
*  Copyright 1996 Harvey Gilpin.                                          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


#include "utils.h"
#include "db.h"
#include "olc.h"
#include "buffer.h"
#include "constants.h"

#include "interpreter.h"
#include "characters.h"
#include "rooms.h"
#include "objects.h"
#include "sedit.h"

/*-------------------------------------------------------------------*/


extern struct zone_data *zone_table;

/*-------------------------------------------------------------------*/

/*
 * Handy macros.
 */
#define S_NUM(i)		((i)->vnum)
#define S_KEEPER(i)		((i)->keeper)
#define S_OPEN1(i)		((i)->open1)
#define S_CLOSE1(i)		((i)->close1)
#define S_OPEN2(i)		((i)->open2)
#define S_CLOSE2(i)		((i)->close2)
#define S_BANK(i)		((i)->bankAccount)
#define S_BROKE_TEMPER(i)	((i)->temper1)
#define S_BITVECTOR(i)		((i)->bitvector)
#define S_NOTRADE(i)		((i)->with_who)
#define S_SORT(i)		((i)->lastsort)
#define S_BUYPROFIT(i)		((i)->profit_buy)
#define S_SELLPROFIT(i)		((i)->profit_sell)
#define S_FUNC(i)		((i)->func)

#define S_ROOMS(i)		((i)->in_room)
#define S_PRODUCTS(i)		((i)->producing)
#define S_NAMELISTS(i)		((i)->type)
#define S_ROOM(i, num)		((i)->in_room[(num)])
#define S_PRODUCT(i, num)	((i)->producing[(num)])
#define S_BUYTYPE(i, num)	(BUY_TYPE((i)->type[(num)]))
#define S_BUYWORD(i, num)	(BUY_WORD((i)->type[(num)]))

#define S_NOITEM1(i)		((i)->no_such_item1)
#define S_NOITEM2(i)		((i)->no_such_item2)
#define S_NOCASH1(i)		((i)->missing_cash1)
#define S_NOCASH2(i)		((i)->missing_cash2)
#define S_NOBUY(i)		((i)->do_not_buy)
#define S_BUY(i)		((i)->message_buy)
#define S_SELL(i)		((i)->message_sell)
#define S_PRAC_NOT_KNOWN(i)		((i)->prac_not_known)
#define S_PRAC_MISSING_CASH(i)		((i)->prac_missing_cash)

/*-------------------------------------------------------------------*/

/*
 * Function prototypes.
 */
//int real_shop(VNum vshop_num);

//	External functions.
extern int olc_edit_bitvector(Descriptor *d, Flags *result, char *argument,
		       Flags old_bv, char *name, const char **values, Flags mask);
SPECIAL(shop_keeper);

/*-------------------------------------------------------------------*\
  utility functions 
\*-------------------------------------------------------------------*/

SEdit::SEdit(Descriptor *d, VNum newVNum) : Editor(d) {
	shop = new Shop;

	//	Presto! A shop.
	shop->vnum = number = newVNum;
	value = 0;
	
	Menu();
}

/*-------------------------------------------------------------------*/

SEdit::SEdit(Descriptor *d, Shop *shopEntry) : Editor(d) { 
	shop = new Shop(*shopEntry);
	
	number = shop->vnum;
	
	Menu();
}


//	Generic string modifier for shop keeper messages
void SEdit::ModifyString(char **str, char *new_t) {
	char *pointer = get_buffer(MAX_STRING_LENGTH);
	char *orig = pointer;
	
		pointer = new_t;
	if(*str)	free(*str);
	*str = str_dup(pointer);
	release_buffer(orig);
}

/*-------------------------------------------------------------------*/

void SEdit::SaveInternally(void) {
	S_NUM(shop) = number;

	Shop::Index[number] = *shop;
	
	olc_add_to_save_list(zone_table[zoneNum].number, OLC_SEDIT);
}


void SEdit::Finish(FinishMode mode) {
//	if (mode == All)		delete shop;
//	else					delfree(shop);
	delete shop;
		
	if (d->character) {
		STATE(d) = CON_PLAYING;
		d->character->Echo("$n stops using OLC.\r\n");
	}
	Editor::Finish();
}


/**************************************************************************
 Menu functions 
 **************************************************************************/
  
void SEdit::DispProductsMenu(void) {

#if defined(CLEAR_SCREEN)
	d->Write("\x1B[H\x1B[J");
#endif
	d->Write("##     VNUM     Product\r\n");
			
	for(int i = 0; i < shop->producing.Count(); i++) {
		d->Writef("%2d - [&cC%5d&c0] - &cY%s&c0\r\n",  
				i, shop->producing[i],
				Object::Index[shop->producing[i]].proto->Name());
	}
	d->Write("\r\n"
				 "&cGA&c0) Add a new product.\r\n"
				 "&cGD&c0) Delete a product.\r\n"
				 "&cGQ&c0) Quit\r\n"
				 "Enter choice : ");
	mode = ProductsMenu;
}

/*-------------------------------------------------------------------*/

void SEdit::DispCompactRoomsMenu(void) {
	SInt32	count = 0;

#if defined(CLEAR_SCREEN)
	d->Write("\x1B[H\x1B[J");
#endif
	for(int i = 0; i < shop->in_room.Count(); i++) {
		d->Writef("%2d - [&cC%5d&c0]  | %s",  
				i, shop->in_room[i], !(++count % 5) ? "\r\n" : "");
	}
	d->Write("\r\n"
			 "&cGA&c0) Add a new room.\r\n"
			 "&cGD&c0) Delete a room.\r\n"
			 "&cGL&c0) Long display.\r\n"
			 "&cGQ&c0) Quit\r\n"
			 "Enter choice : ");
 
	mode = RoomsMenu;
}

/*-------------------------------------------------------------------*/

void SEdit::DispRoomsMenu(void) {
	d->Write(
#if defined(CLEAR_SCREEN)
			"\x1B[H\x1B[J"
#endif
			"##     VNUM     Room\r\n\r\n");
	for(int i = 0; i < shop->in_room.Count(); i++) {
		VNum	room = shop->in_room[i];
		d->Writef("%2d - [&cC%5d&c0] - &cY%s&c0\r\n",  
				i, room, Room::Find(room) ? world[room].Name("<ERROR>") : "<NULL>");
	}
	d->Write("\r\n"
			 "&cGA&c0) Add a new room.\r\n"
			 "&cGD&c0) Delete a room.\r\n"
			 "&cGC&c0) Compact Display.\r\n"
			 "&cGQ&c0) Quit\r\n"
			 "Enter choice : ");
 
	mode = RoomsMenu;
}

/*-------------------------------------------------------------------*/

void SEdit::DispNamelistMenu(void) {
	d->Write(
#if defined(CLEAR_SCREEN)
			"\x1B[H\x1B[J"
#endif
			"##              Type   Namelist\r\n\r\n");
	for(int i = 0; i < shop->type.Count(); i++) {
		d->Writef("%2d - &cC%15s&c0 - &cY%s&c0\r\n",
				i, item_types[shop->type[i].type],
				shop->type[i].keywords ? shop->type[i].keywords : "<None>");
	}
	d->Write("\r\n"
			 "&cGA&c0) Add a new entry.\r\n"
			 "&cGD&c0) Delete an entry.\r\n"
			 "&cGQ&c0) Quit\r\n"
			 "Enter choice : ");
	mode = NamelistMenu;
}
  
/*-------------------------------------------------------------------*/

void SEdit::DispShopFlagsMenu(void) {
#if defined(CLEAR_SCREEN)
	d->Write("\x1B[H\x1B[J");
#endif
	olc_edit_bitvector(d, &(S_BITVECTOR(shop)), "", S_BITVECTOR(shop), "shop flags", shop_bits, 0);
	mode = ShopFlags;
}

/*-------------------------------------------------------------------*/

void SEdit::DispNotradeMenu(void) {
#if defined(CLEAR_SCREEN)
	d->Write("\x1B[H\x1B[J");
#endif
	olc_edit_bitvector(d, &(S_NOTRADE(shop)), "", S_NOTRADE(shop), "shop trade flags", trade_letters, 0);
	mode = NotradeMenu;
}

/*-------------------------------------------------------------------*/

void SEdit::DispTypesMenu(void) {
	SInt32	count = 0;

#if defined(CLEAR_SCREEN)
	d->Write("\x1B[H\x1B[J");
#endif
	for(SInt32 i = 0; i < NUM_ITEM_TYPES; i++) {
		d->Writef("&cG%2d&c0) &cC%-20s  %s", i, item_types[i],
				!(++count % 3) ? "\r\n" : "");
	}
	d->Write("&c0Enter choice : ");
	mode = TypeMenu;
}
    

/*-------------------------------------------------------------------*/

/*
 * Display main menu.
 */
void SEdit::Menu(void) {
	sprintbit(S_NOTRADE(shop), trade_letters, buf1);
	sprintbit(S_BITVECTOR(shop), shop_bits, buf2);
	d->Writef(
#if defined(CLEAR_SCREEN)
				"\x1B[H\x1B[J"
#endif
				"-- Shop Number : [&cC%d&c0]\r\n"
				"&cG0&c0) Keeper      : [&cC%d&c0] &cY%s\r\n"
				"&cG1&c0) Open 1      : &cC%4d&c0          &cG2&c0) Close 1     : &cC%4d\r\n"
				"&cG3&c0) Open 2      : &cC%4d&c0          &cG4&c0) Close 2     : &cC%4d\r\n"
				"&cG5&c0) Sell rate   : &cC%1.2f&c0          &cG6&c0) Buy rate    : &cC%1.2f\r\n"
				"&cG7&c0) Keeper no item : &cY%s\r\n"
				"&cG8&c0) Player no item : &cY%s\r\n"
				"&cG9&c0) Keeper no cash : &cY%s\r\n"
				"&cGA&c0) Player no cash : &cY%s\r\n"
				"&cGB&c0) Keeper no buy  : &cY%s\r\n"
				"&cGC&c0) Buy success    : &cY%s\r\n"
				"&cGD&c0) Sell success   : &cY%s\r\n"
				"&cGE&c0) Skill unknown  : &cY%s\r\n"
				"&cGF&c0) Prac w/o cash  : &cY%s\r\n"
				"&cGG&c0) No Trade With  : &cC%s\r\n"
				"&cGH&c0) Shop flags     : &cC%s\r\n"
				"&cGR&c0) Rooms Menu\r\n"
				"&cGP&c0) Products Menu\r\n"
				"&cGT&c0) Accept Types Menu\r\n"
				"&cGQ&c0) Quit\r\n"
				"Enter Choice : ",
			number,
			S_KEEPER(shop) == NOBODY ? NOBODY : Character::Index[S_KEEPER(shop)].vnum, 
			S_KEEPER(shop) == NOBODY ? "None" : Character::Index[S_KEEPER(shop)].proto->Name(),
			S_OPEN1(shop),		S_CLOSE1(shop),
			S_OPEN2(shop),		S_CLOSE2(shop),
			S_BUYPROFIT(shop),	S_SELLPROFIT(shop),
			S_NOITEM1(shop),
			S_NOITEM2(shop),
			S_NOCASH1(shop),
			S_NOCASH2(shop),
			S_NOBUY(shop),
			S_BUY(shop),
			S_SELL(shop),
			S_PRAC_NOT_KNOWN(shop),
			S_PRAC_MISSING_CASH(shop),
			buf1,
			buf2);

	mode = MainMenu;
}

/**************************************************************************
  The GARGANTUAN event handler
 **************************************************************************/

void SEdit::Parse(char *arg) {
	int i;

	if (mode > NumericalResponse) {
		if(!is_number(arg)) {
			d->Write("Field must be numerical, try again : ");
			return;
		}
	}

	switch (mode) {
/*-------------------------------------------------------------------*/
		case ConfirmSave:
			switch (*arg) {
				case 'y':
				case 'Y':
					d->Write("Saving shop to memory.\r\n");
					mudlogf(NRM, d->character, true, "OLC: %s edits shop %d", d->character->RealName(), number);
					SaveInternally();
//					Finish(Structs);
//					break;
				case 'n':
				case 'N':
					Finish(All);
					break;
				default:
					d->Write("Invalid choice!\r\n"
							"Do you wish to save the shop? : ");
			}
			return;

		/*-------------------------------------------------------------------*/
		case MainMenu:
			i = 0;
			switch (*arg) {
				case 'q':
				case 'Q':
					if (save) {	//	Anything been changed?
						d->Write("Do you wish to save the changes to the shop? (y/n) : ");
						mode = ConfirmSave;
					} else
						Finish(All);
					return;
				case '0':
					mode = ShopKeeper;
					d->Write("Enter virtual number of shop keeper : ");
					return;
				case '1':
					mode = ShopOpen1;
					i++;
					break;
				case '2':
					mode = ShopClose1;
					i++;
					break;
				case '3':
					mode = ShopOpen2;
					i++;
					break;
				case '4':
					mode = ShopClose2;
					i++;
					break;
				case '5':
					mode = BuyProfit;
					i++;
					break;
				case '6':
					mode = SellProfit;
					i++;
					break;
				case '7':
					mode = NoItem1;
					i--;
					break;
				case '8':
					mode = NoItem2;
					i--;
					break;
				case '9':
					mode = NoCash1;
					i--;
					break;
				case 'a':
				case 'A':
					mode = NoCash2;
					i--;
					break;
				case 'b':
				case 'B':
					mode = NoBuy;
					i--;
					break;
				case 'c':
				case 'C':
					mode = Buy;
					i--;
					break;
				case 'd':
				case 'D':
					mode = Sell;
					i--;
					break;
				case 'e':
				case 'E':
					mode = PracNoSkill;
					i--;
					break;
				case 'f':
				case 'F':
					mode = PracNoCash;
					i--;
					break;
				case 'g':
				case 'G':
					DispNotradeMenu();
					return;
				case 'h':
				case 'H':
					DispShopFlagsMenu();
					return;
				case 'r':
				case 'R':
					DispRoomsMenu();
					return;
				case 'p':
				case 'P':
					DispProductsMenu();
					return;
				case 't':
				case 'T':
					DispNamelistMenu();
					return;
				default:
					Menu();
					return;
			}

			if (i != 0) {
				d->Write(i == 1 ? "\r\nEnter new value : " : (i == -1 ?
						"\r\nEnter new text :\r\n] " : "Oops...\r\n"));
				return;
			}
			break; 
			/*-------------------------------------------------------------------*/
		case NamelistMenu:
			switch (*arg) {
				case 'a':
				case 'A':
					DispTypesMenu();
					return;
				case 'd':
				case 'D':
					d->Write("\r\nDelete which entry? : ");
					mode = DeleteType;
					return;
				case 'q':
				case 'Q':
					break;
			}
			break;
			/*-------------------------------------------------------------------*/
		case ProductsMenu:
			switch (*arg) {
				case 'a':
				case 'A':
					d->Write("\r\nEnter new product virtual number : ");
					mode = NewProduct;
					return;
				case 'd':
				case 'D':
					d->Write("\r\nDelete which product? : ");
					mode = DeleteProduct;
					return;
				case 'q':
				case 'Q':
					break;
			}
			break;
			/*-------------------------------------------------------------------*/
		case RoomsMenu:
			switch (*arg) {
				case 'a':
				case 'A':
					d->Write("\r\nEnter new room virtual number : ");
					mode = NewRoom;
					return;
				case 'c':
				case 'C':
					DispCompactRoomsMenu();
					return;
				case 'l':
				case 'L':
					DispRoomsMenu();
					return;
				case 'd':
				case 'D':
					d->Write("\r\nDelete which room? : ");
					mode = DeleteRoom;
					return;
				case 'q':
				case 'Q':
					break;
			}
			break;
			/*-------------------------------------------------------------------*/
			/*. String edits .*/
		case NoItem1:		ModifyString(&S_NOITEM1(shop), arg);		break;
		case NoItem2:		ModifyString(&S_NOITEM2(shop), arg);		break;
		case NoCash1:		ModifyString(&S_NOCASH1(shop), arg);		break;
		case NoCash2:		ModifyString(&S_NOCASH2(shop), arg);		break;
		case NoBuy:			ModifyString(&S_NOBUY(shop), arg);			break;
		case Buy:			ModifyString(&S_BUY(shop), arg);			break;
		case Sell:			ModifyString(&S_SELL(shop), arg);			break;
		case PracNoSkill:	ModifyString(&S_PRAC_NOT_KNOWN(shop), arg);		break;
		case PracNoCash:	ModifyString(&S_PRAC_MISSING_CASH(shop), arg);	break;
		case NamelistEntry:
			{
				BuyData new_entry;
				new_entry.type = value;
				new_entry.keywords = str_dup(arg);
				shop->type.Append(new_entry);
			}
			DispNamelistMenu();
			return;

			/*-------------------------------------------------------------------*/
			/*. Numerical responses .*/

		case ShopKeeper:
			if ((i = atoi(arg)) != -1) {
				if (!Character::Find(i)) {
					d->Write("That mobile does not exist, try again : ");
					return;
				}
			}
			S_KEEPER(shop) = i;
			if (i == -1)
				break;
			/*. Fiddle with special procs .*/
			S_FUNC(shop) = Character::Index[i].func;
			Character::Index[i].func = shop_keeper;
			break;
		case ShopOpen1:			S_OPEN1(shop) = RANGE(0, atoi(arg), 28);		break;
		case ShopOpen2:			S_OPEN2(shop) = RANGE(0, atoi(arg), 28);		break;
		case ShopClose1:		S_CLOSE1(shop) = RANGE(0, atoi(arg), 28);		break;
		case ShopClose2:		S_CLOSE2(shop) = RANGE(0, atoi(arg), 28);		break;
		case BuyProfit:			sscanf(arg, "%f", &S_BUYPROFIT(shop));			break;
		case SellProfit:		sscanf(arg, "%f", &S_SELLPROFIT(shop));			break;
		case TypeMenu:
			value = RANGE(0, atoi(arg), NUM_ITEM_TYPES - 1);
			d->Write("Enter namelist (return for none) :-\r\n| ");
			mode = NamelistEntry;
			return;
		case DeleteType:
			shop->type.Remove(atoi(arg));
			DispNamelistMenu();
			return;
		case NewProduct:
			if ((i = atoi(arg)) != -1) {
				if (!Object::Find(i)) {
					d->Write("That object does not exist, try again : ");
					return;
				}
			}
			if (i > 0)	shop->producing.Append(i);
			DispProductsMenu();
			return;
		case DeleteProduct:
			shop->producing.Remove(atoi(arg));
			DispProductsMenu();
			return;
		case NewRoom:
			if ((i = atoi(arg)) != -1) {
				if (!Room::Find(i)) {
					d->Write("That room does not exist, try again : ");
					return;
				}
			}
			if (i >= 0)	shop->in_room.Append(atoi(arg));
			DispRoomsMenu();
			return;
		case DeleteRoom:
			shop->in_room.Remove(atoi(arg));
			DispRoomsMenu();
			return;

		case ShopFlags:
			switch (*arg && olc_edit_bitvector(d, &(S_BITVECTOR(shop)), arg, S_BITVECTOR(shop), 
							"shop flags", shop_bits, 0)) {
			case 0: 
				Menu(); 
				return;
			default: 
				if (!save)	save = OLC_SAVE_YES;
				DispShopFlagsMenu();
			}
			return;

		case NotradeMenu:
			switch (*arg && olc_edit_bitvector(d, &(S_NOTRADE(shop)), arg, S_NOTRADE(shop), 
							"shop trade flags", trade_letters, 0)) {
			case 0: 
				Menu(); 
				return;
			default: 
				if (!save)	save = OLC_SAVE_YES;
				DispNotradeMenu();
			}
			return;

			/*-------------------------------------------------------------------*/
		default:
			/*. We should never get here .*/
			Finish(All);
			mudlog("SYSERR: sedit_parse(): Reached default case!", BRF, NULL, true);
			d->Write("Oops...\r\n");
			return;
	}
	
	save = OLC_SAVE_YES;
	
	Menu();
}
