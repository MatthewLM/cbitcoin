//
//  CBNetworkCommunicator.h
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

/**
 @file
 @brief Used for communicating to other nodes. The network communicator holds function pointers for receiving messages and functions to send messages. The timeouts are in seconds. It is important to understant that a CBNetworkCommunicator does not guarentee thread safety for everything. Thread safety is only given to the "nodes" list. This means it is completely okay to add and remove nodes from multiple threads. Two threads may try to access the list at once such as if the CBNetworkCommunicator receives a socket timeout event and tries to remove an node at the same time as a thread made by a program using cbitcoin tries to add a new node. When using a CBNetworkCommunicator, threading and networking dependencies need to be satisfied, @see CBDependencies.h Inherits CBObject
*/

#ifndef CBNETWORKCOMMUNICATORH
#define CBNETWORKCOMMUNICATORH

//  Includes

#include "CBNode.h"
#include "CBDependencies.h"
#include "CBAddressBroadcast.h"
#include "CBInventoryBroadcast.h"
#include "CBGetBlocks.h"
#include "CBTransaction.h"
#include "CBBlockHeaders.h"
#include "CBPingPong.h"
#include "CBAlert.h"
#include <time.h>

/**
 @brief Structure for CBNetworkCommunicator objects. @see CBNetworkCommunicator.h
*/
typedef struct {
	CBObject base; /**< CBObject base structure */
	u_int32_t networkID; /**< The 4 byte id for sending and receiving messages for a given network. */
	CBNetworkCommunicatorFlags flags; /**< Flags for the operation of the CBNetworkCommunicator. */
	CBVersion * version; /**< Used for automatic handshaking. This CBVersion will be advertised to nodes. */
	u_int16_t maxConnections; /**< Used for automatic discovery. The CBNetworkCommunicator will not connect to any more than this number of nodes */
	u_int16_t maxIncommingConnections; /**< Maximum number of incomming connections. */
	CBNetworkAddress ** addresses; /**< All known addresses which are not yet connected to. */
	u_int16_t addrNum; /**< The number of addresses for unconnected nodes. Max storage = 65536 nodes or 1.85MB. */
	CBNode ** nodes; /**< A list of pointers to CBNode for connected nodes. */
	u_int16_t nodesNum; /**< The number of connected nodes */
	u_int16_t heartBeat; /**< If the CB_NETWORK_COMMUNICATOR_AUTO_PING flag is set, the CBNetworkCommunicator will send a "ping" message to all nodes after this interval. bitcoin-qt uses 1800 (30 minutes) */
	u_int16_t timeOut; /**< Time of zero contact from a node before timeout. bitcoin-qt uses 5400 (90 minutes) */
	u_int16_t sendTimeOut; /**< Time to wait for a socket to be ready to write before a timeout. */
	u_int16_t recvTimeOut; /**< When receiving data after the initial response, the time to wait for the following data before timeout. */
	u_int16_t responseTimeOut; /**< Time to wait for a node to respond to a request before timeout.  */
	u_int16_t connectionTimeOut; /**< Time to wait for a socket to connect before timeout. */
	CBByteArray * alternativeMessages; /**< Alternative messages to accept. This should be the 12 byte command names each after another with nothing between. */
	u_int32_t * altMaxSizes; /**< Sizes for the alternative messages. Will be freed by this object, so malloc this and give it to this object. Send in NULL for a default CB_BLOCK_MAX_SIZE. */
	u_int16_t port; /**< Port used for connections. Usually 8333 for the bitcoin production network. */
	u_int64_t listeningSocketIPv4; /**< The id of a listening socket on the IPv4 network. */
	u_int64_t listeningSocketIPv6; /**< The id of a listening socket on the IPv6 network. */
	bool isListeningIPv4; /**< True when listening for incomming connections on the IPv4 network. False when not. */
	bool isListeningIPv6; /**< True when listening for incomming connections on the IPv6 network. False when not. */
	u_int64_t accessNodeListMutex; /**< A mutex for modifying the nodes list */
	u_int64_t accessAddressListMutex; /**< A mutex for modifying the adresses list */
	u_int64_t eventLoop; /**< Socket event loop */
	u_int64_t acceptEventIPv4; /**< Event for accepting connections on IPv4 */
	u_int64_t acceptEventIPv6; /**< Event for accepting connections on IPv6 */
	bool IPv4; /**< True if it is beleived IPv4 connections can be successful. */
	bool IPv6; /**< True if it is beleived IPv6 connections can be successful. */
	u_int64_t nounce; /**< Value sent in version messages to check for connections to self */
	int16_t networkTimeOffset; /**< Offset to get from system time to network time. */
	u_int64_t lastPing; /**< Time last ping was sent */
	CBEvents * events; /**< Events. */
	bool isStarted; /**< True if the CBNetworkCommunicator is running. */
} CBNetworkCommunicator;

