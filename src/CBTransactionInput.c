//
//  CBTransactionInput.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 02/05/2012.
//  Copyright (c) 2012 Matthew Mitchell
//  
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

//  SEE HEADER FILE FOR DOCUMENTATION

#include "CBTransactionInput.h"

//  Constructors

CBTransactionInput * CBNewTransactionInput(CBScript * script, uint32_t sequence, CBByteArray * prevOutHash, uint32_t prevOutIndex){
	CBTransactionInput * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeTransactionInput;
	CBInitTransactionInput(self, script, sequence, prevOutHash, prevOutIndex);
	return self;
}
CBTransactionInput * CBNewTransactionInputTakeScriptAndHash(CBScript * script, uint32_t sequence, CBByteArray * prevOutHash, uint32_t prevOutIndex){
	CBTransactionInput * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeTransactionInput;
	CBInitTransactionInputTakeScriptAndHash(self, script, sequence, prevOutHash, prevOutIndex);
	return self;
}
CBTransactionInput * CBNewTransactionInputFromData(CBByteArray * data){
	CBTransactionInput * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeTransactionInput;
	CBInitTransactionInputFromData(self, data);
	return self;
}

//  Object Getter

CBTransactionInput * CBGetTransactionInput(void * self){
	return self;
}

//  Initialisers

void CBInitTransactionInput(CBTransactionInput * self, CBScript * script, uint32_t sequence, CBByteArray * prevOutHash, uint32_t prevOutIndex){
	if (script)
		CBRetainObject(script);
	CBRetainObject(prevOutHash);
	CBInitTransactionInputTakeScriptAndHash(self, script, sequence, prevOutHash, prevOutIndex);
}
void CBInitTransactionInputTakeScriptAndHash(CBTransactionInput * self, CBScript * script, uint32_t sequence, CBByteArray * prevOutHash, uint32_t prevOutIndex){
	self->scriptObject = script;
	self->prevOut.hash = prevOutHash;
	self->prevOut.index = prevOutIndex;
	self->sequence = sequence;
	CBInitMessageByObject(CBGetMessage(self));
}
void CBInitTransactionInputFromData(CBTransactionInput * self, CBByteArray * data){
	self->scriptObject = NULL;
	self->prevOut.hash = NULL;
	CBInitMessageByData(CBGetMessage(self), data);
}

//  Destructor

void CBDestroyTransactionInput(void * vself){
	CBTransactionInput * self = vself;
	if (self->scriptObject) CBReleaseObject(self->scriptObject);
	if (self->prevOut.hash) CBReleaseObject(self->prevOut.hash);
	CBDestroyMessage(self);
}
void CBFreeTransactionInput(void * self){
	CBDestroyTransactionInput(self);
	free(self);
}

//  Functions

uint32_t CBTransactionInputCalculateLength(CBTransactionInput * self){
	if (CBGetMessage(self)->serialised) {
		// If it is serailised, the var int may be of a different size.
		uint8_t byte = CBByteArrayGetByte(CBGetMessage(self)->bytes, 0);
		return (byte < 253 ? 1 : (byte == 253 ? 3 : (byte == 254 ? 5 : 9))) + self->scriptObject->length + 40;
	} else return 40 + CBVarIntSizeOf(self->scriptObject->length) + self->scriptObject->length;
}
uint32_t CBTransactionInputDeserialise(CBTransactionInput * self){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (NOT bytes) {
		CBLogError("Attempting to deserialise a CBTransactionInput with no bytes.");
		return 0;
	}
	if (bytes->length < 41) {
		CBLogError("Attempting to deserialise a CBTransactionInput with less than 41 bytes.");
		return 0;
	}
	CBVarInt scriptLen = CBVarIntDecode(bytes, 36);
	if (scriptLen.val > 10000) {
		CBLogError("Attempting to deserialise a CBTransactionInput with too big a script.");
		return 0;
	}
	uint32_t reqLen = (uint32_t)(40 + scriptLen.size + scriptLen.val);
	if (bytes->length < reqLen) {
		CBLogError("Attempting to deserialise a CBTransactionInput with less bytes than needed according to the length for the script. %i < %i", bytes->length, reqLen);
		return 0;
	}
	// Deserialise by subreferencing byte arrays and reading integers.
	self->prevOut.hash = CBByteArraySubReference(bytes, 0, 32);
	self->prevOut.index = CBByteArrayReadInt32(bytes, 32);
	self->scriptObject = CBNewScriptFromReference(bytes, 36 + scriptLen.size, (uint32_t) scriptLen.val);
	self->sequence = CBByteArrayReadInt32(bytes, (uint32_t) (36 + scriptLen.size + scriptLen.val));
	return reqLen;
}
uint32_t CBTransactionInputSerialise(CBTransactionInput * self){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (NOT bytes) {
		CBLogError("Attempting to serialise a CBTransactionInput with no bytes.");
		return 0;
	}
	if (NOT self->prevOut.hash){
		CBLogError("Attempting to serialise a CBTransactionInput without prevOut.hash.");
		return 0;
	}
	if (NOT self->scriptObject){
		CBLogError("Attempting to serialise a CBTransactionInput without scriptObject.");
		return 0;
	}
	CBVarInt scriptLen = CBVarIntFromUInt64(CBGetByteArray(self->scriptObject)->length);
	uint32_t reqLen = 40 + scriptLen.size + CBGetByteArray(self->scriptObject)->length;
	if (bytes->length < reqLen) {
		CBLogError("Attempting to serialise a CBTransactionInput with less bytes than required. %i < %i", bytes->length, reqLen);
		return 0;
	}
	// Serialise data into the CBByteArray and rereference objects to this CBByteArray to save memory.
	CBByteArrayCopyByteArray(bytes, 0, self->prevOut.hash);
	CBByteArrayChangeReference(self->prevOut.hash, bytes, 0);
	CBByteArraySetInt32(bytes, 32, self->prevOut.index);
	CBVarIntEncode(bytes, 36, scriptLen);
	CBByteArrayCopyByteArray(bytes, 36 + scriptLen.size, CBGetByteArray(self->scriptObject));
	CBByteArrayChangeReference(CBGetByteArray(self->scriptObject), bytes, 36 + scriptLen.size);
	CBByteArraySetInt32(bytes, 36 + scriptLen.size + CBGetByteArray(self->scriptObject)->length, self->sequence);
	// Ensure length is correct
	bytes->length = reqLen;
	// Is serialised.
	CBGetMessage(self)->serialised = true;
	return reqLen;
}

