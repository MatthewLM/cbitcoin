//
//  CBInventoryItem.h
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
 @brief This is an inventory item that contains an object hash and the object type. It can refer to a block, a transaction or an error. Inherits CBMessage
*/

#ifndef CBINVENTORYITEMH
#define CBINVENTORYITEMH

//  Includes

#include "CBMessage.h"

// Constants

typedef enum{
	CB_INVENTORY_ITEM_ERROR = 0,
	CB_INVENTORY_ITEM_TRANSACTION = 1,
	CB_INVENTORY_ITEM_BLOCK = 2,
}CBInventoryItemType;

/**
 @brief Structure for CBInventoryItem objects. @see CBInventoryItem.h
*/
typedef struct{
	CBMessage base; /**< CBMessage base structure */
	CBInventoryItemType type; /**< The type of the inventory item. It can be an error, a transaction or a block */
	CBByteArray * hash; /**< The hash of the inventory item */
} CBInventoryItem;

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
 @brief Gets a CBInventoryItem from another object. Use this to avoid casts.
 @param self The object to obtain the CBInventoryItem from.
 @returns The CBInventoryItem object.
*/
CBInventoryItem * CBGetInventoryItem(void * self);

/**
 @brief Initialises a CBInventoryItem object
 @param self The CBInventoryItem object to initialise
 @returns true on success, false on failure.
*/
bool CBInitInventoryItem(CBInventoryItem * self, CBInventoryItemType type, CBByteArray * hash);
/**
 @brief Initialises a CBInventoryItem object from serialised data
 @param self The CBInventoryItem object to initialise
 @param data The serialised data.
 @returns true on success, false on failure.
*/
bool CBInitInventoryItemFromData(CBInventoryItem * self, CBByteArray * data);

/**
 @brief Frees a CBInventoryItem object.
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
