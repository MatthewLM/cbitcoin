//
//  CBValidator.h
//  cbitcoin
//
//  Created by Matthew Mitchell on 14/09/2012.
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
 @brief Validates blocks, finding the main chain.
 */

#ifndef CBVALIDATORH
#define CBVALIDATORH

#include "CBConstants.h"
#include "CBBlock.h"
#include "CBBigInt.h"
#include "CBValidationFunctions.h"
#include "CBAssociativeArray.h"
#include "CBChainDescriptor.h"
#include <string.h>

// Constants and Macros

#define CBGetValidator(x) ((CBValidator *)x)

typedef enum{
	CB_VALIDATOR_DISABLE_POW_CHECK = 1, /**< Does not verify the proof of work during validation. Used for testing. */
	CB_VALIDATOR_HEADERS_ONLY = 2,/** Only validates and stores the headers of blocks */
}CBValidatorFlags;

/**
 @brief The return type for CBValidatorCompleteBlockValidation and CBProcessBLock
 */
typedef enum{
	CB_BLOCK_VALIDATION_OK, /**< The validation passed with no problems. */
	CB_BLOCK_VALIDATION_BAD, /**< The block failed validation. */
	CB_BLOCK_VALIDATION_ERR, /**< There was an error during the validation processing. */
	CB_BLOCK_VALIDATION_BAD_TIME, /**< The block failed validation due to a bad timestamp */
	CB_BLOCK_VALIDATION_NO_NEW, /**< No new branches are available. */
} CBBlockValidationResult;

typedef enum{
	CB_INPUT_OK,
	CB_INPUT_BAD,
	CB_INPUT_NON_STD
} CBInputCheckResult;

typedef enum{
	CB_ADD_BLOCK_PREV,
	CB_ADD_BLOCK_NEW,
	CB_ADD_BLOCK_LAST
} CBAddBlockType;

#define CB_MAX_ORPHAN_CACHE 20
#define CB_MAX_BRANCH_CACHE 5
#define CB_NO_VALIDATION 0xFFFFFFFF
#define CB_COINBASE_MATURITY 100 // Number of confirming blocks before a block reward can be spent.
#define CB_MAX_SIG_OPS 20000 // Maximum signature operations in a block.
#define CB_BLOCK_ALLOWED_TIME_DRIFT 7200 // 2 Hours from network time
#define CB_MAX_BLOCK_QUEUE CB_MAX_ORPHAN_CACHE

/**
 @brief Describes a point on a chain.
 */
typedef struct{
	uint8_t branch; /**< The branch in the chain */
	uint32_t blockIndex; /**< A block in the branch */
} CBChainPoint;

/**
 @brief Describes the path of a block chain.
 */
typedef struct{
	CBChainPoint points[CB_MAX_BRANCH_CACHE]; /**< A list of last blocks in each branch going back to the genesis branch. */
	uint8_t numBranches; /**< The number of branches in this path. */
} CBChainPath;

/**
 @brief Describes a point on a chain path.
 */
typedef struct{
	uint8_t chainPathIndex; /**< The index of the chain path. */
	uint32_t blockIndex; /**< The block index for the branch this is describing. */
} CBChainPathPoint;

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
	uint16_t working; /**< The number of peers we are receiving this branch from. */
} CBBlockBranch;

/**
 @brief These functions contain the callback object passed into CBValidatorQueueBlock as the first argument. These functions, except for alreadyValidated, onValidatorError and workingOnBranch, should return false if the validator should terminate with an error due to a error in the callback, and then onValidatorError will still be called.
 */
