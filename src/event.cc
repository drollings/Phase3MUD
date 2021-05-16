/***************************************************************************\
[*]    __     __  ___                ___  | LexiMUD is a C++ MUD codebase [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ |        for Sci-Fi MUDs        [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / | Copyright(C) 1997-1999        [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  | All rights reserved           [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   | Chris Jacobson (FearItself)   [*]
[*]      A Creation of the AvP Team!      | fear@technologist.com         [*]
[-]---------------------------------------+-------------------------------[-]
[*] File : event.cp                                                       [*]
[*] Usage: Event Engine                                                   [*]
\***************************************************************************/


#include "types.h"

#include "event.h"
#include "queue.h"
#include "utils.h"
#include "mud.base.h"

extern UInt32 pulse;
Queue *EventQueue;

//priority_queue<Event *, Event::Compare>	EventQueue;

void Event::Initialize(void) {
	EventQueue = new Queue;
}


void Event::Process(void) {
	Event *	theEvent;
	SInt32	newTime;
	
	while (pulse >= EventQueue->QueueKey()) {
		if (!(theEvent = (Event *)EventQueue->QueueHead())) {
			log("SYSERR: Attempt to get a NULL event!\r\n");
			continue;
		}
		theEvent->queue = NULL;
		newTime = theEvent->Run();
		if (newTime > 0)	theEvent->queue = EventQueue->Enqueue(theEvent, newTime + pulse);
		else				delete theEvent;
	}
}


void Event::Shutdown(void) {
	Event *TheEvent;
	while ((TheEvent = (Event *)EventQueue->QueueHead())) {
		delete TheEvent;
	}
	delete EventQueue;
}


Event::Event(time_t when) : queue(NULL), running(false) {
	if (when >= 1)
	queue = EventQueue->Enqueue(this, when + pulse);
}


Event::~Event(void) {
	if (queue)	EventQueue->Dequeue(queue);
}


time_t Event::Time(void) const {
	return queue ? (queue->key - pulse) : 0;
}


time_t Event::Run(void) {
	time_t	result;
	
	running = true;
	result = Execute();
	running = false;

	return result;
}


void Event::Cancel(void) {
	if (!running) {
		if (queue)	EventQueue->Dequeue(queue);
		queue = NULL;
		delete this;
	}
}


Event *Event::Find(EventList &list, const type_info &type) {
	EventList::iterator event = list.begin();
	while (event != list.end()) {
		if (*event)
			if (typeid(**event) == type)
				return *event;
		++event;
	}
	
	return NULL;
}


ListedEvent::ListedEvent(time_t when, EventList *list) : Event(when), eventList(list) {}


ListedEvent::~ListedEvent(void) 
{
	if (eventList)	eventList->remove(this);
}


void ListedEvent::Cancel(void) 
{
	if (eventList) {
		eventList->remove(this);
		eventList = NULL;
	}
	if (!running) {
		if (queue)	EventQueue->Dequeue(queue);
		queue = NULL;
		delete this;
	}
}


