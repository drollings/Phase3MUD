
#ifndef __STRING_H__
#define __STRING_H__


#include "types.h"

#include <cstring>


int strcmpi(const char *arg1, const char *arg2);
int strncmpi(const char *arg1, const char *arg2, int n);
const char *strstri(const char *cs, const char *ct);
int cstrlen(const char *arg1);

class Substring;
class BasicString;
class MUDString;


class Substring {
	friend class BasicString;
	friend class MUDString;
public:
					Substring(const char *source);
					Substring(const Substring *source);
//					Substring(const String &str);
					Substring(int source);
					~Substring(void);
	
	void			Register(void);
	void			Unregister(void);

	inline int		Count(void)					{	return count;		}
	inline int		Length(void)				{	return length;		}
	inline int		Size(void)					{	return size;		}
	inline const char *Chars(void)				{	return string;		}
	Substring *		Copy(void);

protected:
					Substring(void);
					
	char *			string;
	int				length, size;
	int				count;
};


class BasicString {
public:
					BasicString(void);
					BasicString(const char *source);
					BasicString(const BasicString & source);
					BasicString(int size);
					~BasicString(void);
					
	inline			operator const char *() const	{ return str && str->string ? str->string : ""; }
	inline const char *	operator () (void) const	{ return str && str->string ? str->string : ""; }
	
	BasicString &	operator = (const BasicString &s);
	BasicString &	operator = (const char *s);
	
//	inline int		strcmp(const char *s) const			{ return ::strcmp(str->string, s);		}
//	inline int		strncmp(const char *s, int n) const	{ return ::strncmp(str->string, s, n);	}
//	inline int		strcmpi(const char *s) const		{ return ::strcmpi(str->string, s);		}
//	inline int		strncmpi(const char *s, int n) const{ return ::strncmpi(str->string, s, n);	}
	inline bool		operator == (const BasicString &s) const	{ return strcmpi(str->string, s.str->string) == 0;	}
	inline bool		operator == (const char *s) const	{ return strcmpi(str->string, s) == 0;				}
	inline bool		operator != (const BasicString &s) const	{ return strcmpi(str->string, s.str->string) != 0;	}
	inline bool		operator != (const char *s) const	{ return strcmpi(str->string, s) != 0;				}
	
//	inline const char *strstri(const char *s) const		{ return ::strstri(str->string, s);		}
//	inline const char *strstr(const char *s) const		{ return ::strstr(str->string, s);		}
//	inline const char *strstri(const String &s) const	{ return ::strstri(str->string, s.str->string);}
//	inline const char *strstr(const String &s) const	{ return ::strstr(str->string, s.str->string);}
	inline bool		Contains(const char *s) const		{ return strstri(str->string, s);		}
	inline bool		Contains(const BasicString &s) const{ return strstri(str->string, s);		}
	bool			HasSubstring(const char *s) const;
	inline bool		HasSubstring(const BasicString &s) const	{ return HasSubstring(s.str->string);	}
	
//	char			GetChar(int n) const;
//	char &			GetChar(int n);
	char			operator [] (int i) const;	//		{ return GetChar(n);					}
	char &			operator [] (int i);	//			{ return GetChar(n);					}
	
/*	
	String			ExtractLine(void);
	String &		ExtractLine(String &s);
*/
	
//	inline int		strlen(void) const					{ return ::strlen(str->string);			}
//	inline int		cstrlen(void) const					{ return ::cstrlen(str->string);		}
//	inline int		Length(void) const					{ return str->length;					}
//	inline int		Length(void) const					{ return strlen(str->string);			}
	void			Clear(void);
//	int				Size(void) const;		//	Not necessary?
//	char *			Dup(void) const;		//	Not necessary?
	
	BasicString &	SkipSpaces(void);
	BasicString &	DeleteDoubleDollar(void);
	BasicString &	Strip(char ch);
	
//	String			Copy(int length) const;
//	String &		Copy(int length, String &s) const;
	inline BasicString	Copy(int start, int length) const	{ BasicString t;	return Copy(start, length, t);	}
	BasicString &	Copy(int start, int length, BasicString &s) const;
	
	BasicString &	Delete(int start, int count);
	BasicString &	Append(const char *s);
	BasicString &	Insert(int start, const char *s);
	BasicString &	Replace(const char *find, const char *replace);
	BasicString &	ReplaceAll(const char *find, const char *replace);
	
//	String &		sprintf(const char * format, ...) __attribute__ ((format (printf, 2, 3)));
	
	inline BasicString &operator << (const BasicString &s)	{ return Append(s);				}
	inline BasicString &operator << (const char * &s)		{ return Append(s);				}
	BasicString &	operator << (const int i);
	BasicString &	operator << (const unsigned int i);
	BasicString &	operator << (const float f);
	BasicString &	operator << (const double d);
	BasicString &	operator << (const char c);
	
