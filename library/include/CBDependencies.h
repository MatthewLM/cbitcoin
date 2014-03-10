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

#include <stdint.h>
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

void CBAddPoints(uint8_t * point1, uint8_t * point2);
#pragma weak CBAddPoints
void CBKeyGetPublicKey(uint8_t * privKey, uint8_t * pubKey);
#pragma weak CBKeyGetPublicKey
uint8_t CBKeySign(uint8_t * privKey, uint8_t * hash, uint8_t * signature);
#pragma weak CBKeySign
/**
 @brief SHA-256 cryptographic hash function.
 @param data A pointer to the byte data to hash.
 @param length The length of the data to hash.
 @param output A pointer to hold a 32-byte hash.
 */
void CBSha256(uint8_t * data, uint16_t length, uint8_t * output);
#pragma weak CBSha256
/**
 @brief SHA-512 cryptographic hash function.
 @param data A pointer to the byte data to hash.
 @param length The length of the data to hash.
 @param output A pointer to hold a 64-byte hash.
 */
void CBSha512(uint8_t * data, uint16_t len, uint8_t * output);
#pragma weak CBSha512
/**
 @brief RIPEMD-160 cryptographic hash function.
 @param data A pointer to the byte data to hash.
 @param length The length of the data to hash.
 @param output A pointer to hold a 20-byte hash.
 */
void CBRipemd160(uint8_t * data, uint16_t length, uint8_t * output);
#pragma weak CBRipemd160
/**
 @brief SHA-1 cryptographic hash function.
 @param data A pointer to the byte data to hash.
 @param length The length of the data to hash.
 @param output A pointer to hold a 10-byte hash.
 */
void CBSha160(uint8_t * data, uint16_t length, uint8_t * output);
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
bool CBEcdsaVerify(uint8_t * signature, uint8_t sigLen, uint8_t * hash, uint8_t * pubKey, uint8_t keyLen);
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
bool CBSocketBind(CBDepObject * socketID, bool IPv6, uint16_t port);
#pragma weak CBSocketBind
/**
 @brief Begin connecting to an external host with a socket. This should be non-blocking.
 @param socketID The socket id
 @param IP 16 bytes for an IPv6 address to connect to.
 @param IPv6 True if IP address is for the IPv6 network. A IPv6 address can represent addresses for IPv4 too. To avoid the need to detect this, a boolean can be used.
 @param port Port to connect to.
 @returns true if the function was sucessful and false otherwise.
 */
bool CBSocketConnect(CBDepObject socketID, uint8_t * IP, bool IPv6, uint16_t port);
#pragma weak CBSocketConnect
/**
 @brief Begin listening for incomming connections on a bound socket. This should be non-blocking. 
 @param socketID The socket id
 @param maxConnections The maximum incomming connections to allow.
 @returns true if function was sucessful and false otherwise.
 */
bool CBSocketListen(CBDepObject socketID, uint16_t maxConnections);
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
bool CBSocketAddEvent(CBDepObject eventID, uint32_t timeout);
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
int32_t CBSocketSend(CBDepObject socketID, uint8_t * data, uint32_t len);
#pragma weak CBSocketSend
/**
 @brief Receives data from a socket. This should be non-blocking.
 @param socketID The socket id to receive data from.
 @param data The data bytes to write the data to.
 @param len The length of the data.
 @returns The number of bytes actually written into "data", CB_SOCKET_CONNECTION_CLOSE on connection closure, 0 on no bytes received, and CB_SOCKET_FAILURE on failure.
 */
int32_t CBSocketReceive(CBDepObject socketID, uint8_t * data, uint32_t len);
#pragma weak CBSocketReceive
/**
 @brief Calls a callback every "time" seconds, until the timer is ended.
 @param loopID The loop id.
 @param timer The timer sent by reference to be set.
 @param time The number of milliseconds between each call of the callback.
 @param callback The callback function.
 @param arg The callback argument.
 */
bool CBStartTimer(CBDepObject loopID, CBDepObject * timer, uint32_t time, void (*callback)(void *), void * arg);
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
void CBRandomSeed(CBDepObject gen, uint64_t seed);
#pragma weak CBRandomSeed
/**
 @brief Generates a 64 bit integer.
 @param gen The generator.
 @returns The random 64-bit integer integer.
 */
