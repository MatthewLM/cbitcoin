//
//  CBNetworkCommunicator.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 13/07/2012.
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

//  SEE HEADER FILE FOR DOCUMENTATION

#include "CBNetworkCommunicator.h"

//  Constructor

CBNetworkCommunicator * CBNewNetworkCommunicator(){
	CBNetworkCommunicator * self = malloc(sizeof(*self));
	if (NOT self) {
		CBLogError("Cannot allocate %i bytes of memory in CBNewNetworkCommunicator\n", sizeof(*self));
		return NULL;
	}
	CBGetObject(self)->free = CBFreeNetworkCommunicator;
	if (CBInitNetworkCommunicator(self))
		return self;
	free(self);
	return NULL;
}

//  Object Getter

CBNetworkCommunicator * CBGetNetworkCommunicator(void * self){
	return self;
}

//  Initialiser

bool CBInitNetworkCommunicator(CBNetworkCommunicator * self){
	if (NOT CBInitObject(CBGetObject(self)))
		return false;
	// Initialise sentAddrs array
	if (NOT CBInitAssociativeArray(&self->relayedAddrs, CBNetworkAddressIPPortCompare, CBReleaseObject)){
		CBLogError("Could not initialise the array for relayed addresses.");
		return false;
	}
	// Set fields.
	self->relayedAddrsLastClear = time(NULL);
	self->attemptingOrWorkingConnections = 0;
	self->numIncommingConnections = 0;
	self->eventLoop = 0;
	self->acceptEventIPv4 = 0;
	self->acceptEventIPv6 = 0;
	self->isListeningIPv4 = false;
	self->isListeningIPv6 = false;
	self->ourIPv4 = NULL;
	self->ourIPv6 = NULL;
	self->pingTimer = 0;
	self->nonce = 0;
	self->stoppedListening = false;
	self->addrStorage = 0;
	self->reachability = 0;
	self->alternativeMessages = NULL;
	self->pendingIP = 0;
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
	return true;
}

//  Destructor

void CBFreeNetworkCommunicator(void * vself){
	// When this is called all references to the object should be released, so no additional syncronisation is required
	CBNetworkCommunicator * self = vself;
	if (self->isStarted)
		CBNetworkCommunicatorStop(self);
	if (self->alternativeMessages) CBReleaseObject(self->alternativeMessages);
	CBReleaseObject(self->addresses);
	if (self->ourIPv4) CBReleaseObject(self->ourIPv4);
	if (self->ourIPv6) CBReleaseObject(self->ourIPv6);
	free(self->altMaxSizes);
	CBFreeAssociativeArray(&self->relayedAddrs);
	CBFreeObject(self);
}

//  Functions

