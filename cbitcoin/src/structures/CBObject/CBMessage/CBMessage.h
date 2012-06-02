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
 @brief Virtual function table for CBMessage.
 */
typedef struct{
	CBObjectVT base; /**< CBObjectVT base structure */
	u_int32_t (*deserialise)(void *); /**< Pointer to the function used to deserialise message data into the object. Returns the length of the byte data read on success and 0 on failure. Deserialisation should reference the byte data wherever possible except where integer endian conversions are needed which is done by the CBByteArray read functions. */
	u_int32_t (*serialise)(void *); /**< Pointer to the function used to serialise message data as a CBByteArray. Serialisation is not done if it has been done already. self->bytes should be assigned to a suitable CBByteArray at some point before serialisation, a retain must be done for this object. This function should return the length of the written data on sucessful serialisation and false otherwise. */
}CBMessageVT;

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
 @brief Creates a new CBMessageVT.
 @returns A new CBMessageVT.
 */
CBMessageVT * CBCreateMessageVT(void);

/**
 @brief Sets the CBMessageVT function pointers.
 @param VT The CBMessageVT to set.
 */
void CBSetMessageVT(CBMessageVT * VT);

/**
 @brief Gets the CBMessageVT. Use this to avoid casts.
 @param self The object to obtain the CBMessageVT from.
 @returns The CBMessageVT.
 */
CBMessageVT * CBGetMessageVT(void * self);

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
void CBFreeMessage(CBMessage * self);

/**
 @brief Does the processing to free a CBMessage object. Should be called by the children when freeing objects.
 @param self The pointer to the CBMessage object to free.
 */
void CBFreeProcessMessage(CBMessage * self);

//  Functions

/**
 @brief This should be overriden with a function that deserialises the byte data into the object. If this is not overriden then an error will be issued.
 @param self The CBMessage object
 @returns 0
 */
u_int32_t CBMessageBitcoinDeserialise(CBMessage * self);
/**
 @brief This should be overriden with a function that fills "bytes" with the serialised data. If this is not overriden then an error will be issued.
 @param self The CBMessage object
 @param bytes The bytes that are filled for overriding functions.
 @returns 0
 */
u_int32_t CBMessageBitcoinSerialise(CBMessage * self);
/**
 @brief Set a message checksum, giving the error CB_ERROR_MESSAGE_CHECKSUM_BAD_SIZE if the size is not 4 bytes.
 @param self The CBMessage object
 @param checksum The checksum
 @returns true on success, false on failure.
 */
bool CBMessageSetChecksum(CBMessage * self,CBByteArray * checksum);

#endif