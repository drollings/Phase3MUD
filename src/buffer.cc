/***************************************************************************\
[*]    __     __  ___                ___  | LexiMUD is a C++ MUD codebase [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ |        for Sci-Fi MUDs        [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / | Copyright(C) 1997-1999        [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  | All rights reserved           [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   | Chris Jacobson (FearItself)   [*]
[*]      A Creation of the AvP Team!      | fear@technologist.com         [*]
[-]---------------------------------------+-------------------------------[-]
[*] File : buffer.c++                                                     [*]
[*] Usage: Advanced Buffer System by George Greer                         [*]
\***************************************************************************/


#define _BUFFER_C_


#include "structs.h"
#include "utils.h"
#include "buffer.h"
#include "interpreter.h"
#include "characters.h"
#include "comm.h"

#ifdef USE_CIRCLE_BUFFERS
char buf[MAX_STRING_LENGTH];
char buf1[MAX_STRING_LENGTH];
char buf2[MAX_STRING_LENGTH];
char buf3[MAX_STRING_LENGTH];
char arg[MAX_STRING_LENGTH];
#endif


enum {
	BUF_NUKE =		(1 << 0),	//	Use memset() to clear after use
	BUF_CHECK =		(1 << 1),	//	Report how much is used
	BUF_STAYUP =	(1 << 2),	//	Try to stay up after an overflow
	BUF_DETAIL =	(1 << 3),	//	Output detail
	BUF_VERBOSE =	(1 << 4),	//	Output extremely detailed info
	BUF_FASTLOCK =	(1 << 5)	//	Use fast mutex locking (no error checking)
};
Flags	Buffer::options = BUF_NUKE | BUF_STAYUP;

const char * const Buffer::optionsDesc[] = {
	"NUKE", "CHECK", "STAYUP", "DETAIL", "VERBOSE", "FASTLOCK", "\n"
};

//	1 = Check every pointer in every buffer allocated on every new allocation.
//	0 = Recommended, unless you are tracking memory corruption at the time.
//	NOTE: This only works under Linux.  Do not try to use with anything else!
#define BUFFER_PARANOIA		0

/*
 * 1 = Use pvalloc() to allocate pages to protect with mprotect() to make
 *	sure no one fiddles with structures they shouldn't.  Good for the
 *	BUFFER_SNPRINTF support and as a much more noticeable addition to
 *	the paranoia code just above.  It is NOT recommended to use this
 *	option along with BUFFER_MEMORY unless you have a lot of memory.
 *	This is also incompatible with Electric Fence and anything else
 *	which overrides malloc() at link time.  This is tested only on
 *	the GNU C Library v2.
 * 0 = Recommended unless you have memory overruns somewhere.
 */
#define BUFFER_PROTECT		0

namespace {

//	MAGIC_NUMBER - Any arbitrary character unlikely to show up in a string.
//		In this case, we use a control character. (0x6)
//	BUFFER_LIFE - How long can a buffer be unused before it is free()'d?
//	LEASE_LIFE - The amount of time a buffer can be used before it is
//		considered forgotten and released automatically.
const unsigned char MAGIC_NUMBER =	0x6;
const int BUFFER_LIFE =	600 RL_SEC;		//	5 minutes
const int LEASE_LIFE = 	60 RL_SEC;		//	1 minute


//	MEMORY ARRAY OPTIONS
//	--------------------
//	If you change the maximum or minimum allocations, you'll probably also want
//	to change the hash number.  See the 'bufhash.c' program for determining a
//	decent one.  Think prime numbers.
//
//	MAX_ALLOC - The largest buffer magnitude expected.
//	MIN_ALLOC - The lowest buffer magnitude ever handed out.  If 20 bytes are
//		requested, we will allocate this size buffer anyway.
//	BUFFER_HASH - The direct size->array hash mapping number. This is _not_ an
//		arbitrary number.
const int MAX_ALLOC = 	16;		//	2**16 = 65536 bytes/
const int MIN_ALLOC =	6;		//	2**6  = 64 bytes
const int BUFFER_HASH =	11;		//	Not arbitrary!
const int CACHE_SIZE =	(3 + (BUFFER_MEMORY * 22));	//	Cache is 25 in that case
}

//	End of configurables section.
// ----------------------------------------------------------------------------


#if BUFFER_PROTECT
#include <sys/mman.h>
#include <malloc.h>
#endif

#if BUFFER_PARANOIA && !defined(__linux__)
#undef BUFFER_PARANOIA
#define BUFFER_PARANOIA 0
#endif

#if BUFFER_PARANOIA == 0
#define valid_pointer(ptr)	true
#else
#define valid_pointer(ptr)	(((unsigned long)(ptr) & 0x48000000))
#endif

#if BUFFER_THREADED
//#define _MIT_POSIX_THREADS
//#include <pthread.h>		//	All threaded functions

namespace {
pthread_mutex_t listmutex = PTHREAD_MUTEX_INITIALIZER;
//static pthread_mutexattr_t	mutex_opt[2];
pthread_t thread = 0;
}

#define lock_buffers()		(Buffer::ListLock(__FUNCTION__, __LINE__))
#define unlock_buffers()	(Buffer::ListUnlock(__FUNCTION__, __LINE__))
#define LOCK(buf, type)		buf->Lock((type), __FUNCTION__, __LINE__)
#define UNLOCK(buf)			buf->Unlock(__FUNCTION__, __LINE__)
#define LOCKED(buf)			((buf)->locked)
#else
#define lock_buffers()		/* Not necessary. */
#define unlock_buffers()	/* Not necessary. */
#define LOCK(buf, type)		/* Not necessary. */
#define UNLOCK(buf)			/* Not necessary. */
#define LOCKED(buf)			false
#endif


