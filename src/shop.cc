/* ************************************************************************
*   File: shop.c                                        Part of CircleMUD *
*  Usage: shopkeepers: loading config files, spec procs.                  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/***
 * The entire shop rewrite for Circle 3.0 was done by Jeff Fink.  Thanks Jeff!
 ***/

#define __SHOP_C__


#include "structs.h"
#include "buffer.h"
#include "descriptors.h"
#include "rooms.h"
#include "characters.h"
#include "objects.h"
#include "comm.h"
#include "handler.h"
#include "find.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"
#include "shop.h"
#include "files.h"
#include "constants.h"

/* External variables */
extern struct TimeInfoData time_info;

#define SKILL_BASE(skill)        Skills[(skill)]->stats


/* Forward/External function declarations */
void perform_tell(Character *ch, Character *vict, char *arg);
ACMD(do_action);
ACMD(do_echo);
ACMD(do_say);
char *fname(char *namelist);
int can_take_obj(Character * ch, Object * obj);

void load_otrigger(Object *obj);

/* Local variables */
Map<VNum, Shop>	Shop::Index;
int cmd_say, cmd_tell, cmd_emote, cmd_slap, cmd_cough;


int find_oper_num(char token);
int evaluate_expression(Object * obj, char *expr);
bool same_obj(Object * obj1, Object * obj2);
int transaction_amt(char *arg);
char *times_message(Object * obj, char *name, int num);
int ok_damage_shopkeeper(Character * ch, Character * victim);
int add_to_list(Vector<BuyData> &list, int type, int *val);
int add_to_list(Vector<VNum> &list, int type, int *val);
void read_line(FILE * shop_f, const char *string, Ptr data);
void boot_the_shops(FILE * shop_f, char *filename, int rec_count);
void assign_the_shopkeepers(void);
void list_all_shops(Character * ch);
void handle_detailed_list(char *buf, char *buf1, Character * ch);
void show_shops(Character * ch, char *arg);
SPECIAL(shop_keeper);
Object *get_slide_obj_vis(Character * ch, char *name, LList<Object *> &list);
Object *get_hash_obj_vis(Character * ch, char *name, LList<Object *> &list);
void read_list(FILE * shop_f, Vector<VNum> &list, int type);
void read_type_list(FILE * shop_f, Vector<BuyData> &list);


const char *CustomerString(Flags with_who, bool detailed);


const char * MSG_NOT_OPEN_YET		= "Come back later!";
const char * MSG_NOT_REOPEN_YET	= "Sorry, we have closed, but come back later.";
const char * MSG_CLOSED_FOR_DAY	= "Sorry, come back tomorrow.";
const char * MSG_NO_SEE_CHAR		= "I don't trade with someone I can't see!";
const char * MSG_NO_SELL_RACE		= "Get out of here before I call the guards!";
const char * MSG_NO_SELL_CLASS		= "We don't serve your kind here!";
const char * MSG_NO_USED_WANDSTAFF	= "I don't buy used up wands or staves!";
const char * MSG_CANT_KILL_KEEPER	= "Get out of here before I call the guards!";


const char *operator_str[] = {
	"[({",
	"])}",
	"|+",
	"&*",
	"^'"
};


//	Constant list for printing out who we sell to
const char *trade_letters[] = {
	"Good",			/* First, the alignment based ones */
	"Evil",
	"Neutral",
	"Hyumann",		/* Then the race based ones */
	"Elf",
	"Dwomax",
	"Shastick",
	"Murrash",
	"Laori",
	"Thadyri",
	"Terran",
	"Trulor",
	"\n"
};


const char *shop_bits[] = {
	"WILL_FIGHT",
	"USES_BANK",
	"SHOPKEEPER",
	"GUILDMASTER",
	"\n"
};


Shop::Shop(void) : vnum(-1), profit_buy(1.0), profit_sell(1.0), with_who(0), bitvector(0),
		keeper(-1), open1(0), open2(0), close1(28), close2(0), lastsort(0), func(NULL) {	
	no_such_item1	= str_dup("Sorry, I don't stock that item.");
	no_such_item2	= str_dup("You don't seem to have that.");
	missing_cash1	= str_dup("I can't afford that!");
	missing_cash2	= str_dup("You are too poor!");
	do_not_buy		= str_dup("I don't trade in such items.");
	message_buy		= str_dup("That'll be %d coins, thanks.");
	message_sell	= str_dup("I'll give you %d coins for that.");
	prac_not_known	= str_dup("I can't teach you that, sorry.");
	prac_missing_cash	= str_dup("I can't teach for free!");
}


Shop::Shop(const Shop &s) : vnum(s.vnum), producing(s.producing), profit_buy(s.profit_buy),
		profit_sell(s.profit_sell), type(s.type),
		no_such_item1(str_dup(s.no_such_item1)), no_such_item2(str_dup(s.no_such_item2)),
		missing_cash1(str_dup(s.missing_cash1)), missing_cash2(str_dup(s.missing_cash2)),
		do_not_buy(str_dup(s.do_not_buy)), message_buy(str_dup(s.message_buy)),
		message_sell(str_dup(s.message_sell)), 
		prac_not_known(str_dup(s.prac_not_known)),
		prac_missing_cash(str_dup(s.prac_missing_cash)), 
		with_who(s.with_who), bitvector(s.bitvector), 
		keeper(s.keeper), in_room(s.in_room), open1(s.open1), open2(s.open2), 
		close1(s.close1), close2(s.close2), lastsort(s.lastsort), func(s.func) {
}


Shop::~Shop(void) {
	if (no_such_item1)		free(no_such_item1);
	if (no_such_item2)		free(no_such_item2);
	if (missing_cash1)		free(missing_cash1);
	if (missing_cash2)		free(missing_cash2);
	if (do_not_buy)			free(do_not_buy);
	if (message_buy)		free(message_buy);
	if (message_sell)		free(message_sell);
	if (prac_not_known)		free(prac_not_known);
	if (prac_missing_cash)	free(prac_missing_cash);
}


Shop &Shop::operator=(const Shop &s) {
	vnum		= s.vnum;
	profit_buy	= s.profit_buy;
	profit_sell	= s.profit_sell;
	
	bitvector	= s.bitvector;
	with_who	= s.with_who;
	keeper		= s.keeper;
	
	producing	= s.producing;
	type		= s.type;
	in_room		= s.in_room;

	open1		= s.open1;
	open2		= s.open2;
	close1		= s.close1;
	close2		= s.close2;
	
	lastsort	= s.lastsort;
	func		= s.func;

	if (no_such_item1)	free(no_such_item1);	no_such_item1	= str_dup(s.no_such_item1);
	if (no_such_item2)	free(no_such_item2);	no_such_item2	= str_dup(s.no_such_item2);
	if (missing_cash1)	free(missing_cash1);	missing_cash1	= str_dup(s.missing_cash1);
	if (missing_cash2)	free(missing_cash2);	missing_cash2	= str_dup(s.missing_cash2);
	if (do_not_buy)		free(do_not_buy);		do_not_buy		= str_dup(s.do_not_buy);
	if (message_buy)	free(message_buy);		message_buy		= str_dup(s.message_buy);
	if (message_sell)	free(message_sell);		message_sell	= str_dup(s.message_sell);

	if (prac_not_known)	free(prac_not_known);			prac_not_known	= str_dup(s.prac_not_known);
	if (prac_missing_cash)	free(prac_missing_cash);	prac_missing_cash	= str_dup(s.prac_missing_cash);
	
	return *this;
}


