//
//  CBVersionChecksumBytes.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 03/05/2012.
//  Last modified by Matthew Mitchell on 03/05/2012.
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

/**
 @brief Virtual function table for CBVersionChecksumBytes.
*/
typedef struct{
	CBByteArrayVT base; /**< CBByteArrayVT base structure */
}CBVersionChecksumBytesVT;

/**
 @brief Structure for CBVersionChecksumBytes objects. @see CBVersionChecksumBytes.h
*/
typedef struct{
	CBByteArray base; /**< CBByteArray base structure */
} CBVersionChecksumBytes;

/**
 @brief Creates a new CBVersionChecksumBytes object from a string.
 @param string A string to make a CBVersionChecksumBytes object.
 @param events A CBEngine for errors.
 @returns A new CBVersionChecksumBytes object.
 */
CBVersionChecksumBytes * CBNewVersionChecksumBytesFromString(char * string,CBEngine * events);

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
 @param self The CBVersionChecksumBytes object to initialise
 @param string A string to make a CBVersionChecksumBytes object.
 @returns true on success, false on failure.
 */
bool CBInitVersionChecksumBytesFromString(CBVersionChecksumBytes * self,char * string,CBEngine * events);

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

#endif