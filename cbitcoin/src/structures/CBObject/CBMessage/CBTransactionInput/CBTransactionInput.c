//
//  CBTransactionInput.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 02/05/2012.
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

#include "CBTransactionInput.h"

//  Virtual Table Store

static void * VTStore = NULL;
static int objectNum = 0;

//  Constructors

CBTransactionInput * CBNewTransactionInput(CBNetworkParameters * params, void * parentTransaction, CBByteArray * scriptData,CBByteArray * outPointerHash,u_int32_t outPointerIndex,CBEvents * events){
	CBTransactionInput * self = malloc(sizeof(*self));
	objectNum++;
	CBAddVTToObject(CBGetObject(self), &VTStore, CBCreateTransactionInputVT);
	CBInitTransactionInput(self,params,parentTransaction,scriptData,outPointerHash,outPointerIndex,events);
	return self;
}
CBTransactionInput * CBNewTransactionInputFromData(CBNetworkParameters * params, CBByteArray * data, void * parentTransaction,CBEvents * events){
	CBTransactionInput * self = malloc(sizeof(*self));
	objectNum++;
	CBAddVTToObject(CBGetObject(self), &VTStore, CBCreateTransactionInputVT);
	CBInitTransactionInputFromData(self, params, data, parentTransaction, events);
	return self;
}
CBTransactionInput * CBNewUnsignedTransactionInput(CBNetworkParameters * params, void * parentTransaction,CBByteArray * outPointerHash,u_int32_t outPointerIndex,CBEvents * events){
	CBTransactionInput * self = malloc(sizeof(*self));
	objectNum++;
	CBAddVTToObject(CBGetObject(self), &VTStore, CBCreateTransactionInputVT);
	CBInitUnsignedTransactionInput(self,params,parentTransaction,outPointerHash,outPointerIndex, events);
	return self;
}

//  Virtual Table Creation

CBTransactionInputVT * CBCreateTransactionInputVT(){
	CBTransactionInputVT * VT = malloc(sizeof(*VT));
	CBSetTransactionInputVT(VT);
	return VT;
}
void CBSetTransactionInputVT(CBTransactionInputVT * VT){
	CBSetMessageVT((CBMessageVT *)VT);
	((CBObjectVT *)VT)->free = (void (*)(void *))CBFreeTransactionInput;
	((CBMessageVT *)VT)->serialise = (u_int32_t (*)(void *))CBTransactionInputSerialise;
	((CBMessageVT *)VT)->deserialise = (u_int32_t (*)(void *))CBTransactionInputDeserialise;
}

//  Virtual Table Getter

CBTransactionInputVT * CBGetTransactionInputVT(void * self){
	return ((CBTransactionInputVT *)(CBGetObject(self))->VT);
}

//  Object Getter

CBTransactionInput * CBGetTransactionInput(void * self){
	return self;
}

//  Initialisers

bool CBInitTransactionInput(CBTransactionInput * self,CBNetworkParameters * params, void * parentTransaction, CBByteArray * scriptData,CBByteArray * outPointerHash,u_int32_t outPointerIndex,CBEvents * events){
	self->scriptObject = CBNewScript(params, scriptData, events);
	self->outPointerHash = outPointerHash;
	CBGetObjectVT(outPointerHash)->retain(outPointerHash);
	self->outPointerIndex = outPointerIndex;
	self->sequence = CB_TRANSACTION_INPUT_FINAL;
	self->parentTransaction = parentTransaction;
	if (!CBInitMessageByObject(CBGetMessage(self), params, events))
		return false;
	return true;
}
bool CBInitTransactionInputFromData(CBTransactionInput * self,CBNetworkParameters * params, CBByteArray * data, void * parentTransaction,CBEvents * events){
	self->scriptObject = NULL;
	self->outPointerHash = NULL;
	self->parentTransaction = parentTransaction;
	if (!CBInitMessageByData(CBGetMessage(self), params, data, events))
		return false;
	return true;
}
bool CBInitUnsignedTransactionInput(CBTransactionInput * self,CBNetworkParameters * params, void * parentTransaction,CBByteArray * outPointerHash,u_int32_t outPointerIndex,CBEvents * events){
	self->scriptObject = NULL;
	self->outPointerHash = outPointerHash;
	CBGetObjectVT(outPointerHash)->retain(outPointerHash);
	self->outPointerIndex = outPointerIndex;
	self->sequence = CB_TRANSACTION_INPUT_FINAL;
	self->parentTransaction = parentTransaction;
	if (!CBInitMessageByObject(CBGetMessage(self), params, events))
		return false;
	return true;
}

