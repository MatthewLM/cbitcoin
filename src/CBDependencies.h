//
//  CBDependencies.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 15/05/2012.
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
 @brief File for weak linked functions for dependency injection. All these functions are unimplemented. The functions include the crytography functions which are key for the functioning of bitcoin. Sockets must be non-blocking and use an asynchronous onErrorReceived-type system. The use of the sockets is designed to be compatible with libevent. The random number functions should be cryptographically secure. See the dependecies folder for implementations.
 */

#ifndef CBDEPENDENCIESH
#define CBDEPENDENCIESH

#include <stdint.h>
#include <stdbool.h>
#include "CBConstants.h"

// Use weak linking so these functions can be implemented outside of the library.

// Weak linking for cryptographic functions.

#pragma weak CBSha256
#pragma weak CBRipemd160
#pragma weak CBSha160
#pragma weak CBEcdsaVerify

// Weak linking for networking functions.

#pragma weak CBNewSocket
#pragma weak CBSocketBind
#pragma weak CBSocketConnect
#pragma weak CBSocketListen
#pragma weak CBSocketAccept
#pragma weak CBNewEventLoop
#pragma weak CBSocketCanAcceptEvent
#pragma weak CBSocketDidConnectEvent
#pragma weak CBSocketCanSendEvent
#pragma weak CBSocketCanReceiveEvent
#pragma weak CBSocketAddEvent
#pragma weak CBSocketRemoveEvent
#pragma weak CBSocketFreeEvent
#pragma weak CBSocketSend
#pragma weak CBSocketReceive
#pragma weak CBStartTimer
#pragma weak CBEndTimer
#pragma weak CBCloseSocket
#pragma weak CBExitEventLoop

// Weak linking for random number generation functions.

#pragma weak CBNewSecureRandomGenerator
#pragma weak CBSecureRandomSeed
#pragma weak CBRandomSeed
#pragma weak CBSecureRandomInteger
#pragma weak CBFreeSecureRandomGenerator

// CRYPTOGRAPHIC DEPENDENCIES

/**
 @brief SHA-256 cryptographic hash function.
 @param data A pointer to the byte data to hash.
 @param length The length of the data to hash.
 @param output A pointer to hold a 32-byte hash.
 */
void CBSha256(uint8_t * data,uint16_t length,uint8_t * output);
/**
 @brief RIPEMD-160 cryptographic hash function.
 @param data A pointer to the byte data to hash.
 @param length The length of the data to hash.
 @param output A pointer to hold a 20-byte hash.
 */
void CBRipemd160(uint8_t * data,uint16_t length,uint8_t * output);
/**
 @brief SHA-1 cryptographic hash function.
 @param data A pointer to the byte data to hash.
 @param length The length of the data to hash.
 @param output A pointer to hold a 10-byte hash.
 */
void CBSha160(uint8_t * data,uint16_t length,uint8_t * output);
/**
 @brief Verifies an ECDSA signature. This function must stick to the cryptography requirements in OpenSSL version 1.0.0 or any other compatible version. There may be compatibility problems when using libraries or code other than OpenSSL since OpenSSL does not adhere fully to the SEC1 ECDSA standards. This could cause security problems in your code. If in doubt, stick to OpenSSL.
 @param signature BER encoded signature bytes.
 @param sigLen The length of the signature bytes.
 @param hash A 32 byte hash for checking the signature against.
 @param pubKey Public key bytes to check this signature with.
 @param keyLen The length of the public key bytes.
 @returns true if the signature is valid and false if invalid.
 */
bool CBEcdsaVerify(uint8_t * signature,uint8_t sigLen,uint8_t * hash,const uint8_t * pubKey,uint8_t keyLen);

// NETWORKING DEPENDENCIES

/**
 @brief Creates a new TCP/IP socket. The socket should use a non-blocking mode.
 @param socketID Pointer to uint64_t. Can be pointer value.
 @param IPv6 true if the socket is used to connect to the IPv6 network.
 @returns CB_SOCKET_OK if the socket was successfully created, CB_SOCKET_NO_SUPPORT and CB_SOCKET_BAD if the socket could not be created for any other reason.
 */
