//
//  CBCallbackQueue.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 27/10/2013.
//  Copyright (c) 2013 Matthew Mitchell
//
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

#include "CBCallbackQueue.h"

void CBInitCallbackQueue(CBCallbackQueue * queue){
	CBNewMutex(&queue->queueMutex);
	queue->first = NULL;
}
CBCallbackQueueItem * CBCallbackQueueAdd(CBCallbackQueue * queue, void (*callback)(void *), void * arg, bool * nblock){
	CBMutexLock(queue->queueMutex);
	CBCallbackQueueItem * item = malloc(!*nblock ? sizeof(CBCallbackQueueItemBlocking) : sizeof(CBCallbackQueueItem));
	item->userArg = arg;
	item->userCallback = callback;
	if (queue->first == NULL)
		queue->first = queue->last = item;
	else
		queue->last = queue->last->next = item;
	item->next = NULL;
	CBCallbackQueueItemBlocking * blockingItem;
	queue->last->blocking = !*nblock;
	if (!*nblock) {
		blockingItem = (CBCallbackQueueItemBlocking *)queue->last;
		blockingItem->done = nblock;
		CBNewCondition(&blockingItem->cond);
		CBNewMutex(&blockingItem->mutex);
	}
	CBMutexUnlock(queue->queueMutex);
	return item;
}
void CBCallbackQueueWait(CBCallbackQueueItem * item){
	CBCallbackQueueItemBlocking * blockingItem = (CBCallbackQueueItemBlocking *)item;
	CBMutexLock(blockingItem->mutex);
	if (!*blockingItem->done)
		CBConditionWait(blockingItem->cond, blockingItem->mutex);
	CBMutexUnlock(blockingItem->mutex);
	// Free mutex and condition
	CBFreeMutex(blockingItem->mutex);
	CBFreeCondition(blockingItem->cond);
	// Free item
	free(item);
}
void CBCallbackQueueRun(CBCallbackQueue * queue){
	CBMutexLock(queue->queueMutex);
	while (queue->first != NULL) {
		CBCallbackQueueItem * item = queue->first;
		queue->first = item->next;
		item->userCallback(item->userArg);
		// If blocking signal condition and make done
		if (item->blocking) {
			CBCallbackQueueItemBlocking * blockingItem = (CBCallbackQueueItemBlocking *)item;
			CBMutexLock(blockingItem->mutex);
			*blockingItem->done = true;
			CBConditionSignal(blockingItem->cond);
			CBMutexUnlock(blockingItem->mutex);
		}else
			// Free item
			free(item);
	}
	CBMutexUnlock(queue->queueMutex);
}
void CBFreeCallbackQueue(CBCallbackQueue * queue){
	CBFreeMutex(queue->queueMutex);
	while (queue->first != NULL) {
		CBCallbackQueueItem * item = queue->first;
		queue->first = queue->first->next;
		if (item->blocking) {
			CBCallbackQueueItemBlocking * blockingItem = (CBCallbackQueueItemBlocking *)queue->last;
			CBFreeMutex(blockingItem->mutex);
			CBFreeCondition(blockingItem->cond);
		}
		// Free item
		free(item);
	}
}
