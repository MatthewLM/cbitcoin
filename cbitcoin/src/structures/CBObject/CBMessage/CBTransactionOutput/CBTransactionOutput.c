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

//  Constructors

CBTransactionOutput * CBNewTransactionOutput(CBNetworkParameters * params, void * parent, u_int64_t value, CBScript * script,CBEvents * events){
	CBTransactionOutput * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeTransactionOutput;
	CBInitTransactionOutput(self,params,parent,value,script,events);
	return self;
}
CBTransactionOutput * CBNewTransactionOutputFromData(CBNetworkParameters * params, CBByteArray * data, void * parent,CBEvents * events){
	CBTransactionOutput * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeTransactionOutput;
	CBInitTransactionOutputFromData(self,params,data,parent,events);
	return self;
}

//  Object Getter

CBTransactionOutput * CBGetTransactionOutput(void * self){
	return self;
}

//  Initialisers

bool CBInitTransactionOutput(CBTransactionOutput * self,CBNetworkParameters * params, void * parent, u_int64_t value, CBScript * script,CBEvents * events){
	if (script){
		self->scriptObject = script;
		CBRetainObject(script);
	}else
		self->scriptObject = NULL;
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

void CBFreeTransactionOutput(void * vself){
	CBTransactionOutput * self = vself;
	if (self->scriptObject) CBReleaseObject(&self->scriptObject);
	if (self->parentTransaction) CBReleaseObject(&self->parentTransaction);
	CBFreeProcessObject(CBGetObject(self));
}

//  Functions

u_int32_t CBTransactionOutputDeserialise(CBTransactionOutput * self){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (!bytes) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_NULL_BYTES,"Attempting to deserialise a CBTransactionOutput with no bytes.");
		return 0;
	}
	u_int8_t x = CBByteArrayGetByte(bytes, 8); // Check length for decoding CBVarInt
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
	if (scriptLen.val > 10000) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBTransactionInput with too big a script.");
		return 0;
	}
	u_int32_t reqLen = (u_int32_t)(8 + scriptLen.size + scriptLen.val);
	if (bytes->length < reqLen) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBTransactionOutput with less bytes than needed according to the length for the script. %i < %i",bytes->length, reqLen);
		return 0;
	}
	// Deserialise by subreferencing byte arrays and reading integers.
	self->value = CBByteArrayReadUInt64(bytes, 0);
	self->scriptObject = CBNewScriptFromReference(CBGetMessage(self)->params, bytes, 8 + scriptLen.size, (u_int32_t) scriptLen.val, CBGetMessage(self)->events);
	return reqLen;
}
u_int32_t CBTransactionOutputSerialise(CBTransactionOutput * self){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (!bytes) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_SERIALISATION_NULL_BYTES,"Attempting to serialise a CBTransactionInput with no bytes.");
		return 0;
	}
	CBVarInt scriptLen = CBVarIntFromUInt64(CBGetByteArray(self->scriptObject)->length);
	u_int32_t reqLen = 8 + scriptLen.size + CBGetByteArray(self->scriptObject)->length;
	if (bytes->length < reqLen) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_SERIALISATION_BAD_BYTES,"Attempting to serialise a CBTransactionOutput with less bytes than required. %i < %i\n",bytes->length, reqLen);
		return 0;
	}
	if (!self->scriptObject){
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_SERIALISATION_BAD_DATA,"Attempting to serialise a CBTransactionOutput without scriptObject.");
		return 0;
	}
	// Serialise data into the CBByteArray and rereference objects to this CBByteArray to save memory.
	CBByteArraySetUInt64(bytes, 0, self->value);
	CBVarIntEncode(bytes, 8, scriptLen);
	CBByteArrayCopyByteArray(bytes, 8 + scriptLen.size,CBGetByteArray(self->scriptObject));
	CBByteArrayChangeReference(CBGetByteArray(self->scriptObject),bytes,8 + scriptLen.size);
	return reqLen;
}
