//
//  CBAddressBroadcast.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 03/07/2012.
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
 @brief Message for broadcasting CBNetworkAddresses over the network. This is used to share socket information for peers that are accepting incomming connections. When making an object, get the addresses you want to add and then set the "addrNum" and "addresses" fields. When managing the addresses you want to share, it's best to keep one object and change the addresses when necessary by reorganising the pointers with the "addresses" field. Inherits CBMessage
*/

#ifndef CBADDRESSBROADCASTH
#define CBADDRESSBROADCASTH

//  Includes

#include "CBNetworkAddress.h"

// Constants

#define CB_ADDRESS_BROADCAST_MAX_SIZE 1000

/**
 @brief Structure for CBAddressBroadcast objects. @see CBAddressBroadcast.h
*/
typedef struct{
	CBMessage base; /**< CBMessage base structure */
	bool timeStamps; /**< If true, timestamps are included with the CBNetworkAddresses */
	uint8_t addrNum; /**< Number of addresses. Maximum is 30. */
	CBNetworkAddress ** addresses; /**< List of CBNetworkAddresses shared so that peers can find new peers. */
} CBAddressBroadcast;

/**
 @brief Creates a new CBAddressBroadcast object.
 @returns A new CBAddressBroadcast object.
 */
CBAddressBroadcast * CBNewAddressBroadcast(bool timeStamps);
/**
 @brief Creates a new CBAddressBroadcast object from serialised data.
 @param data Serialised CBAddressBroadcast data.
 @returns A new CBAddressBroadcast object.
*/
CBAddressBroadcast * CBNewAddressBroadcastFromData(CBByteArray * data, bool timeStamps);

/**
 @brief Gets a CBAddressBroadcast from another object. Use this to avoid casts.
 @param self The object to obtain the CBAddressBroadcast from.
 @returns The CBAddressBroadcast object.
 */
CBAddressBroadcast * CBGetAddressBroadcast(void * self);

/**
 @brief Initialises a CBAddressBroadcast object
 @param self The CBAddressBroadcast object to initialise
 @returns true on success, false on failure.
*/
bool CBInitAddressBroadcast(CBAddressBroadcast * self, bool timeStamps);
/**
 @brief Initialises a CBAddressBroadcast object from serialised data
 @param self The CBAddressBroadcast object to initialise
 @param data The serialised data.
 @returns true on success, false on failure.
*/
bool CBInitAddressBroadcastFromData(CBAddressBroadcast * self, bool timeStamps, CBByteArray * data);

/**
 @brief Frees a CBAddressBroadcast object.
 @param self The CBAddressBroadcast object to free.
 */
void CBFreeAddressBroadcast(void * self);
 
//  Functions

/**
 @brief Adds a CBNetworkAddress to the list for broadcasting.
 @param self The CBAddressBroadcast object
 @param address The CBNetworkAddress to add.
 @returns true if the network address was added successfully, false if there was an error in doing so.
 */
bool CBAddressBroadcastAddNetworkAddress(CBAddressBroadcast * self, CBNetworkAddress * address);
/**
 @brief Calculates the length needed to serialise the object.
 @param self The CBAddressBroadcast object.
 @returns The length read on success, 0 on failure.
 */
uint32_t CBAddressBroadcastCalculateLength(CBAddressBroadcast * self);
/**
 @brief Deserialises a CBAddressBroadcast so that it can be used as an object.
 @param self The CBAddressBroadcast object
 @returns The length read on success, 0 on failure.
*/
uint32_t CBAddressBroadcastDeserialise(CBAddressBroadcast * self);
/**
 @brief Serialises a CBAddressBroadcast to the byte data.
 @param self The CBAddressBroadcast object
 @param force Serialises everything, replacing any previous serialisation of children objects.
 @returns The length written on success, 0 on failure.
*/
uint32_t CBAddressBroadcastSerialise(CBAddressBroadcast * self, bool force);
/**
 @brief Takes a CBNetworkAddress to the list for broadcasting. This does not retain the CBNetworkAddress so you can pass an CBNetworkAddress into this while releasing control in the calling function.
 @param self The CBAddressBroadcast object
 @param address The CBNetworkAddress to take.
 @returns true if the network address was taken successfully, false if there was an error in doing so.
 */
bool CBAddressBroadcastTakeNetworkAddress(CBAddressBroadcast * self, CBNetworkAddress * address);

#endif
