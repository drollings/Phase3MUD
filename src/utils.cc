/* ************************************************************************
*   File: utils.c                                       Part of CircleMUD *
*  Usage: various internal functions of a utility nature                  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */


#include "structs.h"
#include "utils.h"
#include "buffer.h"
#include "rooms.h"
#include "characters.h"
#include "objects.h"
#include "comm.h"
#include "db.h"
#include "extradesc.h"

Sunlight sunlight;

void tag_argument(char *argument, char *tag);

/* local functions */
struct TimeInfoData real_time_passed(time_t t2, time_t t1);
struct TimeInfoData mud_time_passed(time_t t2, time_t t1);
void DieFollower(Character * ch);
void add_follower(Character * ch, Character * leader);
int wear_message_number (Object *obj, int where);


/* creates a random number in interval [from;to] */
int Number(int from, int to)
{
	/* error checking in case people call Number() incorrectly */
	if (from > to) {
		int tmp = from;
		from = to;
		to = tmp;
//		log("SYSERR: Number() should be called with lowest, then highest. Number(%d, %d), not Number(%d, %d).", from, to, to, from);
	}

	return ((random() % (to - from + 1)) + from);
}


/* simulates dice roll */
int dice(int number, int size) {
	int sum = 0;

	if (size <= 0 || number <= 0)
		return 0;

	while (number-- > 0)
		sum += ((circle_random() % size) + 1);

	return sum;
}


/* Create a duplicate of a string */
char *str_dup(const char *source) {
	char *new_s;

	if (!source)
		return NULL;

	CREATE(new_s, char, strlen(source) + 1);
	return (strcpy(new_s, source));
}



/* str_cmp: a case-insensitive version of strcmp */
/* returns: 0 if equal, 1 if arg1 > arg2, -1 if arg1 < arg2  */
/* scan 'till found different or end of both                 */
int str_cmp(const char *arg1, const char *arg2) {
	int chk, i;

	for (i = 0; *(arg1 + i) || *(arg2 + i); i++)
		if ((chk = LOWER(*(arg1 + i)) - LOWER(*(arg2 + i)))) {
			if (chk < 0)	return -1;
			else			return 1;
		}
//	SInt32	chk;
//	while (*arg1 || *arg2)
//		if ((chk = LOWER(*(arg1++)) - LOWER(*(arg2++))))
//			if (chk < 0)	return -1;
//			else			return 1;
	return 0;
}


/* strn_cmp: a case-insensitive version of strncmp */
/* returns: 0 if equal, 1 if arg1 > arg2, -1 if arg1 < arg2  */
/* scan 'till found different, end of both, or n reached     */
int strn_cmp(const char *arg1, const char *arg2, int n) {
	int chk, i;
	for (i = 0; (*(arg1 + i) || *(arg2 + i)) && (n > 0); i++, n--)
		if ((chk = LOWER(*(arg1 + i)) - LOWER(*(arg2 + i)))) {
			if (chk < 0)	return -1;
			else			return 1;
		}
//	SInt32	chk;
//	while (*arg1 || *arg2)
//		if ((chk = LOWER(*(arg1++)) - LOWER(*(arg2++))))
//			if (chk < 0)	return -1;
//			else			return 1;
//		else if (n-- <= 0)
//			break;
	return 0;
}


/* Return pointer to first occurrence in string ct in */
/* cs, or NULL if not present.  Case insensitive */
const char *str_str(const char *cs, const char *ct) {
	const char *s, *t;

	if (!cs || !ct)
		return NULL;

	while (*cs) {
		t = ct;

		while (*cs && (LOWER(*cs) != LOWER(*t)))
			cs++;

		s = cs;

		while (*t && *cs && (LOWER(*cs) == LOWER(*t))) {
			t++;
			cs++;
		}

		if (!*t)
			return s;
	}

	return NULL;
}


bool is_substring(const char *sub, const char *string) {
	const char *s;

	if ((s = str_str(string, sub))) {
		int len = strlen(string);
		int sublen = strlen(sub);

		if ((s == string || isspace(*(s - 1)) || ispunct(*(s - 1))) &&	/* check front */
				((s + sublen == string + len) || isspace(s[sublen]) ||	/* check end */
				ispunct(s[sublen])))
			return true;
	}
	return false;
}




/*
 * p points to the first quote, returns the matching
 * end quote, or the last non-null char in p.
*/
char *matching_quote(char *p) {
	for (p++; *p && (*p != '"'); p++) {
		if (*p == '\\' /* && *(p+1) */)	p++;
	}
	if (!*p)	p--;
	return p;
}

