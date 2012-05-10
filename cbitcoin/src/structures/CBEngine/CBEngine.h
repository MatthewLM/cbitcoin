//
//  CBEngine.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 27/04/2012.
//  Last modified by Matthew Mitchell on 10/05/2012.
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
 @brief Contains and initialises common structures required. Inherits CBObject
 */

#ifndef CBENGINEH
#define CBENGINEH

//  Includes

#include "CBByteArray.h"
#include "CBEvents.h"

/**
 @brief Virtual function table for CBEngine.
 */
typedef struct{
	CBObjectVT base; /**< CBObjectVT base structure */
}CBEngineVT;

/**
 @brief Structure for CBEngine objects. @see CBEngine.h
 */
typedef struct{
	CBObject base; /**< CBObject base structure */
	CBByteArray * satoshiKey; /**< The key used for alerts by some bitcoin developers */
	CBEvents events;
} CBEngine;

/**
 @brief Creates a new CBEngine object.
 @returns A new CBEngine object.
 */
CBEngine * CBNewEngine(void);

/**
 @brief Creates a new CBEngineVT.
 @returns A new CBEngineVT.
 */
CBEngineVT * CBCreateEngineVT(void);

/**
 @brief Sets the CBEngineVT function pointers.
 @param VT The CBEngineVT to set.
 */
void CBSetEngineVT(CBEngineVT * VT);

/**
 @brief Gets the CBEngineVT. Use this to avoid casts.
 @param self The object to obtain the CBEngineVT from.
 @returns The CBEngineVT.
 */
CBEngineVT * CBGetEngineVT(CBEngine * self);

/**
 @brief Gets a CBEngine from another object. Use this to avoid casts.
 @param self The object to obtain the CBEngine from.
 @returns The CBEngine object.
 */
CBEngine * CBGetEngine(void * self);

/**
 @brief Initialises a CBEngine object
 @param self The CBEngine object to initialise
 @returns true on success, false on failure.
 */
bool CBInitEngine(CBEngine * self);

/**
 @brief Frees a CBEngine object.
 @param self The CBEngine object to free.
 */
void CBFreeEngine(CBEngine * self);

/**
 @brief Does the processing to free a CBEngine object. Should be called by the children when freeing objects.
 @param self The CBEngine object to free.
 */
void CBFreeProcessEngine(CBEngine * self);

//  Functions

void CBDummyErrorReceived(CBError error,char * err,...);

#endif