const UInt32 BUFFER_MINSIZE	=	(1 << MIN_ALLOC);	//	Minimum size, in bytes.
const UInt32 BUFFER_MAXSIZE = 	(1 << MAX_ALLOC);	//	Maximum size, in bytes.
const SInt32 B_FREELIFE	= (BUFFER_LIFE / PULSE_BUFFER);
const SInt32 B_GIVELIFE = (LEASE_LIFE / PULSE_BUFFER);


/* ------------------------------------------------------------------------ */


//	Global variables.
//	buffers - Head of the main buffer allocation linked list.
//	buffer_array - The minimum and maximum array ranges for 'buffers'.
namespace {
SInt32		buffer_array[2] = {0,0};
enum {
	BUF_MAX = 0,
	BUF_MIN = 1
//	BUF_RANGE = 2
};
}

/* External functions and variables. */
extern int circle_shutdown;
extern int circle_reboot;

/* Private functions. */
ACMD(do_overflow);
ACMD(do_buffer);
namespace {
int get_magnitude(UInt32 number);
#if BUFFER_MEMORY
static void really_free(void *ptr);
#else
#define really_free(x)	free(x)
#endif
}

//	Useful macros.
#define buffer_hash(x)		((x) % BUFFER_HASH)
#define USED(x)				((x)->who)
#define SIZE_T_HIGHBIT		(sizeof(size_t) * CHAR_BIT - 1)

/* ------------------------------------------------------------------------ */

/*
 * Private: count_bits(number to count)
 * Find out how many bits are in a number.  Used to see if it can be
 * rounded into a power of 2.
 */
static int count_bits(size_t y) {
	unsigned int i;
	int count = 0;

	for (i = 0; i < sizeof(size_t) * 8; i++)
		count += (y & (1 << i)) >> i;	/* Avoiding a jump to get 0 or 1. */
	return count;
}

/*
 * Private: round_to_pow2(number to round)
 * Converts a number to the next highest power of two.
 * 0000011000111010 (1594)
 *        to
 * 0000100000000000 (2048)
 */

#define		bit(a, b)	((a) & (1 << (b)))
static void round_to_pow2(size_t &y) {
	int i, orig = y;

	if (count_bits(y) <= 1)
		return;

	i = SIZE_T_HIGHBIT;
	if (!bit(y, i)) {
		do {
			if (bit(y, i - 1))	break;
		} while (--i > 0);
		
		if (i <= 0)	return;
		
		y |= (1 << i);
	}
	
	while (i--)
		y &= ~(1 << i);
	
	if (IS_SET(Buffer::options, BUF_VERBOSE))	log("BUF: round_to_pow2: %d -> %d", orig, y);
}


void Buffer::Reboot(void) {
	Init();
	if (!IS_SET(options, BUF_STAYUP)) {
		send_to_all("Emergency reboot.. come back in a minute or two.\r\n");
		log("BUF: SYSERR: Emergency reboot, buffer corrupted!");
		circle_shutdown = circle_reboot = 1;
		return;
	}
	log("BUF: SYSERR: Buffer list cleared, crash imminent!");
}

#if BUFFER_EXTREME_PARANOIA	/* only works on Linux */

#define valid_pointer(ptr)	(((unsigned long)(ptr) & 0x48000000))

//	Private: buffer_sanity_check(nothing)
//	Checks the entire buffer/memory list for overruns.
static void buffer_sanity_check(struct buf_data **check) {
	bool die = false;

	if (!check) {
		buffer_sanity_check(get_buffer_head());
		buffer_sanity_check(get_memory_head());
		return;
	}
	for (int magnitude = 0; magnitude <= buffer_array[BUF_MAX]; magnitude++) {
		struct buf_data *chk;
		if (check[magnitude] && !valid_pointer(check[magnitude]) && (die = true))
			log("BUF: SYSERR: %s/%d head corrupted.", check == get_buffer_head() ? "buffer" : "memory", magnitude);
		for (chk = check[magnitude]; chk; chk = chk->next) {
			if (!valid_pointer(chk->data) && (die = true))
				log("BUF: SYSERR: %p->%p (data) corrupted.", chk, chk->data);
			if (chk->next && !valid_pointer(chk->next) && (die = true))
				log("BUF: SYSERR: %p->%p (next) corrupted.", chk, chk->next);
			if (chk->type != BT_MALLOC && ((unsigned long)chk->var & 0xff0000) != 0)
				log("BUF: SYSERR: %p->%p is set on non-malloc memory.", chk, chk->var);
			if (chk->magic != MAGIC_NUMBER && (die = TRUE))
				log("BUF: SYSERR: %p->%d (magic) corrupted.", chk, chk->magic);
			if (chk->data[chk->req_size] != MAGIC_NUMBER && (die = true))
				log("BUF: SYSERR: %p (%s:%d) overflowed requested size (%d).", chk, chk->who, chk->line, chk->req_size);
			if (chk->data[chk->size] != MAGIC_NUMBER && (die = true))
				log("BUF: SYSERR: %p (%s:%d) overflowed real size (%d).", chk, chk->who, chk->line, chk->size);
		}
	}
	if (die)
		Buffer::Reboot();
}
#endif


