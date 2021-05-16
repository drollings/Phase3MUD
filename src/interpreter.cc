/* ************************************************************************
*   File: interpreter.c                                 Part of CircleMUD *
*  Usage: parse user commands, search for specials, call ACMD functions   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "structs.h"
#include "alias.h"
#include "utils.h"
#include "descriptors.h"
#include "rooms.h"
#include "characters.h"
#include "objects.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "handler.h"
#include "mail.h"
#include "olc.h"
#include "ban.h"
#include "buffer.h"
#include "constants.h"
#include "skills.h"

#include "editor.h"

#if 0
#define EMAIL_PASSWORD
#endif

extern char *motd;
extern char *imotd;
extern char *background;
extern Vector<PlayerIndex> player_table;
extern SInt32 circle_restrict;
extern int no_specials;
extern int max_bad_pws;
extern UInt8 stat_max[NUM_RACES][5];
extern int raceTclass[NUM_RACES][NUM_CLASSES];

/* external functions */
void do_start(Character *ch);
int parse_race(char *arg);
int create_entry(char *name);
bool special(Character *ch, const char *cmd, char *arg);
void roll_real_abils(Character * ch);
int selfcommand_mtrigger(Character *ch, char *cmd, char *argument);
int command_mtrigger(Character *actor, char *cmd, char *argument);
int command_otrigger(Character *actor, char *cmd, char *argument);
int command_wtrigger(Character *actor, char *cmd, char *argument);

void start_otrigger(Object *obj);
void start_otrigger(LList<Object *> &objList);

extern void cg_edit_advantage(Character *ch, char *argument);
extern void cg_edit_disadvantage(Character *ch, char *argument);
extern void display_classes(Character *ch);
extern void display_races(Character * ch);
extern char *cg_info(Character *ch);
extern int parse_race_menu(char arg);

extern bool script_command_interpreter(Scriptable * go, char *argument, int cmd, VNum trigger);


/* local functions */
Alias *find_alias(LList<Alias *> &alias_list, char *str);
void perform_complex_alias(struct txt_q *input_q, char *orig, Alias *a);
int perform_alias(Descriptor *d, char *orig);
bool reserved_word(const char *argument);
int find_name(const char *name);
int _parse_name(char *arg, char *name);
int perform_dupe_check(Descriptor *d);
int enter_player_game (Descriptor *d);

void email_password(Character *ch);


const char *fillers[] = {
	"in"	,
	"from",
	"with",
	"the"	,
	"on"	,
	"at"	,
	"to"	,
	"\n"
};

const char *reserved[] = {
	"a",
	"an",
	"self",
	"me",
	"all",
	"room",
	"someone",
	"something",
	"\n"
};


// Madmax's command logger for troublesome people
void log_cmds(Character *ch, char *argument)
{
	FILE *fl;
	char *tmp;
	time_t ct;

	sprintf(buf, "watch/%s.cmd", GET_NAME(ch));

	ct = time(0);
	tmp = asctime(localtime(&ct));

	skip_spaces(argument);
	delete_doubledollar(argument);

	if (!(fl = fopen(buf, "a"))) {
		perror("log_cmd");
		return;
	}

	*(tmp + strlen(tmp) - 1) = '\0';

	fprintf(fl, "%-19.19s :: %s\n", tmp + 4, argument);
	fclose(fl);
}


#undef CMD_IS
#define CMD_IS(cmd_name)	(!str_cmp(arg, cmd_name))
/*
 * This is the actual command interpreter called from game_loop() in comm.c
 * It makes sure you are the proper level and position to execute the command,
 * then calls the appropriate function.
 */
void command_interpreter(Character *ch, char *argument, int cmd = 0) {
    StrLenInt length;
	char *line, *arg;

	skip_spaces(argument);

	/*
	 * just drop to next line for hitting CR,
	 * or skip if character has been extracted
	 */
	if (!*argument || PURGED(ch))
		return;

	if (PLR_FLAGGED(ch, PLR_WATCHED))
		log_cmds(ch, argument);

	if (IN_ROOM(ch) == NOWHERE) {
		log("CRITICAL ERROR:  %s in NOWHERE and issuing command:  %s!", ch->Name(), argument);
		return;
	}

// CHANGEPOINTS

// 	/* check if this is an olc command */
// 	if ((!IS_NPC(ch)) && (GET_OLC_MODE(ch))) {
// 		if (olc_interpreter(ch, argument))
// 			return;
// 	}

 	if (!cmd && IS_NPC(ch) && (!(ch)->desc || GET_TRUST((ch)->desc->original) >= TRUST_IMPL) && !AFF_FLAGGED(ch, AffectBit::Charm))

		if (script_command_interpreter((Scriptable *) ch, argument, -1, -1))
			return;

	/*
	 * special case to handle one-character, non-alphanumeric commands;
	 * requested by many people so "'hi" or ";godnet test" is possible.
	 * Patch sent by Eric Green and Stefan Wasilewski.
	 */
	arg = get_buffer(MAX_INPUT_LENGTH);
	if (!isalpha(*argument)) {
		arg[0] = argument[0];
		arg[1] = '\0';
		line = argument + 1;
	} else
		line = any_one_arg(argument, arg);

	if (selfcommand_mtrigger(ch, arg, line) || command_wtrigger(ch, arg, line) || 
		command_mtrigger(ch, arg, line) || command_otrigger(ch, arg, line)) {
		release_buffer(arg);
		return; /* command trigger took over */
	}

	/* find the command */
	if (cmd <= 0) {
		cmd = 0;

		for (length = strlen(arg), cmd = 0; *complete_cmd_info[cmd].command != '\n'; ++cmd)
			if (!strncmp(complete_cmd_info[cmd].command, arg, length) &&
				(length >= strlen(complete_cmd_info[cmd].sort_as)))
				if ((GET_LEVEL(ch) >= complete_cmd_info[cmd].minimum_level) &&
					(!IS_STAFFCMD(cmd) || (complete_cmd_info[cmd].staffcmd & STF_FLAGS(ch))) &&
					(!IS_NPCCMD(cmd) || IS_NPC(ch)))
				break;
	}

	if (!no_specials && special(ch, complete_cmd_info[cmd].command, line)) {
		release_buffer(arg);
		return;
	}

	if (*complete_cmd_info[cmd].command == '\n') {
			ch->Send("Huh?!?\r\n");
	} else if (PLR_FLAGGED(ch, PLR_FROZEN) && !STF_FLAGGED(ch, Staff::Coder))
		ch->Send("You try, but the mind-numbing cold prevents you...\r\n");
	else if (!complete_cmd_info[cmd].command_pointer)
		ch->Send("Sorry, that command hasn't been implemented yet.\r\n");
	else if ((!ch->desc || ch->desc->original) && IS_STAFFCMD(cmd))
		ch->Send("You can't use staff commands while switched.\r\n");

	else if (!((1 << GET_POS(ch)) & complete_cmd_info[cmd].position))
		switch (GET_POS(ch)) {
		case POS_DEAD:
			ch->Send("Lie still; you are DEAD!!! :-(\r\n");
			break;
		case POS_INCAP:
		case POS_MORTALLYW:
			ch->Send("You are in a pretty bad shape, unable to do anything!\r\n");
			break;
		case POS_STUNNED:
			ch->Send("All you can do right now is think about the stars!\r\n");
			break;
		case POS_SLEEPING:
			ch->Send("In your dreams, or what?\r\n");
			break;
		case POS_RESTING:
			ch->Send("Nah... You feel too relaxed to do that..\r\n");
			break;
		case POS_SITTING:
			ch->Send("Maybe you should get on your feet first?\r\n");
			break;
		case POS_STANDING:
			ch->Send("You can't do this while standing.\r\n");
			break;
		default:
			break;
		}
	else if (!((1 << POS_FIGHTING) & complete_cmd_info[cmd].position) && FIGHTING(ch))
		ch->Send("No way!  You're fighting for your life!\r\n");
	else if (!((1 << POS_RIDING) & complete_cmd_info[cmd].position) && RIDING(ch))
		ch->Send("Maybe you should dismount first.\r\n");
	else if (((1 << POS_AUDIBLE) & complete_cmd_info[cmd].position) &&
		 ((AFF_FLAGGED(ch, AffectBit::Silence)) || (IN_ROOM(ch) == NOWHERE) ||
		 (SECT(IN_ROOM(ch)) == SECT_VACUUM)))
		ch->Send("You can't make a sound!\r\n");
	else if ((complete_cmd_info[cmd].position != P_ANY) &&
			((1 << POS_MOBILE) & complete_cmd_info[cmd].position) &&
			AFF_FLAGGED(ch, AffectBit::Immobilized))
		ch->Send("You are unable to move!\r\n");
	else {
		if (PLR_FLAGGED(ch, PLR_AFK)) {
			if (!(IS_STAFF(ch) && STF_FLAGGED(ch, Staff::Security | Staff::Admin))) {
				if (!CMD_IS("afk") && !CMD_IS("think") && !CMD_IS(";")) {
					free(GET_AFK(ch));
					GET_AFK(ch) = NULL;
					REMOVE_BIT(PLR_FLAGS(ch), PLR_AFK);
			        act("$n just returned to this world.", TRUE, ch, 0, 0, TO_ROOM);
			        act("Ok, I guess that means you're back.", FALSE, ch, 0, 0, TO_CHAR);
				}
			}
		}

		// This follows the theory that if the command is one that can be performed in any
        // position, it's not something that should disrupt the character's activities.
		if (complete_cmd_info[cmd].position != P_ANY) {
			REMOVE_BIT(AFF_FLAGS(ch), AffectBit::Hide);

			// Remove actions
			if (GET_ACTION(ch)) {
				GET_ACTION(ch)->Cancel();
				ch->Send("You break off your activity.\r\n");
			}
		}

		// We need to check again as something might have happened with the
		// special check. -DH
		if (IN_ROOM(ch) == NOWHERE) {
			log("CRITICAL ERROR:	%s in NOWHERE and issuing command:	%s!", ch->Name(), argument);
			release_buffer(arg);
			return;
		}

		(*complete_cmd_info[cmd].command_pointer) (ch, line, cmd, arg, complete_cmd_info[cmd].subcmd);
	}

	release_buffer(arg);
}
#undef CMD_IS
#define CMD_IS(cmd_name)	(!str_cmp(cmd, cmd_name))


