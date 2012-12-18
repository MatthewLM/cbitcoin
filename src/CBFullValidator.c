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
	uint8_t key[8];
	key[0] = 1;
	key[1] = CB_STORAGE_VALIDATOR_INFO;
	uint8_t data[21];
	if (CBBlockChainStorageGetLength(storage, key)) {
		// Found now load information from storage
		// Basic validator information
		if (NOT CBBlockChainStorageReadValue(storage, key, data, 8, 0)){
			logError("There was an error when reading the validator information from storage.");
			return false;
		}
		self->firstOrphan = data[CB_VALIDATION_FIRST_ORPHAN];
		self->numOrphans = 0;
		uint8_t numOrphansTemp = data[CB_VALIDATION_NUM_ORPHANS];
		self->mainBranch = data[CB_VALIDATION_MAIN_BRANCH];
		self->numBranches = 0;
		uint8_t numBranchesTemp = data[CB_VALIDATION_NUM_BRANCHES];
		self->numUnspentOutputs = CBArrayToInt32(data, CB_VALIDATION_NUM_UNSPENT_OUTPUTS);
		// Loop through the orphans
		key[0] = 2;
		key[1] = CB_STORAGE_ORPHAN;
		for (uint8_t x = 0; x < numOrphansTemp; x++) {
			key[2] = x;
			uint32_t len = CBBlockChainStorageGetLength(storage, key);
			CBByteArray * orphanData = CBNewByteArrayOfSize(len, logError);
			if (NOT orphanData) {
				logError("There was an error when initialising a byte array for an orphan.");
				return false;
			}
			if (NOT CBBlockChainStorageReadValue(storage, key, CBByteArrayGetData(orphanData), len, 0)) {
				logError("There was an error when reading the data for an orphan.");
				CBReleaseObject(orphanData);
				return false;
			}
			self->orphans[x] = CBNewBlockFromData(orphanData, logError);
			if (NOT self->orphans[x]) {
				logError("There was an error when creating a block object for an orphan.");
				CBReleaseObject(orphanData);
				return false;
			}
			CBReleaseObject(orphanData);
			self->numOrphans++;
		}
		// Loop through the branches
		for (uint8_t x = 0; x < numBranchesTemp; x++) {
			// Get simple information
			key[1] = CB_STORAGE_BRANCH_INFO;
			key[2] = x;
			CBInt32ToArray(key, 2, 0);
			if (NOT CBBlockChainStorageReadValue(storage, key, data, 21, 0)) {
				logError("There was an error when reading the data for a branch's information.");
				return false;
			}
			self->branches[x].lastRetargetTime = CBArrayToInt32(data, CB_BRANCH_LAST_RETARGET);
			self->branches[x].lastValidation = CBArrayToInt32(data, CB_BRANCH_LAST_VALIDATION);
			self->branches[x].numBlocks = CBArrayToInt32(data, CB_BRANCH_NUM_BLOCKS);
			self->branches[x].parentBlockIndex = CBArrayToInt32(data, CB_BRANCH_PARENT_BLOCK_INDEX);
			self->branches[x].parentBranch = data[CB_BRANCH_PARENT_BRANCH];
			self->branches[x].startHeight = CBArrayToInt32(data, CB_BRANCH_START_HEIGHT);
			self->branches[x].working = false; // Start off not working on any branch.
			// Get work
			key[1] = CB_STORAGE_WORK;
			uint8_t workLen = CBBlockChainStorageGetLength(storage, key);
			if (NOT CBBigIntAlloc(&self->branches[x].work, workLen)){
				logError("There was an error when allocating memory for a branch's work.");
				return false;
			}
			self->branches[x].work.length = workLen;
			if (NOT CBBlockChainStorageReadValue(storage, key, self->branches[x].work.data, workLen, 0)) {
				logError("There was an error when reading the work for a branch.");
				free(self->branches[x].work.data);
				return false;
			}
			self->numBranches++;
		}
	}else{
		// Write basic validator information to database
		data[CB_VALIDATION_FIRST_ORPHAN] = 0;
		data[CB_VALIDATION_NUM_ORPHANS] = 0;
		data[CB_VALIDATION_MAIN_BRANCH] = 0;
		data[CB_VALIDATION_NUM_BRANCHES] = 1;
		CBInt32ToArray(data, CB_VALIDATION_NUM_UNSPENT_OUTPUTS, 0);
		key[0] = 1;
		key[1] = CB_STORAGE_VALIDATOR_INFO;
		if (NOT CBBlockChainStorageWriteValue(storage, key, data, 8, 0, 8)) {
			logError("Could not write the initial basic validation data.");
			return false;
		}
		// Write the genesis block
		key[0] = 6;
		key[1] = CB_STORAGE_BLOCK;
		memset(key, 0, 5);
		if (NOT CBBlockChainStorageWriteValue(storage, key, (uint8_t [305]){
			0x6F,0xE2,0x8C,0x0A,0xB6,0xF1,0xB3,0x72,0xC1,0xA6,0xA2,0x46,0xAE,0x63,0xF7,0x4F,0x93,0x1E,0x83,0x65,
			0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3B,0xA3,0xED,0xFD,0x7A,0x7B,0x12,0xB2,0x7A,0xC7,0x2C,0x3E,0x67,0x76,
			0x8F,0x61,0x7F,0xC8,0x1B,0xC3,0x88,0x8A,0x51,0x32,0x3A,0x9F,0xB8,0xAA,0x4B,0x1E,0x5E,0x4A,0x29,0xAB,0x5F,0x49,0xFF,0xFF,0x00,
			0x1D,0x1D,0xAC,0x2B,0x7C,0x01,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x4D,0x04,0xFF,
			0xFF,0x00,0x1D,0x01,0x04,0x45,0x54,0x68,0x65,0x20,0x54,0x69,0x6D,0x65,0x73,0x20,0x30,0x33,0x2F,0x4A,0x61,0x6E,0x2F,0x32,0x30,
			0x30,0x39,0x20,0x43,0x68,0x61,0x6E,0x63,0x65,0x6C,0x6C,0x6F,0x72,0x20,0x6F,0x6E,0x20,0x62,0x72,0x69,0x6E,0x6B,0x20,0x6F,0x66,
			0x20,0x73,0x65,0x63,0x6F,0x6E,0x64,0x20,0x62,0x61,0x69,0x6C,0x6F,0x75,0x74,0x20,0x66,0x6F,0x72,0x20,0x62,0x61,0x6E,0x6B,0x73,
			0xFF,0xFF,0xFF,0xFF,0x01,0x00,0xF2,0x05,0x2A,0x01,0x00,0x00,0x00,0x43,0x41,0x04,0x67,0x8A,0xFD,0xB0,0xFE,0x55,0x48,0x27,0x19,
			0x67,0xF1,0xA6,0x71,0x30,0xB7,0x10,0x5C,0xD6,0xA8,0x28,0xE0,0x39,0x09,0xA6,0x79,0x62,0xE0,0xEA,0x1F,0x61,0xDE,0xB6,0x49,0xF6,
			0xBC,0x3F,0x4C,0xEF,0x38,0xC4,0xF3,0x55,0x04,0xE5,0x1E,0xC1,0x12,0xDE,0x5C,0x38,0x4D,0xF7,0xBA,0x0B,0x8D,0x57,0x8A,0x4C,0x70,
			0x2B,0x6B,0xF1,0x1D,0x5F,0xAC,0x00,0x00,0x00,0x00}, 305, 0, 305)) {
			logError("Could not write the genesis block to the block chain database.");
			return false;
		}
		// Write hash index
		key[0] = 21;
		key[1] = CB_STORAGE_BLOCK_HASH_INDEX;
		uint8_t hash[20] = {0x6F,0xE2,0x8C,0x0A,0xB6,0xF1,0xB3,0x72,0xC1,0xA6,0xA2,0x46,0xAE,0x63,0xF7,0x4F,0x93,0x1E,0x83,0x65};
		memcpy(key + 2, hash, 20);
		memset(data, 0, 5); // Index is branch 0 at position 0.
		if (NOT CBBlockChainStorageWriteValue(storage, key, data, 5, 0, 5)) {
			logError("Could not write the genesis block hash index entry to the block chain database.");
			return false;
		}
		// Write branch data
		key[0] = 2;
		key[1] = CB_STORAGE_BRANCH_INFO;
		if (NOT CBBlockChainStorageWriteValue(storage, key, (uint8_t [21]){
			0x29,0xAB,0x5F,0x49, // Last retarget 0x495fab29
			0,0,0,0, // Last validation
			1,0,0,0, // Number of blocks
			0,0,0,0, // Parent block index (redundant)
			0, // Parent block branch (redundant)
			0,0,0,0 // Start Height
			}, 21, 0, 21)) {
			logError("Could not write the initial branch data to the block chain database.");
			return false;
		}
		// Write work
		key[1] = CB_STORAGE_WORK;
		data[0] = 0;
		if (NOT CBBlockChainStorageWriteValue(storage, key, data, 1, 0, 1)) {
			logError("Could not write the initial branch work to the block chain database.");
			return false;
		}
		// Create initial data
		self->mainBranch = 0;
		self->numBranches = 1;
		self->numOrphans = 0;
		self->firstOrphan = 0;
		self->numUnspentOutputs = 0;
		// Create initial branch information
		self->branches[0].lastRetargetTime = 1231006505;
		self->branches[0].startHeight = 0;
		self->branches[0].numBlocks = 1;
		self->branches[0].lastValidation = 0;
		// Create work CBBigInt
		self->branches[0].work.length = 1;
		self->branches[0].work.data = malloc(1);
		if (NOT self->branches[0].work.data) {
			self->logError("Could not allocate 1 byte of memory for the initial branch work.");
			CBResetBlockChainStorage(storage);
			return false;
		}
		self->branches[0].work.data[0] = 0;
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
	// The keys for storage
	uint8_t key[22];
	uint8_t data[5];
	// Increase number of blocks.
	self->branches[branch].numBlocks++; 
	// Modify branch information
	free(self->branches[branch].work.data);
	self->branches[branch].work = work;
	key[0] = 2;
	key[1] = CB_STORAGE_BRANCH_INFO;
	key[2] = branch;
	if (NOT (self->branches[branch].startHeight + self->branches[branch].numBlocks) % 2016){
		self->branches[branch].lastRetargetTime = block->time;
		// Write change to disk
		CBInt32ToArray(data, 0, self->branches[branch].lastRetargetTime);
		if (NOT CBBlockChainStorageWriteValue(self->storage, key, data, 4, CB_BRANCH_LAST_RETARGET, 21)) {
			self->logError("Could not write the new last retarget time for branch %u.", branch);
			return false;
		}
	}
	// Update the number of blocks in storage.
	CBInt32ToArray(data, 0, self->branches[branch].numBlocks);
	if (NOT CBBlockChainStorageWriteValue(self->storage, key, data, 4, CB_BRANCH_NUM_BLOCKS, 21)) {
		self->logError("Could not write the number of blocks for branch %u.", branch);
		return false;
	}
	// Update the branch work in storage
	key[0] = CB_STORAGE_WORK;
	if (NOT CBBlockChainStorageWriteValue(self->storage, key, work.data, work.length, 0, work.length)) {
		self->logError("Could not write the work for branch %u.", branch);
		return false;
	}
	// Store the block
	key[0] = 6;
	key[1] = CB_STORAGE_BLOCK;
	CBInt32ToArray(key, 3, self->branches[branch].numBlocks - 1);
	uint32_t blockLen = CBGetMessage(block)->bytes->length;
	// Write hash
	if (NOT CBBlockChainStorageWriteValue(self->storage, key, CBByteArrayGetData(CBGetMessage(block)->bytes), CB_BLOCK_START, CB_BLOCK_HASH, blockLen + CB_BLOCK_START)) {
		self->logError("Could not write the block hash for a new block to branch %u.", branch);
		return false;
	}
	// Write serailised data
	if (NOT CBBlockChainStorageWriteValue(self->storage, key, CBByteArrayGetData(CBGetMessage(block)->bytes), blockLen, CB_BLOCK_START, blockLen + CB_BLOCK_START)) {
		self->logError("Could not write the block data for a new block to branch %u.", branch);
		return false;
	}
	key[0] = 21;
	// Store block hash index entry
	key[1] = CB_STORAGE_BLOCK_HASH_INDEX;
	memcpy(key + 2, CBBlockGetHash(block), 20);
	data[0] = branch;
	CBInt32ToArray(data, 1, self->branches[branch].numBlocks - 1);
	if (NOT CBBlockChainStorageWriteValue(self->storage, key, data, 5, 0, 5)){
		self->logError("Could not write the block hash index data for a new block to branch %u.", branch);
		return false;
	}
	return true;
}
bool CBFullValidatorAddBlockToOrphans(CBFullValidator * self, CBBlock * block){
	// Save orphan.
	uint8_t pos;
	uint8_t key[3];
	key[0] = 1;
	key[1] = CB_STORAGE_VALIDATOR_INFO;
	if (self->numOrphans == CB_MAX_ORPHAN_CACHE) {
		// Release old orphan
		CBReleaseObject(self->orphans[self->firstOrphan]);
		pos = self->firstOrphan;
		self->firstOrphan++;
		if (NOT CBBlockChainStorageWriteValue(self->storage, key, &self->firstOrphan, 1, CB_VALIDATION_FIRST_ORPHAN, 1)){
			self->logError("There was an error writing the index of the first orphan.");
			return false;
		}
	}else{
		// Adding an orphan.
		pos = self->numOrphans++;
		if (NOT CBBlockChainStorageWriteValue(self->storage, key, &self->numOrphans, 1, CB_VALIDATION_NUM_ORPHANS, 1)){
			self->logError("There was an error writing the number of orphans.");
			return false;
		}
	}
	// Add to memory
	self->orphans[pos] = block;
	CBRetainObject(block);
	// Add to storage
	key[0] = 2;
	key[1] = CB_STORAGE_ORPHAN;
	key[2] = pos;
	uint32_t len = CBGetMessage(block)->bytes->length;
	if (NOT CBBlockChainStorageWriteValue(self->storage, key, CBByteArrayGetData(CBGetMessage(block)->bytes), len, 0, len)){
		self->logError("There was an error writing an orphan.");
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
	uint8_t key[22];
	key[0] = 21;
	key[1] = CB_STORAGE_BLOCK_HASH_INDEX;
	memcpy(key + 2, CBBlockGetHash(block), 20);
	if (CBBlockChainStorageGetLength(self->storage, key))
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
	uint8_t key[6];
	uint8_t data[4];
	for (;;) {
		if (prevIndex >= x) {
			prevIndex -= x;
			key[0] = 5;
			key[1] = branch;
			CBInt32ToArray(key, 2, prevIndex);
			if (NOT CBBlockChainStorageReadValue(self->storage, key, data, 4, CB_BLOCK_TIME)) {
				self->logError("Could not read the time for a block.");
				return 0;
			}
			return CBArrayToInt32(data, 0);
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
		// First read data for the unspent output key.
		uint8_t key[38];
		key[0] = 37;
		key[1] = CB_STORAGE_UNSPENT_OUTPUT;
		memcpy(key + 2, CBByteArrayGetData(allSpentOutputs[transactionIndex][inputIndex].hash), 32);
		CBInt32ToArray(key, 33, allSpentOutputs[transactionIndex][inputIndex].index);
		uint8_t data[14];
		if (NOT CBBlockChainStorageReadValue(self->storage, key, data, 4, 0)) {
			self->logError("Cannot read unspent output information from the block chain database");
			return CB_BLOCK_VALIDATION_ERR;
		}
		uint32_t outputPosition = data[CB_UNSPENT_OUTPUT_REF_POSITION];
		// Now read data for the transaction
		key[0] = 33;
		key[1] = CB_STORAGE_TRANSACTION_INDEX;
		// We already have added the transaction hash into the key
		if (NOT CBBlockChainStorageReadValue(self->storage, key, data, 14, 0)) {
			self->logError("Cannot read a transaction reference from the transaction index.");
			return CB_BLOCK_VALIDATION_ERR;
		}
		uint8_t outputBranch = data[CB_TRANSACTION_REF_BRANCH];
		uint32_t outputBlockIndex = CBArrayToInt32(data, CB_TRANSACTION_REF_BLOCK_INDEX);
		// Check coinbase maturity
		if (data[CB_TRANSACTION_REF_IS_COINBASE] && blockHeight - (self->branches[outputBranch].startHeight + outputBlockIndex) < CB_COINBASE_MATURITY)
			return CB_BLOCK_VALIDATION_BAD;
		// Get the output from storage
		key[0] = 6;
		key[1] = CB_STORAGE_BLOCK;
		key[2] = outputBranch;
		CBInt32ToArray(key, 3, outputBlockIndex);
		// Deserialise output as we read from the database as we do not know the length
		if (NOT CBBlockChainStorageReadValue(self->storage, key, data, 9, CB_BLOCK_START + outputPosition)) {
			self->logError("Could not read block output first 9 bytes for branch %u and index %u.",outputBranch, outputBlockIndex);
			return CB_BLOCK_VALIDATION_ERR;
		}
		uint32_t scriptSize;
		uint8_t varIntSize;
		if (data[8] < 253){
			scriptSize = data[8];
			varIntSize = 0;
		}else{
			if (data[8] == 253)
				varIntSize = 2;
			else if (data[8] == 254)
				varIntSize = 4;
			else
				varIntSize = 8;
			// Read the script size
			uint8_t scriptSizeBytes[varIntSize];
			if (NOT CBBlockChainStorageReadValue(self->storage, key, scriptSizeBytes, varIntSize, outputPosition + 9)) {
				self->logError("Could not read block output varint data for branch %u and index %u.",outputBranch, outputBlockIndex);
				return CB_BLOCK_VALIDATION_ERR;
			}
			if (varIntSize == 2)
				scriptSize = CBArrayToInt16(scriptSizeBytes, 0);
			else
				scriptSize = CBArrayToInt32(scriptSizeBytes, 0);
		}
		// Read the script
		CBScript * script = CBNewScriptOfSize(scriptSize, self->logError);
		if (NOT script) {
			self->logError("Could not create script for a previous output from the blockchain database.");
			return CB_BLOCK_VALIDATION_ERR;
		}
		if (NOT CBBlockChainStorageReadValue(self->storage, key, CBByteArrayGetData(script), scriptSize, outputPosition + 9 + varIntSize)) {
			self->logError("Could not read block output script for branch %u and index %u.",outputBranch, outputBlockIndex);
			return CB_BLOCK_VALIDATION_ERR;
		}
		// Make the object for the output
		prevOut = CBNewTransactionOutput(CBArrayToInt64(data, 0), script, self->logError);
		if (NOT prevOut) {
			self->logError("Could not create a previous output from the blockchain database.");
			return CB_BLOCK_VALIDATION_ERR;
		}
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
CBBlock * CBFullValidatorLoadBlock(CBFullValidator * self, uint32_t blockID, uint32_t branch){
	// Open the file
	uint8_t key[7];
	key[0] = 6;
	key[1] = CB_STORAGE_BLOCK;
	key[2] = branch;
	CBInt32ToArray(key, 3, blockID);
	uint32_t blockDataLen = CBBlockChainStorageGetLength(self->storage, key) - CB_BLOCK_START;
	if (NOT blockDataLen)
		return NULL;
	// Get block data
	CBByteArray * data = CBNewByteArrayOfSize(blockDataLen, self->logError);
	if (NOT data) {
		self->logError("Could not initialise a byte array for loading a block.");
		return NULL;
	}
	if (NOT CBBlockChainStorageReadValue(self->storage, key, CBByteArrayGetData(data), blockDataLen, CB_BLOCK_START)){
		self->logError("Could not read a block from the database.");
		CBReleaseObject(data);
		return NULL;
	}
	// Make and return the block
	CBBlock * block = CBNewBlockFromData(data, self->logError);
	CBReleaseObject(data);
	if (NOT block) {
		self->logError("Could not create a block object when loading a block.");
		return NULL;
	}
	return block;
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
	uint8_t key[22];
	uint8_t data[5];
	key[0] = 21;
	key[1] = CB_STORAGE_BLOCK_HASH_INDEX;
	memcpy(key + 2, CBByteArrayGetData(block->prevBlockHash), 20);
	if (CBBlockChainStorageGetLength(self->storage, key)) {
		// Has a block in the block hash index. Get branch and index.
		if (NOT CBBlockChainStorageReadValue(self->storage, key, data, 5, 0)) {
			self->logError("Could not read a block hash reference from the block chain database.");
			return CB_BLOCK_STATUS_ERROR;
		}
		prevBranch = data[CB_BLOCK_HASH_REF_BRANCH];
		prevBlockIndex = CBArrayToInt32(data, CB_BLOCK_HASH_REF_INDEX);
		// Get previous block target
		key[0] = 6;
		key[1] = CB_STORAGE_BLOCK;
		key[2] = prevBranch;
		CBInt32ToArray(key, 3, prevBlockIndex);
		if (NOT CBBlockChainStorageReadValue(self->storage, key, data, 4, CB_BLOCK_TARGET)) {
			self->logError("Could not read a block target from the block chain database.");
			return CB_BLOCK_STATUS_ERROR;
		}
		prevBlockTarget = CBArrayToInt32(data, 0);
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
		return CB_BLOCK_STATUS_ERROR_BAD_DATA; // Needs recovery.
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
		uint8_t key[7];
		uint8_t data[21];
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
					// Delete from storage
					key[0] = 6;
					key[1] = CB_STORAGE_BLOCK;
					key[2] = earliestBranch;
					CBInt32ToArray(key, 3, x);
					// Get hash
					if (NOT CBBlockChainStorageReadValue(self->storage, key, data, 20, CB_BLOCK_HASH)) {
						self->logError("Could not obtain a block hash from the block chain database.");
						free(txHashes);
						return CB_BLOCK_STATUS_ERROR_BAD_DATA;
					}
					// Remove data
					if (NOT CBBlockChainStorageRemoveValue(self->storage, key)){
						self->logError("Could not remove block value from database.");
						free(txHashes);
						return CB_BLOCK_STATUS_ERROR_BAD_DATA;
					}
					// Remove hash index reference
					key[0] = 21;
					key[1] = CB_STORAGE_BLOCK_HASH_INDEX;
					memcpy(key + 2, data, 20);
					if (NOT CBBlockChainStorageRemoveValue(self->storage, key)){
						self->logError("Could not remove block hash index reference from database.");
						free(txHashes);
						return CB_BLOCK_STATUS_ERROR_BAD_DATA;
					}
				}
				// Change block keys from earliest branch to dependent branch
				uint8_t newKey[7];
				newKey[0] = 6;
				newKey[1] = CB_STORAGE_BLOCK;
				newKey[2] = mergeBranch;
				key[0] = 6;
				key[1] = CB_STORAGE_BLOCK;
				key[2] = earliestBranch;
				for (uint8_t x = 0; x < highestDependency; x++) {
					CBInt32ToArray(key, 3, x);
					CBInt32ToArray(newKey, 3, x);
					if (NOT CBBlockChainStorageChangeKey(self->storage, key, newKey)) {
						self->logError("Could not change a block key for merging two branches.");
						free(txHashes);
						return CB_BLOCK_STATUS_ERROR_BAD_DATA;
					}
				}
				// Make parent information for dependent branch reflect the earliest branch
				self->branches[mergeBranch].numBlocks += self->branches[earliestBranch].numBlocks;
				self->branches[mergeBranch].parentBlockIndex = self->branches[earliestBranch].parentBlockIndex;
				self->branches[mergeBranch].parentBranch = self->branches[earliestBranch].parentBranch;
				self->branches[mergeBranch].startHeight = self->branches[earliestBranch].startHeight;
				// Write updated branch information to storage
				CBInt32ToArray(data, 0, self->branches[mergeBranch].numBlocks);
				CBInt32ToArray(data, 4, self->branches[mergeBranch].parentBlockIndex);
				data[8] = self->branches[mergeBranch].parentBranch;
				CBInt32ToArray(data, 9, self->branches[mergeBranch].startHeight);
				key[0] = 2;
				key[1] = CB_STORAGE_BRANCH_INFO;
				key[2] = mergeBranch;
				if (NOT CBBlockChainStorageWriteValue(self->storage, key, data, 13, CB_BRANCH_NUM_BLOCKS, 21)) {
					self->logError("Could not write the updated branch information for merging two branches together into the block chain database.");
					free(txHashes);
					return CB_BLOCK_STATUS_ERROR_BAD_DATA;
				}
				// Find all of the other dependent branches for the earliest branch and update the parent branch information.
				for (uint8_t x = 0; x < self->numBranches; x++) {
					if (x != mergeBranch && self->branches[x].parentBranch == earliestBranch) {
						self->branches[x].parentBranch = mergeBranch;
						// Update in storage
						key[2] = x;
						if (NOT CBBlockChainStorageWriteValue(self->storage, key, &self->branches[x].parentBranch, 1, CB_BRANCH_PARENT_BRANCH, 21)) {
							self->logError("Could not write the updated parent branch from the overwritten branch during a merge into the block chain database.");
							free(txHashes);
							return CB_BLOCK_STATUS_ERROR_BAD_DATA;
						}
					}
				}
				// Find all of the dependent branches for the dependent branch and update the parent block index.
				for (uint8_t x = 0; x < self->numBranches; x++) {
					if (self->branches[x].parentBranch == mergeBranch) {
						self->branches[x].parentBlockIndex += self->branches[earliestBranch].numBlocks;
						// Update in storage
						key[2] = x;
						CBInt32ToArray(data, 0, self->branches[x].parentBlockIndex);
						if (NOT CBBlockChainStorageWriteValue(self->storage, key, data, 4, CB_BRANCH_PARENT_BLOCK_INDEX, 21)) {
							self->logError("Could not write a updated parent block index during a merge into the block chain database.");
							free(txHashes);
							return CB_BLOCK_STATUS_ERROR_BAD_DATA;
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
			key[0] = 1;
			key[1] = CB_STORAGE_VALIDATOR_INFO;
			if (NOT CBBlockChainStorageWriteValue(self->storage, key, &self->numBranches, 1, CB_VALIDATION_NUM_BRANCHES, 8)) {
				self->logError("Could not write the new number of branches into the block chain database.");
				free(txHashes);
				return CB_BLOCK_STATUS_ERROR_BAD_DATA;
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
				return CB_BLOCK_STATUS_ERROR_BAD_DATA;
			}
			CBBigIntEqualsSubtractionByBigInt(&self->branches[branch].work, &tempWork);
			free(tempWork.data); 
		}
		self->branches[branch].lastValidation = CB_NO_VALIDATION;
		self->branches[branch].startHeight = self->branches[prevBranch].startHeight + prevBlockIndex + 1;
		// Write branch info
		key[0] = 2;
		key[1] = CB_STORAGE_BRANCH_INFO;
		key[2] = branch;
		CBInt32ToArray(data, CB_BRANCH_LAST_RETARGET, self->branches[branch].lastRetargetTime);
		CBInt32ToArray(data, CB_BRANCH_LAST_VALIDATION, self->branches[branch].lastValidation);
		CBInt32ToArray(data, CB_BRANCH_NUM_BLOCKS, self->branches[branch].numBlocks);
		CBInt32ToArray(data, CB_BRANCH_PARENT_BLOCK_INDEX, self->branches[branch].parentBlockIndex);
		data[CB_BRANCH_PARENT_BRANCH] = self->branches[branch].parentBlockIndex;
		CBInt32ToArray(data, CB_BRANCH_START_HEIGHT, self->branches[branch].startHeight);
		if (NOT CBBlockChainStorageWriteValue(self->storage, key, &self->numBranches, 21, 0, 21)) {
			self->logError("Could not write new branch data into the block chain database.");
			free(txHashes);
			return CB_BLOCK_STATUS_ERROR_BAD_DATA;
		}
		// Write branch work
		key[1] = CB_STORAGE_WORK;
		if (NOT CBBlockChainStorageWriteValue(self->storage, key, self->branches[branch].work.data, self->branches[branch].work.length, 0, self->branches[branch].work.length)) {
			self->logError("Could not write new branch work into the block chain database.");
			free(txHashes);
			return CB_BLOCK_STATUS_ERROR_BAD_DATA;
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
			key[0] = 6;
			key[1] = CB_STORAGE_BLOCK;
			key[2] = branch;
			CBInt32ToArray(key, 3, self->branches[branch].numBlocks - 1);
			if (NOT CBBlockChainStorageReadValue(self->storage, key, data, 4, CB_BLOCK_TARGET)) {
				self->logError("Could not read a block target from the block chain database for an orphan.");
				return CB_BLOCK_STATUS_ERROR;
			}
			// Process into the branch.
			CBBlockStatus orphanStatus = CBFullValidatorProcessIntoBranch(self, self->orphans[x], networkTime, branch, branch, self->branches[branch].numBlocks - 1, CBArrayToInt32(data, 0), txHashes);
			if (orphanStatus == CB_BLOCK_STATUS_ERROR_BAD_DATA) {
				self->logError("There was an error when processing an orphan into a branch.");
				return CB_BLOCK_STATUS_ERROR_BAD_DATA;
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
	uint8_t key[6];
	uint8_t data[4];
	// Check timestamp
	if (block->time <= CBFullValidatorGetMedianTime(self, prevBranch, prevBlockIndex))
		return CB_BLOCK_STATUS_BAD;
	uint32_t target;
	bool change = NOT ((self->branches[prevBranch].startHeight + prevBlockIndex + 1) % 2016);
	if (change)
		// Difficulty change for this branch
		target = CBCalculateTarget(prevBlockTarget, block->time - self->branches[branch].lastRetargetTime);
	else
		target = prevBlockTarget;
	// Check target
	if (block->target != target)
		return CB_BLOCK_STATUS_BAD;
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
				return CB_BLOCK_STATUS_ERROR_BAD_DATA;
			// Commit data
			if (NOT CBBlockChainStorageCommitData(self->storage)) {
				self->logError("Could not commit a new block to a side chain.");
				return CB_BLOCK_STATUS_ERROR_BAD_DATA;
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
						for (;;tempBlockIndex--) {
							// Change indicies
							if (NOT CBFullValidatorUpdateUnspentOutputsAndLoad(self, tempBranch, tempBlockIndex, false)){
								self->logError("Could not reverse indicies for unspent outputs and transactions during reorganisation.");
								return CB_BLOCK_STATUS_ERROR_BAD_DATA;
							}
							if (tempBlockIndex == lastBlocks[x])
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
					return CB_BLOCK_STATUS_ERROR_BAD_DATA;
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
						return CB_BLOCK_STATUS_ERROR_BAD_DATA;
					}
				}
				break;
			}else{
				// Go up to last block
				for (;tempBlockIndex <= lastBlocks[pathIndex]; tempBlockIndex++) {
					if (NOT CBFullValidatorUpdateUnspentOutputsAndLoad(self, branchPath[pathIndex], tempBlockIndex, true)){
						self->logError("Could not update indicies for going to the last block for a path during reorganisation.");
						return CB_BLOCK_STATUS_ERROR_BAD_DATA;
					}
				}
				if (pathIndex == CB_MAX_BRANCH_CACHE - 1)
					// Done
					break;
				// Reset block index for next branch
				tempBlockIndex = 0;
			}
		}
		// At this point outputChangeBlockIndex is the block we start at and branchPath[pathIndex] is the branch.
		// Now validate all blocks going up.
		uint8_t * txHashes2 = NULL;
		uint32_t txHashes2AllocSize = 0;
		bool atLeastOne = false; // True when at least one block passed validation for a branch
		uint8_t firstBranch = tempBranch; // Saving the first branch when going back an updating last validated blocks.
		for (;;) {
			// Get block to validate
			CBBlock * tempBlock = CBFullValidatorLoadBlock(self, tempBlockIndex, tempBranch);
			if (NOT tempBlock){
				free(txHashes2);
				return CB_BLOCK_STATUS_ERROR;
			}
			if (NOT CBBlockDeserialise(tempBlock, true)){
				free(txHashes2);
				return CB_BLOCK_STATUS_ERROR_BAD_DATA;
			}
			// Get transaction hashes
			if (txHashes2AllocSize < tempBlock->transactionNum * 32) {
				txHashes2AllocSize = tempBlock->transactionNum * 32;
				uint8_t * temp = realloc(txHashes2, txHashes2AllocSize);
				if (NOT temp){
					free(txHashes2);
					return CB_BLOCK_STATUS_ERROR_BAD_DATA;
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
					CBResetBlockChainStorage(self->storage);
					// Save last valided blocks for each branch
					if (NOT CBFullValidatorSaveLastValidatedBlocks(self, firstBranch, tempBranch)) {
						self->logError("Could not save the last validated blocks for a validatation error during reorganisation");
						return CB_BLOCK_STATUS_ERROR_BAD_DATA;
					}
				}
				if (res == CB_BLOCK_VALIDATION_BAD)
					return CB_BLOCK_STATUS_BAD;
				if (res == CB_BLOCK_VALIDATION_ERR)
					return CB_BLOCK_STATUS_ERROR_BAD_DATA;
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
			return CB_BLOCK_STATUS_ERROR_BAD_DATA;
		}
		// Now we validate the block for the new main chain.
	}
	// We are just validating a new block on the main chain.
	CBBlockValidationResult res = CBFullValidatorCompleteBlockValidation(self, branch, block, txHashes, self->branches[branch].startHeight + self->branches[branch].numBlocks);
	switch (res) {
		case CB_BLOCK_VALIDATION_BAD:
			return CB_BLOCK_STATUS_BAD;
		case CB_BLOCK_VALIDATION_ERR:
			return CB_BLOCK_STATUS_ERROR;
		case CB_BLOCK_VALIDATION_OK:
			// Update branch and unspent outputs.
			if (NOT CBFullValidatorAddBlockToBranch(self, branch, block, work)){
				self->logError("There was an error when adding a new block to the branch.");
				return CB_BLOCK_STATUS_ERROR_BAD_DATA;
			}
			self->branches[branch].lastValidation = self->branches[branch].numBlocks;
			// Update last validation for branch and main branch
			key[0] = 5;
			key[1] = CB_STORAGE_BRANCH_INFO;
			key[2] = branch;
			CBInt32ToArray(data, 0, self->branches[branch].numBlocks);
			if (NOT CBBlockChainStorageWriteValue(self->storage, key, data, 4, CB_BRANCH_LAST_VALIDATION, 21)) {
				self->logError("Could not write the last validated block for a branch when adding a new block to the main chain.");
				return CB_BLOCK_STATUS_ERROR_BAD_DATA;
			}
			if (branch != self->mainBranch) {
				self->mainBranch = branch;
				key[0] = 1;
				key[1] = CB_STORAGE_VALIDATOR_INFO;
				if (NOT CBBlockChainStorageWriteValue(self->storage, key, &branch, 1, CB_VALIDATION_MAIN_BRANCH, 8)) {
					self->logError("Could not write the new main branch when adding a new block to the main chain.");
					return CB_BLOCK_STATUS_ERROR_BAD_DATA;
				}
			}
			// Finally we can commit the remaining data to storage
			if (NOT CBBlockChainStorageCommitData(self->storage)) {
				self->logError("Could not commit updated data when adding a new block to the main chain.");
				return CB_BLOCK_STATUS_ERROR_BAD_DATA;
			}
			return CB_BLOCK_STATUS_MAIN;
	}
}
bool CBFullValidatorSaveLastValidatedBlocks(CBFullValidator * self, uint8_t startBranch, uint8_t endBranch){
	uint8_t key[6];
	uint8_t data[4];
	key[0] = 5;
	key[1] = CB_STORAGE_BRANCH_INFO;
	for (;;) {
		key[2] = endBranch;
		CBInt32ToArray(data, 0, self->branches[endBranch].lastValidation);
		if (NOT CBBlockChainStorageWriteValue(self->storage, key, data, 4, CB_BRANCH_LAST_VALIDATION, 21)) {
			self->logError("Could not write the last validated block for a branch.");
			return false;
		}
		if (endBranch == startBranch)
			break;
		endBranch = self->branches[endBranch].parentBranch;
	}
	if (NOT CBBlockChainStorageCommitData(self->storage)) {
		self->logError("Could not commit the last validated block.");
		return false;
	}
	return true;
}
bool CBFullValidatorUpdateUnspentOutputs(CBFullValidator * self, CBBlock * block, uint8_t branch, uint32_t blockIndex, bool forward){
	// Update unspent outputs... Go through transactions, removing the prevOut references and adding the outputs for one transaction at a time.
	uint8_t key[43];
	uint8_t data[14];
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
				// Place transaction hash in key
				memcpy(key + 2, CBByteArrayGetData(block->transactions[x]->inputs[y]->prevOut.hash), 32);
				// Place output index in key
				CBInt32ToArray(key, 33, block->transactions[x]->inputs[y]->prevOut.index);
				if (forward){
					// Make key for the unspent output
					key[0] = 37;
					key[1] = CB_STORAGE_UNSPENT_OUTPUT;
					// Remove from storage
					if (NOT CBBlockChainStorageRemoveValue(self->storage, key)) {
						self->logError("Could not remove an unspent output reference from storage.");
						free(txReadData);
						return false;
					}
				}else{
					// Add to storage
					// Find output position in transaction
					key[0] = 33;
					key[1] = CB_STORAGE_TRANSACTION_INDEX;
					// Already have the transaction hash in the key
					if (NOT CBBlockChainStorageReadValue(self->storage, key, data, 14, 0)) {
						self->logError("Could not read a transaction reference from the transaction index.");
						free(txReadData);
						return false;
					}
					// Get transaction to find position for output in the block
					uint8_t blockKey[7];
					blockKey[0] = 6;
					blockKey[1] = CB_STORAGE_BLOCK;
					blockKey[2] = data[CB_TRANSACTION_REF_BRANCH];
					memcpy(blockKey + 3, data + CB_TRANSACTION_REF_BLOCK_INDEX, 4);
					// Reallocate transaction data memory if needed.
					if (CBArrayToInt32(data, CB_TRANSACTION_REF_LENGTH_OUTPUTS) > txReadDataSize) {
						txReadDataSize = CBArrayToInt32(data, CB_TRANSACTION_REF_LENGTH_OUTPUTS);
						txReadData = realloc(txReadData,txReadDataSize);
						if (NOT txReadData) {
							self->logError("Could not allocate memory for reading a transaction.");
							free(txReadData);
							return false;
						}
					}
					// Read transaction from the block
					if (NOT CBBlockChainStorageReadValue(self->storage, blockKey, txReadData, CBArrayToInt32(data, CB_TRANSACTION_REF_LENGTH_OUTPUTS), CBArrayToInt32(data, CB_TRANSACTION_REF_POSITION_OUPTUTS))) {
						self->logError("Could not read a transaction from the block-chain database.");
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
					// Write to database
					CBInt32ToArray(data, CB_UNSPENT_OUTPUT_REF_POSITION, CBArrayToInt32(data, CB_TRANSACTION_REF_POSITION_OUPTUTS) + txCursor);
					if (NOT CBBlockChainStorageWriteValue(self->storage, key, data, 4, 0, 4)) {
						self->logError("Could not write unspent output reference into the database when going backwards during re-organisation.");
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
		// Make key
		key[0] = 37;
		key[1] = CB_STORAGE_UNSPENT_OUTPUT;
		memcpy(key + 2, CBTransactionGetHash(block->transactions[x]), 32);
		// For adding the size of outputs
		uint32_t outputsSize = cursor;
		for (uint32_t y = 0; y < block->transactions[x]->outputNum; y++) {
			// Add index to key
			CBInt32ToArray(key, 33, y);
			if (forward) {
				// Create new unspent output reference data
				CBInt32ToArray(data, CB_UNSPENT_OUTPUT_REF_POSITION, cursor + 20); // Plus twenty for the block hash
				// Add to storage
				if (NOT CBBlockChainStorageWriteValue(self->storage, key, data, 4, 0, 4)) {
					self->logError("Could not write new unspent output reference into the database.");
					free(txReadData);
					return false;
				}
			}else{
				// Remove output from storage
				if (NOT CBBlockChainStorageRemoveValue(self->storage, key)) {
					self->logError("Could not remove unspent output reference from the database during re-organisation.");
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
		outputsSize = cursor - outputsSize;
		// Get key for transaction index
		key[0] = 33;
		key[1] = CB_STORAGE_TRANSACTION_INDEX;
		// Already added transaction hash to key
		if (forward) {
			// Add transaction to transaction index
			CBInt32ToArray(data, CB_TRANSACTION_REF_BLOCK_INDEX, blockIndex);
			data[CB_TRANSACTION_REF_BRANCH] = branch;
			CBInt32ToArray(data, CB_TRANSACTION_REF_POSITION_OUPTUTS, cursor + 20); // Plus twenty for the block hash
			CBInt32ToArray(data, CB_TRANSACTION_REF_LENGTH_OUTPUTS, outputsSize);
			data[CB_TRANSACTION_REF_IS_COINBASE] = x == 0;
			if (NOT CBBlockChainStorageWriteValue(self->storage, key, data, 13, 0, 13)) {
				self->logError("Could not write transaction reference to transaction index.");
				free(txReadData);
				return false;
			}
		}else
			// Remove transaction from transaction index.
			if (CBBlockChainStorageRemoveValue(self->storage, key)) {
				self->logError("Could not remove transaction reference from transaction index.");
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
	CBBlock * block = CBFullValidatorLoadBlock(self, blockIndex, branch);
	if (NOT block) {
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