/*
 * p points to the first paren.  returns a pointer to the
 * matching closing paren, or the last non-null char in p.
 */
char *matching_paren(char *p) {
	int i;
	for (p++, i = 1; *p && i; p++) {
		if (*p == '(')			i++;
		else if (*p == ')')		i--;
		else if (*p == '"')		p = matching_quote(p);
	}
	return --p;
}


//	Copy first phrase into first_arg, returns rest of string
char *one_phrase(char *arg, char *first_arg) {
	skip_spaces(arg);

	if (!*arg)	*first_arg = '\0';
	else if (*arg == '"') {
		char *p, c;

		p = matching_quote(arg);
		c = *p;
		*p = '\0';
		strcpy(first_arg, arg + 1);
		if (c == '\0')	return p;
		else			return p + 1;
	} else {
		char *s, *p;

		s = first_arg;
		p = arg;

		while (*p && !isspace(*p) && *p != '"')
			*s++ = *p++;

		*s = '\0';
		return p;
	}
	return arg;
}


/*
 * return 1 if str contains a word or phrase from wordlist.
 * phrases are in double quotes (").
 * if wrdlist is NULL, then return 1, if str is NULL, return 0.
 */
bool word_check(const char *str, const char *wordlist) {
	char *words, *phrase, *s;
	bool	value = false;

	if (*wordlist == '*') return true;

	words = get_buffer(MAX_INPUT_LENGTH);
	phrase = get_buffer(MAX_INPUT_LENGTH);

	strcpy(words, wordlist);

	for (s = one_phrase(words, phrase); *phrase; s = one_phrase(s, phrase)) {
		if (is_substring(phrase, str)) {
			value = true;
			break;
		}
	}

	release_buffer(words);
	release_buffer(phrase);

	return value;
}


/* log a death trap hit */
void log_death_trap(Character * ch) {
	mudlogf(BRF, ch, TRUE, "%s hit death trap #%d (%s)", ch->RealName(),
			world[IN_ROOM(ch)].Virtual(), world[IN_ROOM(ch)].Name("<ERROR>"));
}


/* the "touch" command, essentially. */
int touch(const char *path)
{
	FILE *fl;

	if (!(fl = fopen(path, "a"))) {
		perror(path);
		return -1;
	} else {
		fclose(fl);
		return 0;
	}
}


void sprintbit(UInt32 bitvector, const char *names[], char *result)
{
	long nr;

	*result = '\0';

	for (nr = 0; bitvector; bitvector >>= 1) {
		if (IS_SET(bitvector, 1)) {
			if (*names[nr] != '\n') {
	strcat(result, names[nr]);
	strcat(result, " ");
			} else
	strcat(result, "UNDEFINED ");
		}
		if (*names[nr] != '\n')
			nr++;
	}

	if (!*result)
		strcpy(result, "NOBITS ");
}



void sprinttype(int type, const char *names[], char *result)
{
	int nr = 0;

	while (type && *names[nr] != '\n') {
		type--;
		nr++;
	}

	if (*names[nr] != '\n')
		strcpy(result, names[nr]);
	else
		strcpy(result, "UNDEFINED");
}


/* Calculate the REAL time passed over the last t2-t1 centuries (secs) */
struct TimeInfoData real_time_passed(time_t t2, time_t t1)
{
	long secs;
	struct TimeInfoData now;

	secs = (long) (t2 - t1);

	now.hours = (secs / SECS_PER_REAL_HOUR) % 24;	/* 0..23 hours */
	secs -= SECS_PER_REAL_HOUR * now.hours;

	now.day = (secs / SECS_PER_REAL_DAY);	/* 0..29 days	*/
	secs -= SECS_PER_REAL_DAY * now.day;

	now.month = 0;
	now.year = 0;

	return now;
}



/* Calculate the MUD time passed over the last t2-t1 centuries (secs) */
struct TimeInfoData mud_time_passed(time_t t2, time_t t1)
{
	long secs;
	struct TimeInfoData now;

	secs = (long) (t2 - t1);

	now.hours = (secs / SECS_PER_MUD_HOUR) % 24;	/* 0..23 hours */
	secs -= SECS_PER_MUD_HOUR * now.hours;

	now.day = (secs / SECS_PER_MUD_DAY) % 30;	/* 0..29 days	*/
	secs -= SECS_PER_MUD_DAY * now.day;

