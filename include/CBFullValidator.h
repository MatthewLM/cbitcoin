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
#include "CBSafeOutput.h"
#include "CBBlock.h"
#include "CBBigInt.h"
#include "CBValidationFunctions.h"
#include "string.h"

/**
 @brief References an output in the block storage.
 */
typedef struct{
	uint8_t outputHash[32]; /** The transaction hash for the output */
	uint32_t outputIndex; /** The index for the output */
	uint32_t blockIndex; /**< The index of the block containing this output */
	bool coinbase; /**< True if a coinbase output */
	uint8_t branch; /**< The branch this output belongs to. */
	uint32_t position; /**< The positon of the output in the block */
}CBOutputReference;

/**
 @brief Keeps the target and time for a block in a cache.
 */
typedef struct{
	uint32_t target; /** The target for this block */
	uint32_t time; /**< The block's timestamp */
}CBBlockReference;

/**
 @brief Indexes the block references to the block hashes. When stored as a sorted lookup table, an interpolation search can be used to find the index at which a block reference is stored.
 */
typedef struct{
	uint8_t blockHash[32]; /**< The block hash. */
	uint32_t index; /**< The index for the block reference for this hash */
} CBBlockReferenceHashIndex;

/**
 @brief Represents a block branch.
 */
typedef struct{
	uint32_t numRefs; /**< The number of block references in the branch */
	CBBlockReference * references; /**< The block references */
	CBBlockReferenceHashIndex * referenceTable; /**< The lookup table for block references */
	uint32_t lastRetargetTime; /**< The block timestamp at the last retarget. */
	uint8_t parentBranch; /**< The branch this branch is connected to. */
	uint32_t parentBlockIndex; /**< The block index in the parent branch which this branch is connected to */
	uint32_t startHeight; /**< The starting height where this branch begins */
	uint32_t lastValidation; /**< The index of the last block in this branch that has been fully validated. */
	uint32_t numUnspentOutputs; /**< The number of unspent outputs for this branch upto the last validated block. */
	CBOutputReference * unspentOutputs; /**< A list of unspent outputs for this branch upto the last validated block. */
	CBBigInt work; /**< The total work for this branch. The branch with the highest work is the winner! */
	uint16_t numBlockFiles; /**< Number of open block files for this branch. */
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
	void (*logError)(char *,...); /**< Pointer to error callback */
	void * storage; /**< The storage component object */
} CBFullValidator;

/**
 @brief Creates a new CBFullValidator object.
 @returns A new CBFullValidator object.
 */

CBFullValidator * CBNewFullValidator(char * dataDir, void (*logError)(char *,...));

/**
 @brief Gets a CBFullValidator from another object. Use this to avoid casts.
 @param self The object to obtain the CBFullValidator from.
 @returns The CBFullValidator object.
 */
CBFullValidator * CBGetFullValidator(void * self);

/**
 @brief Initialises a CBFullValidator object.
 @param self The CBFullValidator object to initialise.
 @returns true on success, false on failure.
 */
bool CBInitFullValidator(CBFullValidator * self, char * homeDir, void (*logError)(char *,...));

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
CBFullValidatorAddBlockResult CBFullValidatorAddBlockToOrphans(CBFullValidator * self, CBBlock * block);
/**
 @brief Does basic validation on a block 
 @param self The CBFullValidator object.
 @param block The block to valdiate.
 @param txHashes 32 byte double Sha-256 hashes for the transactions in the block, one after the other. These will be modified by this function.
 @param networkTime The network time.
 @returns The block status.
 */
CBBlockStatus CBFullValidatorBasicBlockValidation(CBFullValidator * self, CBBlock * block, uint8_t * txHashes, uint64_t networkTime);
/**
 @brief Same as CBFullValidatorBasicBlockValidation but copies the "txHashes" so that the original data is not modified.
 @see CBFullValidatorBasicBlockValidation
 */
