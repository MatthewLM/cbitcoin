//
//  CBBlock.h
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
 @brief Structure that holds a bitcoin block. Blocks contain transaction information and use a proof of work system to show that they are legitimate. Inherits CBMessage
*/

#ifndef CBBLOCKH
#define CBBLOCKH

//  Includes

#include "CBMessage.h"

/**
 @brief Virtual function table for CBBlock.
*/
typedef struct{
	CBMessageVT base; /**< CBMessageVT base structure */
}CBBlockVT;

/**
 @brief Structure for CBBlock objects. @see CBBlock.h
*/
typedef struct{
	CBMessage base; /**< CBMessage base structure */
	u_int64_t version;
	CBSha256Hash prevBlockHash;
	CBSha256Hash merkleRoot;
	u_int64_t time;
	u_int64_t difficultyTarget; 
	u_int64_t nonce;
} CBBlock;

/**
 @brief Creates a new CBBlock object.
 @returns A new CBBlock object.
 */
CBBlock * CBNewBlock(void);

/**
 @brief Creates a new CBBlockVT.
 @returns A new CBBlockVT.
 */
CBBlockVT * CBCreateBlockVT(void);
/**
 @brief Sets the CBBlockVT function pointers.
 @param VT The CBBlockVT to set.
 */
void CBSetBlockVT(CBBlockVT * VT);

/**
 @brief Gets the CBBlockVT. Use this to avoid casts.
 @param self The object to obtain the CBBlockVT from.
 @returns The CBBlockVT.
 */
CBBlockVT * CBGetBlockVT(void * self);

/**
 @brief Gets a CBBlock from another object. Use this to avoid casts.
 @param self The object to obtain the CBBlock from.
 @returns The CBBlock object.
 */
CBBlock * CBGetBlock(void * self);

/**
 @brief Initialises a CBBlock object
 @param self The CBBlock object to initialise
 @returns true on success, false on failure.
 */
bool CBInitBlock(CBBlock * self);

/**
 @brief Frees a CBBlock object.
 @param self The CBBlock object to free.
 */
void CBFreeBlock(CBBlock * self);

/**
 @brief Does the processing to free a CBBlock object. Should be called by the children when freeing objects.
 @param self The CBBlock object to free.
 */
void CBFreeProcessBlock(CBBlock * self);
 
//  Functions

#endif