bool Shop::operator==(const Shop &s) {
	return (s.vnum == s.vnum);
}


bool Shop::IsOKChar(Character * keeper, Character * ch) {
	char *buf;
	if (!ch->CanBeSeen(keeper)) {
		buf = get_buffer(256);
		strcpy(buf, MSG_NO_SEE_CHAR);
		do_say(keeper, buf, cmd_say, "say", 0);
		release_buffer(buf);
		return false;
	}
  
	if (NO_STAFF_HASSLE(ch))
		return true;

	if ((IS_GOOD(ch) && IS_SET(with_who, TRADE_NOGOOD)) ||
		(IS_NEUTRAL(ch) && IS_SET(with_who, TRADE_NONEUTRAL)) ||
		(IS_EVIL(ch) && IS_SET(with_who, TRADE_NOEVIL)) ||
		(IS_HYUMANN(ch) && IS_SET(with_who, TRADE_NOHYUMANN)) ||
		(IS_ELF(ch) && IS_SET(with_who, TRADE_NOELF)) ||
		(IS_DWOMAX(ch) && IS_SET(with_who, TRADE_NODWOMAX)) ||
		(IS_SHASTICK(ch) && IS_SET(with_who, TRADE_NOSHASTICK)) ||
		(IS_MURRASH(ch) && IS_SET(with_who, TRADE_NOMURRASH)) ||
		(IS_LAORI(ch) && IS_SET(with_who, TRADE_NOLAORI)) ||
		(IS_THADYRI(ch) && IS_SET(with_who, TRADE_NOTHADYRI)) ||
		(IS_TERRAN(ch) && IS_SET(with_who, TRADE_NOTERRAN)) ||
		(IS_TRULOR(ch) && IS_SET(with_who, TRADE_NOTRULOR))) {

		buf = get_buffer(256);
        strcpy(buf, MSG_NO_SELL_CLASS);
		perform_tell(keeper, ch, buf);
		release_buffer(buf);
		return false;
	}
	return true;
}


bool Shop::IsOpen(Character * keeper, bool msg) {
	char *buf = get_buffer(256);

	if (open1 > time_info.hours)
		strcpy(buf, MSG_NOT_OPEN_YET);
	else if (close1 < time_info.hours)
		if (open2 > time_info.hours)
			strcpy(buf, MSG_NOT_REOPEN_YET);
		else if (close2 < time_info.hours)
			strcpy(buf, MSG_CLOSED_FOR_DAY);

	if (!(*buf)) {
		release_buffer(buf);
		return true;
	}
	if (msg)
		do_say(keeper, buf, cmd_tell, "say", 0);
	release_buffer(buf);
	return false;
}


bool Shop::IsOK(Character * keeper, Character * ch) {
	if (IS_IMMORTAL(ch))			return true;
	if (IsOpen(keeper, true))		return IsOKChar(keeper, ch);
	else							return false;
}


int StackData::Top(void) const		{ return (length > 0) ? data[length - 1] : -1;	}
void StackData::Push(int pushval)	{ data[length++] = pushval;						}
int StackData::Pop(void) {
	if (length > 0)	return data[--length];
    log("Illegal expression %d in shop keyword list", length);
    return 0;
}


void StackData::EvaluateOperation(StackData &values) {
	int	op = Pop();
	switch (op) {
		case OPER_NOT:	values.Push(!values.Pop());					break;
		case OPER_AND:	values.Push(values.Pop() && values.Pop());	break;
		case OPER_OR:	values.Push(values.Pop() || values.Pop());	break;
		default:													break;
	}
}


int find_oper_num(char token) {
	int index;

	for (index = 0; index <= MAX_OPER; index++)
		if (strchr(operator_str[index], token))
				return (index);
	return (NOTHING);
}


int evaluate_expression(Object * obj, char *expr) {
	StackData	ops, vals;
	char *ptr, *end, *name;
	int temp, index;

	if (!expr)
		return true;

	if (!isalpha(*expr))
		return true;

	ptr = expr;
	name = get_buffer(256);
	while (*ptr) {
		if (isspace(*ptr))
			ptr++;
		else {
			if ((temp = find_oper_num(*ptr)) == NOTHING) {
				end = ptr;
				while (*ptr && !isspace(*ptr) && (find_oper_num(*ptr) == NOTHING))
					ptr++;
				strncpy(name, end, ptr - end);
				name[ptr - end] = 0;
				for (index = 0; *extra_bits[index] != '\n'; index++)
					if (!str_cmp(name, extra_bits[index])) {
						vals.Push(OBJ_FLAGGED(obj, 1 << index));
						break;
					}
				if (*extra_bits[index] == '\n')
					vals.Push(isname(name, obj->Name()));
			} else {
				if (temp != OPER_OPEN_PAREN)
					while (ops.Top() > temp)
						ops.EvaluateOperation(vals);

				if (temp == OPER_CLOSE_PAREN) {
					if ((temp = ops.Pop()) != OPER_OPEN_PAREN) {
						log("Illegal parenthesis in shop keyword expression.");
						release_buffer(name);
						return false;
					}
				} else
					ops.Push(temp);
				ptr++;
			}
		}
	}
	release_buffer(name);
	while (ops.Top() != NOTHING)
		ops.EvaluateOperation(vals);
	temp = vals.Pop();
	if (vals.Top() != NOTHING) {
		log("Extra operands left on shop keyword expression stack.");
		return false;
	}
	return (temp);
}


Shop::TradeState Shop::TradeWith(Object *item) {
	SInt32		counter;
	Object *	content;

	//	Is it worth anything?  --  get_selling_obj needs this check
	if (item->TotalValue() < 1)
		return OBJECT_NOVAL;

	//	IS it a no-sell item?
	if (OBJ_FLAGGED(item, ITEM_NOSELL))
		return OBJECT_NOTOK;

	//	Makes sure it doesn't contain a no-sell item
	//	Moved up so that it can't be overridden by a buyword, because
	//	if it is, it would either return OBJECT_CONT_NOTOK, or OBJECT_NOTOK
	//	which is pretty useless.
	LListIterator<Object *>	iter(item->contents);
	while ((content = iter.Next()))
		if (TradeWith(content) != OBJECT_OK)
			return OBJECT_CONT_NOTOK;
	
	for (counter = 0; counter < type.Count(); counter++)
		if (type[counter].type == GET_OBJ_TYPE(item))
			if (evaluate_expression(item, type[counter].keywords))
				return OBJECT_OK;


	return OBJECT_NOTOK;
}