CBSocketReturn CBNewSocket(uint64_t * socketID,bool IPv6);
/*
 @brief Binds the host machine and a port number to a new socket.
 @param socketID The socket id to set to the new socket.
 @param IPv6 true if we are binding a socket for the IPv6 network.
 @param port The port to bind to.
 @returns true if the bind was sucessful and false otherwise.
 */
bool CBSocketBind(uint64_t * socketID,bool IPv6,uint16_t port);
/**
 @brief Begin connecting to an external host with a socket. This should be non-blocking.
 @param socketID The socket id
 @param IP 16 bytes for an IPv6 address to connect to.
 @param IPv6 True if IP address is for the IPv6 network. A IPv6 address can represent addresses for IPv4 too. To avoid the need to detect this, a boolean can be used.
 @param port Port to connect to.
 @returns true if the function was sucessful and false otherwise.
 */
bool CBSocketConnect(uint64_t socketID,uint8_t * IP,bool IPv6,uint16_t port);
/**
 @brief Begin listening for incomming connections on a bound socket. This should be non-blocking. 
 @param socketID The socket id
 @param maxConnections The maximum incomming connections to allow.
 @returns true if function was sucessful and false otherwise.
 */
bool CBSocketListen(uint64_t socketID,uint16_t maxConnections);
/**
 @brief Accepts an incomming IPv4 connection on a bound socket. This should be non-blocking.
 @param socketID The socket id
 @param connectionSocketID A socket id for a new socket for the connection.
 @returns true if function was sucessful and false otherwise.
 */
bool CBSocketAccept(uint64_t socketID,uint64_t * connectionSocketID);
/**
 @brief Starts a event loop for socket onErrorReceived on a seperate thread. Access to the loop id should be thread safe.
 @param loopID A uint64_t storing an integer or pointer representation of the new event loop.
 @param onError If the event loop fails during execution of the thread, this function should be called.
 @param onDidTimeout The function to call for timeout onErrorReceived. The second argument is for the peer given by onErrorReceived. The third is for the timeout type. For receiving data, the timeout should be CB_TIMEOUT_RECEIVE. The CBNetworkCommunicator will determine if it should be changed to CB_TIMEOUT_RESPONSE.
 @param communicator A CBNetworkCommunicator to pass to all event functions (first parameter), including "onError" and "onDidTimeout"
 @returns true on success, false on failure.
 */
bool CBNewEventLoop(uint64_t * loopID,void (*onError)(void *),void (*onDidTimeout)(void *,void *,CBTimeOutType),void * communicator);
/**
 @brief Creates an event where a listening socket is available for accepting a connection. The event should be persistent and not issue timeouts.
 @param loopID The loop id for socket onErrorReceived.
 @param socketID The socket id
 @param onCanAccept The function to call for the event. Accepts "onEventArg" and the socket ID.
 @returns true on success, false on failure.
 */
bool CBSocketCanAcceptEvent(uint64_t * eventID,uint64_t loopID,uint64_t socketID,void (*onCanAccept)(void *,uint64_t));
/**
 @brief Sets a function pointer for the event where a socket has connected. The event only needs to fire once on the successful connection or timeout.
 @param loopID The loop id for socket onErrorReceived.
 @param socketID The socket id
 @param onDidConnect The function to call for the event.
 @param peer The peer to send to the "onDidConnect" or "onDidTimeout" function.
 @returns true on success, false on failure.
 */
bool CBSocketDidConnectEvent(uint64_t * eventID,uint64_t loopID,uint64_t socketID,void (*onDidConnect)(void *,void *),void * peer);
/**
 @brief Sets a function pointer for the event where a socket is available for sending data. This should be persistent.
 @param loopID The loop id for socket onErrorReceived.
 @param socketID The socket id
 @param onCanSend The function to call for the event.
 @param peer The peer to send to the "onCanSend" or "onDidTimeout" function.
 @returns true on success, false on failure.
 */
