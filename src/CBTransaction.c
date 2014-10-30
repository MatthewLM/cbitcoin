//
//  CBTransaction.c
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

#include "CBTransaction.h"
#include <stdio.h>
#include <assert.h>

//  Constructor

CBTransaction * CBNewTransaction(uint32_t lockTime, uint32_t version) {
	
	CBTransaction * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeTransaction;
	CBInitTransaction(self, lockTime, version);
	
	return self;
	
}

CBTransaction * CBNewTransactionFromData(CBByteArray * bytes) {
	
	CBTransaction * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeTransaction;
	CBInitTransactionFromData(self, bytes);
	
	return self;
	
}

//  Initialiser

void CBInitTransaction(CBTransaction * self, uint32_t lockTime, uint32_t version) {
	
	self->lockTime = lockTime;
	self->inputNum = 0;
	self->outputNum = 0;
	self->inputs = NULL;
	self->outputs = NULL;
	self->version = version;
	self->hashSet = false;
	
	CBInitMessageByObject(CBGetMessage(self));
	
}
void CBInitTransactionFromData(CBTransaction * self, CBByteArray * data) {
	
	self->inputNum = 0;
	self->outputNum = 0;
	self->inputs = NULL;
	self->outputs = NULL;
	self->hashSet = false;
	
	CBInitMessageByData(CBGetMessage(self), data);
	
}

//  Destructor

void CBDestroyTransaction(void * vself) {
	
	CBTransaction * self = vself;
	
	for (uint32_t x = 0; x < self->inputNum; x++)
		CBReleaseObject(self->inputs[x]);
	free(self->inputs);
	
	for (uint32_t x = 0; x < self->outputNum; x++)
		CBReleaseObject(self->outputs[x]);
	free(self->outputs);
	
	CBDestroyMessage(CBGetObject(self));
	
}

void CBFreeTransaction(void * self) {
	
	CBDestroyTransaction(self);
	free(self);
	
}

//  Functions

void CBTransactionAddInput(CBTransaction * self, CBTransactionInput * input) {
	
	CBRetainObject(input);
	CBTransactionTakeInput(self, input);
	
}

void CBTransactionAddOutput(CBTransaction * self, CBTransactionOutput * output) {
	
	CBRetainObject(output);
	CBTransactionTakeOutput(self, output);
	
}

void CBTransactionAddP2SHScript(CBTransaction * self, CBScript * p2shScript, uint32_t input) {
	
	uint16_t offset = self->inputs[input]->scriptObject->length;
	
	CBScript * inScript = CBNewByteArrayOfSize(offset + p2shScript->length + CBScriptGetLengthOfPushOp(p2shScript->length));
	CBByteArrayCopyByteArray(inScript, 0, self->inputs[input]->scriptObject);
	
	CBReleaseObject(self->inputs[input]->scriptObject);
	self->inputs[input]->scriptObject = inScript;
	
	CBScriptWritePushOp(inScript, offset, CBByteArrayGetData(p2shScript), p2shScript->length);
	
}

uint8_t CBTransactionAddSignature(CBTransaction * self, CBScript * inScript, uint16_t offset, CBKeyPair * key, CBByteArray * prevOutSubScript, uint32_t input, CBSignType signType) {
	
	// Obtain the signature hash
	uint8_t hash[32];
	if (!CBTransactionGetInputHashForSignature(self, prevOutSubScript, input, signType, hash)){
		CBLogError("Unable to obtain a hash for producing an input signature.");
		return 0;
	}
	
	// Now produce the signature and add push data size for signature
	uint8_t len = CBKeySign(key->privkey, hash, CBByteArrayGetData(inScript) + offset + 1);
	CBByteArraySetByte(inScript, offset, len + 1);
	
	// Add the sign type
	CBByteArraySetByte(inScript, offset + len + 1, signType);
	
	// Adjust length
	inScript->length -= CB_MAX_DER_SIG_SIZE - len;
	
	return len + 2;
	
}

void CBTransactionCalculateHash(CBTransaction * self, uint8_t * hash) {
	
	uint8_t * data = CBByteArrayGetData(CBGetMessage(self)->bytes);
	uint8_t hash2[32];
	
	CBSha256(data, CBGetMessage(self)->bytes->length, hash2);
	CBSha256(hash2, 32, hash);
	
}

