//
//  CBLibEventSockets.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 10/08/2012.
//  Copyright (c) 2012 Matthew Mitchell
//  
//  This file is part of cbitcoin.
//
//  cbitcoin is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//  
//  cbitcoin is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//  
//  You should have received a copy of the GNU General Public License
//  along with cbitcoin.  If not, see <http://www.gnu.org/licenses/>.

/**
 @file
 @brief This is an implementation of the networking dependencies for cbitcoin. It can be included as as a source file in any projects using cbitcoin which require networking capabilities. This file uses libevent for effecient event-based sockets and POSIX threads. The sockets are POSIX sockets with compatibility functions taken from libevent to try and be compatible with windows.
 */

#include "CBDependencies.h" // cbitcoin dependencies to implement
#include <pthread.h> // POSIX threads
#include <ev.h> // libev events
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
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

void * CBStartEventLoop(void *);
void CBCanAccept(struct ev_loop * loop,struct ev_io * watcher,int eventID);
void CBDidConnect(struct ev_loop * loop,struct ev_io * watcher,int eventID);
void CBDidConnectTimeout(struct ev_loop * loop,struct ev_timer * watcher,int eventID);
void CBCanSend(struct ev_loop * loop,struct ev_io * watcher,int eventID);
void CBCanSendTimeout(struct ev_loop * loop,struct ev_timer * watcher,int eventID);
void CBCanReceive(struct ev_loop * loop,struct ev_io * watcher,int eventID);
void CBCanReceiveTimeout(struct ev_loop * loop,struct ev_timer * watcher,int eventID);
void CBFireTimer(struct ev_loop * loop,struct ev_timer * watcher,int eventID);
void CBDoRun(struct ev_loop * loop,struct ev_async * watcher,int event);
/**
 @brief Runs a callback on the network thread.
 @param loopID The loop ID
 @returns true if sucessful, false otherwise.
 */
bool CBRunOnNetworkThread(uint64_t loopID,void (*callback)(void *),void * arg);

typedef struct{
	struct ev_async base;
	void * loop;
}CBAsyncEvent;

typedef struct{
	struct ev_loop * base;
	void (*onError)(void *);
	void (*onTimeOut)(void *,void *,CBTimeOutType); /**< Callback for timeouts */
	void * communicator;
	pthread_t loopThread; /**< The thread ID for the event loop. */
	void  (*userCallback)(void *);
	void * userArg;
	CBAsyncEvent * userEvent;
}CBEventLoop;

union CBOnEvent{
	void (*i)(void *,uint64_t);
	void (*ptr)(void *,void *);
};

typedef struct{
	struct ev_timer base;
	CBEventLoop * loop;
	void (*callback)(void *);
	void * arg;
	void * node;
}CBTimer;

typedef struct{
	struct ev_io base; /**< libev event. */
	CBEventLoop * loop; /**< For getting timeout events */
	union CBOnEvent onEvent;
	int socket;
	void * node;
	void (*timerCallback)(struct ev_loop *,struct ev_timer *,int);
	CBTimer * timeout;
}CBIOEvent;

#endif
