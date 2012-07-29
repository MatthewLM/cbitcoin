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
 @brief File for weak linked functions for dependency injection. All these functions are unimplemented. The functions include the crytography functions which are key for the functioning of bitcoin. The threading dependencies were designed with POSIX threads in mind. Sockets must be non-blocking and use an events type system. The events do not need to be handled by a seperate thread by the dependency implementation because cbitcoin will create new threads when messages need to be handled. The use of the sockets is designed to be compatible with libevent. See the dependecies folder for implementations.
 */

#ifndef CBDEPENDENCIESH
#define CBDEPENDENCIESH

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "CBConstants.h"

// Use weak linking so these functions can be implemented outside of the library.

#pragma weak CBSha256
#pragma weak CBRipemd160
#pragma weak CBSha160
#pragma weak CBEcdsaVerify
#pragma weak CBNewMutex
#pragma weak CBFreeMutex
#pragma weak CBMutexLock
#pragma weak CBMutexUnlock
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
#pragma weak CBCloseSocket
#pragma weak CBExitEventLoop

// CRYPTOGRAPHIC DEPENDENCIES

/*
 @brief SHA-256 cryptographic hash function.
 @param data A pointer to the byte data to hash.
 @param length The length of the data to hash.
 @returns A pointer to a 32-byte hash, allocated in the function
 */
u_int8_t * CBSha256(u_int8_t * data,u_int16_t length);
/*
 @brief RIPEMD-160 cryptographic hash function.
 @param data A pointer to the byte data to hash.
 @param length The length of the data to hash.
 @returns A pointer to a 20-byte hash, allocated in the function
 */
u_int8_t * CBRipemd160(u_int8_t * data,u_int16_t length);
/*
 @brief SHA-1 cryptographic hash function.
 @param data A pointer to the byte data to hash.
 @param length The length of the data to hash.
 @returns A pointer to a 10-byte hash, allocated in the function
 */
u_int8_t * CBSha160(u_int8_t * data,u_int16_t length);
/*
 @brief Verifies an ECDSA signature. This function must stick to the cryptography requirements in OpenSSL version 1.0.0 or any other compatible version. There may be compatibility problems when using libraries or code other than OpenSSL since OpenSSL does not adhere fully to the SEC1 ECDSA standards. This could cause security problems in your code. If in doubt, stick to OpenSSL.
 @param signature BER encoded signature bytes.
 @param sigLen The length of the signature bytes.
 @param hash A 32 byte hash for checking the signature against.
 @param pubKey Public key bytes to check this signature with.
 @param keyLen The length of the public key bytes.
 @returns true if the signature is valid and false if invalid.
 */
bool CBEcdsaVerify(u_int8_t * signature,u_int8_t sigLen,u_int8_t * hash,const u_int8_t * pubKey,u_int8_t keyLen);

// THREADING DEPENDENCIES

/*
 @brief Creates a new mutex.
 @param mutexID The mutex id to be set with newly allocated new memory.
 @returns true if the mutex was successfully created and false if the mutex could not be created.
 */
bool CBNewMutex(u_int64_t * mutexID);
/*
 @brief Frees a new mutex including the ID. It is assured that no thread locks are active before this is called as threads should be canceled.
 @param mutexID The mutex id to free.
 */
void CBFreeMutex(u_int64_t mutexID);
/*
 @brief Obtains a mutex lock when next available.
 @param mutexID The mutex id.
 */
void CBMutexLock(u_int64_t mutexID);
/*
 @brief Unlocks a mutex for other threads.
 @param mutexID The mutex id.
 */
void CBMutexUnlock(u_int64_t mutexID);

// NETWORKING DEPENDENCIES

/*
 @brief Creates a new TCP/IP socket. The socket should use a non-blocking mode.
 @param socketID The socket id. This could be a pointer such as with windows sockets. It is 64 bits to cover 64-bit system pointers.
 @param IPv6 true if the socket is used to connect to the IPv6 network.
 @returns true if the socket was successfully created and false if the socket could not be created.
 */
bool CBNewSocket(u_int64_t * socketID,bool IPv6);
/*
 @brief Binds the host machine and a port number to a new socket.
 @param socketID The socket id to set to the new socket.
 @param IPv6 true if we are binding a socket for the IPv6 network.
 @param port The port to bind to.
 @returns true if the bind was sucessful and false otherwise.
 */
bool CBSocketBind(u_int64_t * socketID,bool IPv6,u_int16_t port);
/*
 @brief Begin connecting to an external host with a socket. This should be non-blocking.
 @param socketID The socket id
 @param IP 16 bytes for an IPv6 address to connect to.
 @param IPv6 True if IP address is for the IPv6 network. A IPv6 address can represent addresses for IPv4 too. To avoid the need to detect this, a boolean can be used.
 @param port Port to connect to.
 @returns true if the function was sucessful and false otherwise.
 */
bool CBSocketConnect(u_int64_t socketID,u_int8_t * IP,bool IPv6,u_int16_t port);
/*
 @brief Begin listening for incomming connections on a bound socket. This should be non-blocking. 
 @param socketID The socket id
 @param maxConnections The maximum incomming connections to allow.
 @returns true if function was sucessful and false otherwise.
 */
