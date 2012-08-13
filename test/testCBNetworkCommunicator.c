//
//  testCBNetworkCommunicator.c
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

#include <stdio.h>
#include "CBNetworkCommunicator.h"
#include "CBLibEventSockets.h"
#include <event2/thread.h>
#include <time.h>
#include "stdarg.h"

void err(CBError a,char * format,...);
void err(CBError a,char * format,...){
	va_list argptr;
    va_start(argptr, format);
    vfprintf(stderr, format, argptr);
    va_end(argptr);
	printf("\n");
}

typedef enum{
	GOTVERSION = 1,
	GOTACK = 2,
	SUCCESS = 4
}Tester;
void onTimeOut(void * tester,void * comm,void * node,CBTimeOutType type);
void onTimeOut(void * tester,void * comm,void * node,CBTimeOutType type){
	if (NOT (*(Tester *)tester & SUCCESS)){
		switch (type) {
			case CB_TIMEOUT_CONNECT:
				printf("TIMEOUT FAIL: CONNECT\n");
				break;
			case CB_TIMEOUT_NO_DATA:
				printf("TIMEOUT FAIL: NO DATA\n");
				break;
			case CB_TIMEOUT_RECEIVE:
				printf("TIMEOUT FAIL: RECEIVE\n");
				break;
			case CB_TIMEOUT_RESPONSE:
				printf("TIMEOUT FAIL: RESPONSE\n");
				break;
			case CB_TIMEOUT_SEND:
				printf("TIMEOUT FAIL: SEND\n");
				break;
		}
	}
	CBNetworkCommunicatorStop((CBNetworkCommunicator *)comm);
}
bool onMessageReceived(void * vtester,void * vcomm,void * vnode);
bool onMessageReceived(void * vtester,void * vcomm,void * vnode){
	Tester * tester = vtester;
	CBNetworkCommunicator * comm = vcomm;
	CBNode * node = vnode;
	CBMessage * theMessage = node->receive;
	switch (theMessage->type) {
		case CB_MESSAGE_TYPE_VERSION:
			if (*tester && NOT (*tester == GOTACK && node->versionSent)) {
				printf("VERSION FAIL\n");
				CBNetworkCommunicatorStop(comm);
			}
			if (CBGetVersion(theMessage)->services) {
				printf("VERSION SERVICES FAIL\n");
				CBNetworkCommunicatorStop(comm);
			}
			if (CBGetVersion(theMessage)->version != CB_PONG_VERSION) {
				printf("VERSION VERSION FAIL\n");
				CBNetworkCommunicatorStop(comm);
			}
			if (memcmp(CBByteArrayGetData(CBGetVersion(theMessage)->userAgent),CB_USER_AGENT_SEGMENT,CBGetVersion(theMessage)->userAgent->length)) {
				printf("VERSION USER AGENT FAIL\n");
				CBNetworkCommunicatorStop(comm);
			}
			if (memcmp(CBByteArrayGetData(CBGetVersion(theMessage)->addSource->ip),(uint8_t [16]){0,0,0,0,0,0,0,0,0,0,0xFF,0xFF,127,0,0,1},CBGetVersion(theMessage)->addSource->ip->length)) {
				printf("VERSION SOURCE IP FAIL\n");
				CBNetworkCommunicatorStop(comm);
			}
			if (memcmp(CBByteArrayGetData(CBGetVersion(theMessage)->addRecv->ip),(uint8_t [16]){0,0,0,0,0,0,0,0,0,0,0xFF,0xFF,127,0,0,1},CBGetVersion(theMessage)->addRecv->ip->length)) {
				printf("VERSION RECEIVE IP FAIL\n");
				CBNetworkCommunicatorStop(comm);
			}
			if (CBGetVersion(theMessage)->addSource->port != 45563 && comm->ourIPv4->port == 45562) {
				printf("VERSION SOURCE PORT FAIL\n");
				CBNetworkCommunicatorStop(comm);
			}
			if (CBGetVersion(theMessage)->addRecv->port != 45562  && comm->ourIPv4->port == 45563) {
				printf("VERSION RECEIVE PORT FAIL\n");
				CBNetworkCommunicatorStop(comm);
			}
			*tester |= GOTVERSION;
			break;
		case CB_MESSAGE_TYPE_VERACK:
			if ((*tester && *tester != GOTVERSION) || node->versionSent) {
				printf("VERACK FAIL\n");
				CBNetworkCommunicatorStop(comm);
			}
			*tester |= GOTACK;
			break;
		case CB_MESSAGE_TYPE_PING:
			if (NOT (*tester & GOTVERSION && *tester & GOTACK)) {
				printf("PING FAIL\n");
				CBNetworkCommunicatorStop(comm);
			}
			CBGetPingPong(theMessage)->ID = CBGetPingPong(theMessage)->ID; // Test access to ID.
			break;
		case CB_MESSAGE_TYPE_PONG:
			if (NOT (*tester & GOTVERSION && *tester & GOTACK) || node->pingsSent != 1) {
				printf("PONG FAIL\n");
				CBNetworkCommunicatorStop(comm);
			}
			CBGetPingPong(theMessage)->ID = CBGetPingPong(theMessage)->ID; // Test access to ID.
			// At least one of the pongs worked. Stop this communicator and thus allow the other communicator to timeout.
			*tester |= SUCCESS;
			CBNetworkCommunicatorStop(comm);
			break;
		default:
			printf("MESSAGE FAIL\n");
			CBNetworkCommunicatorStop(comm);
			break;
	}
	return false;
}
void onNetworkError(void * foo,void * comm);
void onNetworkError(void * foo,void * comm){
	printf("NETWORK ERROR FAIL\n");
	CBNetworkCommunicatorStop((CBNetworkCommunicator *)comm);
}
void onBadTime(void * comm,void * addrMan);
void onBadTime(void * comm,void * addrMan){
	printf("BAD TIME FAIL\n");
	CBNetworkCommunicatorStop((CBNetworkCommunicator *)comm);
}

