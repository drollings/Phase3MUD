/***************************************************************************\
[*]    __     __  ___                ___  | LexiMUD is a C++ MUD codebase [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ |        for Sci-Fi MUDs        [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / | Copyright(C) 1997-1999        [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  | All rights reserved           [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   | Chris Jacobson (FearItself)   [*]
[*]      A Creation of the AvP Team!      | fear@technologist.com         [*]
[-]---------------------------------------+-------------------------------[-]
[*] File : descriptors.c++                                                [*]
[*] Usage: Descriptor code                                                [*]
\***************************************************************************/


#include "types.h"
#include "screen.h"

#include "descriptors.h"
#include "buffer.h"
#include "utils.h"
#include "editor.h"


LList<Descriptor *> descriptor_list;	//	master desc list

//	Buffer management externs
extern int buf_largecount;
extern int buf_overflows;
extern int buf_switches;
extern struct txt_block *bufpool;


Descriptor::Descriptor(UInt32 desc) : descriptor(desc), bad_pws(0), idle_tics(0),
		connected(CON_GET_NAME), wait(1), desc_num(0), login_time(time(0)), showstr_head(NULL),
		showstr_vector(NULL), showstr_count(0), showstr_page(0), default_color(COL_OFF), 
		has_prompt(false), history(new char *[HISTORY_SIZE]),
		history_pos(0), output(small_outbuf), bufptr(0), bufspace(SMALL_BUFSIZE - 1),
		large_outbuf(NULL), character(NULL), original(NULL), snooping(NULL), snoop_by(NULL),
		editor(NULL) {
	static SInt32	last_desc = 0;	//	last descriptor number
	
	host[0] = '\0';
	inbuf[0] = '\0';
	last_input[0] = '\0';
	small_outbuf[0] = '\0';
	host_ip[0] = '\0';
	host_name[0] = '\0';
	user_name[0] = '\0';
	
	memset(&input, 0, sizeof(txt_q));
	memset(&saddr, 0, sizeof(sockaddr_in));
	
	for (UInt32 pos = 0; pos < HISTORY_SIZE; pos++)
		history[pos] = NULL;
	
	if (++last_desc == 1000)
		last_desc = 1;
	desc_num = last_desc;
}


Descriptor::~Descriptor(void) {
	Editor * tmpeditor = editor;

	if (history) {	// Clear the command history
		for (UInt32 count = 0; count < HISTORY_SIZE; count++)
			if (history[count])
				free(history[count]);
		delete history;
	}

	if (showstr_head)		free(showstr_head);
	if (showstr_count)		free(showstr_vector);

	while (tmpeditor) {
		tmpeditor = tmpeditor->prev_editor;
		editor->Finish(Editor::All);
		delete editor;
	}
}


#ifdef PATCH_TWOX
/* 2x */
#undef TWOX_DEBUG
#define TWOX_MAX		99		//	Max value of (x*) digit.
#define TWOX_LEN		2		//	How many digits TWOX_MAX is.

//	Check for character codes we should not do (2*) to.  Examples
//	include newlines, carriage returns, and escape sequences (color).
bool EvilChar(char t) {
	/*
	 * If you use the \000 notation, the number must be converted to octal.
	 * Otherwise just put the number in the array without ' or \ marks.
	 * ex: char badcode[] = { 13, 10, 27, 0 };	(Non-portable version of below)
	 */
	char badcode[] = { '\r', '\n', '\033', '\0' };
	int i;

	for (i = 0; badcode[i] != '\0'; i++)
		if (t == badcode[i])
			return true;
	return false;
}
/* End 2x */
#endif


//	Add a new string to a player's output queue
SInt32 Descriptor::Write(const char *txt) {
//	if (!this)
//		return 0;

	SInt32		size = strlen(txt);
	
	/* if we're in the overflow state already, ignore this new output */

	if (bufptr < 0) {
		return 0;
	}
	
	//	if we have enough space, just write to buffer
	if (bufspace >= size) {
		strcpy(output + bufptr, txt);
		bufspace -= size;
		bufptr += size;
		return size;
	}
	
	//	If the text is too big to fit into even a large buffer, chuck the
	//	new text and switch to the overflow state.
	if ((size + bufptr) > (LARGE_BUFSIZE - 1)) {
		bufptr = -1;
		buf_overflows++;
		return 0;
	}
	buf_switches++;

#if USE_CIRCLE_SOCKET_BUF
	if (bufpool != NULL) {	//	if the pool has a buffer in it, grab it
		large_outbuf = bufpool;
		bufpool = bufpool->next;
	} else {				//	else create a new one
		CREATE(large_outbuf, struct txt_block, 1);
		CREATE(large_outbuf->text, char, LARGE_BUFSIZE);
		buf_largecount++;
	}

	strcpy(large_outbuf->text, output);	//	copy to big buffer
	output = large_outbuf->text;		//	make big buffer primary
#else
	//	Just request the buffer. Copy the contents of the old, and make it
	//	the primary buffer.
	large_outbuf = get_buffer(LARGE_BUFSIZE);
	strcpy(large_outbuf, output);
	output = large_outbuf;
#endif
	strcat(output, txt);						//	now add new text

	//	set the pointer for the next write
	bufptr = strlen(output);

	//	calculate how much space is left in the buffer
	bufspace = LARGE_BUFSIZE - 1 - bufptr;
	
	return size;
}


