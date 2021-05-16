
#include <cctype>

#include "string.h"
#include "utils.h"


int strcmpi(const char *arg1, const char *arg2) {
	int chk, i;

	for (i = 0; *(arg1 + i) || *(arg2 + i); i++)
		if ((chk = tolower(*(arg1 + i)) - tolower(*(arg2 + i)))) {
			if (chk < 0)	return -1;
			else			return 1;
		}
	return 0;
}


int strncmpi(const char *arg1, const char *arg2, int n) {
	int chk, i;

	for (i = 0; (*(arg1 + i) || *(arg2 + i)) && (n > 0); i++, n--)
		if ((chk = tolower(*(arg1 + i)) - tolower(*(arg2 + i)))) {
			if (chk < 0)	return -1;
			else			return 1;
		}
	return 0;
}


const char *strstri(const char *cs, const char *ct) {
	const char *s, *t;

	if (!cs || !ct)	return NULL;

	while (*cs) {
		t = ct;

		while (*cs && (tolower(*cs) != tolower(*t)))	cs++;

		s = cs;

		while (*t && *cs && (tolower(*cs) == tolower(*t))) {
			t++;
			cs++;
		}
    
		if (!*t)	return s;
	}

	return NULL;
}


int cstrlen(const char *str) {
	int		length = 0;
	
	if (!str || *str == '\0')	return 0;
	
	while (*str) {
		if (*str != '^' && *str != '`') {
			str++;
			length++;
			continue;
		}
		
		str++;
		
		if (*str == '^' || *str == '`')	length++;
		
		if (*str)	str++;	//	safety catch to prevent ^\0 and `\0
	}
	
	return length;
}


namespace {
	Substring	emptySC("");
	bool		emptySCDeleted = false;
}


Substring::Substring(void) : string(NULL), length(0), size(0), count(1) {
//	*string = '\0';
}

Substring::Substring(const char *source) : string(NULL), length(0), size(0), count(1) {
	if (source)	{
		length = strlen(source);
		size = length + 1;
		string = new char[size];
		strcpy(string, source);
	}
}

Substring::Substring(const Substring *source) : string(NULL), length(0),
		size(0), count(1) {
	if (source && source->string) {
		size = source->size;
		string = new char[size];
		strcpy(string, source->string);
		length = strlen(string);
	}
}

/*
Substring::Substring(const BasicString &source) : string(NULL), length(0), size(0), count(1) {
	if (source.str && source.str->string) {
		size = source.str->size;
		string = new char[size];
		strcpy(string, source->string);
		length = strlen(string);
	}
}
*/

Substring::Substring(int source) : string(NULL), length(0), size(0), count(1) {
	if (source) {
		size = source;
		string = new char[size];
		*string = '\0';
	}
}


Substring::~Substring(void) {
	if (this == &emptySC)	emptySCDeleted = true;	//	Debug Purposes
	if (string)				delete [] string;
}


void Substring::Register(void) {
	count++;
}


void Substring::Unregister(void) {
	if (--count <= 0 && this != &emptySC)
		delete this;
}


Substring *Substring::Copy(void) {
	if (count > 1) {
		count--;
		return new Substring(this);
	}
	return this;
}


//	BasicString

BasicString::BasicString(void) : str(&emptySC) {
	str->Register();
}


BasicString::BasicString(const char *source) : str(&emptySC) {
	if (source && *source)	str = new Substring(source);
	else					str->Register();
}


BasicString::BasicString(const BasicString &source) : str(source.str) {
	str->Register();
}

BasicString::BasicString(int size) : str(&emptySC) {
	if (size)				str = new Substring(size);
	else					str->Register();
}


BasicString::~BasicString(void) {
	str->Unregister();
}


BasicString &BasicString::operator=(const BasicString &s) {
	str->Unregister();
	str = s.str;
	str->Register();
	
	return *this;
}



BasicString &BasicString::operator=(const char *s) {
	str->Unregister();

	if (s && *s)	str = new Substring(s);
	else {
		str	= &emptySC;
		str->Register();
	}
	
	return *this;
}


void BasicString::Clear(void) {
	str->Unregister();
	str = new Substring("");
//	OR (less safe?):
//	str = str->Copy();
//	*str->string = '\0';
}


bool BasicString::HasSubstring(const char *s) const {
	const char *sub;
	
	if ((sub = strstri(str->string, s)) != NULL) {
		int length = str->length;
		int sublength = ::strlen(s);
		
		if ((sub == str->string || isspace(sub[-1]) || ispunct(sub[-1])) &&
				((sub + sublength == str->string + length) || isspace(sub[sublength]) ||
				ispunct(sub[sublength])))
			return true;
	}
	return false;
}


char BasicString::operator [] (int i) const {
	if (str->length) {
		if (i < 0 || i > str->length)	return 0;
		return str->string[i];
	} else								return 0;
}

