//
//  CBDependencies.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 15/05/2012.
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
 @brief File for weak linked functions for dependency injection. All these functions are unimplemented. The functions include the crytography functions which are key for the functioning of bitcoin. Sockets must be non-blocking and asynchronous. The use of the sockets is designed to be compatible with libevent. The random number functions should be cryptographically secure. See the dependecies folder for implementations.
 */

#ifndef CBDEPENDENCIESH
#define CBDEPENDENCIESH

#include <stdbool.h>
#include <inttypes.h>
#include "CBConstants.h"

// Use weak linking so these functions can be implemented outside of the library.

// union for dependency objects

typedef union{
	void * ptr;
	int i;
} CBDepObject;

// CRYPTOGRAPHIC DEPENDENCIES

// Constants and macros

#define CB_PUBKEY_SIZE 33
#define CB_PRIVKEY_SIZE 32

// Functions

void CBAddPoints(unsigned char * point1, unsigned char * point2);
#pragma weak CBAddPoints

void CBKeyIncrementPubkey(unsigned char * pubKey);
#pragma weak CBKeyIncrementPubkey

void CBKeyGetPublicKey(unsigned char * privKey, unsigned char * pubKey);
#pragma weak CBKeyGetPublicKey

int CBKeySign(unsigned char * privKey, unsigned char * hash, unsigned char * signature);
#pragma weak CBKeySign

/**
 @brief SHA-256 cryptographic hash function.
 @param data A pointer to the byte data to hash.
 @param length The length of the data to hash.
 @param output A pointer to hold a 32-byte hash.
 */
void CBSha256(unsigned char * data, int length, unsigned char * output);
#pragma weak CBSha256

/**
 @brief SHA-512 cryptographic hash function.
 @param data A pointer to the byte data to hash.
 @param length The length of the data to hash.
 @param output A pointer to hold a 64-byte hash.
 */
void CBSha512(unsigned char * data, int len, unsigned char * output);
#pragma weak CBSha512

/**
 @brief RIPEMD-160 cryptographic hash function.
 @param data A pointer to the byte data to hash.
 @param length The length of the data to hash.
 @param output A pointer to hold a 20-byte hash.
 */
void CBRipemd160(unsigned char * data, int length, unsigned char * output);
#pragma weak CBRipemd160

/**
 @brief SHA-1 cryptographic hash function.
 @param data A pointer to the byte data to hash.
 @param length The length of the data to hash.
 @param output A pointer to hold a 10-byte hash.
 */
void CBSha160(unsigned char * data, int length, unsigned char * output);
#pragma weak CBSha160

/**
 @brief Verifies an ECDSA signature. This function must stick to the cryptography requirements in OpenSSL version 1.0.0 or any other compatible version. There may be compatibility problems when using libraries or code other than OpenSSL since OpenSSL does not adhere fully to the SEC1 ECDSA standards. This could cause security problems in your code. If in doubt, stick to OpenSSL.
 @param signature BER encoded signature bytes.
 @param sigLen The length of the signature bytes.
 @param hash A 32 byte hash for checking the signature against.
 @param pubKey Public key bytes to check this signature with.
 @param keyLen The length of the public key bytes.
 @returns true if the signature is valid and false if invalid.
 */
bool CBEcdsaVerify(unsigned char * signature, int sigLen, unsigned char * hash, unsigned char * pubKey, int keyLen);
#pragma weak CBEcdsaVerify

// NETWORKING DEPENDENCIES

// Constants and Macros

typedef enum{
	CB_TIMEOUT_CONNECT,
	CB_TIMEOUT_SEND,
	CB_TIMEOUT_RECEIVE,
	CB_TIMEOUT_CONNECT_ERROR
} CBTimeOutType;

typedef enum{
	CB_SOCKET_OK,
	CB_SOCKET_NO_SUPPORT,
	CB_SOCKET_BAD
} CBSocketReturn;

#define CB_SOCKET_CONNECTION_CLOSE -1
#define CB_SOCKET_FAILURE -2

