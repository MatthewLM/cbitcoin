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

CBTransactionInput * CBNewTransactionInput(CBScript * script,uint32_t sequence, CBByteArray * prevOutHash,uint32_t prevOutIndex,void (*onErrorReceived)(CBError error,char *,...)){
	CBTransactionInput * self = malloc(sizeof(*self));
	if (NOT self) {
		onErrorReceived(CB_ERROR_OUT_OF_MEMORY,"Cannot allocate %i bytes of memory in CBNewTransactionInput\n",sizeof(*self));
		return NULL;
	}
	CBGetObject(self)->free = CBFreeTransactionInput;
	if(CBInitTransactionInput(self,script,sequence,prevOutHash,prevOutIndex,onErrorReceived))
		return self;
	free(self);
	return NULL;
}
CBTransactionInput * CBNewTransactionInputFromData(CBByteArray * data,void (*onErrorReceived)(CBError error,char *,...)){
	CBTransactionInput * self = malloc(sizeof(*self));
	if (NOT self) {
		onErrorReceived(CB_ERROR_OUT_OF_MEMORY,"Cannot allocate %i bytes of memory in CBNewTransactionInputFromData\n",sizeof(*self));
		return NULL;
	}
	CBGetObject(self)->free = CBFreeTransactionInput;
	if(CBInitTransactionInputFromData(self, data, onErrorReceived))
		return self;
	free(self);
	return NULL;
}
CBTransactionInput * CBNewUnsignedTransactionInput(uint32_t sequence,CBByteArray * prevOutHash,uint32_t prevOutIndex,void (*onErrorReceived)(CBError error,char *,...)){
	CBTransactionInput * self = malloc(sizeof(*self));
	if (NOT self) {
		onErrorReceived(CB_ERROR_OUT_OF_MEMORY,"Cannot allocate %i bytes of memory in CBNewUnsignedTransactionInput\n",sizeof(*self));
		return NULL;
	}
	CBGetObject(self)->free = CBFreeTransactionInput;
	if(CBInitUnsignedTransactionInput(self,sequence,prevOutHash,prevOutIndex, onErrorReceived))
		return self;
	free(self);
	return NULL;
}

//  Object Getter

CBTransactionInput * CBGetTransactionInput(void * self){
	return self;
}

//  Initialisers

bool CBInitTransactionInput(CBTransactionInput * self,CBScript * script,uint32_t sequence,CBByteArray * prevOutHash,uint32_t prevOutIndex,void (*onErrorReceived)(CBError error,char *,...)){
	if (script){
		self->scriptObject = script;
		CBRetainObject(script);
	}else
		self->scriptObject = NULL;
	self->prevOut.hash = prevOutHash;
	CBRetainObject(prevOutHash);
	self->prevOut.index = prevOutIndex;
	self->sequence = sequence;
	if (NOT CBInitMessageByObject(CBGetMessage(self), onErrorReceived))
		return false;
	return true;
}
bool CBInitTransactionInputFromData(CBTransactionInput * self, CBByteArray * data,void (*onErrorReceived)(CBError error,char *,...)){
	self->scriptObject = NULL;
	self->prevOut.hash = NULL;
	if (NOT CBInitMessageByData(CBGetMessage(self), data, onErrorReceived))
		return false;
	return true;
}
bool CBInitUnsignedTransactionInput(CBTransactionInput * self,uint32_t sequence,CBByteArray * prevOutHash,uint32_t prevOutIndex,void (*onErrorReceived)(CBError error,char *,...)){
	self->scriptObject = NULL;
	self->prevOut.hash = prevOutHash;
	CBRetainObject(prevOutHash);
	self->prevOut.index = prevOutIndex;
	self->sequence = sequence;
	if (NOT CBInitMessageByObject(CBGetMessage(self), onErrorReceived))
		return false;
	return true;
}

//  Destructor

void CBFreeTransactionInput(void * vself){
	CBTransactionInput * self = vself;
	if (self->scriptObject) CBReleaseObject(self->scriptObject);
	if (self->prevOut.hash) CBReleaseObject(self->prevOut.hash);
	CBFreeMessage(self);
}

