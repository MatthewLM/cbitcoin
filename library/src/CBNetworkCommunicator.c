//
//  CBNetworkCommunicator.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 13/07/2012.
//  Copyright (c) 2012 Matthew Mitchell
//
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

//  SEE HEADER FILE FOR DOCUMENTATION

#include "CBNetworkCommunicator.h"
#include "CBSeedNodes.h"

//  Constructor

CBNetworkCommunicator * CBNewNetworkCommunicator(CBVersionServices services, CBNetworkCommunicatorCallbacks callbacks){
	CBNetworkCommunicator * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeNetworkCommunicator;
	if (!CBInitNetworkCommunicator(self, services, callbacks)){
		free(self);
		return NULL;
	}
	return self;
}

//  Initialiser

bool CBInitNetworkCommunicator(CBNetworkCommunicator * self, CBVersionServices services, CBNetworkCommunicatorCallbacks callbacks){
	CBInitObject(CBGetObject(self), false);
	CBNewMutex(&self->peersMutex);
	// Set fields.
	self->callbacks = callbacks;
	self->attemptingOrWorkingConnections = 0;
	self->numIncommingConnections = 0;
	for (uint8_t x = 0; x < 4; x++) {
		self->ipData[x].isListening = false;
		CBByteArray * ip = CBNewByteArrayOfSize(16);
		// By default give null address
		CBByteArraySetBytes(ip, 0, CB_NULL_ADDRESS, 16);
		self->ipData[x].ourAddress = CBNewNetworkAddress(0, (CBSocketAddress){ip, 0}, services, false);
		self->ipData[x].isSet = false;
		self->ipData[x].listeningPort = 8333;
	}
	self->isPinging = false;
	self->nonce = 0;
	self->stoppedListening = false;
	self->useAddrStorage = false;
	self->reachability = 0;
	self->alternativeMessages = NULL;
	self->alternativeMessages = NULL;
	self->altMaxSizes = NULL;
	self->addedHardcodedSeeds = false;
	self->tryConnectionTimerStarted = false;
	// Default settings
	self->maxAddresses = 1000000;
	self->maxConnections = 8;
	self->maxIncommingConnections = 4;
	self->recvTimeOut = 0;//100;
	self->responseTimeOut = 0;//10000;
	self->sendTimeOut = 0;//1000;
	self->timeOut = 0;// 5400000;
	self->heartBeat = 1800000;
	self->connectionTimeOut = 0;//5000;
	self->flags = 0;
	self->services = services;
	self->blockHeight = 0;
	for (uint8_t x = 0; x < 3; x++) {
		self->ip4s[x] = NULL;
		self->ip4Count[x] = 0;
		self->ip6s[x] = NULL;
		self->ip6Count[x] = 0;
	}
	// Start event loop
	if (! CBNewEventLoop(&self->eventLoop, CBNetworkCommunicatorOnLoopError, CBNetworkCommunicatorOnTimeOut, self)){
		// Cannot create event loop
		CBLogError("The CBNetworkCommunicator event loop could not be created.");
		return false;
	}
	return true;
}

//  Destructor

void CBDestroyNetworkCommunicator(void * vself){
	CBNetworkCommunicator * self = vself;
	CBNetworkCommunicatorStop(self);
	if (self->alternativeMessages) CBReleaseObject(self->alternativeMessages);
	CBReleaseObject(self->addresses);
	for (uint8_t x = 0; x < 4; x++)
		CBReleaseObject(self->ipData[x].ourAddress);
	free(self->altMaxSizes);
	CBFreeMutex(self->peersMutex);
	// Stop event loop
	CBExitEventLoop(self->eventLoop);
}
void CBFreeNetworkCommunicator(void * self){
	CBDestroyNetworkCommunicator(self);
	free(self);
}

//  Functions

