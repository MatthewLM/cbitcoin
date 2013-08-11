//
//  CBGetBlocks.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 11/07/2012.
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
 @brief A message to ask for an inventory of blocks or the block headers (getheaders message) up to the "stopAtHash", the last known block or 500 blocks, whichever comes first. The message comes with a block chain descriptor so the recipient can discover how the blockchain is different so they can send the necessary blocks (starting from a point of agreement). Inherits CBMessage
*/

#ifndef CBGETBLOCKSH
#define CBGETBLOCKSH

//  Includes

#include "CBChainDescriptor.h"

// Constants and Macros

#define CB_GET_BLOCKS_MAX_SIZE 16045
#define CB_GET_HEADERS_MAX_SIZE 64045
#define CBGetGetBlocks(x) ((CBGetBlocks *)x)

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
 @brief Initialises a CBGetBlocks object
 @param self The CBGetBlocks object to initialise
 @returns true on success, false on failure.
*/
void CBInitGetBlocks(CBGetBlocks * self, uint32_t version, CBChainDescriptor * chainDescriptor, CBByteArray * stopAtHash);
/**
 @brief Initialises a CBGetBlocks object from serialised data
 @param self The CBGetBlocks object to initialise
 @param data The serialised data.
 @returns true on success, false on failure.
*/
void CBInitGetBlocksFromData(CBGetBlocks * self, CBByteArray * data);

/**
 @brief Releases and frees all of the objects stored by the CBGetBlocks object.
 @param self The CBGetBlocks object to destroy.
 */
void CBDestroyGetBlocks(void * self);
/**
 @brief Frees a CBGetBlocks object and also calls CBDestroyGetBlocks.
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
