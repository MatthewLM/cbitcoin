//
//  CBBlock.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 01/05/2012.
//  Copyright (c) 2012 Matthew Mitchell
//  
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

//  SEE HEADER FILE FOR DOCUMENTATION

#include "CBBlock.h"

//  Constructor2

CBBlock * CBNewBlock() {
	
	CBBlock * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeBlock;
	CBInitBlock(self);
	
	return self;
	
}

CBBlock * CBNewBlockFromData(CBByteArray * data) {
	
	CBBlock * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeBlock;
	CBInitBlockFromData(self, data);
	
	return self;
	
}

CBBlock * CBNewBlockGenesis() {
	
	CBBlock * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeBlock;
	CBInitBlockGenesis(self);
	
	return self;
	
}

CBBlock * CBNewBlockGenesisHeader() {
	
	CBBlock * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeBlock;
	CBInitBlockGenesisHeader(self);
	
	return self;
	
}

//  Initialiser

void CBInitBlock(CBBlock * self) {
	
	self->prevBlockHash = NULL;
	self->merkleRoot = NULL;
	self->transactions = NULL;
	self->transactionNum = 0;
	self->hashSet = false;
	memset(self->hash, 0, 32);
	
	CBInitMessageByObject(CBGetMessage(self));
	
}

void CBInitBlockFromData(CBBlock * self, CBByteArray * data) {
	
	self->prevBlockHash = NULL;
	self->merkleRoot = NULL;
	self->transactions = NULL;
	self->transactionNum = 0;
	self->hashSet = false;
	memset(self->hash, 0, 32);
	
	CBInitMessageByData(CBGetMessage(self), data);
	
}

void CBInitBlockGenesis(CBBlock * self) {
	
	CBByteArray * data = CBNewByteArrayWithDataCopy((unsigned char [285]){0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3B, 0xA3, 0xED, 0xFD, 0x7A, 0x7B, 0x12, 0xB2, 0x7A, 0xC7, 0x2C, 0x3E, 0x67, 0x76, 0x8F, 0x61, 0x7F, 0xC8, 0x1B, 0xC3, 0x88, 0x8A, 0x51, 0x32, 0x3A, 0x9F, 0xB8, 0xAA, 0x4B, 0x1E, 0x5E, 0x4A, 0x29, 0xAB, 0x5F, 0x49, 0xFF, 0xFF, 0x00, 0x1D, 0x1D, 0xAC, 0x2B, 0x7C, 0x01, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x4D, 0x04, 0xFF, 0xFF, 0x00, 0x1D, 0x01, 0x04, 0x45, 0x54, 0x68, 0x65, 0x20, 0x54, 0x69, 0x6D, 0x65, 0x73, 0x20, 0x30, 0x33, 0x2F, 0x4A, 0x61, 0x6E, 0x2F, 0x32, 0x30, 0x30, 0x39, 0x20, 0x43, 0x68, 0x61, 0x6E, 0x63, 0x65, 0x6C, 0x6C, 0x6F, 0x72, 0x20, 0x6F, 0x6E, 0x20, 0x62, 0x72, 0x69, 0x6E, 0x6B, 0x20, 0x6F, 0x66, 0x20, 0x73, 0x65, 0x63, 0x6F, 0x6E, 0x64, 0x20, 0x62, 0x61, 0x69, 0x6C, 0x6F, 0x75, 0x74, 0x20, 0x66, 0x6F, 0x72, 0x20, 0x62, 0x61, 0x6E, 0x6B, 0x73, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0xF2, 0x05, 0x2A, 0x01, 0x00, 0x00, 0x00, 0x43, 0x41, 0x04, 0x67, 0x8A, 0xFD, 0xB0, 0xFE, 0x55, 0x48, 0x27, 0x19, 0x67, 0xF1, 0xA6, 0x71, 0x30, 0xB7, 0x10, 0x5C, 0xD6, 0xA8, 0x28, 0xE0, 0x39, 0x09, 0xA6, 0x79, 0x62, 0xE0, 0xEA, 0x1F, 0x61, 0xDE, 0xB6, 0x49, 0xF6, 0xBC, 0x3F, 0x4C, 0xEF, 0x38, 0xC4, 0xF3, 0x55, 0x04, 0xE5, 0x1E, 0xC1, 0x12, 0xDE, 0x5C, 0x38, 0x4D, 0xF7, 0xBA, 0x0B, 0x8D, 0x57, 0x8A, 0x4C, 0x70, 0x2B, 0x6B, 0xF1, 0x1D, 0x5F, 0xAC, 0x00, 0x00, 0x00, 0x00}, 285);
	
	unsigned char genesisHash[32] = {0x6F, 0xE2, 0x8C, 0x0A, 0xB6, 0xF1, 0xB3, 0x72, 0xC1, 0xA6, 0xA2, 0x46, 0xAE, 0x63, 0xF7, 0x4F, 0x93, 0x1E, 0x83, 0x65, 0xE1, 0x5A, 0x08, 0x9C, 0x68, 0xD6, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00};
	
	memcpy(self->hash, genesisHash, 32);
	self->hashSet = true;
	
	CBInitMessageByData(CBGetMessage(self), data);
	CBReleaseObject(data);
	CBBlockDeserialise(self, true);
	
}

