//
//  CBBlock.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 01/05/2012.
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

#include "CBBlock.h"

//  Constructor2

CBBlock * CBNewBlock(void (*onErrorReceived)(CBError error,char *,...)){
	CBBlock * self = malloc(sizeof(*self));
	if (NOT self) {
		onErrorReceived(CB_ERROR_OUT_OF_MEMORY,"Cannot allocate %i bytes of memory in CBNewBlock\n",sizeof(*self));
		return NULL;
	}
	CBGetObject(self)->free = CBFreeBlock;
	if(CBInitBlock(self,onErrorReceived))
		return self;
	free(self);
	return NULL;
}
CBBlock * CBNewBlockFromData(CBByteArray * data,void (*onErrorReceived)(CBError error,char *,...)){
	CBBlock * self = malloc(sizeof(*self));
	if (NOT self) {
		onErrorReceived(CB_ERROR_OUT_OF_MEMORY,"Cannot allocate %i bytes of memory in CBNewBlockFromData\n",sizeof(*self));
		return NULL;
	}
	CBGetObject(self)->free = CBFreeBlock;
	if(CBInitBlockFromData(self,data,onErrorReceived))
		return self;
	free(self);
	return NULL;
}
CBBlock * CBNewBlockGenesis(void (*onErrorReceived)(CBError error,char *,...)){
	CBBlock * self = malloc(sizeof(*self));
	if (NOT self) {
		onErrorReceived(CB_ERROR_OUT_OF_MEMORY,"Cannot allocate %i bytes of memory in CBNewBlockGenesis\n",sizeof(*self));
		return NULL;
	}
	CBGetObject(self)->free = CBFreeBlock;
	if(CBInitBlockGenesis(self,onErrorReceived))
		return self;
	free(self);
	return NULL;
}

//  Object Getter

CBBlock * CBGetBlock(void * self){
	return self;
}

//  Initialiser

bool CBInitBlock(CBBlock * self,void (*onErrorReceived)(CBError error,char *,...)){
	self->prevBlockHash = NULL;
	self->merkleRoot = NULL;
	self->transactions = NULL;
	self->transactionNum = 0;
	self->hash = NULL;
	if (NOT CBInitMessageByObject(CBGetMessage(self), onErrorReceived))
		return false;
	return true;
}
bool CBInitBlockFromData(CBBlock * self,CBByteArray * data,void (*onErrorReceived)(CBError error,char *,...)){
	self->prevBlockHash = NULL;
	self->merkleRoot = NULL;
	self->transactions = NULL;
	self->transactionNum = 0;
	self->hash = NULL;
	if (NOT CBInitMessageByData(CBGetMessage(self), data, onErrorReceived))
		return false;
	return true;
}
bool CBInitBlockGenesis(CBBlock * self,void (*onErrorReceived)(CBError error,char *,...)){
	CBByteArray * data = CBNewByteArrayWithDataCopy((uint8_t [285]){0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3B,0xA3,0xED,0xFD,0x7A,0x7B,0x12,0xB2,0x7A,0xC7,0x2C,0x3E,0x67,0x76,0x8F,0x61,0x7F,0xC8,0x1B,0xC3,0x88,0x8A,0x51,0x32,0x3A,0x9F,0xB8,0xAA,0x4B,0x1E,0x5E,0x4A,0x29,0xAB,0x5F,0x49,0xFF,0xFF,0x00,0x1D,0x1D,0xAC,0x2B,0x7C,0x01,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x4D,0x04,0xFF,0xFF,0x00,0x1D,0x01,0x04,0x45,0x54,0x68,0x65,0x20,0x54,0x69,0x6D,0x65,0x73,0x20,0x30,0x33,0x2F,0x4A,0x61,0x6E,0x2F,0x32,0x30,0x30,0x39,0x20,0x43,0x68,0x61,0x6E,0x63,0x65,0x6C,0x6C,0x6F,0x72,0x20,0x6F,0x6E,0x20,0x62,0x72,0x69,0x6E,0x6B,0x20,0x6F,0x66,0x20,0x73,0x65,0x63,0x6F,0x6E,0x64,0x20,0x62,0x61,0x69,0x6C,0x6F,0x75,0x74,0x20,0x66,0x6F,0x72,0x20,0x62,0x61,0x6E,0x6B,0x73,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0xF2,0x05,0x2A,0x01,0x00,0x00,0x00,0x43,0x41,0x04,0x67,0x8A,0xFD,0xB0,0xFE,0x55,0x48,0x27,0x19,0x67,0xF1,0xA6,0x71,0x30,0xB7,0x10,0x5C,0xD6,0xA8,0x28,0xE0,0x39,0x09,0xA6,0x79,0x62,0xE0,0xEA,0x1F,0x61,0xDE,0xB6,0x49,0xF6,0xBC,0x3F,0x4C,0xEF,0x38,0xC4,0xF3,0x55,0x04,0xE5,0x1E,0xC1,0x12,0xDE,0x5C,0x38,0x4D,0xF7,0xBA,0x0B,0x8D,0x57,0x8A,0x4C,0x70,0x2B,0x6B,0xF1,0x1D,0x5F,0xAC,0x00,0x00,0x00,0x00}, 285, onErrorReceived);
	if (NOT data)
		return false;
	self->hash = CBNewByteArrayWithDataCopy((uint8_t []){0x6F,0xE2,0x8C,0x0A,0xB6,0xF1,0xB3,0x72,0xC1,0xA6,0xA2,0x46,0xAE,0x63,0xF7,0x4F,0x93,0x1E,0x83,0x65,0xE1,0x5A,0x08,0x9C,0x68,0xD6,0x19,0x00,0x00,0x00,0x00,0x00}, 32, onErrorReceived);
	if (NOT self->hash) {
		CBReleaseObject(data);
		return false;
	}
	if (NOT CBInitMessageByData(CBGetMessage(self), data, onErrorReceived)){
		CBReleaseObject(data);
		CBReleaseObject(self->hash);
		return false;
	}
	CBReleaseObject(data);
	CBBlockDeserialise(self,true);
	return true;
}

