//
//  CBAddress.h
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
 @brief Based upon an ECDSA public key and a network version code. Used for receiving bitcoins. Inherits CBVersionChecksumBytes.
 @details Here is a diagram of how a bitcoin address is structured created by Alan Reiner, developer of Bitcoin Armory (http://bitcoinarmory.com/):
 \image html CBAddress.png
*/

#ifndef CBADDRESSH
#define CBADDRESSH

//  Includes

#include "CBVersionChecksumBytes.h"

/**
 @brief Structure for CBAddress objects. @see CBAddress.h Alias of CBVersionChecksumBytes
*/
typedef CBVersionChecksumBytes CBAddress;

/**
 @brief Creates a new CBAddress object from a RIPEMD-160 hash.
 @param network A CBNetworkParameters object with the network information.
 @param hash The RIPEMD-160 hash. Must be 20 bytes.
 @param cacheString If true, the bitcoin string for this object will be cached in memory.
 @returns A new CBAddress object.
 */
CBAddress * CBNewAddressFromRIPEMD160Hash(uint8_t * hash, uint8_t networkCode, bool cacheString);
/**
 @brief Creates a new CBAddress object from a base-58 encoded string.
 @param self The CBAddress object to initialise.
 @param string The base-58 encoded CBString with a termination character.
 @param cacheString If true, the bitcoin string for this object will be cached in memory.
 @returns A new CBAddress object. Returns NULL on failure such as an invalid bitcoin address.
 */
CBAddress * CBNewAddressFromString(CBByteArray * string, bool cacheString);

/**
 @brief Gets a CBAddress from another object. Use this to avoid casts.
 @param self The object to obtain the CBAddress from.
 @returns The CBAddress object.
 */
CBAddress * CBGetAddress(void * self);

/**
 @brief Initialises a CBAddress object from a RIPEMD-160 hash.
 @param self The CBAddress object to initialise.
 @param network A CBNetworkParameters object with the network information.
 @param hash The RIPEMD-160 hash. Must be 20 bytes.
 @param cacheString If true, the bitcoin string for this object will be cached in memory.
 @returns true on success, false on failure.
 */
bool CBInitAddressFromRIPEMD160Hash(CBAddress * self, uint8_t networkCode, uint8_t * hash, bool cacheString);
/**
 @brief Initialises a CBAddress object from a base-58 encoded string.
 @param self The CBAddress object to initialise.
 @param string The base-58 encoded CBString with a termination character.
 @param cacheString If true, the bitcoin string for this object will be cached in memory.
 @returns true on success, false on failure.
 */
bool CBInitAddressFromString(CBAddress * self, CBByteArray * string, bool cacheString);

/**
 @brief Frees a CBAddress object.
 @param self The CBAddress object to free.
 */
void CBFreeAddress(void * self);
 
//  Functions

#endif
