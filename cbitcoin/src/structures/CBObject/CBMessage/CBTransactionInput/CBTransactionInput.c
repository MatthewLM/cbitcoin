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

//  Constructors

CBTransactionInput * CBNewTransactionInput(CBNetworkParameters * params, void * parentTransaction, CBScript * script, CBByteArray * outPointerHash,u_int32_t outPointerIndex,CBEvents * events){
	CBTransactionInput * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeTransactionInput;
	CBInitTransactionInput(self,params,parentTransaction,script,outPointerHash,outPointerIndex,events);
	return self;
}
CBTransactionInput * CBNewTransactionInputFromData(CBNetworkParameters * params, CBByteArray * data, void * parentTransaction,CBEvents * events){
	CBTransactionInput * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeTransactionInput;
	CBInitTransactionInputFromData(self, params, data, parentTransaction, events);
	return self;
}
CBTransactionInput * CBNewUnsignedTransactionInput(CBNetworkParameters * params, void * parentTransaction,CBByteArray * outPointerHash,u_int32_t outPointerIndex,CBEvents * events){
	CBTransactionInput * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeTransactionInput;
	CBInitUnsignedTransactionInput(self,params,parentTransaction,outPointerHash,outPointerIndex, events);
	return self;
}

//  Object Getter

CBTransactionInput * CBGetTransactionInput(void * self){
	return self;
}

//  Initialisers

bool CBInitTransactionInput(CBTransactionInput * self,CBNetworkParameters * params, void * parentTransaction, CBScript * script,CBByteArray * outPointerHash,u_int32_t outPointerIndex,CBEvents * events){
	if (script){
		self->scriptObject = script;
		CBRetainObject(script);
	}else
		self->scriptObject = NULL;
	self->outPointerHash = outPointerHash;
	CBRetainObject(outPointerHash);
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
	CBRetainObject(outPointerHash);
	self->outPointerIndex = outPointerIndex;
	self->sequence = CB_TRANSACTION_INPUT_FINAL;
	self->parentTransaction = parentTransaction;
	if (!CBInitMessageByObject(CBGetMessage(self), params, events))
		return false;
	return true;
}

//  Destructor

void CBFreeTransactionInput(void * vself){
	CBTransactionInput * self = vself;
	if (self->scriptObject) CBReleaseObject(&self->scriptObject);
	if (self->outPointerHash) CBReleaseObject(&self->outPointerHash);
	if (self->parentTransaction) CBReleaseObject(&self->parentTransaction);
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
	if (scriptLen.val > 10000) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBTransactionInput with too big a script.");
		return 0;
	}
	u_int32_t reqLen = (u_int32_t)(40 + scriptLen.size + scriptLen.val);
	if (bytes->length < reqLen) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBTransactionInput with less bytes than needed according to the length for the script. %i < %i",bytes->length, reqLen);
		return 0;
	}
	// Deserialise by subreferencing byte arrays and reading integers.
	self->outPointerHash = CBByteArraySubReference(bytes, 0, 32);
	self->outPointerIndex = CBByteArrayReadUInt32(bytes, 32);
	self->scriptObject = CBNewScriptFromReference(CBGetMessage(self)->params, bytes,36 + scriptLen.size, (u_int32_t) scriptLen.val, CBGetMessage(self)->events);
	self->sequence = CBByteArrayReadUInt32(bytes, (u_int32_t) (36 + scriptLen.size + scriptLen.val));
	return reqLen;
}
u_int32_t CBTransactionInputSerialise(CBTransactionInput * self){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (!bytes) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_SERIALISATION_NULL_BYTES,"Attempting to serialise a CBTransactionInput with no bytes.");
		return 0;
	}
	CBVarInt scriptLen = CBVarIntFromUInt64(CBGetByteArray(self->scriptObject)->length);
	u_int32_t reqLen = 40 + scriptLen.size + CBGetByteArray(self->scriptObject)->length;
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
	CBByteArrayCopyByteArray(bytes, 0, self->outPointerHash);
	CBByteArrayChangeReference(self->outPointerHash, bytes, 0);
	CBByteArraySetUInt32(bytes, 32, self->outPointerIndex);
	CBVarIntEncode(bytes, 36, scriptLen);
	CBByteArrayCopyByteArray(bytes, 36 + scriptLen.size,CBGetByteArray(self->scriptObject));
	CBByteArrayChangeReference(CBGetByteArray(self->scriptObject),bytes,36 + scriptLen.size);
	CBByteArraySetUInt32(bytes,36 + scriptLen.size + CBGetByteArray(self->scriptObject)->length,self->sequence);
	return reqLen;
}

