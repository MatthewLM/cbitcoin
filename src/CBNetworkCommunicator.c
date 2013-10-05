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

//  Constructor

CBNetworkCommunicator * CBNewNetworkCommunicator(CBNetworkCommunicatorCallbacks callbacks){
	CBNetworkCommunicator * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeNetworkCommunicator;
	CBInitNetworkCommunicator(self, callbacks);
	return self;
}

//  Initialiser

void CBInitNetworkCommunicator(CBNetworkCommunicator * self, CBNetworkCommunicatorCallbacks callbacks){
	CBInitObject(CBGetObject(self));
	// Initialise sentAddrs array
	CBInitAssociativeArray(&self->relayedAddrs, CBNetworkAddressIPPortCompare, NULL, CBReleaseObject);
	CBNewMutex(&self->peersMutex);
	CBNewMutex(&self->sendMessageMutex);
	// Set fields.
	self->callbacks = callbacks;
	self->relayedAddrsLastClear = time(NULL);
	self->attemptingOrWorkingConnections = 0;
	self->numIncommingConnections = 0;
	self->isListeningIPv4 = false;
	self->isListeningIPv6 = false;
	self->ourIPv4 = NULL;
	self->ourIPv6 = NULL;
	self->isPinging = false;
	self->nonce = 0;
	self->stoppedListening = false;
	self->useAddrStorage = false;
	self->reachability = 0;
	self->alternativeMessages = NULL;
	self->pendingIP = 0;
	self->alternativeMessages = NULL;
	self->altMaxSizes = NULL;
	// Default settings
	self->maxAddresses = 1000000;
	self->maxConnections = 8;
	self->maxIncommingConnections = 4;
	self->recvTimeOut = 100;
	self->responseTimeOut = 5000;
	self->sendTimeOut = 1000;
	self->timeOut = 5400000;
	self->heartBeat = 1800000;
	self->flags = 0;
	self->connectionTimeOut = 5000;
	self->services = 0;
}

//  Destructor

void CBDestroyNetworkCommunicator(void * vself){
	CBNetworkCommunicator * self = vself;
	if (self->isStarted)
		CBNetworkCommunicatorStop(self);
	if (self->alternativeMessages) CBReleaseObject(self->alternativeMessages);
	CBReleaseObject(self->addresses);
	if (self->ourIPv4) CBReleaseObject(self->ourIPv4);
	if (self->ourIPv6) CBReleaseObject(self->ourIPv6);
	free(self->altMaxSizes);
	CBFreeAssociativeArray(&self->relayedAddrs);
	CBFreeMutex(self->peersMutex);
	CBFreeMutex(self->sendMessageMutex);
}
void CBFreeNetworkCommunicator(void * self){
	CBDestroyNetworkCommunicator(self);
	free(self);
}

//  Functions