/**************************************************************************
 * Routines to handle aliasing                                             *
  **************************************************************************/


Alias *find_alias(LList<Alias *> &alias_list, char *str) {
//	while (alias_list) {
	Alias *a;
	START_ITER(iter, a, Alias *, alias_list)
		if (!strcmp(str, a->command))
				return a;
	return NULL;
}


Alias::Alias(char *arg, char *repl) : command(str_dup(arg)), replacement(str_dup(repl)){
//	command = str_dup(arg);
//	replacement = str_dup(repl);
	type = (strchr(repl, ALIAS_SEP_CHAR) || strchr(repl, ALIAS_VAR_CHAR)) ?
		ALIAS_COMPLEX : ALIAS_SIMPLE;
}


Alias::~Alias(void) {
	if (command)		free(command);
	if (replacement)	free(replacement);
}


/* The interface to the outside world: do_alias */
ACMD(do_alias) {
	char *repl, *arg;
	Alias *a;

	if (IS_NPC(ch))
		return;

	arg = get_buffer(MAX_INPUT_LENGTH);
	repl = any_one_arg(argument, arg);

	if (!*arg) {			/* no argument specified -- list currently defined aliases */
		ch->Send("Currently defined aliases:\r\n");
		if (GET_ALIASES(ch).Count() == 0)
			ch->Send(" None.\r\n");
		else {
			START_ITER(iter, a, Alias *, GET_ALIASES(ch))
				ch->Sendf("%-15s %s\r\n", a->command, a->replacement);
		}
	} else {			/* otherwise, add or remove aliases */
		/* is this an alias we've already defined? */
		if ((a = find_alias(GET_ALIASES(ch), arg)) != NULL) {
			GET_ALIASES(ch).Remove(a);
			delete a;
		}
		/* if no replacement string is specified, assume we want to delete */
		if (!*repl) {
			if (!a)	ch->Send("No such alias.\r\n");
			else	ch->Send("Alias deleted.\r\n");
		} else  if (!str_cmp(arg, "alias"))	/* otherwise, either add or redefine an alias */
			ch->Send("You can't alias 'alias'.\r\n");
		else {
			delete_doubledollar(repl);
			a = new Alias(arg, repl);
			GET_ALIASES(ch).Add(a);
			ch->Send("Alias added.\r\n");
		}
	}
	release_buffer(arg);
}

/*
 * Valid numeric replacements are only $1 .. $9 (makes parsing a little
 * easier, and it's not that much of a limitation anyway.)  Also valid
 * is "$*", which stands for the entire original line after the alias.
 * ";" is used to delimit commands.
 */
#define NUM_TOKENS       9

void perform_complex_alias(struct txt_q *input_q, char *orig, Alias *a) {
	struct txt_q temp_queue;
	char *tokens[NUM_TOKENS], *temp, *write_point,
		*buf2 = get_buffer(MAX_STRING_LENGTH),
		*buf = get_buffer(MAX_STRING_LENGTH);
	int num_of_tokens = 0, num;

	/* First, parse the original string */
	temp = strtok(strcpy(buf2, orig), " ");
	while (temp && (num_of_tokens < NUM_TOKENS)) {
		tokens[num_of_tokens++] = temp;
		temp = strtok(NULL, " ");
	}

	/* initialize */
	write_point = buf;
	temp_queue.head = temp_queue.tail = NULL;

	/* now parse the alias */
	for (temp = a->replacement; *temp; temp++) {
		if (*temp == ALIAS_SEP_CHAR) {
			*write_point = '\0';
			buf[MAX_INPUT_LENGTH - 1] = '\0';
			write_to_q(buf, &temp_queue, 1);
			write_point = buf;
		} else if (*temp == ALIAS_VAR_CHAR) {
			temp++;
			if ((num = *temp - '1') < num_of_tokens && num >= 0) {
				strcpy(write_point, tokens[num]);
				write_point += strlen(tokens[num]);
			} else if (*temp == ALIAS_GLOB_CHAR) {
				strcpy(write_point, orig);
				write_point += strlen(orig);
			} else if ((*(write_point++) = *temp) == '$')	/* redouble $ for act safety */
				*(write_point++) = '$';
		} else
			*(write_point++) = *temp;
	}

	*write_point = '\0';
	buf[MAX_INPUT_LENGTH - 1] = '\0';
	write_to_q(buf, &temp_queue, 1);

	/* push our temp_queue on to the _front_ of the input queue */
	if (input_q->head) {
		temp_queue.tail->next = input_q->head;
		input_q->head = temp_queue.head;
	} else
		*input_q = temp_queue;

	release_buffer(buf);
	release_buffer(buf2);
}


