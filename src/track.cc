/***************************************************************************\
[*]		__		 __	___								___	| LexiMUD is a C++ MUD codebase [*]
[*]	 / /	___\ \/ (_) /\/\	/\ /\	/	 \ |				for Sci-Fi MUDs				[*]
[*]	/ /	/ _ \\	/| |/		\/ / \ \/ /\ / | Copyright(C) 1997-1999				[*]
[*] / /__|	__//	\| / /\/\ \ \_/ / /_//	| All rights reserved					 [*]
[*] \____/\___/_/\_\_\/		\/\___/___,'	 | Chris Jacobson (FearItself)	 [*]
[*]			A Creation of the AvP Team!			| fear@technologist.com				 [*]
[-]---------------------------------------+-------------------------------[-]
[*] File : track.c++																											[*]
[*] Usage: Tracking and path generation																	 [*]
\***************************************************************************/


#include "types.h"

#include "track.h"
#include "structs.h"
#include "interpreter.h"
#include "handler.h"
#include "utils.h"
#include "buffer.h"
#include "characters.h"
#include "comm.h"
#include "constants.h"
#include "event.h"
#include "rooms.h"

// int find_first_step(VNum src, char *target, Flags edgetype);
int find_first_step(VNum src, VNum target, Flags edgetype);


/*
 * You can define or not define TRACK_THOUGH_DOORS, depending on whether
 * or not you want track to find paths which lead through closed or
 * hidden doors. A setting of '#if 0' means to not go through the doors
 * while '#if 1' will pass through doors to find the target.
 */
#if 1
#define TRACK_THROUGH_DOORS	1
#endif


struct bfs_queue_struct {
	SInt16 room;
	char dir;
	struct bfs_queue_struct *next;
};

static struct bfs_queue_struct *queue_head = 0, *queue_tail = 0;

/* Utility macros */
#define MARK(room) (SET_BIT(ROOM_FLAGS(room), ROOM_BFS_MARK))
#define UNMARK(room) (REMOVE_BIT(ROOM_FLAGS(room), ROOM_BFS_MARK))
#define IS_MARKED(room) (ROOM_FLAGGED(room, ROOM_BFS_MARK))
#define TOROOM(x, y) (world[(x)].Direction(y)->to_room)
#define IS_CLOSED(x, y) (IS_SET(world[(x)].Direction(y)->exit_info, Exit::Closed))

#ifdef TRACK_THROUGH_DOORS
#define VALID_EDGE(x, y) (world[(x)].Direction(y) &&								 \
				(Room::Find(TOROOM(x, y))) &&	 \
				(!ROOM_FLAGGED(TOROOM(x, y), ROOM_NOTRACK)) && \
				(!IS_MARKED(TOROOM(x, y))))
#else
#define VALID_EDGE(x, y) (world[(x)].Direction(y) &&									\
				(real_room(TOROOM(x, y)) != NOWHERE) &&	 \
				(!IS_CLOSED(x, y)) &&										 \
				(!ROOM_FLAGGED(TOROOM(x, y), ROOM_NOTRACK)) && \
				(!IS_MARKED(TOROOM(x, y))))
#endif

void bfs_enqueue(SInt16 room, int dir)
{
	struct bfs_queue_struct *curr;

	CREATE(curr, struct bfs_queue_struct, 1);
	curr->room = room;
	curr->dir = dir;
	curr->next = 0;

	if (queue_tail) {
		queue_tail->next = curr;
		queue_tail = curr;
	} else
		queue_head = queue_tail = curr;
}


void bfs_dequeue(void)
{
	struct bfs_queue_struct *curr;

	curr = queue_head;

	if (!(queue_head = queue_head->next))
		queue_tail = 0;
	FREE(curr);
}


void bfs_clear_queue(void)
{
	while (queue_head)
		bfs_dequeue();
}


// Ported from Archipelago!
/* find_track_length will find the length of the shortest route
	 in steps, from src to target rooms.	Returns NO_PATH if route
	 exceeds 10 hops or any other error. Closed exits are weighted
	 as two hops.*/
int find_track_length(VNum src, VNum target, int max_len)
{
	int dir, len = 0;
	bool not_done = TRUE;

	if (src == target)
		return(0);

	while (not_done) {
		dir = find_first_step(src, target, 0);

		switch (dir) {
		case BFS_ERROR:
		case BFS_NO_PATH:
			len = -1;
			not_done = FALSE;
			break;
		case BFS_ALREADY_THERE:
			not_done = FALSE;
			break;
		default:
			if (IS_CLOSED(src,dir))
				len += 2;
			else
				len++;
			src = TOROOM(src,dir);
			if (len > max_len)
				return(-1);
			break;
		}
	}

	return(len);
}


#ifdef GOT_RID_OF_IT
/* 
 * find_first_step: given a source room and a target room, find the first
 * step on the shortest path from the source to the target.
 *
 * Intended usage: in mobile_activity, give a mob a dir to go if they're
 * tracking another mob or a PC.  Or, a 'track' skill for PCs.
 */
int find_first_step(VNum src, char *target, Flags edgetype) {
	SInt32	dir = -1;
	Path *	path;
	Flags edge;
	
	if (edgetype)	edge = edgetype;
	else			edge = Path::Global | Path::ThruDoors;
	
	if (src < 0 || src > world.Top() || !target) {
		log("SYSERR: find_first_step(RNum src = %d, char *target = %p): Illegal value.", src, target);
		return dir;
	}

	path = Path::ToName(src, target, 200, edge);

	if (path) {
		dir = path->moves[path->count - 1].dir;
		delete path;
	}
	return dir;
}
#endif


