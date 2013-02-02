//
//  CBPeer.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 22/07/2012.
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

/**
 @file
 @brief Contains data for managing peer connections. Inherits CBNetworkAddress
*/

#ifndef CBPEERH
#define CBPEERH

//  Includes

#include "CBNetworkAddress.h"
#include "CBVersion.h"

// Constants

#define CB_MAX_RESPONSES_EXPECTED 3 // A pong, an inventory broadcast and an address broadcast.
#define CB_NODE_MAX_ADDRESSES_24_HOURS 100 // Maximum number of addresses accepted by a peer in 24 hours. ??? Not implemented
#define CB_SEND_QUEUE_MAX_SIZE 10 // Sent no more than 10 messages at once to a peer.

/**
 @brief Structure for CBPeer objects. @see CBPeer.h
*/
typedef struct{
	CBNetworkAddress base; /**< CBNetworkAddress base structure */
	uint64_t socketID; /**< Not used in the bitcoin protocol. This is used by cbitcoin to store a socket ID for a connection to a CBNetworkAddress. The socket here is not closed when the CBNetworkAddress is freed so needs to be closed elsewhere. */
	CBMessage * receive; /**< Receiving message. NULL if not receiving. This message is exclusive to the peer. */
	CBMessage * sendQueue[CB_SEND_QUEUE_MAX_SIZE]; /**< Messages to send to this peer. NULL if not sending anything. */
	uint8_t sendQueueSize; /**< Upto 10 messages in queue */
	uint8_t sendQueueFront; /**< Index of the front of the queue */
	uint32_t messageSent; /**< Used by a CBNetworkCommunicator to store the message length send. When the header is sent, 24 bytes are taken off. */
	bool sentHeader; /**< True if the sending message's header has been sent. */
	uint8_t * sendingHeader; /**< Stores header to send */
	bool getAddresses; /* True is asked for addresses, false otherwise. */
	uint64_t receiveEvent; /**< Event for receving data from this peer */
	uint64_t sendEvent; /**< Event for sending data from this peer */
	uint64_t connectEvent; /**< Event for connecting to the peer. */
	bool versionSent; /**< True if the version was sent to this peer */
	CBVersion * versionMessage; /**< The version message from this peer. */
	bool versionAck; /**< This peer acknowledged the version message. */
	uint8_t * headerBuffer; /**< Used by a CBNetworkCommunicator to read the message header before processing. */
	uint32_t messageReceived; /**< Used by a CBNetworkCommunicator to store the message length received. When the header is received 24 bytes are taken off. */
	uint16_t acceptedTypes; /**< Set messages that will be accepted on receiving. When a peer tries to send another message, drop the peer. Starts empty */
	bool receivedHeader; /**< True if the receiving message's header has been received. */
	int64_t timeOffset; /**< The offset from the system time this peer has */
	uint64_t timeBroadcast; /**< Time of the last own address brodcast. */
	bool connectionWorking; /**< True when the connection has been successful and the peer has ben added to the CBAddressManager. */
	CBMessageType typesExpected[CB_MAX_RESPONSES_EXPECTED]; /**< List of expected responses */
	uint8_t typesExpectedNum; /**< Number of expected responses */
	bool incomming; /**< Node from an incomming connection if true */
	uint64_t downloadTime; /**< Download time for this peer (in millisconds), not taking the latency into account. Use for determining effeciency. */
	uint32_t downloadAmount; /**< Downloaded bytes measured for this peer. */
	uint64_t latencyTime; /**< Total measured response time (in millisconds) */
	uint16_t responses; /**< Number of measured responses. */
	uint64_t latencyTimerStart; /**< Used to measure latency (in millisconds). */
	uint64_t downloadTimerStart; /**< Used to measure download time (in millisconds). */
	CBMessageType latencyExpected; /**< Message expected for the latency measurement */
} CBPeer;

/**
 @brief Creates a new CBPeer object.
 @returns A new CBPeer object.
 */
CBPeer * CBNewPeerByTakingNetworkAddress(CBNetworkAddress * addr);

/**
 @brief Gets a CBPeer from another object. Use this to avoid casts.
 @param self The object to obtain the CBPeer from.
 @returns The CBPeer object.
 */
CBPeer * CBGetPeer(void * self);

/**
 @brief Initialises a CBPeer object
 @param self The CBPeer object to initialise
 @returns true on success, false on failure.
 */
bool CBInitPeerByTakingNetworkAddress(CBPeer * self);

#endif