uint64_t CBSecureRandomInteger(CBDepObject gen);
#pragma weak CBSecureRandomInteger
/**
 @brief Frees the random number generator.
 @param gen The generator.
 */
void CBFreeSecureRandomGenerator(CBDepObject gen);
#pragma weak CBFreeSecureRandomGenerator
bool CBGet32RandomBytes(uint8_t * bytes);
#pragma weak CBGet32RandomBytes

// BLOCK CHAIN AND ACOUNTER STORAGE DEPENDENCIES

/**
 @brief Returns the database storage object used for block-chain and accounter storage. The block chain and accounter storage should share the same database, though only one of either can exist. They both should share the same database so that the data remains consistent with itself. Any storage object can use the same database.
 @param database The database storage object to set.
 @param dataDir The directory where the data files should be stored.
 @param commitGap The time in milliseconds before changes are commited to disk.
 @param cacheLimit The cache limit before a commit must be made.
 @returns true on success or false on failure.
 */
bool CBNewStorageDatabase(CBDepObject * database, char * dataDir, uint32_t commitGap, uint32_t cacheLimit);
#pragma weak CBNewStorageDatabase
/**
 @brief The data is staged for commit so that it is irreversible by CBBlockChainStorageRevert. Commits when the commitGap or cacheLimit is reached.
 @param database The database storage object.
 @returns true on success and false on failure. cbitcoin will reload the storage contents on failure. Contents should thus be recovered upon initilaising the database storage object.
 */
bool CBStorageDatabaseStage(CBDepObject database);
#pragma weak CBStorageDatabaseStage
/**
 @brief Removes all of the non-staged pending operations.
 @param database The database storage object.
 */
bool CBStorageDatabaseRevert(CBDepObject database);
#pragma weak CBStorageDatabaseRevert
/**
 @brief Frees the database storage object, and commits any staged changes.
 @param database The database storage object.
 */
void CBFreeStorageDatabase(CBDepObject database);
#pragma weak CBFreeStorageDatabase

// BLOCK CHAIN STORAGE DEPENDENCIES

/**
 @brief Returns the object used for block-chain storage.
 @param storage The storage object to set.
 @param database The database object to use.
 @param dataDir The directory where the data files should be stored.
 @param commitGap The time in milliseconds before changes are commited to disk.
 @param cacheLimit The cache limit before a commit must be made.
 @returns true on success or false on failure.
 */
bool CBNewBlockChainStorage(CBDepObject * storage, CBDepObject database);
#pragma weak CBNewBlockChainStorage
/**
 @brief Frees the block-chain storage object.
 @param iself The block-chain storage object.
 */
void CBFreeBlockChainStorage(CBDepObject iself);
#pragma weak CBFreeBlockChainStorage
/**
 @brief Determines if the block is already in the storage.
 @param validator A CBValidator object. The storage object can be found within this.
 @param blockHash The 20 bytes of the hash of the block
 @returns CB_TRUE if the block is in the storage, CB_FALSE if it isn't or CB_ERROR on error.
 */
CBErrBool CBBlockChainStorageBlockExists(void * validator, uint8_t * blockHash);
#pragma weak CBBlockChainStorageBlockExists
/**
 @brief Deletes a block.
 @param validator A CBValidator object. The storage object can be found within this.
 @param branch The branch where the block exists.
 @param blockIndex The index of the block.
 @returns true on sucess or false on failure.
 */
bool CBBlockChainStorageDeleteBlock(void * validator, uint8_t branch, uint32_t blockIndex);
#pragma weak CBBlockChainStorageDeleteBlock
/**
 @brief Deletes an unspent output reference. The ouput should still be in the block. The number of unspent outputs for the transaction should be decremented.
 @param validator A CBValidator object. The storage object can be found within this.
 @param txHash The transaction hash of this output.
 @param outputIndex The index of the output in the transaction.
 @param decrement If true decrement the number of unspent outputs for the transaction.
 @returns true on sucess or false on failure.
 */
bool CBBlockChainStorageDeleteUnspentOutput(void * validator, uint8_t * txHash, uint32_t outputIndex, bool decrement);
#pragma weak CBBlockChainStorageDeleteUnspentOutput
/**
 @brief Deletes a transaction reference once all instances have been removed. Decrement the instance count and remove if zero. The transaction should still be in the block. Reset the unspent output count to 0.
 @param validator A CBValidator object. The storage object can be found within this.
 @param txHash The transaction's hash.
 @returns true on sucess or false on failure.
 */