/**
 @brief Creates a new CBNetworkCommunicator object.
 @returns A new CBNetworkCommunicator object.
 */
CBNetworkCommunicator * CBNewNetworkCommunicator(u_int32_t networkID,CBNetworkCommunicatorFlags flags,CBVersion * version,u_int16_t maxConnections,u_int16_t heartBeat,u_int16_t timeOut,CBByteArray * alternativeMessages,u_int32_t * altMaxSizes,u_int16_t port,CBEvents * events);

/**
 @brief Gets a CBNetworkCommunicator from another object. Use this to avoid casts.
 @param self The object to obtain the CBNetworkCommunicator from.
 @returns The CBNetworkCommunicator object.
 */
CBNetworkCommunicator * CBGetNetworkCommunicator(void * self);

/**
 @brief Initialises a CBNetworkCommunicator object
 @param self The CBNetworkCommunicator object to initialise
 @returns true on success, false on failure.
 */
bool CBInitNetworkCommunicator(CBNetworkCommunicator * self,u_int32_t networkID,CBNetworkCommunicatorFlags flags,CBVersion * version,u_int16_t maxConnections,u_int16_t heartBeat,u_int16_t timeOut,CBByteArray * alternativeMessages,u_int32_t * altMaxSizes,u_int16_t port,CBEvents * events);

/**
 @brief Frees a CBNetworkCommunicator object.
 @param self The CBNetworkCommunicator object to free.
 */
void CBFreeNetworkCommunicator(void * self);
 
//  Functions

/**
 @brief Accepts an incomming connection.
 @param vself The CBNetworkCommunicator object.
 @param socket The listening socket for accepting a connection.
 */
void CBNetworkCommunicatorAcceptConnection(void * vself,u_int64_t socket);
/**
 @brief Adjust the network time offset with a node's time.
 @param self The CBNetworkCommunicator object.
 @param time Time to adjust network time with.
 */
void CBNetworkCommunicatorAdjustTime(CBNetworkCommunicator * self,u_int64_t time);
/**
 @brief Returns true if it is beleived the node can be connected to, otherwise false.
 @param self The CBNetworkCommunicator object.
 @param node The node.
 @returns true if it is beleived the node can be connected to, otherwise false.
 */
bool CBNetworkCommunicatorCanConnect(CBNetworkCommunicator * self,CBNode * node);
/**
 @brief Connects to a node. This node will be added to the node list if it connects correctly.
 @param self The CBNetworkCommunicator object.
 @param node The node to connect to.
 @returns true if successful, false otherwise.
 */
bool CBNetworkCommunicatorConnect(CBNetworkCommunicator * self,CBNode * node);
/**
 @brief Callback for the connection to a node.
 @param vself The CBNetworkCommunicator object.
 @param vnode The CBNode that connected.
 */
void CBNetworkCommunicatorDidConnect(void * vself,void * vnode);
/**
 @brief Disconnects a node.
 @param self The CBNetworkCommunicator object.
 @param node The node.
 */
void CBNetworkCommunicatorDisconnect(CBNetworkCommunicator * self,CBNode * node);
/**
 @brief Determines if a CBNetworkAddress is in the "addresses" list. Compares the IP address.
 @param self The CBNetworkCommunicator object.
 @param addr The address.
 @returns If the address already exists, returns the existing object. Else returns NULL.
 */
CBNetworkAddress * CBNetworkCommunicatorGotNetworkAddress(CBNetworkCommunicator * self,CBNetworkAddress * addr);
/**
 @brief Determines if a CBNetworkAddress is in the "nodes" list. Compares the IP address.
 @param self The CBNetworkCommunicator object.
 @param addr The address.
 @returns If the address already exists as a connected node, returns the existing object. Else returns NULL.
 */
