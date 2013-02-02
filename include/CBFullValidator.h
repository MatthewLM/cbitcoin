//
//  CBFullValidator.h
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
 @brief Validates blocks, finding the main chain.
 */

#ifndef CBFULLVALIDATORH
#define CBFULLVALIDATORH

#include "CBConstants.h"
#include "CBBlock.h"
#include "CBBigInt.h"
#include "CBValidationFunctions.h"
#include "CBAssociativeArray.h"
#include <string.h>

// Constants

typedef enum{
	CB_FULL_VALIDATOR_DISABLE_POW_CHECK = 1, /**< Does not verify the proof of work during validation. Used for testing. */
}CBFullValidatorFlags;

/**
 @brief The return type for CBFullValidatorProcessBlock
 */
typedef enum{
	CB_BLOCK_STATUS_MAIN, /**< The block has extended the main branch. */
	CB_BLOCK_STATUS_SIDE, /**< The block has extended a branch which is not the main branch. */
	CB_BLOCK_STATUS_ORPHAN, /**< The block is an orphan */
	CB_BLOCK_STATUS_BAD, /**< The block is not ok. */
	CB_BLOCK_STATUS_BAD_TIME, /**< The block has a bad time */
	CB_BLOCK_STATUS_DUPLICATE, /**< The block has already been received. */
	CB_BLOCK_STATUS_ERROR, /**< There was an error while processing a block */
	CB_BLOCK_STATUS_CONTINUE, /**< Continue with the validation */
	CB_BLOCK_STATUS_NO_NEW, /**< Cannot remove a branch for a new one. */
} CBBlockStatus;

/**
 @brief The return type for CBFullValidatorCompleteBlockValidation
 */
typedef enum{
	CB_BLOCK_VALIDATION_OK, /**< The validation passed with no problems. */
	CB_BLOCK_VALIDATION_BAD, /**< The block failed valdiation. */
	CB_BLOCK_VALIDATION_ERR, /**< There was an error during the validation processing. */
} CBBlockValidationResult;

#define CB_MAX_ORPHAN_CACHE 20
#define CB_MAX_BRANCH_CACHE 5
#define CB_NO_VALIDATION 0xFFFFFFFF
#define CB_COINBASE_MATURITY 100 // Number of confirming blocks before a block reward can be spent.
#define CB_MAX_SIG_OPS 20000 // Maximum signature operations in a block.
#define CB_BLOCK_ALLOWED_TIME_DRIFT 7200 // 2 Hours from network time

/**
 @brief Gives the last block in a branch used in a chain.
 */
typedef struct{
	uint8_t branch; /**< The branch in the chain */
	uint32_t lastBlock; /**< The last block in the branch the chain uses before the next branch */
} CBChainPath;

/**
 @brief Represents a block branch.
 */
typedef struct{
	uint32_t numBlocks; /**< The number of blocks in the branch */
	uint32_t lastRetargetTime; /**< The block timestamp at the last retarget. */
	uint8_t parentBranch; /**< The branch this branch is connected to. */
	uint32_t parentBlockIndex; /**< The block index in the parent branch which this branch is connected to */
	uint32_t startHeight; /**< The starting height where this branch begins */
	uint32_t lastValidation; /**< The index of the last block in this branch that has been fully validated. */
	CBBigInt work; /**< The total work for this branch. The branch with the highest work is the winner! */
	bool working; /**< True if we this branch is being worked upon */
} CBBlockBranch;

/**
 @brief Structure for CBFullValidator objects. @see CBFullValidator.h
 */
typedef struct{
	CBObject base;
	uint8_t numOrphans; /**< The number of orhpans */
	CBBlock * orphans[CB_MAX_ORPHAN_CACHE]; /**< The ophan block references */
	uint8_t firstOrphan; /**< The orphan added first or rather the front of the orhpan queue (for overwriting) */
	uint8_t mainBranch; /**< The index for the main branch */
	uint8_t numBranches; /**< The number of block-chain branches. Cannot exceed CB_MAX_BRANCH_CACHE */
	CBBlockBranch branches[CB_MAX_BRANCH_CACHE]; /**< The block-chain branches. */
	uint64_t storage; /**< The storage component object */
	CBFullValidatorFlags flags; /**< Flags for validation options */
} CBFullValidator;