//	Private: magic_check(buffer to check)
//	Makes sure a buffer hasn't been overwritten by something else.
void Buffer::Check(void) {
	if (magic != MAGIC_NUMBER) {
		char trashed[16];
		strncpy(trashed, (char *)this, 15);
		trashed[15] = '\0';
		log("BUF: SYSERR: Buffer %p was trashed! (%s)", this, trashed);
		Reboot();
	}
}


//	Private: get_magnitude(number)
//	Simple function to avoid math library, and enforce maximum allocations.
//	If it is not enforced here, we will overrun the bounds of the array.
namespace {
int get_magnitude(UInt32 y) {
	int number = buffer_hash(y) - buffer_array[BUF_MIN];

	if (number > buffer_array[BUF_MAX]) {
		log("BUF: SYSERR: Hash result %d out of range 0-%d.", number, buffer_array[BUF_MAX]);
		number = buffer_array[BUF_MAX];
	}
	
	if (IS_SET(Buffer::options, BUF_VERBOSE))
		log("BUF: get_magnitude: %d bytes -> %d", y, number);

	return number;
}
}

int Buffer::Magnitude(void) {
//	return get_magnitude(round_to_pow2(size));
	return get_magnitude(size);
}


//	Public: exit_buffers()
//	Called to clear out the buffer structures and log any BT_MALLOC's left.
void Buffer::Exit(void) {
//	log("BUF: Shutting down.");

#if BUFFER_THREADED
	pthread_join(thread, NULL);
#endif

//	log("BUF: Clearing buffer memory.");
	for (int magnitude = 0; magnitude <= buffer_array[BUF_MAX]; magnitude++) {
		for (Buffer *b = Buffer::buf[magnitude], *bn = NULL; b; b = bn) {
			bn = b->next;
			LOCK(b, lockWillClear);
			b->Clear();
			LOCK(b, lockWillRemove);
			b->Remove();
			LOCK(b, lockWillFree);
			delete b;
		}
	}
//	log("BUF: Clearing malloc()'d memory.");
	for (int magnitude = 0; magnitude <= buffer_array[BUF_MAX]; magnitude++)
		while (Buffer::mem[magnitude]) {
			LOCK(mem[magnitude], lockWillClear);
			Buffer::mem[magnitude]->Clear();
		}
//	log("BUF: Done.");
}


//	Private: find_hash(none)
//	Determine the size of the buffer array based on the hash size and the
//	allocation limits.
static int find_hash(void) {
	int	maxhash = 0,
		result;

//	log("BUF: Calculating array indices...");
	for (int i = MIN_ALLOC; i <= MAX_ALLOC; i++) {
		result = buffer_hash(1 << i);
		if (result > maxhash)	maxhash = result;
//		if (result < minhash)	minhash = result;
		if (IS_SET(Buffer::options, BUF_DETAIL | BUF_VERBOSE))
			log("BUF: %7d -> %2d", 1 << i, result);
	}
	if (IS_SET(Buffer::options, BUF_DETAIL | BUF_VERBOSE)) {
		log("BUF: ...done.");
		log("BUF: Array range is 0-%d.", maxhash);
	}
//	buffer_array[BUF_MIN] = minhash;
	buffer_array[BUF_MAX] = maxhash;
//	buffer_array[BUF_RANGE] = maxhash - minhash;
	return maxhash;
}



Buffer **Buffer::buf;
Buffer **Buffer::mem;

//	Public: init_buffers(none)
//	This is called from main() to get everything started.
void Buffer::Init(void) {
	static bool	initialized = false;
	
//	InitMutex();

	lock_buffers();
//	Lock(tBuffer);
//	Lock(tMalloc);
	
//	if (!initialized)
//		InitThreaded();
	
	initialized = true;

	find_hash();
	
#if BUFFER_THREADED
	if (thread)
		pthread_cancel(thread);
	
	int error;
	if ((error = pthread_create(&thread, NULL, Buffer::ThreadFunc, NULL)) < 0) {
		perror("pthread_create");
		printf("%d\n", error);
		exit(1);
	}
#endif

	//	Allocate room for the array of pointers.  We can't use the CREATE
	//	macro here because that may use the array we're trying to make!
	Buffer::buf = (Buffer **)calloc(buffer_array[BUF_MAX] + 1, sizeof(Buffer *));
	Buffer::mem = (Buffer **)calloc(buffer_array[BUF_MAX] + 1, sizeof(Buffer *));

//	log("BUF: Buffer cache: %d elements, %d bytes.", CACHE_SIZE, sizeof(struct buf_data *) * CACHE_SIZE);
//	log("BUF: Allocations: %d-%d bytes", 1 << MIN_ALLOC, 1 << MAX_ALLOC);

	//	Put any persistant buffers here.
	//	Ex: new_buffer(8192, BT_PERSIST);

	unlock_buffers();	
//	Unlock(tBuffer);
//	Unlock(tMalloc);
}


//	Private: decrement_all_buffers(none)
//	Reduce the life on all buffers by 1.
void Buffer::Decrement(void) {
//	Lock(Buffer::tBuffer);

	for (int magnitude = 0; magnitude <= buffer_array[BUF_MAX]; magnitude++) {
		for (Buffer *clear = Buffer::buf[magnitude]; clear; clear = clear->next) {
			if (LOCKED(clear))			continue;
			if (clear->type == tMalloc)	continue;
			if (clear->life < 0)
				log("BUF: SYSERR: %p from %s:%d has %d life.", clear, clear->who, clear->line, clear->life);
			else if (clear->life != 0)	//	We don't want to go negative.
				clear->life--;
		}
	}
//	Unlock(Buffer::tBuffer);
}


