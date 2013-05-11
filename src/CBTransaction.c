//
//  CBTransaction.c
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

#include "CBTransaction.h"
#include <stdio.h>
#include <assert.h>

//  Constructor

CBTransaction * CBNewTransaction(uint32_t lockTime, uint32_t version){
	CBTransaction * self = malloc(sizeof(*self));
	if (NOT self) {
		CBLogError("Cannot allocate %i bytes of memory in CBNewTransaction\n", sizeof(*self));
		return NULL;
	}
	CBGetObject(self)->free = CBFreeTransaction;
	if(CBInitTransaction(self, lockTime, version))
		return self;
	free(self);
	return NULL;
}
CBTransaction * CBNewTransactionFromData(CBByteArray * bytes){
	CBTransaction * self = malloc(sizeof(*self));
	if (NOT self) {
		CBLogError("Cannot allocate %i bytes of memory in CBNewTransactionFromData\n", sizeof(*self));
		return NULL;
	}
	CBGetObject(self)->free = CBFreeTransaction;
	if(CBInitTransactionFromData(self, bytes))
		return self;
	free(self);
	return NULL;
}

//  Object Getter

CBTransaction * CBGetTransaction(void * self){
	return self;
}

//  Initialiser

bool CBInitTransaction(CBTransaction * self, uint32_t lockTime, uint32_t version){
	self->lockTime = lockTime;
	self->inputNum = 0;
	self->outputNum = 0;
	self->inputs = NULL;
	self->outputs = NULL;
	self->version = version;
	self->hashSet = false;
	if (NOT CBInitMessageByObject(CBGetMessage(self)))
		return false;
	return true;
}
bool CBInitTransactionFromData(CBTransaction * self, CBByteArray * data){
	self->inputNum = 0;
	self->outputNum = 0;
	self->inputs = NULL;
	self->outputs = NULL;
	self->hashSet = false;
	if (NOT CBInitMessageByData(CBGetMessage(self), data))
		return false;
	return true;
}

//  Destructor

void CBFreeTransaction(void * vself){
	CBTransaction * self = vself;
	for (uint32_t x = 0; x < self->inputNum; x++)
		CBReleaseObject(self->inputs[x]);
	free(self->inputs);
	for (uint32_t x = 0; x < self->outputNum; x++)
		CBReleaseObject(self->outputs[x]);
	free(self->outputs);
	CBFreeMessage(CBGetObject(self));
}

//  Functions

