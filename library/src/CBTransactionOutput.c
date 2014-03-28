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

CBTransactionOutput * CBNewTransactionOutput(uint64_t value, CBScript * script) {
	
	CBTransactionOutput * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeTransactionOutput;
	CBInitTransactionOutput(self, value, script);
	return self;
	
}

CBTransactionOutput * CBNewTransactionOutputTakeScript(uint64_t value, CBScript * script) {
	
	CBTransactionOutput * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeTransactionOutput;
	CBInitTransactionOutputTakeScript(self, value, script);
	return self;
	
}

CBTransactionOutput * CBNewTransactionOutputFromData(CBByteArray * data) {
	
	CBTransactionOutput * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeTransactionOutput;
	CBInitTransactionOutputFromData(self, data);
	return self;
	
}

//  Initialisers

void CBInitTransactionOutput(CBTransactionOutput * self, uint64_t value, CBScript * script) {
	
	if (script)
		CBRetainObject(script);
	
	CBInitTransactionOutputTakeScript(self, value, script);
	
}

void CBInitTransactionOutputTakeScript(CBTransactionOutput * self, uint64_t value, CBScript * script) {
	
	self->scriptObject = script;
	self->value = value;
	CBInitMessageByObject(CBGetMessage(self));
	
}

void CBInitTransactionOutputFromData(CBTransactionOutput * self, CBByteArray * data) {
	
	self->scriptObject = NULL;
	CBInitMessageByData(CBGetMessage(self), data);
	
}

//  Destructor

void CBDestroyTransactionOutput(void * vself) {
	
	CBTransactionOutput * self = vself;
	if (self->scriptObject) CBReleaseObject(self->scriptObject);
	CBDestroyMessage(self);
	
}

void CBFreeTransactionOutput(void * self) {
	
	CBDestroyTransactionOutput(self);
	free(self);
	
}

//  Functions

uint32_t CBTransactionOutputCalculateLength(CBTransactionOutput * self) {
	
	return CBVarIntSizeOf(self->scriptObject->length) + self->scriptObject->length + 8;
	
}
uint32_t CBTransactionOutputDeserialise(CBTransactionOutput * self) {
	
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	
	if (! bytes) {
		CBLogError("Attempting to deserialise a CBTransactionOutput with no bytes.");
		return CB_DESERIALISE_ERROR;
	}
	if (bytes->length < 9) {
		CBLogError("Attempting to deserialise a CBTransactionOutput with less than 9 bytes.");
		return CB_DESERIALISE_ERROR;
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
		return CB_DESERIALISE_ERROR;
	}
	
	CBVarInt scriptLen = CBByteArrayReadVarInt(bytes, 8); // Can now decode.
	if (scriptLen.val > 10000) {
		CBLogError("Attempting to deserialise a CBTransactionInput with too big a script.");
		return CB_DESERIALISE_ERROR;
	}
	
	uint32_t reqLen = (uint32_t)(8 + scriptLen.size + scriptLen.val);
	if (bytes->length < reqLen) {
		CBLogError("Attempting to deserialise a CBTransactionOutput with less bytes than needed according to the length for the script. %i < %i", bytes->length, reqLen);
		return CB_DESERIALISE_ERROR;
	}
	
	// Deserialise by subreferencing byte arrays and reading integers.
	self->value = CBByteArrayReadInt64(bytes, 0);
	self->scriptObject = CBNewScriptFromReference(bytes, 8 + scriptLen.size, (uint32_t) scriptLen.val);
	
	return reqLen;
	
} 
bool CBTransactionOuputGetHash(CBTransactionOutput * self, uint8_t * hash) {
	
	switch (CBTransactionOutputGetType(self)) {
			
		case CB_TX_OUTPUT_TYPE_KEYHASH:{
			// Copy the bitcoin address (public key hash)
			CBScriptOp op = CBByteArrayGetByte(self->scriptObject, 2);
			memcpy(hash, CBByteArrayGetData(self->scriptObject) + 2 + ((op < CB_SCRIPT_OP_PUSHDATA1) ? 1 : (op - CB_SCRIPT_OP_PUSHDATA1 + 2)), 20);
			break;
		}
			
		case CB_TX_OUTPUT_TYPE_MULTISIG:
			// Hash the entire script and use that as the hash identifier.
			CBRipemd160(CBByteArrayGetData(self->scriptObject), self->scriptObject->length, hash);
			break;
			
		case CB_TX_OUTPUT_TYPE_P2SH:
			// Copy the P2SH script hash.
			memcpy(hash, CBByteArrayGetData(self->scriptObject) + 2, 20);
			break;
			
		default:
			// Unsupported
			return true;
	}
	
	return false;
	
}

CBScriptOutputType CBTransactionOutputGetType(CBTransactionOutput * self) {
	
	return CBScriptOutputGetType(self->scriptObject);
	
}

void CBTransactionOutputPrepareBytes(CBTransactionOutput * self) {
	
	CBMessagePrepareBytes(CBGetMessage(self), CBTransactionOutputCalculateLength(self));
	
}

uint32_t CBTransactionOutputSerialise(CBTransactionOutput * self) {
	
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	
	if (! bytes) {
		CBLogError("Attempting to serialise a CBTransactionInput with no bytes.");
		return 0;
	}
	if (! self->scriptObject){
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
	CBByteArraySetVarInt(bytes, 8, scriptLen);
	CBByteArrayCopyByteArray(bytes, 8 + scriptLen.size, CBGetByteArray(self->scriptObject));
	CBByteArrayChangeReference(CBGetByteArray(self->scriptObject), bytes, 8 + scriptLen.size);
	
	// Ensure length is correct
	bytes->length = reqLen;
	
	// Is serialised.
	CBGetMessage(self)->serialised = true;
	
	return reqLen;
	
}
