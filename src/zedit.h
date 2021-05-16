

#ifndef __ZEDIT_H__
#define __ZEDIT_H__


#include "editor.h"
/* #include "db.h" */

class ZEdit : public Editor {
public:
						ZEdit(Descriptor *d, VNum room, VNum zoneNumber);
	virtual				~ZEdit(void) { };
	
	virtual void		SaveInternally(void);
	static void			SaveDisk(SInt32 zone_num);
	
	virtual void		Menu(void);
	virtual void		Parse(char *arg);
	Ptr					Pointer(void) {		return (Ptr) zone;	}

	virtual void		Finish(FinishMode mode);

public:
	static void			NewZone(Character *ch, VNum vzone_num);

protected:
	bool				NewCommand(SInt32 pos);
	void				DeleteCommand(SInt32 pos);
	bool				StartChangeCommand(SInt32 pos);
	void				DispCommandMenu(void);
	void				DispCommandType(void);
	void				DispArg1(void);
	void				DispArg2(void);
	void				DispArg3(void);
	void				DispArg4(void);
	void				DispRepeat(void);
	void				DispClimateMenu(void);
	
protected:
	zone_data *			zone;
	
	enum {
		ZoneName,
		ZoneTop,
		ZoneLife,
		ZoneReset,
		NewEntry,
		ChangeEntry,
		DeleteEntry,
		CommandMenu,
		CommandType,
		Arg1,
		Arg2,
		Arg3,
		Arg4,
		Repeat,
		IfFlag,
		ZoneClimate,
		ZoneClimateEnergy,
		ZoneClimatePattern,
		ZoneClimateFlags,
		ZoneClimateTemperature,
		ZoneClimateWindDir,
		ZoneClimateWindVar,
		ZoneClimatePrecipitation
	};
};

#endif