bool CBTransactionAddInput(CBTransaction * self, CBTransactionInput * input){
	CBRetainObject(input);
	return CBTransactionTakeInput(self, input);
}
bool CBTransactionAddOutput(CBTransaction * self, CBTransactionOutput * output){
	CBRetainObject(output);
	return CBTransactionTakeOutput(self, output);
}
void CBTransactionCalculateHash(CBTransaction * self, uint8_t * hash){
	uint8_t * data = CBByteArrayGetData(CBGetMessage(self)->bytes);
	uint8_t hash2[32];
	CBSha256(data, CBGetMessage(self)->bytes->length, hash2);
	CBSha256(hash2, 32, hash);
}
uint32_t CBTransactionCalculateLength(CBTransaction * self){
	uint32_t len = 8 + CBVarIntSizeOf(self->inputNum) + CBVarIntSizeOf(self->outputNum); // 8 is for version and lockTime.
	for (uint32_t x = 0; x < self->inputNum; x++)
		len += CBTransactionInputCalculateLength(self->inputs[x]);
	for (uint32_t x = 0; x < self->outputNum; x++)
		len += CBTransactionOutputCalculateLength(self->outputs[x]);
	return len;
}
uint32_t CBTransactionDeserialise(CBTransaction * self){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (NOT bytes) {
		CBLogError("Attempting to deserialise a CBTransaction with no bytes.");
		return 0;
	}
	if (bytes->length < 10) {
		CBLogError("Attempting to deserialise a CBTransaction with less than 10 bytes.");
		return 0;
	}
	self->version = CBByteArrayReadInt32(bytes, 0);
	CBVarInt inputOutputLen = CBVarIntDecode(bytes, 4);
	if (NOT inputOutputLen.val
		|| inputOutputLen.val * 41 > bytes->length - 10) {
		CBLogError("Attempting to deserialise a CBTransaction with a bad var int for the number of inputs.");
		return 0;
	}
	uint32_t cursor = 4 + inputOutputLen.size;
	self->inputNum = (uint32_t)inputOutputLen.val;
	self->inputs = malloc(sizeof(*self->inputs) * self->inputNum);
	if (NOT self->inputs) {
		CBLogError("Cannot allocate %i bytes of memory in CBTransactionDeserialise for inputs.\n", sizeof(*self->inputs) * self->inputNum);
		return 0;
	}
	for (uint32_t x = 0; x < self->inputNum; x++) {
		CBByteArray * data = CBByteArraySubReference(bytes, cursor, bytes->length-cursor);
		if (NOT data) {
			CBLogError("Cannot create a new CBByteArray in CBTransactionDeserialise for the input number %u.", x);
			return 0;
		}
		CBTransactionInput * input = CBNewTransactionInputFromData(data);
		if (NOT input) {
			CBLogError("Cannot create a new CBTransactionInput in CBTransactionDeserialise for the input number %u.", x);
			CBReleaseObject(data);
			return 0;
		}
		uint32_t len = CBTransactionInputDeserialise(input);
		if (NOT len){
			CBLogError("CBTransaction cannot be deserialised because of an error with the input number %u.", x);
			CBReleaseObject(data);
			return 0;
		}
		// The input was deserialised correctly. Now adjust the length and add it to the transaction.
		data->length = len;
		CBReleaseObject(data);
		self->inputs[x] = input;
		cursor += len; // Move along to next input
	}
	if (bytes->length < cursor + 5) { // Needs at least 5 more for the output CBVarInt and the lockTime
		CBLogError("Attempting to deserialise a CBTransaction with not enough bytes for the outputs and lockTime.");
		return 0;
	}
	inputOutputLen = CBVarIntDecode(bytes, cursor);
	if (NOT inputOutputLen.val
		|| inputOutputLen.val * 9 > bytes->length - 10) {
		CBLogError("Attempting to deserialise a CBTransaction with a bad var int for the number of outputs.");
		return 0;
	}
	cursor += inputOutputLen.size; // Move past output CBVarInt
	self->outputNum = (uint32_t)inputOutputLen.val;
	self->outputs = malloc(sizeof(*self->outputs) * self->outputNum);
	if (NOT self->outputs) {
		CBLogError("Cannot allocate %i bytes of memory in CBTransactionDeserialise for outputs.\n", sizeof(sizeof(*self->outputs) * self->outputNum));
		return 0;
	}
	for (uint32_t x = 0; x < self->outputNum; x++) {
		CBByteArray * data = CBByteArraySubReference(bytes, cursor, bytes->length-cursor);
		if (NOT data) {
			CBLogError("Cannot create a new CBByteArray in CBTransactionDeserialise for the output number %u.", x);
			return 0;
		}
		CBTransactionOutput * output = CBNewTransactionOutputFromData(data);
		if (NOT output) {
			CBLogError("Cannot create a new CBTransactionOutput in CBTransactionDeserialise for the output number %u.", x);
			CBReleaseObject(data);
			return 0;
		}
		uint32_t len = CBTransactionOutputDeserialise(output);
		if (NOT len){
			CBLogError("CBTransaction cannot be deserialised because of an error with the output number %u.", x);
			CBReleaseObject(data);
			return 0;
		}
		// The output was deserialised correctly. Now adjust the length and add it to the transaction.
		data->length = len;
		CBReleaseObject(data);
		self->outputs[x] = output;
		cursor += len; // Move along to next output
	}
	if (bytes->length < cursor + 4) { // Ensure 4 bytes are available for lockTime
		CBLogError("Attempting to deserialise a CBTransaction with not enough bytes for the lockTime.");
		return 0;
	}
	self->lockTime = CBByteArrayReadInt32(bytes, cursor);
	return cursor + 4;
}
uint8_t * CBTransactionGetHash(CBTransaction * self){
	if (NOT self->hashSet){
		CBTransactionCalculateHash(self, self->hash);
		self->hashSet = true;
	}
	return self->hash;
}
CBGetHashReturn CBTransactionGetInputHashForSignature(void * vself, CBByteArray * prevOutSubScript, uint32_t input, CBSignType signType, uint8_t * hash){
	CBTransaction * self= vself;
	if (self->inputNum < input + 1) {
		CBLogError("Receiving transaction hash to sign cannot be done for because the input index goes past the number of inputs.");
		return CB_TX_HASH_BAD;
	}
	uint8_t last5Bits = (signType & 0x1f); // For some reason this is what the C++ client does.
        CBVarInt vi = CBVarIntFromUInt64(prevOutSubScript->length);
	uint32_t sizeOfData = 12 + prevOutSubScript->length + vi.size; // Version, lock time and the sign type make up 12 bytes.
	if (signType & CB_SIGHASH_ANYONECANPAY) {
		sizeOfData += 41; // Just this one input. 32 bytes for outPointerHash, 4 for outPointerIndex, 1 for varInt, 4 for sequence
	}else{
		sizeOfData += CBVarIntSizeOf(self->inputNum) + self->inputNum * 40 + (self->inputNum - 1); // All inputs
	}
	if (last5Bits == CB_SIGHASH_NONE){
		sizeOfData++; // Just for the CBVarInt and no outputs.
	}else if ((signType & 0x1f) == CB_SIGHASH_SINGLE){
		if (self->outputNum < input + 1) {
			CBLogError("Receiving transaction hash to sign cannot be done for CB_SIGHASH_SINGLE because there are not enough outputs.");
			return CB_TX_HASH_BAD;
		}
		sizeOfData += CBVarIntSizeOf(input + 1) + input * 9; // For outputs up to the input index
		// The size for the output at the input index.
		uint32_t len = CBGetByteArray(self->outputs[input]->scriptObject)->length;
		sizeOfData += 8 + CBVarIntSizeOf(len) + len;
	}else{ // All outputs. Default to SIGHASH_ALL
		sizeOfData += CBVarIntSizeOf(self->outputNum);
		for (uint32_t x = 0; x < self->outputNum; x++) {
			uint32_t len = CBGetByteArray(self->outputs[x]->scriptObject)->length;
			sizeOfData += 8 + CBVarIntSizeOf(len) + len;
		}
	}
	CBByteArray * data = CBNewByteArrayOfSize(sizeOfData);
	if (NOT data)
		return CB_TX_HASH_ERR;
	CBByteArraySetInt32(data, 0, self->version);
	// Copy input data. Scripts are not copied for the inputs.
	uint32_t cursor;
	if (signType & CB_SIGHASH_ANYONECANPAY) {
		CBVarIntEncode(data, 4, CBVarIntFromUInt64(1)); // Only the input the signature is for.
		CBByteArrayCopyByteArray(data, 5, self->inputs[input]->prevOut.hash);
		CBByteArraySetInt32(data, 37, self->inputs[input]->prevOut.index);
		// Add prevOutSubScript
		CBByteArrayCopyByteArray(data, 41, prevOutSubScript);
		cursor = 41 + prevOutSubScript->length;
		CBByteArraySetInt32(data, cursor, self->inputs[input]->sequence);
		cursor += 4;
	}else{
		CBVarInt inputNum = CBVarIntFromUInt64(self->inputNum);
		CBVarIntEncode(data, 4, inputNum);
		cursor = 4 + inputNum.size;
		for (uint32_t x = 0; x < self->inputNum; x++) {
			CBByteArrayCopyByteArray(data, cursor, self->inputs[x]->prevOut.hash);
			cursor += 32;
			CBByteArraySetInt32(data, cursor, self->inputs[x]->prevOut.index);
			cursor += 4;
			// Add prevOutSubScript if the input is for the signature.
			if (x == input) {
				CBVarIntEncode(data, cursor, vi);
				cursor += vi.size;
				CBByteArrayCopyByteArray(data, cursor, prevOutSubScript);
				cursor += prevOutSubScript->length;
			} else {
                            CBVarIntEncode(data, cursor, CBVarIntFromUInt64(0));
                            cursor++;
                        }
			if ((signType == CB_SIGHASH_NONE || signType == CB_SIGHASH_SINGLE) && x != input) {
				CBByteArraySetInt32(data, cursor, 0);
                        }
			else // SIGHASH_ALL or input index for signing sequence
				CBByteArraySetInt32(data, cursor, self->inputs[x]->sequence);
			cursor += 4;
		}
	}
	// Copy output data
	if (last5Bits == CB_SIGHASH_NONE){
		CBVarInt varInt = CBVarIntFromUInt64(0);
		CBVarIntEncode(data, cursor, varInt);
		cursor++;
	}else if (last5Bits == CB_SIGHASH_SINGLE){
		CBVarInt varInt = CBVarIntFromUInt64(input + 1);
		CBVarIntEncode(data, cursor, varInt);
		cursor += varInt.size;
		for (uint32_t x = 0; x < input; x++) {
			CBByteArraySetInt64(data, cursor, CB_OUTPUT_VALUE_MINUS_ONE);
			cursor += 8;
			CBVarIntEncode(data, cursor, CBVarIntFromUInt64(0));
			cursor++;
		}
		CBByteArraySetInt64(data, cursor, self->outputs[input]->value);
		cursor += 8;
		varInt = CBVarIntFromUInt64(CBGetByteArray(self->outputs[input]->scriptObject)->length);
		CBVarIntEncode(data, cursor, varInt);
		cursor += varInt.size;
		CBByteArrayCopyByteArray(data, cursor, CBGetByteArray(self->outputs[input]->scriptObject));
		cursor += varInt.val;
	}else{ // SIGHASH_ALL
		CBVarInt varInt = CBVarIntFromUInt64(self->outputNum);
		CBVarIntEncode(data, cursor, varInt);
		cursor += varInt.size;
		for (uint32_t x = 0; x < self->outputNum; x++) {
			CBByteArraySetInt64(data, cursor, self->outputs[x]->value);
			cursor += 8;
			varInt = CBVarIntFromUInt64(CBGetByteArray(self->outputs[x]->scriptObject)->length);
			CBVarIntEncode(data, cursor, varInt);
			cursor += varInt.size;
			CBByteArrayCopyByteArray(data, cursor, CBGetByteArray(self->outputs[x]->scriptObject));
			cursor += varInt.val;
		}
	}
	// Set lockTime
	CBByteArraySetInt32(data, cursor, self->lockTime);
	CBByteArraySetInt32(data, cursor + 4, signType);
	assert(sizeOfData == cursor + 8); // Must always be like this
	uint8_t firstHash[32];
	CBSha256(CBByteArrayGetData(data), sizeOfData, firstHash);
	CBSha256(firstHash, 32, hash);
	return CB_TX_HASH_OK;
}
bool CBTransactionIsCoinBase(CBTransaction * self){
	return (self->inputNum == 1
			&& self->inputs[0]->prevOut.index == 0xFFFFFFFF
			&& CBByteArrayIsNull(self->inputs[0]->prevOut.hash));
}
uint32_t CBTransactionSerialise(CBTransaction * self, bool force){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (NOT bytes) {
		CBLogError("Attempting to serialise a CBTransaction with no bytes. Now that you think about it, that was a bit stupid wasn't it?");
		return 0;
	}
	if (NOT self->inputNum || NOT self->outputNum) {
		CBLogError("Attempting to serialise a CBTransaction with an empty output or input list.");
		return 0;
	}
	CBVarInt inputNum = CBVarIntFromUInt64(self->inputNum);
	CBVarInt outputNum = CBVarIntFromUInt64(self->outputNum);
	uint32_t cursor = 4 + inputNum.size;
	if (bytes->length < cursor + 5) {
		CBLogError("Attempting to serialise a CBTransaction with less bytes than required. %i < %i\n", bytes->length, cursor + 5);
		return 0;
	}
	// Serialise data into the CBByteArray and rereference objects to this CBByteArray to save memory.
	CBByteArraySetInt32(bytes, 0, self->version);
	CBVarIntEncode(bytes, 4, inputNum);
	for (uint32_t x = 0; x < self->inputNum; x++) {
		if (NOT CBGetMessage(self->inputs[x])->serialised // Serailise if not serialised yet.
			// Serialise if force is true.
			|| force
			// If the data shares the same data as this transaction, re-serialise the input, in case it got overwritten.
			|| CBGetMessage(self->inputs[x])->bytes->sharedData == bytes->sharedData) {
			if (CBGetMessage(self->inputs[x])->serialised)
				// Release old byte array
				CBReleaseObject(CBGetMessage(self->inputs[x])->bytes);
			CBGetMessage(self->inputs[x])->bytes = CBByteArraySubReference(bytes, cursor, bytes->length-cursor);
			if (NOT CBGetMessage(self->inputs[x])->bytes) {
				CBLogError("Cannot create a new CBByteArray sub reference in CBTransactionSerialise for input number %u", x);
				return 0;
			}
			if (NOT CBTransactionInputSerialise(self->inputs[x])) {
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
	if (bytes->length < cursor + 5) { // Check room for output number and lockTime.
		CBLogError("Attempting to serialise a CBTransaction with less bytes than required for output number and the lockTime. %i < %i", bytes->length, cursor + 5);
		return 0;
	}
	CBVarIntEncode(bytes, cursor, outputNum);
	cursor += outputNum.size;
	for (uint32_t x = 0; x < self->outputNum; x++) {
		if (NOT CBGetMessage(self->outputs[x])->serialised // Serailise if not serialised yet.
			// Serialise if force is true.
			|| force
			// If the data shares the same data as this transaction, re-serialise the output, in case it got overwritten.
			|| CBGetMessage(self->outputs[x])->bytes->sharedData == bytes->sharedData) {
			if (CBGetMessage(self->outputs[x])->serialised)
				// Release old byte array
				CBReleaseObject(CBGetMessage(self->outputs[x])->bytes);
			CBGetMessage(self->outputs[x])->bytes = CBByteArraySubReference(bytes, cursor, bytes->length-cursor);
			if (NOT CBGetMessage(self->outputs[x])->bytes) {
				CBLogError("Cannot create a new CBByteArray sub reference in CBTransactionSerialise for output number %u", x);
				return 0;
			}
			if (NOT CBTransactionOutputSerialise(self->outputs[x])) {
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
	if (bytes->length < cursor + 4) { // Check room for lockTime.
		CBLogError("Attempting to serialise a CBTransaction with less bytes than required for the lockTime. %i < %i\n", bytes->length, cursor + 4);
		// Release CBByteArray objects to avoid problems overwritting pointer without release, if serialisation is tried again.
		for (uint32_t y = 0; y < self->inputNum; y++)
			CBReleaseObject(CBGetMessage(self->inputs[y])->bytes);
		for (uint32_t y = 0; y < self->outputNum; y++)
			CBReleaseObject(CBGetMessage(self->outputs[y])->bytes);
		return 0;
	}
	CBByteArraySetInt32(bytes, cursor, self->lockTime);
	// Ensure length is correct
	bytes->length = cursor + 4;
	// Is serialised
	CBGetMessage(self)->serialised = true;
	// Make the hash not set for this serialisation.
	self->hashSet = false;
	return cursor + 4;
}
bool CBTransactionTakeInput(CBTransaction * self, CBTransactionInput * input){
	self->inputNum++;
	CBTransactionInput ** temp = realloc(self->inputs, sizeof(*self->inputs) * self->inputNum);
	if (NOT temp)
		return false;
	self->inputs = temp;
	self->inputs[self->inputNum-1] = input;
	return true;
}
bool CBTransactionTakeOutput(CBTransaction * self, CBTransactionOutput * output){
	self->outputNum++;
	CBTransactionOutput ** temp = realloc(self->outputs, sizeof(*self->outputs) * self->outputNum);
	if (NOT temp)
		return false;
	self->outputs = temp;
	self->outputs[self->outputNum-1] = output;
	return false;
}
