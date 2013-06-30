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
#include "CBValidator.h"

/**
 @brief The offsets to parts of the main validation data
 */
typedef enum{
	CB_VALIDATION_FIRST_ORPHAN = 0, 
	CB_VALIDATION_NUM_ORPHANS = 1, 
	CB_VALIDATION_MAIN_BRANCH = 2, 
	CB_VALIDATION_NUM_BRANCHES = 3, 
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
} CBBranchOffsets;

/**
 @brief The offsets to parts of the block data
 */
typedef enum{
	CB_BLOCK_HASH = 0, /**< The hash is written before the block data */
	CB_BLOCK_START = 20, /**< The start of the serialised block data */
	CB_BLOCK_TIME = 88, 
	CB_BLOCK_TARGET = 92, 
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

// ??? Optimise smaller indices.
typedef struct{
	CBDatabase base;
	CBDatabaseTransaction tx;
	CBDatabaseIndex * orphanIndex; /**< key = [orphanID] */
	CBDatabaseIndex * branchIndex; /**< key = [branchID] Also contains basic validator information  */
	CBDatabaseIndex * blockIndex; /**< key = [branchID, blockID * 4] */
	CBDatabaseIndex * blockHashIndex; /**< key = [hash * 20] Links to the block branch id and position. */
	CBDatabaseIndex * branchWorkIndex; /**< key = [branchID] the work of  branch */
	CBDatabaseIndex * unspentOutputIndex; /**< key = [hash * 32, outputID * 4] */
	CBDatabaseIndex * txIndex; /**< key = [hash * 32] */
} CBBlockChainStorage;

// Other functions

/**
 @brief Changes the number of unspent outputs for a transaction.
 @param storage The storage object
 @param txHash The hash of the transaction
 @param change This number will be added to the number of unspent outputs.
 @returns true on success and false on failure.
 */
bool CBBlockChainStorageChangeUnspentOutputsNum(CBBlockChainStorage * storage, uint8_t * txHash, int8_t change);

#endif
