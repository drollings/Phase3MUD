/* ************************************************************************
*   File: shop.h                                        Part of CircleMUD *
*  Usage: shop file definitions, structures, constants                    *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#ifndef __SHOP_H__
#define __SHOP_H__


#include "stl.vector.h"


class BuyData {
public:
				BuyData(void) : type(-1), keywords(NULL) { };
				BuyData(const BuyData &b) : type(b.type), keywords(str_dup(b.keywords)) { }
				~BuyData(void) { if (keywords)	free(keywords); };
	BuyData &	operator=(const BuyData &b) { type = b.type; keywords = str_dup(b.keywords); return *this; }
				
	int			type;
	char *		keywords;
};


class Shop {
public:
					Shop(void);
					Shop(const Shop &s);
					~Shop(void);
					
	Shop &			operator=(const Shop &s);
	bool			operator==(const Shop &s);
					
	//	Possible states for objects trying to be sold
	enum TradeState {
		OBJECT_DEAD,
		OBJECT_NOTOK,
		OBJECT_OK,
		OBJECT_NOVAL,
		OBJECT_CONT_NOTOK
	};

	bool			IsOKChar(Character * keeper, Character * ch);
	bool			IsOK(Character *keeper, Character *ch);
	bool			IsOpen(Character *keeper, bool msg);
	bool			OKRoom(VNum room);
	
	VNum			Virtual();				//	Virtual number of this shop

	TradeState		TradeWith(Object *item);
	Object *		GetSellingObj(Character * ch, char *name, Character * keeper, bool msg);
	Object *		GetPurchaseObj(Character * ch, char *name, Character * keeper, bool msg);
	bool			Producing(Object *item);
	int 			BuyPrice(Character * ch, Object * obj);
	int				SellPrice(Character * ch, Object * obj);
	Object *		SlideObj(Object * obj, Character * keeper);
	void			SortKeeperObjs(Character * keeper);
	
	char *			ListObject(Character *ch, Object * obj, int cnt, int index);
	const char *	CustomerString(bool detailed) const;
	void			ListDetailed(Character *ch);
	
	void			Buy(char *arg, Character * ch, Character * keeper);
	void			Sell(char *arg, Character * ch, Character * keeper);
	void			Value(char *arg, Character * ch,  Character * keeper);
	void			List(char *arg, Character * ch, Character * keeper);

	void 			Practice(char *argument, Character *ch, Character *keeper);
	int 			PracPrice(Character *ch, SInt16 skillnum, int i);
	void 			ShowGMSkills(Character * ch, Character * keeper);

	void			Parser(char *input);

	VNum			vnum;				//	Virtual number of this shop
	Vector<VNum>	producing;			//	Which item to produce (virtual)
	float			profit_buy,			//	Factor to multiply cost with
					profit_sell;		//	Factor to multiply cost with
	Vector<BuyData>	type;				//	Which items to trade
	char *			no_such_item1;		//	Message if keeper hasn't got an item
	char *			no_such_item2;		//	Message if player hasn't got an item
	char *			missing_cash1;		//	Message if keeper hasn't got cash
	char *			missing_cash2;		//	Message if player hasn't got cash
	char *			do_not_buy;			//	If keeper dosn't buy such things
	char *			message_buy;		//	Message when player buys item
	char *			message_sell;		//	Message when player sells item
	char *			prac_not_known;		//	Message if keeper doesn't know skills
	char *			prac_missing_cash;	//	Message if trainee hasn't got cash
	Flags			with_who;			//	Who does the shop trade with?
	Flags			bitvector;			//	Extra shop attributes
	VNum			keeper;				//	The mobil who owns the shop (virtual)
	Vector<VNum>	in_room;			//	Where is the shop?
	UInt16			open1,				//	When does the shop open?
					open2,
					close1,				//	When does the shop close?
					close2;
	int				lastsort;			//	How many items are sorted in inven?
					SPECIAL(*func);		//	Secondary spec_proc for shopkeeper

public:
	static Map<VNum, Shop>	Index;

	static void SaveDisk(Shop &shop, FILE *fp);

};


inline VNum Shop::Virtual(void)
	{	return vnum;	}

const SInt8 MAX_SHOP_OBJ = 100;		//	"Soft" maximum for list maximums


/* Types of lists to read */
enum{
	LIST_PRODUCE,
	LIST_TRADE,
	LIST_ROOM
};


/* Whom will we not trade with (bitvector for SHOP_TRADE_WITH()) */
#define TRADE_NOGOOD		(1 << 0)
#define TRADE_NOEVIL		(1 << 1)
#define TRADE_NONEUTRAL		(1 << 2)
#define TRADE_NOHYUMANN		(1 << 3)
#define TRADE_NOELF			(1 << 4)
#define TRADE_NODWOMAX		(1 << 5)
#define TRADE_NOSHASTICK	(1 << 6)
#define TRADE_NOMURRASH		(1 << 7)
#define TRADE_NOLAORI		(1 << 8) 
#define TRADE_NOTHADYRI		(1 << 9)
#define TRADE_NOTERRAN		(1 << 10)
#define TRADE_NOTRULOR		(1 << 11)