	now.month = (secs / SECS_PER_MUD_MONTH) % 12;	/* 0..11 months */
	secs -= SECS_PER_MUD_MONTH * now.month;

	now.year = (secs / SECS_PER_MUD_YEAR);	/* 0..XX? years */

	return now;
}



struct TimeInfoData age(Character * ch)
{
	struct TimeInfoData player_age;

	player_age = mud_time_passed(time(0), ch->player->time.birth);

	player_age.year += 17;	/* All players start at 17 */

	return player_age;
}


/* Check if making CH follow VICTIM will create an illegal */
/* Follow "Loop/circle"																		*/
int circle_follow(Character * ch, Character * victim)
{
	Character *k;

	for (k = victim; k; k = k->master) {
		if (k == ch)
			return TRUE;
	}

	return FALSE;
}



/* Do NOT call this before having checked if a circle of followers */
/* will arise. CH will follow leader                               */
void add_follower(Character * ch, Character * leader) {
	if (ch->master) {
		core_dump();
		return;
	}

	ch->master = leader;

	leader->followers.Add(ch);

	act("You now follow $N.", FALSE, ch, 0, leader, TO_CHAR);
	if (ch->CanBeSeen(leader))
		act("$n starts following you.", TRUE, ch, 0, leader, TO_VICT);
	act("$n starts to follow $N.", TRUE, ch, 0, leader, TO_NOTVICT);
}

/*
 * get_line reads the next non-blank line off of the input stream.
 * The newline character is removed from the input.  Lines which begin
 * with '*' are considered to be comments.
 *
 * Returns the number of lines advanced in the file.
 */
int get_line(FILE * fl, char *buf) {
	char *temp = get_buffer(256);
	int lines = 0;

	do {
		lines++;
		fgets(temp, 256, fl);
		if (*temp)
			temp[strlen(temp) - 1] = '\0';
	} while (!feof(fl) && (!*temp));

	if (feof(fl)) {
		*buf = '\0';
		lines = 0;
	} else
		strcpy(buf, temp);

	release_buffer(temp);
	return lines;
}


int get_filename(const char *orig_name, char *filename) {
	char	*ptr, *name;

	if (!*orig_name)
		return 0;

	name = get_buffer(64);
	strcpy(name, orig_name);
	for (ptr = name; *ptr; ptr++)
		*ptr = LOWER(*ptr);

	sprintf(filename, PLR_PREFIX "%c" SEPERATOR "%s.objs", *name, name);

	release_buffer(name);
	return 1;
}


int num_pc_in_room(Room *room) {
	int i = 0;
	Character *ch;

	START_ITER(iter, ch, Character *, room->people)
		if (!IS_NPC(ch))
			i++;

	return i;
}


/* string manipulation fucntion originally by Darren Wilson */
/* (wilson@shark.cc.cc.ca.us) improved and bug fixed by Chris (zero@cnw.com) */
/* completely re-written again by M. Scott 10/15/96 (scottm@workcommn.net), */
/* substitute appearances of 'pattern' with 'replacement' in string */
/* and return the # of replacements */
int replace_str(char **string, char *pattern, char *replacement, int rep_all,
		StrLenInt max_size) {
   char *replace_buffer = NULL;
   char *flow, *jetsam, temp;
   int len, i;

   if ((strlen(*string) - strlen(pattern)) + strlen(replacement) > max_size)
     return -1;

   CREATE(replace_buffer, char, max_size);
   i = 0;
   jetsam = *string;
   flow = *string;
   *replace_buffer = '\0';
   if (rep_all) {
      while ((flow = strstr(flow, pattern)) != NULL) {
	 i++;
	 temp = *flow;
	 *flow = '\0';
	 if ((strlen(replace_buffer) + strlen(jetsam) + strlen(replacement)) > max_size) {
	    i = -1;
	    break;
	 }
	 strcat(replace_buffer, jetsam);
	 strcat(replace_buffer, replacement);
	 *flow = temp;
	 flow += strlen(pattern);
	 jetsam = flow;
      }
      strcat(replace_buffer, jetsam);
   }
   else {
      if ((flow = strstr(*string, pattern)) != NULL) {
	 i++;
	 flow += strlen(pattern);
	 len = (flow - *string) - strlen(pattern);

	 strncpy(replace_buffer, *string, len);
	 replace_buffer[len] = '\0';
	 strcat(replace_buffer, replacement);
	 strcat(replace_buffer, flow);
      }
   }
   if (i == 0) return 0;
   if (i > 0) {
      RECREATE(*string, char, strlen(replace_buffer) + 3);
      strcpy(*string, replace_buffer);
   }
   FREE(replace_buffer);
   return i;
}