	int				Pos(const char c, int start = 0) const;
	

protected:
	Substring *		str;

	virtual char *	Allocate(int size);
};


inline BasicString operator + (const BasicString &lhs, const BasicString &rhs)	{ return BasicString(lhs) << rhs;	}
inline BasicString operator + (const BasicString &lhs, const char * rhs)		{ return BasicString(lhs) << rhs;	}
inline BasicString operator + (const char *lhs, const BasicString &rhs)			{ return BasicString(lhs) << rhs;	}
inline BasicString operator + (const BasicString &lhs, const int rhs)			{ return BasicString(lhs) << rhs;	}
inline BasicString operator + (const BasicString &lhs, const unsigned int rhs)	{ return BasicString(lhs) << rhs;	}
inline BasicString operator + (const BasicString &lhs, const float rhs)			{ return BasicString(lhs) << rhs;	}
//inline BasicString operator + (const BasicString &lhs, const double rhs)		{ return BasicString(lhs) << rhs;	}
inline BasicString operator + (const BasicString &lhs, const char rhs)			{ return BasicString(lhs) << rhs;	}
inline BasicString operator + (const int lhs, const BasicString &rhs)			{ return BasicString() << lhs << rhs;}
inline BasicString operator + (const unsigned int lhs, const BasicString &rhs)	{ return BasicString() << lhs << rhs;}
inline BasicString operator + (const float lhs, const BasicString &rhs)			{ return BasicString() << lhs << rhs;}
//inline BasicString operator + (const double lhs, const BasicString &rhs)		{ return BasicString() << lhs << rhs;}
inline BasicString operator + (const char lhs, const BasicString &rhs)			{ return BasicString() << lhs << rhs;}


class MUDString : public BasicString {
public:
					MUDString(void);
					MUDString(const char *source);
					MUDString(const BasicString & source);
					MUDString(int size);
					~MUDString(void);

	bool			IsNumber(void) const;
	bool			IsAbbrev(const char *arg) const;
	
	MUDString		OnePhrase(MUDString &target) const;
	MUDString		OneWord(MUDString &target) const;
	MUDString		AnyOneArg(MUDString &a) const;
	MUDString		AnyOneName(MUDString &a) const;
	MUDString		OneArgument(MUDString &a) const;
	inline MUDString	TwoArguments(MUDString &a, MUDString &b) const
						{ return OneArgument(a).OneArgument(b);		}
	inline MUDString	ThreeArguments(MUDString &a, MUDString &b, MUDString &c) const
						{ return TwoArguments(a, b).OneArgument(b);	}
	void			HalfChop(MUDString &a, MUDString &b) const
						{ b = AnyOneArg(a);							}

protected:
	virtual char *	Allocate(int size);
};


inline MUDString operator + (const MUDString &lhs, const MUDString &rhs)	{ return MUDString(lhs) << rhs;	}
inline MUDString operator + (const BasicString &lhs, const MUDString &rhs)	{ return MUDString(lhs) << rhs;	}
inline MUDString operator + (const MUDString &lhs, const BasicString &rhs)	{ return MUDString(lhs) << rhs;	}
inline MUDString operator + (const MUDString &lhs, const char * rhs)		{ return MUDString(lhs) << rhs;	}
inline MUDString operator + (const char *lhs, const MUDString &rhs)			{ return MUDString(lhs) << rhs;	}
inline MUDString operator + (const MUDString &lhs, const int rhs)			{ return MUDString(lhs) << rhs;	}
inline MUDString operator + (const MUDString &lhs, const unsigned int rhs)	{ return MUDString(lhs) << rhs;	}
inline MUDString operator + (const MUDString &lhs, const float rhs)			{ return MUDString(lhs) << rhs;	}
//inline MUDString operator + (const MUDString &lhs, const double rhs)		{ return MUDString(lhs) << rhs;	}
inline MUDString operator + (const MUDString &lhs, const char rhs)			{ return MUDString(lhs) << rhs;	}
inline MUDString operator + (const int lhs, const MUDString &rhs)			{ return MUDString() << lhs << rhs;}
inline MUDString operator + (const unsigned int lhs, const MUDString &rhs)	{ return MUDString() << lhs << rhs;}
inline MUDString operator + (const float lhs, const MUDString &rhs)			{ return MUDString() << lhs << rhs;}
//inline MUDString operator + (const double lhs, const MUDString &rhs)		{ return MUDString() << lhs << rhs;}
inline MUDString operator + (const char lhs, const MUDString &rhs)			{ return MUDString() << lhs << rhs;}


#endif
