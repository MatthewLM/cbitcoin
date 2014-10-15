//
//  CBChainDescriptor.h
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
 @brief Stores up to 500 block hashes for a block chain to describe a block chain for a peer so that a peer can send relevent block data. Inherits CBMessage
*/

#ifndef CBCHAINDESCRIPTORH
#define CBCHAINDESCRIPTORH

//  Includes

#include "CBMessage.h"

// Getter

#define CBGetChainDescriptor(x) ((CBChainDescriptor *)x)

/**
 @brief Structure for CBChainDescriptor objects. @see CBChainDescriptor.h
*/
typedef struct{
	CBMessage base; /**< CBMessage base structure */
	uint16_t hashNum; /**< Number of block hashes to describe the block chain. Up to 500. */
	CBByteArray ** hashes; /**< Hashes used to describe the block chain. This should contain hashes in the blockchain but not all of them. The maximum allowed is 500. The usual behaviour is to have the 10 last block hashes and then each hash below those going down to the genesis block has a gap that doubles (See https://en.bitcoin.it/wiki/Protocol_specification#getblocks ). The newest block hashes should come first. This should be NULL if empty. The CBGetBlocks object will release each CBByteArray and free the array when the object is freed. */
} CBChainDescriptor;

/**
 @brief Creates a new CBChainDescriptor object.
 @returns A new CBChainDescriptor object.
*/
CBChainDescriptor * CBNewChainDescriptor(void);

/**
 @brief Creates a new CBChainDescriptor object from serialised data.
 @param data Serialised CBChainDescriptor data.
 @returns A new CBChainDescriptor object.
*/
CBChainDescriptor * CBNewChainDescriptorFromData(CBByteArray * data);

/**
 @brief Initialises a CBChainDescriptor object
 @param self The CBChainDescriptor object to initialise
 @returns true on success, false on failure.
*/
void CBInitChainDescriptor(CBChainDescriptor * self);

/**
 @brief Initialises a CBChainDescriptor object from serialised data
 @param self The CBChainDescriptor object to initialise
 @param data The serialised data.
 @returns true on success, false on failure.
*/
void CBInitChainDescriptorFromData(CBChainDescriptor * self, CBByteArray * data);

/**
 @brief Releases and frees all of the objects stored by the CBChainDescriptor object.
 @param self The CBChainDescriptor object to destroy.
 */
void CBDestroyChainDescriptor(void * self);

/**
 @brief Frees a CBChainDescriptor object and also calls CBDestoryChainDescriptor.
 @param self The CBChainDescriptor object to free.
 */
void CBFreeChainDescriptor(void * self);
 
//  Functions

/**
 @brief Adds a hash to the CBChainDescriptor onto the end.
 @param self The CBChainDescriptor object
 @param hash The hash to add.
 */
void CBChainDescriptorAddHash(CBChainDescriptor * self, CBByteArray * hash);

/**
 @brief Deserialises a CBChainDescriptor so that it can be used as an object.
 @param self The CBChainDescriptor object
 @returns The length read on success, 0 on failure.
*/
uint32_t CBChainDescriptorDeserialise(CBChainDescriptor * self);

/**
 @brief Serialises a CBChainDescriptor to the byte data.
 @param self The CBChainDescriptor object
 @returns The length written on success, 0 on failure.
*/
uint16_t CBChainDescriptorSerialise(CBChainDescriptor * self);

/**
 @brief Takes a hash for the CBChainDescriptor and puts it on the end. The hash is not retained so the calling function is releasing control.
 @param self The CBChainDescriptor object
 @param hash The hash to take.
 */
void CBChainDescriptorTakeHash(CBChainDescriptor * self, CBByteArray * hash);

#endif
