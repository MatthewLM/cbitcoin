//
//  CBValidator.c
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

//  SEE HEADER FILE FOR DOCUMENTATION

#include "CBValidator.h"

//  Constructor

CBValidator * CBNewValidator(CBDepObject storage, CBValidatorFlags flags, CBValidatorCallbacks callbacks){
	CBValidator * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeValidator;
	if (CBInitValidator(self, storage, flags, callbacks))
		return self;
	// If initialisation failed, free the data that exists.
	CBFreeValidator(self);
	return NULL;
}

//  Initialiser

bool CBInitValidator(CBValidator * self, CBDepObject storage, CBValidatorFlags flags, CBValidatorCallbacks callbacks){
	CBInitObject(CBGetObject(self));
	self->storage = storage;
	self->flags = flags;
	self->callbacks = callbacks;
	self->queueFront = 0;
	self->queueSize = 0;
	self->shutDownThread = false;
	// Check whether the database has been created.
	if (CBBlockChainStorageExists(self->storage)) {
		// Found now load information from storage
		// Basic validator information
		if (! CBBlockChainStorageLoadBasicValidator(self)){
			CBLogError("There was an error when loading the validator information.");
			return false;
		}
		// Loop through the orphans
		for (uint8_t x = 0; x < self->numOrphans; x++) {
			if (! CBBlockChainStorageLoadOrphan(self, x)) {
				CBLogError("Could not load orphan %u.", x);
				return false;
			}
		}
		// Loop through the branches
		for (uint8_t x = 0; x < self->numBranches; x++) {
			if (! CBBlockChainStorageLoadBranch(self, x)) {
				CBLogError("Could not load branch %u.", x);
				return false;
			}
			if (! CBBlockChainStorageLoadBranchWork(self, x)) {
				CBLogError("Could not load work for branch %u.", x);
				return false;
			}
		}
	}else{
		// Write the genesis block
		CBBlock * genesis = (flags & CB_VALIDATOR_HEADERS_ONLY) ? CBNewBlockGenesisHeader() : CBNewBlockGenesis();
		if (! CBBlockChainStorageSaveBlock(self, genesis, 0, 0)) {
			CBLogError("Could not save genesis block.");
			return false;
		}
		CBReleaseObject(genesis);
		// Create initial data
		self->mainBranch = 0;
		self->numBranches = 1;
		self->numOrphans = 0;
		self->firstOrphan = 0;
		// Write basic validator information
		if (! CBBlockChainStorageSaveBasicValidator(self)) {
			CBLogError("Could not save the initial basic validation information.");
			return false;
		}
		// Create initial branch information
		self->branches[0].lastRetargetTime = 1231006505;
		self->branches[0].startHeight = 0;
		self->branches[0].numBlocks = 1;
		self->branches[0].lastValidation = 0;
		// Write branch information
		if (! CBBlockChainStorageSaveBranch(self, 0)) {
			CBLogError("Could not save the initial branch information.");
			return false;
		}
		// Create work CBBigInt
		self->branches[0].work.length = 1;
		self->branches[0].work.data = malloc(1);
		self->branches[0].work.data[0] = 0;
		// Write branch work
		if (! CBBlockChainStorageSaveBranchWork(self, 0)) {
			CBLogError("Could not save the initial work.");
			free(self->branches[0].work.data);
			return false;
		}
	}
	// Start thread for block processing
	CBNewMutex(&self->blockQueueMutex);
	CBNewMutex(&self->orphanMutex);
	CBNewCondition(&self->blockProcessWaitCond);
	CBNewThread(&self->blockProcessThread, CBValidatorBlockProcessThread, self);
	return true;
}

//  Destructor

void CBDestroyValidator(void * vself){
	CBValidator * self = vself;
	self->shutDownThread = true;
	CBMutexLock(self->blockQueueMutex);
	if (self->queueSize == 0){
		// The thread is waiting for blocks so wake it.
		CBConditionSignal(self->blockProcessWaitCond);
	}
	CBMutexUnlock(self->blockQueueMutex);
	CBThreadJoin(self->blockProcessThread);
	for (uint8_t x = 0; x < self->queueSize; x++)
		CBReleaseObject(self->blockQueue[(self->queueFront + x) % CB_MAX_BLOCK_QUEUE]);
	CBFreeMutex(self->blockQueueMutex);
	CBFreeMutex(self->orphanMutex);
	CBFreeCondition(self->blockProcessWaitCond);
	// Release orphans
	for (uint8_t x = 0; x < self->numOrphans; x++)
		CBReleaseObject(self->orphans[x]);
		// Release branches
		for (uint8_t x = 0; x < self->numBranches; x++)
			free(self->branches[x].work.data);
}
void CBFreeValidator(void * self){
	CBDestroyValidator(self);
	free(self);
}

//  Functions