bool same_obj(Object * obj1, Object * obj2) {
	int index;

	if (!obj1 || !obj2)
		return (obj1 == obj2);

	if (obj1->Virtual() != obj2->Virtual())
		return false;

	if (GET_OBJ_COST(obj1) != GET_OBJ_COST(obj2))
		return false;

	if (GET_OBJ_EXTRA(obj1) != GET_OBJ_EXTRA(obj2))
		return false;

	if (GET_OBJ_WEIGHT(obj1) != GET_OBJ_WEIGHT(obj2))
		return false;
	
	for (index = 0; index < 8; index++)
		if (GET_OBJ_VAL(obj1, index) != GET_OBJ_VAL(obj2, index))
			return false;
	
	if (str_cmp(obj1->Keywords(), obj2->Keywords()))
		return false;
	
	if (str_cmp(obj1->Name(), obj2->Name()))
		return false;
	
//	if (str_cmp(SSData(obj1->actionDesc), SSData(obj2->actionDesc)))
//		return false;
	
	
	for (index = 0; index < MAX_OBJ_AFFECT; index++)
		if ((obj1->affectmod[index].location != obj2->affectmod[index].location) ||
				(obj1->affectmod[index].modifier != obj2->affectmod[index].modifier))
			return false;

	return true;
}


bool Shop::Producing(Object * item) {
	if (item->Virtual() < 0)
		return false;

	for (int i = 0; i < producing.Count(); i++)
		if (same_obj(item, Object::Index[producing[i]].proto))
			return true;

	return false;
}


int transaction_amt(char *arg) {
	int num;
	char *buf = get_buffer(MAX_INPUT_LENGTH);
	
	one_argument(arg, buf);
	if (*buf)
		if ((is_number(buf))) {
			num = atoi(buf);
			strcpy(arg, arg + strlen(buf) + 1);
			release_buffer(buf);
			return (num);
		}
	release_buffer(buf);
	return (1);
}


char *times_message(Object * obj, char *name, int num)
{
	static char buf[256];
	char *ptr;

	if (obj)
		strcpy(buf, obj->Name());
	else {
		if ((ptr = strchr(name, '.')) == NULL)	ptr = name;
		else									ptr++;
		sprintf(buf, "%s %s", AN(ptr), ptr);
	}

	if (num > 1)
		sprintf(END_OF(buf), " (x %d)", num);

	return (buf);
}


Object *get_slide_obj_vis(Character * ch, char *name, LList<Object *> &list) 
{
	Object *i, *last_match = 0;
	int j, number;
	char *tmpname = get_buffer(MAX_INPUT_LENGTH);
	char *tmp;

	strcpy(tmpname, name);
	tmp = tmpname;
	if (!(number = get_number(&tmp))) {
		release_buffer(tmpname);
		return NULL;
	}

	j = 1;
	START_ITER(iter, i, Object *, list) {
		if (j > number)
			break;
		if (i->CanBeSeen(ch) && isname(tmp, i->Name()) && !same_obj(last_match, i)) {
			if (j == number) {
				release_buffer(tmpname);
				return i;
			}
			last_match = i;
			j++;
		}
	}
	release_buffer(tmpname);
	return NULL;
}


Object *get_hash_obj_vis(Character * ch, char *name, LList<Object *> &list) {
	Object *loop, *last_obj = 0;
	int index;

	if ((is_number(name + 1)))		index = atoi(name + 1);
	else							return NULL;

	START_ITER(iter, loop, Object *, list) {
		if (loop->CanBeSeen(ch) && (loop->TotalValue() > 0) && !same_obj(last_obj, loop)) {
			if (--index == 0) {
				return loop;
			}
			last_obj = loop;
		}
	}
	return NULL;
}


Object *Shop::GetPurchaseObj(Character * ch, char *arg, Character * keeper, bool msg) {
	char	*buf = get_buffer(MAX_STRING_LENGTH),
			*name = get_buffer(MAX_INPUT_LENGTH);
	Object *obj;

	one_argument(arg, name);
	do {
		// obj = (*name == '#' || is_number(name)) ? get_hash_obj_vis(ch, name, keeper->contents) :
		// 		obj = get_slide_obj_vis(ch, name, keeper->contents);
		if (*name == '#')	obj = get_hash_obj_vis(ch, name, keeper->contents);
		else				obj = get_slide_obj_vis(ch, name, keeper->contents);

		if (!obj) {
			if (msg) {
				perform_tell(keeper, ch, no_such_item1);
			}
			release_buffer(buf);
			release_buffer(name);
			return NULL;
		}
		if (obj->TotalValue() <= 0) {
			obj->Extract();
			obj = NULL;
		}
	} while (!obj);
	release_buffer(buf);
	release_buffer(name);
	return obj;
}


int Shop::BuyPrice(Character * ch, Object * obj)
{
	float profit = profit_buy;
	profit *= 0.08 * (10 - MAX(MIN(GET_CHA(ch), 15), 6)) + 1;
	return ((int) (GET_OBJ_COST(obj) * profit));
}


void Shop::Buy(char *arg, Character * ch, Character * keeper) {
	char	*tempstr, *buf;
	Object *obj, *last_obj = NULL;
	int goldamt = 0, buynum, bought = 0;

	if (!IsOK(keeper, ch))
		return;

	if (lastsort < IS_CARRYING_N(keeper))
		SortKeeperObjs(keeper);

	if ((buynum = transaction_amt(arg)) < 0) {
		perform_tell(keeper, ch, "A negative amount?  Try selling me something.");
		return;
	}
	if (!(*arg) || !(buynum)) {
		perform_tell(keeper, ch, "What do you want to buy??");
		return;
	}
	if (!(obj = GetPurchaseObj(ch, arg, keeper, true))) {
		return;
	}  


	if ((BuyPrice(ch, obj) > GET_GOLD(ch)) && !NO_STAFF_HASSLE(ch)) {
		perform_tell(keeper, ch, missing_cash2);
		return;
	}

	if (!can_take_obj(ch, obj)) {
		return;
	}

	while ((obj) && ((GET_GOLD(ch) >= BuyPrice(ch, obj)) || NO_STAFF_HASSLE(ch))
			&& (bought < buynum)) {
		bought++;
		/* Test if producing shop ! */
		if (Producing(obj)) {
			obj = new Object(*obj);
			load_otrigger(obj);
		} else {
			obj->FromChar();
			lastsort--;
		}
		obj->ToChar(ch);

		goldamt += BuyPrice(ch, obj);
		if (!NO_STAFF_HASSLE(ch))
			GET_GOLD(ch) -= BuyPrice(ch, obj);

		last_obj = obj;
		obj = GetPurchaseObj(ch, arg, keeper, false);
		if (!same_obj(obj, last_obj))
			break;
	}

	buf = get_buffer(MAX_STRING_LENGTH);

	if (bought < buynum) {
		if (!obj || !same_obj(last_obj, obj))
			sprintf(buf, "I only have %d to sell you.", bought);
		else if (GET_GOLD(ch) < BuyPrice(ch, obj))
			sprintf(buf, "You can only afford %d.", bought);
		else
			sprintf(buf, "Something screwy only gave you %d.", bought);
		perform_tell(keeper, ch, buf);
	}
//  if (!NO_STAFF_HASSLE(ch))
//    GET_GOLD(keeper) += goldamt;
	tempstr = get_buffer(200),

	sprintf(tempstr, times_message(ch->contents.Top(), 0, bought));
	sprintf(buf, "$n buys %s.", tempstr);
	act(buf, FALSE, ch, obj, 0, TO_ROOM);

	sprintf(buf, message_buy, goldamt);
	perform_tell(keeper, ch, buf);
	ch->Sendf("You now have %s.\r\n", tempstr);

//  if (SHOP_USES_BANK(shop_nr))
//    if (GET_GOLD(keeper) > MAX_OUTSIDE_BANK) {
//      SHOP_BANK(shop_nr) += (GET_GOLD(keeper) - MAX_OUTSIDE_BANK);
//      GET_GOLD(keeper) = MAX_OUTSIDE_BANK;
//    }
	release_buffer(buf);
	release_buffer(tempstr);
}


