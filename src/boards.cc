/***************************************************************************\
[*]    __     __  ___                ___  | LexiMUD is a C++ MUD codebase [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ |        for Sci-Fi MUDs        [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / | Copyright(C) 1997-1999        [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  | All rights reserved           [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   | Chris Jacobson (FearItself)   [*]
[*]      A Creation of the AvP Team!      | fear@technologist.com         [*]
[-]---------------------------------------+-------------------------------[-]
[*] Based on CircleMUD, Copyright 1993-94, and DikuMUD, Copyright 1990-91 [*]
[*] Response and Threading code by Trevor Man Copyright 1997              [*]
[-]---------------------------------------+-------------------------------[-]
[*] File : boards.cp                                                      [*]
[*] Usage: Message Board Engine                                           [*]
\***************************************************************************/


#include "types.h"

#include <dirent.h>

#include "objects.h"
#include "descriptors.h"
#include "characters.h"
#include "rooms.h"
#include "utils.h"
#include "buffer.h"
#include "comm.h"
#include "db.h"
#include "boards.h"
#include "interpreter.h"
#include "handler.h"
#include "find.h"
#include "files.h"


LList<Board *>	board_info;

void strip_string(char *buffer);

ACMD(do_respond);

void Board::InitBoards(void) {
	int n,o, board_vnum;
	struct dirent **namelist;

	n = scandir(BOARD_DIRECTORY, &namelist, 0, alphasort);
	if (n < 0) {
		log("Funny, no board files found.");
		return;
	}
	for (o = 0; o != n; o++) {
		if(strcmp("..",namelist[o]->d_name) && strcmp(".",namelist[o]->d_name)) {
			sscanf(namelist[o]->d_name, "%d", &board_vnum);
			Board::Create(board_vnum);
		}
	}
	//	just before we close, lets look at boards huh?
	Board::LookAt(); 
}


