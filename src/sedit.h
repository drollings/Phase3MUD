

#ifndef __SEDIT_H__
#define __SEDIT_H__


#include "editor.h"
#include "shop.h"

class SEdit : public Editor {
public:
						SEdit(Descriptor *d, VNum newVNum);
						SEdit(Descriptor *d, Shop *shopEntry);
	virtual				~SEdit(void) { };
	
	virtual void		SaveInternally(void);
	
	virtual void		Menu(void);
	virtual void		Parse(char *arg);
	Ptr					Pointer(void) {		return (Ptr) shop;	}

	virtual void		Finish(FinishMode mode);

protected:
	void				RemoveFromTypeList(BuyData **list, int num);
	void				AddToTypeList(BuyData **list, BuyData *new_t);
	void				AddToIntList(VNum **list, VNum new_t);
	void				RemoveFromIntList(VNum **list, VNum num);
	void				ModifyString(char **str, char *new_t);
	
	void				DispProductsMenu(void);
	void				DispCompactRoomsMenu(void);
	void				DispRoomsMenu(void);
	void				DispNamelistMenu(void);
	void				DispShopFlagsMenu(void);
	void				DispNotradeMenu(void);
	void				DispTypesMenu(void);
	
protected:
	Shop *				shop;
	
	enum {
		ProductsMenu = 2000,
		RoomsMenu,
		NamelistMenu,
		
		BuyProfit,
		SellProfit,
		NoItem1,
		NoItem2,
		NoCash1,
		NoCash2,
		NoBuy,
		Buy,
		Sell,
		PracNoSkill,
		PracNoCash,
		
		NamelistEntry,
		
		NumericalResponse = 2050,
		
		TypeMenu,
		ShopFlags,
		NotradeMenu,
		
		ShopKeeper,
		ShopOpen1,
		ShopClose1,
		ShopOpen2,
		ShopClose2,
		
		DeleteType,
		NewProduct,
		DeleteProduct,
		NewRoom,
		DeleteRoom,
        PracNotKnown,
        PracMissingCash
        
	};
};

#endif

