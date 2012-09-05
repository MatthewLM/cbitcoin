//
//  CBLibevSockets.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 19/08/2012.
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

#include "CBLibevSockets.h"

// Implementation

CBSocketReturn CBNewSocket(uint64_t * socketID,bool IPv6){
	*socketID = socket(IPv6 ? PF_INET6 : PF_INET, SOCK_STREAM, 0);
	if (*socketID == -1) {
		if (errno == EAFNOSUPPORT || errno == EPROTONOSUPPORT) {
			return CB_SOCKET_NO_SUPPORT;
		}
		return CB_SOCKET_BAD;
	}
	// Stop SIGPIPE annoying us.
	if (CB_NOSIGPIPE) {
		int i = 1;
		setsockopt((int)*socketID, SOL_SOCKET, SO_NOSIGPIPE, &i, sizeof(i));
	}
	// Make socket non-blocking
	fcntl((int)*socketID, F_SETFL, fcntl((int)*socketID, F_GETFL, 0) | O_NONBLOCK);
	return CB_SOCKET_OK;
}
bool CBSocketBind(uint64_t * socketID,bool IPv6,uint16_t port){
	struct addrinfo hints,*res,*ptr;
	// Set hints for the computer's addresses.
	memset(&hints, 0, sizeof(hints));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = IPv6 ? AF_INET6 : AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	// Get host for listening
	char portStr[6];
	sprintf(portStr, "%u",port);
	if (getaddrinfo(NULL, portStr, &hints, &res))
		return false;
	// Attempt to bind to one of the addresses.
	for(ptr = res; ptr != NULL; ptr = ptr->ai_next) {
		if ((*socketID = socket(ptr->ai_family, ptr->ai_socktype,ptr->ai_protocol)) == -1)
			continue;
		// Prevent EADDRINUSE
		int opt = 1;
		setsockopt((int)*socketID, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
		if (bind((int)*socketID, ptr->ai_addr, ptr->ai_addrlen) == -1) {
			close((int)*socketID);
			continue;
		}
		break; // Success.
	}
	freeaddrinfo(res);
	if (ptr == NULL) // Failure
		return false;
	// Prevent SIGPIPE
	if (CB_NOSIGPIPE) {
		int i = 1;
		setsockopt((int)*socketID, SOL_SOCKET, SO_NOSIGPIPE, &i, sizeof(i));
	}
	// Make socket non-blocking
	fcntl((int)*socketID, F_SETFL, fcntl((int)*socketID, F_GETFL, 0) | O_NONBLOCK);
	return true;
}
bool CBSocketConnect(uint64_t socketID,uint8_t * IP,bool IPv6,uint16_t port){
	// Create sockaddr_in6 information for a IPv6 address
	int res;
	if (IPv6) {
		struct sockaddr_in6 address;
		memset(&address, 0, sizeof(address)); // Clear structure.
		address.sin6_family = AF_INET6;
		memcpy(&address.sin6_addr, IP, 16); // Move IP address into place.
		address.sin6_port = htons(port); // Port number to network order
		res = connect((int)socketID, (struct sockaddr *)&address, sizeof(address));
	}else{
		struct sockaddr_in address;
		memset(&address, 0, sizeof(address)); // Clear structure.
		address.sin_family = AF_INET;
		memcpy(&address.sin_addr, IP + 12, 4); // Move IP address into place. Last 4 bytes for IPv4.
		address.sin_port = htons(port); // Port number to network order
		res = connect((int)socketID, (struct sockaddr *)&address, sizeof(address));
	}
	if (res < 0 && errno == EINPROGRESS)
		return true;
	return false;
}
bool CBSocketListen(uint64_t socketID,uint16_t maxConnections){
	if(listen((int)socketID, maxConnections) == -1){
		return false;
	}
	return true;
}
bool CBSocketAccept(uint64_t socketID,uint64_t * connectionSocketID){
	*connectionSocketID = accept((int)socketID, NULL, NULL);
	if (*connectionSocketID == -1) {
		return false;
	}
	// Stop SIGPIPE
	if (CB_NOSIGPIPE) {
		int i = 1;
		setsockopt((int)*connectionSocketID, SOL_SOCKET, SO_NOSIGPIPE, &i, sizeof(i));
	}
	// Make socket non-blocking
	fcntl((int)*connectionSocketID, F_SETFL, fcntl((int)*connectionSocketID, F_GETFL, 0) | O_NONBLOCK);
	return true;
}
bool CBNewEventLoop(uint64_t * loopID,void (*onError)(void *),void (*onDidTimeout)(void *,void *,CBTimeOutType),void * communicator){
	struct ev_loop * base = ev_loop_new(0);
	// Create arguments for the loop
	CBEventLoop * loop = malloc(sizeof(*loop));
	if (NOT loop)
		return false;
	loop->base = base;
	loop->onError = onError;
	loop->onTimeOut = onDidTimeout;
	loop->communicator = communicator;
	loop->userEvent = malloc(sizeof(*loop->userEvent));
	if (NOT loop->userEvent){
		free(loop);
		return false;
	}
	ev_async_init((struct ev_async *)loop->userEvent, CBDoRun);
	ev_async_start(base, (struct ev_async *)loop->userEvent);
	// Create thread attributes explicitly for portability reasons.
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE); // May need to be joinable.
	// Create joinable thread
	if(pthread_create(&loop->loopThread, &attr, CBStartEventLoop, loop)){
		// Thread creation failed.
		pthread_attr_destroy(&attr);
		ev_loop_destroy(base);
		free(loop->userEvent);
		return 0;
	}
	pthread_attr_destroy(&attr);
	*loopID = (uint64_t)loop;
	return loop;
}
void * CBStartEventLoop(void * vloop){
	CBEventLoop * loop = vloop;
	// Start event loop
	ev_run(loop->base, 0);
	// Break from loop. Free everything.
	ev_loop_destroy(loop->base);
	free(loop->userEvent);
	free(loop);
	return NULL;
}
bool CBSocketCanAcceptEvent(uint64_t * eventID,uint64_t loopID,uint64_t socketID,void (*onCanAccept)(void *,uint64_t)){
	CBIOEvent * event = malloc(sizeof(*event));
	if (NOT event)
		return false;
	event->loop = (CBEventLoop *) loopID;
	event->onEvent.i = onCanAccept;
	event->socket = (int)socketID;
	ev_io_init((struct ev_io *)event, CBCanAccept, (int)socketID, EV_READ);
	*eventID = (uint64_t)event;
	return true;
}
void CBCanAccept(struct ev_loop * loop,struct ev_io * watcher,int eventID){
	CBIOEvent * event = (CBIOEvent *)watcher;
	event->onEvent.i(event->loop->communicator,event->socket);
}
bool CBSocketDidConnectEvent(uint64_t * eventID,uint64_t loopID,uint64_t socketID,void (*onDidConnect)(void *,void *),void * peer){
	CBIOEvent * event = malloc(sizeof(*event));
	if (NOT event)
		return false;
	event->loop = (CBEventLoop *)loopID;
	event->onEvent.ptr = onDidConnect;
	event->peer = peer;
	event->timerCallback = CBDidConnectTimeout;
	ev_io_init((struct ev_io *)event, CBDidConnect, (int)socketID, EV_WRITE);
	*eventID = (uint64_t)event;
	return true;
}
void CBDidConnect(struct ev_loop * loop,struct ev_io * watcher,int eventID){
	CBIOEvent * event = (CBIOEvent *)watcher;
	// This is a one-shot event.
	CBSocketRemoveEvent((uint64_t)watcher);
	// Check for an error
	int optval = -1;
	socklen_t optlen = sizeof(optval);
	getsockopt(event->socket, SOL_SOCKET, SO_ERROR, &optval, &optlen);
	if (optval)
		// Act as timeout
		event->loop->onTimeOut(event->loop->communicator,event->peer,CB_TIMEOUT_CONNECT);
	else
		// Connection successful
		event->onEvent.ptr(event->loop->communicator,event->peer);
}
void CBDidConnectTimeout(struct ev_loop * loop,struct ev_timer * watcher,int eventID){
	CBTimer * event = (CBTimer *) watcher;
	event->loop->onTimeOut(event->loop->communicator,event->peer,CB_TIMEOUT_CONNECT);
}
bool CBSocketCanSendEvent(uint64_t * eventID,uint64_t loopID,uint64_t socketID,void (*onCanSend)(void *,void *),void * peer){
	CBIOEvent * event = malloc(sizeof(*event));
	if (NOT event)
		return false;
	event->loop = (CBEventLoop *)loopID;
	event->onEvent.ptr = onCanSend;
	event->peer = peer;
	event->timerCallback = CBCanSendTimeout;
	ev_io_init((struct ev_io *)event, CBCanSend, (int)socketID, EV_WRITE);
	*eventID = (uint64_t)event;
	return true;
}
void CBCanSend(struct ev_loop * loop,struct ev_io * watcher,int eventID){
	CBIOEvent * event = (CBIOEvent *)watcher;
	// Reset timeout
	if (event->timeout)
		ev_timer_again(loop, (struct ev_timer *)event->timeout);
	// Can send
	event->onEvent.ptr(event->loop->communicator,event->peer);
}
void CBCanSendTimeout(struct ev_loop * loop,struct ev_timer * watcher,int eventID){
	CBTimer * event = (CBTimer *) watcher;
	event->loop->onTimeOut(event->loop->communicator,event->peer,CB_TIMEOUT_SEND);
}
bool CBSocketCanReceiveEvent(uint64_t * eventID,uint64_t loopID,uint64_t socketID,void (*onCanReceive)(void *,void *),void * peer){
	CBIOEvent * event = malloc(sizeof(*event));
	if (NOT event)
		return false;
	event->loop = (CBEventLoop *)loopID;
	event->onEvent.ptr = onCanReceive;
	event->peer = peer;
	event->timerCallback = CBCanReceiveTimeout;
	ev_io_init((struct ev_io *)event, CBCanReceive, (int)socketID, EV_READ);
	*eventID = (uint64_t)event;
	return true;
}
void CBCanReceive(struct ev_loop * loop,struct ev_io * watcher,int eventID){
	CBIOEvent * event = (CBIOEvent *)watcher;
	// Reset timeout
	if (event->timeout)
		ev_timer_again(loop, (struct ev_timer *)event->timeout);
	// Can receive
	event->onEvent.ptr(event->loop->communicator,event->peer);
}
void CBCanReceiveTimeout(struct ev_loop * loop,struct ev_timer * watcher,int eventID){
	CBTimer * event = (CBTimer *) watcher;
	event->loop->onTimeOut(event->loop->communicator,event->peer,CB_TIMEOUT_RECEIVE);
}
bool CBSocketAddEvent(uint64_t eventID,uint32_t timeout){
	CBIOEvent * event = (CBIOEvent *)eventID;
	ev_io_start(event->loop->base, (struct ev_io *)event);
	if (timeout) {
		// Add timer
		event->timeout = malloc(sizeof(*event->timeout));
		if (NOT event->timeout){
			ev_io_stop(event->loop->base, (struct ev_io *)event);
			return false;
		}
		event->timeout->loop = event->loop;
		event->timeout->peer = event->peer;
		ev_timer_init((struct ev_timer *)event->timeout, event->timerCallback, (float)timeout / 1000, (float)timeout / 1000);
		ev_timer_start(event->loop->base, (struct ev_timer *)event->timeout);
	}else
		event->timeout = NULL;
	return true;
}
bool CBSocketRemoveEvent(uint64_t eventID){
	CBIOEvent * event = (CBIOEvent *)eventID;
	if (event->timeout){
		ev_timer_stop(event->loop->base, (struct ev_timer *)event->timeout);
		free(event->timeout);
		event->timeout = NULL;
	}
	ev_io_stop(event->loop->base, (struct ev_io *)event);
	return true;
}
void CBSocketFreeEvent(uint64_t eventID){
	CBSocketRemoveEvent(eventID);
	free((CBIOEvent *)eventID);
}
int32_t CBSocketSend(uint64_t socketID,uint8_t * data,uint32_t len){
	ssize_t res = send((int)socketID, data, len, CB_SEND_FLAGS);
	if (res >= 0)
		return (int32_t)res;
	if (errno == EAGAIN)
		return 0; // False event. Wait again.
	return CB_SOCKET_FAILURE; // Failure
}
int32_t CBSocketReceive(uint64_t socketID,uint8_t * data,uint32_t len){
	ssize_t res = read((int)socketID, data, len);
	if (res > 0)
		return (int32_t)res; // OK, read data.
	if (NOT res)
		return CB_SOCKET_CONNECTION_CLOSE; // If read() gives zero it means the connection was closed.
	if (errno == EAGAIN)
		return 0; // False event. Wait again. No bytes read.
	return CB_SOCKET_FAILURE; // Failure
}
bool CBStartTimer(uint64_t loopID,uint64_t * timer,uint16_t time,void (*callback)(void *),void * arg){
	CBTimer * theTimer = malloc(sizeof(*theTimer));
	if (NOT theTimer)
		return false;
	theTimer->callback = callback;
	theTimer->arg = arg;
	theTimer->loop = (CBEventLoop *)loopID;
	ev_timer_init((struct ev_timer *)theTimer, CBFireTimer, (float)time / 1000, (float)time / 1000);
	ev_timer_start(((CBEventLoop *)loopID)->base, (struct ev_timer *)theTimer);
	*timer = (uint64_t)theTimer;
	return true;
}
void CBFireTimer(struct ev_loop * loop,struct ev_timer * watcher,int eventID){
	CBTimer * theTimer = (CBTimer *)watcher;
	theTimer->callback(theTimer->arg);
}
void CBEndTimer(uint64_t timer){
	CBTimer * theTimer = (CBTimer *)timer;
	ev_timer_stop(theTimer->loop->base, (struct ev_timer *)theTimer);
	free(theTimer);
}
void CBDoRun(struct ev_loop * loop,struct ev_async * watcher,int event){
	CBEventLoop * evloop = (CBEventLoop *)((CBAsyncEvent *)watcher)->loop;
	evloop->userCallback(evloop->userArg);
}
bool CBRunOnNetworkThread(uint64_t loopID,void (*callback)(void *),void * arg){
	CBEventLoop * loop = (CBEventLoop *) loopID;
	loop->userCallback = callback;
	loop->userArg = arg;
	ev_async_send(loop->base, (struct ev_async *)loop->userEvent);
	return true;
}
void CBCloseSocket(uint64_t socketID){
	close((int)socketID);
}
void CBExitEventLoop(uint64_t loopID){
	if (NOT loopID)
		return;
	CBEventLoop * loop = (CBEventLoop *) loopID;
	ev_unloop(loop->base, EVUNLOOP_ONE);
}