//	Private: release_old_buffers()
//	Check for any used buffers that have no remaining life.
void Buffer::ReleaseOld(void) {
//	Lock(Buffer::tBuffer);
	
	for (int magnitude = 0; magnitude <= buffer_array[BUF_MAX]; magnitude++) {
		for (Buffer *relbuf = Buffer::buf[magnitude]; relbuf; relbuf = relbuf->next) {
			if (relbuf->type == tMalloc)continue;
			if (!USED(relbuf))			continue;	//	Can't release, no one is using this
			if (LOCKED(relbuf))			continue;	//	Someone has this locked already
			if (relbuf->life > 0)		continue;	//	We only want expired buffers
			
			LOCK(relbuf, lockWillClear);
			log("BUF: %s:%d forgot to release %d bytes in %p.", relbuf->who ? relbuf->who : "UNKNOWN", relbuf->line, relbuf->size, relbuf);
			relbuf->Clear();
			UNLOCK(relbuf);
		}
	}
//	Unlock(Buffer::tBuffer);
}


//	Private: free_old_buffers(void)
//	Get rid of any old unused buffers.
void Buffer::FreeOld(void) {
	lock_buffers();

	for (int magnitude = 0; magnitude <= buffer_array[BUF_MAX]; magnitude++) {
		for (Buffer *freebuf = Buffer::buf[magnitude], *next_free = NULL; freebuf; freebuf = next_free) {
			next_free = freebuf->next;
			if (freebuf->type != Buffer::tBuffer)	continue;	//	We don't free persistent ones
			if (freebuf->life > 0)					continue;	//	Hasn't expired yet
			if (USED(freebuf))						continue;	//	Needs to be cleared first if used
			if (LOCKED(freebuf))					continue;	//	Already locked, skip it

			LOCK(freebuf, lockWillRemove);
			freebuf->Remove();
			LOCK(freebuf, lockWillFree);
			delete freebuf;
		}
	}
	
	unlock_buffers();
}


/*
 * Public: release_all_buffers()
 * Forcibly release all buffers currently allocated.  This is useful to
 * reclaim any forgotten buffers.
 * See structs.h for PULSE_BUFFER to change the release rate.
 */
void Buffer::ReleaseAll(void) {
#if BUFFER_THREADED
	/* We don't do anything, there's already a thread doing this. */
#else
	Buffer::Decrement();
	Buffer::ReleaseOld();
	Buffer::FreeOld();
#endif
}

/*
 * Private: remove_buffer(buffer to remove)
 * Removes a buffer from the list without freeing it.
 */
void Buffer::Remove(void) {
	Buffer **	head;
	int			magnitude;

#if BUFFER_THREADED
	if (LOCKED(this) != lockWillRemove)
		log("BUF: SYSERR: remove_buffer: Lock bit not properly set on %p!", this);
#endif

	head = Buffer::Head(type);
	magnitude = Magnitude();

	for (Buffer *traverse = head[magnitude], *prev = NULL; traverse; traverse = traverse->next) {
		if (this == traverse) {
			if (traverse == head[magnitude])	head[magnitude] = traverse->next;
			else if (prev)						prev->next = traverse->next;
			else log("BUF: SYSERR: remove_buffer: Don't know what to do with %p.", this);
#if BUFFER_THREADED
			if (LOCKED(this) != lockWillRemove)
				log("BUF: SYSERR: remove_buffer: Lock bit removed from %p during operation!", this);
#endif
			return;
		}
		prev = traverse;
	}
	log("BUF: SYSERR: remove_buffer: Couldn't find the buffer %p from %s:%d.", this, who, line);
}

/*
 * Private: clear_buffer(buffer to clear)
 * This is used to declare an allocated buffer unused.
 */
Buffer::Type Buffer::Clear(void) {
#if BUFFER_THREADED
	if (LOCKED(this) != lockWillClear)
		log("BUF: SYSERR: clear_buffer: Lock not properly set on %p!", this);
#endif

	Check();

	/*
	 * If the magic number we set is not there then we have a suspected
	 * buffer overflow.
	 */
	if (data[req_size] != MAGIC_NUMBER) {
		log("BUF: SYSERR: Overflow in %p (%s) from %s:%d.",
				this, type == tMalloc ? var : "a buffer", who ? who : "UNKNOWN", line);
		log("BUF: SYSERR: ... ticketed for doing %d in a %d (%d) zone.",
				strlen(data) + 1, req_size, size);
		if (data[size] == MAGIC_NUMBER) {
			log("BUF: SYSERR: ... overflow did not compromise memory.");
			data[req_size - 1] = '\0';
			data[req_size] = MAGIC_NUMBER;
		} else
			Buffer::Reboot();
	} else if (type == Buffer::tMalloc) {
		lock_buffers();
		LOCK(this, lockWillRemove);
		Remove();
		LOCK(this, lockWillFree);
		delete this;
		unlock_buffers();
		return Buffer::tMalloc;
	} else {
		if (IS_SET(Buffer::options, BUF_CHECK)) {
			int used = (data && *data) ?
					(IS_SET(Buffer::options, BUF_NUKE) ? Used() : strlen(data)) : 0;
			//	If nothing in clearme->data return 0, else if paranoid_buffer, return
			//	the result of Buffer::Used(), otherwise just strlen() the buffer.
			log("BUF: %s:%d used %d/%d bytes in %p.",
					who, line, used, req_size, this);
		}
		if (IS_SET(Buffer::options, BUF_NUKE))	memset(data, '\0', size);
		else									*data = '\0';
		who = NULL;
		line = 0;
		req_size = size;	//	For Buffers::Exit()
		life = B_FREELIFE;
#if BUFFER_THREADED
		if (LOCKED(this) != lockWillClear)
			log("BUF: SYSERR: clear_buffer: Someone cleared lock bit on %p.", this);
#endif
	}
	return type;
}


