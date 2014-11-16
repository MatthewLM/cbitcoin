//
//  CBInventory.h
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

#define CBGetInventory(x) ((CBInventory *)x)

/**
 @brief Structure for CBInventory objects. @see CBInventory.h
*/
typedef struct{
	CBMessage base; /**< CBMessage base structure */
	int itemNum; /**< Number of items in the inventory. */
	CBInventoryItem * itemFront; /**< The items. */
	CBInventoryItem * itemBack;
} CBInventory;

/**
 @brief Creates a new CBInventory object.
 @returns A new CBInventory object.
*/
CBInventory * CBNewInventory(void);

/**
@brief Creates a new CBInventory object from serialised data.
 @param data Serialised CBInventory data.
 @returns A new CBInventory object.
*/
CBInventory * CBNewInventoryFromData(CBByteArray * data);

/**
 @brief Initialises a CBInventory object
 @param self The CBInventory object to initialise
*/
void CBInitInventory(CBInventory * self);

/**
 @brief Initialises a CBInventory object from serialised data
 @param self The CBInventory object to initialise
 @param data The serialised data.
*/
void CBInitInventoryFromData(CBInventory * self, CBByteArray * data);

/**
 @brief Releases and frees all fo the objects stored by the CBInventory object.
 @param self The CBInventory object to destroy.
 */
void CBDestroyInventory(void * self);

/**
 @brief Frees a CBInventory object and also calls CBDestroyInventory.
 @param self The CBInventory object to free.
*/
void CBFreeInventory(void * self);
 
//  Functions

bool CBInventoryAddInventoryItem(CBInventory * self, CBInventoryItem * item);

/**
 @brief Calculates the length needed to serialise the object.
 @param self The CBInventory object.
 @returns The length read on success, 0 on failure.
 */
int CBInventoryCalculateLength(CBInventory * self);

/**
 @brief Deserialises a CBInventory so that it can be used as an object.
 @param self The CBInventory object
 @returns The length read on success, 0 on failure.
*/
int CBInventoryDeserialise(CBInventory * self);

void CBInventoryPrepareBytes(CBInventory * self);

CBInventoryItem * CBInventoryPopInventoryItem(CBInventory * self);

/**
 @brief Serialises a CBInventory to the byte data.
 @param self The CBInventory object
 @param force Serialises everything, replacing any previous serialisation of children objects.
 @returns The length written on success, 0 on failure.
*/
int CBInventorySerialise(CBInventory * self, bool force);
bool CBInventoryTakeInventoryItem(CBInventory * self, CBInventoryItem * item);

#endif
