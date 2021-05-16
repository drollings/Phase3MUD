#include "conf.h"
#include "sysdep.h"

#include "queue.h"

//	External variables
extern UInt32 pulse;

QueueElement::QueueElement(Ptr theData, SInt32 theKey) : data(theData), key(theKey),
		prev(NULL), next(NULL) {
}


Queue::Queue(void) {
	memset(this, 0, sizeof(Queue));
}


Queue::~Queue(void) {
	SInt32 i;
	QueueElement *qe, *next_qe;
	
	for (i = 0; i < NUM_EVENT_QUEUES; i++) {
		for (qe = head[i]; qe; qe = next_qe) {
			next_qe = qe->next;
			delete qe;
		}
	}
}


QueueElement *Queue::Enqueue(Ptr data, SInt32 key) {
	QueueElement *qe, *i;
	SInt32	bucket;
	
	qe = new QueueElement(data, key);
	bucket = key % NUM_EVENT_QUEUES;
	
	if (!head[bucket]) {
		head[bucket] = qe;
		tail[bucket] = qe;
	} else {
		for (i = tail[bucket]; i; i = i->prev) {
			if (i->key < key) {
				if (i == tail[bucket])
					tail[bucket] = qe;
				else {
					qe->next = i->next;
					i->next->prev = qe;
				}
				qe->prev = i;
				i->next = qe;
				break;
			}
		}
		if (i == NULL) {
			qe->next = head[bucket];
			head[bucket] = qe;
			qe->next->prev = qe;
		}
	}
	return qe;
}


void Queue::Dequeue(QueueElement *qe) {
	SInt32 i;
	
	if (!qe)	return;
	
	i = qe->key % NUM_EVENT_QUEUES;
	
	if (qe->prev)	qe->prev->next = qe->next;
	else			head[i] = qe->next;
	
	if (qe->next)	qe->next->prev = qe->prev;
	else			tail[i] = qe->prev;
	
	delete qe;
}


Ptr Queue::QueueHead(void) {
	Ptr data;
	SInt32 i;
	
	i = pulse % NUM_EVENT_QUEUES;
	
	if (!head[i])
		return NULL;
	
	data = head[i]->data;
	Dequeue(head[i]);
	
	return data;
}


UInt32 Queue::QueueKey(void) {
	SInt32 i;
	
	i = pulse % NUM_EVENT_QUEUES;
	
	if (head[i])		return head[i]->key;
	else				return LONG_MAX;
}