void Free_Error(char * file, int line) {
	mudlogf( BRF, NULL, TRUE,  "CODEERR: NULL pointer in file %s:%d", file, line);
}


/*thanks to Luis Carvalho for sending this my way..it's probably a
  lot shorter than the one I would have made :)  */
void sprintbits(UInt32 vektor, char *outstring) {
	const char * const flags="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

	*outstring = '\0';
	for (UInt32 i = 0; i < 32; i++) {
		if (vektor & 1) {
			*outstring = flags[i];
			outstring++;
		}
		vektor >>= 1;
	}
	*outstring = 0;
}


void tag_argument(char *argument, char *tag)
{
  char *tmp = argument, *ttag = tag, *wrt = argument;
  int i;

  for(i = 0; i < 4; i++)
    *(ttag++) = *(tmp++);

  *ttag = '\0';

  while(*tmp == ' ')	++tmp;
  while(*tmp == ':')	++tmp;
  while(*tmp == ' ')	++tmp;

  while(*tmp)
    *(wrt++) = *(tmp++);

  *wrt = '\0';
}


/* returns the message number to display for where[] and equipment_types[] */
int wear_message_number (Object *obj, int where) {
	if (!obj) {
		log("SYSERR:  NULL passed for obj to wear_message_number");
		return where;
	}
	if (where < WEAR_HAND_R)						return where;
	else if (OBJ_FLAGGED(obj, ITEM_TWO_HAND)) {
		if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)		return POS_WIELD_TWO;
		else										return POS_HOLD_TWO;
	} else if (GET_OBJ_TYPE(obj) == ITEM_WEAPON) {
		if (where == WEAR_HAND_R)					return POS_WIELD;
		else										return POS_WIELD_OFF;
	} else if (GET_OBJ_TYPE(obj) == ITEM_LIGHT)		return POS_LIGHT;
	else											return POS_HOLD;
}
/*

RNum real_number(VNum vnum, IndexData *the_index, int the_top);
RNum real_number(VNum vnum, IndexData *the_index, int the_top) {
	int bot, top, mid, nr, found = -1;

	// First binary search.
	bot = 0;
	top = the_top;

	for (;;) {
		mid = (bot + top) >> 1;

		if (mid == -1) {
			log("SYSERR: real_number mid == -1!  (vnum == %d, top == %d)", vnum, the_top);
			break;
		}
		if ((the_index + mid)->vnum == vnum)
			return mid;
		if (bot >= top)
			break;
		if ((the_index + mid)->vnum > vnum)
			top = mid - 1;
		else
			bot = mid + 1;
	}

	// If not found - use linear on the "new" parts.
	for(nr = 0; nr <= the_top; nr++) {
		if(the_index[nr].vnum == vnum) {
			found = nr;
			break;
		}
	}
	return(found);
}
*/

/* string prefix routine */
int str_prefix(const char *astr, const char *bstr) {
	if (!astr) {
		log("SYSERR: str_prefix: null astr.");
		return TRUE;
	}
	if (!bstr) {
		log("SYSERR: str_prefix: null bstr.");
		return TRUE;
	}
	for(; *astr; astr++, bstr++) {
		if(LOWER(*astr) != LOWER(*bstr)) return TRUE;
	}
	return FALSE;
}


/*
 * This function (derived from basic fork(); abort(); idea by Erwin S.
 * Andreasen) causes your MUD to dump core (assuming you can) but
 * continue running.  The core dump will allow post-mortem debugging
 * that is less severe than assert();  Don't call this directly as
 * core_dump_unix() but as simply 'core_dump()' so that it will be
 * excluded from systems not supporting them. (e.g. Windows '95).
 *
 * XXX: Wonder if flushing streams includes sockets?
 */
void core_dump_func(const char *who, SInt16 line) {
	mudlogf(BRF, NULL, true, "SYSERR: Assertion failed at %s:%d!", who, line);

#ifndef macintosh
	//	These would be duplicated otherwise...
	fflush(stdout);
	fflush(stderr);
	fflush(logfile);

	//	 Kill the child so the debugger or script doesn't think the MUD
	//	crashed.  The 'autorun' script would otherwise run it again.
	if (!fork())
		abort();
#endif
}


Character *desc_orig(Character *ch)
{
	if ((ch->desc) && (ch->desc->original))	return (ch->desc->original);
	else	return (ch);
}


