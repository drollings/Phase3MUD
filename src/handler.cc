/* ************************************************************************
*   File: handler.c                                     Part of CircleMUD *
*  Usage: internal funcs: moving and finding chars/objs                   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */




#include "structs.h"
#include "utils.h"
#include "scripts.h"
#include "buffer.h"
#include "characters.h"
#include "objects.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "event.h"
#include "find.h"

/* external vars */
//extern char *MENU;
//extern Trigger *purged_trigs;		   /* list of mtrigs to be deleted  */
//extern Script *purged_scripts; /* list of mob scripts to be del */

/* external functions */
ACMD(do_return);
void clearMemory(Character * ch);

void free_purged_lists(void);

int count_mobs_in_room(int num, int room);

char *fname(const char *namelist) {
	static char holder[256];
	char *point;

	for (point = holder; isalpha(*namelist); namelist++, point++)
		*point = *namelist;

	*point = '\0';

	return (holder);
}

const char * WHITESPACE = " \t";

int isname(const char *str, const char *namelist) {
	char *			newlist;
	const char *	curtok;

	newlist = str_dup(namelist); /* make a copy since strtok 'modifies' strings */

	for(curtok = strtok(newlist, WHITESPACE); curtok; curtok = strtok(NULL, WHITESPACE))
		if(curtok && is_abbrev(str, curtok)) {
			free(newlist);
			return 1;
		}
	free(newlist);
	return 0;
}


int isword(const char *str, const char *namelist) {
	char	*scan, 
			*buf = get_buffer(MAX_STRING_LENGTH), 
			*buf1 = get_buffer(MAX_STRING_LENGTH);

	strcpy(buf1, str);
	scan = buf1;
	while (*scan) {
		*scan = LOWER(*scan);
		++scan;
	}

	scan = one_word(namelist, buf);
	while (*buf) {
		if (!strcmp(buf1, buf)) {
			release_buffer(buf);
			release_buffer(buf1);
			return TRUE;
		}
		scan = one_word(scan, buf);
	}
	
	release_buffer(buf);
	release_buffer(buf1);
	return FALSE;
}


int silly_isname(const char *str, const char *namelist)
{
  char  *argv[MAX_INPUT_LENGTH], *xargv[MAX_INPUT_LENGTH];
  int   argc, xargc, i, j, exact;
  static char   buf[MAX_INPUT_LENGTH], names[MAX_INPUT_LENGTH], *s;

  strcpy(buf, str);
  argc = split_string(buf, "- \t\n\r,", argv);

  strcpy(names, namelist);
  xargc = split_string(names, "- \t\n\r,", xargv);

  s = argv[argc-1];
  s += strlen(s);
  if (*(--s) == '.') {
    exact = 1;
    *s = 0;
  } else {
    exact = 0;
  }
  /* the string has now been split into separate words with the '-'
     replaced by string terminators.  pointers to the beginning of
     each word are in argv */

  if (exact && argc != xargc)
    return 0;

  for (i = 0; i < argc; i++) {
    for (j = 0; j < xargc; j++) {
      if (xargv[j] && !str_cmp(argv[i], xargv[j])) {
        xargv[j] = NULL;
        break;
      }
    }
    if (j >= xargc)
      return 0;
  }

  return 1;
}

int split_string(char *str, char *sep, char **argv)
     /* str must be writable */
{
  char  *s;
  int   argc=0;

  s = strtok(str, sep);
  if (s)
    argv[argc++] = s;
  else {
    *argv = str;
    return(1);
  }

  while ((s = strtok(NULL, sep)))
    argv[argc++] = s;

  argv[argc] = '\0';

  return(argc);
}



int is_same_group(const Character *ach, const Character *bch ) {
	if (!ach || !bch)
		return FALSE;

	if ( ach->master != NULL ) ach = ach->master;
	if ( bch->master != NULL ) bch = bch->master;
	return ach == bch;
}


/* cleans out the purge lists */
void free_purged_lists() {
	Character *ch;
	Object *obj;
	Trigger *trig;
	Script *sc;
	
	START_ITER(ch_iter, ch, Character *, PurgedChars) {
		PurgedChars.Remove(ch);
		delete ch;
	}

	START_ITER(obj_iter, obj, Object *, PurgedObjs) {
		PurgedObjs.Remove(obj);
		delete obj;
	}

	START_ITER(trig_iter, trig, Trigger *, PurgedTrigs) {
		PurgedTrigs.Remove(trig);
		delete trig;
	}

	START_ITER(script_iter, sc, Script *, PurgedScripts) {
		PurgedScripts.Remove(sc);
		delete sc;
	}
}


struct affect_event_data {
  Affect *affect;
  Character *ch;
};


struct concentration_event_data {
  Concentration *concentration;
  Character *ch;
};