class StackData {
public:
						StackData(void);
						~StackData(void);
	int					Top(void) const;
	int					Pop(void);
	void				Push(int pushval);
	void				EvaluateOperation(StackData &values);
private:
	int					data[100];
	int					length;
};

inline StackData::StackData(void) : length(0) {
	memset(&data, 0, sizeof(int) * 100);
}

inline StackData::~StackData(void) { }


/* Which expression type we are now parsing */
enum {
	OPER_OPEN_PAREN,
	OPER_CLOSE_PAREN,
	OPER_OR,
	OPER_AND,
	OPER_NOT,
	MAX_OPER = OPER_NOT
};


#define SHOP_NUM(i)			(Shop::Index[(i)].vnum)
#define SHOP_KEEPER(i)		(Shop::Index[(i)].keeper)
#define SHOP_OPEN1(i)		(Shop::Index[(i)].open1)
#define SHOP_CLOSE1(i)		(Shop::Index[(i)].close1)
#define SHOP_OPEN2(i)		(Shop::Index[(i)].open2)
#define SHOP_CLOSE2(i)		(Shop::Index[(i)].close2)
#define SHOP_ROOM(i, num)	(Shop::Index[(i)].in_room[(num)])
#define SHOP_BUYTYPE(i, num)	(Shop::Index[(i)].type[(num)].type)
#define SHOP_BUYWORD(i, num)	(Shop::Index[(i)].type[(num)].keywords)
#define SHOP_PRODUCT(i, num)	(Shop::Index[(i)].producing[(num)])
#define SHOP_TRADE_WITH(i)	(Shop::Index[(i)].with_who)
#define SHOP_SORT(i)		(Shop::Index[(i)].lastsort)
#define SHOP_BUYPROFIT(i)	(Shop::Index[(i)].profit_buy)
#define SHOP_SELLPROFIT(i)	(Shop::Index[(i)].profit_sell)
#define SHOP_BITVECTOR(i)	(Shop::Index[(i)].bitvector)
#define SHOP_FUNC(i)		(Shop::Index[(i)].func)

#define NOTRADE_GOOD(i)			(IS_SET(SHOP_TRADE_WITH((i)), TRADE_NOGOOD))
#define NOTRADE_EVIL(i)			(IS_SET(SHOP_TRADE_WITH((i)), TRADE_NOEVIL))
#define NOTRADE_NEUTRAL(i)		(IS_SET(SHOP_TRADE_WITH((i)), TRADE_NONEUTRAL))
#define NOTRADE_HYUMANN(i)		(IS_SET(SHOP_TRADE_WITH((i)), TRADE_NOHYUMANN))
#define NOTRADE_ELF(i)			(IS_SET(SHOP_TRADE_WITH((i)), TRADE_NOELF))
#define NOTRADE_DWOMAX(i)		(IS_SET(SHOP_TRADE_WITH((i)), TRADE_NODWOMAX))
#define NOTRADE_SHASTICK(i)		(IS_SET(SHOP_TRADE_WITH((i)), TRADE_NOSHASTICK))
#define NOTRADE_MURRASH(i)		(IS_SET(SHOP_TRADE_WITH((i)), TRADE_NOMURRASH))
#define NOTRADE_LAORI(i)		(IS_SET(SHOP_TRADE_WITH((i)), TRADE_NOLAORI))
#define NOTRADE_THADYRI(i)		(IS_SET(SHOP_TRADE_WITH((i)), TRADE_NOTHADYRI))
#define NOTRADE_TERRAN(i)		(IS_SET(SHOP_TRADE_WITH((i)), TRADE_NOTERRAN))
#define NOTRADE_TRULOR(i)		(IS_SET(SHOP_TRADE_WITH((i)), TRADE_NOTRULOR))


extern const char * MSG_NOT_OPEN_YET;
extern const char * MSG_NOT_REOPEN_YET;
extern const char * MSG_CLOSED_FOR_DAY;
extern const char * MSG_NO_SEE_CHAR;
extern const char * MSG_NO_SELL_RACE;
extern const char * MSG_NO_SELL_CLASS;
extern const char * MSG_NO_USED_WANDSTAFF;
extern const char * MSG_CANT_KILL_KEEPER;
extern const char * MSG_TRAINER_DISLIKE_CLASS;
extern const char * MSG_TRAINER_DISLIKE_RACE;

#define WILL_START_FIGHT	(1 << 0)
#define WILL_BANK_MONEY		(1 << 1)
#define IS_SHOPKEEPER		(1 << 2)
#define IS_GUILDMASTER		(1 << 3)

#define SHOP_KILL_CHARS(i)	(IS_SET(SHOP_BITVECTOR(i), WILL_START_FIGHT))
#define SHOP_USES_BANK(i)	(IS_SET(SHOP_BITVECTOR(i), WILL_BANK_MONEY))
#define SHOP_IS_SELLING(i)	(IS_SET(SHOP_BITVECTOR(i), IS_SHOPKEEPER))
#define SHOP_IS_TRAINING(i)	(IS_SET(SHOP_BITVECTOR(i), IS_GUILDMASTER))

#endif

