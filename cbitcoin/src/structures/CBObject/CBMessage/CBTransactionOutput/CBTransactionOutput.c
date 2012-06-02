//
//  CBTransactionOutput.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 03/05/2012.
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

#include "CBTransactionOutput.h"

//  Virtual Table Store

static void * VTStore = NULL;
static int objectNum = 0;

//  Constructors

CBTransactionOutput * CBNewTransactionOutput(CBNetworkParameters * params, void * parent, u_int64_t value, CBByteArray * scriptBytes,CBEvents * events){
	CBTransactionOutput * self = malloc(sizeof(*self));
	objectNum++;
	CBAddVTToObject(CBGetObject(self), &VTStore, CBCreateTransactionOutputVT);
	CBInitTransactionOutput(self,params,parent,value,scriptBytes,events);
	return self;
}
CBTransactionOutput * CBNewTransactionOutputFromData(CBNetworkParameters * params, CBByteArray * data, void * parent,CBEvents * events){
	CBTransactionOutput * self = malloc(sizeof(*self));
	objectNum++;
	CBAddVTToObject(CBGetObject(self), &VTStore, CBCreateTransactionOutputVT);
	CBInitTransactionOutputFromData(self,params,data,parent,events);
	return self;
}

//  Virtual Table Creation

CBTransactionOutputVT * CBCreateTransactionOutputVT(){
	CBTransactionOutputVT * VT = malloc(sizeof(*VT));
	CBSetTransactionOutputVT(VT);
	return VT;
}
void CBSetTransactionOutputVT(CBTransactionOutputVT * VT){
	CBSetMessageVT((CBMessageVT *)VT);
	((CBObjectVT *)VT)->free = (void (*)(void *))CBFreeTransactionOutput;
	((CBMessageVT *)VT)->serialise = (u_int32_t (*)(void *))CBTransactionOutputSerialise;
	((CBMessageVT *)VT)->deserialise = (u_int32_t (*)(void *))CBTransactionOutputDeserialise;
}

//  Virtual Table Getter

CBTransactionOutputVT * CBGetTransactionOutputVT(void * self){
	return ((CBTransactionOutputVT *)(CBGetObject(self))->VT);
}

//  Object Getter

CBTransactionOutput * CBGetTransactionOutput(void * self){
	return self;
}

//  Initialisers

bool CBInitTransactionOutput(CBTransactionOutput * self,CBNetworkParameters * params, void * parent, u_int64_t value, CBByteArray * scriptBytes,CBEvents * events){
	self->scriptObject = CBNewScript(params, scriptBytes, events);
	self->value = value;
	self->parentTransaction = parent;
	if (!CBInitMessageByObject(CBGetMessage(self), params, events))
		return false;
	return true;
}
bool CBInitTransactionOutputFromData(CBTransactionOutput * self,CBNetworkParameters * params,CBByteArray * data, void * parent, CBEvents * events){
	self->scriptObject = NULL;
	self->parentTransaction = parent;
	if (!CBInitMessageByData(CBGetMessage(self), params, data, events))
		return false;
	self->parentTransaction = parent;
	return true;
}

//  Destructor

void CBFreeTransactionOutput(CBTransactionOutput * self){
	CBFreeProcessTransactionOutput(self);
	CBFree();
}
void CBFreeProcessTransactionOutput(CBTransactionOutput * self){
	if (self->scriptObject) CBGetObjectVT(self->scriptObject)->release(&self->scriptObject);
	if (self->parentTransaction) CBGetObjectVT(self->parentTransaction)->release(&self->parentTransaction);
	CBFreeProcessObject(CBGetObject(self));
}

//  Functions

u_int32_t CBTransactionOutputDeserialise(CBTransactionOutput * self){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (!bytes) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_NULL_BYTES,"Attempting to deserialise a CBTransactionOutput with no bytes.");
		return 0;
	}
	u_int8_t x = CBGetByteArrayVT(bytes)->getByte(bytes,8); // Check length for decoding CBVarInt
	if (x < 253)
		x = 9;
	else if (x == 253)
		x = 11;
	else if (x == 254)
		x = 13;
	else
		x = 17;
	if (bytes->length < x) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBTransactionOutput with less than %i bytes required.",x);
		return 0;
	}
	CBVarInt scriptLen = CBVarIntDecode(bytes, 8); // Can now decode.
	u_int32_t reqLen = (u_int32_t)(8 + scriptLen.size + scriptLen.val);
	if (bytes->length < reqLen) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBTransactionOutput with less bytes than needed according to the length for the script. %i < %i",bytes->length, reqLen);
		return 0;
	}
	// Deserialise by subreferencing byte arrays and reading integers.
	self->value = CBGetByteArrayVT(bytes)->readUInt64(bytes,0);
	self->scriptObject = CBNewScript(CBGetMessage(self)->params, CBGetByteArrayVT(bytes)->subRef(bytes,8 + scriptLen.size,(u_int32_t) scriptLen.val), CBGetMessage(self)->events);
	return reqLen;
}
u_int32_t CBTransactionOutputSerialise(CBTransactionOutput * self){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (!bytes) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_SERIALISATION_NULL_BYTES,"Attempting to serialise a CBTransactionInput with no bytes.");
		return 0;
	}
	CBVarInt scriptLen = CBVarIntFromUInt64(self->scriptObject->program->length);
	u_int32_t reqLen = 8 + scriptLen.size + self->scriptObject->program->length;
	if (bytes->length < reqLen) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_SERIALISATION_BAD_BYTES,"Attempting to serialise a CBTransactionOutput with less bytes than required. %i < %i\n",bytes->length, reqLen);
		return 0;
	}
	if (!self->scriptObject){
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_SERIALISATION_BAD_DATA,"Attempting to serialise a CBTransactionOutput without scriptObject.");
		return 0;
	}
	// Serialise data into the CBByteArray and rereference objects to this CBByteArray to save memory.
	CBGetByteArrayVT(bytes)->setUInt64(bytes,0,self->value);
	CBVarIntEncode(bytes, 8, scriptLen);
	CBGetByteArrayVT(bytes)->copyArray(bytes,8 + scriptLen.size,self->scriptObject->program);
	CBGetByteArrayVT(self->scriptObject->program)->changeRef(self->scriptObject->program,bytes,8 + scriptLen.size);
	return reqLen;
}
