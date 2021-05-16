/*************************************************************************
*   File: extradesc.h                Part of Aliens vs Predator: The MUD *
*  Usage: Header file for Extra Descriptions                             *
*************************************************************************/


#ifndef __EXTRADESC_H__
#define __EXTRADESC_H__


#include "types.h"
#include "stl.llist.h"

// Extra description: used in objects, mobiles, and rooms
class ExtraDesc {
public:
					ExtraDesc(void);
					ExtraDesc(const ExtraDesc &ed);
					~ExtraDesc(void);
					
	ExtraDesc *		Share(void);
	void			Free(void);
					
	SString *		keyword;				//	Keyword in look/examine
	SString *		description;			//	What to see
	
private:
	UInt32			count;
	
public:
	static const char *	Find(const char *word, LList<ExtraDesc *> &list);
};

inline ExtraDesc::ExtraDesc(void) : keyword(NULL), description(NULL), count(1) { }
inline ExtraDesc::ExtraDesc(const ExtraDesc &ed) :
		keyword(SSShare(ed.keyword)), description(SSShare(ed.description)), count(1) { }

inline ExtraDesc::~ExtraDesc(void) {
	SSFree(keyword);
	SSFree(description);
}

inline ExtraDesc *ExtraDesc::Share(void) {
	count++;
	return this;
}

inline void ExtraDesc::Free(void) {
	count--;
	if (!count)	delete this;
}
/*
ExtraDesc *EDCreate(char *str, char *desc);
ExtraDesc *EDShare(ExtraDesc *shared);
void EDFree (ExtraDesc *shared);
*/


#endif