char * grab_adesc_line(char *from, char *to)
{
	while (*from && ((*from == '\r') || (*from == '\n') || (*from == ' ')))
		++from;

	while (*from && (*from != '|')) {
		if ((*from != '\r') && (*from != '\n'))
			*(to++) = *from;
		++from;
	}

	*to = '\0';
	return from;
}


// Warning:	This dirties buf3. -DH
void adesc_lines(char *fromstring, char *line1, char *line2 = NULL,
								 char *line3 = NULL, char *line4 = NULL)
{
	register char *from = NULL, *to = NULL;
	char *buffer = get_buffer(1024);

	*buffer = '\0';
	strcpy(buffer, fromstring);

	from = buffer;
	to = line1;

	if ((to = line1)) {
		from = grab_adesc_line(from, to);
		if (*from)
			++from;
	}
	if ((to = line2)) {
		from = grab_adesc_line(from, to);
		if (*from)
			++from;
	}
	if ((to = line3)) {
		from = grab_adesc_line(from, to);
		if (*from)
			++from;
	}
	if ((to = line4)) {
		from = grab_adesc_line(from, to);
	}

}


char *money_desc(int amount)
{
	static char buf[128];

	if (amount <= 0) {
		log("SYSERR: Try to create negative or 0 money (%d).", amount);
		return NULL;
	}
	if (amount == 1)
		strcpy(buf, "a gold coin");
	else if (amount <= 10)
		strcpy(buf, "a tiny pile of gold coins");
	else if (amount <= 20)
		strcpy(buf, "a handful of gold coins");
	else if (amount <= 75)
		strcpy(buf, "a little pile of gold coins");
	else if (amount <= 200)
		strcpy(buf, "a small pile of gold coins");
	else if (amount <= 1000)
		strcpy(buf, "a pile of gold coins");
	else if (amount <= 5000)
		strcpy(buf, "a big pile of gold coins");
	else if (amount <= 10000)
		strcpy(buf, "a large heap of gold coins");
	else if (amount <= 20000)
		strcpy(buf, "a huge mound of gold coins");
	else if (amount <= 75000)
		strcpy(buf, "an enormous mound of gold coins");
	else if (amount <= 150000)
		strcpy(buf, "a small mountain of gold coins");
	else if (amount <= 250000)
		strcpy(buf, "a mountain of gold coins");
	else if (amount <= 500000)
		strcpy(buf, "a huge mountain of gold coins");
	else if (amount <= 1000000)
		strcpy(buf, "an enormous mountain of gold coins");
	else
		strcpy(buf, "an absolutely colossal mountain of gold coins");

	return buf;
}


Object *create_money(int amount)
{
	Object *obj;
	ExtraDesc *new_descr;
	char *buf;

	if (amount <= 0) {
		log("SYSERR: Try to create negative or 0 money (%d).", amount);
		return NULL;
	}
	obj = Object::Create();
	new_descr = new ExtraDesc;
	buf = get_buffer(100);

	if (amount == 1) {
		obj->SetKeywords("coin gold");
		obj->SetName("a gold coin");
		obj->SetSDesc("One miserable gold coin is lying here.");
		new_descr->keyword = SString::Create("coin gold");
		new_descr->description = SString::Create("It's just one miserable little gold coin.");
	} else {
		obj->SetKeywords("coins gold");
		obj->SetName(money_desc(amount));
		sprintf(buf, "%s is lying here.", money_desc(amount));
		obj->SetSDesc(CAP(buf));

		new_descr->keyword = SString::Create("coins gold");
		if (amount < 10) {
			sprintf(buf, "There are %d coins.", amount);
			new_descr->description = SString::Create(buf);
		} else if (amount < 100) {
			sprintf(buf, "There are about %d coins.", 10 * (amount / 10));
			new_descr->description = SString::Create(buf);
		} else if (amount < 1000) {
			sprintf(buf, "It looks to be about %d coins.", 100 * (amount / 100));
			new_descr->description = SString::Create(buf);
		} else if (amount < 100000) {
			sprintf(buf, "You guess there are, maybe, %d coins.",
				1000 * ((amount / 1000) + Number(0, (amount / 1000))));
			new_descr->description = SString::Create(buf);
		} else
			new_descr->description = SString::Create("There are a LOT of coins.");
	}

	obj->exDesc.Append(new_descr);

	GET_OBJ_TYPE(obj) = ITEM_MONEY;
	GET_OBJ_WEAR(obj) = ITEM_WEAR_TAKE;
	GET_OBJ_VAL(obj, 0) = amount;
	GET_OBJ_COST(obj) = amount;
	// obj->item_number = NOTHING;

	release_buffer(buf);
	return obj;
}