Buffer *Buffer::Find(const char *given, Buffer::Type type) {
	Buffer *	found = NULL;
	Buffer **	head;
	int			scanned = 0;

//	Buffer::Lock(type);
	
	head = Buffer::Head(type);

	for (int i = 0; i <= buffer_array[BUF_MAX]; i++) {
		scanned++;
		if (!head[i] || head[i]->data != given)
			continue;
		found = head[i];
		break;
	}
	
	for (int i = 0; i <= buffer_array[BUF_MAX] && !found; i++) {
		if (!head[i] || !head[i]->next)
			continue;
		for (found = head[i]->next; found; found = found->next) {
			scanned++;
			if (found->data == given)
				break;
		}
	}
	
	if (IS_SET(Buffer::options, BUF_VERBOSE | BUF_VERBOSE)) {
		if (found)
			log("BUF: Buffers::Find(given = %p, type = %d): found %p->%p (%d scans).",
					given, type, found, found->data, scanned);
		else
			log("BUF: Buffers::Find(given = %p, type = %d): found nothing (%d scans).",
					given, type, scanned);
	}

//	Buffer::Unlock(type);

	return found;
}

/*
 * Public: as release_buffer(buffer data pointer)
 * Used throughout the code to finish their use of the buffer.
 */
void Buffer::Detach(const char *data, Buffer::Type type, const char *func, const int line_n) {
	Buffer *	clear;

	if (!data || !func) {
		log("BUF: SYSERR: detach_buffer: Invalid information passed from %s:%d.", func, line_n);
		return;
	}

	clear = Buffer::Find(data, type);

	if (!clear) {
		log("BUF: SYSERR: detach_buffer: No buffer->data %p found for %s:%d.", data, func, line_n);
#if BUFFER_THREADED
	} else if (LOCKED(clear)) {
		if (LOCKED(clear) == lockWillFree)
			log("BUF: detach_buffer: Buffer %p, requested by %s:%d, already slated to be freed.", clear, func, line_n);
		else if (LOCKED(clear) == lockWillClear)
			log("BUF: detach_buffer: Buffer %p, requested by %s:%d, already being detached.", clear, func, line_n);
		else if (LOCKED(clear) == lockWillRemove)
			log("BUF: detach_buffer: Buffer %p, requested by %s:%d, already being removed from list.", clear, func, line_n);
		else
			log("BUF: detach_buffer: Buffer %p, requested by %s:%d, already locked.", clear, func, line_n);
#endif
	} else if (!clear->who)
		log("BUF: SYSERR: detach_buffer: Buffer %p, requested by %s:%d, already released. Locked: %d", clear, func, line_n, LOCKED(clear));
	else {
		LOCK(clear, lockWillClear);
		if (IS_SET(Buffer::options, BUF_VERBOSE | BUF_DETAIL))
			log("BUF: %s:%d released %d bytes in '%s' (%p) from %s:%d.", func, line_n,
					clear->size, clear->type == Buffer::tMalloc ? clear->var : "a buffer",
					clear, clear->who ? clear->who : "UNKNOWN", clear->line);
		clear->Clear();
		UNLOCK(clear);
	}
}
  

Buffer::~Buffer(void) {
	if (type == tPersist)
		log("BUF: SYSERR: %p->~Buffer: Freeing %d byte persistant buffer.", this, size);
	else if (type == tMalloc) {
		if (IS_SET(Buffer::options, BUF_VERBOSE | BUF_DETAIL))
			log("BUF: %p->~Buffer(): Freeing %d bytes in '%s' from %s:%d.", this, size, var, who, line);
	} else if (IS_SET(Buffer::options, BUF_VERBOSE | BUF_DETAIL))
		log("BUF: %p->~Buffer(): Freeing %d bytes in expired buffer.", this, size);

#if BUFFER_THREADED
	if (LOCKED(this) != lockWillFree)
		log("BUF: SYSERR: %p->~Buffer(): Lock not properly set!", this);
#endif

	if (data)	really_free(data);
	else		log("BUF: SYSERR: %p->~Buffer(): Hey, no data?", this);
}

/*
 * Private: new_buffer(size of buffer, type flag)
 * Finds where it should place the new buffer it's trying to malloc.
 */
Buffer::Buffer(size_t size, Buffer::Type type) : magic(MAGIC_NUMBER), type(type), req_size(0),
		size(size), data(NULL), next(NULL), line(0), who(NULL) {
	Buffer **	buflist;
	int			magnitude;
	
#if BUFFER_THREADED
	locked = lockAcquire;
#endif
	
	if (type != tMalloc)
		round_to_pow2(size);
	
	buflist = Buffer::Head(type);
	magnitude = Magnitude();
	
	if (type != tMalloc)	life = B_FREELIFE;
	else					var = NULL;
	
	data = new char[size + 1];
	
	//	Emulate calloc
	if (type == tMalloc || IS_SET(Buffer::options, BUF_NUKE))	memset(data, 0, size);
	else														*data = '\0';
	data[size] = MAGIC_NUMBER;	//	Implant magic safety catch
	
	lock_buffers();
	next = buflist[magnitude];
	buflist[magnitude] = this;
	unlock_buffers();
}


/*
 * We no longer search for buffers outside the specified power of 2 since
 * the next highest array index may actually be smaller.
 */