/*
 * Given a character and a string, perform alias replacement on it.
 *
 * Return values:
 *   0: String was modified in place; call command_interpreter immediately.
 *   1: String was _not_ modified in place; rather, the expanded aliases
 *      have been placed at the front of the character's input queue.
 */
int perform_alias(Descriptor *d, char *orig) {
	char *first_arg, *ptr;
	Alias *a;

	/* bail out immediately if the guy doesn't have any aliases */
	if (IS_NPC(d->character) || !GET_ALIASES(d->character).Count())
		return 0;

	first_arg = get_buffer(MAX_INPUT_LENGTH);

	/* find the alias we're supposed to match */
	ptr = any_one_arg(orig, first_arg);

	/* bail out if it's null */
	if (!*first_arg) {
		release_buffer(first_arg);
		return 0;
	}

	/* if the first arg is not an alias, return without doing anything */
	if (!(a = find_alias(GET_ALIASES(d->character), first_arg))) {
		release_buffer(first_arg);
		return 0;
	}
	release_buffer(first_arg);

	if (a->type == ALIAS_SIMPLE) {
		strcpy(orig, a->replacement);
		return 0;
	} else {
		perform_complex_alias(&d->input, ptr, a);
		return 1;
	}
}



/***************************************************************************
 * Various other parsing utilities                                         *
 **************************************************************************/

/*
 * searches an array of strings for a target string.  "exact" can be
 * 0 or non-0, depending on whether or not the match must be exact for
 * it to be returned.  Returns -1 if not found; 0..n otherwise.  Array
 * must be terminated with a '\n' so it knows to stop searching.
 */
int search_block(const char *arg, const char **list, bool exact) {
	SInt32		i, l;

	//	Make into lower case, and get length of string
//	for (l = 0; *(arg + l); l++)
//		*(arg + l) = LOWER(*(arg + l));
	l = strlen(arg);

	if (exact) {
		for (i = 0; **(list + i) != '\n'; i++)
			if (!str_cmp(arg, *(list + i)))
				return (i);
	} else {
		if (!l)
			l = 1;	//	Avoid "" to match the first available string
		for (i = 0; **(list + i) != '\n'; i++)
			if (!strn_cmp(arg, *(list + i), l))
				return (i);
	}

	return -1;
}


int search_chars(char arg, const char *list) {
	int i;

	for (i = 0; list[i] != '\n'; i++)
		if (arg == list[i])
			return (i);
	return -1;
}


/*
bool is_number(char *str) {
	if (!str || !*str)		return false;
	if (*str == '-')		str++;
	while (*str)
		if (!isdigit(*(str++)))
			return false;
	return true;
}
*/
bool is_number(const char *str) {
	if (!str || !*str)				return false;
	if (*str == '-' || *str == '+')	str++;
	while (*str && isdigit(*str))	str++;
	if (!*str || isspace(*str))		return true;
	else							return false;
}


void skip_spaces(const char *&string) {
	while (*string && isspace(*string))	string++;
//	for (; *string && isspace(*string); string++);
}


char *delete_doubledollar(char *string) {
	char *read, *write;

	if (!(write = strchr(string, '$')))
		return string;

	read = write;

	while (*read)
		if ((*(write++) = *(read++)) == '$')
			if (*read == '$')
				read++;

	*write = '\0';

	return string;
}


bool fill_word(const char *argument) {
	return (search_block(argument, fillers, TRUE) >= 0);
}


bool reserved_word(const char *argument) {
	return (search_block(argument, reserved, TRUE) >= 0);
}


/*
 * copy the first non-fill-word, space-delimited argument of 'argument'
 * to 'first_arg'; return a pointer to the remainder of the string.
 */
char *one_argument(const char *argument, char *first_arg) {
	char *begin = first_arg;

	if (!argument) {
		log("SYSERR: one_argument received a NULL pointer!");
		core_dump();
		*first_arg = '\0';
		return NULL;
	}

	do {
		skip_spaces(argument);

		first_arg = begin;
		while (*argument && !isspace(*argument)) {
			*(first_arg++) = LOWER(*argument);
			argument++;
		}

		*first_arg = '\0';
	} while (fill_word(begin));

	skip_spaces(argument);

	return const_cast<char *>(argument);
}


/*
 * one_word is like one_argument, except that words in quotes ("") are
 * considered one word.
 */
char *one_word(const char *argument, char *first_arg) {
	char *begin = first_arg;

	if (!argument) {
		log("SYSERR: one_word received a NULL pointer.");
		*first_arg = '\0';
		return NULL;
	}

	do {
		skip_spaces(argument);

		first_arg = begin;

		if (*argument == '\"') {
			argument++;
			while (*argument && *argument != '\"') {
				*(first_arg++) = LOWER(*argument);
				argument++;
			}
			argument++;
		} else {
			while (*argument && !isspace(*argument)) {
				*(first_arg++) = LOWER(*argument);
				argument++;
			}
		}

		*first_arg = '\0';
	} while (fill_word(begin));

	skip_spaces(argument);

	return const_cast<char *>(argument);
}


/* same as one_argument except that it doesn't ignore fill words */
char *any_one_arg(const char *argument, char *first_arg) {
	skip_spaces(argument);

	while (*argument && !isspace(*argument)) {
		*(first_arg++) = LOWER(*argument);
		argument++;
	}

	*first_arg = '\0';

	skip_spaces(argument);

	return const_cast<char *>(argument);
}



/* same as any_one_arg except that it stops at punctuation */
char *any_one_name(const char *argument, char *first_arg) {
	char* arg;

    /* Find first non blank */
//	while(isspace(*argument))
//		argument++;
	skip_spaces(argument);

	//	Find length of first word
	for(arg = first_arg ;
			*argument && !isspace(*argument) &&
			(!ispunct(*argument) || *argument == '#' || *argument == '-');
			arg++, argument++) {
		*arg = LOWER(*argument);
	}
	*arg = '\0';

	// I took this out to fix a problem with the emote codes not having a space after them.
	// I'm not sure how this will impact other bits of code. -DH
	// Second note:  This is only used by emote and act, and seems fine. -DH
	// skip_spaces(argument);

	return const_cast<char *>(argument);
}


/*
 * Same as one_argument except that it takes two args and returns the rest;
 * ignores fill words
 */
char *two_arguments(const char *argument, char *first_arg, char *second_arg) {
	return one_argument(one_argument(argument, first_arg), second_arg);
}


char *three_arguments(const char *argument, char *first_arg, char *second_arg, char *third_arg) {
	return one_argument(two_arguments(argument, first_arg, second_arg), third_arg);
}


/*
 * determine if a given string is an abbreviation of another
 * returns 1 if arg1 is an abbreviation of arg2
 */
bool is_abbrev(const char *arg1, const char *arg2) {
	if (!*arg1 || !*arg2)
		return false;

	for (; *arg1 && *arg2; arg1++, arg2++)
		if (LOWER(*arg1) != LOWER(*arg2))
			return false;

	if (!*arg1)	return true;
	else		return false;
}

/* return first space-delimited token in arg1; remainder of string in arg2 */
void half_chop(const char *string, char *arg1, char *arg2)
{
  char *temp;

  temp = any_one_arg(string, arg1);
  skip_spaces(temp);
  strcpy(arg2, temp);
}



/* Used in specprocs, mostly.  Matches "command" to cmd number, accounting
   for abbreviations */