Object *Shop::GetSellingObj(Character * ch, char *name, Character * keeper, bool msg) {
	char *		buf;
	Object *	obj;
	TradeState	result;

	if (!(obj = get_obj_in_list_vis(ch, name, ch->contents))) {
		if (msg) {
			perform_tell(keeper, ch, no_such_item2);
		}
		return NULL;
	}
	if ((result = TradeWith(obj)) == OBJECT_OK)
		return (obj);

	buf = get_buffer(SMALL_BUFSIZE);
	switch (result) {
		case OBJECT_NOVAL:
			strcpy(buf, "You've got to be kidding, that thing is worthless!");
			break;
		case OBJECT_NOTOK:
			strcpy(buf, do_not_buy);
			break;
		case OBJECT_DEAD:
			strcpy(buf, MSG_NO_USED_WANDSTAFF);
			break;
		case OBJECT_CONT_NOTOK:
			strcpy(buf, "I won't buy the contents of it!");
			break;
		default:
			log("Illegal return value of %d from trade_with() (" __FILE__ ")", result);
			strcpy(buf, "An error has occurred.");
			break;
	}
	if (msg)	perform_tell(keeper, ch, buf);
	release_buffer(buf);
	return NULL;
}


int Shop::SellPrice(Character * ch, Object * obj) {
	float price;
	float buyprice = BuyPrice(ch, obj);
	float profit = profit_sell;
	
	profit *= 1 - (0.08 * (10 - MAX(MIN(GET_CHA(ch), 15), 6)));
	price = (GET_OBJ_COST(obj) * profit);

	if (price > buyprice)
		price = buyprice;
	
	return ((int) price);
}


//	Finally rewrote to sort.  With the InsertBefore() function added to
//	the LList template, it became MUCH easier!
Object *Shop::SlideObj(Object * obj, Character * keeper) {
	int temp;
	Object *loop;

	if (lastsort < IS_CARRYING_N(keeper))
		SortKeeperObjs(keeper);

	/* Extract the object if it is identical to one produced */
	if (Producing(obj)) {
		temp = obj->Virtual();
		obj->Extract();
		return Object::Index[temp].proto;
	}
	lastsort++;
	obj->ToChar(keeper);			//	Move it to the keeper...
	keeper->contents.Remove(obj);	//	But we want to re-order it

	LListIterator<Object *>	iter(keeper->contents);
	
	while((loop = iter.Next())) {
		if (same_obj(obj, loop)) {
			keeper->contents.InsertBefore(obj, loop);
			return obj;
		}
	}
	keeper->contents.Prepend(obj);
	return obj;
}


void Shop::SortKeeperObjs(Character * keeper) {
	Object				*temp;
	LList<Object *>	list;
	
	while (lastsort < IS_CARRYING_N(keeper)) {
		temp = keeper->contents.Top();
		temp->FromChar();
		list.Prepend(temp);
	}

	while (list.Count()) {
		temp = list.Top();
		list.Remove(temp);
		if ((Producing(temp)) && !(get_obj_in_list_num(temp->Virtual(), keeper->contents))) {
			temp->ToChar(keeper);
			lastsort++;
		} else
			SlideObj(temp, keeper);
	}
}


void Shop::Sell(char *arg, Character * ch, Character * keeper) {
	char	*tempstr, *buf, *name;
	Object *obj, *tag = 0;
	int sellnum, sold = 0, goldamt = 0;

	if (!IsOK(keeper, ch))
		return;

	if ((sellnum = transaction_amt(arg)) < 0) {
		perform_tell(keeper, ch, "A negative amount?  Try buying something.");
		return;
	}
	if (!*arg || !sellnum) {
		perform_tell(keeper, ch, "What do you want to sell??");
		release_buffer(buf);
		return;
	}
	name = get_buffer(MAX_INPUT_LENGTH);
	one_argument(arg, name);

	obj = GetSellingObj(ch, name, keeper, true);
	if (!obj) {
		release_buffer(name);
		return;
	}

	if (OBJ_FLAGGED(obj, ITEM_NODROP)) {
		ch->Sendf("For some reason, you cannot get rid of %s!\r\n", obj->Name());
		release_buffer(name);
		return;
		
	}
	
//	if (GET_GOLD(keeper) + SHOP_BANK(shop_nr) < sell_price(ch, obj, shop_nr)) {
//		sprintf(buf, shop_index[shop_nr].missing_cash1, ch->GetName());
//		do_tell(keeper, buf, cmd_tell, 0);
//		release_buffer(name);
//		release_buffer(buf);
//		return;
//	}
	while ((obj) && /* (GET_GOLD(keeper) + SHOP_BANK(shop_nr) >=
			sell_price(ch, obj, shop_nr)) && */ (sold < sellnum)) {
		sold++;

		goldamt += SellPrice(ch, obj);
//		GET_GOLD(keeper) -= sell_price(ch, obj, shop_nr);

		obj->FromChar();
		tag = SlideObj(obj, keeper);
		obj = GetSellingObj(ch, name, keeper, false);
	}

	buf = get_buffer(SMALL_BUFSIZE);

	if (sold < sellnum) {
		if (!obj)
			sprintf(buf, "You only have %d of those.", sold);
//		else if (GET_GOLD(keeper) + SHOP_BANK(shop_nr) < goldamt)
//			sprintf(buf, "%s I can only afford to buy %d of those.", ch->GetName(), sold);
		else
			sprintf(buf, "Something really screwy made me buy %d.", sold);

		perform_tell(keeper, ch, buf);
	}
	tempstr = get_buffer(200);

	GET_GOLD(ch) += goldamt;
	strcpy(tempstr, times_message(0, name, sold));
	sprintf(buf, "$n sells %s.", tempstr);
	act(buf, FALSE, ch, obj, 0, TO_ROOM);

	sprintf(buf, message_sell, goldamt);
	perform_tell(keeper, ch, buf);
	ch->Sendf("The shopkeeper now has %s.\r\n", tempstr);

	release_buffer(buf);
	release_buffer(tempstr);
	release_buffer(name);
  
//	if (GET_GOLD(keeper) < MIN_OUTSIDE_BANK) {
//		goldamt = MIN(MAX_OUTSIDE_BANK - GET_GOLD(keeper), SHOP_BANK(shop_nr));
//		SHOP_BANK(shop_nr) -= goldamt;
//		GET_GOLD(keeper) += goldamt;
//	}
}


