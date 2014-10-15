//
//  CBThreadPoolQueue.h
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

#include "CBDependencies.h"
#include "CBObject.h"
#include <stdlib.h>

typedef struct CBQueueItem CBQueueItem;

struct CBQueueItem{
	bool active;
	bool cleared;
	CBQueueItem * next;
};

typedef struct{
	CBQueueItem * start;
	CBQueueItem * end;
	uint32_t itemNum;
} CBQueue;

typedef struct CBThreadPoolQueue CBThreadPoolQueue;

typedef struct{
	CBQueue queue;
	CBDepObject thread;
	CBThreadPoolQueue * threadPoolQueue;
	CBDepObject waitCond;
	CBDepObject queueMutex;
} CBWorker;

struct CBThreadPoolQueue{
	CBWorker * workers;
	uint32_t numThreads;
	void (*process)(CBThreadPoolQueue * threadPoolQueue, void * item);
	void (*destroy)(void * item);
	bool shutdown;
	void * object;
	CBDepObject finishCond;
	CBDepObject finishMutex;
	bool finished;
};

// Functions

void CBInitThreadPoolQueue(CBThreadPoolQueue * self, uint16_t numThreads, void (*process)(CBThreadPoolQueue * threadPoolQueue, void * item), void (*destroy)(void * item));
void CBDestroyThreadPoolQueue(CBThreadPoolQueue * self);
void CBFreeQueue(CBQueue * queue, void (*destroy)(void * item));

void CBThreadPoolQueueAdd(CBThreadPoolQueue * self, CBQueueItem * item);
void CBThreadPoolQueueClear(CBThreadPoolQueue * self);
void CBThreadPoolQueueThreadLoop(void * self);
void CBThreadPoolQueueWaitUntilFinished(CBThreadPoolQueue * self);
