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

CBNetworkCommunicator * CBNewNetworkCommunicator(u_int32_t networkID,CBNetworkCommunicatorFlags flags,CBVersion * version,u_int16_t maxConnections,u_int16_t heartBeat,u_int16_t timeOut,CBByteArray * alternativeMessages,u_int32_t * altMaxSizes,u_int16_t port,CBEvents * events){
	CBNetworkCommunicator * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeNetworkCommunicator;
	bool res = CBInitNetworkCommunicator(self,networkID,flags,version,maxConnections,heartBeat,timeOut,alternativeMessages,altMaxSizes,port,events);
	if (res) {
		return self;
	}else{
		free(self);
		return NULL;
	}
}

//  Object Getter

CBNetworkCommunicator * CBGetNetworkCommunicator(void * self){
	return self;
}

//  Initialiser

bool CBInitNetworkCommunicator(CBNetworkCommunicator * self,u_int32_t networkID,CBNetworkCommunicatorFlags flags,CBVersion * version,u_int16_t maxConnections,u_int16_t heartBeat,u_int16_t timeOut,CBByteArray * alternativeMessages,u_int32_t * altMaxSizes,u_int16_t port,CBEvents * events){
	// Set fields.
	self->networkID = networkID;
	self->flags = flags;
	self->version = version;
	CBRetainObject(version);
	self->maxConnections = maxConnections;
	self->heartBeat = heartBeat;
	self->timeOut = timeOut;
	self->alternativeMessages = alternativeMessages;
	self->altMaxSizes = altMaxSizes;
	CBRetainObject(alternativeMessages);
	self->nodes = malloc(maxConnections); // Must never accept any more connections
	self->nodesNum = 0;
	self->addresses = NULL;
	self->addrNum = 0;
	self->port = port;
	self->events = events;
	self->eventLoop = 0;
	self->acceptEventIPv4 = 0;
	self->acceptEventIPv6 = 0;
	// Assume we can connect to IPv4 and IPv6 to begin with.
	self->IPv4 = true;
	self->IPv6 = true;
	self->isListeningIPv4 = false;
	self->networkTimeOffset = 0;
	// Create "accessNodeListMutex"
	if(!CBNewMutex(&self->accessNodeListMutex)){
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_NETWORK_COMMUNICATOR_NODE_LIST_MUTEX_CREATE_FAIL,"The CBNetworkCommunicator 'accessNodeListMutex' could not be created.");
		return false;
	}
	// Create "accessAddressListMutex"
	if(!CBNewMutex(&self->accessAddressListMutex)){
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_NETWORK_COMMUNICATOR_ADDR_LIST_MUTEX_CREATE_FAIL,"The CBNetworkCommunicator 'accessAddressListMutex' could not be created.");
		CBFreeMutex(self->accessNodeListMutex);
		return false;
	}
	if (!CBInitObject(CBGetObject(self))){
		CBFreeMutex(self->accessNodeListMutex);
		CBFreeMutex(self->accessAddressListMutex);
		return false;
	}
	return true;
}

//  Destructor

void CBFreeNetworkCommunicator(void * vself){
	// When this is called all references to the object should be released, so no additional syncronisation is required
	CBNetworkCommunicator * self = vself;
	if (self->isStarted) {
		CBNetworkCommunicatorStop(self); // In this function the node mutex lock is required but no more is the node list mutex lock required. ??? Add boolean to prevent mutex lock of node list or not worthwhile?
	}
	CBReleaseObject(&self->version);
	CBReleaseObject(&self->alternativeMessages);
	free(self->nodes); // No mutex lock required here. No more threads should be accessing the list except this one.
	CBFreeMutex(self->accessNodeListMutex);
	CBFreeMutex(self->accessAddressListMutex);
	free(self->altMaxSizes);
	CBFreeObject(self);
}

//  Functions