void CBNetworkCommunicatorAcceptConnection(void * vself, CBDepObject socket){
	CBNetworkCommunicator * self = vself;
	CBDepObject connectSocketID;
	if (! CBSocketAccept(socket, &connectSocketID))
		return;
	// Connected, add CBNetworkAddress. Use a special placeholder ip identifier for the address, so that it is unique.
	CBByteArray * ip = CBNewByteArrayOfSize(16);
	CBByteArraySetInt64(ip, 0, self->pendingIP);
	CBByteArraySetInt64(ip, 8, 0);
	CBPeer * peer = CBNewPeerByTakingNetworkAddress(CBNewNetworkAddress(0, ip, 0, 0, false));
	CBReleaseObject(ip);
	// Increment the pendingIP for the next incomming connection
	self->pendingIP++;
	peer->incomming = true;
	peer->socketID = connectSocketID;
	// Set up receive event
	if (CBSocketCanReceiveEvent(&peer->receiveEvent, self->eventLoop, peer->socketID, CBNetworkCommunicatorOnCanReceive, peer)) {
		// The event works
		if(CBSocketAddEvent(peer->receiveEvent, self->responseTimeOut)){ // Begin receive event.
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
				return;
			}
		}
		// Could create receive event but there was a failure afterwards so free it.
		CBSocketFreeEvent(peer->receiveEvent);
	}
	// Failure, release peer.
	CBCloseSocket(connectSocketID);
	CBReleaseObject(peer);
}
CBConnectReturn CBNetworkCommunicatorConnect(CBNetworkCommunicator * self, CBPeer * peer){
	if (! CBNetworkCommunicatorIsReachable(self, CBGetNetworkAddress(peer)->type))
		return CB_CONNECT_NO_SUPPORT;
	bool isIPv6 = CBGetNetworkAddress(peer)->type & CB_IP_IPv6;
	// Make socket
	CBSocketReturn res = CBNewSocket(&peer->socketID, isIPv6);
	if(res == CB_SOCKET_NO_SUPPORT){
		if (isIPv6) CBNetworkCommunicatorSetReachability(self, CB_IP_IPv6, false);
		else CBNetworkCommunicatorSetReachability(self, CB_IP_IPv4, false);
		if (! CBNetworkCommunicatorIsReachable(self, CB_IP_IPv6 | CB_IP_IPv4))
			// IPv6 and IPv4 not reachable.
			self->callbacks.onNetworkError(self);
		return CB_CONNECT_NO_SUPPORT;
	}else if (res == CB_SOCKET_BAD) {
		if (self->addresses->peersNum == 0)
			self->callbacks.onNetworkError(self);
		return CB_CONNECT_ERROR;
	}
	// Connect
	if (CBSocketConnect(peer->socketID, CBByteArrayGetData(CBGetNetworkAddress(peer)->ip), isIPv6, CBGetNetworkAddress(peer)->port)){
		// Add event for connection
		if (CBSocketDidConnectEvent(&peer->connectEvent, self->eventLoop, peer->socketID, CBNetworkCommunicatorDidConnect, peer)) {
			if (CBSocketAddEvent(peer->connectEvent, self->connectionTimeOut)) {
				// Connection is fine. Retain for waiting for connect.
				CBRetainObject(peer);
				self->attemptingOrWorkingConnections++;
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
void CBNetworkCommunicatorDidConnect(void * vself, void * vpeer){
	CBNetworkCommunicator * self = vself;
	CBPeer * peer = vpeer;
	CBSocketFreeEvent(peer->connectEvent); // No longer need this event.
	// Check to see if in the meantime, that we have not been connected to by the peer. Double connections are bad m'kay.
	if (! CBNetworkAddressManagerGotPeer(self->addresses, CBGetNetworkAddress(peer))){
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
						CBVersion * version = CBNetworkCommunicatorGetVersion(self, CBGetNetworkAddress(peer));
						CBGetMessage(version)->expectResponse = CB_MESSAGE_TYPE_VERACK; // Expect a response for this message.
						CBGetMessage(version)->type = CB_MESSAGE_TYPE_VERSION;
						CBNetworkCommunicatorSendMessage(self, peer, CBGetMessage(version), NULL);
						CBReleaseObject(version);
					}
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
		CBNetworkAddress * addr = realloc(peer, sizeof(CBNetworkAddress));
		if (addr)
			CBNetworkAddressManagerTakeAddress(self->addresses, addr);
	}else
		CBCloseSocket(peer->socketID);
	self->attemptingOrWorkingConnections--;
	if (self->attemptingOrWorkingConnections == 0)
		self->callbacks.onNetworkError(self);
}
void CBNetworkCommunicatorDisconnect(CBNetworkCommunicator * self, CBPeer * peer, uint32_t penalty, bool stopping){
	self->callbacks.onPeerDisconnection(self, peer);
	if (peer->requestedData) CBReleaseObject(peer->requestedData);
	CBSocketFreeEvent(peer->receiveEvent);
	CBSocketFreeEvent(peer->sendEvent);
	// Close the socket
	CBCloseSocket(peer->socketID);
	// Release the receiving message object if it exists.
	if (peer->receive) CBReleaseObject(peer->receive);
	// Release all messages in the send queue
	for (uint8_t x = 0; x < peer->sendQueueSize; x++)
		CBReleaseObject(peer->sendQueue[(peer->sendQueueFront + x) % CB_SEND_QUEUE_MAX_SIZE].message);
	// If incomming, lower the incomming connections number
	if (peer->incomming)
		self->numIncommingConnections--;
	if (self->stoppedListening && self->numIncommingConnections < self->maxIncommingConnections)
		// Start listening again
		CBNetworkCommunicatorStartListening(self);
	// Lower the attempting or working connections number
	self->attemptingOrWorkingConnections--;
	// If this is a working connection, remove from the address manager peer's list.
	if (peer->connectionWorking){
		// If not stopping we can remove the peer.
		if (! stopping){
			// Retain the peer in case we continue to need it
			CBRetainObject(peer);
			// We aren't stopping so we should remove the node from the array.
			CBMutexLock(self->peersMutex);
			CBNetworkAddressManagerRemovePeer(self->addresses, peer);
			CBMutexUnlock(self->peersMutex);
		}
		if (CBGetNetworkAddress(peer)->isPublic) {
			// Public peer, return to addresses list.
			// Apply the penalty given
			CBGetNetworkAddress(peer)->penalty += penalty;
			CBNetworkAddress * temp = realloc(peer, sizeof(CBNetworkAddress));
			if (temp)
				// Got reallocated memory. 
				peer = CBGetPeer(temp);
			// Adding the address may fail. If it does, we just ignore it and lose the address.
			CBNetworkAddressManagerAddAddress(self->addresses, CBGetNetworkAddress(peer));
		}
		// Release the peer from out control if we are not stopping, or release the peer from the peers array if we are stopping
		CBReleaseObject(peer);
	}else{
		// Else we release the object from control of the CBNetworkCommunicator
		CBSocketFreeEvent(peer->connectEvent);
		CBReleaseObject(peer);
	}
	if (self->addresses->peersNum == 0 && self->flags & CB_NETWORK_COMMUNICATOR_AUTO_PING)
		// No more peers so stop pings
		CBNetworkCommunicatorStopPings(self);
	if (self->attemptingOrWorkingConnections == 0 && ! stopping)
		// No more connections so give a network error
		self->callbacks.onNetworkError(self);
}
CBVersion * CBNetworkCommunicatorGetVersion(CBNetworkCommunicator * self, CBNetworkAddress * addRecv){
	CBNetworkAddress * sourceAddr;
	if (addRecv->type & CB_IP_IPv6
		&& self->ourIPv6)
		sourceAddr = self->ourIPv6;
	else
		sourceAddr = self->ourIPv4;
	self->nonce = rand();
	CBVersion * version = CBNewVersion(self->version, self->services, time(NULL), addRecv, sourceAddr, self->nonce, self->userAgent, self->blockHeight);
	return version;
}
bool CBNetworkCommunicatorIsReachable(CBNetworkCommunicator * self, CBIPType type){
	if (type == CB_IP_INVALID)
		return false;
	return self->reachability & type;
}
void CBNetworkCommunicatorOnCanReceive(void * vself, void * vpeer){
	CBNetworkCommunicator * self = vself;
	CBPeer * peer = vpeer;
	// Node kindly has some data available in the socket buffer.
	if (! peer->receive) {
		// New message to be received.
		peer->receive = CBNewMessageByObject();
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
				if (peer->messageReceived == peer->receive->bytes->length){
					// We now have the message.
					CBNetworkCommunicatorOnMessageReceived(self, peer);
				}
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
				// Sent the entire payload. Celebrate with some bonbonbonbons: http://www.youtube.com/watch?v=VWgwJfbeCeU
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
			if (peer->latencyTimerStart == 0){
				peer->latencyTimerStart = CBGetMilliseconds();
				peer->latencyExpected = toSend->expectResponse;
			}
		}
		// Remove message from queue.
		peer->sendQueueSize--;
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
	if (CBByteArrayReadInt32(header, 0) != self->networkID){
		// The network ID bytes is not what we are looking for. We will have to remove the peer.
		CBReleaseObject(header);
		CBNetworkCommunicatorDisconnect(self, peer, CB_24_HOURS, false);
	}
	CBMessageType type;
	uint32_t size = CBByteArrayReadInt32(header, 16);
	bool error = false;
	CBByteArray * typeBytes = CBNewByteArraySubReference(header, 4, 12);
	if (! memcmp(CBByteArrayGetData(typeBytes), "version\0\0\0\0\0", 12)) {
		// Version message
		// Check that we have not received their version yet.
		if (!(peer->handshakeStatus & CB_HANDSHAKE_GOT_VERSION)
			&& size <= CB_VERSION_MAX_SIZE)
			type = CB_MESSAGE_TYPE_VERSION;
		else error = true;
	}else if (! memcmp(CBByteArrayGetData(typeBytes), "verack\0\0\0\0\0\0", 12)){
		// Version acknowledgement message
		// Chek we have sent the version and not received a verack already.
		if (peer->handshakeStatus & CB_HANDSHAKE_SENT_VERSION
			&& !(peer->handshakeStatus & CB_HANDSHAKE_GOT_ACK)
			// Must contain zero payload.
			&& size == 0)
			type = CB_MESSAGE_TYPE_VERACK;
		else error = true;
	}else if (!(peer->handshakeStatus & CB_HANDSHAKE_GOT_VERSION))
		// We have not yet received the version message but we have not received a version or verack message.
		error = true;
	else{
		if (! memcmp(CBByteArrayGetData(typeBytes), "addr\0\0\0\0\0\0\0\0", 12)){
			// Address broadcast message
			if (size > CB_NETWORK_ADDRESS_LIST_MAX_SIZE)
				error = true;
			else
				type = CB_MESSAGE_TYPE_ADDR;
		}else if (! memcmp(CBByteArrayGetData(typeBytes), "inv\0\0\0\0\0\0\0\0\0", 12)){
			// Inventory broadcast message
			if (size > CB_INVENTORY_BROADCAST_MAX_SIZE)
				error = true;
			else
				type = CB_MESSAGE_TYPE_INV;
		}else if (! memcmp(CBByteArrayGetData(typeBytes), "getdata\0\0\0\0\0", 12)){
			// Get data message
			if (size > CB_GET_DATA_MAX_SIZE)
				error = true;
			else
				type = CB_MESSAGE_TYPE_GETDATA;
		}else if (! memcmp(CBByteArrayGetData(typeBytes), "getblocks\0\0\0", 12)){
			// Get blocks message
			if (size > CB_GET_BLOCKS_MAX_SIZE)
				error = true;
			else
				type = CB_MESSAGE_TYPE_GETBLOCKS;
		}else if (! memcmp(CBByteArrayGetData(typeBytes), "getheaders\0\0", 12)){
			// Get headers message
			if (size > CB_GET_HEADERS_MAX_SIZE)
				error = true;
			else
				type = CB_MESSAGE_TYPE_GETHEADERS;
		}else if (! memcmp(CBByteArrayGetData(typeBytes), "tx\0\0\0\0\0\0\0\0\0\0", 12)){
			// Transaction message
			if (size > CB_TX_MAX_SIZE)
				error = true;
			else
				type = CB_MESSAGE_TYPE_TX;
		}else if (! memcmp(CBByteArrayGetData(typeBytes), "block\0\0\0\0\0\0\0", 12)){
			// Block message
			if (size > CB_BLOCK_MAX_SIZE + 89) // Plus 89 for header.
				error = true;
			else
				type = CB_MESSAGE_TYPE_BLOCK;
		}else if (! memcmp(CBByteArrayGetData(typeBytes), "headers\0\0\0\0\0", 12)){
			// Block headers message
			if (size > CB_BLOCK_HEADERS_MAX_SIZE)
				error = true;
			else
				type = CB_MESSAGE_TYPE_HEADERS;
		}else if (! memcmp(CBByteArrayGetData(typeBytes), "getaddr\0\0\0\0\0", 12)){
			// Get Addresses message
			if (size) // Should be empty
				error = true;
			else
				type = CB_MESSAGE_TYPE_GETADDR;
		}else if (! memcmp(CBByteArrayGetData(typeBytes), "ping\0\0\0\0\0\0\0\0", 12)){
			// Ping message
			// Should be empty before version 60000 or 8 bytes.
			if (size > 8 || ((peer->versionMessage->version < CB_PONG_VERSION || self->version < CB_PONG_VERSION) && size)) 
				error = true;
			else
				type = CB_MESSAGE_TYPE_PING;
		}else if (! memcmp(CBByteArrayGetData(typeBytes), "pong\0\0\0\0\0\0\0\0", 12)){
			// Pong message
			if (size > 8)
				error = true;
			else
				type = CB_MESSAGE_TYPE_PONG;
		}else if (! memcmp(CBByteArrayGetData(typeBytes), "alert\0\0\0\0\0\0\0", 12)){
			// Alert message
			if (size > CB_ALERT_MAX_SIZE)
				error = true;
			else
				type = CB_MESSAGE_TYPE_ALERT;
		}else{
			// Either alternative or invalid
			error = true; // Default error until OK alternative message found.
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
							altSize = CB_BLOCK_MAX_SIZE;
						if (size <= altSize) // Should correspond to the user given value
							error = false;
						break;
					}
				}
			}
		}
	}
	CBReleaseObject(typeBytes);
	if (error || !self->callbacks.acceptingType(self, peer, type)) {
		// Error with the message header type or length
		CBNetworkCommunicatorDisconnect(self, peer, CB_24_HOURS, false);
		CBReleaseObject(header);
		return;
	}
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
		case CB_MESSAGE_TYPE_VERSION:
			peer->receive = realloc(peer->receive, sizeof(CBVersion)); // For storing additional data
			CBGetObject(peer->receive)->free = CBFreeVersion;
			len = CBVersionDeserialise(CBGetVersion(peer->receive));
			break;
		case CB_MESSAGE_TYPE_ADDR:
			peer->receive = realloc(peer->receive, sizeof(CBNetworkAddress));
			CBGetObject(peer->receive)->free = CBFreeNetworkAddress;
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
			len = CBTransactionDeserialise(CBGetTransaction(peer->receive));
			break;
		case CB_MESSAGE_TYPE_BLOCK:
			peer->receive = realloc(peer->receive, sizeof(CBBlock));
			CBGetObject(peer->receive)->free = CBFreeBlock;
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
	// Check that the deserialised message's size read matches the message length.
	if (! peer->receive->bytes) {
		if (len) {
			// Error
			CBNetworkCommunicatorDisconnect(self, peer, CB_24_HOURS, false);
			return;
		}
	}else if (len != peer->receive->bytes->length){
		// Error
		CBNetworkCommunicatorDisconnect(self, peer, CB_24_HOURS, false);
		return;
	}
	// Deserialisation was sucessful.
	// Call event callback
	CBOnMessageReceivedAction action = self->callbacks.onMessageReceived(self, peer);
	if (action == CB_MESSAGE_ACTION_CONTINUE) {
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
		
	}
	// Release objects and get ready for next message
	switch (action) {
		case CB_MESSAGE_ACTION_DISCONNECT:
			// Node misbehaving. Disconnect.
			CBNetworkCommunicatorDisconnect(self, peer, CB_24_HOURS, false);
			break;
		case CB_MESSAGE_ACTION_CONTINUE:
			CBReleaseObject(peer->receive);
			// Update "lastSeen"
			// First remove the peer from the array as is will be moved.
			// Retain as we still need the peer.
			CBRetainObject(peer);
			CBMutexLock(self->peersMutex);
			CBNetworkAddressManagerRemovePeer(self->addresses, peer);
			CBMutexUnlock(self->peersMutex);
			// Update lastSeen
			CBGetNetworkAddress(peer)->lastSeen = time(NULL);
			CBNetworkAddressManagerTakePeer(self->addresses, peer);
			// Reset variables for receiving the next message
			peer->receivedHeader = false;
			peer->receive = NULL;
		default:
			break;
	}
}
void CBNetworkCommunicatorOnTimeOut(void * vself, void * vpeer, CBTimeOutType type){
	CBNetworkCommunicator * self = vself;
	CBPeer * peer = vpeer;
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
			// Check if we have the address as a connected peer
			CBPeer * peerB = CBNetworkAddressManagerGotPeer(self->addresses, addrs->addresses[x]);
			if (! peerB){
				// Do not already have this address as a peer
				if (addrs->addresses[x]->type & CB_IP_INVALID)
					// Address broadcasts should not contain invalid addresses.
					return CB_MESSAGE_ACTION_DISCONNECT;
				if (addrs->addresses[x]->type & CB_IP_LOCAL && ! (CBGetNetworkAddress(peer)->type & CB_IP_LOCAL))
					// Do not allow peers to send local addresses to non-local peers.
					return CB_MESSAGE_ACTION_DISCONNECT;
				// Else leave the time
				// Check if we have the address as a stored address
				CBNetworkAddress * addr = CBNetworkAddressManagerGotNetworkAddress(self->addresses, addrs->addresses[x]);
				if (addr){
					// Already have the address. Modify the existing object.
					if (addrs->addresses[x]->lastSeen > addr->lastSeen){
						// The peer has seen it sooner than us, so update the timestamp.
						// First take out the address
						// Retain since we still need the address
						CBRetainObject(addr);
						CBNetworkAddressManagerRemoveAddress(self->addresses, addr);
						// Update the last seen timestamp
						addr->lastSeen = addrs->addresses[x]->lastSeen;
						// As we've updated the timestamp for when the address is last seen, we need to reinsert it.
						CBNetworkAddressManagerTakeAddress(self->addresses, addr);
					}
					addr->services = addrs->addresses[x]->services;
					addr->version = addrs->addresses[x]->version;
				}else if (CBNetworkCommunicatorIsReachable(self, addrs->addresses[x]->type)){
					// Do not have the address so add it if we believe we can reach it and if it is not us.
					if ((self->ourIPv4
						 && addrs->addresses[x]->type & CB_IP_IPv4
						 && CBNetworkAddressEquals(self->ourIPv4, addrs->addresses[x]))
						|| (self->ourIPv6
							&& addrs->addresses[x]->type & CB_IP_IPv6
							&& CBNetworkAddressEquals(self->ourIPv6, addrs->addresses[x])))
						// Is us
						continue;
					// Make copy of the address since otherwise we will corrupt the CBNetworkAddress when trying the connection.
					CBByteArray * ipCopy = CBByteArrayCopy(addrs->addresses[x]->ip);
					if (ipCopy) {
						CBNetworkAddress * copy = CBNewNetworkAddress(addrs->addresses[x]->lastSeen, ipCopy, addrs->addresses[x]->port, addrs->addresses[x]->services, true);
						CBReleaseObject(ipCopy);
						if (copy){
							// See if the maximum number of addresses have been reached. If it has then remove an address before adding this one.
							if (self->addresses->addrNum > self->maxAddresses) {
								// This does not include peers, so there may actually be more stored addresses than the maxAddresses, as peers can be stored when public.
								CBNetworkAddress * addr = CBNetworkAddressManagerSelectAndRemoveAddress(self->addresses);
								// Delete from storage also.
								if (! CBAddressStorageDeleteAddress(self->addrStorage, addr))
									CBLogError("Could not remove an address from storage.");
							}
							// Add the address to the address manager and the storage (If the storage object is not NULL).
							CBNetworkAddressManagerAddAddress(self->addresses, copy);
							if (self->useAddrStorage)
								CBAddressStorageSaveAddress(self->addrStorage, copy);
							CBReleaseObject(copy);
							didAdd = true;
						}else return CB_MESSAGE_ACTION_DISCONNECT;
					}else return CB_MESSAGE_ACTION_DISCONNECT;
				}
			}else{
				// We have an advertised peer. This means it is public and should return to the address store.
				CBGetNetworkAddress(peerB)->isPublic = true;
				// Add the address to storage
				if (self->useAddrStorage)
					CBAddressStorageSaveAddress(self->addrStorage, CBGetNetworkAddress(peerB));
			}
		}
		if (! peer->getAddresses && addrs->addrNum < 10) {
			// Unsolicited addresses. Send out addresses to two random peers.
			// Check if we need to refresh the relayed addresses array
			uint64_t now = time(NULL);
			if (now - self->relayedAddrsLastClear > CB_24_HOURS) {
				// Clear the array
				CBAssociativeArrayClear(&self->relayedAddrs);
				self->relayedAddrsLastClear = now;
			}
			// Remove any addresses that we have already relayed since the last refresh, and then add the ones we haven't.
			for (uint8_t x = addrs->addrNum - 1; x--;) {
				CBFindResult res = CBAssociativeArrayFind(&self->relayedAddrs, addrs->addresses[x]);
				if (res.found) {
					// The address was relayed. Remove it from the broadcast
					CBReleaseObject(addrs->addresses[x]);
					if (addrs->addrNum - x > 1)
						memmove(addrs->addresses + x, addrs->addresses + x + 1, addrs->addrNum - x - 1);
					addrs->addrNum--;
				}else{
					// Add this address to the relayed addresses array
					CBAssociativeArrayInsert(&self->relayedAddrs, addrs->addresses[x], res.position, NULL);
					CBRetainObject(addrs->addresses[x]);
				}
			}
			// We can only broadcast when we have at least one remaining address
			if (addrs->addrNum) {
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
						if (CBNetworkAddressIPPortCompare(NULL, addrs->addresses[x], peerToRelay) == CB_COMPARE_EQUAL)
							in = true;
					if (peerToRelay != peer && ! in && peerToRelay->handshakeStatus == CB_HANDSHAKE_DONE) {
						// Relay the broadcast
						printf("%s is giving %s (relayed addr) to %s (%p)\n",
							   (self->ourIPv4->port == 45562)? "L1" : ((self->ourIPv4->port == 45563)? "L2" : "CN"),
							   (addrs->addresses[0]->port == 45562)? "L1" : ((addrs->addresses[0]->port == 45563)? "L2" : "CN"),
							   (CBGetNetworkAddress(peerToRelay)->port == 45562)? "L1" : ((CBGetNetworkAddress(peerToRelay)->port == 45563)? "L2" : ((CBGetNetworkAddress(peerToRelay)->port == 45564) ? "CN" : "UK")), (void *)peerToRelay);
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
		}
		if (didAdd)
			// We have new address information so try connecting to addresses.
			CBNetworkCommunicatorTryConnections(self);
		// Got addresses.
		peer->getAddresses = false;
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
				&& CBGetNetworkAddress(peerToInclude)->isPublic // Public
				&& (CBGetNetworkAddress(peerToInclude)->type != CB_IP_LOCAL // OK if not local
					|| CBGetNetworkAddress(peerToInclude)->type == CB_IP_LOCAL)) { // Or if the peer we are sending to is local
				CBNetworkAddressListAddNetworkAddress(addr, CBGetNetworkAddress(peerToInclude));
				printf("%s is giving %s (peer) to %s (%p)\n",
						   (self->ourIPv4->port == 45562)? "L1" : ((self->ourIPv4->port == 45563)? "L2" : "CN"),
						   (CBGetNetworkAddress(peerToInclude)->port == 45562)? "L1" : ((CBGetNetworkAddress(peerToInclude)->port == 45563)? "L2" : "CN"),
						   (CBGetNetworkAddress(peer)->port == 45562)? "L1" : ((CBGetNetworkAddress(peer)->port == 45563)? "L2" : ((CBGetNetworkAddress(peer)->port == 45564) ? "CN" : "UK")), (void *)peer);
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
				|| CBGetNetworkAddress(peer)->type == CB_IP_LOCAL){
				CBNetworkAddressListAddNetworkAddress(addr, addrs[x]);
				printf("%s is giving %s (addr) to %s (%p)\n",
				   (self->ourIPv4->port == 45562)? "L1" : ((self->ourIPv4->port == 45563)? "L2" : "CN"),
				   (addrs[x]->port == 45562)? "L1" : ((addrs[x]->port == 45563)? "L2" : "CN"),
				   (CBGetNetworkAddress(peer)->port == 45562)? "L1" : ((CBGetNetworkAddress(peer)->port == 45563)? "L2" : ((CBGetNetworkAddress(peer)->port == 45564) ? "CN" : "UK")), (void *)peer);
			}
			CBReleaseObject(addrs[x]);
		}
		// Send address broadcast, if we have at least one.
		if (addr->addrNum)
			CBNetworkCommunicatorSendMessage(self, peer, CBGetMessage(addr), NULL);
		CBReleaseObject(addr);
	}
	// Use opportunity to discover if we should broadcast our own addresses for recieving incoming connections.
	if (// Only share address if we allow for incomming connections.
		self->maxIncommingConnections
		// Share when we are listening for incoming connections.
		&& (self->isListeningIPv4 || self->isListeningIPv6)
		// Every 24 hours
		&& peer->time < time(NULL) - 86400) {
		peer->time = time(NULL);
		CBNetworkAddressList * addrs = CBNewNetworkAddressList(self->version >= CB_ADDR_TIME_VERSION && peer->versionMessage->version >= CB_ADDR_TIME_VERSION);
		CBGetMessage(addrs)->type = CB_MESSAGE_TYPE_ADDR;
		if (self->ourIPv6) {
			self->ourIPv6->lastSeen = time(NULL);
			CBNetworkAddressListAddNetworkAddress(addrs, self->ourIPv6);
		}
		if (self->ourIPv4) {
			self->ourIPv4->lastSeen = time(NULL);
			CBNetworkAddressListAddNetworkAddress(addrs, self->ourIPv4);
		}
		printf("%s is giving self to %s (%p)\n", (self->ourIPv4->port == 45562)? "L1" : ((self->ourIPv4->port == 45563)? "L2" : "CN"), (CBGetNetworkAddress(peer)->port == 45562)? "L1" : ((CBGetNetworkAddress(peer)->port == 45563)? "L2" : ((CBGetNetworkAddress(peer)->port == 45564) ? "CN" : "UK")), (void *)peer);
		CBNetworkCommunicatorSendMessage(self, peer, CBGetMessage(addrs), NULL);
		CBReleaseObject(addrs);
	}
	return CB_MESSAGE_ACTION_CONTINUE; // Do not disconnect.
}
CBOnMessageReceivedAction CBNetworkCommunicatorProcessMessageAutoHandshake(CBNetworkCommunicator * self, CBPeer * peer){
	bool endHandshake = false;
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
				CBCompare res = CBNetworkAddressIPPortCompare(NULL, CBGetVersion(peer->receive)->addSource,
															  self->ourIPv6 ? self->ourIPv6 : self->ourIPv4);
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
			// Copy over reported version and services for the CBNetworkAddress.
			CBGetNetworkAddress(peer)->version = peer->versionMessage->version;
			CBGetNetworkAddress(peer)->services = peer->versionMessage->services;
			// Remove the peer from the arrays so that it is repositioned with the updated data
			CBRetainObject(peer);
			CBMutexLock(self->peersMutex);
			CBNetworkAddressManagerRemovePeer(self->addresses, peer);
			CBMutexUnlock(self->peersMutex);
			// Change port and IP number to the port and IP the peer wants us to recognise them with.
			CBGetNetworkAddress(peer)->port = peer->versionMessage->addSource->port;
			if (CBByteArrayCompare(CBGetNetworkAddress(peer)->ip, peer->versionMessage->addSource->ip) != CB_COMPARE_EQUAL) {
				// The IP is different so change it.
				CBReleaseObject(CBGetNetworkAddress(peer)->ip);
				CBGetNetworkAddress(peer)->ip = peer->versionMessage->addSource->ip;
			}
			// Now re-insert the peer with the new information
			CBMutexLock(self->peersMutex);
			CBNetworkAddressManagerTakePeer(self->addresses, peer);
			CBMutexUnlock(self->peersMutex);
			// Set the IP type
			CBGetNetworkAddress(peer)->type = CBGetIPType(CBByteArrayGetData(CBGetNetworkAddress(peer)->ip));
			// Adjust network time
			peer->timeOffset = peer->versionMessage->time - time(NULL); // Set the time offset for this peer.
			// Now we have the network time, add the peer for the time offset. Ignore on failure
			CBNetworkAddressManagerTakePeerTimeOffset(self->addresses, peer);
			// We received the version
			peer->handshakeStatus |= CB_HANDSHAKE_GOT_VERSION;
			// Send our acknowledgement
			CBMessage * ack = CBNewMessageByObject();
			ack->type = CB_MESSAGE_TYPE_VERACK;
			bool ok = CBNetworkCommunicatorSendMessage(self, peer, ack, NULL);
			CBReleaseObject(ack);
			if (! ok)
				return CB_MESSAGE_ACTION_DISCONNECT; // If we cannot send an acknowledgement, exit.
			// Send version next if we have not already.
			if (! (peer->handshakeStatus & CB_HANDSHAKE_SENT_VERSION)) {
				CBVersion * version = CBNetworkCommunicatorGetVersion(self, CBGetNetworkAddress(peer));
				if (version) {
					CBGetMessage(version)->expectResponse = CB_MESSAGE_TYPE_VERACK; // Expect a response for this message.
					CBGetMessage(version)->type = CB_MESSAGE_TYPE_VERSION;
					bool ok = CBNetworkCommunicatorSendMessage(self, peer, CBGetMessage(version), NULL);
					CBReleaseObject(version);
					if (! ok)
						return CB_MESSAGE_ACTION_DISCONNECT;
				}else
					return CB_MESSAGE_ACTION_DISCONNECT;
			}else
				// Sent already. End of handshake
				endHandshake = true;
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
		peer->getAddresses = true;
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
bool CBNetworkCommunicatorSendMessage(CBNetworkCommunicator * self, CBPeer * peer, CBMessage * message, void (*callback)(void *, void *)){
	CBMutexLock(self->sendMessageMutex);
	if (peer->sendQueueSize == CB_SEND_QUEUE_MAX_SIZE)
		return false;
	printf("%s is sending %u to %s (%p) (%u)\n", (self->ourIPv4->port == 45562)? "L1" : ((self->ourIPv4->port == 45563)? "L2" : "CN"), message->type, (CBGetNetworkAddress(peer)->port == 45562)? "L1" : ((CBGetNetworkAddress(peer)->port == 45563)? "L2" : ((CBGetNetworkAddress(peer)->port == 45564) ? "CN" : "UK")), (void *)peer, peer->handshakeStatus);
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
				len = CBNetworkAddressSerialise(CBGetNetworkAddress(message), false);
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
	CBMutexUnlock(self->sendMessageMutex);
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
					CBNetworkCommunicatorSendMessage(self, peer, CBGetMessage(pingPong), NULL);
					CBGetMessage(peer)->expectResponse = CB_MESSAGE_TYPE_PONG; // Expect a pong.
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
	CBRetainObject(ourIPv4);
	self->ourIPv4 = ourIPv4;
}
void CBNetworkCommunicatorSetOurIPv6(CBNetworkCommunicator * self, CBNetworkAddress * ourIPv6){
	CBRetainObject(ourIPv6);
	self->ourIPv6 = ourIPv6;
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
bool CBNetworkCommunicatorStart(CBNetworkCommunicator * self){
	// Create the socket event loop
	if (! CBNewEventLoop(&self->eventLoop, CBNetworkCommunicatorOnLoopError, CBNetworkCommunicatorOnTimeOut, self)){
		// Cannot create event loop
		CBLogError("The CBNetworkCommunicator event loop could not be created.");
		return false;
	}
	self->isStarted = true;
	return true; // All OK.
}
void CBNetworkCommunicatorStartListening(CBNetworkCommunicator * self){
	if (CBNetworkCommunicatorIsReachable(self, CB_IP_IPv4)) {
		// Create new bound IPv4 socket
		if (CBSocketBind(&self->listeningSocketIPv4, false, self->ourIPv4->port)){
			// Add event for accepting connection for both sockets
			if (CBSocketCanAcceptEvent(&self->acceptEventIPv4, self->eventLoop, self->listeningSocketIPv4, CBNetworkCommunicatorAcceptConnection)) {
				if(CBSocketAddEvent(self->acceptEventIPv4, 0)) // No timeout for listening for incomming connections.
					// Start listening on IPv4
					if(CBSocketListen(self->listeningSocketIPv4, self->maxIncommingConnections)){
						// Is listening
						self->isListeningIPv4 = true;
					}
				if (! self->isListeningIPv4)
					// Failure to add event
					CBSocketFreeEvent(self->acceptEventIPv4);
			}
		}
		if (! self->isListeningIPv4)
			// Failure to create event
			CBCloseSocket(self->listeningSocketIPv4);
	}
	// Now for the IPv6
	if (CBNetworkCommunicatorIsReachable(self, CB_IP_IPv6)) {
		// Create new bound IPv6 socket
		if (CBSocketBind(&self->listeningSocketIPv6, true, self->ourIPv6->port)){
			// Add event for accepting connection for both sockets
			if (CBSocketCanAcceptEvent(&self->acceptEventIPv6, self->eventLoop, self->listeningSocketIPv6, CBNetworkCommunicatorAcceptConnection)) {
				if(CBSocketAddEvent(self->acceptEventIPv6, 0)) // No timeout for listening for incomming connections.
					// Start listening on IPv6
					if(CBSocketListen(self->listeningSocketIPv6, self->maxIncommingConnections)){
						// Is listening
						self->isListeningIPv6 = true;
					}
				if (! self->isListeningIPv6)
					// Failure to add event
					CBSocketFreeEvent(self->acceptEventIPv6);
			}
		}
		if (! self->isListeningIPv6)
			// Failure to create event
			CBCloseSocket(self->listeningSocketIPv6);
	}
}
void CBNetworkCommunicatorStartPings(CBNetworkCommunicator * self){
	self->isPinging = true;
	CBStartTimer(self->eventLoop, &self->pingTimer, self->heartBeat, CBNetworkCommunicatorSendPings, self);
}
void CBNetworkCommunicatorStop(CBNetworkCommunicator * self){
	if (self->isListeningIPv4 || self->isListeningIPv6)
		CBNetworkCommunicatorStopListening(self);
	self->isStarted = false;
	// Disconnect all the peers
	CBAssociativeArrayForEach(CBPeer * peer, &self->addresses->peers)
		CBNetworkCommunicatorDisconnect(self, peer, 0, true); // "true" we are stopping.
	// Now reset the peers arrays. The addresses were released in CBNetworkCommunicatorDisconnect, so this function only clears the array nodes.
	CBNetworkAddressManagerClearPeers(self->addresses);
	// Stop event loop
	CBExitEventLoop(self->eventLoop);
}
void CBNetworkCommunicatorStopListening(CBNetworkCommunicator * self){
	if (self->isListeningIPv4) {
		// Stop listening on IPv4
		CBSocketFreeEvent(self->acceptEventIPv4);
		CBCloseSocket(self->listeningSocketIPv4);
		self->isListeningIPv4 = false;
	}
	if (self->isListeningIPv6) {
		// Stop listening on IPv6
		CBSocketFreeEvent(self->acceptEventIPv6);
		CBCloseSocket(self->listeningSocketIPv6);
		self->isListeningIPv6 = false;
	}
}
void CBNetworkCommunicatorStopPings(CBNetworkCommunicator * self){
	if (self->isPinging){
		CBEndTimer(self->pingTimer);
		self->isPinging = false;
	}
}
void CBNetworkCommunicatorTryConnections(CBNetworkCommunicator * self){
	if (self->attemptingOrWorkingConnections >= self->maxConnections)
		return; // Cannot connect to any more peers
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
		CBPeer * peer = CBNewPeerByTakingNetworkAddress(addrs[x]);
		peer->incomming = false;
		CBConnectReturn res = CBNetworkCommunicatorConnect(self, peer);
		if (res == CB_CONNECT_ERROR || res == CB_CONNECT_FAILED) {
			// Convert back to address.
			CBNetworkAddress * temp = realloc(peer, sizeof(*temp));
			CBNetworkAddress * addr;
			if (temp)
				// Reallocation worked
				addr = temp;
			else
				addr = CBGetNetworkAddress(peer);
			if (res == CB_CONNECT_FAILED)
				// Add penalty if failed
				addr->lastSeen -= 3600;
			// Re-insert into the address manager, since we could not connect this time. If it fails, ignore and the address will not be added.
			CBNetworkAddressManagerTakeAddress(self->addresses, addrs[x]);
		}
		// Either the connection was OK and it should either timeout or finalise, or we forget about it because it is not supported.
	}
	// Free address pointer memory.
	free(addrs);
}