uint32_t CBTransactionCalculateLength(CBTransaction * self) {
	
	// 8 is for version and lockTime.
	uint32_t len = 8 + CBVarIntSizeOf(self->inputNum) + CBVarIntSizeOf(self->outputNum);
	
	for (uint32_t x = 0; x < self->inputNum; x++)
		len += CBTransactionInputCalculateLength(self->inputs[x]);
	
	for (uint32_t x = 0; x < self->outputNum; x++)
		len += CBTransactionOutputCalculateLength(self->outputs[x]);
	
	return len;
	
}

uint32_t CBTransactionDeserialise(CBTransaction * self) {
	
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (! bytes) {
		CBLogError("Attempting to deserialise a CBTransaction with no bytes.");
		return CB_DESERIALISE_ERROR;
	}
	if (bytes->length < 10) {
		CBLogError("Attempting to deserialise a CBTransaction with less than 10 bytes.");
		return CB_DESERIALISE_ERROR;
	}
	
	self->version = CBByteArrayReadInt32(bytes, 0);
	
	CBVarInt inputOutputLen = CBByteArrayReadVarInt(bytes, 4);
	if (! inputOutputLen.val
		|| inputOutputLen.val * 41 > bytes->length - 10) {
		CBLogError("Attempting to deserialise a CBTransaction with a bad var int for the number of inputs.");
		return CB_DESERIALISE_ERROR;
	}
	
	uint32_t cursor = 4 + inputOutputLen.size;
	
	self->inputNum = (uint32_t)inputOutputLen.val;
	self->inputs = malloc(sizeof(*self->inputs) * self->inputNum);
	
	for (uint32_t x = 0; x < self->inputNum; x++) {
		
		CBByteArray * data = CBByteArraySubReference(bytes, cursor, bytes->length-cursor);
		CBTransactionInput * input = CBNewTransactionInputFromData(data);
		uint32_t len = CBTransactionInputDeserialise(input);
		if (len == CB_DESERIALISE_ERROR){
			CBLogError("CBTransaction cannot be deserialised because of an error with the input number %u.", x);
			CBReleaseObject(data);
			return CB_DESERIALISE_ERROR;
		}
		
		// The input was deserialised correctly. Now adjust the length and add it to the transaction.
		data->length = len;
		CBReleaseObject(data);
		self->inputs[x] = input;
		
		// Move along to next input
		cursor += len;
	}
	
	// Needs at least 5 more for the output CBVarInt and the lockTime
	if (bytes->length < cursor + 5) {
		CBLogError("Attempting to deserialise a CBTransaction with not enough bytes for the outputs and lockTime.");
		return CB_DESERIALISE_ERROR;
	}
	
	inputOutputLen = CBByteArrayReadVarInt(bytes, cursor);
	if (inputOutputLen.val == 0
		|| inputOutputLen.val * 9 > bytes->length - 10) {
		CBLogError("Attempting to deserialise a CBTransaction with a bad var int for the number of outputs.");
		return CB_DESERIALISE_ERROR;
	}
	
	// Move past output CBVarInt
	cursor += inputOutputLen.size;
	
	self->outputNum = (uint32_t)inputOutputLen.val;
	self->outputs = malloc(sizeof(*self->outputs) * self->outputNum);
	
	for (uint32_t x = 0; x < self->outputNum; x++) {
		
		CBByteArray * data = CBByteArraySubReference(bytes, cursor, bytes->length-cursor);
		CBTransactionOutput * output = CBNewTransactionOutputFromData(data);
		uint32_t len = CBTransactionOutputDeserialise(output);
		if (len == CB_DESERIALISE_ERROR){
			CBLogError("CBTransaction cannot be deserialised because of an error with the output number %u.", x);
			CBReleaseObject(data);
			return CB_DESERIALISE_ERROR;
		}
		
		// The output was deserialised correctly. Now adjust the length and add it to the transaction.
		data->length = len;
		CBReleaseObject(data);
		self->outputs[x] = output;
		
		// Move along to next output
		cursor += len; 
	}
	
	// Ensure 4 bytes are available for lockTime
	if (bytes->length < cursor + 4) {
		CBLogError("Attempting to deserialise a CBTransaction with not enough bytes for the lockTime.");
		return CB_DESERIALISE_ERROR;
	}
	
	self->lockTime = CBByteArrayReadInt32(bytes, cursor);
	
	return cursor + 4;
	
}

uint8_t * CBTransactionGetHash(CBTransaction * self) {
	
	if (! self->hashSet){
		CBTransactionCalculateHash(self, self->hash);
		self->hashSet = true;
	}
	
	return self->hash;
	
}

