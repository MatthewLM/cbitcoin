//
//  CBNetworkParameters.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 29/04/2012.
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
 @brief Contains data needed for working with a block chain. Everything for wokring with a particular network including the bitcoin test network as well as the productuon network. Inherits CBObject.
 */

#ifndef CBNETWORKPARAMETERSH
#define CBNETWORKPARAMETERSH

//  Includes

#include "CBObject.h"
#include <stdbool.h>

/**
 @brief Structure for CBNetworkParameters objects. @see CBNetworkParameters.h
 */
typedef struct{
	CBObject base; /**< CBObject base structure */
	u_int8_t networkCode; /**< The network code. The macros CB_TEST_NETWORK and CB_PRODUCTION_NETWORK are used for the test and production bitcoin networks. */
} CBNetworkParameters;

/**
 @brief Creates a new CBNetworkParameters object.
 @returns A new CBNetworkParameters object.
 */
CBNetworkParameters * CBNewNetworkParameters(void);

/**
 @brief Gets a CBNetworkParameters from another object. Use this to avoid casts.
 @param self The object to obtain the CBNetworkParameters from.
 @returns The CBNetworkParameters object.
 */
CBNetworkParameters * CBGetNetworkParameters(void * self);

/**
 @brief Initialises a CBNetworkParameters object
 @param self The CBNetworkParameters object to initialise
 @returns true on success, false on failure.
 */
bool CBInitNetworkParameters(CBNetworkParameters * self);

/**
 @brief Frees a CBNetworkParameters object.
 @param self The CBNetworkParameters object to free.
 */
void CBFreeNetworkParameters(void * self);

//  Functions

#endif