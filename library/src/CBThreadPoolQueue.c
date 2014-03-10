//
//  CBThreadPoolQueue.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 18/12/2013.
//  Copyright (c) 2012 Matthew Mitchell
//  
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

//  SEE HEADER FILE FOR DOCUMENTATION 

#include "CBThreadPoolQueue.h"
#include <assert.h>

void CBInitThreadPoolQueue(CBThreadPoolQueue * self, uint16_t numThreads, void (*process)(CBThreadPoolQueue * threadPoolQueue, void * item), void (*destroy)(void * item)){
	// Create threads
	self->workers = malloc(sizeof(*self->workers) * numThreads);
	self->numThreads = numThreads;
	self->shutdown = false;
	self->process = process;
	self->destroy = destroy;
	self->finished = true;
	CBNewCondition(&self->finishCond);
	CBNewMutex(&self->finishMutex);
	for (uint16_t x = 0; x < numThreads; x++) {
		self->workers[x].queue.itemNum = 0;
		self->workers[x].queue.start = NULL;
		self->workers[x].threadPoolQueue = self;
		CBNewCondition(&self->workers[x].waitCond);
		CBNewMutex(&self->workers[x].queueMutex);
		CBNewThread(&self->workers[x].thread, CBThreadPoolQueueThreadLoop, self->workers + x);
	}
}
void CBDestroyThreadPoolQueue(CBThreadPoolQueue * self){
	self->shutdown = true;
	for (uint16_t x = 0; x < self->numThreads; x++) {
		CBMutexLock(self->workers[x].queueMutex);
		if (self->workers[x].queue.itemNum == 0)
			// The thread is waiting for items, so wake it.
			CBConditionSignal(self->workers[x].waitCond);
		CBMutexUnlock(self->workers[x].queueMutex);
		CBThreadJoin(self->workers[x].thread);
		CBFreeThread(self->workers[x].thread);
		CBFreeCondition(self->workers[x].waitCond);
		CBFreeMutex(self->workers[x].queueMutex);
		// Free queue
		CBFreeQueue(&self->workers[x].queue, self->destroy);
	}
	CBFreeCondition(self->finishCond);
	CBFreeMutex(self->finishMutex);
	free(self->workers);
}
void CBFreeQueue(CBQueue * queue, void (*destroy)(void * item)){
	while (queue->start) {
		CBQueueItem * item = queue->start;
		queue->start = item->next;
		if (item->active) {
			// This item is being used, do not free but tell it that we have cleared the queue
			item->cleared = true;
		}else{
			destroy(item);
			free(item);
		}
	}
}

void CBThreadPoolQueueAdd(CBThreadPoolQueue * self, CBQueueItem * item){
	item->next = NULL;
	item->active = false;
	item->cleared = false;
	// Find worker with least items to process
	uint16_t workerI = 0;
	for (uint16_t x = 1; x < self->numThreads; x++){
		CBMutexLock(self->workers[x].queueMutex);
		if (self->workers[x].queue.itemNum < self->workers[workerI].queue.itemNum)
			workerI = x;
		CBMutexUnlock(self->workers[x].queueMutex);
		// Queue may change in the meantime but this wont cause any problems
	}
	CBWorker * worker = self->workers + workerI;
	CBMutexLock(worker->queueMutex); // Must get queue mutex first before finish mutex.
	CBMutexLock(self->finishMutex);
	self->finished = false;
	worker->queue.itemNum++;
	if (!worker->queue.start) {
		worker->queue.end = worker->queue.start = item;
		// We have added the first item to the queue so wake up the thread.
		assert(worker->queue.itemNum);
		CBConditionSignal(worker->waitCond);
	}else
		worker->queue.end = worker->queue.end->next = item;
	CBMutexUnlock(worker->queueMutex);
	CBMutexUnlock(self->finishMutex);
}
void CBThreadPoolQueueClear(CBThreadPoolQueue * self){
	for (uint16_t x = 0; x < self->numThreads; x++) {
		CBMutexLock(self->workers[x].queueMutex);
		// Empty queue
		CBFreeQueue(&self->workers[x].queue, self->destroy);
		self->workers[x].queue.itemNum = 0;
		CBMutexUnlock(self->workers[x].queueMutex);
	}
	CBMutexLock(self->finishMutex);
	self->finished = true;
	CBConditionSignal(self->finishCond);
	CBMutexUnlock(self->finishMutex);
}
void CBThreadPoolQueueThreadLoop(void * vself){
	CBWorker * self = vself;
	CBMutexLock(self->queueMutex);
	for (;;) {
		while (self->queue.itemNum == 0 && !self->threadPoolQueue->shutdown)
			// Wait for more items to process. We require a loop because when the condition is signaled, the mutex is unlocked but may not be given to this thread. Another thread may use the mutex to clear the queue again, so check in a loop.
			CBConditionWait(self->waitCond, self->queueMutex);
		assert(self->queue.itemNum || self->threadPoolQueue->shutdown);
		// Check to see if the thread should terminate
		if (self->threadPoolQueue->shutdown){
			CBMutexUnlock(self->queueMutex);
			return;
		}
		// Process the next item.
		CBQueueItem * item = self->queue.start;
		item->active = true; // Prevent deletion.
		CBMutexUnlock(self->queueMutex);
		self->threadPoolQueue->process(self->threadPoolQueue, item);
		// Now we have finished with the item, remove it from the queue
		CBMutexLock(self->queueMutex);
		// Maybe we cleared the queue. Check that it hasn't been.
		if (!item->cleared) {
			self->queue.start = item->next;
			if (--self->queue.itemNum == 0){
				// Look for complete emptiness
				CBMutexLock(self->threadPoolQueue->finishMutex);
				bool empty = true;
				for (uint16_t x = 0; x < self->threadPoolQueue->numThreads; x++)
					if (self->threadPoolQueue->workers[x].queue.itemNum) {
						empty = false;
						break;
					}
				if (empty){
					self->threadPoolQueue->finished = true;
					CBConditionSignal(self->threadPoolQueue->finishCond);
				}
				CBMutexUnlock(self->threadPoolQueue->finishMutex);
			}
		}
		// Now we can destroy the item.
		self->threadPoolQueue->destroy(item);
		free(item);
	}
}
void CBThreadPoolQueueWaitUntilFinished(CBThreadPoolQueue * self) {
	CBMutexLock(self->finishMutex);
	if (!self->finished) // Do not check in loop as no threads ought to add to the queue whilst waiting to finish.
		CBConditionWait(self->finishCond, self->finishMutex);
	CBMutexUnlock(self->finishMutex);
}