Board * Board::Create(VNum vnum) {
//	struct board_msg_info *bsmg;
	SInt32	boards = 0,
			t_messages = 0,
			error = 0;
	SInt32	t[4], mnum, poster,timestamp;
	IDNum	id;
	FILE 	*fl;
	Board 	*temp_board=NULL;
//	struct board_memory_type *memboard, *list;
	Object *obj;
	char *filename, *buf;
	
	filename = get_buffer(MAX_INPUT_LENGTH);
	buf = get_buffer(MAX_INPUT_LENGTH);
	
	sprintf(filename, BOARD_DIRECTORY "%d.brd", vnum);
	
	if(!(fl = fopen(filename, "r"))) {
		/* this is for boards which exist as an object, but don't have a file yet */
		/* Ie, new boards */

		if(!(fl = fopen(filename,"w"))) {
			log("SYSERR: Error while creating new board file '%s'", filename);
		} else {
			obj = Object::Read(vnum);
			
			if (!obj) {
				fclose(fl);
				remove(filename);
				release_buffer(filename);
				release_buffer(buf);
				return NULL;
			}
			
			temp_board = new Board;
			temp_board->readLevel = GET_OBJ_VAL(obj, 0);
			temp_board->writeLevel = GET_OBJ_VAL(obj, 1);
			temp_board->removeLevel = GET_OBJ_VAL(obj, 2);
			temp_board->vnum = vnum;
			temp_board->lastId = 0;
			obj->Extract();
			fprintf(fl,"Board File\n%d %d %d %d\n", temp_board->readLevel,
					temp_board->writeLevel, temp_board->removeLevel, temp_board->messages.Count());
			fclose(fl);

			board_info.Add(temp_board);
			release_buffer(buf);
			release_buffer(filename);
			return temp_board;
		}
	} else {
		get_line(fl,buf);
		
		if(!(str_cmp("Board File", buf))) {
			boards++;
			
			temp_board = new Board;
			temp_board->vnum = vnum;
			get_line(fl, buf); 
			if (sscanf(buf,"%d", t) != 1)
				error=1;
			if (!Object::Find(vnum)) {
				log("Ignoring non-existant board file: %d", vnum);
				if (temp_board)
					delete temp_board;
				release_buffer(buf);
				release_buffer(filename);
				return NULL;    /* realize why I do this yes? */
			}
			obj = Object::Read(vnum);

			temp_board->readLevel = GET_OBJ_VAL(obj, 0);
			temp_board->writeLevel = GET_OBJ_VAL(obj, 1);
			temp_board->removeLevel = GET_OBJ_VAL(obj, 2);	
			obj->Extract();
			if (error) {
				log("Parse error in board %d - attempting to continue", temp_board->vnum);
				/* attempting board ressurection */

				if(t[0] < 0 || t[0] > MAX_TRUST)		temp_board->readLevel = MAX_TRUST;
				if(t[1] < 0 || t[1] > MAX_TRUST)		temp_board->writeLevel = MAX_TRUST;
				if(t[2] < 0 || t[2] > MAX_TRUST)		temp_board->removeLevel = MAX_TRUST;
			}
//			list = NULL;	/* or BOARD_MEMORY(temp_board) but this should be null anyway...*/
			while(get_line(fl,buf)) {
				/* this is the loop that reads the messages and also the message read info stuff */
				if (*buf == 'S') {
					sscanf(buf,"S %d %d %d ", &mnum, &poster, &timestamp);
//					CREATE(memboard, struct board_memory_type, 1);
//					MEMORY_READER(memboard)=poster;
//					MEMORY_TIMESTAMP(memboard)=timestamp;
//					bmsg = BOARD_MESSAGES(temp_board);
//					sflag = 0;
					//	Check and see if memory is still valid
//					while (bmsg && !sflag) {
//						if (MESG_TIMESTAMP(bmsg) == MEMORY_TIMESTAMP(memboard) &&
//								(mnum == ((MESG_TIMESTAMP(bmsg) % 301 + MESG_POSTER(bmsg) %301) % 301))) {
//							sflag = 1;
							/* probably ought to put a flag saying that
							   memory should be deleted because the reader
							   no longer exists on the mud, but it will
							   figure it out in a few iterations. Let it be
							   happy, and let me be lazy */
//						}
//						bmsg = MESG_NEXT(bmsg);
//					}
//					if (sflag) {
//						if (BOARD_MEMORY(temp_board, mnum)) {
//							list = BOARD_MEMORY(temp_board, mnum);
//							BOARD_MEMORY(temp_board, mnum) = memboard;
//							MEMORY_NEXT(memboard) = list;
//						} else {
//							BOARD_MEMORY(temp_board, mnum) = memboard;
//							MEMORY_NEXT(memboard) = NULL;
//						}
//					} else {
//						FREE(memboard);
//					}
//					if(BOARD_MEMORY(temp_board,mnum)) {
//						list=BOARD_MEMORY(temp_board,mnum);
//						BOARD_MEMORY(temp_board,mnum)=memboard;
//						MEMORY_NEXT(memboard)=list;
//					} else {
//						BOARD_MEMORY(temp_board,mnum)=memboard;
//						MEMORY_NEXT(memboard)=NULL;
//					}
				} else if (*buf == '#') {
					id = atoi(buf + 1);
					Board::Parse(fl, temp_board, filename, id);
					if (id > temp_board->lastId)
						temp_board->lastId = id;
				}
			}
			
			/* add it to the list. */

			board_info.Add(temp_board);
		}
		fclose(fl);
	}

	release_buffer(buf);
	release_buffer(filename);
	return temp_board;
}


void Board::Parse( FILE *fl, Board *t_board, char *filename, UInt32 id) {
	BoardMessage *tmsg;
	char *subject = get_buffer(MAX_INPUT_LENGTH);
	char *buf = get_buffer(MAX_STRING_LENGTH);
	
	tmsg = new BoardMessage;
	
	MESG_ID(tmsg) = id;
	
	fscanf(fl, "%d\n", &(MESG_POSTER(tmsg)));
	fscanf(fl, "%ld\n", &(MESG_TIMESTAMP(tmsg)));

	get_line(fl,subject);

	MESG_SUBJECT(tmsg)=str_dup(subject);
	MESG_DATA(tmsg)=fread_string(fl,buf, filename);

	t_board->messages.Append(tmsg);

	release_buffer(subject);
	release_buffer(buf);
}