bool CBTransactionGetInputHashForSignature(void * vself, CBByteArray * prevOutSubScript, uint32_t input, CBSignType signType, uint8_t * hash) {
	
	uint32_t cursor;
	CBTransaction * self = vself;
	
	if (self->inputNum < input + 1)
		return false;
	
	uint8_t last5Bits = (signType & 0x1f);
	
	CBVarInt prevOutputSubScriptVarInt = CBVarIntFromUInt64(prevOutSubScript->length);
	
	// Version, lock time and the sign type make up 12 bytes.
	uint32_t sizeOfData = 12 + prevOutSubScript->length + prevOutputSubScriptVarInt.size;
	
	if (signType & CB_SIGHASH_ANYONECANPAY)
		// Just this one input. 32 bytes for outPointerHash, 4 for outPointerIndex, 4 for sequence and one for the *inputNum* var int
		sizeOfData += 41;
	else
		// All inputs with 1 byte var int except one.
		sizeOfData += CBVarIntSizeOf(self->inputNum) + self->inputNum * 41 - 1;
	
	if (last5Bits == CB_SIGHASH_NONE)
		// Just for the CBVarInt and no outputs.
		sizeOfData++;
	
	else if ((signType & 0x1f) == CB_SIGHASH_SINGLE) {
		
		if (self->outputNum < input + 1)
			return false;
		
		// For outputs up to the input index
		sizeOfData += CBVarIntSizeOf(input + 1) + input * 9;
		
		// The size for the output at the input index.
		uint32_t len = CBGetByteArray(self->outputs[input]->scriptObject)->length;
		sizeOfData += 8 + CBVarIntSizeOf(len) + len;
		
	}else{
		
		// All outputs. Default to SIGHASH_ALL
		sizeOfData += CBVarIntSizeOf(self->outputNum);
		
		for (uint32_t x = 0; x < self->outputNum; x++) {
			uint32_t len = CBGetByteArray(self->outputs[x]->scriptObject)->length;
			sizeOfData += 8 + CBVarIntSizeOf(len) + len;
		}
		
	}
	
	CBByteArray * data = CBNewByteArrayOfSize(sizeOfData);
	
	CBByteArraySetInt32(data, 0, self->version);
	
	// Copy input data. Scripts are not copied for the inputs.
	if (signType & CB_SIGHASH_ANYONECANPAY) {
		
		CBByteArraySetVarInt(data, 4, CBVarIntFromUInt64(1)); // Only the input the signature is for.
		CBByteArrayCopyByteArray(data, 5, self->inputs[input]->prevOut.hash);
		CBByteArraySetInt32(data, 37, self->inputs[input]->prevOut.index);
		
		// Add prevOutSubScript
		CBByteArraySetVarInt(data, 41, prevOutputSubScriptVarInt);
		cursor = 41 + prevOutputSubScriptVarInt.size;
		CBByteArrayCopyByteArray(data, cursor, prevOutSubScript);
		cursor += prevOutSubScript->length;
		CBByteArraySetInt32(data, cursor, self->inputs[input]->sequence);
		cursor += 4;
		
	}else{
		
		CBVarInt inputNum = CBVarIntFromUInt64(self->inputNum);
		CBByteArraySetVarInt(data, 4, inputNum);
		cursor = 4 + inputNum.size;
		
		for (uint32_t x = 0; x < self->inputNum; x++) {
			
			CBByteArrayCopyByteArray(data, cursor, self->inputs[x]->prevOut.hash);
			cursor += 32;
			CBByteArraySetInt32(data, cursor, self->inputs[x]->prevOut.index);
			cursor += 4;
			
			// Add prevOutSubScript if the input is for the signature.
			if (x == input) {
				CBByteArraySetVarInt(data, cursor, prevOutputSubScriptVarInt);
				cursor += prevOutputSubScriptVarInt.size;
				CBByteArrayCopyByteArray(data, cursor, prevOutSubScript);
				cursor += prevOutSubScript->length;
			}else{
				CBByteArraySetVarInt(data, cursor, CBVarIntFromUInt64(0));
				cursor++;
			}
			
			if ((signType == CB_SIGHASH_NONE || signType == CB_SIGHASH_SINGLE) && x != input)
				CBByteArraySetInt32(data, cursor, 0);
			else
				// SIGHASH_ALL or input index for signing sequence
				CBByteArraySetInt32(data, cursor, self->inputs[x]->sequence);
			cursor += 4;
		}
	}
	
	// Copy output data
	if (last5Bits == CB_SIGHASH_NONE) {
		
		CBVarInt varInt = CBVarIntFromUInt64(0);
		CBByteArraySetVarInt(data, cursor, varInt);
		cursor++;
		
	}else if (last5Bits == CB_SIGHASH_SINGLE) {
		
		CBVarInt varInt = CBVarIntFromUInt64(input + 1);
		CBByteArraySetVarInt(data, cursor, varInt);
		cursor += varInt.size;
		
		for (uint32_t x = 0; x < input; x++) {
			CBByteArraySetInt64(data, cursor, CB_OUTPUT_VALUE_MINUS_ONE);
			cursor += 8;
			CBByteArraySetVarInt(data, cursor, CBVarIntFromUInt64(0));
			cursor++;
		}
		
		CBByteArraySetInt64(data, cursor, self->outputs[input]->value);
		cursor += 8;
		varInt = CBVarIntFromUInt64(CBGetByteArray(self->outputs[input]->scriptObject)->length);
		CBByteArraySetVarInt(data, cursor, varInt);
		cursor += varInt.size;
		CBByteArrayCopyByteArray(data, cursor, CBGetByteArray(self->outputs[input]->scriptObject));
		cursor += varInt.val;
		
	}else{ // SIGHASH_ALL
		
		CBVarInt varInt = CBVarIntFromUInt64(self->outputNum);
		CBByteArraySetVarInt(data, cursor, varInt);
		cursor += varInt.size;
		
		for (uint32_t x = 0; x < self->outputNum; x++) {
			CBByteArraySetInt64(data, cursor, self->outputs[x]->value);
			cursor += 8;
			varInt = CBVarIntFromUInt64(CBGetByteArray(self->outputs[x]->scriptObject)->length);
			CBByteArraySetVarInt(data, cursor, varInt);
			cursor += varInt.size;
			CBByteArrayCopyByteArray(data, cursor, CBGetByteArray(self->outputs[x]->scriptObject));
			cursor += varInt.val;
		}
		
	}
	
	// Set lockTime and sign type
	CBByteArraySetInt32(data, cursor, self->lockTime);
	CBByteArraySetInt32(data, cursor + 4, signType);
	
	// Must always be like this
	assert(sizeOfData == cursor + 8);
	
	// Now hash the data
	uint8_t firstHash[32];
	CBSha256(CBByteArrayGetData(data), sizeOfData, firstHash);
	CBSha256(firstHash, 32, hash);
	CBReleaseObject(data);
	
	return true;
	
}