typedef struct{
	bool (*start)(void *); /**< Begins validating blocks. */
	uint64_t (*alreadyValidated)(void *, CBTransaction *); /**< Return the total input value if the transaction has already been validated or else 0. */
	bool (*isOrphan)(void *, CBBlock *); /**< Callback for when a block is an orphan. */
	bool (*deleteBranchCallback)(void *, uint8_t branch); /**< Callback for deleting a branch. "branch" is the index of the branch to delete. */
	bool (*workingOnBranch)(void *, uint8_t branch); /**< The branch to be worked on. Return true if this branch is OK or false if the validator should consider the block invalid. */
	bool (*newBranchCallback)(void *, uint8_t branch, uint8_t parent, uint32_t blockHeight); /**< Callback for when a new branch is made. "branch" is the index of the new branch, parent is the parent branch and "blockHeight" is the height of the first block on the branch. */
	bool (*addBlock)(void *, uint8_t branch, CBBlock * block, uint32_t blockHeight, CBAddBlockType addType); /**< Callback for when the validator adds a block to the main chain (tentatively during reorg). "branch" is the index of the branch the block is on. "block" is the block. "blockHeight" is the height of the block. For "addType" @see CBAddBlockType. */
	bool (*rmBlock)(void *, uint8_t branch, CBBlock * block); /**< Callback for when the validator removes a block from the main chain (tentatively during reorg). "branch" is the index of the branch the block is on. "block" is the block. */
	bool (*finish)(void *, CBBlock * block); /**< Called once the validator is finished processing a block which is not determined to be invalid yet, and when it is safe to commit the changes to the database. */
	bool (*invalidBlock)(void *, CBBlock * block); /**< Called when a block is found to be invalid. */
	bool (*noNewBranches)(void *, CBBlock * block); /**< Called when a block cannot fit onto any new branches */
	void (*onValidatorError)(void *); /**< Called upon an error with the validator. */
} CBValidatorCallbacks;

/**
 @brief Structure for CBValidator objects. @see CBValidator.h
 */
typedef struct{
	CBObject base;
	uint8_t numOrphans; /**< The number of orhpans */
	CBBlock * orphans[CB_MAX_ORPHAN_CACHE]; /**< The ophan block references */
	uint8_t firstOrphan; /**< The front of the orhpan queue (for overwriting) */
	uint8_t mainBranch; /**< The index for the main branch */
	uint8_t numBranches; /**< The number of block-chain branches. Cannot exceed CB_MAX_BRANCH_CACHE */
	CBBlockBranch branches[CB_MAX_BRANCH_CACHE]; /**< The block-chain branches. */
	CBDepObject storage; /**< The storage component object */
	CBValidatorFlags flags; /**< Flags for validation options */
	CBValidatorCallbacks callbacks; /**< @see CBValidatorCallbacks */
	CBDepObject blockProcessThread;
	CBDepObject blockQueueMutex; /**< Mutex for accessing and modifying the block queue. */
	CBDepObject orphanMutex;
	CBDepObject blockProcessWaitCond; /**< Condition for waiting for more queued blocks. */
	CBBlock * blockQueue[CB_MAX_BLOCK_QUEUE];
	void * callbackQueue[CB_MAX_BLOCK_QUEUE];
	uint8_t queueFront;
	uint8_t queueSize;
	bool shutDownThread;
} CBValidator;

/**
 @brief Creates a new CBValidator object.
 @param storage The block-chain storage component.
 @param flags The flags used for validating this block.
 @param callbacks @see CBValidatorCallbacks
 @returns A new CBValidator object.
 */
CBValidator * CBNewValidator(CBDepObject storage, CBValidatorFlags flags, CBValidatorCallbacks callbacks);

/**
 @brief Initialises a CBValidator object.
 @param self The CBValidator object to initialise.
 @param storage The block-chain storage component.
 @param flags The flags used for validating this block.
 @param callbacks @see CBValidatorCallbacks
 @returns true on success, false on failure.
 */
bool CBInitValidator(CBValidator * self, CBDepObject storage, CBValidatorFlags flags, CBValidatorCallbacks callbacks);

/**
 @brief Release a free all of the objects stored by the CBValidator object.
 @param self The CBValidator object to destroy.
 */
void CBDestroyValidator(void * vself);
/**
 @brief Frees a CBValidator object and also calls CBDestroyValidator.
 @param self The CBValidator object to free.
 */
void CBFreeValidator(void * vself);

// Functions

CBCompare CBPtrCompare(CBAssociativeArray *, void * ptr1, void * ptr2);
bool CBValidatorAddBlockDirectly(CBValidator * self, CBBlock * block, void * callbackObj);
/**
 @brief Adds a block to a branch.
 @param self The CBValidator object.
 @param branch The index of the branch to add the block to.
 @param block The block to add.
 @param work The new branch work. This is not the block work but the total work upto this block. This is taken by the function and the old work is freed.
 @returns true on success and false on failure.
 */