void Shop::Value(char *arg, Character * ch, Character * keeper) {
	char *buf, *name;
	Object *obj;

	if (!IsOK(keeper, ch))
		return;

	if (!(*arg)) {
	perform_tell(keeper, ch, "What do you want me to evaluate??");
		return;
	}
	name = get_buffer(MAX_INPUT_LENGTH);
	one_argument(arg, name);
	obj = GetSellingObj(ch, name, keeper, true);
	release_buffer(name);
	if (!obj)
		return;

	buf = get_buffer(MAX_INPUT_LENGTH);
	sprintf(buf, "I'll give you %d coins for that!", SellPrice(ch, obj));
	perform_tell(keeper, ch, buf);
	release_buffer(buf);
}


char *Shop::ListObject(Character *ch, Object * obj, int cnt, int index) {
	static char buf[256];
	char		*buf2 = get_buffer(300),
				*buf3 = get_buffer(200);
	SInt32		contentCount;

	if (Producing(obj))		strcpy(buf2, "Unlimited   ");
	else					sprintf(buf2, "%5d       ", cnt);
	sprintf(buf, " %2d)  %s", index, buf2);

	/* Compile object name and information */
	strcpy(buf3, obj->Name());
	if ((GET_OBJ_TYPE(obj) == ITEM_DRINKCON) && (GET_OBJ_VAL(obj, 1)))
		sprintf(END_OF(buf3), " of %s", drinks[GET_OBJ_VAL(obj, 2)]);
	else if ((contentCount = obj->contents.Count()))
		sprintf(END_OF(buf3), " (%d content%s)", contentCount, ((contentCount > 1) ? "s" : ""));

	if ((GET_OBJ_TYPE(obj) == ITEM_WAND) || (GET_OBJ_TYPE(obj) == ITEM_STAFF))
		if (GET_OBJ_VAL(obj, 2) < GET_OBJ_VAL(obj, 1))
			strcat(buf3, " (partially used)");

	/* FUTURE: */
	/* Add glow/hum/etc */

	sprintf(buf2, "%-40s %6d  %-6d\r\n", buf3, BuyPrice(ch, obj), GET_OBJ_WEIGHT(obj));
	strcat(buf, CAP(buf2));
	release_buffer(buf2);
	release_buffer(buf3);

	return (buf);
}


void Shop::List(char *arg, Character * ch, Character * keeper) {
	char *buf, *name;
	Object *obj, *last_obj = 0;
	int cnt = 0, index = 0;

	if (!IsOK(keeper, ch))
		return;
    

	if (lastsort < IS_CARRYING_N(keeper))
		SortKeeperObjs(keeper);

	buf = get_buffer(MAX_STRING_LENGTH);
	name = get_buffer(200);
  
	one_argument(arg, name);
  
	strcpy(buf, "&cB ##   Available   Item                                       Cost Weight&c0\n\r");
	strcat(buf, "&cL-------------------------------------------------------------------------&c0\n\r");
	START_ITER(iter, obj, Object *, keeper->contents) {
		if (obj->CanBeSeen(ch) && (obj->TotalValue() > 0)) {
			if (!last_obj) {
				last_obj = obj;
				cnt = 1;
			} else if (same_obj(last_obj, obj))
				cnt++;
			else {
				index++;
				if (!(*name) || isname(name, last_obj->Keywords()))
					strcat(buf, ListObject(ch, last_obj, cnt, index));
				cnt = 1;
				last_obj = obj;
			}
		}
	}
	index++;
	if (!last_obj) {
		if (*name)	strcpy(buf, "Presently, none of those are for sale.\r\n");
		else		strcpy(buf, "Currently, there is nothing for sale.\r\n");
	} else if (!(*name) || isname(name, last_obj->Keywords()))
		strcat(buf, ListObject(ch, last_obj, cnt, index));

	release_buffer(name);
	page_string(ch->desc, buf, true);
	release_buffer(buf);
}


bool Shop::OKRoom(VNum room) {
	for (int i = 0; i < in_room.Count(); i++)
		if (in_room[i] == room)
			return true;
	return false;
}


SPECIAL(shop_keeper) {
	Character *keeper = (Character *) me;
	Map<VNum, Shop>::Iterator	iter(Shop::Index);
	Shop *	shop;

	while ((shop = iter.Next()))
		if (shop->keeper == keeper->Virtual())
			break;

	if (!shop)	return false;

	if (shop->func)	/* Check secondary function */
		if ((shop->func) (ch, me, cmd, NULL))
			return true;

	if (keeper == ch) {
		if (cmd)	shop->lastsort = 0;	/* Safety in case "drop all" */
		return false;
	}
	
	if (!shop->OKRoom(IN_ROOM(ch)))
		return false;

	if (!AWAKE(keeper))
		return false;

	if (SHOP_IS_SELLING(shop->vnum)) {
		if (CMD_IS("buy")) {
			shop->Buy(argument, ch, keeper);
			return true;
		}
		else if (CMD_IS("sell")) {
			shop->Sell(argument, ch, keeper);
			return true;
		}
		else if (CMD_IS("value")) {
			shop->Value(argument, ch, keeper);
			return true;
		}
		else if (CMD_IS("list")) {
			shop->List(argument, ch, keeper);
			return true;
		}
	}

	if ((!IS_NPC(ch)) && (SHOP_IS_TRAINING(shop->vnum))) {
		if (CMD_IS("practice")) {
			shop->Practice(argument, ch, keeper);
			return true;
		}
	}

	return false;
}


int ok_damage_shopkeeper(Character * ch, Character * victim) {
	if (IS_NPC(victim) && (Character::Index[victim->Virtual()].func == shop_keeper))
		return (FALSE);
	return (TRUE);
}


int add_to_list(Vector<VNum> &list, int type, int *val) {
	if (*val >= 0) {
		if (type == LIST_PRODUCE)
			*val = Object::Find(*val) ? *val : -1;
		if (*val >= 0) {
			list.Append(*val);
		} else
			*val = 0;
		return (FALSE);
	}
	return (FALSE);
}


int add_to_list(Vector<BuyData> &list, int type, int *val) {
	if (*val >= 0) {
		if (*val >= 0) {
			list.Append(BuyData());
			list.Top().type = *val;
//			list.Top().keywords = NULL;
		} else
			*val = 0;
		return (FALSE);
	}
	return (FALSE);
}


void read_line(FILE * shop_f, const char *string, Ptr data)
{
  char *buf = get_buffer(MAX_STRING_LENGTH);
  if (!get_line(shop_f, buf) || !sscanf(buf, string, data)) {
    log("Error in shop #%d", Shop::Index.Top());
    exit(1);
  }
  release_buffer(buf);
}


