//
//  CBGetBlocks.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 11/07/2012.
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
 @brief A message to ask for an inventory of blocks or the block headers (getheaders message) up to the "stopAtHash", the last known block or 500 blocks, whichever comes first. The message comes with a block chain descriptor so the recipient can discover how the blockchain is different so they can send the necessary blocks (starting from a point of agreement). Inherits CBMessage
*/

#ifndef CBGETBLOCKSH
#define CBGETBLOCKSH

//  Includes

#include "CBChainDescriptor.h"

// Constants

#define CB_GET_BLOCKS_MAX_SIZE 16045
#define CB_GET_HEADERS_MAX_SIZE 64045

/**
 @brief Structure for CBGetBlocks objects. @see CBGetBlocks.h
*/
typedef struct{
	CBMessage base; /**< CBMessage base structure */
	uint32_t version; /**< Protocol version for this message */
	CBChainDescriptor * chainDescriptor; /**< CBChainDescriptor for determining what blocks are needed. */
	CBByteArray * stopAtHash; /**< The block hash to stop at when retreiving an inventory. Else up to 500 block hashes can be given. */
} CBGetBlocks;

/**
 @brief Creates a new CBGetBlocks object.
 @returns A new CBGetBlocks object.
*/
CBGetBlocks * CBNewGetBlocks(uint32_t version, CBChainDescriptor * chainDescriptor, CBByteArray * stopAtHash);
/**
@brief Creates a new CBGetBlocks object from serialised data.
 @param data Serialised CBGetBlocks data.
 @returns A new CBGetBlocks object.
*/
CBGetBlocks * CBNewGetBlocksFromData(CBByteArray * data);

/**
 @brief Gets a CBGetBlocks from another object. Use this to avoid casts.
 @param self The object to obtain the CBGetBlocks from.
 @returns The CBGetBlocks object.
*/
CBGetBlocks * CBGetGetBlocks(void * self);

/**
 @brief Initialises a CBGetBlocks object
 @param self The CBGetBlocks object to initialise
 @returns true on success, false on failure.
*/
bool CBInitGetBlocks(CBGetBlocks * self, uint32_t version, CBChainDescriptor * chainDescriptor, CBByteArray * stopAtHash);
/**
 @brief Initialises a CBGetBlocks object from serialised data
 @param self The CBGetBlocks object to initialise
 @param data The serialised data.
 @returns true on success, false on failure.
*/
bool CBInitGetBlocksFromData(CBGetBlocks * self, CBByteArray * data);

/**
 @brief Frees a CBGetBlocks object.
 @param self The CBGetBlocks object to free.
 */
void CBFreeGetBlocks(void * self);
 
//  Functions

/**
 @brief Calculates the length needed to serialise the object.
 @param self The CBGetBlocks object.
 @returns The length read on success, 0 on failure.
 */
uint32_t CBGetBlocksCalculateLength(CBGetBlocks * self);
/**
 @brief Deserialises a CBGetBlocks so that it can be used as an object.
 @param self The CBGetBlocks object
 @returns The length read on success, 0 on failure.
*/
uint16_t CBGetBlocksDeserialise(CBGetBlocks * self);
/**
 @brief Serialises a CBGetBlocks to the byte data.
 @param self The CBGetBlocks object
 @param force Serialises everything, replacing any previous serialisation of children objects.
 @returns The length written on success, 0 on failure.
*/
uint16_t CBGetBlocksSerialise(CBGetBlocks * self, bool force);

#endif