bool CBValidatorAddBlockToBranch(CBValidator * self, uint8_t branch, CBBlock * block, CBBigInt work);
/**
 @brief Adds a block to the orphans.
 @param self The CBValidator object.
 @param block The block to add.
 @returns true on success and false on error.
 */
bool CBValidatorAddBlockToOrphans(CBValidator * self, CBBlock * block);
/**
 @brief Does basic validation on a block. It does not check if a block is a duplicate so this should be checked beforehand. Do not request blocks that the node already owns.
 @param self The CBValidator object.
 @param block The block to valdiate.
 @param networkTime The network time.
 @returns The block status.
 */
CBBlockValidationResult CBValidatorBasicBlockValidation(CBValidator * self, CBBlock * block, uint64_t networkTime);
CBErrBool CBValidatorBlockExists(CBValidator * self, uint8_t * hash);
void CBValidatorBlockProcessThread(void * validator);
/**
 @brief Completes the validation for a block during main branch extention or reorganisation.
 @param self The CBValidator object.
 @param branch The branch being validated.
 @param block The block to complete validation for.
 @param height The height of the block.
 @returns CB_BLOCK_VALIDATION_OK if the block passed validation, CB_BLOCK_VALIDATION_BAD if the block failed validation and CB_BLOCK_VALIDATION_ERR on an error.
 */
CBBlockValidationResult CBValidatorCompleteBlockValidation(CBValidator * self, uint8_t branch, CBBlock * block, uint32_t height);
/**
 @brief Gets the block height of the alst block in the main chain.
 @param self The CBValidator
 @returns The block height.
 */
uint32_t CBValidatorGetBlockHeight(CBValidator * self);
/**
 @brief Gets a chain descriptor object for the main chain.
 @param self The CBValidator
 @returns The chain descriptor or NULL on failure.
 */
CBChainDescriptor * CBValidatorGetChainDescriptor(CBValidator * self, uint8_t branch, uint8_t * extraBlock);
/**
 @brief Determines the point on chain1 where chain2 intersects (The fork point or where the second chain ends).
 @param chain1 The first chain to get the path point for.
 @param chain2 The second chain to look for an intersection with the first.
 @returns The point of intersection on the first chain.
 */
CBChainPathPoint CBValidatorGetChainIntersection(CBChainPath * chain1, CBChainPath * chain2);
/**
 @brief Gets the chain path for a branch going back down to the genesis branch.
 @param self The CBValidator object.
 @param branch The branch to start at.
 @param blockIndex The block index to start at.
 @returns The chain path for the branch and block index going back to the genesis branch.
 */
CBChainPath CBValidatorGetChainPath(CBValidator * self, uint8_t branch, uint32_t blockIndex);
/**
 @brief Gets the chain path for the main chain.
 @param self The CBValidator object.
 @returns The chain path for the main chain.
 */
CBChainPath CBValidatorGetMainChainPath(CBValidator * self);
/**
 @brief Gets the mimimum time minus one allowed for a new block onto a branch.
 @param self The CBValidator object.
 @param branch The id of the branch.
 @param prevIndex The index of the last block to determine the minimum time minus one for when adding onto this block.
 @returns The time on success, or 0 on failure.
 */
uint32_t CBValidatorGetMedianTime(CBValidator * self, uint8_t branch, uint32_t prevIndex);
/**
 @brief Validates a transaction input.
 @param self The CBValidator object.
 @param branch The branch being validated.
 @param block The block begin validated.
 @param blockHeight The height of the block being validated
 @param transactionIndex The index of the transaction to validate.
 @param inputIndex The index of the input to validate.
 @param value Pointer to the total value of the transaction. This will be incremented by this function with the input value.
 @param sigOps Pointer to the total number of signature operations. This is increased by the signature operations for the input and verified to be less that the maximum allowed signature operations.
 @returns CB_BLOCK_VALIDATION_OK if the transaction passed validation, CB_BLOCK_VALIDATION_BAD if the transaction failed validation and CB_BLOCK_VALIDATION_ERR on an error.
 */
