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
 */

#ifndef CBOBJECTH
#define CBOBJECTH

//  Includes

#include <stdlib.h>
#include <stdbool.h>
#include "CBConstants.h"
/**
 @brief Virtual function table for CBObject.
 */
typedef struct{
	void (*free)(void *); /**< Pointer to the function used to free a CBObject. */
	void (*release)(void *); /**< Pointer to the function used to release (calling free if necessary) a CBObject by reference. Must exist with each retain to free memory. An additional call to release should exist for each object created. */
	void (*retain)(void *); /**< Pointer to the function used to retain a CBObject. Always call release when done with an object. */
}CBObjectVT;

/**
 @brief Base structure for all other structures. @see CBObject.h
 */
typedef struct CBObject{
	void * VT; /**< Keeps a count of references for an object for memory management. */
	u_int32_t references; /**< Keeps a count of the references to an object for memory management. */
} CBObject;
/**
 @brief Creates a new CBObject.
 @returns A new CBObject.
 */
CBObject * CBNewObject(void);

/**
 @brief Creates a new CBObjectVT.
 @returns A new CBObjectVT.
 */
CBObjectVT * CBCreateObjectVT(void);

/**
 @brief Sets the CBObjectVT function pointers.
 @param VT The CBObjectVT to set.
 */
void CBSetObjectVT(CBObjectVT * VT);

/**
 @brief Gets the CBObjectVT. Use this to avoid casts.
 @param self The object to obtain the CBObjectVT from.
 @returns The CBObjectVT.
 */
CBObjectVT * CBGetObjectVT(void * self);

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
void CBFreeObject(CBObject * self);

/**
 @brief Does the processing to free a CBObject. Should be called by the children when freeing objects.
 @param self The object to free.
 */
void CBFreeProcessObject(CBObject * self);

//  Functions

/**
 @brief Adds the virtual table to the object.
 @param VTStore The pointer to the memory for the virtual table. If NULL the virtual table will be created.
 @param getVT A pointer to the function that creates the virtual table for the object.
 */
void CBAddVTToObject(CBObject * self,void * VTStore,void * getVT);

/**
 @brief Releases a CBObject. The reference counter is decremented and if the reference count is returned to 0, the object will be freed. The pointer will be assigned to NULL.
 @param self The pointer to the object to release.
 */
void CBReleaseObject(CBObject ** self);

/**
 @brief Retains a CBObject. The reference counter is incremented.
 @param self The object to retain.
 */
void CBRetainObject(CBObject * self);

#endif