bool CBBlockChainStorageDeleteTransactionRef(void * validator, uint8_t * txHash);
#pragma weak CBBlockChainStorageDeleteTransactionRef
/**
 @brief Determines if there is previous data or if initial data needs to be created.
 @param iself The block-chain storage object.
 @returns true if there is previous block-chain data or false if initial data is needed.
 */
bool CBBlockChainStorageExists(CBDepObject iself);
#pragma weak CBBlockChainStorageExists
/**
 @brief Obtains the hash for a block.
 @param validator A CBValidator object. The storage object can be found within this.
 @param branch The branch where the block exists.
 @param blockIndex The index of the block.
 @param hash The hash to set.
 @returns true on sucess or false on failure.
 */
bool CBBlockChainStorageGetBlockHash(void * validator, uint8_t branch, uint32_t blockIndex, uint8_t hash[]);
#pragma weak CBBlockChainStorageGetBlockHash
/**
 @brief Obtains the header for a block, which has not be deserialised.
 @param validator A CBValidator object. The storage object can be found within this.
 @param branch The branch where the block exists.
 @param blockIndex The index of the block.
 @returns The block header on sucess or NULL on failure.
 */
void * CBBlockChainStorageGetBlockHeader(void * validator, uint8_t branch, uint32_t blockIndex);
#pragma weak CBBlockChainStorageGetBlockHeader
/**
 @brief Gets the location of a block.
 @param validator A CBValidator object. The storage object can be found within this.
 @param blockHash The 20 bytes of the hash of the block
 @param branch Will be set to the branch of the block.
 @param index Will be set to the index of the block.
 @returns CB_TRUE if found, CB_FALSE if not found and CB_ERROR on an error.
 */
CBErrBool CBBlockChainStorageGetBlockLocation(void * validator, uint8_t * blockHash, uint8_t * branch, uint32_t * index);
#pragma weak CBBlockChainStorageGetBlockLocation
/**
 @brief Obtains the timestamp for a block.
 @param validator A CBValidator object. The storage object can be found within this.
 @param branch The branch where the block exists.
 @param blockIndex The index of the block.
 @returns The time on sucess or 0 on failure.
 */
uint32_t CBBlockChainStorageGetBlockTime(void * validator, uint8_t branch, uint32_t blockIndex);
#pragma weak CBBlockChainStorageGetBlockTime
/**
 @brief Obtains the target for a block.
 @param validator A CBValidator object. The storage object can be found within this.
 @param branch The branch where the block exists.
 @param blockIndex The index of the block.
 @returns The target on sucess or 0 on failure.
 */
uint32_t CBBlockChainStorageGetBlockTarget(void * validator, uint8_t branch, uint32_t blockIndex);
#pragma weak CBBlockChainStorageGetBlockTarget
/**
 @brief Determines if a transaction exists in the index, which has more than zero unspent outputs.
 @param validator A CBValidator object. The storage object can be found within this.
 @param txHash The hash of the transaction to search for.
 @param true on success or false on failure.
 */
CBErrBool CBBlockChainStorageIsTransactionWithUnspentOutputs(void * validator, uint8_t * txHash);
#pragma weak CBBlockChainStorageIsTransactionWithUnspentOutputs
/**
 @brief Loads the basic validator information, not any branches or orphans.
 @param validator A CBValidator object. The storage object can be found within this.
 @returns true on sucess or false on failure.
 */
bool CBBlockChainStorageLoadBasicValidator(void * validator);
#pragma weak CBBlockChainStorageLoadBasicValidator
/**
 @brief Loads a block from storage.
 @param validator The CBValidator object.
 @param blockID The index of the block int he branch.
 @param branch The branch the block belongs to.
 @returns A new CBBlock object with serailised block data which has not been deserialised or NULL on failure.
 */
void * CBBlockChainStorageLoadBlock(void * validator, uint32_t blockID, uint32_t branch);
#pragma weak CBBlockChainStorageLoadBlock
/**
 @brief Loads a branch
 @param validator A CBValidator object. The storage object can be found within this.
 @param branchNum The number of the branch to load.
 @returns true on sucess or false on failure.
 */
