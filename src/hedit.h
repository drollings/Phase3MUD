
#ifndef __HEDIT_H__
#define __HEDIT_H__


#include "editor.h"
#include "help.h"

class HEdit : public Editor {
public:
						HEdit(Descriptor *d, char *helpName);
						HEdit(Descriptor *d, Help &helpItem);
	virtual				~HEdit(void) { };
	
	virtual void		SaveInternally(void);
	static void			SaveDisk(SInt32 unused);
	
	virtual void		Menu(void);
	virtual void		Parse(char *arg);
	Ptr					Pointer(void) {		return (Ptr) help;	}
	
	virtual void		Finish(FinishMode mode);
	
	static VNum			Find(const char *keyword);
	
protected:
	Help *				help;
	
	enum {
		Keywords,
		Entry,
		MinTrust,
		Prerequisite,
		Chance
	};
};


#endif

