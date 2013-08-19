//
//  CBThreads.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 17/08/2013.
//  Copyright (c) 2012 Matthew Mitchell
//  
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

// Includes

#include "CBDependencies.h"
#include <pthread.h>
#include <stdlib.h>
#include <assert.h>

// Implementation

void CBNewThread(CBDepObject * thread, void * (*function)(void *), void * arg){
	thread->ptr = malloc(sizeof(pthread_t));
	assert(pthread_create(thread->ptr, NULL, function, arg) == 0);
}
void * CBThreadJoin(CBDepObject thread){
	void * returnPtr;
	assert(pthread_join(*(pthread_t *)thread.ptr, &returnPtr) == 0);
	return returnPtr;
}
void CBNewMutex(CBDepObject * mutex){
	mutex->ptr = malloc(sizeof(pthread_mutex_t));
	assert(pthread_mutex_init(mutex->ptr, NULL) == 0);
}
void CBFreeMutex(CBDepObject mutex){
	assert(pthread_mutex_destroy(mutex.ptr) == 0);
}
void CBMutexLock(CBDepObject mutex){
	assert(pthread_mutex_lock(mutex.ptr) == 0);
}
void CBMutexUnlock(CBDepObject mutex){
	assert(pthread_mutex_unlock(mutex.ptr) == 0);
}
void CBNewCondition(CBDepObject * cond){
	cond->ptr = malloc(sizeof(pthread_cond_t));
	assert(pthread_cond_init(cond->ptr, NULL) == 0);
}
void CBFreeCondition(CBDepObject cond){
	assert(pthread_cond_destroy(cond.ptr) == 0);
}
void CBConditionWait(CBDepObject cond, CBDepObject mutex){
	assert(pthread_cond_wait(cond.ptr, mutex.ptr) == 0);
}
void CBConditionSignal(CBDepObject cond){
	assert(pthread_cond_signal(cond.ptr) == 0);
}
