/*
 * queue.h
 *
 *  Created on: Sep 13, 2013
 *      Author: rasmus
 */

#ifndef QUEUE_H_
#define QUEUE_H_

#define MAX_QUEUE_SIZE 20

#include <pthread.h>
#include "../HCI_Parser/HCI_Defs.h"

typedef struct {
	pthread_mutex_t q_mutex;
	datagram_t content[MAX_QUEUE_SIZE];
	int front;
	int count;
	int maxCount;
} queue_t;

extern queue_t queueCreate(void);
extern void queueDestroy(queue_t *q);
extern int enqueue(queue_t *q, datagram_t *elem);
extern int dequeue(queue_t *q, datagram_t *elem);
extern int queueCount(queue_t *q);
extern int queueMaxCount(queue_t *q);

#endif /* QUEUE_H_ */