//	Add a new string to a player's output queue
SInt32 Descriptor::Writef(const char *fmt, ...) {
//	if (!this)
//		return 0;

	va_list		args;
	SInt32		size = 0;
	char *		txt = get_buffer(MAX_STRING_LENGTH);
	
	va_start(args, fmt);
	size += vsprintf(txt, fmt, args);
	va_end(args);
	
	/* if we're in the overflow state already, ignore this new output */

	if (bufptr < 0) {
		release_buffer(txt);
		return 0;
	}
	
	//	if we have enough space, just write to buffer
	if (bufspace >= size) {
		strcpy(output + bufptr, txt);
		bufspace -= size;
		bufptr += size;
		release_buffer(txt);
		return size;
	}
	
	//	If the text is too big to fit into even a large buffer, chuck the
	//	new text and switch to the overflow state.
	if ((size + bufptr) > (LARGE_BUFSIZE - 1)) {
		bufptr = -1;
		buf_overflows++;
		release_buffer(txt);
		return 0;
	}
	buf_switches++;

#if USE_CIRCLE_SOCKET_BUF
	if (bufpool != NULL) {	//	if the pool has a buffer in it, grab it
		large_outbuf = bufpool;
		bufpool = bufpool->next;
	} else {				//	else create a new one
		CREATE(large_outbuf, struct txt_block, 1);
		CREATE(large_outbuf->text, char, LARGE_BUFSIZE);
		buf_largecount++;
	}

	strcpy(large_outbuf->text, output);	//	copy to big buffer
	output = large_outbuf->text;		//	make big buffer primary
#else
	//	Just request the buffer. Copy the contents of the old, and make it
	//	the primary buffer.
	large_outbuf = get_buffer(LARGE_BUFSIZE);
	strcpy(large_outbuf, output);
	output = large_outbuf;
#endif
	strcat(output, txt);						//	now add new text

	//	set the pointer for the next write
	bufptr = strlen(output);

	//	calculate how much space is left in the buffer
	bufspace = LARGE_BUFSIZE - 1 - bufptr;
	
	release_buffer(txt);
	
	return size;
}


/* ******************************************************************
*  merge_to_output; DH's usage of the double snippet                *
****************************************************************** */


#undef TWOX_DEBUG
#define TWOX_MAX	99	/* Max value of (x*) digit.	*/
#define TWOX_LEN	2	/* How many digits TWOX_MAX is.	*/

/*
 * Check for character codes we should not do (2*) to.  Examples
 * include newlines, carriage returns, and escape sequences (color).
 */
int is_evil_char(char t)
{
  /*
   * If you use the \000 notation, the number must be converted to octal.
   * Otherwise just put the number in the array without ' or \ marks.
   * ex: char badcode[] = { 13, 10, 27, 0 };	(Non-portable version of below)
   */
  char badcode[] = { '\r', '\n', '\033', '\0' };
  int i;

  for (i = 0; badcode[i] != '\0'; i++)
    if (t == badcode[i])
      return TRUE;
  return FALSE;
}