// Functions

/**
 @brief Creates a new TCP/IP socket. The socket should use a non-blocking mode.
 @param socketID Pointer to CBDepObject for the socket
 @param IPv6 true if the socket is used to connect to the IPv6 network.
 @returns CB_SOCKET_OK if the socket was successfully created, CB_SOCKET_NO_SUPPORT and CB_SOCKET_BAD if the socket could not be created for any other reason.
 */
CBSocketReturn CBNewSocket(CBDepObject * socketID, bool IPv6);
#pragma weak CBNewSocket

/*
 @brief Binds the host machine and a port number to a new socket.
 @param socketID The socket id to set to the new socket.
 @param IPv6 true if we are binding a socket for the IPv6 network.
 @param port The port to bind to.
 @returns true if the bind was sucessful and false otherwise.
 */
bool CBSocketBind(CBDepObject * socketID, bool IPv6, int port);
#pragma weak CBSocketBind

/**
 @brief Begin connecting to an external host with a socket. This should be non-blocking.
 @param socketID The socket id
 @param IP 16 bytes for an IPv6 address to connect to.
 @param IPv6 True if IP address is for the IPv6 network. A IPv6 address can represent addresses for IPv4 too. To avoid the need to detect this, a boolean can be used.
 @param port Port to connect to.
 @returns true if the function was sucessful and false otherwise.
 */
bool CBSocketConnect(CBDepObject socketID, unsigned char * IP, bool IPv6, int port);
#pragma weak CBSocketConnect

/**
 @brief Begin listening for incomming connections on a bound socket. This should be non-blocking. 
 @param socketID The socket id
 @param maxConnections The maximum incomming connections to allow.
 @returns true if function was sucessful and false otherwise.
 */
bool CBSocketListen(CBDepObject socketID, int maxConnections);
#pragma weak CBSocketListen

/**
 @brief Accepts an incomming IPv4 connection on a bound socket. This should be non-blocking.
 @param socketID The socket id
 @param connectionSocketID A socket id for a new socket for the connection.
 @param sockAddr The CBSocketAddress of the connection.
 @returns true on success and false on failure.
 */
bool CBSocketAccept(CBDepObject socketID, CBDepObject * connectionSocketID, void * sockAddr);
#pragma weak CBSocketAccept

/**
 @brief Starts a event loop for socket events on a seperate thread. Access to the loop id should be thread safe.
 @param loopID A CBDepObject storing an integer or pointer representation of the new event loop.
 @param onError If the event loop fails during execution of the thread, this function should be called.
 @param onDidTimeout The function to call for timeout event. The second argument is for the peer. The third is for the timeout type. For receiving data, the timeout should be CB_TIMEOUT_RECEIVE. The CBNetworkCommunicator will determine if it should be changed to CB_TIMEOUT_RESPONSE.
 @param communicator A CBNetworkCommunicator to pass to all event functions (first parameter), including "onError" and "onDidTimeout"
 @returns true on success, false on failure.
 */
bool CBNewEventLoop(CBDepObject * loopID, void (*onError)(void *), void (*onDidTimeout)(void *, void *, CBTimeOutType), void * communicator);
#pragma weak CBNewEventLoop

bool CBNetworkCommunicatorLoadDNS(void * comm, char * domain);
#pragma weak CBNetworkCommunicatorLoadDNS

/**
 @brief Runs a callback on the event loop.
 @param loopID The loop ID
 @param block If true, do not return until the callback has been executed.
 @returns true if sucessful, false otherwise.
 */
bool CBRunOnEventLoop(CBDepObject loopID, void (*callback)(void *), void * arg, bool block);
#pragma weak CBRunOnEventLoop

/**
 @brief Creates an event where a listening socket is available for accepting a connection. The event should be persistent and not issue timeouts.
 @param eventID The event object to set.
 @param loopID The loop id.
 @param socketID The socket id.
 @param onCanAccept The function to call for the event. Accepts "onEventArg" and the socket ID.
 @returns true on success, false on failure.
 */