bool CBBlockChainStorageLoadBranch(void * validator, uint8_t branchNum);
#pragma weak CBBlockChainStorageLoadBranch
/**
 @brief Loads a branch's work
 @param validator A CBValidator object. The storage object can be found within this.
 @param branchNum The number of the branch to load the work for.
 @returns true on sucess or false on failure.
 */
bool CBBlockChainStorageLoadBranchWork(void * validator, uint8_t branchNum);
#pragma weak CBBlockChainStorageLoadBranchWork
/**
 @brief Obtains the outputs for a transaction
 @param validator A CBValidator object. The storage object can be found within this.
 @param txHash The transaction hash to load the output data for.
 @param data A pointer to the buffer to set to the data.
 @param dataAllocSize If the size of the outputs is beyond this number, data will be reallocated to the size of the outputs and the number will be increased to the size of the outputs.
 @param position The position of the outputs in the block to be set.
 @returns true on sucess or false on failure.
 */
bool CBBlockChainStorageLoadOutputs(void * validator, uint8_t * txHash, uint8_t ** data, uint32_t * dataAllocSize, uint32_t * position);
#pragma weak CBBlockChainStorageLoadOutputs
/**
 @brief Obtains an unspent output.
 @param validator A CBValidator object. The storage object can be found within this.
 @param txHash The transaction hash of this output.
 @param outputIndex The index of the output in the transaction.
 @param coinbase Will be set to true if the output exists in a coinbase transaction or false otherwise.
 @param outputHeight The height of the block the output exists in.
 @returns The output as a CBTransactionOutput object on sucess or NULL on failure.
 */
void * CBBlockChainStorageLoadUnspentOutput(void * validator, uint8_t * txHash, uint32_t outputIndex, bool * coinbase, uint32_t * outputHeight);
#pragma weak CBBlockChainStorageLoadUnspentOutput
/**
 @brief Moves a block to a new location.
 @param validator A CBValidator object. The storage object can be found within this.
 @param branch The branch where the block exists.
 @param blockIndex The index of the block.
 @param newBranch The branch where the block will be moved into.
 @param newIndex The index the block will take in the new branch.
 @returns true on sucess or false on failure.
 */
bool CBBlockChainStorageMoveBlock(void * validator, uint8_t branch, uint32_t blockIndex, uint8_t newBranch, uint32_t newIndex);
#pragma weak CBBlockChainStorageMoveBlock
/**
 @brief Removes all of the non-staged changes in the underlying database storage object.
 @param iself The storage object.
 */
void CBBlockChainStorageRevert(CBDepObject iself);
#pragma weak CBBlockChainStorageRevert
/**
 @brief Saves the basic validator information, not any branches or orphans.
 @param validator A CBValidator object. The storage object can be found within this.
 @returns true on sucess or false on failure.
 */
bool CBBlockChainStorageSaveBasicValidator(void * validator);
#pragma weak CBBlockChainStorageSaveBasicValidator 
/**
 @brief Saves a block.
 @param validator A CBValidator object. The storage object can be found within this.
 @param block The block to save.
 @param branch The branch of this block
 @param blockIndex The index of this block.
 @returns true on sucess or false on failure.
 */
bool CBBlockChainStorageSaveBlock(void * validator, void * block, uint8_t branch, uint32_t blockIndex);
#pragma weak CBBlockChainStorageSaveBlock
/**
 @brief Saves a block, with it's header only.
 @param validator A CBValidator object. The storage object can be found within this.
 @param block The block to save.
 @param branch The branch of this block
 @param blockIndex The index of this block.
 @returns true on sucess or false on failure.
 */
bool CBBlockChainStorageSaveBlockHeader(void * validator, void * block, uint8_t branch, uint32_t blockIndex);
#pragma weak CBBlockChainStorageSaveBlockHeader
/**
 @brief Saves the branch information without commiting to disk.
 @param validator A CBValidator object. The storage object can be found within this.
 @param branch The branch number to save.
 @returns true on sucess or false on failure.
 */
bool CBBlockChainStorageSaveBranch(void * validator, uint8_t branch);
#pragma weak CBBlockChainStorageSaveBranch
/**
 @brief Saves the work for a branch without commiting to disk.
 @param validator A CBValidator object. The storage object can be found within this.
 @param branch The branch number with the work to save.
 @returns true on sucess or false on failure.
 */
