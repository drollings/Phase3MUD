/***************************************************************************\
[*]    __     __  ___                ___  | LexiMUD is a C++ MUD codebase [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ |        for Sci-Fi MUDs        [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / | Copyright(C) 1997-1999        [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  | All rights reserved           [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   | Chris Jacobson (FearItself)   [*]
[*]      A Creation of the AvP Team!      | fear@technologist.com         [*]
[-]---------------------------------------+-------------------------------[-]
[*] File : worldparser.c++                                                [*]
[*] Usage: Database parsing core                                          [*]
\***************************************************************************/

#include "worldparser.h"
#include "rooms.h"
#include "characters.h"
#include "objects.h"
#include "extradesc.h"
#include "strings.h"
#include "utils.h"
#include "buffer.h"
#include "db.h"
#include "olc.h"
#include "constants.h"
#include "skills.h"
#include "affects.h"
#include "opinion.h"
#include "shop.h"
#include "combat.h"
#include "scripts.h"
#include "maputils.h"
#include "property.h"

extern struct zone_data *zone_table;	/* zone table			 */
extern SInt32 top_of_zone_table;	/* top element of zone tab	 */
class SkillRef;
extern const char *position_block[];

namespace Parser {
	char *	file = NULL;
	VNum	number = -1;
	bool	error = false;
}


char *Parser::MatchingQuote(char *p) {
	for (p++; *p && (*p != '"'); p++) {
		if (*p == '\\' && *(p+1))	p++;
	}
	if (!*p)	p--;
	return p;
}

/*
 * p points to the first paren.  returns a pointer to the
 * matching closing paren, or the last non-null char in p.
 */
char *Parser::MatchingBracket(char *p) {
	int i;
	for (p++, i = 1; *p && i; p++) {
		if (*p == '{')			i++;
		else if (*p == '}')		i--;
		else if (*p == '"')		p = MatchingQuote(p);
	}
	return --p;
}


//char *Parser::GetKeyword(char *input, char *keyword) {
/*	SInt32 counter = 0;
	
	while (isalnum(*input)) {
		*(keyword++) = LOWER(*input);
		input++;
		if (++counter == 255)	break;
	}
	
//	for (counter = 0;isalnum(*input) && (counter < 255);
//			*(keyword++) = LOWER(*(input++)), counter++);
	*keyword = '\0';
	skip_spaces(input);
	return input;
*/
//	return any_one_arg(input, keyword);
//}


char *Parser::GetKeyword(char *input, char *keyword) {
	while (isspace(*input))	input++;
	
	while (*input && !isspace(*input) && (*input != ';') && (*input != '='))
//	while (*input && (isalnum(*input) || (*input == '_') || (*input == '-')))
	{
		if (*input == '/') {
			switch (*(input + 1)) {
				case '/':
					while (*input && *input != '\n')	input++;
					break;
				case '*':
					input += 2;
					while (*input) {
						if (*input == '*' && *(input + 1) == '/') {
							input += 2;
							break;
						}
						input++;
					}
					break;
				default:
					*(keyword++) = *(input++);
			}
		} else
			*(keyword++) = *(input++);
	}
	*keyword = '\0';
	
//	for(arg = keyword; *input && !isspace(*input) && (*input != ';') && (*input != '=');
//			arg++, input++) {
//		*arg = LOWER(*input);
//	}
//	*arg = '\0';
	
	while (isspace(*input))	input++;

	return input;
}

char *Parser::ReadString(char *input, char *string) {
//	while (1) {
		while (*input && (*input != '"')) {				//	Find start
			//	Maybe also detect stray tokens, but be careful
			//	not to count every character!
			++input;
		}
		++input;
		while (*input && (*input != '"')) {				//	Then read till end
			if (*input == '\\') {						//	Allow cntrl characters
				input++;	//	Move to next char
				switch (*input) {
					case 'n':		//	Newline
						*(string++) = '\r';
						*(string++) = '\n';
						break;
					case '"':		//	"
						*(string++) = '"';
						break;
					case 'r':		//	Nothing - for the \r\n'rs
					case '\n':
					case '\r':		//	JIC Bug catchers
						break;
					default:		//	Maybe should somehow escape-translate?
						*(string++) = *input;
				}
			} else if (*input != '\n')	//	Allow spacing
				*(string++) = *input;
			input++;
		}
		*string = '\0';
		if (*input)	input++;
//		while (*input && *input != '"' && *input != ';')	input++;
//		if (*input == ';')	break;
//	}
	return input;
}


char *Parser::ReadBlock(char *input, char *block) {
	char *end_block, *next_input;
	
	while (*input && *input != '{')	input++;
	if (*input) {
		next_input = end_block = MatchingBracket(input);
		input++;
		skip_spaces(input);
		while (*end_block && isspace(*end_block)) end_block--;
		if (end_block > input) {
			strncpy(block, input, end_block - input);
			block[end_block - input] = '\0';
		} else
			*block = '\0';
		input = next_input + 1;
		skip_spaces(input);
	} else
		Error("End of String when reading block");
	return input;
}


// This is a specialized variant of ReadString that requires "; together to terminate.
char *Parser::ReadStringBlock(char *input, char *string) {
//	while (1) {
		while (*input && (*input != '"')) {				//	Find start
			//	Maybe also detect stray tokens, but be careful
			//	not to count every character!
			++input;
		}
		++input;
		while (*input) {				//	Then read till end
        	if (*input == '"') {
	        	if (*(input + 1) == ';') {	//	This makes sure extraneous " characters don't disrupt the string.
                	break;
                } else {
					*(string++) = *input;
                }
			} else if (*input == '\\') {						//	Allow cntrl characters
				input++;	//	Move to next char
				switch (*input) {
					case 'n':		//	Newline
						*(string++) = '\r';
						*(string++) = '\n';
						break;
					case '"':		//	"
						*(string++) = '"';
						break;
					case 'r':		//	Nothing - for the \r\n'rs
					case '\n':
					case '\r':		//	JIC Bug catchers
						break;
					default:		//	Maybe should somehow escape-translate?
						*(string++) = *input;
				}
			} else if (*input != '\n')	//	Allow spacing
				*(string++) = *input;
			input++;
		}
		*string = '\0';
		if (*input)	input++;
//		while (*input && *input != '"' && *input != ';')	input++;
//		if (*input == ';')	break;
//	}
	return input;
}


char *Parser::FlagParser(char *input, Flags *result, const char **bits, const char *what) {
	SInt32	i;
	char *keyword = get_buffer(PARSER_BUFFER_LENGTH);
	
	skip_spaces(input);
	if (*input == '{') {	//	List
		input = ReadBlock(input, keyword);
		
		char *key2 = get_buffer(PARSER_BUFFER_LENGTH);
		char *keyPtr = keyword;
		
		while (*keyPtr) {
			keyPtr = any_one_arg(keyPtr, key2);
			
			if (!*key2)	break;
			
			if ((i = search_block(key2, bits, false)) != -1)
				SET_BIT(*result, (1 << i));
			else
				Error("%s: Unknown flag \"%s\"", what, key2);
		}
		release_buffer(key2);
	} else if (is_number(input)) {	//	Compressed-save: #
		input = Parser::GetKeyword(input, keyword);
		*result = atoi(keyword);
	} else {
		Error("Bad %s", what);
		//	ERROR HANDLER - Non-number/List
	}
	release_buffer(keyword);
	return input;
}


void Parser::Error(char *format, ...) {
	va_list args;
	char *buf = get_buffer(PARSER_BUFFER_LENGTH);
	
	va_start(args, format);
	vsprintf(buf, format, args);
	va_end(args);
	
	log("Parse Error: %s:#%d - %s", file ? file : "<ERROR>", number,
			*buf ? buf : "<ERROR>");
	error = true;
	
	release_buffer(buf);
}


char *Parser::Parse(char *input, char *keyword) {
//	skip_spaces(input);
	*keyword = '\0';
	if (!*input)	return input;						//	End of parsing
	while (*input) {									//	Find last keyword before '='
		if ((*input == '/') && (*(input++) == '*')) {	//	Handle comments!
			input++;
			while (*input && (*input != '*') && (*(input+1) != '/'))	input++;
			if (*input)	input += 2;		//	End of comment found, now skip to end
		}
//		skip_spaces(input);
		//	Note: I do the (redundant) check here for speed purposes,
		//	even though Parser::GetKeyword and sub-functions can handle
		//	*input == '\0' and exit appropriately
		if (*input)	input = GetKeyword(input, keyword);
		if (*input != '=') {
			//	ERROR HANDLER - stray keyword
			Error("Expected \"=\", but got something else");
			++input;	// This is a fix for a possible infinite loop - probably obsolete. -DH
//			break;
		} else {
			input++;
			break;
		}
	}
	skip_spaces(input);
	return input;
}


void Parser::ExDesc(char *input, LList<ExtraDesc *> &exdescList) {
	char *	keyword = get_buffer(PARSER_BUFFER_LENGTH);
	ExtraDesc *ed;
	
	while (*input /* && !error */) {
		input = ReadString(input, keyword);
		
		if (*keyword) {
			ed = new ExtraDesc();
			
			ed->keyword = SString::Create(keyword);
			
			skip_spaces(input);
			if (*input == '=') {
				input++;
				input = ReadString(input, keyword);
				if (*keyword)
					ed->description = SString::Create(keyword);
			} else
				SSFree(ed->keyword);
			
			if (ed->keyword->Data() && ed->description->Data()) {
				exdescList.Append(ed);
			} else {
				//	ERROR HANDLER - Bad exdesc
				Error("Bad exdesc");
				delete ed;
			}
		}
		while (*input && (*input != ';'))			input++;
		while (isspace(*input) || *input == ';')	input++;
	}
	release_buffer(keyword);
}


