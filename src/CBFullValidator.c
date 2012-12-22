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

CBFullValidator * CBNewFullValidator(uint64_t storage, bool * badDataBase, void (*logError)(char *,...)){
	CBFullValidator * self = malloc(sizeof(*self));
	if (NOT self) {
		logError("Cannot allocate %i bytes of memory in CBNewFullNode\n", sizeof(*self));
		return NULL;
	}
	CBGetObject(self)->free = CBFreeFullValidator;
	if (CBInitFullValidator(self, storage, badDataBase, logError))
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

bool CBInitFullValidator(CBFullValidator * self, uint64_t storage, bool * badDataBase, void (*logError)(char *,...)){
	if (NOT CBInitObject(CBGetObject(self)))
		return false;
	*badDataBase = false;
	self->logError = logError;
	self->storage = storage;
	// Check whether the database has been created.
	if (CBBlockChainStorageExists(self->storage)) {
		// Found now load information from storage
		// Basic validator information
		if (NOT CBBlockChainStorageLoadBasicValidator(self)){
			logError("There was an error when loading the validator information.");
			return false;
		}
		// Loop through the orphans
		for (uint8_t x = 0; x < self->numOrphans; x++) {
			if (NOT CBBlockChainStorageLoadOrphan(self, x)) {
				logError("Could not load orphan %u.",x);
				return false;
			}
		}
		// Loop through the branches
		for (uint8_t x = 0; x < self->numBranches; x++) {
			if (NOT CBBlockChainStorageLoadBranch(self, x)) {
				logError("Could not load branch %u.",x);
				return false;
			}
			if (NOT CBBlockChainStorageLoadBranchWork(self, x)) {
				logError("Could not load work for branch %u.",x);
				return false;
			}
		}
	}else{
		// Write the genesis block
		CBBlock * genesis = CBNewBlockGenesis(logError);
		if (NOT CBBlockChainStorageSaveBlock(self, genesis, 0, 0)) {
			logError("Could not save genesis block.");
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
			logError("Could not save the initial basic validation information.");
			return false;
		}
		// Create initial branch information
		self->branches[0].lastRetargetTime = 1231006505;
		self->branches[0].startHeight = 0;
		self->branches[0].numBlocks = 1;
		self->branches[0].lastValidation = 0;
		// Write branch information
		if (NOT CBBlockChainStorageSaveBranch(self, 0)) {
			logError("Could not save the initial branch information.");
			return false;
		}
		// Create work CBBigInt
		self->branches[0].work.length = 1;
		self->branches[0].work.data = malloc(1);
		if (NOT self->branches[0].work.data) {
			self->logError("Could not allocate 1 byte of memory for the initial branch work.");
			return false;
		}
		self->branches[0].work.data[0] = 0;
		// Write branch work
		if (NOT CBBlockChainStorageSaveBranchWork(self, 0)) {
			logError("Could not save the initial work.");
			return false;
		}
		// Now try to commit the data
		if (NOT CBBlockChainStorageCommitData(storage)){
			self->logError("Could not commit initial block-chain data.");
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
		self->logError("Could not save a block");
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
		self->logError("Could not write the branch information for branch %u.", branch);
		return false;
	}
	// Update the branch work in storage
	if (NOT CBBlockChainStorageSaveBranchWork(self, branch)) {
		self->logError("Could not write the branch work for branch %u.", branch);
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
		self->logError("There was an error when saving the basic validation information when adding an orhpan block.");
		return false;
	}
	// Add to memory
	self->orphans[pos] = block;
	CBRetainObject(block);
	// Add to storage
	if (NOT CBBlockChainStorageSaveOrphan(self, block, pos)) {
		self->logError("Could not save an orphan.");
		return false;
	}
	// Commit data
	if (NOT CBBlockChainStorageCommitData(self->storage)) {
		self->logError("There was commiting data for an orphan.");
		return false;
	}
	return true;
}
CBBlockStatus CBFullValidatorBasicBlockValidation(CBFullValidator * self, CBBlock * block, uint8_t * txHashes, uint64_t networkTime){
	// Get the block hash
	uint8_t * hash = CBBlockGetHash(block);
	// Check if duplicate.
	for (uint8_t x = 0; x < self->numOrphans; x++)
		if (NOT memcmp(CBBlockGetHash(self->orphans[x]), hash, 32))
			return CB_BLOCK_STATUS_DUPLICATE;
	// Look in block hash index
	if (CBBlockChainStorageBlockExists(self,CBBlockGetHash(block)))
		return CB_BLOCK_STATUS_DUPLICATE;
	// Check block has transactions
	if (NOT block->transactionNum)
		return CB_BLOCK_STATUS_BAD;
	// Check block hash against target and that it is below the maximum allowed target.
	if (NOT CBValidateProofOfWork(hash, block->target))
		return CB_BLOCK_STATUS_BAD;
	// Check the block is within two hours of the network time.
	if (block->time > networkTime + 7200)
		return CB_BLOCK_STATUS_BAD_TIME;
	// Calculate merkle root.
	CBCalculateMerkleRoot(txHashes, block->transactionNum);
	// Check merkle root
	int res = memcmp(txHashes, CBByteArrayGetData(block->merkleRoot), 32);
	if (res)
		return CB_BLOCK_STATUS_BAD;
	return CB_BLOCK_STATUS_CONTINUE;
}
CBBlockStatus CBFullValidatorBasicBlockValidationCopy(CBFullValidator * self, CBBlock * block, uint8_t * txHashes, uint64_t networkTime){
	uint8_t * hashes = malloc(block->transactionNum * 32);
	if (NOT hashes)
		return CB_BLOCK_STATUS_ERROR;
	memcpy(hashes, txHashes, block->transactionNum * 32);
	CBBlockStatus res = CBFullValidatorBasicBlockValidation(self, block, hashes, networkTime);
	free(hashes);
	return res;
}
CBBlockValidationResult CBFullValidatorCompleteBlockValidation(CBFullValidator * self, uint8_t branch, CBBlock * block, uint8_t * txHashes, uint32_t height){
	// Check that the first transaction is a coinbase transaction.
	if (NOT CBTransactionIsCoinBase(block->transactions[0]))
		return CB_BLOCK_VALIDATION_BAD;
	uint64_t blockReward = CBCalculateBlockReward(height);
	uint64_t coinbaseOutputValue = 0;
	uint32_t sigOps = 0;
	// Do validation for transactions.
	bool err;
	CBPrevOut ** allSpentOutputs = malloc(sizeof(*allSpentOutputs) * block->transactionNum);
	for (uint32_t x = 0; x < block->transactionNum; x++) {
		// Check that the transaction is final.
		if (NOT CBTransactionIsFinal(block->transactions[x], block->time, height))
			return CB_BLOCK_VALIDATION_BAD;
		// Do the basic validation
		uint64_t outputValue;
		allSpentOutputs[x] = CBTransactionValidateBasic(block->transactions[x], NOT x, &outputValue, &err);
		if (err){
			for (uint32_t c = 0; c < x; c++)
				free(allSpentOutputs[c]);
			return CB_BLOCK_VALIDATION_ERR;
		}
		if (NOT allSpentOutputs[x]){
			for (uint32_t c = 0; c < x; c++)
				free(allSpentOutputs[c]);
			return CB_BLOCK_VALIDATION_BAD;
		}
		// Check correct structure for coinbase
		if (CBTransactionIsCoinBase(block->transactions[x])){
			if (x)
				return CB_BLOCK_VALIDATION_BAD;
			coinbaseOutputValue += outputValue;
		}else if (NOT x)
			return CB_BLOCK_VALIDATION_BAD;
		// Count sigops
		sigOps += CBTransactionGetSigOps(block->transactions[x]);
		if (sigOps > CB_MAX_SIG_OPS)
			return CB_BLOCK_VALIDATION_BAD;
		// Verify each input and count input values
		uint64_t inputValue = 0;
		for (uint32_t y = 1; y < block->transactions[x]->inputNum; y++) {
			CBBlockValidationResult res = CBFullValidatorInputValidation(self, branch, block, height, x, y, allSpentOutputs, txHashes, &inputValue,&sigOps);
			if (res != CB_BLOCK_VALIDATION_OK) {
				for (uint32_t c = 0; c < x; c++)
					free(allSpentOutputs[c]);
				return res;
			}
		}
		// Done now with the spent outputs
		for (uint32_t c = 0; c < x; c++)
			free(allSpentOutputs[c]);
		if (x){
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
		branch = self->branches[branch].parentBranch;
		prevIndex = self->branches[branch].numBlocks - 1;
		x -= prevIndex;
	}
}
CBBlockValidationResult CBFullValidatorInputValidation(CBFullValidator * self, uint8_t branch, CBBlock * block, uint32_t blockHeight, uint32_t transactionIndex,uint32_t inputIndex, CBPrevOut ** allSpentOutputs, uint8_t * txHashes, uint64_t * value, uint32_t * sigOps){
	// Check that the previous output is not already spent by this block.
	for (uint32_t a = 0; a < transactionIndex; a++)
		for (uint32_t b = 0; b < block->transactions[a]->inputNum; b++)
			if (CBByteArrayCompare(allSpentOutputs[transactionIndex][inputIndex].hash, allSpentOutputs[a][b].hash)
				&& allSpentOutputs[transactionIndex][inputIndex].index == allSpentOutputs[a][b].index)
				// Duplicate found.
				return CB_BLOCK_VALIDATION_BAD;
	// Now we need to check that the output is in this block (before this transaction) or unspent elsewhere in the blockchain.
	CBTransactionOutput * prevOut;
	bool found = false;
	for (uint32_t a = 0; a < transactionIndex; a++) {
		if (NOT memcmp(txHashes + 32*a, CBByteArrayGetData(allSpentOutputs[transactionIndex][inputIndex].hash), 32)) {
			// This is the transaction hash. Make sure there is the output.
			if (block->transactions[a]->outputNum <= allSpentOutputs[transactionIndex][inputIndex].index)
				// Too few outputs.
				return CB_BLOCK_VALIDATION_BAD;
			prevOut = block->transactions[a]->outputs[allSpentOutputs[transactionIndex][inputIndex].index];
			found = true;
			break;
		}
	}
	if (NOT found) {
		// Not found in this block. Look in database for the unspent output.
		if (NOT CBBlockChainStorageUnspentOutputExists(self, CBByteArrayGetData(allSpentOutputs[transactionIndex][inputIndex].hash), allSpentOutputs[transactionIndex][inputIndex].index))
			return CB_BLOCK_VALIDATION_BAD;
		// Exists so therefore load the unspent output
		bool coinbase;
		uint32_t outputHeight;
		prevOut = CBBlockChainStorageLoadUnspentOutput(self, CBByteArrayGetData(allSpentOutputs[transactionIndex][inputIndex].hash), allSpentOutputs[transactionIndex][inputIndex].index, &coinbase, &outputHeight);
		if (NOT prevOut) {
			self->logError("Could not load an unspent output.");
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
	if (res == CB_SCRIPT_INVALID){
		CBFreeScriptStack(stack);
		CBReleaseObject(prevOut);
		return CB_BLOCK_VALIDATION_BAD;
	}
	// Verify P2SH inputs.
	if (CBScriptIsP2SH(prevOut->scriptObject)){
		if (NOT CBScriptIsPushOnly(prevOut->scriptObject)){
			CBFreeScriptStack(stack);
			CBReleaseObject(prevOut);
			return CB_BLOCK_VALIDATION_BAD;
		}
		// Since the output is a P2SH we include the serialised script in the signature operations
		CBScript * p2shScript = CBNewScriptWithData(stack.elements[stack.length - 1].data, stack.elements[stack.length - 1].length, self->logError);
		if (NOT p2shScript) {
			self->logError("Could not create a P2SH script for counting sig ops.");
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
	if (res == CB_SCRIPT_INVALID)
		return CB_BLOCK_VALIDATION_BAD;
	return CB_BLOCK_VALIDATION_OK;
}
CBBlockStatus CBFullValidatorProcessBlock(CBFullValidator * self, CBBlock * block, uint64_t networkTime){
	// Get transaction hashes.
	uint8_t * txHashes = malloc(32 * block->transactionNum);
	if (NOT txHashes)
		return CB_BLOCK_STATUS_ERROR;
	// Put the hashes for transactions into a list.
	for (uint32_t x = 0; x < block->transactionNum; x++)
		memcpy(txHashes + 32*x, CBTransactionGetHash(block->transactions[x]), 32);
	// Determine what type of block this is.
	uint8_t prevBranch;
	uint32_t prevBlockIndex;
	uint32_t prevBlockTarget;
	if (CBBlockChainStorageBlockExists(self, CBByteArrayGetData(block->prevBlockHash))) {
		// Has a block in the block hash index. Get branch and index.
		if (NOT CBBlockChainStorageGetBlockLocation(self, CBByteArrayGetData(block->prevBlockHash), &prevBranch, &prevBlockIndex)) {
			self->logError("Could not get the location of a previous block.");
			return CB_BLOCK_STATUS_ERROR;
		}
		// Get previous block target
		prevBlockTarget = CBBlockChainStorageGetBlockTarget(self, prevBranch, prevBlockIndex);
		if (NOT prevBlockTarget) {
			self->logError("Could not get the target of a previous block.");
			return CB_BLOCK_STATUS_ERROR;
		}
	}else{
		// Orphan block. End here.
		// Do basic validation
		CBBlockStatus res = CBFullValidatorBasicBlockValidation(self, block, txHashes, networkTime);
		free(txHashes);
		if (res != CB_BLOCK_STATUS_CONTINUE)
			return res;
		// Add block to orphans
		if(CBFullValidatorAddBlockToOrphans(self,block))
			return CB_BLOCK_STATUS_ORPHAN;
		return CB_BLOCK_STATUS_ERROR; // Needs recovery.
	}
	// Do basic validation with a copy of the transaction hashes.
	CBBlockStatus status = CBFullValidatorBasicBlockValidationCopy(self, block, txHashes, networkTime);
	if (status != CB_BLOCK_STATUS_CONTINUE){
		free(txHashes);
		return status;
	}
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
			if (self->branches[earliestBranch].working || earliestBranch == self->mainBranch) {
				// Cannot overwrite branch
				free(txHashes);
				return CB_BLOCK_STATUS_NO_NEW;
			}
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
						self->logError("Could not delete a block when removing a branch.");
						free(txHashes);
						return CB_BLOCK_STATUS_ERROR;
					}
				}
				// Change block keys from earliest branch to dependent branch
				for (uint8_t x = 0; x < highestDependency; x++) {
					if (NOT CBBlockChainStorageMoveBlock(self, earliestBranch, x, mergeBranch, x)) {
						self->logError("Could not move a block for merging two branches.");
						free(txHashes);
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
					self->logError("Could not save the updated branch information for merging two branches together.");
					free(txHashes);
					return CB_BLOCK_STATUS_ERROR;
				}
				// Find all of the other dependent branches for the earliest branch and update the parent branch information.
				for (uint8_t x = 0; x < self->numBranches; x++) {
					if (x != mergeBranch && self->branches[x].parentBranch == earliestBranch) {
						self->branches[x].parentBranch = mergeBranch;
						// Update in storage
						if (NOT CBBlockChainStorageSaveBranch(self, x)) {
							self->logError("Could not write the updated parent branch from the overwritten branch during a merge.");
							free(txHashes);
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
							self->logError("Could not write an updated parent block index during a merge.");
							free(txHashes);
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
				self->logError("Could not write the new number of branches.");
				free(txHashes);
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
			if (NOT CBCalculateBlockWork(&tempWork, prevBlockTarget)){
				free(txHashes);
				return CB_BLOCK_STATUS_ERROR;
			}
			CBBigIntEqualsSubtractionByBigInt(&self->branches[branch].work, &tempWork);
			free(tempWork.data); 
		}
		// Set the remaining data
		self->branches[branch].numBlocks = 0;
		self->branches[branch].lastValidation = CB_NO_VALIDATION;
		self->branches[branch].startHeight = self->branches[prevBranch].startHeight + prevBlockIndex + 1;
		// Write branch info
		if (NOT CBBlockChainStorageSaveBranch(self, branch)) {
			self->logError("Could not save new branch data.");
			free(txHashes);
			return CB_BLOCK_STATUS_ERROR;
		}
		// Write branch work
		if (NOT CBBlockChainStorageSaveBranchWork(self, branch)) {
			self->logError("Could not save the new branch's work.");
			free(txHashes);
			return CB_BLOCK_STATUS_ERROR;
		}
	}
	// Got branch ready for block. Now process into the branch.
	status = CBFullValidatorProcessIntoBranch(self, block, networkTime, branch, prevBranch, prevBlockIndex, prevBlockTarget, txHashes);
	free(txHashes);
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
			// Make transaction hashes.
			txHashes = malloc(32 * self->orphans[x]->transactionNum);
			if (NOT txHashes)
				break;
			// Put the hashes for transactions into a list.
			for (uint32_t x = 0; x < self->orphans[x]->transactionNum; x++)
				memcpy(txHashes + 32*x, CBTransactionGetHash(self->orphans[x]->transactions[x]), 32);
			// Get previous block target for orphan.
			uint32_t target = CBBlockChainStorageGetBlockTarget(self, branch, self->branches[branch].numBlocks - 1);
			if (NOT target) {
				self->logError("Could not get a block target for an orphan.");
				return CB_BLOCK_STATUS_ERROR;
			}
			// Process into the branch.
			CBBlockStatus orphanStatus = CBFullValidatorProcessIntoBranch(self, self->orphans[x], networkTime, branch, branch, self->branches[branch].numBlocks - 1, target, txHashes);
			if (orphanStatus == CB_BLOCK_STATUS_ERROR) {
				self->logError("There was an error when processing an orphan into a branch.");
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
	return status;
}
CBBlockStatus CBFullValidatorProcessIntoBranch(CBFullValidator * self, CBBlock * block, uint64_t networkTime, uint8_t branch, uint8_t prevBranch, uint32_t prevBlockIndex, uint32_t prevBlockTarget, uint8_t * txHashes){
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
		if (CBBigIntCompareToBigInt(&work,&self->branches[self->mainBranch].work) != CB_COMPARE_MORE_THAN){
			// Add to branch without complete validation
			if (NOT CBFullValidatorAddBlockToBranch(self, branch, block, work))
				return CB_BLOCK_STATUS_ERROR;
			// Commit data
			if (NOT CBBlockChainStorageCommitData(self->storage)) {
				self->logError("Could not commit a new block to a side chain.");
				return CB_BLOCK_STATUS_ERROR;
			}
			return CB_BLOCK_STATUS_SIDE;
		}
		// Potential block-chain reorganisation. Validate the side branch. THIS NEEDS TO START AT THE FIRST BRANCH BACK WHERE VALIDATION IS NOT COMPLETE AND GO UP THROUGH THE BLOCKS VALIDATING THEM. Includes Prior Branches!
		uint8_t tempBranch = prevBranch;
		uint32_t lastBlocks[CB_MAX_BRANCH_CACHE]; // Used to store the last blocks in each branch in the potential new path upto the newest branch in the path.
		uint8_t branchPath[CB_MAX_BRANCH_CACHE]; // The branches in the new path.
		uint8_t pathIndex = CB_MAX_BRANCH_CACHE - 1; // The starting index of the path. If CB_MAX_BRANCH_CACHE - 1 then we are only validating the branch we added to.
		// Modify unspent output and transaction indices to start validation.
		// Make first last block the last block in the branch we are validating up-to.
		lastBlocks[pathIndex] = self->branches[prevBranch].numBlocks - 1;
		// Get all last blocks and next branches for path information
		while (self->branches[tempBranch].startHeight) { // While not genesis
			// Add path information
			branchPath[pathIndex] = tempBranch;
			lastBlocks[--pathIndex] = self->branches[tempBranch].parentBlockIndex;
			tempBranch = self->branches[tempBranch].parentBranch;
		}
		branchPath[pathIndex] = 0; // Add genesis
		// Get unspent output index for validating the potential new main chain
		tempBranch = self->mainBranch; // Goes down to become the branch of the fork.
		uint32_t tempBlockIndex = self->branches[self->mainBranch].numBlocks - 1; // Goes down to become the block index of the fork.
		uint8_t forkIndex; // The index whre the fork happens in the path information.
		// Go backwards towards fork
		for (;;) {
			// See if the main-chain branch is in the branch path for the potetial new chain.
			bool done = false;
			for (uint8_t x = pathIndex; x < CB_MAX_BRANCH_CACHE; x++) {
				if (branchPath[x] == tempBranch) {
					// The branch for the main-chain path is a branch in the potential new main chain branch path.
					// Go down to fork point, unless this is the fork point
					if (tempBlockIndex > lastBlocks[x]) {
						// Above fork point, go down to fork point
						for (;tempBlockIndex != lastBlocks[x];tempBlockIndex--) {
							// Change indicies
							if (NOT CBFullValidatorUpdateUnspentOutputsAndLoad(self, tempBranch, tempBlockIndex, false)){
								self->logError("Could not reverse indicies for unspent outputs and transactions during reorganisation.");
								return CB_BLOCK_STATUS_ERROR;
							}
							if (tempBlockIndex == lastBlocks[x] + 1)
								// Done here
								break;
						}
					}
					done = true;
					forkIndex = x;
					break;
				}
			}
			if (done)
				// If done, exit now and do not go to the next branch
				break;
			// We need to go down to next branch
			// Update indicies for all the blocks in the branch
			while (tempBlockIndex--) {
				// Change indicies
				if (NOT CBFullValidatorUpdateUnspentOutputsAndLoad(self, tempBranch, tempBlockIndex, false)){
					self->logError("Could not reverse indicies for unspent outputs and transactions during reorganisation.");
					return CB_BLOCK_STATUS_ERROR;
				}
			}
			// Go down a branch
			tempBranch = self->branches[tempBranch].parentBranch;
			tempBlockIndex = self->branches[tempBranch].parentBlockIndex;
		}
		// Go upwards to last validation point in new chain path
		for (pathIndex = forkIndex;; pathIndex++) {
			if (lastBlocks[pathIndex] >= self->branches[branchPath[pathIndex]].lastValidation) {
				// Go up to last validation point, updating indicies
				for (;tempBlockIndex <= self->branches[branchPath[pathIndex]].lastValidation; tempBlockIndex++) {
					if (NOT CBFullValidatorUpdateUnspentOutputsAndLoad(self, branchPath[pathIndex], tempBlockIndex, true)){
						self->logError("Could not update indicies for going to a previously validated point during reorganisation.");
						return CB_BLOCK_STATUS_ERROR;
					}
				}
				break;
			}else if (self->branches[branchPath[pathIndex]].lastValidation != CB_NO_VALIDATION){
				// Go up to last block
				for (;tempBlockIndex <= lastBlocks[pathIndex]; tempBlockIndex++) {
					if (NOT CBFullValidatorUpdateUnspentOutputsAndLoad(self, branchPath[pathIndex], tempBlockIndex, true)){
						self->logError("Could not update indicies for going to the last block for a path during reorganisation.");
						return CB_BLOCK_STATUS_ERROR;
					}
				}
				if (pathIndex == CB_MAX_BRANCH_CACHE - 1)
					// Done
					break;
				// Reset block index for next branch
				tempBlockIndex = 0;
			}else
				// This is the block we should start at.
				break;
		}
		// At this point outputChangeBlockIndex is the block we start at and branchPath[pathIndex] is the branch.
		// Now validate all blocks going up.
		uint8_t * txHashes2 = NULL;
		uint32_t txHashes2AllocSize = 0;
		bool atLeastOne = false; // True when at least one block passed validation for a branch
		tempBranch = branchPath[pathIndex]; // Use the branch we landed on.
		uint8_t firstBranch = tempBranch; // Saving the first branch when going back an updating last validated blocks.
		for (;;) {
			// Get block to validate
			CBBlock * tempBlock = CBBlockChainStorageLoadBlock(self, tempBlockIndex, tempBranch);
			if (NOT tempBlock){
				free(txHashes2);
				return CB_BLOCK_STATUS_ERROR;
			}
			if (NOT CBBlockDeserialise(tempBlock, true)){
				free(txHashes2);
				return CB_BLOCK_STATUS_ERROR;
			}
			// Get transaction hashes
			if (txHashes2AllocSize < tempBlock->transactionNum * 32) {
				txHashes2AllocSize = tempBlock->transactionNum * 32;
				uint8_t * temp = realloc(txHashes2, txHashes2AllocSize);
				if (NOT temp){
					free(txHashes2);
					return CB_BLOCK_STATUS_ERROR;
				}
				txHashes2 = temp;
			}
			for (uint32_t x = 0; x < tempBlock->transactionNum; x++)
				memcpy(txHashes2 + 32*x, CBTransactionGetHash(tempBlock->transactions[x]), 32);
			// Validate block
			CBBlockValidationResult res = CBFullValidatorCompleteBlockValidation(self, tempBranch, tempBlock, txHashes2, self->branches[tempBranch].startHeight + tempBlockIndex);
			if (res == CB_BLOCK_VALIDATION_BAD
				|| res == CB_BLOCK_VALIDATION_ERR){
				// Free data
				free(txHashes2);
				CBReleaseObject(tempBlock);
				if (atLeastOne && res == CB_BLOCK_VALIDATION_BAD) {
					// Clear IO operations, thus reverting to previous main-chain.
					CBBlockChainStorageReset(self->storage);
					// Save last valided blocks for each branch
					if (NOT CBFullValidatorSaveLastValidatedBlocks(self, firstBranch, tempBranch)) {
						self->logError("Could not save the last validated blocks for a validatation error during reorganisation");
						return CB_BLOCK_STATUS_ERROR;
					}
					if (NOT CBBlockChainStorageCommitData(self->storage)) {
						self->logError("Could not commit the last validated block for a validatation error during reorganisation.");
						return false;
					}
				}
				if (res == CB_BLOCK_VALIDATION_BAD)
					return CB_BLOCK_STATUS_BAD;
				if (res == CB_BLOCK_VALIDATION_ERR)
					return CB_BLOCK_STATUS_ERROR;
			}
			if (tempBlockIndex > self->branches[tempBranch].lastValidation)
				self->branches[tempBranch].lastValidation = tempBlockIndex;
			atLeastOne = true;
			// This block passed validation. Move onto next block.
			if (tempBlockIndex == lastBlocks[pathIndex]) {
				// Came to the last block in the branch
				// Update last validation information
				self->branches[tempBranch].lastValidation = tempBlockIndex;
				// Check if last block
				if (tempBlockIndex == self->branches[branch].numBlocks - 1 && tempBranch == branch)
					break;
				// Go to next branch
				tempBranch = branchPath[++pathIndex];
				tempBlockIndex = 0;
				atLeastOne = false;
			}else
				tempBlockIndex++;
			CBReleaseObject(tempBlock);
		}
		// Free transaction hashes
		free(txHashes2);
		// Save last validated blocks
		if (NOT CBFullValidatorSaveLastValidatedBlocks(self, firstBranch, tempBranch)) {
			self->logError("Could not save the last validated blocks during reorganisation");
			return CB_BLOCK_STATUS_ERROR;
		}
		// Now we validate the block for the new main chain.
	}
	// We are just validating a new block on the main chain.
	CBBlockValidationResult res = CBFullValidatorCompleteBlockValidation(self, branch, block, txHashes, self->branches[branch].startHeight + self->branches[branch].numBlocks);
	switch (res) {
		case CB_BLOCK_VALIDATION_BAD:
			CBBlockChainStorageReset(self->storage);
			return CB_BLOCK_STATUS_BAD;
		case CB_BLOCK_VALIDATION_ERR:
			return CB_BLOCK_STATUS_ERROR;
		case CB_BLOCK_VALIDATION_OK:
			// Update branch and unspent outputs.
			if (NOT CBFullValidatorAddBlockToBranch(self, branch, block, work)){
				self->logError("There was an error when adding a new block to the branch.");
				return CB_BLOCK_STATUS_ERROR;
			}
			self->branches[branch].lastValidation = self->branches[branch].numBlocks - 1;
			// Update storage
			if (NOT CBBlockChainStorageSaveBranch(self, branch)) {
				self->logError("Could not save the last validated block for a branch when adding a new block to the main chain.");
				return CB_BLOCK_STATUS_ERROR;
			}
			if (branch != self->mainBranch) {
				// Update main branch
				self->mainBranch = branch;
				if (NOT CBBlockChainStorageSaveBasicValidator(self)) {
					self->logError("Could not save the new main branch when adding a new block to the main chain.");
					return CB_BLOCK_STATUS_ERROR;
				}
			}
			// Update the unspent output indicies.
			if (NOT CBFullValidatorUpdateUnspentOutputs(self, block, branch, self->branches[branch].lastValidation, true)) {
				self->logError("Could not update the unspent outputs when adding a block to the main chain.");
				return CB_BLOCK_STATUS_ERROR;
			}
			// Finally we can commit the remaining data to storage
			if (NOT CBBlockChainStorageCommitData(self->storage)) {
				self->logError("Could not commit updated data when adding a new block to the main chain.");
				return CB_BLOCK_STATUS_ERROR;
			}
			return CB_BLOCK_STATUS_MAIN;
	}
}
bool CBFullValidatorSaveLastValidatedBlocks(CBFullValidator * self, uint8_t startBranch, uint8_t endBranch){
	for (;;) {
		if (NOT CBBlockChainStorageSaveBranch(self, endBranch)) {
			self->logError("Could not save the last validated block for a branch.");
			return false;
		}
		if (endBranch == startBranch)
			break;
		endBranch = self->branches[endBranch].parentBranch;
	}
	return true;
}
bool CBFullValidatorUpdateUnspentOutputs(CBFullValidator * self, CBBlock * block, uint8_t branch, uint32_t blockIndex, bool forward){
	// Update unspent outputs... Go through transactions, removing the prevOut references and adding the outputs for one transaction at a time.
	uint32_t cursor = 80; // Cursor to find output positions.
	uint8_t byte = CBByteArrayGetByte(CBGetMessage(block)->bytes, 80);
	cursor += byte < 253 ? 1 : (byte == 253 ? 3 : (byte == 254 ? 5 : 9));
	uint8_t * txReadData = NULL;
	uint32_t txReadDataSize = 0;
	for (uint32_t x = 0; x < block->transactionNum; x++) {
		cursor += 4; // Move along version number
		// Move along input number
		byte = CBByteArrayGetByte(CBGetMessage(block)->bytes, cursor);
		cursor += byte < 253 ? 1 : (byte == 253 ? 3 : (byte == 254 ? 5 : 9));
		// First remove output references than add new outputs.
		for (uint32_t y = 0; y < block->transactions[x]->inputNum; y++) {
			if (x) {
				// Only non-coinbase transactions contain prevOut references in inputs.
				uint8_t * txHash = CBByteArrayGetData(block->transactions[x]->inputs[y]->prevOut.hash);
				uint32_t outputIndex = block->transactions[x]->inputs[y]->prevOut.index;
				if (forward){
					// Remove output
					if (NOT CBBlockChainStorageDeleteUnspentOutput(self, txHash, outputIndex)) {
						self->logError("Could not remove an output as unspent.");
						free(txReadData);
						return false;
					}
				}else{
					// Add to storage
					// Read transaction outputs from the block
					uint32_t outputsPos;
					if (NOT CBBlockChainStorageLoadOutputs(self, txHash, txReadData, &txReadDataSize, &outputsPos)) {
						self->logError("Could not load a transaction's outputs for re-organisation.");
						free(txReadData);
						return false;
					}
					// Find the position of the output by looping through outputs
					uint32_t txCursor = 0;
					for (uint32_t x = 0; x < block->transactions[x]->inputs[y]->prevOut.index; x++) {
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
					// Add the posiion within thr outputs to the start of the outputs.
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
					if (NOT CBBlockChainStorageSaveUnspentOutput(self, txHash, outputIndex, outputsPos, outputLen)) {
						self->logError("Could not save an unspent output reference when going backwards during re-organisation.");
						free(txReadData);
						return false;
					}
				}
			}
			// Move cursor along script varint. We look at byte data in case it is longer than needed.
			cursor += 36; // Output reference
			// Var int
			uint8_t byte = CBByteArrayGetByte(CBGetMessage(block)->bytes, cursor);
			cursor += byte < 253 ? 1 : (byte == 253 ? 3 : (byte == 254 ? 5 : 9));
			// Move along script
			cursor += block->transactions[x]->inputs[y]->scriptObject->length;
			// Move cursor along sequence
			cursor += 4;
		}
		// Move cursor past output number to first output
		byte = CBByteArrayGetByte(CBGetMessage(block)->bytes, cursor);
		cursor += byte < 253 ? 1 : (byte == 253 ? 3 : (byte == 254 ? 5 : 9));
		// Now add new outputs
		uint8_t * txHash = CBTransactionGetHash(block->transactions[x]);
		// For adding the size of outputs
		uint32_t outputsPos = cursor;
		for (uint32_t y = 0; y < block->transactions[x]->outputNum; y++) {
			if (forward) {
				// Add to storage.
				if (NOT CBBlockChainStorageSaveUnspentOutput(self, txHash, y, cursor, CBGetMessage(block->transactions[x]->outputs[y])->bytes->length)) {
					self->logError("Could not write a new unspent output reference.");
					free(txReadData);
					return false;
				}
			}else{
				// Remove output from storage
				if (NOT CBBlockChainStorageDeleteUnspentOutput(self, txHash, y)) {
					self->logError("Could not remove unspent output reference during re-organisation.");
					free(txReadData);
					return false;
				}
			}
			// Move cursor past the output
			cursor += 8; // Value
			// Var int
			uint8_t byte = CBByteArrayGetByte(CBGetMessage(block)->bytes, cursor);
			cursor += byte < 253 ? 1 : (byte == 253 ? 3 : (byte == 254 ? 5 : 9));
			// Script length
			cursor += block->transactions[x]->outputs[y]->scriptObject->length;
		}
		if (forward) {
			// Add transaction to transaction index
			if (NOT CBBlockChainStorageSaveTransactionRef(self, txHash, branch, blockIndex, outputsPos, cursor - outputsPos, x == 0)) {
				self->logError("Could not write transaction reference to transaction index.");
				free(txReadData);
				return false;
			}
		}else
			// Remove transaction from transaction index.
			if (NOT CBBlockChainStorageDeleteTransactionRef(self, txHash)) {
				self->logError("Could not remove transaction reference from the transaction index.");
				free(txReadData);
				return false;
			}
	}
	// Free transaction read memory.
	free(txReadData);
	return true;
}
bool CBFullValidatorUpdateUnspentOutputsAndLoad(CBFullValidator * self, uint8_t branch, uint32_t blockIndex, bool forward){
	// Load the block
	CBBlock * block = CBBlockChainStorageLoadBlock(self, blockIndex, branch);
	if (NOT block) {
		CBReleaseObject(block);
		self->logError("Could not deserailise a block for re-organisation.");
		return false;
	}
	// Deserailise block
	if (NOT CBBlockDeserialise(block, true)) {
		self->logError("Could not open a block for re-organisation.");
		return false;
	}
	// Update indices going backwards
	bool res = CBFullValidatorUpdateUnspentOutputs(self, block, branch, blockIndex, forward);
	if (NOT res)
		self->logError("Could not update the unspent outputs and transaction indices.");
	// Free the block
	CBReleaseObject(block);
	return res;
}
