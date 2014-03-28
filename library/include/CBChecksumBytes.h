//
//  CBChecksumBytes.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 03/05/2012.
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
 @brief Represents a key, begining with a version byte and ending with a checksum. Inherits CBByteArray
*/

#ifndef CBChecksumBytesH
#define CBChecksumBytesH

//  Includes

#include "CBByteArray.h"
#include "CBBase58.h"

// Getter

#define CBGetChecksumBytes(x) ((CBChecksumBytes *)x)

/**
 @brief Structure for CBChecksumBytes objects. @see CBChecksumBytes.h
*/
typedef struct{
	CBByteArray base; /**< CBByteArray base structure */
	bool cacheString; /**< If true, cache string */
	CBByteArray * cachedString; /**< Pointer to cached CBByteArray string */
} CBChecksumBytes;

/**
 @brief Creates a new CBChecksumBytes object from a base-58 encoded string. The base-58 string will be validated by it's checksum. This returns NULL if the string is invalid. The CB_ERROR_BASE58_DECODE_CHECK_TOO_SHORT error is given if the decoded data is less than 4 bytes. CB_ERROR_BASE58_DECODE_CHECK_INVALID is given if the checksum does not match.
 @param string A base-58 encoded CBString to make a CBChecksumBytes object with a termination character.
 @param cacheString If true, the bitcoin string for this object will be cached in memory.
 @returns A new CBChecksumBytes object or NULL on failure.
 */
CBChecksumBytes * CBNewChecksumBytesFromString(CBByteArray * string, bool cacheString);

/**
 @brief Creates a new CBChecksumBytes object from bytes.
 @param bytes The bytes for the CBChecksumBytes object.
 @param size The size of the byte data.
 @param cacheString If true, the bitcoin string for this object will be cached in memory.
 @returns A new CBChecksumBytes object.
 */
CBChecksumBytes * CBNewChecksumBytesFromBytes(uint8_t * bytes, uint32_t size, bool cacheString);

/**
 @brief Creates a new CBChecksumBytes object from a hex string.
 @param hex
 @param cacheString If true, the bitcoin string for this object will be cached in memory.
 @returns A new CBChecksumBytes object.
 */
CBChecksumBytes * CBNewChecksumBytesFromHex(char * hex, bool cacheString);

/**
 @brief Initialises a CBChecksumBytes object from a string.
 @param self The CBChecksumBytes object to initialise.
 @param string A CBString to make a CBChecksumBytes object with a termination character.
 @param cacheString If true, the bitcoin string for this object will be cached in memory.
 @returns true on success, false on failure.
 */
bool CBInitChecksumBytesFromString(CBChecksumBytes * self, CBByteArray * string, bool cacheString);

/**
 @brief Initialises a new CBChecksumBytes object from bytes.
 @param self The CBChecksumBytes object to initialise.
 @param bytes The bytes for the CBChecksumBytes object.
 @param size The size of the byte data.
 @param cacheString If true, the bitcoin string for this object will be cached in memory.
 */
void CBInitChecksumBytesFromBytes(CBChecksumBytes * self, uint8_t * bytes, uint32_t size, bool cacheString);

/**
 @brief Initialises a new CBChecksumBytes object from a hex string.
 @param self The CBChecksumBytes object to initialise.
 @param hex
 @param cacheString If true, the bitcoin string for this object will be cached in memory.
 */
void CBInitChecksumBytesFromHex(CBChecksumBytes * self, char * hex, bool cacheString);

/**
 @brief Release and free all of the objects stored by the CBChecksumBytes object.
 @param self The CBChecksumBytes object to destroy.
 */
void CBDestroyChecksumBytes(void * self);

/**
 @brief Frees a CBChecksumBytes object and also calls CBdestroyChecksumBytes.
 @param self The CBChecksumBytes object to free.
 */
void CBFreeChecksumBytes(void * self);
 
//  Functions

/**
 @brief Gets the prefix for a CBChecksumBytes object.
 @param self The CBChecksumBytes object.
 @returns The prefix code. @see CBBase58Prefix
 */
CBBase58Prefix CBChecksumBytesGetPrefix(CBChecksumBytes * self);

/**
 @brief Gets the string representation for a CBChecksumBytes object as a base-58 encoded CBString.
 @param self The CBChecksumBytes object.
 @returns The object represented as a base-58 encoded CBString. Do not modify this. Copy if modification is required.
 */
CBByteArray * CBChecksumBytesGetString(CBChecksumBytes * self);

#endif
