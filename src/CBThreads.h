//
//  CBThreads.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 06/10/2013.
//  Copyright (c) 2013 Matthew Mitchell
//
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

#ifndef CBTHREADSH
#define CBTHREADSH

#include "CBDependencies.h" // cbitcoin dependencies to implement
#include "CBAssert.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef CBDEBUG

#include <execinfo.h>

// Debug mutexes
typedef struct{
	pthread_mutex_t base;
	char ** lockStack[5];
	int lockStackBTSizes[5];
	uint8_t lockStackSize;
	uint16_t threadID; /**< ID of thread that last locked mutex */
} CBMutex;

#else

typedef pthread_mutex_t CBMutex;

#endif

typedef struct{
	pthread_t thread;
	uint16_t ID;
	void (*func)(void *);
	void * arg;
} CBThread;

uint16_t CBGetTID(void);
void * CBRunThread(void * vthread);

#endif