CBNode * CBNetworkCommunicatorGotNode(CBNetworkCommunicator * self,CBNetworkAddress * addr);
/**
 @brief Processes a new received message for auto discovery.
 @param self The CBNetworkCommunicator object.
 @param node The node
 @returns true if node should be disconnected, false otherwise.
 */
bool CBNetworkCommunicatorProcessMessageAutoDiscovery(CBNetworkCommunicator * self,CBNode * node);
/**
 @brief Processes a new received message for auto handshaking.
 @param self The CBNetworkCommunicator object.
 @param node The node
 @returns true if node should be disconnected, false otherwise.
 */
bool CBNetworkCommunicatorProcessMessageAutoHandshake(CBNetworkCommunicator * self,CBNode * node);
/**
 @brief Remove a CBNetworkAddress node from the nodes list.
 @param self The CBNetworkCommunicator object.
 @param node The CBNode to remove
 */
void CBNetworkCommunicatorRemoveNode(CBNetworkCommunicator * self,CBNode * node);
/**
 @brief Called when a node socket is ready for reading.
 @param vself The CBNetworkCommunicator object.
 @param vnode The CBNode index with data to read.
 */
void CBNetworkCommunicatorOnCanReceive(void * vself,void * vnode);
/**
 @brief Called when a node socket is ready for writing.
 @param vself The CBNetworkCommunicator object.
 @param vnode The CBNode
 */
void CBNetworkCommunicatorOnCanSend(void * vself,void * vnode);
/**
 @brief Called when a header is received.
 @param self The CBNetworkCommunicator object.
 @param node The CBNode.
 */
void CBNetworkCommunicatorOnHeaderRecieved(CBNetworkCommunicator * self,CBNode * node);
/**
 @brief Called on an error with the socket event loop. The error event is given with CB_ERROR_NETWORK_COMMUNICATOR_LOOP_FAIL.  
 @param vself The CBNetworkCommunicator object.
 */
void CBNetworkCommunicatorOnLoopError(void * vself);
/**
 @brief Called when an entire message is received.
 @param self The CBNetworkCommunicator object.
 @param node The CBNode.
 */
void CBNetworkCommunicatorOnMessageRecieved(CBNetworkCommunicator * self,CBNode * node);
/**
 @brief Called on a timeout error. The node is removed.
 @param vself The CBNetworkCommunicator object.
 @param vnode The CBNode index which timedout.
 */
void CBNetworkCommunicatorOnTimeOut(void * vself,void * vnode);
/**
 @brief Sends, what should be, a correctly serialised message by placing it on the send queue.
 @param self The CBNetworkCommunicator object.
 @param node The CBNode.
 @param message The CBMessage to send.
 @returns true if successful, false otherwise.
 */
bool CBNetworkCommunicatorSendMessage(CBNetworkCommunicator * self,CBNode * node,CBMessage * message);
/**
 @brief Starts a CBNetworkCommunicator by connecting to the nodes in the nodes list. This starts the socket event loop.
 @param vself The CBNetworkCommunicator object.
 @returns true if the CBNetworkCommunicator started successfully, false otherwise.
 */
bool CBNetworkCommunicatorStart(CBNetworkCommunicator * self);
/**
 @brief Causes the CBNetworkCommunicator to begin listening for incomming connections. "isListeningIPv4" and/or "isListeningIPv6" should be set to true if either IPv4 or IPv6 sockets are active.
 @param vself The CBNetworkCommunicator object.
 @param maxIncommingConnections The maximum number of incomming connections to accept.
 */
void CBNetworkCommunicatorStartListening(CBNetworkCommunicator * self,u_int16_t maxIncommingConnections);
/**
 @brief Closes all connections. This may be neccessary in case of failure in which case CBNetworkCommunicatorStart can be tried again to reconnect to the listed nodes.
 @param vself The CBNetworkCommunicator object.
 */
void CBNetworkCommunicatorStop(CBNetworkCommunicator * self);
/**
 @brief Takes a CBNode an places it into the addresses list.
 @param self The CBNetworkCommunicator object.
 @param node The CBNetworkAddress to take.
 */
void CBNetworkCommunicatorTakeAddress(CBNetworkCommunicator * self,CBNetworkAddress * addr);
/**
 @brief Takes a CBNode an places it into the nodes list.
 @param self The CBNetworkCommunicator object.
 @param node The CBNode to take.
 */
void CBNetworkCommunicatorTakeNode(CBNetworkCommunicator * self,CBNode * node);

#endif
