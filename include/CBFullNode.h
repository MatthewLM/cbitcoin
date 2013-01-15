//
//  CBFullNode.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 14/09/2012.
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
 @brief Downloads and validates the entire bitcoin block-chain.
 */

#ifndef CBFULLNODEH
#define CBFULLNODEH

#include "CBConstants.h"
#include "CBNetworkCommunicator.h"

/**
 @brief Structure for CBFullNode objects. @see CBFullNode.h
 */
typedef struct{
	CBNetworkCommunicator base;
} CBFullNode;

/**
 @brief Creates a new CBFullNode object.
 @returns A new CBFullNode object.
 */
CBFullNode * CBNewFullNode(void);

/**
 @brief Gets a CBFullNode from another object. Use this to avoid casts.
 @param self The object to obtain the CBFullNode from.
 @returns The CBFullNode object.
 */
CBFullNode * CBGetFullNode(void * self);

/**
 @brief Initialises a CBFullNode object
 @param self The CBFullNode object to initialise
 @returns true on success, false on failure.
 */
bool CBInitFullNode(CBFullNode * self);

/**
 @brief Frees a CBFullNode object.
 @param self The CBFullNode object to free.
 */
void CBFreeFullNode(void * self);

// Functions

/**
 @brief Handles an onBadTime event.
 @param self The CBFullNode object.
 */
void CBFullNodeOnBadTime(void * self);

#endif