void Board::LookAt(void) {
	SInt32	messages = 0;
	Board *	tboard;
	
	LListIterator<Board *>	iter(board_info);
	while ((tboard = iter.Next()))
		messages += tboard->messages.Count();
	
	log("There are %d boards located; %d messages", board_info.Count(), messages);
}


int Board::Show(VNum vnum, Character *ch) {
	Board *			thisboard;
	BoardMessage *	message;
	char *tmstr;
	int msgcount=0, yesno = 0;
	char *buf, *buf2, *buf3;
	const char *name;

	/* board locate */
	thisboard = Board::Find(vnum);
	if (!thisboard) {
		log("Creating new board - board #%d", vnum);
		thisboard = Board::Create(vnum);
	}
	
	if (GET_LEVEL(ch) < thisboard->readLevel) {
		ch->Send("*** Security code invalid. ***\r\n");
		return 1;
	}
	
	/* send the standard board boilerplate */

	buf = get_buffer(MAX_STRING_LENGTH);
	buf2 = get_buffer(MAX_STRING_LENGTH);

	strcpy(buf, "This is a bulletin board.\r\n"
			"Usage: READ/REMOVE <messg #>, WRITE <header>.\r\n");
	SInt32	numMessages = thisboard->messages.Count();
	if (numMessages == 0) {
		strcat(buf, "The board is empty.\r\n");
		ch->Send(buf);
		release_buffer(buf);
		release_buffer(buf2);
		return 1;
	} else {
		sprintf(buf + strlen(buf), "There %s %d message%s on the board.\r\n",
				(numMessages == 1) ? "is" : "are",
				numMessages,
				(numMessages == 1) ? "" : "s");
	}
	
	buf3 = get_buffer(MAX_STRING_LENGTH);
	
	LListIterator<BoardMessage *> message_iter(thisboard->messages);
	
	while (message_iter.Next())	;

	while ((message = message_iter.Prev())) {
		tmstr = asctime(localtime( &MESG_TIMESTAMP(message)));
		*(tmstr + strlen(tmstr) - 1) = '\0';
		
		name = get_name_by_id(MESG_POSTER(message));
		strcpy(buf3, name ? name : "<UNKNOWN>");
		sprintf(buf2,"(%s)", CAP(buf3));
		
		sprintf(buf + strlen(buf), "[%5d] : %6.10s %-23s :: %s\r\n", /* yesno ? 'X' : ' ', */
				MESG_ID(message), tmstr, buf2, MESG_SUBJECT(message) ? MESG_SUBJECT(message) : "No Subject");
	}
	release_buffer(buf3);
	release_buffer(buf2);
	page_string(ch->desc, buf, true);
	release_buffer(buf);
	return 1;
}