int find_first_step(VNum src, VNum target, Flags edgetype) {
	SInt32	dir = -1;
	Flags	edge;
	Room	*room;
	
	if (edgetype)	edge = edgetype;
	else			edge = Path::Global | Path::ThruDoors;
	
	if (src < 0 || src > world.Top() || target < 0 || target > world.Top()) {
		log("SYSERR: find_first_step(RNum src = %d, RNum target = %d): Illegal value.", src, target);
		return dir;
	}

	if (src == target)	return BFS_ALREADY_THERE;

	MapIterator<VNum, Room>		iter(world);
	while ((room = iter.Next())) {
		UNMARK(room->Virtual());
	}

	MARK(src);

	/* first, enqueue the first steps, saving which direction we're going. */
	for (dir = 0; dir < NUM_OF_DIRS; dir++)
		if (VALID_EDGE(src, dir)) {
			MARK(TOROOM(src, dir));
			bfs_enqueue(TOROOM(src, dir), dir);
		}

	/* now, do the classic BFS. */
	while (queue_head) {
		if (queue_head->room == target) {
			dir = queue_head->dir;
			bfs_clear_queue();
			return dir;
		} else {
			for (dir = 0; dir < NUM_OF_DIRS; dir++)
				if (VALID_EDGE(queue_head->room, dir)) {
					MARK(TOROOM(queue_head->room, dir));
					bfs_enqueue(TOROOM(queue_head->room, dir), queue_head->dir);
				}
			bfs_dequeue();
		}
	}

	return BFS_NO_PATH;
}


ACMD(do_path) {
	ch->Send("This command is temporarily disabled.\r\n");
#if 0
	char *		name = get_buffer(MAX_INPUT_LENGTH);
	char *		directs = get_buffer(MAX_STRING_LENGTH);
	Path *		path = NULL;
	char *		dir;
	SInt32		next;
	
	one_argument(argument, name);
	
	if (!*name)
		ch->Send("Find path to whom?\r\n");
	else if (*name == '.')
		path = Path::ToFullName(IN_ROOM(ch), name + 1, 200, Path::Global | Path::ThruDoors);
	else
		path = Path::ToName(IN_ROOM(ch), name, 200, Path::ThruDoors);
		
	if (path) {
		dir = directs;
		for (next = path->count - 1; next >= 0; next--)
			*dir++ = dirs[path->moves[next].dir][0];
		*dir++ = '\r';
		*dir++ = '\n';
		*dir = '\0';
		
		ch->Send("Shortest route to %s:\r\n%s", path->victim->Name(), directs);
	} else
		ch->Send("Can't find target.\r\n");
	
	release_buffer(name);
	release_buffer(directs);
#endif
}


class TrackEvent : public ActionEvent {
public:
						TrackEvent(time_t when, Character &tch, const char *str) :
						ActionEvent(when, tch), arg(str) {}
					
	string					arg;

	time_t					Execute(void);
};


time_t			TrackEvent::Execute(void)
{
	Character 		*vict;
	IDNum			target;
    VNum 			room = NOWHERE;
	SInt8			dir, diceroll;

	if (PURGED(ch))		return 0;

	diceroll = CHAR_SKILL(ch).RollSkill(ch, SKILL_TRACKING);

	if (diceroll < 0) {
		/* Find a random direction. :) */
		do {
			dir = Number(0, NUM_OF_DIRS - 1);
		} while (!CAN_GO(ch, dir));
		sprintf(buf, "You sense a trail %s from here!\r\n", dirs[dir]);
		ch->Send(buf);
		return 0;
	}

	if (!(vict = get_char_vis(ch, arg.c_str(), FIND_CHAR_WORLD))) {
		ch->Send("No one is around by that name.\r\n");
		return 0;
	}

	/* They passed the skill check. */
	dir = find_first_step(IN_ROOM(ch), IN_ROOM(vict), 0);

	switch (dir) {
	case -1:
		if (IN_ROOM(ch) == IN_ROOM(vict))
			ch->Send("You're already in the same room!!\r\n");
		else
			ch->Sendf("You can't sense a trail to %s from here.\r\n", HMHR(vict));
		break;
	default:
		ch->Sendf("You sense a trail %s from here!\r\n", dirs[dir]);
		CHAR_SKILL(ch).RollToLearn(ch, SKILL_TRACKING, diceroll);
		break;
	}

	return 0;
}



/********************************************************
* Functions and Commands which use the above functions. *
********************************************************/

ACMD(do_track)
{
	Character *vict;
	int dir;
	SInt16 diceroll;

	if (GET_ACTION(ch)) {
		act("You are already busy!", FALSE, ch, NULL, NULL, TO_CHAR);
		return;
	}

	/* The character must have the track skill. */
	if (!GET_SKILL(ch, SKILL_TRACKING)) {
		ch->Send("You have no idea how.\r\n");
		return;
	}
	one_argument(argument, arg);
	if (!*arg) {
		ch->Send("Whom are you trying to track?\r\n");
		return;
	}

	new TrackEvent(2 RL_SEC, *ch, arg);

	ch->Send("You carefully look over the surrounding area.\r\n");
	act("$n carefully looks over the surroundings.", TRUE, ch, NULL, NULL, TO_ROOM);
}
