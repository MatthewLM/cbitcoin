//
//  CBFullValidator.c
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

//  SEE HEADER FILE FOR DOCUMENTATION

#include "CBFullValidator.h"

//  Constructor

CBFullValidator * CBNewFullValidator(uint64_t storage, bool * badDataBase, CBFullValidatorFlags flags){
	CBFullValidator * self = malloc(sizeof(*self));
	if (NOT self) {
		CBLogError("Cannot allocate %i bytes of memory in CBNewFullNode\n", sizeof(*self));
		return NULL;
	}
	CBGetObject(self)->free = CBFreeFullValidator;
	if (CBInitFullValidator(self, storage, badDataBase, flags))
		return self;
	// If initialisation failed, free the data that exists.
	CBFreeFullValidator(self);
	return NULL;
}

//  Object Getter

CBFullValidator * CBGetFullValidator(void * self){
	return self;
}

//  Initialiser

bool CBInitFullValidator(CBFullValidator * self, uint64_t storage, bool * badDataBase, CBFullValidatorFlags flags){
	if (NOT CBInitObject(CBGetObject(self)))
		return false;
	*badDataBase = false;
	self->storage = storage;
	self->flags = flags;
	// Check whether the database has been created.
	if (CBBlockChainStorageExists(self->storage)) {
		// Found now load information from storage
		// Basic validator information
		if (NOT CBBlockChainStorageLoadBasicValidator(self)){
			CBLogError("There was an error when loading the validator information.");
			return false;
		}
		// Loop through the orphans
		for (uint8_t x = 0; x < self->numOrphans; x++) {
			if (NOT CBBlockChainStorageLoadOrphan(self, x)) {
				CBLogError("Could not load orphan %u.", x);
				return false;
			}
		}
		// Loop through the branches
		for (uint8_t x = 0; x < self->numBranches; x++) {
			if (NOT CBBlockChainStorageLoadBranch(self, x)) {
				CBLogError("Could not load branch %u.", x);
				return false;
			}
			if (NOT CBBlockChainStorageLoadBranchWork(self, x)) {
				CBLogError("Could not load work for branch %u.", x);
				return false;
			}
		}
	}else{
		// Write the genesis block
		CBBlock * genesis = CBNewBlockGenesis();
		if (NOT CBBlockChainStorageSaveBlock(self, genesis, 0, 0)) {
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
		if (NOT CBBlockChainStorageSaveBasicValidator(self)) {
			CBLogError("Could not save the initial basic validation information.");
			return false;
		}
		// Create initial branch information
		self->branches[0].lastRetargetTime = 1231006505;
		self->branches[0].startHeight = 0;
		self->branches[0].numBlocks = 1;
		self->branches[0].lastValidation = 0;
		// Write branch information
		if (NOT CBBlockChainStorageSaveBranch(self, 0)) {
			CBLogError("Could not save the initial branch information.");
			return false;
		}
		// Create work CBBigInt
		self->branches[0].work.length = 1;
		self->branches[0].work.data = malloc(1);
		if (NOT self->branches[0].work.data) {
			CBLogError("Could not allocate 1 byte of memory for the initial branch work.");
			return false;
		}
		self->branches[0].work.data[0] = 0;
		// Write branch work
		if (NOT CBBlockChainStorageSaveBranchWork(self, 0)) {
			CBLogError("Could not save the initial work.");
			return false;
		}
		// Now try to commit the data
		if (NOT CBBlockChainStorageCommitData(storage)){
			CBLogError("Could not commit initial block-chain data.");
			*badDataBase = true;
			return false;
		}
	}
	return true;
}

//  Destructor

void CBFreeFullValidator(void * vself){
	CBFullValidator * self = vself;
	// Release orphans
	for (uint8_t x = 0; x < self->numOrphans; x++)
		CBReleaseObject(self->orphans[x]);
	// Release branches
	for (uint8_t x = 0; x < self->numBranches; x++)
		free(self->branches[x].work.data);
	CBFreeObject(self);
}

//  Functions

bool CBFullValidatorAddBlockToBranch(CBFullValidator * self, uint8_t branch, CBBlock * block, CBBigInt work){
	// Store the block
	if (NOT CBBlockChainStorageSaveBlock(self, block, branch, self->branches[branch].numBlocks)) {
		CBLogError("Could not save a block");
		return false;
	}
	// Increase number of blocks.
	self->branches[branch].numBlocks++; 
	// Modify branch information
	free(self->branches[branch].work.data);
	self->branches[branch].work = work;
	if (NOT (self->branches[branch].startHeight + self->branches[branch].numBlocks) % 2016)
		self->branches[branch].lastRetargetTime = block->time;
	// Save branch information
	if (NOT CBBlockChainStorageSaveBranch(self, branch)) {
		CBLogError("Could not write the branch information for branch %u.", branch);
		return false;
	}
	// Update the branch work in storage
	if (NOT CBBlockChainStorageSaveBranchWork(self, branch)) {
		CBLogError("Could not write the branch work for branch %u.", branch);
		return false;
	}
	return true;
}
bool CBFullValidatorAddBlockToOrphans(CBFullValidator * self, CBBlock * block){
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
	if (NOT CBBlockChainStorageSaveBasicValidator(self)) {
		CBLogError("There was an error when saving the basic validation information when adding an orhpan block.");
		return false;
	}
	// Add to memory
	self->orphans[pos] = block;
	CBRetainObject(block);
	// Add to storage
	if (NOT CBBlockChainStorageSaveOrphan(self, block, pos)) {
		CBLogError("Could not save an orphan.");
		return false;
	}
	// Commit data
	if (NOT CBBlockChainStorageCommitData(self->storage)) {
		CBLogError("There was commiting data for an orphan.");
		return false;
	}
	return true;
}
CBBlockStatus CBFullValidatorBasicBlockValidation(CBFullValidator * self, CBBlock * block, uint64_t networkTime){
	// Get the block hash
	uint8_t * hash = CBBlockGetHash(block);
	// Check if duplicate.
	for (uint8_t x = 0; x < self->numOrphans; x++)
		if (NOT memcmp(CBBlockGetHash(self->orphans[x]), hash, 32))
			return CB_BLOCK_STATUS_DUPLICATE;
	// Look in block hash index
	if (CBBlockChainStorageBlockExists(self, hash))
		return CB_BLOCK_STATUS_DUPLICATE;
	// Check block has transactions
	if (NOT block->transactionNum)
		return CB_BLOCK_STATUS_BAD;
	// Check block hash against target and that it is below the maximum allowed target.
	if (NOT (self->flags & CB_FULL_VALIDATOR_DISABLE_POW_CHECK)
		&& NOT CBValidateProofOfWork(hash, block->target))
		return CB_BLOCK_STATUS_BAD;
	// Check the block is within two hours of the network time.
	if (block->time > networkTime + CB_BLOCK_ALLOWED_TIME_DRIFT)
		return CB_BLOCK_STATUS_BAD_TIME;
	// Calculate merkle root.
	uint8_t * txHashes = CBBlockCalculateMerkleRoot(block);
	if (NOT txHashes)
		return CB_BLOCK_STATUS_ERROR;
	// Check merkle root
	int res = memcmp(txHashes, CBByteArrayGetData(block->merkleRoot), 32);
	if (res)
		return CB_BLOCK_STATUS_BAD;
	return CB_BLOCK_STATUS_CONTINUE;
}
CBBlockValidationResult CBFullValidatorCompleteBlockValidation(CBFullValidator * self, uint8_t branch, CBBlock * block, uint32_t height){
	// Check that the first transaction is a coinbase transaction.
	if (NOT CBTransactionIsCoinBase(block->transactions[0]))
		return CB_BLOCK_VALIDATION_BAD;
	uint64_t blockReward = CBCalculateBlockReward(height);
	uint64_t coinbaseOutputValue;
	uint32_t sigOps = 0;
	// Do validation for transactions.
	for (uint32_t x = 0; x < block->transactionNum; x++) {
		// Check for duplicate transactions which have unspent outputs, except for two blocks (See BIP30 https://en.bitcoin.it/wiki/BIP_0030 and https://github.com/bitcoin/bitcoin/blob/master/src/main.cpp#L1568)
		if (memcmp(CBBlockGetHash(block), (uint8_t []){0xec, 0xca, 0xe0, 0x00, 0xe3, 0xc8, 0xe4, 0xe0, 0x93, 0x93, 0x63, 0x60, 0x43, 0x1f, 0x3b, 0x76, 0x03, 0xc5, 0x63, 0xc1, 0xff, 0x61, 0x81, 0x39, 0x0a, 0x4d, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00}, 32)
			&& memcmp(CBBlockGetHash(block), (uint8_t []){0x21, 0xd7, 0x7c, 0xcb, 0x4c, 0x08, 0x38, 0x6a, 0x04, 0xac, 0x01, 0x96, 0xae, 0x10, 0xf6, 0xa1, 0xd2, 0xc2, 0xa3, 0x77, 0x55, 0x8c, 0xa1, 0x90, 0xf1, 0x43, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00}, 32)) {
			// Now check for duplicate in previous blocks.
			bool exists;
			if (NOT CBBlockChainStorageIsTransactionWithUnspentOutputs(self, CBTransactionGetHash(block->transactions[x]), &exists)) {
				CBLogError("Could not detemine if a transaction exists with unspent outputs.");
				return CB_BLOCK_VALIDATION_ERR;
			}
			if (exists)
				return CB_BLOCK_VALIDATION_BAD;
		}
		// Check that the transaction is final.
		if (NOT CBTransactionIsFinal(block->transactions[x], block->time, height))
			return CB_BLOCK_VALIDATION_BAD;
		// Do the basic validation
		uint64_t outputValue;
		if (NOT CBTransactionValidateBasic(block->transactions[x], NOT x, &outputValue))
			return CB_BLOCK_VALIDATION_BAD;
		// Count and verify sigops
		sigOps += CBTransactionGetSigOps(block->transactions[x]);
		if (sigOps > CB_MAX_SIG_OPS)
			return CB_BLOCK_VALIDATION_BAD;
		if (NOT x)
			// This is the coinbase, take the output as the coinbase output.
			coinbaseOutputValue = outputValue;
		else {
			uint64_t inputValue = 0;
			// Verify each input and count input values
			for (uint32_t y = 0; y < block->transactions[x]->inputNum; y++) {
				CBBlockValidationResult res = CBFullValidatorInputValidation(self, branch, block, height, x, y, &inputValue, &sigOps);
				if (res != CB_BLOCK_VALIDATION_OK)
					return res;
			}
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
uint32_t CBFullValidatorGetMedianTime(CBFullValidator * self, uint8_t branch, uint32_t prevIndex){
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
CBBlockValidationResult CBFullValidatorInputValidation(CBFullValidator * self, uint8_t branch, CBBlock * block, uint32_t blockHeight, uint32_t transactionIndex, uint32_t inputIndex, uint64_t * value, uint32_t * sigOps){
	// Create variable for the previous output reference.
	CBPrevOut prevOutRef = block->transactions[transactionIndex]->inputs[inputIndex]->prevOut;
	// Check that the previous output is not already spent by this block.
	for (uint32_t a = 0; a < transactionIndex; a++)
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
		if (NOT memcmp(CBTransactionGetHash(block->transactions[a]), CBByteArrayGetData(prevOutRef.hash), 32)) {
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
	if (NOT found) {
		// Not found in this block. Look in database for the unspent output.
		if (NOT CBBlockChainStorageUnspentOutputExists(self, CBByteArrayGetData(prevOutRef.hash), prevOutRef.index))
			return CB_BLOCK_VALIDATION_BAD;
		// Exists so therefore load the unspent output
		bool coinbase;
		uint32_t outputHeight;
		prevOut = CBBlockChainStorageLoadUnspentOutput(self, CBByteArrayGetData(prevOutRef.hash), prevOutRef.index, &coinbase, &outputHeight);
		if (NOT prevOut) {
			CBLogError("Could not load an unspent output.");
			return CB_BLOCK_VALIDATION_ERR;
		}
		// Check coinbase maturity
		if (coinbase && blockHeight - outputHeight < CB_COINBASE_MATURITY)
			return CB_BLOCK_VALIDATION_BAD;
	}
	// We have sucessfully received an output for this input. Verify the input script for the output script.
	CBScriptStack stack = CBNewEmptyScriptStack();
	// Execute the input script.
	CBScriptExecuteReturn res = CBScriptExecute(block->transactions[transactionIndex]->inputs[inputIndex]->scriptObject, &stack, CBTransactionGetInputHashForSignature, block->transactions[transactionIndex], inputIndex, false);
	if (res == CB_SCRIPT_ERR){
		CBFreeScriptStack(stack);
		CBReleaseObject(prevOut);
		return CB_BLOCK_VALIDATION_ERR;
	}
	// Check is script is invalid, but for input scripts, do not care if false.
	if (res == CB_SCRIPT_INVALID){
		CBFreeScriptStack(stack);
		CBReleaseObject(prevOut);
		return CB_BLOCK_VALIDATION_BAD;
	}
	// Verify P2SH inputs.
	if (CBScriptIsP2SH(prevOut->scriptObject)){
		// For P2SH inputs, there must be no script operations except for push operations.
		if (NOT CBScriptIsPushOnly(block->transactions[transactionIndex]->inputs[inputIndex]->scriptObject)
			// We must have data in the stack.
			|| NOT stack.length){
			CBFreeScriptStack(stack);
			CBReleaseObject(prevOut);
			return CB_BLOCK_VALIDATION_BAD;
		}
		// Since the output is a P2SH we include the serialised script in the signature operations
		CBScript * p2shScript = CBNewScriptWithDataCopy(stack.elements[stack.length - 1].data, stack.elements[stack.length - 1].length);
		if (NOT p2shScript) {
			CBLogError("Could not create a P2SH script for counting sig ops.");
			return CB_BLOCK_VALIDATION_ERR;
		}
		*sigOps += CBScriptGetSigOpCount(p2shScript, true);
		if (*sigOps > CB_MAX_SIG_OPS){
			CBFreeScriptStack(stack);
			CBReleaseObject(prevOut);
			return CB_BLOCK_VALIDATION_BAD;
		}
		CBReleaseObject(p2shScript);
	}
	// Execute the output script.
	res = CBScriptExecute(prevOut->scriptObject, &stack, CBTransactionGetInputHashForSignature, block->transactions[transactionIndex], inputIndex, true);
	// Finished with the stack.
	CBFreeScriptStack(stack);
	// Increment the value with the input value then be done with the output
	*value += prevOut->value;
	CBReleaseObject(prevOut);
	// Check the result of the output script
	if (res == CB_SCRIPT_ERR)
		return CB_BLOCK_VALIDATION_ERR;
	if (res == CB_SCRIPT_INVALID
		|| res == CB_SCRIPT_FALSE)
		return CB_BLOCK_VALIDATION_BAD;
	return CB_BLOCK_VALIDATION_OK;
}
CBBlockStatus CBFullValidatorProcessBlock(CBFullValidator * self, CBBlock * block, uint64_t networkTime){
	uint8_t prevBranch;
	uint32_t prevBlockIndex;
	uint32_t prevBlockTarget;
	// Determine what type of block this is.
	if (CBBlockChainStorageBlockExists(self, CBByteArrayGetData(block->prevBlockHash))) {
		// Has a block in the block hash index. Get branch and index.
		if (NOT CBBlockChainStorageGetBlockLocation(self, CBByteArrayGetData(block->prevBlockHash), &prevBranch, &prevBlockIndex)) {
			CBLogError("Could not get the location of a previous block.");
			return CB_BLOCK_STATUS_ERROR;
		}
		// Get previous block target
		prevBlockTarget = CBBlockChainStorageGetBlockTarget(self, prevBranch, prevBlockIndex);
		if (NOT prevBlockTarget) {
			CBLogError("Could not get the target of a previous block.");
			return CB_BLOCK_STATUS_ERROR;
		}
	}else{
		// Orphan block. End here.
		// Do basic validation
		CBBlockStatus res = CBFullValidatorBasicBlockValidation(self, block, networkTime);
		if (res != CB_BLOCK_STATUS_CONTINUE)
			return res;
		// Add block to orphans
		if(CBFullValidatorAddBlockToOrphans(self, block))
			return CB_BLOCK_STATUS_ORPHAN;
		return CB_BLOCK_STATUS_ERROR; // Needs recovery.
	}
	// Do basic validation with a  of the transaction hashes.
	CBBlockStatus status = CBFullValidatorBasicBlockValidation(self, block, networkTime);
	if (status != CB_BLOCK_STATUS_CONTINUE)
		return status;
	// Not an orphan. See if this is an extention or new branch.
	uint8_t branch;
	if (prevBlockIndex == self->branches[prevBranch].numBlocks - 1)
		// Extension
		branch = prevBranch;
	else{
		// New branch
		if (self->numBranches == CB_MAX_BRANCH_CACHE) {
			// ??? SINCE BRANCHES ARE TRIMMED OR DELETED THERE ARE SECURITY IMPLICATIONS: A peer could prevent other branches having a chance by creating many branches when the node is not up-to-date. To protect against this potential attack peers should only be allowed to provide one branch at a time and before accepting new branches from peers there should be a completed branch which is not the main branch.
			// Trim branch from longest ago which is not being worked on and isn't main branch
			uint8_t earliestBranch = 0;
			uint32_t earliestIndex = self->branches[0].parentBlockIndex + self->branches[0].numBlocks;
			for (uint8_t x = 1; x < self->numBranches; x++) {
				if (NOT self->branches[x].working
					&& self->branches[x].parentBlockIndex + self->branches[x].numBlocks < earliestIndex) {
					earliestIndex = self->branches[x].parentBlockIndex + self->branches[x].numBlocks;
					earliestBranch = x;
				}
			}
			if (self->branches[earliestBranch].working || earliestBranch == self->mainBranch)
				// Cannot overwrite branch
				return CB_BLOCK_STATUS_NO_NEW;
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
					if (NOT CBBlockChainStorageDeleteBlock(self, earliestBranch, x)) {
						CBLogError("Could not delete a block when removing a branch.");
						return CB_BLOCK_STATUS_ERROR;
					}
				}
				// Change block keys from earliest branch to dependent branch
				for (uint8_t x = 0; x < highestDependency; x++) {
					if (NOT CBBlockChainStorageMoveBlock(self, earliestBranch, x, mergeBranch, x)) {
						CBLogError("Could not move a block for merging two branches.");
						return CB_BLOCK_STATUS_ERROR;
					}
				}
				// Make parent information for dependent branch reflect the earliest branch
				self->branches[mergeBranch].numBlocks += self->branches[earliestBranch].numBlocks;
				self->branches[mergeBranch].parentBlockIndex = self->branches[earliestBranch].parentBlockIndex;
				self->branches[mergeBranch].parentBranch = self->branches[earliestBranch].parentBranch;
				self->branches[mergeBranch].startHeight = self->branches[earliestBranch].startHeight;
				// Write updated branch information to storage
				if (NOT CBBlockChainStorageSaveBranch(self, mergeBranch)) {
					CBLogError("Could not save the updated branch information for merging two branches together.");
					return CB_BLOCK_STATUS_ERROR;
				}
				// Find all of the other dependent branches for the earliest branch and update the parent branch information.
				for (uint8_t x = 0; x < self->numBranches; x++) {
					if (x != mergeBranch && self->branches[x].parentBranch == earliestBranch) {
						self->branches[x].parentBranch = mergeBranch;
						// Update in storage
						if (NOT CBBlockChainStorageSaveBranch(self, x)) {
							CBLogError("Could not write the updated parent branch from the overwritten branch during a merge.");
							return CB_BLOCK_STATUS_ERROR;
						}
					}
				}
				// Find all of the dependent branches for the dependent branch and update the parent block index.
				for (uint8_t x = 0; x < self->numBranches; x++) {
					if (self->branches[x].parentBranch == mergeBranch) {
						self->branches[x].parentBlockIndex += self->branches[earliestBranch].numBlocks;
						// Update in storage
						if (NOT CBBlockChainStorageSaveBranch(self, x)) {
							CBLogError("Could not write an updated parent block index during a merge.");
							return CB_BLOCK_STATUS_ERROR;
						}
					}
				}
			}
			// Overwrite earliest branch
			branch = earliestBranch;
			// Delete work
			free(self->branches[branch].work.data);
			// Do not delete from storage as it will get overwritten immediately anyway.
			// Do not commit here either as the branch needs to be overwritten in order for the database to be consistent.
		}else{
			// No overwritting
			branch = self->numBranches++;
			// Update number of branches into storage
			if (NOT CBBlockChainStorageSaveBasicValidator(self)) {
				CBLogError("Could not write the new number of branches.");
				return CB_BLOCK_STATUS_ERROR;
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
			if (NOT CBCalculateBlockWork(&tempWork, prevBlockTarget))
				return CB_BLOCK_STATUS_ERROR;
			CBBigIntEqualsSubtractionByBigInt(&self->branches[branch].work, &tempWork);
			free(tempWork.data); 
		}
		// Set the remaining data
		self->branches[branch].working = false;
		self->branches[branch].numBlocks = 0;
		self->branches[branch].lastValidation = CB_NO_VALIDATION;
		self->branches[branch].startHeight = self->branches[prevBranch].startHeight + prevBlockIndex + 1;
		// Write branch info
		if (NOT CBBlockChainStorageSaveBranch(self, branch)) {
			CBLogError("Could not save new branch data.");
			return CB_BLOCK_STATUS_ERROR;
		}
		// Write branch work
		if (NOT CBBlockChainStorageSaveBranchWork(self, branch)) {
			CBLogError("Could not save the new branch's work.");
			return CB_BLOCK_STATUS_ERROR;
		}
	}
	// Got branch ready for block. Now process into the branch.
	status = CBFullValidatorProcessIntoBranch(self, block, networkTime, branch, prevBranch, prevBlockIndex, prevBlockTarget);
	if (status != CB_BLOCK_STATUS_MAIN
		&& status != CB_BLOCK_STATUS_SIDE)
		return status;
	// Now go through any orphans
	bool first = true;
	for (uint8_t x = 0; x < self->numOrphans;){
		if (NOT memcmp(CBBlockGetHash(block), CBByteArrayGetData(self->orphans[x]->prevBlockHash), 32)) {
			if (first)
				// Do not release the block passed to this function
				first = false;
			else
				// Release orphan blocks.
				CBReleaseObject(block);
			// Moving onto this block.
			// Get previous block target for orphan.
			uint32_t target = CBBlockChainStorageGetBlockTarget(self, branch, self->branches[branch].numBlocks - 1);
			if (NOT target) {
				CBLogError("Could not get a block target for an orphan.");
				return CB_BLOCK_STATUS_ERROR;
			}
			// Process into the branch.
			CBBlockStatus orphanStatus = CBFullValidatorProcessIntoBranch(self, self->orphans[x], networkTime, branch, branch, self->branches[branch].numBlocks - 1, target);
			if (orphanStatus == CB_BLOCK_STATUS_ERROR) {
				CBLogError("There was an error when processing an orphan into a branch.");
				return CB_BLOCK_STATUS_ERROR;
			}
			// Remove orphan now we are done.
			self->numOrphans--;
			if (NOT self->numOrphans){
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
	// Finally commit all data
	if (NOT CBBlockChainStorageCommitData(self->storage)) {
		CBLogError("Could not commit updated data when adding a new block to the main chain.");
		return CB_BLOCK_STATUS_ERROR;
	}
	return status;
}
CBBlockStatus CBFullValidatorProcessIntoBranch(CBFullValidator * self, CBBlock * block, uint64_t networkTime, uint8_t branch, uint8_t prevBranch, uint32_t prevBlockIndex, uint32_t prevBlockTarget){
	// Check timestamp
	if (block->time <= CBFullValidatorGetMedianTime(self, prevBranch, prevBlockIndex)){
		CBBlockChainStorageReset(self->storage);
		return CB_BLOCK_STATUS_BAD;
	}
	uint32_t target;
	bool change = NOT ((self->branches[prevBranch].startHeight + prevBlockIndex + 1) % 2016);
	if (change)
		// Difficulty change for this branch
		target = CBCalculateTarget(prevBlockTarget, block->time - self->branches[branch].lastRetargetTime);
	else
		target = prevBlockTarget;
	// Check target
	if (block->target != target){
		CBBlockChainStorageReset(self->storage);
		return CB_BLOCK_STATUS_BAD;
	}
	// Calculate total work
	CBBigInt work;
	if (NOT CBCalculateBlockWork(&work, block->target))
		return CB_BLOCK_STATUS_ERROR;
	if (NOT CBBigIntEqualsAdditionByBigInt(&work, &self->branches[branch].work))
		return CB_BLOCK_STATUS_ERROR;
	if (branch != self->mainBranch) {
		// Check if the block is adding to a side branch without becoming the main branch
		if (CBBigIntCompareToBigInt(&work, &self->branches[self->mainBranch].work) != CB_COMPARE_MORE_THAN){
			// Add to branch without complete validation
			if (NOT CBFullValidatorAddBlockToBranch(self, branch, block, work))
				return CB_BLOCK_STATUS_ERROR;
			return CB_BLOCK_STATUS_SIDE;
		}
		// Potential block-chain reorganisation. Validate the side branch. THIS NEEDS TO START AT THE FIRST BRANCH BACK WHERE VALIDATION IS NOT COMPLETE AND GO UP THROUGH THE BLOCKS VALIDATING THEM. Includes Prior Branches!
		uint8_t tempBranch = prevBranch;
		CBChainPath chainPathStack[CB_MAX_BRANCH_CACHE]; // Used to store the last blocks in each branch in the potential new path upto the newest branch in the path.
		uint8_t pathIndex = 0; // The starting index of the path stack. If 0 then we are only validating the branch we added to.
		// Modify unspent output and transaction indices to start validation.
		// Make the first last block, the last block in the branch we are validating up-to.
		chainPathStack[pathIndex].lastBlock = self->branches[prevBranch].numBlocks - 1;
		// Get all last blocks and next branches for path information
		while (tempBranch) { // While not the genesis branch
			// Add path information
			chainPathStack[pathIndex].branch = tempBranch;
			chainPathStack[++pathIndex].lastBlock = self->branches[tempBranch].parentBlockIndex;
			tempBranch = self->branches[tempBranch].parentBranch;
		}
		// Add the genesis branch
		chainPathStack[pathIndex].branch = 0;
		// Get unspent output index for validating the potential new main chain
		// Start at the main branch and go backwards until we reach the path of the potential new chain, and thus the fork point.
		tempBranch = self->mainBranch;
		// Start at the latest block in the main chain
		uint32_t tempBlockIndex = self->branches[self->mainBranch].numBlocks - 1;
		// Go backwards towards fork
		for (;;) {
			// See if the main-chain branch we are currently at, is in the chain path for the potetial new chain.
			bool done = false;
			for (uint8_t x = pathIndex + 1; x--;) {
				if (chainPathStack[x].branch == tempBranch) {
					// The branch for the main-chain path is a branch in the potential new main chain branch path.
					// Go down to fork point, unless this is the fork point
					if (tempBlockIndex > chainPathStack[x].lastBlock) {
						// Above fork point, go down to fork point
						for (;; tempBlockIndex--) {
							// Change unspent output and transaction information, as going backwards through the main-chain.
							if (NOT CBFullValidatorUpdateUnspentOutputsAndLoad(self, tempBranch, tempBlockIndex, false)){
								CBLogError("Could not reverse indicies for unspent outputs and transactions during reorganisation.");
								return CB_BLOCK_STATUS_ERROR;
							}
							// Break before tempBlockIndex becomes the last block. We only want to remove everything over the last block in the potential new chain path branch. And we want tempBlockIndex to equal the block above the last block.
							if (tempBlockIndex == chainPathStack[x].lastBlock + 1)
								break;
						}
					}
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
				if (NOT CBFullValidatorUpdateUnspentOutputsAndLoad(self, tempBranch, tempBlockIndex, false)){
					CBLogError("Could not reverse indicies for unspent outputs and transactions during reorganisation.");
					return CB_BLOCK_STATUS_ERROR;
				}
			}
			// Go down a branch
			tempBlockIndex = self->branches[tempBranch].parentBlockIndex;
			tempBranch = self->branches[tempBranch].parentBranch;
		}
		// Go upwards to last validation point in new chain path. Then we need to start validating for the potential new chain.
		for (;; pathIndex--) {
			// See if the last block in the path branch is higher or equal to the last validation point. In this case we go to the validation point and end there.
			if (chainPathStack[pathIndex].lastBlock >= self->branches[chainPathStack[pathIndex].branch].lastValidation) {
				// Go up to last validation point, updating unspent output and transaction information.
				for (;tempBlockIndex <= self->branches[chainPathStack[pathIndex].branch].lastValidation; tempBlockIndex++) {
					if (NOT CBFullValidatorUpdateUnspentOutputsAndLoad(self, chainPathStack[pathIndex].branch, tempBlockIndex, true)){
						CBLogError("Could not update indicies for going to a previously validated point during reorganisation.");
						return CB_BLOCK_STATUS_ERROR;
					}
				}
				break;
			}else if (self->branches[chainPathStack[pathIndex].branch].lastValidation != CB_NO_VALIDATION){
				// The branch is validated enough to reach the last block. Go up to last block, updating unspent output and transaction information.
				for (;tempBlockIndex <= chainPathStack[pathIndex].lastBlock; tempBlockIndex++) {
					if (NOT CBFullValidatorUpdateUnspentOutputsAndLoad(self, chainPathStack[pathIndex].branch, tempBlockIndex, true)){
						CBLogError("Could not update indicies for going to the last block for a path during reorganisation.");
						return CB_BLOCK_STATUS_ERROR;
					}
				}
				// If the pathIndex is to the last branch, we can end here.
				if (NOT pathIndex)
					// Done
					break;
				// Reset block index for next branch
				tempBlockIndex = 0;
			}else
				// This is the block we should start at, as no validation has been done on this branch.
				break;
		}
		// At this point tempBlockIndex is the block we start at and chainPathStack[pathIndex].branch is the branch.
		// Now validate all blocks going up.
		bool atLeastOne = false; // True when at least one block passed validation for a branch
		tempBranch = chainPathStack[pathIndex].branch;
		// Saving the first branch for updating last validated blocks.
		uint8_t branches = 1 << tempBranch;
		// Loop through blocks unless we have reached the newest block already.
		if (tempBlockIndex < self->branches[branch].numBlocks || tempBranch != branch){
			for (;;) {
				// Get block to validate
				CBBlock * tempBlock = CBBlockChainStorageLoadBlock(self, tempBlockIndex, tempBranch);
				if (NOT tempBlock)
					return CB_BLOCK_STATUS_ERROR;
				if (NOT CBBlockDeserialise(tempBlock, true))
					return CB_BLOCK_STATUS_ERROR;
				// Validate block
				CBBlockValidationResult res = CBFullValidatorCompleteBlockValidation(self, tempBranch, tempBlock, self->branches[tempBranch].startHeight + tempBlockIndex);
				if (res == CB_BLOCK_VALIDATION_BAD
					|| res == CB_BLOCK_VALIDATION_ERR){
					CBReleaseObject(tempBlock);
					// Clear IO operations, thus reverting to previous main-chain.
					CBBlockChainStorageReset(self->storage);
					if (res == CB_BLOCK_VALIDATION_BAD) {
						// Save last valided blocks for each branch
						if (NOT CBFullValidatorSaveLastValidatedBlocks(self, branches)) {
							CBLogError("Could not save the last validated blocks for a validatation error during reorganisation");
							return CB_BLOCK_STATUS_ERROR;
						}
						if (NOT CBBlockChainStorageCommitData(self->storage)) {
							CBLogError("Could not commit the last validated block for a validatation error during reorganisation.");
							return CB_BLOCK_STATUS_ERROR;
						}
					}
					if (res == CB_BLOCK_VALIDATION_BAD)
						return CB_BLOCK_STATUS_BAD;
					if (res == CB_BLOCK_VALIDATION_ERR)
						return CB_BLOCK_STATUS_ERROR;
				}
				// Update the validation information, if this block is the latest validated.
				if (tempBlockIndex > self->branches[tempBranch].lastValidation
					|| self->branches[tempBranch].lastValidation == CB_NO_VALIDATION){
					self->branches[tempBranch].lastValidation = tempBlockIndex;
					if (NOT atLeastOne){
						// Add this branch to be updated
						branches |= 1 << tempBranch;
						atLeastOne = true;
					}
				}
				// This block passed validation. Move onto next block.
				if (tempBlockIndex == chainPathStack[pathIndex].lastBlock) {
					// We came to the last block in the branch for the potential new chain.
					// Check if last block
					if (tempBlockIndex == self->branches[branch].numBlocks - 1 && tempBranch == branch)
						break;
					// Go to next branch
					tempBranch = chainPathStack[--pathIndex].branch;
					tempBlockIndex = 0;
					atLeastOne = false;
				}else
					// We have not reached the end of this branch for the chain yet. Thus move to the next block in the branch.
					tempBlockIndex++;
				// We are done with the block.
				CBReleaseObject(tempBlock);
			}
			// Save last validated blocks
			if (NOT CBFullValidatorSaveLastValidatedBlocks(self, branches)) {
				CBLogError("Could not save the last validated blocks during reorganisation");
				return CB_BLOCK_STATUS_ERROR;
			}
		}
		// We are done with the reorganisation. Now we validate the block for the new main chain.
	}
	// Validate a new block for the main chain.
	CBBlockValidationResult res = CBFullValidatorCompleteBlockValidation(self, branch, block, self->branches[branch].startHeight + self->branches[branch].numBlocks);
	// If the validation was bad then reset the pending IO and return.
	if (res == CB_BLOCK_VALIDATION_BAD) {
		CBBlockChainStorageReset(self->storage);
		return CB_BLOCK_STATUS_BAD;
	}
	if (res == CB_BLOCK_VALIDATION_ERR)
		return CB_BLOCK_STATUS_ERROR;
	// Everything is OK so update branch and unspent outputs.
	if (NOT CBFullValidatorAddBlockToBranch(self, branch, block, work)){
		CBLogError("There was an error when adding a new block to the branch.");
		return CB_BLOCK_STATUS_ERROR;
	}
	self->branches[branch].lastValidation = self->branches[branch].numBlocks - 1;
	// Update storage
	if (NOT CBBlockChainStorageSaveBranch(self, branch)) {
		CBLogError("Could not save the last validated block for a branch when adding a new block to the main chain.");
		return CB_BLOCK_STATUS_ERROR;
	}
	if (branch != self->mainBranch) {
		// Update main branch
		self->mainBranch = branch;
		if (NOT CBBlockChainStorageSaveBasicValidator(self)) {
			CBLogError("Could not save the new main branch when adding a new block to the main chain.");
			return CB_BLOCK_STATUS_ERROR;
		}
	}
	// Update the unspent output indicies.
	if (NOT CBFullValidatorUpdateUnspentOutputsForward(self, block, branch, self->branches[branch].lastValidation)) {
		CBLogError("Could not update the unspent outputs when adding a block to the main chain.");
		return CB_BLOCK_STATUS_ERROR;
	}
	return CB_BLOCK_STATUS_MAIN;
}
bool CBFullValidatorSaveLastValidatedBlocks(CBFullValidator * self, uint8_t branches){
	for (uint8_t x = 0; x < 5; x++) {
		if (branches & (1 << x)
			&& NOT CBBlockChainStorageSaveBranch(self, x)) {
			CBLogError("Could not save the last validated block for a branch.");
			return false;
		}
	}
	return true;
}
bool CBFullValidatorUpdateUnspentOutputsBackward(CBFullValidator * self, CBBlock * block, uint8_t branch, uint32_t blockIndex){
	// Update unspent outputs... Go through transactions, adding the prevOut references and removing the outputs for one transaction at a time.
	uint8_t * txReadData = NULL;
	uint32_t txReadDataSize = 0;
	for (uint32_t x = block->transactionNum; x--;) {
		// Remove transaction from transaction index.
		uint8_t * txHash = CBTransactionGetHash(block->transactions[x]);
		if (NOT CBBlockChainStorageDeleteTransactionRef(self, txHash)) {
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
				if (NOT CBBlockChainStorageLoadOutputs(self, prevTxHash, &txReadData, &txReadDataSize, &outputsPos)) {
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
				if (NOT CBBlockChainStorageSaveUnspentOutput(self, prevTxHash, outputIndex, outputsPos, outputLen, true)) {
					CBLogError("Could not save an unspent output reference when going backwards during re-organisation.");
					free(txReadData);
					return false;
				}
			}
		}
		// Loop through outputs
		for (uint32_t y = block->transactions[x]->outputNum; y--;) {
			// Remove output from storage
			if (NOT CBBlockChainStorageDeleteUnspentOutput(self, txHash, y, false)) {
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
bool CBFullValidatorUpdateUnspentOutputsForward(CBFullValidator * self, CBBlock * block, uint8_t branch, uint32_t blockIndex){
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
				if (NOT CBBlockChainStorageDeleteUnspentOutput(self, txHash, outputIndex, true)) {
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
			if (NOT CBBlockChainStorageSaveUnspentOutput(self, txHash, y, cursor, 
														 CBGetMessage(block->transactions[x]->outputs[y])->bytes->length, false)) {
				CBLogError("Could not write a new unspent output reference.");
				return false;
			}
			// Move cursor past the output
			cursor += CBTransactionOutputCalculateLength(block->transactions[x]->outputs[y]);
		}
		// Add transaction to transaction index
		if (NOT CBBlockChainStorageSaveTransactionRef(self, txHash, branch, blockIndex, outputsPos, cursor - outputsPos, 
													  x == 0, block->transactions[x]->outputNum)) {
			CBLogError("Could not write transaction reference to transaction index.");
			return false;
		}
		// Move along locktime
		cursor += 4;
	}
	return true;
}
bool CBFullValidatorUpdateUnspentOutputsAndLoad(CBFullValidator * self, uint8_t branch, uint32_t blockIndex, bool forward){
	// Load the block
	CBBlock * block = CBBlockChainStorageLoadBlock(self, blockIndex, branch);
	if (NOT block) {
		CBReleaseObject(block);
		CBLogError("Could not deserailise a block for re-organisation.");
		return false;
	}
	// Deserailise block
	if (NOT CBBlockDeserialise(block, true)) {
		CBLogError("Could not open a block for re-organisation.");
		return false;
	}
	// Update indices going backwards
	bool res;
	if (forward)
		res = CBFullValidatorUpdateUnspentOutputsForward(self, block, branch, blockIndex);
	else
		res = CBFullValidatorUpdateUnspentOutputsBackward(self, block, branch, blockIndex);
	if (NOT res)
		CBLogError("Could not update the unspent outputs and transaction indices.");
	// Free the block
	CBReleaseObject(block);
	return res;
}
