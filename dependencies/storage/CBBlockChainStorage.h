//
//  CBBlockChainStorage.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 20/12/2012.
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
 @brief Implements the block-chain storage dependency with use of the CBDatabase.
 */

#ifndef CBBLOCKCHAINSTORAGEH
#define CBBLOCKCHAINSTORAGEH

#include "CBDatabase.h"

/**
 @brief The offsets to parts of the main validation data
 */
typedef enum{
	CB_VALIDATION_MAIN_BRANCH = 1,
	CB_VALIDATION_NUM_BRANCHES = 2,
} CBValidationOffsets;

/**
 @brief The offsets to parts of the branch data.
 */
typedef enum{
	CB_BRANCH_LAST_RETARGET = 0, 
	CB_BRANCH_LAST_VALIDATION = 4, 
	CB_BRANCH_NUM_BLOCKS = 8, 
	CB_BRANCH_PARENT_BLOCK_INDEX = 12, 
	CB_BRANCH_PARENT_BRANCH = 16,
	CB_BRANCH_START_HEIGHT = 17,
	CB_BRANCH_WORK = 21,
} CBBranchOffsets;

/**
 @brief The offsets to parts of the block data
 */
typedef enum{
	CB_BLOCK_HASH = 0, /**< The hash is written before the block data */
	CB_BLOCK_START = 32, /**< The start of the serialised block data */
	CB_BLOCK_TIME = 100,
	CB_BLOCK_TARGET = 104,
} CBBlockOffsets;

/**
 @brief The offsets to parts of the unspent output reference data
 */
typedef enum{
	CB_TX_REF_BLOCK_INDEX = 0, /**< The block index in the branch where the transaction exists. */
	CB_TX_REF_BRANCH = 4, /**< The branch where the transaction exists. */
	CB_TX_REF_POSITION_OUPTUTS = 5, /**< The byte position in the block where the first transaction output exists. */
	CB_TX_REF_LENGTH_OUTPUTS = 9, /**< The length in bytes of the transaction outputs. */
	CB_TX_REF_IS_COINBASE = 13, /**< 1 if the transaction is a coinbase, else 0 */
	CB_TX_REF_NUM_UNSPENT_OUTPUTS = 14, /**< The number of unspent outputs for a transaction. */
	CB_TX_REF_INSTANCE_COUNT = 18, /**< The number of times this transaction has appeared on the block-chain. */
} CBTransactionReferenceOffsets;

/**
 @brief The offsets to parts of the unspent output reference data
 */
typedef enum{
	CB_UNSPENT_OUTPUT_REF_POSITION = 0, /**< Byte position in the block where this output exists. */
	CB_UNSPENT_OUTPUT_REF_LENGTH = 4 /**< Length of the output in bytes. */
} CBUnspentOutputReferenceOffsets;

/**
 @brief The offsets to parts of the block reference data
 */
typedef enum{
	CB_BLOCK_HASH_REF_BRANCH = 0, 
	CB_BLOCK_HASH_REF_INDEX = 1, 
} CBBlockHashRefOffsets;

/**
 @brief The offsets to the extra data
 */
typedef enum{
	CB_EXTRA_CREATED_BLOCK_CHAIN_STORAGE = 0,
	CB_EXTRA_BASIC_VALIDATOR_INFO = 1,
	CB_EXTRA_BRANCHES = 5,
} CBExtraDataOffsets;

// ??? Optimise smaller indices.
typedef struct{
	CBDatabase * database;
	CBDatabaseIndex * blockIndex; /**< key = [branchID, blockID * 4] */
	CBDatabaseIndex * blockHashIndex; /**< key = [hash * 20] Links to the block branch id and position. */
	CBDatabaseIndex * unspentOutputIndex; /**< key = [hash * 32, outputID * 4] */
	CBDatabaseIndex * txIndex; /**< key = [hash * 32] */
} CBBlockChainStorage;

// Other functions

/**
 @brief Changes the number of unspent outputs for a transaction.
 @param storage The storage object
 @param txHash The hash of the transaction
 @param add If true add to the number, else take away.
 @returns true on success and false on failure.
 */
bool CBBlockChainStorageChangeUnspentOutputsNum(CBBlockChainStorage * storage, uint8_t * txHash, bool add);

uint32_t CBBlockChainStorageGetUInt32(CBBlockChainStorage * storage, uint8_t branch, uint32_t blockIndex, CBBlockOffsets offset);

#endif