/*
BasicString BasicString::ExtractLine(void) {
	BasicString s;
	return ExtractLine(s);
}


BasicString &BasicString::ExtractLine(BasicString &s) {
	int pos = Pos('\n');
	if (pos >= 0) {
		Copy(0, pos++, s);
		Delete(0, pos);
	}
	return s;
}
*/

char & BasicString::operator [] (int i) {
	str = str->Copy();
	i = RANGE(0, i, str->size - 1);
	return str->string[i];
}


BasicString & BasicString::SkipSpaces(void) {	//	A sort of Skip-spaces
	char /**dest,*/ *source;
	
	if (*str->string) {
		str = str->Copy();
		
		source = /*dest = */str->string;
		
		while (isspace(*source))	source++;
		strcpy(str->string, source);
//		while (*dest)				*dest++ = *source++;	// Replace with strcpy(dest, src) ?
//		*dest = '\0';
	}
	
	return *this;
}


BasicString & BasicString::Strip(char ch) {
	char	*dest, *source;
	
	if (*str->string) {
		str = str->Copy();
		
		source = dest = str->string;
		
		while (*source) {
			if (*source != ch)
				*dest++ = *source;
			*source++;
		}
		*dest = '\0';
		
		str->length = ::strlen(str->string);
	}
	
	return *this;
}


BasicString & BasicString::DeleteDoubleDollar(void) {
	char *read, *write;
	
	if (!(write = strchr(str->string, '$')))
		return *this;
	
	str = str->Copy();
	
	read = write;
	
	while (*read)
		if ((*write++ = *read++) == '$')
			if (*read == '$')
				read++;
	
	*write = '\0';
	
	return *this;
}


BasicString & BasicString::Copy(int start, int length, BasicString &s) const {
	if (start < 0 || start >= str->length || length <= 0) {
		s.Clear();
		return s;
	}

	if (start + length > str->length)	length = str->length - start;
	
	if (length <= 0) {
		s.Clear();
		return s;
	}
	
	s.str->Unregister();
	s.str = new Substring(length + 1);
	strncpy(s.str->string, str->string + start, length);
	s.str->string[length] = '\0';
	
	return s;
}


BasicString & BasicString::Delete(int start, int count) {
	if (!str->length || !count || start > str->length)		return *this;
	if (start + count > str->length)	count = str->length - start;
	if (count >= count)					Clear();
	else {
		str = str->Copy();
		strcpy(str->string + start, str->string + start + count);
		str->length = str->length - count;
	}
	return *this;
}


BasicString & BasicString::Append(const char *s) {
	if (!s || !*s)		return *this;
	
	int		newLength = str->length + ::strlen(s);
	
	if (newLength >= str->size - 1) {
		char *newStr = new char[newLength + 1];
		strcpy(newStr, str->string);
		str->Unregister();
		str = new Substring;
		str->string = newStr;
		str->size = newLength + 1;
	} else
		str = str->Copy();

	strcat(str->string, s);
	str->length = newLength;
	
	return *this;
}


BasicString & BasicString::Insert(int start, const char *s) {
	if (start < 0 || start > str->length)	return *this;
	if (!s || !*s)							return *this;
	
	int		sLength = ::strlen(s);
	int		newLength = str->length + sLength;
		
	if (newLength >= str->size) {
		char *	newStr = new char[newLength + 1];
			
		memcpy(newStr, str->string, start);
		memcpy(newStr + start, s, newLength - str->length);
		memcpy(newStr + start + sLength, str->string + start, str->length - start);
		newStr[newLength] = '\0';
			
		str->Unregister();
		str = new Substring;
		str->size = newLength + 1;
		str->length = newLength;
		str->string = newStr;
	} else {
		str = str->Copy();
		memcpy(str->string + start + sLength, str->string + start, str->length - start);
		memcpy(str->string + start, s, newLength - str->length);
		str->string[newLength] = '\0';
		str->length = newLength;
	}
	
	return *this;
}


MUDString MUDString::OneArgument(MUDString &a) const {
	MUDString	rem;
	
	int start, end;
	
	for (start = 0; start < str->length; start++)		//	find start of first word
		if (!isspace(str->string[start]))
			break;
	
	for (end = start; end < str->length; end++)	//	find end of first word
		if (isspace(str->string[end]))
			break;
	
	if (end > start)			Copy(start, end - start, a);

	for (; end < str->length; end++)
		if (!isspace(str->string[end]))
			break;
	
	if (end < str->length)		Copy(end, str->length - end, rem);
	
	return rem;
}


bool MUDString::IsNumber(void) const {
	char *	source = this->str->string;
	
	if (!*source)						return false;
	if (*source == '-')					source++;
	while (*source && isdigit(*source))	source++;
	if (!source || isspace(*source))	return true;
	else								return false;
}


bool MUDString::IsAbbrev(const char *arg) const {
	char *	string;
	if (!arg || !*arg)	return false;
	for (string = str->string; *string && *arg; string++, arg++)
		if (tolower(*string) != tolower(*arg))
			return false;
	
	if (!*string)	return true;
	else			return false;
}
