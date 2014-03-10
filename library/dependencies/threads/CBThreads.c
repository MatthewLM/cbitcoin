//
//  CBThreads.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 17/08/2013.
//  Copyright (c) 2013 Matthew Mitchell
//  
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

// Includes

#include "CBThreads.h"

// Implementation

pthread_mutex_t idMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_key_t key;
bool keyCreated = false;
uint16_t nextId = 1;

uint16_t CBGetTID(void){
	CBThread * thread = pthread_getspecific(key);
	return (thread == NULL)? 0 : thread->ID;
}
void CBNewThread(CBDepObject * uthread, void (*function)(void *), void * arg){
	CBThread * thread = malloc(sizeof(CBThread));
	thread->func = function;
	thread->arg = arg;
	// Create thread attributes explicitly for portability reasons.
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE); // May need to be joinable.
	// Set ID and associate data with thread
	CBAssertExpression(pthread_mutex_lock(&idMutex), == 0);
	thread->ID = nextId++;
	if (!keyCreated) {
		keyCreated = true;
		pthread_key_create(&key, NULL);
	}
	CBAssertExpression(pthread_mutex_unlock(&idMutex), == 0);
	// Create joinable thread
	CBAssertExpression(pthread_create(&thread->thread, &attr, CBRunThread, thread), == 0);
	pthread_attr_destroy(&attr);
	uthread->ptr = thread;
}
void CBFreeThread(CBDepObject thread){
	free(thread.ptr);
}
void * CBRunThread(void * vthread){
	CBThread * thread = vthread;
	pthread_setspecific(key, thread);
	thread->func(thread->arg);
	return NULL;
}
void CBThreadJoin(CBDepObject thread){
	CBAssertExpression(pthread_join(*(pthread_t *)thread.ptr, NULL), == 0);
} 
void CBNewMutex(CBDepObject * mutex){
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	mutex->ptr = malloc(sizeof(CBMutex));
	CBAssertExpression(pthread_mutex_init(mutex->ptr, &attr), == 0);
	pthread_mutexattr_destroy(&attr);
	#ifdef CBDEBUG
	((CBMutex *)mutex->ptr)->lockStackSize = 0;
	#endif
}
void CBFreeMutex(CBDepObject mutex){
	CBAssertExpression(pthread_mutex_destroy(mutex.ptr), == 0);
	#ifdef CBDEBUG
	assert(((CBMutex *)mutex.ptr)->lockStackSize == 0);
	#endif
	free(mutex.ptr);
}
void CBMutexLock(CBDepObject mutex){
	CBAssertExpression(pthread_mutex_lock(mutex.ptr), == 0);
	#ifdef CBDEBUG
	CBMutex * mu = mutex.ptr;
	void * array[5];
	mu->lockStackBTSizes[mu->lockStackSize] = backtrace(array, 5);
	mu->lockStack[mu->lockStackSize] = backtrace_symbols(array, mu->lockStackBTSizes[mu->lockStackSize]);
	mu->lockStackSize++;
	mu->threadID = CBGetTID();
	#endif
}
void CBMutexUnlock(CBDepObject mutex){
	#ifdef CBDEBUG
	CBMutex * mu = mutex.ptr;
	free(mu->lockStack[--mu->lockStackSize]);
	#endif
	CBAssertExpression(pthread_mutex_unlock(mutex.ptr), == 0);
}
void CBNewCondition(CBDepObject * cond){
	cond->ptr = malloc(sizeof(pthread_cond_t));
	CBAssertExpression(pthread_cond_init(cond->ptr, NULL), == 0);
}
void CBFreeCondition(CBDepObject cond){
	CBAssertExpression(pthread_cond_destroy(cond.ptr), == 0);
	free(cond.ptr);
}
void CBConditionWait(CBDepObject cond, CBDepObject mutex){
	CBAssertExpression(pthread_cond_wait(cond.ptr, mutex.ptr), == 0);
}
void CBConditionSignal(CBDepObject cond){
	CBAssertExpression(pthread_cond_signal(cond.ptr), == 0);
}
uint8_t CBGetNumberOfCores(void){
	return sysconf(_SC_NPROCESSORS_ONLN);
}