int find_command_abbrev(char *command)
{
	int cmd;
	const int len = strlen(command);

	for (cmd = 0; *complete_cmd_info[cmd].command != '\n'; ++cmd)
		if (!strncmp(complete_cmd_info[cmd].command, command, len))
			return cmd;

	return -1;
}


/* Used in specprocs, mostly.  (Exactly) matches "command" to cmd number */
int find_command(const char *command) {
	int cmd;

	for (cmd = 0; *complete_cmd_info[cmd].command != '\n'; cmd++)
		if (!strcmp(complete_cmd_info[cmd].command, command))
			return cmd;

	return -1;
}


bool special(Character *ch, const char *cmd, char *arg) {
	Object *i;
	Character *k;
	int j;

	//	Room
	if (GET_ROOM_SPEC(IN_ROOM(ch)) != NULL)
		if (GET_ROOM_SPEC(IN_ROOM(ch)) (ch, &world[IN_ROOM(ch)], cmd, arg))
			return true;

	//	In EQ List
	for (j = 0; j < NUM_WEARS; j++)
		if (GET_EQ(ch, j) && GET_OBJ_SPEC(GET_EQ(ch, j)) != NULL)
			if (GET_OBJ_SPEC(GET_EQ(ch, j)) (ch, GET_EQ(ch, j), cmd, arg))
				return true;

	//	In Inventory
	START_ITER(obj_iter, i, Object *, ch->contents)
		if (GET_OBJ_SPEC(i) && GET_OBJ_SPEC(i) (ch, i, cmd, arg))
			return true;

	//	In Mobile
	START_ITER(mob_iter, k, Character *, world[IN_ROOM(ch)].people)
		if (GET_MOB_SPEC(k) && GET_MOB_SPEC(k) (ch, k, cmd, arg))
			return true;

	//	In Object present
	START_ITER(world_iter, i, Object *, world[IN_ROOM(ch)].contents)
		if (GET_OBJ_SPEC(i) && GET_OBJ_SPEC(i) (ch, i, cmd, arg))
			return true;

	return false;
}



/* *************************************************************************
*  Stuff for controlling the non-playing sockets (get name, pwd etc)       *
************************************************************************* */


/* locate entry in p_table with entry->name == name. -1 mrks failed search */
int find_name(const char *name)
{
  int i;

  for (i = 0; i < player_table.Count(); i++) {
    if (!str_cmp(player_table[i].name, name))
      return i;
  }

  return -1;
}


int _parse_name(char *arg, char *name)
{
  int i;

  /* skip whitespaces */
  for (; isspace(*arg); arg++);

  for (i = 0; (*name = *arg); arg++, i++, name++)
    if (!isalpha(*arg))
      return 1;

  if (!i)
    return 1;

  return 0;
}


#define RECON		1
#define USURP		2
#define UNSWITCH	3

int perform_dupe_check(Descriptor *d) {
	Descriptor *k; //, *next_k;
	Character *target = NULL, *ch;
	int mode = 0;

	IDNum id = GET_IDNUM(d->character);

	/*
	 * Now that this descriptor has successfully logged in, disconnect all
	 * other descriptors controlling a character with the same ID number.
	 */

	START_ITER(desc_iter, k, Descriptor *, descriptor_list) {
		if (k == d)
			continue;

		if (k->original && (GET_IDNUM(k->original) == id)) {    /* switched char */
			k->Write("\r\nMultiple login detected -- disconnecting.\r\n");
			STATE(k) = CON_CLOSE;
			if (!target) {
				target = k->original;
				mode = UNSWITCH;
			}
			if (k->character)
				k->character->desc = NULL;
			k->character = NULL;
			k->original = NULL;
		} else if (k->character && (GET_IDNUM(k->character) == id)) {
			if (!target && (STATE(k) == CON_PLAYING)) {
				k->Write("\r\nThis body has been usurped!\r\n");
				target = k->character;
				mode = USURP;
			}
			k->character->desc = NULL;
			k->character = NULL;
			k->original = NULL;
			k->Write("\r\nMultiple login detected -- disconnecting.\r\n");
			STATE(k) = CON_CLOSE;
		}
	}

	/*
	 * now, go through the character list, deleting all characters that
	 * are not already marked for deletion from the above step (i.e., in the
	 * CON_HANGUP state), and have not already been selected as a target for
	 * switching into.  In addition, if we haven't already found a target,
	 * choose one if one is available (while still deleting the other
	 * duplicates, though theoretically none should be able to exist).
	 */
	START_ITER(ch_iter, ch, Character *, Characters) {
		if (IS_NPC(ch))				continue;
		if (GET_IDNUM(ch) != id)	continue;

		//	ignore chars with descriptors (already handled by above step)
		if (ch->desc)				continue;

		//	don't extract the target char we've found one already
		if (ch == target)			continue;

		//	we don't already have a target and found a candidate for switching
		if (!target) {
			target = ch;
			mode = RECON;
			continue;
		}

		//	we've found a duplicate - blow him away, dumping his eq in limbo.
		if (IN_ROOM(ch) != NOWHERE)
			ch->FromRoom();
		ch->ToRoom(1);
		ch->Extract();
	}

	//	no target for swicthing into was found - allow login to continue
	if (!target)
		return 0;

	//	Okay, we've found a target.  Connect d to target.
	delete d->character; /* get rid of the old char */
	d->character = target;
	target->desc = d;
	d->original = NULL;
	target->player->timer = 0;
	REMOVE_BIT(PLR_FLAGS(target), PLR_MAILING | PLR_WRITING);
	STATE(d) = CON_PLAYING;

	switch (mode) {
		case RECON:
			d->Write("Reconnecting.\r\n");
			act("$n has reconnected.", TRUE, target, 0, 0, TO_ROOM);
			mudlogf( NRM, target, TRUE,  "%s [%s] has reconnected.", target->RealName(), d->host);
			break;
		case USURP:
			d->Write("You take over your own body, already in use!\r\n");
			act("$n suddenly keels over in pain, surrounded by a white aura...\r\n"
				"$n's body has been taken over by a new spirit!", TRUE, target, 0, 0, TO_ROOM);
			mudlogf(NRM, target, TRUE, "%s has re-logged in ... disconnecting old socket.",
					target->RealName());
			break;
		case UNSWITCH:
			d->Write("Reconnecting to unswitched char.");
			mudlogf( NRM, target, TRUE,  "%s [%s] has reconnected.", target->RealName(), d->host);
			break;
	}

	return 1;
}


/* load the player, put them in the right room - used by copyover_recover too */
int enter_player_game (Descriptor *d) {
	VNum load_room;
	int load_result;

	d->character->Reset();
	d->character->ToWorld();
	load_room = d->character->StartRoom();
	d->character->ToRoom(load_room);
	load_result = Crash_load(d->character);

	start_otrigger(d->character->contents);
	for (SInt32 i = 0; i < NUM_WEARS; ++i)
		if (GET_EQ(d->character, i))	start_otrigger(GET_EQ(d->character, i));

	d->character->Save(d->character->AbsoluteRoom());
	return load_result;
}


