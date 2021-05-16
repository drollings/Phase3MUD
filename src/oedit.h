

#ifndef __OEDIT_H__
#define __OEDIT_H__


#include "editor.h"
#include "objects.h"
#include "extradesc.h"
#include "index.h"

class OEdit : public Editor {
public:
						OEdit(Descriptor *d, VNum newVNum);
						OEdit(Descriptor *d, VNum newVNum, IndexData<Object> &objEntry);
	virtual				~OEdit(void) { };
	
	virtual void		SaveInternally(void);
	static void			SaveDisk(SInt32 zone_num);
	
	virtual void		Menu(void);
	virtual void		Parse(char *arg);
	Ptr					Pointer(void) {		return (Ptr) obj;	}

	virtual void		Finish(FinishMode mode);

protected:
	virtual void		DispContainerFlagsMenu(void);
	virtual void		DispExtradescMenu(void);
	virtual void		DispPromptApplyMenu(void);
	virtual void		DispApplyMenu(void);
	virtual void		DispVehicleFlagsMenu(void);
	virtual void		DispValues(void);
	virtual void		DispValueMenu(void);
	virtual void		DispTypesMenu(void);
	virtual void		DispExtraMenu(void);
	virtual void		DispWearMenu(void);
	virtual void		DispAffectsMenu(void);
	virtual void		DispScriptsMenu(void);

protected:
	Object *			obj;
	Vector<VNum>		triggers;
	ExtraDesc *			desc;
	
	enum {
		PromptApply = 2000,
		ApplyMenu,
		ApplyMod,
		ValueMenu,
		ExtradescMenu,
		ExtradescKey,
		ExtradescDesc,
		WizardType,
		WizardWear,
		WizardQuality
	};
};


#endif

