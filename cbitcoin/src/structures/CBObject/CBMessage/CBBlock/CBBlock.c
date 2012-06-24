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

CBBlock * CBNewBlock(void * params,CBEvents * events){
	CBBlock * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeBlock;
	CBInitBlock(self,params,events);
	return self;
}
CBBlock * CBNewBlockFromData(void * params,CBByteArray * data,CBEvents * events){
	CBBlock * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeBlock;
	CBInitBlockFromData(self,params,data,events);
	return self;
}

//  Object Getter

CBBlock * CBGetBlock(void * self){
	return self;
}

//  Initialiser

bool CBInitBlock(CBBlock * self,void * params,CBEvents * events){
	self->prevBlockHash = NULL;
	self->merkleRoot = NULL;
	self->transactions = NULL;
	self->transactionNum = 0;
	if (!CBInitMessageByObject(CBGetMessage(self), params, events))
		return false;
	return true;
}
bool CBInitBlockFromData(CBBlock * self,void * params,CBByteArray * data,CBEvents * events){
	self->prevBlockHash = NULL;
	self->merkleRoot = NULL;
	self->transactions = NULL;
	self->transactionNum = 0;
	if (!CBInitMessageByData(CBGetMessage(self), params, data, events))
		return false;
	return true;
}

//  Destructor

void CBFreeBlock(void * vself){
	CBBlock * self = vself;
	if(self->prevBlockHash) CBReleaseObject(self->prevBlockHash);
	if(self->merkleRoot) CBReleaseObject(self->merkleRoot);
	for (u_int32_t x = 0; x < self->transactionNum; x++) {
		if(self->transactions[x]) CBReleaseObject(self->transactions[x]);
	}
	if(self->transactions) CBReleaseObject(self->transactions);
	CBFreeObject(CBGetObject(self));
}

//  Functions

u_int32_t CBBlockDeserialise(CBBlock * self){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (!bytes) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_NULL_BYTES,"Attempting to deserialise a CBBlock with no bytes.");
		return 0;
	}
	if (bytes->length < 81) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBBlock with less than 81 bytes. Minimum for header.");
		return 0;
	}
	self->version = CBByteArrayReadUInt32(bytes, 0);
	self->prevBlockHash = CBByteArraySubReference(bytes, 4, 32);
	self->merkleRoot = CBByteArraySubReference(bytes, 36, 32);
	self->time = CBByteArrayReadUInt32(bytes, 68);
	self->difficulty = CBByteArrayReadUInt32(bytes, 72);
	self->nounce = CBByteArrayReadUInt32(bytes, 76);
	// If first VarInt byte is zero, then stop here for headers, otherwise look for 8 more bytes and continue
	if (CBByteArrayGetByte(bytes, 80)) {
		// More to come
		if (bytes->length < 89) {
			CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBBlock with a non-zero varint with less than 89 bytes.");
			return 0;
		}
		CBVarInt transactionNumVarInt = CBVarIntDecode(bytes, 80);
		if (transactionNumVarInt.val*60 > bytes->length - 81) {
			CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBBlock with too many transactions for the byte data length.");
			return 0;
		}
		self->transactionNum = (u_int32_t)transactionNumVarInt.val;
		self->transactions = malloc(sizeof(*self->transactions) * self->transactionNum);
		u_int32_t cursor = 80 + transactionNumVarInt.size;
		for (u_int16_t x = 0; x < self->transactionNum; x++) {
			CBTransaction * transaction = CBNewTransactionFromData(CBGetMessage(self)->params, CBByteArraySubReference(bytes, cursor, bytes->length-cursor), CBGetMessage(self)->events);
			u_int32_t len = CBTransactionDeserialise(transaction);
			if (!len) {
				CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"CBBlock cannot be deserialised because of an error with the transaction number %u.",x);
				CBReleaseObject(&transaction);
				for (u_int16_t y = 0; y < x; y++) {
					CBReleaseObject(&self->transactions[y]);
				}
				self->transactions = NULL;
				return 0;
			}
			CBGetMessage(transaction)->bytes->length = len;
			self->transactions[x] = transaction;
			cursor += len;
		}
		return cursor;
	}else return 81; // Just header
}
u_int32_t CBBlockSerialise(CBBlock * self){
	
}