Buffer *Buffer::FindAvailable(size_t size) {
	Buffer *search;
	int		magnitude, scans = 0;

	lock_buffers();
	
//	size = round_to_pow2(size);
    
	magnitude = get_magnitude(size);
    
	for (search = Buffer::buf[magnitude]; search; search = search->next) {
		scans++;
		if (search->type == Buffer::tMalloc) {
			log("BUF: SYSERR: Ack, memory (%p, %s:%d, '%s') in buffer list!", search, search->who, search->line, search->var ? search->var : "NULL");
			continue;
		}
		if (LOCKED(search))			continue;
		if (USED(search))			continue;
		if (search->size < size)	continue;
		LOCK(search, lockAcquire);
		break;
	}
	if (IS_SET(Buffer::options, BUF_VERBOSE | BUF_DETAIL))
		log("BUF: find_free_buffer: %d scans for %d bytes, found %p.", scans, size, search);

	unlock_buffers();

	return search;
}

/*
 * Public: as get_buffer(size of buffer)
 * Requests a buffer from the free pool.  If a buffer of the desired size
 * is not available, one is created.
 */
char *Buffer::Acquire(size_t size, Buffer::Type type, const char *varname, const char *who, UInt16 line) {
	Buffer *	give;

#if BUFFER_EXTREME_PARANOIA
  buffer_sanity_check(NULL);
#endif

	if (size > BUFFER_MAXSIZE)
		log("BUF: %s:%d requested %d bytes for '%s'.", who, line, size, varname);

	if (size == 0) {
		log("BUF: SYSERR: %s:%d requested 0 bytes.", who, line);
		return NULL;
	} else if (type == Buffer::tMalloc)
		give = new Buffer(size, type);
	else if (!(give = FindAvailable(size))) {
		//	Minimum buffer size, since small ones have high overhead.
		UInt32 allocate_size = size;
		if (allocate_size < BUFFER_MINSIZE /* && type != Buffer::tMalloc */)
			allocate_size = BUFFER_MINSIZE;

		if (IS_SET(Buffer::options, BUF_VERBOSE | BUF_DETAIL))
			log("BUF: acquire_buffer: Making a new %d byte buffer for %s:%d.", allocate_size, who, line);

		/*
		 * If we don't have a valid pointer by now, we're out of memory so just
		 * return NULL and hope things don't crash too soon.
		 */
		give = new Buffer(allocate_size, type);
	}

	give->Check();

#if BUFFER_THREADED
	if (LOCKED(give) != lockAcquire) {
		log("BUF: SYSERR: acquire_buffer: Someone stole my buffer.");
		abort();
	}
#endif

	give->who = who;
	give->line = line;
	give->req_size = size;

	if (type != Buffer::tMalloc)	give->life = B_GIVELIFE;
	else							give->var = varname;

	if (IS_SET(Buffer::options, BUF_VERBOSE | BUF_DETAIL))
		log("BUF: %s:%d requested %d bytes for '%s', received %d in %p.", who, line, size, varname ? varname : "a buffer", give->size, give);

	/*
	 * Plant a magic number to see if someone overruns the buffer. If the first
	 * character of the buffer is not NUL then somehow our memory was
	 * overwritten...most likely by someone doing a release_buffer() and
	 * keeping a secondary pointer to the buffer.
	 */
	give->data[give->req_size] = MAGIC_NUMBER;
	if (*give->data != '\0') {
		log("BUF: SYSERR: acquire_buffer: Buffer %p is not empty as it ought to be!", give);
		*give->data = '\0';
	}

#if 0
	/* This will only work if the buf_data is allocated by valloc(). */
	if (type == BT_MALLOC)
    if (mprotect(give, sizeof(struct buf_data), PROT_READ) < 0)
      abort();
#endif

	UNLOCK(give);

	if (give->type == tBuffer && give->life <= 1) {
		log("BUF: SYSERR: acquire_buffer: give->life <= 1 (Invalid lease life?!)");
		give->life = 10;
	}

	return give->data;
}

/*
 * Public: as 'show buffers'
 * This is really only useful to see who has lingering buffers around
 * or if you are curious.  It can't be called in the middle of a
 * command run by a player so it'll usually show the same thing.
 * You can call this with a NULL parameter to have it logged at any
 * time though.
 */
/*
 * XXX: This code works but is ugly and misleading.
 */
void show_buffers(Character *ch, Buffer::Type buffer_type, int display_type) {
	Buffer *disp, **head = NULL;
	int i;
	char *buf;
	char *buf_type[] = { "Buffer", "Persist", "Malloc" };

	if (display_type == 1) {
		head = Buffer::buf;
		log("BUF: --- Buffer list --- (Inaccurate if not perfect hash.)");
	} else if (display_type == 2) {
		head = Buffer::mem;
		log("BUF: --- Memory list --- (Byte categories are inaccurate.)");
	}
	if (display_type == 1 || display_type == 2) {
		for (int size = MIN_ALLOC; size <= MAX_ALLOC; size++) {
			int bytes, magnitude = get_magnitude(1 << size);
			for (i = 0, bytes = 0, disp = head[magnitude]; disp; disp = disp->next, i++)
				bytes += disp->size;
			log("%5d bytes (%2d): %5d items, %6d bytes, %5d overhead.", (1 << size), magnitude, i, bytes, sizeof(Buffer) * i);
		}
		return;
	}

	buf = get_buffer(MAX_STRING_LENGTH);
	head = Buffer::buf;
	
	if (ch)		ch->Send("-- ## Size  Life Type    Allocated\r\n");
	else		log("-- ## Size  Life Type    Allocated");
	for (int size = 0; size <= buffer_array[BUF_MAX]; size++)
		for (i = 0, disp = head[size]; disp; disp = disp->next) {
			if (buffer_type != -1 && buffer_type != disp->type) /* -1 == all */
				continue;
			sprintf(buf, "%2d %2d %5d %4d %7s %s:%d%s",
					size, ++i, disp->size, disp->life,
					buf_type[(int)disp->type], disp->who ? disp->who : "unused",
					disp->line ? disp->line : 0, ch ? "\r\n" : "");
			if (ch)		ch->Send(buf);
			else		log(buf);
		}

	release_buffer(buf);
}