void CBTransactionHashToString(CBTransaction * self, char output[CB_TX_HASH_STR_SIZE]) {
	
	uint8_t * hash = CBTransactionGetHash(self);
	for (uint8_t x = 0; x < 20; x++)
		sprintf(output + x*2, "%02x", hash[x]);
	output[40] = '\0';
	
}

bool CBTransactionInputIsStandard(CBScript * inputScript, CBScript * outputScript, CBScript * p2sh) {
	
	// Get the number of stack items
	uint16_t num = CBScriptIsPushOnly(inputScript);
	
	if (p2sh) {
		
		if (CBScriptIsKeyHash(p2sh)) {
			if (num != 3)
				return false;
			
		}else if (CBScriptIsMultisig(p2sh)){
			if (num - 1 != CBScriptOpGetNumber(CBByteArrayGetByte(p2sh, 0)))
				return false;
			
		}else if (CBScriptIsPubkey(p2sh)){
			if (num != 2)
				return false;
			
		}else
			return false;
		
		return true;
		
	}
	
	CBScriptOutputType type = CBScriptOutputGetType(outputScript);
	if (type == CB_TX_OUTPUT_TYPE_UNKNOWN)
		return false;
	
	if (type == CB_TX_OUTPUT_TYPE_KEYHASH) {
		if (num != 2)
			return false;
		
	}else if (type == CB_TX_OUTPUT_TYPE_PUBKEY) {
		if (num != 1)
			return false;
		
	}else if (num != CBScriptOpGetNumber(CBByteArrayGetByte(outputScript, 0)))
		return false;
	
	return true;
	
}

bool CBTransactionIsCoinBase(CBTransaction * self) {
	
	return (self->inputNum == 1
			&& self->inputs[0]->prevOut.index == 0xFFFFFFFF
			&& CBByteArrayIsNull(self->inputs[0]->prevOut.hash));
	
}

