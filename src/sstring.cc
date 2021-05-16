/***************************************************************************\
[*]    __     __  ___                ___  | LexiMUD is a C++ MUD codebase [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ |        for Sci-Fi MUDs        [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / | Copyright(C) 1997-1999        [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  | All rights reserved           [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   | Chris Jacobson (FearItself)   [*]
[*]      A Creation of the AvP Team!      | fear@technologist.com         [*]
[-]---------------------------------------+-------------------------------[-]
[*] File : sstring.c++                                                    [*]
[*] Usage: Shared strings                                                 [*]
\***************************************************************************/


#include "types.h"
#include "utils.h"
#include "files.h"
#include "stl.llist.h"

#define MAX_SSCOUNT   INT_MAX


#if 0
class SharedString;
class NSString;


class SharedString {
public:
					SharedString(void);
					SharedString(const char *x);
					SharedString(const SharedString &orig);
					SharedString(SInt32 size);
					~SharedString(void);
	
	const char *	operator()(void) const;
					operator const char *(void) const;
	const char *	Chars(void) const;
	
	UInt32			Inc(void);
	UInt32			Dec(void);
	
	UInt32			Count(void) const;
	UInt32			Length(void) const;
	UInt32			Size(void) const;
protected:
	UInt32			count;
	UInt32			size;
	char *			string;
};


inline SharedString::SharedString(void) : count(1), size(0), string(new char[1])	{ };
inline SharedString::SharedString(const char *x) : count(1), size(strlen(x) + 1),
		string(new char[size]) {
	strcpy(string, x);
}
inline SharedString::SharedString(const SharedString &orig) : count(1), size(orig.Length() + 1),
		string(new char[size]) {
	strcpy(string, orig.string);
}
inline SharedString::SharedString(SInt32 size) : count(1), size(size), string(new char[size]) { };
inline SharedString::~SharedString(void) {
	delete [] string;
}

inline const char *	SharedString::Chars(void) const			{ return string;			}
inline const char *	SharedString::operator()(void) const	{ return Chars();			}
inline 				SharedString::operator const char *(void) const	{ return Chars();	}
inline UInt32		SharedString::Inc(void)					{ return ++count;			}
inline UInt32		SharedString::Dec(void) {
	SInt32 theCount = --count;
	if (!theCount)
		delete this;
	return theCount;
}

inline UInt32	SharedString::Count(void) const				{ return count;				}
inline UInt32	SharedString::Length(void) const			{ return strlen(string);	}
inline UInt32	SharedString::Size(void) const				{ return size;				}


class NSString {
protected:
	static SharedString sharedEmpty;
	SharedString *	shared;

public:
					NSString(void);
					NSString(const char *s);
					NSString(const NSString & x);
					NSString(SInt32 asize);
					~NSString(void);
					
	const char *	operator()(void) const;
					operator const char*(void) const;
	
	UInt32			Count(void) const;
	UInt32			Length(void) const;
	UInt32			Size(void) const;
	void			Clear(void);
	void			Alloc(SInt32);
//	char *			dup(void) const;
};

inline NSString::NSString(void) : shared(&NSString::sharedEmpty)	{ shared->Inc;	}
inline NSString::NSString(const char *s) : shared(new SharedString(s))	{ }
inline NSString::NSString(const NSString &x) : shared(x.shared)		{ shared->Inc;	}
inline NSString::NSString(SInt32 size) : shared(new SharedString(size))	{ }

inline NSString::~NSString(void) {
	shared->Dec();
}

inline const char *	NSString::operator()(void) const{ return shared->Chars();			}
inline				NSString::operator const char *() const { return shared->Chars();	}
inline UInt32		NSString::Count(void) const		{ return shared->Count();			}
inline UInt32		NSString::Length(void) const	{ return shared->Length();			}
inline UInt32		NSString::Size(void) const		{ return shared->Size();			}


SharedString NSString::sharedEmpty;
#endif


SString::SString(void) : str(0), count(1) {
}

SString::SString(const char *string) : str(str_dup(string)), count(1) {
}


SString *SString::Share() {
	if (!this)	return NULL;
	count++;
	return this;
}


void SString::Free(void) {
	if (!this)	return;
	if (--count == 0) {
		if (str)	free(str);
		delete this;
	}
}


/* create a new shared string */
SString *SString::Create(const char *str) {
	return str ? new SString(str) : NULL;
}


SString *SString::fread(FILE * fl, char *error, char *filename) {
	char *str;
	SString *tmp;
	
	str = fread_string(fl, error, filename);
	tmp = SString::Create(str);
	
	if(str)	free(str);
	return tmp;
}


#ifdef GOT_RID_OF_IT
// CHANGEPOINT:  This really should go elsewhere, but...
template <class T> 
Shared <T> * Shared<T>::Share() {
	if (!this)	return NULL;
	count++;
	return this;
}


template <class T> 
void Shared<T>::Free(void) {
	if (!this)	return;
	if (--count == 0) {
		if (data)	delete data;
		delete this;
	}
}


/* create a new shared datum */
template <class T> 
Shared<T> * Shared<T>::Create(const T & datum) {
	return (new Shared(datum));
}


template <class T> 
Shared<T>::Shared(void) : data(NULL), count(1) {
}

template <class T> 
Shared<T>::Shared(const T & datum) : count(1) {
	data = new T;
	*data = datum;
}
#endif


