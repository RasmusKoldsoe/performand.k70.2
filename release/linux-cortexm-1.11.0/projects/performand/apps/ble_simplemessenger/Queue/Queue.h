/*
 * queue.h
 *
 *  Created on: Sep 13, 2013
 *      Author: rasmus
 */

#ifndef QUEUE_H_
#define QUEUE_H_

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "../HCI_Parser/HCI_Defs.h"

#define MAX_QUEUE_SIZE 10

typedef struct {
	pthread_mutex_t q_mutex;
	datagram_t content[MAX_QUEUE_SIZE];
	int front;
	int count;
} queue_t;

queue_t queueCreate(void);
void queueDestroy(queue_t *q);
int enqueue(queue_t *q, datagram_t *elem);
int dequeue(queue_t *q, datagram_t *elem);
int queueCount(queue_t *q);

#endif /* QUEUE_H_ */