bool CBBlockChainStorageSaveBranchWork(void * validator, uint8_t branch);
#pragma weak CBBlockChainStorageSaveBranchWork
/**
 @brief Saves a transaction reference. If the transaction exists in the block chain already, increment a counter. Else add a new reference.
 @param validator A CBValidator object. The storage object can be found within this.
 @param txHash The transaction's hash.
 @param branch The branch where the transaction exists.
 @param blockIndex The index of the block which contains the transaction.
 @param outputPos The position of the outputs in the block for this transaction.
 @param outputsLen The length of the outputs in this transaction.
 @param coinbase If true the transaction is a coinbase, else it is not.
 @param numOutputs The number of outputs for this transaction.
 @returns true if successful or false otherwise.
 */
bool CBBlockChainStorageSaveTransactionRef(void * validator, uint8_t * txHash, uint8_t branch, uint32_t blockIndex, uint32_t outputPos, uint32_t outputsLen, bool coinbase, uint32_t numOutputs);
#pragma weak CBBlockChainStorageSaveTransactionRef
/**
 @brief Saves an output as unspent.
 @param validator A CBValidator object. The storage object can be found within this.
 @param txHash The transaction hash of this output.
 @param outputIndex The index of the output in the transaction.
 @param position The position of the unspent output in the block.
 @param length The length of the unspent output in the block.
 @param increment If true, increment the number of unspent outputs for the transaction.
 @returns true if successful or false otherwise.
 */
bool CBBlockChainStorageSaveUnspentOutput(void * validator, uint8_t * txHash, uint32_t outputIndex, uint32_t position, uint32_t length, bool increment);
#pragma weak CBBlockChainStorageSaveUnspentOutput
/**
 @brief Determines if a transaction exists in the block chain.
 @param validator A CBValidator object. The storage object can be found within this.
 @param txHash The transaction's hash.
 @returns CB_TRUE if it exists, CB_FALSE if it doesn't or CB_ERROR on error.
 */
CBErrBool CBBlockChainStorageTransactionExists(void * validator, uint8_t * txHash);
#pragma weak CBBlockChainStorageTransactionExists
/**
 @brief Determines if an unspent output exists.
 @param validator A CBValidator object. The storage object can be found within this.
 @param txHash The transaction hash of this output.
 @param outputIndex The index of the output in the transaction.
 @returns CB_TRUE if it exists, CB_FALSE if it doesn't or CB_ERROR on error.
 */
CBErrBool CBBlockChainStorageUnspentOutputExists(void * validator, uint8_t * txHash, uint32_t outputIndex);
#pragma weak CBBlockChainStorageUnspentOutputExists

// ADDRESS STORAGE DEPENDENCES

/**
 @brief Creates a new address storage object.
 @param storage The object to set.
 @param database The database object to use.
 @returns true on success and false on failure.
 */
bool CBNewAddressStorage(CBDepObject * storage, CBDepObject database);
#pragma weak CBNewAddressStorage
/**
 @brief Frees the address storage object.
 @param self The address storage object.
 */
void CBFreeAddressStorage(CBDepObject self);
#pragma weak CBFreeAddressStorage
/**
 @brief Removes an address from storage.
 @param self The address storage object.
 @param address The address object.
 @returns true on success or false on failure.
 */
bool CBAddressStorageDeleteAddress(CBDepObject self, void * address);
#pragma weak CBAddressStorageDeleteAddress
/**
 @brief Obtains the number of addresses in storage.
 @param self The address storage object.
 @returns The number of addresses in storage
 */
uint64_t CBAddressStorageGetNumberOfAddresses(CBDepObject self);
#pragma weak CBAddressStorageGetNumberOfAddresses
/**
 @brief Loads all of the addresses from storage into an address manager.
 @param self The address storage object.
 @param addrMan A CBNetworkAddressManager object.
 @returns true on success or false on failure.
 */
bool CBAddressStorageLoadAddresses(CBDepObject self, void * addrMan);
#pragma weak CBAddressStorageLoadAddresses
/**
 @brief Saves an address to storage. If the number of addresses is at "maxAddresses" remove an address to make room.
 @param self The address storage object.
 @param address The CBNetworkAddress object.
 @returns true on success or false on failure.
 */