void CBInitBlockGenesisHeader(CBBlock * self) {
	
	CBByteArray * data = CBNewByteArrayWithDataCopy((unsigned char [82]){0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3B, 0xA3, 0xED, 0xFD, 0x7A, 0x7B, 0x12, 0xB2, 0x7A, 0xC7, 0x2C, 0x3E, 0x67, 0x76, 0x8F, 0x61, 0x7F, 0xC8, 0x1B, 0xC3, 0x88, 0x8A, 0x51, 0x32, 0x3A, 0x9F, 0xB8, 0xAA, 0x4B, 0x1E, 0x5E, 0x4A, 0x29, 0xAB, 0x5F, 0x49, 0xFF, 0xFF, 0x00, 0x1D, 0x1D, 0xAC, 0x2B, 0x7C, 0x01, 0x00}, 82);
	
	unsigned char genesisHash[32] = {0x6F, 0xE2, 0x8C, 0x0A, 0xB6, 0xF1, 0xB3, 0x72, 0xC1, 0xA6, 0xA2, 0x46, 0xAE, 0x63, 0xF7, 0x4F, 0x93, 0x1E, 0x83, 0x65, 0xE1, 0x5A, 0x08, 0x9C, 0x68, 0xD6, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00};
	
	memcpy(self->hash, genesisHash, 32);
	self->hashSet = true;
	
	CBInitMessageByData(CBGetMessage(self), data);
	CBReleaseObject(data);
	CBBlockDeserialise(self, false);
	
}

//  Destructor

void CBDestroyBlock(void * vself) {
	
	CBBlock * self = vself;
	
	if(self->prevBlockHash) CBReleaseObject(self->prevBlockHash);
	if(self->merkleRoot) CBReleaseObject(self->merkleRoot);
	
	// Check transaction pointer for NULL since the transaction number can be set without having any transactions.
	if (self->transactions) {
		for (int x = 0; x < self->transactionNum; x++)
			CBReleaseObject(self->transactions[x]);
		free(self->transactions);
	}
	
	CBDestroyMessage(CBGetObject(self));
	
}
void CBFreeBlock(void * self) {
	
	CBDestroyBlock(self);
	free(self);
	
}

//  Functions

void CBBlockCalculateAndSetMerkleRoot(CBBlock * self) {
	
	unsigned char * newRootData = CBBlockCalculateMerkleRoot(self);
	CBByteArray * newRoot = CBNewByteArrayWithData(newRootData, 32);
	
	// Release old merkle root, if it has been previously set.
	if (self->merkleRoot) CBReleaseObject(self->merkleRoot);
	self->merkleRoot = newRoot;
	
}

void CBBlockCalculateHash(CBBlock * self, unsigned char * hash) {
	
	unsigned char * headerData = CBByteArrayGetData(CBGetMessage(self)->bytes);
	unsigned char hash2[32];
	
	CBSha256(headerData, 80, hash2);
	CBSha256(hash2, 32, hash);
	
}

int CBBlockCalculateLength(CBBlock * self, bool transactions) {
	
	int len = 80 + CBVarIntSizeOf(self->transactionNum);
	
	if (transactions) {
		for (int x = 0; x < self->transactionNum; x++)
			len += CBTransactionCalculateLength(self->transactions[x]);
		return len;
	}
	
	return len + 1; // Plus the stupid pointless null byte.
	
}

unsigned char * CBBlockCalculateMerkleRoot(CBBlock * self) {
	
	unsigned char * txHashes = malloc(32 * self->transactionNum);
	
	// Ensure serialisation of transactions and then add their hashes for the calculation
	for (int x = 0; x < self->transactionNum; x++)
		memcpy(txHashes + 32*x, CBTransactionGetHash(self->transactions[x]), 32);
	
	CBCalculateMerkleRoot(txHashes, self->transactionNum);
	
	return txHashes;
	
}

