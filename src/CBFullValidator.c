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

/*CBFullValidator * CBNewFullValidator(uint64_t storage, bool * badDataBase, void (*logError)(char *,...)){
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
	// Create block index
	if (NOT CBInitAssociativeArray(&self->blockIndex, 20)){
		logError("There was an error when initialising the block index.");
		self->blockIndex.root = NULL;
		self->outputIndex.root = NULL;
		return false;
	}
	if (NOT CBInitAssociativeArray(&self->outputIndex, 32)){
		logError("There was an error when reading the output index.");
		self->outputIndex.root = NULL;
		return false;
	}
	// Check whether the database has been created.
	uint8_t key[8] = {CB_STORAGE_VALIDATOR_INFO,0,0,0,0,0};
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
		key[0] = CB_STORAGE_ORPHAN;
		for (uint8_t x = 0; x < numOrphansTemp; x++) {
			key[1] = x;
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
			key[0] = CB_STORAGE_BRANCH_INFO;
			key[1] = x;
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
			key[0] = CB_STORAGE_WORK;
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
			// Create chronological index
			self->branches[x].chronoBlockRefs = malloc(sizeof(*self->branches[x].chronoBlockRefs) * self->branches[x].numBlocks);
			if (NOT self->branches[x].chronoBlockRefs) {
				logError("There was an error when allocated data for a branch's chronological block index.");
				free(self->branches[x].work.data);
				return false;
			}
			self->numBranches++;
			// Get block index data
			key[0] = CB_STORAGE_BLOCK;
			for (uint32_t y = 0; y < self->branches[x].numBlocks; y++) {
				CBInt32ToArray(key, 2, y);
				// Get the hash
				CBBlockReference * ref = malloc(sizeof(*ref));
				if (NOT ref) {
					logError("Could not allocate memory for a block reference.");
					return false;
				}
				if (NOT CBBlockChainStorageReadValue(storage, key, ref->hash, 20, CB_BLOCK_HASH)) {
					logError("There was an error when reading the hash for a block.");
					free(ref);
					return false;
				}
				// Get the time and target
				if (NOT CBBlockChainStorageReadValue(storage, key, data, 8, CB_BLOCK_TIME)) {
					logError("There was an error when reading the time and target for a block.");
					free(ref);
					return false;
				}
				ref->time = CBArrayToInt32(data, 0);
				ref->target = CBArrayToInt32(data, 4);
				ref->branch = x;
				ref->index = y;
				// Insert into indices
				if (NOT CBAssociativeArrayInsert(&self->blockIndex, ref->hash, CBAssociativeArrayFind(&self->blockIndex, ref->hash), NULL)){
					logError("There was an error when inserting a block reference into the index.");
					free(ref);
					return false;
				}
				self->branches[x].chronoBlockRefs[y] = ref;
			}
		}
		// Get unspent output information
		key[0] = CB_STORAGE_UNSPENT_OUTPUT;
		for (uint32_t x = 0; x < self->numUnspentOutputs; x++) {
			CBInt32ToArray(key, 1, x);
			key[5] = 0;
			CBOutputReference * ref = malloc(sizeof(*ref));
			if (NOT ref) {
				logError("Could not allocate memory for an unspent output reference.");
				return false;
			}
			if (NOT CBBlockChainStorageReadValue(storage, key, ref->outputHash, 32, 0)) {
				logError("There was an error when reading unspent output information.");
				free(ref);
				return false;
			}
			if (NOT CBBlockChainStorageReadValue(storage, key, data, 14, 0)) {
				logError("There was an error when reading unspent output information.");
				free(ref);
				return false;
			}
			ref->blockIndex = CBArrayToInt32(data, CB_UNSPENT_OUTPUT_BLOCK_INDEX);
			ref->branch = data[CB_UNSPENT_OUTPUT_BRANCH];
			ref->coinbase = data[CB_UNSPENT_OUTPUT_COINBASE];
			ref->outputIndex = CBArrayToInt32(data, CB_UNSPENT_OUTPUT_OUTPUT_INDEX);
			ref->position = CBArrayToInt32(data, CB_UNSPENT_OUTPUT_POSITION);
			ref->storageID = x;
			// Insert into index
			if (NOT CBAssociativeArrayInsert(&self->outputIndex, ref->outputHash, CBAssociativeArrayFind(&self->outputIndex, ref->outputHash), NULL)){
				logError("There was an error when inserting an unspent output reference into the index.");
				free(ref);
				return false;
			}
		}
	}else{
		// Write basic validator information to database
		data[CB_VALIDATION_FIRST_ORPHAN] = 0;
		data[CB_VALIDATION_NUM_ORPHANS] = 0;
		data[CB_VALIDATION_MAIN_BRANCH] = 0;
		data[CB_VALIDATION_NUM_BRANCHES] = 1;
		CBInt32ToArray(data, CB_VALIDATION_NUM_UNSPENT_OUTPUTS, 0);
		key[0] = CB_STORAGE_VALIDATOR_INFO;
		memset(key + 1, 0, 5);
		if (NOT CBBlockChainStorageWriteValue(storage, key, data, 8, 0, 8)) {
			logError("Could not write the initial basic validation data.");
			return false;
		}
		// Write the genesis block
		key[0] = CB_STORAGE_BLOCK;
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
			logError("Could not write the genesis lock to the block chain database.");
			return false;
		}
		// Write branch data
		key[0] = CB_STORAGE_BRANCH_INFO;
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
		key[0] = CB_STORAGE_WORK;
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
		// Create block reference
		CBBlockReference * ref = malloc(sizeof(*ref));
		if (NOT ref){
			logError("Could not allocate memory for the genesis block reference.");
			CBResetBlockChainStorage(storage);
			self->branches[0].chronoBlockRefs = NULL; // To avoid free() errors.
			self->branches[0].work.data = NULL;
			return false;
		}
		uint8_t hash[20] = {0x6F,0xE2,0x8C,0x0A,0xB6,0xF1,0xB3,0x72,0xC1,0xA6,0xA2,0x46,0xAE,0x63,0xF7,0x4F,0x93,0x1E,0x83,0x65};
		memcpy(ref->hash,hash,20);
		ref->time = 1231006505;
		ref->target = CB_MAX_TARGET;
		ref->branch = 0;
		ref->index = 0;
		// Create chronological block index
		self->branches[0].chronoBlockRefs = malloc(sizeof(*self->branches[0].chronoBlockRefs));
		if (NOT self->branches[0].chronoBlockRefs) {
			logError("Could not allocate memory for the initial chronological block index.");
			CBResetBlockChainStorage(storage);
			self->branches[0].work.data = NULL;
			return false;
		}
		// Insert reference into indices
		self->branches[0].chronoBlockRefs[0] = ref;
		if (NOT CBAssociativeArrayInsert(&self->blockIndex, ref->hash, CBAssociativeArrayFind(&self->blockIndex, ref->hash), NULL)) {
			logError("Could not insert the genesis block reference into the block hash index.");
			CBResetBlockChainStorage(storage);
			self->branches[0].work.data = NULL;
			return false;
		}
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
	for (uint8_t x = 0; x < self->numBranches; x++){
		free(self->branches[x].work.data);
		free(self->branches[x].chronoBlockRefs);
	}
	// Release indices
	if (self->blockIndex.root)
		CBFreeAssociativeArray(&self->blockIndex, true);
	if (self->outputIndex.root)
		CBFreeAssociativeArray(&self->outputIndex, true);
	CBFreeObject(self);
}

//  Functions

bool CBFullValidatorAddBlockToBranch(CBFullValidator * self, uint8_t branch, CBBlock * block, CBBigInt work){
	// The keys for storage
	uint8_t key[6];
	// Create block reference
	CBBlockReference * ref = malloc(sizeof(*ref));
	if (NOT ref) {
		self->logError("Could not allocate a block reference for branch %u.", branch);
		return false;
	}
	// Add data to block reference
	ref->branch = branch;
	memcpy(ref->hash, CBBlockGetHash(block), 20);
	ref->index = self->branches[branch].numBlocks++; // Increase number of blocks.
	ref->target = block->target;
	ref->time = block->time;
	// Insert the new reference into the indices.
	if (NOT CBAssociativeArrayInsert(&self->blockIndex, ref->hash, CBAssociativeArrayFind(&self->blockIndex, ref->hash), NULL)) {
		self->logError("Could not insert a block reference into the block hash index for branch %u.", branch);
		free(ref);
		return false;
	}
	self->branches[branch].chronoBlockRefs = realloc(self->branches[branch].chronoBlockRefs, self->branches[branch].numBlocks);
	if (NOT self->branches[branch].chronoBlockRefs) {
		self->logError("Could not insert a block reference into the chronological block index for branch %u.", branch);
		return false;
	}
	self->branches[branch].chronoBlockRefs[ref->index] = ref;
	// Modify branch information
	uint8_t data[4];
	key[0] = CB_STORAGE_BRANCH_INFO;
	key[1] = branch;
	CBInt32ToArray(key, 2, 0);
	free(self->branches[branch].work.data);
	self->branches[branch].work = work;
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
	key[0] = CB_STORAGE_BLOCK;
	CBInt32ToArray(key, 2, ref->index);
	uint32_t blockLen = CBGetMessage(block)->bytes->length;
	if (NOT CBBlockChainStorageWriteValue(self->storage, key, ref->hash, 20, 0, 20 + blockLen)) {
		self->logError("Could not write the block hash for a new block to branch %u.", branch);
		return false;
	}
	if (NOT CBBlockChainStorageWriteValue(self->storage, key, CBByteArrayGetData(CBGetMessage(block)->bytes), blockLen, 20, 20 + blockLen)) {
		self->logError("Could not write the block data for a new block to branch %u.", branch);
		return false;
	}
	return true;
}
bool CBFullValidatorAddBlockToOrphans(CBFullValidator * self, CBBlock * block){
	// Save orphan.
	uint8_t pos;
	uint8_t key[2];
	if (self->numOrphans == CB_MAX_ORPHAN_CACHE) {
		// Release old orphan
		CBReleaseObject(self->orphans[self->firstOrphan]);
		pos = self->firstOrphan;
		self->firstOrphan++;
		key[0] = CB_STORAGE_VALIDATOR_INFO;
		if (NOT CBBlockChainStorageWriteValue(self->storage, key, &self->firstOrphan, 1, CB_VALIDATION_FIRST_ORPHAN, 1)){
			self->logError("There was an error writing the index of the first orphan.");
			return false;
		}
	}else{
		// Adding an orphan.
		pos = self->numOrphans++;
		key[0] = CB_STORAGE_VALIDATOR_INFO;
		if (NOT CBBlockChainStorageWriteValue(self->storage, key, &self->numOrphans, 1, CB_VALIDATION_NUM_ORPHANS, 1)){
			self->logError("There was an error writing the number of orphans.");
			return false;
		}
	}
	// Add to memory
	self->orphans[pos] = block;
	CBRetainObject(block);
	// Add to storage
	key[0] = CB_STORAGE_ORPHAN;
	key[1] = pos;
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
	// Look in block index
	if (CBAssociativeArrayFind(&self->blockIndex, hash).found)
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
			return self->branches[branch].chronoBlockRefs[prevIndex]->time;
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
		// Not found in this block. Look in unspent outputs index.
		CBFindResult res = CBAssociativeArrayFind(&self->outputIndex, CBByteArrayGetData(allSpentOutputs[transactionIndex][inputIndex].hash));
		if (NOT res.found)
			// No unspent outputs for this input.
			return CB_BLOCK_VALIDATION_BAD;
		CBOutputReference * outRef = (CBOutputReference *)res.node->elements[res.pos];
		// Check coinbase maturity
		if (outRef->coinbase && blockHeight - (self->branches[outRef->branch].startHeight + outRef->blockIndex) < CB_COINBASE_MATURITY)
			return CB_BLOCK_VALIDATION_BAD;
		// Get the output from storage
		uint8_t key[6];
		key[0] = CB_STORAGE_BLOCK;
		key[1] = outRef->branch;
		CBInt32ToArray(key, 2, outRef->blockIndex);
		// Deserialise output as we read from the database as we do not know the length
		uint8_t data[9];
		if (NOT CBBlockChainStorageReadValue(self->storage, key, data, 9, outRef->position)) {
			self->logError("Could not read block output first 9 bytes for branch %u and index %u.",outRef->branch, outRef->blockIndex);
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
			if (NOT CBBlockChainStorageReadValue(self->storage, key, scriptSizeBytes, varIntSize, outRef->position + 9)) {
				self->logError("Could not read block output varint data for branch %u and index %u.",outRef->branch, outRef->blockIndex);
				return CB_BLOCK_VALIDATION_ERR;
			}
			if (varIntSize == 2)
				scriptSize = CBArrayToInt16(scriptSizeBytes, 0);
			else
				scriptSize = CBArrayToInt32(scriptSizeBytes, 0);
		}
		// Read the script
		CBScript * script = CBNewScriptOfSize(scriptSize, self->logError);
		if (NOT CBBlockChainStorageReadValue(self->storage, key, CBByteArrayGetData(script), scriptSize, outRef->position + 9 + varIntSize)) {
			self->logError("Could not read block output script for branch %u and index %u.",outRef->branch, outRef->blockIndex);
			return CB_BLOCK_VALIDATION_ERR;
		}
		// Make the object for the output
		prevOut = CBNewTransactionOutput(CBArrayToInt64(data, 0), script, self->logError);
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
	uint8_t key[6];
	key[0] = CB_STORAGE_BLOCK;
	key[1] = branch;
	CBInt32ToArray(key, 2, blockID);
	uint32_t blockDataLen = CBBlockChainStorageGetLength(self->storage, key);
	if (NOT blockDataLen)
		return NULL;
	// Get block data
	CBByteArray * data = CBNewByteArrayOfSize(blockDataLen, self->logError);
	if (NOT data) {
		self->logError("Could not initialise a byte array for loading a block.");
		return NULL;
	}
	if (NOT CBBlockChainStorageReadValue(self->storage, key, CBByteArrayGetData(data), blockDataLen - 20, 20)){
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
	CBFindResult res = CBAssociativeArrayFind(&self->blockIndex, CBByteArrayGetData(block->prevBlockHash));
	if (res.found) {
		CBBlockReference * ref = (CBBlockReference *)res.node->elements[res.pos];
		prevBranch = ref->branch;
		prevBlockIndex = ref->index;
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
		uint8_t key[6];
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
				key[0] = CB_STORAGE_BLOCK;
				key[1] = earliestBranch;
				// Delete blocks after highest dependency
				for (uint32_t x = highestDependency; x < self->branches[earliestBranch].numBlocks; x++){
					// Remove from block hash index
					CBAssociativeArrayDelete(&self->blockIndex, CBAssociativeArrayFind(&self->blockIndex, self->branches[earliestBranch].chronoBlockRefs[x]->hash));
					// Free data
					free(self->branches[earliestBranch].chronoBlockRefs[x]);
					// Delete from storage also
					CBInt32ToArray(key, 2, x);
					if (NOT CBBlockChainStorageRemoveValue(self->storage, key)){
						self->logError("Could not remove block value from database.");
						free(txHashes);
						return CB_BLOCK_STATUS_ERROR_BAD_DATA;
					}
				}
				// Move the blocks from the dependent branch chrono index into the earliest branch chrono index.
				self->branches[earliestBranch].chronoBlockRefs = realloc(self->branches[earliestBranch].chronoBlockRefs, highestDependency + 1 + self->branches[mergeBranch].numBlocks);
				memcpy(self->branches[earliestBranch].chronoBlockRefs + highestDependency + 1, self->branches[mergeBranch].chronoBlockRefs, self->branches[mergeBranch].numBlocks);
				// Now free dependent branch chrono index
				free(self->branches[mergeBranch].chronoBlockRefs);
				// Make the earliest branch chrono index the dependent branch chrono index
				self->branches[mergeBranch].chronoBlockRefs = self->branches[earliestBranch].chronoBlockRefs;
				// Change block keys from earliest branch to dependent branch
				uint8_t newKey[6];
				newKey[0] = CB_STORAGE_BLOCK;
				newKey[1] = mergeBranch;
				for (uint8_t x = 0; x < highestDependency; x++) {
					CBInt32ToArray(key, 2, x);
					CBInt32ToArray(newKey, 2, x);
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
				key[0] = CB_STORAGE_BRANCH_INFO;
				key[1] = mergeBranch;
				CBInt32ToArray(key, 2, 0);
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
						key[1] = x;
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
						key[1] = x;
						CBInt32ToArray(data, 0, self->branches[x].parentBlockIndex);
						if (NOT CBBlockChainStorageWriteValue(self->storage, key, data, 4, CB_BRANCH_PARENT_BLOCK_INDEX, 21)) {
							self->logError("Could not write a updated parent block index during a merge into the block chain database.");
							free(txHashes);
							return CB_BLOCK_STATUS_ERROR_BAD_DATA;
						}
					}
				}
			}else
				// Free earleist branch chrono index
				free(self->branches[earliestBranch].chronoBlockRefs);
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
			key[0] = CB_STORAGE_VALIDATOR_INFO;
			memset(key, 0, 5);
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
			if (NOT CBCalculateBlockWork(&tempWork,self->branches[prevBranch].chronoBlockRefs[y]->target)){
				free(txHashes);
				return CB_BLOCK_STATUS_ERROR_BAD_DATA;
			}
			CBBigIntEqualsSubtractionByBigInt(&self->branches[branch].work, &tempWork);
			free(tempWork.data); 
		}
		self->branches[branch].lastValidation = CB_NO_VALIDATION;
		self->branches[branch].startHeight = self->branches[prevBranch].startHeight + prevBlockIndex + 1;
		// Write branch info
		key[0] = CB_STORAGE_BRANCH_INFO;
		key[1] = branch;
		CBInt32ToArray(key, 0, 0);
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
		key[0] = CB_STORAGE_WORK;
		if (NOT CBBlockChainStorageWriteValue(self->storage, key, self->branches[branch].work.data, self->branches[branch].work.length, 0, self->branches[branch].work.length)) {
			self->logError("Could not write new branch work into the block chain database.");
			free(txHashes);
			return CB_BLOCK_STATUS_ERROR_BAD_DATA;
		}
		// Commit new branch data to the database! Yay!
		if (NOT CBBlockChainStorageCommitData(self->storage)) {
			self->logError("Could not commit new branch data into the block chain database.");
			free(txHashes);
			return CB_BLOCK_STATUS_ERROR_BAD_DATA;
		}
	}
	// Got branch ready for block. Now process into the branch.
	status = CBFullValidatorProcessIntoBranch(self, block, networkTime, branch, prevBranch, prevBlockIndex, txHashes);
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
			// Process into the branch.
			CBBlockStatus orphanStatus = CBFullValidatorProcessIntoBranch(self, self->orphans[x], networkTime, branch, branch, self->branches[branch].numBlocks - 1, txHashes);
			if (orphanStatus == CB_BLOCK_STATUS_ERROR)
				break;
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
CBBlockStatus CBFullValidatorProcessIntoBranch(CBFullValidator * self, CBBlock * block, uint64_t networkTime, uint8_t branch, uint8_t prevBranch, uint32_t prevBlockIndex, uint8_t * txHashes){
	uint8_t key[6];
	uint8_t data[4];
	key[0] = CB_STORAGE_BRANCH_INFO;
	CBInt32ToArray(key, 2, 0);
	// Check timestamp
	if (block->time <= CBFullValidatorGetMedianTime(self, prevBranch, prevBlockIndex))
		return CB_BLOCK_STATUS_BAD;
	uint32_t target;
	bool change = NOT ((self->branches[prevBranch].startHeight + prevBlockIndex + 1) % 2016);
	if (change)
		// Difficulty change for this branch
		target = CBCalculateTarget(self->branches[prevBranch].chronoBlockRefs[prevBlockIndex]->target, block->time - self->branches[branch].lastRetargetTime);
	else
		target = self->branches[prevBranch].chronoBlockRefs[prevBlockIndex]->target;
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
			return CB_BLOCK_STATUS_SIDE;
		}
		// Potential block-chain reorganisation. Validate the side branch. THIS NEEDS TO START AT THE FIRST BRANCH BACK WHERE VALIDATION IS NOT COMPLETE AND GO UP THROUGH THE BLOCKS VALIDATING THEM. Includes Prior Branches!
		uint8_t tempBranch = prevBranch;
		uint32_t lastBlocks[CB_MAX_BRANCH_CACHE - 1]; // Used to store the last blocks in each branch going to the branch we are validating for.
		uint8_t lastBlocksIndex = CB_MAX_BRANCH_CACHE - 1; // The starting point of lastBlocks. If (CB_MAX_BRANCH_CACHE - 1) then we are only validating the branch we added to.
		uint8_t branches[CB_MAX_BRANCH_CACHE - 1]; // Branches to go to after the last blocks.
		// Go back until we find the branch with validated blocks in it.
		uint32_t tempBlockIndex;
		for (;;){
			if (self->branches[tempBranch].lastValidation == CB_NO_VALIDATION){
				// No validation on this branch
				if (self->branches[self->branches[tempBranch].parentBranch].lastValidation >= self->branches[tempBranch].parentBlockIndex) {
					// Parent fully validated upto last index, start at begining of the this branch.
					tempBlockIndex = 0;
					break;
				}
				branches[--lastBlocksIndex] = tempBranch;
				lastBlocks[lastBlocksIndex] = self->branches[tempBranch].parentBlockIndex;
				tempBranch = self->branches[tempBranch].parentBranch;
			}else{
				// Not fully validated. Start at last validation plus one.
				tempBlockIndex = self->branches[tempBranch].lastValidation + 1;
				break;
			}
		}
		// Get all parents
		uint8_t i = lastBlocksIndex;
		uint8_t b = tempBranch;
		while (self->branches[b].startHeight) {
			branches[--i] = b;
			lastBlocks[i] = self->branches[b].parentBlockIndex;
			b = self->branches[b].parentBranch;
		}
		// Get unspent output index for validating the potential new main chain
		CBOutputReference ** addedOutputs;
		CBOutputReference ** removedOutputs;
		uint8_t outputChangeBranch = self->mainBranch;
		uint32_t outputChainBlockIndex = self->branches[self->mainBranch].numBlocks - 1;
		// Go backwards towards fork
		while (true) {
			// See if the branch is in the branch path for the potetial new chain.
			for (uint8_t x = i; x < lastBlocksIndex; x++) {
				if (branches[x] == outputChangeBranch) {
					// Go down to fork point, unless this is the fork point
					if (outputChainBlockIndex > lastBlocks[x]) {
						// Above fork point, go down to fork point
						
					}else
						// Reached fork point
						break;
				}
			}
			// We need to go down to next branch
			
		}
		// Go upwards to last validation point
		
		// Now validate all blocks going up.
		uint8_t * txHashes2 = NULL;
		uint32_t txHashes2AllocSize = 0;
		bool atLeastOne = false; // True when at least one block passed validation for a branch
		while (tempBlockIndex != self->branches[branch].numBlocks || tempBranch != branch) {
			// Get block
			CBBlock * tempBlock = CBFullValidatorLoadBlock(self, self->branches[tempBranch].chronoBlockRefs[tempBlockIndex]->index, tempBranch);
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
				free(txHashes2);
				CBReleaseObject(tempBlock);
				if (atLeastOne) {
					// Save last OK validation before returning
					key[1] = tempBranch;
					CBInt32ToArray(data, 0, self->branches[tempBranch].lastValidation);
					if (NOT CBBlockChainStorageWriteValue(self->storage, key, data, 4, CB_BRANCH_LAST_VALIDATION, 21)) {
						self->logError("Could not write the last validated block for a branch during reorganisation and a validation error.");
						return CB_BLOCK_STATUS_ERROR;
					}
					if (NOT CBBlockChainStorageCommitData(self->storage)) {
						self->logError("Could not commit the last validated block for a branch during reorganisation and a validation error.");
						return CB_BLOCK_STATUS_ERROR_BAD_DATA;
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
			if (tempBlockIndex == lastBlocks[lastBlocksIndex]) {
				// Came to the last block in the branch
				// Update last validation information
				key[1] = tempBranch;
				CBInt32ToArray(data, 0, tempBlockIndex);
				if (NOT CBBlockChainStorageWriteValue(self->storage, key, data, 4, CB_BRANCH_LAST_VALIDATION, 21)) {
					self->logError("Could not write the last validated block for a branch during reorganisation.");
					return CB_BLOCK_STATUS_ERROR;
				}
				if (NOT CBBlockChainStorageCommitData(self->storage)) {
					self->logError("Could not commit the last validated block for a branch during reorganisation.");
					return CB_BLOCK_STATUS_ERROR_BAD_DATA;
				}
				// Go to next branch
				tempBranch = branches[lastBlocksIndex];
				tempBlockIndex = 0;
				lastBlocksIndex++;
				atLeastOne = false;
			}else
				tempBlockIndex++;
			CBReleaseObject(tempBlock);
		}
		free(txHashes2);
		// Now we validate the block for the new main chain.
	}
	// We are just validating a new block on the main chain
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
			key[1] = branch;
			CBInt32ToArray(data, 0, self->branches[branch].numBlocks);
			if (NOT CBBlockChainStorageWriteValue(self->storage, key, data, 4, CB_BRANCH_LAST_VALIDATION, 21)) {
				self->logError("Could not write the last validated block for a branch when adding a new block to the main chain.");
				return CB_BLOCK_STATUS_ERROR;
			}
			if (branch != self->mainBranch) {
				self->mainBranch = branch;
				key[0] = CB_STORAGE_VALIDATOR_INFO;
				key[1] = 0;
				if (NOT CBBlockChainStorageWriteValue(self->storage, key, &branch, 1, CB_VALIDATION_MAIN_BRANCH, 8)) {
					self->logError("Could not write the new main branch when adding a new block to the main chain.");
					return CB_BLOCK_STATUS_ERROR;
				}
			}
			if (NOT CBBlockChainStorageCommitData(self->storage)) {
				self->logError("Could not commit updated data when adding a new block to the main chain.");
				return CB_BLOCK_STATUS_ERROR_BAD_DATA;
			}
			return CB_BLOCK_STATUS_MAIN;
	}
}
bool CBFullValidatorUpdateUnspentOutputs(CBFullValidator * self, CBBlock * block, uint8_t branch, uint32_t blockIndex){
	// Update unspent outputs... Go through transactions, removing the prevOut references and adding the outputs for one transaction at a time.
	uint8_t key[6];
	uint8_t data[15];
	key[0] = CB_STORAGE_UNSPENT_OUTPUT;
	key[5] = 0;
	uint32_t cursor = 80; // Cursor to find output positions.
	uint8_t byte = CBByteArrayGetByte(CBGetMessage(block)->bytes, 80);
	cursor += byte < 253 ? 1 : (byte == 253 ? 2 : (byte == 254 ? 4 : 8));
	for (uint32_t x = 0; x < block->transactionNum; x++) {
		cursor += 4; // Move along version number
		// Move along input number
		byte = CBByteArrayGetByte(CBGetMessage(block)->bytes, cursor);
		cursor += byte < 253 ? 1 : (byte == 253 ? 2 : (byte == 254 ? 4 : 8));
		// First remove output references than add new outputs.
		for (uint32_t y = 0; y < block->transactions[x]->inputNum; y++) {
			if (x) {
				// Only remove for non-coinbase transactions
				CBFindResult res = CBAssociativeArrayFind(&self->outputIndex, CBByteArrayGetData(block->transactions[x]->inputs[y]->prevOut.hash));
				CBOutputReference * ref = (CBOutputReference *)res.node->elements[res.pos];
				// Remove
				CBAssociativeArrayDelete(&self->outputIndex, res);
				free(ref);
				// Remove from storage
				CBInt32ToArray(key, 1, ref->storageID);
				if (NOT CBBlockChainStorageRemoveValue(self->storage, key)) {
					self->logError("Could not remove an unspent output reference from storage.");
					return false;
				}
			}
			// Move cursor along script varint. We look at byte data in case it is longer than needed.
			cursor += 36; // Output reference
			// Var int
			uint8_t byte = CBByteArrayGetByte(CBGetMessage(block)->bytes, cursor);
			cursor += byte < 253 ? 1 : (byte == 253 ? 2 : (byte == 254 ? 4 : 8));
			// Move along script
			cursor += block->transactions[x]->inputs[y]->scriptObject->length;
			// Move cursor along sequence
			cursor += 4;
		}
		// Move cursor past output number to first output
		byte = CBByteArrayGetByte(CBGetMessage(block)->bytes, cursor);
		cursor += byte < 253 ? 1 : (byte == 253 ? 2 : (byte == 254 ? 4 : 8));
		// Now add new outputs
		for (uint32_t y = 0; y < block->transactions[x]->outputNum; y++) {
			// Create new unspent output reference
			CBOutputReference * ref = malloc(sizeof(*ref));
			if (NOT ref) {
				self->logError("Could not allocate memory for a new unspent output reference.");
				return false;
			}
			ref->blockIndex = blockIndex;
			ref->branch = branch;
			ref->coinbase = x == 0;
			ref->outputIndex = y;
			ref->position = cursor + 20; // Plus twenty for the block hash
			ref->storageID = self->numUnspentOutputs++; // Increase number of unsepnt outputs and assign ID at the same time.
			memcpy(ref->outputHash, CBTransactionGetHash(block->transactions[x]), 32);
			// Add to index
			if (NOT CBAssociativeArrayInsert(&self->outputIndex, ref->outputHash, CBAssociativeArrayFind(&self->outputIndex, ref->outputHash), NULL)) {
				self->logError("Could not insert a new unspent output reference into the output index.");
				return false;
			}
			// Add to storage
			CBInt32ToArray(key, 1, ref->storageID);
			CBInt32ToArray(data, CB_UNSPENT_OUTPUT_BLOCK_INDEX, ref->blockIndex);
			data[CB_UNSPENT_OUTPUT_BRANCH] = ref->branch;
			data[CB_UNSPENT_OUTPUT_COINBASE] = ref->coinbase;
			CBInt32ToArray(data, CB_UNSPENT_OUTPUT_OUTPUT_INDEX, ref->outputIndex);
			CBInt32ToArray(data, CB_UNSPENT_OUTPUT_POSITION, ref->position);
			if (NOT CBBlockChainStorageWriteValue(self->storage, key, data, 14, 0, 14)) {
				self->logError("Could not write new unspent output reference into the database.");
				return false;
			}
			// Move cursor past the output
			cursor += 8; // Value
			// Var int
			uint8_t byte = CBByteArrayGetByte(CBGetMessage(block)->bytes, cursor);
			cursor += byte < 253 ? 1 : (byte == 253 ? 2 : (byte == 254 ? 4 : 8));
			// Script length
			cursor += block->transactions[x]->outputs[y]->scriptObject->length;
		}
	}
	return true;
}*/
