

#ifndef __SCRIPTEDIT_H__
#define __SCRIPTEDIT_H__


#include "editor.h"
#include "scripts.h"
#include "index.h"

class ScriptEdit : public Editor {
public:
						ScriptEdit(Descriptor *d, VNum newVNum);
						ScriptEdit(Descriptor *d, VNum newVNum, IndexData<Trigger> &trigEntry);
	virtual				~ScriptEdit(void) { };
	
	virtual void		SaveInternally(void);
	
	virtual void		Menu(void);
	virtual void		Parse(char *arg);
	Ptr					Pointer(void) {		return (Ptr) trig;	}

	virtual void		Finish(FinishMode mode);

protected:
	Trigger *			trig;
	
	enum {
		ScriptScript = 2000
	};
};

#endif

