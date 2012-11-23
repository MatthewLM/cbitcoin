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
 @brief Used for communicating to other peers. The network communicator can send and receive bitcoin messages and uses function pointers for message handlers. The timeouts are in milliseconds. It is important to understant that a CBNetworkCommunicator does not guarentee thread safety for everything. Thread safety is only given to the "peers" list. This means it is completely okay to add and remove peers from multiple threads. Two threads may try to access the list at once such as if the CBNetworkCommunicator receives a socket timeout event and tries to remove an peer at the same time as a thread made by a program using cbitcoin tries to add a new peer. When using a CBNetworkCommunicator, threading and networking dependencies need to be satisfied, @see CBDependencies.h Inherits CBObject
*/

#ifndef CBNETWORKCOMMUNICATORH
#define CBNETWORKCOMMUNICATORH

//  Includes

#include "CBPeer.h"
#include "CBDependencies.h"
#include "CBAddressManager.h"
#include "CBInventoryBroadcast.h"
#include "CBGetBlocks.h"
#include "CBTransaction.h"
#include "CBBlockHeaders.h"
#include "CBPingPong.h"
#include "CBAlert.h"

/**
 @brief Structure for CBNetworkCommunicator objects. @see CBNetworkCommunicator.h
*/
typedef struct {
	CBObject base; /**< CBObject base structure */
	uint32_t networkID; /**< The 4 byte id for sending and receiving messages for a given network. */
	CBNetworkCommunicatorFlags flags; /**< Flags for the operation of the CBNetworkCommunicator. */
	int32_t version; /**< Used for automatic handshaking. This version will be advertised to peers. */
	CBVersionServices services; /**< Used for automatic handshaking. These services will be advertised */
	CBByteArray * userAgent; /**< Used for automatic handshaking. This user agent will be advertised. */
	int32_t blockHeight; /** Set to the current block height for advertising to peers during the automated handshake. */
	CBNetworkAddress * ourIPv4; /**< IPv4 network address for us. */
	CBNetworkAddress * ourIPv6; /**< IPv6 network address for us. */
	uint32_t attemptingOrWorkingConnections; /**< All connections being attempted or sucessful */
	uint32_t maxConnections; /**< Maximum number of peers allowed to connect to. */
	uint32_t numIncommingConnections; /**< Number of incomming connections made */
	uint32_t maxIncommingConnections; /**< Maximum number of incomming connections. */
	CBAddressManager * addresses; /**< All addresses both connected and unconnected */
	uint16_t heartBeat; /**< If the CB_NETWORK_COMMUNICATOR_AUTO_PING flag is set, the CBNetworkCommunicator will send a "ping" message to all peers after this interval. bitcoin-qt uses 1800 (30 minutes) */
	uint16_t timeOut; /**< Time of zero contact from a peer before timeout. bitcoin-qt uses 5400 (90 minutes) */
	uint16_t sendTimeOut; /**< Time to wait for a socket to be ready to write before a timeout. */
	uint16_t recvTimeOut; /**< When receiving data after the initial response, the time to wait for the following data before timeout. */
	uint16_t responseTimeOut; /**< Time to wait for a peer to respond to a request before timeout.  */
	uint16_t connectionTimeOut; /**< Time to wait for a socket to connect before timeout. */
	CBByteArray * alternativeMessages; /**< Alternative messages to accept. This should be the 12 byte command names each after another with nothing between. Pass NULL for no alternative message types. */
	uint32_t * altMaxSizes; /**< Sizes for the alternative messages. Will be freed by this object, so malloc this and give it to this object. Send in NULL for a default CB_BLOCK_MAX_SIZE. */
	uint64_t listeningSocketIPv4; /**< The id of a listening socket on the IPv4 network. */
	uint64_t listeningSocketIPv6; /**< The id of a listening socket on the IPv6 network. */
	bool isListeningIPv4; /**< True when listening for incomming connections on the IPv4 network. False when not. */
	bool isListeningIPv6; /**< True when listening for incomming connections on the IPv6 network. False when not. */
	uint64_t eventLoop; /**< Socket event loop */
	uint64_t acceptEventIPv4; /**< Event for accepting connections on IPv4 */
	uint64_t acceptEventIPv6; /**< Event for accepting connections on IPv6 */
	uint64_t nonce; /**< Value sent in version messages to check for connections to self */
	uint64_t pingTimer; /**< Timer for ping event */
	bool isStarted; /**< True if the CBNetworkCommunicator is running. */
	void * callbackHandler; /**< Sent to event callbacks */
	bool stoppedListening; /**< True if listening was stopped because there are too many connections */
	void (*onTimeOut)(void *,void *,void *,CBTimeOutType); /**< Timeout event callback with a void pointer argument for the callback handler followed by a CBNetworkCommunicator and CBNetworkAddress. The callback should return as quickly as possible. Use threads for operations that would otherwise delay the event loop for too long. The second argument is the CBNetworkCommunicator responsible for the timeout. The third argument is the peer with the timeout. Lastly there is the CBTimeOutType */
	CBOnMessageReceivedAction (*onMessageReceived)(void *,void *,void *); /**< The callback for when a message has been received from a peer. The first argument in the void pointer for the callback handler. The second argument is the CBNetworkCommunicator responsible for receiving the message. The third argument is the CBNetworkAddress peer the message was received from. Return the action that should be done after returning. Access the message by the "receive" feild in the CBNetworkAddress peer. Lookup the type of the message and then cast and/or handle the message approriately. The alternative message bytes can be found in the peer's "alternativeTypeBytes" field. Do not delay the thread for very long. */
	void (*onNetworkError)(void *,void *); /**< Called when both IPv4 and IPv6 fails. Has an argument for the callback handler and then the CBNetworkCommunicator. */
	void (*logError)(char *,...); /**< Pointer to error callback */
} CBNetworkCommunicator;

