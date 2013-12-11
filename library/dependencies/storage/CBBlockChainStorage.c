//
//  CBBlockChainStorage.c
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

//  SEE HEADER FILE FOR DOCUMENTATION

#include "CBBlockChainStorage.h"

bool CBNewBlockChainStorage(CBDepObject * storage, CBDepObject database){
	CBBlockChainStorage * self = malloc(sizeof(*self));
	// Load indices
	self->blockHashIndex = CBLoadIndex(database.ptr, CB_INDEX_BLOCK_HASH, 32, 10000);
	if (self->blockHashIndex){
		self->blockIndex = CBLoadIndex(database.ptr, CB_INDEX_BLOCK, 5, 10000);
		if (self->blockIndex){
			self->orphanIndex = CBLoadIndex(database.ptr, CB_INDEX_ORPHAN, 1, 0);
			if (self->orphanIndex){
				self->txIndex = CBLoadIndex(database.ptr, CB_INDEX_TX, 32, 10000);
				if (self->txIndex) {
					self->unspentOutputIndex = CBLoadIndex(database.ptr, CB_INDEX_UTXOUT, 36, 10000);
					if (self->unspentOutputIndex){
						self->database = database.ptr;
						storage->ptr = self;
						return true;
					}
					CBFreeIndex(self->txIndex);
				}
				CBFreeIndex(self->orphanIndex);
			}
			CBFreeIndex(self->blockIndex);
		}
		CBFreeIndex(self->blockHashIndex);
	}
	CBLogError("Could not load one of the block chain storage indices.");
	CBFreeDatabase(self->database);
	return false;
}
void CBFreeBlockChainStorage(CBDepObject self){
	CBBlockChainStorage * storageObj = self.ptr;
	CBFreeIndex(storageObj->blockHashIndex);
	CBFreeIndex(storageObj->blockIndex);
	CBFreeIndex(storageObj->orphanIndex);
	CBFreeIndex(storageObj->txIndex);
	CBFreeIndex(storageObj->unspentOutputIndex);
}
CBErrBool CBBlockChainStorageBlockExists(void * validator, uint8_t * blockHash){
	CBBlockChainStorage * storageObj = ((CBValidator *)validator)->storage.ptr;
	uint32_t len;
	if (! CBDatabaseGetLength(storageObj->blockHashIndex, blockHash, &len)){
		CBLogError("There was an error attempting to find the length for a block to see if it exists");
		return CB_ERROR;
	}
	return len != CB_DOESNT_EXIST;
}
bool CBBlockChainStorageChangeUnspentOutputsNum(CBBlockChainStorage * storage, uint8_t * txHash, int8_t change){
	// Read the number of unspent outputs to be decremented
	uint8_t data[4];
	if (CBDatabaseReadValue(storage->txIndex, txHash, data, 4, CB_TX_REF_NUM_UNSPENT_OUTPUTS, false) != CB_DATABASE_INDEX_FOUND)
		return false;
	CBInt32ToArray(data, 0, CBArrayToInt32(data, 0) + change);
	// Now write the new value
	CBDatabaseWriteValueSubSection(storage->txIndex, txHash, data, 4, CB_TX_REF_NUM_UNSPENT_OUTPUTS);
	return true;
}
bool CBBlockChainStorageDeleteBlock(void * validator, uint8_t branch, uint32_t blockIndex){
	CBBlockChainStorage * storageObj = ((CBValidator *)validator)->storage.ptr;
	uint8_t key[5], hash[32];
	// Delete from storage
	key[0] = branch;
	CBInt32ToArray(key, 1, blockIndex);
	// Get hash
	if (CBDatabaseReadValue(storageObj->blockIndex, key, hash, 32, CB_BLOCK_HASH, false) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Could not obtain a block hash from the block chain database.");
		return false;
	}
	// Remove data
	if (! CBDatabaseRemoveValue(storageObj->blockIndex, key, false)){
		CBLogError("Could not remove block value from database.");
		return false;
	}
	// Remove hash index reference
	if (! CBDatabaseRemoveValue(storageObj->blockHashIndex, hash, false)){
		CBLogError("Could not remove block hash index reference from database.");
		return false;
	}
	return true;
}
bool CBBlockChainStorageDeleteUnspentOutput(void * validator, uint8_t * txHash, uint32_t outputIndex, bool decrement){
	CBBlockChainStorage * storageObj = ((CBValidator *)validator)->storage.ptr;
	uint8_t key[36];
	// Place transaction hash into the key
	memcpy(key, txHash, 32);
	// Place output index into the key
	CBInt32ToArray(key, 32, outputIndex);
	// Remove from storage
	if (! CBDatabaseRemoveValue(storageObj->unspentOutputIndex, key, false)) {
		CBLogError("Could not remove an unspent output reference from storage.");
		return false;
	}
	if (decrement
		// For the transaction, decrement the number of unspent outputs
		&& ! CBBlockChainStorageChangeUnspentOutputsNum(storageObj, txHash, -1)) {
		CBLogError("Could not decrement the number of unspent outputs for a transaction.");
		return false;
	}
	return true;
}
bool CBBlockChainStorageDeleteTransactionRef(void * validator, uint8_t * txHash){
	CBBlockChainStorage * storageObj = ((CBValidator *)validator)->storage.ptr;
	uint8_t data[8];
	// Read the instance count
	if (CBDatabaseReadValue(storageObj->txIndex, txHash, data, 4, CB_TX_REF_INSTANCE_COUNT, false) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Could not read a transaction reference from storage.");
		return false;
	}
	uint32_t txInstanceNum = CBArrayToInt32(data, 0) - 1;
	if (txInstanceNum != 0) {
		// There are still more instances of this transaction. Do not remove the transaction, only make the unspent output number equal to zero and decrement the instance count.
		CBInt32ToArray(data, 0, 0);
		CBInt32ToArray(data, 4, txInstanceNum);
		// Write to storage.
		CBDatabaseWriteValueSubSection(storageObj->txIndex, txHash, data, 8, CB_TX_REF_NUM_UNSPENT_OUTPUTS);
	// Else this was the last instance.
	// Remove from storage
	}else if (! CBDatabaseRemoveValue(storageObj->txIndex, txHash, false)) {
		CBLogError("Could not remove a transaction reference from storage.");
		return false;
	}
	return true;
}
bool CBBlockChainStorageExists(CBDepObject self){
	uint8_t * exists = &((CBBlockChainStorage *)self.ptr)->database->current.extraData[CB_EXTRA_CREATED_BLOCK_CHAIN_STORAGE];
	bool ret = *exists != 0;
	*exists = 1;
	return ret;
}
bool CBBlockChainStorageGetBlockHash(void * validator, uint8_t branch, uint32_t blockIndex, uint8_t hash[]){
	CBBlockChainStorage * storageObj = ((CBValidator *)validator)->storage.ptr;
	uint8_t key[5];
	key[0] = branch;
	CBInt32ToArray(key, 1, blockIndex);
	if (CBDatabaseReadValue(storageObj->blockIndex, key, hash, 32, CB_BLOCK_HASH, false) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Could not read the hash for a block.");
		return false;
	}
	return true;
}
void * CBBlockChainStorageGetBlockHeader(void * validator, uint8_t branch, uint32_t blockIndex){
	CBBlockChainStorage * storageObj = ((CBValidator *)validator)->storage.ptr;
	uint8_t key[5];
	key[0] = branch;
	CBInt32ToArray(key, 1, blockIndex);
	uint32_t blockDataLen;
	if (! CBDatabaseGetLength(storageObj->blockIndex, key, &blockDataLen)) {
		CBLogError("There was an error when checking if a block header exists by getting its length.");
		return NULL;
	}
	if (blockDataLen == CB_DOESNT_EXIST)
		return NULL;
	blockDataLen -= CB_BLOCK_START;
	// We do want any more than 90 bytes
	if (blockDataLen > 90)
		blockDataLen = 90;
	// Get block data
	CBByteArray * data = CBNewByteArrayOfSize(blockDataLen);
	if (CBDatabaseReadValue(storageObj->blockIndex, key, CBByteArrayGetData(data), blockDataLen, CB_BLOCK_START, false) != CB_DATABASE_INDEX_FOUND){
		CBLogError("Could not read a block from the database.");
		CBReleaseObject(data);
		return NULL;
	}
	// Trim the data to only include the var int and then add a null byte
	uint8_t varIntSize = CBVarIntDecode(data, 80).size;
	data->length = 81 + varIntSize;
	CBByteArraySetByte(data, data->length - 1, 0);
	// Make and return the block
	CBBlock * block = CBNewBlockFromData(data);
	CBReleaseObject(data);
	return block;
}
CBErrBool CBBlockChainStorageGetBlockLocation(void * validator, uint8_t * blockHash, uint8_t * branch, uint32_t * index){
	CBBlockChainStorage * storageObj = ((CBValidator *)validator)->storage.ptr;
	uint8_t data[5];
	CBIndexFindStatus stat = CBDatabaseReadValue(storageObj->blockHashIndex, blockHash, data, 5, 0, false);
	if (stat == CB_DATABASE_INDEX_ERROR) {
		CBLogError("Could not read a block hash reference from the block chain database.");
		return CB_ERROR;
	}
	if (stat == CB_DATABASE_INDEX_NOT_FOUND)
		return CB_FALSE;
	*branch = data[CB_BLOCK_HASH_REF_BRANCH];
	*index = CBArrayToInt32(data, CB_BLOCK_HASH_REF_INDEX);
	return CB_TRUE;
}
uint32_t CBBlockChainStorageGetBlockTime(void * validator, uint8_t branch, uint32_t blockIndex){
	CBBlockChainStorage * storageObj = ((CBValidator *)validator)->storage.ptr;
	uint8_t key[5], data[4];
	key[0] = branch;
	CBInt32ToArray(key, 1, blockIndex);
	if (CBDatabaseReadValue(storageObj->blockIndex, key, data, 4, CB_BLOCK_TIME, false) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Could not read the time for a block.");
		return 0;
	}
	return CBArrayToInt32(data, 0);
}
uint32_t CBBlockChainStorageGetBlockTarget(void * validator, uint8_t branch, uint32_t blockIndex){
	CBBlockChainStorage * storageObj = ((CBValidator *)validator)->storage.ptr;
	uint8_t key[5], data[5];
	key[0] = branch;
	CBInt32ToArray(key, 1, blockIndex);
	if (CBDatabaseReadValue(storageObj->blockIndex, key, data, 4, CB_BLOCK_TARGET, false) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Could not read the target for a block.");
		return 0;
	}
	return CBArrayToInt32(data, 0);
}
CBErrBool CBBlockChainStorageIsTransactionWithUnspentOutputs(void * validator, uint8_t * txHash){
	CBBlockChainStorage * storageObj = ((CBValidator *)validator)->storage.ptr;
	uint8_t data[4];
	// See if the transaction exists
	uint32_t len;
	if (! CBDatabaseGetLength(storageObj->txIndex, txHash, &len)){
		CBLogError("There was an error getting the length of a transaction entry to determine if a transaction exists.");
		return CB_ERROR;
	}
	if (len == CB_DOESNT_EXIST)
		return CB_FALSE;
	// Now see if the transaction has unspent outputs
	if (CBDatabaseReadValue(storageObj->txIndex, txHash, data, 4, CB_TX_REF_NUM_UNSPENT_OUTPUTS, false) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Could not read the number of unspent outputs for a transaction.");
		return CB_ERROR;
	}
	return CBArrayToInt32(data, 0) > 0;
}
bool CBBlockChainStorageLoadBasicValidator(void * validator){
	CBValidator * validatorObj = validator;
	uint8_t * extraData = ((CBBlockChainStorage *)validatorObj->storage.ptr)->database->current.extraData;
	validatorObj->firstOrphan = extraData[CB_EXTRA_BASIC_VALIDATOR_INFO + CB_VALIDATION_FIRST_ORPHAN];
	validatorObj->numOrphans = extraData[CB_EXTRA_BASIC_VALIDATOR_INFO + CB_VALIDATION_NUM_ORPHANS];
	validatorObj->mainBranch = extraData[CB_EXTRA_BASIC_VALIDATOR_INFO + CB_VALIDATION_MAIN_BRANCH];
	validatorObj->numBranches = extraData[CB_EXTRA_BASIC_VALIDATOR_INFO + CB_VALIDATION_NUM_BRANCHES];
	return true;
}
void * CBBlockChainStorageLoadBlock(void * validator, uint32_t blockID, uint32_t branch){
	CBBlockChainStorage * storageObj = ((CBValidator *)validator)->storage.ptr;
	uint8_t key[5];
	key[0] = branch;
	CBInt32ToArray(key, 1, blockID);
	uint32_t blockDataLen;
	if (! CBDatabaseGetLength(storageObj->blockIndex, key, &blockDataLen)) {
		CBLogError("There was an error getting the length of a block to see if it exists when loading.");
		return NULL;
	}
	if (blockDataLen == CB_DOESNT_EXIST)
		return NULL;
	blockDataLen -= CB_BLOCK_START;
	// Get block data
	CBByteArray * data = CBNewByteArrayOfSize(blockDataLen);
	if (CBDatabaseReadValue(storageObj->blockIndex, key, CBByteArrayGetData(data), blockDataLen, CB_BLOCK_START, false) != CB_DATABASE_INDEX_FOUND){
		CBLogError("Could not read a block from the database.");
		CBReleaseObject(data);
		return NULL;
	}
	// Make and return the block
	CBBlock * block = CBNewBlockFromData(data);
	CBReleaseObject(data);
	return block;
}
bool CBBlockChainStorageLoadBranch(void * validator, uint8_t branchNum){
	CBValidator * validatorObj = validator;
	uint8_t * extraData = ((CBBlockChainStorage *)validatorObj->storage.ptr)->database->current.extraData;
	// Get simple information
	uint16_t branchOffset = CB_EXTRA_BRANCHES + CB_BRANCH_DATA_SIZE * branchNum;
	validatorObj->branches[branchNum].lastRetargetTime = CBArrayToInt32(extraData, branchOffset + CB_BRANCH_LAST_RETARGET);
	validatorObj->branches[branchNum].lastValidation = CBArrayToInt32(extraData, branchOffset + CB_BRANCH_LAST_VALIDATION);
	validatorObj->branches[branchNum].numBlocks = CBArrayToInt32(extraData, branchOffset + CB_BRANCH_NUM_BLOCKS);
	validatorObj->branches[branchNum].parentBlockIndex = CBArrayToInt32(extraData, branchOffset + CB_BRANCH_PARENT_BLOCK_INDEX);
	validatorObj->branches[branchNum].parentBranch = extraData[branchOffset + CB_BRANCH_PARENT_BRANCH];
	validatorObj->branches[branchNum].startHeight = CBArrayToInt32(extraData, branchOffset + CB_BRANCH_START_HEIGHT);
	validatorObj->branches[branchNum].working = false; // Start off not working on any branch.
	return true;
}
bool CBBlockChainStorageLoadBranchWork(void * validator, uint8_t branchNum){
	CBValidator * validatorObj = validator;
	uint8_t * extraData = ((CBBlockChainStorage *)validatorObj->storage.ptr)->database->current.extraData;
	// Get work
	uint16_t workOffset = CB_EXTRA_BRANCHES + CB_BRANCH_DATA_SIZE * branchNum + CB_BRANCH_WORK;
	uint8_t workLen = extraData[workOffset];
	CBBigIntAlloc(&validatorObj->branches[branchNum].work, workLen);
	validatorObj->branches[branchNum].work.length = workLen;
	memcpy(validatorObj->branches[branchNum].work.data, extraData + workOffset + 1, workLen);
	return true;
}
bool CBBlockChainStorageLoadOrphan(void * validator, uint8_t orphanNum){
	CBValidator * validatorObj = validator;
	CBBlockChainStorage * storageObj = validatorObj->storage.ptr;
	uint32_t len;
	if (! CBDatabaseGetLength(storageObj->orphanIndex, (uint8_t []){orphanNum}, &len)){
		CBLogError("There was an error getting the length of an orphan for determining if it exists.");
		return false;
	}
	if (len == CB_DOESNT_EXIST) {
		CBLogError("Attempting to load an orphan that does not exist.");
		return false;
	}
	CBByteArray * orphanData = CBNewByteArrayOfSize(len);
	if (CBDatabaseReadValue(storageObj->orphanIndex, (uint8_t []){orphanNum}, CBByteArrayGetData(orphanData), len, 0, false) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("There was an error when reading the data for an orphan.");
		CBReleaseObject(orphanData);
		return false;
	}
	validatorObj->orphans[orphanNum] = CBNewBlockFromData(orphanData);
	CBReleaseObject(orphanData);
	return true;
}
bool CBBlockChainStorageLoadOutputs(void * validator, uint8_t * txHash, uint8_t ** data, uint32_t * dataAllocSize, uint32_t * position){
	CBBlockChainStorage * storageObj = ((CBValidator *)validator)->storage.ptr;
	uint8_t dataArr[14];
	if (CBDatabaseReadValue(storageObj->txIndex, txHash, dataArr, 14, 0, false) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Could not read a transaction reference from the transaction index.");
		return false;
	}
	// Set position of outputs
	*position = CBArrayToInt32(dataArr, CB_TX_REF_POSITION_OUPTUTS);
	// Get transaction to find position for output in the block
	// Reallocate transaction data memory if needed.
	if (CBArrayToInt32(dataArr, CB_TX_REF_LENGTH_OUTPUTS) > *dataAllocSize) {
		*dataAllocSize = CBArrayToInt32(dataArr, CB_TX_REF_LENGTH_OUTPUTS);
		*data = realloc(*data, *dataAllocSize);
	}
	// Read transaction from the block
	uint8_t key[5];
	key[0] = dataArr[CB_TX_REF_BRANCH];
	memcpy(key + 1, dataArr + CB_TX_REF_BLOCK_INDEX, 4);
	if (CBDatabaseReadValue(storageObj->blockIndex, key, *data, CBArrayToInt32(dataArr, CB_TX_REF_LENGTH_OUTPUTS), CB_BLOCK_START + *position, false) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Could not read a transaction from the block-chain database.");
		return false;
	}
	return true;
}
void * CBBlockChainStorageLoadUnspentOutput(void * validator, uint8_t * txHash, uint32_t outputIndex, bool * coinbase, uint32_t * outputHeight){
	CBValidator * validatorObj = validator;
	CBBlockChainStorage * storageObj = validatorObj->storage.ptr;
	// First read data for the unspent output key.
	uint8_t key[36], data[14];
	memcpy(key, txHash, 32);
	CBInt32ToArray(key, 32, outputIndex);
	if (CBDatabaseReadValue(storageObj->unspentOutputIndex, key, data, 8, 0, false) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Cannot read unspent output information from the block chain database");
		return NULL;
	}
	uint32_t outputPosition = CBArrayToInt32(data, CB_UNSPENT_OUTPUT_REF_POSITION);
	uint32_t outputLength = CBArrayToInt32(data, CB_UNSPENT_OUTPUT_REF_LENGTH);
	// Now read data for the transaction
	if (CBDatabaseReadValue(storageObj->txIndex, key, data, 14, 0, false) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Cannot read a transaction reference from the transaction index.");
		return NULL;
	}
	uint8_t outputBranch = data[CB_TX_REF_BRANCH];
	uint32_t outputBlockIndex = CBArrayToInt32(data, CB_TX_REF_BLOCK_INDEX);
	// Set coinbase
	*coinbase = data[CB_TX_REF_IS_COINBASE];
	// Set output height
	*outputHeight = validatorObj->branches[outputBranch].startHeight + outputBlockIndex;
	// Get the output from storage
	key[0] = outputBranch;
	CBInt32ToArray(key, 1, outputBlockIndex);
	// Get output data
	CBByteArray * outputBytes = CBNewByteArrayOfSize(outputLength);
	if (CBDatabaseReadValue(storageObj->blockIndex, key, CBByteArrayGetData(outputBytes), outputLength, CB_BLOCK_START + outputPosition, false) != CB_DATABASE_INDEX_FOUND) {
		CBLogError("Could not read an unspent output");
		CBReleaseObject(outputBytes);
		return NULL;
	}
	// Create output object
	CBTransactionOutput * output = CBNewTransactionOutputFromData(outputBytes);
	CBReleaseObject(outputBytes);
	if (! CBTransactionOutputDeserialise(output)) {
		CBLogError("Could not deserialise an unspent output");
		CBReleaseObject(output);
		return NULL;
	}
	return output;
}
bool CBBlockChainStorageMoveBlock(void * validator, uint8_t branch, uint32_t blockIndex, uint8_t newBranch, uint32_t newIndex){
	CBBlockChainStorage * storageObj = ((CBValidator *)validator)->storage.ptr;
	uint8_t key[5], newKey[5];
	key[0] = branch;
	CBInt32ToArray(key, 1, blockIndex);
	newKey[0] = newBranch;
	CBInt32ToArray(newKey, 1, blockIndex);
	if (! CBDatabaseChangeKey(storageObj->blockIndex, key, newKey, false)){
		CBLogError("Could not change the key for the location of a block.");
		return false;
	}
	return true;
}
void CBBlockChainStorageRevert(CBDepObject iself){
	CBDatabaseClearCurrent(((CBBlockChainStorage *)iself.ptr)->database);
}
bool CBBlockChainStorageSaveBasicValidator(void * validator){
	CBValidator * validatorObj = validator;
	uint8_t * extraData = ((CBBlockChainStorage *)validatorObj->storage.ptr)->database->current.extraData;
	extraData[CB_EXTRA_BASIC_VALIDATOR_INFO + CB_VALIDATION_FIRST_ORPHAN] = validatorObj->firstOrphan;
	extraData[CB_EXTRA_BASIC_VALIDATOR_INFO + CB_VALIDATION_NUM_ORPHANS] = validatorObj->numOrphans;
	extraData[CB_EXTRA_BASIC_VALIDATOR_INFO + CB_VALIDATION_MAIN_BRANCH] = validatorObj->mainBranch;
	extraData[CB_EXTRA_BASIC_VALIDATOR_INFO + CB_VALIDATION_NUM_BRANCHES] = validatorObj->numBranches;
	return true;
}
bool CBBlockChainStorageSaveBlock(void * validator, void * block, uint8_t branch, uint32_t blockIndex){
	CBValidator * validatorObj = validator;
	CBBlockChainStorage * storageObj = validatorObj->storage.ptr;
	CBBlock * blockObj = block;
	// Write the block data
	uint8_t key[5];
	key[0] = branch;
	CBInt32ToArray(key, 1, blockIndex);
	uint8_t * dataParts[2] = {CBBlockGetHash(blockObj), CBByteArrayGetData(CBGetMessage(blockObj)->bytes)};
	uint32_t dataSizes[2] = {32, CBGetMessage(blockObj)->bytes->length};
	CBDatabaseWriteConcatenatedValue(storageObj->blockIndex, key, 2, dataParts, dataSizes);
	// Write to the block hash index
	CBDatabaseWriteValue(storageObj->blockHashIndex, CBBlockGetHash(blockObj), key, 5);
	return true;
}
bool CBBlockChainStorageSaveBlockHeader(void * validator, void * block, uint8_t branch, uint32_t blockIndex){
	CBValidator * validatorObj = validator;
	CBBlockChainStorage * storageObj = validatorObj->storage.ptr;
	CBBlock * blockObj = block;
	// Write the block data
	uint8_t key[5];
	key[0] = branch;
	CBInt32ToArray(key, 1, blockIndex);
	uint8_t null = 0;
	uint8_t * dataParts[3] = {CBBlockGetHash(blockObj), CBByteArrayGetData(CBGetMessage(blockObj)->bytes), &null};
	uint32_t dataSizes[3] = {32, 80 + CBVarIntDecode(CBGetMessage(blockObj)->bytes, 80).size, 1};
	CBDatabaseWriteConcatenatedValue(storageObj->blockIndex,key, 3, dataParts, dataSizes);
	// Write to the block hash index
	CBDatabaseWriteValue(storageObj->blockHashIndex, CBBlockGetHash(blockObj), key, 5);
	return true;
}
bool CBBlockChainStorageSaveBranch(void * validator, uint8_t branchNum){
	CBValidator * validatorObj = validator;
	uint8_t * extraData = ((CBBlockChainStorage *)validatorObj->storage.ptr)->database->current.extraData;
	// Make data
	uint16_t branchOffset = CB_EXTRA_BRANCHES + CB_BRANCH_DATA_SIZE * branchNum;
	CBInt32ToArray(extraData, branchOffset + CB_BRANCH_LAST_RETARGET, validatorObj->branches[branchNum].lastRetargetTime);
	CBInt32ToArray(extraData, branchOffset + CB_BRANCH_LAST_VALIDATION, validatorObj->branches[branchNum].lastValidation);
	CBInt32ToArray(extraData, branchOffset + CB_BRANCH_NUM_BLOCKS, validatorObj->branches[branchNum].numBlocks);
	CBInt32ToArray(extraData, branchOffset + CB_BRANCH_PARENT_BLOCK_INDEX, validatorObj->branches[branchNum].parentBlockIndex);
	extraData[branchOffset + CB_BRANCH_PARENT_BRANCH] = validatorObj->branches[branchNum].parentBranch;
	CBInt32ToArray(extraData, branchOffset + CB_BRANCH_START_HEIGHT, validatorObj->branches[branchNum].startHeight);
	return true;
}
bool CBBlockChainStorageSaveBranchWork(void * validator, uint8_t branchNum){
	CBValidator * validatorObj = validator;
	uint8_t * extraData = ((CBBlockChainStorage *)validatorObj->storage.ptr)->database->current.extraData;
	uint16_t workOffset = CB_EXTRA_BRANCHES + CB_BRANCH_DATA_SIZE * branchNum + CB_BRANCH_WORK;
	extraData[workOffset] = validatorObj->branches[branchNum].work.length;
	memcpy(extraData + workOffset + 1, validatorObj->branches[branchNum].work.data, extraData[workOffset]);
	return true;
}
bool CBBlockChainStorageSaveOrphan(void * validator, void * block, uint8_t orphanNum){
	CBValidator * validatorObj = validator;
	CBBlockChainStorage * storageObj = validatorObj->storage.ptr;
	CBBlock * blockObj = block;
	CBDatabaseWriteValue(storageObj->orphanIndex, (uint8_t []){orphanNum}, CBByteArrayGetData(CBGetMessage(blockObj)->bytes), CBGetMessage(blockObj)->bytes->length);
	return true;
}
bool CBBlockChainStorageSaveOrphanHeader(void * validator, void * block, uint8_t orphanNum){
	CBValidator * validatorObj = validator;
	CBBlockChainStorage * storageObj = validatorObj->storage.ptr;
	CBBlock * blockObj = block;
	uint8_t null = 0;
	uint8_t * dataParts[3] = {CBByteArrayGetData(CBGetMessage(blockObj)->bytes), &null};
	uint32_t dataSizes[3] = {80 + CBVarIntDecode(CBGetMessage(blockObj)->bytes, 80).size, 1};
	CBDatabaseWriteConcatenatedValue(storageObj->orphanIndex, (uint8_t []){orphanNum}, 2, dataParts, dataSizes);
	return true;
}
bool CBBlockChainStorageSaveTransactionRef(void * validator, uint8_t * txHash, uint8_t branch, uint32_t blockIndex, uint32_t outputPos, uint32_t outputsLen, bool coinbase, uint32_t numOutputs){
	CBValidator * validatorObj = validator;
	CBBlockChainStorage * storageObj = validatorObj->storage.ptr;
	uint32_t len;
	uint8_t data[22];
	if (! CBDatabaseGetLength(storageObj->txIndex, txHash, &len)) {
		CBLogError("There was an error getting the length of a transaction entry to see if it already exists.");
		return false;
	}
	if (len != CB_DOESNT_EXIST) {
		// We have the transaction already. Thus increase the instance count.
		if (CBDatabaseReadValue(storageObj->txIndex, txHash, data + 4, 4,  CB_TX_REF_NUM_UNSPENT_OUTPUTS, false) != CB_DATABASE_INDEX_FOUND) {
			CBLogError("Could not read a transaction reference from the transaction index.");
			return false;
		}
		// Increase the instance count. We change nothing else as we will use the first instance for all other instances.
		CBInt32ToArray(data, 4, CBArrayToInt32(data, 4) + 1);
		// Set the number of unspent outputs back to the number of outputs in the transaction
		CBInt32ToArray(data, 0, numOutputs);
		CBDatabaseWriteValueSubSection(storageObj->txIndex, txHash, data, 8,  CB_TX_REF_NUM_UNSPENT_OUTPUTS);
	}else{
		// This transaction has not yet been seen in the block chain.
		CBInt32ToArray(data, CB_TX_REF_BLOCK_INDEX, blockIndex);
		data[CB_TX_REF_BRANCH] = branch;
		CBInt32ToArray(data, CB_TX_REF_POSITION_OUPTUTS, outputPos);
		CBInt32ToArray(data, CB_TX_REF_LENGTH_OUTPUTS, outputsLen);
		data[CB_TX_REF_IS_COINBASE] = coinbase;
		// We start with an instance count of one
		CBInt32ToArray(data, CB_TX_REF_INSTANCE_COUNT, 1);
		// Set the number of unspent outputs back to the number of outputs in the transaction
		CBInt32ToArray(data, CB_TX_REF_NUM_UNSPENT_OUTPUTS, numOutputs);
		// Write to the transaction index.
		CBDatabaseWriteValue(storageObj->txIndex, txHash, data, 22);
	}
	return true;
}
bool CBBlockChainStorageSaveUnspentOutput(void * validator, uint8_t * txHash, uint32_t outputIndex, uint32_t position, uint32_t length, bool increment){
	CBBlockChainStorage * storageObj = ((CBValidator *)validator)->storage.ptr;
	uint8_t key[36], data[8];
	memcpy(key, txHash, 32);
	CBInt32ToArray(key, 32, outputIndex);
	CBInt32ToArray(data, CB_UNSPENT_OUTPUT_REF_POSITION, position);
	CBInt32ToArray(data, CB_UNSPENT_OUTPUT_REF_LENGTH, length);
	// Add to storage
	CBDatabaseWriteValue(storageObj->unspentOutputIndex, key, data, 8);
	if (increment
		// For the transaction, increment the number of unspent outputs
		&& ! CBBlockChainStorageChangeUnspentOutputsNum(storageObj, txHash, +1)) {
		CBLogError("Could not increment the number of unspent outputs for a transaction.");
		return false;
	}
	return true;
}
CBErrBool CBBlockChainStorageTransactionExists(void * validator, uint8_t * txHash){
	CBValidator * validatorObj = validator;
	CBBlockChainStorage * storageObj = validatorObj->storage.ptr;
	uint32_t len;
	if (! CBDatabaseGetLength(storageObj->txIndex, txHash, &len)) {
		CBLogError("There was an error trying to obtain the length of a transaction entry");
		return CB_ERROR;
	}
	return len != CB_DOESNT_EXIST;
}
CBErrBool CBBlockChainStorageUnspentOutputExists(void * validator, uint8_t * txHash, uint32_t outputIndex){
	CBValidator * validatorObj = validator;
	CBBlockChainStorage * storageObj = validatorObj->storage.ptr;
	uint8_t key[36];
	memcpy(key, txHash, 32);
	CBInt32ToArray(key, 32, outputIndex);
	uint32_t len;
	if (! CBDatabaseGetLength(storageObj->unspentOutputIndex, key, &len)) {
		CBLogError("There was an error trying to obtain the length of an unspent output entry");
		return CB_ERROR;
	}
	return len != CB_DOESNT_EXIST;
}