bool CBSocketListen(u_int64_t socketID,u_int16_t maxConnections);
/*
 @brief Accepts an incomming connections on a bound socket. This should be non-blocking.
 @param socketID The socket id
 @param connectionSocketID A socket id for a new socket for the connection.
 @param IP 16 bytes to be set by the function for the IP of the incomming connection.
 @returns true if function was sucessful and false otherwise.
 */
bool CBSocketAccept(u_int64_t socketID,u_int64_t * connectionSocketID,u_int8_t * IP);
/*
 @brief Starts a event loop for socket events on a seperate thread. Access to the loop id should be thread safe. 
 @param onError If the event loop fails during execution of the thread, this function should be called.
 @param onDidTimeout The function to call for timeout events. The second argument is for the node given by events.
 @param communicator A CBNetworkCommunicator to pass to all event functions (first parameter), including "onError" and "onDidTimeout"
 @returns A loop id for adding events. 0 on failure.
 */
u_int64_t CBNewEventLoop(void (*onError)(void *),void (*onDidTimeout)(void *,void *),void * communicator);
/*
 @brief Creates an event where a listening socket is available for accepting a connection. The event should be persistent and not issue timeouts.
 @param loopID The loop id for socket events.
 @param socketID The socket id
 @param onCanAccept The function to call for the event. Accepts "onEventArg" and the socket ID.
 @returns The event ID or 0 on failure.
 */
u_int64_t CBSocketCanAcceptEvent(u_int64_t loopID,u_int64_t socketID,void (*onCanAccept)(void *,u_int64_t));
/*
 @brief Sets a function pointer for the event where a socket has connected. The event only needs to fire once on the successful connection or timeout.
 @param loopID The loop id for socket events.
 @param socketID The socket id
 @param onDidConnect The function to call for the event.
 @param node The node to send to the "onDidConnect" or "onDidTimeout" function.
 @returns The event ID or 0 on failure.
 */
u_int64_t CBSocketDidConnectEvent(u_int64_t loopID,u_int64_t socketID,void (*onDidConnect)(void *,void *),void * node);
/*
 @brief Sets a function pointer for the event where a socket is available for sending data. This should be persistent.
 @param loopID The loop id for socket events.
 @param socketID The socket id
 @param onCanSend The function to call for the event.
 @param node The node to send to the "onCanSend" or "onDidTimeout" function.
 @returns The event ID or 0 on failure.
 */
u_int64_t CBSocketCanSendEvent(u_int64_t loopID,u_int64_t socketID,void (*onCanSend)(void *,void *),void * node);
/*
 @brief Sets a function pointer for the event where a socket is available for receiving data. This should be persistent.
 @param loopID The loop id for socket events.
 @param socketID The socket id
 @param onCanReceive The function to call for the event.
 @param node The node to send to the "onCanReceive" or "onDidTimeout" function.
 @returns The event ID or 0 on failure.
 */
u_int64_t CBSocketCanReceiveEvent(u_int64_t loopID,u_int64_t socketID,void (*onCanReceive)(void *,void *),void * node);
/*
 @brief Adds an event to be pending.
 @param eventID The event ID to add.
 @param timeout The time in seconds to issue a timeout for the event. 0 for no timeout.
 @returns true if sucessful, false otherwise.
 */
bool CBSocketAddEvent(u_int64_t eventID,u_int16_t timeout);
/*
 @brief Removes an event so no more events are made.
 @param eventID The event ID to remove
 @returns true if sucessful, false otherwise.
 */
bool CBSocketRemoveEvent(u_int64_t eventID);
/*
 @brief Makes an event non-pending and frees it.
 @param eventID The event to free.
 */
void CBSocketFreeEvent(u_int64_t eventID);
/*
 @brief Sends data to a socket. This should be non-blocking.
 @param socketID The socket id to send to.
 @param data The data bytes to send.
 @param len The length of the data to send.
 @returns The number of bytes actually sent, and CB_SOCKET_FAILURE on failure that suggests further data cannot be sent.
 */
int32_t CBSocketSend(u_int64_t socketID,u_int8_t * data,u_int32_t len);
/*
 @brief Receives data from a socket. This should be non-blocking.
 @param socketID The socket id to receive data from.
 @param data The data bytes to write the data to.
 @param len The length of the data.
 @returns The number of bytes actually written into "data", CB_SOCKET_CONNECTION_CLOSE on connection closure, 0 on no bytes received, and CB_SOCKET_FAILURE on failure.
 */
int32_t CBSocketReceive(u_int64_t socketID,u_int8_t * data,u_int32_t len);
/*
 @brief Closes a socket. The id should be freed, as well as any other data relating to this socket.
 @param socketID The socket id to be closed.
 */
void CBCloseSocket(u_int64_t socketID);
/*
 @brief Exits an event loop and frees all data relating to it.
 @param loopID The loop ID. If zero, do nothing.
 */
void CBExitEventLoop(u_int64_t loopID);

#endif