int CBBlockDeserialise(CBBlock * self, bool transactions) {
	
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (! bytes) {
		CBLogError("Attempting to deserialise a CBBlock with no bytes.");
		return CB_DESERIALISE_ERROR;
	}
	if (bytes->length < 82) {
		CBLogError("Attempting to deserialise a CBBlock with less than 82 bytes (%u bytes). Minimum for header (With null byte).", bytes->length);
		return CB_DESERIALISE_ERROR;
	}
	
	self->version = CBByteArrayReadInt32(bytes, 0);
	self->prevBlockHash = CBByteArraySubReference(bytes, 4, 32);
	self->merkleRoot = CBByteArraySubReference(bytes, 36, 32);
	self->time = CBByteArrayReadInt32(bytes, 68);
	self->target = CBByteArrayReadInt32(bytes, 72);
	self->nonce = CBByteArrayReadInt32(bytes, 76);
	
	// If first VarInt is zero, then stop here for headers, otherwise look for 8 more bytes and continue
	CBVarInt txNumVI = CBByteArrayReadVarInt(bytes, 80);
	
	if (transactions && txNumVI.val) {
		
		// More to come
		
		if (bytes->length < 89) {
			CBLogError("Attempting to deserialise a CBBlock with a non-zero varint with less than 89 bytes.");
			return CB_DESERIALISE_ERROR;
		}
		
		if (txNumVI.val * 60 > bytes->length - 81) {
			CBLogError("Attempting to deserialise a CBBlock with too many transactions for the byte data length.");
			return CB_DESERIALISE_ERROR;
		}
		
		self->transactionNum = (int)txNumVI.val;
		self->transactions = malloc(sizeof(*self->transactions) * self->transactionNum);
		
		int cursor = 80 + txNumVI.size;
		
		for (int x = 0; x < self->transactionNum; x++) {
			
			CBByteArray * data = CBByteArraySubReference(bytes, cursor, bytes->length-cursor);
			CBTransaction * transaction = CBNewTransactionFromData(data);
			int len = CBTransactionDeserialise(transaction);
			
			if (len == CB_DESERIALISE_ERROR){
				CBLogError("CBBlock cannot be deserialised because of an error with the transaction number %" PRIu16 ".", x);
				CBReleaseObject(data);
				return CB_DESERIALISE_ERROR;
			}
			
			// Read just the CBByteArray length
			data->length = len;
			CBReleaseObject(data);
			self->transactions[x] = transaction;
			cursor += len;
			
		}
		
		return cursor;
	}
	// Just header
	
	if (bytes->length < 80 + txNumVI.size + 1) {
		CBLogError("Attempting to deserialise a CBBlock header with not enough space to cover the var int.");
		return CB_DESERIALISE_ERROR;
	}
	
	// This value is undefined in the protocol. Should best be zero when getting the headers since there is not supposed to be any transactions. Would have probably been better if the var int was dropped completely for headers only.
	self->transactionNum = (int)CBByteArrayReadVarInt(bytes, 80).val;

	self->transactions = NULL;
	
	// Ensure null byte is null. This null byte is a bit of a nuissance but it exists in the protocol when there are no transactions.
	if (CBByteArrayGetByte(bytes, 80 + txNumVI.size) != 0) {
		CBLogError("Attempting to deserialise a CBBlock header with a final byte which is not null.");
		return CB_DESERIALISE_ERROR;
	}
	
	return 80 + txNumVI.size + 1; // 80 header bytes, the var int and the null byte
	
}

unsigned char * CBBlockGetHash(CBBlock * self) {
	
	if (! self->hashSet){
		CBBlockCalculateHash(self, self->hash);
		self->hashSet = true;
	}
	
	return self->hash;
	
}

void CBBlockHashToString(CBBlock * self, char output[CB_BLOCK_HASH_STR_SIZE]) {
	
	unsigned char * hash = CBBlockGetHash(self);
	
	for (int x = 0; x < CB_BLOCK_HASH_STR_BYTES; x++)
		sprintf(output + x*2, "%02x", hash[CB_BLOCK_HASH_STR_BYTES-x-1]);
	
	output[CB_BLOCK_HASH_STR_SIZE-1] = '\0';
	
}