void Room::Parse(char *input) {
	static SInt32	zoneNum = 0;
	char *	keyword = get_buffer(PARSER_BUFFER_LENGTH);
	Trigger *trig = NULL;
	SInt32	i;
	
	if (Virtual() <= (zoneNum ? zone_table[zoneNum - 1].top : -1)) {
		log("SYSERR: Room #%d is below zone %d.", Virtual(), zoneNum);
		exit(1);
	}
	while (Virtual() > zone_table[zoneNum].top) {
		if (++zoneNum > top_of_zone_table) {
			log("SYSERR: Room %d is outside of any zone.", Virtual());
			exit(1);
		}
	}
	
	zone = zoneNum;
	ID(Virtual() + ROOM_ID_BASE);

	while (*input /* && !Parser::error */) {
		input = Parser::Parse(input, keyword);
		
		if (!*keyword)	break;
		
		// I've converted this to a switched setup to speed loading. -DH
		switch (*keyword) {
		case 'c':
			if (!str_cmp(keyword, "coord")) {
				input = Parser::GetKeyword(input, keyword);
				if (is_number(keyword))	coordx = atoi(keyword);
				else	Parser::Error("Expected \"coord = # #\", got \"%s\"", keyword);

				input = Parser::GetKeyword(input, keyword);
				if (is_number(keyword))	coordy = atoi(keyword);
				else	Parser::Error("Expected \"coord = # #\", got \"%s\"", keyword);
			} else {
				//	ERROR HANDLER - Unknown keyword
				Parser::Error("Unknown keyword \"%s\"", keyword);
			}
			break;
		case 'd':
			if (!str_cmp(keyword, "desc")) {
				input = Parser::ReadStringBlock(input, keyword);
				description = *keyword ? SString::Create(keyword) : NULL;
			} else {
				//	ERROR HANDLER - Unknown keyword
				Parser::Error("Unknown keyword \"%s\"", keyword);
			}
			break;
		case 'e':
			if (!str_cmp(keyword, "exits")) {
				input = Parser::ReadBlock(input, keyword);
				char *	secondKeyword = get_buffer(PARSER_BUFFER_LENGTH);
				char *	bufPtr = keyword;
				SInt32	key;
			
				while (*bufPtr /* && !error */) {
					bufPtr = Parser::Parse(bufPtr, secondKeyword);
				
					if (!*secondKeyword)	break;
				
					key = search_block(secondKeyword, (const char **)dirs, true);
					if (key != -1) {
						if (!dir_option[key]) {
							dir_option[key] = new RoomDirection();
							bufPtr = Parser::ReadBlock(bufPtr, secondKeyword);
							dir_option[key]->Parser(secondKeyword);
						} else {
							//	ERROR HANDLER - Duplicate direction
							Parser::Error("Duplicate direction %s", secondKeyword);
						}
					} else {
						//	ERROR HANDLER - Uknown direction
						Parser::Error("Unknown direction %s", secondKeyword);
					}
					while (*bufPtr && (*bufPtr != ';'))			bufPtr++;
					while (isspace(*bufPtr) || *bufPtr == ';')	bufPtr++;
				}
				release_buffer(secondKeyword);
			} else if (!str_cmp(keyword, "exdesc")) {
				input = Parser::ReadBlock(input, keyword);
				Parser::ExDesc(keyword, ex_description);
			} else {
				//	ERROR HANDLER - Unknown keyword
				Parser::Error("Unknown keyword \"%s\"", keyword);
			}
			break;
		case 'f':
			if (!str_cmp(keyword, "flags")) {
				Flags newflags = 0;	// Oh, how I hate this kludge.  It's necessary to keep privacy.
				input = Parser::FlagParser(input, &newflags, room_bits, "Room Flags");
				SetRoomFlags(newflags);
			} else {
				//	ERROR HANDLER - Unknown keyword
				Parser::Error("Unknown keyword \"%s\"", keyword);
			}
			break;
		case 'n':
			if (!str_cmp(keyword, "name")) {
				input = Parser::ReadString(input, keyword);
				SetName(*keyword ? str_dup(keyword) : NULL);
			} else {
				//	ERROR HANDLER - Unknown keyword
				Parser::Error("Unknown keyword \"%s\"", keyword);
			}
			break;
		case 's':
			if (!str_cmp(keyword, "sector")) {
				input = Parser::ReadString(input, keyword);
				if ((i = search_block(keyword, sector_types, false)) != -1)
					SetSectorType((Sector)i);
				else {
					SetSectorType(SECT_INSIDE);	//	Default value
				}
			} else if (!str_cmp(keyword, "specproc")) {
				input = Parser::GetKeyword(input, keyword);
				if ((world[vnum].func = spec_lookup(keyword, spec_room_table)) == NULL) {
					Parser::Error("Bad specproc \"%s\"", keyword);
				}
				input = Parser::ReadString(input, keyword);
				if (keyword)		world[vnum].farg = str_dup(keyword);
			} else {
				Parser::Error("Unknown keyword \"%s\"", keyword);
			}
			break;
		case 't':
			if (!str_cmp(keyword, "triggers")) {
				input = Parser::ReadBlock(input, keyword);
			
				char *	item = get_buffer(PARSER_BUFFER_LENGTH);
				for (char *itemPtr = Parser::GetKeyword(keyword, item); *item;
						itemPtr = Parser::GetKeyword(itemPtr, item)) {
					i = atoi(item);
					if ((trig = Trigger::Read(i))) {
						if (!(world[Virtual()].script))		world[Virtual()].script = new Script(Datatypes::Room);
						add_trigger(world[Virtual()].script, trig); //, loc);
					}
				}
				release_buffer(item);
			} else {
				//	ERROR HANDLER - Unknown keyword
				Parser::Error("Unknown keyword \"%s\"", keyword);
			}
			break;
		default:
			//	ERROR HANDLER - Unknown keyword
			Parser::Error("Unknown keyword \"%s\"", keyword);
		}
		while (*input && (*input != ';'))			input++;
		while (isspace(*input) || *input == ';')	input++;
	}
	coordx = MAX(MIN(coordx, 99), -1);
	coordy = MAX(MIN(coordy, 99), -1);

	if (coordx >= 0)	mapnums[coordy][coordx] = Virtual();

	release_buffer(keyword);
}


void RoomDirection::Parser(char *input) {
	char *	keyword = get_buffer(PARSER_BUFFER_LENGTH);
	
	while (*input /* && !Parser::error */) {
		input = Parser::Parse(input, keyword);
		
		if (!*keyword)	break;
		
		if (!str_cmp(keyword, "keyword")) {
			input = Parser::ReadString(input, keyword);
			this->keyword = *keyword ? str_dup(keyword) : NULL;
		} else if (!str_cmp(keyword, "general") || !str_cmp(keyword, "desc")) {
			input = Parser::ReadStringBlock(input, keyword);
			general_description = *keyword ? str_dup(keyword) : NULL;
		} else if (!str_cmp(keyword, "flags")) {
			input = Parser::FlagParser(input, &exit_info, exit_bits, "Exit Flags");
		} else if (!str_cmp(keyword, "hp")) {
			input = Parser::GetKeyword(input, keyword);
			if (is_number(keyword))	max_hit_points = atoi(keyword);
			else	Parser::Error("Expected \"hp = #\", got \"%s\"", keyword);
			hit_points = max_hit_points;
		} else if (!str_cmp(keyword, "pickmod")) {
			input = Parser::GetKeyword(input, keyword);
			if (is_number(keyword))	pick_modifier = atoi(keyword);
			else	Parser::Error("Expected \"pickmod = #\", got \"%s\"", keyword);
		} else if (!str_cmp(keyword, "dr")) {
			input = Parser::GetKeyword(input, keyword);
			if (is_number(keyword))	dam_resist = atoi(keyword);
			else	Parser::Error("Expected \"dr = #\", got \"%s\"", keyword);
		} else if (!str_cmp(keyword, "material")) {
			input = Parser::ReadString(input, keyword);
			if ((material = (Materials::Material)search_block(keyword, material_types, false)) == -1) {
				material = Materials::Undefined;	//	Default value
				Parser::Error("Bad material \"%s\"", keyword);
			}
		} else if (!str_cmp(keyword, "key")) {
			input = Parser::GetKeyword(input, keyword);
			if (is_number(keyword))	key = atoi(keyword);
			else {
				//	ERROR HANDLER - Non-number
				Parser::Error("Expected \"key = #\", got \"%s\"", keyword);
			}
		} else if (!str_cmp(keyword, "room")) {
			input = Parser::GetKeyword(input, keyword);
			if (is_number(keyword))	to_room = atoi(keyword);
			else {
				//	ERROR HANDLER - Non-number
				Parser::Error("Expected \"room = #\", got \"%s\"", keyword);
			}
		} else {
			//	ERROR HANDLER - Unknown field
			Parser::Error("Unknown roomdirection field \"%s\"", keyword);
		}
		while (*input && (*input != ';'))			input++;
		while (isspace(*input) || *input == ';')	input++;
	}
	release_buffer(keyword);
}


extern const char *Damage::types[];

void Object::Parser(char *input) {
	char *	keyword = get_buffer(PARSER_BUFFER_LENGTH);
	SInt32	temp;

	while (*input /* && !Parser::error */) {
		input = Parser::Parse(input, keyword);
		
		if (!*keyword)	break;
		
		if (!str_cmp(keyword, "keyw")) {
			input = Parser::ReadString(input, keyword);
			SetKeywords(keyword);
		} else if (!str_cmp(keyword, "name")) {
			input = Parser::ReadString(input, keyword);
			SetName(keyword);
		} else if (!str_cmp(keyword, "short")) {
			input = Parser::ReadString(input, keyword);
			SetSDesc(keyword);
		} else if (!str_cmp(keyword, "action")) {
			input = Parser::ReadString(input, keyword);
			actionDesc = *keyword ? SString::Create(keyword) : NULL;
		} else if (!str_cmp(keyword, "exdesc")) {
			input = Parser::ReadBlock(input, keyword);
			Parser::ExDesc(keyword, exDesc);
		} else if (!str_cmp(keyword, "values")) {
			input = Parser::ReadBlock(input, keyword);
			char *	secondKeyword = get_buffer(PARSER_BUFFER_LENGTH);
			char *	blockPtr = keyword;

			for (SInt32 count = 0; *blockPtr /* && !Parser::error */ && (count < 8); count++) {
				blockPtr = Parser::GetKeyword(blockPtr, secondKeyword);
				
				if (!*secondKeyword)	break;
				
				if (is_number(secondKeyword))	value[count] = atoi(secondKeyword);
				else	Parser::Error("Expected number for value %d", count);
			}
			
			release_buffer(secondKeyword);
		} else if (!str_cmp(keyword, "cost")) {
			input = Parser::GetKeyword(input, keyword);
			if (is_number(keyword))	cost = atoi(keyword);
			else					Parser::Error("Expected number for 'cost', got \"%s\"", keyword);
		} else if (!str_cmp(keyword, "weight")) {
			input = Parser::GetKeyword(input, keyword);
			if (is_number(keyword))	weight = atoi(keyword);
			else					Parser::Error("Expected number for 'weight', got \"%s\"", keyword);
		} else if (!str_cmp(keyword, "pd")) {
			input = Parser::GetKeyword(input, keyword);
			if (is_number(keyword))	passive_defense = atoi(keyword);
			else					Parser::Error("Expected number for 'pd', got \"%s\"", keyword);
		} else if (!str_cmp(keyword, "dr")) {
			input = Parser::GetKeyword(input, keyword);
			if (is_number(keyword))	damage_resistance = atoi(keyword);
			else					Parser::Error("Expected number for 'dr', got \"%s\"", keyword);
		} else if (!str_cmp(keyword, "timer")) {
			input = Parser::GetKeyword(input, keyword);
			if (is_number(keyword))	timer = atoi(keyword);
			else					Parser::Error("Expected number for 'timer', got \"%s\"", keyword);
		} else if (!str_cmp(keyword, "type")) {
			input = Parser::ReadString(input, keyword);
			if (is_number(keyword))	type = RANGE(atoi(keyword), 0, NUM_ITEM_TYPES - 1);
			else if ((temp = search_block(keyword, item_types, false)) == -1) {
				type = 0;	//	Default value
				Parser::Error("Bad item type name \"%s\"", keyword);
			} else
				type = temp;
		} else if (!str_cmp(keyword, "material")) {
			input = Parser::ReadString(input, keyword);
			if ((material = (Materials::Material)search_block(keyword, material_types, false)) == -1) {
				material = Materials::Undefined;	//	Default value
				Parser::Error("Bad material \"%s\"", keyword);
			}
		} else if (!str_cmp(keyword, "wear")) {
			input = Parser::FlagParser(input, &wear, wear_bits, "Object Wear Flags");
		} else if (!str_cmp(keyword, "extra")) {
			input = Parser::FlagParser(input, &extra, extra_bits, "Object Extra Flags");
		} else if (!str_cmp(keyword, "affects")) {
			input = Parser::FlagParser(input, &affectbits, affected_bits, "Object Affect Flags");
		} else if (!str_cmp(keyword, "applies")) {
			input = Parser::ReadBlock(input, keyword);
			char *	secondKeyword = get_buffer(PARSER_BUFFER_LENGTH);
			char *	blockPtr = keyword;

			for (SInt32 count = 0; *blockPtr && /* !Parser::error && */ (count < MAX_OBJ_AFFECT);
					count++) {
				blockPtr = Parser::Parse(blockPtr, secondKeyword);
				
				if (!*secondKeyword)	break;
				
				if ((temp = search_block(secondKeyword, apply_types, false)) != -1)
					affectmod[count].location = static_cast< ::Affect::Location >(temp);	//	Default value
				
				if (affectmod[count].location == ::Affect::None)	Parser::Error("Bad apply \"%s\"", secondKeyword);
				else {
					blockPtr = Parser::GetKeyword(blockPtr, secondKeyword);
					if (is_number(secondKeyword))	affectmod[count].modifier = atoi(secondKeyword);
					else	Parser::Error("Expected number for apply %s", apply_types[affectmod[count].location]);
				}
				while (*blockPtr && (*blockPtr != ';'))			blockPtr++;
				while (isspace(*blockPtr) || *blockPtr == ';')	blockPtr++;
			}
			
			release_buffer(secondKeyword);
		} else if (!str_cmp(keyword, "triggers")) {
			input = Parser::ReadBlock(input, keyword);
			
			char *	item = get_buffer(PARSER_BUFFER_LENGTH);
			for (char *itemPtr = Parser::GetKeyword(keyword, item); *item;
					itemPtr = Parser::GetKeyword(itemPtr, item)) {
				Index[Virtual()].triggers.Append(atoi(item));
			}
			release_buffer(item);
		} else if (!str_cmp(keyword, "specproc")) {
			input = Parser::GetKeyword(input, keyword);
			if ((Object::Index[vnum].func = spec_lookup(keyword, spec_obj_table)) == NULL) {
				Parser::Error("Bad specproc \"%s\"", keyword);
			}
			input = Parser::ReadString(input, keyword);
			if (keyword)		Object::Index[vnum].farg = str_dup(keyword);
		} else {	//	ERROR HANDLER - Unknown keyword
			Parser::Error("Unknown keyword \"%s\"", keyword);
		}
		while (*input && (*input != ';'))			input++;
		while (isspace(*input) || *input == ';')	input++;
	}
	release_buffer(keyword);
}


