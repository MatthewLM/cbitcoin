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
	GOTPING = 4,
	GOTPONG = 8,
	GOTGETADDR = 16,
}TesterProgress;

typedef struct{
	TesterProgress prog[7];
	CBPeer * peerToProg[7];
	uint8_t progNum;
	int complete;
	int addrComplete;
	CBNetworkCommunicator * comms[3];
	pthread_mutex_t testingMutex;
}Tester;

void onTimeOut(void * tester,void * comm,void * peer,CBTimeOutType type);
void onTimeOut(void * tester,void * comm,void * peer,CBTimeOutType type){
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
	CBNetworkCommunicatorStop((CBNetworkCommunicator *)comm);
}
void stop(void * comm);
void stop(void * comm){
	CBNetworkCommunicatorStop(comm);
}
CBOnMessageReceivedAction onMessageReceived(void * vtester,void * vcomm,void * vpeer);
CBOnMessageReceivedAction onMessageReceived(void * vtester,void * vcomm,void * vpeer){
	Tester * tester = vtester;
	pthread_mutex_lock(&tester->testingMutex); // Only one processing of test at a time.
	CBPeer * peer = vpeer;
	CBNetworkCommunicator * comm = vcomm;
	CBMessage * theMessage = peer->receive;
	// Assign peer to tester progress.
	uint8_t x = 0;
	for (; x < tester->progNum; x++) {
		if (tester->peerToProg[x] == peer) {
			break;
		}
	}
	if (x == tester->progNum) {
		printf("NEW NODE OBJ: (%s,%p),(%p)\n",(comm->ourIPv4->port == 45562)? "L1" : ((comm->ourIPv4->port == 45563)? "L2" : "CN"),comm,peer);
		tester->peerToProg[x] = peer;
		tester->progNum++;
	}
	TesterProgress * prog = tester->prog + x;
	switch (theMessage->type) {
		case CB_MESSAGE_TYPE_VERSION:
			if (NOT ((peer->versionSent && *prog == GOTACK) || (*prog == 0))) {
				printf("VERSION FAIL\n");
				exit(EXIT_FAILURE);
			}
			if (CBGetVersion(theMessage)->services) {
				printf("VERSION SERVICES FAIL\n");
				exit(EXIT_FAILURE);
			}
			if (CBGetVersion(theMessage)->version != CB_PONG_VERSION) {
				printf("VERSION VERSION FAIL\n");
				exit(EXIT_FAILURE);
			}
			if (memcmp(CBByteArrayGetData(CBGetVersion(theMessage)->userAgent),CB_USER_AGENT_SEGMENT,CBGetVersion(theMessage)->userAgent->length)) {
				printf("VERSION USER AGENT FAIL\n");
				exit(EXIT_FAILURE);
			}
			if (memcmp(CBByteArrayGetData(CBGetVersion(theMessage)->addSource->ip),(uint8_t [16]){0,0,0,0,0,0,0,0,0,0,0xFF,0xFF,127,0,0,1},CBGetVersion(theMessage)->addSource->ip->length)) {
				printf("VERSION SOURCE IP FAIL\n");
				exit(EXIT_FAILURE);
			}
			if (memcmp(CBByteArrayGetData(CBGetVersion(theMessage)->addRecv->ip),(uint8_t [16]){0,0,0,0,0,0,0,0,0,0,0xFF,0xFF,127,0,0,1},CBGetVersion(theMessage)->addRecv->ip->length)) {
				printf("VERSION RECEIVE IP FAIL\n");
				exit(EXIT_FAILURE);
			}
			*prog |= GOTVERSION;
			break;
		case CB_MESSAGE_TYPE_VERACK:
			if ((NOT peer->versionSent || peer->versionAck)) {
				printf("VERACK FAIL\n");
				exit(EXIT_FAILURE);
			}
			*prog |= GOTACK;
			break;
		case CB_MESSAGE_TYPE_PING:
			if (NOT (*prog & GOTVERSION && *prog & GOTACK)) {
				printf("PING FAIL\n");
				exit(EXIT_FAILURE);
			}
			CBGetPingPong(theMessage)->ID = CBGetPingPong(theMessage)->ID; // Test access to ID.
			*prog |= GOTPING;
			break;
		case CB_MESSAGE_TYPE_PONG:
			if (NOT (*prog & GOTVERSION && *prog & GOTACK)) {
				printf("PONG FAIL\n");
				exit(EXIT_FAILURE);
			}
			CBGetPingPong(theMessage)->ID = CBGetPingPong(theMessage)->ID; // Test access to ID.
			// At least one of the pongs worked.
			*prog |= GOTPONG;
			break;
		case CB_MESSAGE_TYPE_GETADDR:
			if (NOT (*prog & GOTVERSION && *prog & GOTACK)) {
				printf("GET ADDR FAIL\n");
				exit(EXIT_FAILURE);
			}
			*prog |= GOTGETADDR;
			break;
		case CB_MESSAGE_TYPE_ADDR:
			if (NOT (*prog & GOTVERSION && *prog & GOTACK)) {
				printf("ADDR FAIL\n");
				exit(EXIT_FAILURE);
			}
			tester->addrComplete++;
			break;
		default:
			printf("MESSAGE FAIL\n");
			exit(EXIT_FAILURE);
			break;
	}
	if (*prog == (GOTVERSION | GOTACK | GOTPING | GOTPONG | GOTGETADDR)) {
		*prog = 0;
		tester->complete++;
	}
	printf("COMPLETION: %i - %i\n",tester->addrComplete,tester->complete);
	if (tester->addrComplete == 6 && tester->complete == 6) { // Connector sends other peer twice (2). Listeners send self to connector (2). Listeners send selves to each other (2). 2 + 2 + 2 = 6
		// Completed testing
		printf("DONE\n");
		printf("STOPPING COMM L1\n");
		CBRunOnNetworkThread(tester->comms[0]->eventLoop, stop, tester->comms[0]);
		printf("STOPPING COMM L2\n");
		CBRunOnNetworkThread(tester->comms[1]->eventLoop, stop, tester->comms[1]);
		printf("STOPPING COMM CN\n");
		CBRunOnNetworkThread(tester->comms[2]->eventLoop, stop, tester->comms[2]);
		return CB_MESSAGE_ACTION_RETURN;
	}
	pthread_mutex_unlock(&tester->testingMutex);
	return CB_MESSAGE_ACTION_CONTINUE;
}
void onNetworkError(void * foo,void * comm);
void onNetworkError(void * foo,void * comm){
	printf("DID LOSE LAST NODE\n");
	exit(EXIT_FAILURE);
}
void onBadTime(void * comm,void * addrMan);
void onBadTime(void * comm,void * addrMan){
	printf("BAD TIME FAIL\n");
	exit(EXIT_FAILURE);
}

