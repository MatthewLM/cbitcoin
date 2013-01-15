//
//  CBObject.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 28/04/2012.
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
 @brief The base structure for other structures using OOP-style and reference counting features.
 @details CBObject is the root structure for all other OOP-type structures in cbitcoin. It provides memory mangement through reference counting. Other structures inherit CBObject through the CBOBjectVT structure and the CBOBject structure itself. The retain, release and free functions are used for memory management. 
 
 The rule for memory management is to retain an object before returning it, to retain an object when giving it to another object, and to release an object once the object is no longer needed. When a new object is created it should be retained. Unless required for thread safety, objects don't need to be retained when passed into functions. Whenever an object is created it must be released and objects must be released for each time they are retained. Failure to abide by this properly will cause memory leaks or segmentation faults.
 
 Remember to pass a reference to an object to the release function. Forgeting to do this will break your code.
 */ // ??? Need for all objects to check initiailisation returns and return NULL on failure.

#ifndef CBOBJECTH
#define CBOBJECTH

//  Includes

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "CBConstants.h"

/**
 @brief Base structure for all other structures. @see CBObject.h
 */
typedef struct CBObject{
	void (*free)(void *); /**< Pointer to the function to free the object. */
	uint32_t references; /**< Keeps a count of the references to an object for memory management. */
} CBObject;
/**
 @brief Creates a new CBObject.
 @returns A new CBObject.
 */
CBObject * CBNewObject(void);

/**
 @brief Gets a CBObject from another object. Use this to avoid casts.
 @param self The object to obtain the CBObject from.
 @returns The CBObject.
 */
CBObject * CBGetObject(void * self);

/**
 @brief Initialises a CBObject
 @param self The CBObject to initialise
 @returns This always returns true.
 */
bool CBInitObject(CBObject * self);

/**
 @brief Frees a CBObject.
 @param self The CBOBject to free.
 */
void CBFreeObject(void * self);

//  Functions

/**
 @brief Releases a CBObject. The reference counter is decremented and if the reference count is returned to 0, the object will be freed. The pointer will be assigned to NULL.
 @param self The pointer to the object to release.
 */
void CBReleaseObject(void * self);

/**
 @brief Retains a CBObject. The reference counter is incremented.
 @param self The object to retain.
 */
void CBRetainObject(void * self);

#endif