void Character::Parser(char *input) {
	char	*keyword = get_buffer(PARSER_BUFFER_LENGTH), *ptr;
	SInt32	i[5];

	while (*input /* && !Parser::error */) {
		input = Parser::Parse(input, keyword);
		
		if (!*keyword)	break;
		if (!str_cmp(keyword, "keyw")) {
			input = Parser::ReadString(input, keyword);
			SetKeywords(keyword);
		} else if (!str_cmp(keyword, "name")) {
			input = Parser::ReadString(input, keyword);
			SetName(keyword);
		} else if (!str_cmp(keyword, "short")) {
			input = Parser::ReadString(input, keyword);
			SetSDesc(keyword);
		} else if (!str_cmp(keyword, "long")) {
			input = Parser::ReadStringBlock(input, keyword);
			SetLDesc(keyword);
		} else if (!str_cmp(keyword, "sex")) {
			input = Parser::GetKeyword(input, keyword);
			if (is_number(keyword))	general.sex = (Sex)RANGE(0, atoi(keyword), NUM_GENDERS - 1);
			else if ((general.sex = (Sex)search_block(keyword, genders, false)) == -1) {
				general.sex = Male;	//	Default value
				Parser::Error("Bad gender \"%s\"", keyword);
			}
		} else if (!str_cmp(keyword, "race")) {
			input = Parser::GetKeyword(input, keyword);
			if (is_number(keyword))	general.race = (Race)RANGE(0, atoi(keyword), NUM_RACES - 1);
			else if ((general.race = (Race)search_block(keyword, race_types, false)) == -1) {
				general.race = RACE_ANIMAL;	//	Default value
				Parser::Error("Bad race \"%s\"", keyword);
			}
		} else if (!str_cmp(keyword, "mort")) {
			input = Parser::GetKeyword(input, keyword);
			if (is_number(keyword))	general.mortality = (Mortality)RANGE(0, atoi(keyword), NUM_MORTALITIES - 1);
			else if ((general.mortality = (Mortality)search_block(keyword, mortality_types, false)) == -1) {
				general.mortality = MORTALITY_MORTAL;	//	Default value
				Parser::Error("Bad mortality \"%s\"", keyword);
			}
		} else if (!str_cmp(keyword, "class")) {
			input = Parser::GetKeyword(input, keyword);
			if (is_number(keyword))	general.clss = (Class)RANGE(0, atoi(keyword), NUM_CLASSES - 1);
			else if ((general.clss = (Class) search_block(keyword, class_types, false)) == -1) {
				general.clss = CLASS_UNDEFINED;	//	Default value
				Parser::Error("Bad class \"%s\"", keyword);
			}
		} else if (!str_cmp(keyword, "level")) {
			input = Parser::GetKeyword(input, keyword);
			if (is_number(keyword))	general.level = RANGE(0, atoi(keyword), 100);
			else	Parser::Error("Expected \"level = #\", got \"%s\"", keyword);
		} else if (!str_cmp(keyword, "dr")) {
			input = Parser::GetKeyword(input, keyword);
			if (is_number(keyword))	GET_DR(this) = RANGE(0, atoi(keyword), 100);
			else	Parser::Error("Expected \"dr = #\", got \"%s\"", keyword);
		} else if (!str_cmp(keyword, "hp")) {
			input = Parser::GetKeyword(input, keyword);
			if (sscanf(keyword, " %d d %d + %d", i, i+1, i+2) == 3) {
				mob->numhitdice = i[0];
				mob->sizehitdice = i[1];
				mob->modhitdice = i[2];
			} else {
				mob->numhitdice = 0;
				mob->sizehitdice = 0;
				mob->modhitdice = 1;
				Parser::Error("Expected \"hp = #d#+#\", got \"%s\"", keyword);
			}
		} else if (!str_cmp(keyword, "damage")) {
			input = Parser::GetKeyword(input, keyword);
			if (sscanf(keyword, " %d d %d + %d", i, i+1, i+2) == 3) {
				mob->damage.number = i[0];
				mob->damage.size = i[1];
				points.damroll = i[2];
			} else {
				mob->damage.number = 0;
				mob->damage.size = 0;
				points.damroll = 1;
				//	ERROR HANDLER - Bad format damage
				Parser::Error("Expected \"damage = #d#+#\", got \"%s\"", keyword);
			}
		} else if (!str_cmp(keyword, "attack")) {
			input = Parser::GetKeyword(input, keyword);
			if (is_number(keyword))	mob->attack_type = RANGE(0, atoi(keyword), NUM_ATTACK_TYPES - 1);
			else if ((mob->attack_type = (Skill::Number(keyword, FIRST_COMBAT_MESSAGE, LAST_COMBAT_MESSAGE) - TYPE_HIT)) <= -1) {
				mob->attack_type = 0;
				Parser::Error("Bad attack type \"%s\"", keyword);
			}
		} else if (!str_cmp(keyword, "hitroll")) {
			input = Parser::GetKeyword(input, keyword);
			if (is_number(keyword))	GET_HITROLL(this) = atoi(keyword);
			else	Parser::Error("Expected \"hitroll = #\", got \"%s\"", keyword);
		} else if (!str_cmp(keyword, "align")) {
			input = Parser::GetKeyword(input, keyword);
			if (is_number(keyword))	GET_ALIGNMENT(this) = atoi(keyword);
			else	Parser::Error("Expected \"align = #\", got \"%s\"", keyword);
		} else if (!str_cmp(keyword, "disposition")) {
			input = Parser::GetKeyword(input, keyword);
			if (is_number(keyword))	GET_DISPOSITION(this) = atoi(keyword);
			else	Parser::Error("Expected \"disposition = #\", got \"%s\"", keyword);
		} else if (!str_cmp(keyword, "maxriders")) {
			input = Parser::GetKeyword(input, keyword);
			if (is_number(keyword))	GET_MAX_RIDERS(this) = atoi(keyword);
			else	Parser::Error("Expected \"maxriders = #\", got \"%s\"", keyword);
		} else if (!str_cmp(keyword, "gold")) {
			input = Parser::GetKeyword(input, keyword);
			if (is_number(keyword))	GET_GOLD(this) = atoi(keyword);
			else	Parser::Error("Expected \"gold = #\", got \"%s\"", keyword);
		} else if (!str_cmp(keyword, "expmod")) {
			input = Parser::GetKeyword(input, keyword);
			if (is_number(keyword))	GET_EXP_MOD(this) = atoi(keyword);
			else	Parser::Error("Expected \"expmod = #\", got \"%s\"", keyword);
		} else if (!str_cmp(keyword, "pts")) {
			input = Parser::GetKeyword(input, keyword);
			if (is_number(keyword))	GET_TOTALPTS(this) = atoi(keyword);
			else	Parser::Error("Expected \"pts = #\", got \"%s\"", keyword);
		} else if (!str_cmp(keyword, "position")) {
			input = Parser::GetKeyword(input, keyword);
			if (is_number(keyword))	general.position = (Position)RANGE(0, atoi(keyword), (int) POS_STANDING);
			else if ((general.position = (Position)search_block(keyword, positions, false)) == -1) {
				general.position = POS_STANDING;
				Parser::Error("Bad position \"%s\"", keyword);
			}
		} else if (!str_cmp(keyword, "defposition")) {
			input = Parser::GetKeyword(input, keyword);
			if (is_number(keyword))	mob->default_pos = (Position)RANGE(0, atoi(keyword), (int) POS_STANDING);
			else if ((mob->default_pos = (Position)search_block(keyword, positions, false)) == -1) {
				mob->default_pos = POS_STANDING;
				Parser::Error("Bad default position \"%s\"", keyword);
			}
		} else if (!str_cmp(keyword, "material")) {
			input = Parser::ReadString(input, keyword);
			if ((material = (Materials::Material)search_block(keyword, material_types, false)) == -1) {
				material = Materials::Undefined;	//	Default value
				Parser::Error("Bad material \"%s\"", keyword);
			}
		} else if (!str_cmp(keyword, "bodytype")) {
			input = Parser::GetKeyword(input, keyword);
			if (is_number(keyword))	GET_BODYTYPE(this) = (Bodytype)RANGE(0, atoi(keyword), ((int) MAX_BODYTYPE - 1));
			else if ((GET_BODYTYPE(this) = (Bodytype)search_block(keyword, bodytypes, false)) == -1) {
				GET_BODYTYPE(this) = BODYTYPE_HUMANOID;
				Parser::Error("Bad bodytype \"%s\"", keyword);
			}
		} else if (!str_cmp(keyword, "specproc")) {
			input = Parser::GetKeyword(input, keyword);
			if ((Character::Index[vnum].func = spec_lookup(keyword, spec_mob_table)) == NULL) {
				Parser::Error("Bad specproc \"%s\"", keyword);
			}
			input = Parser::ReadString(input, keyword);
			if (keyword)		Character::Index[vnum].farg = str_dup(keyword);
		} else if (!str_cmp(keyword, "flags")) {
			input = Parser::FlagParser(input, &general.act, action_bits, "Mob Flags");
		} else if (!str_cmp(keyword, "affects")) {
			input = Parser::FlagParser(input, &affectbits, affected_bits, "Mob Affect Flags");
		} else if (!str_cmp(keyword, "extended")) {
			input = Parser::GetKeyword(input, keyword);
			if (is_number(keyword))	MOB_TYPE(this) = atoi(keyword);
			else	Parser::Error("Expected \"type = #\", got \"%s\"", keyword);
		} else if (!str_cmp(keyword, "points")) {
			input = Parser::ReadBlock(input, keyword);
			char *	secondKeyword = get_buffer(PARSER_BUFFER_LENGTH);
			char *	valueString = get_buffer(PARSER_BUFFER_LENGTH);
			char *	blockPtr = keyword;
			SInt32	value;
			
			while (*blockPtr /* && !Parser::error */) {
				blockPtr = Parser::Parse(blockPtr, secondKeyword);
				blockPtr = Parser::GetKeyword(blockPtr, valueString);
				
				if (is_number(valueString))		value = RANGE(0, atoi(valueString), 200);
				else {
					Parser::Error("Expected points member = #, got \"%s\"", valueString);
					break;
				}
				
				
				if (!*secondKeyword)	break;
				if (!str_cmp(secondKeyword, "str"))				real_abils.Str = value;
				else if (!str_cmp(secondKeyword, "int"))		real_abils.Int = value;
				else if (!str_cmp(secondKeyword, "wis"))		real_abils.Wis = value;
				else if (!str_cmp(secondKeyword, "dex"))		real_abils.Dex = value;
				else if (!str_cmp(secondKeyword, "con"))		real_abils.Con = value;
				else if (!str_cmp(secondKeyword, "cha"))		real_abils.Cha = value;
				else Parser::Error("Unknown points member \"%s\"", secondKeyword);

				while (*blockPtr && (*blockPtr != ';'))			blockPtr++;
				while (isspace(*blockPtr) || *blockPtr == ';')	blockPtr++;
			}
			release_buffer(secondKeyword);
			release_buffer(valueString);
#ifdef GOT_RID_OF_IT
		} else if (!str_cmp(keyword, "dodge")) {
			input = Parser::GetKeyword(input, keyword);
			if (is_number(keyword))	mob->dodge = atoi(keyword);
			else	Parser::Error("Expected \"dodge = #\", got \"%s\"", keyword);
#endif
		} else if (!str_cmp(keyword, "clan")) {
			input = Parser::GetKeyword(input, keyword);
			if (is_number(keyword))	general.misc.clannum = MAX(0, atoi(keyword));
			else	Parser::Error("Expected \"clan = #\", got \"%s\"", keyword);
		} else if (!str_cmp(keyword, "opinions")) {
			input = Parser::ReadBlock(input, keyword);
			char *	secondKeyword = get_buffer(PARSER_BUFFER_LENGTH);
			char *	blockPtr = keyword;

			while (*blockPtr /* && !Parser::error */) {
				blockPtr = Parser::Parse(blockPtr, secondKeyword);
				
				if (!*secondKeyword)	break;
				
				Opinion *	theOpinion = NULL;
				
				if (!str_cmp(secondKeyword, "hates"))		mob->hates = theOpinion = new Opinion();
				else if (!str_cmp(secondKeyword, "fears"))	mob->fears = theOpinion = new Opinion();
				
				if (theOpinion) {
					blockPtr = Parser::ReadBlock(blockPtr, secondKeyword);
					char *	thirdKeyword = get_buffer(PARSER_BUFFER_LENGTH);
					char *	subBlockPtr = secondKeyword;
					
					while (*subBlockPtr /* && !Parser::error */) {
						subBlockPtr = Parser::Parse(subBlockPtr, thirdKeyword);
					
						if (!*thirdKeyword)	break;
						if (!str_cmp(thirdKeyword, "sexes")) {
							subBlockPtr = Parser::FlagParser(subBlockPtr, &theOpinion->sex, genders, "Opinion Genders");
						} else if (!str_cmp(thirdKeyword, "races")) {
							subBlockPtr = Parser::FlagParser(subBlockPtr, &theOpinion->race, race_types, "Opinion Genders");
						} else if (!str_cmp(thirdKeyword, "vnum")) {
							subBlockPtr = Parser::GetKeyword(subBlockPtr, thirdKeyword);
							if (is_number(thirdKeyword))	theOpinion->vnum = atoi(thirdKeyword);
							else Parser::Error("Expected vnum = #, got \"%s\"", thirdKeyword);				
						} else	Parser::Error("Unknown opinion keyword \"%s\"", thirdKeyword);
						while (*subBlockPtr && (*subBlockPtr != ';'))			subBlockPtr++;
						while (isspace(*subBlockPtr) || *subBlockPtr == ';')	subBlockPtr++;
					}
					release_buffer(thirdKeyword);
				} else	Parser::Error("Unknown opinion type \"%s\"", secondKeyword);
				while (*blockPtr && (*blockPtr != ';'))			blockPtr++;
				while (isspace(*blockPtr) || *blockPtr == ';')	blockPtr++;
			}
			release_buffer(secondKeyword);
		} else if (!str_cmp(keyword, "triggers")) {
			input = Parser::ReadBlock(input, keyword);
			
			char *	item = get_buffer(PARSER_BUFFER_LENGTH);
			for (char *itemPtr = Parser::GetKeyword(keyword, item); *item;
					itemPtr = Parser::GetKeyword(itemPtr, item)) {
				Index[Virtual()].triggers.Append(atoi(item));
			}
			release_buffer(item);



		} else if (!str_cmp(keyword, "skills")) {
			input = Parser::ReadBlock(input, keyword);
			char *	secondKeyword = get_buffer(PARSER_BUFFER_LENGTH);
			char *	valueString = get_buffer(PARSER_BUFFER_LENGTH);
			char *	blockPtr = keyword;
			SInt32	value;
			SInt16	skillnum;
			
			while (*blockPtr /* && !Parser::error */) {
				blockPtr = Parser::Parse(blockPtr, secondKeyword);
				blockPtr = Parser::GetKeyword(blockPtr, valueString);
				
				if (is_number(valueString))		value = RANGE(0, atoi(valueString), 255);
				else {
					Parser::Error("Expected skill = #, got \"%s\"", valueString);
					break;
				}
				
				// Replace all underscores with spaces
				while ((ptr = strchr(secondKeyword, '_')) != NULL)
					*ptr = ' ';

				if (!*secondKeyword)	break;
				skillnum = Skill::Number(secondKeyword);
				if (skillnum >= 0) {
					SET_SKILL(this, skillnum, value);
				} else {
					Parser::Error("Unknown skill \"%s\"", secondKeyword);
				}

				while (*blockPtr && (*blockPtr != ';'))			blockPtr++;
				while (isspace(*blockPtr) || *blockPtr == ';')	blockPtr++;
			}
			release_buffer(secondKeyword);
			release_buffer(valueString);
		} else
			Parser::Error("Unknown keyword \"%s\"", keyword);
		while (*input && (*input != ';'))			input++;
		while (isspace(*input) || *input == ';')	input++;
	}
	release_buffer(keyword);
}