/* deal with newcomers and other non-playing sockets */
void nanny(Descriptor *d, char *arg) {
	char *buf, *tmp_name;
	int player_i, load_result, thingy;
	Editor * tmpeditor = NULL;

	skip_spaces(arg);

	if (d->Edit()) {
		tmpeditor = (Editor *) d->Edit();	// Sigh, another cast to discard const for a member method.
		tmpeditor->Parse(arg);
		return;
	}

	switch (STATE(d)) {

//   /*. OLC states .*/
// 		case CON_OEDIT: 
// 		case CON_REDIT: 
// 		case CON_ZEDIT: 
// 		case CON_MEDIT: 
// 		case CON_SEDIT: 
// 		case CON_AEDIT:
// 		case CON_HEDIT:
// 		case CON_SCRIPTEDIT:
// 		case CON_CEDIT:		/* Editing clans */
// 			d->Editor()->Parse(arg);
// 			break;
// /*. End of OLC states .*/
// 		
		case CON_GET_NAME:		/* wait for input of name */
			if (!d->character) {
				d->character = new Character;
				d->character->player = new PlayerData;
				d->character->desc = d;
			}
			if (!*arg)
				STATE(d) = CON_CLOSE;
			else {
				buf = get_buffer(128);
				tmp_name = get_buffer(MAX_INPUT_LENGTH);

				if ((_parse_name(arg, tmp_name)) || strlen(tmp_name) < 2 ||
						strlen(tmp_name) > MAX_NAME_LENGTH || // !Valid_Name(tmp_name) ||
						fill_word(strcpy(buf, tmp_name)) || reserved_word(buf))
					;	//	Bad name breaker
				else if ((player_i = d->character->Load(tmp_name)) > -1) {
					//	Character loaded
					GET_PFILEPOS(d->character) = player_i;

					if (PLR_FLAGGED(d->character, PLR_DELETED)) {
						/* We get false positive from the original deleted name. */
						get_filename(d->character->RealName(), buf);
						remove(buf);

						SSFree(d->character->name);
						d->character->name = NULL;
						/* Check for multiple creations... */
						if (Ban::ValidName(tmp_name)) {
							delete d->character;
							d->character = new Character;
							d->character->player = new PlayerData;
							d->character->desc = d;
							d->character->SetName(CAP(tmp_name));
							GET_PFILEPOS(d->character) = -1;

							STATE(d) = CON_NAME_CNFRM;
						}
					} else {
/* undo it just in case they are set */
						REMOVE_BIT(PLR_FLAGS(d->character), PLR_WRITING | PLR_MAILING);

						d->Write("Password: ");
						echo_off(d);
						d->idle_tics = 0;
						STATE(d) = CON_PASSWORD;
					}
				} else {	//	Player unknown... new characters
					SSFree(d->character->name);
					d->character->name = NULL;
					if (Ban::ValidName(tmp_name)) {
						d->character->SetName(CAP(tmp_name));
						STATE(d) = CON_NAME_CNFRM;
					}
				}

				if (STATE(d) == CON_GET_NAME)
					d->Write("Invalid name, please try another: ");
				else if (STATE(d) == CON_NAME_CNFRM)
					d->Writef("Did I hear you correctly, '%s' (Y/N)? ", tmp_name);
				release_buffer(buf);
				release_buffer(tmp_name);
			}

			// QANSI upgrade - DH
			if (d->termtype == TERM_ANSI) {
				SET_BIT(PRF_FLAGS(d->character), Preference::Color);
			} else {
				REMOVE_BIT(PRF_FLAGS(d->character), Preference::Color);
			}

			break;

		case CON_NAME_CNFRM:		/* wait for conf. of new name    */
			if (UPPER(*arg) == 'Y') {
				if (Ban::Banned(d->host) != Ban::Not) {
//					d->Write("Sorry, new characters are not allowed from your site!\r\n");
					d->Write("Sorry, no new characters allowed from your site.");
//					mudlogf(NRM, TRUST_IMMORTAL, TRUE, "Request for new char %s denied from [%s] (siteban)",
//							d->character->RealName(), d->host);
					STATE(d) = CON_CLOSE;
					return;
				}
				if (circle_restrict) {
//					d->Write("Sorry, new players can't be created at the moment.\r\n");
					d->Write("Sorry, no new players can be created at the moment.  Try again later.");
//					mudlogf(NRM, TRUST_IMMORTAL, TRUE, "Request for new char %s denied from [%s] (wizlock)",
//							d->character->RealName(), d->host);
					STATE(d) = CON_CLOSE;
					return;
				}
#ifdef EMAIL_PASSWORD
				// EMAIL VALIDATION
				d->Write("\r\n"
						 "For security reasons, you need to enter your email address.\r\n\n"
						 "You will be sent an email with a randomly generated password at this\r\n"
						 "address which you will need before you can enter the game.  You may\r\n"
						 "change your password to your liking afterwards.  If you have any\r\n"
						 "questions, please email us.\r\n\n"
						 "All information will be kept confidential to the MUD administration, for\r\n"
						 "security purposes only.  We reserve the right to deny access to the game\r\n"
						 "based on an invalid or incorrect email address.\r\n\n"
						 "Your email address: ");
				STATE(d) = CON_GETEMAIL;
#else
				d->Writef("Enter a password, '%s': ", d->character->RealName());

				echo_off(d);
				STATE(d) = CON_NEWPASSWD;
#endif
			} else if (*arg == 'n' || *arg == 'N') {
				d->Write("Okay, what IS it, then? ");
				SSFree(d->character->name);
				d->character->name = NULL;
				STATE(d) = CON_GET_NAME;
			} else {
				d->Write("Please answer 'yes' or 'no' : ");
			}
			break;
		case CON_PASSWORD:		/* get pwd for known player      */
			echo_on(d);    /* turn echo back on */

			if (!*arg)
				STATE(d) = CON_CLOSE;
			else {
				if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH)) {
					mudlogf(BRF, NULL, TRUE,  "Bad PW: %s [%s]", d->character->RealName(), d->host);
					GET_BAD_PWS(d->character)++;
					d->character->Save(NOWHERE);
					if (++(d->bad_pws) >= max_bad_pws) {	/* 3 strikes and you're out. */
						d->Write("Wrong password... access denied.\r\n");
						STATE(d) = CON_CLOSE;
					} else {
						d->Write("Wrong password.  Try again.\r\nPassword: ");
						echo_off(d);
					}
					return;
				}
				load_result = GET_BAD_PWS(d->character);
				GET_BAD_PWS(d->character) = 0;

				if ((Ban::Banned(d->host) == Ban::Select) && !PLR_FLAGGED(d->character, PLR_SITEOK)) {
					d->Write("This character has not been cleared for access from your site!\r\n");
					STATE(d) = CON_CLOSE;
//					mudlogf(NRM, TRUST_IMMORTAL, TRUE, "Connection attempt for %s denied from %s",
//							d->character->RealName(), d->host);
					return;
				}
				if (GET_LEVEL(d->character) < circle_restrict) {
					d->Write("The game is temporarily restricted.. try again later.\r\n");
					STATE(d) = CON_CLOSE;
//					mudlogf(NRM, TRUST_IMMORTAL, TRUE, "Request for login denied for %s [%s] (wizlock)",
//							d->character->RealName(), d->host);
					return;
				}
      /* check and make sure no other copies of this player are logged in */
				if (perform_dupe_check(d))
					return;

				d->character->player->time.logon = time(0);	// Set the last logon time to now.

				d->Writef("%s", IS_STAFF(d->character) ? imotd : motd);

				mudlogf( BRF, d->character, TRUE,
						"%s [%s] has connected.", d->character->RealName(), d->host);

				if (load_result) {
					d->Writef("\r\n\r\n\007\007\007"
							"&cR%d LOGIN FAILURE%s SINCE LAST SUCCESSFUL LOGIN.&c0\r\n",
							load_result, (load_result > 1) ? "S" : "");

					GET_BAD_PWS(d->character) = 0;
				}
				d->Write("\r\n\n*** PRESS ENTER WHEN READY: ");
				STATE(d) = CON_RMOTD;
			}
			break;

		case CON_NEWPASSWD:
		case CON_CHPWD_GETNEW:
			if (!*arg || strlen(arg) > MAX_PWD_LENGTH || strlen(arg) < 3 || !str_cmp(arg, d->character->RealName())) {
				d->Write("\r\nIllegal password.\r\n"
						 "Password: ");
				break;
			}
			strncpy(GET_PASSWD(d->character), CRYPT(arg, d->character->RealName()), MAX_PWD_LENGTH);
			*(GET_PASSWD(d->character) + MAX_PWD_LENGTH) = '\0';

			d->Write("\r\nPlease verify password: ");

			STATE(d) = (STATE(d) == CON_NEWPASSWD) ? CON_CNFPASSWD : CON_CHPWD_VRFY;

			break;

		case CON_CNFPASSWD:
		case CON_CHPWD_VRFY:
			if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH)) {
				d->Write("\r\nPasswords don't match... start over.\r\n" "Password: ");
				STATE(d) = (STATE(d) == CON_CNFPASSWD) ? CON_NEWPASSWD : CON_CHPWD_GETNEW;
				break;
			}
			echo_on(d);

			if (STATE(d) == CON_CNFPASSWD) {
				d->Write("\r\nYou need to enter a valid email address for security reasons.\r\n"
						 "You will be sent an email with a randomly generated password at this\r\n"
						 "address before you can enter the game.  Addresses from free public\r\n"
						 "email services are only allowed if that is your ONLY address.\r\n"
						 "All information will be kept confidential, for security purposes only.\r\n"
						 "We reserve the right to deny access to the game based on an invalid\r\n"
						 "or incorrect email address.\r\n"
						 "\r\n"
						 "EMail Address: ");
				STATE(d) = CON_GETEMAIL;
			} else {
				d->character->Save(NOWHERE);
				d->Writef("\r\nDone.\r\n%s", (d->termtype == TERM_PLAIN ? PLAINMENU : MENU));

				STATE(d) = CON_MENU;
			}
			break;

		case CON_QANSI:		// Have to ask if they want color BEFORE
			switch (*arg) {		// we use color codes

			case 'n':
			case 'N':
				d->termtype = TERM_PLAIN;
				d->Write(PLAINGREETINGS);
				break;
			default:
				d->termtype = TERM_ANSI;
				d->Write(GREETINGS);
				break;
			}
			d->Write("\x1B[2K\n\rBy what name do you wish to be known? ");
			STATE(d) = CON_GET_NAME;

			break;

		case CON_GETEMAIL:
			if (strchr(arg, '@') && !strchr(arg, ' ')) {
				d->character->player->email = str_dup(arg);

				roll_real_abils(d->character);
			    d->Write(cg_info(d->character));
				STATE(d) = CON_CGINFO;
			} else {
				d->Write("\r\nYou must enter a valid email address.\r\n"
						 "\r\n"
						 "EMail Address: ");
			}
			break;

	case CON_CGINFO:		/* JD's new Character Creator		*/
		switch (*arg) {
		case 's':
		case 'S':
			d->Write("Sex (M/F) ? ");
			STATE(d) = CON_QSEX;
			break;
// 		case 'b':
// 		case 'B':
// 			display_classes(d->character);
// 			STATE(d) = CON_QCLASS;
// 			break;
		case 'r':
		case 'R':
			display_races(d->character);
			STATE(d) = CON_QRACE;
			break;
		case '1':
			d->Write("Strength? ");
			STATE(d) = CON_QSTR;
			break;
		case '2':
			d->Write("Intelligence? ");
			STATE(d) = CON_QINT;
			break;
		case '3':
			d->Write("Wisdom? ");
			STATE(d) = CON_QWIS;
			break;
		case '4':
			d->Write("Dexterity? ");
			STATE(d) = CON_QDEX;
			break;
		case '5':
			d->Write("Constitution? ");
			STATE(d) = CON_QCON;
			break;
		case '6':
			d->Write("Charisma? ");
			STATE(d) = CON_QCHA;
			break;
		case 'a':
		case 'A':
			d->Write("1) Evil        2) Neutral     3) Good\r\n");
			STATE(d) = CON_QALIGN;
			break;
		case 'd':
		case 'D':
			d->Write("Enter a short description of your character, i.e. \"a short, sandy-haired elf\" :\r\n");
			STATE(d) = CON_SDESC;
			break;
		case 'l':
		case 'L':
			d->Write("1) Chaotic     2) Neutral     3) Lawful\r\n");
			STATE(d) = CON_QLAWFUL;
			break;
		case '+':
			STATE(d) = CON_QADV;
			cg_edit_advantage(d->character, "");
			break;
		case '-':
			STATE(d) = CON_QDISADV;
			cg_edit_disadvantage(d->character, "");
			break;
		case 'q':
		case 'Q':
			if (((GET_TOTALPTS(d->character)) > 120) ||
				(!raceTclass[(int) GET_RACE(d->character)][(int) GET_CLASS(d->character)])) {
				d->Write(cg_info(d->character));
				return;
			}
			if (!d->character->Keywords()) {
				d->Write(cg_info(d->character));
				return;
			}
			d->Write("\r\nAre you satisfied with this character?  (y/N)  ");
			STATE(d) = CON_QSAVE;
			break;
		default:
			d->Write(cg_info(d->character));
			STATE(d) = CON_CGINFO;
			break;
		}
		break;

	case CON_QSTR:
		thingy = atoi(arg);
		if (thingy >= 3) {
			thingy = MIN(max_stat_table[GET_RACE(d->character)][0], thingy);
			GET_STR(d->character) = thingy;
			GET_TOTALPTS(d->character) = sum_char_pts(d->character);
		} else
			d->Write("Sorry, but you MUST have at least 3 in ALL stats!!!");
		d->Write(cg_info(d->character));
		STATE(d) = CON_CGINFO;
		break;

	case CON_QINT:
		thingy = atoi(arg);
		if (thingy >= 3) {
			thingy = MIN(max_stat_table[GET_RACE(d->character)][1], thingy);
			GET_INT(d->character) = thingy;
			GET_TOTALPTS(d->character) = sum_char_pts(d->character);
		} else
			d->Write("Sorry, but you MUST have at least 3 in ALL stats!!!");

		d->Write(cg_info(d->character));
		STATE(d) = CON_CGINFO;
		break;

	case CON_QWIS:
		thingy = atoi(arg);
		if (thingy >= 3) {
			thingy = MIN(max_stat_table[GET_RACE(d->character)][2], thingy);
			GET_WIS(d->character) = thingy;
			GET_TOTALPTS(d->character) = sum_char_pts(d->character);
		} else
			d->Write("Sorry, but you MUST have at least 3 in ALL stats!!!");
		d->Write(cg_info(d->character));
		STATE(d) = CON_CGINFO;
		break;

	case CON_QDEX:
		thingy = atoi(arg);
		if (thingy >= 3) {
			thingy = MIN(max_stat_table[GET_RACE(d->character)][3], thingy);
			GET_DEX(d->character) = thingy;
			GET_TOTALPTS(d->character) = sum_char_pts(d->character);
		} else
			d->Write("Sorry, but you MUST have at least 3 in ALL stats!!!");
		d->Write(cg_info(d->character));
		STATE(d) = CON_CGINFO;
		break;

	case CON_QCON:
		thingy = atoi(arg);
		if (thingy >= 3) {
			thingy = MIN(max_stat_table[GET_RACE(d->character)][4], thingy);
			GET_CON(d->character) = thingy;
			GET_TOTALPTS(d->character) = sum_char_pts(d->character);
		} else
			d->Write("Sorry, but you MUST have at least 3 in ALL stats!!!");
		d->Write(cg_info(d->character));
		STATE(d) = CON_CGINFO;
		break;

	case CON_QCHA:
		thingy = atoi(arg);
		if (thingy >= 3) {
			thingy = MIN(max_stat_table[GET_RACE(d->character)][5], thingy);
			GET_CHA(d->character) = thingy;
			GET_TOTALPTS(d->character) = sum_char_pts(d->character);
		} else
			d->Write("Sorry, but you MUST have at least 3 in ALL stats!!!");
		d->Write(cg_info(d->character));
		STATE(d) = CON_CGINFO;
		break;

	case CON_QSEX:		/* query sex of new user				 */
			switch (LOWER(*arg)) {
				case 'm':	d->character->general.sex = Male;		break;
				case 'f':	d->character->general.sex = Female;		break;
				default:
					d->Write("That is NOT a gender..\r\n"
							 "Try again: ");
					return;
			}
		d->Write(cg_info(d->character));
		STATE(d) = CON_CGINFO;
		break;

	case CON_QALIGN:		/* query alignment of new user				 */
		switch (*arg) {
		case '1':
			GET_ALIGNMENT(d->character) = -750;
			break;
		case '2':
			GET_ALIGNMENT(d->character) = 0;
			break;
		case '3':
			GET_ALIGNMENT(d->character) = 750;
			break;
		default:
			d->Write("That is not an alignment..\r\n"
		"What IS your alignment? ");
			return;
		}
		d->Write(cg_info(d->character));
		STATE(d) = CON_CGINFO;
		break;

	case CON_SDESC:		/* query alignment of new user				 */
		if (arg && *arg) {
			char *p;
			buf = get_buffer(MAX_INPUT_LENGTH);
			*buf1 = '\0';
			*buf2 = '\0';
			int count = 1;
			d->character->SetSDesc(arg);

			// Make sure the character's race is a keyword entry
			strcpy(buf3, race_types[GET_RACE(d->character)]);
			p = buf3;
			while (*p) {
				*p = LOWER(*p);
				++p;
			}

			if (!strstr(arg, buf3)) {
				strcpy(buf1, buf3);
			}

			half_chop(arg, buf2, arg);

			while (buf2 && *buf2) {

				// Strip non-alphabetic characters from the keyword, and change hyphens to spaces.
				p = buf2;
				while (p && *p) {
					if (*p == '-')							*p = ' ';
					else if (isdigit(*p) || isalpha(*p))	;
					else {
						*p = '\0';
						break;
					}

					++p;
				}

				// Make sure
				if ((strlen(buf2) >= 3) && (strcmp(buf2, "the"))) {
					++count;
					sprintf(buf1 + strlen(buf1), "%s%s", *buf1 ? " " : "", buf2);
				}
				half_chop(arg, buf2, arg);
			}

			// We need at least three valid keywords other
			if (count >= 4)			d->character->SetKeywords(buf1);
			else					d->character->SetKeywords((const char *) NULL);

			release_buffer(buf);
		}
		d->Write(cg_info(d->character));
		STATE(d) = CON_CGINFO;
		break;

	case CON_QLAWFUL:		/* query lawfulness of new user				 */
		switch (*arg) {
		case '1':
			GET_LAWFULNESS(d->character) = -750;
			break;
		case '2':
			GET_LAWFULNESS(d->character) = 0;
			break;
		case '3':
			GET_LAWFULNESS(d->character) = 750;
			break;
		default:
			d->Write("That is not an alignment..\r\n"
		"What IS your alignment? ");
			return;
		}
		d->Write(cg_info(d->character));
		STATE(d) = CON_CGINFO;
		break;

	case CON_QRACE:

		load_result = parse_race_menu(*arg);
		if (load_result == RACE_ANIMAL) {
			d->Write("\r\nThat's not a valid race.");
		} else {
			GET_RACE(d->character) = (Race) load_result;
			if (!raceTclass[(int) GET_RACE(d->character)][(int) GET_CLASS(d->character)])
				GET_CLASS(d->character) = CLASS_UNDEFINED;
			ADV_FLAGS(d->character) = 0;
			DISADV_FLAGS(d->character) = 0;
			if (GET_RACE(d->character) == RACE_LAORI)
				SET_BIT(ADV_FLAGS(d->character), Advantages::Flight);

			GET_STR(d->character) = MAX(3, MIN(GET_STR(d->character), max_stat_table[GET_RACE(d->character)][0]));
			GET_INT(d->character) = MAX(3, MIN(GET_INT(d->character), max_stat_table[GET_RACE(d->character)][1]));
			GET_WIS(d->character) = MAX(3, MIN(GET_WIS(d->character), max_stat_table[GET_RACE(d->character)][2]));
			GET_DEX(d->character) = MAX(3, MIN(GET_DEX(d->character), max_stat_table[GET_RACE(d->character)][3]));
			GET_CON(d->character) = MAX(3, MIN(GET_CON(d->character), max_stat_table[GET_RACE(d->character)][4]));
			GET_CHA(d->character) = MAX(3, MIN(GET_CHA(d->character), max_stat_table[GET_RACE(d->character)][5]));
		}

		d->character->SetKeywords((SString *) NULL);
		d->character->SetSDesc((SString *) NULL);

		d->Write(cg_info(d->character));
		STATE(d) = CON_CGINFO;
		break;

	case CON_QADV:
		if (arg && (*arg == '0')) {
			d->Write(cg_info(d->character));
			STATE(d) = CON_CGINFO;
			break;
		}
		cg_edit_advantage(d->character, arg);
		cg_edit_advantage(d->character, "");
		break;

	case CON_QDISADV:
		if (arg && (*arg == '0')) {
			d->Write(cg_info(d->character));
			STATE(d) = CON_CGINFO;
			break;
		}
		cg_edit_disadvantage(d->character, arg);
		cg_edit_disadvantage(d->character, "");
		break;

	case CON_QSAVE:
		switch (*arg) {
		case 'y':
		case 'Y':
			d->character->real_abils = d->character->aff_abils;

			if ((GET_TOTALPTS(d->character)) > 60)	GET_EXP_MOD(d->character) = GET_TOTALPTS(d->character) - 60;
			else									GET_EXP_MOD(d->character) = 0;

			// d->character->save(GET_LOADROOM(d->character));
			// page_string(d, START_MESSG, true);
			// d->Write("\r\n\n*** PRESS RETURN: ");
			// STATE(d) = CON_NEWPLAYMESG;

			mudlogf( NRM, NULL, TRUE,  "%s [%s] new player.", d->character->RealName(), d->host);
			((d->character)->points.max_hit) = ((d->character)->points.hit) = MAX((((d->character)->aff_abils.Con) * 10), 1);
			((d->character)->points.max_mana) = ((d->character)->points.mana) = MAX((((d->character)->aff_abils.Wis) * 10), 1);
			((d->character)->points.max_move) = ((d->character)->points.move) = MAX((((d->character)->aff_abils.Dex) * 10), 1);

#ifdef EMAIL_PASSWORD
			email_password(d->character);
#endif
			d->character->Initialize();	// Creates the player index entry.

			do_start(d->character);
			d->character->Save(NOWHERE);
			Crash_crashsave(d->character);

#ifdef EMAIL_PASSWORD
			d->Write("\r\n"
					 "By now, an email with your randomly generated password is en route to your\r\n"
					 "mailbox.  If no email arrives within the next twenty minutes, please email\r\n"
					 "sfn@sfn.zenonline.com and let the staff know!\r\n\n"
					 "See you soon!\r\n\n");

			STATE(d) = CON_CLOSE;
#else
			d->Write("\r\n\n*** PRESS ENTER WHEN READY: ");
			STATE(d) = CON_RMOTD;
#endif
			break;
		default:
			d->Write(cg_info(d->character));
			STATE(d) = CON_CGINFO;
			break;
		}
		break;

	case CON_NEWPLAYMESG:
		d->Write(motd);
		d->Write("\r\n\n*** PRESS RETURN: ");
		STATE(d) = CON_RMOTD;
		break;

		case CON_RMOTD:		/* read CR after printing motd   */
			if (d->termtype == TERM_PLAIN)	d->Write(PLAINMENU);
			else							d->Write(MENU);
			STATE(d) = CON_MENU;
			break;

		case CON_MENU:		/* get selection from main menu  */
			switch (*arg) {
				case '0':
					d->Write("Goodbye.\r\n");
					STATE(d) = CON_CLOSE;
					break;

				case '1':
					load_result = enter_player_game(d);
					d->character->Send(WELC_MESSG);

					act("$n has entered the game.", TRUE, d->character, 0, 0, TO_ROOM);
					mudlogf( BRF, d->character, TRUE,
						"%s [%s] has entered the game.", d->character->RealName(), d->host);

					STATE(d) = CON_PLAYING;
// 					if (!GET_LEVEL(d->character)) {
// 						do_start(d->character);
// 						d->character->Save(d->character->AbsoluteRoom());
// 						Crash_crashsave(d->character);
// 					}
					look_at_room(d->character, 0);
					if (has_mail(GET_IDNUM(d->character)))
						d->character->Send("You have mail waiting.\r\n");
					if (load_result == 2) {	/* rented items lost */
						d->Write("\r\n\007You could not afford your rent!\r\n"
								 "Your possesions have been donated to the Salvation Army!\r\n");
						}
					d->has_prompt = 0;
					break;

				case '2':
					d->Write("Enter the text you'd like others to see when they look at you.\r\n"
							 "(/s saves /h for help)\r\n");
					if (!d->character->SSLDesc()) {
						d->character->general.longDesc = SString::Create("");
					}
					tmpeditor = new SStringEditor(d, &d->character->general.longDesc, EXDSCR_LENGTH);
					d->Edit(tmpeditor);
					STATE(d) = CON_EXDESC;
					break;

				case '3':
					page_string(d, background, false);
					STATE(d) = CON_RMOTD;
					break;

				case '4':
					d->Write("\r\nEnter your old password: ");
					echo_off(d);
					STATE(d) = CON_CHPWD_GETOLD;
					break;

				case '5':
					d->Write("\r\nEnter your password for verification: ");
					echo_off(d);
					STATE(d) = CON_DELCNF1;
					break;

				default:
					d->Writef("\r\nThat's not a menu choice!\r\n");
					break;
			}

			break;

		case CON_CHPWD_GETOLD:
			if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH)) {
				echo_on(d);
				d->Writef("\r\nIncorrect password.\r\n%s", (d->termtype == TERM_PLAIN ? PLAINMENU : MENU));
				STATE(d) = CON_MENU;
			} else {
				d->Write("\r\nEnter a new password: ");
				STATE(d) = CON_CHPWD_GETNEW;
			}
			return;

		case CON_DELCNF1:
			echo_on(d);
			if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH)) {
				d->Writef("\r\nIncorrect password.\r\n%s", (d->termtype == TERM_PLAIN ? PLAINMENU : MENU));
				STATE(d) = CON_MENU;
			} else {
				d->Write("\r\nYOU ARE ABOUT TO DELETE THIS CHARACTER PERMANENTLY.\r\n"
						 "ARE YOU ABSOLUTELY SURE?\r\n\r\n"
						 "Please type \"yes\" to confirm: ");
				STATE(d) = CON_DELCNF2;
			}
			break;

		case CON_DELCNF2:
			if (!str_cmp(arg, "yes")) {
				if (PLR_FLAGGED(d->character, PLR_FROZEN)) {
					d->Write("You try to kill yourself, but the ice stops you.\r\n"
							 "Character not deleted.\r\n\r\n");
					STATE(d) = CON_CLOSE;
				} else {
					if (!STF_FLAGGED(d->character, Staff::Admin))
						SET_BIT(PLR_FLAGS(d->character), PLR_DELETED);
					d->character->Save(NOWHERE);

					d->Writef("Character '%s' deleted!\r\n"
							 "Goodbye.\r\n", d->character->RealName());

					buf = get_buffer(128);
					get_filename(d->character->RealName(), buf);
					remove(buf);
					release_buffer(buf);

					mudlogf(NRM, d->character, TRUE, "%s (lev %d) has self-deleted.",
							d->character->RealName(), GET_LEVEL(d->character));
					STATE(d) = CON_CLOSE;
				}
			} else {
				d->Writef("\r\nCharacter not deleted.\r\n%s", (d->termtype == TERM_PLAIN ? PLAINMENU : MENU));
				STATE(d) = CON_MENU;
			}
			break;

