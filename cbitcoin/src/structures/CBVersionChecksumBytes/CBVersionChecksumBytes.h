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
#include "CBString.h"

/**
 @brief Virtual function table for CBVersionChecksumBytes.
*/
typedef struct{
	CBByteArrayVT base; /**< CBByteArrayVT base structure */
	u_int8_t (*getVersion)(void *); /**< A function pointer to the function to get the version from a CBVersionChecksumBytes */
	CBString * (*getString)(void *); /**< A function pointer to the function to get a string representation of a CBVersionChecksumBytes */
}CBVersionChecksumBytesVT;

/**
 @brief Structure for CBVersionChecksumBytes objects. @see CBVersionChecksumBytes.h
*/
typedef struct{
	CBByteArray base; /**< CBByteArray base structure */
	bool cacheString; /**< If true, cache bitcoin string */
	CBString * cached; /**< Pointer to cached CBString */
} CBVersionChecksumBytes;

/**
 @brief Creates a new CBVersionChecksumBytes object from a base-58 encoded string. The base-58 string will be validated by it's checksum. This returns NULL if the string is invalid. The CB_ERROR_BASE58_DECODE_CHECK_TOO_SHORT error is given if the decoded data is less than 4 bytes. CB_ERROR_BASE58_DECODE_CHECK_INVALID is given if the checksum does not match.
 @param string A base-58 encoded CBString to make a CBVersionChecksumBytes object.
 @param cacheString If true, the bitcoin string for this object will be cached in memory.
 @param events A CBEngine for errors.
 @param dependencies Takes the SHA-256 function for the checksum.
 @returns A new CBVersionChecksumBytes object or NULL on failure.
 */
CBVersionChecksumBytes * CBNewVersionChecksumBytesFromString(CBString * string,bool cacheString,CBEvents * events,CBDependencies * dependencies);
/**
 @brief Creates a new CBVersionChecksumBytes object from bytes.
 @param bytes The bytes for the CBVersionChecksumBytes object.
 @param size The size of the byte data.
 @param cacheString If true, the bitcoin string for this object will be cached in memory.
 @param events A CBEngine for errors.
 @returns A new CBVersionChecksumBytes object.
 */
CBVersionChecksumBytes * CBNewVersionChecksumBytesFromBytes(u_int8_t * bytes,u_int32_t size,bool cacheString,CBEvents * events);

/**
 @brief Creates a new CBVersionChecksumBytesVT.
 @returns A new CBVersionChecksumBytesVT.
 */
CBVersionChecksumBytesVT * CBCreateVersionChecksumBytesVT(void);
/**
 @brief Sets the CBVersionChecksumBytesVT function pointers.
 @param VT The CBVersionChecksumBytesVT to set.
 */
void CBSetVersionChecksumBytesVT(CBVersionChecksumBytesVT * VT);

/**
 @brief Gets the CBVersionChecksumBytesVT. Use this to avoid casts.
 @param self The object to obtain the CBVersionChecksumBytesVT from.
 @returns The CBVersionChecksumBytesVT.
 */
CBVersionChecksumBytesVT * CBGetVersionChecksumBytesVT(void * self);

/**
 @brief Gets a CBVersionChecksumBytes from another object. Use this to avoid casts.
 @param self The object to obtain the CBVersionChecksumBytes from.
 @returns The CBVersionChecksumBytes object.
 */
CBVersionChecksumBytes * CBGetVersionChecksumBytes(void * self);

/**
 @brief Initialises a CBVersionChecksumBytes object from a string.
 @param self The CBVersionChecksumBytes object to initialise.
 @param string A CBString to make a CBVersionChecksumBytes object.
 @param cacheString If true, the bitcoin string for this object will be cached in memory.
 @param events A CBEngine for errors.
 @param dependencies Takes the SHA-256 function for the checksum.
 @returns true on success, false on failure.
 */
bool CBInitVersionChecksumBytesFromString(CBVersionChecksumBytes * self,CBString * string,bool cacheString,CBEvents * events,CBDependencies * dependencies);
/**
 @brief Initialises a new CBVersionChecksumBytes object from bytes.
 @param self The CBVersionChecksumBytes object to initialise.
 @param bytes The bytes for the CBVersionChecksumBytes object.
 @param size The size of the byte data.
 @param cacheString If true, the bitcoin string for this object will be cached in memory.
 @param events A CBEngine for errors.
 @returns true on success, false on failure.
 */
bool CBInitVersionChecksumBytesFromBytes(CBVersionChecksumBytes * self,u_int8_t * bytes,u_int32_t size,bool cacheString,CBEvents * events);

/**
 @brief Frees a CBVersionChecksumBytes object.
 @param self The CBVersionChecksumBytes object to free.
 */
void CBFreeVersionChecksumBytes(CBVersionChecksumBytes * self);

/**
 @brief Does the processing to free a CBVersionChecksumBytes object. Should be called by the children when freeing objects.
 @param self The CBVersionChecksumBytes object to free.
 */
void CBFreeProcessVersionChecksumBytes(CBVersionChecksumBytes * self);
 
//  Functions

/**
 @brief Gets the version for a CBVersionChecksumBytes object.
 @param self The CBVersionChecksumBytes object.
 @returns The version code. The Macros CB_PRODUCTION_NETWORK and CB_TEST_NETWORK should correspond to this. 
 */
u_int8_t CBVersionChecksumBytesGetVersion(CBVersionChecksumBytes * self);
/**
 @brief Gets the string representation for a CBVersionChecksumBytes object as a base-58 encoded CBString.
 @param self The CBVersionChecksumBytes object.
 @returns The object represented as a base-58 encoded CBString. Do not modify this. Copy if modification is required.
 */
CBString * CBVersionChecksumBytesGetString(CBVersionChecksumBytes * self);

#endif