void Shop::Parser(char *input) {
	char *	keyword = get_buffer(PARSER_BUFFER_LENGTH);
	SInt32	i;

	while (*input /* && !Parser::error */) {
		input = Parser::Parse(input, keyword);
		
		if (!*keyword)	break;
		if (!str_cmp(keyword, "buyprofit")) {
			input = Parser::GetKeyword(input, keyword);
			profit_buy = atof(keyword);
		} else if (!str_cmp(keyword, "sellprofit")) {
			input = Parser::GetKeyword(input, keyword);
			profit_sell = atof(keyword);
		} else if (!str_cmp(keyword, "keeper")) {
			input = Parser::GetKeyword(input, keyword);
			if (!is_number(keyword) || !Character::Find(keeper = atoi(keyword))) {
				Parser::Error("Bad keeper \"%s\"", keyword);
				keeper = -1;
			}
		} else if (!str_cmp(keyword, "produces")) {
			input = Parser::ReadBlock(input, keyword);
			
			char *	secondKeyword = get_buffer(PARSER_BUFFER_LENGTH);
			char *	blockPtr = keyword;
			while (*blockPtr) {
				blockPtr = Parser::GetKeyword(blockPtr, secondKeyword);
				if (Object::Find(i = atoi(secondKeyword)))
					producing.Append(i);
			}
			release_buffer(secondKeyword);
			
		} else if (!str_cmp(keyword, "rooms")) {
			input = Parser::ReadBlock(input, keyword);
			
			char *	secondKeyword = get_buffer(PARSER_BUFFER_LENGTH);
			char *	blockPtr = keyword;
			while (*blockPtr) {
				blockPtr = Parser::GetKeyword(blockPtr, secondKeyword);
				if (world.Find(i = atoi(secondKeyword)))
					in_room.Append(i);
			}
			release_buffer(secondKeyword);
			
		} else if (!str_cmp(keyword, "type")) {
			input = Parser::ReadBlock(input, keyword);
			
			type.SetSize(type.Count() + 1);
			BuyData &temp = type.Top();
			
			char *	secondKeyword = get_buffer(PARSER_BUFFER_LENGTH);
			char *	blockPtr = keyword;
			while (*blockPtr) {
				blockPtr = Parser::Parse(blockPtr, secondKeyword);
				
				if (!*secondKeyword)	break;
				
				if (!str_cmp(secondKeyword, "type")) {
					blockPtr = Parser::GetKeyword(blockPtr, secondKeyword);
					if (is_number(secondKeyword))	temp.type = RANGE(-1, atoi(secondKeyword), NUM_ITEM_TYPES - 1);
					else	Parser::Error("Expected \"type = #\", got \"%s\"", secondKeyword);
				} else if (!str_cmp(secondKeyword, "keywords")) {
					blockPtr = Parser::ReadString(blockPtr, secondKeyword);
					temp.keywords = str_dup(secondKeyword);
				} else
					Parser::Error("Unknown shop type keyword \"%s\"", secondKeyword);
				while (*blockPtr && (*blockPtr != ';'))				blockPtr++;
				while (isspace(*blockPtr) || (*blockPtr == ';'))	blockPtr++;
			}
			
			if (temp.type == -1)	type.Remove(type.Count() - 1);
			
			release_buffer(secondKeyword);
		} else if (!str_cmp(keyword, "notwith")) {
			input = Parser::FlagParser(input, &with_who, trade_letters, "Shop Trade Flags");
		} else if (!str_cmp(keyword, "flags")) {
			if (is_number(input))	bitvector = atoi(input);
			else input = Parser::FlagParser(input, &bitvector, shop_bits, "Shop Flags");
		} else if (!str_cmp(keyword, "no_item1")) {
			input = Parser::ReadString(input, keyword);
			no_such_item1 = str_dup(keyword);
		} else if (!str_cmp(keyword, "no_item2")) {
			input = Parser::ReadString(input, keyword);
			no_such_item2 = str_dup(keyword);
		} else if (!str_cmp(keyword, "no_cash1")) {
			input = Parser::ReadString(input, keyword);
			missing_cash1 = str_dup(keyword);
		} else if (!str_cmp(keyword, "no_cash2")) {
			input = Parser::ReadString(input, keyword);
			missing_cash2 = str_dup(keyword);
		} else if (!str_cmp(keyword, "do_not_buy")) {
			input = Parser::ReadString(input, keyword);
			do_not_buy = str_dup(keyword);
		} else if (!str_cmp(keyword, "message_buy")) {
			input = Parser::ReadString(input, keyword);
			message_buy = str_dup(keyword);
		} else if (!str_cmp(keyword, "message_sell")) {
			input = Parser::ReadString(input, keyword);
			message_sell = str_dup(keyword);
		} else if (!str_cmp(keyword, "prac_not_known")) {
			input = Parser::ReadString(input, keyword);
			prac_not_known = str_dup(keyword);
		} else if (!str_cmp(keyword, "prac_missing_cash")) {
			input = Parser::ReadString(input, keyword);
			prac_missing_cash = str_dup(keyword);
		} else if (!str_cmp(keyword, "open1")) {
			input = Parser::GetKeyword(input, keyword);
			if (is_number(keyword))	open1 = RANGE(0, atoi(keyword), 28);
			else	Parser::Error("Expected \"open1 = #\", got \"%s\"", keyword);
		} else if (!str_cmp(keyword, "open2")) {
			input = Parser::GetKeyword(input, keyword);
			if (is_number(keyword))	open2 = RANGE(0, atoi(keyword), 28);
			else	Parser::Error("Expected \"open2 = #\", got \"%s\"", keyword);
		} else if (!str_cmp(keyword, "close1")) {
			input = Parser::GetKeyword(input, keyword);
			if (is_number(keyword))	close1 = RANGE(0, atoi(keyword), 28);
			else	Parser::Error("Expected \"close1 = #\", got \"%s\"", keyword);
		} else if (!str_cmp(keyword, "close2")) {
			input = Parser::GetKeyword(input, keyword);
			if (is_number(keyword))	close2 = RANGE(0, atoi(keyword), 28);
			else	Parser::Error("Expected \"close2 = #\", got \"%s\"", keyword);
		} else
			Parser::Error("Unknown keyword \"%s\"", keyword);
		while (*input && (*input != ';'))			input++;
		while (isspace(*input) || *input == ';')	input++;
	}
	release_buffer(keyword);
}