// This function split off from say_spell
// This would be an ideal place for a real String class
void garble_text(char * origtext, char * newtext, struct syllable *syllables,
                 StrLenInt len, SInt8 howmuch)
{
	char *output = get_buffer(MAX_STRING_LENGTH);

	// String output = "";

	int i = 0, j = 0;
	unsigned int ofs = 0;

	*output = '\0';
	if (len > MAX_STRING_LENGTH)
		len = MAX_STRING_LENGTH;

	howmuch = MIN(MAX(-10, howmuch), 0);

	while (*(origtext + ofs) && (ofs < len)) {
		if ((howmuch == 10) || (++j % howmuch)) {
			for (i = 0; *(syllables[i].org); i++) {
			 	if (!strncmp(syllables[i].org, origtext + ofs, strlen(syllables[i].org))) {
			 		strcat(output, syllables[i].news);
			 		ofs += strlen(syllables[i].org);
		 		}
			}
		}
	}

	if (strlen(output) > len) {
//		output.del((int) len - 3, output.length() - len + 3);
//		output += "...";
		output[len - 4] = '\0';
		strcat(output, "...");
	}

	strcpy(newtext, output);

	release_buffer(output);
}


bool same_alignment(Character *ch, Character *vict)
{
	if (ch == NULL || vict == NULL)
		return FALSE;

	if (IS_EVIL(ch) && IS_EVIL(vict))
		return TRUE;
	if (IS_NEUTRAL(ch) && IS_NEUTRAL(vict))
		return TRUE;
	if (IS_GOOD(ch) && IS_GOOD(vict))
		return TRUE;

	return FALSE;
}


bool opposed_alignment(Character *ch, Character *vict)
{
	if (ch == NULL || vict == NULL)
		return FALSE;

	if (IS_EVIL(ch) && IS_GOOD(vict))
		return TRUE;
	if (IS_GOOD(ch) && IS_EVIL(vict))
		return TRUE;

	return FALSE;
}


GameData::~GameData(void)
{
	Affect *tmp_affect;

	SSFree(name);

	START_ITER(iter, tmp_affect, Affect *, affected) {
		delete tmp_affect;
		affected.Remove(tmp_affect);
	}
}


void		MUDObject::SetRoom(VNum theRoom)
{
	room = theRoom; inside = &world[theRoom];
}


const char *	MUDObject::CalledName(const Character *ch) const
{
	if (!IS_NPC(ch)) {
		if (ch->player->CalledNames.Find(GET_ID(this))) {
			return (ch->player->CalledNames[GET_ID(this)].Value());
		}
	}
	return Name();
}


const char *	Character::CalledName(const Character *ch) const
{
	if (!IS_NPC(ch)) {
		if (ch->player->CalledNames.Find(GET_ID(this))) {
			return (ch->player->CalledNames[GET_ID(this)].Value());
		}
	}
	if (!IS_NPC(this) && (this != ch) && !IS_STAFF(ch)) {
		return SDesc();
	} else {
		return Name();
	}
}


const char *	MUDObject::CalledKeywords(const Character *ch) const
{
	if (!IS_NPC(ch)) {
		if (ch->player->CalledNames.Find(GET_ID(this))) {
			return (ch->player->CalledNames[GET_ID(this)].Value());
		}
	}
	return Keywords();
}


const char *	Character::CalledKeywords(const Character *ch) const
{
	if (!IS_NPC(ch)) {
		if (ch == this)		return Name();
		if (ch->player->CalledNames.Find(GET_ID(this))) {
			return (ch->player->CalledNames[GET_ID(this)].Value());
		}
	}
	if (!IS_NPC(this) && IS_STAFF(ch)) {
		return Name();
	}
	return Keywords();
}


CallName::~CallName(void) {
	if (string)	{
		delete string;
		string = NULL;
	}
}


int strip_last_char(char *line, char letter)
{
	char *pos = line + strlen(line);

	while (pos >= line) {
		if (*pos == letter) {
			*pos = '\0';
			return 1;
		}
		--pos;
	}

	return 0;
}


const char * 	Scriptable::UID(void) const
{
	static char buffer[20];

	sprintf(buffer, "%c%d", UID_CHAR, ID());

    return buffer;
}


bool	MUDObject::SameRoom(MUDObject * other) const
{
	return	(inside == other->inside);
}