void CBNetworkCommunicatorAcceptConnection(void * vself, uint64_t socket){
	CBNetworkCommunicator * self = vself;
	uint64_t connectSocketID;
	if (NOT CBSocketAccept(socket, &connectSocketID))
		return;
	// Connected, add CBNetworkAddress. Use a special placeholder ip identifier for the address, so that it is unique.
	CBByteArray * ip = CBNewByteArrayOfSize(16);
	CBByteArraySetInt64(ip, 0, self->pendingIP);
	CBPeer * peer = CBNewPeerByTakingNetworkAddress(CBNewNetworkAddress(0, ip, 0, 0, false));
	CBReleaseObject(ip);
	if (NOT peer)
		return;
	// Increment the pendingIP for the next incomming connection
	self->pendingIP++;
	peer->incomming = true;
	peer->socketID = connectSocketID;
	peer->acceptedTypes = CB_MESSAGE_TYPE_VERSION; // The peer connected to us, we want the version from them.
	// Set up receive event
	if (CBSocketCanReceiveEvent(&peer->receiveEvent, self->eventLoop, peer->socketID, CBNetworkCommunicatorOnCanReceive, peer)) {
		// The event works
		if(CBSocketAddEvent(peer->receiveEvent, self->responseTimeOut)){ // Begin receive event.
			// Success
			if (CBSocketCanSendEvent(&peer->sendEvent, self->eventLoop, peer->socketID, CBNetworkCommunicatorOnCanSend, peer)) {
				// Both events work. Take the peer.
				if (CBAddressManagerTakePeer(self->addresses, peer)){
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
					return;
				}
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
	if (NOT CBNetworkCommunicatorIsReachable(self, CBGetNetworkAddress(peer)->type))
		return CB_CONNECT_NO_SUPPORT;
	bool isIPv6 = CBGetNetworkAddress(peer)->type & CB_IP_IPv6;
	// Make socket
	CBSocketReturn res = CBNewSocket(&peer->socketID, isIPv6);
	if(res == CB_SOCKET_NO_SUPPORT){
		if (isIPv6) CBNetworkCommunicatorSetReachability(self, CB_IP_IPv6, false);
		else CBNetworkCommunicatorSetReachability(self, CB_IP_IPv4, false);
		if (NOT CBNetworkCommunicatorIsReachable(self, CB_IP_IPv6 | CB_IP_IPv4))
			// IPv6 and IPv4 not reachable.
			self->onNetworkError(self->callbackHandler, self);
		return CB_CONNECT_NO_SUPPORT;
	}else if (res == CB_SOCKET_BAD) {
		if (NOT self->addresses->peersNum)
			self->onNetworkError(self->callbackHandler, self);
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
	if (NOT CBAddressManagerGotPeer(self->addresses, CBGetNetworkAddress(peer))){
		// Make receive event
		if (CBSocketCanReceiveEvent(&peer->receiveEvent, self->eventLoop, peer->socketID, CBNetworkCommunicatorOnCanReceive, peer)) {
			// Make send event
			if (CBSocketCanSendEvent(&peer->sendEvent, self->eventLoop, peer->socketID, CBNetworkCommunicatorOnCanSend, peer)) {
				if (CBSocketAddEvent(peer->sendEvent, self->sendTimeOut)) {
					if (CBAddressManagerTakePeer(self->addresses, peer)){
						if (self->addresses->peersNum == 1 && self->flags & CB_NETWORK_COMMUNICATOR_AUTO_PING)
							// Got first peer, start pings
							CBNetworkCommunicatorStartPings(self);
						peer->connectionWorking = true;
						// Connection OK, so begin handshake if auto handshaking is enabled.
						bool fail = false;
						if (self->flags & CB_NETWORK_COMMUNICATOR_AUTO_HANDSHAKE){
							CBVersion * version = CBNetworkCommunicatorGetVersion(self, CBGetNetworkAddress(peer));
							if (version) {
								CBGetMessage(version)->expectResponse = CB_MESSAGE_TYPE_VERACK; // Expect a response for this message.
								CBGetMessage(version)->type = CB_MESSAGE_TYPE_VERSION;
								peer->acceptedTypes = CB_MESSAGE_TYPE_VERACK; // Accept the acknowledgement.
								CBNetworkCommunicatorSendMessage(self, peer, CBGetMessage(version));
								CBReleaseObject(version);
								// We are now sending the version
								peer->versionSent = true;
							}else
								fail = true;
						}
						if (NOT fail)
							return;
					}
				}
				CBSocketFreeEvent(peer->sendEvent);
			}
			CBSocketFreeEvent(peer->receiveEvent);
		}
	}
	// Close socket on failure
	CBCloseSocket(peer->socketID);
	self->attemptingOrWorkingConnections--;
	if (NOT self->attemptingOrWorkingConnections) {
		self->onNetworkError(self->callbackHandler, self);
	}
	// Add the address back to the addresses listwith no penalty here since it was definitely our fault.
	CBNetworkAddress * addr = realloc(peer, sizeof(CBNetworkAddress));
	if (addr)
		CBAddressManagerTakeAddress(self->addresses, addr);
}
void CBNetworkCommunicatorDisconnect(CBNetworkCommunicator * self, CBPeer * peer, uint32_t penalty, bool stopping){
	// Free CBLogError if they exist
	if (peer->receiveEvent) CBSocketFreeEvent(peer->receiveEvent);
	if (peer->sendEvent) CBSocketFreeEvent(peer->sendEvent);
	// Close the socket
	CBCloseSocket(peer->socketID);
	// Release the receiving message object if it exists.
	if (peer->receive) CBReleaseObject(peer->receive);
	// Release all messages in the send queue
	for (uint8_t x = 0; x < peer->sendQueueSize; x++)
		CBReleaseObject(peer->sendQueue[(peer->sendQueueFront + x) % CB_SEND_QUEUE_MAX_SIZE]);
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
		if (NOT stopping){
			// Retain the peer in case we continue to need it
			CBRetainObject(peer);
			// We aren't stopping so we should remove the node from the array.
			CBAddressManagerRemovePeer(self->addresses, peer);
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
			CBAddressManagerAddAddress(self->addresses, CBGetNetworkAddress(peer));
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
	if (NOT self->attemptingOrWorkingConnections && NOT stopping)
		// No more connections so give a network error
		self->onNetworkError(self->callbackHandler, self);
}
CBVersion * CBNetworkCommunicatorGetVersion(CBNetworkCommunicator * self, CBNetworkAddress * addRecv){
	CBNetworkAddress * sourceAddr;
	if (addRecv->type & CB_IP_IPv6
		&& self->ourIPv6)
		sourceAddr = self->ourIPv6;
	else if (self->ourIPv4)
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
	if (NOT peer->receive) {
		// New message to be received.
		peer->receive = CBNewMessageByObject();
		if (NOT peer->receive) {
			CBLogError("Could not create a new message object for receiving a message from a peer.");
			CBNetworkCommunicatorDisconnect(self, peer, 0, false);
			return;
		}
		peer->headerBuffer = malloc(24); // Twenty-four bytes for the message header.
		if (NOT peer->headerBuffer) {
			// If we run out of memory we want to disconnect as nothing more can be done here.
			CBLogError("Could not allocate memory for a message header.");
			CBNetworkCommunicatorDisconnect(self, peer, 0, false);
			return;
		}
		peer->messageReceived = 0; // So far received nothing.
		// From now on use timeout for receiving data.
		if (NOT CBSocketAddEvent(peer->receiveEvent, self->recvTimeOut)){
			CBLogError("Could not change the timeout for a peer's receive event for receiving a new message");
			CBNetworkCommunicatorDisconnect(self, peer, 0, false);
			return;
		}
		// Start download timer
		peer->downloadTimerStart = CBGetMilliseconds();
	}
	if (NOT peer->receivedHeader) {
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
	if (NOT peer->sentHeader) {
		// Need to send the header
		if (NOT peer->messageSent) {
			// Create header
			peer->sendingHeader = malloc(24);
			if (NOT peer->sendingHeader) {
				CBNetworkCommunicatorDisconnect(self, peer, 0, false);
				return;
			}
			// Network ID
			CBInt32ToArray(peer->sendingHeader, 0, self->networkID);
			// Get the message we are sending.
			CBMessage * toSend = peer->sendQueue[peer->sendQueueFront];
			// Message type text
			switch (toSend->type) {
				case CB_MESSAGE_TYPE_VERSION:
					memcpy(peer->sendingHeader + 4, "version\0\0\0\0\0", 12);
					break;
				case CB_MESSAGE_TYPE_VERACK:
					memcpy(peer->sendingHeader + 4, "verack\0\0\0\0\0\0", 12);
					break;
				case CB_MESSAGE_TYPE_ADDR:
					memcpy(peer->sendingHeader + 4, "addr\0\0\0\0\0\0\0\0", 12);
					break;
				case CB_MESSAGE_TYPE_INV:
					memcpy(peer->sendingHeader + 4, "inv\0\0\0\0\0\0\0\0\0", 12);
					break;
				case CB_MESSAGE_TYPE_GETDATA:
					memcpy(peer->sendingHeader + 4, "getdata\0\0\0\0\0", 12);
					break;
				case CB_MESSAGE_TYPE_GETBLOCKS:
					memcpy(peer->sendingHeader + 4, "getblocks\0\0\0", 12);
					break;
				case CB_MESSAGE_TYPE_GETHEADERS:
					memcpy(peer->sendingHeader + 4, "getheaders\0\0", 12);
					break;
				case CB_MESSAGE_TYPE_TX:
					memcpy(peer->sendingHeader + 4, "tx\0\0\0\0\0\0\0\0\0\0", 12);
					break;
				case CB_MESSAGE_TYPE_BLOCK:
					memcpy(peer->sendingHeader + 4, "block\0\0\0\0\0\0\0", 12);
					break;
				case CB_MESSAGE_TYPE_HEADERS:
					memcpy(peer->sendingHeader + 4, "headers\0\0\0\0\0", 12);
					break;
				case CB_MESSAGE_TYPE_GETADDR:
					memcpy(peer->sendingHeader + 4, "getaddr\0\0\0\0\0", 12);
					break;
				case CB_MESSAGE_TYPE_PING:
					memcpy(peer->sendingHeader + 4, "ping\0\0\0\0\0\0\0\0", 12);
					break;
				case CB_MESSAGE_TYPE_PONG:
					memcpy(peer->sendingHeader + 4, "pong\0\0\0\0\0\0\0\0", 12);
					break;
				case CB_MESSAGE_TYPE_ALERT:
					memcpy(peer->sendingHeader + 4, "alert\0\0\0\0\0\0\0", 12);
					break;
				default:
					memcpy(peer->sendingHeader + 4, toSend->altText, 12);
					break;
			}
			// Length
			if (toSend->bytes){
				CBInt32ToArray(peer->sendingHeader, 16, toSend->bytes->length);
			}else
				memset(peer->sendingHeader + 16, 0, 4);
			assert(peer->sendingHeader[16] != 27 || peer->sendingHeader[4] != 'a');
			// Checksum
			memcpy(peer->sendingHeader + 20, toSend->checksum, 4);
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
				if (peer->sendQueue[peer->sendQueueFront]->bytes) {
					// Now send the bytes
					peer->messageSent = 0;
					peer->sentHeader = true;
				}else{
					// Nothing more to send!
					finished = true;
				}
			}
		}
	}else{
		// Sent header
		int32_t len = CBSocketSend(peer->socketID, CBByteArrayGetData(peer->sendQueue[peer->sendQueueFront]->bytes) + peer->messageSent, peer->sendQueue[peer->sendQueueFront]->bytes->length - peer->messageSent);
		if (len == CB_SOCKET_FAILURE)
			CBNetworkCommunicatorDisconnect(self, peer, 0, false);
		else{
			peer->messageSent += len;
			if (peer->messageSent == peer->sendQueue[peer->sendQueueFront]->bytes->length)
				// Sent the entire payload. Celebrate with some bonbonbonbons: http://www.youtube.com/watch?v=VWgwJfbeCeU
				finished = true;
		}
	}
	if (finished) {
		// Reset variables for next send.
		peer->messageSent = 0;
		peer->sentHeader = false;
		// Done sending message.
		if (peer->sendQueue[peer->sendQueueFront]->expectResponse){
			CBSocketAddEvent(peer->receiveEvent, self->responseTimeOut); // Expect response.
			// Add to list of expected responses.
			if (peer->typesExpectedNum == CB_MAX_RESPONSES_EXPECTED) {
				CBNetworkCommunicatorDisconnect(self, peer, CB_24_HOURS, false);
				return;
			}
			peer->typesExpected[peer->typesExpectedNum] = peer->sendQueue[peer->sendQueueFront]->expectResponse;
			peer->typesExpectedNum++;
			// Start timer for latency measurement, if not already waiting.
			if (NOT peer->latencyTimerStart){
				peer->latencyTimerStart = CBGetMilliseconds();
				peer->latencyExpected = peer->sendQueue[peer->sendQueueFront]->expectResponse;
			}
		}
		// Remove message from queue.
		peer->sendQueueSize--;
		CBReleaseObject(peer->sendQueue[peer->sendQueueFront]);
		if (peer->sendQueueSize) {
			peer->sendQueueFront++;
			if (peer->sendQueueFront == CB_SEND_QUEUE_MAX_SIZE) {
				peer->sendQueueFront = 0;
			}
		}else{
			// Remove send event as we have nothing left to send
			CBSocketRemoveEvent(peer->sendEvent);
		}
	}
}
void CBNetworkCommunicatorOnHeaderRecieved(CBNetworkCommunicator * self, CBPeer * peer){
	// Make a CBByteArray. ??? Could be modified not to use a CBByteArray, but it is cleaner this way and easier to maintain.
	CBByteArray * header = CBNewByteArrayWithData(peer->headerBuffer, 24);
	if (NOT header)
		CBNetworkCommunicatorDisconnect(self, peer, 0, false);
	if (CBByteArrayReadInt32(header, 0) != self->networkID){
		// The network ID bytes is not what we are looking for. We will have to remove the peer.
		CBReleaseObject(header);
		CBNetworkCommunicatorDisconnect(self, peer, CB_24_HOURS, false);
	}
	CBMessageType type;
	uint32_t size;
	bool error = false;
	CBByteArray * typeBytes = CBNewByteArraySubReference(header, 4, 12);
	if (NOT typeBytes){
		CBReleaseObject(header);
		CBNetworkCommunicatorDisconnect(self, peer, 0, false);
	}
	if (NOT memcmp(CBByteArrayGetData(typeBytes), "version\0\0\0\0\0", 12)) {
		// Version message
		// Check if we are accepting version messages at the moment.
		if (peer->acceptedTypes & CB_MESSAGE_TYPE_VERSION){
			// Check payload size
			size = CBByteArrayReadInt32(header, 16);
			if (size > CB_VERSION_MAX_SIZE) // Illegal size. cbitcoin will not accept this.
				error = true;
			else
				type = CB_MESSAGE_TYPE_VERSION;
		}else error = true;
	}else if (NOT memcmp(CBByteArrayGetData(typeBytes), "verack\0\0\0\0\0\0", 12)){
		// Version acknowledgement message
		if (peer->acceptedTypes & CB_MESSAGE_TYPE_VERACK){
			size = CBByteArrayReadInt32(header, 16);
			if (size) // Must contain zero payload.
				error = true;
			else
				type = CB_MESSAGE_TYPE_VERACK;
		}else error = true;
	}else if (NOT memcmp(CBByteArrayGetData(typeBytes), "addr\0\0\0\0\0\0\0\0", 12)){
		// Address broadcast message
		if (peer->acceptedTypes & CB_MESSAGE_TYPE_ADDR){
			size = CBByteArrayReadInt32(header, 16);
			if (size > CB_ADDRESS_BROADCAST_MAX_SIZE)
				error = true;
			else
				type = CB_MESSAGE_TYPE_ADDR;
		}else error = true;
	}else if (NOT memcmp(CBByteArrayGetData(typeBytes), "inv\0\0\0\0\0\0\0\0\0", 12)){
		// Inventory broadcast message
		if (peer->acceptedTypes & CB_MESSAGE_TYPE_INV){
			size = CBByteArrayReadInt32(header, 16);
			if (size > CB_INVENTORY_BROADCAST_MAX_SIZE)
				error = true;
			else
				type = CB_MESSAGE_TYPE_INV;
		}else error = true;
	}else if (NOT memcmp(CBByteArrayGetData(typeBytes), "getdata\0\0\0\0\0", 12)){
		// Get data message
		if (peer->acceptedTypes & CB_MESSAGE_TYPE_GETDATA){
			size = CBByteArrayReadInt32(header, 16);
			if (size > CB_GET_DATA_MAX_SIZE)
				error = true;
			else
				type = CB_MESSAGE_TYPE_GETDATA;
		}else error = true;
	}else if (NOT memcmp(CBByteArrayGetData(typeBytes), "getblocks\0\0\0", 12)){
		// Get blocks message
		if (peer->acceptedTypes & CB_MESSAGE_TYPE_BLOCK){
			size = CBByteArrayReadInt32(header, 16);
			if (size > CB_GET_BLOCKS_MAX_SIZE)
				error = true;
			else
				type = CB_MESSAGE_TYPE_GETBLOCKS;
		}else error = true;
	}else if (NOT memcmp(CBByteArrayGetData(typeBytes), "getheaders\0\0", 12)){
		// Get headers message
		if (peer->acceptedTypes & CB_MESSAGE_TYPE_GETHEADERS){
			size = CBByteArrayReadInt32(header, 16);
			if (size > CB_GET_HEADERS_MAX_SIZE)
				error = true;
			else
				type = CB_MESSAGE_TYPE_GETHEADERS;
		}else error = true;
	}else if (NOT memcmp(CBByteArrayGetData(typeBytes), "tx\0\0\0\0\0\0\0\0\0\0", 12)){
		// Transaction message
		if (peer->acceptedTypes & CB_MESSAGE_TYPE_TX){
			size = CBByteArrayReadInt32(header, 16);
			if (size > CB_TRANSACTION_MAX_SIZE)
				error = true;
			else
				type = CB_MESSAGE_TYPE_TX;
		}else error = true;
	}else if (NOT memcmp(CBByteArrayGetData(typeBytes), "block\0\0\0\0\0\0\0", 12)){
		// Block message
		if (peer->acceptedTypes & CB_MESSAGE_TYPE_BLOCK){
			size = CBByteArrayReadInt32(header, 16);
			if (size > CB_BLOCK_MAX_SIZE + 89) // Plus 89 for header.
				error = true;
			else
				type = CB_MESSAGE_TYPE_BLOCK;
		}else error = true;
	}else if (NOT memcmp(CBByteArrayGetData(typeBytes), "headers\0\0\0\0\0", 12)){
		// Block headers message
		if (peer->acceptedTypes & CB_MESSAGE_TYPE_HEADERS){
			size = CBByteArrayReadInt32(header, 16);
			if (size > CB_BLOCK_HEADERS_MAX_SIZE)
				error = true;
			else
				type = CB_MESSAGE_TYPE_HEADERS;
		}else error = true;
	}else if (NOT memcmp(CBByteArrayGetData(typeBytes), "getaddr\0\0\0\0\0", 12)){
		// Get Addresses message
		if (peer->acceptedTypes & CB_MESSAGE_TYPE_GETADDR){
			size = CBByteArrayReadInt32(header, 16);
			if (size) // Should be empty
				error = true;
			else
				type = CB_MESSAGE_TYPE_GETADDR;
		}else error = true;
	}else if (NOT memcmp(CBByteArrayGetData(typeBytes), "ping\0\0\0\0\0\0\0\0", 12)){
		// Ping message
		if (peer->acceptedTypes & CB_MESSAGE_TYPE_PING){
			size = CBByteArrayReadInt32(header, 16);
			if (size > 8 || ((peer->versionMessage->version < CB_PONG_VERSION || self->version < CB_PONG_VERSION) && size)) // Should be empty before version 60000 or 8 bytes.
				error = true;
			else
				type = CB_MESSAGE_TYPE_PING;
		}else error = true;
	}else if (NOT memcmp(CBByteArrayGetData(typeBytes), "pong\0\0\0\0\0\0\0\0", 12)){
		// Pong message
		if (peer->acceptedTypes & CB_MESSAGE_TYPE_PONG){
			size = CBByteArrayReadInt32(header, 16);
			if (size > 8)
				error = true;
			else
				type = CB_MESSAGE_TYPE_PONG;
		}else error = true;
	}else if (NOT memcmp(CBByteArrayGetData(typeBytes), "alert\0\0\0\0\0\0\0", 12)){
		// Alert message
		if (peer->acceptedTypes & CB_MESSAGE_TYPE_ALERT){
			size = CBByteArrayReadInt32(header, 16);
			if (size > CB_ALERT_MAX_SIZE)
				error = true;
			else
				type = CB_MESSAGE_TYPE_ALERT;
		}else error = true;
	}else{
		// Either alternative or invalid
		error = true; // Default error until OK alternative message found.
		if (self->alternativeMessages) {
			for (uint16_t x = 0; x < self->alternativeMessages->length / 12; x++) {
				// Check this alternative message
				if (NOT memcmp(CBByteArrayGetData(typeBytes), CBByteArrayGetData(self->alternativeMessages) + 12 * x, 12)){
					// Alternative message
					type = CB_MESSAGE_TYPE_ALT;
					// Check payload size
					size = CBByteArrayReadInt32(header, 16);
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
	CBReleaseObject(typeBytes);
	if (error) {
		// Error with the message header type or length
		CBNetworkCommunicatorDisconnect(self, peer, CB_24_HOURS, false);
		CBReleaseObject(header);
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
				if (type == CB_MESSAGE_TYPE_VERACK && NOT peer->versionMessage){
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
		if (NOT peer->receive->bytes)
			CBNetworkCommunicatorDisconnect(self, peer, 0, false);
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
	self->eventLoop = 0;
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
			if (NOT peer->receive) {
				CBNetworkCommunicatorDisconnect(self, peer, 0, false);
				return;
			}
			CBGetObject(peer->receive)->free = CBFreeVersion;
			len = CBVersionDeserialise(CBGetVersion(peer->receive));
			break;
		case CB_MESSAGE_TYPE_ADDR:
			peer->receive = realloc(peer->receive, sizeof(CBAddressBroadcast));
			if (NOT peer->receive) {
				CBNetworkCommunicatorDisconnect(self, peer, 0, false);
				return;
			}
			CBGetObject(peer->receive)->free = CBFreeAddressBroadcast;
			CBGetAddressBroadcast(peer->receive)->timeStamps = peer->versionMessage->version > 31401; // Timestamps start version 31402 and up.
			len = CBAddressBroadcastDeserialise(CBGetAddressBroadcast(peer->receive));
			break;
		case CB_MESSAGE_TYPE_INV:
		case CB_MESSAGE_TYPE_GETDATA:
			peer->receive = realloc(peer->receive, sizeof(CBInventoryBroadcast));
			if (NOT peer->receive) {
				CBNetworkCommunicatorDisconnect(self, peer, 0, false);
				return;
			}
			CBGetObject(peer->receive)->free = CBFreeInventoryBroadcast;
			len = CBInventoryBroadcastDeserialise(CBGetInventoryBroadcast(peer->receive));
			break;
		case CB_MESSAGE_TYPE_GETBLOCKS:
		case CB_MESSAGE_TYPE_GETHEADERS:
			peer->receive = realloc(peer->receive, sizeof(CBGetBlocks));
			if (NOT peer->receive) {
				CBNetworkCommunicatorDisconnect(self, peer, 0, false);
				return;
			}
			CBGetObject(peer->receive)->free = CBFreeGetBlocks;
			len = CBGetBlocksDeserialise(CBGetGetBlocks(peer->receive));
			break;
		case CB_MESSAGE_TYPE_TX:
			peer->receive = realloc(peer->receive, sizeof(CBTransaction));
			if (NOT peer->receive) {
				CBNetworkCommunicatorDisconnect(self, peer, 0, false);
				return;
			}
			CBGetObject(peer->receive)->free = CBFreeTransaction;
			len = CBTransactionDeserialise(CBGetTransaction(peer->receive));
			break;
		case CB_MESSAGE_TYPE_BLOCK:
			peer->receive = realloc(peer->receive, sizeof(CBBlock));
			if (NOT peer->receive) {
				CBNetworkCommunicatorDisconnect(self, peer, 0, false);
				return;
			}
			CBGetObject(peer->receive)->free = CBFreeBlock;
			len = CBBlockDeserialise(CBGetBlock(peer->receive), true); // true -> Including transactions.
			break;
		case CB_MESSAGE_TYPE_HEADERS:
			peer->receive = realloc(peer->receive, sizeof(CBBlockHeaders));
			if (NOT peer->receive) {
				CBNetworkCommunicatorDisconnect(self, peer, 0, false);
				return;
			}
			CBGetObject(peer->receive)->free = CBFreeBlockHeaders;
			len = CBBlockHeadersDeserialise(CBGetBlockHeaders(peer->receive));
			break;
		case CB_MESSAGE_TYPE_PING:
			if (peer->versionMessage->version >= 60000 && self->version >= 60000){
				peer->receive = realloc(peer->receive, sizeof(CBPingPong));
				if (NOT peer->receive) {
					CBNetworkCommunicatorDisconnect(self, peer, 0, false);
					return;
				}
				CBGetObject(peer->receive)->free = CBFreePingPong;
				len = CBPingPongDeserialise(CBGetPingPong(peer->receive));
			}else len = 0;
		case CB_MESSAGE_TYPE_PONG:
			peer->receive = realloc(peer->receive, sizeof(CBPingPong));
			if (NOT peer->receive) {
				CBNetworkCommunicatorDisconnect(self, peer, 0, false);
				return;
			}
			CBGetObject(peer->receive)->free = CBFreePingPong;
			len = CBPingPongDeserialise(CBGetPingPong(peer->receive));
			break;
		case CB_MESSAGE_TYPE_ALERT:
			peer->receive = realloc(peer->receive, sizeof(CBAlert));
			if (NOT peer->receive) {
				CBNetworkCommunicatorDisconnect(self, peer, 0, false);
				return;
			}
			CBGetObject(peer->receive)->free = CBFreeAlert;
			len = CBAlertDeserialise(CBGetAlert(peer->receive));
			break;
		default:
			len = 0; // Zero default
			break;
	}
	// Check that the deserialised message's size read matches the message length.
	if (NOT peer->receive->bytes) {
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
	CBOnMessageReceivedAction action = self->onMessageReceived(self->callbackHandler, self, peer);
	if (action == CB_MESSAGE_ACTION_CONTINUE) {
		// Automatic responses
		if (self->flags & CB_NETWORK_COMMUNICATOR_AUTO_HANDSHAKE)
			// Auto handshake responses
			action = CBNetworkCommunicatorProcessMessageAutoHandshake(self, peer);
		if (self->flags & CB_NETWORK_COMMUNICATOR_AUTO_DISCOVERY && action == CB_MESSAGE_ACTION_CONTINUE)
			// Auto discovery responses
			action = CBNetworkCommunicatorProcessMessageAutoDiscovery(self, peer);
		if (self->flags & CB_NETWORK_COMMUNICATOR_AUTO_PING && action == CB_MESSAGE_ACTION_CONTINUE)
			// Auto ping response
			action = CBNetworkCommunicatorProcessMessageAutoPingPong(self, peer);
	}
	// Release objects and get ready for next message
	switch (action) {
		case CB_MESSAGE_ACTION_DISCONNECT:
			// Node misbehaving. Disconnect.
			CBNetworkCommunicatorDisconnect(self, peer, CB_24_HOURS, false);
			break;
		case CB_MESSAGE_ACTION_STOP:
			CBNetworkCommunicatorStop(self);
			break;
		case CB_MESSAGE_ACTION_CONTINUE:
			CBReleaseObject(peer->receive);
			// Update "lastSeen"
			// First remove the peer from the array as is will be moved.
			// Retain as we still need the peer.
			CBRetainObject(peer);
			CBAddressManagerRemovePeer(self->addresses, peer);
			// Update lastSeen
			CBGetNetworkAddress(peer)->lastSeen = time(NULL);
			if (NOT CBAddressManagerTakePeer(self->addresses, peer)){
				CBLogError("Could not reinsert a peer when updating it's last seen timestamp.");
				CBNetworkCommunicatorDisconnect(self, peer, 0, false);
				break;
			}
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
	if (type == CB_TIMEOUT_RECEIVE && NOT peer->messageReceived && NOT peer->receivedHeader) {
		// Not responded
		if (peer->typesExpectedNum)
			type = CB_TIMEOUT_RESPONSE;
		else
			// No response expected but peer did not send any information for a long time.
			type = CB_TIMEOUT_NO_DATA;
	}
	// Remove CBNetworkAddress. 1 day penalty.
	CBNetworkCommunicatorDisconnect(self, peer, CB_24_HOURS, false);
	// Send event for network timeout. The peer will be NULL if it wasn't retained elsewhere.
	self->onTimeOut(self->callbackHandler, self, peer, type);
}
CBOnMessageReceivedAction CBNetworkCommunicatorProcessMessageAutoDiscovery(CBNetworkCommunicator * self, CBPeer * peer){
	if (peer->receive->type == CB_MESSAGE_TYPE_ADDR) {
		// Received addresses.
		CBAddressBroadcast * addrs = CBGetAddressBroadcast(peer->receive);
		// Only accept no timestamps when we have less than 1000 addresses.
		if(peer->versionMessage->version < CB_ADDR_TIME_VERSION
		   && self->addresses->addrNum > 1000)
			return CB_MESSAGE_ACTION_CONTINUE;
		// Loop through addresses and store them.
		bool didAdd = false; // True when we add an address.
		for (uint16_t x = 0; x < addrs->addrNum; x++) {
			// Check if we have the address as a connected peer
			CBPeer * peerB = CBAddressManagerGotPeer(self->addresses, addrs->addresses[x]);
			if (NOT peerB){
				// Do not already have this address as a peer
				if (addrs->addresses[x]->type & CB_IP_INVALID)
					// Address broadcasts should not contain invalid addresses.
					return CB_MESSAGE_ACTION_DISCONNECT;
				if (addrs->addresses[x]->type & CB_IP_LOCAL && NOT (CBGetNetworkAddress(peer)->type & CB_IP_LOCAL))
					// Do not allow peers to send local addresses to non-local peers.
					return CB_MESSAGE_ACTION_DISCONNECT;
				// Else leave the time
				// Check if we have the address as a stored address
				CBNetworkAddress * addr = CBAddressManagerGotNetworkAddress(self->addresses, addrs->addresses[x]);
				if (addr){
					// Already have the address. Modify the existing object.
					if (addrs->addresses[x]->lastSeen > addr->lastSeen){
						// The peer has seen it sooner than us, so update the timestamp.
						// First take out the address
						// Retain since we still need the address
						CBRetainObject(addr);
						CBAddressManagerRemoveAddress(self->addresses, addr);
						// Update the last seen timestamp
						addr->lastSeen = addrs->addresses[x]->lastSeen;
						// As we've updated the timestamp for when the address is last seen, we need to reinsert it.
						CBAddressManagerTakeAddress(self->addresses, addr);
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
					// Make copy of the address since otherwise we will corrupt the CBAddressBroadcast when trying the connection.
					CBByteArray * ipCopy = CBByteArrayCopy(addrs->addresses[x]->ip);
					if (ipCopy) {
						CBNetworkAddress * copy = CBNewNetworkAddress(addrs->addresses[x]->lastSeen, ipCopy, addrs->addresses[x]->port, addrs->addresses[x]->services, true);
						CBReleaseObject(ipCopy);
						if (copy){
							// See if the maximum number of addresses have been reached. If it has then remove an address before adding this one.
							if (self->addresses->addrNum > self->maxAddresses) {
								// This does not include peers, so there may actually be more stored addresses than the maxAddresses, as peers can be stored when public.
								CBNetworkAddress * addr = CBAddressManagerSelectAndRemoveAddress(self->addresses);
								// Delete from storage also.
								if (NOT CBAddressStorageDeleteAddress(self->addrStorage, addr))
									CBLogError("Could not remove an address from storage.");
							}
							// Add the address to the address manager and the storage (If the storage object is not NULL).
							CBAddressManagerAddAddress(self->addresses, copy);
							if (self->addrStorage)
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
				if (self->addrStorage)
					CBAddressStorageSaveAddress(self->addrStorage, CBGetNetworkAddress(peerB));
			}
		}
		if (NOT peer->getAddresses && addrs->addrNum < 10) {
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
					if (CBAssociativeArrayInsert(&self->relayedAddrs, addrs->addresses[x], res.position, NULL))
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
					CBPeer * peerToRelay = CBAddressManagerGetPeer(self->addresses, index);
					CBReleaseObject(peerToRelay);
					// Check that the peer is not the peer that sent us the broadcast and the peer is not contained in the broadcast
					bool inBroadcast = false;
					for (uint8_t x = 0; x < addrs->addrNum; x++)
						if (CBNetworkAddressCompare(addrs->addresses[x], peerToRelay) == CB_COMPARE_EQUAL)
							inBroadcast = true;
					if (peerToRelay != peer && NOT inBroadcast) {
						// Relay the broadcast
						CBNetworkCommunicatorSendMessage(self, peerToRelay, CBGetMessage(addrs));
						if (NOT --peersToBeGiven)
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
		CBAddressBroadcast * addrBroadcast = CBNewAddressBroadcast(self->version >= CB_ADDR_TIME_VERSION && peer->versionMessage->version >= CB_ADDR_TIME_VERSION);
		if (NOT addrBroadcast)
			return CB_MESSAGE_ACTION_DISCONNECT;
		CBGetMessage(addrBroadcast)->type = CB_MESSAGE_TYPE_ADDR;
		// Try connected peers. Only send peers that are public (private addresses are those which connect to us but haven't relayed their address).
		uint8_t peersSent = 0;
		for (uint32_t y = 0; peersSent < 28 && y < self->addresses->peersNum; y++) { // 28 to have room for 5 addresses.
			// Get the peer
			CBPeer * peerToInclude = CBAddressManagerGetPeer(self->addresses, y);
			if (peerToInclude != peer // Not the peer we are sending to
				&& CBGetNetworkAddress(peerToInclude)->isPublic // Public
				&& (CBGetNetworkAddress(peerToInclude)->type != CB_IP_LOCAL // OK if not local
					|| CBGetNetworkAddress(peerToInclude)->type == CB_IP_LOCAL)) { // Or if the peer we are sending to is local
				if (NOT CBAddressBroadcastAddNetworkAddress(addrBroadcast, CBGetNetworkAddress(peerToInclude))){
					// Memory problem. Free space by disconnecting node.
					CBLogError("Could not add a peer to an address broadcast.");
					CBReleaseObject(addrBroadcast);
					return CB_MESSAGE_ACTION_DISCONNECT;
				}
				peersSent++;
			}
			// Release peer
			CBReleaseObject(peerToInclude);
		}
		// Now add stored addresses
		uint8_t numAddrs = 33 - peersSent;
		CBNetworkAddress * addrs[numAddrs];
		numAddrs = CBAddressManagerGetAddresses(self->addresses, numAddrs, addrs);
		for (uint8_t x = 0; x < numAddrs; x++){
			if ((addrs[x]->type != CB_IP_LOCAL
				|| CBGetNetworkAddress(peer)->type == CB_IP_LOCAL)
				// If the address is not local or if the peer we are sending to is local, the address is acceptable.
				&& NOT CBAddressBroadcastAddNetworkAddress(addrBroadcast, addrs[x])){
				CBLogError("Could not add a network address to an address broadcast.");
				CBReleaseObject(addrBroadcast);
				for (uint8_t y = x; y < numAddrs; y++)
					CBReleaseObject(addrs[y]);
				return CB_MESSAGE_ACTION_DISCONNECT;
			}
			CBReleaseObject(addrs[x]);
		}
		// Send address broadcast, if we have at least one.
		if (addrBroadcast->addrNum)
			CBNetworkCommunicatorSendMessage(self, peer, CBGetMessage(addrBroadcast));
		CBReleaseObject(addrBroadcast);
	}
	// Use opportunity to discover if we should broadcast our own addresses for recieving incoming connections.
	if (self->maxIncommingConnections // Only share address if we allow for incomming connections.
		&& (self->isListeningIPv4 || self->isListeningIPv6) // Share when we are listening for incoming connections.
		&& peer->timeBroadcast < time(NULL) - 86400 // Every 24 hours
		&& peer->acceptedTypes & CB_MESSAGE_TYPE_ADDR) { // Only share address if they are allowed.
		peer->timeBroadcast = time(NULL);
		CBAddressBroadcast * addrBroadcast = CBNewAddressBroadcast(self->version >= CB_ADDR_TIME_VERSION && peer->versionMessage->version >= CB_ADDR_TIME_VERSION);
		if (NOT addrBroadcast)
			return CB_MESSAGE_ACTION_DISCONNECT;
		CBGetMessage(addrBroadcast)->type = CB_MESSAGE_TYPE_ADDR;
		if (self->ourIPv6) {
			self->ourIPv6->lastSeen = time(NULL);
			if(NOT CBAddressBroadcastAddNetworkAddress(addrBroadcast, self->ourIPv6)){
				// Memory issue
				CBReleaseObject(addrBroadcast);
				return CB_MESSAGE_ACTION_DISCONNECT;
			}
		}
		if (self->ourIPv4) {
			self->ourIPv4->lastSeen = time(NULL);
			if(NOT CBAddressBroadcastAddNetworkAddress(addrBroadcast, self->ourIPv4)){
				CBReleaseObject(addrBroadcast);
				return CB_MESSAGE_ACTION_DISCONNECT;
			}
		}
		CBNetworkCommunicatorSendMessage(self, peer, CBGetMessage(addrBroadcast));
		CBReleaseObject(addrBroadcast);
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
			// Check if we have a connection to this peer already
			CBPeer * peerCheck = CBAddressManagerGotPeer(self->addresses, CBGetVersion(peer->receive)->addSource);
			if (peerCheck){
				// Release the peer check
				CBReleaseObject(peerCheck);
				// If the peer we have got is this peer then we know that this can be the only connection, as the address has been previously known.
				if (peerCheck != peer)
					// Disconnect this connection, as we already have this connection.
					return CB_MESSAGE_ACTION_DISCONNECT;
			}
			// Remove the source address from the address manager if it has it. Now that the peer is connected, we identify it by the source address and do not want to try connections to the same address.
			CBAddressManagerRemoveAddress(self->addresses, CBGetNetworkAddress(CBGetVersion(peer->receive)->addSource));
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
			CBAddressManagerRemovePeer(self->addresses, peer);
			// Change port and IP number to the port and IP the peer wants us to recognise them with.
			CBGetNetworkAddress(peer)->port = peer->versionMessage->addSource->port;
			if (CBByteArrayCompare(CBGetNetworkAddress(peer)->ip, peer->versionMessage->addSource->ip) != CB_COMPARE_EQUAL) {
				// The IP is different so change it.
				CBReleaseObject(CBGetNetworkAddress(peer)->ip);
				CBGetNetworkAddress(peer)->ip = peer->versionMessage->addSource->ip;
			}
			// Now re-insert the peer with the new information
			if (NOT CBAddressManagerTakePeer(self->addresses, peer)){
				// Make the connection not working, so no attempt is made to remove it from the address manager.
				peer->connectionWorking = false;
				CBLogError("Could not re-insert a peer when updating the information from it's version message.");
				return CB_MESSAGE_ACTION_DISCONNECT;
			}
			// Set the IP type
			CBGetNetworkAddress(peer)->type = CBGetIPType(CBByteArrayGetData(CBGetNetworkAddress(peer)->ip));
			// Adjust network time
			peer->timeOffset = peer->versionMessage->time - time(NULL); // Set the time offset for this peer.
			// Now we have the network time, add the peer for the time offset. Ignore on failure
			CBAddressManagerTakePeerTimeOffset(self->addresses, peer);
			// Disallow version from here.
			peer->acceptedTypes &= ~CB_MESSAGE_TYPE_VERSION;
			// Send our acknowledgement
			CBMessage * ack = CBNewMessageByObject();
			if (NOT ack)
				return CB_MESSAGE_ACTION_DISCONNECT;
			ack->type = CB_MESSAGE_TYPE_VERACK;
			bool ok = CBNetworkCommunicatorSendMessage(self, peer, ack);
			CBReleaseObject(ack);
			if (NOT ok)
				return CB_MESSAGE_ACTION_DISCONNECT; // If we cannot send an acknowledgement, exit.
			// Send version next if we have not already.
			if (NOT peer->versionSent) {
				CBVersion * version = CBNetworkCommunicatorGetVersion(self, CBGetNetworkAddress(peer));
				if (version) {
					CBGetMessage(version)->expectResponse = CB_MESSAGE_TYPE_VERACK; // Expect a response for this message.
					peer->acceptedTypes = CB_MESSAGE_TYPE_VERACK; // Ony allow version acknowledgement from here.
					CBGetMessage(version)->type = CB_MESSAGE_TYPE_VERSION;
					bool ok = CBNetworkCommunicatorSendMessage(self, peer, CBGetMessage(version));
					CBReleaseObject(version);
					if (NOT ok)
						return CB_MESSAGE_ACTION_DISCONNECT;
					// As we are sending the version we don't want to do that again.
					peer->versionSent = true;
				}else
					return CB_MESSAGE_ACTION_DISCONNECT;
			}else
				// Sent already. End of handshake
				endHandshake = true;
		}
	}else if (peer->receive->type == CB_MESSAGE_TYPE_VERACK) {
		peer->versionAck = true;
		peer->acceptedTypes &= ~CB_MESSAGE_TYPE_VERACK; // Got the version acknowledgement. Do not need it again
		if (peer->versionMessage)
			// We already have their version message. That's all!
			endHandshake = true;
		else
			// We need their version.
			peer->acceptedTypes |= CB_MESSAGE_TYPE_VERSION;
	}
	if (endHandshake) {
		// The end of the handshake. Allow pings, inventory advertisments, address requests, address broadcasts and alerts.
		peer->acceptedTypes |= CB_MESSAGE_TYPE_PING;
		peer->acceptedTypes |= CB_MESSAGE_TYPE_INV;
		peer->acceptedTypes |= CB_MESSAGE_TYPE_GETADDR;
		peer->acceptedTypes |= CB_MESSAGE_TYPE_ADDR;
		peer->acceptedTypes |= CB_MESSAGE_TYPE_ALERT;
		if (self->flags & CB_NETWORK_COMMUNICATOR_AUTO_DISCOVERY) {
			// Request addresses
			CBMessage * getaddr = CBNewMessageByObject();
			if (NOT getaddr)
				return  CB_MESSAGE_ACTION_DISCONNECT;
			getaddr->type = CB_MESSAGE_TYPE_GETADDR;
			// Send the message without an expected response. We do not expect an "addr" message because the peer may not send us anything if they do not have addresses to give.
			bool ok = CBNetworkCommunicatorSendMessage(self, peer, getaddr);
			CBReleaseObject(getaddr);
			if (NOT ok)
				return CB_MESSAGE_ACTION_DISCONNECT;
			peer->getAddresses = true;
		}
	}
	return CB_MESSAGE_ACTION_CONTINUE;
}
CBOnMessageReceivedAction CBNetworkCommunicatorProcessMessageAutoPingPong(CBNetworkCommunicator * self, CBPeer * peer){
	if (peer->receive->type == CB_MESSAGE_TYPE_PING
		&& peer->versionMessage->version >= CB_PONG_VERSION
		&& self->version >= CB_PONG_VERSION){
		peer->receive->type = CB_MESSAGE_TYPE_PONG;
		if(NOT CBNetworkCommunicatorSendMessage(self, peer, peer->receive))
			return CB_MESSAGE_ACTION_DISCONNECT;
	}else if (peer->receive->type == CB_MESSAGE_TYPE_PONG){
		if(self->version < CB_PONG_VERSION || peer->versionMessage->version < CB_PONG_VERSION)
			return CB_MESSAGE_ACTION_DISCONNECT; // This peer should not be sending pong messages.
		// No longer want a pong
		peer->acceptedTypes &= ~CB_MESSAGE_TYPE_PONG;
	}
	return CB_MESSAGE_ACTION_CONTINUE;
}
bool CBNetworkCommunicatorSendMessage(CBNetworkCommunicator * self, CBPeer * peer, CBMessage * message){
	if (peer->sendQueueSize == CB_SEND_QUEUE_MAX_SIZE)
		return false;
	// Serialise message if needed.
	if (NOT message->serialised) {
		uint32_t len;
		switch (message->type) {
			case CB_MESSAGE_TYPE_VERSION:
				len = CBVersionCalculateLength(CBGetVersion(message));
				if (NOT len)
					return false;
				message->bytes = CBNewByteArrayOfSize(len);
				if (NOT message->bytes)
					return false;
				len = CBVersionSerialise(CBGetVersion(message), false);
				break;
			case CB_MESSAGE_TYPE_ADDR:
				// "CBAddressBroadcastCalculateLength" cannot fail.
				message->bytes = CBNewByteArrayOfSize(CBAddressBroadcastCalculateLength(CBGetAddressBroadcast(message)));
				if (NOT message->bytes)
					return false;
				len = CBAddressBroadcastSerialise(CBGetAddressBroadcast(message), false);
				break;
			case CB_MESSAGE_TYPE_INV:
			case CB_MESSAGE_TYPE_GETDATA:
				// "CBInventoryBroadcastCalculateLength" cannot fail.
				message->bytes = CBNewByteArrayOfSize(CBInventoryBroadcastCalculateLength(CBGetInventoryBroadcast(message)));
				if (NOT message->bytes)
					return false;
				len = CBInventoryBroadcastSerialise(CBGetInventoryBroadcast(message), false);
				break;
			case CB_MESSAGE_TYPE_GETBLOCKS:
			case CB_MESSAGE_TYPE_GETHEADERS:
				// "CBGetBlocksCalculateLength" cannot fail.
				message->bytes = CBNewByteArrayOfSize(CBGetBlocksCalculateLength(CBGetGetBlocks(message)));
				if (NOT message->bytes)
					return false;
				len = CBGetBlocksSerialise(CBGetGetBlocks(message), false);
				break;
			case CB_MESSAGE_TYPE_TX:
				len = CBTransactionCalculateLength(CBGetTransaction(message));
				if (NOT len)
					return false;
				message->bytes = CBNewByteArrayOfSize(len);
				if (NOT message->bytes)
					return false;
				len = CBTransactionSerialise(CBGetTransaction(message), false);
				break;
			case CB_MESSAGE_TYPE_BLOCK:
				len = CBBlockCalculateLength(CBGetBlock(message), true);
				if (NOT len)
					return false;
				message->bytes = CBNewByteArrayOfSize(len);
				if (NOT message->bytes)
					return false;
				len = CBBlockSerialise(CBGetBlock(message), true, false); // true -> Including transactions.
				break;
			case CB_MESSAGE_TYPE_HEADERS:
				// "CBBlockHeadersCalculateLength" cannot fail.
				message->bytes = CBNewByteArrayOfSize(CBBlockHeadersCalculateLength(CBGetBlockHeaders(message)));
				if (NOT message->bytes)
					return false;
				len = CBBlockHeadersSerialise(CBGetBlockHeaders(message), false);
				break;
			case CB_MESSAGE_TYPE_PING:
				if (peer->versionMessage->version >= 60000 && self->version >= 60000){
					message->bytes = CBNewByteArrayOfSize(8);
					if (NOT message->bytes)
						return false;
					len = CBPingPongSerialise(CBGetPingPong(message));
				}
			case CB_MESSAGE_TYPE_PONG:
				message->bytes = CBNewByteArrayOfSize(8);
				if (NOT message->bytes)
					return false;
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
		message->checksum[0] = hash2[0];
		message->checksum[1] = hash2[1];
		message->checksum[2] = hash2[2];
		message->checksum[3] = hash2[3];
	}else{
		// Empty bytes checksum
		message->checksum[0] = 0x5D;
		message->checksum[1] = 0xF6;
		message->checksum[2] = 0xE0;
		message->checksum[3] = 0xE2;
	}
	peer->sendQueue[(peer->sendQueueFront + peer->sendQueueSize) % CB_SEND_QUEUE_MAX_SIZE] = message; // Node will send the message from this.
	if (peer->sendQueueSize == 0){
		bool ok = CBSocketAddEvent(peer->sendEvent, self->sendTimeOut);
		if (NOT ok)
			return false;
	}
	peer->sendQueueSize++;
	CBRetainObject(message);
	return true;
}
void CBNetworkCommunicatorSendPings(void * vself){
	CBNetworkCommunicator * self = vself;
	CBMessage * ping = CBNewMessageByObject();
	if (NOT ping)
		return;
	ping->type = CB_MESSAGE_TYPE_PING;
	CBPosition it;
	if (self->version >= CB_PONG_VERSION) {
		CBPingPong * pingPong = CBNewPingPong(rand());
		CBGetMessage(pingPong)->type = CB_MESSAGE_TYPE_PING;
		if (CBAssociativeArrayGetFirst(&self->addresses->peers, &it)) for (;;) {
			CBPeer * peer = it.node->elements[it.index];
			if (peer->acceptedTypes & CB_MESSAGE_TYPE_PING){ // Only send ping if they can send ping.
				if (peer->versionMessage->version >= CB_PONG_VERSION){
					CBNetworkCommunicatorSendMessage(self, peer, CBGetMessage(pingPong));
					CBGetMessage(peer)->expectResponse = CB_MESSAGE_TYPE_PONG; // Expect a pong.
					peer->acceptedTypes |= CB_MESSAGE_TYPE_PONG;
				}else
					CBNetworkCommunicatorSendMessage(self, peer, CBGetMessage(ping));
			}
			if (CBAssociativeArrayIterate(&self->addresses->peers, &it))
				break;
		}
		CBReleaseObject(pingPong);
	}else if (CBAssociativeArrayGetFirst(&self->addresses->peers, &it)) for (;;) {
		CBPeer * peer = it.node->elements[it.index];
		// Only send ping if they can send ping.
		if (peer->acceptedTypes & CB_MESSAGE_TYPE_PING)
			CBNetworkCommunicatorSendMessage(self, peer, CBGetMessage(ping));
		if (CBAssociativeArrayIterate(&self->addresses->peers, &it))
			break;
	}
	CBReleaseObject(ping);
}
void CBNetworkCommunicatorSetAddressManager(CBNetworkCommunicator * self, CBAddressManager * addrMan){
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
	if (NOT CBNewEventLoop(&self->eventLoop, CBNetworkCommunicatorOnLoopError, CBNetworkCommunicatorOnTimeOut, self)){
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
				if (NOT self->isListeningIPv4)
					// Failure to add event
					CBSocketFreeEvent(self->acceptEventIPv4);
			}
		}
		if (NOT self->isListeningIPv4)
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
				if (NOT self->isListeningIPv6)
					// Failure to add event
					CBSocketFreeEvent(self->acceptEventIPv6);
			}
		}
		if (NOT self->isListeningIPv6)
			// Failure to create event
			CBCloseSocket(self->listeningSocketIPv6);
	}
}
void CBNetworkCommunicatorStartPings(CBNetworkCommunicator * self){
	CBStartTimer(self->eventLoop, &self->pingTimer, self->heartBeat, CBNetworkCommunicatorSendPings, self);
}
void CBNetworkCommunicatorStop(CBNetworkCommunicator * self){
	if (self->isListeningIPv4 || self->isListeningIPv6)
		CBNetworkCommunicatorStopListening(self);
	self->isStarted = false;
	// Disconnect all the peers, this
	CBPosition it;
	if (CBAssociativeArrayGetFirst(&self->addresses->peers, &it)) for (;;){
		CBNetworkCommunicatorDisconnect(self, it.node->elements[it.index], 0, true); // "true" we are stopping.
		if (CBAssociativeArrayIterate(&self->addresses->peers, &it))
			break;
	}
	// Now reset the peers arrays. The addresses were released in CBNetworkCommunicatorDisconnect, so this function only clears the array nodes.
	CBAddressManagerClearPeers(self->addresses);
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
	if (self->pingTimer) CBEndTimer(self->pingTimer);
	self->pingTimer = 0;
}
void CBNetworkCommunicatorTryConnections(CBNetworkCommunicator * self){
	if (self->attemptingOrWorkingConnections >= self->maxConnections)
		return; // Cannot connect to any more peers
	uint16_t connectTo = self->maxConnections - self->attemptingOrWorkingConnections;
	uint32_t addrNum = self->addresses->addrNum;
	if (addrNum < connectTo)
		connectTo = addrNum; // Cannot connect to any more than the address we have
	CBNetworkAddress ** addrs = malloc(sizeof(*addrs) * connectTo);
	if (NOT addrs){
		CBLogError("Could not allocate memory for an array to hold addresses to connect to.");
		return;
	}
	connectTo = CBAddressManagerGetAddresses(self->addresses, connectTo, addrs);
	for (uint64_t x = 0; x < connectTo; x++) {
		// Remove the address from the address manager
		CBAddressManagerRemoveAddress(self->addresses, addrs[x]);
		// We haven't got the address as a peer.
		// Convert network address into peer
		CBPeer * peer = CBNewPeerByTakingNetworkAddress(addrs[x]);
		if (NOT peer){
			CBLogError("Could not convert a network address to a peer for connecting to it.");
			free(addrs);
			return;
		}
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
			CBAddressManagerTakeAddress(self->addresses, addrs[x]);
		}
		// Either the connection was OK and it should either timeout or finalise, or we forget about it because it is not supported.
	}
	// Free address pointer memory.
	free(addrs);
}