/**
 @brief Creates a new CBFullValidator object.
 @param storage The block-chain storage component.
 @param badDataBase Will be set to true if the database needs recovery or false otherwise.
 @param flags The flags used for validating this block.
 @returns A new CBFullValidator object.
 */

CBFullValidator * CBNewFullValidator(uint64_t storage, bool * badDataBase, CBFullValidatorFlags flags);

/**
 @brief Gets a CBFullValidator from another object. Use this to avoid casts.
 @param self The object to obtain the CBFullValidator from.
 @returns The CBFullValidator object.
 */
CBFullValidator * CBGetFullValidator(void * self);

/**
 @brief Initialises a CBFullValidator object.
 @param self The CBFullValidator object to initialise.
 @param storage The block-chain storage component.
 @param badDataBase Will be set to true if the database needs recovery or false otherwise.
 @param flags The flags used for validating this block.
 @returns true on success, false on failure.
 */
bool CBInitFullValidator(CBFullValidator * self, uint64_t storage, bool * badDataBase, CBFullValidatorFlags flags);

/**
 @brief Frees a CBFullValidator object.
 @param self The CBFullValidator object to free.
 */
void CBFreeFullValidator(void * vself);

// Functions

/**
 @brief Adds a block to a branch.
 @param self The CBFullValidator object.
 @param branch The index of the branch to add the block to.
 @param block The block to add.
 @param work The new branch work. This is not the block work but the total work upto this block. This is taken by the function and the old work is freed.
 @returns true on success and false on failure.
 */
bool CBFullValidatorAddBlockToBranch(CBFullValidator * self, uint8_t branch, CBBlock * block, CBBigInt work);
/**
 @brief Adds a block to the orphans.
 @param self The CBFullValidator object.
 @param block The block to add.
 @returns true on success and false on error.
 */
bool CBFullValidatorAddBlockToOrphans(CBFullValidator * self, CBBlock * block);
/**
 @brief Does basic validation on a block 
 @param self The CBFullValidator object.
 @param block The block to valdiate.
 @param networkTime The network time.
 @returns The block status.
 */
CBBlockStatus CBFullValidatorBasicBlockValidation(CBFullValidator * self, CBBlock * block, uint64_t networkTime);
/**
 @brief Completes the validation for a block during main branch extention or reorganisation.
 @param self The CBFullValidator object.
 @param branch The branch being validated
 @param block The block to complete validation for.
 @param height The height of the block.
 @returns CB_BLOCK_VALIDATION_OK if the block passed validation, CB_BLOCK_VALIDATION_BAD if the block failed validation and CB_BLOCK_VALIDATION_ERR on an error.
 */
CBBlockValidationResult CBFullValidatorCompleteBlockValidation(CBFullValidator * self, uint8_t branch, CBBlock * block, uint32_t height);
/**
 @brief Ensures a file can be opened.
 @param self The CBFullValidator object.
 */
void CBFullValidatorEnsureCanOpen(CBFullValidator * self);
/**
 @brief Gets the mimimum time minus one allowed for a new block onto a branch.
 @param self The CBFullValidator object.
 @param branch The id of the branch.
 @param prevIndex The index of the last block to determine the minimum time minus one for when adding onto this block.
 @returns The time on success, or 0 on failure.
 */