void read_list(FILE * shop_f, Vector<VNum> &list, int type) {
	int temp;

	do {
		read_line(shop_f, "%d", &temp);
		add_to_list(list, type, &temp);
	} while (temp >= 0);
}


void read_type_list(FILE * shop_f, Vector<BuyData> &list) {
	int num;
	char *ptr, *buf;

	buf = get_buffer(MAX_STRING_LENGTH);
	do {
		fgets(buf, MAX_STRING_LENGTH - 1, shop_f);
		if ((ptr = strchr(buf, ';')) != NULL)	*ptr = 0;
		else									*(END_OF(buf) - 1) = 0;
		num = NOTHING;
		for (int i = 0; *item_types[i] != '\n'; i++) {
			if (!strn_cmp(item_types[i], buf, strlen(item_types[i]))) {
				num = i;
				strcpy(buf, buf + strlen(item_types[i]));
				break;
			}
		}
		ptr = buf;
		if (num == NOTHING) {
			sscanf(buf, "%d", &num);
			while (!isdigit(*ptr))	ptr++;
			while (isdigit(*ptr))	ptr++;
		}
		while (isspace(*ptr))				ptr++;
		while (isspace(*(END_OF(ptr) - 1)))	*(END_OF(ptr) - 1) = 0;
		add_to_list(list, LIST_TRADE, &num);
		if (*ptr)
			list.Top().keywords = str_dup(ptr);
	} while (num >= 0);
	release_buffer(buf);
}


void boot_the_shops(FILE * shop_f, char *filename, int rec_count) {
	char *buf, *buf2 = get_buffer(150);
	int temp, new_format = 0;
	int done = 0;

	sprintf(buf2, "beginning of shop file %s", filename);

	while (!done) {
		buf = fread_string(shop_f, buf2, filename);
		if (*buf == '#') {		/* New shop */
			sscanf(buf, "#%d\n", &temp);
			sprintf(buf2, "shop #%d in shop file %s", temp, filename);
			FREE(buf);		/* Plug memory leak! */
//			if (!Shop::Top)	Shop::Index.SetSize(rec_count);
			
			Shop &	shop = Shop::Index[temp];
			
			shop.vnum = temp;
			read_list(shop_f, shop.producing, LIST_PRODUCE);
			
			read_line(shop_f, "%f", &shop.profit_buy);
			read_line(shop_f, "%f", &shop.profit_sell);
			
			read_type_list(shop_f, shop.type);
			
			shop.no_such_item1 = fread_string(shop_f, buf2, filename);
			shop.no_such_item2 = fread_string(shop_f, buf2, filename);
			shop.do_not_buy = fread_string(shop_f, buf2, filename);
			shop.missing_cash1 = fread_string(shop_f, buf2, filename);
			shop.missing_cash2 = fread_string(shop_f, buf2, filename);
			shop.message_buy = fread_string(shop_f, buf2, filename);
			shop.message_sell = fread_string(shop_f, buf2, filename);
			int	temp;
			read_line(shop_f, "%d", &temp);
			read_line(shop_f, "%d", &temp);
			read_line(shop_f, "%d", &shop.keeper);

			shop.keeper = Character::Find(shop.keeper) ? shop.keeper : -1;
			read_line(shop_f, "%d", &shop.with_who);

			read_list(shop_f, shop.in_room, LIST_ROOM);

			read_line(shop_f, "%hu", &shop.open1);
			read_line(shop_f, "%hu", &shop.close1);
			read_line(shop_f, "%hu", &shop.open2);
			read_line(shop_f, "%hu", &shop.close2);

			shop.lastsort = 0;
			shop.func = 0;
		} else {
			if (*buf == '$')		/* EOF */
				done = TRUE;
			else if (strstr(buf, "v3.0"))	/* New format marker */
				new_format = 1;
			FREE(buf);		/* Plug memory leak! */
		}
	}
	release_buffer(buf2);
}


void assign_the_shopkeepers(void)
{
	cmd_say = find_command("say");
	cmd_tell = find_command("tell");
	cmd_emote = find_command("emote");
	cmd_slap = find_command("slap");
	cmd_cough = find_command("cough");
	Map<VNum, Shop>::Iterator	iter(Shop::Index);
	Shop *shop;
	while ((shop = iter.Next())) {
		if (shop->keeper == NOBODY)
			continue;
	// This check is necessary to prevent the shop_keeper function from being assigned to both
	// Character and Shop functions.	The result would be a recursive loop. -DH
		if (Character::Index[shop->keeper].func && Character::Index[shop->keeper].func != shop_keeper)
			shop->func = Character::Index[shop->keeper].func;
		Character::Index[shop->keeper].func = shop_keeper;
	}
}


const char *Shop::CustomerString(bool detailed) const {
	static char buf[256];
	int i;

	*buf = '\0';
	for (i = 0; *trade_letters[i] != '\n'; i++) {
		if (!IS_SET(with_who, 1 << i)) {
			if (detailed) {
				if (*buf) strcat(buf, ", ");
				strcat(buf, trade_letters[i]);
			} else
				sprintf(END_OF(buf), "%c", *trade_letters[i]);
		} else if (!detailed)
			strcat(buf, "_");
	}
	
	return buf;
}


void list_all_shops(Character * ch) {
	int shop_nr = 0;
	char	*buf = get_buffer(MAX_STRING_LENGTH);

//	strcpy(buf, "\r\n");
	Map<VNum, Shop>::Iterator	iter(Shop::Index);
	Shop *	shop;
	while ((shop = iter.Next())) {
		if (!(shop_nr % 19)) {
			strcat(buf, " ##   Virtual   Where    Keeper    Buy   Sell   Customers\r\n"
						"---------------------------------------------------------\r\n");
		}
		sprintf(END_OF(buf), "%3d   %6d   %6d    ", ++shop_nr, shop->vnum, shop->in_room[0]);
		if (shop->keeper < 0)	strcat(buf, "<NONE>");
		else					sprintf(END_OF(buf), "%6d", Character::Index[shop->keeper].vnum);
		sprintf(END_OF(buf), "   %3.2f   %3.2f    %s\r\n", shop->profit_sell, shop->profit_buy,
				shop->CustomerString(false));
				
		// CHANGEPOINT:
		// A bug in the buffer code does not allow it to appropriately handle
		// an overflow of the preset size.  Hence, we have to check and abort.
		
		if (strlen(buf) > (MAX_STRING_LENGTH - 200)) {
			strcat(buf, "** OVERFLOW **\r\n");
			break;
		}
	}
	/*
	 * There is a reason for putting these releases above the page_string.
	 * If page_string needs memory, we can use these instead of causing
	 * it to potentially have to create another buffer.
	 */
	page_string(ch->desc, buf, true);
	release_buffer(buf);
}