// Taken care of in game_loop()
		case CON_CLOSE:
//			close_socket(d);
			break;

		case CON_IDENT:
			//	Idle on the player while we lookup their IP.  No input accepted.  Hah!
			d->Write("Your domain is being looked up, please be patient.\r\n");
			break;

		default:
			log("SYSERR: Nanny: illegal state of con'ness (%d) for '%s'; closing connection.", STATE(d), d->character ? d->character->RealName() : "<unknown>");
			STATE(d) = CON_DISCONNECT;
			break;
	}
}


void email_password(Character *ch)
{
	FILE *emailfile;
	UInt32 i;

	if (ch->player->email == NULL) {
		ch->Send("ERROR:  You have no email address!  Send email to\r\n"
				 "sfn@sfn.zenonline.com to report this bug!\r\n");
	}

	strcpy(buf, "cp " MSC_PREFIX "email " ETC_PREFIX "email.out");
	system(buf);

	if ((emailfile = fopen(ETC_PREFIX "email.out", "w")) == 0) {
		log("CRITICAL ERROR:  Unable to send intro email to %s!", ch->RealName());
		return;
	}

	for (i = 0; i < 5; ++i) {
		GET_PASSWD(ch)[i] = ((i + 2) % 2 ? Number('1', '9') : Number('a', 'z'));
	}
	GET_PASSWD(ch)[5] = '\0';

	sprintf(buf,
			"Your password is \"%s\", entered as shown without the quotes.\n"
			"Note that passwords are case-sensitive, so don't type in all caps!\n\n"
			"Finally, enjoy the MUD!\n\n"
			"       SFN Administration\n",
			GET_PASSWD(ch));
	fprintf(emailfile, buf);

	fclose(emailfile);

	sprintf(buf,
		"mail -s\"Password for %s on sfn.zenonline.com 4000\" %s < " ETC_PREFIX "email.out",
		ch->RealName(),
		ch->player->email);

	system(buf);

	// system("rm " ETC_PREFIX "email.out");

	strncpy(GET_PASSWD(ch), CRYPT(GET_PASSWD(ch), ch->RealName()), MAX_PWD_LENGTH);
}