/*
 * Tests the overflow code.  Do not use. :)
 */
ACMD(do_overflow) {
	//	Write 256 bytes into the 130 byte buffer. Tweak to suit.
	int write = 256, bufsize = 130;
	char *buf;

	buf = get_buffer(bufsize);
	while (--write)
		buf[write] = write;
	release_buffer(buf);

	if (ch)	ch->Send("Ok!\r\n");
}

char *BUFFER_FORMAT =
"buffer (add | delete) size (persistant | temporary)\r\n"
"buffer verbose - toggle verbose mode.\r\n"
"buffer detailed - toggle detailed mode.\r\n"
"buffer paranoid - toggle between memset() or *buf = NUL.\r\n"
"buffer check - toggle buffer usage checking.\r\n"
"buffer overflow - toggle immediate reboot on overflow.\r\n";

ACMD(do_buffer) {
	char *arg1, *arg2, *arg3;
	long size;
	int persistant = FALSE;

	//	This looks nifty.
	half_chop(argument, (arg1 = get_buffer(MAX_INPUT_LENGTH)), argument);
	half_chop(argument, (arg2 = get_buffer(MAX_INPUT_LENGTH)), argument);
	half_chop(argument, (arg3 = get_buffer(MAX_INPUT_LENGTH)), argument);

	if (!*arg1)								size = -1;
	else if (!*arg2 || !*arg3)				size = -2;
	else if ((size = atoi(arg2)) == 0)		size = -1;
	else if (is_abbrev(arg3, "persistant"))	persistant = TRUE;
	else if (is_abbrev(arg3, "temporary"))	persistant = FALSE;
	else									persistant = FALSE;

	//	Don't need these now.
	release_buffer(arg2);
	release_buffer(arg3);

  if (size == -1)	/* Oops, error. */
    ch->Send(BUFFER_FORMAT);
  else if (size == -2) {	/* -2 means a toggle command. */
    if (is_abbrev(arg1, "verbose")) {
      Buffer::options ^= BUF_VERBOSE;
      ch->Send(IS_SET(Buffer::options, BUF_VERBOSE) ? "Verbose On.\r\n" : "Verbose Off.\r\n");
    } else if (is_abbrev(arg1, "detailed")) {
      Buffer::options ^= BUF_DETAIL;
      ch->Send(IS_SET(Buffer::options, BUF_DETAIL) ? "Detailed On.\r\n" : "Detailed Off.\r\n");
    } else if (is_abbrev(arg1, "paranoid")) {
      Buffer::options ^= BUF_NUKE;
      ch->Send(IS_SET(Buffer::options, BUF_NUKE) ? "Now paranoid.\r\n" : "No longer paranoid.\r\n");
    } else if (is_abbrev(arg1, "check")) {
      Buffer::options ^= BUF_CHECK;
      ch->Send(IS_SET(Buffer::options, BUF_CHECK) ? "Checking on.\r\n" : "Not checking.\r\n");
    } else if (is_abbrev(arg1, "overflow")) {
      Buffer::options ^= BUF_STAYUP;
      ch->Send(!IS_SET(Buffer::options, BUF_STAYUP) ? "Reboot on overflow.\r\n" : "Will try to keep going.\r\n");
    } else
      ch->Send(BUFFER_FORMAT);
  } else if (is_abbrev(arg1, "delete")) {
    Buffer *toy, **b_head = Buffer::buf;
    for (toy = b_head[get_magnitude(size)]; toy; toy = toy->next) {
      if (USED(toy))
	continue;
      if (toy->size != (unsigned long) size)
	continue;
      if (persistant != toy->type)
	continue;
      if (LOCKED(toy))
	continue;
      LOCK(toy, Buffer::lockWillRemove);
      toy->Remove();
      LOCK(toy, Buffer::lockWillFree);
      delete toy;
      break;
    }
    if (!toy)
      ch->Send("Not found.\r\n");
  } else if (is_abbrev(arg1, "add"))
    new Buffer(size, persistant ? Buffer::tPersist : Buffer::tBuffer); /* So easy. :) */
  else
    ch->Send(BUFFER_FORMAT);

  release_buffer(arg1);
}


int Buffer::Used(void) {
	int cnt = req_size - 1;
	while (cnt > 0 && data[cnt] == '\0');
		cnt--;
	return cnt;
//	int cnt;
//	for (cnt = buf->req_size - 1; cnt > 0 && buf->data[cnt] == '\0'; cnt--);
//	return cnt;
}

/* ------------------------------------------------------------------------ */

#if BUFFER_MEMORY

char *debug_str_dup(const char *txt, const char *varname, const char *func, UInt16 line) {
	if (Buffer::options & BUF_VERBOSE)
		log("BUF: debug_str_dup: %d bytes from %s:%d for '%s'.", strlen(txt) + 1, func, line, varname);
	return strcpy(acquire_buffer(strlen(txt) + 1, BT_MALLOC, varname, func, line), txt);
}

