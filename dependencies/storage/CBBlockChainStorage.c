//
//  CBBlockChainStorage.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 20/12/2012.
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

#include "CBBlockChainStorage.h"

// KEYS

uint8_t CB_VALIDATOR_INFO_KEY[2] = {1, CB_STORAGE_VALIDATOR_INFO};
uint8_t CB_ORPHAN_KEY[3] = {2, CB_STORAGE_ORPHAN, 0};
uint8_t CB_BRANCH_KEY[3] = {2, CB_STORAGE_BRANCH_INFO, 0};
uint8_t CB_WORK_KEY[3] = {2, CB_STORAGE_WORK, 0};
uint8_t CB_BLOCK_KEY[7] = {6, CB_STORAGE_BLOCK, 0, 0, 0, 0, 0};
uint8_t CB_NEW_BLOCK_KEY[7] = {6, CB_STORAGE_BLOCK, 0, 0, 0, 0, 0};
uint8_t CB_BLOCK_HASH_INDEX_KEY[22] = {21, CB_STORAGE_BLOCK_HASH_INDEX, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
uint8_t CB_UNSPENT_OUTPUT_KEY[38] = {37, CB_STORAGE_UNSPENT_OUTPUT, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
uint8_t CB_TRANSACTION_INDEX_KEY[34] = {33, CB_STORAGE_TRANSACTION_INDEX, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
uint8_t CB_DATA_ARRAY[22];

uint64_t CBNewBlockChainStorage(char * dataDir){
	return (uint64_t)CBNewDatabase(dataDir, "blk");
}
void CBFreeBlockChainStorage(uint64_t iself){
	CBFreeDatabase((CBDatabase *)iself);
}
bool CBBlockChainStorageBlockExists(void * validator, uint8_t * blockHash){
	CBFullValidator * validatorObj = validator;
	memcpy(CB_BLOCK_HASH_INDEX_KEY + 2, blockHash, 20);
	return CBDatabaseGetLength((CBDatabase *)validatorObj->storage, CB_BLOCK_HASH_INDEX_KEY);
}
bool CBBlockChainStorageChangeUnspentOutputsNum(CBDatabase * database, uint8_t * txHash, int8_t change){
	// Place transaction hash into the key
	memcpy(CB_TRANSACTION_INDEX_KEY + 2, txHash, 32);
	// Read the number of unspent outputs to be decremented
	if (NOT CBDatabaseReadValue(database, CB_TRANSACTION_INDEX_KEY, CB_DATA_ARRAY, 18, 0))
		return false;
	CBInt32ToArray(CB_DATA_ARRAY, CB_TRANSACTION_REF_NUM_UNSPENT_OUTPUTS, 
				   CBArrayToInt32(CB_DATA_ARRAY, CB_TRANSACTION_REF_NUM_UNSPENT_OUTPUTS) + change);
	// Now write the new value
	return CBDatabaseWriteValue(database, CB_TRANSACTION_INDEX_KEY, CB_DATA_ARRAY, 18);
}
bool CBBlockChainStorageCommitData(uint64_t iself){
	return CBDatabaseCommit((CBDatabase *)iself);
}
bool CBBlockChainStorageDeleteBlock(void * validator, uint8_t branch, uint32_t blockIndex){
	CBFullValidator * validatorObj = validator;
	CBDatabase * database = (CBDatabase *)validatorObj->storage;
	// Delete from storage
	CB_BLOCK_KEY[2] = branch;
	CBInt32ToArray(CB_BLOCK_KEY, 3, blockIndex);
	// Get hash
	if (NOT CBDatabaseReadValue(database, CB_BLOCK_KEY, CB_DATA_ARRAY, 20, CB_BLOCK_HASH)) {
		CBLogError("Could not obtain a block hash from the block chain database.");
		return false;
	}
	// Remove data
	if (NOT CBDatabaseRemoveValue(database, CB_BLOCK_KEY)){
		CBLogError("Could not remove block value from database.");
		return false;
	}
	// Remove hash index reference
	memcpy(CB_BLOCK_HASH_INDEX_KEY + 2, CB_DATA_ARRAY, 20);
	if (NOT CBDatabaseRemoveValue(database, CB_BLOCK_HASH_INDEX_KEY)){
		CBLogError("Could not remove block hash index reference from database.");
		return false;
	}
	return true;
}
bool CBBlockChainStorageDeleteUnspentOutput(void * validator, uint8_t * txHash, uint32_t outputIndex, bool decrement){
	CBFullValidator * validatorObj = validator;
	CBDatabase * database = (CBDatabase *)validatorObj->storage;
	// Place transaction hash into the key
	memcpy(CB_UNSPENT_OUTPUT_KEY + 2, txHash, 32);
	// Place output index into the key
	CBInt32ToArray(CB_UNSPENT_OUTPUT_KEY, 34, outputIndex);
	// Remove from storage
	if (NOT CBDatabaseRemoveValue(database, CB_UNSPENT_OUTPUT_KEY)) {
		CBLogError("Could not remove an unspent output reference from storage.");
		return false;
	}
	if (decrement
		// For the transaction, decrement the number of unspent outputs
		&& NOT CBBlockChainStorageChangeUnspentOutputsNum(database, txHash, -1)) {
		CBLogError("Could not decrement the number of unspent outputs for a transaction.");
		return false;
	}
	return true;
}
bool CBBlockChainStorageDeleteTransactionRef(void * validator, uint8_t * txHash){
	CBFullValidator * validatorObj = validator;
	CBDatabase * database = (CBDatabase *)validatorObj->storage;
	// Place transaction hash into the key
	memcpy(CB_TRANSACTION_INDEX_KEY + 2, txHash, 32);
	// Read the instance count
	if (NOT CBDatabaseReadValue(database, CB_TRANSACTION_INDEX_KEY, CB_DATA_ARRAY, 22, 0)) {
		CBLogError("Could not read a transaction reference from storage.");
		return false;
	}
	uint32_t txInstanceNum = CB_DATA_ARRAY[CB_TRANSACTION_REF_INSTANCE_COUNT] - 1;
	if (txInstanceNum) {
		// There are still more instances of this transaction. Do not remove the transaction, only make the unspent output number equal to zero and decrement the instance count.
		CB_DATA_ARRAY[CB_TRANSACTION_REF_NUM_UNSPENT_OUTPUTS] = 0;
		CB_DATA_ARRAY[CB_TRANSACTION_REF_INSTANCE_COUNT] = txInstanceNum;
		// Write to storage.
		if (NOT CBDatabaseWriteValue(database, CB_TRANSACTION_INDEX_KEY, CB_DATA_ARRAY, 22)) {
			CBLogError("Could not update a transaction reference for deleting an instance.");
			return false;
		}
	}else{
		// This was the last instance.
		// Remove from storage
		if (NOT CBDatabaseRemoveValue(database, CB_TRANSACTION_INDEX_KEY)) {
			CBLogError("Could not remove a transaction reference from storage.");
			return false;
		}
	}
	return true;
}
bool CBBlockChainStorageExists(uint64_t iself){
	return CBDatabaseGetLength((CBDatabase *)iself, CB_VALIDATOR_INFO_KEY);
}
bool CBBlockChainStorageGetBlockLocation(void * validator, uint8_t * blockHash, uint8_t * branch, uint32_t * index){
	CBFullValidator * validatorObj = validator;
	CBDatabase * database = (CBDatabase *)validatorObj->storage;
	memcpy(CB_BLOCK_HASH_INDEX_KEY + 2, blockHash, 20);
	if (NOT CBDatabaseReadValue(database, CB_BLOCK_HASH_INDEX_KEY, CB_DATA_ARRAY, 5, 0)) {
		CBLogError("Could not read a block hash reference from the block chain database.");
		return false;
	}
	*branch = CB_DATA_ARRAY[CB_BLOCK_HASH_REF_BRANCH];
	*index = CBArrayToInt32(CB_DATA_ARRAY, CB_BLOCK_HASH_REF_INDEX);
	return true;
}
uint32_t CBBlockChainStorageGetBlockTime(void * validator, uint8_t branch, uint32_t blockIndex){
	CBFullValidator * validatorObj = validator;
	CBDatabase * database = (CBDatabase *)validatorObj->storage;
	CB_BLOCK_KEY[2] = branch;
	CBInt32ToArray(CB_BLOCK_KEY, 3, blockIndex);
	if (NOT CBDatabaseReadValue(database, CB_BLOCK_KEY, CB_DATA_ARRAY, 4, CB_BLOCK_TIME)) {
		CBLogError("Could not read the time for a block.");
		return 0;
	}
	return CBArrayToInt32(CB_DATA_ARRAY, 0);
}
uint32_t CBBlockChainStorageGetBlockTarget(void * validator, uint8_t branch, uint32_t blockIndex){
	CBFullValidator * validatorObj = validator;
	CBDatabase * database = (CBDatabase *)validatorObj->storage;
	CB_BLOCK_KEY[2] = branch;
	CBInt32ToArray(CB_BLOCK_KEY, 3, blockIndex);
	if (NOT CBDatabaseReadValue(database, CB_BLOCK_KEY, CB_DATA_ARRAY, 4, CB_BLOCK_TARGET)) {
		CBLogError("Could not read the target for a block.");
		return 0;
	}
	return CBArrayToInt32(CB_DATA_ARRAY, 0);
}
bool CBBlockChainStorageIsTransactionWithUnspentOutputs(void * validator, uint8_t * txHash, bool * exists){
	CBFullValidator * validatorObj = validator;
	CBDatabase * database = (CBDatabase *)validatorObj->storage;
	// Place transaction hash into the key
	memcpy(CB_TRANSACTION_INDEX_KEY + 2, txHash, 32);
	// See if the transaction exists
	if (NOT CBDatabaseGetLength(database, CB_TRANSACTION_INDEX_KEY)){
		*exists = false;
		return true;
	}
	// Now see if the transaction has unspent outputs
	if (NOT CBDatabaseReadValue(database, CB_TRANSACTION_INDEX_KEY, CB_DATA_ARRAY, 4, CB_TRANSACTION_REF_NUM_UNSPENT_OUTPUTS)) {
		CBLogError("Could not read the number of unspent outputs for a transaction.");
		return false;
	}
	*exists = CBArrayToInt32(CB_DATA_ARRAY, 0);
	return true;
}
bool CBBlockChainStorageLoadBasicValidator(void * validator){
	CBFullValidator * validatorObj = validator;
	CBDatabase * database = (CBDatabase *)validatorObj->storage;
	if (NOT CBDatabaseReadValue(database, CB_VALIDATOR_INFO_KEY, CB_DATA_ARRAY, 4, 0)){
		CBLogError("There was an error when reading the validator information from storage.");
		return false;
	}
	validatorObj->firstOrphan = CB_DATA_ARRAY[CB_VALIDATION_FIRST_ORPHAN];
	validatorObj->numOrphans = CB_DATA_ARRAY[CB_VALIDATION_NUM_ORPHANS];
	validatorObj->mainBranch = CB_DATA_ARRAY[CB_VALIDATION_MAIN_BRANCH];
	validatorObj->numBranches = CB_DATA_ARRAY[CB_VALIDATION_NUM_BRANCHES];
	return true;
}
void * CBBlockChainStorageLoadBlock(void * validator, uint32_t blockID, uint32_t branch){
	CBFullValidator * validatorObj = validator;
	CBDatabase * database = (CBDatabase *)validatorObj->storage;
	CB_BLOCK_KEY[2] = branch;
	CBInt32ToArray(CB_BLOCK_KEY, 3, blockID);
	uint32_t blockDataLen = CBDatabaseGetLength(database, CB_BLOCK_KEY);
	if (NOT blockDataLen)
		return NULL;
	blockDataLen -= CB_BLOCK_START;
	// Get block data
	CBByteArray * data = CBNewByteArrayOfSize(blockDataLen);
	if (NOT data) {
		CBLogError("Could not initialise a byte array for loading a block.");
		return NULL;
	}
	if (NOT CBDatabaseReadValue(database, CB_BLOCK_KEY, CBByteArrayGetData(data), blockDataLen, CB_BLOCK_START)){
		CBLogError("Could not read a block from the database.");
		CBReleaseObject(data);
		return NULL;
	}
	// Make and return the block
	CBBlock * block = CBNewBlockFromData(data);
	CBReleaseObject(data);
	if (NOT block) {
		CBLogError("Could not create a block object when loading a block.");
		return NULL;
	}
	return block;
}
bool CBBlockChainStorageLoadBranch(void * validator, uint8_t branchNum){
	CBFullValidator * validatorObj = validator;
	CBDatabase * database = (CBDatabase *)validatorObj->storage;
	// Get simple information
	CB_BRANCH_KEY[2] = branchNum;
	if (NOT CBDatabaseReadValue(database, CB_BRANCH_KEY, CB_DATA_ARRAY, 21, 0)) {
		CBLogError("There was an error when reading the data for a branch's information.");
		return false;
	}
	validatorObj->branches[branchNum].lastRetargetTime = CBArrayToInt32(CB_DATA_ARRAY, CB_BRANCH_LAST_RETARGET);
	validatorObj->branches[branchNum].lastValidation = CBArrayToInt32(CB_DATA_ARRAY, CB_BRANCH_LAST_VALIDATION);
	validatorObj->branches[branchNum].numBlocks = CBArrayToInt32(CB_DATA_ARRAY, CB_BRANCH_NUM_BLOCKS);
	validatorObj->branches[branchNum].parentBlockIndex = CBArrayToInt32(CB_DATA_ARRAY, CB_BRANCH_PARENT_BLOCK_INDEX);
	validatorObj->branches[branchNum].parentBranch = CB_DATA_ARRAY[CB_BRANCH_PARENT_BRANCH];
	validatorObj->branches[branchNum].startHeight = CBArrayToInt32(CB_DATA_ARRAY, CB_BRANCH_START_HEIGHT);
	validatorObj->branches[branchNum].working = false; // Start off not working on any branch.
	return true;
}
bool CBBlockChainStorageLoadBranchWork(void * validator, uint8_t branchNum){
	CBFullValidator * validatorObj = validator;
	CBDatabase * database = (CBDatabase *)validatorObj->storage;
	// Get work
	CB_WORK_KEY[2] = branchNum;
	uint8_t workLen = CBDatabaseGetLength(database, CB_WORK_KEY);
	if (NOT CBBigIntAlloc(&validatorObj->branches[branchNum].work, workLen)){
		CBLogError("There was an error when allocating memory for a branch's work.");
		return false;
	}
	validatorObj->branches[branchNum].work.length = workLen;
	if (NOT CBDatabaseReadValue(database, CB_WORK_KEY, validatorObj->branches[branchNum].work.data, workLen, 0)) {
		CBLogError("There was an error when reading the work for a branch.");
		free(validatorObj->branches[branchNum].work.data);
		return false;
	}
	return true;
}
bool CBBlockChainStorageLoadOrphan(void * validator, uint8_t orphanNum){
	CBFullValidator * validatorObj = validator;
	CBDatabase * database = (CBDatabase *)validatorObj->storage;
	CB_ORPHAN_KEY[2] = orphanNum;
	uint32_t len = CBDatabaseGetLength(database, CB_ORPHAN_KEY);
	CBByteArray * orphanData = CBNewByteArrayOfSize(len);
	if (NOT orphanData) {
		CBLogError("There was an error when initialising a byte array for an orphan.");
		return false;
	}
	if (NOT CBDatabaseReadValue(database, CB_ORPHAN_KEY, CBByteArrayGetData(orphanData), len, 0)) {
		CBLogError("There was an error when reading the data for an orphan.");
		CBReleaseObject(orphanData);
		return false;
	}
	validatorObj->orphans[orphanNum] = CBNewBlockFromData(orphanData);
	if (NOT validatorObj->orphans[orphanNum]) {
		CBLogError("There was an error when creating a block object for an orphan.");
		CBReleaseObject(orphanData);
		return false;
	}
	CBReleaseObject(orphanData);
	return true;
}
bool CBBlockChainStorageLoadOutputs(void * validator, uint8_t * txHash, uint8_t ** data, uint32_t * dataAllocSize, uint32_t * position){
	CBFullValidator * validatorObj = validator;
	CBDatabase * database = (CBDatabase *)validatorObj->storage;
	memcpy(CB_TRANSACTION_INDEX_KEY + 2, txHash, 32);
	if (NOT CBDatabaseReadValue(database, CB_TRANSACTION_INDEX_KEY, CB_DATA_ARRAY, 14, 0)) {
		CBLogError("Could not read a transaction reference from the transaction index.");
		return false;
	}
	// Set position of outputs
	*position = CBArrayToInt32(CB_DATA_ARRAY, CB_TRANSACTION_REF_POSITION_OUPTUTS);
	// Get transaction to find position for output in the block
	// Reallocate transaction data memory if needed.
	if (CBArrayToInt32(CB_DATA_ARRAY, CB_TRANSACTION_REF_LENGTH_OUTPUTS) > *dataAllocSize) {
		*dataAllocSize = CBArrayToInt32(CB_DATA_ARRAY, CB_TRANSACTION_REF_LENGTH_OUTPUTS);
		*data = realloc(*data, *dataAllocSize);
		if (NOT *data) {
			CBLogError("Could not allocate memory for reading a transaction.");
			return false;
		}
	}
	// Read transaction from the block
	CB_BLOCK_KEY[2] = CB_DATA_ARRAY[CB_TRANSACTION_REF_BRANCH];
	memcpy(CB_BLOCK_KEY + 3, CB_DATA_ARRAY + CB_TRANSACTION_REF_BLOCK_INDEX, 4);
	if (NOT CBDatabaseReadValue(database, CB_BLOCK_KEY, *data, CBArrayToInt32(CB_DATA_ARRAY, CB_TRANSACTION_REF_LENGTH_OUTPUTS), CB_BLOCK_START + *position)) {
		CBLogError("Could not read a transaction from the block-chain database.");
		return false;
	}
	return true;
}
void * CBBlockChainStorageLoadUnspentOutput(void * validator, uint8_t * txHash, uint32_t outputIndex, bool * coinbase, uint32_t * outputHeight){
	CBFullValidator * validatorObj = validator;
	CBDatabase * database = (CBDatabase *)validatorObj->storage;
	// First read data for the unspent output key.
	memcpy(CB_UNSPENT_OUTPUT_KEY + 2, txHash, 32);
	CBInt32ToArray(CB_UNSPENT_OUTPUT_KEY, 34, outputIndex);
	if (NOT CBDatabaseReadValue(database, CB_UNSPENT_OUTPUT_KEY, CB_DATA_ARRAY, 8, 0)) {
		CBLogError("Cannot read unspent output information from the block chain database");
		return NULL;
	}
	uint32_t outputPosition = CBArrayToInt32(CB_DATA_ARRAY, CB_UNSPENT_OUTPUT_REF_POSITION);
	uint32_t outputLength = CBArrayToInt32(CB_DATA_ARRAY, CB_UNSPENT_OUTPUT_REF_LENGTH);
	// Now read data for the transaction
	memcpy(CB_TRANSACTION_INDEX_KEY + 2, txHash, 32);
	if (NOT CBDatabaseReadValue(database, CB_TRANSACTION_INDEX_KEY, CB_DATA_ARRAY, 14, 0)) {
		CBLogError("Cannot read a transaction reference from the transaction index.");
		return NULL;
	}
	uint8_t outputBranch = CB_DATA_ARRAY[CB_TRANSACTION_REF_BRANCH];
	uint32_t outputBlockIndex = CBArrayToInt32(CB_DATA_ARRAY, CB_TRANSACTION_REF_BLOCK_INDEX);
	// Set coinbase
	*coinbase = CB_DATA_ARRAY[CB_TRANSACTION_REF_IS_COINBASE];
	// Set output height
	*outputHeight = validatorObj->branches[outputBranch].startHeight + outputBlockIndex;
	// Get the output from storage
	CB_BLOCK_KEY[2] = outputBranch;
	CBInt32ToArray(CB_BLOCK_KEY, 3, outputBlockIndex);
	// Get output data
	CBByteArray * outputBytes = CBNewByteArrayOfSize(outputLength);
	if (NOT outputBytes) {
		CBLogError("Could not create  CBByteArray for an unspent output.");
		return NULL;
	}
	if (NOT CBDatabaseReadValue(database, CB_BLOCK_KEY, CBByteArrayGetData(outputBytes), outputLength, CB_BLOCK_START + outputPosition)) {
		CBLogError("Could not read an unspent output");
		CBReleaseObject(outputBytes);
		return NULL;
	}
	// Create output object
	CBTransactionOutput * output = CBNewTransactionOutputFromData(outputBytes);
	CBReleaseObject(outputBytes);
	if (NOT output) {
		CBLogError("Could not create an object for an unspent output");
		return NULL;
	}
	if (NOT CBTransactionOutputDeserialise(output)) {
		CBLogError("Could not deserialise an unspent output");
		return NULL;
	}
	return output;
}
bool CBBlockChainStorageMoveBlock(void * validator, uint8_t branch, uint32_t blockIndex, uint8_t newBranch, uint32_t newIndex){
	CBFullValidator * validatorObj = validator;
	CBDatabase * database = (CBDatabase *)validatorObj->storage;
	CB_BLOCK_KEY[2] = branch;
	CBInt32ToArray(CB_BLOCK_KEY, 3, blockIndex);
	CB_NEW_BLOCK_KEY[2] = newBranch;
	if (NOT CBDatabaseChangeKey(database, CB_BLOCK_KEY, CB_NEW_BLOCK_KEY)) {
		CBLogError("Could not move a block location in the block-chain database.");
		return false;
	}
	return true;
}
void CBBlockChainStorageReset(uint64_t iself){
	CBDatabaseClearPending((CBDatabase *)iself);
}
bool CBBlockChainStorageSaveBasicValidator(void * validator){
	CBFullValidator * validatorObj = validator;
	CBDatabase * database = (CBDatabase *)validatorObj->storage;
	CB_DATA_ARRAY[CB_VALIDATION_FIRST_ORPHAN] = validatorObj->firstOrphan;
	CB_DATA_ARRAY[CB_VALIDATION_NUM_ORPHANS] = validatorObj->numOrphans;
	CB_DATA_ARRAY[CB_VALIDATION_MAIN_BRANCH] = validatorObj->mainBranch;
	CB_DATA_ARRAY[CB_VALIDATION_NUM_BRANCHES] = validatorObj->numBranches;
	if (NOT CBDatabaseWriteValue(database, CB_VALIDATOR_INFO_KEY, CB_DATA_ARRAY, 4)) {
		CBLogError("Could not write the initial basic validation data.");
		return false;
	}
	return true;
}
bool CBBlockChainStorageSaveBlock(void * validator, void * block, uint8_t branch, uint32_t blockIndex){
	CBFullValidator * validatorObj = validator;
	CBBlock * blockObj = block;
	// Write the block data
	CB_BLOCK_KEY[2] = branch;
	CBInt32ToArray(CB_BLOCK_KEY, 3, blockIndex);
	uint8_t * dataParts[2] = {CBBlockGetHash(blockObj), CBByteArrayGetData(CBGetMessage(blockObj)->bytes)};
	uint32_t dataSizes[2] = {20, CBGetMessage(blockObj)->bytes->length};
	if (NOT CBDatabaseWriteConcatenatedValue((CBDatabase *)validatorObj->storage, CB_BLOCK_KEY, 2, dataParts, dataSizes)) {
		CBLogError("Could not write a block to the block-chain database.");
		return false;
	}
	// Write to the block hash index
	memcpy(CB_BLOCK_HASH_INDEX_KEY + 2, CBBlockGetHash(blockObj), 20);
	CB_DATA_ARRAY[CB_BLOCK_HASH_REF_BRANCH] = branch;
	CBInt32ToArray(CB_DATA_ARRAY, CB_BLOCK_HASH_REF_INDEX, blockIndex);
	if (NOT CBDatabaseWriteValue((CBDatabase *)validatorObj->storage, CB_BLOCK_HASH_INDEX_KEY, CB_DATA_ARRAY, 5)) {
		CBLogError("Could not write a block hash to the block-chain database block hash index.");
		return false;
	}
	return true;
}
bool CBBlockChainStorageSaveBranch(void * validator, uint8_t branch){
	CBFullValidator * validatorObj = validator;
	CBDatabase * database = (CBDatabase *)validatorObj->storage;
	CB_BRANCH_KEY[2] = branch;
	// Make data
	CBInt32ToArray(CB_DATA_ARRAY, CB_BRANCH_LAST_RETARGET, validatorObj->branches[branch].lastRetargetTime);
	CBInt32ToArray(CB_DATA_ARRAY, CB_BRANCH_LAST_VALIDATION, validatorObj->branches[branch].lastValidation);
	CBInt32ToArray(CB_DATA_ARRAY, CB_BRANCH_NUM_BLOCKS, validatorObj->branches[branch].numBlocks);
	CBInt32ToArray(CB_DATA_ARRAY, CB_BRANCH_PARENT_BLOCK_INDEX, validatorObj->branches[branch].parentBlockIndex);
	CB_DATA_ARRAY[CB_BRANCH_PARENT_BRANCH] = validatorObj->branches[branch].parentBranch;
	CBInt32ToArray(CB_DATA_ARRAY, CB_BRANCH_START_HEIGHT, validatorObj->branches[branch].startHeight);
	// Write data
	if (NOT CBDatabaseWriteValue(database, CB_BRANCH_KEY, CB_DATA_ARRAY, 21)) {
		CBLogError("Could not write branch information.");
		return false;
	}
	return true;
}
bool CBBlockChainStorageSaveBranchWork(void * validator, uint8_t branch){
	CBFullValidator * validatorObj = validator;
	CBDatabase * database = (CBDatabase *)validatorObj->storage;
	CB_WORK_KEY[2] = branch;
	if (NOT CBDatabaseWriteValue(database, CB_WORK_KEY, validatorObj->branches[branch].work.data, validatorObj->branches[branch].work.length)) {
		CBLogError("Could not write branch work.");
		return false;
	}
	return true;
}
bool CBBlockChainStorageSaveOrphan(void * validator, void * block, uint8_t orphanNum){
	CBFullValidator * validatorObj = validator;
	CBDatabase * database = (CBDatabase *)validatorObj->storage;
	CBBlock * blockObj = block;
	CB_ORPHAN_KEY[2] = orphanNum;
	if (NOT CBDatabaseWriteValue(database, CB_ORPHAN_KEY, CBByteArrayGetData(CBGetMessage(blockObj)->bytes), CBGetMessage(blockObj)->bytes->length)) {
		CBLogError("Could not write an orphan.");
		return false;
	}
	return true;
}
bool CBBlockChainStorageSaveTransactionRef(void * validator, uint8_t * txHash, uint8_t branch, uint32_t blockIndex, uint32_t outputPos, uint32_t outputsLen, bool coinbase, uint32_t numOutputs){
	CBFullValidator * validatorObj = validator;
	CBDatabase * database = (CBDatabase *)validatorObj->storage;
	memcpy(CB_TRANSACTION_INDEX_KEY + 2, txHash, 32);
	if (CBDatabaseGetLength(database, CB_TRANSACTION_INDEX_KEY)) {
		// We have the transaction already. Thus obtain the data already in the index.
		if (NOT CBDatabaseReadValue(database, CB_TRANSACTION_INDEX_KEY, CB_DATA_ARRAY, 22, 0)) {
			CBLogError("Could not read a transaction reference from the transaction index.");
			return false;
		}
		// Increase the instance count. We change nothing else as we will use the first instance for all other instances.
		CBInt32ToArray(CB_DATA_ARRAY, CB_TRANSACTION_REF_INSTANCE_COUNT, CBArrayToInt32(CB_DATA_ARRAY, CB_TRANSACTION_REF_INSTANCE_COUNT) + 1);
	}else{
		// This transaction has not yet been seen in the block chain.
		CBInt32ToArray(CB_DATA_ARRAY, CB_TRANSACTION_REF_BLOCK_INDEX, blockIndex);
		CB_DATA_ARRAY[CB_TRANSACTION_REF_BRANCH] = branch;
		CBInt32ToArray(CB_DATA_ARRAY, CB_TRANSACTION_REF_POSITION_OUPTUTS, outputPos);
		CBInt32ToArray(CB_DATA_ARRAY, CB_TRANSACTION_REF_LENGTH_OUTPUTS, outputsLen);
		CB_DATA_ARRAY[CB_TRANSACTION_REF_IS_COINBASE] = coinbase;
		// We start with an instance count of one
		CBInt32ToArray(CB_DATA_ARRAY, CB_TRANSACTION_REF_INSTANCE_COUNT, 1);
	}
	// Always set the number of unspent outputs back to the number of outputs in the transaction
	CBInt32ToArray(CB_DATA_ARRAY, CB_TRANSACTION_REF_NUM_UNSPENT_OUTPUTS, numOutputs);
	// Write to the transaction index.
	if (NOT CBDatabaseWriteValue(database, CB_TRANSACTION_INDEX_KEY, CB_DATA_ARRAY, 22)) {
		CBLogError("Could not write transaction reference to transaction index.");
		return false;
	}
	return true;
}
bool CBBlockChainStorageSaveUnspentOutput(void * validator, uint8_t * txHash, uint32_t outputIndex, uint32_t position, uint32_t length, bool increment){
	CBFullValidator * validatorObj = validator;
	CBDatabase * database = (CBDatabase *)validatorObj->storage;
	memcpy(CB_UNSPENT_OUTPUT_KEY + 2, txHash, 32);
	CBInt32ToArray(CB_UNSPENT_OUTPUT_KEY, 34, outputIndex);
	CBInt32ToArray(CB_DATA_ARRAY, CB_UNSPENT_OUTPUT_REF_POSITION, position);
	CBInt32ToArray(CB_DATA_ARRAY, CB_UNSPENT_OUTPUT_REF_LENGTH, length);
	// Add to storage
	if (NOT CBDatabaseWriteValue(database, CB_UNSPENT_OUTPUT_KEY, CB_DATA_ARRAY, 8)) {
		CBLogError("Could not write new unspent output reference into the database.");
		return false;
	}
	if (increment
		// For the transaction, increment the number of unspent outputs
		&& NOT CBBlockChainStorageChangeUnspentOutputsNum(database, txHash, +1)) {
		CBLogError("Could not increment the number of unspent outputs for a transaction.");
		return false;
	}
	return true;
}
bool CBBlockChainStorageUnspentOutputExists(void * validator, uint8_t * txHash, uint32_t outputIndex){
	CBFullValidator * validatorObj = validator;
	memcpy(CB_UNSPENT_OUTPUT_KEY + 2, txHash, 32);
	CBInt32ToArray(CB_UNSPENT_OUTPUT_KEY, 34, outputIndex);
	return CBDatabaseGetLength((CBDatabase *)validatorObj->storage, CB_UNSPENT_OUTPUT_KEY);
}
