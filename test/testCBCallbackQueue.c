//
//  testCBCallbackQueue.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 03/11/2013.
//  Copyright (c) 2013 Matthew Mitchell
//
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "CBDependencies.h"

pthread_mutex_t argmutex = PTHREAD_MUTEX_INITIALIZER;
CBDepObject eventLoop;

void callback(void * arg);
void callback(void * arg){
	pthread_mutex_lock(&argmutex);
	int parg = (*(int *)arg)++;
	pthread_mutex_unlock(&argmutex);
	if (parg % 3 == 0)
		CBRunOnEventLoop(eventLoop, callback, arg, 0);
}

int main(){
	unsigned int s = (unsigned int)time(NULL);
	s = 1393368855;
	printf("Session = %ui\n", s);
	puts("Skipping this test.");
	return EXIT_SUCCESS;
	srand(s);
	CBNewEventLoop(&eventLoop, NULL, NULL, NULL);
	int arg = 0;
	for (int x = 0; x < 1000; x++)
		CBRunOnEventLoop(eventLoop, callback, &arg, x == 999 ? true : rand() % 2);
	if (arg != 1499) {
		printf("ARG FAIL %u != 1499\n", arg);
		return EXIT_FAILURE;
	}
	CBExitEventLoop(eventLoop);
	return EXIT_SUCCESS;
}
