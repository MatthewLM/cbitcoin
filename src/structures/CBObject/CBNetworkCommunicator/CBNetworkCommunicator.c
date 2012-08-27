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

CBNetworkCommunicator * CBNewNetworkCommunicator(CBEvents * events){
	CBNetworkCommunicator * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeNetworkCommunicator;
	bool res = CBInitNetworkCommunicator(self,events);
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

bool CBInitNetworkCommunicator(CBNetworkCommunicator * self,CBEvents * events){
	// Set fields.
	self->attemptingOrWorkingConnections = 0;
	self->numIncommingConnections = 0;
	self->events = events;
	self->eventLoop = 0;
	self->acceptEventIPv4 = 0;
	self->acceptEventIPv6 = 0;
	self->isListeningIPv4 = false;
	self->isListeningIPv6 = false;
	self->ourIPv4 = NULL;
	self->ourIPv6 = NULL;
	self->pingTimer = 0;
	if (NOT CBInitObject(CBGetObject(self)))
		return false;
	return true;
}

//  Destructor

void CBFreeNetworkCommunicator(void * vself){
	// When this is called all references to the object should be released, so no additional syncronisation is required
	CBNetworkCommunicator * self = vself;
	if (self->isStarted)
		CBNetworkCommunicatorStop(self);
	CBReleaseObject(self->alternativeMessages);
	CBReleaseObject(self->addresses);
	if (self->ourIPv4) CBReleaseObject(self->ourIPv4);
	if (self->ourIPv6) CBReleaseObject(self->ourIPv6);
	free(self->altMaxSizes);
	CBFreeObject(self);
}

//  Functions

void CBNetworkCommunicatorAcceptConnection(void * vself,uint64_t socket){
	CBNetworkCommunicator * self = vself;
	uint64_t connectSocketID;
	printf("%s TRIED ACCEPT\n",(self->ourIPv4->port == 45562)? "L1" : ((self->ourIPv4->port == 45563)? "L2" : "CN"));
	if (NOT CBSocketAccept(socket, &connectSocketID))
		return;
	printf("%s MADE ACCEPT\n",(self->ourIPv4->port == 45562)? "L1" : ((self->ourIPv4->port == 45563)? "L2" : "CN"));
	// Connected, add CBNetworkAddress
	CBNode * node = CBNewNodeByTakingNetworkAddress(CBNewNetworkAddress(0, NULL, 0, 0, self->events));
	node->incomming = true;
	node->socketID = connectSocketID;
	node->acceptedTypes = CB_MESSAGE_TYPE_VERSION; // The node connected to us, we want the version from them.
	// Set up receive event
	if (CBSocketCanReceiveEvent(&node->receiveEvent,self->eventLoop,node->socketID, CBNetworkCommunicatorOnCanReceive, node)) {
		// The event works
		if(CBSocketAddEvent(node->receiveEvent, self->timeOut)){ // Begin receive event.
			// Success
			if (CBSocketCanSendEvent(&node->sendEvent,self->eventLoop, node->socketID, CBNetworkCommunicatorOnCanSend, node)) {
				// Both events work. Take the node.
				CBAddressManagerTakeNode(self->addresses, node);
				if (self->addresses->nodesNum == 1 && self->flags & CB_NETWORK_COMMUNICATOR_AUTO_PING)
					// Got first node, start pings
					CBNetworkCommunicatorStartPings(self);
				node->connectionWorking = true;
				self->attemptingOrWorkingConnections++;
				self->numIncommingConnections++;
				printf("%s HAS ACCEPTED OK\n",(self->ourIPv4->port == 45562)? "L1" : ((self->ourIPv4->port == 45563)? "L2" : "CN"));
				if (self->numIncommingConnections == self->maxIncommingConnections || self->attemptingOrWorkingConnections == self->maxConnections) {
					// Reached maximum connections, stop listening.
					CBNetworkCommunicatorStopListening(self);
				}
				return;
			}
		}
		// Could create receive event but there was a failure afterwards so free it.
		CBSocketFreeEvent(node->receiveEvent);
	}
	// Failure, release node.
	CBCloseSocket(connectSocketID);
	CBReleaseObject(node);
	printf("%s HAS ACCEPTED FAIL\n",(self->ourIPv4->port == 45562)? "L1" : ((self->ourIPv4->port == 45563)? "L2" : "CN"));
}
CBConnectReturn CBNetworkCommunicatorConnect(CBNetworkCommunicator * self,CBNode * node){
	if (NOT CBAddressManagerIsReachable(self->addresses, CBGetNetworkAddress(node)->type))
		return CB_CONNECT_NO_SUPPORT;
	bool isIPv6 = CBGetNetworkAddress(node)->type & CB_IP_IPv6;
	// Make socket
	CBSocketReturn res = CBNewSocket(&node->socketID, isIPv6);
	if(res == CB_SOCKET_NO_SUPPORT){
		if (isIPv6) CBAddressManagerSetReachability(self->addresses, CB_IP_IPv6, false);
		else CBAddressManagerSetReachability(self->addresses, CB_IP_IPv4, false);
		if (NOT CBAddressManagerIsReachable(self->addresses, CB_IP_IPv6 | CB_IP_IPv4))
			// IPv6 and IPv4 not reachable.
			self->events->onNetworkError(self->callbackHandler,self);
		return CB_CONNECT_NO_SUPPORT;
	}else if (res == CB_SOCKET_BAD) {
		if (NOT self->addresses->nodesNum) {
			self->events->onNetworkError(self->callbackHandler,self);
		}
		return CB_CONNECT_FAIL;
	}
	// Connect
	if(CBSocketConnect(node->socketID, CBByteArrayGetData(CBGetNetworkAddress(node)->ip), isIPv6, CBGetNetworkAddress(node)->port)){
		// Add event for connection
		if (CBSocketDidConnectEvent(&node->connectEvent,self->eventLoop, node->socketID, CBNetworkCommunicatorDidConnect, node)) {
			if (CBSocketAddEvent(node->connectEvent, self->connectionTimeOut)) {
				printf("%s CONNECTION WAIT\n",(self->ourIPv4->port == 45562)? "L1" : ((self->ourIPv4->port == 45563)? "L2" : "CN"));
				// Connection is fine. Retain for waiting for connect.
				CBRetainObject(node);
				self->attemptingOrWorkingConnections++;
				return CB_CONNECT_OK;
			}else
				CBSocketFreeEvent(node->connectEvent);
		}
		CBCloseSocket(node->socketID);
		return CB_CONNECT_FAIL;
	}
	CBCloseSocket(node->socketID);
	return CB_CONNECT_BAD;
}
void CBNetworkCommunicatorDidConnect(void * vself,void * vnode){
	CBNetworkCommunicator * self = vself;
	printf("%s CONNECTED OK\n",(self->ourIPv4->port == 45562)? "L1" : ((self->ourIPv4->port == 45563)? "L2" : "CN"));
	CBNode * node = vnode;
	CBSocketFreeEvent(node->connectEvent); // No longer need this event.
	// Check to see if in the meantime, that we have not been connected to by the node. Double connections are bad m'kay.
	if (NOT CBAddressManagerGotNode(self->addresses, CBGetNetworkAddress(node))){
		// Make receive event
		if (CBSocketCanReceiveEvent(&node->receiveEvent,self->eventLoop, node->socketID, CBNetworkCommunicatorOnCanReceive, node)) {
			// Make send event
			if (CBSocketCanSendEvent(&node->sendEvent,self->eventLoop, node->socketID, CBNetworkCommunicatorOnCanSend, node)) {
				if (CBSocketAddEvent(node->sendEvent, self->sendTimeOut)) {
					CBAddressManagerTakeNode(self->addresses, node);
					if (self->addresses->nodesNum == 1 && self->flags & CB_NETWORK_COMMUNICATOR_AUTO_PING)
						// Got first node, start pings
						CBNetworkCommunicatorStartPings(self);
					node->connectionWorking = true;
					// Connection OK, so begin handshake if auto handshaking is enabled.
					if (self->flags & CB_NETWORK_COMMUNICATOR_AUTO_HANDSHAKE){
						CBVersion * version = CBNetworkCommunicatorGetVersion(self,CBGetNetworkAddress(node));
						CBGetMessage(version)->expectResponse = CB_MESSAGE_TYPE_VERACK; // Expect a response for this message.
						CBGetMessage(version)->type = CB_MESSAGE_TYPE_VERSION;
						node->acceptedTypes = CB_MESSAGE_TYPE_VERACK; // Accept the acknowledgement.
						CBNetworkCommunicatorSendMessage(self, node, CBGetMessage(version));
						CBReleaseObject(version);
						node->versionSent = true;
					}
					return;
				}
				CBSocketFreeEvent(node->sendEvent);
			}
			CBSocketFreeEvent(node->receiveEvent);
		}
	}
	// Close socket on failure
	CBCloseSocket(node->socketID);
	self->attemptingOrWorkingConnections--;
	if (NOT self->attemptingOrWorkingConnections) {
		self->events->onNetworkError(self->callbackHandler,self);
	}
	if (CBGetNetworkAddress(node)->public)
		// No penalty here since it was definitely our fault.
		CBAddressManagerTakeAddress(self->addresses, realloc(node, sizeof(CBNetworkAddress)));
	else
		CBReleaseObject(node);
}
void CBNetworkCommunicatorDisconnect(CBNetworkCommunicator * self,CBNode * node,u_int16_t penalty,bool stopping){
	printf("%s DISCONNECT FROM %s (%p)\n",(self->ourIPv4->port == 45562)? "L1" : ((self->ourIPv4->port == 45563)? "L2" : "CN"),(CBGetNetworkAddress(node)->port == 45562)? "L1" : ((CBGetNetworkAddress(node)->port == 45563)? "L2" : ((CBGetNetworkAddress(node)->port == 45564) ? "CN" : "UK")),node);
	// Apply the penalty given
	CBGetNetworkAddress(node)->score -= penalty;
	// Free events if they exist
	if (node->receiveEvent) CBSocketFreeEvent(node->receiveEvent);
	if (node->sendEvent) CBSocketFreeEvent(node->sendEvent);
	// Close the socket
	CBCloseSocket(node->socketID);
	// Release the receiving message object if it exists.
	if (node->receive) CBReleaseObject(node->receive);
	// Release all messages in the send queue
	for (uint8_t x = 0; x < node->sendQueueSize; x++)
		CBReleaseObject(node->sendQueue[(node->sendQueueFront + x) % CB_SEND_QUEUE_MAX_SIZE]);
	// If incomming, lower the incomming connections number
	if (node->incomming){
		if (self->numIncommingConnections == self->maxIncommingConnections || self->attemptingOrWorkingConnections == self->maxConnections)
			// Start listening again
			CBNetworkCommunicatorStartListening(self);
		self->numIncommingConnections--;
	}
	// Lower the attempting or working connections number
	self->attemptingOrWorkingConnections--;
	// If this is a working connection, remove from the address manager.
	if (node->connectionWorking)
		CBAddressManagerRemoveNode(self->addresses, node);
	else{
		// Else we release the object from control of the CBNetworkCommunicator
		CBSocketFreeEvent(node->connectEvent);
		CBReleaseObject(node);
	}
	if (self->addresses->nodesNum == 0 && self->flags & CB_NETWORK_COMMUNICATOR_AUTO_PING)
		// No more nodes so stop pings
		CBNetworkCommunicatorStopPings(self);
	if (NOT self->attemptingOrWorkingConnections && NOT stopping)
		// No more connections so give a network error
		self->events->onNetworkError(self->callbackHandler,self);
}
CBVersion * CBNetworkCommunicatorGetVersion(CBNetworkCommunicator * self,CBNetworkAddress * addRecv){
	CBNetworkAddress * sourceAddr;
	if (addRecv->type & CB_IP_IPv6
		&& self->ourIPv6)
		sourceAddr = self->ourIPv6;
	else if (self->ourIPv4)
		sourceAddr = self->ourIPv4;
	self->nounce = rand();
	CBVersion * version = CBNewVersion(self->version, self->services, time(NULL), addRecv, sourceAddr, self->nounce, self->userAgent, self->blockHeight, self->events);
	return version;
}
CBOnMessageReceivedAction CBNetworkCommunicatorProcessMessageAutoDiscovery(CBNetworkCommunicator * self,CBNode * node){
	if (node->receive->type == CB_MESSAGE_TYPE_ADDR) {
		// Received addresses.
		CBAddressBroadcast * addrs = CBGetAddressBroadcast(node->receive);
		// Only accept no timestaps when we have less than 100 addresses.
		if(node->versionMessage->version < CB_ADDR_TIME_VERSION
		   && CBAddressManagerGetNumberOfAddresses(self->addresses) > 1000)
			return CB_MESSAGE_ACTION_CONTINUE;
		// Store addresses.
		uint64_t now = time(NULL) + self->addresses->networkTimeOffset;
		// Loop through addresses
		bool didAdd = false; // True when we add an address.
		for (uint16_t x = 0; x < addrs->addrNum; x++) {
			// Check if we have the address as a connected node
			CBNode * nodeB = CBAddressManagerGotNode(self->addresses,addrs->addresses[x]);
			if(NOT nodeB){
				// Do not already have this address as a node
				if (addrs->addresses[x]->type & CB_IP_INVALID)
					// Address broadcasts should not contain invalid addresses.
					return CB_MESSAGE_ACTION_DISCONNECT;
				if (addrs->addresses[x]->type & CB_IP_LOCAL && NOT (CBGetNetworkAddress(node)->type & CB_IP_LOCAL))
					// Do not allow nodes to send local addresses to non-local nodes.
					return CB_MESSAGE_ACTION_DISCONNECT;
				// Adjust time priority
				if (addrs->addresses[x]->score > now + 600)
					// Address is advertised more than ten minutes from now. Give low priority at 5 days ago
					addrs->addresses[x]->score = (uint32_t)(now - 432000); // ??? Needs fixing for integer overflow in the year 2106.
				else if (addrs->addresses[x]->score > now - 900) // Sooner than fifteen minutes ago. Give highest priority.
					addrs->addresses[x]->score = (uint32_t)now;
				// Else leave the time
				// Check if we have the address as a stored address
				CBNetworkAddress * addr = CBAddressManagerGotNetworkAddress(self->addresses,addrs->addresses[x]);
				if(addr){
					// Already have the address. Modify the existing object.
					addr->score = addrs->addresses[x]->score;
					addr->services = addrs->addresses[x]->services;
					addr->version = addrs->addresses[x]->version;
				}else if (CBAddressManagerIsReachable(self->addresses, addrs->addresses[x]->type)){
					// Do not have the address so add it if we believe we can reach it and if it is not us.
					if ((self->ourIPv4
						 && addrs->addresses[x]->type & CB_IP_IPv4
						 && CBNetworkAddressEquals(self->ourIPv4, addrs->addresses[x]))
						|| (self->ourIPv6
							&& addrs->addresses[x]->type & CB_IP_IPv6
							&& CBNetworkAddressEquals(self->ourIPv6, addrs->addresses[x])))
								// Is us
								continue;
					printf("%s ADDED ADDRESS for %s\n",(self->ourIPv4->port == 45562)? "L1" : ((self->ourIPv4->port == 45563)? "L2" : "CN"),(addrs->addresses[x]->port == 45562)? "L1" : ((addrs->addresses[x]->port == 45563)? "L2" : "CN"));
					// Make copy of the address since otherwise we will corrupt the CBAddressBroadcast when trying the connection.
					CBNetworkAddress * copy = CBNewNetworkAddress(addrs->addresses[x]->score, addrs->addresses[x]->ip, addrs->addresses[x]->port, addrs->addresses[x]->services, CBGetMessage(addrs->addresses[x])->events);
					CBAddressManagerAddAddress(self->addresses, copy);
					CBReleaseObject(copy);
					didAdd = true;
				}
			}else
				// We have an advertised node. This means it is public and should return to the address store.
				CBGetNetworkAddress(nodeB)->public = true;
		}
		if (NOT node->getAddresses && addrs->addrNum < 10) {
			// Unsolicited addresses. Less than ten so relay to two random nodes. bitcoin-qt does something weird. We wont. Keep it simple... Right ???
			for (uint16_t x = rand() % (self->addresses->nodesNum - 1),y = 0; x < self->addresses->nodesNum && y < 2; x++,y++) {
				if (self->addresses->nodes[x] == node)
					continue;
				CBNetworkCommunicatorSendMessage(self, self->addresses->nodes[x], CBGetMessage(addrs));
			}
		}
		if (didAdd)
			// We have new address information so try connecting to addresses.
			CBNetworkCommunicatorTryConnections(self);
		// Got addresses.
		node->getAddresses = false;
	}else if (node->receive->type == CB_MESSAGE_TYPE_GETADDR) {
		// Give 33 nodes with the highest times with a some randomisation added. Try connected nodes first. Do not send empty addr.
		CBAddressBroadcast * addrBroadcast = CBNewAddressBroadcast(self->version >= CB_ADDR_TIME_VERSION && node->versionMessage->version >= CB_ADDR_TIME_VERSION, self->events);
		CBGetMessage(addrBroadcast)->type = CB_MESSAGE_TYPE_ADDR;
		// Try connected nodes. Only send nodes that are public (private addresses are those which connect to us but haven't relayed their address).
		uint16_t nodesSent = 0;
		uint16_t y = 0;
		while (nodesSent < 28 && y < self->addresses->nodesNum) { // 28 to have room for 5 addresses.
			if (self->addresses->nodes[y] != node // Not the node we are sending to
				&& CBGetNetworkAddress(self->addresses->nodes[y])->public // Public
				&& (CBGetNetworkAddress(self->addresses->nodes[y])->type != CB_IP_LOCAL // OK if not local
					|| CBGetNetworkAddress(node)->type == CB_IP_LOCAL)) { //               Or if the node we are sending to is local
						CBAddressBroadcastAddNetworkAddress(addrBroadcast, CBGetNetworkAddress(self->addresses->nodes[y]));
						nodesSent++;
						printf("%s SENDING %s ADDR TO %s\n",(self->ourIPv4->port == 45562)? "L1" : ((self->ourIPv4->port == 45563)? "L2" : "CN"),(CBGetNetworkAddress(self->addresses->nodes[y])->port == 45562)? "L1" : ((CBGetNetworkAddress(self->addresses->nodes[y])->port == 45563)? "L2" : "CN"),(CBGetNetworkAddress(node)->port == 45562)? "L1" : ((CBGetNetworkAddress(node)->port == 45563)? "L2" : "CN"));
			}
			y++;
		}
		// Now add stored addresses
		CBNetworkAddressLocator * addrs = CBAddressManagerGetAddresses(self->addresses, 33 - nodesSent);
		for (u_int8_t x = 0; (addrs + x)->addr != NULL; x++){
			if ((addrs + x)->addr->type != CB_IP_LOCAL
				|| CBGetNetworkAddress(node)->type == CB_IP_LOCAL)
				// If the address is not local or if the node we are sending to is local, the address is acceptable.
				CBAddressBroadcastAddNetworkAddress(addrBroadcast, (addrs + x)->addr);
			CBReleaseObject((addrs + x)->addr);
		}
		free(addrs);
		// Send address broadcast, if we have at least one.
		if (addrBroadcast->addrNum)
			CBNetworkCommunicatorSendMessage(self, node, CBGetMessage(addrBroadcast));
		CBReleaseObject(addrBroadcast);
	}
	// Use opportunity to discover if we should broadcast our own addresses for recieving incoming connections.
	if (self->maxIncommingConnections // Only share address if we allow for incomming connections.
		&& (self->isListeningIPv4 || self->isListeningIPv6) // Share when we are listening for incoming connections.
		&& node->timeBroadcast < time(NULL) - 86400 // Every 24 hours
		&& node->acceptedTypes & CB_MESSAGE_TYPE_ADDR) { // Only share address if they are allowed.
		node->timeBroadcast = time(NULL);
		CBAddressBroadcast * addrBroadcast = CBNewAddressBroadcast(self->version >= CB_ADDR_TIME_VERSION && node->versionMessage->version >= CB_ADDR_TIME_VERSION, self->events);
		CBGetMessage(addrBroadcast)->type = CB_MESSAGE_TYPE_ADDR;
		if (self->ourIPv6) {
			self->ourIPv6->score = (uint32_t)time(NULL) + self->addresses->networkTimeOffset;
			CBAddressBroadcastAddNetworkAddress(addrBroadcast,self->ourIPv6);
		}
		if (self->ourIPv4) {
			self->ourIPv4->score = (uint32_t)time(NULL) + self->addresses->networkTimeOffset;
			CBAddressBroadcastAddNetworkAddress(addrBroadcast,self->ourIPv4);
		}
		CBNetworkCommunicatorSendMessage(self, node, CBGetMessage(addrBroadcast));
		CBReleaseObject(addrBroadcast);
	}else if (self->ourIPv4->port != 45564
			  && CBGetNetworkAddress(node)->port != 45564){
		printf("%s DIDN'T SEND SELF TO %s (%i,%i,%i,%i,%i,%lli,%li)\n",(self->ourIPv4->port == 45562)? "L1" : ((self->ourIPv4->port == 45563)? "L2" : "CN"),(CBGetNetworkAddress(node)->port == 45562)? "L1" : ((CBGetNetworkAddress(node)->port == 45563)? "L2" : "CN"),self->maxIncommingConnections,self->isListeningIPv4, self->numIncommingConnections,node->timeBroadcast < time(NULL) - 86400,node->acceptedTypes & CB_MESSAGE_TYPE_ADDR,node->timeBroadcast, time(NULL));
	}
	return CB_MESSAGE_ACTION_CONTINUE; // Do not disconnect.
}
CBOnMessageReceivedAction CBNetworkCommunicatorProcessMessageAutoHandshake(CBNetworkCommunicator * self,CBNode * node){
	bool endHandshake = false;
	if (node->receive->type == CB_MESSAGE_TYPE_VERSION) {
		// Node sent us their version. How very nice of them.
		// Check version and nounce. 
		if (CBGetVersion(node->receive)->version < CB_MIN_PROTO_VERSION
			|| (CBGetVersion(node->receive)->nounce == self->nounce && self->nounce > 0))
			// Disconnect node
			return CB_MESSAGE_ACTION_DISCONNECT;
		else{ // Version OK
			// Check if we have a connection to this node already
			CBNode * nodeCheck = CBAddressManagerGotNode(self->addresses, CBGetVersion(node->receive)->addSource);
			if (nodeCheck && nodeCheck != node)
				// Drop the connection
				return CB_MESSAGE_ACTION_DISCONNECT;
			// Remove the source address from the address manager if it has it. It will be added again if the node wishes to broadcast their address. Now that the node is connected, we identify it by the source address and do not want to try connections to the same address.
			CBAddressManagerRemoveAddress(self->addresses, CBGetNetworkAddress(CBGetVersion(node->receive)->addSource));
			// Save version message
			node->versionMessage = CBGetVersion(node->receive);
			CBRetainObject(node->versionMessage);
			// Change version 10300 to 300
			if (node->versionMessage->version == 10300)
				node->versionMessage->version = 300;
			// Copy over reported version and services for the CBNetworkAddress.
			CBGetNetworkAddress(node)->version = node->versionMessage->version;
			CBGetNetworkAddress(node)->services = node->versionMessage->services;
			CBGetNetworkAddress(node)->score = (uint32_t)time(NULL); // ??? Loss of integer precisibon.
			// Change port and IP number to the port and IP the node wants us to recognise them with.
			CBGetNetworkAddress(node)->port = node->versionMessage->addSource->port;
			CBGetNetworkAddress(node)->ip = node->versionMessage->addSource->ip;
			CBGetNetworkAddress(node)->type = CBGetIPType(CBByteArrayGetData(CBGetNetworkAddress(node)->ip));
			// Adjust network time
			node->timeOffset = node->versionMessage->time - time(NULL); // Set the time offset for this node.
			// Disallow version from here.
			node->acceptedTypes &= ~CB_MESSAGE_TYPE_VERSION;
			// Send our acknowledgement
			CBMessage * ack = CBNewMessageByObject(self->events);
			ack->type = CB_MESSAGE_TYPE_VERACK;
			bool ok = CBNetworkCommunicatorSendMessage(self,node,ack);
			CBReleaseObject(ack);
			if (NOT ok)
				return CB_MESSAGE_ACTION_DISCONNECT; // If we cannot send an acknowledgement, exit.
			// Send version next if we have not already.
			if (NOT node->versionSent) {
				CBVersion * version = CBNetworkCommunicatorGetVersion(self, CBGetNetworkAddress(node));
				CBGetMessage(version)->expectResponse = CB_MESSAGE_TYPE_VERACK; // Expect a response for this message.
				node->acceptedTypes = CB_MESSAGE_TYPE_VERACK; // Ony allow version acknowledgement from here.
				CBGetMessage(version)->type = CB_MESSAGE_TYPE_VERSION;
				bool ok = CBNetworkCommunicatorSendMessage(self, node, CBGetMessage(version));
				CBReleaseObject(version);
				if (NOT ok)
					return CB_MESSAGE_ACTION_DISCONNECT;
				node->versionSent = true;
			}else
				// Sent already. End of handshake
				endHandshake = true;
		}
	}else if (node->receive->type == CB_MESSAGE_TYPE_VERACK) {
		node->versionAck = true;
		node->acceptedTypes &= ~CB_MESSAGE_TYPE_VERACK; // Got the version acknowledgement. Do not need it again
		if (node->versionMessage)
			// We already have their version message. That's all!
			endHandshake = true;
		else
			// We need their version.
			node->acceptedTypes |= CB_MESSAGE_TYPE_VERSION;
	}
	if (endHandshake) {
		// The end of the handshake. Allow pings, inventory advertisments, address requests, address broadcasts and alerts.
		node->acceptedTypes |= CB_MESSAGE_TYPE_PING;
		node->acceptedTypes |= CB_MESSAGE_TYPE_INV;
		node->acceptedTypes |= CB_MESSAGE_TYPE_GETADDR;
		node->acceptedTypes |= CB_MESSAGE_TYPE_ADDR;
		node->acceptedTypes |= CB_MESSAGE_TYPE_ALERT;
		if (self->flags & CB_NETWORK_COMMUNICATOR_AUTO_DISCOVERY) {
			// Request addresses
			CBMessage * getaddr = CBNewMessageByObject(self->events);
			getaddr->type = CB_MESSAGE_TYPE_GETADDR;
			// Send the message without an expected response. We do not expect an "addr" message because the node may not send us anything if they do not have addresses to give.
			bool ok = CBNetworkCommunicatorSendMessage(self,node,getaddr);
			CBReleaseObject(getaddr);
			if (NOT ok)
				return CB_MESSAGE_ACTION_DISCONNECT;
			node->getAddresses = true;
		}
	}
	return CB_MESSAGE_ACTION_CONTINUE;
}
CBOnMessageReceivedAction CBNetworkCommunicatorProcessMessageAutoPingPong(CBNetworkCommunicator * self,CBNode * node){
	if (node->receive->type == CB_MESSAGE_TYPE_PING
		&& node->versionMessage->version >= CB_PONG_VERSION
		&& self->version >= CB_PONG_VERSION){
		node->receive->type = CB_MESSAGE_TYPE_PONG;
		if(NOT CBNetworkCommunicatorSendMessage(self, node, node->receive))
			return CB_MESSAGE_ACTION_DISCONNECT;
	}else if (node->receive->type == CB_MESSAGE_TYPE_PONG){
		if(self->version < CB_PONG_VERSION || node->versionMessage->version < CB_PONG_VERSION)
			return CB_MESSAGE_ACTION_DISCONNECT; // This node should not be sending pong messages.
		// No longer want a pong
		node->acceptedTypes &= ~CB_MESSAGE_TYPE_PONG;
	}
	return CB_MESSAGE_ACTION_CONTINUE;
}
void CBNetworkCommunicatorOnCanReceive(void * vself,void * vnode){
	CBNetworkCommunicator * self = vself;
	CBNode * node = vnode;
	// Node kindly has some data available in the socket buffer.
	if (NOT node->receive) {
		// New message to be received.
		node->receive = CBNewMessageByObject(self->events);
		node->headerBuffer = malloc(24); // Twenty-four bytes for the message header.
		node->messageReceived = 0; // So far received nothing.
		CBSocketAddEvent(node->receiveEvent, self->recvTimeOut); // From now on use timeout for receiving data.
	}
	if (NOT node->receivedHeader) { // Not received the complete message header yet.
		 int32_t num = CBSocketReceive(node->socketID, node->headerBuffer + node->messageReceived, 24 - node->messageReceived);
		switch (num) {
			case CB_SOCKET_CONNECTION_CLOSE:
				free(node->headerBuffer); // Free the buffer
				CBNetworkCommunicatorDisconnect(self, node, 7200,false); // Remove with penalty for disconnection
				return;
			case CB_SOCKET_FAILURE:
				// Failure so remove node.
				free(node->headerBuffer); // Free the buffer
				CBNetworkCommunicatorDisconnect(self, node, 0,false);
				return;
			default:
				// Did read some bytes
				node->messageReceived += num;
				if (node->messageReceived == 24)
					// Hurrah! The header has been received.
					CBNetworkCommunicatorOnHeaderRecieved(self,node);
				return;
		}
	}else{
		// Means we have payload to fetch as we have the header but not the payload
		int32_t num = CBSocketReceive(node->socketID, CBByteArrayGetData(node->receive->bytes) + node->messageReceived, node->receive->bytes->length - node->messageReceived);
		switch (num) {
			case CB_SOCKET_CONNECTION_CLOSE:
				CBNetworkCommunicatorDisconnect(self, node, 7200, false); // Remove with penalty for disconnection
				return;
			case CB_SOCKET_FAILURE:
				// Failure so remove node.
				CBNetworkCommunicatorDisconnect(self, node, 0, false);
				return;
			default:
				// Did read some bytes
				node->messageReceived += num;
				if (node->messageReceived == node->receive->bytes->length)
					// We now have the message.
					CBNetworkCommunicatorOnMessageReceived(self,node);
				return;
		}
	}
}
void CBNetworkCommunicatorOnCanSend(void * vself,void * vnode){
	CBNetworkCommunicator * self = vself;
	CBNode * node = vnode;
	bool finished = false;
	// Can now send data
	if (NOT node->sentHeader) {
		// Need to send the header
		if (NOT node->messageSent) {
			// Create header
			node->sendingHeader = malloc(24);
			node->sendingHeader[0] = self->networkID;
			node->sendingHeader[1] = self->networkID >> 8;
			node->sendingHeader[2] = self->networkID >> 16;
			node->sendingHeader[3] = self->networkID >> 24;
			// Get the message we are sending.
			CBMessage * toSend = node->sendQueue[node->sendQueueFront];
			// Message type text
			if (toSend->type == CB_MESSAGE_TYPE_VERSION)
				memcpy(node->sendingHeader + 4, "version\0\0\0\0\0", 12);
			else if (toSend->type == CB_MESSAGE_TYPE_VERACK)
				memcpy(node->sendingHeader + 4, "verack\0\0\0\0\0\0", 12);
			else if (toSend->type == CB_MESSAGE_TYPE_ADDR)
				memcpy(node->sendingHeader + 4, "addr\0\0\0\0\0\0\0\0", 12);
			else if (toSend->type == CB_MESSAGE_TYPE_INV)
				memcpy(node->sendingHeader + 4, "inv\0\0\0\0\0\0\0\0\0", 12);
			else if (toSend->type == CB_MESSAGE_TYPE_GETDATA)
				memcpy(node->sendingHeader + 4, "getdata\0\0\0\0\0", 12);
			else if (toSend->type == CB_MESSAGE_TYPE_GETBLOCKS)
				memcpy(node->sendingHeader + 4, "getblocks\0\0\0", 12);
			else if (toSend->type == CB_MESSAGE_TYPE_GETHEADERS)
				memcpy(node->sendingHeader + 4, "getheaders\0\0", 12);
			else if (toSend->type == CB_MESSAGE_TYPE_TX)
				memcpy(node->sendingHeader + 4, "tx\0\0\0\0\0\0\0\0\0\0", 12);
			else if (toSend->type == CB_MESSAGE_TYPE_BLOCK)
				memcpy(node->sendingHeader + 4, "block\0\0\0\0\0\0\0", 12);
			else if (toSend->type == CB_MESSAGE_TYPE_HEADERS)
				memcpy(node->sendingHeader + 4, "headers\0\0\0\0\0", 12);
			else if (toSend->type == CB_MESSAGE_TYPE_GETADDR)
				memcpy(node->sendingHeader + 4, "getaddr\0\0\0\0\0", 12);
			else if (toSend->type == CB_MESSAGE_TYPE_PING)
				memcpy(node->sendingHeader + 4, "ping\0\0\0\0\0\0\0\0", 12);
			else if (toSend->type == CB_MESSAGE_TYPE_PONG)
				memcpy(node->sendingHeader + 4, "pong\0\0\0\0\0\0\0\0", 12);
			else if (toSend->type == CB_MESSAGE_TYPE_ALERT)
				memcpy(node->sendingHeader + 4, "alert\0\0\0\0\0\0\0", 12);
			else if (toSend->type == CB_MESSAGE_TYPE_ALT)
				memcpy(node->sendingHeader + 4, toSend->altText, 12);
			printf("%s SENT %s to %s (%p)\n",(self->ourIPv4->port == 45562)? "L1" : ((self->ourIPv4->port == 45563)? "L2" : "CN"),node->sendingHeader + 4,(CBGetNetworkAddress(node)->port == 45562)? "L1" : ((CBGetNetworkAddress(node)->port == 45563)? "L2" : ((CBGetNetworkAddress(node)->port == 45564) ? "CN" : "UK")),node);
			// Length
			if (toSend->bytes){
				node->sendingHeader[16] = toSend->bytes->length;
				node->sendingHeader[17] = toSend->bytes->length << 8;
				node->sendingHeader[18] = toSend->bytes->length << 16;
				node->sendingHeader[19] = toSend->bytes->length << 24;
			}else{
				memset(node->sendingHeader + 16, 0, 4);
			}
			// Checksum
			node->sendingHeader[20] = toSend->checksum[0];
			node->sendingHeader[21] = toSend->checksum[1];
			node->sendingHeader[22] = toSend->checksum[2];
			node->sendingHeader[23] = toSend->checksum[3];
		}
		int8_t len = CBSocketSend(node->socketID, node->sendingHeader + node->messageSent, 24 - node->messageSent);
		if (len == CB_SOCKET_FAILURE) {
			free(node->sendingHeader);
			CBNetworkCommunicatorDisconnect(self, node, 0, false);
		}else{
			node->messageSent += len;
			if (node->messageSent == 24) {
				// Done header
				free(node->sendingHeader);
				if (node->sendQueue[node->sendQueueFront]->bytes) {
					// Now send the bytes
					node->messageSent = 0;
					node->sentHeader = true;
				}else{
					// Nothing more to send!
					finished = true;
				}
			}
		}
	}else{
		// Sent header
		int32_t len = CBSocketSend(node->socketID, CBByteArrayGetData(node->sendQueue[node->sendQueueFront]->bytes) + node->messageSent, node->sendQueue[node->sendQueueFront]->bytes->length - node->messageSent);
		if (len == CB_SOCKET_FAILURE)
			CBNetworkCommunicatorDisconnect(self, node, 0, false);
		else{
			node->messageSent += len;
			if (node->messageSent == node->sendQueue[node->sendQueueFront]->bytes->length)
				// Sent the entire payload. Celebrate with some bonbonbonbons: http://www.youtube.com/watch?v=VWgwJfbeCeU
				finished = true;
		}
	}
	if (finished) {
		// Reset variables for next send.
		node->messageSent = 0;
		node->sentHeader = false;
		// Done sending message.
		if (node->sendQueue[node->sendQueueFront]->expectResponse){
			CBSocketAddEvent(node->receiveEvent, self->responseTimeOut); // Expect response.
			// Add to list of expected responses.
			if (node->typesExpectedNum == CB_MAX_RESPONSES_EXPECTED) {
				CBNetworkCommunicatorDisconnect(self, node, CB_24_HOURS, false);
				return;
			}
			node->typesExpected[node->typesExpectedNum] = node->sendQueue[node->sendQueueFront]->expectResponse;
			node->typesExpectedNum++;
		}
		// Remove message from queue.
		node->sendQueueSize--;
		CBReleaseObject(node->sendQueue[node->sendQueueFront]);
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
void CBNetworkCommunicatorOnHeaderRecieved(CBNetworkCommunicator * self,CBNode * node){
	// Make a CBByteArray. ??? Could be modified not to use a CBByteArray, but it is cleaner this way and easier to maintain.
	CBByteArray * header = CBNewByteArrayWithData(node->headerBuffer, 24, self->events);
	if (CBByteArrayReadInt32(header, 0) != self->networkID)
		// The network ID bytes is not what we are looking for. We will have to remove the node.
		CBNetworkCommunicatorDisconnect(self, node, CB_24_HOURS, false);
	CBMessageType type;
	uint32_t size;
	bool error = false;
	CBByteArray * typeBytes = CBNewByteArraySubReference(header, 4, 12);
	if (NOT memcmp(CBByteArrayGetData(typeBytes),"version\0\0\0\0\0",12)) {
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
	}else if (NOT memcmp(CBByteArrayGetData(typeBytes), "verack\0\0\0\0\0\0", 12)){
		// Version acknowledgement message
		if (node->acceptedTypes & CB_MESSAGE_TYPE_VERACK){
			size = CBByteArrayReadInt32(header, 16);
			if (size) // Must contain zero payload.
				error = true;
			else
				type = CB_MESSAGE_TYPE_VERACK;
		}else error = true;
	}else if (NOT memcmp(CBByteArrayGetData(typeBytes), "addr\0\0\0\0\0\0\0\0", 12)){
		// Address broadcast message
		if (node->acceptedTypes & CB_MESSAGE_TYPE_ADDR){
			size = CBByteArrayReadInt32(header, 16);
			if (size > CB_ADDRESS_BROADCAST_MAX_SIZE)
				error = true;
			else
				type = CB_MESSAGE_TYPE_ADDR;
		}else error = true;
	}else if (NOT memcmp(CBByteArrayGetData(typeBytes), "inv\0\0\0\0\0\0\0\0\0", 12)){
		// Inventory broadcast message
		if (node->acceptedTypes & CB_MESSAGE_TYPE_INV){
			size = CBByteArrayReadInt32(header, 16);
			if (size > CB_INVENTORY_BROADCAST_MAX_SIZE)
				error = true;
			else
				type = CB_MESSAGE_TYPE_INV;
		}else error = true;
	}else if (NOT memcmp(CBByteArrayGetData(typeBytes), "getdata\0\0\0\0\0", 12)){
		// Get data message
		if (node->acceptedTypes & CB_MESSAGE_TYPE_GETDATA){
			size = CBByteArrayReadInt32(header, 16);
			if (size > CB_GET_DATA_MAX_SIZE)
				error = true;
			else
				type = CB_MESSAGE_TYPE_GETDATA;
		}else error = true;
	}else if (NOT memcmp(CBByteArrayGetData(typeBytes), "getblocks\0\0\0", 12)){
		// Get blocks message
		if (node->acceptedTypes & CB_MESSAGE_TYPE_BLOCK){
			size = CBByteArrayReadInt32(header, 16);
			if (size > CB_GET_BLOCKS_MAX_SIZE)
				error = true;
			else
				type = CB_MESSAGE_TYPE_GETBLOCKS;
		}else error = true;
	}else if (NOT memcmp(CBByteArrayGetData(typeBytes), "getheaders\0\0", 12)){
		// Get headers message
		if (node->acceptedTypes & CB_MESSAGE_TYPE_GETHEADERS){
			size = CBByteArrayReadInt32(header, 16);
			if (size > CB_GET_HEADERS_MAX_SIZE)
				error = true;
			else
				type = CB_MESSAGE_TYPE_GETHEADERS;
		}else error = true;
	}else if (NOT memcmp(CBByteArrayGetData(typeBytes), "tx\0\0\0\0\0\0\0\0\0\0", 12)){
		// Transaction message
		if (node->acceptedTypes & CB_MESSAGE_TYPE_TX){
			size = CBByteArrayReadInt32(header, 16);
			if (size > CB_TRANSACTION_MAX_SIZE)
				error = true;
			else
				type = CB_MESSAGE_TYPE_TX;
		}else error = true;
	}else if (NOT memcmp(CBByteArrayGetData(typeBytes), "block\0\0\0\0\0\0\0", 12)){
		// Block message
		if (node->acceptedTypes & CB_MESSAGE_TYPE_BLOCK){
			size = CBByteArrayReadInt32(header, 16);
			if (size > CB_BLOCK_MAX_SIZE + 89) // Plus 89 for header.
				error = true;
			else
				type = CB_MESSAGE_TYPE_BLOCK;
		}else error = true;
	}else if (NOT memcmp(CBByteArrayGetData(typeBytes), "headers\0\0\0\0\0", 12)){
		// Block headers message
		if (node->acceptedTypes & CB_MESSAGE_TYPE_HEADERS){
			size = CBByteArrayReadInt32(header, 16);
			if (size > CB_BLOCK_HEADERS_MAX_SIZE)
				error = true;
			else
				type = CB_MESSAGE_TYPE_HEADERS;
		}else error = true;
	}else if (NOT memcmp(CBByteArrayGetData(typeBytes), "getaddr\0\0\0\0\0", 12)){
		// Get Addresses message
		if (node->acceptedTypes & CB_MESSAGE_TYPE_GETADDR){
			size = CBByteArrayReadInt32(header, 16);
			if (size) // Should be empty
				error = true;
			else
				type = CB_MESSAGE_TYPE_GETADDR;
		}else error = true;
	}else if (NOT memcmp(CBByteArrayGetData(typeBytes), "ping\0\0\0\0\0\0\0\0", 12)){
		// Ping message
		if (node->acceptedTypes & CB_MESSAGE_TYPE_PING){
			size = CBByteArrayReadInt32(header, 16);
			if (size > 8 || ((node->versionMessage->version < CB_PONG_VERSION || self->version < CB_PONG_VERSION) && size)) // Should be empty before version 60000 or 8 bytes.
				error = true;
			else
				type = CB_MESSAGE_TYPE_PING;
		}else error = true;
	}else if (NOT memcmp(CBByteArrayGetData(typeBytes), "pong\0\0\0\0\0\0\0\0", 12)){
		// Pong message
		if (node->acceptedTypes & CB_MESSAGE_TYPE_PONG){
			size = CBByteArrayReadInt32(header, 16);
			if (size > 8)
				error = true;
			else
				type = CB_MESSAGE_TYPE_PONG;
		}else error = true;
	}else if (NOT memcmp(CBByteArrayGetData(typeBytes), "alert\0\0\0\0\0\0\0", 12)){
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
	printf("%s RECEIVED %s from %s (%p)\n",(self->ourIPv4->port == 45562)? "L1" : ((self->ourIPv4->port == 45563)? "L2" : "CN"),CBByteArrayGetData(typeBytes),(CBGetNetworkAddress(node)->port == 45562)? "L1" : ((CBGetNetworkAddress(node)->port == 45563)? "L2" : ((CBGetNetworkAddress(node)->port == 45564) ? "CN" : "UK")),node);
	CBReleaseObject(typeBytes);
	if (error) {
		// Error with the message header type or length
		CBNetworkCommunicatorDisconnect(self, node, CB_24_HOURS, false);
		CBReleaseObject(header);
	}
	// If this is a response we have been waiting for, no longer wait for it
	for (uint8_t x = 0; x < node->typesExpectedNum; x++) {
		if (type == node->typesExpected[x]) {
			bool remove = true;
			if (self->flags & CB_NETWORK_COMMUNICATOR_AUTO_HANDSHAKE)
				if (type == CB_MESSAGE_TYPE_VERACK && NOT node->versionMessage){
					// We expect the version
					node->typesExpected[x] = CB_MESSAGE_TYPE_VERSION;
					remove = false;
				}
			if (remove) {
				node->typesExpectedNum--;
				memmove(node->typesExpected + x, node->typesExpected + x + 1, node->typesExpectedNum - x);
				if (NOT node->typesExpectedNum)
					// No more responses expected, back to usual timeOut.
					CBSocketAddEvent(node->receiveEvent, self->timeOut);
			}
			break;
		}
	}
	// The type and size is OK, make the message
	node->receive->type = type;
	// Get checksum
	node->receive->checksum[0] = CBByteArrayGetByte(header, 20);
	node->receive->checksum[1] = CBByteArrayGetByte(header, 21);
	node->receive->checksum[2] = CBByteArrayGetByte(header, 22);
	node->receive->checksum[3] = CBByteArrayGetByte(header, 23);
	// Message is now ready. Free the header.
	CBReleaseObject(header); // Took the header buffer which should be freed here.
	if (size) {
		node->receive->bytes = CBNewByteArrayOfSize(size, self->events);
		// Change variables for receiving the payload.
		node->receivedHeader = true;
		node->messageReceived = 0;
	}else
		// Got the message
		CBNetworkCommunicatorOnMessageReceived(self,node);
}
void CBNetworkCommunicatorOnLoopError(void * vself){
	CBNetworkCommunicator * self = vself;
	CBGetMessage(self)->events->onErrorReceived(CB_ERROR_NETWORK_COMMUNICATOR_LOOP_FAIL,"The socket event loop failed. Stoping the CBNetworkCommunicator...");
	self->eventLoop = 0;
	CBNetworkCommunicatorStop(self);
}
void CBNetworkCommunicatorOnMessageReceived(CBNetworkCommunicator * self,CBNode * node){
	// Check checksum
	uint8_t hash[32];
	uint8_t hash2[32];
	if (node->receive->bytes) {
		CBSha256(CBByteArrayGetData(node->receive->bytes), node->receive->bytes->length, hash);
		CBSha256(hash, 32, hash2);
	}else{
		hash2[0] = 0x5D;
		hash2[1] = 0xF6;
		hash2[2] = 0xE0;
		hash2[3] = 0xE2;
	}
	if (memcmp(hash2, node->receive->checksum, 4)) {
		// Checksum failure. There is no excuse for this. Drop the node. Why have checksums anyway???
		CBNetworkCommunicatorDisconnect(self, node, CB_24_HOURS, false);
		return;
	}
	// Deserialise and give the onMessageReceived or onAlternativeMessageReceived event
	uint32_t len;
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
			if (node->versionMessage->version >= 60000 && self->version >= 60000){
				node->receive = realloc(node->receive, sizeof(CBPingPong));
				CBGetObject(node->receive)->free = CBFreePingPong;
				len = CBPingPongDeserialise(CBGetPingPong(node->receive));
			}else len = 0;
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
	if (NOT node->receive->bytes) {
		if (len) {
			// Error
			CBNetworkCommunicatorDisconnect(self, node, CB_24_HOURS, false);
			return;
		}
	}else if (len != node->receive->bytes->length){
		// Error
		CBNetworkCommunicatorDisconnect(self, node, CB_24_HOURS, false);
		return;
	}
	// Deserialisation was sucessful.
	// Call event callback
	CBOnMessageReceivedAction action = self->events->onMessageReceived(self->callbackHandler,self,node);
	if (action == CB_MESSAGE_ACTION_CONTINUE) {
		// Automatic responses
		if (self->flags & CB_NETWORK_COMMUNICATOR_AUTO_HANDSHAKE)
			// Auto handshake responses
			action = CBNetworkCommunicatorProcessMessageAutoHandshake(self,node);
		if (self->flags & CB_NETWORK_COMMUNICATOR_AUTO_DISCOVERY && action == CB_MESSAGE_ACTION_CONTINUE)
			// Auto discovery responses
			action = CBNetworkCommunicatorProcessMessageAutoDiscovery(self,node);
		if (self->flags & CB_NETWORK_COMMUNICATOR_AUTO_PING && action == CB_MESSAGE_ACTION_CONTINUE)
			// Auto ping response
			action = CBNetworkCommunicatorProcessMessageAutoPingPong(self, node);
	}
	// Release objects and get ready for next message
	switch (action) {
		case CB_MESSAGE_ACTION_DISCONNECT:
			// Node misbehaving. Disconnect.
			printf("%s DISCONNECT FROM %s (%p) BY %i\n",(self->ourIPv4->port == 45562)? "L1" : ((self->ourIPv4->port == 45563)? "L2" : "CN"),(CBGetNetworkAddress(node)->port == 45562)? "L1" : ((CBGetNetworkAddress(node)->port == 45563)? "L2" : ((CBGetNetworkAddress(node)->port == 45564) ? "CN" : "UK")),node,node->receive->type);
			CBNetworkCommunicatorDisconnect(self, node, CB_24_HOURS, false);
			break;
		case CB_MESSAGE_ACTION_STOP:
			CBNetworkCommunicatorStop(self);
			break;
		case CB_MESSAGE_ACTION_CONTINUE:
			CBReleaseObject(node->receive);
			// Reset variables for receiving the next message
			node->receivedHeader = false;
			node->receive = NULL;
		default:
			break;
	}
}
void CBNetworkCommunicatorOnTimeOut(void * vself,void * vnode,CBTimeOutType type){
	CBNetworkCommunicator * self = vself;
	CBNode * node = vnode;
	if (type == CB_TIMEOUT_RECEIVE && NOT node->messageReceived && NOT node->receivedHeader) {
		// Not responded
		if (node->typesExpectedNum)
			type = CB_TIMEOUT_RESPONSE;
		else
			// No response expected but node did not send any information for a long time.
			type = CB_TIMEOUT_NO_DATA;
	}
	if (CBGetNetworkAddress(node)->public && CBGetNetworkAddress(node)->ip){ // Cannot take node as address in the case where the IP is NULL
		CBRetainObject(node); // Retain node for returning to addresses list
		CBNetworkCommunicatorDisconnect(self,node, CB_24_HOURS, false); // Remove CBNetworkAddress. 1 day penalty.
		// Convert node to address and add it back to the addresses list.
		CBNetworkAddress * addr = realloc(node, sizeof(*addr));
		CBAddressManagerTakeAddress(self->addresses, addr);
	}else
		CBNetworkCommunicatorDisconnect(self,node, CB_24_HOURS, false); // Remove CBNetworkAddress. 1 day penalty.
	// Send event for network timeout. The node will be NULL if it wasn't retained elsewhere.
	self->events->onTimeOut(self->callbackHandler,self,node, type);
}
bool CBNetworkCommunicatorSendMessage(CBNetworkCommunicator * self,CBNode * node,CBMessage * message){
	if (node->sendQueueSize == CB_SEND_QUEUE_MAX_SIZE)
		return false;
	// Serialise message if needed.
	if (NOT message->bytes) {
		uint32_t len;
		switch (message->type) {
			case CB_MESSAGE_TYPE_VERSION:
				len = CBVersionCalculateLength(CBGetVersion(message));
				if (NOT len)
					return false;
				message->bytes = CBNewByteArrayOfSize(len, self->events);
				len = CBVersionSerialise(CBGetVersion(message));
				break;
			case CB_MESSAGE_TYPE_ADDR:
				// "CBAddressBroadcastCalculateLength" cannot fail.
				message->bytes = CBNewByteArrayOfSize(CBAddressBroadcastCalculateLength(CBGetAddressBroadcast(message)), self->events);
				len = CBAddressBroadcastSerialise(CBGetAddressBroadcast(message));
				break;
			case CB_MESSAGE_TYPE_INV:
			case CB_MESSAGE_TYPE_GETDATA:
				// "CBInventoryBroadcastCalculateLength" cannot fail.
				message->bytes = CBNewByteArrayOfSize(CBInventoryBroadcastCalculateLength(CBGetInventoryBroadcast(message)), self->events);
				len = CBInventoryBroadcastSerialise(CBGetInventoryBroadcast(message));
				break;
			case CB_MESSAGE_TYPE_GETBLOCKS:
			case CB_MESSAGE_TYPE_GETHEADERS:
				// "CBGetBlocksCalculateLength" cannot fail.
				message->bytes = CBNewByteArrayOfSize(CBGetBlocksCalculateLength(CBGetGetBlocks(message)), self->events);
				len = CBGetBlocksSerialise(CBGetGetBlocks(message));
				break;
			case CB_MESSAGE_TYPE_TX:
				len = CBTransactionCalculateLength(CBGetTransaction(message));
				if (NOT len)
					return false;
				message->bytes = CBNewByteArrayOfSize(len, self->events);
				len = CBTransactionSerialise(CBGetTransaction(message));
				break;
			case CB_MESSAGE_TYPE_BLOCK:
				len = CBBlockCalculateLength(CBGetBlock(message),true);
				if (NOT len)
					return false;
				message->bytes = CBNewByteArrayOfSize(len, self->events);
				len = CBBlockSerialise(CBGetBlock(message),true); // true -> Including transactions.
				break;
			case CB_MESSAGE_TYPE_HEADERS:
				// "CBBlockHeadersCalculateLength" cannot fail.
				message->bytes = CBNewByteArrayOfSize(CBBlockHeadersCalculateLength(CBGetBlockHeaders(message)), self->events);
				len = CBBlockHeadersSerialise(CBGetBlockHeaders(message));
				break;
			case CB_MESSAGE_TYPE_PING:
				if (node->versionMessage->version >= 60000 && self->version >= 60000){
					message->bytes = CBNewByteArrayOfSize(8, self->events);
					len = CBPingPongSerialise(CBGetPingPong(message));
				}
			case CB_MESSAGE_TYPE_PONG:
				message->bytes = CBNewByteArrayOfSize(8, self->events);
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
	node->sendQueue[(node->sendQueueFront + node->sendQueueSize) % CB_SEND_QUEUE_MAX_SIZE] = message; // Node will send the message from this.
	if (node->sendQueueSize == 0){
		bool ok = CBSocketAddEvent(node->sendEvent, self->sendTimeOut);
		if (NOT ok)
			return false;
	}
	node->sendQueueSize++;
	CBRetainObject(message);
	return true;
}
void CBNetworkCommunicatorSendPings(void * vself){
	CBNetworkCommunicator * self = vself;
	CBMessage * ping = CBNewMessageByObject(self->events);
	ping->type = CB_MESSAGE_TYPE_PING;
	if (self->version >= CB_PONG_VERSION) {
		CBPingPong * pingPong = CBNewPingPong(rand(),self->events);
		CBGetMessage(pingPong)->type = CB_MESSAGE_TYPE_PING;
		for (uint16_t x = 0; x < self->addresses->nodesNum; x++) {
			if (self->addresses->nodes[x]->acceptedTypes & CB_MESSAGE_TYPE_PING){ // Only send ping if they can send ping.
				if(self->addresses->nodes[x]->versionMessage->version >= CB_PONG_VERSION){
					CBNetworkCommunicatorSendMessage(self, self->addresses->nodes[x], CBGetMessage(pingPong));
					CBGetMessage(pingPong)->expectResponse = CB_MESSAGE_TYPE_PONG; // Expect a pong.
					self->addresses->nodes[x]->acceptedTypes |= CB_MESSAGE_TYPE_PONG;
				}else
					CBNetworkCommunicatorSendMessage(self, self->addresses->nodes[x], CBGetMessage(ping));
			}
		}
		CBReleaseObject(pingPong);
	}else for (uint16_t x = 0; x < self->addresses->nodesNum; x++)
		if (self->addresses->nodes[x]->acceptedTypes & CB_MESSAGE_TYPE_PING) // Only send ping if they can send ping.
			CBNetworkCommunicatorSendMessage(self, self->addresses->nodes[x], CBGetMessage(ping));
	CBReleaseObject(ping);
}
void CBNetworkCommunicatorSetAddressManager(CBNetworkCommunicator * self,CBAddressManager * addrMan){
	CBRetainObject(addrMan);
	self->addresses = addrMan;
}
void CBNetworkCommunicatorSetAlternativeMessages(CBNetworkCommunicator * self,CBByteArray * altMessages,uint32_t * altMaxSizes){
	CBRetainObject(altMessages);
	self->alternativeMessages = altMessages;
	self->altMaxSizes = altMaxSizes;
}
void CBNetworkCommunicatorSetOurIPv4(CBNetworkCommunicator * self,CBNetworkAddress * ourIPv4){
	CBRetainObject(ourIPv4);
	self->ourIPv4 = ourIPv4;
}
void CBNetworkCommunicatorSetOurIPv6(CBNetworkCommunicator * self,CBNetworkAddress * ourIPv6){
	CBRetainObject(ourIPv6);
	self->ourIPv6 = ourIPv6;
}
void CBNetworkCommunicatorSetUserAgent(CBNetworkCommunicator * self,CBByteArray * userAgent){
	CBRetainObject(userAgent);
	self->userAgent = userAgent;
}
bool CBNetworkCommunicatorStart(CBNetworkCommunicator * self){
	// Create the socket event loop
	if (NOT CBNewEventLoop(&self->eventLoop,CBNetworkCommunicatorOnLoopError, CBNetworkCommunicatorOnTimeOut, self)){
		// Cannot create event loop
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_NETWORK_COMMUNICATOR_LOOP_CREATE_FAIL,"The CBNetworkCommunicator event loop could not be created.");
		return false;
	}
	self->isStarted = true;
	return true; // All OK.
}
void CBNetworkCommunicatorStartListening(CBNetworkCommunicator * self){
	printf("%s TRY LISTENING\n",(self->ourIPv4->port == 45562)? "L1" : ((self->ourIPv4->port == 45563)? "L2" : "CN"));
	if (self->addresses->reachablity & CB_IP_IPv4) {
		// Create new bound IPv4 socket
		if (CBSocketBind(&self->listeningSocketIPv4, false, self->ourIPv4->port)){
			// Add event for accepting connection for both sockets
			if (CBSocketCanAcceptEvent(&self->acceptEventIPv4,self->eventLoop, self->listeningSocketIPv4, CBNetworkCommunicatorAcceptConnection)) {
				if(CBSocketAddEvent(self->acceptEventIPv4, 0)) // No timeout for listening for incomming connections.
					// Start listening on IPv4
					if(CBSocketListen(self->listeningSocketIPv4, self->maxIncommingConnections)){
						// Is listening
						printf("%s IS LISTENING\n",(self->ourIPv4->port == 45562)? "L1" : ((self->ourIPv4->port == 45563)? "L2" : "CN"));
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
	if (self->addresses->reachablity & CB_IP_IPv6) {
		// Create new bound IPv6 socket
		if (CBSocketBind(&self->listeningSocketIPv6, true, self->ourIPv6->port)){
			// Add event for accepting connection for both sockets
			if (CBSocketCanAcceptEvent(&self->acceptEventIPv6,self->eventLoop, self->listeningSocketIPv6, CBNetworkCommunicatorAcceptConnection)) {
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
	// Disconnect all the nodes
	printf("%s DISCONNECTING WITH %i NODES\n",(self->ourIPv4->port == 45562)? "L1" : ((self->ourIPv4->port == 45563)? "L2" : "CN"),self->addresses->nodesNum);
	uint16_t removeNum = self->addresses->nodesNum; // Assign to temporary variable because "self->addresses->nodesNum" will decrement each iteration.
	for (uint16_t x = 0; x < removeNum; x++)
		CBNetworkCommunicatorDisconnect(self, self->addresses->nodes[x], 0, true); // "true" we are stopping.
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
	if (self->attemptingOrWorkingConnections >= self->maxConnections) {
		return; // Cannot connect to any more nodes
	}
	uint16_t connectTo = self->maxConnections - self->attemptingOrWorkingConnections;
	u_int64_t addrNum = CBAddressManagerGetNumberOfAddresses(self->addresses);
	if (addrNum < connectTo) {
		connectTo = addrNum; // Cannot connect to any more than the address we have
	}
	CBNetworkAddressLocator * addrs = CBAddressManagerGetAddresses(self->addresses, connectTo);
	for (u_int64_t x = 0; (addrs + x)->addr != NULL; x++) {
		CBNode * node = CBNewNodeByTakingNetworkAddress((addrs + x)->addr);
		node->incomming = false;
		uint8_t bucketIndex = (addrs + x)->bucketIndex;
		uint16_t addrIndex = (addrs + x)->addrIndex;
		CBConnectReturn res = CBNetworkCommunicatorConnect(self, node);
		if ((res == CB_CONNECT_BAD || res == CB_CONNECT_FAIL) && CBGetNetworkAddress(node)->public) {
			// Convert back to address and set the pointer in the bucket.
			(self->addresses->buckets + bucketIndex)->addresses[addrIndex] = realloc(node, sizeof(CBNetworkAddress));
			// Add penalty if bad
			if (res == CB_CONNECT_BAD)
				(self->addresses->buckets + bucketIndex)->addresses[addrIndex]->score -= 3600;
		}else{
			// Remove from addresses becasue the connection was ok, we had a failure and we should not return the address or there is no support for this address.
			CBBucket * bucket = (self->addresses->buckets + bucketIndex);
			bucket->addrNum--;
			if (bucket->addrNum) {
				// Move memory down.
				if (bucket->addrNum - addrIndex)
					memmove(bucket->addresses + addrIndex, bucket->addresses + addrIndex + 1, (bucket->addrNum - addrIndex) * sizeof(*bucket->addresses));
				// Reallocate memory.
				bucket->addresses = realloc(bucket->addresses, sizeof(*bucket->addresses) * bucket->addrNum);
			}
			// Release from addresses.
			CBReleaseObject(node);
		}
		CBReleaseObject(node);
	}
	free(addrs);
}