int Board::DisplayMessage(VNum vnum, Character * ch, IDNum arg) {
	Board *thisboard;
	BoardMessage *message;
	char *tmstr;
	char *buf, *buf2;
	const char *name;
//	int msgcount,mem,sflag;
//	struct board_memory_type *mboard_type, *list;
 
	/* guess we'll have to locate the board now in the list */
 	
 	thisboard = Board::Find(vnum);
	
	if (!thisboard) {
		log("Creating new board - board #%d", vnum);
		thisboard = Board::Create(vnum);
	}
	
	if (GET_LEVEL(ch) < thisboard->readLevel) {
		ch->Send("*** Security code invalid. ***\r\n");
		return 1;
	}
    
	if (!thisboard->messages.Count()) {
		ch->Send("The board is empty!\r\n");
		return 1;
	}
	
	/* now we locate the message.*/  
	if (arg < 1) {
		ch->Send("You must specify the (positive) number of the message to be read!\r\n");
		return 1;
	}
	
	LListIterator<BoardMessage *>	messageIter(thisboard->messages);
	while ((message = messageIter.Next()))
		if (MESG_ID(message) == arg)
			break;
	
	if (!message) {
		ch->Send("That message exists only in your imagination.\r\n");
		return 1;
	}
	/* Have message, lets add the fact that this player read the mesg */
	
//	mem = ((MESG_TIMESTAMP(message)%301 + MESG_POSTER(message)%301)%301);

	/*make the new node */
//	CREATE(mboard_type, struct board_memory_type, 1);
//	MEMORY_READER(mboard_type)=GET_IDNUM(ch);
//	MEMORY_TIMESTAMP(mboard_type)=MESG_TIMESTAMP(message);
//	MEMORY_NEXT(mboard_type)=NULL;

	/* make sure that the slot is open, free */
//	list=BOARD_MEMORY(thisboard,mem);
//	sflag = 1;
//	while (list && sflag) {
//		if ((MEMORY_READER(list) == MEMORY_READER(mboard_type)) &&
//				(MEMORY_TIMESTAMP(list) == MEMORY_TIMESTAMP(mboard_type))
//			sflag = 0;
//		list = MEMORY_NEXT(list);
//	}
//	if (sflag) {
//		list = BOARD_MEMORY(thisboard, mem);
//		BOARD_MEMORY(thisboard,mem) = mboard_type;
//		MEMORY_NEXT(mboard_type)=list;
//	} else if (mboard_type)
//		FREE(mboard_type);

	/* before we print out the message, we may as well restore a human readable timestamp. */
	tmstr = asctime(localtime(&MESG_TIMESTAMP(message)));
	*(tmstr + strlen(tmstr) - 1) = '\0';

	buf = get_buffer(MAX_STRING_LENGTH);
	buf2 = get_buffer(MAX_STRING_LENGTH);
	name = get_name_by_id(MESG_POSTER(message));
	strcpy(buf2, name ? name : "<UNKNOWN>");
	sprintf(buf,"Message %d : %6.10s (%s) :: %s\r\n\r\n%s\r\n",
			MESG_ID(message), tmstr, CAP(buf2),
			MESG_SUBJECT(message) ? MESG_SUBJECT(message) : "No Subject",
			MESG_DATA(message) ? MESG_DATA(message) : "Looks like this message is empty.");
	page_string(ch->desc, buf, true);
	thisboard->Save();
	release_buffer(buf);
	release_buffer(buf2);
	return 1;
}
 
int Board::RemoveMessage(VNum vnum, Character *ch, IDNum arg) {
	Board *			thisboard;
	BoardMessage *	message;
	Descriptor *	d;
	char *buf;
	
	
	thisboard = Board::Find(vnum);
	
	if (!thisboard) {
		ch->Send("Error: Your board could not be found. Please report.\n");
		log("Error in Board_remove_msg - board #%d", vnum);
		return 0;
	}
	
	if (arg < 1) {
		ch->Send("You must specify the (positive) number of the message to be read!\r\n");
		return 1;
	}
	
	LListIterator<BoardMessage *>	messageIter(thisboard->messages);
	while ((message = messageIter.Next()))
		if (MESG_ID(message) == arg)
			break;

	if(!message) {
		ch->Send("That message exists only in your imagination.\r\n");
		return 1;
	}

	if(GET_IDNUM(ch) != MESG_POSTER(message) && GET_LEVEL(ch) < thisboard->removeLevel) {
		ch->Send("*** Security code invalid. ***\r\n");
		return 1;
	}

	//	perform check for mesg in creation
	LListIterator<Descriptor *>		descIter(descriptor_list);
	while ((d = descIter.Next())) {
		if ((STATE(d) == CON_PLAYING) && d->Edit() && d->Editing() == &(MESG_DATA(message))) {
			ch->Send("At least wait until the author is finished before removing it!\r\n");
			return 1;
		}
	}

	/* everything else is peachy, kill the message */
	thisboard->messages.Remove(message);
	
	if (MESG_SUBJECT(message))	free(MESG_SUBJECT(message));
	if (MESG_DATA(message))		free(MESG_DATA(message));
	delete message;

	ch->Send("Message removed.\r\n");
	buf = get_buffer(MAX_INPUT_LENGTH);
	sprintf(buf, "$n just removed message %d.", arg);
	act(buf, FALSE, ch, 0, 0, TO_ROOM);
	release_buffer(buf);
	thisboard->Save();
	return 1; 
}
 

SInt32 Board::Save(VNum vnum) {
	Board *			thisboard;

	thisboard = Board::Find(vnum);
	
	if (!thisboard) {
		log("Creating new board - board #%d", vnum);
		thisboard = Board::Create(vnum);
	}
	return thisboard->Save();
}


