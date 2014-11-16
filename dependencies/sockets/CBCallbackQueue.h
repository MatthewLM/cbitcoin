//
//  CBCallbackQueue.h
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

#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include "CBDependencies.h"

#ifndef CBCallbackQueueH
#define CBCallbackQueueH

typedef struct CBCallbackQueueItem CBCallbackQueueItem;

struct CBCallbackQueueItem{
	void  (*userCallback)(void *);
	void * userArg;
	bool blocking;
	CBCallbackQueueItem * next;
};

typedef struct{
	CBCallbackQueueItem item;
	bool * done;
	CBDepObject cond;
	CBDepObject mutex;
} CBCallbackQueueItemBlocking;

typedef struct{
	CBCallbackQueueItem * first;
	CBCallbackQueueItem * last;
	CBDepObject queueMutex;
}CBCallbackQueue;

void CBInitCallbackQueue(CBCallbackQueue * queue);
CBCallbackQueueItem * CBCallbackQueueAdd(CBCallbackQueue * queue, void (*callback)(void *), void * arg, bool * nblock);
void CBCallbackQueueWait(CBCallbackQueueItem * item);
void CBCallbackQueueRun(CBCallbackQueue * queue);
void CBFreeCallbackQueue(CBCallbackQueue * queue);

#endif
