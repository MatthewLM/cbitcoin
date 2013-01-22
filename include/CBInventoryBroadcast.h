//
//  CBInventoryBroadcast.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 10/07/2012.
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
 @brief Used to advertise an inventory or ask for an inventory. An inventory is a list of objects  which can be blocks or transsactions. Inherits CBMessage
*/

#ifndef CBINVENTORYBROADCASTH
#define CBINVENTORYBROADCASTH

//  Includes

#include "CBInventoryItem.h"

// Constants

#define CB_GET_DATA_MAX_SIZE 50000
#define CB_INVENTORY_BROADCAST_MAX_SIZE 50000

/**
 @brief Structure for CBInventoryBroadcast objects. @see CBInventoryBroadcast.h
*/
typedef struct{
	CBMessage base; /**< CBMessage base structure */
	uint16_t itemNum; /**< Number of items in the inventory. */
	CBInventoryItem ** items; /**< The items. This should be a memory block of pointers to CBInventoryItems. It will be freed when the CBInventoryBroadcast is freed. When adding an item it should be retained. When removing an item it should be released. Leave or set as NULL if empty. */
} CBInventoryBroadcast;

/**
 @brief Creates a new CBInventoryBroadcast object.
 @returns A new CBInventoryBroadcast object.
*/
CBInventoryBroadcast * CBNewInventoryBroadcast(void);
/**
@brief Creates a new CBInventoryBroadcast object from serialised data.
 @param data Serialised CBInventoryBroadcast data.
 @returns A new CBInventoryBroadcast object.
*/
CBInventoryBroadcast * CBNewInventoryBroadcastFromData(CBByteArray * data);

/**
 @brief Gets a CBInventoryBroadcast from another object. Use this to avoid casts.
 @param self The object to obtain the CBInventoryBroadcast from.
 @returns The CBInventoryBroadcast object.
 */
CBInventoryBroadcast * CBGetInventoryBroadcast(void * self);

/**
 @brief Initialises a CBInventoryBroadcast object
 @param self The CBInventoryBroadcast object to initialise
 @returns true on success, false on failure.
*/
bool CBInitInventoryBroadcast(CBInventoryBroadcast * self);
/**
 @brief Initialises a CBInventoryBroadcast object from serialised data
 @param self The CBInventoryBroadcast object to initialise
 @param data The serialised data.
 @returns true on success, false on failure.
*/
bool CBInitInventoryBroadcastFromData(CBInventoryBroadcast * self, CBByteArray * data);

/**
 @brief Frees a CBInventoryBroadcast object.
 @param self The CBInventoryBroadcast object to free.
*/
void CBFreeInventoryBroadcast(void * self);
 
//  Functions

/**
 @brief Calculates the length needed to serialise the object.
 @param self The CBInventoryBroadcast object.
 @returns The length read on success, 0 on failure.
 */
uint32_t CBInventoryBroadcastCalculateLength(CBInventoryBroadcast * self);
/**
 @brief Deserialises a CBInventoryBroadcast so that it can be used as an object.
 @param self The CBInventoryBroadcast object
 @returns The length read on success, 0 on failure.
*/
uint32_t CBInventoryBroadcastDeserialise(CBInventoryBroadcast * self);
/**
 @brief Serialises a CBInventoryBroadcast to the byte data.
 @param self The CBInventoryBroadcast object
 @param force Serialises everything, replacing any previous serialisation of children objects.
 @returns The length written on success, 0 on failure.
*/
uint32_t CBInventoryBroadcastSerialise(CBInventoryBroadcast * self, bool force);

#endif