CBCompare CBPtrCompare(CBAssociativeArray * foo, void * ptr1, void * ptr2){
	if (ptr1 > ptr2)
		return CB_COMPARE_MORE_THAN;
	if (ptr1 < ptr2)
		return CB_COMPARE_LESS_THAN;
	return CB_COMPARE_EQUAL;
}
bool CBValidatorAddBlockDirectly(CBValidator * self, CBBlock * block, void * callbackObj){
	self->branches[self->mainBranch].lastValidation = self->branches[self->mainBranch].numBlocks;
	// Update unspent outputs
	if (!CBValidatorAddBlockToMainChain(self, block, self->mainBranch, self->branches[self->mainBranch].numBlocks, CB_ADD_BLOCK_LAST, callbackObj)) {
		CBLogError("Could not update the unspent outputs when adding a block directly to the main chain.");
		return false;
	}
	// Update work
	CBBigInt work;
	CBCalculateBlockWork(&work, block->target);
	CBBigIntEqualsAdditionByBigInt(&work, &self->branches[self->mainBranch].work);
	// Add to branch
	if (!CBValidatorAddBlockToBranch(self, self->mainBranch, block, work)){
		CBLogError("There was an error when adding a new block directly to the main branch.");
		return false;
	}
	// Update storage
	if (!CBBlockChainStorageSaveBranch(self, self->mainBranch)) {
		CBLogError("Could not save the last validated block for the main branch when adding a new block directly to the main chain.");
		return false;
	}
	// Callbacks
	if (!self->callbacks.finish(callbackObj, block)) {
		CBLogError("finish returned false when adding directly.");
		return false;
	}
	return true;
}
bool CBValidatorAddBlockToBranch(CBValidator * self, uint8_t branch, CBBlock * block, CBBigInt work){
	// Store the block
	if (! ((self->flags & CB_VALIDATOR_HEADERS_ONLY) ?
			 CBBlockChainStorageSaveBlockHeader : CBBlockChainStorageSaveBlock)(self, block, branch, self->branches[branch].numBlocks)) {
		CBLogError("Could not save a block");
		return false;
	}
	// Increase number of blocks.
	self->branches[branch].numBlocks++; 
	// Modify branch information
	free(self->branches[branch].work.data);
	self->branches[branch].work = work;
	if (! (self->branches[branch].startHeight + self->branches[branch].numBlocks) % 2016)
		self->branches[branch].lastRetargetTime = block->time;
	// Save branch information
	if (! CBBlockChainStorageSaveBranch(self, branch)) {
		CBLogError("Could not write the branch information for branch %u.", branch);
		return false;
	}
	// Update the branch work in storage
	if (! CBBlockChainStorageSaveBranchWork(self, branch)) {
		CBLogError("Could not write the branch work for branch %u.", branch);
		return false;
	}
	return true;
}
bool CBValidatorAddBlockToOrphans(CBValidator * self, CBBlock * block){
	CBMutexLock(self->orphanMutex);
	// Save orphan.
	uint8_t pos;
	if (self->numOrphans == CB_MAX_ORPHAN_CACHE) {
		// Release old orphan
		CBReleaseObject(self->orphans[self->firstOrphan]);
		pos = self->firstOrphan;
		self->firstOrphan++;
	}else
		// Adding an orphan.
		pos = self->numOrphans++;
	// Update validation information in storage
	if (! CBBlockChainStorageSaveBasicValidator(self)) {
		CBLogError("There was an error when saving the basic validation information when adding an orhpan block.");
		return false;
	}
	// Add to memory
	self->orphans[pos] = block;
	CBMutexUnlock(self->orphanMutex);
	CBRetainObject(block);
	// Add to storage
	if (! ((self->flags & CB_VALIDATOR_HEADERS_ONLY) ?
			 CBBlockChainStorageSaveOrphanHeader : CBBlockChainStorageSaveOrphan)(self, block, pos)) {
		CBLogError("Could not save an orphan.");
		return false;
	}
	return true;
}
CBBlockValidationResult CBValidatorBasicBlockValidation(CBValidator * self, CBBlock * block, uint64_t networkTime){
	// Get the block hash
	uint8_t * hash = CBBlockGetHash(block);
	// Check block hash against target and that it is below the maximum allowed target.
	if (! (self->flags & CB_VALIDATOR_DISABLE_POW_CHECK)
		&& ! CBValidateProofOfWork(hash, block->target))
		return CB_BLOCK_VALIDATION_BAD;
	// Check block has transactions
	if (block->transactionNum == 0)
		return CB_BLOCK_VALIDATION_BAD;
	// Check the block is within two hours of the network time.
	if (block->time > networkTime + CB_BLOCK_ALLOWED_TIME_DRIFT)
		return CB_BLOCK_VALIDATION_BAD_TIME;
	// Verify the merkle root only when we are fully validating.
	if (! (self->flags & CB_VALIDATOR_HEADERS_ONLY)) {
		// Calculate merkle root.
		uint8_t * txHashes = CBBlockCalculateMerkleRoot(block);
		// Check merkle root
		int res = memcmp(txHashes, CBByteArrayGetData(block->merkleRoot), 32);
		free(txHashes);
		if (res)
			return CB_BLOCK_VALIDATION_BAD;
	}
	return CB_BLOCK_VALIDATION_OK;
}
CBErrBool CBValidatorBlockExists(CBValidator * self, uint8_t * hash){
	CBMutexLock(self->orphanMutex);
	// Check orphans first
	for (uint8_t x = 0; x < self->numOrphans; x++)
		if (!memcmp(CBBlockGetHash(self->orphans[x]), hash, 32)){
			CBMutexUnlock(self->orphanMutex);
			return CB_TRUE;
		}
	CBMutexUnlock(self->orphanMutex);
	// Check block queue
	CBMutexLock(self->blockQueueMutex);
	for (uint8_t x = 0; x < self->queueSize; x++)
		if (!memcmp(hash, CBBlockGetHash(self->blockQueue[(self->queueFront + x) % CB_MAX_BLOCK_QUEUE]), 32)){
			CBMutexUnlock(self->blockQueueMutex);
			return CB_TRUE;
		}
	CBMutexUnlock(self->blockQueueMutex);
	// Look in block hash index
	CBErrBool res = CBBlockChainStorageBlockExists(self, hash);
	return  res;
}
void CBValidatorBlockProcessThread(void * validator){
	CBValidator * self = validator;
	CBMutexLock(self->blockQueueMutex);
	for (;;) {
		if (self->queueSize == 0 && !self->shutDownThread)
			// Wait for more blocks
			CBConditionWait(self->blockProcessWaitCond, self->blockQueueMutex);
		// Check to see if the thread should terminate
		if (self->shutDownThread){
			CBMutexUnlock(self->blockQueueMutex);
			return;
		}
		// Get the block
		CBBlock * block = self->blockQueue[self->queueFront];
		CBMutexUnlock(self->blockQueueMutex);
		void * callbackObj = self->callbackQueue[self->queueFront];
		switch (CBValidatorProcessBlock(self, block)) {
			case CB_BLOCK_VALIDATION_BAD:
				if (!self->callbacks.invalidBlock(callbackObj, block)) {
					CBLogError("invalidBlock returned false");
					self->callbacks.onValidatorError(callbackObj);
				}
				break;
			case CB_BLOCK_VALIDATION_ERR:
				self->callbacks.onValidatorError(callbackObj);
				break;
			case CB_BLOCK_VALIDATION_NO_NEW:
				if (!self->callbacks.noNewBranches(callbackObj, block)) {
					CBLogError("noNewBranches returned false");
					self->callbacks.onValidatorError(callbackObj);
				}
				break;
			default:
				if (!self->callbacks.finish(callbackObj, block)) {
					CBLogError("finish returned false");
					self->callbacks.onValidatorError(callbackObj);
				}
				break;
		}
		// Now we have finished with the block, remove it from the queue
		CBMutexLock(self->blockQueueMutex);
		CBReleaseObject(block);
		if (++self->queueFront == CB_MAX_BLOCK_QUEUE)
			self->queueFront = 0;
		self->queueSize--;
	}
}
CBBlockValidationResult CBValidatorCompleteBlockValidation(CBValidator * self, uint8_t branch, CBBlock * block, uint32_t height){
	// Check that the first transaction is a coinbase transaction.
	if (! CBTransactionIsCoinBase(block->transactions[0]))
		return CB_BLOCK_VALIDATION_BAD;
	uint64_t blockReward = CBCalculateBlockReward(height);
	uint64_t coinbaseOutputValue;
	uint32_t sigOps = 0;
	// Do validation for transactions.
	for (uint32_t x = 0; x < block->transactionNum; x++) {
		// Skip transaction validation if validated already
		uint64_t alreadyValidatedInputVal = self->callbacks.alreadyValidated(self->callbackQueue[self->queueFront], block->transactions[x]);
		uint64_t outputValue = 0;
		if (alreadyValidatedInputVal == 0) {
			// Check for duplicate transactions which have unspent outputs, except for two blocks (See BIP30 https://en.bitcoin.it/wiki/BIP_0030 and https://github.com/bitcoin/bitcoin/blob/master/src/main.cpp#L1568)
			if (memcmp(CBBlockGetHash(block), (uint8_t []){0xec, 0xca, 0xe0, 0x00, 0xe3, 0xc8, 0xe4, 0xe0, 0x93, 0x93, 0x63, 0x60, 0x43, 0x1f, 0x3b, 0x76, 0x03, 0xc5, 0x63, 0xc1, 0xff, 0x61, 0x81, 0x39, 0x0a, 0x4d, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00}, 32)
				&& memcmp(CBBlockGetHash(block), (uint8_t []){0x21, 0xd7, 0x7c, 0xcb, 0x4c, 0x08, 0x38, 0x6a, 0x04, 0xac, 0x01, 0x96, 0xae, 0x10, 0xf6, 0xa1, 0xd2, 0xc2, 0xa3, 0x77, 0x55, 0x8c, 0xa1, 0x90, 0xf1, 0x43, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00}, 32)) {
				// Now check for duplicate in previous blocks.
				CBErrBool exists = CBBlockChainStorageIsTransactionWithUnspentOutputs(self, CBTransactionGetHash(block->transactions[x]));
				if (exists == CB_ERROR) {
					CBLogError("Could not detemine if a transaction exists with unspent outputs.");
					return CB_BLOCK_VALIDATION_ERR;
				}
				if (exists == CB_TRUE)
					return CB_BLOCK_VALIDATION_BAD;
			}
			// Check that the transaction is final.
			if (! CBTransactionIsFinal(block->transactions[x], block->time, height))
				return CB_BLOCK_VALIDATION_BAD;
			// Do the basic validation
			if (! CBTransactionValidateBasic(block->transactions[x], ! x, &outputValue))
				return CB_BLOCK_VALIDATION_BAD;
			// Count and verify sigops
			sigOps += CBTransactionGetSigOps(block->transactions[x]);
			if (sigOps > CB_MAX_SIG_OPS)
				return CB_BLOCK_VALIDATION_BAD;
		}else
			// Count the output value
			for (uint32_t x = 0; x < block->transactions[x]->outputNum; x++)
				outputValue += block->transactions[x]->outputs[x]->value;
		if (! x)
			// This is the coinbase, take the output as the coinbase output.
			coinbaseOutputValue = outputValue;
		else {
			uint64_t inputValue = 0;
			if (alreadyValidatedInputVal == 0) {
				// Verify each input and count input values
				for (uint32_t y = 0; y < block->transactions[x]->inputNum; y++) {
					CBBlockValidationResult res = CBValidatorInputValidation(self, branch, block, height, x, y, &inputValue, &sigOps);
					if (res != CB_BLOCK_VALIDATION_OK)
						return res;
				}
			}else
				// Else take the cached input value
				inputValue = alreadyValidatedInputVal;
			// Verify values and add to block reward
			if (inputValue < outputValue)
				return CB_BLOCK_VALIDATION_BAD;
			blockReward += inputValue - outputValue;
		}
	}
	// Verify coinbase output for reward
	if (coinbaseOutputValue > blockReward)
		return CB_BLOCK_VALIDATION_BAD;
	return CB_BLOCK_VALIDATION_OK;
}
CBChainPathPoint CBValidatorGetChainIntersection(CBChainPath * chain1, CBChainPath * chain2){
	CBChainPathPoint point;
	for (uint8_t x = 0;; x++) {
		for (uint8_t y = 0; y < chain2->numBranches; y++) {
			if (chain1->points[x].branch == chain2->points[y].branch) {
				// This is where the intersection exists.
				point.chainPathIndex = x;
				if (chain1->points[x].blockIndex < chain2->points[x].blockIndex)
					point.blockIndex = chain1->points[x].blockIndex;
				else
					point.blockIndex = chain2->points[x].blockIndex;
				return point;
			}
		}
	}
}
uint32_t CBValidatorGetBlockHeight(CBValidator * self){
	return self->branches[self->mainBranch].startHeight + self->branches[self->mainBranch].numBlocks - 1;
}
CBChainDescriptor * CBValidatorGetChainDescriptor(CBValidator * self){
	CBChainDescriptor * chainDesc = CBNewChainDescriptor();
	uint32_t blockIndex = self->branches[self->mainBranch].numBlocks - 1;
	uint8_t branch = self->mainBranch;
	uint32_t step = 1, start = 0;
	for (;; start++) {
		// Add the block hash
		CBByteArray * hash = CBNewByteArrayOfSize(32);
		if (! CBBlockChainStorageGetBlockHash(self, branch, blockIndex, CBByteArrayGetData(hash))) {
			CBLogError("Could not get a block hash from the storage for a chain descriptor.");
			return NULL;
		}
		CBChainDescriptorTakeHash(chainDesc, hash);
		// If this is the genesis block, break
		if (self->branches[branch].startHeight == 0
			&& blockIndex == 0)
			break;
		// Now step backwards
		if (start > 9)
			step *= 2;
		uint32_t stepsToGo = step;
		for (;;) {
			if (blockIndex < stepsToGo) {
				// If this is the genesis branch, go to the genesis block
				if (self->branches[branch].startHeight == 0){
					blockIndex = 0;
					break;
				}else{
					// Go to the next branch
					stepsToGo -= blockIndex;
					blockIndex = self->branches[branch].parentBlockIndex;
					branch = self->branches[branch].parentBranch;
				}
			}else{
				blockIndex -= stepsToGo;
				break;
			}
		}
	}
	return chainDesc;
}
CBChainPath CBValidatorGetChainPath(CBValidator * self, uint8_t branch, uint32_t blockIndex){
	CBChainPath chainPath;
	// The starting index of the path stack.
	chainPath.numBranches = 0;
	// Make the first last block, the last block in the branch we are validating up-to.
	chainPath.points[chainPath.numBranches].blockIndex = blockIndex;
	// Get all last blocks and next branches for path information
	while (self->branches[branch].startHeight > 0) { // While not the genesis branch
		// Add path information
		chainPath.points[chainPath.numBranches].branch = branch;
		chainPath.points[++chainPath.numBranches].blockIndex = self->branches[branch].parentBlockIndex;
		branch = self->branches[branch].parentBranch;
	}
	// Add the genesis branch
	chainPath.points[chainPath.numBranches].branch = branch;
	chainPath.numBranches++;
	return chainPath;
}
CBChainPath CBValidatorGetMainChainPath(CBValidator * self){
	return CBValidatorGetChainPath(self, self->mainBranch, self->branches[self->mainBranch].numBlocks - 1);
}
uint32_t CBValidatorGetMedianTime(CBValidator * self, uint8_t branch, uint32_t prevIndex){
	uint32_t height = self->branches[branch].startHeight + prevIndex;
	height = (height > 12)? 12 : height;
	uint8_t x = height/2; // Go back median amount
	for (;;) {
		if (prevIndex >= x) {
			prevIndex -= x;
			return CBBlockChainStorageGetBlockTime(self, branch, prevIndex);
		}
		x -= prevIndex + 1;
		branch = self->branches[branch].parentBranch;
		prevIndex = self->branches[branch].numBlocks - 1;
	}
}
CBBlockValidationResult CBValidatorInputValidation(CBValidator * self, uint8_t branch, CBBlock * block, uint32_t blockHeight, uint32_t transactionIndex, uint32_t inputIndex, uint64_t * value, uint32_t * sigOps){
	// Create variable for the previous output reference.
	CBPrevOut prevOutRef = block->transactions[transactionIndex]->inputs[inputIndex]->prevOut;
	// Check that the previous output is not already spent by this block.
	for (uint32_t a = 1; a < transactionIndex; a++)
		for (uint32_t b = 0; b < block->transactions[a]->inputNum; b++)
			if (CBByteArrayCompare(prevOutRef.hash, block->transactions[a]->inputs[b]->prevOut.hash) == CB_COMPARE_EQUAL
				&& prevOutRef.index == block->transactions[a]->inputs[b]->prevOut.index)
				// Duplicate found.
				return CB_BLOCK_VALIDATION_BAD;
	// Now we need to check that the output is in this block (before this transaction) or unspent elsewhere in the blockchain.
	CBTransactionOutput * prevOut;
	bool found = false;
	// Start transaction search at 1. We cannot spend the coinbase output.
	for (uint32_t a = 1; a < transactionIndex; a++) {
		if (! memcmp(CBTransactionGetHash(block->transactions[a]), CBByteArrayGetData(prevOutRef.hash), 32)) {
			// This is the transaction hash. Make sure there is the output.
			if (block->transactions[a]->outputNum <= prevOutRef.index)
				// Too few outputs.
				return CB_BLOCK_VALIDATION_BAD;
			prevOut = block->transactions[a]->outputs[prevOutRef.index];
			// Retain previous output, as though it was retained when returned by a function
			CBRetainObject(prevOut);
			// Yes we have found a previous output in the block.
			found = true;
			break;
		}
	}
	if (! found) {
		// Not found in this block. Look in database for the unspent output.
		CBErrBool exists = CBBlockChainStorageUnspentOutputExists(self, CBByteArrayGetData(prevOutRef.hash), prevOutRef.index);
		if (exists == CB_ERROR) {
			CBLogError("Could not determine if an unspent output exists.");
			return CB_BLOCK_VALIDATION_ERR;
		}
		if (exists == CB_FALSE)
			return CB_BLOCK_VALIDATION_BAD;
		// Exists so therefore load the unspent output
		CBErrBool ok = CBValidatorLoadUnspentOutputAndCheckMaturity(self, prevOutRef, blockHeight, &prevOut);
		if (ok == CB_ERROR) {
			CBLogError("Could not load an unspent output and check maturity.");
			return CB_BLOCK_VALIDATION_ERR;
		}
		if (ok == CB_FALSE)
			return CB_BLOCK_VALIDATION_BAD;
	}
	// We have sucessfully received an output for this input. Verify the input script for the output script.
	if (CBValidatorVerifyScripts(self, block->transactions[transactionIndex], inputIndex, prevOut, sigOps, false) == CB_INPUT_BAD) {
		CBReleaseObject(prevOut);
		return CB_BLOCK_VALIDATION_BAD;
	}
	// Increment the value with the input value then be done with the output
	*value += prevOut->value;
	CBReleaseObject(prevOut);
	return CB_BLOCK_VALIDATION_OK;
}
CBErrBool CBValidatorLoadUnspentOutputAndCheckMaturity(CBValidator * self, CBPrevOut prevOutRef, uint32_t blockHeight, CBTransactionOutput ** output){
	bool coinbase;
	uint32_t outputHeight;
	*output = CBBlockChainStorageLoadUnspentOutput(self, CBByteArrayGetData(prevOutRef.hash), prevOutRef.index, &coinbase, &outputHeight);
	if (! *output) {
		CBLogError("Could not load an unspent output.");
		return CB_ERROR;
	}
	// Check coinbase maturity
	return !coinbase || blockHeight - outputHeight >= CB_COINBASE_MATURITY;
}
CBBlockValidationResult CBValidatorProcessBlock(CBValidator * self, CBBlock * block){
	uint8_t prevBranch;
	uint32_t prevBlockIndex;
	uint32_t prevBlockTarget;
	// Call start
	if (!self->callbacks.start(self->callbackQueue[self->queueFront])) {
		CBLogError("start returned false");
		return CB_BLOCK_VALIDATION_ERR;
	}
	// Determine what type of block this is.
	if (CBBlockChainStorageBlockExists(self, CBByteArrayGetData(block->prevBlockHash))) {
		// Has a block in the block hash index. Get branch and index.
		if (CBBlockChainStorageGetBlockLocation(self, CBByteArrayGetData(block->prevBlockHash), &prevBranch, &prevBlockIndex) != CB_TRUE) {
			CBLogError("Could not get the location of a previous block.");
			return CB_BLOCK_VALIDATION_ERR;
		}
		// Get previous block target
		prevBlockTarget = CBBlockChainStorageGetBlockTarget(self, prevBranch, prevBlockIndex);
		if (!prevBlockTarget) {
			CBLogError("Could not get the target of a previous block.");
			return CB_BLOCK_VALIDATION_ERR;
		}
	}else{
		// Orphan block. End here.
		// Add block to orphans
		if (CBValidatorAddBlockToOrphans(self, block))
			return CB_BLOCK_VALIDATION_OK;
		else
			return CB_BLOCK_VALIDATION_ERR;
	}
	// Not an orphan. See if this is an extention or new branch.
	uint8_t branch;
	if (prevBlockIndex == self->branches[prevBranch].numBlocks - 1){
		// Call workingOnBranch
		if (! self->callbacks.workingOnBranch(self->callbackQueue[self->queueFront], prevBranch))
			return CB_BLOCK_VALIDATION_BAD;
		// Extension
		branch = prevBranch;
	}else{
		// New branch
		if (self->numBranches == CB_MAX_BRANCH_CACHE) {
			// ??? SINCE BRANCHES ARE TRIMMED OR DELETED THERE ARE SECURITY IMPLICATIONS: A peer could prevent other branches having a chance by creating many branches when the node is not up-to-date. To protect against this potential attack peers should only be allowed to provide one branch at a time and before accepting new branches from peers there should be a completed branch which is not the main branch.
			// Trim branch from longest ago which is not being worked on and isn't main branch
			uint8_t earliestBranch = 0;
			uint32_t earliestIndex = self->branches[0].parentBlockIndex + self->branches[0].numBlocks;
			for (uint8_t x = 1; x < self->numBranches; x++) {
				if (self->branches[x].working == 0
					&& self->branches[x].parentBlockIndex + self->branches[x].numBlocks < earliestIndex) {
					earliestIndex = self->branches[x].parentBlockIndex + self->branches[x].numBlocks;
					earliestBranch = x;
				}
			}
			// ??? Add check if only the peer we got this block from was previously working on the earliest branch
			if (self->branches[earliestBranch].working || earliestBranch == self->mainBranch)
				// Cannot overwrite branch
				return CB_BLOCK_VALIDATION_NO_NEW;
			// Call workingOnBranch
			if (! self->callbacks.workingOnBranch(self->callbackQueue[self->queueFront], earliestBranch))
				return CB_BLOCK_VALIDATION_BAD;
			// Check to see the highest block dependency for other branches.
			uint32_t highestDependency = 0;
			uint8_t mergeBranch = 255;
			for (uint8_t x = 0; x < self->numBranches; x++) {
				if (self->branches[x].parentBranch == earliestBranch) {
					if (self->branches[x].parentBlockIndex > highestDependency) {
						highestDependency = self->branches[x].parentBlockIndex;
						mergeBranch = x;
					}
				}
			}
			if (mergeBranch != 255) {
				// Merge the branch with the branch with the highest dependent branch.
				// Delete blocks after highest dependency
				for (uint32_t x = highestDependency; x < self->branches[earliestBranch].numBlocks; x++){
					// Delete
					if (! CBBlockChainStorageDeleteBlock(self, earliestBranch, x)) {
						CBLogError("Could not delete a block when removing a branch.");
						return CB_BLOCK_VALIDATION_ERR;
					}
				}
				// Change block keys from earliest branch to dependent branch
				for (uint8_t x = 0; x < highestDependency; x++) {
					if (! CBBlockChainStorageMoveBlock(self, earliestBranch, x, mergeBranch, x)) {
						CBLogError("Could not move a block for merging two branches.");
						return CB_BLOCK_VALIDATION_ERR;
					}
				}
				// Make parent information for dependent branch reflect the earliest branch
				self->branches[mergeBranch].numBlocks += self->branches[earliestBranch].numBlocks;
				self->branches[mergeBranch].parentBlockIndex = self->branches[earliestBranch].parentBlockIndex;
				self->branches[mergeBranch].parentBranch = self->branches[earliestBranch].parentBranch;
				self->branches[mergeBranch].startHeight = self->branches[earliestBranch].startHeight;
				// Write updated branch information to storage
				if (!CBBlockChainStorageSaveBranch(self, mergeBranch)) {
					CBLogError("Could not save the updated branch information for merging two branches together.");
					return CB_BLOCK_VALIDATION_ERR;
				}
				// Find all of the other dependent branches for the earliest branch and update the parent branch information.
				for (uint8_t x = 0; x < self->numBranches; x++) {
					if (x != mergeBranch && self->branches[x].parentBranch == earliestBranch) {
						self->branches[x].parentBranch = mergeBranch;
						// Update in storage
						if (! CBBlockChainStorageSaveBranch(self, x)) {
							CBLogError("Could not write the updated parent branch from the overwritten branch during a merge.");
							return CB_BLOCK_VALIDATION_ERR;
						}
					}
				}
				// Find all of the dependent branches for the dependent branch and update the parent block index.
				for (uint8_t x = 0; x < self->numBranches; x++) {
					if (self->branches[x].parentBranch == mergeBranch) {
						self->branches[x].parentBlockIndex += self->branches[earliestBranch].numBlocks;
						// Update in storage
						if (! CBBlockChainStorageSaveBranch(self, x)) {
							CBLogError("Could not write an updated parent block index during a merge.");
							return CB_BLOCK_VALIDATION_ERR;
						}
					}
				}
				// Call the delete branch callback
				if (!self->callbacks.deleteBranchCallback(self->callbackQueue[self->queueFront], earliestBranch)){
					CBLogError("deleteBranchCallback returned false");
					return CB_BLOCK_VALIDATION_ERR;
				}
			}
			// Overwrite earliest branch
			branch = earliestBranch;
			// Delete work
			free(self->branches[branch].work.data);
			// Do not delete from storage as it will get overwritten immediately anyway.
			// Do not stage here either as the branch needs to be overwritten in order for the database to be consistent.
		}else{
			// No overwritting
			branch = self->numBranches++;
			// Update number of branches into storage
			if (! CBBlockChainStorageSaveBasicValidator(self)) {
				CBLogError("Could not write the new number of branches.");
				return CB_BLOCK_VALIDATION_ERR;
			}
		}
		// Initialise minimal data the new branch.
		// Record parent branch.
		self->branches[branch].parentBranch = prevBranch;
		self->branches[branch].parentBlockIndex = prevBlockIndex;
		// Set retarget time
		self->branches[branch].lastRetargetTime = self->branches[prevBranch].lastRetargetTime;
		// Calculate the work
		self->branches[branch].work.length = self->branches[prevBranch].work.length;
		self->branches[branch].work.data = malloc(self->branches[branch].work.length);
		// Copy work over
		memcpy(self->branches[branch].work.data, self->branches[prevBranch].work.data, self->branches[branch].work.length);
		// Remove later block work down to the fork
		for (uint32_t y = prevBlockIndex + 1; y < self->branches[prevBranch].numBlocks; y++) {
			CBBigInt tempWork;
			CBCalculateBlockWork(&tempWork, prevBlockTarget);
			CBBigIntEqualsSubtractionByBigInt(&self->branches[branch].work, &tempWork);
			free(tempWork.data); 
		}
		// Set the remaining data
		self->branches[branch].working = 0;
		self->branches[branch].numBlocks = 0;
		self->branches[branch].lastValidation = CB_NO_VALIDATION;
		self->branches[branch].startHeight = self->branches[prevBranch].startHeight + prevBlockIndex + 1;
		// Write branch info
		if (!CBBlockChainStorageSaveBranch(self, branch)) {
			CBLogError("Could not save new branch data.");
			return CB_BLOCK_VALIDATION_ERR;
		}
		// Write branch work
		if (!CBBlockChainStorageSaveBranchWork(self, branch)) {
			CBLogError("Could not save the new branch's work.");
			return CB_BLOCK_VALIDATION_ERR;
		}
		// Call newBranchCallback
		if (!self->callbacks.newBranchCallback(self->callbackQueue[self->queueFront], branch, prevBranch, self->branches[branch].startHeight)){
			CBLogError("newBranchCallback returned false.");
			return CB_BLOCK_VALIDATION_ERR;
		}
	}
	// Got branch ready for block. Now process into the branch.
	CBBlockValidationResult result = CBValidatorProcessIntoBranch(self, block, branch, prevBranch, prevBlockIndex, prevBlockTarget);
	if (result != CB_BLOCK_VALIDATION_OK)
		return result;
	// Now go through any orphans
	bool first = true;
	CBMutexLock(self->orphanMutex);
	for (uint8_t x = 0; x < self->numOrphans;){
		if (! memcmp(CBBlockGetHash(block), CBByteArrayGetData(self->orphans[x]->prevBlockHash), 32)) {
			if (first)
				// Do not release the block passed to this function
				first = false;
			else
				// Release orphan blocks.
				CBReleaseObject(block);
			// Moving onto this block.
			// Get previous block target for orphan.
			uint32_t target = CBBlockChainStorageGetBlockTarget(self, branch, self->branches[branch].numBlocks - 1);
			if (!target) {
				CBLogError("Could not get a block target for an orphan.");
				return CB_BLOCK_VALIDATION_ERR;
			}
			// Process into the branch.
			result = CBValidatorProcessIntoBranch(self, self->orphans[x], branch, branch, self->branches[branch].numBlocks - 1, target);
			if (result == CB_BLOCK_VALIDATION_ERR) {
				CBLogError("There was an error when processing an orphan into a branch.");
				return CB_BLOCK_VALIDATION_ERR;
			}
			// Remove orphan now we are done.
			self->numOrphans--;
			if (! self->numOrphans){
				// No more orphans left. End here and release the orphan.
				CBReleaseObject(self->orphans[x]);
				break;
			}
			// Store orphan as temporary value used for checking the block hash.
			block = self->orphans[x];
			if (self->numOrphans > x)
				// Move orphans down
				memmove(self->orphans + x, self->orphans + x + 1, sizeof(*self->orphans) * (self->numOrphans - x));
			// Reset x and set the block as we will now go through the orphans again until no more can be satisfied for this branch.
			x = 0;
		}else
			// Try next orphan
			x++;
	}
	CBMutexUnlock(self->orphanMutex);
	return result;
}
CBBlockValidationResult CBValidatorProcessIntoBranch(CBValidator * self, CBBlock * block, uint8_t branch, uint8_t prevBranch, uint32_t prevBlockIndex, uint32_t prevBlockTarget){
	// Check timestamp
	if (block->time <= CBValidatorGetMedianTime(self, prevBranch, prevBlockIndex)){
		CBBlockChainStorageRevert(self->storage);
		return CB_BLOCK_VALIDATION_BAD;
	}
	uint32_t target;
	bool change = ! ((self->branches[prevBranch].startHeight + prevBlockIndex + 1) % 2016);
	if (change)
		// Difficulty change for this branch
		target = CBCalculateTarget(prevBlockTarget, block->time - self->branches[branch].lastRetargetTime);
	else
		target = prevBlockTarget;
	// Check target
	if (block->target != target){
		CBBlockChainStorageRevert(self->storage);
		return CB_BLOCK_VALIDATION_BAD;
	}
	// Calculate total work
	CBBigInt work;
	CBCalculateBlockWork(&work, block->target);
	CBBigIntEqualsAdditionByBigInt(&work, &self->branches[branch].work);
	if (branch != self->mainBranch) {
		// Check if the block is adding to a side branch without becoming the main branch
		if (CBBigIntCompareToBigInt(&work, &self->branches[self->mainBranch].work) != CB_COMPARE_MORE_THAN)
			// Add to branch without complete validation
			return CBValidatorAddBlockToBranch(self, branch, block, work) ? CB_BLOCK_VALIDATION_OK : CB_BLOCK_VALIDATION_ERR;
		// Potential block-chain reorganisation.
		// Get the chain path for the potential new chain down to the genesis branch.
		CBChainPath chainPath = CBValidatorGetChainPath(self, prevBranch, self->branches[prevBranch].numBlocks - 1);
		// Validate the side branch. This starts at the first branch where validation is not complete and goes up through the blocks, validating each one. Includes Prior Branches!
		// Start at the main branch and go backwards until we reach the path of the potential new chain, and thus the fork point.
		uint8_t tempBranch = self->mainBranch;
		// Start at the latest block in the main chain
		uint32_t tempBlockIndex = self->branches[self->mainBranch].numBlocks - 1;
		uint8_t pathIndex;
		for (;;) {
			// See if the main-chain branch we are currently at, is in the chain path for the potetial new chain.
			bool done = false;
			for (uint8_t x = chainPath.numBranches; x--;) {
				if (chainPath.points[x].branch == tempBranch) {
					// The branch for the main-chain path is a branch in the potential new main chain branch path.
					// Go down to fork point, unless this is the fork point
					if (tempBlockIndex > chainPath.points[x].blockIndex) {
						// Above fork point, go down to fork point
						for (;; tempBlockIndex--) {
							// Change unspent output and transaction information, as going backwards through the main-chain.
							if (! CBValidatorUpdateMainChain(self, tempBranch, tempBlockIndex, false)){
								CBLogError("Could not remove a block from the main chain.");
								return CB_BLOCK_VALIDATION_ERR;
							}
							// Break before tempBlockIndex becomes the last block. We only want to remove everything over the last block in the potential new chain path branch. And we want tempBlockIndex to equal the block above the last block.
							if (tempBlockIndex == chainPath.points[x].blockIndex + 1)
								break;
						}
						// We have gone down this branch to the block on the main chain which is above the block which both chains share. The fork point needs to be set to the block on the new chain, so decrement x to the next branch and set tempBlockIndex to zero.
						x--;
						tempBlockIndex = 0;
					}else
						// Else we are on the block which is the last of blocks which is on the main chain. We have already validated this so we are interested in the next one on the new chain.
						if (tempBlockIndex == chainPath.points[x].blockIndex) {
							// Move to the next branch
							x--;
							tempBlockIndex = 0;
						}else
							// We can just move up a block
							tempBlockIndex++;
					// We have gone down to the fork point, so we are done.
					done = true;
					// Set the path index of the fork point.
					pathIndex = x;
					break;
				}
			}
			if (done)
				// If done, exit now and do not go to the next branch
				break;
			// We need to go down to next branch, to see if that is included in the potential new chain.
			// Update information for all the blocks in the branch
			for (tempBlockIndex++; tempBlockIndex--;) {
				// Change unspent output and transaction information, going backwards.
				if (! CBValidatorUpdateMainChain(self, tempBranch, tempBlockIndex, false)){
					CBLogError("Could not remove a block from the main chain.");
					return CB_BLOCK_VALIDATION_ERR;
				}
			}
			// Go down a branch
			tempBlockIndex = self->branches[tempBranch].parentBlockIndex;
			tempBranch = self->branches[tempBranch].parentBranch;
		}
		// Go upwards to last validation point in new chain path. Then we need to start validating for the potential new chain.
		for (;; pathIndex--) {
			// See if the last block in the path branch is higher or equal to the last validation point. In this case we go to the validation point and end there.
			if (chainPath.points[pathIndex].blockIndex >= self->branches[chainPath.points[pathIndex].branch].lastValidation) {
				// Go up to last validation point, updating unspent output and transaction information.
				for (;tempBlockIndex <= self->branches[chainPath.points[pathIndex].branch].lastValidation; tempBlockIndex++) {
					if (! CBValidatorUpdateMainChain(self, chainPath.points[pathIndex].branch, tempBlockIndex, true)) {
						CBLogError("Could not update indicies for going to a previously validated point during reorganisation.");
						return CB_BLOCK_VALIDATION_ERR;
					}
				}
				break;
			}else if (self->branches[chainPath.points[pathIndex].branch].lastValidation != CB_NO_VALIDATION){
				// The branch is validated enough to reach the last block. Go up to last block, updating unspent output and transaction information.
				for (;tempBlockIndex <= chainPath.points[pathIndex].blockIndex; tempBlockIndex++) {
					if (! CBValidatorUpdateMainChain(self, chainPath.points[pathIndex].branch, tempBlockIndex, true)){
						CBLogError("Could not update indicies for going to the last block for a path during reorganisation.");
						return CB_BLOCK_VALIDATION_ERR;
					}
				}
				// If the pathIndex is to the last branch, we can end here.
				if (! pathIndex)
					// Done
					break;
				// Reset block index for next branch
				tempBlockIndex = 0;
			}else
				// This is the block we should start at, as no validation has been done on this branch.
				break;
		}
		// At this point tempBlockIndex is the block we start at and chainPath[pathIndex].branch is the branch.
		// Now validate all blocks going up.
		bool atLeastOne = false; // True when at least one block passed validation for a branch
		tempBranch = chainPath.points[pathIndex].branch;
		// Saving the first branch for updating last validated blocks.
		uint8_t branches = 1 << tempBranch;
		// Loop through blocks unless we have reached the newest block already.
		if (tempBlockIndex < self->branches[branch].numBlocks || tempBranch != branch){
			for (;;) {
				// Get block to validate
				CBBlock * tempBlock = CBBlockChainStorageLoadBlock(self, tempBlockIndex, tempBranch);
				if (tempBlock == NULL) {
					CBLogError("Could not load a block from storage for validating onto the main chain.");
					return CB_BLOCK_VALIDATION_ERR;
				}
				if (! CBBlockDeserialise(tempBlock, ! (self->flags & CB_VALIDATOR_HEADERS_ONLY))){
					CBLogError("Could not deserailise a block to validate during reorganisation.");
					return CB_BLOCK_VALIDATION_ERR;
				}
				// Validate block if not headers only
				if (! (self->flags & CB_VALIDATOR_HEADERS_ONLY)) {
					CBBlockValidationResult res = CBValidatorCompleteBlockValidation(self, tempBranch, tempBlock, self->branches[tempBranch].startHeight + tempBlockIndex);
					if (res == CB_BLOCK_VALIDATION_BAD
						|| res == CB_BLOCK_VALIDATION_ERR){
						CBReleaseObject(tempBlock);
						// Clear IO operations, thus reverting to previous main-chain.
						CBBlockChainStorageRevert(self->storage);
						if (res == CB_BLOCK_VALIDATION_BAD) {
							// Save last valided blocks for each branch
							if (! CBValidatorSaveLastValidatedBlocks(self, branches)) {
								CBLogError("Could not save the last validated blocks for a validatation error during reorganisation");
								return CB_BLOCK_VALIDATION_ERR;
							}
						}
						return res;
					}
					// Update the validation information, if this block is the latest validated.
					if (tempBlockIndex > self->branches[tempBranch].lastValidation
						|| self->branches[tempBranch].lastValidation == CB_NO_VALIDATION){
						self->branches[tempBranch].lastValidation = tempBlockIndex;
						if (! atLeastOne){
							// Add this branch to be updated
							branches |= 1 << tempBranch;
							atLeastOne = true;
						}
					}
				}
				// Update the unspent output indicies.
				if (!CBValidatorAddBlockToMainChain(self, tempBlock, tempBranch, self->branches[tempBranch].lastValidation, CB_ADD_BLOCK_NEW, self->callbackQueue[self->queueFront])) {
					CBLogError("Could not update the unspent outputs when validating a block during reorganisation.");
					return CB_BLOCK_VALIDATION_ERR;
				}
				// This block passed validation. Move onto next block.
				if (tempBlockIndex == chainPath.points[pathIndex].blockIndex) {
					// We came to the last block in the branch for the potential new chain.
					// Check if last block
					if (tempBranch == branch)
						break;
					// Go to next branch
					tempBranch = chainPath.points[--pathIndex].branch;
					tempBlockIndex = 0;
					atLeastOne = false;
				}else
					// We have not reached the end of this branch for the chain yet. Thus move to the next block in the branch.
					tempBlockIndex++;
				// We are done with the block.
				CBReleaseObject(tempBlock);
			}
			// Save last validated blocks
			if (!CBValidatorSaveLastValidatedBlocks(self, branches)) {
				CBLogError("Could not save the last validated blocks during reorganisation");
				return CB_BLOCK_VALIDATION_ERR;
			}
		}
		// We are done with the reorganisation. Now we validate the block for the new main chain.
	}
	if (! (self->flags & CB_VALIDATOR_HEADERS_ONLY)) {
		// Validate a new block for the main chain.
		CBBlockValidationResult res = CBValidatorCompleteBlockValidation(self, branch, block, self->branches[branch].startHeight + self->branches[branch].numBlocks);
		// If the validation was bad then reset the pending IO and return.
		if (res == CB_BLOCK_VALIDATION_BAD) {
			CBBlockChainStorageRevert(self->storage);
			return res;
		}
		if (res == CB_BLOCK_VALIDATION_ERR)
			return res;
		self->branches[branch].lastValidation = self->branches[branch].numBlocks;
		// Update the unspent output indicies.
		if (!CBValidatorAddBlockToMainChain(self, block, branch, self->branches[branch].lastValidation, CB_ADD_BLOCK_LAST, self->callbackQueue[self->queueFront])) {
			CBLogError("Could not update the unspent outputs when adding a block to the main chain.");
			return CB_BLOCK_VALIDATION_ERR;
		}
	}else
		// Call addBlock 
		if (!self->callbacks.addBlock(self->callbackQueue[self->queueFront], branch, block, self->branches[branch].startHeight + self->branches[branch].numBlocks, CB_ADD_BLOCK_LAST)) {
			CBLogError("addBlock returned false");
			return false;
		}
	// Everything is OK so update branch and unspent outputs.
	if (!CBValidatorAddBlockToBranch(self, branch, block, work)){
		CBLogError("There was an error when adding a new block to the branch.");
		return CB_BLOCK_VALIDATION_ERR;
	}
	// Update storage
	if (!CBBlockChainStorageSaveBranch(self, branch)) {
		CBLogError("Could not save the last validated block for a branch when adding a new block to the main chain.");
		return CB_BLOCK_VALIDATION_ERR;
	}
	if (branch != self->mainBranch) {
		// Update main branch
		self->mainBranch = branch;
		if (! CBBlockChainStorageSaveBasicValidator(self)) {
			CBLogError("Could not save the new main branch when adding a new block to the main chain.");
			return CB_BLOCK_VALIDATION_ERR;
		}
	}
	return CB_BLOCK_VALIDATION_OK;
}
bool CBValidatorQueueBlock(CBValidator * self, CBBlock * block, void * callbackObj){
	// Schedule the block for complete processing.
	CBRetainObject(block);
	CBMutexLock(self->blockQueueMutex);
	if (self->queueSize == CB_MAX_BLOCK_QUEUE)
		return false;
	uint8_t index = (self->queueFront + self->queueSize) % CB_MAX_BLOCK_QUEUE;
	self->blockQueue[index] = block;
	self->callbackQueue[index] = callbackObj;
	self->queueSize++;
	if (self->queueSize == 1)
		// We have just added a block to the queue when there was not one before so wake-up the processing thread.
		CBConditionSignal(self->blockProcessWaitCond);
	CBMutexUnlock(self->blockQueueMutex);
	return true;
}
bool CBValidatorSaveLastValidatedBlocks(CBValidator * self, uint8_t branches){
	for (uint8_t x = 0; x < 5; x++) {
		if (branches & (1 << x)
			&& ! CBBlockChainStorageSaveBranch(self, x)) {
			CBLogError("Could not save the last validated block for a branch.");
			return false;
		}
	}
	return true;
}
bool CBValidatorRemoveBlockFromMainChain(CBValidator * self, CBBlock * block, uint8_t branch, uint32_t blockIndex){
	// Call rmBlock
	if (!self->callbacks.rmBlock(self->callbackQueue[self->queueFront], branch, block)) {
		CBLogError("rmBlock returned false");
		return false;
	}
	// If headers only we can end here
	if (self->flags & CB_VALIDATOR_HEADERS_ONLY)
		return true;
	// Update unspent outputs... Go through transactions, adding the prevOut references and removing the outputs for one transaction at a time.
	uint8_t * txReadData = NULL;
	uint32_t txReadDataSize = 0;
	for (uint32_t x = block->transactionNum; x--;) {
		// Remove transaction from transaction index.
		uint8_t * txHash = CBTransactionGetHash(block->transactions[x]);
		if (! CBBlockChainStorageDeleteTransactionRef(self, txHash)) {
			CBLogError("Could not remove transaction reference from the transaction index.");
			free(txReadData);
			return false;
		}
		// Loop thorugh inputs
		for (uint32_t y = block->transactions[x]->inputNum; y--;) {
			if (x) {
				// Only non-coinbase transactions contain prevOut references in inputs.
				uint8_t * prevTxHash = CBByteArrayGetData(block->transactions[x]->inputs[y]->prevOut.hash);
				uint32_t outputIndex = block->transactions[x]->inputs[y]->prevOut.index;
				// Add to storage
				// Read transaction outputs from the block
				uint32_t outputsPos;
				if (! CBBlockChainStorageLoadOutputs(self, prevTxHash, &txReadData, &txReadDataSize, &outputsPos)) {
					CBLogError("Could not load a transaction's outputs for re-organisation.");
					free(txReadData);
					return false;
				}
				// Find the position of the output by looping through outputs
				uint32_t txCursor = 0;
				for (uint32_t z = block->transactions[x]->inputs[y]->prevOut.index; z--;) {
					// Add 8 for value
					txCursor += 8;
					// Add script value
					if (txReadData[txCursor] < 253)
						txCursor += 1 + txReadData[txCursor];
					else if (txReadData[txCursor] == 253)
						txCursor += 3 + CBArrayToInt16(txReadData, txCursor + 1);
					else if (txReadData[txCursor] == 254)
						txCursor += 5 + CBArrayToInt32(txReadData, txCursor + 1);
					else if (txReadData[txCursor] == 255)
						txCursor += 9 + CBArrayToInt64(txReadData, txCursor + 1);
				}
				// Add the posiion within the outputs to the start of the outputs.
				outputsPos += txCursor;
				// Get length
				uint32_t outputLen = 8;
				if (txReadData[txCursor] < 253)
					outputLen += 1 + txReadData[txCursor];
				else if (txReadData[txCursor] == 253)
					outputLen += 3 + CBArrayToInt16(txReadData, txCursor + 1);
				else if (txReadData[txCursor] == 254)
					outputLen += 5 + CBArrayToInt32(txReadData, txCursor + 1);
				else if (txReadData[txCursor] == 255)
					outputLen += 9 + CBArrayToInt64(txReadData, txCursor + 1);
				// Save unspent output
				if (! CBBlockChainStorageSaveUnspentOutput(self, prevTxHash, outputIndex, outputsPos, outputLen, true)) {
					CBLogError("Could not save an unspent output reference when going backwards during re-organisation.");
					free(txReadData);
					return false;
				}
			}
		}
		// Loop through outputs
		for (uint32_t y = block->transactions[x]->outputNum; y--;) {
			// Remove output from storage
			if (! CBBlockChainStorageDeleteUnspentOutput(self, txHash, y, false)) {
				CBLogError("Could not remove unspent output reference during re-organisation.");
				free(txReadData);
				return false;
			}
		}
	}
	// Free transaction read memory.
	free(txReadData);
	return true;
}
bool CBValidatorAddBlockToMainChain(CBValidator * self, CBBlock * block, uint8_t branch, uint32_t blockIndex, CBAddBlockType addType, void * callbackObj){
	// Call addBlock
	if (!self->callbacks.addBlock(callbackObj, branch, block, self->branches[branch].startHeight + blockIndex, addType)) {
		CBLogError("addBlock returned false");
		return false;
	}
	// If headers only we can end here
	if (self->flags & CB_VALIDATOR_HEADERS_ONLY)
		return true;
	// Update unspent outputs... Go through transactions, removing the prevOut references and adding the outputs for one transaction at a time.
	uint32_t cursor = 80; // Cursor to find output positions.
	uint8_t byte = CBByteArrayGetByte(CBGetMessage(block)->bytes, 80);
	cursor += byte < 253 ? 1 : (byte == 253 ? 3 : (byte == 254 ? 5 : 9));
	for (uint32_t x = 0; x < block->transactionNum; x++) {
		// Move along version number
		cursor += 4;
		// Move along input number
		byte = CBByteArrayGetByte(CBGetMessage(block)->bytes, cursor);
		cursor += byte < 253 ? 1 : (byte == 253 ? 3 : (byte == 254 ? 5 : 9));
		// First remove output references than add new outputs.
		for (uint32_t y = 0; y < block->transactions[x]->inputNum; y++) {
			if (x) {
				// Only non-coinbase transactions contain prevOut references in inputs.
				uint8_t * txHash = CBByteArrayGetData(block->transactions[x]->inputs[y]->prevOut.hash);
				uint32_t outputIndex = block->transactions[x]->inputs[y]->prevOut.index;
				// Remove output
				if (! CBBlockChainStorageDeleteUnspentOutput(self, txHash, outputIndex, true)) {
					CBLogError("Could not remove an output as unspent.");
					return false;
				}
			}
			// Move cursor along script varint. We look at byte data in case it is longer than needed.
			cursor += CBTransactionInputCalculateLength(block->transactions[x]->inputs[y]);
		}
		// Move cursor past output number to first output
		byte = CBByteArrayGetByte(CBGetMessage(block)->bytes, cursor);
		cursor += byte < 253 ? 1 : (byte == 253 ? 3 : (byte == 254 ? 5 : 9));
		// Now add new outputs
		uint8_t * txHash = CBTransactionGetHash(block->transactions[x]);
		// For adding the size of outputs
		uint32_t outputsPos = cursor;
		for (uint32_t y = 0; y < block->transactions[x]->outputNum; y++) {
			// Add to storage.
			if (! CBBlockChainStorageSaveUnspentOutput(self, txHash, y, cursor, 
														 CBGetMessage(block->transactions[x]->outputs[y])->bytes->length, false)) {
				CBLogError("Could not write a new unspent output reference.");
				return false;
			}
			// Move cursor past the output
			cursor += CBTransactionOutputCalculateLength(block->transactions[x]->outputs[y]);
		}
		// Add transaction to transaction index
		if (! CBBlockChainStorageSaveTransactionRef(self, txHash, branch, blockIndex, outputsPos, cursor - outputsPos, x == 0, block->transactions[x]->outputNum)) {
			CBLogError("Could not write transaction reference to transaction index.");
			return false;
		}
		// Move along locktime
		cursor += 4;
	}
	return true;
}
bool CBValidatorUpdateMainChain(CBValidator * self, uint8_t branch, uint32_t blockIndex, bool forward){
	// Load the block
	CBBlock * block = CBBlockChainStorageLoadBlock(self, blockIndex, branch);
	if (! block) {
		CBReleaseObject(block);
		CBLogError("Could not deserailise a block for re-organisation.");
		return false;
	}
	// Deserialise block
	if (! CBBlockDeserialise(block, ! (self->flags & CB_VALIDATOR_HEADERS_ONLY))) {
		CBLogError("Could not open a block for re-organisation.");
		return false;
	}
	bool res;
	if (forward)
		res = CBValidatorAddBlockToMainChain(self, block, branch, blockIndex, CB_ADD_BLOCK_PREV, self->callbackQueue[self->queueFront]);
	else
		res = CBValidatorRemoveBlockFromMainChain(self, block, branch, blockIndex);
	if (! res)
		CBLogError("Could not update the unspent outputs and transaction indices.");
	// Free the block
	CBReleaseObject(block);
	return res;
}
bool CBValidatorVerifyScripts(CBValidator * self, CBTransaction * tx, uint32_t inputIndex, CBTransactionOutput * output, uint32_t * sigOps, bool checkStandard){
	CBScript * p2sh = NULL;
	// Vrify scripts, check input for standard and count p2sh sigops.
	CBScriptStack stack = CBNewEmptyScriptStack();
	// Execute the input script.
	CBScriptExecuteReturn scriptRes = CBScriptExecute(tx->inputs[inputIndex]->scriptObject, &stack, CBTransactionGetInputHashForSignature, tx, inputIndex, false);
	// Check is script is invalid, but for input scripts, do not care if false.
	if (scriptRes == CB_SCRIPT_INVALID) {
		CBFreeScriptStack(stack);
		return CB_INPUT_BAD;
	}
	// Verify P2SH inputs.
	if (CBScriptIsP2SH(output->scriptObject)){
		// For P2SH inputs, there must be no script operations except for push operations, and there should be at least one push operation.
		if (CBScriptIsPushOnly(tx->inputs[inputIndex]->scriptObject) == 0){
			CBFreeScriptStack(stack);
			return CB_INPUT_BAD;
		}
		// Since the output is a P2SH we include the serialised script in the signature operations
		p2sh = CBNewScriptWithDataCopy(stack.elements[stack.length - 1].data, stack.elements[stack.length - 1].length);
		*sigOps += CBScriptGetSigOpCount(p2sh, true);
		if (*sigOps > CB_MAX_SIG_OPS){
			CBFreeScriptStack(stack);
			return CB_INPUT_BAD;
		}
	}
	// Check for standard input
	if (checkStandard && CBTransactionInputIsStandard(tx->inputs[inputIndex], output, p2sh)) {
		CBFreeScriptStack(stack);
		return CB_INPUT_NON_STD;
	}
	if (p2sh)
		CBReleaseObject(p2sh);
	// Execute the output script.
	scriptRes = CBScriptExecute(output->scriptObject, &stack, CBTransactionGetInputHashForSignature, tx, inputIndex, true);
	// Finished with the stack.
	CBFreeScriptStack(stack);
	return (scriptRes == CB_SCRIPT_TRUE) ? CB_INPUT_OK : CB_INPUT_BAD;
}
