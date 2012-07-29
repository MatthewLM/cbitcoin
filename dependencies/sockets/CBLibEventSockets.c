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

// This is an implementation of the networking dependencies for cbitcoin. It can be included as as a source file in any projects using cbitcoin which require networking capabilities. This file uses libevent for effecient event-based sockets and POSIX threads. The sockets are POSIX sockets with compatibility functions taken from libevent to try and be compatible with windows.

// Includes

#include "CBDependencies.h" // cbitcoin dependencies to implement
#include <pthread.h> // POSIX threads
#include <event2/event.h> // libevent events
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

// Implementation

bool CBNewSocket(u_int64_t * socketID,bool IPv6){
	*socketID = socket(IPv6 ? PF_INET6 : PF_INET, SOCK_STREAM, 0);
	if (*socketID == -1) {
		return false;
	}
	// Make socket non-blocking
	evutil_make_socket_nonblocking((evutil_socket_t)*socketID);
	return true;
}
bool CBSocketBind(u_int64_t * socketID,bool IPv6,u_int16_t port){
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
		if (bind((evutil_socket_t)*socketID, ptr->ai_addr, ptr->ai_addrlen) == -1) {
			evutil_closesocket((evutil_socket_t)*socketID);
			continue;
		}
		break; // Success.
	}
	freeaddrinfo(res);
	if (ptr == NULL) // Failure
		return false;
	// Make socket non-blocking
	evutil_make_socket_nonblocking((evutil_socket_t)*socketID);
	return true;
}
bool CBSocketConnect(u_int64_t socketID,u_int8_t * IP,bool IPv6,u_int16_t port){
	// Create sockaddr_in6 information for a IPv6 address
	struct sockaddr_in6 address;
	memset(&address, 0, sizeof(address)); // Clear sockaddr_in6 structure.
	address.sin6_family = IPv6 ? AF_INET6 : AF_INET;
	memcpy(&address.sin6_addr, IP, 16); // Move IP address into place. POSIX standards should allow this.
	address.sin6_port = htons(port); // Port number to network order
	if (connect((evutil_socket_t)socketID, (struct sockaddr *)&address, sizeof(address)))
		// Connection failure
		return false;
	return true;
}
bool CBSocketListen(u_int64_t socketID,u_int16_t maxConnections){
	if(listen((evutil_socket_t)socketID, maxConnections) == -1){
		return false;
	}
	return true;
}
bool CBSocketAccept(u_int64_t socketID,u_int64_t * connectionSocketID,u_int8_t * IP){
	struct sockaddr_in6 address;
	socklen_t size = sizeof(address);
	*connectionSocketID = accept((evutil_socket_t)socketID, (struct sockaddr *)&address, &size);
	if (*connectionSocketID == -1) {
		return false;
	}
	// Fill out IPv6 address
	memcpy(IP, &address.sin6_addr, 16);
	// Make socket non-blocking
	evutil_make_socket_nonblocking((evutil_socket_t)*connectionSocketID);
	return true;
}
void event_base_add_virtual(struct event_base *); // Add virtual event.
void * CBStartEventLoop(void *);
typedef struct{
	struct event_base * base;
	void (*onError)(void *);
	// Callback for timeouts
	void (*onTimeOut)(void *,void *);
	void * communicator;
	pthread_t loopThread; /**< The thread ID for the event loop. */
}CBEventLoop;
u_int64_t CBNewEventLoop(void (*onError)(void *),void (*onDidTimeout)(void *,void *),void * communicator){
	struct event_base * base = event_base_new();
	// Create dummy event to maintain loop. When libevent 2.1 is released as stable EVLOOP_NO_EXIT_ON_EMPTY can be used. For now there is a hack solution.
	event_base_add_virtual(base);
	// Create arguments for the loop
	CBEventLoop * loop = malloc(sizeof(*loop));
	loop->base = base;
	loop->onError = onError;
	loop->onTimeOut = onDidTimeout;
	loop->communicator = communicator;
	// Create thread attributes explicitly for portability reasons.
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE); // Needs to be joinable.
	// Create joinable thread
	if(pthread_create(&loop->loopThread, &attr, CBStartEventLoop, loop)){
		// Thread creation failed.
		pthread_attr_destroy(&attr);
		event_base_free(base);
		return 0;
	}
	pthread_attr_destroy(&attr);
	return (u_int64_t) loop;
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
typedef struct{
	CBEventLoop * loop; /**< For getting timeout events */
	struct event * event; /**< libevent event. */
	void * onEvent;
	void * node;
}CBEvent;
void CBCanAccept(evutil_socket_t socketID,short eventNum,void * arg);
u_int64_t CBSocketCanAcceptEvent(u_int64_t loopID,u_int64_t socketID,void (*onCanAccept)(void *,u_int64_t)){
	CBEvent * event = malloc(sizeof(*event));
	event->onEvent = onCanAccept;
	event->event = event_new((struct event_base *)loopID, (evutil_socket_t)socketID, EV_READ|EV_PERSIST, CBCanAccept, event);
	if (!event->event) {
		free(event);
		event = 0;
	}
	return (u_int64_t)event;
}
void CBCanAccept(evutil_socket_t socketID,short eventNum,void * arg){
	CBEvent * event = arg;
	((void (*)(void *,u_int64_t))event->onEvent)(event->loop->communicator,socketID);
}
void CBDidConnect(evutil_socket_t socketID,short eventNum,void * arg);
u_int64_t CBSocketDidConnectEvent(u_int64_t loopID,u_int64_t socketID,void (*onDidConnect)(void *,void *),void * node){
	CBEvent * event = malloc(sizeof(*event));
	event->loop = (CBEventLoop *)loopID;
	event->onEvent = onDidConnect;
	event->node = node;
	event->event = event_new((struct event_base *)loopID, (evutil_socket_t)socketID, EV_TIMEOUT|EV_READ, CBDidConnect, event);
	if (!event->event) {
		free(event);
		event = 0;
	}
	return (u_int64_t)event;
}
void CBDidConnect(evutil_socket_t socketID,short eventNum,void * arg){
	CBEvent * event = arg;
	if (eventNum & EV_TIMEOUT) {
		// Timeout for the connection
		event->loop->onTimeOut(event->loop->communicator,event->node);
	}else{
		// Connection successful
		((void (*)(void *,void *))event->onEvent)(event->loop->communicator,event->node);
	}
}
void CBCanSend(evutil_socket_t socketID,short eventNum,void * arg);
u_int64_t CBSocketCanSendEvent(u_int64_t loopID,u_int64_t socketID,void (*onCanSend)(void *,void *),void * node){
	CBEvent * event = malloc(sizeof(*event));
	event->loop = (CBEventLoop *)loopID;
	event->onEvent = onCanSend;
	event->node = node;
	event->event = event_new((struct event_base *)loopID, (evutil_socket_t)socketID, EV_TIMEOUT|EV_WRITE|EV_PERSIST, CBCanSend, event);
	if (!event->event) {
		free(event);
		event = 0;
	}
	return (u_int64_t)event;
}
void CBCanSend(evutil_socket_t socketID,short eventNum,void * arg){
	CBEvent * event = arg;
	if (eventNum & EV_TIMEOUT) {
		// Timeout when waiting to write.
		event->loop->onTimeOut(event->loop->communicator,event->node);
	}else{
		// Can send
		((void (*)(void *,void *))event->onEvent)(event->loop->communicator,event->node);
	}
}
void CBCanReceive(evutil_socket_t socketID,short eventNum,void * arg);
u_int64_t CBSocketCanReceiveEvent(u_int64_t loopID,u_int64_t socketID,void (*onCanReceive)(void *,void *),void * node){
	CBEvent * event = malloc(sizeof(*event));
	event->loop = (CBEventLoop *)loopID;
	event->onEvent = onCanReceive;
	event->node = node;
	event->event = event_new((struct event_base *)loopID, (evutil_socket_t)socketID, EV_TIMEOUT|EV_READ|EV_PERSIST, CBCanReceive, event);
	if (!event->event) {
		free(event);
		event = 0;
	}
	return (u_int64_t)event;
}
void CBCanReceive(evutil_socket_t socketID,short eventNum,void * arg){
	CBEvent * event = arg;
	if (eventNum & EV_TIMEOUT) {
		// Timeout when waiting to receive
		event->loop->onTimeOut(event->loop->communicator,event->node);
	}else{
		// Can receive
		((void (*)(void *,void *))event->onEvent)(event->loop->communicator,event->node);
	}
}
bool CBSocketAddEvent(u_int64_t eventID,u_int16_t timeout){
	CBEvent * event = (CBEvent *)eventID;
	int res;
	if (timeout) {
		struct timeval time = {timeout,0};
		res = event_add(event->event, &time);
	}else
		res = event_add(event->event, NULL);
	if (res) {
		return false;
	}
	return true;
}
bool CBSocketRemoveEvent(u_int64_t eventID){
	CBEvent * event = (CBEvent *)eventID;
	if(event_del(event->event))
		return false;
	else
		return true;
}
void CBSocketFreeEvent(u_int64_t eventID){
	CBEvent * event = (CBEvent *)eventID;
	event_free(event->event);
	free(event);
}
int32_t CBSocketSend(u_int64_t socketID,u_int8_t * data,u_int32_t len){
	int res = write((evutil_socket_t)socketID, data,len);
	if (res >= 0)
		return res;
	if (errno == EAGAIN)
		return 0; // False event. Wait again.
	return CB_SOCKET_FAILURE; // Failure
}
int32_t CBSocketReceive(u_int64_t socketID,u_int8_t * data,u_int32_t len){
	int res = read((evutil_socket_t)socketID, data, len);
	if (res > 0)
		return res; // OK, read data.
	if (!res)
		return CB_SOCKET_CONNECTION_CLOSE; // If read() gives zero it means the connection was closed.
	if (errno == EAGAIN)
		return 0; // False event. Wait again. No bytes read.
	return CB_SOCKET_FAILURE; // Failure
}
void CBCloseSocket(u_int64_t socketID){
	evutil_closesocket((evutil_socket_t)socketID);
}
void CBExitEventLoop(u_int64_t loopID){
	if (!loopID) {
		return;
	}
	CBEventLoop * loop = (CBEventLoop *) loopID;
	if(event_base_loopbreak(loop->base)){
		// Error occured. No choice but to do a dirty closure.
		pthread_cancel(loop->loopThread);
		event_base_free(loop->base);
		free(loop);
	}else{
		// Join gracefully
		pthread_join(loop->loopThread, NULL);
	}
}
