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
 @brief Serialisable data structure. Provides the basic framework for serialising and deserialising bitcoin messages. Messages shouldn't be serialised including message headers. Message headers should be added before sending messages and removed when deserialising. Inherits CBObject
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
	CBMessageType type; /**< The type of the message */
	uint8_t * altText; /**< For an alternative message: This is the type text. */
	CBByteArray * bytes; /**< Raw message data minus the message header. When serialising this should be assigned to a CBByteArray large enough to hold the serialised data. */
	uint8_t checksum[4]; /**< The message checksum. When sending messages using a CBNetworkCommunicator, this is calculated for you. */
	void (*logError)(char *,...); /**< Pointer to error callback */
	CBMessageType expectResponse; /**< Set to zero if no message expected or the type of message expected as a response. */
	bool serialised; /**< True if this object has been serialised. If an object as already been serialised it is not serialised by parent objects. For instance when serialising a block, the transactions are not serialised if they have been already. However objects can be explicitly reserialised */
} CBMessage;

/**
 @brief Creates a new CBMessage object. This message will be created with object data and not with byte data. The message can be serialised for the byte data used over the network.
 @param 
 @returns A new CBMessage object.
 */
CBMessage * CBNewMessageByObject(void (*logError)(char *,...));

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
bool CBInitMessageByObject(CBMessage * self,void (*logError)(char *,...));
/**
 @brief Initialises a CBMessage object from byte data.
 @param self The CBMessage object to initialise
 @param data The byte data for the object. The data will not be copied but retained by this object. 
 @returns true on success, false on failure.
 */
bool CBInitMessageByData(CBMessage * self,CBByteArray * data,void (*logError)(char *,...));

/**
 @brief Frees a CBMessage object.
 @param self The CBMessage object to free.
 */
void CBFreeMessage(void * self);

//  Functions

#endif