void handle_detailed_list(char *buf, char *buf1, Character * ch) {
	if ((strlen(buf1) + strlen(buf) < 78) || (strlen(buf) < 20))
		strcat(buf, buf1);
	else {
		strcat(buf, "\r\n");
		ch->Send(buf);
		sprintf(buf, "            %s", buf1);
	}
}


void Shop::ListDetailed(Character * ch) {
	Object *obj;
	int index;
	char	*buf = get_buffer(MAX_STRING_LENGTH),
			*buf1 = get_buffer(MAX_STRING_LENGTH);

	ch->Sendf("Vnum:       [%5d]\r\n", vnum);

	strcpy(buf, "Rooms:      ");
	for (index = 0; index < in_room.Count(); index++) {
		if (index)
			strcat(buf, ", ");
		if (!Room::Find(in_room[index]))
			sprintf(buf1, "%s (#%d)", world[in_room[index]].Name("<ERROR>"), in_room[index]);
		else
			sprintf(buf1, "<UNKNOWN> (#%d)", in_room[index]);
		handle_detailed_list(buf, buf1, ch);
	}
	if (!index)		ch->Send("Rooms:      None!\r\n");
	else			ch->Sendf("%s\r\n", buf);

	ch->Send("Shopkeeper: ");
	if (keeper != NOBODY) {
		ch->Sendf("%s (#%d), Special Function: %s\r\n",
				Character::Index[keeper].proto->RealName(),
				keeper, YESNO(func));
	} else
		ch->Send("<NONE>\r\n");

	strcpy(buf1, CustomerString(true));
	ch->Sendf("Customers:  %s\r\n", (*buf1) ? buf1 : "None");

	strcpy(buf, "Produces:   ");
	for (index = 0; index < producing.Count(); index++) {
		obj = Object::Index[producing[index]].proto;
		if (index)
			strcat(buf, ", ");
		sprintf(buf1, "%s (#%d)", obj->Name(), Object::Index[producing[index]].vnum);
		handle_detailed_list(buf, buf1, ch);
	}
	if (!index)	ch->Send("Produces:   Nothing!\r\n");
	else {
		strcat(buf, "\r\n");
		ch->Send(buf);
	}

	strcpy(buf, "Buys:       ");
	for (index = 0; index < type.Count(); index++) {
		if (index)
			strcat(buf, ", ");
		sprintf(buf1, "%s (#%d) ", item_types[type[index].type], type[index].type);
		if (type[index].keywords)
			sprintf(END_OF(buf1), "[%s]", type[index].keywords);
		else
			strcat(buf1, "[all]");
		handle_detailed_list(buf, buf1, ch);
	}
	if (!index)		ch->Send("Buys:       Nothing!\r\n");
	else {
		strcat(buf, "\r\n");
		ch->Send(buf);
	}

	ch->Sendf("Buy at:     [%4.2f], Sell at: [%4.2f], Open: [%d-%d, %d-%d]\r\n",
			profit_sell, profit_buy, open1, close1, open2, close2);

	release_buffer(buf1);
	release_buffer(buf);
}


void show_shops(Character * ch, char *arg) {
	Shop *	shop = NULL;

	if (!*arg)		list_all_shops(ch);
	else {
		if (!str_cmp(arg, ".")) {
			Map<VNum, Shop>::Iterator	iter(Shop::Index);
			while ((shop = iter.Next()))
				if (shop->OKRoom(IN_ROOM(ch)))
					break;

			if (!shop) {
				ch->Send("This isn't a shop!\r\n");
				return;
			}
		} else if (is_number(arg)) {
			VNum	shop_nr = atoi(arg);
			if (Shop::Index.Find(shop_nr))	shop = &Shop::Index[shop_nr];
		}

		if (!shop)	ch->Send("Illegal shop number.\r\n");
		else		shop->ListDetailed(ch);
	}
}


extern void fprintstring(FILE *fp, const char *fmt, const char *txt);
extern void fprintbits(FILE *fp, const char *fmt, UInt32 bits, const char *bitnames[]);

void Shop::SaveDisk(Shop &shop, FILE *fp)
{
	int i = shop.vnum;

	fprintf(fp, "#%d = {\n", i);
	if (shop.profit_buy != 1)	fprintf(fp, "\tbuyprofit = %.2f;\n", shop.profit_buy);
	if (shop.profit_sell != 1)	fprintf(fp, "\tsellprofit = %.2f;\n", shop.profit_sell);
	if (shop.keeper != -1)		fprintf(fp, "\tkeeper = %d;\n", shop.keeper);
	if (shop.with_who)			fprintbits(fp, "\tnotwith = { %s };\n", shop.with_who, trade_letters);

	if (shop.no_such_item1)		fprintstring(fp, "\tno_item1 = %s;\n", shop.no_such_item1);
	if (shop.no_such_item2)		fprintstring(fp, "\tno_item2 = %s;\n", shop.no_such_item2);
	if (shop.missing_cash1)		fprintstring(fp, "\tno_cash1 = %s;\n", shop.missing_cash1);
	if (shop.missing_cash2)		fprintstring(fp, "\tno_cash2 = %s;\n", shop.missing_cash2);
	if (shop.do_not_buy)		fprintstring(fp, "\tdo_not_buy = %s;\n", shop.do_not_buy);
	if (shop.message_buy)		fprintstring(fp, "\tmessage_buy = %s;\n", shop.message_buy);
	if (shop.message_sell)		fprintstring(fp, "\tmessage_sell = %s;\n", shop.message_sell);

	if (shop.prac_not_known)	fprintstring(fp, "\tprac_not_known = %s;\n", shop.prac_not_known);
	if (shop.prac_missing_cash)	fprintstring(fp, "\tprac_missing_cash = %s;\n", shop.prac_missing_cash);
	
	if (shop.bitvector)			fprintbits(fp, "\tflags = { %s };\n", shop.bitvector, shop_bits);

	if (shop.open1)				fprintf(fp, "\topen1 = %d;\n", shop.open1);
	if (shop.open2)				fprintf(fp, "\topen2 = %d;\n", shop.open2);
	if (shop.close1 != 28)		fprintf(fp, "\tclose1 = %d;\n", shop.close1);
	if (shop.close2)			fprintf(fp, "\tclose2 = %d;\n", shop.close2);
	
	if (shop.type.Count()) {
		for (int i = 0; i < shop.type.Count(); i++) {
			BuyData &	data = shop.type[i];
			if (data.type == -1)	break;
			fprintf(fp, "\ttype = {\n"
						"\t\ttype = %d;\n", data.type);
			if (data.keywords && *data.keywords)
				fprintstring(fp, "\t\tkeywords = %s;\n", data.keywords);
			fprintf(fp, "\t};\n");
		}
	}
	
	if (shop.in_room.Count() && shop.in_room[0] != -1) {
		fprintf(fp, "\trooms = { ");
		for (int i = 0; i < shop.in_room.Count(); i++) {
			VNum	room = shop.in_room[i];
			if (room == -1)	break;
			fprintf(fp, "%d ", room);
		}
		fprintf(fp, "};\n");
	}
	
	if (shop.producing.Count() && shop.producing[0] != -1) {
		fprintf(fp, "\tproduces = { ");
		for (int i = 0; i < shop.producing.Count(); i++) {
			VNum	product = shop.producing[i];
			if (product == -1)	break;
			fprintf(fp, "%d ", product);
		}
		fprintf(fp, "};\n");
	}
	
	fprintf(fp, "};\n");
}


