/***************************************************************************\
[*]    __     __  ___                ___  | LexiMUD is a C++ MUD codebase [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ |        for Sci-Fi MUDs        [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / | Copyright(C) 1997-1999        [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  | All rights reserved           [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   | Chris Jacobson (FearItself)   [*]
[*]      A Creation of the AvP Team!      | fear@technologist.com         [*]
[-]---------------------------------------+-------------------------------[-]
[*] File : ident.h                                                        [*]
[*] Usage: Ident Services                                                 [*]
\***************************************************************************/

#include "descriptors.h"

#ifndef macintosh
#include <pthread.h>
#endif

namespace Identd {
	void				Startup(void);
	void				Shutdown(void);
	
	//	Functions
	void				Lookup(Descriptor *newd);
	char *				LookupHost(struct sockaddr_in sa);
	char *				LookupUser(struct sockaddr_in sa);
	void				Receive(void);
	void *				Loop(void *);
	
	//	Sub-Accessors
	bool				IsActive(void);
	
	struct LookupRec {
		bool				done;

		struct sockaddr_in	address;
		SInt32				fd;
		char				host[256];
		char				user[256];
	};
	
	extern LList<LookupRec *>	lookups;
#ifndef macintosh
	extern pthread_t		thread;
	extern pthread_mutex_t	lookup_mutex;
#endif
	
	struct Prefs {
		bool				host;
		bool				user;
	};
	extern Prefs lookup;
}

