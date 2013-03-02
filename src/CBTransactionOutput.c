//
//  CBTransactionOutput.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 03/05/2012.
//  Copyright (c) 2012 Matthew Mitchell
//  
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

//  SEE HEADER FILE FOR DOCUMENTATION

#include "CBTransactionOutput.h"

//  Constructors

CBTransactionOutput * CBNewTransactionOutput(uint64_t value, CBScript * script){
	CBTransactionOutput * self = malloc(sizeof(*self));
	if (NOT self) {
		CBLogError("Cannot allocate %i bytes of memory in CBNewTransactionOutput\n", sizeof(*self));
		return NULL;
	}
	CBGetObject(self)->free = CBFreeTransactionOutput;
	if(CBInitTransactionOutput(self, value, script))
		return self;
	free(self);
	return NULL;
}
CBTransactionOutput * CBNewTransactionOutputFromData(CBByteArray * data){
	CBTransactionOutput * self = malloc(sizeof(*self));
	if (NOT self) {
		CBLogError("Cannot allocate %i bytes of memory in CBNewTransactionOutputFromData\n", sizeof(*self));
		return NULL;
	}
	CBGetObject(self)->free = CBFreeTransactionOutput;
	if (CBInitTransactionOutputFromData(self, data))
		return self;
	free(self);
	return NULL;
}

//  Object Getter

CBTransactionOutput * CBGetTransactionOutput(void * self){
	return self;
}

//  Initialisers

bool CBInitTransactionOutput(CBTransactionOutput * self, uint64_t value, CBScript * script){
	if (script){
		self->scriptObject = script;
		CBRetainObject(script);
	}else
		self->scriptObject = NULL;
	self->value = value;
	if (NOT CBInitMessageByObject(CBGetMessage(self)))
		return false;
	return true;
}
bool CBInitTransactionOutputFromData(CBTransactionOutput * self, CBByteArray * data){
	self->scriptObject = NULL;
	if (NOT CBInitMessageByData(CBGetMessage(self), data))
		return false;
	return true;
}

//  Destructor

void CBDestroyTransactionOutput(void * vself){
	CBTransactionOutput * self = vself;
	if (self->scriptObject) CBReleaseObject(self->scriptObject);
	CBDestroyMessage(self);
}
void CBFreeTransactionOutput(void * self){
	CBDestroyTransactionOutput(self);
	free(self);
}

//  Functions

uint32_t CBTransactionOutputCalculateLength(CBTransactionOutput * self){
	if (CBGetMessage(self)->serialised) {
		// If it is serailised, the var int may be of a different size.
		uint8_t byte = CBByteArrayGetByte(CBGetMessage(self)->bytes, 0);
		return (byte < 253 ? 1 : (byte == 253 ? 3 : (byte == 254 ? 5 : 9))) + self->scriptObject->length + 8;
	} else return CBVarIntSizeOf(self->scriptObject->length) + self->scriptObject->length + 8;
}
uint32_t CBTransactionOutputDeserialise(CBTransactionOutput * self){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (NOT bytes) {
		CBLogError("Attempting to deserialise a CBTransactionOutput with no bytes.");
		return 0;
	}
	if (bytes->length < 9) {
		CBLogError("Attempting to deserialise a CBTransactionOutput with less than 9 bytes.");
		return 0;
	}
	uint8_t x = CBByteArrayGetByte(bytes, 8); // Check length for decoding CBVarInt
	if (x < 253)
		x = 9;
	else if (x == 253)
		x = 11;
	else if (x == 254)
		x = 13;
	else
		x = 17;
	if (bytes->length < x) {
		CBLogError("Attempting to deserialise a CBTransactionOutput with less than %i bytes required.", x);
		return 0;
	}
	CBVarInt scriptLen = CBVarIntDecode(bytes, 8); // Can now decode.
	if (scriptLen.val > 10000) {
		CBLogError("Attempting to deserialise a CBTransactionInput with too big a script.");
		return 0;
	}
	uint32_t reqLen = (uint32_t)(8 + scriptLen.size + scriptLen.val);
	if (bytes->length < reqLen) {
		CBLogError("Attempting to deserialise a CBTransactionOutput with less bytes than needed according to the length for the script. %i < %i", bytes->length, reqLen);
		return 0;
	}
	// Deserialise by subreferencing byte arrays and reading integers.
	self->value = CBByteArrayReadInt64(bytes, 0);
	self->scriptObject = CBNewScriptFromReference(bytes, 8 + scriptLen.size, (uint32_t) scriptLen.val);
	if (NOT self->scriptObject){
		CBLogError("Cannot create a new CBScript in CBTransactionOutputDeserialise");
		return 0;
	}
	return reqLen;
}
bool CBTransactionOuputGetHash(CBTransactionOutput * self, uint8_t * hash){
	switch (CBTransactionOutputGetType(output)) {
		case CB_TX_OUTPUT_TYPE_KEYHASH:{
			// See if the public-key hash is one we are watching.
			uint32_t cursor = 2;
			CBScriptGetPushAmount(output->scriptObject, &cursor);
			memcpy(hash, CBByteArrayGetData(output->scriptObject) + cursor, 20);
			break;
		}
		case CB_TX_OUTPUT_TYPE_MULTISIG:
			// Hash the entire script and then see if we are watching that script.
			CBSha256(CBByteArrayGetData(output->scriptObject), output->scriptObject->length, hash);
			break;
		case CB_TX_OUTPUT_TYPE_P2SH:
			// See if the script hash is one we are watching.
			memcpy(hash, CBByteArrayGetData(output->scriptObject) + 2, 20);
			break;
		default:
			// Unsupported
			return true;
	}
	return false;
}
CBTransactionOutputType CBTransactionOutputGetType(CBTransactionOutput * self){
	if (CBScriptIsKeyHash(self->scriptObject))
		return CB_TX_OUTPUT_TYPE_KEYHASH;
	if (CBScriptIsMultisig(self->scriptObject))
		return CB_TX_OUTPUT_TYPE_MULTISIG;
	if (CBScriptIsP2SH(self->scriptObject))
		return CB_TX_OUTPUT_TYPE_P2SH;
	return CB_TX_OUTPUT_TYPE_UNKNOWN;
}
uint32_t CBTransactionOutputSerialise(CBTransactionOutput * self){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (NOT bytes) {
		CBLogError("Attempting to serialise a CBTransactionInput with no bytes.");
		return 0;
	}
	if (NOT self->scriptObject){
		CBLogError("Attempting to serialise a CBTransactionOutput without scriptObject.");
		return 0;
	}
	CBVarInt scriptLen = CBVarIntFromUInt64(CBGetByteArray(self->scriptObject)->length);
	uint32_t reqLen = 8 + scriptLen.size + CBGetByteArray(self->scriptObject)->length;
	if (bytes->length < reqLen) {
		CBLogError("Attempting to serialise a CBTransactionOutput with less bytes than required. %i < %i", bytes->length, reqLen);
		return 0;
	}
	// Serialise data into the CBByteArray and rereference objects to this CBByteArray to save memory.
	CBByteArraySetInt64(bytes, 0, self->value);
	CBVarIntEncode(bytes, 8, scriptLen);
	CBByteArrayCopyByteArray(bytes, 8 + scriptLen.size, CBGetByteArray(self->scriptObject));
	CBByteArrayChangeReference(CBGetByteArray(self->scriptObject), bytes, 8 + scriptLen.size);
	// Ensure length is correct
	bytes->length = reqLen;
	// Is serialised.
	CBGetMessage(self)->serialised = true;
	return reqLen;
}