const char *trig_kinds[] = {
	"MOB",
	"OBJECT",
	"WORLD",
	"\n"
};


void Trigger::Parse(char *input) {
	char *	keyword = get_buffer(PARSER_BUFFER_LENGTH);
	
	while (*input /* && !Parser::error */) {
		input = Parser::Parse(input, keyword);
		
		if (!*keyword)	break;
		
		if (!str_cmp(keyword, "name")) {
			input = Parser::ReadString(input, keyword);
			name = SString::Create(keyword);
		} else if (!str_cmp(keyword, "mobtriggers")) {
			input = Parser::FlagParser(input, &trigger_type_mob, mtrig_types, "Mob Trigger Type");
		} else if (!str_cmp(keyword, "objtriggers")) {
			input = Parser::FlagParser(input, &trigger_type_obj, otrig_types, "Obj Trigger Type");
		} else if (!str_cmp(keyword, "wldtriggers")) {
			input = Parser::FlagParser(input, &trigger_type_wld, wtrig_types, "Wld Trigger Type");
		} else if (!str_cmp(keyword, "commands")) {
			input = Parser::ReadStringBlock(input, keyword);
			char *s = keyword;
			CmdlistElement *cle;
			
			cmdlist = new Cmdlist(strtok(s, "\n\r"));
			cle = cmdlist->command;

			while ((s = strtok(NULL, "\n\r"))) {
				cle->next = new CmdlistElement(s);
				cle = cle->next;
			}
		} else if (!str_cmp(keyword, "arg")) {
			input = Parser::ReadString(input, keyword);
			arglist = SString::Create(keyword);
		} else if (!str_cmp(keyword, "narg")) {
			input = Parser::GetKeyword(input, keyword);
			narg = atoi(keyword);
		} else {
			//	ERROR HANDLER - Unknown keyword
			Parser::Error("Unknown keyword \"%s\"", keyword);
		}
		while (*input && (*input != ';'))			input++;
		while (isspace(*input) || *input == ';')	input++;
	}
	release_buffer(keyword);
}


void Skill::Parser(char *input) {
	char		*keyword = get_buffer(PARSER_BUFFER_LENGTH);
	SkillRef	tempref;
	int			tmp;
	char		*ptr;
	
	while (*input /* && !Parser::error */) {
		input = Parser::Parse(input, keyword);
		
		if (!*keyword)	break;
		
		if (!str_cmp(keyword, "name")) {
			input = Parser::ReadString(input, keyword);
			name = str_dup(keyword);
		} else if (!str_cmp(keyword, "types")) {
			input = Parser::FlagParser(input, &types, namedtypes, "Skill types");
		} else if (!str_cmp(keyword, "stats")) {
			input = Parser::FlagParser(input, &stats, named_bases, "Skill bases");
		} else if (!str_cmp(keyword, "wearoff")) {
			input = Parser::ReadString(input, keyword);
			wearoff = str_dup(keyword);
		} else if (!str_cmp(keyword, "wearoffroom")) {
			input = Parser::ReadString(input, keyword);
			wearoffroom = str_dup(keyword);
		} else if (!str_cmp(keyword, "position")) {
			input = Parser::FlagParser(input, &min_position, position_block, "Skill position");
		} else if (!str_cmp(keyword, "startmod")) {
			input = Parser::GetKeyword(input, keyword);
			if (is_number(keyword))	startmod = atoi(keyword);
			else	Parser::Error("Expected \"startmod = #\", got \"%s\"", keyword);
		} else if (!str_cmp(keyword, "mana")) {
			input = Parser::GetKeyword(input, keyword);
			if (is_number(keyword))	mana = atoi(keyword);
			else	Parser::Error("Expected \"mana = #\", got \"%s\"", keyword);
		} else if (!str_cmp(keyword, "maxincr")) {
			input = Parser::GetKeyword(input, keyword);
			if (is_number(keyword))	maxincr = atoi(keyword);
			else	Parser::Error("Expected \"maxincr = #\", got \"%s\"", keyword);
		} else if (!str_cmp(keyword, "violent")) {
			input = Parser::GetKeyword(input, keyword);
			if (is_number(keyword))	violent = atoi(keyword);
			else	Parser::Error("Expected \"violent = 0/1\", got \"%s\"", keyword);
		} else if (!str_cmp(keyword, "delay")) {
			input = Parser::GetKeyword(input, keyword);
			if (is_number(keyword))	delay = atoi(keyword);
			else	Parser::Error("Expected \"delay = #\", got \"%s\"", keyword);
		} else if (!str_cmp(keyword, "lag")) {
			input = Parser::GetKeyword(input, keyword);
			if (is_number(keyword))	lag = atoi(keyword);
			else	Parser::Error("Expected \"lag = #\", got \"%s\"", keyword);
		} else if (!str_cmp(keyword, "target")) {
			input = Parser::FlagParser(input, &target, Skill::targeting, "Skill target");
		} else if (!str_cmp(keyword, "routines")) {
			input = Parser::FlagParser(input, &routines, spell_routines, "Skill routines");
		} else if (!str_cmp(keyword, "prereq")) {
			input = Parser::GetKeyword(input, buf);

			tempref.skillnum = -1;
			tempref.chance = 0;

			if (!*buf)	Parser::Error("Skill:  Bad prerequisite \"%s\"", keyword);
			else {
				// Replace all underscores with spaces
				while ((ptr = strchr(buf, '_')) != NULL)
					*ptr = ' ';
				tempref.skillnum = Skill::Number(buf, FIRST_STAT, LAST_STAT);
			}
			
			if (tempref.skillnum < 0)	tempref.skillnum = Skill::Number(buf);

			if (tempref.skillnum >= 0) {
				input = Parser::GetKeyword(input, buf);
				if (!*buf)	Parser::Error("Skill:  Bad prerequisite \"%s\"", keyword);
				else		tempref.chance = atoi(buf);

				prerequisites.Append(tempref);
			}
		} else if (!str_cmp(keyword, "anreq")) {
			input = Parser::GetKeyword(input, buf);

			tempref.skillnum = -1;
			tempref.chance = 0;

			if (!*buf)	Parser::Error("Skill:  Bad anrequisite \"%s\"", keyword);
			else		{
				// Replace all underscores with spaces
				while ((ptr = strchr(buf, '_')) != NULL)
					*ptr = ' ';
				tempref.skillnum = Skill::Number(buf, FIRST_STAT, LAST_STAT);
			}
			
			if (tempref.skillnum < 0)	tempref.skillnum = Skill::Number(buf);

			if (tempref.skillnum >= 0) {
				input = Parser::GetKeyword(input, buf);
				if (!*buf)	Parser::Error("Skill:  Bad anrequisite \"%s\"", keyword);
				else		tempref.chance = atoi(buf);

				anrequisites.Append(tempref);
			}
		} else if (!str_cmp(keyword, "default")) {
			input = Parser::GetKeyword(input, buf);
			
			tempref.skillnum = -1;
			tempref.chance = 0;

			if (!*buf)	Parser::Error("Skill:  Bad prerequisite \"%s\"", keyword);
			else {
				// Replace all underscores with spaces
				while ((ptr = strchr(buf, '_')) != NULL)
					*ptr = ' ';
				tempref.skillnum = Skill::Number(buf, FIRST_STAT, LAST_STAT);
			}
			
			if (tempref.skillnum < 0)	tempref.skillnum = Skill::Number(buf);

			if (tempref.skillnum >= 0) {
				input = Parser::GetKeyword(input, buf);
				if (!*buf)	Parser::Error("Skill:  Bad default \"%s\"", keyword);
				else		tempref.chance = atoi(buf);

				defaults.Append(tempref);
			}
		} else if (!str_cmp(keyword, "saving")) {
			input = Parser::ReadBlock(input, keyword);
			properties.Add(Property::Type::Saving);
			properties[Property::Type::Saving]->Parse(keyword);
		} else if (!str_cmp(keyword, "material")) {
			input = Parser::ReadBlock(input, keyword);
			properties.Add(Property::Type::Materials);
			properties[Property::Type::Materials]->Parse(keyword);
		} else if (!str_cmp(keyword, "damage")) {
			input = Parser::ReadBlock(input, keyword);
			properties.Add(Property::Type::Damage);
			properties[Property::Type::Damage]->Parse(keyword);
		} else if (!str_cmp(keyword, "affect")) {
			input = Parser::ReadBlock(input, keyword);
			properties.Add(Property::Type::Affects);
			properties[Property::Type::Affects]->Parse(keyword);
		} else if (!str_cmp(keyword, "areas")) {
			input = Parser::ReadBlock(input, keyword);
			properties.Add(Property::Type::Areas);
			properties[Property::Type::Areas]->Parse(keyword);
		} else if (!str_cmp(keyword, "summons")) {
			input = Parser::ReadBlock(input, keyword);
			properties.Add(Property::Type::Summons);
			properties[Property::Type::Summons]->Parse(keyword);
		} else if (!str_cmp(keyword, "points")) {
			input = Parser::ReadBlock(input, keyword);
			properties.Add(Property::Type::Points);
			properties[Property::Type::Points]->Parse(keyword);
		} else if (!str_cmp(keyword, "unaffect")) {
			input = Parser::ReadBlock(input, keyword);
			properties.Add(Property::Type::Unaffects);
			properties[Property::Type::Unaffects]->Parse(keyword);
		} else if (!str_cmp(keyword, "creations")) {
			input = Parser::ReadBlock(input, keyword);
			properties.Add(Property::Type::Creations);
			properties[Property::Type::Creations]->Parse(keyword);
		} else if (!str_cmp(keyword, "room")) {
			input = Parser::ReadBlock(input, keyword);
			properties.Add(Property::Type::Room);
			properties[Property::Type::Room]->Parse(keyword);
		} else if (!str_cmp(keyword, "scripted")) {
			input = Parser::ReadBlock(input, keyword);
			properties.Add(Property::Type::Scripted);
			properties[Property::Type::Scripted]->Parse(keyword);
		} else if (!str_cmp(keyword, "concentrate")) {
			input = Parser::ReadBlock(input, keyword);
			properties.Add(Property::Type::Concentrate);
			properties[Property::Type::Concentrate]->Parse(keyword);
		} else if (!str_cmp(keyword, "misfire")) {
			input = Parser::ReadBlock(input, keyword);
			properties.Add(Property::Type::Misfire);
			properties[Property::Type::Misfire]->Parse(keyword);
		} else if (!str_cmp(keyword, "manual")) {
			input = Parser::ReadBlock(input, keyword);
			properties.Add(Property::Type::Manual);
			properties[Property::Type::Manual]->Parse(keyword);
		} else {
			//	ERROR HANDLER - Unknown keyword
			Parser::Error("Unknown keyword \"%s\"", keyword);
		}
		while (*input && (*input != ';'))			++input;
		while (isspace(*input) || *input == ';')	++input;
	}
	release_buffer(keyword);
}