/**
 @brief Creates a new CBNetworkCommunicator object.
 @returns A new CBNetworkCommunicator object.
 */
CBNetworkCommunicator * CBNewNetworkCommunicator(void (*logError)(char *,...));

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
bool CBInitNetworkCommunicator(CBNetworkCommunicator * self,void (*logError)(char *,...));

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
void CBNetworkCommunicatorAcceptConnection(void * vself,uint64_t socket);
/**
 @brief Returns true if it is beleived the network address can be connected to, otherwise false.
 @param self The CBNetworkCommunicator object.
 @param addr The CBNetworkAddress.
 @returns true if it is beleived the peer can be connected to, otherwise false.
 */
bool CBNetworkCommunicatorCanConnect(CBNetworkCommunicator * self,CBNetworkAddress * addr);
/**
 @brief Connects to a peer. This peer will be added to the peer list if it connects correctly.
 @param self The CBNetworkCommunicator object.
 @param peer The peer to connect to.
 @returns CB_CONNECT_OK if successful. CB_CONNECT_NO_SUPPORT if the IP version is not supported. CB_CONNECT_BAD if the connection failed and the address will be penalised. CB_CONNECT_FAIL if the connection failed but the address will not be penalised.
 */
CBConnectReturn CBNetworkCommunicatorConnect(CBNetworkCommunicator * self,CBPeer * peer);
/**
 @brief Callback for the connection to a peer.
 @param vself The CBNetworkCommunicator object.
 @param vpeer The CBPeer that connected.
 */
void CBNetworkCommunicatorDidConnect(void * vself,void * vpeer);
/**
 @brief Disconnects a peer.
 @param self The CBNetworkCommunicator object.
 @param peer The peer.
 @param penalty Penalty to the score of the address.
 @param stopping If true, do not call "onNetworkError" because the CBNetworkCommunicator is stopping.
 */
void CBNetworkCommunicatorDisconnect(CBNetworkCommunicator * self,CBPeer * peer,uint32_t penalty,bool stopping);
/**
 @brief Gets a new version message for this.
 @param self The CBNetworkCommunicator object.
 @param addRecv The CBNetworkAddress of the receipient.
 */
CBVersion * CBNetworkCommunicatorGetVersion(CBNetworkCommunicator * self,CBNetworkAddress * addRecv);
/**
 @brief Processes a new received message for auto discovery.
 @param self The CBNetworkCommunicator object.
 @param peer The peer
 @returns true if peer should be disconnected, false otherwise.
 */
CBOnMessageReceivedAction CBNetworkCommunicatorProcessMessageAutoDiscovery(CBNetworkCommunicator * self,CBPeer * peer);
/**
 @brief Processes a new received message for auto handshaking.
 @param self The CBNetworkCommunicator object.
 @param peer The peer
 @returns true if peer should be disconnected, false otherwise.
 */
CBOnMessageReceivedAction CBNetworkCommunicatorProcessMessageAutoHandshake(CBNetworkCommunicator * self,CBPeer * peer);
/**
 @brief Processes a new received message for auto ping pongs.
 @param self The CBNetworkCommunicator object.
 @param peer The peer
 @returns true if peer should be disconnected, false otherwise.
 */
CBOnMessageReceivedAction CBNetworkCommunicatorProcessMessageAutoPingPong(CBNetworkCommunicator * self,CBPeer * peer);
/**
 @brief Called when a peer socket is ready for reading.
 @param vself The CBNetworkCommunicator object.
 @param vpeer The CBPeer index with data to read.
 */
void CBNetworkCommunicatorOnCanReceive(void * vself,void * vpeer);
/**
 @brief Called when a peer socket is ready for writing.
 @param vself The CBNetworkCommunicator object.
 @param vpeer The CBPeer
 */
void CBNetworkCommunicatorOnCanSend(void * vself,void * vpeer);
/**
 @brief Called when a header is received.
 @param self The CBNetworkCommunicator object.
 @param peer The CBPeer.
 */
