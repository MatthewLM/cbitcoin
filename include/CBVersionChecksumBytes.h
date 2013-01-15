//
//  CBVersionChecksumBytes.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 03/05/2012.
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
 @brief Represents a key, begining with a version byte and ending with a checksum. Inherits CBByteArray
*/

#ifndef CBVERSIONCHECKSUMBYTESH
#define CBVERSIONCHECKSUMBYTESH

//  Includes

#include "CBByteArray.h"
#include "CBBase58.h"

/**
 @brief Structure for CBVersionChecksumBytes objects. @see CBVersionChecksumBytes.h
*/
typedef struct{
	CBByteArray base; /**< CBByteArray base structure */
	bool cacheString; /**< If true, cache string */
	CBByteArray * cachedString; /**< Pointer to cached CBByteArray string */
} CBVersionChecksumBytes;

/**
 @brief Creates a new CBVersionChecksumBytes object from a base-58 encoded string. The base-58 string will be validated by it's checksum. This returns NULL if the string is invalid. The CB_ERROR_BASE58_DECODE_CHECK_TOO_SHORT error is given if the decoded data is less than 4 bytes. CB_ERROR_BASE58_DECODE_CHECK_INVALID is given if the checksum does not match.
 @param string A base-58 encoded CBString to make a CBVersionChecksumBytes object with a termination character.
 @param cacheString If true, the bitcoin string for this object will be cached in memory.
 @returns A new CBVersionChecksumBytes object or NULL on failure.
 */
CBVersionChecksumBytes * CBNewVersionChecksumBytesFromString(CBByteArray * string, bool cacheString);
/**
 @brief Creates a new CBVersionChecksumBytes object from bytes.
 @param bytes The bytes for the CBVersionChecksumBytes object.
 @param size The size of the byte data.
 @param cacheString If true, the bitcoin string for this object will be cached in memory.
 @returns A new CBVersionChecksumBytes object.
 */
CBVersionChecksumBytes * CBNewVersionChecksumBytesFromBytes(uint8_t * bytes, uint32_t size, bool cacheString);

/**
 @brief Gets a CBVersionChecksumBytes from another object. Use this to avoid casts.
 @param self The object to obtain the CBVersionChecksumBytes from.
 @returns The CBVersionChecksumBytes object.
 */
CBVersionChecksumBytes * CBGetVersionChecksumBytes(void * self);

/**
 @brief Initialises a CBVersionChecksumBytes object from a string.
 @param self The CBVersionChecksumBytes object to initialise.
 @param string A CBString to make a CBVersionChecksumBytes object with a termination character.
 @param cacheString If true, the bitcoin string for this object will be cached in memory.
 @returns true on success, false on failure.
 */
bool CBInitVersionChecksumBytesFromString(CBVersionChecksumBytes * self, CBByteArray * string, bool cacheString);
/**
 @brief Initialises a new CBVersionChecksumBytes object from bytes.
 @param self The CBVersionChecksumBytes object to initialise.
 @param bytes The bytes for the CBVersionChecksumBytes object.
 @param size The size of the byte data.
 @param cacheString If true, the bitcoin string for this object will be cached in memory.
 @returns true on success, false on failure.
 */
bool CBInitVersionChecksumBytesFromBytes(CBVersionChecksumBytes * self, uint8_t * bytes, uint32_t size, bool cacheString);

/**
 @brief Frees a CBVersionChecksumBytes object.
 @param self The CBVersionChecksumBytes object to free.
 */
void CBFreeVersionChecksumBytes(void * self);
 
//  Functions

/**
 @brief Gets the version for a CBVersionChecksumBytes object.
 @param self The CBVersionChecksumBytes object.
 @returns The version code. The Macros CB_PRODUCTION_NETWORK and CB_TEST_NETWORK should correspond to this. 
 */
uint8_t CBVersionChecksumBytesGetVersion(CBVersionChecksumBytes * self);
/**
 @brief Gets the string representation for a CBVersionChecksumBytes object as a base-58 encoded CBString.
 @param self The CBVersionChecksumBytes object.
 @returns The object represented as a base-58 encoded CBString. Do not modify this. Copy if modification is required.
 */
CBByteArray * CBVersionChecksumBytesGetString(CBVersionChecksumBytes * self);

#endif