//  Destructor

void CBFreeBlock(void * vself){
	CBBlock * self = vself;
	if(self->prevBlockHash) CBReleaseObject(self->prevBlockHash);
	if(self->merkleRoot) CBReleaseObject(self->merkleRoot);
	if (self->transactions) { // Check for the loop since the transaction number can be set without having any transactions.
		for (uint32_t x = 0; x < self->transactionNum; x++)
			if(self->transactions[x]) CBReleaseObject(self->transactions[x]);
		free(self->transactions);
	}
	if(self->hash) CBReleaseObject(self->hash);
	CBFreeMessage(CBGetObject(self));
}

//  Functions

CBByteArray * CBBlockCalculateHash(CBBlock * self){
	uint8_t * headerData = CBByteArrayGetData(CBGetMessage(self)->bytes);
	uint8_t hash1[32];
	uint8_t * hash2 = malloc(32);
	if (NOT hash2) {
		CBGetMessage(self)->onErrorReceived(CB_ERROR_OUT_OF_MEMORY,"Cannot allocate 32 bytes of memory in CBBlockCalculateHash\n");
		return NULL;
	}
	CBSha256(headerData, 80, hash1);
	CBSha256(hash1, 32, hash2);
	return CBNewByteArrayWithData(hash2, 32, CBGetMessage(self)->onErrorReceived);
}
uint32_t CBBlockCalculateLength(CBBlock * self, bool transactions){
	uint32_t len = 80 + CBVarIntSizeOf(self->transactionNum);
	if (transactions) {
		for (uint32_t x = 0; x < self->transactionNum; x++) {
			uint32_t tLen = CBTransactionCalculateLength(self->transactions[x]);
			if (NOT tLen)
				return 0;
			len += tLen;
		}
		return len;
	}else return len + 1; // Plus the stupid pointless null byte.
}
uint32_t CBBlockDeserialise(CBBlock * self,bool transactions){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (NOT bytes) {
		CBGetMessage(self)->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_NULL_BYTES,"Attempting to deserialise a CBBlock with no bytes.");
		return 0;
	}
	if (bytes->length < 82) {
		CBGetMessage(self)->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBBlock with less than 82 bytes. Minimum for header (With null byte).");
		return 0;
	}
	self->version = CBByteArrayReadInt32(bytes, 0);
	self->prevBlockHash = CBByteArraySubReference(bytes, 4, 32);
	if (NOT self->prevBlockHash){
		CBGetMessage(self)->onErrorReceived(CB_ERROR_INIT_FAIL,"Cannot create the previous block hash CBByteArray in CBBlockDeserialise.");
		return 0;
	}
	self->merkleRoot = CBByteArraySubReference(bytes, 36, 32);
	if (NOT self->merkleRoot){
		CBGetMessage(self)->onErrorReceived(CB_ERROR_INIT_FAIL,"Cannot create the merkle root CBByteArray in CBBlockDeserialise.");
		return 0;
	}
	self->time = CBByteArrayReadInt32(bytes, 68);
	self->target = CBByteArrayReadInt32(bytes, 72);
	self->nounce = CBByteArrayReadInt32(bytes, 76);
	// If first VarInt byte is zero, then stop here for headers, otherwise look for 8 more bytes and continue
	uint8_t firstByte = CBByteArrayGetByte(bytes, 80);
	if (transactions && firstByte) {
		// More to come
		if (bytes->length < 89) {
			CBGetMessage(self)->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBBlock with a non-zero varint with less than 89 bytes.");
			return 0;
		}
		CBVarInt transactionNumVarInt = CBVarIntDecode(bytes, 80);
		if (transactionNumVarInt.val*60 > bytes->length - 81) {
			CBGetMessage(self)->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBBlock with too many transactions for the byte data length.");
			return 0;
		}
		self->transactionNum = (uint32_t)transactionNumVarInt.val;
		self->transactions = malloc(sizeof(*self->transactions) * self->transactionNum);
		if (NOT self->transactionNum) {
			CBGetMessage(self)->onErrorReceived(CB_ERROR_OUT_OF_MEMORY,"Cannot allocate %i bytes of memory in CBBlockDeserialise\n",sizeof(*self->transactions) * self->transactionNum);
			return 0;
		}
		uint32_t cursor = 80 + transactionNumVarInt.size;
		for (uint16_t x = 0; x < self->transactionNum; x++) {
			CBByteArray * data = CBByteArraySubReference(bytes, cursor, bytes->length-cursor);
			if (NOT data) {
				CBGetMessage(self)->onErrorReceived(CB_ERROR_INIT_FAIL,"Could not create a new CBByteArray in CBBlockDeserialise for the transaction number %u.",x);
			}
			CBTransaction * transaction = CBNewTransactionFromData(data, CBGetMessage(self)->onErrorReceived);
			if (NOT transaction){
				CBGetMessage(self)->onErrorReceived(CB_ERROR_INIT_FAIL,"Could not create a new CBTransaction in CBBlockDeserialise for the transaction number %u.",x);
				CBReleaseObject(data);
				return 0;
			}
			uint32_t len = CBTransactionDeserialise(transaction);
			if (NOT len){
				CBGetMessage(self)->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"CBBlock cannot be deserialised because of an error with the transaction number %u.",x);
				CBReleaseObject(data);
				return 0;
			}
			// Readjust CBByteArray length
			data->length = len;
			CBReleaseObject(data);
			self->transactions[x] = transaction;
			cursor += len;
		}
		return cursor;
	}else{ // Just header
		uint8_t x;
		if (firstByte < 253) {
			x = 1;
		}else if (firstByte == 253){
			x = 2;
		}else if (firstByte == 254){
			x = 4;
		}else{
			x = 8;
		}
		if (bytes->length < 80 + x + 1) {
			CBGetMessage(self)->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBBlock header with not enough space to cover the var int.");
			return 0;
		}
		self->transactionNum = (uint32_t)CBVarIntDecode(bytes, 80).val; // This value is undefined in the protocol. Should best be zero when getting the headers since there is not supposed to be any transactions. Would have probably been better if the var int was dropped completely for headers only.
		// Ensure null byte is null. This null byte is a bit of a nuissance but it exists in the protocol when there are no transactions.
		if (CBByteArrayGetByte(bytes, 80 + x) != 0) {
			CBGetMessage(self)->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBBlock header with a final byte which is not null. This is not what it is supposed to be but you already knew that right?");
			return 0;
		}
		return 80 + x + 1; // 80 header bytes, the var int and the null byte
	}
}