void *debug_calloc(size_t number, size_t size, const char *varname, const char *func, int line)
{
  if (Buffer::options & BUF_VERBOSE)
    log("BUF: debug_calloc: %d*%d bytes from %s:%d for '%s'.", number, size, func, line, varname);
  return acquire_buffer(number * size, BT_MALLOC, varname, func, line);
}

void debug_free(void *ptr, const char *func, UInt16 line)
{
  if (Buffer::options & BUF_VERBOSE)
    log("BUF: debug_free: %p from %s:%d.", ptr, func, line);
  /* BT_MALLOC's are free()'d */
  detach_buffer(ptr, BT_MALLOC, func, line);
}

void *debug_realloc(void *ptr, size_t size, const char *varname, const char *func, int line)
{
  void *xtra;
  if (Buffer::options & BUF_VERBOSE)
    log("BUF: debug_realloc: '%s' (%p) resized to %d bytes for %s:%d.", varname, ptr, size, func, line);
  xtra = acquire_buffer(size, BT_MALLOC, varname, func, line);
  memmove(xtra, ptr, size);
  detach_buffer(ptr, BT_MALLOC, func, line);
  return xtra;
}
#endif

/* ------------------------------------------------------------------------ */

#if BUFFER_THREADED

void Buffer::ListLock(const char *func, UInt16 line) {
	if (IS_SET(Buffer::options, BUF_VERBOSE))
		log("BUF: List locked by %s:%d.", func, line);
	pthread_mutex_lock(&listmutex);
}


void Buffer::ListUnlock(const char *func, UInt16 line) {
	if (IS_SET(Buffer::options, BUF_VERBOSE))
		log("BUF: List unlocked by %s:%d.", func, line);
	pthread_mutex_unlock(&listmutex);
}

void Buffer::Lock(Buffer::tLock locktype, const char *func, UInt16 line) {
	if (!this)
		log("BUF: SYSERR: buffer_lock: buf == NULL from %s:%d", func, line);
	else if (LOCKED(this) != Buffer::lockNone && LOCKED(this) != Buffer::lockWillRemove &&
			LOCKED(this) != Buffer::lockWillClear &&
			locktype != Buffer::lockWillFree && locktype != Buffer::lockWillRemove)
		log("BUF: SYSERR: buffer_lock: Trying to lock a buffer %p already locked, from %d to %d at %s:%d.", this, LOCKED(this), type, func, line);
	else
		locked = locktype;
}


void Buffer::Unlock(const char *func, UInt16 line) {
	if (!this)
		log("BUF: SYSERR: buffer_unlock: buf == NULL from %s:%d.", func, line);
	else if (LOCKED(this) == Buffer::lockNone)
		log("BUF: SYSERR: buffer_unlock: Buffer %p isn't locked, from %s:%d.", buf, func, line);
	else
		locked = Buffer::lockNone;
}

/*
 * Public: buffer_thread()
 * If we ever have a multithreaded base, this would be a very nice
 * application for it.  I plan to add a timer on how long someone
 * has used a buffer.
 *
 * Work in progress.
 */
void *Buffer::ThreadFunc(void *nothing) {
	struct timeval tv;

	log("BUF: Started buffer thread.");

	while (!circle_shutdown) {
		//	This is a generic function to reduce the life on all buffers by 1.
		Buffer::Decrement();

		//	This checks for any buffers which were never released.
    	Buffer::ReleaseOld();

		//	Here we free() any buffer which has not been used in a while.
		Buffer::FreeOld();

		tv.tv_sec = 1;
		tv.tv_usec = 0;
//		tv.tv_usec = OPT_USEC;
		
		//	Sleep the same amount of time the main loop does.
		select(0, NULL, NULL, NULL, &tv);
		
		//	See if we should exit.
		pthread_testcancel();
	}

	log("BUF: Buffer thread exited.");

	pthread_exit(NULL);
	return NULL;
}
#endif

/* ------------------------------------------------------------------------ */

#if 0 /* BUFFER_SNPRINTF */
/*
 * Buffer using sprintf() with bounds checking via snprintf()
 */
int bprintf(buffer *str, const char *format, ...)
{
  va_list args;
  int chars;

  va_start(args, format);
  chars = vsnprintf(str->data, str->req_size, args);
  va_end(args);
  return chars;
}

/*
 * Buffer wrapper.
 * b2b = from buffer to buffer.
 * s2b = from string to buffer.
 * b2s = from buffer to string.
 * s2s = doesn't exist, think about it.
 *
 * Another options would be to make b2b_cpy the default strcpy with #define.
 */
char *b2b_cpy(buffer *str1, const buffer *str2) { strcpy(str1->data, str2->data); }
char *s2b_cpy(buffer *str1, const char *str2) { strcpy(str1->data, str2); }
char *b2s_cpy(char *str1, const buffer *str2) { strcpy(str1, str2->data); )
char *b2s_cat(char *str1, const buffer *str2) { strcat(str1, str2->data); }
char *s2b_cat(buffer *str1, const char *str2) { strcat(str1->data, str2); }
char *b2b_cat(buffer *str1, const buffer *str2) { strcat(str1->data, str2->data); }
#endif

/* ------------------------------------------------------------------------ */

/*
 * At the bottom because we need to undefine the macro to get correct
 * results.
 */
#if BUFFER_MEMORY
#undef free
void really_free(void *ptr) {
	if (!ptr)	return;	//	Your OS may already do this, but it's insignificant.
	free(ptr);
}
#endif