void CBNetworkCommunicatorAcceptConnection(void * vself,u_int64_t socket){
	CBNetworkCommunicator * self = vself;
	u_int8_t * IP = malloc(16);
	u_int64_t connectSocketID;
	bool result = CBSocketAccept(socket, &connectSocketID, IP);
	if (!result) {
		// No luck accepting the connection.
		free(IP);
		return;
	}
	// Connected, add CBNetworkAddress
	CBByteArray * ipByteArray = CBNewByteArrayWithData(IP, 16, self->events);
	CBNode * node = CBNewNodeByTakingNetworkAddress(CBNewNetworkAddress(0, ipByteArray, self->port, 0, self->events));
	CBReleaseObject(&ipByteArray);
	if (!node) 
		// No luck with that node I'm afraid. What happened is the mutex could not be created for the node.
		return;
	node->socketID = connectSocketID;
	node->acceptedTypes = CB_MESSAGE_TYPE_VERSION; // The node connected to us, we want the version from them.
	// Set up receive event
	node->receiveEvent = CBSocketCanReceiveEvent(self->eventLoop,node->socketID, CBNetworkCommunicatorOnCanReceive, node);
	if (node->receiveEvent) {
		// The event works
		if(CBSocketAddEvent(node->receiveEvent, self->timeOut)){ // Begin receive event.
			// Success
			node->sendEvent = CBSocketCanSendEvent(self->eventLoop, node->socketID, CBNetworkCommunicatorOnCanSend, node);
			if (node->sendEvent) {
				// Both events work. Take the node and return.
				CBNetworkCommunicatorTakeNode(self, node);
				return;
			}
		}
		// Could create receive event but there was a failure afterwards so free it.
		CBSocketFreeEvent(node->receiveEvent);
	}
	// Failure, release node.
	CBReleaseObject(node);
}
void CBNetworkCommunicatorAdjustTime(CBNetworkCommunicator * self,u_int64_t time){
	int64_t maxOffset = 0;
	int64_t minOffset = UINT64_MAX;
	for (u_int16_t x = 0; x < self->nodesNum; x++)
		if (self->nodes[x]->versionMessage) { // Only get time from nodes that have given a version message.
			if (self->nodes[x]->timeOffset > maxOffset)
				maxOffset = self->nodes[x]->timeOffset;
			else if (self->nodes[x]->timeOffset < minOffset)
				minOffset = self->nodes[x]->timeOffset;
		}
	int16_t median = (maxOffset + minOffset)/2;
	if (median > CB_NETWORK_TIME_ALLOWED_TIME_DRIFT) {
		// Revert to system time
		self->networkTimeOffset = 0;
		// Check to see if any nodes are within 5 minutes of the system time and do not have the same time (why???), else give error
		bool found = false;
		for (u_int16_t x = 0; x < self->nodesNum; x++)
			if (self->nodes[x]->timeOffset < 300)
				found = true;
		if (!found)
			self->events->onBadTime();
	}else
		self->networkTimeOffset = median;
}
bool CBNetworkCommunicatorCanConnect(CBNetworkCommunicator * self,CBNode * node){
	if (memcmp(CBGetNetworkAddress(node)->ip, (u_int8_t []){0,0,0,0,0,0,0,0,0,0,0xFF,0xFF}, 12)){
		if (self->IPv6)
			return true;
	}else{
		if (self->IPv4)
			return true;
	}
	return false;
}
bool CBNetworkCommunicatorConnect(CBNetworkCommunicator * self,CBNode * node){
	// Determine if IPv4 or IPv6
	bool isIPv6 = false;
	if (memcmp(CBGetNetworkAddress(node)->ip, (u_int8_t []){0,0,0,0,0,0,0,0,0,0,0xFF,0xFF}, 12)) {
		// Is IPv6
		isIPv6 = true;
	}
	// Make socket
	if(!CBNewSocket(&node->socketID,isIPv6)){
		if (isIPv6) self->IPv6 = false; 
		else self->IPv4 = false;
		if (!self->IPv6 && !self->IPv4 && !self->nodesNum)
			self->events->onNetworkError();
		return false;
	}
	// Add event for connection
	node->connectEvent = CBSocketDidConnectEvent(self->eventLoop, node->socketID, CBNetworkCommunicatorDidConnect, node);
	if (node->connectEvent) {
		if (CBSocketAddEvent(node->connectEvent, self->connectionTimeOut)) {
			// Connect
			if(CBSocketConnect(node->socketID, CBByteArrayGetData(CBGetNetworkAddress(node)->ip), isIPv6, CBGetNetworkAddress(node)->port)){
				// Connection is fine
				return true;
			}else{
				if (isIPv6) self->IPv6 = false; 
				else self->IPv4 = false;
				if (!self->IPv6 && !self->IPv4 && !self->nodesNum)
					self->events->onNetworkError();
			}
		}
		CBSocketFreeEvent(node->connectEvent);
	}
	CBCloseSocket(node->socketID);
	return false;
}
void CBNetworkCommunicatorDidConnect(void * vself,void * vnode){
	CBNetworkCommunicator * self = vself;
	CBNode * node = vnode;
	CBSocketFreeEvent(node->connectEvent); // No longer need this event.
	// Make receive event
	node->receiveEvent = CBSocketCanReceiveEvent(self->eventLoop, node->socketID, CBNetworkCommunicatorOnCanReceive, node);
	if (node->receiveEvent) {
		if (CBSocketAddEvent(node->receiveEvent, self->responseTimeOut)) {
			// Make send event
			node->sendEvent = CBSocketCanSendEvent(self->eventLoop, node->socketID, CBNetworkCommunicatorOnCanSend, node);
			if (node->sendEvent) {
				if (CBSocketAddEvent(node->sendEvent, self->sendTimeOut)) {
					CBNetworkCommunicatorTakeNode(self, node);
					// Connection OK, so begin handshake if auto handshaking is enabled
					if (self->flags & CB_NETWORK_COMMUNICATOR_AUTO_HANDSHAKE)
						CBNetworkCommunicatorSendMessage(self, node, CBGetMessage(self->version));
					return;
				}
				CBSocketFreeEvent(node->sendEvent);
			}
		}
		CBSocketFreeEvent(node->receiveEvent);
	}
}
CBNetworkAddress * CBNetworkCommunicatorGotNetworkAddress(CBNetworkCommunicator * self,CBNetworkAddress * addr){
	for (u_int16_t x = 0; x < self->addrNum; x++) {
		if (CBByteArrayEquals(self->addresses[x]->ip, addr->ip)) {
			return self->addresses[x];
		}
	}
	return NULL;
}
CBNode * CBNetworkCommunicatorGotNode(CBNetworkCommunicator * self,CBNetworkAddress * addr){
	for (u_int16_t x = 0; x < self->nodesNum; x++) {
		if (CBByteArrayEquals(CBGetNetworkAddress(self->nodes[x])->ip, addr->ip)) {
			return self->nodes[x];
		}
	}
	return NULL;
}
bool CBNetworkCommunicatorProcessMessageAutoDiscovery(CBNetworkCommunicator * self,CBNode * node){
	if (node->receive->type == CB_MESSAGE_TYPE_ADDR) {
		// Received addresses. 
		CBAddressBroadcast * addrs = CBGetAddressBroadcast(node->receive);
		u_int64_t now = time(NULL) + self->networkTimeOffset;
		// Loop through addresses
		for (u_int16_t x = 0; x < addrs->addrNum; x++) {
			// Test if the version doesn't provide timestamps in address messages and we already have 1000 addresses. If so, return now. It is the way bitcoin-qt does it.
			if(node->versionMessage->version < CB_ADDR_TIME_VERSION 
			 && self->addrNum + self->nodesNum > 1000)
				return false;
			// Check if we have the address as a connected node
			CBNode * node = CBNetworkCommunicatorGotNode(self,addrs->addresses[x]);
			if(!node){
				// Do not already have this address as a node
				// Adjust time priority
				if (addrs->addresses[x]->time > now + 600)
					// Address is advertised more than ten minutes from now. Give low priority at 5 days ago
					addrs->addresses[x]->time = (u_int32_t)(now - 432000); // ??? Needs fixing for integer overflow in the year 2106.
				else if (addrs->addresses[x]->time > now - 900) // More than fifteen minutes ago. Give highest priority.
					addrs->addresses[x]->time = (u_int32_t)now;
				// Check if we have the address as a stored address
				// Else leave the time
				CBNetworkAddress * addr = CBNetworkCommunicatorGotNetworkAddress(self,addrs->addresses[x]);
				if(addr){
					// Already have the address. Modify the existing object.
					addr->time = addrs->addresses[x]->time;
					addr->services = addrs->addresses[x]->services;
					addr->version = addrs->addresses[x]->version;
				}else{
					// Do not have the address so add it if we believe we can connect to it.
					if (CBNetworkCommunicatorCanConnect(self,node)) {
						CBRetainObject(addrs->addresses[x]); // The CBMessage will be released with this CBNetworkAddress.
						CBNetworkCommunicatorTakeAddress(self, addrs->addresses[x]);
					}
				}
			}
		}
		if (!node->getAddresses && addrs->addrNum < 10 && self->nodesNum < 2) {
			// Unsolicited addresses. Less than ten so relay to two random nodes. bitcoin-qt does something weird. We wont. Keep it simple... Right ???
			for (u_int16_t x = rand() % (self->nodesNum - 1),y = 0; x < self->nodesNum || y == 2; x++,y++) {
				if (self->nodes[x] == node)
					continue;
				CBNetworkCommunicatorSendMessage(self, self->nodes[x], CBGetMessage(addrs));
			}
		}
		if (node->getAddresses) node->getAddresses = false; // Got addresses.
	}else if (node->receive->type == CB_MESSAGE_TYPE_GETADDR) {
		// Give 33 nodes with the highest times with a some randomisation added.
		CBAddressBroadcast * addrBroadcast = CBNewAddressBroadcast(self->version->version >= CB_ADDR_TIME_VERSION, self->events);
		CBGetMessage(addrBroadcast)->type = CB_MESSAGE_TYPE_ADDR;
		if (self->addrNum < 34)
			// Send all.
			for (u_int16_t x = 0; x < self->addrNum; x++)
				CBAddressBroadcastAddNetworkAddress(addrBroadcast, self->addresses[x]);
		else{
			// Send only 33
			u_int16_t i = self->addrNum - 1;
			for (u_int16_t x = 0; x < 33; x++) {
				u_int16_t maxMovement = i + x - 32; // Maximum movement downwards allowed such that 33 nodes can be produced.
				u_int16_t movement = rand() % maxMovement;
				// Bias towards small numbers
				movement *= movement;
				movement /= maxMovement;
				i -= movement;
				CBAddressBroadcastAddNetworkAddress(addrBroadcast, self->addresses[i]);
			}
		}
		// Send address broadcast
		CBNetworkCommunicatorSendMessage(self, node, CBGetMessage(addrBroadcast));
		CBReleaseObject(&addrBroadcast);
	}
	return false; // Do not disconnect.
}
bool CBNetworkCommunicatorProcessMessageAutoHandshake(CBNetworkCommunicator * self,CBNode * node){
	if (node->receive->type == CB_MESSAGE_TYPE_VERSION) {
		// Node sent us their version. How very nice of them.
		// Check version and nounce.
		if (CBGetVersion(node->receive)->version < CB_MIN_PROTO_VERSION
			|| (CBGetVersion(node->receive)->nounce == self->nounce && self->nounce > 0))
			// Disconnect node
			return true;
		else{ // Version OK
			// Save version message
			node->versionMessage = CBGetVersion(node->receive);
			CBRetainObject(node->versionMessage);
			// Change version 10300 to 300
			if (node->versionMessage->version == 10300) {
				node->versionMessage->version = 300;
			}
			// Copy over reported version, time and services for the CBNetworkAddress.
			CBGetNetworkAddress(node)->version = node->versionMessage->version;
			CBGetNetworkAddress(node)->time = (u_int32_t)node->versionMessage->time; // ??? Loss of integer precision.
			CBGetNetworkAddress(node)->services = node->versionMessage->services;
			// Adjust network time
			node->timeOffset = time(NULL) - node->versionMessage->time; // Set the time offset for this node.
			// Disallow version from here.
			node->acceptedTypes &= ~CB_MESSAGE_TYPE_VERSION;
			// Send our acknowledgement
			CBMessage * ack = CBNewMessageByObject(self->events);
			ack->type = CB_MESSAGE_TYPE_VERACK;
			if(!CBNetworkCommunicatorSendMessage(self,node,ack))
				return true; // If we cannot send an acknowledgement, exit.
			CBReleaseObject(&ack);
			// Send version next.
			CBNetworkCommunicatorSendMessage(self, node, CBGetMessage(self->version));
		}
	}else if (node->receive->type == CB_MESSAGE_TYPE_VERACK) {
		node->versionAck = true;
		node->acceptedTypes &= ~CB_MESSAGE_TYPE_VERACK; // Got the version acknowledgement. Do not need it again
		if (node->versionMessage) {
			// We already have their version message. The end of the handshake. Allow pings, inventory advertisments, address requests, address broadcasts and alerts.
			node->acceptedTypes |= CB_MESSAGE_TYPE_PING;
			node->acceptedTypes |= CB_MESSAGE_TYPE_INV;
			node->acceptedTypes |= CB_MESSAGE_TYPE_GETADDR;
			node->acceptedTypes |= CB_MESSAGE_TYPE_ADDR;
			node->acceptedTypes |= CB_MESSAGE_TYPE_ALERT;
			// Request addresses
			CBMessage * getaddr = CBNewMessageByObject(self->events);
			getaddr->type = CB_MESSAGE_TYPE_GETADDR;
			CBNetworkCommunicatorSendMessage(self,node,getaddr);
			CBReleaseObject(&getaddr);
			node->getAddresses = true;
		}else{
			// We need their version.
			node->acceptedTypes |= CB_MESSAGE_TYPE_VERSION;
		}
	}
	return false;
}
void CBNetworkCommunicatorDisconnect(CBNetworkCommunicator * self,CBNode * node){
	CBSocketFreeEvent(node->receiveEvent);
	CBSocketFreeEvent(node->sendEvent);
	CBCloseSocket(node->socketID);
	if (node->receive) CBReleaseObject(&node->receive);
	for (u_int8_t x = 0; x < node->sendQueueSize; x++) {
		CBReleaseObject(&node->sendQueue[x]);
	}
	free(node->sendQueue);
}
void CBNetworkCommunicatorRemoveNode(CBNetworkCommunicator * self,CBNode * node){
	CBNetworkCommunicatorDisconnect(self, node);
	CBReleaseObject(&node);
	CBMutexLock(self->accessNodeListMutex); // Use mutex lock when modifing the nodes list.
	// Find position of node. Basic linear search (Better alternative in this case ???). Assumes node is in the list properly or a fatal overflow is caused.
	u_int16_t nodePos = 0;
	for (;; nodePos++) if (self->nodes[nodePos] == node) break;
	// Moves rest of nodes down
	memmove(self->nodes + nodePos, self->nodes + nodePos + 1, self->nodesNum - nodePos - 1);
	CBMutexUnlock(self->accessNodeListMutex); // Now other threads can use the list.
}
void CBNetworkCommunicatorOnCanReceive(void * vself,void * vnode){
	CBNetworkCommunicator * self = vself;
	CBNode * node = vnode;
	// Node kindly has some data available in the socket buffer.
	if (!node->receive) {
		// New message to be received.
		node->receive = CBNewMessageByObject(self->events);
		node->headerBuffer = malloc(24); // Twenty-four bytes for the message header.
		node->messageReceived = 0; // So far received nothing.
		node->receiveStart = clock(); // Time message download.
		CBSocketAddEvent(node->receiveEvent, self->recvTimeOut); // From now on use timeout for receiving data.
	}
	if (!node->receivedHeader) { // Not received the complete message header yet.
		 int32_t num = CBSocketReceive(node->socketID, node->headerBuffer + node->messageReceived, 24 - node->messageReceived);
		switch (num) {
			case CB_SOCKET_CONNECTION_CLOSE:
			case CB_SOCKET_FAILURE:
				// Failure or connection lost so remove node.
				CBNetworkCommunicatorRemoveNode(self, node);
				free(node->headerBuffer); // Free the buffer
				return;
			default:
				// Did read some bytes
				node->messageReceived += num;
				if (node->messageReceived == 24){
					// Hurrah! The header has been received.
					CBNetworkCommunicatorOnHeaderRecieved(self,node);
					node->messageReceived = 0; // Reset for payload.
				}
				return;
		}
	}else{
		// Means we have payload to fetch as we have the header but not the payload
		int32_t num = CBSocketReceive(node->socketID, CBByteArrayGetData(node->receive->bytes) + node->messageReceived, node->receive->bytes->length - node->messageReceived);
		switch (num) {
			case CB_SOCKET_CONNECTION_CLOSE:
			case CB_SOCKET_FAILURE:
				// Failure or connection lost so remove node.
				CBNetworkCommunicatorRemoveNode(self, node);
				return;
			default:
				// Did read some bytes
				node->messageReceived += num;
				if (node->messageReceived == node->receive->bytes->length)
					// We now have the message.
					CBNetworkCommunicatorOnMessageRecieved(self,node);
				return;
		}
	}
}
void CBNetworkCommunicatorOnCanSend(void * vself,void * vnode){
	CBNetworkCommunicator * self = vself;
	CBNode * node = vnode;
	// Can now send data
	if (!node->sentHeader) {
		// Need to send the header
		if (!node->messageSent) {
			// Create header
			node->sendingHeader = malloc(24);
			node->sendingHeader[0] = self->networkID;
			node->sendingHeader[1] = self->networkID << 8;
			node->sendingHeader[2] = self->networkID << 16;
			node->sendingHeader[3] = self->networkID << 24;
			CBMessage * toSend = node->sendQueue[node->sendQueueFront];
			// Message type text
			if (toSend->type == CB_MESSAGE_TYPE_VERSION) {
				memcpy(node->sendingHeader, "version\0\0\0\0\0", 12);
			}else if (toSend->type == CB_MESSAGE_TYPE_VERACK) {
				memcpy(node->sendingHeader, "verack\0\0\0\0\0\0", 12);
			}else if (toSend->type == CB_MESSAGE_TYPE_ADDR) {
				memcpy(node->sendingHeader, "addr\0\0\0\0\0\0\0\0", 12);
			}else if (toSend->type == CB_MESSAGE_TYPE_INV) {
				memcpy(node->sendingHeader, "inv\0\0\0\0\0\0\0\0\0", 12);
			}else if (toSend->type == CB_MESSAGE_TYPE_GETDATA) {
				memcpy(node->sendingHeader, "getdata\0\0\0\0\0", 12);
			}else if (toSend->type == CB_MESSAGE_TYPE_GETBLOCKS) {
				memcpy(node->sendingHeader, "getblocks\0\0\0", 12);
			}else if (toSend->type == CB_MESSAGE_TYPE_GETHEADERS) {
				memcpy(node->sendingHeader, "getheaders\0\0", 12);
			}else if (toSend->type == CB_MESSAGE_TYPE_TX) {
				memcpy(node->sendingHeader, "tx\0\0\0\0\0\0\0\0\0\0", 12);
			}else if (toSend->type == CB_MESSAGE_TYPE_BLOCK) {
				memcpy(node->sendingHeader, "block\0\0\0\0\0\0\0", 12);
			}else if (toSend->type == CB_MESSAGE_TYPE_HEADERS) {
				memcpy(node->sendingHeader, "headers\0\0\0\0\0", 12);
			}else if (toSend->type == CB_MESSAGE_TYPE_GETADDR) {
				memcpy(node->sendingHeader, "getaddr\0\0\0\0\0", 12);
			}else if (toSend->type == CB_MESSAGE_TYPE_PING) {
				memcpy(node->sendingHeader, "ping\0\0\0\0\0\0\0\0", 12);
			}else if (toSend->type == CB_MESSAGE_TYPE_PONG) {
				memcpy(node->sendingHeader, "pong\0\0\0\0\0\0\0\0", 12);
			}else if (toSend->type == CB_MESSAGE_TYPE_ALERT) {
				memcpy(node->sendingHeader, "alert\0\0\0\0\0\0\0", 12);
			}else if (toSend->type == CB_MESSAGE_TYPE_ALT) {
				memcpy(node->sendingHeader, toSend->altText, 12);
			}
			// Length
			node->sendingHeader[16] = toSend->bytes->length;
			node->sendingHeader[17] = toSend->bytes->length << 8;
			node->sendingHeader[18] = toSend->bytes->length << 16;
			node->sendingHeader[19] = toSend->bytes->length << 24;
			// Checksum
			node->sendingHeader[20] = toSend->checksum[0];
			node->sendingHeader[21] = toSend->checksum[1];
			node->sendingHeader[22] = toSend->checksum[2];
			node->sendingHeader[23] = toSend->checksum[3];
		}
		u_int8_t len = CBSocketSend(node->socketID, node->sendingHeader + node->messageSent, 24 - node->messageSent);
		if (len == CB_SOCKET_FAILURE) {
			CBNetworkCommunicatorRemoveNode(self, node);
			free(node->sendingHeader);
		}else{
			node->messageSent += len;
			if (node->messageSent == 24) {
				// Done header
				free(node->sendingHeader);
				node->messageSent = 0;
				node->sentHeader = true;
			}
		}
	}else{
		// Sent header
		u_int8_t len = CBSocketSend(node->socketID, CBByteArrayGetData(node->sendQueue[node->sendQueueFront]->bytes) + node->messageSent, 24 - node->messageSent);
		if (len == CB_SOCKET_FAILURE)
			CBNetworkCommunicatorRemoveNode(self, node);
		else{
			node->messageSent += len;
			if (node->messageSent == node->sendQueue[node->sendQueueFront]->bytes->length) {
				// Sent the entire payload. Celebrate with some bonbonbonbons.
				CBReleaseObject(&node->sendQueue[node->sendQueueFront]);
				node->sendQueueSize--;
				if (node->sendQueueSize) {
					node->sendQueueFront++;
					if (node->sendQueueFront == CB_SEND_QUEUE_MAX_SIZE) {
						node->sendQueueFront = 0;
					}
				}else{
					// Remove send event as we have nothing left to send
					CBSocketRemoveEvent(node->sendEvent);
				}
			}
		}
	}
}
void CBNetworkCommunicatorOnHeaderRecieved(CBNetworkCommunicator * self,CBNode * node){
	// Make a CBByteArray. ??? Could be modified not to use a CBByteArray, but it is cleaner this way and easier to maintain.
	CBByteArray * header = CBNewByteArrayWithData(node->headerBuffer, 24, self->events);
	if (CBByteArrayReadInt32(header, 0) != self->networkID) {
		// The network ID bytes is not what we are looking for. This is a bit of a nuissance. We will have to remove the node.
		CBNetworkCommunicatorRemoveNode(self, node);
		return;
	}
	CBMessageType type;
	u_int32_t size;
	bool error = false;
	CBByteArray * typeBytes = CBNewByteArraySubReference(header, 4, 12);
	if (!memcmp(CBByteArrayGetData(typeBytes),"version\0\0\0\0\0",12)) {
		// Version message
		// Check if we are accepting version messages at the moment.
		if (node->acceptedTypes & CB_MESSAGE_TYPE_VERSION){
			// Check payload size
			size = CBByteArrayReadInt32(header, 16);
			if (size > CB_VERSION_MAX_SIZE) // Illegal size. cbitcoin will not accept this.
				error = true;
			else
				type = CB_MESSAGE_TYPE_VERSION;
		}else error = true;
	}else if (!memcmp(CBByteArrayGetData(typeBytes), "verack\0\0\0\0\0\0", 12)){
		// Version acknowledgement message
		if (node->acceptedTypes & CB_MESSAGE_TYPE_VERACK){
			size = CBByteArrayReadInt32(header, 16);
			if (size) // Must contain zero payload.
				error = true;
			else
				type = CB_MESSAGE_TYPE_VERACK;
		}else error = true;
	}else if (!memcmp(CBByteArrayGetData(typeBytes), "addr\0\0\0\0\0\0\0\0", 12)){
		// Address broadcast message
		if (node->acceptedTypes & CB_MESSAGE_TYPE_ADDR){
			size = CBByteArrayReadInt32(header, 16);
			if (size > CB_ADDRESS_BROADCAST_MAX_SIZE)
				error = true;
			else
				type = CB_MESSAGE_TYPE_ADDR;
		}else error = true;
	}else if (!memcmp(CBByteArrayGetData(typeBytes), "inv\0\0\0\0\0\0\0\0\0", 12)){
		// Inventory broadcast message
		if (node->acceptedTypes & CB_MESSAGE_TYPE_INV){
			size = CBByteArrayReadInt32(header, 16);
			if (size > CB_INVENTORY_BROADCAST_MAX_SIZE)
				error = true;
			else
				type = CB_MESSAGE_TYPE_INV;
		}else error = true;
	}else if (!memcmp(CBByteArrayGetData(typeBytes), "getdata\0\0\0\0\0", 12)){
		// Get data message
		if (node->acceptedTypes & CB_MESSAGE_TYPE_GETDATA){
			size = CBByteArrayReadInt32(header, 16);
			if (size > CB_GET_DATA_MAX_SIZE)
				error = true;
			else
				type = CB_MESSAGE_TYPE_GETDATA;
		}else error = true;
	}else if (!memcmp(CBByteArrayGetData(typeBytes), "getblocks\0\0\0", 12)){
		// Get blocks message
		if (node->acceptedTypes & CB_MESSAGE_TYPE_BLOCK){
			size = CBByteArrayReadInt32(header, 16);
			if (size > CB_GET_BLOCKS_MAX_SIZE)
				error = true;
			else
				type = CB_MESSAGE_TYPE_GETBLOCKS;
		}else error = true;
	}else if (!memcmp(CBByteArrayGetData(typeBytes), "getheaders\0\0", 12)){
		// Get headers message
		if (node->acceptedTypes & CB_MESSAGE_TYPE_GETHEADERS){
			size = CBByteArrayReadInt32(header, 16);
			if (size > CB_GET_HEADERS_MAX_SIZE)
				error = true;
			else
				type = CB_MESSAGE_TYPE_GETHEADERS;
		}else error = true;
	}else if (!memcmp(CBByteArrayGetData(typeBytes), "tx\0\0\0\0\0\0\0\0\0\0", 12)){
		// Transaction message
		if (node->acceptedTypes & CB_MESSAGE_TYPE_TX){
			size = CBByteArrayReadInt32(header, 16);
			if (size > CB_TRANSACTION_MAX_SIZE)
				error = true;
			else
				type = CB_MESSAGE_TYPE_TX;
		}else error = true;
	}else if (!memcmp(CBByteArrayGetData(typeBytes), "block\0\0\0\0\0\0\0", 12)){
		// Block message
		if (node->acceptedTypes & CB_MESSAGE_TYPE_BLOCK){
			size = CBByteArrayReadInt32(header, 16);
			if (size > CB_BLOCK_MAX_SIZE)
				error = true;
			else
				type = CB_MESSAGE_TYPE_BLOCK;
		}else error = true;
	}else if (!memcmp(CBByteArrayGetData(typeBytes), "headers\0\0\0\0\0", 12)){
		// Block headers message
		if (node->acceptedTypes & CB_MESSAGE_TYPE_HEADERS){
			size = CBByteArrayReadInt32(header, 16);
			if (size > CB_BLOCK_HEADERS_MAX_SIZE)
				error = true;
			else
				type = CB_MESSAGE_TYPE_HEADERS;
		}else error = true;
	}else if (!memcmp(CBByteArrayGetData(typeBytes), "getaddr\0\0\0\0\0", 12)){
		// Get Addresses message
		if (node->acceptedTypes & CB_MESSAGE_TYPE_GETADDR){
			size = CBByteArrayReadInt32(header, 16);
			if (size) // Should be empty
				error = true;
			else
				type = CB_MESSAGE_TYPE_GETADDR;
		}else error = true;
	}else if (!memcmp(CBByteArrayGetData(typeBytes), "ping\0\0\0\0\0\0\0\0", 12)){
		// Ping message
		if (node->acceptedTypes & CB_MESSAGE_TYPE_PING){
			size = CBByteArrayReadInt32(header, 16);
			if (size > 8 || ((node->versionMessage->version < 60000 || self->version->version < 60000) && size)) // Should be empty before version 60000 or 8 bytes.
				error = true;
			else
				type = CB_MESSAGE_TYPE_PING;
		}else error = true;
	}else if (!memcmp(CBByteArrayGetData(typeBytes), "pong\0\0\0\0\0\0\0\0", 12)){
		// Pong message
		if (node->acceptedTypes & CB_MESSAGE_TYPE_PONG){
			size = CBByteArrayReadInt32(header, 16);
			if (size > 8)
				error = true;
			else
				type = CB_MESSAGE_TYPE_PONG;
		}else error = true;
	}else if (!memcmp(CBByteArrayGetData(typeBytes), "alert\0\0\0\0\0\0\0", 12)){
		// Alert message
		if (node->acceptedTypes & CB_MESSAGE_TYPE_ALERT){
			size = CBByteArrayReadInt32(header, 16);
			if (size > CB_ALERT_MAX_SIZE)
				error = true;
			else
				type = CB_MESSAGE_TYPE_ALERT;
		}else error = true;
	}else{
		// Either alternative or invalid
		error = true; // Default error until OK alternative message found.
		for (u_int16_t x = 0; x < self->alternativeMessages->length / 12; x++) {
			// Check this alternative message
			if (!memcmp(CBByteArrayGetData(typeBytes), CBByteArrayGetData(self->alternativeMessages) + 12 * x, 12)){
				// Alternative message
				type = CB_MESSAGE_TYPE_ALT;
				// Check payload size
				size = CBByteArrayReadInt32(header, 16);
				u_int32_t altSize;
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
	CBReleaseObject(&typeBytes);
	if (error) {
		// Error with the message header type or length
		CBNetworkCommunicatorRemoveNode(self, node);
		CBReleaseObject(&node->receive);
		CBReleaseObject(&header);
		return;
	}
	// The type and size is OK, make the message
	node->receive->type = type;
	node->receive->bytes = CBNewByteArrayOfSize(size, self->events);
	// Get checksum
	node->receive->checksum[0] = CBByteArrayGetByte(header, 20);
	node->receive->checksum[1] = CBByteArrayGetByte(header, 21);
	node->receive->checksum[2] = CBByteArrayGetByte(header, 22);
	node->receive->checksum[3] = CBByteArrayGetByte(header, 23);
	// Message is now ready. Free the header.
	CBReleaseObject(&header); // Took the header buffer which should be freed here.
	node->receivedHeader = true;
}
void CBNetworkCommunicatorOnLoopError(void * vself){
	CBNetworkCommunicator * self = vself;
	CBGetMessage(self)->events->onErrorReceived(CB_ERROR_NETWORK_COMMUNICATOR_LOOP_FAIL,"The socket event loop failed. Stoping the CBNetworkCommunicator...");
	self->eventLoop = 0;
	CBNetworkCommunicatorStop(self);
}
void CBNetworkCommunicatorOnMessageRecieved(CBNetworkCommunicator * self,CBNode * node){
	// Update stats
	node->timeUsed += (u_int32_t)(clock()-node->receiveStart)/CLOCKS_PER_SEC;
	node->bytesTransferred += node->receive->bytes->length + 24;
	// Check checksum
	u_int8_t * hash1 = CBSha256(CBByteArrayGetData(node->receive->bytes), node->receive->bytes->length);
	u_int8_t * hash2 = CBSha256(hash1, 32);
	free(hash1);
	if (memcmp(hash2, node->receive->checksum, 4)) {
		// Checksum failure. There is no excuse for this. Drop the node. ??? Why have checksums anyway?
		CBNetworkCommunicatorRemoveNode(self, node);
		free(hash2);
		return;
	}
	free(hash2);
	// Deserialise and give the onMessageReceived or onAlternativeMessageReceived event
	u_int32_t len;
	switch (node->receive->type) {
		case CB_MESSAGE_TYPE_VERSION:
			node->receive = realloc(node->receive, sizeof(CBVersion)); // For storing additional data
			CBGetObject(node->receive)->free = CBFreeVersion;
			len = CBVersionDeserialise(CBGetVersion(node->receive));
			break;
		case CB_MESSAGE_TYPE_ADDR:
			node->receive = realloc(node->receive, sizeof(CBAddressBroadcast));
			CBGetObject(node->receive)->free = CBFreeAddressBroadcast;
			CBGetAddressBroadcast(node->receive)->timeStamps = node->versionMessage->version > 31401; // Timestamps start version 31402 and up.
			len = CBAddressBroadcastDeserialise(CBGetAddressBroadcast(node->receive));
			break;
		case CB_MESSAGE_TYPE_INV:
		case CB_MESSAGE_TYPE_GETDATA:
			node->receive = realloc(node->receive, sizeof(CBInventoryBroadcast));
			CBGetObject(node->receive)->free = CBFreeInventoryBroadcast;
			len = CBInventoryBroadcastDeserialise(CBGetInventoryBroadcast(node->receive));
			break;
		case CB_MESSAGE_TYPE_GETBLOCKS:
		case CB_MESSAGE_TYPE_GETHEADERS:
			node->receive = realloc(node->receive, sizeof(CBGetBlocks));
			CBGetObject(node->receive)->free = CBFreeGetBlocks;
			len = CBGetBlocksDeserialise(CBGetGetBlocks(node->receive));
			break;
		case CB_MESSAGE_TYPE_TX:
			node->receive = realloc(node->receive, sizeof(CBTransaction));
			CBGetObject(node->receive)->free = CBFreeTransaction;
			len = CBTransactionDeserialise(CBGetTransaction(node->receive));
			break;
		case CB_MESSAGE_TYPE_BLOCK:
			node->receive = realloc(node->receive, sizeof(CBBlock));
			CBGetObject(node->receive)->free = CBFreeBlock;
			len = CBBlockDeserialise(CBGetBlock(node->receive),true); // true -> Including transactions.
			break;
		case CB_MESSAGE_TYPE_HEADERS:
			node->receive = realloc(node->receive, sizeof(CBBlockHeaders));
			CBGetObject(node->receive)->free = CBFreeBlockHeaders;
			len = CBBlockHeadersDeserialise(CBGetBlockHeaders(node->receive));
			break;
		case CB_MESSAGE_TYPE_PING:
			if (node->versionMessage->version >= 60000 && self->version->version >= 60000){
				node->receive = realloc(node->receive, sizeof(CBPingPong));
				CBGetObject(node->receive)->free = CBFreePingPong;
				len = CBPingPongDeserialise(CBGetPingPong(node->receive));
			}
			else len = 1;
		case CB_MESSAGE_TYPE_PONG:
			node->receive = realloc(node->receive, sizeof(CBPingPong));
			CBGetObject(node->receive)->free = CBFreePingPong;
			len = CBPingPongDeserialise(CBGetPingPong(node->receive));
			break;
		case CB_MESSAGE_TYPE_ALERT:
			node->receive = realloc(node->receive, sizeof(CBAlert));
			CBGetObject(node->receive)->free = CBFreeAlert;
			len = CBAlertDeserialise(CBGetAlert(node->receive));
			break;
		default:
			len = 0; // Zero default
			break;
	}
	// Check that the deserialised message's size read matches the message length.
	if (len != node->receive->bytes->length)
		// Error
		CBNetworkCommunicatorRemoveNode(self, node);
	// Deserialisation was sucessful. 
	// By default wait until we consider the node dead.
	CBSocketAddEvent(node->receiveEvent, self->timeOut);
	// Call event callback
	bool disconnect = self->events->onMessageReceived(self,node);
	if (!disconnect) {
		// Automatic responses
		if (self->flags & CB_NETWORK_COMMUNICATOR_AUTO_DISCOVERY)
			// Auto discovery responses
			CBNetworkCommunicatorProcessMessageAutoDiscovery(self,node);
		else if (self->flags & CB_NETWORK_COMMUNICATOR_AUTO_HANDSHAKE)
			// Auto handshake responses
			CBNetworkCommunicatorProcessMessageAutoHandshake(self,node);
		else if (self->flags & CB_NETWORK_COMMUNICATOR_AUTO_PING) {
			// Auto ping response
			if (node->receive->type == CB_MESSAGE_TYPE_PING
				&& node->versionMessage->version >= CB_PONG_VERSION
				&& self->version->version >= CB_PONG_VERSION)
				CBNetworkCommunicatorSendMessage(self, node, node->receive);
			else if (node->receive->type == CB_MESSAGE_TYPE_PONG
					 && self->version->version >= CB_PONG_VERSION
					 && node->versionMessage->version < CB_PONG_VERSION)
				disconnect = true; // This node should not be sending pong messages.
			// Use opportunity to check if we should send ping messages to nodes to check the connections or keep connections alive.
			if (self->lastPing < time(NULL) - CB_PING_TIME) {
				CBMessage * ping = CBNewMessageByObject(self->events);
				ping->type = CB_MESSAGE_TYPE_PING;
				if (self->version->version >= CB_PONG_VERSION) {
					CBPingPong * pingPong = CBNewPingPong(rand(),self->events);
					CBGetMessage(pingPong)->type = CB_MESSAGE_TYPE_PING;
					for (u_int16_t x = 0; x < self->nodesNum; x++) {
						if (self->nodes[x]->acceptedTypes & CB_MESSAGE_TYPE_PING){ // Only send ping if they can send ping.
							if(self->nodes[x]->versionMessage->version >= CB_PONG_VERSION){
								CBNetworkCommunicatorSendMessage(self, self->nodes[x], CBGetMessage(pingPong));
								CBSocketAddEvent(node->receiveEvent, self->responseTimeOut); // Expect response.
							}else
								CBNetworkCommunicatorSendMessage(self, self->nodes[x], CBGetMessage(ping));
						}
					}
					CBReleaseObject(&pingPong);
				}else for (u_int16_t x = 0; x < self->nodesNum; x++)
					if (self->nodes[x]->acceptedTypes & CB_MESSAGE_TYPE_PING) // Only send ping if they can send ping.
						CBNetworkCommunicatorSendMessage(self, self->nodes[x], CBGetMessage(ping));
				CBReleaseObject(&ping);
				self->lastPing = time(NULL);
			}
		}
	}
	// Release objects and get ready for next message
	if (disconnect)
		// Node misbehaving. Disconnect.
		CBNetworkCommunicatorRemoveNode(self, node);
	else
		CBReleaseObject(&node->receive);
}
void CBNetworkCommunicatorOnTimeOut(void * vself,void * vnode){
	CBNetworkCommunicator * self = vself;
	CBNode * node = vnode;
	// Remove CBNetworkAddress
	CBNetworkCommunicatorRemoveNode(self,node);
	// Send event for network timeout. The node will be NULL if it wasn't retained elsewhere.
	CBGetMessage(self)->events->onTimeOut(self,node);
}
bool CBNetworkCommunicatorSendMessage(CBNetworkCommunicator * self,CBNode * node,CBMessage * message){
	if (node->sendQueueSize == CB_SEND_QUEUE_MAX_SIZE) {
		return false;
	}
	if (message->bytes->length) {
		// Make checksum
		u_int8_t * hash1 = CBSha256(CBByteArrayGetData(message->bytes), message->bytes->length);
		u_int8_t * hash2 = CBSha256(hash1, 32);
		free(hash1);
		message->checksum[0] = hash2[0];
		message->checksum[1] = hash2[1];
		message->checksum[2] = hash2[2];
		message->checksum[3] = hash2[3];
		free(hash2);
	}else{
		// Empty bytes checksum
		message->checksum[0] = 0x5D;
		message->checksum[1] = 0xF6;
		message->checksum[2] = 0xE0;
		message->checksum[3] = 0xE2;
	}
	CBRetainObject(message);
	node->sendQueue[(node->sendQueueFront + node->sendQueueSize) % CB_SEND_QUEUE_MAX_SIZE] = message; // Node will send the message from this.
	node->sendQueueSize++;
	if (node->sendQueueSize == 1) {
		CBSocketAddEvent(node->sendEvent, self->sendTimeOut);
	}
}
bool CBNetworkCommunicatorStart(CBNetworkCommunicator * self){
	// Create the socket event loop
	self->eventLoop = CBNewEventLoop(CBNetworkCommunicatorOnLoopError, CBNetworkCommunicatorOnTimeOut, self);
	if (!self->eventLoop){
		// Cannot create event loop
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_NETWORK_COMMUNICATOR_LOOP_CREATE_FAIL,"The CBNetworkCommunicator event loop could not be created.");
		return false;
	}
	self->isStarted = true;
	return true; // All OK.
}
void CBNetworkCommunicatorStartListening(CBNetworkCommunicator * self,u_int16_t maxIncommingConnections){
	// Create new bound IPv4 socket
	if (CBSocketBind(&self->listeningSocketIPv4, false, self->port)){
		// Add event for accepting connection for both sockets
		self->acceptEventIPv4 = CBSocketCanAcceptEvent(self->eventLoop, self->listeningSocketIPv4, CBNetworkCommunicatorAcceptConnection);
		if (self->acceptEventIPv4) {
			if(CBSocketAddEvent(self->acceptEventIPv4, 0)) // No timeout for listening for incomming connections.
				// Start listening on IPv4
				if(CBSocketListen(self->listeningSocketIPv4, self->maxIncommingConnections)){
					// Is listening
					self->isListeningIPv4 = true;
				}
			if (!self->isListeningIPv4)
				// Failure to add event
				CBSocketFreeEvent(self->acceptEventIPv4);
		}
	}
	if (!self->isListeningIPv4){
		// Failure to create event
		CBCloseSocket(self->listeningSocketIPv4);
		self->IPv4 = false;
	}
	// Now for the IPv6
	// Create new bound IPv6 socket
	if (CBSocketBind(&self->listeningSocketIPv6, true, self->port)){
		// Add event for accepting connection for both sockets
		self->acceptEventIPv6 = CBSocketCanAcceptEvent(self->eventLoop, self->listeningSocketIPv6, CBNetworkCommunicatorAcceptConnection);
		if (self->acceptEventIPv6) {
			if(CBSocketAddEvent(self->acceptEventIPv6, 0)) // No timeout for listening for incomming connections.
				// Start listening on IPv6
				if(CBSocketListen(self->listeningSocketIPv6, self->maxIncommingConnections)){
					// Is listening
					self->isListeningIPv6 = true;
				}
			if (!self->isListeningIPv6)
				// Failure to add event
				CBSocketFreeEvent(self->acceptEventIPv6);
		}
	}
	if (!self->isListeningIPv6)
		// Failure to create event
		CBCloseSocket(self->listeningSocketIPv6);
}
void CBNetworkCommunicatorStop(CBNetworkCommunicator * self){
	self->isStarted = false;
	if (self->isListeningIPv4) {
		// Stop listening on IPv4
		CBSocketFreeEvent(self->acceptEventIPv4);
		CBCloseSocket(self->listeningSocketIPv4);
	}
	if (self->isListeningIPv6) {
		// Stop listening on IPv6
		CBSocketFreeEvent(self->acceptEventIPv6);
		CBCloseSocket(self->listeningSocketIPv6);
	}
	// Stop event loop
	CBExitEventLoop(self->eventLoop);
	// Disconnect all the nodes
	CBMutexLock(self->accessNodeListMutex);
	for (u_int16_t x = 0; x < self->nodesNum; x++)
		CBNetworkCommunicatorRemoveNode(self, self->nodes[x]);
	CBMutexUnlock(self->accessNodeListMutex);
}
void CBNetworkCommunicatorTakeNode(CBNetworkCommunicator * self,CBNode * node){
	CBMutexLock(self->accessNodeListMutex); // Use mutex lock when modifing the nodes list.
	self->nodesNum++;
	self->nodes[self->nodesNum-1] = node;
	CBMutexUnlock(self->accessNodeListMutex); // Now other threads can use the list.
}
void CBNetworkCommunicatorTakeAddress(CBNetworkCommunicator * self,CBNetworkAddress * addr){
	// First see if we can connect it as a node
	if (self->nodesNum < self->maxConnections) {
		CBNode * node = CBNewNodeByTakingNetworkAddress(addr);
		bool ok = CBNetworkCommunicatorConnect(self, node);
		if (!ok)
			CBReleaseObject(&node);
		return;
	}
	// Waiting for a node to disconnect
	CBMutexLock(self->accessAddressListMutex); // Use mutex lock when modifing the adresses list.
	// Find insersion point for address
	u_int16_t insert = 0;
	for (; insert < self->addrNum; insert++) {
		if (self->addresses[insert]->time > addr->time)
			// Insert here
			break;
	}
	if (self->addrNum > CB_MAX_ADDRESS_STORE - self->nodesNum) {
		// A lot of addresses stored, remove random address but with bias to an old address.
		u_int16_t remove = (rand() % self->addrNum); 
		remove *= remove;
		remove /= self->addrNum;
		// Free address
		CBReleaseObject(&self->addresses[remove]);
		if (insert < remove)
			// Insersions happens below removal. Move memory up to overwrite removed address and make space to insert.
			memmove(self->addresses + insert + 1, self->addresses + insert, remove-insert);
		else if (insert > remove){
			// Insersion happens above removal. Move memory down to overwrite removed address and make space to insert.
			memmove(self->addresses + remove, self->addresses + remove + 1, insert-remove);
			insert--; // Move insert down since we moved memory down. Or it wont be ordered or we could have an overflow.
		}
	}else{
		self->addrNum++;
		// Move memory up to allow insertion of address.
		memmove(self->addresses + insert + 1, self->addresses + insert, self->addrNum - insert - 1);
	}
	self->addresses[insert] = addr;
	CBMutexUnlock(self->accessAddressListMutex); // Now other threads can use the list.
}