CBByteArray * CBBlockGetHash(CBBlock * self){
	if (NOT self->hash)
		self->hash = CBBlockCalculateHash(self);
	CBRetainObject(self->hash);
	return self->hash;
}
uint32_t CBBlockSerialise(CBBlock * self,bool transactions){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (NOT bytes) {
		CBGetMessage(self)->onErrorReceived(CB_ERROR_MESSAGE_SERIALISATION_NULL_BYTES,"Attempting to serialise a CBBlock with no bytes.");
		return 0;
	}
	CBVarInt transactionNum = CBVarIntFromUInt64(self->transactionNum);
	uint32_t cursor = 80 + transactionNum.size;
	if (bytes->length < cursor + 1) {
		CBGetMessage(self)->onErrorReceived(CB_ERROR_MESSAGE_SERIALISATION_BAD_BYTES,"Attempting to serialise a CBBlock with less bytes than required for the header, transaction number var int and at least a null byte. %i < %i\n",bytes->length, cursor);
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
	CBByteArraySetInt32(bytes, 76, self->nounce);
	// Do Transactions
	CBVarIntEncode(bytes, 80, transactionNum);
	if (transactions) {
		for (uint32_t x = 0; x < self->transactionNum; x++) {
			CBGetMessage(self->transactions[x])->bytes = CBByteArraySubReference(bytes, cursor, bytes->length-cursor);
			if (NOT CBGetMessage(self->transactions[x])->bytes) {
				CBGetMessage(self)->onErrorReceived(CB_ERROR_INIT_FAIL,"Cannot create a new CBByteArray sub reference in CBBlockSerialise for the transaction number %u",x);
				return 0;
			}
			uint32_t len = CBTransactionSerialise(self->transactions[x]);
			if (NOT len) {
				CBGetMessage(self)->onErrorReceived(CB_ERROR_MESSAGE_SERIALISATION_BAD_BYTES,"CBBlock cannot be serialised because of an error with the transaction number %u.",x);
				return 0;
			}
			CBGetMessage(self->transactions[x])->bytes->length = len;
			cursor += len;
		}
	}else{
		// Add null byte since there are to be no transactions (header only).
		CBByteArraySetByte(bytes, cursor, 0);
		cursor++;
	}
	return cursor;
}