int main(){
	CBEvents events;
	Tester tester;
	memset(&tester, 0, sizeof(tester));
	pthread_mutex_init(&tester.testingMutex, NULL);
	events.onErrorReceived = err;
	events.onBadTime = onBadTime;
	events.onMessageReceived = onMessageReceived;
	events.onNetworkError = onNetworkError;
	events.onTimeOut = onTimeOut;
	evthread_use_pthreads();
	// Create three CBNetworkCommunicators and connect over the loopback address. Two will listen, one will connect. Test auto handshake, auto ping and auto discovery.
	CBByteArray * altMessages = CBNewByteArrayOfSize(0, &events);
	CBByteArray * altMessages2 = CBNewByteArrayOfSize(0, &events);
	CBByteArray * altMessages3 = CBNewByteArrayOfSize(0, &events);
	CBByteArray * loopBack = CBNewByteArrayWithDataCopy((uint8_t [16]){0,0,0,0,0,0,0,0,0,0,0xFF,0xFF,127,0,0,1}, 16, &events);
	CBByteArray * loopBack2 = CBByteArrayCopy(loopBack); // Do not use in more than one thread.
	CBNetworkAddress * addrListen = CBNewNetworkAddress(0, loopBack, 45562, 0, &events);
	CBNetworkAddress * addrListenB = CBNewNetworkAddress(0, loopBack2, 45562, 0, &events); // Use in connector thread.
	addrListenB->public = true; // Ensure addresses are relayed.
	CBNetworkAddress * addrListen2 = CBNewNetworkAddress(0, loopBack, 45563, 0, &events);
	CBNetworkAddress * addrListen2B = CBNewNetworkAddress(0, loopBack2, 45563, 0, &events); // Use in connector thread.
	addrListen2B->public = true; // Ensure addresses are relayed.
	CBNetworkAddress * addrConnect = CBNewNetworkAddress(0, loopBack, 45564, 0, &events); // Different port over loopback to seperate the CBNetworkCommunicators.
	CBReleaseObject(loopBack);
	CBReleaseObject(loopBack2);
	CBByteArray * userAgent = CBNewByteArrayFromString(CB_USER_AGENT_SEGMENT, &events);
	CBByteArray * userAgent2 = CBNewByteArrayFromString(CB_USER_AGENT_SEGMENT, &events);
	CBByteArray * userAgent3 = CBNewByteArrayFromString(CB_USER_AGENT_SEGMENT, &events);
	// First listening CBNetworkCommunicator setup.
	CBAddressManager * addrManListen = CBNewAddressManager(&events);
	addrManListen->maxAddressesInBucket = 2;
	CBAddressManagerSetReachability(addrManListen, CB_IP_IPv4 | CB_IP_LOCAL, true);
	CBNetworkCommunicator * commListen = CBNewNetworkCommunicator(&events);
	addrManListen->callbackHandler = commListen;
	commListen->networkID = CB_PRODUCTION_NETWORK_BYTES;
	commListen->flags = CB_NETWORK_COMMUNICATOR_AUTO_HANDSHAKE | CB_NETWORK_COMMUNICATOR_AUTO_PING | CB_NETWORK_COMMUNICATOR_AUTO_DISCOVERY;
	commListen->version = CB_PONG_VERSION;
	commListen->maxConnections = 3;
	commListen->maxIncommingConnections = 3; // One for connector, one for the other listener and an extra so that we continue to share our address.
	commListen->heartBeat = 1;
	commListen->timeOut = 2;
	commListen->sendTimeOut = 1;
	commListen->recvTimeOut = 1;
	commListen->responseTimeOut = 1;
	commListen->connectionTimeOut = 1;
	CBNetworkCommunicatorSetAlternativeMessages(commListen, altMessages, NULL);
	CBNetworkCommunicatorSetAddressManager(commListen, addrManListen);
	CBNetworkCommunicatorSetUserAgent(commListen, userAgent);
	CBNetworkCommunicatorSetOurIPv4(commListen, addrListen);
	commListen->callbackHandler = &tester;
	// Second listening CBNetworkCommunicator setup.
	CBAddressManager * addrManListen2 = CBNewAddressManager(&events);
	addrManListen2->maxAddressesInBucket = 2;
	CBAddressManagerSetReachability(addrManListen2, CB_IP_IPv4 | CB_IP_LOCAL, true);
	CBNetworkCommunicator * commListen2 = CBNewNetworkCommunicator(&events);
	addrManListen2->callbackHandler = commListen2;
	commListen2->networkID = CB_PRODUCTION_NETWORK_BYTES;
	commListen2->flags = CB_NETWORK_COMMUNICATOR_AUTO_HANDSHAKE | CB_NETWORK_COMMUNICATOR_AUTO_PING | CB_NETWORK_COMMUNICATOR_AUTO_DISCOVERY;
	commListen2->version = CB_PONG_VERSION;
	commListen2->maxConnections = 3;
	commListen2->maxIncommingConnections = 3;
	commListen2->heartBeat = 1;
	commListen2->timeOut = 2;
	commListen2->sendTimeOut = 1;
	commListen2->recvTimeOut = 1;
	commListen2->responseTimeOut = 1;
	commListen2->connectionTimeOut = 1;
	CBNetworkCommunicatorSetAlternativeMessages(commListen2, altMessages2, NULL);
	CBNetworkCommunicatorSetAddressManager(commListen2, addrManListen2);
	CBNetworkCommunicatorSetUserAgent(commListen2, userAgent2);
	CBNetworkCommunicatorSetOurIPv4(commListen2, addrListen2);
	commListen2->callbackHandler = &tester;
	// Connecting CBNetworkCommunicator setup.
	CBAddressManager * addrManConnect = CBNewAddressManager(&events);
	addrManConnect->maxAddressesInBucket = 2;
	CBAddressManagerSetReachability(addrManConnect, CB_IP_IPv4 | CB_IP_LOCAL, true);
	// We are going to connect to both listing CBNetworkCommunicators.
	CBAddressManagerAddAddress(addrManConnect, addrListenB);
	CBAddressManagerAddAddress(addrManConnect, addrListen2B);
	CBNetworkCommunicator * commConnect = CBNewNetworkCommunicator(&events);
	addrManConnect->callbackHandler = commConnect;
	commConnect->networkID = CB_PRODUCTION_NETWORK_BYTES;
	commConnect->flags = CB_NETWORK_COMMUNICATOR_AUTO_HANDSHAKE | CB_NETWORK_COMMUNICATOR_AUTO_PING | CB_NETWORK_COMMUNICATOR_AUTO_DISCOVERY;
	commConnect->version = CB_PONG_VERSION;
	commConnect->maxConnections = 2;
	commConnect->maxIncommingConnections = 0;
	commConnect->heartBeat = 1;
	commConnect->timeOut = 2;
	commConnect->sendTimeOut = 1;
	commConnect->recvTimeOut = 1;
	commConnect->responseTimeOut = 1;
	commConnect->connectionTimeOut = 1;
	CBNetworkCommunicatorSetAlternativeMessages(commConnect, altMessages3, NULL);
	CBNetworkCommunicatorSetAddressManager(commConnect, addrManConnect);
	CBNetworkCommunicatorSetUserAgent(commConnect, userAgent3);
	CBNetworkCommunicatorSetOurIPv4(commConnect, addrConnect);
	commConnect->callbackHandler = &tester;
	// Release objects
	CBReleaseObject(altMessages);
	CBReleaseObject(userAgent);
	// Give tester communicators
	tester.comms[0] = commListen;
	tester.comms[1] = commListen2;
	tester.comms[2] = commConnect;
	// Start listening on first listener. Can start listening on this thread since the event loop will not be doing anything.
	CBNetworkCommunicatorStart(commListen);
	pthread_t listenThread = ((CBEventLoop *) commListen->eventLoop)->loopThread;
	CBNetworkCommunicatorStartListening(commListen);
	if (NOT commListen->isListeningIPv4) {
		printf("FIRST LISTEN FAIL\n");
		exit(EXIT_FAILURE);
	}
	// Start listening on second listener.
	CBNetworkCommunicatorStart(commListen2);
	pthread_t listen2Thread = ((CBEventLoop *) commListen2->eventLoop)->loopThread;
	CBNetworkCommunicatorStartListening(commListen2);
	if (NOT commListen2->isListeningIPv4) {
		printf("SECOND LISTEN FAIL\n");
		exit(EXIT_FAILURE);
	}
	// Start connection
	CBNetworkCommunicatorStart(commConnect);
	pthread_t connectThread = ((CBEventLoop *) commConnect->eventLoop)->loopThread;
	CBNetworkCommunicatorTryConnections(commConnect);
	// Wait until the network loop ends for all CBNetworkCommunicators.
	pthread_join(listenThread, NULL);
	pthread_join(listen2Thread, NULL);
	pthread_join(connectThread, NULL);
	// Check addresses in the first listening CBNetworkCommunicator
	uint8_t i = CBAddressManagerGetBucketIndex(commListen->addresses, addrListen2);
	CBBucket * bucket = commListen->addresses->buckets + i;
	if (bucket->addrNum != 1) { // Only L2 should go back to addresses. CN is private
		printf("ADDRESS DISCOVERY LISTEN ONE ADDR NUM FAIL %i != 1\n",bucket->addrNum);
		exit(EXIT_FAILURE);
	}
	if (i != CBAddressManagerGetBucketIndex(commListen->addresses, addrConnect)) {
		printf("ADDRESS DISCOVERY LISTEN ONE SAME BUCKET FAIL\n");
		exit(EXIT_FAILURE);
	}
	if(!CBAddressManagerGotNetworkAddress(commListen->addresses, addrListen2)){
		printf("ADDRESS DISCOVERY LISTEN ONE LISTEN TWO FAIL\n");
		exit(EXIT_FAILURE);
	}
	// Check the addresses in the second.
	i = CBAddressManagerGetBucketIndex(commListen2->addresses, addrListen);
	bucket = commListen2->addresses->buckets + i;
	if (bucket->addrNum != 1) {
		printf("ADDRESS DISCOVERY LISTEN TWO ADDR NUM FAIL\n");
		exit(EXIT_FAILURE);
	}
	if (i != CBAddressManagerGetBucketIndex(commListen2->addresses, addrConnect)) {
		printf("ADDRESS DISCOVERY LISTEN TWO SAME BUCKET FAIL\n");
		exit(EXIT_FAILURE);
	}
	if(!CBAddressManagerGotNetworkAddress(commListen2->addresses, addrListen)){
		printf("ADDRESS DISCOVERY LISTEN TWO LISTEN ONE FAIL\n");
		exit(EXIT_FAILURE);
	}
	// And lastly the connecting CBNetworkCommunicator
	i = CBAddressManagerGetBucketIndex(commConnect->addresses, addrListen);
	bucket = commConnect->addresses->buckets + i;
	if (bucket->addrNum != 2) {
		printf("ADDRESS DISCOVERY CONNECT ADDR NUM FAIL\n");
		exit(EXIT_FAILURE);
	}
	if (i != CBAddressManagerGetBucketIndex(commConnect->addresses, addrListen2)) {
		printf("ADDRESS DISCOVERY CONNECT SAME BUCKET FAIL\n");
		exit(EXIT_FAILURE);
	}
	if(!CBAddressManagerGotNetworkAddress(commConnect->addresses, addrListen)){
		printf("ADDRESS DISCOVERY CONNECT LISTEN ONE FAIL\n");
		exit(EXIT_FAILURE);
	}
	if(!CBAddressManagerGotNetworkAddress(commConnect->addresses, addrListen2)){
		printf("ADDRESS DISCOVERY CONNECT LISTEN TWO FAIL\n");
		exit(EXIT_FAILURE);
	}
	// Release all final objects.
	CBReleaseObject(addrListen);
	CBReleaseObject(addrListen2);
	CBReleaseObject(addrConnect);
	CBReleaseObject(commListen);
	CBReleaseObject(commListen2);
	CBReleaseObject(commConnect);
	pthread_mutex_destroy(&tester.testingMutex);
	return EXIT_SUCCESS;
}
