//
//  CBBlock.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 01/05/2012.
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
 @brief Structure that holds a bitcoin block. Blocks contain transaction information and use a proof of work system to show that they are legitimate. Inherits CBMessage
*/

#ifndef CBBLOCKH
#define CBBLOCKH

//  Includes

#include "CBTransaction.h"
#include "CBBigInt.h"
#include "CBValidationFunctions.h"

// Constants and Macros

#define CB_BLOCK_MAX_SIZE 1000000 // ~0.95 MB
#define CB_BLOCK_HASH_STR_BYTES 32
#define CB_BLOCK_HASH_STR_SIZE CB_BLOCK_HASH_STR_BYTES*2+1
#define CBGetBlock(x) ((CBBlock *)x)

/**
 @brief Structure for CBBlock objects. @see CBBlock.h
*/
typedef struct{
	CBMessage base; /**< CBMessage base structure */
	uint8_t hash[32]; /**< The hash for this block. NULL if it needs to be calculated or set. */
	bool hashSet; /**< True if the hash has been set, false otherwise */
	uint32_t version;
	CBByteArray * prevBlockHash; /**< The previous block hash. */
	CBByteArray * merkleRoot; /**< The merkle tree root hash. */
	uint32_t time; /**< Timestamp for the block. The network uses 32 bits. ??? Add overflow detection. If previous block timestamps are above some threshold, assume an overflow. But this will possibly be fixed in the future in the protocol, so will have to be changed anyway. */
	uint32_t target; /**< The compact target representation. */
	uint32_t nonce; /**< Nounce used in generating the block. */
	uint32_t transactionNum; /**< Number of transactions in the block. */
	CBTransaction ** transactions; /**< The transactions included in this block. NULL if only the header has been received. */
} CBBlock;

/**
 @brief Creates a new CBBlock object. Set the members after creating the block object.
 @returns A new CBBlock object.
 */
CBBlock * CBNewBlock(void);
/**
 @brief Creates a new CBBlock object.
 @param data Serialised block data.
 @returns A new CBBlock object.
 */
CBBlock * CBNewBlockFromData(CBByteArray * data);
/**
 @brief Creates a new CBBlock object with the genesis information for the bitcoin block chain. This will have serialised data as well as object data.
 @returns A new CBBlock object.
 */
CBBlock * CBNewBlockGenesis(void);
/**
 @brief Creates a new CBBlock object with the genesis information for the bitcoin block chain. This function only includes the header. This will have serialised data as well as object data.
 @returns A new CBBlock object.
 */
CBBlock * CBNewBlockGenesisHeader(void);

/**
 @brief Initialises a CBBlock object
 @param self The CBBlock object to initialise
 */
void CBInitBlock(CBBlock * self);
/**
 @brief Initialises a CBBlock object from serialised data
 @param self The CBBlock object to initialise
 @param data The serialised data.
 */
void CBInitBlockFromData(CBBlock * self, CBByteArray * data);

/**
 @brief Initialises a CBBlock object with the genesis information for the bitcoin block chain. This will have serialised data as well as object data.
 @param self The CBBlock object to initialise.
 */
void CBInitBlockGenesis(CBBlock * self);

/**
 @brief Initialises a CBBlock object with the genesis information for the bitcoin block chain. This function only includes the header. This will have serialised data as well as object data.
 @param self The CBBlock object to initialise.
 */
void CBInitBlockGenesisHeader(CBBlock * self);

/**
 @brief Releases and frees all of the objects stored by the CBBlock object.
 @param self The CBBlock object to destroy.
 */
void CBDestroyBlock(void * self);
/**
 @brief Frees a CBBlock object and also calls CBDestroyBlock.
 @param self The CBBlock object to free.
 */
void CBFreeBlock(void * self);
 
//  Functions

/**
 @brief Calculates the merkle root from the transactions and sets the merkle root of this block to the result. The transactions should be serialised.
 @param self The CBBlock object.
 */
void CBBlockCalculateAndSetMerkleRoot(CBBlock * self);
/**
 @brief Calculates the hash for a block.
 @param self The CBBlock object. This should be serialised.
 @param The hash for the block to be set. This should be 32 bytes long.
 */
void CBBlockCalculateHash(CBBlock * self, uint8_t * hash);
/**
 @brief Calculates the length needed to serialise the object.
 @param self The CBBlock object.
 @param transactions If true, the full block, if not true just the header.
 @returns The length read on success, 0 on failure.
 */
uint32_t CBBlockCalculateLength(CBBlock * self, bool transactions);
/**
 @brief Calculates the merkle root from the transactions. The transactions should be serailised.
 @param self The CBBlock object.
 @returns The merkle root on success, NULL on failure. Ensure to free the returned data.
 */
uint8_t * CBBlockCalculateMerkleRoot(CBBlock * self);
/**
 @brief Deserialises a CBBlock so that it can be used as an object.
 @param self The CBBlock object
 @param transactions If true deserialise transactions. If false there do not deserialise for transactions.
 @returns The length read on success, 0 on failure.
 */
uint32_t CBBlockDeserialise(CBBlock * self, bool transactions);
/**
 @brief Retrieves or calculates the hash for a block. Hashes taken from this fuction are cached.
 @param self The CBBlock object. This should be serialised.
 @returns The hash for the block. This is a 32 byte long, double SHA-256 hash and is a pointer to the hash field in the block.
 */
uint8_t * CBBlockGetHash(CBBlock * self);
void CBBlockHashToString(CBBlock * self, char output[CB_BLOCK_HASH_STR_SIZE]);
/**
 @brief Serialises a CBBlock to the byte data.
 @param self The CBBlock object
 @param transactions If true serialise transactions. If false there do not serialise for transactions.
 @param force Serialises everything, replacing any previous serialisation of children objects.
 @returns The length read on success, 0 on failure.
 */
uint32_t CBBlockSerialise(CBBlock * self, bool transactions, bool force);

#endif
