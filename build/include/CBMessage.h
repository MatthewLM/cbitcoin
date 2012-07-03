//
//  CBMessage.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 27/04/2012.
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
 @brief Serialisable data structure. Provides the basic framework for serialising and deserialising bitcoin objects. A CBMessage stores a byte array for an object, and this is always stored unlike in bitcoinj beacuse objects reference this data and there is no point in removing the references in the CBMessage objects. Copying the data and then removing the byte data instead may save a few bytes but at the expense of copy and reserialisation operations. Therefore when deserialising objects, references should be used whenever possible except when endian conversions are necessary. Inherits CBObject
 */

#ifndef CBMESSAGEH
#define CBMESSAGEH

//  Includes

#include <stdlib.h>
#include <stdbool.h>
#include "CBVarInt.h"

/**
 @brief Structure for CBMessage objects. @see CBMessage.h
 */
typedef struct CBMessage{
	CBObject base; /**< CBObject base structure */
	CBByteArray * bytes; /**< Raw message data. When serialising this should be assigned to a CBByteArray large enough to hold the serialised data. */
	CBByteArray * checksum;
	void * params; /**< Storage for parameters. */
	CBEvents * events; /**< Pointer to bitcoin event centre for errors */
} CBMessage;

/**
 @brief Creates a new CBMessage object. This message will be created with object data and not with byte data. The message can be serialised for the byte data used over the network.
 @returns A new CBMessage object.
 */
CBMessage * CBNewMessageByObject(void * params,CBEvents * events);

/**
 @brief Gets a CBMessage from another object. Use this to avoid casts.
 @param self The object to obtain the CBMessage from.
 @returns The CBMessage object.
 */
CBMessage * CBGetMessage(void * self);

/**
 @brief Initialises a CBMessage object
 @param self The CBMessage object to initialise
 @returns true on success, false on failure.
 */
bool CBInitMessageByObject(CBMessage * self,void * params,CBEvents * events);
/**
 @brief Initialises a CBMessage object from byte data.
 @param self The CBMessage object to initialise
 @param data The byte data for the object. The data will not be copied but retained by this object. 
 @returns true on success, false on failure.
 */
bool CBInitMessageByData(CBMessage * self,void * params,CBByteArray * data,CBEvents * events);

/**
 @brief Frees a CBMessage object.
 @param self The CBMessage object to free.
 */
void CBFreeMessage(void * self);

/**
 @brief Does the processing to free a CBMessage object. Should be called by the children when freeing objects.
 @param self The pointer to the CBMessage object to free.
 */
void CBFreeProcessMessage(CBMessage * self);

//  Functions

#endif