bool CBSocketCanAcceptEvent(CBDepObject * eventID, CBDepObject loopID, CBDepObject socketID, void (*onCanAccept)(void *, CBDepObject));
#pragma weak CBSocketCanAcceptEvent

/**
 @brief Sets a function pointer for the event where a socket has connected. The event only needs to fire once on the successful connection or timeout.
 @param eventID The event object to set.
 @param loopID The loop id.
 @param socketID The socket id
 @param onDidConnect The function to call for the event.
 @param peer The peer to send to the "onDidConnect" or "onDidTimeout" function.
 @returns true on success, false on failure.
 */
bool CBSocketDidConnectEvent(CBDepObject * eventID, CBDepObject loopID, CBDepObject socketID, void (*onDidConnect)(void *, void *), void * peer);
#pragma weak CBSocketDidConnectEvent

/**
 @brief Sets a function pointer for the event where a socket is available for sending data. This should be persistent.
 @param eventID The event object to set.
 @param loopID The loop id.
 @param socketID The socket id
 @param onCanSend The function to call for the event.
 @param peer The peer to send to the "onCanSend" or "onDidTimeout" function.
 @returns true on success, false on failure.
 */
bool CBSocketCanSendEvent(CBDepObject * eventID, CBDepObject loopID, CBDepObject socketID, void (*onCanSend)(void *, void *), void * peer);
#pragma weak CBSocketCanSendEvent

/**
 @brief Sets a function pointer for the event where a socket is available for receiving data. This should be persistent.
 @param eventID The event object to set.
 @param loopID The loop id.
 @param socketID The socket id
 @param onCanReceive The function to call for the event.
 @param peer The peer to send to the "onCanReceive" or "onDidTimeout" function.
 @returns true on success, false on failure.
 */
bool CBSocketCanReceiveEvent(CBDepObject * eventID, CBDepObject loopID, CBDepObject socketID, void (*onCanReceive)(void *, void *), void * peer);
#pragma weak CBSocketCanReceiveEvent

/**
 @brief Adds an event to be pending.
 @param eventID The event ID to add.
 @param timeout The time in milliseconds to issue a timeout for the event. 0 for no timeout.
 @returns true if sucessful, false otherwise.
 */
bool CBSocketAddEvent(CBDepObject eventID, int timeout);
#pragma weak CBSocketAddEvent

/**
 @brief Removes an event so no more events are made.
 @param eventID The event ID to remove
 @returns true if sucessful, false otherwise.
 */
bool CBSocketRemoveEvent(CBDepObject eventID);
#pragma weak CBSocketRemoveEvent

/**
 @brief Makes an event non-pending and frees it.
 @param eventID The event to free.
 */
void CBSocketFreeEvent(CBDepObject eventID);
#pragma weak CBSocketFreeEvent

/**
 @brief Sends data to a socket. This should be non-blocking.
 @param socketID The socket id to send to.
 @param data The data bytes to send.
 @param len The length of the data to send.
 @returns The number of bytes actually sent, and CB_SOCKET_FAILURE on failure that suggests further data cannot be sent.
 */
int32_t CBSocketSend(CBDepObject socketID, unsigned char * data, int len);
#pragma weak CBSocketSend

/**
 @brief Receives data from a socket. This should be non-blocking.
 @param socketID The socket id to receive data from.
 @param data The data bytes to write the data to.
 @param len The length of the data.
 @returns The number of bytes actually written into "data", CB_SOCKET_CONNECTION_CLOSE on connection closure, 0 on no bytes received, and CB_SOCKET_FAILURE on failure.
 */
int32_t CBSocketReceive(CBDepObject socketID, unsigned char * data, int len);
#pragma weak CBSocketReceive

/**
 @brief Calls a callback every "time" seconds, until the timer is ended.
 @param loopID The loop id.
 @param timer The timer sent by reference to be set.
 @param time The number of milliseconds between each call of the callback.
 @param callback The callback function.
 @param arg The callback argument.
 */
