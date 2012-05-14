//
//  CBSha256Hash.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 01/05/2012.
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
 @brief Stores SHA-256 hashes as byte arrays. Inherits CBByteArray
*/

#ifndef CBSHA256HASHH
#define CBSHA256HASHH

//  Includes

#include "CBByteArray.h"
#include <stdbool.h>

/**
 @brief Virtual function table for CBSha256Hash.
*/
typedef struct{
	CBByteArrayVT base; /**< CBByteArrayVT base structure */
}CBSha256HashVT;

/**
 @brief Structure for CBSha256Hash objects. @see CBSha256Hash.h
*/
typedef struct{
	CBByteArray base; /**< CBByteArray base structure */
	int hash;
} CBSha256Hash;

/**
 @brief Creates a new empty CBSha256Hash object.
 @param events CBEngine for errors.
 @returns A new empty CBSha256Hash object.
 */
CBSha256Hash * CBNewEmptySha256Hash(CBEvents * events);
/**
 @brief Creates a new CBSha256Hash object from CBByteArray. Shares the underlying byte data.
 @param bytes CBByteArray to make CBSha256Hash
 @param events CBEngine for errors.
 @returns A new CBSha256Hash object.
 */
CBSha256Hash * CBNewSha256HashFromByteArray(CBByteArray * bytes,CBEvents * events);
/**
 @brief Creates a new CBSha256Hash object from CBByteArray and a hash. Shares the underlying byte data.
 @param bytes CBByteArray to make CBSha256Hash
 @param hash The hash
 @param events CBEngine for errors.
 @returns A new CBSha256Hash object.
 */
CBSha256Hash * CBNewSha256HashFromByteArrayAndHash(CBByteArray * bytes,int hash,CBEvents * events);

/**
 @brief Creates a new CBSha256HashVT.
 @returns A new CBSha256HashVT.
 */
CBSha256HashVT * CBCreateSha256HashVT(void);
/**
 @brief Sets the CBSha256HashVT function pointers.
 @param VT The CBSha256HashVT to set.
 */
void CBSetSha256HashVT(CBSha256HashVT * VT);

/**
 @brief Gets the CBSha256HashVT. Use this to avoid casts.
 @param self The object to obtain the CBSha256HashVT from.
 @returns The CBSha256HashVT.
 */
CBSha256HashVT * CBGetSha256HashVT(void * self);

/**
 @brief Gets a CBSha256Hash from another object. Use this to avoid casts.
 @param self The object to obtain the CBSha256Hash from.
 @returns The CBSha256Hash object.
 */
CBSha256Hash * CBGetSha256Hash(void * self);

/**
 @brief Initialises an empty CBSha256Hash object.
 @param self The CBSha256Hash object to initialise.
 @param events CBEngine for errors.
 @returns true on success, false on failure.
 */
bool CBInitEmptySha256Hash(CBSha256Hash * self,CBEvents * events);
/**
 @brief Initialises a CBSha256Hash object from a CBByteArray
 @param self The CBSha256Hash object to initialise.
 @param hash The hash
 @param events CBEngine for errors.
 @returns true on success, false on failure.
 */
bool CBInitSha256HashFromByteArrayAndHash(CBSha256Hash * self,CBByteArray * bytes,int hash,CBEvents * events);

/**
 @brief Frees a CBSha256Hash object.
 @param self The CBSha256Hash object to free.
 */
void CBFreeSha256Hash(CBSha256Hash * self);

/**
 @brief Does the processing to free a CBSha256Hash object. Should be called by the children when freeing objects.
 @param self The CBSha256Hash object to free.
 */
void CBFreeProcessSha256Hash(CBSha256Hash * self);
 
//  Functions

#endif