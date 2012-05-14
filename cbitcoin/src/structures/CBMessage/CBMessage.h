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
 @brief Serialisable data structure. Inherits CBObject
 */

#ifndef CBMESSAGEH
#define CBMESSAGEH

//  Includes

#include <stdlib.h>
#include <stdbool.h>
#include "CBSha256Hash.h"
#include "CBVarInt.h"

/**
 @brief Virtual function table for CBMessage.
 */
typedef struct{
	CBObjectVT base; /**< CBObjectVT base structure */
	void (*maybeParse)(void *); /**< Pointer to the function used to parse a CBMessage if it has not been parsed already */
	void (*parse)(void *); /**< Pointer to the function used to parse a CBMessage */
	void (*parseLite)(void *); /**< Pointer to the function used to do a partial parse a CBMessage */
	CBByteArray * (*processSerialisation)(void *); /**< Pointer to the function used to serialise message data as a CBByteArray and return this array. */
	CBByteArray * (*readBytes)(void *,int); /**< Pointer to the function used to read a length of bytes. */
	u_int64_t (*readVarInt)(void *); /**< Pointer to the function used to read a variable size integer. */
	void (*unCache)(void *); /**< Pointer to the function used to remove cached byte array. */
	CBByteArray * (*unsafeSerialise)(void *); /**< Pointer to the function used to receive the serialised CBMessage byte array data without ensuring safe mutability */
}CBMessageVT;

/**
 @brief Structure for CBMessage objects. @see CBMessage.h
 */
typedef struct CBMessage{
	CBObject base; /**< CBObject base structure */
	int cursor; /**< Offset for parsing */
	int length; /**< The length of the message. Stored in the CBMessage structure since CBByteArray may not be cached. */
	CBByteArray * bytes; /**< Raw message data */
	bool parsed;
	bool recached;
	bool parseLazy; /**< Delay parsing until read is requested if true. */
	bool parseRetain; /**< If true, the byte array will be retained for future serialisation. */
	int protocolVersion; /**< Version of the bitcoin protocol. */
	CBByteArray * checksum;
	void * params; /**< Storage for parameters. */
	CBEvents * events; /**< Pointer to bitcoin event centre for errors */
} CBMessage;

/**
 @brief Creates a new CBMessage object.
 @returns A new CBMessage object.
 */
CBMessage * CBNewMessage(void * params,CBByteArray * bytes,int length,int protocolVersion,bool parseLazy,bool parseRetain,CBEvents * events);

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
bool CBInitMessage(CBMessage * self,void * params,CBByteArray * bytes,int length,int protocolVersion,bool parseLazy,bool parseRetain,CBEvents * events);

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
 @brief Receives the CBByteArray for a message, serialising if no cache is found and caching if parseRetain is true. Copies array if required.
 @param self The CBMessage object
 @returns The CBByteArray for this message. Safe for modification.
 */
CBByteArray * CBMessageBitcoinSerialise(CBMessage * self);
/**
 @brief Returns the message length, doing a parse if necessary. Children should override to give message size with lazy parsing.
 @param self The CBMessage object
 @returns The message length.
 */
int CBMessageGetLength(CBMessage * self);
void CBMessageMaybeParse(CBMessage * self);
void CBMessageParse(CBMessage * self);
void CBMessageParseLite(CBMessage * self);
/**
 @brief This should be overriden with a function that returns the serialised message data. If this is not overriden then an error will be issued.
 @param self The CBMessage object
 @returns An empty CBByteArray.
 */
CBByteArray * CBMessageProcessBitcoinSerialise(CBMessage * self);
/**
 @brief Reads a byte array from a CBMessage.
 @param self The CBMessage object.
 @returns A new CBByteArray.
 */
CBByteArray * CBMessageReadByteArray(CBMessage * self);
/**
 @brief Reads bytes from the CBMessage
 @param self The CBMessage object.
 @param length Number of bytes to read.
 @returns A new CBByteArray.
 */
CBByteArray * CBMessageReadBytes(CBMessage * self,int length);
/**
 @brief Reads a SHA-256 hash from the message
 @param self The CBMessage object.
 @returns The SHA-256 hash
 */
CBSha256Hash * CBMessageReadHash(CBMessage * self);
/**
 @brief Reads an unsigned 32 bit integer from the CBMessage and moves along the cursor.
 @param self The CBMessage object
 @returns An unsigned 32 bit integer.
 */
u_int32_t CBMessageReadUInt32(CBMessage * self);
/**
 @brief Reads an unsigned 64 bit integer from the CBMessage and moves along the cursor.
 @param self The CBMessage object
 @returns An unsigned 64 bit integer.
 */
u_int64_t CBMessageReadUInt64(CBMessage * self);
/**
 @brief Reads an variable interger from the CBMessage.
 @param self The CBMessage object
 @returns An unsigned 64 bit integer.
 */
u_int64_t CBMessageReadVarInt(CBMessage * self);
/**
 @brief Set a message checksum, giving the error CB_ERROR_MESSAGE_CHECKSUM_BAD_SIZE if the size is not 4 bytes.
 @param self The CBMessage object
 @param checksum The checksum
 @returns true on success, false on failure.
 */
bool CBMessageSetChecksum(CBMessage * self,CBByteArray * checksum);
/**
 @brief Removes message cache for changing data.
 @param self The CBMessage object
 */
void CBMessageUnCache(CBMessage * self);
/**
 @brief Receives the CBByteArray for a message, serialising if no cache is found and caching if parseRetain is true. Only for when no modification is required.
 @param self The CBMessage object
 @returns The CBByteArray for this message. Unsafe for modification.
 */
CBByteArray * CBMessageUnsafeBitcoinSerialise(CBMessage * self);

#endif