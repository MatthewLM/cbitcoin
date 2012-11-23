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

/*CBFullValidator * CBNewFullValidator(char * homeDir, void (*logError)(char *,...)){
	CBFullValidator * self = malloc(sizeof(*self));
	if (NOT self) {
		logError("Cannot allocate %i bytes of memory in CBNewFullNode\n", sizeof(*self));
		return NULL;
	}
	CBGetObject(self)->free = CBFreeFullValidator;
	if (CBInitFullValidator(self, homeDir, logError))
		return self;
	free(self);
	return NULL;
}

//  Object Getter

CBFullValidator * CBGetFullValidator(void * self){
	return self;
}

//  Initialiser

bool CBInitFullValidator(CBFullValidator * self, char * dataDir, void (*logError)(char *,...)){
	if (NOT CBInitObject(CBGetObject(self)))
		return false;
	self->logError = logError;
	// Create block-chain storage object
	self->storage = CBNewBlockChainStorageObject(dataDir, logError);
	if (NOT self->storage){
		logError("Could not create the block chain storage object");
		return false;
	}
	return true;
}

//  Destructor

void CBFreeFullValidator(void * vself){
	CBFullValidator * self = vself;
	CBFreeBLockChainStorageObject(self->storage);
	CBFreeObject(self);
}

//  Functions

bool CBFullValidatorAddBlockToBranch(CBFullValidator * self, uint8_t branch, CBBlock * block, CBBigInt work){
	// The keys for storage
	uint8_t key[7];
	uint8_t data[4];
	key[0] = CB_STORAGE_BRANCHES;
	key[1] = branch;
	// Reallocate memory
	// Reallocate memory for the references
	CBBlockReference * temp = realloc(self->branches[branch].references, sizeof(*self->branches[branch].references) * (self->branches[branch].numRefs + 1));
	if (NOT temp)
		return false;
	self->branches[branch].references = temp;
	// Reallocate memory for the lookup table
	CBBlockReferenceHashIndex * temp2 = realloc(self->branches[branch].referenceTable, sizeof(*self->branches[branch].referenceTable) * (self->branches[branch].numRefs + 1));
	if (NOT temp2)
		return false;
	self->branches[branch].referenceTable = temp2;
	bool found;
	// Get the index position for the lookup table.
	uint32_t indexPos = CBFullValidatorFindBlockReference(self->branches[branch].referenceTable, self->branches[branch].numRefs, CBBlockGetHash(block), &found);
	// Adjust size of unspent output information and make sure that is OK.
	uint32_t temp3 = self->branches[branch].numUnspentOutputs;
	for (uint32_t x = 0; x < block->transactionNum; x++){
		// For each input, an output has been spent but only do this for non-coinbase transactions sicne coinbase transactions make new coins.
		if (x)
			temp3 -= block->transactions[x]->inputNum;
		// For each output, we have another unspent output.
		temp3 += block->transactions[x]->outputNum;
	}
	// Get the index of the block reference and increase the number of references.
	uint32_t refIndex = self->branches[branch].numRefs++;
	key[2] = CB_BRANCH_NUM_REFS;
	CBInt32ToArray(data, 0, self->branches[branch].numRefs);
	if (NOT CBBlockChainStorageWriteValue(self->storage, key, 3, data, 4)){
		self->logError("There was an error updating the number of references for branch %u.", branch);
		return false;
	}
	// Reallocate for new size
	CBOutputReference * temp4 = realloc(self->branches[branch].unspentOutputs, temp3 * sizeof(*self->branches[branch].unspentOutputs));
	if (NOT temp4)
		return false;
	self->branches[branch].unspentOutputs = temp4;
	// Prepare serialisation byte array
	CBByteArray * data = CBNewByteArrayOfSize(self->branches[branch].numRefs*54 + 54 + temp3*52 + 26 + work.length, self->logError);
	if (NOT data)
		return false;
	// Modify memory
	// Modify validator information. Insert new reference. This involves adding the reference to the end of the refence data and inserting an index into a lookup table.
	// Insert reference index into lookup table
	if (indexPos < self->branches[branch].numRefs - 1)
		// Move references up
		memmove(self->branches[branch].referenceTable + indexPos + 1, self->branches[branch].referenceTable + indexPos, sizeof(*self->branches[branch].referenceTable) * (self->branches[branch].numRefs - indexPos - 1));
	self->branches[branch].referenceTable[indexPos].index = refIndex;
	memcpy(self->branches[branch].referenceTable[indexPos].blockHash,CBBlockGetHash(block), 32);
	// Reference index to storage
	key[2] = CB_BRANCH_TABLE_HASH;
	CBInt32ToArray(key, 3, indexPos);
	if (NOT CBBlockChainStorageWriteValue(self->storage, key, 7, self->branches[branch].referenceTable[indexPos].blockHash, 32)){
		self->logError("There was an error updating the block hash number %i in the lookup table for branch %u.", indexPos, branch);
		return false;
	}
	key[2] = CB_BRANCH_TABLE_INDEX;
	CBInt32ToArray(data, 0, self->branches[branch].referenceTable[indexPos].index);
	if (NOT CBBlockChainStorageWriteValue(self->storage, key, 7, data, 4)){
		self->logError("There was an error updating the block index number %i in the lookup table for branch %u.", indexPos, branch);
		return false;
	}
	// Update branch data
	if (NOT (self->branches[branch].startHeight + self->branches[branch].numRefs) % 2016){
		self->branches[branch].lastRetargetTime = block->time;
		key[2] = CB_BRANCH_LAST_RETARGET;
		CBInt32ToArray(data, 0, block->time);
		if (NOT CBBlockChainStorageWriteValue(self->storage, key, 3, data, 4)){
			self->logError("There was an error updating the last retarget time for branch %u.", branch);
			return false;
		}
	}
	free(self->branches[branch].work.data);
	self->branches[branch].work = work;
	key[2] = CB_BRANCH_WORK;
	if (NOT CBBlockChainStorageWriteValue(self->storage, key, 3, work.data, work.length)){
		self->logError("There was an error updating the total work for branch %u.", branch);
		return false;
	}
	// Insert block data
	self->branches[branch].references[refIndex].target = block->target;
	self->branches[branch].references[refIndex].time = block->time;
	// Update unspent outputs... Go through transactions, removing the prevOut references and adding the outputs for one transaction at a time.
	uint32_t cursor = 80; // Cursor to find output positions.
	uint8_t byte = CBByteArrayGetByte(CBGetMessage(block)->bytes, 80);
	cursor += byte < 253 ? 1 : (byte == 253 ? 2 : (byte == 254 ? 4 : 8));
	for (uint32_t x = 0; x < block->transactionNum; x++) {
		bool found;
		cursor += 4; // Move along version number
		// Move along input number
		byte = CBByteArrayGetByte(CBGetMessage(block)->bytes, cursor);
		cursor += byte < 253 ? 1 : (byte == 253 ? 2 : (byte == 254 ? 4 : 8));
		// First remove output references than add new outputs.
		for (uint32_t y = 0; y < block->transactions[x]->inputNum; y++) {
			if (x) {
				// Only remove for non-coinbase transactions
				uint32_t ref = CBFullValidatorFindOutputReference(self->branches[branch].unspentOutputs, self->branches[branch].numUnspentOutputs, CBByteArrayGetData(block->transactions[x]->inputs[y]->prevOut.hash), block->transactions[x]->inputs[y]->prevOut.index, &found);
				// Remove by overwrite.
				memmove(self->branches[branch].unspentOutputs + ref, self->branches[branch].unspentOutputs + ref + 1, (self->branches[branch].numUnspentOutputs - ref - 1) * sizeof(*self->branches[branch].unspentOutputs));
				self->branches[branch].numUnspentOutputs--;
			}
			// Move cursor along script varint. We look at byte data in case it is longer than needed.
			uint8_t byte = CBByteArrayGetByte(CBGetMessage(block)->bytes, cursor);
			cursor += byte < 253 ? 1 : (byte == 253 ? 2 : (byte == 254 ? 4 : 8));
			// Move along script and output reference
			cursor += block->transactions[x]->inputs[y]->scriptObject->length + 36;
			// Move cursor along sequence
			cursor += 4;
		}
		// Move cursor past output number to first output
		byte = CBByteArrayGetByte(CBGetMessage(block)->bytes, cursor);
		cursor += byte < 253 ? 1 : (byte == 253 ? 2 : (byte == 254 ? 4 : 8));
		// Now add new outputs
		for (uint32_t y = 0; y < block->transactions[x]->outputNum; y++) {
			uint32_t ref = CBFullValidatorFindOutputReference(self->branches[branch].unspentOutputs, self->branches[branch].numUnspentOutputs, CBTransactionGetHash(block->transactions[x]), y, &found);
			// Insert the output information at the reference point.
			if (self->branches[branch].numUnspentOutputs > ref)
				// Move other references up to make room
				memmove(self->branches[branch].unspentOutputs + ref + 1, self->branches[branch].unspentOutputs + ref, (self->branches[branch].numUnspentOutputs - ref) * sizeof(*self->branches[branch].unspentOutputs));
			self->branches[branch].numUnspentOutputs++;
			CBInt32ToArray(key, 3, y);
			key[2] = CB_BRANCH_UNSPENT_OUTPUT_BRANCH;
			self->branches[branch].unspentOutputs[ref].branch = branch;
			CBInt32ToArray(data, 0, branch);
			if (NOT CBBlockChainStorageWriteValue(self->storage, key, 7, data, 4)){
				self->logError("There was an error updating the branch for unspent output %u for branch %u.", ref, branch);
				return false;
			}
			data[0] = NOT x;
			self->branches[branch].unspentOutputs[ref].coinbase = data[0];
			key[2] = CB_BRANCH_UNSPENT_OUTPUT_COINBASE;
			if (NOT CBBlockChainStorageWriteValue(self->storage, key, 7, data, 1)){
				self->logError("There was an error updating the coinbase bool for unspent output %u for branch %u.", ref, branch);
				return false;
			}
			memcpy(self->branches[branch].unspentOutputs[ref].outputHash,CBTransactionGetHash(block->transactions[x]),32);
			key[2] = CB_BRANCH_UNSPENT_OUTPUT_HASH;
			if (NOT CBBlockChainStorageWriteValue(self->storage, key, 7, CBTransactionGetHash(block->transactions[x]), 32)){
				self->logError("There was an error updating the hash for unspent output %u for branch %u.", ref, branch);
				return false;
			}
			self->branches[branch].unspentOutputs[ref].outputIndex = y;
			key[2] = CB_BRANCH_UNSPENT_OUTPUT_INDEX;
			CBInt32ToArray(data, 0, y);
			if (NOT CBBlockChainStorageWriteValue(self->storage, key, 7, data, 4)){
				self->logError("There was an error updating the index for unspent output %u for branch %u.", ref, branch);
				return false;
			}
			self->branches[branch].unspentOutputs[ref].position = cursor - 80; // Block cursor minus header
			key[2] = CB_BRANCH_UNSPENT_OUTPUT_POS;
			CBInt32ToArray(data, 0, cursor);
			if (NOT CBBlockChainStorageWriteValue(self->storage, key, 7, data, 4)){
				self->logError("There was an error updating the position for unspent output %u for branch %u.", ref, branch);
				return false;
			}
			self->branches[branch].unspentOutputs[ref].blockIndex = refIndex;
			key[2] = CB_BRANCH_UNSPENT_OUTPUT_BLOCK_INDEX;
			CBInt32ToArray(data, 0, refIndex);
			if (NOT CBBlockChainStorageWriteValue(self->storage, key, 7, data, 4)){
				self->logError("There was an error updating the block index for the unspent output %u for branch %u.", ref, branch);
				return false;
			}
			// Move cursor past the output
			uint8_t byte = CBByteArrayGetByte(CBGetMessage(block)->bytes, cursor);
			cursor += byte < 253 ? 1 : (byte == 253 ? 2 : (byte == 254 ? 4 : 8));
			cursor += 8;
		}
	}
	// Update number of unspent outputs.
	key[2] = CB_BRANCH_NUM_UNSPENT_OUTPUTS;
	CBInt32ToArray(data, 0, self->branches[branch].numUnspentOutputs);
	if (NOT CBBlockChainStorageWriteValue(self->storage, key, 3, data, 4)){
		self->logError("There was an error updating the number of unspent outputs for branch %u.", branch);
		return false;
	}
	// Store the block
	key[2] = CB_BRANCH_BLOCK_HEADER;
	CBInt32ToArray(key, 3, refIndex);
	// Write block data
	if (NOT CBBlockChainStorageWriteValue(self->storage, key, 7, CBByteArrayGetData(CBGetMessage(block)->bytes), 80)){
		self->logError("There was an error writing the header for block%u-%u.", branch, refIndex);
		return false;
	}
	key[2] = CB_BRANCH_BLOCK_TRANSACTIONS;
	if (NOT CBBlockChainStorageWriteValue(self->storage, key, 7, CBByteArrayGetData(CBGetMessage(block)->bytes) + 80, CBGetMessage(block)->bytes->length - 80)){
		self->logError("There was an error writing the transactions for block%u-%u.", branch, refIndex);
		return false;
	}
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
		key[0] = CB_STORAGE_FIRST_ORPHAN;
		if (NOT CBBlockChainStorageWriteValue(self->storage, key, 1, &self->firstOrphan, 1)){
			self->logError("There was an error writing the index of the first orphan.");
			return false;
		}
	}else{
		// Adding an orphan.
		pos = self->numOrphans++;
		key[0] = CB_STORAGE_NUM_ORPHANS;
		if (NOT CBBlockChainStorageWriteValue(self->storage, key, 1, &self->numOrphans, 1)){
			self->logError("There was an error writing the number of orphans.");
			return false;
		}
	}
	// Add to memory
	self->orphans[pos] = block;
	CBRetainObject(block);
	// Add to storage
	key[0] = CB_STORAGE_ORPHANS;
	key[1] = pos;
	if (NOT CBBlockChainStorageWriteValue(self->storage, key, 2, CBByteArrayGetData(CBGetMessage(block)->bytes), CBGetMessage(block)->bytes->length)){
		self->logError("There was an error writing an orphan at position %u.", pos);
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
	for (uint8_t x = 0; x < self->numBranches; x++){
		bool found;
		CBFullValidatorFindBlockReference(self->branches[x].referenceTable, self->branches[x].numRefs, hash, &found);
		if (found)
			return CB_BLOCK_STATUS_DUPLICATE;
	}
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
			return self->branches[branch].references[prevIndex].time;
		}
		branch = self->branches[branch].parentBranch;
		prevIndex = self->branches[branch].numRefs - 1;
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
		CBOutputReference * outRef;
		bool found;
		uint32_t i = CBFullValidatorFindOutputReference(self->branches[branch].unspentOutputs, self->branches[branch].numUnspentOutputs,CBByteArrayGetData(allSpentOutputs[transactionIndex][inputIndex].hash),allSpentOutputs[transactionIndex][inputIndex].index,&found);
		if (NOT found)
			// No unspent outputs for this input.
			return CB_BLOCK_VALIDATION_BAD;
		outRef = self->branches[branch].unspentOutputs + i;
		// Check coinbase maturity
		if (outRef->coinbase && blockHeight - outRef->height < CB_COINBASE_MATURITY) 
			return CB_BLOCK_VALIDATION_BAD;
		// Get the output from storage
		uint8_t key[6];
		key[0] = CB_STORAGE_BRANCHES;
		key[1] = outRef->branch;
		key[2] = CB_BRANCH_BLOCK_TRANSACTIONS;
		CBInt32ToArray(key, 3, outRef->blockIndex);
		uint32_t size;
		uint8_t * data = CBBlockChainStorageReadValue(self->storage, key, 7, &size);
		if (NOT data) {
			self->logError("Could not read block transaction data for branch %u and index %u.",outRef->branch, outRef->blockIndex);
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
uint32_t CBFullValidatorFindBlockReference(CBBlockReferenceHashIndex * lookupTable, uint32_t refNum, uint8_t * hash, bool * found){
	// Block branch block reference lists and the unspent output reference list, use sorted lists, therefore this uses an interpolation search which is an optimsation on binary search.
	if (NOT refNum){
		// No references yet so add at index 0
		*found = false;
		return 0;
	}
	uint32_t left = 0;
	uint32_t right = refNum - 1;
	uint64_t miniKey = CBHashMiniKey(hash);
	uint64_t leftMiniKey;
	uint64_t rightMiniKey;
	for (;;) {
		CBBlockReferenceHashIndex * leftRef = lookupTable + left;
		CBBlockReferenceHashIndex * rightRef = lookupTable + right;
		if (memcmp(hash, rightRef->blockHash, 32) > 0){
			*found = false;
			return right + 1; // Above the right
		}
		if (memcmp(hash, leftRef->blockHash, 32) < 0) {
			*found = false;
			return left; // Below the left.
		}
		uint32_t pos;
		leftMiniKey = CBHashMiniKey(leftRef->blockHash);
		rightMiniKey = CBHashMiniKey(rightRef->blockHash);
		if (rightMiniKey - leftMiniKey)
			pos = left + (uint32_t)( (right - left) * (miniKey - leftMiniKey) ) / (rightMiniKey - leftMiniKey);
		else
			pos = (right + left)/2;
		int res = memcmp(hash, lookupTable + pos, 32);
		if (NOT res){
			*found = true;
			return pos;
		}
		else if (res > 0)
			left = pos + 1;
		else
			right = pos - 1;
	}
}
uint32_t CBFullValidatorFindOutputReference(CBOutputReference * refs, uint32_t refNum, uint8_t * hash, uint32_t index, bool * found){
	// Same as CBFullValidatorFindBlockReference but for ouutput references.
	if (NOT refNum) {
		*found = false;
		return 0;
	}
	uint32_t left = 0;
	uint32_t right = refNum - 1;
	uint64_t miniKey = CBHashMiniKey(hash);
	uint64_t leftMiniKey;
	uint64_t rightMiniKey;
	for (;;) {
		CBOutputReference * leftRef = refs + left;
		CBOutputReference * rightRef = refs + right;
		int res = memcmp(hash, rightRef->outputHash, 32);
		if (res > 0 || (NOT res && index > rightRef->outputIndex)) {
			*found = false;
			return right + 1; // Above the right
		}
		res = memcmp(hash, leftRef->outputHash, 32);
		if (res < 0 || (NOT res && index < leftRef->outputIndex)) {
			*found = false;
			return left; // Below the left.
		}
		uint32_t pos;
		if (rightMiniKey - leftMiniKey){
			leftMiniKey = CBHashMiniKey(leftRef->outputHash);
			rightMiniKey = CBHashMiniKey(rightRef->outputHash);
			if (rightMiniKey - leftMiniKey)
				pos = left + (uint32_t)( (right - left) * (miniKey - leftMiniKey) ) / (rightMiniKey - leftMiniKey);
			else
				pos = pos = (right + left)/2;
		}else
			pos = pos = (right + left)/2;
		res = memcmp(hash, (refs + pos)->outputHash, 32);
		if (NOT res && index == (refs + pos)->outputIndex){
			*found = true;
			return pos;
		}
		else if (res > 0 || (NOT res && index > (refs + pos)->outputIndex))
			left = pos + 1;
		else
			right = pos - 1;
	}
}
CBBlock * CBFullValidatorLoadBlock(CBFullValidator * self, CBBlockReference blockRef, uint32_t branch){
	// Open the file
	char blockFile[strlen(self->dataDir) + 22];
	sprintf(blockFile, "%sblocks%u-%u.dat",self->dataDir, branch, blockRef.ref.fileID);
	FILE * fd = fopen(blockFile, "rb");
	if (NOT fd)
		return NULL;
	// Now we have the file and it is open we need the block. Get the length of the block.
	uint8_t length[4];
	fseek(fd, blockRef.ref.filePos, SEEK_SET);
	if (fread(length, 1, 4, fd) != 4){
		fclose(fd);
		return NULL;
	}
	CBByteArray * data = CBNewByteArrayOfSize(length[3] << 24 | length[2] << 16 | length[1] << 8 | length[0], self->logError);
	// Now read block data
	if (fread(CBByteArrayGetData(data), 1, data->length, fd) != data->length) {
		CBReleaseObject(data);
		fclose(fd);
		return NULL;
	}
	fclose(fd);
	// Make and return the block
	CBBlock * block = CBNewBlockFromData(data, self->logError);
	CBReleaseObject(data);
	if (NOT block)
		return NULL;
	return block;
}
CBValidatorLoadResult CBFullValidatorLoadBranchValidator(CBFullValidator * self, uint8_t branch){
	// Open branch data file
	unsigned long dataDirLen = strlen(self->dataDir);
	char branchFilePath[dataDirLen + strlen(CB_ADDRESS_DATA_FILE) + 1];
	sprintf(branchFilePath, "%sbranch%u.dat",self->dataDir, branch);
	if (access(branchFilePath, F_OK)) {
		// The validator file exists, open it
		FILE * file = fopen(branchFilePath, "rb");
		if (NOT file)
			return CB_VALIDATOR_LOAD_ERR;
		// Get the file length
		struct stat stbuf;
		if (fstat(fileno(file), &stbuf)){
			fclose(file);
			return CB_VALIDATOR_LOAD_ERR;
		}
		unsigned long fileLen = stbuf.st_size;
		// Copy file contents into buffer.
		CBByteArray * buffer = CBNewByteArrayOfSize((uint32_t)fileLen, self->logError);
		if (NOT buffer) {
			fclose(file);
			self->logError("Could not create buffer of size %u.",fileLen);
			return CB_VALIDATOR_LOAD_ERR;
		}
		size_t res = fread(CBByteArrayGetData(buffer), 1, fileLen, file);
		if(res != fileLen){
			CBReleaseObject(buffer);
			self->logError("Could not read %u bytes of data into buffer. fread returned %u",fileLen,res);
			return CB_VALIDATOR_LOAD_ERR;
		}
		// Deserailise data
		if (buffer->length >= 26){
			self->branches[branch].numRefs = CBByteArrayReadInt32(buffer, 0);
			if (buffer->length >= 26 + self->branches[branch].numRefs*54) {
				self->branches[branch].references = malloc(sizeof(*self->branches[branch].references) * self->branches[branch].numRefs);
				if (self->branches[branch].references) {
					self->branches[branch].referenceTable = malloc(sizeof(*self->branches[branch].referenceTable) * self->branches[branch].numRefs);
					if (self->branches[branch].referenceTable) {
						uint32_t cursor = 4;
						for (uint32_t x = 0; x < self->branches[branch].numRefs; x++) {
							// Load block reference
							self->branches[branch].references[x].ref.fileID = CBByteArrayReadInt16(buffer, cursor);
							cursor += 2;
							self->branches[branch].references[x].ref.filePos = CBByteArrayReadInt64(buffer, cursor);
							cursor += 8;
							self->branches[branch].references[x].target = CBByteArrayReadInt32(buffer, cursor);
							cursor += 4;
							self->branches[branch].references[x].time = CBByteArrayReadInt32(buffer, cursor);
							cursor += 4;
							// Load block reference index
							memcpy(self->branches[branch].referenceTable[x].blockHash, CBByteArrayGetData(buffer) + cursor, 32);
							cursor += 32;
							self->branches[branch].referenceTable[x].index = CBByteArrayReadInt32(buffer, cursor);
							cursor += 4;
						}
						self->branches[branch].lastRetargetTime = CBByteArrayReadInt32(buffer, cursor);
						cursor += 4;
						self->branches[branch].parentBranch = CBByteArrayGetByte(buffer, cursor);
						cursor++;
						self->branches[branch].parentBlockIndex = CBByteArrayReadInt32(buffer, cursor);
						cursor+= 4;
						self->branches[branch].startHeight = CBByteArrayReadInt32(buffer, cursor);
						cursor+= 4;
						self->branches[branch].lastValidation = CBByteArrayReadInt32(buffer, cursor);
						cursor+= 4;
						self->branches[branch].numUnspentOutputs = CBByteArrayReadInt32(buffer, cursor);
						cursor += 4;
						if (buffer->length >= cursor + self->branches[branch].numUnspentOutputs*52 + 1) {
							self->branches[branch].unspentOutputs = malloc(sizeof(*self->branches[branch].unspentOutputs) * self->branches[branch].numUnspentOutputs);
							if (self->branches[branch].unspentOutputs) {
								for (uint32_t x = 0; x < self->branches[branch].numUnspentOutputs; x++) {
									memcpy(self->branches[branch].unspentOutputs[x].outputHash, CBByteArrayGetData(buffer) + cursor, 32);
									cursor += 32;
									self->branches[branch].unspentOutputs[x].outputIndex = CBByteArrayReadInt32(buffer, cursor);
									cursor += 4;
									self->branches[branch].unspentOutputs[x].ref.fileID = CBByteArrayReadInt16(buffer, cursor);
									cursor += 2;
									self->branches[branch].unspentOutputs[x].ref.filePos = CBByteArrayReadInt64(buffer, cursor);
									cursor += 8;
									self->branches[branch].unspentOutputs[x].height = CBByteArrayReadInt32(buffer, cursor);
									cursor += 4;
									self->branches[branch].unspentOutputs[x].coinbase = CBByteArrayGetByte(buffer, cursor);
									cursor += 1;
									self->branches[branch].unspentOutputs[x].branch = CBByteArrayGetByte(buffer, cursor);
									cursor += 1;
								}
								// Get work
								self->branches[branch].work.length = CBByteArrayGetByte(buffer, cursor);
								cursor++;
								if (buffer->length >= cursor + self->branches[branch].work.length) {
									// Allocate bigint
									if (CBBigIntAlloc(&self->branches[branch].work, self->branches[branch].work.length)) {
										memcpy(self->branches[branch].work.data, CBByteArrayGetData(buffer) + cursor,self->branches[branch].work.length);
										return CB_VALIDATOR_LOAD_OK;
									}else
										self->logError("CBBigIntAloc failed in CBFullValidatorLoadBranchValidator for %u bytes", self->branches[branch].work.length);
								}else
									self->logError("Not enough data for the branch work %u < %u",buffer->length, cursor + self->branches[branch].work.length);
							}else
								self->logError("Could not allocate %u bytes of memory for unspent outputs in CBFullValidatorLoadBranchValidator for branch",sizeof(*self->branches[branch].unspentOutputs) * self->branches[branch].numUnspentOutputs);
						}else
							self->logError("There is not enough data for the unspent outputs. %i < %i",buffer->length, cursor + self->branches[branch].numUnspentOutputs*52 + 1);
					}else
						self->logError("Could not allocate %u bytes of memory for the reference table in CBFullValidatorLoadBranchValidator.",sizeof(*self->branches[branch].referenceTable) * self->branches[branch].numRefs);
					free(self->branches[branch].references);
				}else
					self->logError("Could not allocate %u bytes of memory for references in CBFullValidatorLoadBranchValidator.",sizeof(*self->branches[branch].references) * self->branches[branch].numRefs);
			}else
				self->logError("Not enough data for the references %u < %u",buffer->length, 26 + self->branches[branch].numRefs*54);
		}else
			self->logError("Not enough data for the number of references %u < 26",buffer->length);
		CBReleaseObject(buffer);
		return CB_VALIDATOR_LOAD_ERR;
	}else if (NOT branch){
		// The branch file does not exist.
		// Prepare output allocation
		if (NOT CBSafeOutputAlloc(self->output, 2)){
			self->logError("Could not allocate memory for output in CBFullValidatorLoadBranchValidator.");
			return CB_VALIDATOR_LOAD_ERR;
		}
		// Allocate data
		self->branches[0].references = malloc(sizeof(*self->branches[0].references));
		if (NOT self->branches[0].references) {
			self->logError("Could not allocate %u byte of memory for the genesis reference in CBFullValidatorLoadBranchValidator.",sizeof(*self->branches[0].references));
			return CB_VALIDATOR_LOAD_ERR;
		}
		self->branches[0].referenceTable = malloc(sizeof(*self->branches[0].referenceTable));
		if (NOT self->branches[0].referenceTable) {
			self->logError("Could not allocate %u bytes of memory for the reference lookup table in CBFullValidatorLoadBranchValidator.",sizeof(*self->branches[0].referenceTable));
			free(self->branches[0].references);
			return CB_VALIDATOR_LOAD_ERR;
		}
		// Initialise data with the genesis block.
		self->branches[0].lastRetargetTime = 1231006505;
		self->branches[0].startHeight = 0;
		self->branches[0].numRefs = 1;
		self->branches[0].lastValidation = 0;
		uint8_t genesisHash[32] = {0x6F,0xE2,0x8C,0x0A,0xB6,0xF1,0xB3,0x72,0xC1,0xA6,0xA2,0x46,0xAE,0x63,0xF7,0x4F,0x93,0x1E,0x83,0x65,0xE1,0x5A,0x08,0x9C,0x68,0xD6,0x19,0x00,0x00,0x00,0x00,0x00};
		self->branches[0].references[0].ref.fileID = 0;
		self->branches[0].references[0].ref.filePos = 0;
		self->branches[0].references[0].target = CB_MAX_TARGET;
		self->branches[0].references[0].time = 1231006505;
		self->branches[0].work.length = 1;
		self->branches[0].work.data = malloc(1);
		if (NOT self->branches[0].work.data) {
			self->logError("Could not allocate 1 byte of memory for the initial branch work in CBFullValidatorLoadBranchValidator.");
			free(self->branches[0].references);
			free(self->branches[0].referenceTable);
			free(self->branches[0].unspentOutputs);
			return CB_VALIDATOR_LOAD_ERR;
		}
		self->branches[0].work.data[0] = 0;
		memcpy(self->branches[0].referenceTable[0].blockHash,genesisHash,32);
		self->branches[0].referenceTable[0].index = 0;
		// The genesis output is not counted.
		self->branches[0].numUnspentOutputs = 0;
		self->branches[0].unspentOutputs = NULL;
		// Write genesis block to the first block file
		char blockFilePath[dataDirLen + 14];
		memcpy(blockFilePath, self->dataDir, dataDirLen);
		strcpy(blockFilePath + dataDirLen, "blocks0-0.dat");
		// Write genesis block data. Begins with length.
		CBSafeOutputAddSaveFileOperation(self->output, blockFilePath, (uint8_t []){
			0x1D,0x01,0x00,0x00, // The Length of the block is 285 bytes or 0x11D
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
			0x2B,0x6B,0xF1,0x1D,0x5F,0xAC,0x00,0x00,0x00,0x00}, 289);
		// Write to the branch file
		CBSafeOutputAddSaveFileOperation(self->output, branchFilePath, (uint8_t [81]){
			0x01,0x00,0x00,0x00, // One reference
			0x00,0x00, // File ID
			0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // File Pos
			0xFF, 0xFF, 0xFF, 0x1D, // Target
			0x29,0xAB,0x5F,0x49, // Time-stamp
			0x6F,0xE2,0x8C,0x0A,0xB6,0xF1,0xB3,0x72,0xC1,0xA6,0xA2,0x46,0xAE,0x63,0xF7,0x4F,0x93,0x1E,0x83,0x65,0xE1,0x5A,0x08,0x9C,0x68,0xD6,0x19,0x00,0x00,0x00,0x00,0x00, // Hash
			0x00,0x00,0x00,0x00, // Index
			0x29,0xAB,0x5F,0x49, // Last retarget time
			0x00, // Parent branch
			0x00,0x00,0x00,0x00, // Index of block on previous branch
			0x00,0x00,0x00,0x00, // Start height
			0x00,0x00,0x00,0x00, // Last validation
			0x00,0x00,0x00,0x00, // Num unspent outputs
			0x01, // Work length
			0x00, // Work
		}, 81);
		// Commit
		CBSafeOutputResult res = CBSafeOutputCommit(self->output, self->backup);
		if (res == CB_SAFE_OUTPUT_OK)
			return CB_VALIDATOR_LOAD_OK;
		return CB_VALIDATOR_LOAD_ERR_DATA;
	}
	return CB_VALIDATOR_LOAD_ERR;
}
CBValidatorLoadResult CBFullValidatorLoadValidator(CBFullValidator * self){
	// Get file name
	char validatorFilePath[strlen(self->dataDir) + strlen(CB_VALIDATION_DATA_FILE) + 1];
	memcpy(validatorFilePath, self->dataDir, strlen(self->dataDir));
	strcpy(validatorFilePath + strlen(self->dataDir), CB_VALIDATION_DATA_FILE);
	if (access(validatorFilePath, F_OK)) {
		// File exists, open for reading
		FILE * file = fopen(validatorFilePath, "rb");
		if (file) {
			// Get the file length
			struct stat stbuf;
			if (fstat(fileno(file), &stbuf)){
				fclose(file);
				return CB_VALIDATOR_LOAD_ERR;
			}
			unsigned long fileLen = stbuf.st_size;
			// Copy file contents into buffer.
			CBByteArray * buffer = CBNewByteArrayOfSize((uint32_t)fileLen, self->logError);
			if (NOT buffer) {
				fclose(file);
				self->logError("Could not create buffer of size %u.",fileLen);
				return CB_VALIDATOR_LOAD_ERR;
			}
			size_t res = fread(CBByteArrayGetData(buffer), 1, fileLen, file);
			if(res != fileLen){
				CBReleaseObject(buffer);
				fclose(file);
				self->logError("Could not read %u bytes of data into buffer. fread returned %u",fileLen,res);
				return CB_VALIDATOR_LOAD_ERR;
			}
			// Deserailise data
			if (buffer->length >= 3){
				self->mainBranch = CBByteArrayGetByte(buffer, 0);
				self->numBranches = CBByteArrayGetByte(buffer, 1);
				// Now do orhpans
				self->numOrphans = CBByteArrayGetByte(buffer, 2);
				uint8_t cursor = 3;
				if (buffer->length >= cursor + self->numOrphans*82){
					for (uint8_t x = 0; x < self->numOrphans; x++) {
						bool err = false;
						CBByteArray * data = CBNewByteArraySubReference(buffer, cursor, buffer->length - 56);
						if (data) {
							self->orphans[x] = CBNewBlockFromData(data, self->logError);
							if (self->orphans[x]) {
								// Deserialise
								uint32_t len = CBBlockDeserialise(self->orphans[x], true);
								if (len) {
									data->length = len;
									// Make orhpan block data a copy of the subreference.
									CBGetMessage(self->orphans[x])->bytes = CBByteArrayCopy(data);
									if (CBGetMessage(self->orphans[x])->bytes) {
										CBReleaseObject(data);
										CBReleaseObject(data);
									}else{
										self->logError("Could not create byte copy for orphan %u",x);
										err = true;
									}
								}else{
									self->logError("Could not deserailise orphan %u",x);
									err = true;
								}
								if (err)
									CBReleaseObject(self->orphans[x]);
							}else{
								self->logError("Could not create block object for orphan %u",x);
								err = true;
							}
							if (err)
								CBReleaseObject(data);
						}else{
							self->logError("Could not create byte reference for orphan %u",x);
							err = true;
						}
						if (err){
							for (uint8_t y = 0; y < x; y++)
								CBReleaseObject(self->orphans[y]);
							CBReleaseObject(buffer);
							fclose(file);
							return CB_VALIDATOR_LOAD_ERR;
						}
					}
					// Success
					CBReleaseObject(buffer);
					// All done
					return CB_VALIDATOR_LOAD_OK;
				}else
					self->logError("There is not enough data for the orhpans. %i < %i",buffer->length, cursor + self->numOrphans*82);
			}else
				self->logError("Not enough data for the minimum required data %u < 3",buffer->length);
			CBReleaseObject(buffer);
			fclose(file);
			return CB_VALIDATOR_LOAD_ERR;
		}
		self->logError("Could not open validation data");
		return CB_VALIDATOR_LOAD_ERR;
	}
	// Create initial data
	// Create directory if it doesn't already exist
	if(mkdir(self->dataDir, S_IREAD | S_IWRITE))
		if (errno != EEXIST){
			self->logError("Could not create the data directory.");
			return CB_VALIDATOR_LOAD_ERR;
		}
	// Open validator file
	self->mainBranch = 0;
	self->numBranches = 1;
	self->numOrphans = 0;
	// Write initial validator data
	if (NOT CBSafeOutputAlloc(self->output, 1)){
		self->logError("Could not allocate memory for the output.");
		return CB_VALIDATOR_LOAD_ERR;
	}
	CBSafeOutputAddSaveFileOperation(self->output, validatorFilePath, (uint8_t []){0,1,0}, 3);
	// Commit
	CBSafeOutputResult res = CBSafeOutputCommit(self->output, self->backup);
	if (res == CB_SAFE_OUTPUT_OK)
		return CB_VALIDATOR_LOAD_OK;
	return CB_VALIDATOR_LOAD_ERR_DATA;
}

CBBlockStatus CBFullValidatorProcessBlock(CBFullValidator * self, CBBlock * block, uint64_t networkTime){
	bool found;
	// Get transaction hashes.
	uint8_t * txHashes = malloc(32 * block->transactionNum);
	if (NOT txHashes)
		return CB_BLOCK_STATUS_ERROR;
	// Put the hashes for transactions into a list.
	for (uint32_t x = 0; x < block->transactionNum; x++)
		memcpy(txHashes + 32*x, CBTransactionGetHash(block->transactions[x]), 32);
	// Determine what type of block this is.
	uint8_t prevBranch = 0;
	uint32_t prevBlockIndex;
	for (; prevBranch < self->numBranches; prevBranch++){
		uint32_t refIndex = CBFullValidatorFindBlockReference(self->branches[prevBranch].referenceTable, self->branches[prevBranch].numRefs, CBByteArrayGetData(block->prevBlockHash),&found);
		if (found){
			// The block is extending this branch or creating a side branch to this branch
			prevBlockIndex = self->branches[prevBranch].referenceTable[refIndex].index;
			break;
		}
	}
	if (prevBranch == self->numBranches){
		// Orphan block. End here.
		// Do basic validation
		CBBlockStatus res = CBFullValidatorBasicBlockValidation(self, block, txHashes, networkTime);
		free(txHashes);
		if (res != CB_BLOCK_STATUS_CONTINUE)
			return res;
		// Add block to orphans
		if(CBFullValidatorAddBlockToOrphans(self,block))
			return CB_BLOCK_STATUS_ORPHAN;
		return CB_BLOCK_STATUS_ERROR;
	}
	// Not an orphan. See if this is an extention or new branch.
	uint8_t branch;
	if (prevBlockIndex == self->branches[prevBranch].numRefs - 1) {
		// Extension
		// Do basic validation with a copy of the transaction hashes.
		CBBlockStatus res = CBFullValidatorBasicBlockValidationCopy(self, block, txHashes, networkTime);
		if (res != CB_BLOCK_STATUS_CONTINUE){
			free(txHashes);
			return res;
		}
		branch = prevBranch;
	}else{
		// New branch
		if (self->numBranches == CB_MAX_BRANCH_CACHE) {
			// ??? SINCE BRANCHES ARE TRIMMED OR DELETED THERE ARE SECURITY IMPLICATIONS: A peer could prevent other branches having a chance by creating many branches when the node is not up-to-date. To protect against this potential attack peers should only be allowed to provide one branch at a time and before accepting new branches from peers branches should be up-to-date with all other peers.
			// Trim branch from longest ago.
			uint8_t earliestBranch = 0;
			uint32_t earliestIndex = self->branches[0].parentBlockIndex + self->branches[0].numRefs;
			for (uint8_t x = 1; x < self->numBranches; x++) {
				if (self->branches[x].parentBlockIndex + self->branches[x].numRefs < earliestIndex) {
					earliestIndex = self->branches[x].parentBlockIndex + self->branches[x].numRefs;
					earliestBranch = x;
				}
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
				// Merge the branch with the branch with the highest dependency.
				// Use the earliest branch and add the highest dependency onto it.
				// First remove the redundant blocks.
				// Then add all the blocks from the highest dependency
				// Remove the highest dependency
			}else{
				// Delete the branch.
				// Modify validator file
				if (NOT CBSafeOutputAlloc(self->output, 1)){
					free(txHashes);
					return CB_BLOCK_STATUS_ERROR;
				}
				char validatorFilePath[strlen(self->dataDir) + strlen(CB_VALIDATION_DATA_FILE) + 1];
				memcpy(validatorFilePath, self->dataDir, strlen(self->dataDir));
				strcpy(validatorFilePath + strlen(self->dataDir), CB_VALIDATION_DATA_FILE);
				if (NOT CBSafeOutputAddFile(self->output, validatorFilePath, 1, 0)) {
					free(txHashes);
					return CB_BLOCK_STATUS_ERROR;
				}
				CBSafeOutputAddOverwriteOperation(self->output, 2, (uint8_t []){self->mainBranch - (self->mainBranch > earliestBranch),self->numBranches - 1}, 2);
				// Delete block files
				uint16_t numBlockFiles = 0;
				for (uint8_t x = earliestBranch; x < self->numBranches; x++) {
					numBlockFiles += self->branches[x].numBlockFiles * (1 + (x > earliestBranch)); // Extra filename for branches above for renaming.
				}
				char * blockFiles = malloc((strlen(self->dataDir) + 22) * numBlockFiles);
				if (NOT blockFiles) {
					free(txHashes);
					return CB_BLOCK_STATUS_ERROR;
				}
				char * blockFile = blockFiles;
				for (uint16_t x = 0; x < self->branches[earliestBranch].numBlockFiles; x++) {
					sprintf(blockFile, "%sblocks%u-%u.dat",self->dataDir, earliestBranch, x);
					CBSafeOutputAddDeleteFileOperation(self->output, blockFile);
					blockFile += strlen(self->dataDir) + 22;
				}
				// Rename validator and block files
				for (uint8_t x = earliestBranch + 1; x < self->numBranches; x++) {
					for (uint16_t y = 0; y < self->branches[x].numBlockFiles; y++) {
						sprintf(blockFile, "%sblocks%u-%u.dat", self->dataDir, x, y);
						char * blockFile2 = blockFile + strlen(self->dataDir) + 22;
						sprintf(blockFile2, "%sblocks%u-%u.dat",self->dataDir, x - 1, y);
						CBSafeOutputAddRenameFileOperation(self->output, blockFile, blockFile2);
						blockFile = blockFile2 + strlen(self->dataDir) + 22;
					}
				}
				char * branchFiles = malloc((strlen(self->dataDir) + 14) * ((self->numBranches - earliestBranch)*2 - 1));
				if (NOT branchFiles) {
					free(txHashes);
					return CB_BLOCK_STATUS_ERROR;
				}
				char * branchFile = branchFiles;
				// Remove validator file
				sprintf(branchFile, "%sbranch%u.dat",self->dataDir, earliestBranch);
				CBSafeOutputAddDeleteFileOperation(self->output, branchFile);
				// Rename validation files
				for (uint8_t x = earliestBranch + 1; x < self->numBranches; x++) {
					sprintf(branchFile, "%sbranch%u.dat", self->dataDir, x);
					char * branchFile2 = branchFile + strlen(self->dataDir) + 14;
					sprintf(branchFile2, "%sbranch%u.dat",self->dataDir, x - 1);
					CBSafeOutputAddRenameFileOperation(self->output, branchFile, branchFile2);
					blockFile = branchFile2 + strlen(self->dataDir) + 14;
				}
				// Commit IO
				CBSafeOutputResult res = CBSafeOutputCommit(self->output, self->backup);
				if (res == CB_SAFE_OUTPUT_FAIL_BAD_STATE) {
					free(txHashes);
					return CB_BLOCK_STATUS_ERROR_BAD_DATA;
				}else if (res == CB_SAFE_OUTPUT_FAIL_PREVIOUS){
					free(txHashes);
					return CB_BLOCK_STATUS_ERROR;
				}
				free(blockFiles);
				// Delete branch data
				free(self->branches[earliestBranch].references);
				free(self->branches[earliestBranch].referenceTable);
				free(self->branches[earliestBranch].unspentOutputs);
				free(self->branches[earliestBranch].work.data);
				// Move other branches down.
				if (earliestBranch != CB_MAX_BRANCH_CACHE - 1)
					memmove(self->branches + earliestBranch, self->branches + earliestBranch + 1, CB_MAX_BRANCH_CACHE - earliestBranch - 1);
				// Modify validator information
				self->numBranches--;
				if (self->mainBranch > earliestBranch)
					self->mainBranch--;
			}
		}
		// Do basic validation with a copy of the transaction hashes.
		CBBlockStatus res = CBFullValidatorBasicBlockValidationCopy(self, block, txHashes, networkTime);
		if (res != CB_BLOCK_STATUS_CONTINUE){
			free(txHashes);
			return res;
		}
		branch = self->numBranches;
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
		for (uint32_t y = prevBlockIndex + 1; y < self->branches[prevBranch].numRefs; y++) {
			CBBigInt tempWork;
			if (NOT CBCalculateBlockWork(&tempWork,self->branches[prevBranch].references[y].target)){
				free(txHashes);
				return CB_BLOCK_STATUS_ERROR;
			}
			CBBigIntEqualsSubtractionByBigInt(&self->branches[branch].work, &tempWork);
			free(tempWork.data); 
		}
		self->branches[branch].lastValidation = CB_NO_VALIDATION;
		self->branches[branch].startHeight = self->branches[prevBranch].startHeight + prevBlockIndex + 1;
		self->numBranches++;
	}
	// Got branch ready for block. Now process into the branch.
	CBBlockStatus res = CBFullValidatorProcessIntoBranch(self, block, networkTime, branch, prevBranch, prevBlockIndex, txHashes);
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
			if (CBFullValidatorProcessIntoBranch(self, self->orphans[x], networkTime, branch, branch, self->branches[branch].numRefs - 1, txHashes) == CB_BLOCK_STATUS_ERROR)
				break;
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
	return res;
}
CBBlockStatus CBFullValidatorProcessIntoBranch(CBFullValidator * self, CBBlock * block, uint64_t networkTime, uint8_t branch, uint8_t prevBranch, uint32_t prevBlockIndex, uint8_t * txHashes){
	// Check timestamp
	if (block->time <= CBFullValidatorGetMedianTime(self, prevBranch, prevBlockIndex))
		return CB_BLOCK_STATUS_BAD;
	uint32_t target;
	bool change = NOT ((self->branches[prevBranch].startHeight + prevBlockIndex + 1) % 2016);
	if (change)
		// Difficulty change for this branch
		target = CBCalculateTarget(self->branches[prevBranch].references[prevBlockIndex].target, block->time - self->branches[branch].lastRetargetTime);
	else
		target = self->branches[prevBranch].references[prevBlockIndex].target;
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
			CBFullValidatorAddBlockToBranchResult res = CBFullValidatorAddBlockToBranch(self, branch, block, work);
			switch (res) {
				case CB_ADD_BLOCK_FAIL_BAD_DATA:
					// During output the data has been left in a bad state and recovery is needed.
					return CB_BLOCK_STATUS_ERROR_BAD_DATA;
				case CB_ADD_BLOCK_FAIL_PREVIOUS:
					// The output failed and the block has not been added.
					return CB_BLOCK_STATUS_ERROR;
				case CB_ADD_BLOCK_FAIL_BAD_MEMORY:
					// The output failed and the block has not been added.
					return CB_BLOCK_STATUS_ERROR;
				case CB_ADD_BLOCK_OK:
					// The block has been added to the branch OK.
					return CB_BLOCK_STATUS_SIDE;
			}
		}
		// Potential block-chain reorganisation. Validate the side branch. THIS NEEDS TO START AT THE FIRST BRANCH BACK WHERE VALIDATION IS NOT COMPLETE AND GO UP THROUGH THE BLOCKS VALIDATING THEM. Includes Prior Branches!
		uint8_t tempBranch = prevBranch;
		uint32_t lastBlocks[3]; // Used to store the last blocks in each branch going to the branch we are validating for.
		uint8_t lastBlocksIndex = 3; // The starting point of lastBlocks. If 3 then we are only validating the branch we added to.
		uint8_t branches[3]; // Branches to go to after the last blocks.
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
				tempBranch = self->branches[tempBranch].parentBranch;
				lastBlocks[lastBlocksIndex] = self->branches[tempBranch].parentBlockIndex;
			}else{
				// Not fully validated. Start at last validation plus one.
				tempBlockIndex = self->branches[tempBranch].lastValidation + 1;
				break;
			}
		}
		// Now validate all blocks going up.
		uint8_t * txHashes2 = NULL;
		uint32_t txHashes2AllocSize = 0;
		while (tempBlockIndex != self->branches[branch].numRefs || tempBranch != branch) {
			// Get block
			CBBlock * tempBlock = CBFullValidatorLoadBlock(self, self->branches[tempBranch].references[tempBlockIndex],tempBranch);
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
			if (res == CB_BLOCK_VALIDATION_BAD){
				free(txHashes2);
				return CB_BLOCK_STATUS_BAD;
			}
			if (res == CB_BLOCK_VALIDATION_ERR){
				free(txHashes2);
				return CB_BLOCK_STATUS_ERROR;
			}
			// This block passed validation. Move onto next block.
			if (tempBlockIndex == lastBlocks[lastBlocksIndex]) {
				// Came to the last block in the branch
				tempBranch = branches[lastBlocksIndex];
				tempBlockIndex = 0;
				lastBlocksIndex++;
			}else
				tempBlockIndex++;
			CBReleaseObject(tempBlock);
		}
		free(txHashes2);
		// Now we validate the block for the new main chain.
	}
	// We are just validating a new block on the main chain
	CBBlockValidationResult res = CBFullValidatorCompleteBlockValidation(self, branch, block, txHashes, self->branches[branch].startHeight + self->branches[branch].numRefs);
	switch (res) {
		case CB_BLOCK_VALIDATION_BAD:
			return CB_BLOCK_STATUS_BAD;
		case CB_BLOCK_VALIDATION_ERR:
			return CB_BLOCK_STATUS_ERROR;
		case CB_BLOCK_VALIDATION_OK:
			// Update branch and unspent outputs.
			self->branches[branch].lastValidation = self->branches[branch].numRefs;
			CBFullValidatorAddBlockToBranchResult res = CBFullValidatorAddBlockToBranch(self, branch, block, work);
			switch (res) {
				case CB_ADD_BLOCK_FAIL_BAD_DATA:
					// During output the data has been left in a bad state and recovery is needed.
					return CB_BLOCK_STATUS_ERROR_BAD_DATA;
				case CB_ADD_BLOCK_FAIL_PREVIOUS:
					// The output failed and the block has not been added.
					return CB_BLOCK_STATUS_ERROR;
				case CB_ADD_BLOCK_FAIL_BAD_MEMORY:
					// The output failed and the block has not been added.
					return CB_BLOCK_STATUS_ERROR;
				case CB_ADD_BLOCK_OK:
					// The block has been added to the branch OK.
					self->mainBranch = branch;
					return CB_BLOCK_STATUS_MAIN;
			}
	}
}*/