//  Destructor

void CBFreeTransactionInput(CBTransactionInput * self){
	CBFreeProcessTransactionInput(self);
	CBFree();
}
void CBFreeProcessTransactionInput(CBTransactionInput * self){
	if (self->scriptObject) CBGetObjectVT(self->scriptObject)->release(&self->scriptObject);
	if (self->outPointerHash) CBGetObjectVT(self->outPointerHash)->release(&self->outPointerHash);
	if (self->parentTransaction) CBGetObjectVT(self->parentTransaction)->release(&self->parentTransaction);
	CBFreeProcessMessage(CBGetMessage(self));
}

//  Functions

u_int32_t CBTransactionInputDeserialise(CBTransactionInput * self){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (!bytes) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_NULL_BYTES,"Attempting to deserialise a CBTransactionInput with no bytes.");
		return 0;
	}
	if (bytes->length < 41) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBTransactionInput with less than 41 bytes.");
		return 0;
	}
	CBVarInt scriptLen = CBVarIntDecode(bytes, 36);
	u_int32_t reqLen = (u_int32_t)(40 + scriptLen.size + scriptLen.val);
	if (bytes->length < reqLen) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBTransactionInput with less bytes than needed according to the length for the script. %i < %i",bytes->length, reqLen);
		return 0;
	}
	// Deserialise by subreferencing byte arrays and reading integers.
	self->outPointerHash = CBGetByteArrayVT(bytes)->subRef(bytes,0,32);
	self->outPointerIndex = CBGetByteArrayVT(bytes)->readUInt32(bytes,32);
	self->scriptObject = CBNewScript(CBGetMessage(self)->params, CBGetByteArrayVT(bytes)->subRef(bytes,36 + scriptLen.size,(u_int32_t) scriptLen.val), CBGetMessage(self)->events);
	self->sequence = CBGetByteArrayVT(bytes)->readUInt32(bytes,(u_int32_t) (36 + scriptLen.size + scriptLen.val));
	return reqLen;
}
u_int32_t CBTransactionInputSerialise(CBTransactionInput * self){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (!bytes) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_SERIALISATION_NULL_BYTES,"Attempting to serialise a CBTransactionInput with no bytes.");
		return 0;
	}
	CBVarInt scriptLen = CBVarIntFromUInt64(self->scriptObject->program->length);
	u_int32_t reqLen = 40 + scriptLen.size + self->scriptObject->program->length;
	if (bytes->length < reqLen) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_SERIALISATION_BAD_BYTES,"Attempting to serialise a CBTransactionInput with less bytes than required. %i < %i\n",bytes->length, reqLen);
		return 0;
	}
	if (!self->outPointerHash){
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_SERIALISATION_BAD_DATA,"Attempting to serialise a CBTransactionInput without outPointerHash.");
		return 0;
	}
	if (!self->scriptObject){
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_SERIALISATION_BAD_DATA,"Attempting to serialise a CBTransactionInput without scriptObject.");
		return 0;
	}
	// Serialise data into the CBByteArray and rereference objects to this CBByteArray to save memory.
	CBGetByteArrayVT(bytes)->copyArray(bytes,0,self->outPointerHash);
	CBGetByteArrayVT(self->outPointerHash)->changeRef(self->outPointerHash,bytes,0);
	CBGetByteArrayVT(bytes)->setUInt32(bytes,32,self->outPointerIndex);
	CBVarIntEncode(bytes, 36, scriptLen);
	CBGetByteArrayVT(bytes)->copyArray(bytes,36 + scriptLen.size,self->scriptObject->program);
	CBGetByteArrayVT(self->scriptObject->program)->changeRef(self->scriptObject->program,bytes,36 + scriptLen.size);
	CBGetByteArrayVT(bytes)->setUInt32(bytes,36 + scriptLen.size + self->scriptObject->program->length,self->sequence);
	return reqLen;
}