// Guild-related functions imported from Phase I's guild.c

int Shop::PracPrice(Character *ch, SInt16 skillnum, int i)
{
	float howmuch = 0.0;
	
	howmuch = Skills[skillnum]->startmod * Skills[skillnum]->maxincr;
	
	if (howmuch < 0)
		howmuch = -howmuch;
		
	howmuch = howmuch * (20 + i) * profit_buy;
	howmuch *= 0.07 * (10 - MAX(MIN(GET_CHA(ch), 15), 6)) + 1;
	
	return ((int) howmuch);
}


void Shop::ShowGMSkills(Character * ch, Character * keeper)
{
	SInt16 i, sortpos, tempvalue, temptheory = 0;
	SkillKnown *ch_skill = NULL, *keeper_skill = NULL;
	extern SInt16 spell_sort_info[TOP_SKILLO_DEFINE + 1];
	extern const char *named_bases[];
	
	extern int avg_base_chance(Character *ch, SInt16 skillnum);
	extern char *how_good(int percent, int theory);

	*buf = '\0';

	if (!GET_PRACTICES(ch))
		strcpy(buf, "&cGYou have no training sessions remaining.&c0\r\n");
	else
		sprintf(buf, "&cGYou have &cY%d&cG training session%s remaining.&c0\r\n",
						GET_PRACTICES(ch), (GET_PRACTICES(ch) == 1 ? "" : "s"));

	strcat(buf,	 "&cGI can teach you the following skills:\r\n\r\n"
		"&cWSkill                         Proficiency  Base Stats     Mod 3d6  Pts  Gold\r\n"
		"&cL----------------------------------------------------------------------------&c0\r\n");


	CHAR_SKILL(keeper).GetTable(&keeper_skill);
	
	while (keeper_skill) {
		if (strlen(buf) >= MAX_STRING_LENGTH - 100) {
			strcat(buf, "**OVERFLOW**\r\n");
			break;
		}

		i = keeper_skill->skillnum;

		FINDSKILL(ch, i, ch_skill);

		// if ((SKILLCHANCE(keeper, i, keeper_skill) > SKILL_NOCHANCE) && 
		//		(GET_SKILL(keeper, i, keeper_skill))) {
		if ((GET_SKILL(keeper, i)) && 
			 (CHAR_SKILL(ch).CheckSkillPrereq(ch, i))) {
			 
			if (ch_skill) {
				tempvalue = ch_skill->proficient;
				ch_skill->proficient = ch_skill->theory;
				temptheory = SKILLCHANCE(ch, i, ch_skill);
				ch_skill->proficient = tempvalue;
				tempvalue = SKILLCHANCE(ch, i, ch_skill);
			} else {
				tempvalue = SKILLCHANCE(ch, i, ch_skill);
				temptheory = tempvalue;
			}
			
			sprintbit(SKILL_BASE(i), named_bases, buf3);
			if (tempvalue != SKILL_NOCHANCE)
				sprintf(buf2, "%3d %3d", (tempvalue - avg_base_chance(ch, i)), tempvalue);
			else
				strcpy(buf2, "  -   -");
			sprintf(buf + strlen(buf), "&cc%-28s %-16s &cb[&cB%-12.12s&cb] &cG%-7s %4d  &cY%d&c0\r\n", 
						 Skill::Name(i), 
			 how_good(tempvalue, temptheory),
			 buf3,
			 buf2,
			 GET_SKILLTHEORY(ch, i),
			 PracPrice(ch, i, GET_SKILLTHEORY(ch, i)));
		}

		keeper_skill = keeper_skill->next;
	}

	page_string(ch->desc, buf, 1);
	*buf = '\0';
}


void Shop::Practice(char *argument, Character *ch, Character *keeper)
{
	SInt16 skillnum = -1;

	skip_spaces(argument);

	if (!IsOKChar(keeper, ch))
		return;

	if (!*argument) {
		ShowGMSkills(ch, keeper);
		return;
	}

	// Locate the offset in spells[]
	skillnum = Skill::Number(argument);

#ifdef TLO_DEBUG_MESSAGES
	// A diagnostic message.
	if (skillnum != -1) {
		ch->Sendf("Practicing %s	PreReq1: %d	PreReq2: %d\r\n", 
			Skill::Name(skillnum), skill_info[skillnum].prereq1, 
			skill_info[skillnum].prereq2);
	}
#endif

	// If the keeper has no practice in the skill or cannot perform it,
	// we'll act as if the skill doesn't exist.
	// if ((GET_SKILL(keeper, skillnum, tmpskill) == 0) || 
	//		(SKILLCHANCE(keeper, skillnum, tmpskill) == SKILL_NOCHANCE))
	if ((skillnum > 0) && (GET_SKILL(keeper, skillnum) == 0) &&
		 (CHAR_SKILL(ch).CheckSkillPrereq(ch, skillnum)))
		skillnum = -1;

	// If the skill doesn't exist or GM doesn't know it, tell the player off.
	if (skillnum == -1) {
		perform_tell(keeper, ch, prac_not_known);
		return;
	}

	if (GET_PRACTICES(ch) <= 0) {
		ch->Send("You cannot focus on practicing right now.\r\n");
		return;
	}

	if (!CHAR_SKILL(ch).CheckSkillPrereq(ch, skillnum)) {
		ch->Sendf("Such things are beyond your grasp.\r\n");
		return;
	}
	
/*	Is the player maxxed out with the skill?	*/

	if (GET_SKILL(ch, skillnum) >= GET_SKILL(keeper, skillnum)) {
		perform_tell(keeper, ch, "You have learned all I have to teach you.");
		return;
	}

	if (GET_SKILLTHEORY(ch, skillnum) >= (GET_SKILL(ch, skillnum) + 10)) {
		ch->Send("You need more use out in the field before training further.\r\n");
		return;
	}

/*	Does the Player have enough gold to train?	*/

	if (GET_GOLD(ch) < PracPrice(ch, skillnum, GET_SKILLTHEORY(ch, skillnum))) {
		perform_tell(keeper, ch, prac_missing_cash);
		return;
	}

/*	If we've made it this far, then its time to practice	*/

	ch->Send("You practice for a while...\r\n");
	GET_PRACTICES(ch) -= 1;
	GET_GOLD(ch) -= PracPrice(ch, skillnum, GET_SKILLTHEORY(ch, skillnum));
	GET_TOTALPTS(ch) += CHAR_SKILL(ch).LearnTheory(skillnum, NULL, 1);
	// SET_SKILL(ch, skillnum, MIN(LEARNED(skillnum), percent));

	if (GET_SKILL(ch, skillnum) >= 250)
		ch->Send("You have learned all there is to know in that area.\r\n");
 
	return;
}
