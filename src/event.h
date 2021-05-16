/***************************************************************************\
[*]    __     __  ___                ___  | LexiMUD is a C++ MUD codebase [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ |        for Sci-Fi MUDs        [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / | Copyright(C) 1997-1999        [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  | All rights reserved           [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   | Chris Jacobson (FearItself)   [*]
[*]      A Creation of the AvP Team!      | fear@technologist.com         [*]
[-]---------------------------------------+-------------------------------[-]
[*] File : event.h                                                        [*]
[*] Usage: Event Engine                                                   [*]
\***************************************************************************/


#ifndef __EVENT_H__
#define __EVENT_H__


#include <list>
#include <typeinfo>

//	The CURRENT event system

class Event;
class Character;
typedef ::std::list<Event *>	EventList;


class QueueElement;

class Event {
public:
						Event(time_t when = 0);
						virtual ~Event(void);
	virtual time_t		Run(void);
	virtual void		Cancel(void);
	time_t				Time(void) const;

	virtual time_t		Execute(void) = 0;
	
protected:
	QueueElement *		queue;
	bool				running;
	
public:
	static void			Initialize(void);
	static void			Shutdown(void);
	static void			Process(void);
	static Event *		Find(EventList &, const std::type_info &);

	friend class		QueueElement;
};


class ListedEvent : public Event {
public:
						ListedEvent(time_t when = 0, EventList *eventList = NULL);
						virtual ~ListedEvent(void);

	virtual void		Cancel(void);

private:
	EventList *			eventList;	// This data member is intentionally unused for the Event class.
									// It is set to NULL by default but used for remove calls for the destructor.
};


class ActionEvent : public Event {
public:
						ActionEvent(time_t when, Character &tch);
						virtual ~ActionEvent(void);

	virtual time_t		Run(void);
	virtual void		Cancel(void);

protected:
	Character *			ch;
};


#endif

