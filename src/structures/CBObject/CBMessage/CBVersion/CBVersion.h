//
//  CBVersion.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 01/07/2012.
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
 @brief Contains infromation relating to the version of a peer, the timestamp, features, IPs, user agent and last block recieveed. Inherits CBMessage
*/

#ifndef CBVERSIONH
#define CBVERSIONH

//  Includes

#include "CBMessage.h"
#include "CBNetworkAddress.h"

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
	uint64_t nounce; /**< Nounce used to detect self. */
	CBByteArray * userAgent; /**< Used to identify bitcoin software. Should be a string no more than 400 characters in length. */
	int32_t blockHeight; /**< The latest block height for the peer. Should probably be unsigned but the protocol specifies a signed integer. */
} CBVersion;

/**
 @brief Creates a new CBVersion object.
 @returns A new CBVersion object.
 */
CBVersion * CBNewVersion(int32_t version,CBVersionServices services,int64_t time,CBNetworkAddress * addRecv,CBNetworkAddress * addSource,uint64_t nounce,CBByteArray * userAgent,int32_t blockHeight,void (*onErrorReceived)(CBError error,char *,...));
/**
 @brief Creates a new CBVersion object from serialised data.
 @param data A CBByteArray holding the serialised data.
 @returns A new CBVersion object.
 */
CBVersion * CBNewVersionFromData(CBByteArray * data,void (*onErrorReceived)(CBError error,char *,...));

/**
 @brief Gets a CBVersion from another object. Use this to avoid casts.
 @param self The object to obtain the CBVersion from.
 @returns The CBVersion object.
 */
CBVersion * CBGetVersion(void * self);

/**
 @brief Initialises a CBVersion object
 @param self The CBVersion object to initialise
 @returns true on success, false on failure.
 */
bool CBInitVersion(CBVersion * self,int32_t version,CBVersionServices services,int64_t time,CBNetworkAddress * addRecv,CBNetworkAddress * addSource,uint64_t nounce,CBByteArray * userAgent,int32_t blockHeight,void (*onErrorReceived)(CBError error,char *,...));
/**
 @brief Initialises a new CBVersion object from serialised data.
 @param self The CBVersion object to initialise
 @param data A CBByteArray holding the serialised data.
 @returns true on success, false on failure.
 */
bool CBInitVersionFromData(CBVersion * self,CBByteArray * data,void (*onErrorReceived)(CBError error,char *,...));

/**
 @brief Frees a CBVersion object.
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
/**
 @brief Serialises a CBVersion to the byte data.
 @param self The CBVersion object
 @returns The length written on success, 0 on failure.
 */
uint32_t CBVersionSerialise(CBVersion * self);

#endif
