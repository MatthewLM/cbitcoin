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
 @brief Based upon an ECDSA public key. Used to receive bitcoins with. Inherits CBObject
*/

#ifndef CBADDRESSH
#define CBADDRESSH

//  Includes

#include "CBObject.h"

/**
 @brief Virtual function table for CBAddress.
*/
typedef struct{
	CBObjectVT base; /**< CBObjectVT base structure */
}CBAddressVT;

/**
 @brief Structure for CBAddress objects. @see CBAddress.h
*/
typedef struct{
	CBObject base; /**< CBObject base structure */
} CBAddress;

/**
 @brief Creates a new CBAddress object.
 @returns A new CBAddress object.
 */
CBAddress * CBNewAddress(void);

/**
 @brief Creates a new CBAddressVT.
 @returns A new CBAddressVT.
 */
CBAddressVT * CBCreateAddressVT(void);
/**
 @brief Sets the CBAddressVT function pointers.
 @param VT The CBAddressVT to set.
 */
void CBSetAddressVT(CBAddressVT * VT);

/**
 @brief Gets the CBAddressVT. Use this to avoid casts.
 @param self The object to obtain the CBAddressVT from.
 @returns The CBAddressVT.
 */
CBAddressVT * CBGetAddressVT(void * self);

/**
 @brief Gets a CBAddress from another object. Use this to avoid casts.
 @param self The object to obtain the CBAddress from.
 @returns The CBAddress object.
 */
CBAddress * CBGetAddress(void * self);

/**
 @brief Initialises a CBAddress object
 @param self The CBAddress object to initialise
 @returns true on success, false on failure.
 */
bool CBInitAddress(CBAddress * self);

/**
 @brief Frees a CBAddress object.
 @param self The CBAddress object to free.
 */
void CBFreeAddress(CBAddress * self);

/**
 @brief Does the processing to free a CBAddress object. Should be called by the children when freeing objects.
 @param self The CBAddress object to free.
 */
void CBFreeProcessAddress(CBAddress * self);
 
//  Functions

#endif