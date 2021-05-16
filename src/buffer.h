#ifndef __BUFFER_H__
#define __BUFFER_H__

#include <stdarg.h>

#include "types.h"
#include "internal.defs.h"


//	CONFIGURABLES (aka, The place to shoot your own foot.) :)
//	---------------------------------------------------------
#define BUFFER_MEMORY				0	//	Use buffer system for CREATE
#define USE_CIRCLE_BUFFERS			1	//	Include original CircleMUD buffers
#ifdef macintosh
#define BUFFER_THREADED				0	//	Use threads
#else
#define BUFFER_THREADED				0	//	Use threads
#endif


#ifdef USE_CIRCLE_BUFFERS
extern char	buf[MAX_STRING_LENGTH];
extern char	buf1[MAX_STRING_LENGTH];
extern char	buf2[MAX_STRING_LENGTH];
extern char	buf3[MAX_STRING_LENGTH];
extern char	arg[MAX_STRING_LENGTH];
#endif

class Buffer {
public:
	enum Type {			//	Types for the memory to allocate.
		tBuffer,		//	Stack type memory.
		tPersist,		//	A buffer that doesn't time out.
		tMalloc			//	A malloc() memory tracker.
	};
	
//protected:
					Buffer(size_t size, Type type);
					~Buffer(void);
	
	Type			Clear(void);
	void			Remove(void);
	int				Used(void);
	int				Magnitude(void);
	
	SInt8			magic;			//	Have we been trashed?
	Type			type;			//	What type of buffer are we?
	size_t			req_size;		//	How much did the function request?
	const size_t	size;			//	How large is this buffer realy?
	char *			data;			//	The buffer passed back to functions
public:
	Buffer *		next;			//	Next structure
	
#if BUFFER_THREADED
	enum tLock {
		lockNone		= 0,
		lockAcquire		= 1 << 0,
		lockWillClear	= 1 << 1,
		lockWillFree	= 1 << 2,
		lockWillRemove	= 1 << 3
	} locked;						//	Don't touch this item, we're locked.
	
	void Lock(tLock type, const char *func, UInt16 line);
	void Unlock(const char *func, UInt16 line);
	
	static void *ThreadFunc(void *);
	static void ListLock(const char *func, UInt16 line);
	static void ListUnlock(const char *func, UInt16 line);
#endif
	
public:
	union {
		SInt32			life;		//	An idle counter to free unused ones.	(B)
		const char *	var;		//	Name of variable allocated to.			(M)
	};
	SInt16			line;			//	What source code line is using this.
	const char	*	who;			//	Name of the function using this
	
	void			Check(void);
	
public:
	static void		Init(void);
	static void		Exit(void);
	
	static void		Reboot(void);
	
	static Buffer **Head(Type type);
//	static void		Lock(Type type);
//	static void		Unlock(Type type);
	
	static Buffer *	Find(const char *given, Type type);
	static Buffer *	FindAvailable(size_t size);
	static void		Detach(const char *data, Type type,
			const char *func, const int line_n);
	static char *	Acquire(size_t size, Type type,
			const char *varname, const char *who, UInt16 line);
	
	static void		ReleaseAll(void);
	
	static Buffer **buf;
	static Buffer **mem;
	
	static Flags	options;
	static const char *const optionsDesc[];
	
private:
	static void		Decrement(void);
	static void		ReleaseOld(void);
	static void		FreeOld(void);
};


inline Buffer **Buffer::Head(Buffer::Type type) {
	return (type == Buffer::tMalloc ? mem : buf);
}


/*
 * These macros neatly pass the required information without breaking the
 * rest of the code.  The 'get_buffer()' code should be used for a temporary
 * memory chunk such as the current CircleMUD 'buf,' 'buf1,' and 'buf2'
 * variables.  Remember to use 'release_buffer()' to give up the requested
 * buffer when finished with it.  'release_my_buffers()' may be used in
 * functions with a lot of return statements but it is _not_ encouraged.
 * 'get_memory()' and 'release_memory()' should only be used for memory that
 * you always want handled here regardless of BUFFER_MEMORY.
 */
#define get_buffer(a)		Buffer::Acquire((a), Buffer::tBuffer, NULL, __FUNCTION__, __LINE__)
#define get_memory(a)		Buffer::Acquire((a), Buffer::tMalloc, NULL, __FUNCTION__, __LINE__)
#define release_buffer(a)	do { Buffer::Detach((a), Buffer::tBuffer, __FUNCTION__, __LINE__); (a) = NULL; } while(0)
#define release_memory(a)	do { Buffer::Detach((a), Buffer::tMalloc, __FUNCTION__, __LINE__); (a) = NULL; } while(0)
#define release_my_buffers()	detach_my_buffers(__FUNCTION__, __LINE__)


//	Public functions for outside use.
#if 0 /* BUFFER_SNPRINTF */
buffer *str_cpy(buffer *d, buffer*s);
int bprintf(buffer *buf, const char *format, ...);
#endif
#if BUFFER_MEMORY
void *debug_calloc(size_t number, size_t size, const char *var, const char *func, int line);
void *debug_realloc(void *ptr, size_t size, const char *var, const char *func, int line);
void debug_free(void *ptr, const char *func, ush_int line);
char *debug_str_dup(const char *txt, const char *var, const char *func, ush_int line);
#endif
class Character;
void show_buffers(Character *ch, Buffer::Type type, int display_type);

#endif

