/***************************************************************************\
[*]    __     __  ___                ___  | LexiMUD is a C++ MUD codebase [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ |        for Sci-Fi MUDs        [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / | Copyright(C) 1997-1999        [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  | All rights reserved           [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   | Chris Jacobson (FearItself)   [*]
[*]      A Creation of the AvP Team!      | fear@technologist.com         [*]
[-]---------------------------------------+-------------------------------[-]
[*] File : types.h                                                        [*]
[*] Usage: Basic data types                                               [*]
\***************************************************************************/

#ifndef __TYPES_H__
#define __TYPES_H__

#include "conf.h"
#include "sysdep.h"


typedef char		Int8;
typedef short		Int16;
typedef int			Int32;

typedef unsigned char	UInt8;
typedef signed char		SInt8;

//#if defined(macintosh)
//    Basic Types
typedef	unsigned short	UInt16;
typedef signed short	SInt16;
typedef unsigned int	UInt32;
typedef signed int		SInt32;

//#else
#if 0
#if SIZEOF_INT == 4
typedef unsigned int  UInt32;
typedef signed int  	  SInt32;
#elif SIZEOF_LONG == 4
typedef unsigned long UInt32;
typedef signed long 	  SInt32;
#else
#error Cannot continue: I know of no 2-byte value
#endif

#if SIZEOF_SHORT == 2
typedef unsigned short    UInt16;
typedef signed short  SInt16;
#elif SIZEOF_INT == 2
typedef unsigned int  UInt16;
typedef signed int  	  SInt16;
#else
#error Cannot continue: I know of no 2-byte value
#endif

#endif

typedef void *			Ptr;
typedef const void *	CPtr;

//	Circle Types
typedef SInt16			VNum;
//typedef SInt16			RNum;

typedef UInt8			Type;
typedef UInt32			IDNum;
typedef UInt32			Flags;

typedef unsigned int	StrLenInt;


#ifdef GOT_RID_OF_IT
template <class T> 
class Shared
{
public:
						Shared(void);
						Shared(const T & datum);
	Shared <T> *		Share(void);
	void				Free(void);
	const T *			Data(void);

	static Shared<T> *	Create(const T & datum);
//protected:
	T *					data;
	UInt32				count;
};

template <class T>
inline const T * Shared<T>::Data(void) {	return this ? &(this->data) : NULL;	}
#endif


// Shared Strings
class SString 
{
public:
						SString(void);
						SString(const char *str);
	SString *			Share(void);
	void				Free(void);
	const char *		Data(void);
	const char **		StrPtr(void);

	static SString *	Create(const char *str);
	static SString *	fread(FILE *fl, char *error, char *filename);
//protected:
	char *				str;
	UInt32				count;
};

inline const char *SString::Data(void) {	return this ? this->str : NULL;		}
#define SSData(shared)		shared->Data()
#define SSShare(shared)		shared->Share()
#define SSFree(shared)	\
	{					\
		shared->Free();	\
		shared = NULL;	\
	}


class CallName {
public:
	CallName(void) : id(0), string(NULL) { }
	CallName(IDNum newid, char *newstring) : id(newid), string(newstring) { }
	~CallName(void);

private:
	IDNum id;
	char * string;
	
public:
	IDNum ID(void) const;
	char * Value(void) const;

	char * operator = (char * newstring);
	void Set(IDNum newid, const char * newstring);
};


inline IDNum CallName::ID(void) const
	{ return id; }
inline char * CallName::Value(void) const
	{ return string; }
inline char * CallName::operator = (char * newstring) 
	{
		if (string) delete string;
		return (string = strdup(newstring));
	}
inline void CallName::Set(IDNum newid, const char * newstring)
	{
		id = newid;
		if (string) delete string;
		string = strdup(newstring);
	}


//	This structure is purely intended to be an easy way to transfer
//	and return information about time (real or mudwise).
struct TimeInfoData {
//	SInt8	hours, day, month;
	UInt16	hours	: 6;	//	0-31
	UInt16	day		: 10;	//	0-511
	UInt16	month	: 5;	//	0-15
	UInt16	year	: 13;	//	0-4095
};	//	34 bits


struct Dice {
	UInt16	number;
	UInt16	size;
};


namespace Datatypes {
	enum {
		Undefined,
		Integer,
		Value,
		SpellValue,
		DiffValue,
		Bitvector,
		Bool,
		CString,
		CStringLine,
		SString,
		ObjValue,
		SkillRef,
		Base,
		Editable,
		Scriptable,
		GameData,
		MUDObject,
		Object,
		Room,
		Character,
		AffectEditable,
		Trigger,
		Script,
		VNum = 1 << 8,		// Modifiers on how the data is manipulated
		IDNum = 1 << 9,
		Pointer = 1 << 10,
		List = 1 << 11,
		Vector = 1 << 12,
		Map = 1 << 13
	};
}

enum {
	EDT_SINT8 = 1,
	EDT_UINT8,
	EDT_SINT16,
	EDT_UINT16,
	EDT_SINT32,
	EDT_UINT32,
	EDT_VNUM,
	EDT_IDNUM
};

#endif	// __TYPES_H__