void Method::Parser(char *input) {
	char			*keyword = get_buffer(PARSER_BUFFER_LENGTH);
    SInt32			tmp;
	char			*ptr;
    MethodMessage	*msg = NULL;

    startfunc = (void (*)(Character *, SInt16, SInt16, SInt16, MUDObject *)) funcs[0];
    endfunc = (void (*)(Character *, SInt16, SInt16, SInt16, MUDObject *)) funcs[1];
    disruptfunc = (void (*)(Character *, SInt16, SInt16, SInt16, MUDObject *)) funcs[2];

	while (*input /* && !Parser::error */) {
		input = Parser::Parse(input, keyword);
		
		if (!*keyword)	break;
		
		if (!str_cmp(keyword, "verb")) {
			input = Parser::ReadString(input, keyword);
			verb = str_dup(keyword);
		} else if (!str_cmp(keyword, "startchar")) {
			input = Parser::ReadString(input, keyword);
			startchar = str_dup(keyword);
		} else if (!str_cmp(keyword, "startroom")) {
			input = Parser::ReadString(input, keyword);
			startroom = str_dup(keyword);
		} else if (!str_cmp(keyword, "stage")) {
			input = Parser::ReadString(input, buf1);
			input = Parser::ReadString(input, buf2);
            msg = new MethodMessage;
			msg->tochar = str_dup(buf1);
			msg->toroom = str_dup(buf2);
            Messages.Append(*msg);
            msg = NULL;
		} else if (!str_cmp(keyword, "startvis")) {
			input = Parser::ReadString(input, keyword);
			startvis = (atoi(keyword) ? true : false);
		} else if (!str_cmp(keyword, "stagevis")) {
			input = Parser::ReadString(input, keyword);
			endvis = (atoi(keyword) ? true : false);
		} else if (!str_cmp(keyword, "skill")) {
			input = Parser::ReadString(input, keyword);
			skill = Skill::Number(keyword);
		} else if (!str_cmp(keyword, "hit")) {
			input = Parser::GetKeyword(input, keyword);
			if (is_number(keyword))	hit = atoi(keyword);
			else	Parser::Error("Expected \"hit = #\", got \"%s\"", keyword);
		} else if (!str_cmp(keyword, "mana")) {
			input = Parser::GetKeyword(input, keyword);
			if (is_number(keyword))	mana = atoi(keyword);
			else	Parser::Error("Expected \"mana = #\", got \"%s\"", keyword);
		} else if (!str_cmp(keyword, "move")) {
			input = Parser::GetKeyword(input, keyword);
			if (is_number(keyword))	move = atoi(keyword);
			else	Parser::Error("Expected \"move = 0/1\", got \"%s\"", keyword);
		} else if (!str_cmp(keyword, "build")) {
			input = Parser::GetKeyword(input, keyword);
			if (is_number(keyword))	build = atoi(keyword);
			else	Parser::Error("Expected \"build = #\", got \"%s\"", keyword);
		} else if (!str_cmp(keyword, "decay")) {
			input = Parser::GetKeyword(input, keyword);
			if (is_number(keyword))	decay = atoi(keyword);
			else	Parser::Error("Expected \"decay = #\", got \"%s\"", keyword);
		} else if (!str_cmp(keyword, "ceiling")) {
			input = Parser::GetKeyword(input, keyword);
			if (is_number(keyword))	ceiling = atoi(keyword);
			else	Parser::Error("Expected \"ceiling = #\", got \"%s\"", keyword);
		} else if (!str_cmp(keyword, "startfunc")) {
			input = Parser::GetKeyword(input, keyword);
			if ((tmp = search_block(keyword, class_types, false)) == -1) {
				tmp = 0;
				Parser::Error("Bad startfunc \"%s\"", keyword);
			}
            startfunc = (void (*)(Character *, SInt16, SInt16, SInt16, MUDObject *)) funcs[tmp];
		} else if (!str_cmp(keyword, "endfunc")) {
			input = Parser::GetKeyword(input, keyword);
			if ((tmp = search_block(keyword, class_types, false)) == -1) {
				tmp = 1;
				Parser::Error("Bad endfunc \"%s\"", keyword);
			}
            endfunc = (void (*)(Character *, SInt16, SInt16, SInt16, MUDObject *)) funcs[tmp];
		} else if (!str_cmp(keyword, "disruptfunc")) {
			input = Parser::GetKeyword(input, keyword);
			if ((tmp = search_block(keyword, class_types, false)) == -1) {
				tmp = 2;
				Parser::Error("Bad disruptfunc \"%s\"", keyword);
			}
            disruptfunc = (void (*)(Character *, SInt16, SInt16, SInt16, MUDObject *)) funcs[tmp];
		} else {
			//	ERROR HANDLER - Unknown keyword
			Parser::Error("Unknown keyword \"%s\"", keyword);
		}
		while (*input && (*input != ';'))			++input;
		while (isspace(*input) || *input == ';')	++input;
	}
	release_buffer(keyword);
}


namespace Property {

	void Saving::Parse(char *input)
	{
	    char	   *keyword = get_buffer(PARSER_BUFFER_LENGTH);

	    while (*input /* && !Parser::error */) {
	    	input = Parser::Parse(input, keyword);

	    	if (!*keyword) break;

	    	if (!str_cmp(keyword, "formula")) {
	    		input = Parser::ReadString(input, keyword);
	    		formula = str_dup(keyword);
	    	} else {
	    		// ERROR HANDLER - Unknown keyword
	    		Parser::Error("Unknown keyword \"%s\"", keyword);
	    	}
	    	while (*input && (*input != ';'))		   ++input;
	    	while (isspace(*input) || *input == ';')   ++input;
	    }
	    release_buffer(keyword);
	}


	void Materials::Parse(char *input)
	{
	    char		*keyword = get_buffer(PARSER_BUFFER_LENGTH);
		int			i;

	    while (*input /* && !Parser::error */) {
	    	input = Parser::Parse(input, keyword);

	    	if (!*keyword) break;

	    	if (!str_cmp(keyword, "components")) {
	    		input = Parser::ReadBlock(input, keyword);

	    		char * secondKeyword = get_buffer(PARSER_BUFFER_LENGTH);
	    		char * blockPtr = keyword;
	    		while (*blockPtr) {
	    			blockPtr = Parser::GetKeyword(blockPtr, secondKeyword);
	    			if (Object::Find(i = atoi(secondKeyword)))
	    				components.Append(i);
	    		}
	    		release_buffer(secondKeyword);

	    	} else {
	    		// ERROR HANDLER - Unknown keyword
	    		Parser::Error("Unknown keyword \"%s\"", keyword);
	    	}
	    	while (*input && (*input != ';'))		   ++input;
	    	while (isspace(*input) || *input == ';')   ++input;
	    }
	    release_buffer(keyword);
	}


	void Damage::Parse(char *input)
	{
	    char	   *keyword = get_buffer(PARSER_BUFFER_LENGTH);

	    while (*input /* && !Parser::error */) {
	    	input = Parser::Parse(input, keyword);

	    	if (!*keyword) break;

	    	if (!str_cmp(keyword, "formula")) {
	    		input = Parser::ReadString(input, keyword);
	    		formula = str_dup(keyword);
	    	} else if (!str_cmp(keyword, "range")) {
	    		input = Parser::GetKeyword(input, keyword);
	    		if (is_number(keyword))    range = atoi(keyword);
	    		else   Parser::Error("Expected \"range = #\", got \"%s\"", keyword);
	    	} else if (!str_cmp(keyword, "numdice")) {
	    		input = Parser::GetKeyword(input, keyword);
	    		if (is_number(keyword))    numdice = atoi(keyword);
	    		else   Parser::Error("Expected \"range = #\", got \"%s\"", keyword);
	    	} else if (!str_cmp(keyword, "sizedice")) {
	    		input = Parser::GetKeyword(input, keyword);
	    		if (is_number(keyword))    sizedice = atoi(keyword);
	    		else   Parser::Error("Expected \"range = #\", got \"%s\"", keyword);
	    	} else if (!str_cmp(keyword, "modifier")) {
	    		input = Parser::GetKeyword(input, keyword);
	    		if (is_number(keyword))    modifier = atoi(keyword);
	    		else   Parser::Error("Expected \"range = #\", got \"%s\"", keyword);
	    	} else if (!str_cmp(keyword, "damagetype")) {
	    		input = Parser::ReadString(input, keyword);
	    		if (is_number(keyword))    damagetype = RANGE(0, atoi(keyword), ::Damage::NumTypes);
	    		else if ((damagetype = search_block(keyword, damage_types, false)) == -1) {
	    			damagetype = ::Damage::Undefined;    //  Default value
	    			Parser::Error("Bad damagetype \"%s\"", keyword);
	    		}
	    	} else if (!str_cmp(keyword, "hitslocation")) {
	    		input = Parser::GetKeyword(input, keyword);
	    		if (is_number(keyword))    hitslocation = atoi(keyword);
	    		else   Parser::Error("Expected \"hitslocation = 0/1\", got \"%s\"", keyword);
	    	} else {
	    		// ERROR HANDLER - Unknown keyword
	    		Parser::Error("Unknown keyword \"%s\"", keyword);
	    	}
	    	while (*input && (*input != ';'))		   ++input;
	    	while (isspace(*input) || *input == ';')   ++input;
	    }
	    release_buffer(keyword);
	}


