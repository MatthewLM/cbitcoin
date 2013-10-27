//
//  CBCallbackQueue.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 27/10/2013.
//  Copyright (c) 2012 Matthew Mitchell
//
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

#include "CBCallbackQueue.h"

void CBInitCallbackQueue(CBCallbackQueue * queue){
	pthread_mutex_init(&queue->queueMutex, NULL);
	queue->first = NULL;
}
CBCallbackQueueItem * CBCallbackQueueAdd(CBCallbackQueue * queue, void (*callback)(void *), void * arg, bool * nblock){
	pthread_mutex_lock(&queue->queueMutex);
	CBCallbackQueueItem * item = malloc(!*nblock ? sizeof(CBCallbackQueueItemBlocking) : sizeof(CBCallbackQueueItem));
	item->userArg = arg;
	item->userCallback = callback;
	if (queue->first == NULL)
		queue->first = queue->last = item;
	else
		queue->last = queue->last->next = item;
	CBCallbackQueueItemBlocking * blockingItem;
	queue->last->blocking = !*nblock;
	if (!*nblock) {
		blockingItem = (CBCallbackQueueItemBlocking *)queue->last;
		blockingItem->done = nblock;
		pthread_cond_init(&blockingItem->cond, NULL);
		pthread_mutex_init(&blockingItem->mutex, NULL);
	}
	pthread_mutex_unlock(&queue->queueMutex);
	return item;
}
void CBCallbackQueueWait(CBCallbackQueueItem * item){
	CBCallbackQueueItemBlocking * blockingItem = (CBCallbackQueueItemBlocking *)item;
	pthread_mutex_lock(&blockingItem->mutex);
	if (!*blockingItem->done)
		pthread_cond_wait(&blockingItem->cond, &blockingItem->mutex);
	pthread_mutex_unlock(&blockingItem->mutex);
	// Free mutex and condition
	pthread_mutex_destroy(&blockingItem->mutex);
	pthread_cond_destroy(&blockingItem->cond);
	// Free item
	free(item);
}
void CBCallbackQueueRun(CBCallbackQueue * queue){
	pthread_mutex_lock(&queue->queueMutex);
	while (queue->first != NULL) {
		CBCallbackQueueItem * item = queue->first;
		queue->first = queue->first->next;
		item->userCallback(item->userArg);
		// If blocking signal condition and make done
		if (item->blocking) {
			CBCallbackQueueItemBlocking * blockingItem = (CBCallbackQueueItemBlocking *)queue->last;
			pthread_mutex_lock(&blockingItem->mutex);
			*blockingItem->done = true;
			pthread_cond_signal(&blockingItem->cond);
			pthread_mutex_unlock(&blockingItem->mutex);
		}else
			// Free item
			free(item);
	}
	pthread_mutex_unlock(&queue->queueMutex);
}
void CBFreeCallbackQueue(CBCallbackQueue * queue){
	pthread_mutex_destroy(&queue->queueMutex);
	while (queue->first != NULL) {
		CBCallbackQueueItem * item = queue->first;
		queue->first = queue->first->next;
		if (item->blocking) {
			CBCallbackQueueItemBlocking * blockingItem = (CBCallbackQueueItemBlocking *)queue->last;
			pthread_mutex_destroy(&blockingItem->mutex);
			pthread_cond_destroy(&blockingItem->cond);
		}
		// Free item
		free(item);
	}
}
