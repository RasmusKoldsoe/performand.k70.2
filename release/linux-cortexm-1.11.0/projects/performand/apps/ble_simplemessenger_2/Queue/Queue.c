/*
 * queue.c
 *
 *  Created on: Sep 13, 2013
 *      Author: rasmus
 */
#include <stdio.h>
#include <string.h>

#include "Queue.h"

#define max(a,b) (a>b?a:b)

queue_t queueCreate()
{
	queue_t q;

	memset(&q, 0, sizeof(q));

	int r;
	r = pthread_mutex_init(&q.q_mutex, NULL);
	if(r < 0) {
		fprintf(stderr, "[Bluetooth] Error initializing queue mutex\n");
	}
	return q;
}

void queueDestroy(queue_t *q)
{
	pthread_mutex_lock(&q->q_mutex);
	pthread_mutex_destroy(&q->q_mutex);
	memset(q, 0, sizeof(q));
}

int enqueue(queue_t *q, datagram_t *elem)
{
	pthread_mutex_lock(&q->q_mutex);

	if(q->count > MAX_QUEUE_SIZE) {
		fprintf(stderr, "[Bluetooth] WARNING: Enqueue on full queue. Clearing queue!\n");
		q->count = 0;
		q->front = 0;
		//pthread_mutex_unlock(&q->q_mutex);
		//return -1;
	}
	
	int newIndex = (q->front + q->count) % MAX_QUEUE_SIZE;
	memcpy(&q->content[newIndex], elem, sizeof(*elem));
	q->count++;
	pthread_mutex_unlock(&q->q_mutex);
	q->maxCount = max(q->maxCount, q->count);

	return 0;
}

int dequeue(queue_t *q, datagram_t *elem)
{
	if(q->count <= 0){
		fprintf(stderr, "[Bluetooth] WARNING: Dequeue from empty queue\n");
		return -1;
	}
	pthread_mutex_lock(&q->q_mutex);
	// Store next element
	memset(elem, 0, sizeof(datagram_t));
	memcpy(elem, &q->content[q->front], sizeof(*elem));

	// Advance - remember to wrap around
	q->front++;
	q->front %= MAX_QUEUE_SIZE;
	q->count--;
	pthread_mutex_unlock(&q->q_mutex);
	return 0;
}

int queueCount(queue_t *q)
{
	return q->count;
}

int queueMaxCount(queue_t *q)
{
	return q->maxCount;
}
