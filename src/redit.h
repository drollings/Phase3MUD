

#ifndef __REDIT_H__
#define __REDIT_H__


#include "editor.h"
#include "rooms.h"
#include "extradesc.h"

class REdit : public Editor {
public:
						REdit(Descriptor *d, VNum newVNum, bool showmenu = true);
						REdit(Descriptor *d, VNum newVNum, Room &roomEntry, bool showmenu = true);
	virtual				~REdit(void) { };
	
	virtual void		SaveInternally(void);
	
	virtual void		Menu(void);
	virtual void		Parse(char *arg);
	Ptr					Pointer(void) {		return (Ptr) room;	}

	virtual void		Finish(FinishMode mode);

protected:
	static void			FreeWorldRoom(Room &room);
	static void			FreeRoom(Room *room);
	
	virtual void		DispExtradescMenu(void);
	virtual void		DispExitMenu(void);
	virtual void		DispExitFlagsMenu(void);
	virtual void		DispFlagsMenu(void);
	virtual void		DispSectorsMenu(void);
	
protected:
	Room *				room;
	Vector<VNum>		triggers;
	ExtraDesc *			desc;
	
	enum {
		ExitMenu = 2000,
		ExtradescKey,
		ExtradescDesc,
		ExtradescMenu
	};

};

#define OLC_EXIT room->dir_option[value]

#endif

