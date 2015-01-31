//
//  CBVersion.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 01/07/2012.
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
 @brief Contains infromation relating to the version of a peer, the timestamp, features, IPs, user agent and last block recieveed. Inherits CBMessage
*/

#ifndef CBVERSIONH
#define CBVERSIONH

//  Includes

#include "CBMessage.h"
#include "CBNetworkAddress.h"
#include <inttypes.h>
#include <assert.h>

// Constants and Macros

#define CB_MIN_PROTO_VERSION 209
#define CB_ADDR_TIME_VERSION 31402 // Version where times are included in addr messages.
#define CB_PONG_VERSION 60002 // Version where pings are responded with pongs.
#define CBGetVersion(x) ((CBVersion *)x)

/**
 @brief Structure for CBVersion objects. @see CBVersion.h
*/
typedef struct{
	CBMessage base; /**< CBObject base structure */
	int32_t version; /**< The protocol version. There appears to be no good reason why this is signed. */
	CBVersionServices services; /**< The services which a peer is offering. */
	int64_t time; /**< The timestamp of this peer. This assumes time(NULL) returns a correct 64 bit timestamp which it should to avoid massive problems in the future. */
	CBNetworkAddress * addRecv; /**< Socket information for the recieving peer. */
	CBNetworkAddress * addSource; /**< The socket information for the source address. */
	uint64_t nonce; /**< Nounce used to detect self. */
	CBByteArray * userAgent; /**< Used to identify bitcoin software. Should be a string no more than 400 characters in length. */
	int32_t blockHeight; /**< The latest block height for the peer. Should probably be unsigned but the protocol specifies a signed integer. */
} CBVersion;

/**
 @brief Creates a new CBVersion object.
 @returns A new CBVersion object.
 */
CBVersion * CBNewVersion(int32_t version, CBVersionServices services, int64_t time, CBNetworkAddress * addRecv, CBNetworkAddress * addSource, uint64_t nonce, CBByteArray * userAgent, int32_t blockHeight);

/**
 @brief Creates a new CBVersion object from serialised data.
 @param data A CBByteArray holding the serialised data.
 @returns A new CBVersion object.
 */
CBVersion * CBNewVersionFromData(CBByteArray * data);

/**
 @brief Initialises a CBVersion object
 @param self The CBVersion object to initialise
 */
void CBInitVersion(CBVersion * self, int32_t version, CBVersionServices services, int64_t time, CBNetworkAddress * addRecv, CBNetworkAddress * addSource, uint64_t nonce, CBByteArray * userAgent, int32_t blockHeight);

/**
 @brief Initialises a new CBVersion object from serialised data.
 @param self The CBVersion object to initialise
 @param data A CBByteArray holding the serialised data.
 */
void CBInitVersionFromData(CBVersion * self, CBByteArray * data);

/**
 @brief Release and free the objects stored by the CBVersion object.
 @param self The CBVersion object to destroy.
 */
void CBDestroyVersion(void * self);

/**
 @brief Frees a CBVersion object and also calls CBDestroyVersion.
 @param self The CBVersion object to free.
 */
void CBFreeVersion(void * self);
 
//  Functions

/**
 @brief Deserialises a CBVersion so that it can be used as an object.
 @param self The CBVersion object
 @returns The length read on success, 0 on failure.
 */
uint32_t CBVersionDeserialise(CBVersion * self);

/**
 @brief Calculates the length needed to serialise the object
 @param self The CBVersion object
 @returns The length read on success, 0 on failure.
 */
uint32_t CBVersionCalculateLength(CBVersion * self);

void CBVersionPrepareBytes(CBVersion * self);

/**
 @brief Serialises a CBVersion to the byte data.
 @param self The CBVersion object
 @param force Serialises everything, replacing any previous serialisation of children objects.
 @returns The length written on success, 0 on failure.
 */
uint32_t CBVersionSerialise(CBVersion * self, bool force);
uint16_t CBVersionStringMaxSize(CBVersion * self);
void CBVersionToString(CBVersion * self, char output[]);

#endif
