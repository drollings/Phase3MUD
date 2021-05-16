

#ifndef __WORLDPARSER_H__
#define __WORLDPARSER_H__


#include "types.h"
#include "stl.llist.h"

#include "index.h"

class Character;
class ExtraDesc;
class Room;


namespace Parser {
	void		Error(char *format, ...);
	char *		MatchingQuote(char *p);
	char *		MatchingBracket(char *p);
	char *		GetKeyword(char *input, char *keyword);
	char *		ReadString(char *input, char *string);
	char *		ReadStringBlock(char *input, char *string);
	char *		ReadBlock(char *input, char *block);
	char *		Parse(char *input, char *keyword);
	char *		FlagParser(char *input, Flags *result, const char **bits, const char *what);
	void		ExDesc(char *input, LList<ExtraDesc *> &exdescList);
	void		Exit(char *input, Room *room);
//	void		Shop(char *input, shop_data *shop);

	extern VNum		number;
	extern char *	file;
	extern bool		error;
}


#define	PARSER_BUFFER_LENGTH	(MAX_STRING_LENGTH * 4)


#endif

