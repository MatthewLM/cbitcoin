//
//  CBLibEventSockets.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 30/05/2012.
//  Copyright (c) 2012 Matthew Mitchell
//  
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

#include "CBLibEventSockets.h"

// Implementation

CBSocketReturn CBNewSocket(CBDepObject * socketID, bool IPv6){
	// You need to use PF_INET for IPv4 mapped IPv6 addresses despite using the IPv6 format.
	socketID->i = socket(IPv6 ? PF_INET6 : PF_INET, SOCK_STREAM, 0);
	if (socketID->i == -1) {
		if (errno == EAFNOSUPPORT || errno == EPROTONOSUPPORT)
			return CB_SOCKET_NO_SUPPORT;
		return CB_SOCKET_BAD;
	}
	// Stop SIGPIPE annoying us.
	int i = 1;
	if (CB_NOSIGPIPE)
		setsockopt((evutil_socket_t)socketID->i, SOL_SOCKET, SO_NOSIGPIPE, &i, sizeof(i));
	// Make address reusable
	setsockopt((evutil_socket_t)socketID->i, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i));
	// Make socket non-blocking
	evutil_make_socket_nonblocking((evutil_socket_t)socketID->i);
	return CB_SOCKET_OK;
}
bool CBSocketBind(CBDepObject * socketID, bool IPv6, uint16_t port){
	struct addrinfo hints, *res, *ptr;
	// Set hints for the computer's addresses.
	memset(&hints, 0, sizeof(hints));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = IPv6 ? AF_INET6 : AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	// Get host for listening
	char portStr[6];
	sprintf(portStr, "%u", port);
	if (getaddrinfo(NULL, portStr, &hints, &res))
		return false;
	// Attempt to bind to one of the addresses.
	for(ptr = res; ptr != NULL; ptr = ptr->ai_next) {
		if ((socketID->i = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol)) == -1)
			continue;
		// Prevent EADDRINUSE
		int opt = 1;
		setsockopt((evutil_socket_t)socketID->i, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
		if (bind((evutil_socket_t)socketID->i, ptr->ai_addr, ptr->ai_addrlen) == -1) {
			CBLogWarning("Bind gave the error %s for address on port %u.", strerror(errno), port);
			evutil_closesocket((evutil_socket_t)socketID->i);
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
		setsockopt((evutil_socket_t)socketID->i, SOL_SOCKET, SO_NOSIGPIPE, &i, sizeof(i));
	}
	// Make socket non-blocking
	evutil_make_socket_nonblocking((evutil_socket_t)socketID->i);
	return true;
}
bool CBSocketConnect(CBDepObject socketID, uint8_t * IP, bool IPv6, uint16_t port){
	// Create sockaddr_in6 information for a IPv6 address
	int res;
	if (IPv6) {
		struct sockaddr_in6 address;
		memset(&address, 0, sizeof(address)); // Clear structure.
		address.sin6_family = AF_INET6;
		memcpy(&address.sin6_addr, IP, 16); // Move IP address into place.
		address.sin6_port = htons(port); // Port number to network order
		res = connect((evutil_socket_t)socketID.i, (struct sockaddr *)&address, sizeof(address));
	}else{
		struct sockaddr_in address;
		memset(&address, 0, sizeof(address)); // Clear structure.
		address.sin_family = AF_INET;
		memcpy(&address.sin_addr, IP + 12, 4); // Move IP address into place. Last 4 bytes for IPv4.
		address.sin_port = htons(port); // Port number to network order
		res = connect((evutil_socket_t)socketID.i, (struct sockaddr *)&address, sizeof(address));
	}
	if (res < 0 && errno == EINPROGRESS)
		return true;
	return false;
}
bool CBSocketListen(CBDepObject socketID, uint16_t maxConnections){
	if(listen((evutil_socket_t)socketID.i, maxConnections) == -1)
		return false;
	return true;
}
bool CBSocketAccept(CBDepObject socketID, CBDepObject * connectionSocketID){
	connectionSocketID->i = accept((evutil_socket_t)socketID.i, NULL, NULL);
	if (connectionSocketID->i == -1)
		return false;
	// Make socket non-blocking
	evutil_make_socket_nonblocking((evutil_socket_t)connectionSocketID->i);
	// Stop SIGPIPE
	if (CB_NOSIGPIPE) {
		int i = 1;
		setsockopt((evutil_socket_t)connectionSocketID->i, SOL_SOCKET, SO_NOSIGPIPE, &i, sizeof(i));
	}
	return true;
}
bool CBNewEventLoop(CBDepObject * loopID, void (*onError)(void *), void (*onDidTimeout)(void *, void *, CBTimeOutType), void * communicator){
	// Use threads
	evthread_use_pthreads();
	struct event_base * base = event_base_new();
	// Create dummy event to maintain loop. When libevent 2.1 is released as stable EVLOOP_NO_EXIT_ON_EMPTY can be used. For now there is a hack solution.
	event_base_add_virtual(base);
	// Create arguments for the loop
	CBEventLoop * loop = malloc(sizeof(*loop));
	loop->base = base;
	loop->onError = onError;
	loop->onTimeOut = onDidTimeout;
	loop->communicator = communicator;
	loop->userEvent = event_new(loop->base, 0, 0, CBDoRun, loop);
	// Create queue
	CBInitCallbackQueue(&loop->queue);
	// Create thread
	CBNewThread(&loop->loopThread, CBStartEventLoop, loop);
	loopID->ptr = loop;
	return true;
}
void CBStartEventLoop(void * vloop){
	CBEventLoop * loop = vloop;
	// Start event loop
	CBLogVerbose("Starting network event loop.");
	if(event_base_dispatch(loop->base) == -1){
		// Error
		loop->onError(loop->communicator);
	}
	// Break from loop. Free everything.
	event_base_free(loop->base);
	CBFreeCallbackQueue(&loop->queue);
	free(loop);
}
bool CBSocketCanAcceptEvent(CBDepObject * eventID, CBDepObject loopID, CBDepObject socketID, void (*onCanAccept)(void *, CBDepObject)){
	CBEvent * event = malloc(sizeof(*event));
	event->loop = loopID.ptr;
	event->onEvent.i = onCanAccept;
	event->event = event_new(event->loop->base, (evutil_socket_t)socketID.i, EV_READ|EV_PERSIST, CBCanAccept, event);
	eventID->ptr = event;
	return true;
}
void CBCanAccept(evutil_socket_t socketID, short eventNum, void * arg){
	CBEvent * event = arg;
	CBDepObject socket;
	socket.i = socketID;
	event->onEvent.i(event->loop->communicator, socket);
}
bool CBSocketDidConnectEvent(CBDepObject * eventID, CBDepObject loopID, CBDepObject socketID, void (*onDidConnect)(void *, void *), void * peer){
	CBEvent * event = malloc(sizeof(*event));
	event->loop = loopID.ptr;
	event->onEvent.ptr = onDidConnect;
	event->peer = peer;
	event->event = event_new(event->loop->base, (evutil_socket_t)socketID.i, EV_TIMEOUT|EV_WRITE, CBDidConnect, event);
	eventID->ptr = event;
	return true;
}
void CBDidConnect(evutil_socket_t socketID, short eventNum, void * arg){
	CBEvent * event = arg;
	if (eventNum & EV_TIMEOUT) {
		// Timeout for the connection
		event->loop->onTimeOut(event->loop->communicator, event->peer, CB_TIMEOUT_CONNECT);
	}else{
		int optval = -1;
		socklen_t optlen = sizeof(optval);
		getsockopt(socketID, SOL_SOCKET, SO_ERROR, &optval, &optlen);
		if (optval){
			// Act as timeout
			CBLogError("Connection error: %s", strerror(optval));
			event->loop->onTimeOut(event->loop->communicator, event->peer, CB_TIMEOUT_CONNECT_ERROR);
		}else
			// Connection successful
			event->onEvent.ptr(event->loop->communicator, event->peer);
	}
}
bool CBSocketCanSendEvent(CBDepObject * eventID, CBDepObject loopID, CBDepObject socketID, void (*onCanSend)(void *, void *), void * peer){
	CBEvent * event = malloc(sizeof(*event));
	event->loop = loopID.ptr;
	event->onEvent.ptr = onCanSend;
	event->peer = peer;
	event->event = event_new(event->loop->base, (evutil_socket_t)socketID.i, EV_TIMEOUT|EV_WRITE|EV_PERSIST, CBCanSend, event);
	eventID->ptr = event;
	return true;
}
void CBCanSend(evutil_socket_t socketID, short eventNum, void * arg){
	CBEvent * event = arg;
	if (eventNum & EV_TIMEOUT) {
		// Timeout when waiting to write.
		event->loop->onTimeOut(event->loop->communicator, event->peer, CB_TIMEOUT_SEND);
	}else{
		// Can send
		event->onEvent.ptr(event->loop->communicator, event->peer);
	}
}
bool CBSocketCanReceiveEvent(CBDepObject * eventID, CBDepObject loopID, CBDepObject socketID, void (*onCanReceive)(void *, void *), void * peer){
	CBEvent * event = malloc(sizeof(*event));
	event->loop = loopID.ptr;
	event->onEvent.ptr = onCanReceive;
	event->peer = peer;
	event->event = event_new(event->loop->base, (evutil_socket_t)socketID.i, EV_TIMEOUT|EV_READ|EV_PERSIST, CBCanReceive, event);
	eventID->ptr = event;
	return true;
}
void CBCanReceive(evutil_socket_t socketID, short eventNum, void * arg){
	CBEvent * event = arg;
	if (eventNum & EV_TIMEOUT) {
		// Timeout when waiting to receive
		event->loop->onTimeOut(event->loop->communicator, event->peer, CB_TIMEOUT_RECEIVE);
	}else{
		// Can receive
		event->onEvent.ptr(event->loop->communicator, event->peer);
	}
}
bool CBSocketAddEvent(CBDepObject eventID, uint32_t timeout){
	CBEvent * event = eventID.ptr;
	int res;
	if (timeout) {
		uint32_t secs = timeout / 1000;
		struct timeval time = {secs, (timeout - secs*1000) * 1000};
		res = event_add(event->event, &time);
	}else
		res = event_add(event->event, NULL);
	return ! res;
}
bool CBSocketRemoveEvent(CBDepObject eventID){
	CBEvent * event = eventID.ptr;
	if(event_del(event->event))
		return false;
	else
		return true;
}
void CBSocketFreeEvent(CBDepObject eventID){
	CBEvent * event = eventID.ptr;
	event_free(event->event);
	free(event);
}
int32_t CBSocketSend(CBDepObject socketID, uint8_t * data, uint32_t len){
	ssize_t res = send((evutil_socket_t)socketID.i, data, len, CB_SEND_FLAGS);
	if (res >= 0)
		return (int32_t)res;
	if (errno == EAGAIN)
		return 0; // False event. Wait again.
	return CB_SOCKET_FAILURE; // Failure
}
int32_t CBSocketReceive(CBDepObject socketID, uint8_t * data, uint32_t len){
	ssize_t res = read((evutil_socket_t)socketID.i, data, len);
	if (res > 0)
		return (int32_t)res; // OK, read data.
	if (! res)
		return CB_SOCKET_CONNECTION_CLOSE; // If read() gives zero it means the connection was closed.
	if (errno == EAGAIN)
		return 0; // False event. Wait again. No bytes read.
	return CB_SOCKET_FAILURE; // Failure
}
bool CBStartTimer(CBDepObject loopID, CBDepObject * timer, uint16_t time, void (*callback)(void *), void * arg){
	CBTimer * theTimer = malloc(sizeof(*theTimer));
	theTimer->callback = callback;
	theTimer->arg = arg;
	theTimer->timer = event_new(((CBEventLoop *)loopID.ptr)->base, -1, EV_PERSIST, CBFireTimer, theTimer);
	timer->ptr = theTimer;
	int res;
	if (time) {
		uint32_t secs = time / 1000;
		struct timeval timev = {secs, (time - secs*1000) * 1000};
		res = event_add(theTimer->timer, &timev);
	}else
		res = event_add(theTimer->timer, NULL);
	return ! res;
}
void CBFireTimer(evutil_socket_t foo, short bar, void * timer){
	CBTimer * theTimer = timer;
	theTimer->callback(theTimer->arg);
}
void CBEndTimer(CBDepObject timer){
	CBTimer * theTimer = timer.ptr;
	event_free(theTimer->timer);
	free(theTimer);
}
void CBDoRun(evutil_socket_t socketID, short eventNum, void * arg){
	CBEventLoop * loop = arg;
	CBCallbackQueueRun(&loop->queue);
}
bool CBRunOnEventLoop(CBDepObject loopID, void (*callback)(void *), void * arg, bool block){
	CBEventLoop * loop = loopID.ptr;
	bool done = !block;
	if (block && pthread_equal(((CBThread *)loop->loopThread.ptr)->thread, pthread_self()) != 0){
		// We are in the event loop already and we are supposed to block.
		callback(arg);
		return true;
	}
	CBCallbackQueueItem * item = CBCallbackQueueAdd(&loop->queue, callback, arg, &done);
	event_active(loop->userEvent, 0, 0);
	if (block)
		CBCallbackQueueWait(item);
	return true;
}
void CBCloseSocket(CBDepObject socketID){
	evutil_closesocket((evutil_socket_t)socketID.i);
}
void CBExitEventLoop(CBDepObject loopID){
	CBEventLoop * loop = loopID.ptr;
	if(event_base_loopbreak(loop->base)){
		// Error occured. No choice but to do a dirty closure.
		pthread_cancel(((CBThread *)loop->loopThread.ptr)->thread);
		event_base_free(loop->base);
		// Destory queue
		CBFreeCallbackQueue(&loop->queue);
		free(loop);
	}
}
