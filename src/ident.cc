/***************************************************************************\
[*]    __     __  ___                ___  | LexiMUD is a C++ MUD codebase [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ |        for Sci-Fi MUDs        [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / | Copyright(C) 1997-1999        [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  | All rights reserved           [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   | Chris Jacobson (FearItself)   [*]
[*]      A Creation of the AvP Team!      | fear@technologist.com         [*]
[-]---------------------------------------+-------------------------------[-]
[*] File : ident.cp                                                       [*]
[*] Usage: Ident Services                                                 [*]
\***************************************************************************/


#include "types.h"


#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>

#ifdef macintosh
#include <machine/endian.h>
#define IDENTD_THREADED 0
#else
#define IDENTD_THREADED 0
#endif

#include "ident.h"
#include "ban.h"
#include "utils.h"
#include "buffer.h"




extern SInt32		port;
extern bool			no_external_procs;
extern char *		GREETINGS;
extern char *		ANSI;


namespace Identd {
	LList<LookupRec *>	lookups;
#if IDENTD_THREADED
	pthread_t			thread = 0;
	pthread_mutex_t		lookup_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif
	Prefs				lookup;
}


bool Identd::IsActive(void) {
#if IDENTD_THREADED
	return (Identd::thread > 0);
#else
	return true;
#endif
}


void Identd::Startup(void) {
	lookup.host = true;
	lookup.user = false;
	
#if IDENTD_THREADED
	int error;
	if ((error = pthread_create(&thread, NULL, Loop, NULL)) < 0) {
		perror("Error creating Ident process");
		printf("%d\n", error);
	}
#endif
}


void Identd::Shutdown(void) {
#if IDENTD_THREADED
	if (IsActive())
		pthread_join(Identd::thread, NULL);
#endif

	LookupRec *l;
	while ((l = lookups.Top())) {
		lookups.Remove(l);
		delete l;
	}
}


void Identd::Receive(void) {
#if IDENTD_THREADED
	return;
#endif
	Descriptor *	d;
	LookupRec *		l;

#if IDENTD_THREADED
	pthread_mutex_lock(&lookup_mutex);
#endif
	LListIterator<Identd::LookupRec *>		iter(lookups);	
	while ((l = iter.Next())) {
#if IDENTD_THREADED
		pthread_mutex_unlock(&lookup_mutex);
#endif
		if (!l->done)	continue;
		
		LListIterator<Descriptor *>		iter(descriptor_list);
		while ((d = iter.Next())) {
			if (d->descriptor == l->fd) {
				*d->host = '\0';
				if (*l->user) {
					strcpy(d->user_name, l->user);
					sprintf(d->host + strlen(d->host), "%s@", l->user);
				}
				if (*l->host) {
					strcpy(d->host_name, l->host);
					strcat(d->host, l->host);
				} else
					strcat(d->host, d->host_ip);
				
				//	determine if the site is banned
				if (Ban::Banned(d->host) == Ban::All) {
					d->Write("Sorry, this site is banned.\r\n");
					STATE(d) = CON_DISCONNECT;
					mudlogf(CMP, NULL, TRUE,  "Connection attempt denied from [%s]", d->host);
				} else {	//	Log new connections
					d->Write(ANSI);
					STATE(d) = CON_QANSI;
					mudlogf(CMP, NULL, TRUE, "New connection from [%s]", d->host);
				}
				break;
			}
		}
#if IDENTD_THREADED
		pthread_mutex_lock(&lookup_mutex);
#endif
		lookups.Remove(l);
	}
#if IDENTD_THREADED
	pthread_mutex_unlock(&lookup_mutex);
#endif
}