bool CBStartTimer(CBDepObject loopID, CBDepObject * timer, int time, void (*callback)(void *), void * arg);
#pragma weak CBStartTimer

/**
 @brief Ends a timer.
 @param timer The timer.
 */
void CBEndTimer(CBDepObject timer);
#pragma weak CBEndTimer

/**
 @brief Closes a socket. The id should be freed, as well as any other data relating to this socket.
 @param socketID The socket id to be closed.
 */
void CBCloseSocket(CBDepObject socketID);
#pragma weak CBCloseSocket

/**
 @brief Exits an event loop and frees all data relating to it.
 @param loopID The loop ID. If zero, do nothing.
 */
void CBExitEventLoop(CBDepObject loopID);
#pragma weak CBExitEventLoop

// RANDOM DEPENDENCIES

/**
 @brief Returns an instance of a cryptographically secure random number generator.
 @param The generator as a pointer or integer.
 @returns true on success, false on failure.
 */
bool CBNewSecureRandomGenerator(CBDepObject * gen);
#pragma weak CBNewSecureRandomGenerator

/**
 @brief Seeds the random number generator securely.
 @param gen The generator.
 @retruns true if the random number generator was securely seeded, or false otherwise.
 */
bool CBSecureRandomSeed(CBDepObject gen);
#pragma weak CBSecureRandomSeed

/**
 @brief Seeds the generator from a 64-bit integer.
 @param gen The generator.
 @param seed The 64-bit integer.
 */
void CBRandomSeed(CBDepObject gen, long long int seed);
#pragma weak CBRandomSeed

/**
 @brief Generates an unsigned 64 bit integer.
 @param gen The generator.
 @returns The random unsigned 64-bit integer integer.
 */
unsigned long long int CBSecureRandomInteger(CBDepObject gen);
#pragma weak CBSecureRandomInteger

/**
 @brief Frees the random number generator.
 @param gen The generator.
 */
void CBFreeSecureRandomGenerator(CBDepObject gen);
#pragma weak CBFreeSecureRandomGenerator

bool CBGet32RandomBytes(unsigned char * bytes);
#pragma weak CBGet32RandomBytes

// THREADING DEPENDENCIES

void CBNewThread(CBDepObject * thread, void (*function)(void *), void * arg);
#pragma weak CBNewThread

void CBFreeThread(CBDepObject thread);
#pragma weak CBFreeThread

void CBThreadJoin(CBDepObject thread);
#pragma weak CBThreadJoin

void CBNewMutex(CBDepObject * mutex);
#pragma weak CBNewMutex

void CBFreeMutex(CBDepObject mutex);
#pragma weak CBFreeMutex

void CBMutexLock(CBDepObject mutex);
#pragma weak CBMutexLock

void CBMutexUnlock(CBDepObject mutex);
#pragma weak CBMutexUnlock

void CBNewCondition(CBDepObject * cond);
#pragma weak CBNewCondition

void CBFreeCondition(CBDepObject cond);
#pragma weak CBFreeCondition

void CBConditionWait(CBDepObject cond, CBDepObject mutex);
#pragma weak CBConditionWait

void CBConditionSignal(CBDepObject cond);
#pragma weak CBConditionSignal

int CBGetNumberOfCores(void);
#pragma weak CBGetNumberOfCores

// LOGGING DEPENDENCIES

/**
 @brief Logs an error. Should be thread-safe.
 @param error The error message followed by arguments displayed by the printf() format.
 */
void CBLogError(char * error, ...);
#pragma weak CBLogError

void CBLogWarning(char * warning, ...);
#pragma weak CBLogWarning

void CBLogVerbose(char * message, ...);
#pragma weak CBLogVerbose

void CBLogFile(char * file);
#pragma weak CBLogFile

// TIME DEPENDENCIES

/**
 @brief Millisecond precision time function
 @returns A value which when called twice will reflect the milliseconds passed between each call.
 */
long long int CBGetMilliseconds(void);
#pragma weak CBGetMilliseconds

#endif
