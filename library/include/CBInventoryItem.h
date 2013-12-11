//
//  CBInventoryItem.h
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
 @brief This is an inventory item that contains an object hash and the object type. It can refer to a block, a transaction or an error. Inherits CBMessage
*/

#ifndef CBINVENTORYITEMH
#define CBINVENTORYITEMH

//  Includes

#include "CBMessage.h"

// Constants and Macros

#define CBGetInventoryItem(x) ((CBInventoryItem *)x)

typedef enum{
	CB_INVENTORY_ITEM_ERROR = 0,
	CB_INVENTORY_ITEM_TX = 1,
	CB_INVENTORY_ITEM_BLOCK = 2,
}CBInventoryItemType;

/**
 @brief Structure for CBInventoryItem objects. @see CBInventoryItem.h
*/
typedef struct CBInventoryItem CBInventoryItem;

struct CBInventoryItem{
	CBMessage base; /**< CBMessage base structure */
	CBInventoryItemType type; /**< The type of the inventory item. It can be an error, a transaction or a block */
	CBByteArray * hash; /**< The hash of the inventory item */
	CBInventoryItem * next; /**< The next inventory item in an inventory */
};

/**
 @brief Creates a new CBInventoryItem object.
 @returns A new CBInventoryItem object.
*/
CBInventoryItem * CBNewInventoryItem(CBInventoryItemType type, CBByteArray * hash);
/**
@brief Creates a new CBInventoryItem object from serialised data.
 @param data Serialised CBInventoryItem data.
 @returns A new CBInventoryItem object.
*/
CBInventoryItem * CBNewInventoryItemFromData(CBByteArray * data);

/**
 @brief Initialises a CBInventoryItem object
 @param self The CBInventoryItem object to initialise
 @returns true on success, false on failure.
*/
void CBInitInventoryItem(CBInventoryItem * self, CBInventoryItemType type, CBByteArray * hash);
/**
 @brief Initialises a CBInventoryItem object from serialised data
 @param self The CBInventoryItem object to initialise
 @param data The serialised data.
 @returns true on success, false on failure.
*/
void CBInitInventoryItemFromData(CBInventoryItem * self, CBByteArray * data);

/**
 @brief Releases and frees all of the objects stored by the CBInventoryItem object.
 @param self The CBInventoryItem object to free.
 */
void CBDestroyInventoryItem(void * self);
/**
 @brief Frees a CBInventoryItem object and also calls CBDestroyInventoryItem.
 @param self The CBInventoryItem object to free.
 */
void CBFreeInventoryItem(void * self);
 
//  Functions

/**
 @brief Deserialises a CBInventoryItem so that it can be used as an object.
 @param self The CBInventoryItem object
 @returns The length read on success, 0 on failure.
*/
uint32_t CBInventoryItemDeserialise(CBInventoryItem * self);
/**
 @brief Serialises a CBInventoryItem to the byte data.
 @param self The CBInventoryItem object
 @returns The length written on success, 0 on failure.
*/
uint32_t CBInventoryItemSerialise(CBInventoryItem * self);

#endif