	void Affects::Parse(char *input)
	{
	    char	   *keyword = get_buffer(PARSER_BUFFER_LENGTH);
	    ::AffectEditable	* tempaffect;
		SInt32		tmpmodifier, tmpduration;
		Flags		tmpflags;
		::Affect::Location	tmplocation;

	    while (*input /* && !Parser::error */) {
	    	input = Parser::Parse(input, keyword);

	    	if (!*keyword) break;

	    	if (!str_cmp(keyword, "affect")) {
	    		input = Parser::GetKeyword(input, keyword);

	    		if (!*keyword) Parser::Error("Skill:  Bad affect \"%s\"", keyword);

	    		if (is_number(keyword))    tmplocation = (::Affect::Location) RANGE(0, atoi(keyword), ::Affect::MAX);
	    		else if ((tmplocation = (::Affect::Location) search_block(keyword, apply_types, false)) == -1) {
	    			tmplocation = ::Affect::None;    //  Default value
	    			Parser::Error("Bad affect \"%s\"", keyword);
	    		}

	    		if (tmplocation != -1) {
	    			input = Parser::GetKeyword(input, keyword);
	    			if (is_number(keyword))    tmpmodifier = atoi(keyword);
	    			else   Parser::Error("Skill:  Bad affect \"%s\"", keyword);;

	    			input = Parser::GetKeyword(input, keyword);
	    			if (is_number(keyword))    tmpduration = atoi(keyword);
	    			else   Parser::Error("Skill:  Bad affect \"%s\"", keyword);;

					tmpflags = 0;
	    			input = Parser::FlagParser(input, &tmpflags, affected_bits, "Affect Property Flags");

					tempaffect = new ::AffectEditable(tmpmodifier, tmpduration, tmplocation, tmpflags);
					affects.Append(*tempaffect);
	    		}

	    	} else if (!str_cmp(keyword, "tovict")) {
	    		input = Parser::ReadString(input, keyword);
	    		tovict = str_dup(keyword);
	    	} else if (!str_cmp(keyword, "toroom")) {
	    		input = Parser::ReadString(input, keyword);
	    		toroom = str_dup(keyword);
	    	} else if (!str_cmp(keyword, "addaffect")) {
	    		input = Parser::GetKeyword(input, keyword);
	    		if (is_number(keyword))    accum_affect = atoi(keyword);
	    		else   Parser::Error("Expected \"addaffect = 0/1\", got \"%s\"", keyword);
	    	} else if (!str_cmp(keyword, "addduration")) {
	    		input = Parser::GetKeyword(input, keyword);
	    		if (is_number(keyword))    accum_duration = atoi(keyword);
	    		else   Parser::Error("Expected \"addduration = 0/1\", got \"%s\"", keyword);
	    	} else if (!str_cmp(keyword, "tovict")) {
	    		input = Parser::ReadString(input, keyword);
	    		tovict = str_dup(keyword);
	    	} else if (!str_cmp(keyword, "toroom")) {
	    		input = Parser::ReadString(input, keyword);
	    		toroom = str_dup(keyword);
			} else if (!str_cmp(keyword, "totrig")) {
				input = Parser::ReadStringBlock(input, keyword);
				char *s = keyword;
				CmdlistElement *cle;

				totrig = new Trigger;
			
				totrig->cmdlist = new Cmdlist(strtok(s, "\n\r"));
				cle = totrig->cmdlist->command;

				while ((s = strtok(NULL, "\n\r"))) {
					cle->next = new CmdlistElement(s);
					cle = cle->next;
				}
			} else if (!str_cmp(keyword, "fromtrig")) {
				input = Parser::ReadStringBlock(input, keyword);
				char *s = keyword;
				CmdlistElement *cle;
			
				fromtrig = new Trigger;

				fromtrig->cmdlist = new Cmdlist(strtok(s, "\n\r"));
				cle = fromtrig->cmdlist->command;

				while ((s = strtok(NULL, "\n\r"))) {
					cle->next = new CmdlistElement(s);
					cle = cle->next;
				}
	    	} else {
	    		// ERROR HANDLER - Unknown keyword
	    		Parser::Error("Unknown keyword \"%s\"", keyword);
	    	}
	    	while (*input && (*input != ';'))		   ++input;
	    	while (isspace(*input) || *input == ';')   ++input;
	    }
	    release_buffer(keyword);
	}


	void Areas::Parse(char *input)
	{
	    char	   *keyword = get_buffer(PARSER_BUFFER_LENGTH);

	    while (*input /* && !Parser::error */) {
	    	input = Parser::Parse(input, keyword);

	    	if (!*keyword) break;

	    	if (!str_cmp(keyword, "flags")) {
	    		input = Parser::FlagParser(input, &flags, targeting_types, "Area Spell Flags");
	    	} else if (!str_cmp(keyword, "how_many")) {
	    		input = Parser::GetKeyword(input, keyword);
	    		if (is_number(keyword))    how_many = atoi(keyword);
	    		else   Parser::Error("Expected \"how_many = #\", got \"%s\"", keyword);
	    	} else if (!str_cmp(keyword, "tochar")) {
	    		input = Parser::ReadString(input, keyword);
	    		tochar = str_dup(keyword);
	    	} else if (!str_cmp(keyword, "toroom")) {
	    		input = Parser::ReadString(input, keyword);
	    		toroom = str_dup(keyword);
	    	} else {
	    		// ERROR HANDLER - Unknown keyword
	    		Parser::Error("Unknown keyword \"%s\"", keyword);
	    	}
	    	while (*input && (*input != ';'))		   ++input;
	    	while (isspace(*input) || *input == ';')   ++input;
	    }
	    release_buffer(keyword);
	}


	void Summons::Parse(char *input)
	{
	    char	   *keyword = get_buffer(PARSER_BUFFER_LENGTH);

	    while (*input /* && !Parser::error */) {
	    	input = Parser::Parse(input, keyword);

	    	if (!*keyword) break;

	    	if (!str_cmp(keyword, "tosummon")) {
	    		input = Parser::GetKeyword(input, keyword);
	    		if (is_number(keyword))    tosummon = atoi(keyword);
	    		else   Parser::Error("Expected \"tosummon = #\", got \"%s\"", keyword);
	    	} else if (!str_cmp(keyword, "tochar")) {
	    		input = Parser::ReadString(input, keyword);
	    		tochar = str_dup(keyword);
	    	} else if (!str_cmp(keyword, "toroom")) {
	    		input = Parser::ReadString(input, keyword);
	    		toroom = str_dup(keyword);
	    	} else {
	    		// ERROR HANDLER - Unknown keyword
	    		Parser::Error("Unknown keyword \"%s\"", keyword);
	    	}
	    	while (*input && (*input != ';'))		   ++input;
	    	while (isspace(*input) || *input == ';')   ++input;
	    }
	    release_buffer(keyword);
	}