void Identd::Lookup(Descriptor *d) {
	char *	str;
	
	*d->user_name = *d->host_name = '\0';
#if IDENTD_THREADED
	if (IsActive()) {
		STATE(d) = CON_IDENT;
		LookupRec *	msg = new LookupRec;
		msg->address	= d->saddr;
		msg->fd			= d->descriptor;
		
		pthread_mutex_lock(&lookup_mutex);
		lookups.Add(msg);
		pthread_mutex_unlock(&lookup_mutex);
	} else {
#endif
		if (lookup.host && (str = LookupHost(d->saddr)))	strncpy(d->host_name, str, HOST_LENGTH);
		if (lookup.user && (str = LookupUser(d->saddr)))	strncpy(d->user_name, str, HOST_LENGTH);

		*d->host = '\0';
		if (*d->user_name)	sprintf(d->host + strlen(d->host), "%s@", d->user_name);
		if (*d->host_name)	strncat(d->host, d->host_name, HOST_LENGTH - strlen(d->host));
		else				strncat(d->host, d->host_ip, HOST_LENGTH - strlen(d->host));
		d->Write(ANSI);
#if IDENTD_THREADED
	}
#endif
}


void *Identd::Loop(void *) {
	char *			str;
	struct			timeval tv;
	
	log("IDENTD: Identd thread starting up.");
	
	for (;;) {
		if (lookups.Count()) {
			LookupRec *l;

#if IDENTD_THREADED
			pthread_mutex_lock(&lookup_mutex);
#endif
			LListIterator<LookupRec *>	iter(lookups);
			while ((l = iter.Next())) {
#if IDENTD_THREADED
				pthread_mutex_unlock(&lookup_mutex);
#endif
				if (!l->done) {
					if (lookup.host && (str = LookupHost(l->address)))	strcpy(l->host, str);
					if (lookup.user && (str = LookupUser(l->address)))	strcpy(l->host, str);
					l->done = true;
				}
#if IDENTD_THREADED
				pthread_mutex_lock(&lookup_mutex);
#endif
			}
#if IDENTD_THREADED
			pthread_mutex_unlock(&lookup_mutex);
#endif
		}
		
		tv.tv_sec = 0;
		tv.tv_usec = OPT_USEC;
		
		select(0, NULL, NULL, NULL, &tv);
		
#if IDENTD_THREADED
		pthread_testcancel();
#endif
	}
	
	log("IDENTD: Identd thread exiting.");
}


char *Identd::LookupHost(struct sockaddr_in sa) {
	static char			hostname[256];
	struct hostent *	hent = NULL;
	
	if (!(hent = gethostbyaddr((char *)&sa.sin_addr, sizeof(sa.sin_addr), AF_INET))) {
		perror("IdentServer::LookupHost(): gethostbyaddr");
		return NULL;
	}
	strcpy(hostname, hent->h_name);
	return hostname;
}


char *Identd::LookupUser(struct sockaddr_in sa) {
	static char			username[256];
	char *				string;
	struct hostent *	hent;
	struct sockaddr_in	saddr;
	SInt32				sock;
	
	string = get_buffer(MAX_INPUT_LENGTH);

	sprintf(string, "%d, %d\r\n", ntohs(sa.sin_port), port);
	
	if (!(hent = gethostbyaddr((char *) &sa.sin_addr, sizeof(sa.sin_addr), AF_INET)))
		perror("IdentServer::LookupUser(): gethostbyaddr");
	else {
		char *	result;
		saddr.sin_family	= hent->h_addrtype;
		saddr.sin_port		= htons(port);
		memcpy(&saddr.sin_addr, hent->h_addr, hent->h_length);
		
		if ((sock = socket(hent->h_addrtype, SOCK_STREAM, 0)) < 0) {
			perror("IdentServer::LookupUser(): socket");
			release_buffer(string);
			return NULL;
		}
		
		if (connect(sock, (struct sockaddr *) &saddr, sizeof(saddr)) < 0) {
			if (errno != ECONNREFUSED)
				perror("IdentServer::LookupUser(): connect");
			close(sock);
			release_buffer(string);
			return NULL;
		}
		
		write(sock, string, strlen(string));
		
		read(sock, string, 256);
		
		close(sock);
		
		SInt32	sport, cport;
		char	*mtype, *otype;
		mtype = get_buffer(MAX_INPUT_LENGTH);
		otype = get_buffer(MAX_INPUT_LENGTH);
		
		sscanf(string, " %d , %d : %s : %s : %s ", &sport, &cport, mtype, otype, username);
		
		result = (!strcmp(mtype, "USERID") ? username : NULL);
		
		release_buffer(string);
		release_buffer(mtype);
		release_buffer(otype);
		
		return result;
	}
	release_buffer(string);
	return NULL;
}