bool CBTransactionIsStandard(CBTransaction * self) {
	
	if (self->version > CB_TX_MAX_STANDARD_VERSION)
		return false;
	
	if (CBTransactionCalculateLength(self) > CB_TX_MAX_STANDARD_SIZE)
		return false;
	
	for (uint32_t x = 0; x < self->inputNum; x++) {
		if (self->inputs[x]->scriptObject->length > 500)
			return false;
		if (CBScriptIsPushOnly(self->inputs[x]->scriptObject) == 0)
			return false;
	}
	
	for (uint32_t x = 0; x < self->outputNum; x++) {
		
		CBScriptOutputType type = CBTransactionOutputGetType(self->outputs[x]);
		if (type == CB_TX_OUTPUT_TYPE_UNKNOWN)
			return false;
		
		if (type == CB_TX_OUTPUT_TYPE_MULTISIG) {
			uint8_t pubKeyNum = CBScriptOpGetNumber(CBByteArrayGetByte(self->outputs[x]->scriptObject, self->outputs[x]->scriptObject->length - 2));
			if (pubKeyNum > 3)
				// Only allow upto m of 3 multisig.
				return false;
		}
		
	}
	
	return true;
	
}

void CBTransactionPrepareBytes(CBTransaction * self) {
	
	CBMessagePrepareBytes(CBGetMessage(self), CBTransactionCalculateLength(self));
	
}

uint32_t CBTransactionSerialise(CBTransaction * self, bool force) {
	
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (! bytes || bytes->length < CBTransactionCalculateLength(self)) {
		CBLogError("Attempting to serialise a CBTransaction with no bytes or not enough bytes.");
		return 0;
	}
	
	if (! self->inputNum || ! self->outputNum) {
		CBLogError("Attempting to serialise a CBTransaction with an empty output or input list.");
		return 0;
	}
	
	CBVarInt inputNum = CBVarIntFromUInt64(self->inputNum);
	CBVarInt outputNum = CBVarIntFromUInt64(self->outputNum);
	
	uint32_t cursor = 4 + inputNum.size;
	
	// Serialise data into the CBByteArray and rereference objects to this CBByteArray to save memory.
	
	CBByteArraySetInt32(bytes, 0, self->version);
	CBByteArraySetVarInt(bytes, 4, inputNum);
	
	for (uint32_t x = 0; x < self->inputNum; x++) {
		
		if (! CBGetMessage(self->inputs[x])->serialised // Serialise if not serialised yet.
			// Serialise if force is true.
			|| force
			// If the data shares the same data as this transaction, re-serialise the input, in case it got overwritten.
			|| CBGetMessage(self->inputs[x])->bytes->sharedData == bytes->sharedData) {
			
			if (CBGetMessage(self->inputs[x])->serialised)
				// Release old byte array
				CBReleaseObject(CBGetMessage(self->inputs[x])->bytes);
			
			CBGetMessage(self->inputs[x])->bytes = CBByteArraySubReference(bytes, cursor, bytes->length-cursor);

			if (! CBTransactionInputSerialise(self->inputs[x])) {
				
				CBLogError("CBTransaction cannot be serialised because of an error with the input number %u.", x);
				
				// Release CBByteArray objects to avoid problems overwritting pointer without release, if serialisation is tried again.
				for (uint32_t y = 0; y < x + 1; y++)
					CBReleaseObject(CBGetMessage(self->inputs[y])->bytes);
				
				return 0;
				
			}
			
		}else{
			
			// Move serialsed data to one location
			CBByteArrayCopyByteArray(bytes, cursor, CBGetMessage(self->inputs[x])->bytes);
			CBByteArrayChangeReference(CBGetMessage(self->inputs[x])->bytes, bytes, cursor);
			
		}
		
		cursor += CBGetMessage(self->inputs[x])->bytes->length;
	}
	
	CBByteArraySetVarInt(bytes, cursor, outputNum);
	cursor += outputNum.size;
	
	for (uint32_t x = 0; x < self->outputNum; x++) {
		
		if (! CBGetMessage(self->outputs[x])->serialised // Serailise if not serialised yet.
			// Serialise if force is true.
			|| force
			// If the data shares the same data as this transaction, re-serialise the output, in case it got overwritten.
			|| CBGetMessage(self->outputs[x])->bytes->sharedData == bytes->sharedData) {
			
			if (CBGetMessage(self->outputs[x])->serialised)
				// Release old byte array
				CBReleaseObject(CBGetMessage(self->outputs[x])->bytes);
			
			CBGetMessage(self->outputs[x])->bytes = CBByteArraySubReference(bytes, cursor, bytes->length-cursor);

			if (! CBTransactionOutputSerialise(self->outputs[x])) {
				
				CBLogError("CBTransaction cannot be serialised because of an error with the output number %u.", x);
				
				// Release CBByteArray objects to avoid problems overwritting pointer without release, if serialisation is tried again.
				for (uint32_t y = 0; y < self->inputNum; y++)
					CBReleaseObject(CBGetMessage(self->inputs[y])->bytes);
				for (uint32_t y = 0; y < x + 1; y++)
					CBReleaseObject(CBGetMessage(self->outputs[y])->bytes);
				
				return 0;
				
			}
			
		}else{
			
			// Move serialsed data to one location
			CBByteArrayCopyByteArray(bytes, cursor, CBGetMessage(self->outputs[x])->bytes);
			CBByteArrayChangeReference(CBGetMessage(self->outputs[x])->bytes, bytes, cursor);
			
		}
		
		cursor += CBGetMessage(self->outputs[x])->bytes->length;
	}

	// Set locktime
	CBByteArraySetInt32(bytes, cursor, self->lockTime);
	
	// Ensure length is correct
	bytes->length = cursor + 4;
	
	// Is serialised
	CBGetMessage(self)->serialised = true;
	
	// Make the hash not set for this serialisation.
	self->hashSet = false;
	
	return cursor + 4;
	
}

