//
//  CBLibEventSockets.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 30/05/2012.
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

#include "CBLibEventSockets.h"

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
		setsockopt((evutil_socket_t)*socketID, SOL_SOCKET, SO_NOSIGPIPE, &i, sizeof(i));
	}
	// Make socket non-blocking
	evutil_make_socket_nonblocking((evutil_socket_t)*socketID);
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
		int opt = 1;
		// Prevent EADDRINUSE
		setsockopt((evutil_socket_t)*socketID, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
		if (bind((evutil_socket_t)*socketID, ptr->ai_addr, ptr->ai_addrlen) == -1) {
			evutil_closesocket((evutil_socket_t)*socketID);
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
		setsockopt((evutil_socket_t)*socketID, SOL_SOCKET, SO_NOSIGPIPE, &i, sizeof(i));
	}
	// Make socket non-blocking
	evutil_make_socket_nonblocking((evutil_socket_t)*socketID);
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
		res = connect((evutil_socket_t)socketID, (struct sockaddr *)&address, sizeof(address));
	}else{
		struct sockaddr_in address;
		memset(&address, 0, sizeof(address)); // Clear structure.
		address.sin_family = AF_INET;
		memcpy(&address.sin_addr, IP + 12, 4); // Move IP address into place. Last 4 bytes for IPv4.
		address.sin_port = htons(port); // Port number to network order
		res = connect((evutil_socket_t)socketID, (struct sockaddr *)&address, sizeof(address));
	}
	if (res < 0 && errno == EINPROGRESS)
		return true;
	return false;
}
bool CBSocketListen(uint64_t socketID,uint16_t maxConnections){
	if(listen((evutil_socket_t)socketID, maxConnections) == -1){
		return false;
	}
	return true;
}
bool CBSocketAccept(uint64_t socketID,uint64_t * connectionSocketID){
	*connectionSocketID = accept((evutil_socket_t)socketID, NULL, NULL);
	if (*connectionSocketID == -1) {
		return false;
	}
	// Make socket non-blocking
	evutil_make_socket_nonblocking((evutil_socket_t)*connectionSocketID);
	// Stop SIGPIPE
	if (CB_NOSIGPIPE) {
		int i = 1;
		setsockopt((evutil_socket_t)*connectionSocketID, SOL_SOCKET, SO_NOSIGPIPE, &i, sizeof(i));
	}
	return true;
}
bool CBNewEventLoop(uint64_t * loopID,void (*onError)(void *),void (*onDidTimeout)(void *,void *,CBTimeOutType),void * communicator){
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
	// Create thread attributes explicitly for portability reasons.
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE); // May need to be joinable.
	// Create joinable thread
	if(pthread_create(&loop->loopThread, &attr, CBStartEventLoop, loop)){
		// Thread creation failed.
		pthread_attr_destroy(&attr);
		event_base_free(base);
		return 0;
	}
	pthread_attr_destroy(&attr);
	*loopID = (uint64_t)loop;
	return loop;
}
void * CBStartEventLoop(void * vloop){
	CBEventLoop * loop = vloop;
	// Start event loop
	if(event_base_dispatch(loop->base) == -1){
		// Error
		loop->onError(loop->communicator);
	}
	// Break from loop. Free everything.
	event_base_free(loop->base);
	free(loop);
	return NULL;
}
bool CBSocketCanAcceptEvent(uint64_t * eventID,uint64_t loopID,uint64_t socketID,void (*onCanAccept)(void *,uint64_t)){
	CBEvent * event = malloc(sizeof(*event));
	event->loop = (CBEventLoop *) loopID;
	event->onEvent.i = onCanAccept;
	event->event = event_new(((CBEventLoop *)loopID)->base, (evutil_socket_t)socketID, EV_READ|EV_PERSIST, CBCanAccept, event);
	if (NOT event->event) {
		free(event);
		event = 0;
	}
	*eventID = (uint64_t)event;
	return event;
}
void CBCanAccept(evutil_socket_t socketID,short eventNum,void * arg){
	CBEvent * event = arg;
	event->onEvent.i(event->loop->communicator,socketID);
}
bool CBSocketDidConnectEvent(uint64_t * eventID,uint64_t loopID,uint64_t socketID,void (*onDidConnect)(void *,void *),void * node){
	CBEvent * event = malloc(sizeof(*event));
	event->loop = (CBEventLoop *)loopID;
	event->onEvent.ptr = onDidConnect;
	event->node = node;
	event->event = event_new(((CBEventLoop *)loopID)->base, (evutil_socket_t)socketID, EV_TIMEOUT|EV_WRITE, CBDidConnect, event);
	if (NOT event->event) {
		free(event);
		event = 0;
	}
	*eventID = (uint64_t)event;
	return event;
}
void CBDidConnect(evutil_socket_t socketID,short eventNum,void * arg){
	CBEvent * event = arg;
	if (eventNum & EV_TIMEOUT) {
		// Timeout for the connection
		event->loop->onTimeOut(event->loop->communicator,event->node,CB_TIMEOUT_CONNECT);
	}else{
		int optval = -1;
		socklen_t optlen = sizeof(optval);
		getsockopt(socketID, SOL_SOCKET, SO_ERROR, &optval, &optlen);
		if (optval)
			// Act as timeout
			event->loop->onTimeOut(event->loop->communicator,event->node,CB_TIMEOUT_CONNECT);
		else
			// Connection successful
			event->onEvent.ptr(event->loop->communicator,event->node);
	}
}
bool CBSocketCanSendEvent(uint64_t * eventID,uint64_t loopID,uint64_t socketID,void (*onCanSend)(void *,void *),void * node){
	CBEvent * event = malloc(sizeof(*event));
	event->loop = (CBEventLoop *)loopID;
	event->onEvent.ptr = onCanSend;
	event->node = node;
	event->event = event_new(((CBEventLoop *)loopID)->base, (evutil_socket_t)socketID, EV_TIMEOUT|EV_WRITE|EV_PERSIST, CBCanSend, event);
	if (NOT event->event) {
		free(event);
		event = 0;
	}
	*eventID = (uint64_t)event;
	return event;
}
void CBCanSend(evutil_socket_t socketID,short eventNum,void * arg){
	CBEvent * event = arg;
	if (eventNum & EV_TIMEOUT) {
		// Timeout when waiting to write.
		event->loop->onTimeOut(event->loop->communicator,event->node,CB_TIMEOUT_SEND);
	}else{
		// Can send
		event->onEvent.ptr(event->loop->communicator,event->node);
	}
}
bool CBSocketCanReceiveEvent(uint64_t * eventID,uint64_t loopID,uint64_t socketID,void (*onCanReceive)(void *,void *),void * node){
	CBEvent * event = malloc(sizeof(*event));
	event->loop = (CBEventLoop *)loopID;
	event->onEvent.ptr = onCanReceive;
	event->node = node;
	event->event = event_new(((CBEventLoop *)loopID)->base, (evutil_socket_t)socketID, EV_TIMEOUT|EV_READ|EV_PERSIST, CBCanReceive, event);
	if (NOT event->event) {
		free(event);
		event = 0;
	}
	*eventID = (uint64_t)event;
	return event;
}
void CBCanReceive(evutil_socket_t socketID,short eventNum,void * arg){
	CBEvent * event = arg;
	if (eventNum & EV_TIMEOUT) {
		// Timeout when waiting to receive
		event->loop->onTimeOut(event->loop->communicator,event->node,CB_TIMEOUT_RECEIVE);
	}else{
		// Can receive
		event->onEvent.ptr(event->loop->communicator,event->node);
	}
}
bool CBSocketAddEvent(uint64_t eventID,uint16_t timeout){
	CBEvent * event = (CBEvent *)eventID;
	int res;
	if (timeout) {
		struct timeval time = {timeout,0};
		res = event_add(event->event, &time);
	}else
		res = event_add(event->event, NULL);
	return NOT res;
}
bool CBSocketRemoveEvent(uint64_t eventID){
	CBEvent * event = (CBEvent *)eventID;
	if(event_del(event->event))
		return false;
	else
		return true;
}
void CBSocketFreeEvent(uint64_t eventID){
	CBEvent * event = (CBEvent *)eventID;
	event_free(event->event);
	free(event);
}
int32_t CBSocketSend(uint64_t socketID,uint8_t * data,uint32_t len){
	ssize_t res = send((evutil_socket_t)socketID, data, len, CB_SEND_FLAGS);
	if (res >= 0)
		return (int32_t)res;
	if (errno == EAGAIN)
		return 0; // False event. Wait again.
	return CB_SOCKET_FAILURE; // Failure
}
int32_t CBSocketReceive(uint64_t socketID,uint8_t * data,uint32_t len){
	ssize_t res = read((evutil_socket_t)socketID, data, len);
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
	theTimer->callback = callback;
	theTimer->arg = arg;
	theTimer->timer = event_new(((CBEventLoop *)loopID)->base, -1, EV_PERSIST, CBFireTimer, theTimer);
	if (NOT theTimer->timer) {
		free(theTimer);
		theTimer = 0;
	}
	*timer = (uint64_t)theTimer;
	int res;
	if (time) {
		struct timeval timev = {time,0};
		res = event_add(theTimer->timer, &timev);
	}else
		res = event_add(theTimer->timer, NULL);
	return NOT res;
}
void CBFireTimer(evutil_socket_t foo,short bar,void * timer){
	CBTimer * theTimer = timer;
	theTimer->callback(theTimer->arg);
}
void CBEndTimer(uint64_t timer){
	CBTimer * theTimer = (CBTimer *)timer;
	event_free(theTimer->timer);
	free(theTimer);
}
void CBDoRun(evutil_socket_t socketID,short eventNum,void * arg){
	CBEventLoop * loop = arg;
	loop->userCallback(loop->userArg);
}
bool CBRunOnNetworkThread(uint64_t loopID,void (*callback)(void *),void * arg){
	CBEventLoop * loop = (CBEventLoop *) loopID;
	loop->userCallback = callback;
	loop->userArg = arg;
	event_active(loop->userEvent, 0, 0);
}
void CBCloseSocket(uint64_t socketID){
	evutil_closesocket((evutil_socket_t)socketID);
}
void CBExitEventLoop(uint64_t loopID){
	if (NOT loopID) {
		return;
	}
	CBEventLoop * loop = (CBEventLoop *) loopID;
	if(event_base_loopbreak(loop->base)){
		// Error occured. No choice but to do a dirty closure.
		pthread_cancel(loop->loopThread);
		event_base_free(loop->base);
		free(loop);
	}
}
