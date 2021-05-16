

#ifndef __CEDIT_H__
#define __CEDIT_H__


#include "editor.h"
#include "clans.h"

class CEdit : public Editor {
public:
						CEdit(Descriptor *d, VNum newNum);
						CEdit(Descriptor *d, const Clan &clanEntry);
	virtual				~CEdit(void) { };
	
	virtual void		SaveInternally(void);
	static void			SaveDisk(SInt32 unused);
	
	virtual void		Menu(void);
	virtual void		Parse(char *arg);
	Ptr					Pointer(void) {		return (Ptr) clan;	}

	virtual void		Finish(FinishMode mode);
	
protected:
	virtual void		RankMenu(void);
	
protected:
	Clan *				clan;
	
	enum {
		ClanName,
		ClanDescription,
		ClanOwner,
		ClanRoom,
		ClanSavings,
		ClanRanks,
		ClanRankEntry
	};
};


#endif