bool CBTransactionSignMultisigInput(CBTransaction * self, CBKeyPair * key, CBByteArray * prevOutSubScript, uint32_t input, CBSignType signType) {
	
	CBScript * inScript;
	uint16_t offset;
	
	if (self->inputs[input]->scriptObject) {
		
		offset = self->inputs[input]->scriptObject->length;
		inScript = CBNewByteArrayOfSize(offset + CB_MAX_DER_SIG_SIZE + 2);
		CBByteArrayCopyByteArray(inScript, 0, self->inputs[input]->scriptObject);
		CBReleaseObject(self->inputs[input]->scriptObject);
		self->inputs[input]->scriptObject = inScript;
		
	}else{
		
		inScript = self->inputs[input]->scriptObject = CBNewScriptOfSize(CB_MAX_DER_SIG_SIZE + 3);
		CBByteArraySetByte(inScript, 0, CB_SCRIPT_OP_0);
		offset = 1;
		
	}
	
	return CBTransactionAddSignature(self, inScript, offset, key, prevOutSubScript, input, signType);
	
}

bool CBTransactionSignPubKeyHashInput(CBTransaction * self, CBKeyPair * key, CBByteArray * prevOutSubScript, uint32_t input, CBSignType signType) {
	
	// Initilialise script data
	self->inputs[input]->scriptObject = CBNewScriptOfSize(CB_PUBKEY_SIZE + CB_MAX_DER_SIG_SIZE + 3);
	
	// Add the signature.
	uint8_t sigLen = CBTransactionAddSignature(self, self->inputs[input]->scriptObject, 0, key, prevOutSubScript, input, signType);
	if (!sigLen){
		CBLogError("Unable to add a signature ot a pubkey hash transaction input.");
		return false;
	}
	
	// Add the public key
	CBByteArraySetByte(self->inputs[input]->scriptObject, sigLen, CB_PUBKEY_SIZE);
	memcpy(CBByteArrayGetData(self->inputs[input]->scriptObject) + sigLen + 1, key->pubkey.key, CB_PUBKEY_SIZE);
	
	return true;
	
}

bool CBTransactionSignPubKeyInput(CBTransaction * self, CBKeyPair * key, CBByteArray * prevOutSubScript, uint32_t input, CBSignType signType) {
	
	// Initilialise script data
	self->inputs[input]->scriptObject = CBNewScriptOfSize(CB_MAX_DER_SIG_SIZE + 2);
	
	// Add the signature.
	if (!CBTransactionAddSignature(self, self->inputs[input]->scriptObject, 0, key, prevOutSubScript, input, signType)){
		CBLogError("Unable to add a signature ot a pubkey hash transaction input.");
		return false;
	}
	
	return true;
	
}

void CBTransactionTakeInput(CBTransaction * self, CBTransactionInput * input) {
	
	self->inputs = realloc(self->inputs, sizeof(*self->inputs) * ++self->inputNum);
	self->inputs[self->inputNum - 1] = input;
	
}

void CBTransactionTakeOutput(CBTransaction * self, CBTransactionOutput * output) {
	
	self->outputs = realloc(self->outputs, sizeof(*self->outputs) * ++self->outputNum);
	self->outputs[self->outputNum - 1] = output;
}
