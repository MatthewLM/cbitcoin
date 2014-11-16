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
		setsockopt(socketID->i, SOL_SOCKET, SO_NOSIGPIPE, &i, sizeof(i));
	// Make address reusable
	setsockopt(socketID->i, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i));
	// Make socket non-blocking
	fcntl(socketID->i, F_SETFL, fcntl(socketID->i, F_GETFL, 0) | O_NONBLOCK);
	return CB_SOCKET_OK;
}
bool CBSocketBind(CBDepObject * socketID, bool IPv6, int port){
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
		setsockopt(socketID->i, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
		if (bind(socketID->i, ptr->ai_addr, ptr->ai_addrlen) == -1) {
			CBLogWarning("Bind gave the error %s for address on port %u.", strerror(errno), port);
			close(socketID->i);
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
		setsockopt(socketID->i, SOL_SOCKET, SO_NOSIGPIPE, &i, sizeof(i));
	}
	// Make socket non-blocking
	fcntl(socketID->i, F_SETFL, fcntl(socketID->i, F_GETFL, 0) | O_NONBLOCK);
	return true;
}
bool CBSocketConnect(CBDepObject socketID, unsigned char * IP, bool IPv6, int port){
	// Create sockaddr_in6 information for a IPv6 address
	int res;
	if (IPv6) {
		struct sockaddr_in6 address;
		memset(&address, 0, sizeof(address)); // Clear structure.
		address.sin6_family = AF_INET6;
		memcpy(&address.sin6_addr, IP, 16); // Move IP address into place.
		address.sin6_port = htons(port); // Port number to network order
		res = connect(socketID.i, (struct sockaddr *)&address, sizeof(address));
	}else{
		struct sockaddr_in address;
		memset(&address, 0, sizeof(address)); // Clear structure.
		address.sin_family = AF_INET;
		memcpy(&address.sin_addr, IP + 12, 4); // Move IP address into place. Last 4 bytes for IPv4.
		address.sin_port = htons(port); // Port number to network order
		res = connect(socketID.i, (struct sockaddr *)&address, sizeof(address));
	}
	if (res < 0 && errno == EINPROGRESS)
		return true;
	return false;
}
bool CBSocketListen(CBDepObject socketID, int maxConnections){
	if(listen(socketID.i, maxConnections) == -1)
		return false;
	return true;
}
bool CBSocketAccept(CBDepObject socketID, CBDepObject * connectionSocketID){
	connectionSocketID->i = accept(socketID.i, NULL, NULL);
	if (connectionSocketID->i == -1)
		return false;
	// Make socket non-blocking
	fcntl(connectionSocketID->i, F_SETFL, fcntl(connectionSocketID->i, F_GETFL, 0) | O_NONBLOCK);
	// Stop SIGPIPE
	if (CB_NOSIGPIPE) {
		int i = 1;
		setsockopt(connectionSocketID->i, SOL_SOCKET, SO_NOSIGPIPE, &i, sizeof(i));
	}
	return true;
}
bool CBNewEventLoop(CBDepObject * loopID,void (*onError)(void *),void (*onDidTimeout)(void *,void *,CBTimeOutType),void * communicator){
	struct ev_loop * base = ev_loop_new(0);
	// Create arguments for the loop
	CBEventLoop * loop = malloc(sizeof(*loop));
	loop->base = base;
	loop->onError = onError;
	loop->onTimeOut = onDidTimeout;
	loop->communicator = communicator;
	loop->userEvent = malloc(sizeof(*loop->userEvent));
	loop->userEvent->loop = loop;
	ev_async_init((struct ev_async *)loop->userEvent, CBDoRun);
	ev_async_start(base, (struct ev_async *)loop->userEvent);
	// Create queue
	CBInitCallbackQueue(&loop->queue);
	// Create thread
	CBNewThread(&loop->loopThread, CBStartEventLoop, loop);
	loopID->ptr = loop;
	return true;
}
void CBStartEventLoop(void * vloop){
	CBEventLoop * loop = vloop;
	CBLogVerbose("Starting network event loop.");
	// Start event loop
	ev_run(loop->base, 0);
	// Break from loop. Free everything.
	ev_loop_destroy(loop->base);
	free(loop->userEvent);
	CBFreeCallbackQueue(&loop->queue);
	CBFreeThread(loop->loopThread);
	free(loop);
}
bool CBSocketCanAcceptEvent(CBDepObject * eventID, CBDepObject loopID, CBDepObject socketID, void (*onCanAccept)(void *, CBDepObject)){
	CBIOEvent * event = malloc(sizeof(*event));
	event->loop = loopID.ptr;
	event->onEvent.i = onCanAccept;
	event->socket = socketID;
	ev_io_init((struct ev_io *)event, CBCanAccept, socketID.i, EV_READ);
	eventID->ptr = event;
	return true;
}
void CBCanAccept(struct ev_loop * loop,struct ev_io * watcher,int eventID){
	CBIOEvent * event = (CBIOEvent *)watcher;
	event->onEvent.i(event->loop->communicator, event->socket);
}
bool CBSocketDidConnectEvent(CBDepObject * eventID, CBDepObject loopID, CBDepObject socketID, void (*onDidConnect)(void *, void *), void * peer){
	CBIOEvent * event = malloc(sizeof(*event));
	event->loop = loopID.ptr;
	event->onEvent.ptr = onDidConnect;
	event->peer = peer;
	event->timerCallback = CBDidConnectTimeout;
	event->socket = socketID;
	ev_io_init((struct ev_io *)event, CBDidConnect, socketID.i, EV_WRITE);
	eventID->ptr = event;
	return true;
}
void CBDidConnect(struct ev_loop * loop, struct ev_io * watcher, int eventID){
	CBIOEvent * event = (CBIOEvent *)watcher;
	// This is a one-shot event.
	CBSocketRemoveEvent((CBDepObject){.ptr=watcher});
	// Check for an error
	int optval = -1;
	socklen_t optlen = sizeof(optval);
	getsockopt(event->socket.i, SOL_SOCKET, SO_ERROR, &optval, &optlen);
	if (optval)
		// Act as timeout
		event->loop->onTimeOut(event->loop->communicator, event->peer, CB_TIMEOUT_CONNECT_ERROR);
	else
		// Connection successful
		event->onEvent.ptr(event->loop->communicator, event->peer);
}
void CBDidConnectTimeout(struct ev_loop * loop,struct ev_timer * watcher,int eventID){
	CBTimer * event = (CBTimer *) watcher;
	event->loop->onTimeOut(event->loop->communicator,event->peer,CB_TIMEOUT_CONNECT);
}
bool CBSocketCanSendEvent(CBDepObject * eventID, CBDepObject loopID, CBDepObject socketID, void (*onCanSend)(void *, void *), void * peer){
	CBIOEvent * event = malloc(sizeof(*event));
	event->loop = loopID.ptr;
	event->onEvent.ptr = onCanSend;
	event->peer = peer;
	event->timerCallback = CBCanSendTimeout;
	event->timeout = NULL;
	ev_io_init((struct ev_io *)event, CBCanSend, socketID.i, EV_WRITE);
	eventID->ptr = event;
	return true;
}
void CBCanSend(struct ev_loop * loop, struct ev_io * watcher, int eventID){
	CBIOEvent * event = (CBIOEvent *)watcher;
	// Reset timeout
	if (event->timeout)
		ev_timer_again(loop, (struct ev_timer *)event->timeout);
	// Can send
	event->onEvent.ptr(event->loop->communicator,event->peer);
}
void CBCanSendTimeout(struct ev_loop * loop, struct ev_timer * watcher, int eventID){
	CBTimer * event = (CBTimer *) watcher;
	event->loop->onTimeOut(event->loop->communicator, event->peer, CB_TIMEOUT_SEND);
}
bool CBSocketCanReceiveEvent(CBDepObject * eventID, CBDepObject loopID, CBDepObject socketID, void (*onCanReceive)(void *, void *), void * peer){
	CBIOEvent * event = malloc(sizeof(*event));
	event->loop = loopID.ptr;
	event->onEvent.ptr = onCanReceive;
	event->peer = peer;
	event->timerCallback = CBCanReceiveTimeout;
	ev_io_init((struct ev_io *)event, CBCanReceive, socketID.i, EV_READ);
	eventID->ptr = event;
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
bool CBSocketAddEvent(CBDepObject eventID, int timeout){
	CBIOEvent * event = eventID.ptr;
	ev_io_start(event->loop->base, (struct ev_io *)event);
	if (timeout) {
		// Add timer
		event->timeout = malloc(sizeof(*event->timeout));
		event->timeout->loop = event->loop;
		event->timeout->peer = event->peer;
		float timeoutFloat = (float)timeout / 1000;
		ev_timer_init((struct ev_timer *)event->timeout, event->timerCallback, timeoutFloat, timeoutFloat);
		ev_timer_start(event->loop->base, (struct ev_timer *)event->timeout);
	}else
		event->timeout = NULL;
	return true;

}
bool CBSocketRemoveEvent(CBDepObject eventID){
	CBIOEvent * event = eventID.ptr;
	if (event->timeout){
		ev_timer_stop(event->loop->base, (struct ev_timer *)event->timeout);
		free(event->timeout);
		event->timeout = NULL;
	}
	ev_io_stop(event->loop->base, (struct ev_io *)event);
	return true;
}
void CBSocketFreeEvent(CBDepObject eventID){
	CBSocketRemoveEvent(eventID);
	free(eventID.ptr);
}
int32_t CBSocketSend(CBDepObject socketID, unsigned char * data, int len){
	ssize_t res = send(socketID.i, data, len, CB_SEND_FLAGS);
	if (res >= 0)
		return (int32_t)res;
	if (errno == EAGAIN)
		return 0; // False event. Wait again.
	return CB_SOCKET_FAILURE; // Failure
}
int32_t CBSocketReceive(CBDepObject socketID, unsigned char * data, int len){
	ssize_t res = read(socketID.i, data, len);
	if (res > 0)
		return (int32_t)res; // OK, read data.
	if (! res)
		return CB_SOCKET_CONNECTION_CLOSE; // If read() gives zero it means the connection was closed.
	if (errno == EAGAIN)
		return 0; // False event. Wait again. No bytes read.
	return CB_SOCKET_FAILURE; // Failure
}
bool CBStartTimer(CBDepObject loopID, CBDepObject * timer, int time, void (*callback)(void *), void * arg){
	CBTimer * theTimer = malloc(sizeof(*theTimer));
	theTimer->callback = callback;
	theTimer->arg = arg;
	theTimer->loop = loopID.ptr;
	ev_timer_init((struct ev_timer *)theTimer, CBFireTimer, (float)time / 1000, (float)time / 1000);
	ev_timer_start(theTimer->loop->base, (struct ev_timer *)theTimer);
	timer->ptr = theTimer;
	return true;
}
void CBFireTimer(struct ev_loop * loop,struct ev_timer * watcher,int eventID){
	CBTimer * theTimer = (CBTimer *)watcher;
	theTimer->callback(theTimer->arg);
}
void CBEndTimer(CBDepObject timer){
	CBTimer * theTimer = timer.ptr;
	ev_timer_stop(theTimer->loop->base, (struct ev_timer *)theTimer);
	free(theTimer);
}
void CBDoRun(struct ev_loop * loop,struct ev_async * watcher,int event){
	CBEventLoop * evloop = (CBEventLoop *)((CBAsyncEvent *)watcher)->loop;
	CBCallbackQueueRun(&evloop->queue);
}
bool CBRunOnEventLoop(CBDepObject loopID, void (*callback)(void *), void * arg, bool block){
	CBEventLoop * loop = loopID.ptr;
	bool done = !block;
	if (pthread_equal(((CBThread *)loop->loopThread.ptr)->thread, pthread_self()) != 0){
		// We are in the event loop already.
		callback(arg);
		return true;
	}
	CBCallbackQueueItem * item = CBCallbackQueueAdd(&loop->queue, callback, arg, &done);
	ev_async_send(loop->base, (struct ev_async *)loop->userEvent);
	if (block)
		CBCallbackQueueWait(item);
	return true;
}
void CBCloseSocket(CBDepObject socketID){
	close(socketID.i);
}
void CBExitEventLoop(CBDepObject loopID){
	CBEventLoop * loop = loopID.ptr;
	ev_unloop(loop->base, EVUNLOOP_ONE);
}
