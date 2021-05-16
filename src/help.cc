/***************************************************************************\
[*]    __     __  ___                ___  | LexiMUD is a C++ MUD codebase [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ |        for Sci-Fi MUDs        [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / | Copyright(C) 1997-1999        [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  | All rights reserved           [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   | Chris Jacobson (FearItself)   [*]
[*]      A Creation of the AvP Team!      | fear@technologist.com         [*]
[-]---------------------------------------+-------------------------------[-]
[*] File : help.c++                                                       [*]
[*] Usage: Help system code                                               [*]
\***************************************************************************/



#include "help.h"
#include "character.defs.h"
#include "buffer.h"
#include "utils.h"
#include "handler.h"
#include "strings.h"

#define READ_SIZE 256

void get_one_line(FILE *fl, char *buf);

Help *Help::Table = NULL;
SInt32 Help::Top = 0;
Help *Help::WizTable = NULL;
SInt32 Help::WizTop = 0;


Help::Help(void) : keywords(NULL), entry(NULL), level(0) {
}


Help::~Help(void) {
	if (keywords)	free(keywords);
	if (entry)		free(entry);
}


void Help::Free() {
	if (keywords)	free(keywords);
	if (entry)		free(entry);
	keywords = entry = NULL;
}


void Help::Load(FILE *fl) {
	char	*key = get_buffer(READ_SIZE+1),
			*entry = get_buffer(32384);
	char	*line = get_buffer(READ_SIZE+1);

	/* get the keyword line */
	get_one_line(fl, key);
	while (*key != '$') {
	
		Help &el = Table[Top];
		
		get_one_line(fl, line);
		*entry = '\0';
		while (*line != '#') {
			strcat(entry, strcat(line, "\r\n"));
			get_one_line(fl, line);
		}
		el.level = 0;
		if ((*line == '#') && *(line + 1))
			el.level = atoi((line + 1));

		el.trust = RANGE(0, el.level, TRUST_IMMORTAL);

		/* now, add the entry to the index with each keyword on the keyword line */
		el.entry = str_dup(entry);
		el.keywords = str_dup(key);
		
		++Top;
		
		/* get next keyword line (or $) */
		get_one_line(fl, key);
	}
	release_buffer(key);
	release_buffer(entry);
	release_buffer(line);
}


// More of the cursed, cursed redundancy because I'm in a hurry. -DH
void Help::WizLoad(FILE *fl) {
	char	*key = get_buffer(READ_SIZE+1),
			*entry = get_buffer(32384);
	char	*line = get_buffer(READ_SIZE+1);

	/* get the keyword line */
	get_one_line(fl, key);
	while (*key != '$') {
	
		Help &el = WizTable[WizTop];
		
		get_one_line(fl, line);
		*entry = '\0';
		while (*line != '#') {
			strcat(entry, strcat(line, "\r\n"));
			get_one_line(fl, line);
		}
		el.level = 0;
		if ((*line == '#') && *(line + 1))
			el.level = atoi((line + 1));

		el.trust = RANGE(1, el.level, TRUST_IMMORTAL);

		/* now, add the entry to the index with each keyword on the keyword line */
		el.entry = str_dup(entry);
		el.keywords = str_dup(key);
		
		++WizTop;
		
		/* get next keyword line (or $) */
		get_one_line(fl, key);
	}
	release_buffer(key);
	release_buffer(entry);
	release_buffer(line);
}


Help * Help::Find(const char *keyword, int wizhelp = 0) {
	SInt32	i;
	
	switch (wizhelp) {		// This isn't as neat as I want, but I don't feel like fudging with pointers
	case TRUE:
		for (i = 0; i < WizTop; ++i)
			if (isword(keyword, WizTable[i].keywords))
				return (WizTable + i);
		break;
	default:
		for (i = 0; i < Top; ++i)
			if (isword(keyword, Table[i].keywords))
				return (Table + i);
		break;
	}
	
	return NULL;
}
