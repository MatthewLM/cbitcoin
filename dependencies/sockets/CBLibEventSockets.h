//
//  CBLibEventSockets.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 10/08/2012.
//  Copyright (c) 2012 Matthew Mitchell
//  
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

/**
 @file
 @brief This is an implementation of the networking dependencies for cbitcoin. It can be included as as a source file in any projects using cbitcoin which require networking capabilities. This file uses libevent for effecient event-based sockets and POSIX threads. The sockets are POSIX sockets with compatibility functions taken from libevent to try and be compatible with windows.
 */

#include "CBDependencies.h" // cbitcoin dependencies to implement
#include <pthread.h> // POSIX threads
#include <event2/event.h> // libevent CBLogError
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

#ifndef CBLIBEVENTSOCKETSH
#define CBLIBEVENTSOCKETSH

// Define if we have SO_NOSIGPIPE

#ifdef SO_NOSIGPIPE
#define CB_NOSIGPIPE true
#else
#define CB_NOSIGPIPE false
#define SO_NOSIGPIPE 0
#endif

// Define send flags

#ifdef MSG_NOSIGNAL
#define CB_SEND_FLAGS MSG_NOSIGNAL
#else
#define CB_SEND_FLAGS 0
#endif

void event_base_add_virtual(struct event_base *); // Add virtual event.
void * CBStartEventLoop(void *);
void CBCanAccept(evutil_socket_t socketID, short eventNum, void * arg);
void CBDidConnect(evutil_socket_t socketID, short eventNum, void * arg);
void CBCanSend(evutil_socket_t socketID, short eventNum, void * arg);
void CBCanReceive(evutil_socket_t socketID, short eventNum, void * arg);
void CBFireTimer(evutil_socket_t foo, short bar, void * timer);
void CBDoRun(evutil_socket_t socketID, short eventNum, void * arg);

typedef struct{
	struct event_base * base;
	void (*onError)(void *);
	void (*onTimeOut)(void *, void *, CBTimeOutType); /**< Callback for timeouts */
	void * communicator;
	pthread_t loopThread; /**< The thread ID for the event loop. */
	void  (*userCallback)(void *);
	void * userArg;
	struct event * userEvent;
}CBEventLoop;

union CBOnEvent{
	void (*i)(void *, CBDepObject);
	void (*ptr)(void *, void *);
};

typedef struct{
	CBEventLoop * loop; /**< For getting timeout CBLogError */
	struct event * event; /**< libevent event. */
	union CBOnEvent onEvent;
	void * peer;
}CBEvent;

typedef struct{
	void (*callback)(void *);
	void * arg;
	struct event * timer;
}CBTimer;

#endif