void CBBlockPrepareBytes(CBBlock * self, bool transactions) {
	
	CBMessagePrepareBytes(CBGetMessage(self), CBBlockCalculateLength(self, transactions));
	
}

int CBBlockSerialise(CBBlock * self, bool transactions, bool force) {
	
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (! bytes) {
		CBLogError("Attempting to serialise a CBBlock with no bytes.");
		return 0;
	}
	
	CBVarInt transactionNum = CBVarIntFromUInt64(self->transactionNum);
	int cursor = 80 + transactionNum.size;
	if (bytes->length < cursor + 1) {
		CBLogError("Attempting to serialise a CBBlock with less bytes than required for the header, transaction number var int and at least a null byte. %i < %i", bytes->length, cursor + 1);
		return 0;
	}
	
	// Do header
	CBByteArraySetInt32(bytes, 0, self->version);
	CBByteArrayCopyByteArray(bytes, 4, self->prevBlockHash);
	CBByteArrayChangeReference(self->prevBlockHash, bytes, 4);
	CBByteArrayCopyByteArray(bytes, 36, self->merkleRoot);
	CBByteArrayChangeReference(self->merkleRoot, bytes, 36);
	CBByteArraySetInt32(bytes, 68, self->time);
	CBByteArraySetInt32(bytes, 72, self->target);
	CBByteArraySetInt32(bytes, 76, self->nonce);
	
	// Do Transactions
	CBByteArraySetVarInt(bytes, 80, transactionNum);
	
	if (transactions) {
		
		for (int x = 0; x < self->transactionNum; x++) {
			
			if (! CBGetMessage(self->transactions[x])->serialised // Serailise if not serialised yet.
				// Serialise if force is true.
				|| force
				// If the data shares the same data as this block, re-serialise the transaction, in case it got overwritten.
				|| CBGetMessage(self->transactions[x])->bytes->sharedData == bytes->sharedData) {
				
				if (CBGetMessage(self->transactions[x])->serialised)
					// Release old byte array
					CBReleaseObject(CBGetMessage(self->transactions[x])->bytes);
				
				CBGetMessage(self->transactions[x])->bytes = CBByteArraySubReference(bytes, cursor, bytes->length-cursor);
				
				// Serialise transaction, also forcing reserialisation of inputs and outputs.
				if (! CBTransactionSerialise(self->transactions[x], true)) {
					CBLogError("CBBlock cannot be serialised because of an error with the transaction number %" PRIu32 ".", x);
					return 0;
				}
				
				cursor += CBGetMessage(self->transactions[x])->bytes->length;
				
			}else{
				
				// Move serialsed data to one location
				if (bytes->length < cursor + CBGetMessage(self->transactions[x])->bytes->length) {
					CBLogError("CBBlock cannot be serialised because there was not enough bytes for the transaction number %" PRIu32 " (%" PRIu32 " < %" PRIu32 ").", x, bytes->length, cursor + CBGetMessage(self->transactions[x])->bytes->length);
					return 0;
				}
				
				CBByteArrayCopyByteArray(bytes, cursor, CBGetMessage(self->transactions[x])->bytes);
				CBByteArrayChangeReference(CBGetMessage(self->transactions[x])->bytes, bytes, cursor);
				
				// Change input and output references.
				
				cursor += 4;
				cursor += CBByteArrayReadVarIntSize(bytes, cursor);
				
				for (int y = 0; y < self->transactions[x]->inputNum; y++) {
					CBByteArray * barr = CBGetMessage(self->transactions[x]->inputs[y])->bytes;
					CBByteArrayChangeReference(barr, bytes, cursor);
					cursor += barr->length;
				}
				
				cursor += CBByteArrayReadVarIntSize(bytes, cursor);
				
				for (int y = 0; y < self->transactions[x]->outputNum; y++) {
					CBByteArray * barr = CBGetMessage(self->transactions[x]->outputs[y])->bytes;
					CBByteArrayChangeReference(barr, bytes, cursor);
					cursor += barr->length;
				}
				
				cursor += 4;
				
			}
		}
		
	}else{
		// Add null byte since there are to be no transactions (header only).
		CBByteArraySetByte(bytes, cursor, 0);
		cursor++;
	}
	
	// Reset hash
	self->hashSet = false;
	
	// Ensure data length is correct.
	bytes->length = cursor;
	
	// Is now serialised.
	CBGetMessage(self)->serialised = true;
	
	return cursor;
	
}
