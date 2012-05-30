//
//  CBString.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 16/05/2012.
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
 @brief String structure that uses the reference counting system. Inherits CBObject
*/

#ifndef CBSTRINGH
#define CBSTRINGH

//  Includes

#include "CBObject.h"
#include <string.h>

/**
 @brief Virtual function table for CBString.
*/
typedef struct{
	CBObjectVT base; /**< CBObjectVT base structure */
}CBStringVT;

/**
 @brief Structure for CBString objects. @see CBString.h
*/
typedef struct{
	CBObject base; /**< CBObject base structure */
	char * string; /**< C string. This object is a wrapper around a C string to provide a consistent reference counting system */
} CBString;

/**
 @brief Creates a new CBString object by copying a C string.
 @param string C string to copy into a new CBString object.
 @returns A new CBString object.
 */
CBString * CBNewStringByCopyingCString(char * string);
/**
 @brief Creates a new CBString object by taking a C string. This object becomes responsible for the memory management of this string.
 @param string The C string. Must be the pointer taken from malloc so it can be freed.
 @returns A new CBString object.
 */
CBString * CBNewStringByTakingCString(char * string);

/**
 @brief Creates a new CBStringVT.
 @returns A new CBStringVT.
 */
CBStringVT * CBCreateStringVT(void);
/**
 @brief Sets the CBStringVT function pointers.
 @param VT The CBStringVT to set.
 */
void CBSetStringVT(CBStringVT * VT);

/**
 @brief Gets the CBStringVT. Use this to avoid casts.
 @param self The object to obtain the CBStringVT from.
 @returns The CBStringVT.
 */
CBStringVT * CBGetStringVT(void * self);

/**
 @brief Gets a CBString from another object. Use this to avoid casts.
 @param self The object to obtain the CBString from.
 @returns The CBString object.
 */
CBString * CBGetString(void * self);

/**
 @brief Initialises a CBString object by copying a C string.
 @param self The CBString object to initialise
 @param string C string to copy into a new CBString object.
 @returns true on success, false on failure.
 */
bool CBInitStringByCopyingCString(CBString * self,char * string);
/**
 @brief Initialises a CBString object by taking a C string. This object becomes responsible for the memory management of this string.
 @param self The CBString object to initialise
 @param string The C string. Must be the pointer taken from malloc so it can be freed.
 @returns true on success, false on failure.
 */
bool CBInitStringByTakingCString(CBString * self,char * string);

/**
 @brief Frees a CBString object.
 @param self The CBString object to free.
 */
void CBFreeString(CBString * self);

/**
 @brief Does the processing to free a CBString object. Should be called by the children when freeing objects.
 @param self The CBString object to free.
 */
void CBFreeProcessString(CBString * self);
 
//  Functions

#endif