bool CBAddressStorageSaveAddress(CBDepObject self, void * address);
#pragma weak CBAddressStorageSaveAddress

// ACCOUNTER DEPENDENCIES

// Cosntants

#define CB_UNCONFIRMED UINT32_MAX

// Structures

typedef struct{
	uint8_t addrHash[20];
	int64_t amount;
} CBAccountTransactionDetails;

/**
 @brief Details for a transaction on a branch for an account.
 */
typedef struct{
	CBAccountTransactionDetails accountTxDetails;
	uint8_t txHash[32];
	uint64_t timestamp;
	uint32_t height;
	uint64_t cumBalance;
	int64_t cumUnconfBalance;
} CBTransactionDetails;

typedef struct CBTransactionAccountDetailList CBTransactionAccountDetailList;

struct CBTransactionAccountDetailList{
	CBAccountTransactionDetails accountTxDetails;
	uint64_t accountID;
	CBTransactionAccountDetailList * next;
};

/**
 @brief Details for an unspent output on a branch for an account.
 */
typedef struct{
	uint8_t txHash[32];
	uint32_t index;
	uint8_t watchedHash[20];
	uint64_t value;
} CBUnspentOutputDetails;

// Function implemented by the core

void CBFreeTransactionAccountDetailList(CBTransactionAccountDetailList * list);

// Functions

/**
 @brief Creates a new accounter storage object.
 @param storage The object to set.
 @param database The database storage object.
 @returns true on success and false on failure.
 */
bool CBNewAccounterStorage(CBDepObject * storage, CBDepObject database);
#pragma weak CBNewAccounterStorage
/**
 @brief Creates a new accounter storage transaction cursor object for iterating through transactions.
 @param cursor The object to set.
 @param accounter The accounter object.
 @param accountID The ID of the account to find the transactions for.
 @param timeMin The minimum time of the transactions.
 @param timeMax The maximum time of the transactions.
 @returns true on success and false on failure.
 */
bool CBNewAccounterStorageTransactionCursor(CBDepObject * cursor, CBDepObject accounter, uint64_t accountID, uint64_t timeMin, uint64_t timeMax);
#pragma weak CBNewAccounterStorageTransactionCursor
/**
 @brief Creates a new accounter storage unspent output cursor object for iterating through unspent outputs.
 @param cursor The object to set.
 @param accounter The accounter object.
 @param accountID The ID of the account to find the unspent outputs for.
 @returns true on success and false on failure.
 */
bool CBNewAccounterStorageUnspentOutputCursor(CBDepObject * cursor, CBDepObject accounter, uint64_t accountID);
#pragma weak CBNewAccounterStorageUnspentOutputCursor
/**
 @brief Frees the accounter storage object.
 @param self The accounter storage object.
 */
void CBFreeAccounterStorage(CBDepObject self);
#pragma weak CBFreeAccounterStorage
/**
 @brief Frees the accounter cursor object.
 @param self The accounter cursor object.
 */
void CBFreeAccounterCursor(CBDepObject self);
#pragma weak CBFreeAccounterCursor
/**
 @brief Adds a watched output hash to an account.
 @param self The accounter object. 
 @param hash The hash as created by CBTransactionOuputGetHash.
 @param accountID The ID of the account to add the hash for.
 @returns true on success and false on failure.
 */
bool CBAccounterAddWatchedOutputToAccount(CBDepObject self, uint8_t * hash, uint64_t accountID);
#pragma weak CBAccounterAddWatchedOutputToAccount
/**
 @brief Modifies the height information of a transaction.
 @param self The accounter object. 
 @param tx The transaction object.
 @returns true on success and false on failure.
 */
bool CBAccounterTransactionChangeHeight(CBDepObject self, void * tx, uint32_t oldHeight, uint32_t newHeight);
#pragma weak CBAccounterTransactionChangeHeight
/**
 @brief Processes a found transaction on a branch for all of the accounts
 @param self The accounter object.
 @param tx The found transaction.
 @param blockHeight The height the transaction was found. Make this zero if the branch is CB_NO_BRANCH
 @param time The timestamp for the transaction.
 @param details A linked list of transaction details for various accounts.
 @returns CB_TRUE if this transaction belongs to an account, CB_FALSE if it doesn't and CB_ERROR on failure.
 */
