

#ifndef __MEDIT_H__
#define __MEDIT_H__


#include "editor.h"
#include "characters.h"
#include "index.h"

class MEdit : public Editor {
public:
						MEdit(Descriptor *d, VNum newVNum);
						MEdit(Descriptor *d, VNum newVNum, IndexData<Character> &mobEntry);
	virtual				~MEdit(void) { };
	
	virtual void		SaveInternally(void);
	static void			SaveDisk(SInt32 unused);
	
	virtual void		Menu(void);
	virtual void		Parse(char *arg);
	Ptr					Pointer(void) {		return (Ptr) mob;	}

	virtual void		Finish(FinishMode mode);

protected:
	void				DispPositions(void);
	void				DispSex(void);
	void				DispAttackTypes(void);
	void				DispRaces(void);
	void				DispMobFlags(void);
	void				DispAffFlags(void);
	void				DispExtendedMenu(void);
	void				DispOpinion(Opinion *op, SInt32 num);
	void				DispOpinionMenu(void);
	void				DispOpinionGenders(void);
	void				DispOpinionRaces(void);
	void 				DispSkills(void);
	
protected:
	Character *			mob;
	Vector<VNum>		triggers;
	Opinion *			opinion;
	
	
	enum {
		OpinionsMenu = 2000,
		
		SkillMenu,
        SkillPoints,
		MobClan,
		
		ExtendedMenu,
		
		OpinionsHateSex,
		OpinionsHateRace,
		OpinionsHateVNum,
		OpinionsFearSex,
		OpinionsFearRace,
		OpinionsFearVNum
	};
};


#endif

