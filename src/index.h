/*************************************************************************
*   File: index.h                    Part of Aliens vs Predator: The MUD *
*  Usage: Header file for indexes                                        *
*************************************************************************/

#ifndef	__INDEX_H__
#define __INDEX_H__

#include "types.h"
#include "internal.defs.h"

#include "stl.map.h"
#include "stl.vector.h"
#include "stl.llist.h"

class TrigVarData;
// class Character;

template <class T>
class IndexData {
public:
							IndexData(void);
							IndexData(const IndexData<T> &i);
							~IndexData(void);
					
	IndexData<T> &			operator=(const IndexData<T> &i);
						
	VNum					vnum;				//	virtual number
	SInt32					number;				//	number of existing units
	Vector<VNum>			triggers;			//	triggers for mob/obj
	LList<TrigVarData *>	variables;			//	list of local vars for trigger
	T *						proto;
							SPECIAL(*func);		//	specfunc
	char					*farg;				//	specfunc arg
};


template <class T>
IndexData<T>::IndexData(void) : vnum(-1), number(0), proto(NULL), func(NULL), farg(NULL) {
}


template <class T>
IndexData<T>::IndexData(const IndexData &i) : vnum(i.vnum), number(i.number), triggers(i.triggers),
		variables(i.variables), proto(i.proto), func(i.func), farg(strdup(i.farg)) {
}


template <class T>
IndexData<T>::~IndexData(void) {
}


template<class T>
IndexData<T> &IndexData<T>::operator=(const IndexData<T> &i) {
	vnum = i.vnum;
	number = i.number;
	triggers = i.triggers;
	variables = i.variables;
	proto = i.proto;
	func = i.func;
	farg = strdup(i.farg);
	
	return *this;
}


template <class T>
class Index : public Map<VNum, IndexData<T> > {
public:
	VNum		Find(T *t) const;
};


template<class T>
VNum Index<T>::Find(T *t) const {
	for (int i = 0; i < vector.Count(); i++)
		if (vector[i].element.proto == t)
			return vector[i].index;
	return -1;
}


#endif

