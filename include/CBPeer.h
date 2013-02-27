//
//  CBPeer.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 22/07/2012.
//  Copyright (c) 2012 Matthew Mitchell
//  
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

/**
 @file
 @brief Contains data for managing peer connections. Inherits CBNetworkAddress
*/

#ifndef CBPEERH
#define CBPEERH

//  Includes

#include "CBNetworkAddress.h"
#include "CBVersion.h"
#include "CBInventoryBroadcast.h"
#include "CBAssociativeArray.h"

// Constants

#define CB_MAX_RESPONSES_EXPECTED 3 // A pong, an inventory broadcast and an address broadcast.
#define CB_NODE_MAX_ADDRESSES_24_HOURS 100 // Maximum number of addresses accepted by a peer in 24 hours. ??? Not implemented
#define CB_SEND_QUEUE_MAX_SIZE 10 // Sent no more than 10 messages at once to a peer.
#define CB_PEER_NO_WORKING 0xFF // When not working on any branch.

/**
 @brief Stores a message to send in the queue with the callback to call when the message is sent.
 */
typedef struct{
	CBMessage * message;
	void (*callback)(void *, void *);
} CBSendQueueItem;

/**
 @brief Structure for CBPeer objects. @see CBPeer.h
*/
typedef struct{
	CBNetworkAddress base; /**< CBNetworkAddress base structure */
	uint64_t socketID; /**< Not used in the bitcoin protocol. This is used by cbitcoin to store a socket ID for a connection to a CBNetworkAddress. The socket here is not closed when the CBNetworkAddress is freed so needs to be closed elsewhere. */
	CBMessage * receive; /**< Receiving message. NULL if not receiving. This message is exclusive to the peer. */
	CBSendQueueItem sendQueue[CB_SEND_QUEUE_MAX_SIZE]; /**< Messages to send to this peer. NULL if not sending anything. */
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
	bool connectionWorking; /**< True when the connection has been successful and the peer has ben added to the CBNetworkAddressManager. */
	CBMessageType typesExpected[CB_MAX_RESPONSES_EXPECTED]; /**< List of expected responses */
	uint8_t typesExpectedNum; /**< Number of expected responses */
	bool incomming; /**< Node from an incomming connection if true */
	uint64_t downloadTime; /**< Download time for this peer (in millisconds), not taking the latency into account. Use for determining effeciency. */
	uint64_t downloadAmount; /**< Downloaded bytes measured for this peer. */
	uint32_t latencyTime; /**< Total measured response time (in millisconds) */
	uint16_t responses; /**< Number of measured responses. */
	uint64_t latencyTimerStart; /**< Used to measure latency (in millisconds). */
	uint64_t downloadTimerStart; /**< Used to measure download time (in millisconds). */
	CBMessageType latencyExpected; /**< Message expected for the latency measurement */
	uint32_t advertisedData; /**< The number of inventory items we have advertised but not satisfied with the actual data yet. */
	CBInventoryBroadcast * requestedData; /**< Data which is requested by this node */
	uint16_t sendDataIndex; /**< The index of the inventory broadcast to send data to a peer. */
	CBAssociativeArray expectedObjects; /**< 20 bytes of the hashes for the objects we expect the peer to provide us with and nothing else (ie. After get data requests). */
	uint8_t branchWorkingOn; /**< The branch we are receiving from this peer */
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
