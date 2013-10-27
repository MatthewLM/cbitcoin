//
//  testCBNetworkCommunicator.c
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

#include <stdio.h>
#include "CBNetworkCommunicator.h"
#include "CBLibEventSockets.h"
#include <event2/thread.h>
#include <time.h>
#include "stdarg.h"

typedef enum{
	GOTVERSION = 1, 
	GOTACK = 2, 
	GOTPING = 4, 
	GOTPONG = 8, 
	GOTGETADDR = 16,
	COMPLETE = 32,
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

uint64_t CBGetMilliseconds(void){
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

bool onTimeOut(CBNetworkCommunicator * comm, void * peer);
bool onTimeOut(CBNetworkCommunicator * comm, void * peer){
	CBLogError("TIMEOUT FAIL");
	CBNetworkCommunicatorStop((CBNetworkCommunicator *)comm);
	return false;
}

void stop(void * comm);
void stop(void * comm){
	CBNetworkCommunicatorStop(comm);
}

bool acceptType(CBNetworkCommunicator *, CBPeer *, CBMessageType);
bool acceptType(CBNetworkCommunicator * comm, CBPeer * peer, CBMessageType type){
	if (type == CB_MESSAGE_TYPE_VERACK && peer->handshakeStatus & CB_HANDSHAKE_GOT_ACK) {
		CBLogError("ALREADY HAVE ACK FAIL\n");
		exit(EXIT_FAILURE);
	}
	if (type == CB_MESSAGE_TYPE_VERSION && peer->handshakeStatus & CB_HANDSHAKE_GOT_VERSION) {
		CBLogError("ALREADY HAVE VERSION FAIL\n");
		exit(EXIT_FAILURE);
	}
	return true;
}

Tester tester;

CBOnMessageReceivedAction onMessageReceived(CBNetworkCommunicator * comm, CBPeer * peer, CBMessage * theMessage);
CBOnMessageReceivedAction onMessageReceived(CBNetworkCommunicator * comm, CBPeer * peer, CBMessage * theMessage){
	pthread_mutex_lock(&tester.testingMutex); // Only one processing of test at a time.
	// Assign peer to tester progress.
	uint8_t x = 0;
	for (; x < tester.progNum; x++)
		if (tester.peerToProg[x] == peer)
			break;
	if (x == tester.progNum) {
		tester.peerToProg[x] = peer;
		tester.progNum++;
	}
	TesterProgress * prog = tester.prog + x;
	switch (theMessage->type) {
		case CB_MESSAGE_TYPE_VERSION:
			if (! ((peer->handshakeStatus & CB_HANDSHAKE_SENT_VERSION && *prog == GOTACK) || (*prog == 0))) {
				CBLogError("VERSION FAIL");
				exit(EXIT_FAILURE);
			}
			if (CBGetVersion(theMessage)->services) {
				CBLogError("VERSION SERVICES FAIL");
				exit(EXIT_FAILURE);
			}
			if (CBGetVersion(theMessage)->version != CB_PONG_VERSION) {
				CBLogError("VERSION VERSION FAIL");
				exit(EXIT_FAILURE);
			}
			if (memcmp(CBByteArrayGetData(CBGetVersion(theMessage)->userAgent), CB_USER_AGENT_SEGMENT, CBGetVersion(theMessage)->userAgent->length)) {
				CBLogError("VERSION USER AGENT FAIL");
				exit(EXIT_FAILURE);
			}
			if (memcmp(CBByteArrayGetData(CBGetVersion(theMessage)->addSource->ip), (uint8_t [16]){0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF, 127, 0, 0, 1}, CBGetVersion(theMessage)->addSource->ip->length)) {
				CBLogError("VERSION SOURCE IP FAIL");
				exit(EXIT_FAILURE);
			}
			if (memcmp(CBByteArrayGetData(CBGetVersion(theMessage)->addRecv->ip), (uint8_t [16]){0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF, 127, 0, 0, 1}, CBGetVersion(theMessage)->addRecv->ip->length)) {
				CBLogError("VERSION RECEIVE IP FAIL");
				exit(EXIT_FAILURE);
			}
			*prog |= GOTVERSION;
			break;
		case CB_MESSAGE_TYPE_VERACK:
			if (! (peer->handshakeStatus & CB_HANDSHAKE_SENT_VERSION)) {
				CBLogError("VERACK FAIL = %u", peer->handshakeStatus);
				exit(EXIT_FAILURE);
			}
			*prog |= GOTACK;
			break;
		case CB_MESSAGE_TYPE_PING:
			if (! (*prog & GOTVERSION && *prog & GOTACK)) {
				CBLogError("PING FAIL");
				exit(EXIT_FAILURE);
			}
			CBGetPingPong(theMessage)->ID = CBGetPingPong(theMessage)->ID; // Test access to ID.
			*prog |= GOTPING;
			break;
		case CB_MESSAGE_TYPE_PONG:
			if (! (*prog & GOTVERSION && *prog & GOTACK)) {
				CBLogError("PONG FAIL");
				exit(EXIT_FAILURE);
			}
			CBGetPingPong(theMessage)->ID = CBGetPingPong(theMessage)->ID; // Test access to ID.
			// At least one of the pongs worked.
			*prog |= GOTPONG;
			break;
		case CB_MESSAGE_TYPE_GETADDR:
			if (! (*prog & GOTVERSION && *prog & GOTACK)) {
				CBLogError("GET ADDR FAIL");
				exit(EXIT_FAILURE);
			}
			*prog |= GOTGETADDR;
			break;
		case CB_MESSAGE_TYPE_ADDR:
			if (! (*prog & GOTVERSION && *prog & GOTACK)) {
				CBLogError("ADDR FAIL");
				exit(EXIT_FAILURE);
			}
			tester.addrComplete++;
			break;
		default:
			CBLogError("MESSAGE FAIL");
			exit(EXIT_FAILURE);
			break;
	}
	if (*prog == (GOTVERSION | GOTACK | GOTPING | GOTPONG | GOTGETADDR)) {
		*prog |= COMPLETE;
		tester.complete++;
	}
	if (tester.addrComplete > 7) {
		// addrComplete can be seven in the case that one of the listening nodes has connected to the other and then the connecting node asks the other node what addresses it has and it has the other node.
		CBLogError("ADDR COMPLETE FAIL");
		exit(EXIT_FAILURE);
	}
	if (tester.complete == 6) {
		if (tester.addrComplete > 4) {
			// Usually addrComplete will be 6 but sometimes addresses are not sent in response to getaddr when the address is being connected to. In reality this is not a problem, as it is seldom an address will be in a connecting state.
			// Completed testing
			CBLogVerbose("DONE");
			CBLogVerbose("STOPPING COMM L1");
			CBRunOnEventLoop(tester.comms[0]->eventLoop, stop, tester.comms[0], false);
			CBLogVerbose("STOPPING COMM L2");
			CBRunOnEventLoop(tester.comms[1]->eventLoop, stop, tester.comms[1], false);
			CBLogVerbose("STOPPING COMM CN");
			CBRunOnEventLoop(tester.comms[2]->eventLoop, stop, tester.comms[2], false);
			return CB_MESSAGE_ACTION_RETURN;
		}else{
			CBLogError("ADDR COMPLETE DURING COMPLETE FAIL");
			exit(EXIT_FAILURE);
		}
	}
	pthread_mutex_unlock(&tester.testingMutex);
	return CB_MESSAGE_ACTION_CONTINUE;
}
void onNetworkError(CBNetworkCommunicator * comm);
void onNetworkError(CBNetworkCommunicator * comm){
	CBLogError("DID LOSE LAST NODE");
	exit(EXIT_FAILURE);
}
void onBadTime(void * foo);
void onBadTime(void * foo){
	CBLogError("BAD TIME FAIL");
	exit(EXIT_FAILURE);
}
void onPeerWhatever(CBNetworkCommunicator * foo, CBPeer * bar);
void onPeerWhatever(CBNetworkCommunicator * foo, CBPeer * bar){
	return;
}

void onPeerFree(void * peer);
void onPeerFree(void * peer){
	return;
}

void CBNetworkCommunicatorTryConnectionsVoid(void * comm);
void CBNetworkCommunicatorTryConnectionsVoid(void * comm){
	CBNetworkCommunicatorTryConnections(comm);
}

void CBNetworkCommunicatorStartListeningVoid(void * comm);
void CBNetworkCommunicatorStartListeningVoid(void * comm){
	CBNetworkCommunicatorStartListening(comm);
}

int main(){
	puts("You may need to move your mouse around if this test stalls.");
	memset(&tester, 0, sizeof(tester));
	pthread_mutex_init(&tester.testingMutex, NULL);
	// Create three CBNetworkCommunicators and connect over the loopback address. Two will listen, one will connect. Test auto handshake, auto ping and auto discovery.
	CBByteArray * loopBack = CBNewByteArrayWithDataCopy((uint8_t [16]){0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF, 127, 0, 0, 1}, 16);
	CBByteArray * loopBack2 = CBByteArrayCopy(loopBack); // Do not use in more than one thread.
	CBNetworkAddress * addrListen = CBNewNetworkAddress(0, loopBack, 45562, 0, false);
	CBNetworkAddress * addrListenB = CBNewNetworkAddress(0, loopBack2, 45562, 0, true); // Use in connector thread.
	CBNetworkAddress * addrListen2 = CBNewNetworkAddress(0, loopBack, 45563, 0, false);
	CBNetworkAddress * addrListen2B = CBNewNetworkAddress(0, loopBack2, 45563, 0, true); // Use in connector thread.
	CBNetworkAddress * addrConnect = CBNewNetworkAddress(0, loopBack, 45564, 0, false); // Different port over loopback to seperate the CBNetworkCommunicators.
	CBReleaseObject(loopBack);
	CBReleaseObject(loopBack2);
	CBByteArray * userAgent = CBNewByteArrayFromString(CB_USER_AGENT_SEGMENT, false);
	CBByteArray * userAgent2 = CBNewByteArrayFromString(CB_USER_AGENT_SEGMENT, false);
	CBByteArray * userAgent3 = CBNewByteArrayFromString(CB_USER_AGENT_SEGMENT, false);
	// First listening CBNetworkCommunicator setup.
	CBNetworkAddressManager * addrManListen = CBNewNetworkAddressManager(onBadTime);
	addrManListen->maxAddressesInBucket = 2;
	CBNetworkCommunicatorCallbacks callbacks = {
		onPeerWhatever,
		onPeerFree,
		onTimeOut,
		acceptType,
		onMessageReceived,
		onNetworkError
	};
	CBNetworkCommunicator * commListen = CBNewNetworkCommunicator(callbacks);
	CBNetworkCommunicatorSetReachability(commListen, CB_IP_IP4 | CB_IP_LOCAL, true);
	addrManListen->callbackHandler = commListen;
	commListen->networkID = CB_PRODUCTION_NETWORK_BYTES;
	commListen->flags = CB_NETWORK_COMMUNICATOR_AUTO_HANDSHAKE | CB_NETWORK_COMMUNICATOR_AUTO_PING | CB_NETWORK_COMMUNICATOR_AUTO_DISCOVERY;
	commListen->version = CB_PONG_VERSION;
	commListen->maxConnections = 3;
	commListen->maxIncommingConnections = 3; // One for connector, one for the other listener and an extra so that we continue to share our address.
	commListen->heartBeat = 2000;
	commListen->timeOut = 3000;
	commListen->recvTimeOut = 1000;
	CBNetworkCommunicatorSetAlternativeMessages(commListen, NULL, NULL);
	CBNetworkCommunicatorSetNetworkAddressManager(commListen, addrManListen);
	CBNetworkCommunicatorSetUserAgent(commListen, userAgent);
	CBNetworkCommunicatorSetOurIPv4(commListen, addrListen);
	// Second listening CBNetworkCommunicator setup.
	CBNetworkAddressManager * addrManListen2 = CBNewNetworkAddressManager(onBadTime);
	addrManListen2->maxAddressesInBucket = 2;
	CBNetworkCommunicator * commListen2 = CBNewNetworkCommunicator(callbacks);
	CBNetworkCommunicatorSetReachability(commListen2, CB_IP_IP4 | CB_IP_LOCAL, true);
	addrManListen2->callbackHandler = commListen2;
	commListen2->networkID = CB_PRODUCTION_NETWORK_BYTES;
	commListen2->flags = CB_NETWORK_COMMUNICATOR_AUTO_HANDSHAKE | CB_NETWORK_COMMUNICATOR_AUTO_PING | CB_NETWORK_COMMUNICATOR_AUTO_DISCOVERY;
	commListen2->version = CB_PONG_VERSION;
	commListen2->maxConnections = 3;
	commListen2->maxIncommingConnections = 3;
	commListen2->heartBeat = 2000;
	commListen2->timeOut = 3000;
	commListen2->recvTimeOut = 1000;
	CBNetworkCommunicatorSetAlternativeMessages(commListen2, NULL, NULL);
	CBNetworkCommunicatorSetNetworkAddressManager(commListen2, addrManListen2);
	CBNetworkCommunicatorSetUserAgent(commListen2, userAgent2);
	CBNetworkCommunicatorSetOurIPv4(commListen2, addrListen2);
	// Connecting CBNetworkCommunicator setup.
	CBNetworkAddressManager * addrManConnect = CBNewNetworkAddressManager(onBadTime);
	addrManConnect->maxAddressesInBucket = 2;
	// We are going to connect to both listing CBNetworkCommunicators.
	CBNetworkAddressManagerAddAddress(addrManConnect, addrListenB);
	CBNetworkAddressManagerAddAddress(addrManConnect, addrListen2B);
	CBNetworkCommunicator * commConnect = CBNewNetworkCommunicator(callbacks);
	CBNetworkCommunicatorSetReachability(commConnect, CB_IP_IP4 | CB_IP_LOCAL, true);
	addrManConnect->callbackHandler = commConnect;
	commConnect->networkID = CB_PRODUCTION_NETWORK_BYTES;
	commConnect->flags = CB_NETWORK_COMMUNICATOR_AUTO_HANDSHAKE | CB_NETWORK_COMMUNICATOR_AUTO_PING | CB_NETWORK_COMMUNICATOR_AUTO_DISCOVERY;
	commConnect->version = CB_PONG_VERSION;
	commConnect->maxConnections = 2;
	commConnect->maxIncommingConnections = 0;
	commConnect->heartBeat = 2000;
	commConnect->timeOut = 3000;
	commConnect->recvTimeOut = 1000;
	CBNetworkCommunicatorSetAlternativeMessages(commConnect, NULL, NULL);
	CBNetworkCommunicatorSetNetworkAddressManager(commConnect, addrManConnect);
	CBNetworkCommunicatorSetUserAgent(commConnect, userAgent3);
	CBNetworkCommunicatorSetOurIPv4(commConnect, addrConnect);
	// Release objects
	CBReleaseObject(userAgent);
	// Give tester communicators
	tester.comms[0] = commListen;
	tester.comms[1] = commListen2;
	tester.comms[2] = commConnect;
	// Start listening on first listener. Can start listening on this thread since the event loop will not be doing anything.
	CBNetworkCommunicatorStart(commListen);
	CBDepObject listenThread = ((CBEventLoop *) commListen->eventLoop.ptr)->loopThread;
	CBRunOnEventLoop(commListen->eventLoop, CBNetworkCommunicatorStartListeningVoid, commListen, true);
	if (! commListen->ipData[CB_IP4_NETWORK].isListening) {
		CBLogError("FIRST LISTEN FAIL");
		exit(EXIT_FAILURE);
	}
	// Start listening on second listener.
	CBNetworkCommunicatorStart(commListen2);
	CBDepObject listen2Thread = ((CBEventLoop *) commListen2->eventLoop.ptr)->loopThread;
	CBRunOnEventLoop(commListen2->eventLoop, CBNetworkCommunicatorStartListeningVoid, commListen2, true);
	if (! commListen2->ipData[CB_IP4_NETWORK].isListening) {
		CBLogError("SECOND LISTEN FAIL");
		exit(EXIT_FAILURE);
	}
	// Start connection
	CBNetworkCommunicatorStart(commConnect);
	CBDepObject connectThread = ((CBEventLoop *) commConnect->eventLoop.ptr)->loopThread;
	CBRunOnEventLoop(commConnect->eventLoop, CBNetworkCommunicatorTryConnectionsVoid, commConnect, true);
	// Wait until the network loop ends for all CBNetworkCommunicators.
	CBThreadJoin(listenThread);
	CBThreadJoin(listen2Thread);
	CBThreadJoin(connectThread);
	// Check addresses in the first listening CBNetworkCommunicator
	if (commListen->addresses->addrNum != 1) { // Only L2 should go back to addresses. CN is private
		CBLogError("ADDRESS DISCOVERY LISTEN ONE ADDR NUM FAIL %i != 1", commListen->addresses->addrNum);
		exit(EXIT_FAILURE);
	}
	if(! CBNetworkAddressManagerGotNetworkAddress(commListen->addresses, addrListen2)){
		CBLogError("ADDRESS DISCOVERY LISTEN ONE LISTEN TWO FAIL");
		exit(EXIT_FAILURE);
	}
	// Check the addresses in the second.
	if (commListen->addresses->addrNum != 1) {
		CBLogError("ADDRESS DISCOVERY LISTEN TWO ADDR NUM FAIL");
		exit(EXIT_FAILURE);
	}
	if (! CBNetworkAddressManagerGotNetworkAddress(commListen2->addresses, addrListen)){
		CBLogError("ADDRESS DISCOVERY LISTEN TWO LISTEN ONE FAIL");
		exit(EXIT_FAILURE);
	}
	// And lastly the connecting CBNetworkCommunicator
	if (commConnect->addresses->addrNum != 2) {
		CBLogError("ADDRESS DISCOVERY CONNECT ADDR NUM FAIL");
		exit(EXIT_FAILURE);
	}
	addrListen->bucketSet = false;
	if (! CBNetworkAddressManagerGotNetworkAddress(commConnect->addresses, addrListen)){
		CBLogError("ADDRESS DISCOVERY CONNECT LISTEN ONE FAIL");
		exit(EXIT_FAILURE);
	}
	addrListen2->bucketSet = false;
	if (! CBNetworkAddressManagerGotNetworkAddress(commConnect->addresses, addrListen2)){
		CBLogError("ADDRESS DISCOVERY CONNECT LISTEN TWO FAIL");
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