SInt32 Board::Save(void) {
	BoardMessage *	message;
//	struct board_memory_type *memboard;
	Object *obj;
	FILE *fl;
	int i=1, counter = 0;

	/* before we save to the file, lets make sure that our board is updated */
	sprintf(buf, BOARD_DIRECTORY "%d.brd", vnum);

	obj = Object::Read(vnum);
	if (!obj) {
		remove(buf);
		return 0;
	}
	readLevel = GET_OBJ_VAL(obj, 0);
	writeLevel = GET_OBJ_VAL(obj, 1);
	removeLevel = GET_OBJ_VAL(obj, 2);
	obj->Extract();

	if (!(fl = fopen(buf, "wb"))) {
		perror("SYSERR: Error writing board");
		return 0;
	}
	//	now we write out the basic stats of the board

	fprintf(fl,"Board File\n%d\n", messages.Count());

	//	now write out the saved info...
	LListIterator<BoardMessage *>	messageIter(messages);
	while ((message = messageIter.Next())) {
		strcpy(buf, MESG_DATA(message) && *MESG_DATA(message) ? MESG_DATA(message) : "Empty!");
		strip_string(buf);
		fprintf(fl,	"#%d\n"
					"%d\n"
					"%ld\n"
					"%s\n"
					"%s~\n",
				MESG_ID(message), MESG_POSTER(message), MESG_TIMESTAMP(message),
				MESG_SUBJECT(message) && *MESG_SUBJECT(message)? MESG_SUBJECT(message) : "Empty", buf);
//		for (counter = 0; counter != 301; counter++) {
//		memboard = BOARD_MEMORY(thisboard, counter);
//		while (memboard) {
//			fprintf(fl, "S %d %d %d\n", counter, MEMORY_READER(memboard), MEMORY_TIMESTAMP(memboard));
//			memboard = MEMORY_NEXT(memboard);
//		}
	}
	fclose(fl);
	return 1;
}


void Board::WriteMessage(VNum vnum, Character *ch, char *arg) {
	Board 			*thisboard;
	BoardMessage	*message;
	Editor			*tmpeditor = NULL;

	if(IS_NPC(ch)) {
		ch->Send("Orwellian police thwart your attempt at free speech.\r\n");
		return;
	}

	/* board locate */
	thisboard = Board::Find(vnum);
	
	if (!thisboard) {
		ch->Send("Error: Your board could not be found. Please report.\n");
		log("Error in Board::WriteMessage(vnum = %d, ch = %s, arg = %s)", vnum, ch->RealName(), arg);
		return;
	}

	if (GET_LEVEL(ch) < thisboard->writeLevel) {
		ch->Send("You are not cleared to write on this board.\r\n");
		return;
	}

	if (!*arg)
		strcpy(arg, "No Subject");
	else {
		skip_spaces(arg);
		delete_doubledollar(arg);
		arg[81] = '\0';
	}

//	if (!*arg) {
//		ch->Send("We must have a headline!\r\n");
//		return;
//	}

	message = new BoardMessage;
	MESG_ID(message) = ++thisboard->lastId;
	MESG_POSTER(message)=GET_IDNUM(ch);
	MESG_TIMESTAMP(message)=time(0);
	MESG_SUBJECT(message) = str_dup(arg);
	MESG_DATA(message) = NULL;

	if (thisboard->messages.Count() > 75)
		thisboard->messages.Remove(thisboard->messages.Bottom());
	thisboard->messages.Add(message);

	ch->Send("Write your message.  (/s saves /h for help)\r\n\r\n");
	act("$n starts to write a message.", TRUE, ch, 0, 0, TO_ROOM);

	SET_BIT(PLR_FLAGS(ch), PLR_WRITING);
	tmpeditor = new BoardEditor(ch->desc, &MESG_DATA(message), MAX_MESSAGE_LENGTH, vnum);
	ch->desc->Edit(tmpeditor);
}