void CBNetworkCommunicatorAcceptConnection(void * vself, CBDepObject socket){
	CBNetworkCommunicator * self = vself;
	CBDepObject connectSocketID;
	CBSocketAddress sockAddr;
	if (! CBSocketAccept(socket, &connectSocketID, &sockAddr)){
		CBLogError("Unable to accept a connection.");
		return;
	}
	// Connected, add peer
	CBNetworkAddress * addr = CBNewNetworkAddress(0, sockAddr, 0, false);
	CBPeer * peer = CBNewPeer(addr);
	CBReleaseObject(addr);
	peer->incomming = true;
	peer->socketID = connectSocketID;
	// Set up receive event
	if (CBSocketCanReceiveEvent(&peer->receiveEvent, self->eventLoop, peer->socketID, CBNetworkCommunicatorOnCanReceive, peer)) {
		// The event works
		if (CBSocketAddEvent(peer->receiveEvent, self->responseTimeOut)){ // Begin receive event.
			// Success
			if (CBSocketCanSendEvent(&peer->sendEvent, self->eventLoop, peer->socketID, CBNetworkCommunicatorOnCanSend, peer)) {
				// Both events work. Take the peer.
				CBMutexLock(self->peersMutex);
				CBNetworkAddressManagerTakePeer(self->addresses, peer);
				CBMutexUnlock(self->peersMutex);
				if (self->addresses->peersNum == 1 && self->flags & CB_NETWORK_COMMUNICATOR_AUTO_PING)
					// Got first peer, start pings
					CBNetworkCommunicatorStartPings(self);
				peer->connectionWorking = true;
				self->attemptingOrWorkingConnections++;
				self->numIncommingConnections++;
				if (self->numIncommingConnections == self->maxIncommingConnections || self->attemptingOrWorkingConnections == self->maxConnections) {
					// Reached maximum connections, stop listening.
					CBNetworkCommunicatorStopListening(self);
					self->stoppedListening = true;
				}
				self->callbacks.onPeerConnection(self, peer);
				CBNetworkAddressToString(peer->addr, peer->peerStr);
				CBLogVerbose("Accepted an incoming connection from %s. %u incoming connections.", peer->peerStr, self->numIncommingConnections);
				return;
			}
		}
		// Could create receive event but there was a failure afterwards so free it.
		CBSocketFreeEvent(peer->receiveEvent);
	}
	// Failure, release peer.
	CBCloseSocket(connectSocketID);
	CBReleaseObject(peer);
	CBLogError("Failure setting up events for incoming peer.");
}
CBConnectReturn CBNetworkCommunicatorConnect(CBNetworkCommunicator * self, CBPeer * peer){
	if (! CBNetworkCommunicatorIsReachable(self, peer->addr->type))
		return CB_CONNECT_NO_SUPPORT;
	bool isIPv6 = peer->addr->type & CB_IP_IP6
				  || peer->addr->type & CB_IP_TOR
				  || peer->addr->type & CB_IP_I2P;
	// Make socket
	CBSocketReturn res = CBNewSocket(&peer->socketID, isIPv6);
	if (res == CB_SOCKET_NO_SUPPORT){
		CBNetworkCommunicatorSetReachability(self, peer->addr->type, false);
		if (! CBNetworkCommunicatorIsReachable(self, CB_IP_IP6 | CB_IP_IP4 | CB_IP_I2P | CB_IP_TOR)){
			// No networks reachable.
			CBLogError("No networks are reachable.");
			self->callbacks.onNetworkError(self, CB_ERROR_UNREACHABLE);
		}
		return CB_CONNECT_NO_SUPPORT;
	}else if (res == CB_SOCKET_BAD)
		return CB_CONNECT_ERROR;
	// Connect
	if (CBSocketConnect(peer->socketID, CBByteArrayGetData(peer->addr->sockAddr.ip), isIPv6, peer->addr->sockAddr.port)){
		// Add event for connection
		if (CBSocketDidConnectEvent(&peer->connectEvent, self->eventLoop, peer->socketID, CBNetworkCommunicatorDidConnect, peer)) {
			if (CBSocketAddEvent(peer->connectEvent, self->connectionTimeOut)) {
				// ??? Connection timeout does not work.
				// Connection is fine. Retain for waiting for connect.
				CBRetainObject(peer);
				self->attemptingOrWorkingConnections++;
				peer->connecting = true; // In the process of connecting.
				return CB_CONNECT_OK;
			}else
				CBSocketFreeEvent(peer->connectEvent);
		}
		CBCloseSocket(peer->socketID);
		return CB_CONNECT_ERROR;
	}
	CBCloseSocket(peer->socketID);
	return CB_CONNECT_FAILED;
}
void CBNetworkCommunicatorDetermineIP(CBNetworkCommunicator * self, CBNetworkAddress * addr, bool ipv4){
	uint32_t * count = ipv4 ? self->ip4Count : self->ip6Count;
	CBNetworkAddress ** ips = ipv4 ? self->ip4s : self->ip6s;
	void (*setIp)(CBNetworkCommunicator *, CBNetworkAddress *) = ipv4 ? CBNetworkCommunicatorSetOurIPv4 : CBNetworkCommunicatorSetOurIPv6;
	// Ensure port is the same as the port we specifiy
	addr->sockAddr.port = self->ipData[ipv4 ? CB_IP4_NETWORK : CB_IP6_NETWORK].listeningPort;
	uint8_t insert = 255;
	uint32_t maxCount = count[(count[0] > count[1]) ? 0 : 1], minCount = UINT32_MAX;
	if (count[2] > maxCount) maxCount = count[2];
	for (uint8_t x = 0; x < 3; x++) {
		if (!ips[x]) {
			insert = x;
			if (maxCount == 0)
				// This is the new address of ours.
				setIp(self, addr);
			break;
		}
		if (CBNetworkAddressIPPortCompare(NULL, ips[x], addr) == CB_COMPARE_EQUAL){
			if (count[x]++ > maxCount)
				// This is the new address of ours.
				setIp(self, ips[x]);
			break;
		}
		if (count[x] < minCount) {
			insert = x;
			minCount = count[x];
		}
	}
	if (insert != 255) {
		ips[insert] = addr;
		addr->services = self->services;
		addr->isPublic = self->numIncommingConnections != 0;
		CBRetainObject(ips[insert]);
	}
}
void CBNetworkCommunicatorDidConnect(void * vself, void * vpeer){
	CBNetworkCommunicator * self = vself;
	CBPeer * peer = vpeer;
	peer->connecting = false; // No longer in the process of connecting.
	CBSocketFreeEvent(peer->connectEvent); // No longer need this event.
	// Check to see if in the meantime, that we have not been connected to by the peer. Double connections are bad m'kay.
	if (! CBNetworkAddressManagerGotPeer(self->addresses, peer->addr)){
		// Make receive event
		if (CBSocketCanReceiveEvent(&peer->receiveEvent, self->eventLoop, peer->socketID, CBNetworkCommunicatorOnCanReceive, peer)) {
			// Make send event
			if (CBSocketCanSendEvent(&peer->sendEvent, self->eventLoop, peer->socketID, CBNetworkCommunicatorOnCanSend, peer)) {
				if (CBSocketAddEvent(peer->sendEvent, self->sendTimeOut)) {
					CBMutexLock(self->peersMutex);
					CBNetworkAddressManagerTakePeer(self->addresses, peer);
					CBMutexUnlock(self->peersMutex);
					if (self->addresses->peersNum == 1 && self->flags & CB_NETWORK_COMMUNICATOR_AUTO_PING)
						// Got first peer, start pings
						CBNetworkCommunicatorStartPings(self);
					peer->connectionWorking = true;
					// Connection OK, so begin handshake if auto handshaking is enabled.
					if (self->flags & CB_NETWORK_COMMUNICATOR_AUTO_HANDSHAKE){
						CBVersion * version = CBNetworkCommunicatorGetVersion(self, peer->addr);
						CBGetMessage(version)->expectResponse = CB_MESSAGE_TYPE_VERSION; // Expect a version in return.
						CBGetMessage(version)->type = CB_MESSAGE_TYPE_VERSION;
						CBNetworkCommunicatorSendMessage(self, peer, CBGetMessage(version), NULL);
						CBReleaseObject(version);
					}
					CBLogVerbose("Did connect to %s", peer->peerStr);
					self->callbacks.onPeerConnection(self, peer);
					return;
				}
				CBSocketFreeEvent(peer->sendEvent);
			}
			CBSocketFreeEvent(peer->receiveEvent);
		}
		// Close socket on failure
		CBCloseSocket(peer->socketID);
		// Add the address back to the addresses listwith no penalty here since it was definitely our fault.
		// Do not return the address if we have the peer already.
		CBNetworkAddressManagerAddAddress(self->addresses, peer->addr);
		CBReleaseObject(peer);
		CBLogVerbose("Failed to setup events for the peer %s which we connected to", peer->peerStr);
	}else{
		CBCloseSocket(peer->socketID);
		CBLogVerbose("Detected a double connection of %s", peer->peerStr);
	}
	self->attemptingOrWorkingConnections--;
	if (self->attemptingOrWorkingConnections == 0)
		CBNetworkCommunicatorNoPeers(self);
}
void CBNetworkCommunicatorDisconnect(CBNetworkCommunicator * self, CBPeer * peer, uint32_t penalty, bool stopping){
	CBLogVerbose("Disconnecting from %s", peer->peerStr);
	bool wasWorking = peer->connectionWorking;
	peer->connectionWorking = false;
	// Close the socket
	CBCloseSocket(peer->socketID);
	// If incomming, lower the incomming connections number
	if (peer->incomming)
		self->numIncommingConnections--;
	if (!stopping && self->stoppedListening && self->numIncommingConnections < self->maxIncommingConnections)
		// Start listening again
		CBNetworkCommunicatorStartListening(self);
	// Lower the attempting or working connections number
	self->attemptingOrWorkingConnections--;
	// If this is a working connection, remove from the address manager peer's list.
	if (wasWorking){
		// Release data created when connection was working
		CBSocketFreeEvent(peer->receiveEvent);
		CBSocketFreeEvent(peer->sendEvent);
		// Release the receiving message object if it exists.
		if (peer->receive) CBReleaseObject(peer->receive);
		// Release all messages in the send queue
		// Ensure the send queue is not being modified by mutex
		for (uint8_t x = 0; x < peer->sendQueueSize; x++)
			CBReleaseObject(peer->sendQueue[(peer->sendQueueFront + x) % CB_SEND_QUEUE_MAX_SIZE].message);
		if (peer->addr->isPublic) {
			// Public peer, return to addresses list.
			// Apply the penalty given
			peer->addr->penalty += penalty;
			// Adding the address may fail. If it does, we just ignore it and lose the address.
			CBNetworkAddressManagerAddAddress(self->addresses, peer->addr);
		}
		// If not stopping we can remove the peer.
		if (! stopping){
			// We aren't stopping so we should remove the node from the array.
			CBMutexLock(self->peersMutex);
			// This will release the peer
			CBNetworkAddressManagerRemovePeer(self->addresses, peer);
			CBMutexUnlock(self->peersMutex);
		}
	}else{
		// Else we release the object from control of the CBNetworkCommunicator
		// Free connectEvent only if we are connecting to it.
		if (peer->connecting)
			CBSocketFreeEvent(peer->connectEvent);
		CBReleaseObject(peer);
	}
	if (self->addresses->peersNum == 0 && self->flags & CB_NETWORK_COMMUNICATOR_AUTO_PING)
		// No more peers so stop pings
		CBNetworkCommunicatorStopPings(self);
	if (! stopping) {
		if (self->attemptingOrWorkingConnections != 0)
			// Try for more connections in 20 seconds.
			CBNetworkCommunicatorRetryConnections(self);
		else
			// No more connections so give a network error
			CBNetworkCommunicatorNoPeers(self);
	}
}
CBNetworkAddress * CBNetworkCommunicatorGetOurMainAddress(CBNetworkCommunicator * self, CBIPType recipientType){
	// I2P or Tor used if available
	if (self->ipData[CB_TOR_NETWORK].isSet)
		return self->ipData[CB_TOR_NETWORK].ourAddress;
	if (self->ipData[CB_I2P_NETWORK].isSet)
		return self->ipData[CB_I2P_NETWORK].ourAddress;
	if (recipientType & CB_IP_IP6
			 && self->ipData[CB_IP6_NETWORK].isSet)
		return self->ipData[CB_IP6_NETWORK].ourAddress;
	return self->ipData[CB_IP4_NETWORK].ourAddress;
}
CBVersion * CBNetworkCommunicatorGetVersion(CBNetworkCommunicator * self, CBNetworkAddress * addRecv){
	CBNetworkAddress * sourceAddr = CBNetworkCommunicatorGetOurMainAddress(self, addRecv->type);
	self->nonce = rand();
	// If the peer's address is local give a null address
	if (addRecv->type & CB_IP_LOCAL) {
		CBByteArray * ip = CBNewByteArrayWithDataCopy(CB_NULL_ADDRESS, 16);
		addRecv = CBNewNetworkAddress(0, (CBSocketAddress){ip, 0}, 0, false);
		CBReleaseObject(ip);
	}else
		CBRetainObject(addRecv);
	CBVersion * version = CBNewVersion(self->version, self->services, time(NULL), addRecv, sourceAddr, self->nonce, self->userAgent, self->blockHeight);
	CBReleaseObject(addRecv);
	return version;
}
bool CBNetworkCommunicatorIsReachable(CBNetworkCommunicator * self, CBIPType type){
	if (type == CB_IP_INVALID)
		return false;
	return self->reachability & type;
}
void CBNetworkCommunicatorNoPeers(CBNetworkCommunicator * self){
	// Give error
	self->callbacks.onNetworkError(self, CB_ERROR_NO_PEERS);
	if (!(self->flags & CB_NETWORK_COMMUNICATOR_BOOTSTRAP)
		|| self->flags & CB_NETWORK_COMMUNICATOR_INCOMING_ONLY)
		// Only retry connections ourself when bootstapping is enabled.
		return;
	if (!self->addedHardcodedSeeds) {
		// Straightaway add the hardcoded seeds and try the connections.
		self->addedHardcodedSeeds = true;
		CBForEach(uint8_t * node, seedNodes) {
			CBByteArray * ipBytes = CBNewByteArrayWithDataCopy((uint8_t [16]){0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF, node[3], node[2], node[1], node[0]}, 16);
			CBNetworkAddress * addr = CBNewNetworkAddress(time(NULL), (CBSocketAddress){ipBytes, 8333}, CB_SERVICE_FULL_BLOCKS, false);
			CBReleaseObject(ipBytes);
			CBNetworkAddressManagerAddAddress(self->addresses, addr);
			CBReleaseObject(addr);
		}
		CBNetworkCommunicatorTryConnections(self, false);
		return;
	}
	CBNetworkCommunicatorRetryConnections(self);
}
void CBNetworkCommunicatorOnCanReceive(void * vself, void * vpeer){
	CBNetworkCommunicator * self = vself;
	CBPeer * peer = vpeer;
	// Node kindly has some data available in the socket buffer.
	if (! peer->receive) {
		// New message to be received.
		peer->receive = CBNewMessageByObject();
		peer->receive->serialised = true;
		peer->headerBuffer = malloc(24); // Twenty-four bytes for the message header.
		peer->messageReceived = 0; // So far received nothing.
		// From now on use timeout for receiving data.
		if (! CBSocketAddEvent(peer->receiveEvent, self->recvTimeOut)){
			CBLogError("Could not change the timeout for a peer's receive event for receiving a new message");
			CBNetworkCommunicatorDisconnect(self, peer, 0, false);
			return;
		}
		// Start download timer
		peer->downloadTimerStart = CBGetMilliseconds();
	}
	if (! peer->receivedHeader) {
		// Not received the complete message header yet.
		int32_t num = CBSocketReceive(peer->socketID, peer->headerBuffer + peer->messageReceived, 24 - peer->messageReceived);
		switch (num) {
			case CB_SOCKET_CONNECTION_CLOSE:
				free(peer->headerBuffer); // Free the buffer
				CBNetworkCommunicatorDisconnect(self, peer, 7200, false); // Remove with penalty for disconnection
				return;
			case CB_SOCKET_FAILURE:
				// Failure so remove peer.
				free(peer->headerBuffer); // Free the buffer
				CBNetworkCommunicatorDisconnect(self, peer, 0, false);
				return;
			default:
				// Did read some bytes
				peer->messageReceived += num;
				if (peer->messageReceived == 24)
					// Hurrah! The header has been received.
					CBNetworkCommunicatorOnHeaderRecieved(self, peer);
				return;
		}
	}else{
		// Means we have payload to fetch as we have the header but not the payload
		int32_t num = CBSocketReceive(peer->socketID, CBByteArrayGetData(peer->receive->bytes) + peer->messageReceived, peer->receive->bytes->length - peer->messageReceived);
		switch (num) {
			case CB_SOCKET_CONNECTION_CLOSE:
				CBNetworkCommunicatorDisconnect(self, peer, 7200, false); // Remove with penalty for disconnection
				return;
			case CB_SOCKET_FAILURE:
				// Failure so remove peer.
				CBNetworkCommunicatorDisconnect(self, peer, 0, false);
				return;
			default:
				// Did read some bytes
				peer->messageReceived += num;
				if (peer->messageReceived == peer->receive->bytes->length)
					// We now have the message.
					CBNetworkCommunicatorOnMessageReceived(self, peer);
				return;
		}
	}
}
void CBNetworkCommunicatorOnCanSend(void * vself, void * vpeer){
	CBNetworkCommunicator * self = vself;
	CBPeer * peer = vpeer;
	bool finished = false;
	// Can now send data
	// Get the message we are sending.
	CBMessage * toSend = peer->sendQueue[peer->sendQueueFront].message;
	if (! peer->sentHeader) {
		// Need to send the header
		if (peer->messageSent == 0) {
			// Create header
			peer->sendingHeader = malloc(24);
			// Network ID
			CBInt32ToArray(peer->sendingHeader, CB_MESSAGE_HEADER_NETWORK_ID, self->networkID);
			// Message type text
			switch (toSend->type) {
				case CB_MESSAGE_TYPE_VERSION:
					memcpy(peer->sendingHeader + CB_MESSAGE_HEADER_TYPE, "version\0\0\0\0\0", 12);
					break;
				case CB_MESSAGE_TYPE_VERACK:
					memcpy(peer->sendingHeader + CB_MESSAGE_HEADER_TYPE, "verack\0\0\0\0\0\0", 12);
					break;
				case CB_MESSAGE_TYPE_ADDR:
					memcpy(peer->sendingHeader + CB_MESSAGE_HEADER_TYPE, "addr\0\0\0\0\0\0\0\0", 12);
					break;
				case CB_MESSAGE_TYPE_INV:
					memcpy(peer->sendingHeader + CB_MESSAGE_HEADER_TYPE, "inv\0\0\0\0\0\0\0\0\0", 12);
					break;
				case CB_MESSAGE_TYPE_GETDATA:
					memcpy(peer->sendingHeader + CB_MESSAGE_HEADER_TYPE, "getdata\0\0\0\0\0", 12);
					break;
				case CB_MESSAGE_TYPE_GETBLOCKS:
					memcpy(peer->sendingHeader + CB_MESSAGE_HEADER_TYPE, "getblocks\0\0\0", 12);
					break;
				case CB_MESSAGE_TYPE_GETHEADERS:
					memcpy(peer->sendingHeader + CB_MESSAGE_HEADER_TYPE, "getheaders\0\0", 12);
					break;
				case CB_MESSAGE_TYPE_TX:
					memcpy(peer->sendingHeader + CB_MESSAGE_HEADER_TYPE, "tx\0\0\0\0\0\0\0\0\0\0", 12);
					break;
				case CB_MESSAGE_TYPE_BLOCK:
					memcpy(peer->sendingHeader + CB_MESSAGE_HEADER_TYPE, "block\0\0\0\0\0\0\0", 12);
					break;
				case CB_MESSAGE_TYPE_HEADERS:
					memcpy(peer->sendingHeader + CB_MESSAGE_HEADER_TYPE, "headers\0\0\0\0\0", 12);
					break;
				case CB_MESSAGE_TYPE_GETADDR:
					memcpy(peer->sendingHeader + CB_MESSAGE_HEADER_TYPE, "getaddr\0\0\0\0\0", 12);
					break;
				case CB_MESSAGE_TYPE_PING:
					memcpy(peer->sendingHeader + CB_MESSAGE_HEADER_TYPE, "ping\0\0\0\0\0\0\0\0", 12);
					break;
				case CB_MESSAGE_TYPE_PONG:
					memcpy(peer->sendingHeader + CB_MESSAGE_HEADER_TYPE, "pong\0\0\0\0\0\0\0\0", 12);
					break;
				case CB_MESSAGE_TYPE_ALERT:
					memcpy(peer->sendingHeader + CB_MESSAGE_HEADER_TYPE, "alert\0\0\0\0\0\0\0", 12);
					break;
				default:
					memcpy(peer->sendingHeader + CB_MESSAGE_HEADER_TYPE, toSend->altText, 12);
					break;
			}
			// Length
			if (toSend->bytes){
				CBInt32ToArray(peer->sendingHeader, CB_MESSAGE_HEADER_LENGTH, toSend->bytes->length);
			}else
				memset(peer->sendingHeader + CB_MESSAGE_HEADER_LENGTH, 0, 4);
			// Checksum
			memcpy(peer->sendingHeader + CB_MESSAGE_HEADER_CHECKSUM, toSend->checksum, 4);
		}
		int32_t len = CBSocketSend(peer->socketID, peer->sendingHeader + peer->messageSent, 24 - peer->messageSent);
		if (len == CB_SOCKET_FAILURE) {
			free(peer->sendingHeader);
			CBNetworkCommunicatorDisconnect(self, peer, 0, false);
		}else{
			peer->messageSent += len;
			if (peer->messageSent == 24) {
				// Done header
				free(peer->sendingHeader);
				if (toSend->bytes) {
					// Now send the bytes
					peer->messageSent = 0;
					peer->sentHeader = true;
				}else
					// Nothing more to send!
					finished = true;
			}
		}
	}else{
		// Sent header
		int32_t len = CBSocketSend(peer->socketID, CBByteArrayGetData(toSend->bytes) + peer->messageSent, toSend->bytes->length - peer->messageSent);
		if (len == CB_SOCKET_FAILURE)
			CBNetworkCommunicatorDisconnect(self, peer, 0, false);
		else{
			peer->messageSent += len;
			if (peer->messageSent == toSend->bytes->length)
				// Sent the entire payload.
				finished = true;
		}
	}
	if (finished) {
		// If we sent version or verack, record this
		if (toSend->type == CB_MESSAGE_TYPE_VERSION)
			peer->handshakeStatus |= CB_HANDSHAKE_SENT_VERSION;
		else if (toSend->type == CB_MESSAGE_TYPE_VERACK)
			peer->handshakeStatus |= CB_HANDSHAKE_SENT_ACK;
		// Reset variables for next send.
		peer->messageSent = 0;
		peer->sentHeader = false;
		// Done sending message.
		if (toSend->expectResponse){
			CBSocketAddEvent(peer->receiveEvent, self->responseTimeOut); // Expect response.
			// Add to list of expected responses.
			if (peer->typesExpectedNum == CB_MAX_RESPONSES_EXPECTED) {
				CBNetworkCommunicatorDisconnect(self, peer, CB_24_HOURS, false);
				return;
			}
			peer->typesExpected[peer->typesExpectedNum] = toSend->expectResponse;
			peer->typesExpectedNum++;
			// Start timer for latency measurement, if not already waiting.
			if (peer->latencyTimerStart == 0) {
				peer->latencyTimerStart = CBGetMilliseconds();
				peer->latencyExpected = toSend->expectResponse;
			}
		}
		// Remove message from queue.
		peer->sendQueueSize--;
		char typeStr[CB_MESSAGE_TYPE_STR_SIZE];
		CBMessageTypeToString(toSend->type, typeStr);
		CBReleaseObject(toSend);
		if (peer->sendQueueSize) {
			peer->sendQueueFront++;
			if (peer->sendQueueFront == CB_SEND_QUEUE_MAX_SIZE)
				peer->sendQueueFront = 0;
		}else
			// Remove send event as we have nothing left to send
			CBSocketRemoveEvent(peer->sendEvent);
		// Now call the callback, since the message was sent, unless the callback is NULL
		if (peer->sendQueue[peer->sendQueueFront].callback)
			peer->sendQueue[peer->sendQueueFront].callback(self, peer);
	}
}
void CBNetworkCommunicatorOnHeaderRecieved(CBNetworkCommunicator * self, CBPeer * peer){
	// Make a CBByteArray. ??? Could be modified not to use a CBByteArray, but it is cleaner this way and easier to maintain.
	CBByteArray * header = CBNewByteArrayWithData(peer->headerBuffer, 24);
	uint32_t networkID = CBByteArrayReadInt32(header, 0);
	if (networkID != self->networkID){
		// The network ID bytes is not what we are looking for. We will have to remove the peer.
		CBReleaseObject(header);
		CBLogWarning("Peer %s gave us a bad network ID (%x). We expected %x.", peer->peerStr, networkID, self->networkID);
		CBNetworkCommunicatorDisconnect(self, peer, CB_24_HOURS, false);
		return;
	}
	CBMessageType type = CB_MESSAGE_TYPE_NONE;
	uint32_t size = CBByteArrayReadInt32(header, 16);
	bool error = false;
	CBByteArray * typeBytes = CBNewByteArraySubReference(header, 4, 12);
	if (! memcmp(CBByteArrayGetData(typeBytes), "version\0\0\0\0\0", 12)) {
		// Version message
		// Check that we have not received their version yet.
		type = CB_MESSAGE_TYPE_VERSION;
		if (peer->handshakeStatus & CB_HANDSHAKE_GOT_VERSION)
			 error = true;
	}else if (!(peer->handshakeStatus & CB_HANDSHAKE_GOT_VERSION)){
		// We have not yet received the version message.
		CBLogWarning("Received non-version message before version message from %s.", peer->peerStr);
		error = true;
	}else if (! memcmp(CBByteArrayGetData(typeBytes), "verack\0\0\0\0\0\0", 12)){
		// Version acknowledgement message
		// Chek we have sent the version and not received a verack already.
		type = CB_MESSAGE_TYPE_VERACK;
		if (!(peer->handshakeStatus & CB_HANDSHAKE_SENT_VERSION)
			|| peer->handshakeStatus & CB_HANDSHAKE_GOT_ACK)
			error = true;
	}else{
		if (! memcmp(CBByteArrayGetData(typeBytes), "addr\0\0\0\0\0\0\0\0", 12)){
			// Address broadcast message
			type = CB_MESSAGE_TYPE_ADDR;
		}else if (! memcmp(CBByteArrayGetData(typeBytes), "inv\0\0\0\0\0\0\0\0\0", 12)){
			// Inventory broadcast message
			type = CB_MESSAGE_TYPE_INV;
		}else if (! memcmp(CBByteArrayGetData(typeBytes), "getdata\0\0\0\0\0", 12)){
			// Get data message
			type = CB_MESSAGE_TYPE_GETDATA;
		}else if (! memcmp(CBByteArrayGetData(typeBytes), "getblocks\0\0\0", 12)){
			// Get blocks message
			type = CB_MESSAGE_TYPE_GETBLOCKS;
		}else if (! memcmp(CBByteArrayGetData(typeBytes), "getheaders\0\0", 12)){
			// Get headers message
			type = CB_MESSAGE_TYPE_GETHEADERS;
		}else if (! memcmp(CBByteArrayGetData(typeBytes), "tx\0\0\0\0\0\0\0\0\0\0", 12)){
			// Transaction message
			type = CB_MESSAGE_TYPE_TX;
			if (size > CB_BLOCK_MAX_SIZE)
				error = true;
		}else if (! memcmp(CBByteArrayGetData(typeBytes), "block\0\0\0\0\0\0\0", 12)){
			// Block message
			type = CB_MESSAGE_TYPE_BLOCK;
			if (size > CB_BLOCK_MAX_SIZE)
				error = true;
		}else if (! memcmp(CBByteArrayGetData(typeBytes), "headers\0\0\0\0\0", 12)){
			// Block headers message
			type = CB_MESSAGE_TYPE_HEADERS;
		}else if (! memcmp(CBByteArrayGetData(typeBytes), "getaddr\0\0\0\0\0", 12)){
			// Get Addresses message
			type = CB_MESSAGE_TYPE_GETADDR;
		}else if (! memcmp(CBByteArrayGetData(typeBytes), "ping\0\0\0\0\0\0\0\0", 12)){
			// Ping message
			// Should be empty before version 60000.
			type = CB_MESSAGE_TYPE_PING;
			if ((peer->versionMessage->version < CB_PONG_VERSION || self->version < CB_PONG_VERSION) && size)
				error = true;
		}else if (! memcmp(CBByteArrayGetData(typeBytes), "pong\0\0\0\0\0\0\0\0", 12)){
			// Pong message
			type = CB_MESSAGE_TYPE_PONG;
		}else if (! memcmp(CBByteArrayGetData(typeBytes), "alert\0\0\0\0\0\0\0", 12)){
			// Alert message
			type = CB_MESSAGE_TYPE_ALERT;
		}else{
			// Either alternative or unknown.
			if (self->alternativeMessages) {
				for (uint16_t x = 0; x < self->alternativeMessages->length / 12; x++) {
					// Check this alternative message
					if (! memcmp(CBByteArrayGetData(typeBytes), CBByteArrayGetData(self->alternativeMessages) + 12 * x, 12)){
						// Alternative message
						type = CB_MESSAGE_TYPE_ALT;
						// Check payload size
						uint32_t altSize;
						if (self->altMaxSizes)
							altSize = self->altMaxSizes[x];
						else
							altSize = CB_MAX_MESSAGE_SIZE;
						if (size <= altSize) // Should correspond to the user given value
							error = false;
						break;
					}
				}
			}
		}
	}
	CBReleaseObject(typeBytes);
	char typeStr[CB_MESSAGE_TYPE_STR_SIZE];
	CBMessageTypeToString(type, typeStr);
	CBLogVerbose("Received a message header from %s with the type %s (%u).", peer->peerStr, typeStr, type);
	char headerHex[49];
	CBByteArrayToString(header, 0, 24, headerHex, false);
	CBLogVerbose("HeaderHex = %s", headerHex);
	if (error || !self->callbacks.acceptingType(self, peer, type) || size > CB_MAX_MESSAGE_SIZE) {
		// Error with the message header type or length
		CBLogWarning("There was an error with the message.");
		CBNetworkCommunicatorDisconnect(self, peer, CB_24_HOURS, false);
		CBReleaseObject(header);
		return;
	}
	if (type != CB_MESSAGE_TYPE_NONE) {
		// If this is a response we have been waiting for, no longer wait for it
		for (uint8_t x = 0; x < peer->typesExpectedNum; x++) {
			if (type == peer->typesExpected[x]) {
				// Record latency
				if (peer->latencyTimerStart && peer->latencyExpected == type) {
					peer->latencyTime += CBGetMilliseconds() - peer->latencyTimerStart;
					peer->responses++;
					peer->latencyTimerStart = 0;
				}
				// Remove
				bool remove = true;
				if (self->flags & CB_NETWORK_COMMUNICATOR_AUTO_HANDSHAKE)
					if (type == CB_MESSAGE_TYPE_VERACK && ! peer->versionMessage){
						// We expect the version
						peer->typesExpected[x] = CB_MESSAGE_TYPE_VERSION;
						remove = false;
					}
				if (remove) {
					peer->typesExpectedNum--;
					memmove(peer->typesExpected + x, peer->typesExpected + x + 1, peer->typesExpectedNum - x);
				}
				break;
			}
		}
	}
	// The type and size is OK, make the message
	peer->receive->type = type;
	// Get checksum
	peer->receive->checksum[0] = CBByteArrayGetByte(header, 20);
	peer->receive->checksum[1] = CBByteArrayGetByte(header, 21);
	peer->receive->checksum[2] = CBByteArrayGetByte(header, 22);
	peer->receive->checksum[3] = CBByteArrayGetByte(header, 23);
	// Message is now ready. Free the header.
	CBReleaseObject(header); // Took the header buffer which should be freed here.
	if (size) {
		peer->receive->bytes = CBNewByteArrayOfSize(size);
		// Change variables for receiving the payload.
		peer->receivedHeader = true;
		peer->messageReceived = 0;
	}else
		// Got the message
		CBNetworkCommunicatorOnMessageReceived(self, peer);
}
void CBNetworkCommunicatorOnLoopError(void * vself){
	CBNetworkCommunicator * self = vself;
	CBLogError("The socket event loop failed. Stoping the CBNetworkCommunicator...");
	CBNetworkCommunicatorStop(self);
}
void CBNetworkCommunicatorOnMessageReceived(CBNetworkCommunicator * self, CBPeer * peer){
	// Record download time
	peer->downloadTime += CBGetMilliseconds() - peer->downloadTimerStart;
	peer->downloadAmount += 24 + (peer->receive->bytes ? peer->receive->bytes->length : 0);
	// Change timeout of receive event, depending on wether or not we want more responses.
	CBSocketAddEvent(peer->receiveEvent, peer->typesExpectedNum ? self->responseTimeOut : self->timeOut);
	// Check checksum
	uint8_t hash[32];
	uint8_t hash2[32];
	if (peer->receive->bytes) {
		CBSha256(CBByteArrayGetData(peer->receive->bytes), peer->receive->bytes->length, hash);
		CBSha256(hash, 32, hash2);
	}else{
		hash2[0] = 0x5D;
		hash2[1] = 0xF6;
		hash2[2] = 0xE0;
		hash2[3] = 0xE2;
	}
	if (memcmp(hash2, peer->receive->checksum, 4)) {
		// Checksum failure. There is no excuse for this. Drop the peer. Why have checksums anyway???
		CBNetworkCommunicatorDisconnect(self, peer, CB_24_HOURS, false);
		return;
	}
	// Deserialise and give the onMessageReceived or onAlternativeMessageReceived event
	uint32_t len;
	switch (peer->receive->type) {
		case CB_MESSAGE_TYPE_NONE:
			// Unknown message
			return;
		case CB_MESSAGE_TYPE_VERSION:
			peer->receive = realloc(peer->receive, sizeof(CBVersion)); // For storing additional data
			CBGetObject(peer->receive)->free = CBFreeVersion;
			len = CBVersionDeserialise(CBGetVersion(peer->receive));
			break;
		case CB_MESSAGE_TYPE_ADDR:
			peer->receive = realloc(peer->receive, sizeof(CBNetworkAddress));
			CBGetObject(peer->receive)->free = CBFreeNetworkAddressList;
			CBGetNetworkAddressList(peer->receive)->timeStamps = peer->versionMessage->version > 31401; // Timestamps start version 31402 and up.
			len = CBNetworkAddressListDeserialise(CBGetNetworkAddressList(peer->receive));
			break;
		case CB_MESSAGE_TYPE_INV:
		case CB_MESSAGE_TYPE_GETDATA:
			peer->receive = realloc(peer->receive, sizeof(CBInventory));
			CBGetObject(peer->receive)->free = CBFreeInventory;
			len = CBInventoryDeserialise(CBGetInventory(peer->receive));
			break;
		case CB_MESSAGE_TYPE_GETBLOCKS:
		case CB_MESSAGE_TYPE_GETHEADERS:
			peer->receive = realloc(peer->receive, sizeof(CBGetBlocks));
			CBGetObject(peer->receive)->free = CBFreeGetBlocks;
			len = CBGetBlocksDeserialise(CBGetGetBlocks(peer->receive));
			break;
		case CB_MESSAGE_TYPE_TX:
			peer->receive = realloc(peer->receive, sizeof(CBTransaction));
			CBGetObject(peer->receive)->free = CBFreeTransaction;
			CBGetTransaction(peer->receive)->hashSet = false;
			len = CBTransactionDeserialise(CBGetTransaction(peer->receive));
			break;
		case CB_MESSAGE_TYPE_BLOCK:
			peer->receive = realloc(peer->receive, sizeof(CBBlock));
			CBGetObject(peer->receive)->free = CBFreeBlock;
			CBGetBlock(peer->receive)->hashSet = false;
			len = CBBlockDeserialise(CBGetBlock(peer->receive), true); // true -> Including transactions.
			break;
		case CB_MESSAGE_TYPE_HEADERS:
			peer->receive = realloc(peer->receive, sizeof(CBBlockHeaders));
			CBGetObject(peer->receive)->free = CBFreeBlockHeaders;
			len = CBBlockHeadersDeserialise(CBGetBlockHeaders(peer->receive));
			break;
		case CB_MESSAGE_TYPE_PING:
			if (peer->versionMessage->version >= 60000 && self->version >= 60000){
				peer->receive = realloc(peer->receive, sizeof(CBPingPong));
				CBGetObject(peer->receive)->free = CBFreePingPong;
				len = CBPingPongDeserialise(CBGetPingPong(peer->receive));
			}else len = 0;
		case CB_MESSAGE_TYPE_PONG:
			peer->receive = realloc(peer->receive, sizeof(CBPingPong));
			CBGetObject(peer->receive)->free = CBFreePingPong;
			len = CBPingPongDeserialise(CBGetPingPong(peer->receive));
			break;
		case CB_MESSAGE_TYPE_ALERT:
			peer->receive = realloc(peer->receive, sizeof(CBAlert));
			CBGetObject(peer->receive)->free = CBFreeAlert;
			len = CBAlertDeserialise(CBGetAlert(peer->receive));
			break;
		default:
			len = 0; // Zero default
			break;
	}
	// We allow for messages given to us to be of a different length, for protocol extensions.
	// Check deserialisation
	if (len == CB_DESERIALISE_ERROR) {
		CBNetworkCommunicatorDisconnect(self, peer, CB_24_HOURS, false);
		return;
	}
	// Deserialisation was sucessful.
	char messageTypeStr[CB_MESSAGE_TYPE_STR_SIZE];
	CBMessageTypeToString(peer->receive->type, messageTypeStr);
	CBLogVerbose("Processing message from %s with the type %s.", peer->peerStr, messageTypeStr);
	CBOnMessageReceivedAction action;
	// Automatic responses
	if (peer->handshakeStatus == CB_HANDSHAKE_DONE) {
		// Handshake done, do discovery and pings
		if (self->flags & CB_NETWORK_COMMUNICATOR_AUTO_DISCOVERY)
			// Auto discovery responses
			action = CBNetworkCommunicatorProcessMessageAutoDiscovery(self, peer);
		if (self->flags & CB_NETWORK_COMMUNICATOR_AUTO_PING
			// And continue from auto discovery
			&& action == CB_MESSAGE_ACTION_CONTINUE)
			// Auto ping response
			action = CBNetworkCommunicatorProcessMessageAutoPingPong(self, peer);
	}else if (self->flags & CB_NETWORK_COMMUNICATOR_AUTO_HANDSHAKE)
		// Auto handshake responses
		action = CBNetworkCommunicatorProcessMessageAutoHandshake(self, peer);
	if (action == CB_MESSAGE_ACTION_CONTINUE) 
		// Call event callback
		action = self->callbacks.onMessageReceived(self, peer, peer->receive);
	// Release objects and get ready for next message
	if (action == CB_MESSAGE_ACTION_CONTINUE) {
		CBReleaseObject(peer->receive);
		// Update "lastSeen"
		// First remove the peer from the array as is will be moved.
		// Retain as we still need the peer.
		CBRetainObject(peer);
		CBMutexLock(self->peersMutex);
		CBNetworkAddressManagerRemovePeer(self->addresses, peer);
		CBMutexUnlock(self->peersMutex);
		// Update lastSeen
		peer->addr->lastSeen = time(NULL);
		CBNetworkAddressManagerTakePeer(self->addresses, peer);
		// Reset variables for receiving the next message
		peer->receivedHeader = false;
		peer->receive = NULL;
	}else
		// Node misbehaving. Disconnect.
		CBNetworkCommunicatorDisconnect(self, peer, CB_24_HOURS, false);
}
void CBNetworkCommunicatorOnTimeOut(void * vself, void * vpeer, CBTimeOutType type){
	CBNetworkCommunicator * self = vself;
	CBPeer * peer = vpeer;
	CBLogWarning("%s from peer: %s", (char *[4]){"Connection timeout", "Send timeout", "Receive timeout", "Connection error"}[type], peer->peerStr);
	if (type == CB_TIMEOUT_RECEIVE && peer->messageReceived == 0 && ! peer->receivedHeader) {
		// Not responded
		if (peer->typesExpectedNum
			// This is a timeout of an expected response. Call onTimeOut to see if we should disconnect the node or not
			&& !self->callbacks.onTimeOut(self, peer)){
			// Should not be disconnected, remove expected types
			peer->typesExpectedNum = 0;
			return;
		}
	}
	// Remove CBNetworkAddress. 1 day penalty.
	CBNetworkCommunicatorDisconnect(self, peer, CB_24_HOURS, false);
}
CBOnMessageReceivedAction CBNetworkCommunicatorProcessMessageAutoDiscovery(CBNetworkCommunicator * self, CBPeer * peer){
	if (peer->receive->type == CB_MESSAGE_TYPE_ADDR) {
		// Received addresses.
		CBNetworkAddressList * addrs = CBGetNetworkAddressList(peer->receive);
		// Only accept no timestamps when we have less than 1000 addresses.
		if(peer->versionMessage->version < CB_ADDR_TIME_VERSION
		   && self->addresses->addrNum > 1000)
			return CB_MESSAGE_ACTION_CONTINUE;
		// Loop through addresses and store them.
		bool didAdd = false; // True when we add an address.
		for (uint16_t x = 0; x < addrs->addrNum; x++) {
			bool removeAddr = false;
			// Check if we have the address as a connected peer
			CBPeer * peerB = CBNetworkAddressManagerGotPeer(self->addresses, addrs->addresses[x]);
			if (! peerB){
				// Do not already have this address as a peer
				if (addrs->addresses[x]->type & CB_IP_INVALID)
					// Address broadcasts should not contain invalid addresses.
					return CB_MESSAGE_ACTION_DISCONNECT;
				if (addrs->addresses[x]->type & CB_IP_LOCAL && ! (peer->addr->type & CB_IP_LOCAL))
					// Do not allow peers to send local addresses to non-local peers.
					return CB_MESSAGE_ACTION_DISCONNECT;
				// Else leave the time
				// Check if we have the address as a stored address
				CBNetworkAddress * addr = CBNetworkAddressManagerGotNetworkAddress(self->addresses, addrs->addresses[x]);
				if (addr){
					// Modify services and version. If none changed then remove this address as we do not want to relay an address twice
					if (addr->services == addrs->addresses[x]->services)
						removeAddr = true;
					else
						addr->services = addrs->addresses[x]->services;
				}else if (CBNetworkCommunicatorIsReachable(self, addrs->addresses[x]->type)){
					// Do not have the address so add it if we believe we can reach it and if it is not us.
					CBIPNetwork network;
					if (addrs->addresses[x]->type & CB_IP_IP4)
						network = CB_IP4_NETWORK;
					else if (addrs->addresses[x]->type & CB_IP_IP6)
						network = CB_IP6_NETWORK;
					else if (addrs->addresses[x]->type & CB_IP_TOR)
						network = CB_TOR_NETWORK;
					else
						network = CB_I2P_NETWORK;
					if (self->ipData[network].isSet && CBNetworkAddressEquals(self->ipData[network].ourAddress, addrs->addresses[x]))
						// Is us
						continue;
					// Make copy of the address since otherwise we will corrupt the CBNetworkAddress when trying the connection.
					CBByteArray * ipCopy = CBByteArrayCopy(addrs->addresses[x]->sockAddr.ip);
					if (ipCopy) {
						CBNetworkAddress * copy = CBNewNetworkAddress(addrs->addresses[x]->lastSeen, (CBSocketAddress){ipCopy, addrs->addresses[x]->sockAddr.port}, addrs->addresses[x]->services, true);
						CBReleaseObject(ipCopy);
						// See if the maximum number of addresses have been reached. If it has then remove an address before adding this one.
						if (self->addresses->addrNum > self->maxAddresses) {
							// This does not include peers, so there may actually be more stored addresses than the maxAddresses, as peers can be stored when public.
							CBNetworkAddress * addr = CBNetworkAddressManagerSelectAndRemoveAddress(self->addresses);
							// Delete from storage also.
							if (self->useAddrStorage && ! CBAddressStorageDeleteAddress(self->addrStorage, addr))
								CBLogError("Could not remove an address from storage.");
						}
						// Add the address to the address manager and the storage (If the storage object is not NULL).
						CBNetworkAddressManagerAddAddress(self->addresses, copy);
						if (self->useAddrStorage)
							CBAddressStorageSaveAddress(self->addrStorage, copy);
						CBReleaseObject(copy);
						didAdd = true;
					}else return CB_MESSAGE_ACTION_DISCONNECT;
				}
			}else{
				// We have an advertised peer. This means it is public and should return to the address store.
				peerB->addr->isPublic = true;
				// Add the address to storage
				if (self->useAddrStorage)
					CBAddressStorageSaveAddress(self->addrStorage, peerB->addr);
				// We do not want to relay this address
				removeAddr = true;
			}
			if (removeAddr) {
				CBReleaseObject(addrs->addresses[x]);
				addrs->addrNum--;
				if (addrs->addrNum > x)
					memmove(addrs->addresses + x, addrs->addresses + x + 1, (addrs->addrNum - x)*sizeof(CBNetworkAddress *));
			}
		}
		if (peer->allowRelay && addrs->addrNum < 10 && addrs->addrNum > 0) {
			// Unsolicited addresses. Send out addresses to two random peers.
			// Select and send to two peers
			uint32_t index = rand() % self->addresses->peersNum;
			uint32_t start = index;
			uint8_t peersToBeGiven = 2;
			for (;;) {
				// Get the peer object
				CBPeer * peerToRelay = CBNetworkAddressManagerGetPeer(self->addresses, index);
				CBReleaseObject(peerToRelay);
				// Check that the peer is not the peer that sent us the broadcast and the peer is not contained in the broadcast
				bool in = false;
				for (uint8_t x = 0; x < addrs->addrNum; x++)
					if (CBNetworkAddressIPPortCompare(NULL, addrs->addresses[x], peerToRelay->addr) == CB_COMPARE_EQUAL)
						in = true;
				if (peerToRelay != peer && ! in && peerToRelay->handshakeStatus == CB_HANDSHAKE_DONE) {
					// Relay the broadcast
					char addrStrs[CBNetworkAddressListStringMaxSize(addrs)];
					CBNetworkAddressListToString(addrs, addrStrs);
					CBLogVerbose("Relaying the following addresses to %s: %s", peerToRelay->peerStr, addrStrs);
					CBNetworkCommunicatorSendMessage(self, peerToRelay, CBGetMessage(addrs), NULL);
					if (--peersToBeGiven == 0)
						break;
				}
				// Move to the next peer if possible
				if (index == self->addresses->peersNum - 1)
					index = 0;
				else
					index++;
				if (index == start)
					break;
			}
		}
		if (didAdd)
			// We have new address information so try connecting to addresses.
			CBNetworkCommunicatorTryConnections(self, false);
		// Got addresses. Allow relays again.
		peer->allowRelay = true;
	}else if (peer->receive->type == CB_MESSAGE_TYPE_GETADDR) {
		// Give 33 peers with the highest times with a some randomisation added. Try connected peers first. Do not send empty addr.
		CBNetworkAddressList * addr = CBNewNetworkAddressList(self->version >= CB_ADDR_TIME_VERSION && peer->versionMessage->version >= CB_ADDR_TIME_VERSION);
		CBGetMessage(addr)->type = CB_MESSAGE_TYPE_ADDR;
		// Try connected peers. Only send peers that are public (private addresses are those which connect to us but haven't relayed their address).
		uint8_t peersSent = 0;
		for (uint32_t y = 0; peersSent < 28 && y < self->addresses->peersNum; y++) { // 28 to have room for 5 addresses.
			// Get the peer
			CBPeer * peerToInclude = CBNetworkAddressManagerGetPeer(self->addresses, y);
			if (peerToInclude != peer // Not the peer we are sending to
				&& peerToInclude->addr->isPublic // Public
				&& (peerToInclude->addr->type != CB_IP_LOCAL // OK if not local
					|| peerToInclude->addr->type == CB_IP_LOCAL)) { // Or if the peer we are sending to is local
				CBNetworkAddressListAddNetworkAddress(addr, peerToInclude->addr);
				peersSent++;
			}
			// Release peer
			CBReleaseObject(peerToInclude);
		}
		// Now add stored addresses
		uint8_t numAddrs = 33 - peersSent;
		CBNetworkAddress * addrs[numAddrs];
		numAddrs = CBNetworkAddressManagerGetAddresses(self->addresses, numAddrs, addrs);
		for (uint8_t x = 0; x < numAddrs; x++){
			if (addrs[x]->type != CB_IP_LOCAL
				|| peer->addr->type == CB_IP_LOCAL)
				CBNetworkAddressListAddNetworkAddress(addr, addrs[x]);
			CBReleaseObject(addrs[x]);
		}
		// Send address broadcast, if we have at least one.
		if (addr->addrNum){
			char addrStrs[CBNetworkAddressListStringMaxSize(addr)];
			CBNetworkAddressListToString(addr, addrStrs);
			CBLogVerbose("Giving the following addresses to %s: %s", peer->peerStr, addrStrs);
			CBNetworkCommunicatorSendMessage(self, peer, CBGetMessage(addr), NULL);
		}
		CBReleaseObject(addr);
	}
	// Use opportunity to discover if we should broadcast our own addresses for recieving incoming connections.
	if (// Only share address if we allow for incomming connections.
		self->maxIncommingConnections
		// Share when we are listening for incoming connections.
		&& (self->ipData[0].isListening || self->ipData[1].isListening || self->ipData[2].isListening || self->ipData[3].isListening)
		// Every 24 hours
		&& peer->time < time(NULL) - 86400) {
		peer->time = time(NULL);
		CBNetworkAddressList * addrs = CBNewNetworkAddressList(self->version >= CB_ADDR_TIME_VERSION && peer->versionMessage->version >= CB_ADDR_TIME_VERSION);
		CBGetMessage(addrs)->type = CB_MESSAGE_TYPE_ADDR;
		for (uint8_t x = 0; x < 4; x++) {
			if (self->ipData[x].isListening) {
				self->ipData[x].ourAddress->lastSeen = time(NULL);
				CBNetworkAddressListAddNetworkAddress(addrs, self->ipData[x].ourAddress);
			}
		}
		CBLogVerbose("Giving self to %s", peer->peerStr);
		CBNetworkCommunicatorSendMessage(self, peer, CBGetMessage(addrs), NULL);
		CBReleaseObject(addrs);
	}
	return CB_MESSAGE_ACTION_CONTINUE; // Do not disconnect.
}
CBOnMessageReceivedAction CBNetworkCommunicatorProcessMessageAutoHandshake(CBNetworkCommunicator * self, CBPeer * peer){
	if (peer->receive->type == CB_MESSAGE_TYPE_VERSION) {
		// Node sent us their version. How very nice of them.
		// Check version and nonce.
		if (CBGetVersion(peer->receive)->version < CB_MIN_PROTO_VERSION
			|| (CBGetVersion(peer->receive)->nonce == self->nonce && self->nonce > 0))
			// Disconnect peer
			return CB_MESSAGE_ACTION_DISCONNECT;
		else{ // Version OK
			// Check if we have a connection to this peer already and that it is a seperate connection
			CBPeer * peerCheck = CBNetworkAddressManagerGotPeer(self->addresses, CBGetVersion(peer->receive)->addSource);
			if (peerCheck && peerCheck != peer){
				// Release the peer check
				CBReleaseObject(peerCheck);
				// Sometimes two peers may try to connect to each other at the same time. Then both peers send a version message at the same time. They will both come to this part of the code. We don't want both peers to disconnect the other in this case. We want to keep one connection between the peers going, so we use a deterministic method for both peers to only disconnect one, by comparing the addresses.
				CBNetworkAddress * ours = CBNetworkCommunicatorGetOurMainAddress(self, CBGetVersion(peer->receive)->addSource->type);
				CBCompare res = CBNetworkAddressIPPortCompare(NULL, CBGetVersion(peer->receive)->addSource, ours);
				if (res == CB_COMPARE_MORE_THAN)
					// Disconnect this connection.
					return CB_MESSAGE_ACTION_DISCONNECT;
				else
					// Disconnect the other connection.
					CBNetworkCommunicatorDisconnect(self, peerCheck, 0, false);
			}
			// Remove the source address from the address manager if it has it. Now that the peer is connected, we identify it by the source address and do not want to try connections to the same address.
			CBNetworkAddressManagerRemoveAddress(self->addresses, CBGetNetworkAddress(CBGetVersion(peer->receive)->addSource));
			// Save version message
			peer->versionMessage = CBGetVersion(peer->receive);
			CBRetainObject(peer->versionMessage);
			// Change version 10300 to 300
			if (peer->versionMessage->version == 10300)
				peer->versionMessage->version = 300;
			peer->addr->services = peer->versionMessage->services;
			// If we are determining our IPs then record IP. If the ip was the null ip then it means the ip is not publically accessible
			if (memcmp(CBByteArrayGetData(peer->versionMessage->addRecv->sockAddr.ip), CB_NULL_ADDRESS, 16) != 0) {
				if (peer->versionMessage->addRecv->type == CB_IP_IP4) {
					if (self->flags & CB_NETWORK_COMMUNICATOR_DETERMINE_IP4)
						CBNetworkCommunicatorDetermineIP(self, peer->versionMessage->addRecv, true);
				}else if (peer->versionMessage->addRecv->type == CB_IP_IP6){
					if (self->flags & CB_NETWORK_COMMUNICATOR_DETERMINE_IP6)
						CBNetworkCommunicatorDetermineIP(self, peer->versionMessage->addRecv, false);
				}
			}
			// Send version next if we have not already.
			if (! (peer->handshakeStatus & CB_HANDSHAKE_SENT_VERSION)) {
				CBVersion * version = CBNetworkCommunicatorGetVersion(self, peer->addr);
				if (version) {
					CBGetMessage(version)->expectResponse = CB_MESSAGE_TYPE_VERACK; // Expect a response for this message.
					CBGetMessage(version)->type = CB_MESSAGE_TYPE_VERSION;
					bool ok = CBNetworkCommunicatorSendMessage(self, peer, CBGetMessage(version), NULL);
					CBReleaseObject(version);
					if (! ok)
						return CB_MESSAGE_ACTION_DISCONNECT;
				}else
					return CB_MESSAGE_ACTION_DISCONNECT;
			}
			// Send our acknowledgement
			CBMessage * ack = CBNewMessageByObject();
			ack->type = CB_MESSAGE_TYPE_VERACK;
			bool ok = CBNetworkCommunicatorSendMessage(self, peer, ack, NULL);
			CBReleaseObject(ack);
			if (! ok)
				return CB_MESSAGE_ACTION_DISCONNECT; // If we cannot send an acknowledgement, exit.
			// If the addSource is ::ffff:0.0.0.0 the peer does not know its address.
			if (memcmp(CBByteArrayGetData(peer->versionMessage->addSource->sockAddr.ip), CB_NULL_ADDRESS, 16) != 0) {
				// See if the network address changed.
				bool portChange = peer->addr->sockAddr.port != peer->versionMessage->addSource->sockAddr.port;
				bool ipChange = CBByteArrayCompare(peer->addr->sockAddr.ip, peer->versionMessage->addSource->sockAddr.ip) != CB_COMPARE_EQUAL;
				if (portChange || ipChange) {
					// Remove the peer from the arrays so that it is repositioned with the updated data
					CBRetainObject(peer);
					CBMutexLock(self->peersMutex);
					CBNetworkAddressManagerRemovePeer(self->addresses, peer);
					CBMutexUnlock(self->peersMutex);
					// Change port and IP number to the port and IP the peer wants us to recognise them with.
					peer->addr->sockAddr.port = peer->versionMessage->addSource->sockAddr.port;
					if (ipChange) {
						// The IP is different so change it.
						CBReleaseObject(peer->addr->sockAddr.ip);
						peer->addr->sockAddr.ip = peer->versionMessage->addSource->sockAddr.ip;
						CBRetainObject(peer->addr->sockAddr.ip);
					}
					// Now re-insert the peer with the new information
					CBMutexLock(self->peersMutex);
					CBNetworkAddressManagerTakePeer(self->addresses, peer);
					CBMutexUnlock(self->peersMutex);
				}
			}
			// Set the IP type
			peer->addr->type = CBGetIPType(CBByteArrayGetData(peer->addr->sockAddr.ip));
			// Set the peer address string
			CBNetworkAddressToString(peer->addr, peer->peerStr);
			// Adjust network time
			peer->timeOffset = peer->versionMessage->time - time(NULL); // Set the time offset for this peer.
			// Now we have the network time, add the peer for the time offset. Ignore on failure
			CBNetworkAddressManagerTakePeerTimeOffset(self->addresses, peer);
			// We received the version
			peer->handshakeStatus |= CB_HANDSHAKE_GOT_VERSION;
			// Log version
			char versionStr[CBVersionStringMaxSize(peer->versionMessage)];
			CBVersionToString(peer->versionMessage, versionStr);
			CBLogVerbose("%s sent us their version:\n%s", peer->peerStr, versionStr);
		}
	}else if (peer->receive->type == CB_MESSAGE_TYPE_VERACK)
		// Got the verack
		peer->handshakeStatus |= CB_HANDSHAKE_GOT_ACK;
	if (self->flags & CB_NETWORK_COMMUNICATOR_AUTO_DISCOVERY
		// Request addresses when we have done the handshake or we have just sent the ack to finish the handshake
		&& peer->handshakeStatus & CB_HANDSHAKE_SENT_VERSION
		&& peer->handshakeStatus & CB_HANDSHAKE_GOT_ACK
		&& peer->handshakeStatus & CB_HANDSHAKE_GOT_VERSION) {
		// Request addresses
		CBMessage * getaddr = CBNewMessageByObject();
		getaddr->type = CB_MESSAGE_TYPE_GETADDR;
		// Send the message without an expected response. We do not expect an "addr" message because the peer may not send us anything if they do not have addresses to give.
		bool ok = CBNetworkCommunicatorSendMessage(self, peer, getaddr, NULL);
		CBReleaseObject(getaddr);
		if (! ok)
			return CB_MESSAGE_ACTION_DISCONNECT;
		peer->allowRelay = false;
	}
	return CB_MESSAGE_ACTION_CONTINUE;
}
CBOnMessageReceivedAction CBNetworkCommunicatorProcessMessageAutoPingPong(CBNetworkCommunicator * self, CBPeer * peer){
	if (peer->receive->type == CB_MESSAGE_TYPE_PING
		&& peer->versionMessage->version >= CB_PONG_VERSION
		&& self->version >= CB_PONG_VERSION){
		peer->receive->type = CB_MESSAGE_TYPE_PONG;
		if (! CBNetworkCommunicatorSendMessage(self, peer, peer->receive, NULL))
			return CB_MESSAGE_ACTION_DISCONNECT;
	}else if (peer->receive->type == CB_MESSAGE_TYPE_PONG){
		if (self->version < CB_PONG_VERSION || peer->versionMessage->version < CB_PONG_VERSION)
			return CB_MESSAGE_ACTION_DISCONNECT; // This peer should not be sending pong messages.
	}
	return CB_MESSAGE_ACTION_CONTINUE;
}
void CBNetworkCommunicatorRetryConnections(CBNetworkCommunicator * self){
	// Wait 20 Seconds before trying connections.
	if (!self->tryConnectionTimerStarted) {
		self->tryConnectionTimerStarted = true;
		CBStartTimer(self->eventLoop, &self->retryConnectionsTimer, 20000, CBNetworkCommunicatorRetryConnectionsProcess, self);
	}
}
void CBNetworkCommunicatorRetryConnectionsProcess(void * vself){
	CBNetworkCommunicator * self = vself;
	// Stop timer
	CBEndTimer(self->retryConnectionsTimer);
	// Look-up DNS again.
	CBNetworkCommunicatorTryConnections(self, true);
	self->tryConnectionTimerStarted = false;
}
bool CBNetworkCommunicatorSendMessage(CBNetworkCommunicator * self, CBPeer * peer, CBMessage * message, void (*callback)(void *, void *)){
	if (peer->sendQueueSize == CB_SEND_QUEUE_MAX_SIZE || !peer->connectionWorking)
		return false;
	char typeStr[CB_MESSAGE_TYPE_STR_SIZE];
	CBMessageTypeToString(message->type, typeStr);
	CBLogVerbose("Sending message of type %s (%u) to %s.", typeStr, message->type, peer->peerStr);
	// Serialise message if needed.
	if (! message->serialised) {
		uint32_t len;
		switch (message->type) {
			case CB_MESSAGE_TYPE_VERSION:
				len = CBVersionCalculateLength(CBGetVersion(message));
				if (len == 0)
					return false;
				message->bytes = CBNewByteArrayOfSize(len);
				len = CBVersionSerialise(CBGetVersion(message), false);
				break;
			case CB_MESSAGE_TYPE_ADDR:
				// "CBNetworkAddressCalculateLength" cannot fail.
				message->bytes = CBNewByteArrayOfSize(CBNetworkAddressListCalculateLength(CBGetNetworkAddressList(message)));
				len = CBNetworkAddressListSerialise(CBGetNetworkAddressList(message), false);
				break;
			case CB_MESSAGE_TYPE_INV:
			case CB_MESSAGE_TYPE_GETDATA:
				// "CBInventoryCalculateLength" cannot fail.
				message->bytes = CBNewByteArrayOfSize(CBInventoryCalculateLength(CBGetInventory(message)));
				len = CBInventorySerialise(CBGetInventory(message), false);
				break;
			case CB_MESSAGE_TYPE_GETBLOCKS:
			case CB_MESSAGE_TYPE_GETHEADERS:
				// "CBGetBlocksCalculateLength" cannot fail.
				message->bytes = CBNewByteArrayOfSize(CBGetBlocksCalculateLength(CBGetGetBlocks(message)));
				len = CBGetBlocksSerialise(CBGetGetBlocks(message), false);
				break;
			case CB_MESSAGE_TYPE_TX:
				len = CBTransactionCalculateLength(CBGetTransaction(message));
				if (len == 0)
					return false;
				message->bytes = CBNewByteArrayOfSize(len);
				len = CBTransactionSerialise(CBGetTransaction(message), false);
				break;
			case CB_MESSAGE_TYPE_BLOCK:
				len = CBBlockCalculateLength(CBGetBlock(message), true);
				if (len == 0)
					return false;
				message->bytes = CBNewByteArrayOfSize(len);
				len = CBBlockSerialise(CBGetBlock(message), true, false); // true -> Including transactions.
				break;
			case CB_MESSAGE_TYPE_HEADERS:
				// "CBBlockHeadersCalculateLength" cannot fail.
				message->bytes = CBNewByteArrayOfSize(CBBlockHeadersCalculateLength(CBGetBlockHeaders(message)));
				len = CBBlockHeadersSerialise(CBGetBlockHeaders(message), false);
				break;
			case CB_MESSAGE_TYPE_PING:
				if (peer->versionMessage->version >= 60000 && self->version >= 60000){
					message->bytes = CBNewByteArrayOfSize(8);
					len = CBPingPongSerialise(CBGetPingPong(message));
				}
			case CB_MESSAGE_TYPE_PONG:
				message->bytes = CBNewByteArrayOfSize(8);
				len = CBPingPongSerialise(CBGetPingPong(message));
				break;
			case CB_MESSAGE_TYPE_ALERT:
				// This should have been serialised before!
				return false;
				break;
			default:
				break;
		}
		if (message->bytes) {
			if (message->bytes->length != len)
				return false;
		}
	}
	if (message->bytes) {
		// Make checksum
		uint8_t hash[32];
		uint8_t hash2[32];
		CBSha256(CBByteArrayGetData(message->bytes), message->bytes->length, hash);
		CBSha256(hash, 32, hash2);
		memcpy(message->checksum, hash2, 4);
	}else{
		// Empty bytes checksum
		message->checksum[0] = 0x5D;
		message->checksum[1] = 0xF6;
		message->checksum[2] = 0xE0;
		message->checksum[3] = 0xE2;
	}
	// Add the message and callback to the send queue
	uint8_t sendQueueIndex = (peer->sendQueueFront + peer->sendQueueSize) % CB_SEND_QUEUE_MAX_SIZE;
	peer->sendQueue[sendQueueIndex].message = message;
	peer->sendQueue[sendQueueIndex].callback = callback;
	if (peer->sendQueueSize == 0
		&& !CBSocketAddEvent(peer->sendEvent, self->sendTimeOut))
		return false;
	peer->sendQueueSize++;
	CBRetainObject(message);
	return true;
}
void CBNetworkCommunicatorSendPings(void * vself){
	CBNetworkCommunicator * self = vself;
	CBMessage * ping = CBNewMessageByObject();
	ping->type = CB_MESSAGE_TYPE_PING;
	if (self->version >= CB_PONG_VERSION) {
		CBPingPong * pingPong = CBNewPingPong(rand());
		CBGetMessage(pingPong)->type = CB_MESSAGE_TYPE_PING;
		CBAssociativeArrayForEach(CBPeer * peer, &self->addresses->peers)
			// Only send pings after handshake
			if (peer->handshakeStatus == CB_HANDSHAKE_DONE){
				if (peer->versionMessage->version >= CB_PONG_VERSION){
					CBGetMessage(pingPong)->expectResponse = CB_MESSAGE_TYPE_PONG; // Expect a pong.
					CBNetworkCommunicatorSendMessage(self, peer, CBGetMessage(pingPong), NULL);
				}else
					CBNetworkCommunicatorSendMessage(self, peer, CBGetMessage(ping), NULL);
			}
		CBReleaseObject(pingPong);
	}else CBAssociativeArrayForEach(CBPeer * peer, &self->addresses->peers)
		// Only send ping after handshake
		if (peer->handshakeStatus == CB_HANDSHAKE_DONE)
			CBNetworkCommunicatorSendMessage(self, peer, CBGetMessage(ping), NULL);
	CBReleaseObject(ping);
}
void CBNetworkCommunicatorSetNetworkAddressManager(CBNetworkCommunicator * self, CBNetworkAddressManager * addrMan){
	CBRetainObject(addrMan);
	self->addresses = addrMan;
}
void CBNetworkCommunicatorSetAlternativeMessages(CBNetworkCommunicator * self, CBByteArray * altMessages, uint32_t * altMaxSizes){
	if (altMessages) CBRetainObject(altMessages);
	self->alternativeMessages = altMessages;
	self->altMaxSizes = altMaxSizes;
}
void CBNetworkCommunicatorSetOurIPv4(CBNetworkCommunicator * self, CBNetworkAddress * ourIPv4){
	CBReleaseObject(self->ipData[CB_IP4_NETWORK].ourAddress);
	CBRetainObject(ourIPv4);
	self->ipData[CB_IP4_NETWORK].ourAddress = ourIPv4;
	self->ipData[CB_IP4_NETWORK].isSet = true;
	self->ipData[CB_IP4_NETWORK].listeningPort = ourIPv4->sockAddr.port;
}
void CBNetworkCommunicatorSetOurIPv6(CBNetworkCommunicator * self, CBNetworkAddress * ourIPv6){
	CBReleaseObject(self->ipData[CB_IP6_NETWORK].ourAddress);
	CBRetainObject(ourIPv6);
	self->ipData[CB_IP6_NETWORK].ourAddress = ourIPv6;
	self->ipData[CB_IP6_NETWORK].isSet = true;
	self->ipData[CB_IP6_NETWORK].listeningPort = ourIPv6->sockAddr.port;
}
void CBNetworkCommunicatorSetReachability(CBNetworkCommunicator * self, CBIPType type, bool reachable){
	if (reachable)
		self->reachability |= type;
	else
		self->reachability &= ~type;
}
void CBNetworkCommunicatorSetUserAgent(CBNetworkCommunicator * self, CBByteArray * userAgent){
	CBRetainObject(userAgent);
	self->userAgent = userAgent;
}
void CBNetworkCommunicatorStartListening(CBNetworkCommunicator * self){
	CBIPType ipTypes[4] = {CB_IP_IP4, CB_IP_IP6, CB_IP_TOR, CB_IP_I2P};
	char * networkStrs[4] = {"IPv4","IPv6","Tor","I2P"};
	for (uint8_t x = 0; x < 4; x++) {
		if (CBNetworkCommunicatorIsReachable(self, ipTypes[x])) {
			// Create new bound IPv4 socket
			if (CBSocketBind(&self->ipData[x].listeningSocket, (x == CB_IP4_NETWORK) ? false :  true, self->ipData[x].listeningPort)){
				// Add event for accepting connection for both sockets
				if (CBSocketCanAcceptEvent(&self->ipData[x].acceptEvent, self->eventLoop, self->ipData[x].listeningSocket, CBNetworkCommunicatorAcceptConnection)) {
					if(CBSocketAddEvent(self->ipData[x].acceptEvent, 0)){ // No timeout for listening for incomming connections.
						// Start listening
						if (CBSocketListen(self->ipData[x].listeningSocket, self->maxIncommingConnections)){
							// Is listening
							self->ipData[x].isListening = true;
							CBLogVerbose("Now listening on the %s network on port %u.", networkStrs[x], self->ipData[x].listeningPort);
							continue;
						}else
							CBLogError("Unable to start listening on the %s network on port %u.", networkStrs[x], self->ipData[x].listeningPort);
					}else
						CBLogError("Unable to add an accept event for the %s network on port %u.", networkStrs[x], self->ipData[x].listeningPort);
					// Failure to add event
					CBSocketFreeEvent(self->ipData[x].acceptEvent);
				}else
					CBLogError("Unable to create an accept event for the %s network on port %u.", networkStrs[x], self->ipData[x].listeningPort);
			}else
				CBLogError("Unable to bind a socket for listening on the %s network on port %u.", networkStrs[x], self->ipData[x].listeningPort);
			// Failure to create event
			CBCloseSocket(self->ipData[x].listeningSocket);
		}
	}
}
void CBNetworkCommunicatorStartPings(CBNetworkCommunicator * self){
	self->isPinging = true;
	CBStartTimer(self->eventLoop, &self->pingTimer, self->heartBeat, CBNetworkCommunicatorSendPings, self);
}
void CBNetworkCommunicatorStop(CBNetworkCommunicator * self){
	if (self->ipData[0].isListening || self->ipData[1].isListening || self->ipData[2].isListening || self->ipData[3].isListening)
		CBNetworkCommunicatorStopListening(self);
	// Disconnect all the peers
	CBAssociativeArrayForEach(CBPeer * peer, &self->addresses->peers)
		CBNetworkCommunicatorDisconnect(self, peer, 0, true); // "true" we are stopping.
	// Now reset the peers arrays. The addresses were released in CBNetworkCommunicatorDisconnect, so this function only clears the array nodes.
	CBNetworkAddressManagerClearPeers(self->addresses);
}
void CBNetworkCommunicatorStopListening(CBNetworkCommunicator * self){
	for (uint8_t x = 0; x < 4; x++) {
		if (self->ipData[x].isListening) {
			CBSocketFreeEvent(self->ipData[x].acceptEvent);
			CBCloseSocket(self->ipData[x].listeningSocket);
			self->ipData[x].isListening = false;
		}
	}
}
void CBNetworkCommunicatorStopPings(CBNetworkCommunicator * self){
	if (self->isPinging){
		CBEndTimer(self->pingTimer);
		self->isPinging = false;
	}
}
void CBNetworkCommunicatorTryConnections(CBNetworkCommunicator * self, bool dns){
	if (self->attemptingOrWorkingConnections >= self->maxConnections
		|| self->flags & CB_NETWORK_COMMUNICATOR_INCOMING_ONLY)
		return; // Cannot connect to any more peers
	if (dns && self->flags & CB_NETWORK_COMMUNICATOR_BOOTSTRAP)
		// Get DNS nodes
		CBForEach(char * domain, CB_SEED_DOMAINS)
			if (!CBNetworkCommunicatorLoadDNS(self, domain))
				CBLogWarning("Unable to get address information from %s", domain);
	uint16_t connectTo = self->maxConnections - self->attemptingOrWorkingConnections;
	uint32_t addrNum = self->addresses->addrNum;
	if (addrNum < connectTo)
		connectTo = addrNum; // Cannot connect to any more than the address we have
	CBNetworkAddress ** addrs = malloc(sizeof(*addrs) * connectTo);
	connectTo = CBNetworkAddressManagerGetAddresses(self->addresses, connectTo, addrs);
	for (uint64_t x = 0; x < connectTo; x++) {
		// Remove the address from the address manager
		CBNetworkAddressManagerRemoveAddress(self->addresses, addrs[x]);
		// We haven't got the address as a peer.
		// Convert network address into peer
		CBPeer * peer = CBNewPeer(addrs[x]);
		CBReleaseObject(addrs[x]);
		char addrStr[CB_NETWORK_ADDR_STR_SIZE];
		CBNetworkAddressToString(peer->addr, addrStr);
		strcpy(peer->peerStr, addrStr);
		peer->incomming = false;
		CBConnectReturn res = CBNetworkCommunicatorConnect(self, peer);
		if (res == CB_CONNECT_ERROR || res == CB_CONNECT_FAILED) {
			// Add address back to manager
			if (res == CB_CONNECT_FAILED)
				// Add penalty if failed
				peer->addr->lastSeen -= 3600;
			// Re-insert into the address manager, since we could not connect this time. If it fails, ignore and the address will not be added.
			CBNetworkAddressManagerAddAddress(self->addresses, peer->addr);
			CBReleaseObject(peer);
			CBLogWarning("Unable to connect to the address: %s", addrStr);
		}else if (res == CB_CONNECT_OK) {
			CBLogVerbose("Made a connection to peer %s", addrStr);
		}
		// Either the connection was OK and it should either timeout or finalise, or we forget about it because it is not supported.
	}
	if (self->attemptingOrWorkingConnections == 0)
		CBNetworkCommunicatorNoPeers(self);
	// Free address pointer memory.
	free(addrs);
}
