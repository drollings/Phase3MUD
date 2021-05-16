#ifndef __SAFEPTR_H__
#define __SAFEPTR_H__


#include <list>

#ifdef macintosh
#include "null.h"
#endif


// A reference-counted safe pointer implementation by Daniel Rollings AKA Daniel Houghton

class SafePtrHandle;
class SafePtrList;


class SafePtrList {
public:
						SafePtrList(void) {};
	virtual				~SafePtrList(void)	{	ClearSafePtrs();	}

private:
	void				ClearSafePtrs(void);

	std::list < SafePtrHandle * >	safePtrHandles;

	friend class		SafePtrHandle;
};


class SafePtrHandle {
protected:
//						SafePtr(void) : ptr(NULL) { /*Increment();*/ }
						SafePtrHandle(void) : spb_ptr(NULL) {};
						SafePtrHandle(SafePtrHandle &other) : spb_ptr(other.spb_ptr) {	spb_ptr->safePtrHandles.push_back(this);	}
						SafePtrHandle(SafePtrList *base) : spb_ptr(base) {	if (base) spb_ptr->safePtrHandles.push_back(this);	}
	virtual				~SafePtrHandle(void) {	Clear();	};

public:
	SafePtrHandle &		operator=(const SafePtrHandle &other);
	// SafePtrHandle &		operator=(SafePtrList *thing);

	inline bool			operator==(const SafePtrHandle &other) const { return spb_ptr == other.spb_ptr; }
	inline bool			operator==(const SafePtrList *object) const { return spb_ptr == object; }
	inline bool			operator==(const SafePtrList &object) const { return spb_ptr == &object; }
	inline bool			operator!=(const SafePtrHandle &other) const { return spb_ptr != other.spb_ptr; }
	inline bool			operator!=(const SafePtrList *object) const { return spb_ptr != object; }
	inline bool			operator!=(const SafePtrList &object) const { return spb_ptr != &object; }
	
	inline bool			valid(void) const			{ return spb_ptr != NULL;				}
	inline bool			operator!(void) const		{ return spb_ptr == NULL;				}
	inline 				operator bool(void) const	{ return spb_ptr != NULL;				}

	inline SafePtrList *	operator()(void) const 		{ return spb_ptr;	}
	inline SafePtrList *	operator()(void)			{ return spb_ptr;	}

	void				Set(SafePtrList *base);
	void				Clear(void);
	inline SafePtrList	*PointsTo(void)	const 		{ return spb_ptr;	}

protected:
	SafePtrList			*spb_ptr;

	friend class		SafePtrList;
};


inline SafePtrHandle &	SafePtrHandle::operator=(const SafePtrHandle &other) {
	if (this != &other) {
		if (spb_ptr)	
			spb_ptr->safePtrHandles.remove(this);
		spb_ptr = other.spb_ptr;
		if (spb_ptr)	
			spb_ptr->safePtrHandles.push_back(this);
	}
	return *this;
}


inline void SafePtrHandle::Set(SafePtrList *base) {
	spb_ptr = base;
	if (base)	
		base->safePtrHandles.push_back(this);
}

inline void SafePtrHandle::Clear(void) {
	if (spb_ptr) {
		spb_ptr->safePtrHandles.remove(this);
	}
	spb_ptr = NULL;
}


template<class _T>
class SafePtr : public SafePtrHandle {
public:
						SafePtr(void) {}
						SafePtr(SafePtrList *base) : SafePtrHandle(base) {}
						SafePtr(const SafePtr<_T> &other) : SafePtrHandle(*other) {}
						~SafePtr(void) {}
						
	inline _T *			operator->(void)			{ return static_cast<_T *>(PointsTo());		}
	inline const _T *	operator->(void) const		{ return static_cast<_T *>(PointsTo());		}
	inline _T &			operator*(void)				{ return *static_cast<_T *>(PointsTo());	}
	inline const _T &	operator*(void)	const		{ return *static_cast<_T *>(PointsTo());	}

	SafePtr<_T> &		operator=(_T *thing);

	inline _T *			operator()(void)			{ return static_cast<_T *>(PointsTo());		}
	inline const _T *	operator()(void) const		{ return static_cast<_T *>(PointsTo());		}
};


template<class _T>
SafePtr<_T> &	SafePtr<_T>::operator=(_T *thing) {
	if (spb_ptr != thing) {
		Clear();
		Set(thing);
	}
	return *this;
}


#endif