uint32_t CBFullValidatorGetMedianTime(CBFullValidator * self, uint8_t branch, uint32_t prevIndex);
/**
 @brief Validates a transaction input.
 @param self The CBFullValidator object.
 @param branch The branch being validated.
 @param block The block begin validated.
 @param blockHeight The height of the block being validated
 @param transactionIndex The index of the transaction to validate.
 @param inputIndex The index of the input to validate.
 @param value Pointer to the total value of the transaction. This will be incremented by this function with the input value.
 @param sigOps Pointer to the total number of signature operations. This is increased by the signature operations for the input and verified to be less that the maximum allowed signature operations.
 @returns CB_BLOCK_VALIDATION_OK if the transaction passed validation, CB_BLOCK_VALIDATION_BAD if the transaction failed validation and CB_BLOCK_VALIDATION_ERR on an error.
 */
CBBlockValidationResult CBFullValidatorInputValidation(CBFullValidator * self, uint8_t branch, CBBlock * block, uint32_t blockHeight, uint32_t transactionIndex, uint32_t inputIndex, uint64_t * value, uint32_t * sigOps);
/**
 @brief Processes a block. Block headers are validated, ensuring the integrity of the transaction data is OK, checking the block's proof of work and calculating the total branch work to the genesis block. If the block extends the main branch complete validation is done. If the block extends a branch to become the new main branch because it has the most work, a re-organisation of the block-chain is done.
 @param self The CBFullValidator object.
 @param block The block to process.
 @param networkTime The network time.
 @return The status of the block.
 */
CBBlockStatus CBFullValidatorProcessBlock(CBFullValidator * self, CBBlock * block, uint64_t networkTime);
/**
 @brief Processes a block into a branch. This is used once basic validation is done on a block and it is determined what branch it needs to go into and when this branch is ready to receive the block.
 @param self The CBFullValidator object.
 @param block The block to process.
 @param networkTime The network time.
 @param branch The branch to add to.
 @param prevBranch The branch of the previous block.
 @param prevBlockIndex The index of the previous block.
 @param prevBlockTarget The target of the previous block.
 @param txHashes The transaction hashes for the block.
 @return The status of the block.
 */
CBBlockStatus CBFullValidatorProcessIntoBranch(CBFullValidator * self, CBBlock * block, uint64_t networkTime, uint8_t branch, uint8_t prevBranch, uint32_t prevBlockIndex, uint32_t prevBlockTarget);
/**
 @brief Saves the last validated blocks from startBranch to endBranch
 @param self The CBFullValidator object.
 @param startBranch The branch with the earliest starting block.
 @param endBranch The endBranch that can go down to the startBranch.
 @returns true if the function executed successfully or false on an error.
 */
bool CBFullValidatorSaveLastValidatedBlocks(CBFullValidator * self, uint8_t branches);
/**
 @brief Updates the unspent outputs and transaction index for a branch, for removing a block's transaction information.
 @param self The CBFullValidator object.
 @param block The block with the transaction data to search for changing unspent outputs.
 @param branch The branch the block is for.
 @param blockIndex The block index in the branch.
 @returns true on successful execution or false on error.
 */
bool CBFullValidatorUpdateUnspentOutputsBackward(CBFullValidator * self, CBBlock * block, uint8_t branch, uint32_t blockIndex);
/**
 @brief Updates the unspent outputs and transaction index for a branch, for adding a block's transaction information.
 @param self The CBFullValidator object.
 @param block The block with the transaction data to search for changing unspent outputs.
 @param branch The branch the block is for.
 @param blockIndex The block index in the branch.
 @returns true on successful execution or false on error.
 */
bool CBFullValidatorUpdateUnspentOutputsForward(CBFullValidator * self, CBBlock * block, uint8_t branch, uint32_t blockIndex);
/**
 @brief Updates the unspent outputs and transaction index for a branch in reverse and loads a block to do this.
 @param self The CBFullValidator object.
 @param branch The branch the block is for.
 @param blockIndex The block index in the branch.
 @param forward If true the indices will be updated when adding blocks, else it will be updated removing blocks for re-organisation.
 @returns true on successful execution or false on error.
 */
bool CBFullValidatorUpdateUnspentOutputsAndLoad(CBFullValidator * self, uint8_t branch, uint32_t blockIndex, bool forward);

#endif