int main(){
	CBEvents events;
	Tester testerListen = 0, testerConnect = 0;
	events.onErrorReceived = err;
	events.onBadTime = onBadTime;
	events.onMessageReceived = onMessageReceived;
	events.onNetworkError = onNetworkError;
	events.onTimeOut = onTimeOut;
	// Using multiple threads to access libevent
	evthread_use_pthreads();
	// Create two CBNetworkCommunicators and connect over the loopback address. One will listen, one will connect. Test auto handshake and auto ping between them.
	CBByteArray * altMessages = CBNewByteArrayOfSize(0, &events);
	CBByteArray * altMessages2 = CBNewByteArrayOfSize(0, &events);
	CBByteArray * loopBack = CBNewByteArrayWithDataCopy((uint8_t [16]){0,0,0,0,0,0,0,0,0,0,0xFF,0xFF,127,0,0,1}, 16, &events);
	CBByteArray * loopBack2 = CBByteArrayCopy(loopBack); // Do not use in more than one thread.
	CBNetworkAddress * addrListen = CBNewNetworkAddress(0, loopBack, 45562, 0, &events);
	CBNetworkAddress * addrListen2 = CBNewNetworkAddress(0, loopBack, 45562, 0, &events); // Use in second thread.
	CBNetworkAddress * addrConnect = CBNewNetworkAddress(0, loopBack2, 45563, 0, &events); // Different port over loopback to seperate the CBNetworkCommunicators.
	CBReleaseObject(loopBack);
	CBReleaseObject(loopBack2);
	CBByteArray * userAgent = CBNewByteArrayFromString(CB_USER_AGENT_SEGMENT, &events);
	// Listening CBNetworkCommunicator setup.
	CBAddressManager * addrManListen = CBNewAddressManager(&events);
	addrManListen->maxAddressesInBucket = 1;
	CBAddressManagerSetReachability(addrManListen, CB_IP_IPv4 | CB_IP_LOCAL, true);
	CBNetworkCommunicator * commListen = CBNewNetworkCommunicator(&events);
	addrManListen->callbackHandler = commListen;
	commListen->networkID = CB_PRODUCTION_NETWORK_BYTES;
	commListen->flags = CB_NETWORK_COMMUNICATOR_AUTO_HANDSHAKE | CB_NETWORK_COMMUNICATOR_AUTO_PING;
	commListen->version = CB_PONG_VERSION;
	commListen->maxConnections = 1;
	commListen->maxIncommingConnections = 1;
	commListen->heartBeat = 10;
	commListen->timeOut = 20;
	commListen->sendTimeOut = 1;
	commListen->recvTimeOut = 1;
	commListen->responseTimeOut = 10;
	commListen->connectionTimeOut = 10;
	CBNetworkCommunicatorSetAlternativeMessages(commListen, altMessages, NULL);
	CBNetworkCommunicatorSetAddressManager(commListen, addrManListen);
	CBNetworkCommunicatorSetUserAgent(commListen, userAgent);
	CBNetworkCommunicatorSetOurIPv4(commListen, addrListen);
	commListen->callbackHandler = &testerListen;
	// Connecting CBNetworkCommunicator setup.
	CBAddressManager * addrManConnect = CBNewAddressManager(&events);
	addrManConnect->maxAddressesInBucket = 1;
	CBAddressManagerSetReachability(addrManConnect, CB_IP_IPv4 | CB_IP_LOCAL, true);
	CBAddressManagerAddAddress(addrManConnect, addrListen2); // We are going to connect to the listing CBNetworkCommunicator.
	CBNetworkCommunicator * commConnect = CBNewNetworkCommunicator(&events);
	addrManConnect->callbackHandler = commConnect;
	commConnect->networkID = CB_PRODUCTION_NETWORK_BYTES;
	commConnect->flags = CB_NETWORK_COMMUNICATOR_AUTO_HANDSHAKE | CB_NETWORK_COMMUNICATOR_AUTO_PING;
	commConnect->version = CB_PONG_VERSION;
	commConnect->maxConnections = 1;
	commConnect->maxIncommingConnections = 0;
	commConnect->heartBeat = 10;
	commConnect->timeOut = 20;
	commConnect->sendTimeOut = 1;
	commConnect->recvTimeOut = 1;
	commConnect->responseTimeOut = 10;
	commConnect->connectionTimeOut = 10;
	CBNetworkCommunicatorSetAlternativeMessages(commConnect, altMessages2, NULL);
	CBNetworkCommunicatorSetAddressManager(commConnect, addrManConnect);
	CBNetworkCommunicatorSetUserAgent(commConnect, userAgent);
	CBNetworkCommunicatorSetOurIPv4(commConnect, addrConnect);
	commConnect->callbackHandler = &testerConnect;
	// Release objects
	CBReleaseObject(altMessages);
	CBReleaseObject(addrListen);
	CBReleaseObject(addrListen2);
	CBReleaseObject(addrConnect);
	CBReleaseObject(userAgent);
	// Start listening. Can start listening on this thread since the event loop will not be doing anything.
	CBNetworkCommunicatorStart(commListen);
	pthread_t listenThread = ((CBEventLoop *) commListen->eventLoop)->loopThread;
	CBNetworkCommunicatorStartListening(commListen);
	// Start connection
	CBNetworkCommunicatorStart(commConnect);
	pthread_t connectThread = ((CBEventLoop *) commConnect->eventLoop)->loopThread;
	CBNetworkCommunicatorTryConnections(commConnect);
	// Wait until the network loop ends for both CBNetworkCommunicators.
	pthread_join(listenThread, NULL);
	pthread_join(connectThread, NULL);
	if (NOT (testerListen & SUCCESS) && NOT (testerConnect & SUCCESS))
		printf("NO SUCCESS\n");
	// Release communicators
	CBReleaseObject(commListen);
	CBReleaseObject(commConnect);
	return 0;
}