CBBlockStatus CBFullValidatorBasicBlockValidationCopy(CBFullValidator * self, CBBlock * block, uint8_t * txHashes, uint64_t networkTime);
/**
 @brief Completes the validation for a block during main branch extention or reorganisation.
 @param self The CBFullValidator object.
 @param branch The branch being validated
 @param block The block to complete validation for.
 @param txHashes 32 byte double Sha-256 hashes for the transactions in the block, one after the other.
 @param height The height of the block.
 @returns CB_BLOCK_VALIDATION_OK if the block passed validation, CB_BLOCK_VALIDATION_BAD if the block failed validation and CB_BLOCK_VALIDATION_ERR on an error.
 */
CBBlockValidationResult CBFullValidatorCompleteBlockValidation(CBFullValidator * self, uint8_t branch, CBBlock * block, uint8_t * txHashes,uint32_t height);
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
 @param allSpentOutputs The previous outputs returned from CBTransactionValidateBasic
 @param txHashes 32 byte double Sha-256 hashes for the transactions in the block, one after the other.
 @param value Pointer to the total value of the transaction. This will be incremented by this function with the input value.
 @param sigOps Pointer to the total number of signature operations. This is increased by the signature operations for the input and verified to be less that the maximum allowed signature operations.
 @returns CB_BLOCK_VALIDATION_OK if the transaction passed validation, CB_BLOCK_VALIDATION_BAD if the transaction failed validation and CB_BLOCK_VALIDATION_ERR on an error.
 */
CBBlockValidationResult CBFullValidatorInputValidation(CBFullValidator * self, uint8_t branch, CBBlock * block, uint32_t blockHeight, uint32_t transactionIndex,uint32_t inputIndex, CBPrevOut ** allSpentOutputs, uint8_t * txHashes, uint64_t * value, uint32_t * sigOps);
/**
 @brief Finds a block reference ad returns the index or finds the insertion point if the reference was no found.
 @param lookupTable The table of references to search.
 @param refNum The number of references to search.
 @param hash The hash of the block to search for.
 @param found This is set to true if the reference was found or false otherwise.
 @returns The position of the matching reference in the lookup table or the index of where the reference index should go in the case the reference was not found.
 */
uint32_t CBFullValidatorFindBlockReference(CBBlockReferenceHashIndex * lookupTable, uint32_t refNum, uint8_t * hash, bool * found);
/**
 @brief Finds an output reference ad returns the index or finds the insertion point if the reference was no found.
 @param refs The output references to search.
 @param refNum The number of references to search.
 @param hash The transaction hash of the output to search for.
 @param index The index of the output.
 @param found This is set to true if the reference was found or false otherwise.
 @returns The index of the matching reference or the index of where the reference should go in the case the reference was not found.
 */
uint32_t CBFullValidatorFindOutputReference(CBOutputReference * refs, uint32_t refNum, uint8_t * hash, uint32_t index, bool * found);
/**
 @brief Loads a block from storage.
 @param self The CBFullValidator object.
 @param blockRef A reference to the block in storage.
 @param branch The branch the block belongs to.
 @returns A new CBBlockObject with serailised block data which has not been deserialised or NULL on failure.
 */
CBBlock * CBFullValidatorLoadBlock(CBFullValidator * self, CBBlockReference blockRef, uint32_t branch);
/**
 @brief Loads the validation data for a block-chain branch. This only loads data if the validator file for the branch has not been opened already.
 @param self The CBFullValidator object.
 @param branch The index of the branch to load the data for.
 @returns @see CBValidatorLoadResult
 */
CBValidatorLoadResult CBFullValidatorLoadBranchValidator(CBFullValidator * self, uint8_t branch);
/**
 @brief Loads the general validation data. This only loads data if the validator file has not been opened already.
 @param self The CBFullValidator object.
 @returns @see CBValidatorLoadResult
 */
CBValidatorLoadResult CBFullValidatorLoadValidator(CBFullValidator * self);
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
 @param txHashes The transaction hashes for the block.
 @return The status of the block.
 */
CBBlockStatus CBFullValidatorProcessIntoBranch(CBFullValidator * self, CBBlock * block, uint64_t networkTime, uint8_t branch, uint8_t prevBranch, uint32_t prevBlockIndex, uint8_t * txHashes);

#endif