void Board::RespondMessage(VNum vnum, Character *ch, IDNum mnum) {
	Board 			*thisboard;
	BoardMessage *message, *other;
	Editor			*tmpeditor = NULL;
	char *buf;
	int gcount=0;

	thisboard = Board::Find(vnum);
	if (!thisboard) {
		ch->Send("Error: Your board could not be found. Please report.\n");
		log("Error in Board::RespondMessage(vnum = %d, ch = %s, mnum = %d)", vnum, ch->RealName(), mnum);
		return;
	}

	if ((GET_LEVEL(ch) < thisboard->writeLevel) || (GET_LEVEL(ch) < thisboard->readLevel)) {
		ch->Send("You are not cleared to write on this board.\r\n");
		return;
	}

	//	locate message to be repsponded to
	LListIterator<BoardMessage *>	messageIter(thisboard->messages);
	while ((other = messageIter.Next()))
		if (MESG_ID(other) == mnum)
			break;
	
	if(!other) {
		ch->Send("That message exists only in your imagination.\r\n");
		return;
	}
	
	buf = get_buffer(MAX_STRING_LENGTH);
	
	message = new BoardMessage;
	MESG_ID(message) = ++thisboard->lastId;
	MESG_POSTER(message) = GET_IDNUM(ch);
	MESG_TIMESTAMP(message) = time(0);
	sprintf(buf, "Re: %s", MESG_SUBJECT(other));
	MESG_SUBJECT(message) = str_dup(buf);
	MESG_DATA(message) = NULL;

	if (thisboard->messages.Count() > 75)
		thisboard->messages.Remove(thisboard->messages.Bottom());
	thisboard->messages.Add(message);
	
	ch->Send("Write your message.  (/s saves /h for help)\r\n\r\n");
	act("$n starts to write a message.", TRUE, ch, 0, 0, TO_ROOM);

//	if (!IS_NPC(ch))
//		SET_BIT(PLR_FLAGS(ch), PLR_WRITING);

	sprintf(buf,"\t------- Quoted message -------\r\n"
				"%s"
				"\t------- End Quote -------\r\n",
			MESG_DATA(other));

	MESG_DATA(message) = str_dup(buf);

	SET_BIT(PLR_FLAGS(ch), PLR_WRITING);
	tmpeditor = new BoardEditor(ch->desc, &MESG_DATA(message), MAX_MESSAGE_LENGTH, vnum);
	ch->desc->Edit(tmpeditor);
	
	release_buffer(buf);
	return;
}


/*
int mesglookup(struct board_msg_info *message, Character *ch, struct board_info_type *board) {
	int mem;
	struct board_memory_type *mboard_type;
 
	mem = ((MESG_TIMESTAMP(message)%301 + MESG_POSTER(message)%301)%301);

	// now, we check the mem slot. If its null, we return no, er.. 0..
	// if its full, we double check against the timestamp and reader - 
	// mislabled as poster, but who cares... if they're not true, we
	// go to the linked next slot, and repeat
	mboard_type=BOARD_MEMORY(board,mem);
	while(mboard_type) {
		if(MEMORY_READER(mboard_type)==GET_IDNUM(ch) &&
				MEMORY_TIMESTAMP(mboard_type)==MESG_TIMESTAMP(message)) {
			return 1;
		} else {
			mboard_type=MEMORY_NEXT(mboard_type);
		}
	}
	return 0;
}
*/

Board *Board::Find(VNum vnum) {
	Board *	board;
	
	LListIterator<Board *>	boardIter(board_info);
 	while ((board = boardIter.Next()))
		if (board->vnum == vnum)
			break;
	
	return board;
}


ACMD(do_respond) {
	Object *	obj;

	if(IS_NPC(ch)) {
		ch->Send("As a mob, you never bothered to learn to read or write.\r\n");
		return;
	}
	
	if(!(obj = get_obj_in_list_type(ITEM_BOARD, ch->contents)))
		obj = get_obj_in_list_type(ITEM_BOARD, world[IN_ROOM(ch)].contents);

	if (obj) {
		char *	arg = get_buffer(MAX_INPUT_LENGTH);
		SInt32	mnum = 0;
		
		argument = one_argument(argument, arg);
		if (!*arg)									ch->Send("Respond to what?\r\n");
		else if (isname(arg, obj->Name("")))		ch->Send("You cannot reply to an entire board!\r\n");
		else if (!(mnum = atoi(arg)))				ch->Send("You must reply to a message number.\r\n");
		else										Board::RespondMessage(obj->Virtual(), ch, mnum);
		release_buffer(arg);
	} else
		ch->Send("There is no board here!\r\n");
}