//  Functions

uint32_t CBTransactionInputDeserialise(CBTransactionInput * self){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (NOT bytes) {
		CBGetMessage(self)->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_NULL_BYTES,"Attempting to deserialise a CBTransactionInput with no bytes.");
		return 0;
	}
	if (bytes->length < 41) {
		CBGetMessage(self)->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBTransactionInput with less than 41 bytes.");
		return 0;
	}
	CBVarInt scriptLen = CBVarIntDecode(bytes, 36);
	if (scriptLen.val > 10000) {
		CBGetMessage(self)->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBTransactionInput with too big a script.");
		return 0;
	}
	uint32_t reqLen = (uint32_t)(40 + scriptLen.size + scriptLen.val);
	if (bytes->length < reqLen) {
		CBGetMessage(self)->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBTransactionInput with less bytes than needed according to the length for the script. %i < %i",bytes->length, reqLen);
		return 0;
	}
	// Deserialise by subreferencing byte arrays and reading integers.
	self->prevOut.hash = CBByteArraySubReference(bytes, 0, 32);
	if (NOT self->prevOut.hash){
		CBGetMessage(self)->onErrorReceived(CB_ERROR_INIT_FAIL,"Cannot create a new CBByteArray in CBTransactionInputDeserialise");
		return 0;
	}
	self->prevOut.index = CBByteArrayReadInt32(bytes, 32);
	self->scriptObject = CBNewScriptFromReference(bytes,36 + scriptLen.size, (uint32_t) scriptLen.val);
	if (NOT self->scriptObject) {
		CBGetMessage(self)->onErrorReceived(CB_ERROR_INIT_FAIL,"Cannot create a new CBScript in CBTransactionInputDeserialise");
		CBReleaseObject(self->prevOut.hash);
		return 0;
	}
	self->sequence = CBByteArrayReadInt32(bytes, (uint32_t) (36 + scriptLen.size + scriptLen.val));
	return reqLen;
}
uint32_t CBTransactionInputSerialise(CBTransactionInput * self){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (NOT bytes) {
		CBGetMessage(self)->onErrorReceived(CB_ERROR_MESSAGE_SERIALISATION_NULL_BYTES,"Attempting to serialise a CBTransactionInput with no bytes.");
		return 0;
	}
	if (NOT self->prevOut.hash){
		CBGetMessage(self)->onErrorReceived(CB_ERROR_MESSAGE_SERIALISATION_BAD_DATA,"Attempting to serialise a CBTransactionInput without prevOut.hash.");
		return 0;
	}
	if (NOT self->scriptObject){
		CBGetMessage(self)->onErrorReceived(CB_ERROR_MESSAGE_SERIALISATION_BAD_DATA,"Attempting to serialise a CBTransactionInput without scriptObject.");
		return 0;
	}
	CBVarInt scriptLen = CBVarIntFromUInt64(CBGetByteArray(self->scriptObject)->length);
	uint32_t reqLen = 40 + scriptLen.size + CBGetByteArray(self->scriptObject)->length;
	if (bytes->length < reqLen) {
		CBGetMessage(self)->onErrorReceived(CB_ERROR_MESSAGE_SERIALISATION_BAD_BYTES,"Attempting to serialise a CBTransactionInput with less bytes than required. %i < %i\n",bytes->length, reqLen);
		return 0;
	}
	// Serialise data into the CBByteArray and rereference objects to this CBByteArray to save memory.
	CBByteArrayCopyByteArray(bytes, 0, self->prevOut.hash);
	CBByteArrayChangeReference(self->prevOut.hash, bytes, 0);
	CBByteArraySetInt32(bytes, 32, self->prevOut.index);
	CBVarIntEncode(bytes, 36, scriptLen);
	CBByteArrayCopyByteArray(bytes, 36 + scriptLen.size,CBGetByteArray(self->scriptObject));
	CBByteArrayChangeReference(CBGetByteArray(self->scriptObject),bytes,36 + scriptLen.size);
	CBByteArraySetInt32(bytes,36 + scriptLen.size + CBGetByteArray(self->scriptObject)->length,self->sequence);
	return reqLen;
}

