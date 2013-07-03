//
//  CBAccounter.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 01/07/2013.
//  Copyright (c) 2012 Matthew Mitchell
//  
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

/**
 @file
 @brief Keeps track of accounts and processes transactions. Inherits CBObject
*/

#ifndef CBACCOUNTERH
#define CBACCOUNTERH

//  Includes

#include "CBObject.h"
#include "CBDependencies.h"

/**
 @brief Structure for CBAccounter objects. @see CBAccounter.h
*/
typedef struct{
	CBObject base; /**< CBObject base structure */
	CBDepObject storage; /**< The accounting storage. */
} CBAccounter;

/**
 @brief Creates a new CBAccounter object.
 @param dataDir The directory for the accounter storage.
 @returns A new CBAccounter object.
 */
CBAccounter * CBNewAccounter(char * dataDir);

/**
 @brief Gets a CBAccounter from another object. Use this to avoid casts.
 @param self The object to obtain the CBAccounter from.
 @returns The CBAccounter object.
 */
CBAccounter * CBGetAccounter(void * self);

/**
 @brief Initialises a CBAccounter object
 @param self The CBAccounter object to initialise
 @param dataDir The directory for the accounter storage.
 @returns true on success, false on failure.
 */
bool CBInitAccounter(CBAccounter * self, char * dataDir);

/**
@brief Release and free the objects stored by the CBAccounter object.
@param self The CBAccounter object to destroy.
*/
void CBDestroyAccounter(void * self);

/**
 @brief Frees a CBAccounter object and also calls CBDestroyAccounter.
 @param self The CBAccounter object to free.
 */
void CBFreeAccounter(void * self);
 
//  Functions

#endif
