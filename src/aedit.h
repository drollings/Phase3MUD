
#ifndef __AEDIT_H__
#define __AEDIT_H__


#include "editor.h"
#include "socials.h"

class AEdit : public Editor {
public:
						AEdit(Descriptor *d, char *actionName);
						AEdit(Descriptor *d, VNum num, Social &social);
	virtual				~AEdit(void) { };
	
	virtual void		SaveInternally(void);
	static void			SaveDisk(SInt32 unused);
	
	virtual void		Menu(void);
	virtual void		Parse(char *arg);
	Ptr					Pointer(void) {		return (Ptr) action;	}

	virtual void		Finish(FinishMode mode);

protected:
	Social *			action;
	
	enum {
		ActionName,
		SortAs,
		MinCharPos,
		MinVictPos,
		MinCharLevel,
		NoVictChar,
		NoVictOthers,
		VictFoundChar,
		VictFoundOthers,
		VictFoundVict,
		VictNotFound,
		SelfChar,
		SelfOthers,
		VictFoundBodyChar,
		VictFoundBodyOthers,
		VictFoundBodyVict,
		ObjFoundChar,
		ObjFoundOthers
	};
};


#endif