void CBNetworkCommunicatorOnHeaderRecieved(CBNetworkCommunicator * self,CBPeer * peer);
/**
 @brief Called on an error with the socket event loop. The error event is given with CB_ERROR_NETWORK_COMMUNICATOR_LOOP_FAIL.
 @param vself The CBNetworkCommunicator object.
 */
void CBNetworkCommunicatorOnLoopError(void * vself);
/**
 @brief Called when an entire message is received.
 @param self The CBNetworkCommunicator object.
 @param peer The CBPeer.
 */
void CBNetworkCommunicatorOnMessageReceived(CBNetworkCommunicator * self,CBPeer * peer);
/**
 @brief Called on a timeout error. The peer is removed.
 @param vself The CBNetworkCommunicator object.
 @param vpeer The CBPeer index which timedout.
 @param type The type of the timeout
 */
void CBNetworkCommunicatorOnTimeOut(void * vself,void * vpeer,CBTimeOutType type);
/**
 @brief Sends a message by placing it on the send queue. Will serialise standard messages (unless serialised already) but not alternative messages or alert messages.
 @param self The CBNetworkCommunicator object.
 @param peer The CBPeer.
 @param message The CBMessage to send.
 @returns true if successful, false otherwise.
 */
bool CBNetworkCommunicatorSendMessage(CBNetworkCommunicator * self,CBPeer * peer,CBMessage * message);
/**
 @brief Sends pings to all connected peers.
 @param self The CBNetworkCommunicator object.
 */
void CBNetworkCommunicatorSendPings(void * vself);
/**
 @brief Sets the CBAddressManager.
 @param self The CBNetworkCommunicator object.
 @param addr The CBAddressManager
 */
void CBNetworkCommunicatorSetAddressManager(CBNetworkCommunicator * self,CBAddressManager * addrMan);
/**
 @brief Sets the alternative messages
 @param self The alternative messages as a CBByteArray with 12 characters per message command, one after the other.
 @param altMaxSizes An allocated memory block of 32 bit integers with the max sizes for the alternative messages.
 @param addr The CBAddressManager
 */
void CBNetworkCommunicatorSetAlternativeMessages(CBNetworkCommunicator * self,CBByteArray * altMessages,uint32_t * altMaxSizes);
/**
 @brief Sets the IPv4 address for the CBNetworkCommunicator.
 @param self The CBNetworkCommunicator object.
 @param addr The IPv4 address as a CBNetworkAddress.
 */
void CBNetworkCommunicatorSetOurIPv4(CBNetworkCommunicator * self,CBNetworkAddress * ourIPv4);
/**
 @brief Sets the IPv6 address for the CBNetworkCommunicator.
 @param self The CBNetworkCommunicator object.
 @param addr The IPv6 address as a CBNetworkAddress.
 */
void CBNetworkCommunicatorSetOurIPv6(CBNetworkCommunicator * self,CBNetworkAddress * ourIPv6);
/**
 @brief Sets the user agent.
 @param self The CBNetworkCommunicator object.
 @param addr The user agent as a CBByteArray.
 */
void CBNetworkCommunicatorSetUserAgent(CBNetworkCommunicator * self,CBByteArray * userAgent);
/**
 @brief Starts a CBNetworkCommunicator by connecting to the peers in the peers list. This starts the socket event loop.
 @param self The CBNetworkCommunicator object.
 @returns true if the CBNetworkCommunicator started successfully, false otherwise.
 */
bool CBNetworkCommunicatorStart(CBNetworkCommunicator * self);
/**
 @brief Causes the CBNetworkCommunicator to begin listening for incomming connections. "isListeningIPv4" and/or "isListeningIPv6" should be set to true if either IPv4 or IPv6 sockets are active.
 @param vself The CBNetworkCommunicator object.
 */
void CBNetworkCommunicatorStartListening(CBNetworkCommunicator * self);
/**
 @brief Starts the timer event for sending pings (heartbeat).
 @param vself The CBNetworkCommunicator object.
 */
void CBNetworkCommunicatorStartPings(CBNetworkCommunicator * self);
/**
 @brief Closes all connections. This may be neccessary in case of failure in which case CBNetworkCommunicatorStart can be tried again to reconnect to the listed peers.
 @param vself The CBNetworkCommunicator object.
 */
void CBNetworkCommunicatorStop(CBNetworkCommunicator * self);
/**
 @brief Stops listening for both IPv6 connections and IPv4 connections.
 @param vself The CBNetworkCommunicator object.
 */
void CBNetworkCommunicatorStopListening(CBNetworkCommunicator * self);
/**
 @brief Stops the timer event for sending pings (heartbeat).
 @param vself The CBNetworkCommunicator object.
 */
void CBNetworkCommunicatorStopPings(CBNetworkCommunicator * self);
/**
 @brief Looks at the stored addresses and tries to connect to addresses up to the maximum number of allowed connections or as many as there are in the case the maximum number of connections is greater than the number of addresses, plus connected peers.
 @param self The CBNetworkCommunicator object.
 */
void CBNetworkCommunicatorTryConnections(CBNetworkCommunicator * self);

#endif
