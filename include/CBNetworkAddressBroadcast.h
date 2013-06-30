//
//  CBNetworkAddressBroadcast.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 03/07/2012.
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
 @brief Message for broadcasting CBNetworkAddresses over the network. This is used to share socket information for peers that are accepting incomming connections. When making an object, get the addresses you want to add and then set the "addrNum" and "addresses" fields. When managing the addresses you want to share, it's best to keep one object and change the addresses when necessary by reorganising the pointers with the "addresses" field. Inherits CBMessage
*/

#ifndef CBNetworkAddressBroadcastH
#define CBNetworkAddressBroadcastH

//  Includes

#include "CBNetworkAddress.h"

// Constants

#define CB_NETWORK_ADDRESS_BROADCAST_MAX_SIZE 1000

/**
 @brief Structure for CBNetworkAddressBroadcast objects. @see CBNetworkAddressBroadcast.h
*/
typedef struct{
	CBMessage base; /**< CBMessage base structure */
	bool timeStamps; /**< If true, timestamps are included with the CBNetworkAddresses */
	uint8_t addrNum; /**< Number of addresses. Maximum is 30. */
	CBNetworkAddress ** addresses; /**< List of CBNetworkAddresses shared so that peers can find new peers. */
} CBNetworkAddressBroadcast;

/**
 @brief Creates a new CBNetworkAddressBroadcast object.
 @returns A new CBNetworkAddressBroadcast object.
 */
CBNetworkAddressBroadcast * CBNewNetworkAddressBroadcast(bool timeStamps);
/**
 @brief Creates a new CBNetworkAddressBroadcast object from serialised data.
 @param data Serialised CBNetworkAddressBroadcast data.
 @returns A new CBNetworkAddressBroadcast object.
*/
CBNetworkAddressBroadcast * CBNewNetworkAddressBroadcastFromData(CBByteArray * data, bool timeStamps);

/**
 @brief Gets a CBNetworkAddressBroadcast from another object. Use this to avoid casts.
 @param self The object to obtain the CBNetworkAddressBroadcast from.
 @returns The CBNetworkAddressBroadcast object.
 */
CBNetworkAddressBroadcast * CBGetNetworkAddressBroadcast(void * self);

/**
 @brief Initialises a CBNetworkAddressBroadcast object
 @param self The CBNetworkAddressBroadcast object to initialise
 @returns true on success, false on failure.
*/
void CBInitNetworkAddressBroadcast(CBNetworkAddressBroadcast * self, bool timeStamps);
/**
 @brief Initialises a CBNetworkAddressBroadcast object from serialised data
 @param self The CBNetworkAddressBroadcast object to initialise
 @param data The serialised data.
 @returns true on success, false on failure.
*/
void CBInitNetworkAddressBroadcastFromData(CBNetworkAddressBroadcast * self, bool timeStamps, CBByteArray * data);

/**
 @brief Release and free all of the objects stored by the CBNetworkAddressBroadcast object.
 @param self The CBNetworkAddressBroadcast object to destroy.
 */
void CBDestroyNetworkAddressBroadcast(void * self);
/**
 @brief Frees a CBNetworkAddressBroadcast object and also calls CBDestroyNetworkAddressBroadcast.
 @param self The CBNetworkAddressBroadcast object to free.
 */
void CBFreeNetworkAddressBroadcast(void * self);
 
//  Functions

/**
 @brief Adds a CBNetworkAddress to the list for broadcasting.
 @param self The CBNetworkAddressBroadcast object
 @param address The CBNetworkAddress to add.
 */
void CBNetworkAddressBroadcastAddNetworkAddress(CBNetworkAddressBroadcast * self, CBNetworkAddress * address);
/**
 @brief Calculates the length needed to serialise the object.
 @param self The CBNetworkAddressBroadcast object.
 @returns The length read on success, 0 on failure.
 */
uint32_t CBNetworkAddressBroadcastCalculateLength(CBNetworkAddressBroadcast * self);
/**
 @brief Deserialises a CBNetworkAddressBroadcast so that it can be used as an object.
 @param self The CBNetworkAddressBroadcast object
 @returns The length read on success, 0 on failure.
*/
uint32_t CBNetworkAddressBroadcastDeserialise(CBNetworkAddressBroadcast * self);
/**
 @brief Serialises a CBNetworkAddressBroadcast to the byte data.
 @param self The CBNetworkAddressBroadcast object
 @param force Serialises everything, replacing any previous serialisation of children objects.
 @returns The length written on success, 0 on failure.
*/
uint32_t CBNetworkAddressBroadcastSerialise(CBNetworkAddressBroadcast * self, bool force);
/**
 @brief Takes a CBNetworkAddress to the list for broadcasting. This does not retain the CBNetworkAddress so you can pass an CBNetworkAddress into this while releasing control in the calling function.
 @param self The CBNetworkAddressBroadcast object
 @param address The CBNetworkAddress to take.
 */
void CBNetworkAddressBroadcastTakeNetworkAddress(CBNetworkAddressBroadcast * self, CBNetworkAddress * address);

#endif