CBErrBool CBAccounterFoundTransaction(CBDepObject self, void * tx, uint32_t blockHeight, uint64_t time, CBTransactionAccountDetailList ** details);
#pragma weak CBAccounterFoundTransaction
/**
 @brief Gets the confirmed balance for an account.
 @param self The accounter object.
 @param branch The branch ID.
 @param accountID The account ID.
 @param balance The balance to set.
 @returns true on success, false on failure.
 */
bool CBAccounterGetAccountBalance(CBDepObject self, uint64_t accountID, uint64_t * balance, int64_t * unconfBalance);
#pragma weak CBAccounterGetAccountBalance
/**
 @brief Gets the next transaction by a cursor.
 @param cursor The cursor object.
 @param details A pointer to a CBTransactionDetails object to be set.
 @returns CB_TRUE if there was a next transaction, CB_FALSE if there wasn't or CB_ERROR on an error.
 */
CBErrBool CBAccounterGetNextTransaction(CBDepObject cursor, CBTransactionDetails * details);
#pragma weak CBAccounterGetNextTransaction
/**
 @brief Gets the next unspent output by a cursor.
 @param cursor The cursor object.
 @param details A pointer to a CBUnspentOutputDetails object to be set.
 @returns CB_TRUE if there was a next unspent output, CB_FALSE if there wasn't or CB_ERROR on an error.
 */
CBErrBool CBAccounterGetNextUnspentOutput(CBDepObject cursor, CBUnspentOutputDetails * details);
#pragma weak CBAccounterGetNextUnspentOutput
bool CBAccounterGetTransactionTime(CBDepObject self, uint8_t * txHash, uint64_t * time);
#pragma weak CBAccounterGetTransactionTime
CBErrBool CBAccounterIsOurs(CBDepObject self, uint8_t * txHash);
#pragma weak CBAccounterIsOurs
/**
 @brief Processes a transaction being lost for all of the accounts.
 @param self The accounter object.
 @param tx The transaction being lost.
 @returns true on success and false on failure.
 */
bool CBAccounterLostTransaction(CBDepObject self, void * tx, uint32_t height);
#pragma weak CBAccounterLostTransaction
/**
 @brief Merges a source account into destination account. The source account remains as it was before and the destination account gains all of the watched hashes, transactions and unspent outputs of the source account.
 @param self The accounter object.
 @param accountDest The destination account.
 @param accountSrc The source account.
 @returns true on success and false on failure.
 */
bool CBAccounterMergeAccountIntoAccount(CBDepObject self, uint64_t accountDest, uint64_t accountSrc);
#pragma weak CBAccounterMergeAccountIntoAccount
/**
 @brief Gets the object for a new account.
 @param self The accounter object.
 @returns the object of the account of 0 on error.
 */
uint64_t CBAccounterNewAccount(CBDepObject self);
#pragma weak CBAccounterNewAccount
CBErrBool CBAccounterTransactionExists(CBDepObject self, uint8_t * hash);
#pragma weak CBAccounterTransactionExists

// NODE STORAGE DEPENDENCIES

bool CBNewNodeStorage(CBDepObject * storage, CBDepObject database);
#pragma weak CBNewNodeStorage
void CBFreeNodeStorage(CBDepObject storage);
#pragma weak CBFreeNodeStorage
bool CBNodeStorageGetStartScanningTime(CBDepObject storage, uint64_t * startTime);
#pragma weak CBNodeStorageGetStartScanningTime
bool CBNodeStorageLoadUnconfTxs(void * node);
#pragma weak CBNodeStorageLoadUnconfTxs
bool CBNodeStorageRemoveOurTx(CBDepObject storage, void * tx);
#pragma weak CBNodeStorageRemoveOurTx
bool CBNodeStorageAddOurTx(CBDepObject storage, void * tx);
#pragma weak CBNodeStorageAddOurTx
bool CBNodeStorageRemoveOtherTx(CBDepObject storage, void * tx);
#pragma weak CBNodeStorageRemoveOtherTx
bool CBNodeStorageAddOtherTx(CBDepObject storage, void * tx);
#pragma weak CBNodeStorageAddOtherTx

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
uint8_t CBGetNumberOfCores(void);
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
uint64_t CBGetMilliseconds(void);
#pragma weak CBGetMilliseconds

#endif