	void Points::Parse(char *input)
	{
	    char	   *keyword = get_buffer(PARSER_BUFFER_LENGTH);
	    SInt32 i[3];

	    while (*input /* && !Parser::error */) {
	    	input = Parser::Parse(input, keyword);

	    	if (!*keyword) break;

	    	if (!str_cmp(keyword, "hitnumdice")) {
	    		input = Parser::GetKeyword(input, keyword);
	    		if (is_number(keyword))    hitnumdice = atoi(keyword);
	    		else   Parser::Error("Expected \"hitnumdice = #\", got \"%s\"", keyword);
	    	} else if (!str_cmp(keyword, "hitsizedice")) {
	    		input = Parser::GetKeyword(input, keyword);
	    		if (is_number(keyword))    hitsizedice = atoi(keyword);
	    		else   Parser::Error("Expected \"hitsizedice = #\", got \"%s\"", keyword);
	    	} else if (!str_cmp(keyword, "hit")) {
	    		input = Parser::GetKeyword(input, keyword);
	    		if (is_number(keyword))    hit = atoi(keyword);
	    		else   Parser::Error("Expected \"hit = #\", got \"%s\"", keyword);
	    	} else if (!str_cmp(keyword, "mananumdice")) {
	    		input = Parser::GetKeyword(input, keyword);
	    		if (is_number(keyword))    mananumdice = atoi(keyword);
	    		else   Parser::Error("Expected \"mananumdice = #\", got \"%s\"", keyword);
	    	} else if (!str_cmp(keyword, "manasizedice")) {
	    		input = Parser::GetKeyword(input, keyword);
	    		if (is_number(keyword))    manasizedice = atoi(keyword);
	    		else   Parser::Error("Expected \"manasizedice = #\", got \"%s\"", keyword);
	    	} else if (!str_cmp(keyword, "mana")) {
	    		input = Parser::GetKeyword(input, keyword);
	    		if (is_number(keyword))    mana = atoi(keyword);
	    		else   Parser::Error("Expected \"mana = #\", got \"%s\"", keyword);
	    	} else if (!str_cmp(keyword, "movenumdice")) {
	    		input = Parser::GetKeyword(input, keyword);
	    		if (is_number(keyword))    movenumdice = atoi(keyword);
	    		else   Parser::Error("Expected \"movenumdice = #\", got \"%s\"", keyword);
	    	} else if (!str_cmp(keyword, "movesizedice")) {
	    		input = Parser::GetKeyword(input, keyword);
	    		if (is_number(keyword))    movesizedice = atoi(keyword);
	    		else   Parser::Error("Expected \"movesizedice = #\", got \"%s\"", keyword);
	    	} else if (!str_cmp(keyword, "move")) {
	    		input = Parser::GetKeyword(input, keyword);
	    		if (is_number(keyword))    hit = atoi(keyword);
	    		else   Parser::Error("Expected \"move = #\", got \"%s\"", keyword);
#if 0
	    	if (!str_cmp(keyword, "hit")) {
	    		input = Parser::ReadString(input, keyword);
	    		if (sscanf(keyword, "%d d %d + %d", i, i+1, i+2) == 3) {
	    			hitnumdice = i[0];
	    			hitsizedice = i[1];
	    			hit = i[2];
	    		} else {
	    			Parser::Error("Expected \"hit = #d#+#\", got \"%s\"", keyword);
	    		}
	    	} else if (!str_cmp(keyword, "mana")) {
	    		input = Parser::ReadString(input, keyword);
	    		if (sscanf(keyword, "%d d %d + %d", i, i+1, i+2) == 3) {
	    			mananumdice = i[0];
	    			manasizedice = i[1];
	    			mana = i[2];
	    		} else {
	    			Parser::Error("Expected \"mana = #d#+#\", got \"%s\"", keyword);
	    		}
	    	} else if (!str_cmp(keyword, "move")) {
	    		input = Parser::ReadString(input, keyword);
	    		if (sscanf(keyword, "%d d %d + %d", i, i+1, i+2) == 3) {
	    			movenumdice = i[0];
	    			movesizedice = i[1];
	    			move = i[2];
	    		} else {
	    			Parser::Error("Expected \"move = #d#+#\", got \"%s\"", keyword);
	    		}
#endif
	    	} else if (!str_cmp(keyword, "formula")) {
	    		input = Parser::ReadString(input, keyword);
	    		formula = str_dup(keyword);
	    	} else if (!str_cmp(keyword, "tochar")) {
	    		input = Parser::ReadString(input, keyword);
	    		tochar = str_dup(keyword);
	    	} else if (!str_cmp(keyword, "tovict")) {
	    		input = Parser::ReadString(input, keyword);
	    		tovict = str_dup(keyword);
	    	} else if (!str_cmp(keyword, "toroom")) {
	    		input = Parser::ReadString(input, keyword);
	    		toroom = str_dup(keyword);
	    	} else {
	    		// ERROR HANDLER - Unknown keyword
	    		Parser::Error("Unknown keyword \"%s\"", keyword);
	    	}
	    	while (*input && (*input != ';'))		   ++input;
	    	while (isspace(*input) || *input == ';')   ++input;
	    }
	    release_buffer(keyword);
	}


	void Unaffects::Parse(char *input)
	{
	    char	   *keyword = get_buffer(PARSER_BUFFER_LENGTH);

	    while (*input /* && !Parser::error */) {
	    	input = Parser::Parse(input, keyword);

	    	if (!*keyword) break;

	    	if (!str_cmp(keyword, "spellnum")) {
	    		input = Parser::ReadString(input, keyword);
				if (is_number(keyword))	spellnum = atoi(keyword);
				else if ((spellnum = (Skill::Number(keyword, FIRST_SPELL))) <= -1) {
					spellnum = 0;
					Parser::Error("Bad spell \"%s\"", keyword);
				}
	    	} else if (!str_cmp(keyword, "tovict")) {
	    		input = Parser::ReadString(input, keyword);
	    		tovict = str_dup(keyword);
	    	} else if (!str_cmp(keyword, "toroom")) {
	    		input = Parser::ReadString(input, keyword);
	    		toroom = str_dup(keyword);
	    	} else {
	    		// ERROR HANDLER - Unknown keyword
	    		Parser::Error("Unknown keyword \"%s\"", keyword);
	    	}
	    	while (*input && (*input != ';'))		   ++input;
	    	while (isspace(*input) || *input == ';')   ++input;
	    }
	    release_buffer(keyword);
	}


	void Creations::Parse(char *input)
	{
	    char	   *keyword = get_buffer(PARSER_BUFFER_LENGTH);
		VNum		i, num;

	    while (*input /* && !Parser::error */) {
	    	input = Parser::Parse(input, keyword);

	    	if (!*keyword) break;

	    	if (!str_cmp(keyword, "objects")) {
	    		input = Parser::ReadBlock(input, keyword);

	    		char * secondKeyword = get_buffer(PARSER_BUFFER_LENGTH);
				i = 0;
				for (char *itemPtr = Parser::GetKeyword(keyword, secondKeyword); *secondKeyword;
						itemPtr = Parser::GetKeyword(itemPtr, secondKeyword)) {
	    			objects.Append(atoi(secondKeyword));
	    		}
	    		release_buffer(secondKeyword);
	    	} else {
	    		// ERROR HANDLER - Unknown keyword
	    		Parser::Error("Unknown keyword \"%s\"", keyword);
	    	}
	    	while (*input && (*input != ';'))		   ++input;
	    	while (isspace(*input) || *input == ';')   ++input;
	    }
	    release_buffer(keyword);
	}


	void Room::Parse(char *input)
	{
	    char	   *keyword = get_buffer(PARSER_BUFFER_LENGTH);
	    ::AffectEditable	* tempaffect;
		SInt32		tmpmodifier, tmpduration;
		Flags		tmpflags;
		::Affect::Location	tmplocation;

	    while (*input /* && !Parser::error */) {
	    	input = Parser::Parse(input, keyword);

	    	if (!*keyword) break;

	    	if (!str_cmp(keyword, "affect")) {
	    		input = Parser::GetKeyword(input, keyword);

	    		if (!*keyword) Parser::Error("Skill:  Bad affect \"%s\"", keyword);

	    		if (is_number(keyword))    tmplocation = (::Affect::Location) RANGE(0, atoi(keyword), ::Affect::MAX);
	    		else if ((tmplocation = (::Affect::Location) search_block(keyword, apply_types, false)) == -1) {
	    			tmplocation = ::Affect::None;    //  Default value
	    			Parser::Error("Bad affect \"%s\"", keyword);
	    		}

	    		if (tmplocation != -1) {
	    			input = Parser::GetKeyword(input, keyword);
	    			if (is_number(keyword))    tmpmodifier = atoi(keyword);
	    			else   Parser::Error("Skill:  Bad affect \"%s\"", keyword);;

	    			input = Parser::GetKeyword(input, keyword);
	    			if (is_number(keyword))    tmpduration = atoi(keyword);
	    			else   Parser::Error("Skill:  Bad affect \"%s\"", keyword);;

					tmpflags = 0;
	    			input = Parser::FlagParser(input, &tmpflags, affected_bits, "Affect Property Flags");

					tempaffect = new ::AffectEditable(tmpmodifier, tmpduration, tmplocation, tmpflags);
					affects.Append(*tempaffect);
	    		}

	    	} else if (!str_cmp(keyword, "tochar")) {
	    		input = Parser::ReadString(input, keyword);
	    		tochar = str_dup(keyword);
	    	} else if (!str_cmp(keyword, "toroom")) {
	    		input = Parser::ReadString(input, keyword);
	    		toroom = str_dup(keyword);
	    	} else {
	    		// ERROR HANDLER - Unknown keyword
	    		Parser::Error("Unknown keyword \"%s\"", keyword);
	    	}
	    	while (*input && (*input != ';'))		   ++input;
	    	while (isspace(*input) || *input == ';')   ++input;
	    }
	    release_buffer(keyword);
	}


	void Scripted::Parse(char *input)
	{
	    char	   *keyword = get_buffer(PARSER_BUFFER_LENGTH);

	    while (*input /* && !Parser::error */) {
	    	input = Parser::Parse(input, keyword);

	    	if (!*keyword) break;

	    	if (!str_cmp(keyword, "script")) {
				input = Parser::ReadStringBlock(input, keyword);
				char *s = keyword;
				CmdlistElement *cle;
			
				trig->cmdlist = new Cmdlist(strtok(s, "\n\r"));
				cle = trig->cmdlist->command;

				while ((s = strtok(NULL, "\n\r"))) {
					cle->next = new CmdlistElement(s);
					cle = cle->next;
				}
	    	} else {
	    		// ERROR HANDLER - Unknown keyword
	    		Parser::Error("Unknown keyword \"%s\"", keyword);
	    	}
	    	while (*input && (*input != ';'))		   ++input;
	    	while (isspace(*input) || *input == ';')   ++input;
	    }
	    release_buffer(keyword);
	}


	void Concentrate::Parse(char *input)
	{
	    char	   *keyword = get_buffer(PARSER_BUFFER_LENGTH);

	    while (*input /* && !Parser::error */) {
	    	input = Parser::Parse(input, keyword);

	    	if (!*keyword) break;

	    	if (!str_cmp(keyword, "interval")) {
	    		input = Parser::GetKeyword(input, keyword);
	    		if (is_number(keyword))    interval = atoi(keyword);
			} else if (!str_cmp(keyword, "wearoff")) {
				input = Parser::ReadString(input, keyword);
				wearoff = str_dup(keyword);
			} else if (!str_cmp(keyword, "cost")) {
	    		input = Parser::GetKeyword(input, keyword);
	    		if (is_number(keyword))    cost = atoi(keyword);
			} else if (!str_cmp(keyword, "trig")) {
				input = Parser::ReadStringBlock(input, keyword);
				char *s = keyword;
				CmdlistElement *cle;
			
				trig = new Trigger;
			
				trig->cmdlist = new Cmdlist(strtok(s, "\n\r"));
				cle = trig->cmdlist->command;

				while ((s = strtok(NULL, "\n\r"))) {
					cle->next = new CmdlistElement(s);
					cle = cle->next;
				}
	    	} else {
	    		// ERROR HANDLER - Unknown keyword
	    		Parser::Error("Unknown keyword \"%s\"", keyword);
	    	}
	    	while (*input && (*input != ';'))		   ++input;
	    	while (isspace(*input) || *input == ';')   ++input;
	    }
	    release_buffer(keyword);
	}


	void Misfire::Parse(char *input)
	{
	    char	   *keyword = get_buffer(PARSER_BUFFER_LENGTH);

	    while (*input /* && !Parser::error */) {
	    	input = Parser::Parse(input, keyword);

	    	if (!*keyword) break;

	    	if (!str_cmp(keyword, "vnum")) {
	    		input = Parser::GetKeyword(input, keyword);
	    		if (is_number(keyword))    vnum = atoi(keyword);
	    		else   Parser::Error("Expected \"trigger = #\", got \"%s\"", keyword);
	    	} else {
	    		// ERROR HANDLER - Unknown keyword
	    		Parser::Error("Unknown keyword \"%s\"", keyword);
	    	}
	    	while (*input && (*input != ';'))		   ++input;
	    	while (isspace(*input) || *input == ';')   ++input;
	    }
	    release_buffer(keyword);
	}


	void Manual::Parse(char *input)
	{
	    char	   *keyword = get_buffer(PARSER_BUFFER_LENGTH);

	    while (*input /* && !Parser::error */) {
	    	input = Parser::Parse(input, keyword);

	    	if (!*keyword) break;

	    	if (!str_cmp(keyword, "routine")) {
				input = Parser::ReadString(input, keyword);
				if ((routine = search_block(keyword, ::ManualSpells::Names, false)) == -1) {
					routine = -1;	//	Default value
					Parser::Error("Bad routine \"%s\"", keyword);
				}
	    	} else {
	    		// ERROR HANDLER - Unknown keyword
	    		Parser::Error("Unknown keyword \"%s\"", keyword);
	    	}
	    	while (*input && (*input != ';'))		   ++input;
	    	while (isspace(*input) || *input == ';')   ++input;
	    }
	    release_buffer(keyword);
	}


}
