//
//  CBInventoryBroadcast.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 10/07/2012.
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
 @brief Used to advertise an inventory or ask for an inventory. An inventory is a list of objects  which can be blocks or transsactions. Inherits CBMessage
*/

#ifndef CBINVENTORYBROADCASTH
#define CBINVENTORYBROADCASTH

//  Includes

#include "CBInventoryItem.h"

// Constants and Macros

#define CB_GET_DATA_MAX_SIZE 50000
#define CB_INVENTORY_BROADCAST_MAX_SIZE 50000
#define CBGetInventoryBroadcast(x) ((CBInventoryBroadcast *)x)

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
 @brief Initialises a CBInventoryBroadcast object
 @param self The CBInventoryBroadcast object to initialise
*/
void CBInitInventoryBroadcast(CBInventoryBroadcast * self);
/**
 @brief Initialises a CBInventoryBroadcast object from serialised data
 @param self The CBInventoryBroadcast object to initialise
 @param data The serialised data.
*/
void CBInitInventoryBroadcastFromData(CBInventoryBroadcast * self, CBByteArray * data);

/**
 @brief Releases and frees all fo the objects stored by the CBInventoryBroadcast object.
 @param self The CBInventoryBroadcast object to destroy.
 */
void CBDestroyInventoryBroadcast(void * self);
/**
 @brief Frees a CBInventoryBroadcast object and also calls CBDestroyInventoryBroadcast.
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
