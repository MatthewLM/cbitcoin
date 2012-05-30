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

CBTransactionInput * CBNewTransactionInput(CBNetworkParameters * params, void * parentTransaction, CBByteArray * scriptData,CBByteArray * outPointerHash,u_int32_t outPointerIndex,u_int32_t protocolVersion,CBEvents * events){
	CBTransactionInput * self = malloc(sizeof(*self));
	objectNum++;
	CBAddVTToObject(CBGetObject(self), &VTStore, CBCreateTransactionInputVT);
	CBInitTransactionInput(self,params,parentTransaction,scriptData,outPointerHash,outPointerIndex,protocolVersion,events);
	return self;
}
CBTransactionInput * CBNewTransactionInputFromData(CBNetworkParameters * params, CBByteArray * data,u_int32_t protocolVersion,CBEvents * events){
	CBTransactionInput * self = malloc(sizeof(*self));
	objectNum++;
	CBAddVTToObject(CBGetObject(self), &VTStore, CBCreateTransactionInputVT);
	CBInitMessageByData(CBGetMessage(self), params, data, protocolVersion, events);
	return self;
}
CBTransactionInput * CBNewUnsignedTransactionInput(CBNetworkParameters * params, void * parentTransaction,CBByteArray * outPointerHash,u_int32_t outPointerIndex,u_int32_t protocolVersion,CBEvents * events){
	CBTransactionInput * self = malloc(sizeof(*self));
	objectNum++;
	CBAddVTToObject(CBGetObject(self), &VTStore, CBCreateTransactionInputVT);
	CBInitUnsignedTransactionInput(self,params,parentTransaction,outPointerHash,outPointerIndex, protocolVersion, events);
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
	((CBMessageVT *)VT)->serialise = (bool (*)(void *))CBTransactionInputSerialise;
	((CBMessageVT *)VT)->deserialise = (bool (*)(void *))CBTransactionInputDeserialise;
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

bool CBInitTransactionInput(CBTransactionInput * self,CBNetworkParameters * params, void * parentTransaction, CBByteArray * scriptData,CBByteArray * outPointerHash,u_int32_t outPointerIndex,u_int32_t protocolVersion,CBEvents * events){
	self->scriptObject = CBNewScript(params, scriptData, events);
	self->outPointerHash = outPointerHash;
	CBGetObjectVT(outPointerHash)->retain(outPointerHash);
	self->outPointerIndex = outPointerIndex;
	self->sequence = CB_TRANSACTION_INPUT_FINAL;
	self->parentTransaction = parentTransaction;
	if (!CBInitMessageByObject(CBGetMessage(self), params, 40 + CBVarIntSizeOf(scriptData->length) + scriptData->length, protocolVersion, events))
		return false;
	return true;
}
bool CBInitUnsignedTransactionInput(CBTransactionInput * self,CBNetworkParameters * params, void * parentTransaction,CBByteArray * outPointerHash,u_int32_t outPointerIndex,u_int32_t protocolVersion,CBEvents * events){
	self->scriptObject = NULL;
	self->outPointerHash = outPointerHash;
	CBGetObjectVT(outPointerHash)->retain(outPointerHash);
	self->outPointerIndex = outPointerIndex;
	self->sequence = CB_TRANSACTION_INPUT_FINAL;
	self->parentTransaction = parentTransaction;
	if (!CBInitMessageByObject(CBGetMessage(self), params, 41, protocolVersion, events))
		return false;
	return true;
}

//  Destructor

void CBFreeTransactionInput(CBTransactionInput * self){
	CBFreeProcessTransactionInput(self);
	CBFree();
}
void CBFreeProcessTransactionInput(CBTransactionInput * self){
	CBGetObjectVT(self->scriptObject)->release(&self->scriptObject);
	CBGetObjectVT(self->outPointerHash)->release(&self->outPointerHash);
	CBGetObjectVT(self->parentTransaction)->release(&self->parentTransaction);
	CBFreeProcessMessage(CBGetMessage(self));
}

//  Functions

bool CBTransactionInputDeserialise(CBTransactionInput * self){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (!bytes) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_NULL_BYTES,"Attempting to deserialise a CBTransactionInput with no bytes.");
		return false;
	}
	if (!bytes->length < 41) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBTransactionInput with less than 41 bytes.");
		return false;
	}
	CBVarInt scriptLen = CBVarIntDecode(bytes, 36);
	if (bytes->length < 40 + scriptLen.size + scriptLen.val) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBTransactionInput with less bytes than needed according to the length for the script.");
		return false;
	}
	// Deserialise by subreferencing byte arrays and reading integers.
	self->outPointerHash = CBGetByteArrayVT(bytes)->subRef(bytes,0,32);
	self->outPointerIndex = CBGetByteArrayVT(bytes)->readUInt32(bytes,32);
	self->scriptObject = CBNewScript(CBGetMessage(self)->params, CBGetByteArrayVT(bytes)->subRef(bytes,36 + scriptLen.size,(u_int32_t) scriptLen.val), CBGetMessage(self)->events);
	self->sequence = CBGetByteArrayVT(bytes)->readUInt32(bytes,(u_int32_t) (36 + scriptLen.size + scriptLen.val));
	return true;
}
bool CBTransactionInputSerialise(CBTransactionInput * self){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (!bytes) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_SERIALISATION_NULL_BYTES,"Attempting to serialise a CBTransactionInput with no bytes.");
		return false;
	}
	CBVarInt scriptLen = CBVarIntFromUInt64(self->scriptObject->program->length);
	if (!bytes->length < 40 + scriptLen.size + self->scriptObject->program->length) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_SERIALISATION_BAD_BYTES,"Attempting to serialise a CBTransactionInput with less bytes than required.");
		return false;
	}
	if (!self->outPointerHash){
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_SERIALISATION_BAD_DATA,"Attempting to serialise a CBTransactionInput without outPointerHash.");
		return false;
	}
	if (!self->scriptObject){
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_SERIALISATION_BAD_DATA,"Attempting to serialise a CBTransactionInput without scriptObject.");
		return false;
	}
	// Serialise data into the CBByteArray and rereference objects to this CBByteArray to save memory.
	CBGetByteArrayVT(bytes)->copyArray(bytes,0,self->outPointerHash);
	CBGetByteArrayVT(self->outPointerHash)->changeRef(self->outPointerHash,bytes,0);
	CBGetByteArrayVT(bytes)->setUInt32(bytes,32,self->outPointerIndex);
	CBVarIntEncode(bytes, 36, scriptLen);
	CBGetByteArrayVT(bytes)->copyArray(bytes,36 + scriptLen.size,self->scriptObject->program);
	CBGetByteArrayVT(self->scriptObject->program)->changeRef(self->scriptObject->program,bytes,36 + scriptLen.size);
	CBGetByteArrayVT(bytes)->setUInt32(bytes,36 + scriptLen.size + self->scriptObject->program->length,self->sequence);
	return true;
}