bool CBSocketCanSendEvent(uint64_t * eventID,uint64_t loopID,uint64_t socketID,void (*onCanSend)(void *,void *),void * peer);
/**
 @brief Sets a function pointer for the event where a socket is available for receiving data. This should be persistent.
 @param loopID The loop id for socket onErrorReceived.
 @param socketID The socket id
 @param onCanReceive The function to call for the event.
 @param peer The peer to send to the "onCanReceive" or "onDidTimeout" function.
 @returns true on success, false on failure.
 */
bool CBSocketCanReceiveEvent(uint64_t * eventID,uint64_t loopID,uint64_t socketID,void (*onCanReceive)(void *,void *),void * peer);
/**
 @brief Adds an event to be pending.
 @param eventID The event ID to add.
 @param timeout The time in milliseconds to issue a timeout for the event. 0 for no timeout.
 @returns true if sucessful, false otherwise.
 */
bool CBSocketAddEvent(uint64_t eventID,uint32_t timeout);
/**
 @brief Removes an event so no more onErrorReceived are made.
 @param eventID The event ID to remove
 @returns true if sucessful, false otherwise.
 */
bool CBSocketRemoveEvent(uint64_t eventID);
/**
 @brief Makes an event non-pending and frees it.
 @param eventID The event to free.
 */
void CBSocketFreeEvent(uint64_t eventID);
/**
 @brief Sends data to a socket. This should be non-blocking.
 @param socketID The socket id to send to.
 @param data The data bytes to send.
 @param len The length of the data to send.
 @returns The number of bytes actually sent, and CB_SOCKET_FAILURE on failure that suggests further data cannot be sent.
 */
int32_t CBSocketSend(uint64_t socketID,uint8_t * data,uint32_t len);
/**
 @brief Receives data from a socket. This should be non-blocking.
 @param socketID The socket id to receive data from.
 @param data The data bytes to write the data to.
 @param len The length of the data.
 @returns The number of bytes actually written into "data", CB_SOCKET_CONNECTION_CLOSE on connection closure, 0 on no bytes received, and CB_SOCKET_FAILURE on failure.
 */
int32_t CBSocketReceive(uint64_t socketID,uint8_t * data,uint32_t len);
/**
 @brief Calls a callback every "time" seconds, until the timer is ended.
 @param loopID The loop id for onErrorReceived.
 @param timer The timer sent by reference to be set.
 @param time The number of milliseconds between each call of the callback.
 @param callback The callback function.
 @param arg The callback argument.
 */
bool CBStartTimer(uint64_t loopID,uint64_t * timer,uint16_t time,void (*callback)(void *),void * arg);
/**
 @brief Ends a timer.
 @param timer The timer sent by reference to be set.
 */
void CBEndTimer(uint64_t timer);
/**
 @brief Closes a socket. The id should be freed, as well as any other data relating to this socket.
 @param socketID The socket id to be closed.
 */
void CBCloseSocket(uint64_t socketID);
/**
 @brief Exits an event loop and frees all data relating to it.
 @param loopID The loop ID. If zero, do nothing.
 */
void CBExitEventLoop(uint64_t loopID);

// RANDOM DEPENDENCIES

/**
 @brief Returns an instance of a cryptographically secure random number generator.
 @param The generator as a pointer or integer.
 @returns true on success, false on failure.
 */
bool CBNewSecureRandomGenerator(uint64_t * gen);
/**
 @brief Seeds the random number generator securely.
 @param gen The generator.
 */
void CBSecureRandomSeed(uint64_t gen);
/**
 @brief Seeds the generator from a 64-bit integer.
 @param gen The generator.
 @param seed The 64-bit integer.
 */
void CBRandomSeed(uint64_t gen,uint64_t seed);
/**
 @brief Generates a 64 bit integer.
 @param gen The generator.
 @returns The random 64-bit integer integer.
 */
uint64_t CBSecureRandomInteger(uint64_t gen);
/**
 @brief Frees the random number generator.
 @param gen The generator.
 */
void CBFreeSecureRandomGenerator(uint64_t gen);

#endif