//	Add a new string to a player's output queue
SInt32 Descriptor::MergeWrite(const char *txt) {
//	if (!this)
//		return 0;

	SInt32		size = strlen(txt);

  char *twox;
	
	/* if we're in the overflow state already, ignore this new output */

	if (bufptr < 0) {
		return 0;
	}
	

  if ((twox = strstr(output, txt)) && *twox && !is_evil_char(*twox)) {
//  if ((twox = strstr(t->output, txt)) && *twox && !is_evil_char(*twox) &&
//     (*(twox - 1) == *(twox - 1 + size))) {
    /*
     * error - if we failed processing, allow the normal code to process it.
     */
    int error = FALSE;

#if defined(TWOX_DEBUG)
    /*
     * If you have any problems with lost characters, color bleeding, or
     * excessive (2*)'s, turn on the debugging and check for what the
     * first digit printed out is.  Then add that value to the is_evil_char()
     * function in the badcode[] array.
     */
    sprintf(buf, "%d-%d-%d-%d", *twox, *(twox + 1), *(twox + 2), *(twox + 3));
    log(buf);
#endif

    /*
     * We're already a (x*) string, let's try to increment the number.
     * Too bad we can't check for the ( here but we'll do that later.
     */
    if (*(twox - 1) == ')' && *(twox - 2) == 'x') {
      /*
       * number - holds the (x*) digit to increment it.
       * pbegin - where the ( is in the (x*) so we can overwite the old.
       */
      int number;
      char *pbegin;

      /*
       * Scan backwards for the beginning of the (x*) construct.  We also
       * stop at the begining of the output buffer to prevent mismatch.
       */
      for (pbegin = twox; *pbegin != '(' && pbegin != output; pbegin--);

      /*
       * Somehow we got a *) without a ( matching.  It's a theory that people
       * could try to exploit the mismatch so log who saw it also.  This
       * only means that the character saw it, not necessarily did it.
       */
      if (sscanf(pbegin, "(%dx)%*s", &number) != 1) {
	  	// CHANGEPOINT
        // sprintf(buf, "(2x) Invalid construct %s:\n%s", character->Name(), pbegin);
        sprintf(buf, "(2x) Invalid construct:\n%s", pbegin);
        log(buf);
        error = TRUE;
      } else if (number >= TWOX_MAX) /* Cap the iterations. */
        error = TRUE;
      else {
        /*
         * bufdiff - difference of the two (x*)'s for buffer alignment.
         * digit - the new (x*), the size is arbitrary based on TWOX_MAX.
         */
        char *digit = (char *)malloc(TWOX_LEN+4);
        int bufdiff = twox - pbegin;

        /* Create the number. */
        snprintf(digit, TWOX_LEN + 4, "(%dx)", number + 1);
        /* Figure out how much more space this construct used than the old. */
        bufdiff = strlen(digit) - bufdiff;

        /* If we don't have room in the buffer, just fall through. */
        if (bufspace - bufdiff >= 0) {
          /*
           * new2x - a copy of the old string.  We overwrite the old string
           *         and that causes nasty overlap problems without the
           *         str_dup().
           */
          char *new2x = str_dup(twox);

          /* Overwrite the old position with the (x*) and the old string. */
          strcpy(pbegin, digit);
          strcat(pbegin, new2x);
          /* Adjust the buffers. */
          bufspace -= bufdiff;
          bufptr += bufdiff;
          free(new2x);
        } else /* Not enough room, let other code handle it. */
          error = TRUE;

        /* Be tidy, free everything. */
        free(digit);
      }
    } else if (*twox && bufspace < 4)
      /*
       * Not enough room for a (2*), which means the code below won't have
       * room to simply append it either but it will switch to a larger
       * buffer size or overflow down there that I don't want to duplicate.
       */
      error = TRUE;
    else if (*twox) {
      /*
       * We have a dupe and 4 bytes to spare. Simply overwrite the old
       * string and adjust buffers by 4.
       */
      char *new2x = str_dup(twox);
      sprintf(twox, "(2x)%s", new2x);
      bufspace -= 4;
      bufptr += 4;
      free(new2x);
    }
    /*
     * If error, we couldn't process the string here, allow the code
     * below to take a stab at it so we don't just toss the input.
     */
    if (!error)
      return 0;
  }



	//	if we have enough space, just write to buffer
	if (bufspace >= size) {
		strcpy(output + bufptr, txt);
		bufspace -= size;
		bufptr += size;
		return size;
	}
	
	//	If the text is too big to fit into even a large buffer, chuck the
	//	new text and switch to the overflow state.
	if ((size + bufptr) > (LARGE_BUFSIZE - 1)) {
		bufptr = -1;
		buf_overflows++;
		return 0;
	}
	buf_switches++;

#if USE_CIRCLE_SOCKET_BUF
	if (bufpool != NULL) {	//	if the pool has a buffer in it, grab it
		large_outbuf = bufpool;
		bufpool = bufpool->next;
	} else {				//	else create a new one
		CREATE(large_outbuf, struct txt_block, 1);
		CREATE(large_outbuf->text, char, LARGE_BUFSIZE);
		buf_largecount++;
	}

	strcpy(large_outbuf->text, output);	//	copy to big buffer
	output = large_outbuf->text;		//	make big buffer primary
#else
	//	Just request the buffer. Copy the contents of the old, and make it
	//	the primary buffer.
	large_outbuf = get_buffer(LARGE_BUFSIZE);
	strcpy(large_outbuf, output);
	output = large_outbuf;
#endif
	strcat(output, txt);						//	now add new text

	//	set the pointer for the next write
	bufptr = strlen(output);

	//	calculate how much space is left in the buffer
	bufspace = LARGE_BUFSIZE - 1 - bufptr;
	
	return size;
}


void Descriptor::Edit(Editor * new_editor)
{
	if (editor)
		new_editor->prev_editor = editor;

	editor = new_editor;
}


// This function returns the first object's pointer that was popped onto the
// stack.  The idea is that if we edit an object, then add more editors
// to the stack, we should still have a pointer to the first editor's target
// for comparison so we can tell the first object is being edited.
Ptr Descriptor::Editing(void)
{
	Editor * tmpeditor = editor;
	
	if (tmpeditor) {
		while (tmpeditor->prev_editor) {
			tmpeditor = tmpeditor->prev_editor;
		}
		return tmpeditor->Pointer();
	}

	return NULL;
}