CBBlockValidationResult CBValidatorInputValidation(CBValidator * self, uint8_t branch, CBBlock * block, uint32_t blockHeight, uint32_t transactionIndex, uint32_t inputIndex, uint64_t * value, uint32_t * sigOps);
CBErrBool CBValidatorLoadUnspentOutputAndCheckMaturity(CBValidator * self, CBPrevOut prevOutRef, uint32_t blockHeight, CBTransactionOutput ** output);
/**
 @brief Processes a block. Without calling CBValidatorBasicBlockValidation (should be called beforehand), blocks are validated, ensuring the integrity of the transaction data is OK, checking the block's proof of work and calculating the total branch work to the genesis block. If the block extends the main branch complete validation is done. If the block extends a branch to become the new main branch because it has the most work, a re-organisation of the block-chain is done. This function only calls the newBranchCallback and newValidBlock callbacks.
 @param self The CBValidator object.
 @param block The block to process.
 @return @see CBBlockValidationResult
 */
CBBlockValidationResult CBValidatorProcessBlock(CBValidator * self, CBBlock * block);
/**
 @brief Processes a block into a branch. This is used once basic validation is done on a block and it is determined what branch it needs to go into and when this branch is ready to receive the block.
 @param self The CBValidator object.
 @param block The block to process.
 @param branch The branch to add to.
 @param prevBranch The branch of the previous block.
 @param prevBlockIndex The index of the previous block.
 @param prevBlockTarget The target of the previous block.
 @param txHashes The transaction hashes for the block.
 @returns The result.
 */
CBBlockValidationResult CBValidatorProcessIntoBranch(CBValidator * self, CBBlock * block, uint8_t branch, uint8_t prevBranch, uint32_t prevBlockIndex, uint32_t prevBlockTarget);
bool CBValidatorQueueBlock(CBValidator * self, CBBlock * block, void * callbackObj);
/**
 @brief Saves the last validated blocks from startBranch to endBranch
 @param self The CBValidator object.
 @param startBranch The branch with the earliest starting block.
 @param endBranch The endBranch that can go down to the startBranch.
 @returns true if the function executed successfully or false on an error.
 */
bool CBValidatorSaveLastValidatedBlocks(CBValidator * self, uint8_t branches);
/**
 @brief Updates the unspent outputs and transaction index for a branch, for removing a block's transaction information.
 @param self The CBValidator object.
 @param block The block with the transaction data to search for changing unspent outputs.
 @param branch The branch the block is for.
 @param blockIndex The block index in the branch.
 @returns true on successful execution or false on error.
 */
bool CBValidatorRemoveBlockFromMainChain(CBValidator * self, CBBlock * block, uint8_t branch, uint32_t blockIndex);
/**
 @brief Updates the unspent outputs and transaction index for a branch, for adding a block's transaction information.
 @param self The CBValidator object.
 @param block The block with the transaction data to search for changing unspent outputs.
 @param branch The branch the block is for.
 @param blockIndex The block index in the branch.
 @param addType @see CBAddBlockType
 @param callbackObj The callback object for calling the validator newBlock callback.
 @returns true on successful execution or false on error.
 */
bool CBValidatorAddBlockToMainChain(CBValidator * self, CBBlock * block, uint8_t branch, uint32_t blockIndex, CBAddBlockType addType, void * callbackObj);
/**
 @brief Updates the unspent outputs and transaction index for a branch in reverse and loads a block to do this.
 @param self The CBValidator object.
 @param branch The branch the block is for.
 @param blockIndex The block index in the branch.
 @param lostTxs The array which should be updated with lost or refound transactions.
 @param forward If true the indices will be updated when adding blocks, else it will be updated removing blocks for re-organisation.
 @returns true on successful execution or false on error.
 */
bool CBValidatorUpdateMainChain(CBValidator * self, uint8_t branch, uint32_t blockIndex, bool forward);
bool CBValidatorVerifyScripts(CBValidator * self, CBTransaction * tx, uint32_t inputIndex, CBTransactionOutput * output, uint32_t * sigOps, bool checkStandard);

#endif
