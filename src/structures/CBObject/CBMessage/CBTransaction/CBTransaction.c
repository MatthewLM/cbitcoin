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

CBTransaction * CBNewTransaction(u_int32_t lockTime, u_int32_t version, CBEvents * events){
	CBTransaction * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeTransaction;
	CBInitTransaction(self, lockTime, version, events);
	return self;
}
CBTransaction * CBNewTransactionFromData(CBByteArray * bytes, CBEvents * events){
	CBTransaction * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeTransaction;
	CBInitTransactionFromData(self, bytes, events);
	return self;
}

//  Object Getter

CBTransaction * CBGetTransaction(void * self){
	return self;
}

//  Initialiser

bool CBInitTransaction(CBTransaction * self, u_int32_t lockTime, u_int32_t version, CBEvents * events){
	self->lockTime = lockTime;
	self->inputNum = 0;
	self->outputNum = 0;
	self->inputs = NULL;
	self->outputs = NULL;
	self->version = version;
	if (!CBInitMessageByObject(CBGetMessage(self), events))
		return false;
	return true;
}
bool CBInitTransactionFromData(CBTransaction * self, CBByteArray * data,CBEvents * events){
	self->inputNum = 0;
	self->outputNum = 0;
	self->inputs = NULL;
	self->outputs = NULL;
	if (!CBInitMessageByData(CBGetMessage(self), data, events))
		return false;
	return true;
}

//  Destructor

void CBFreeTransaction(void * vself){
	CBTransaction * self = vself;
	for (u_int32_t x = 0; x < self->inputNum; x++)
		CBReleaseObject(&self->inputs[x]);
	free(self->inputs);
	for (u_int32_t x = 0; x < self->outputNum; x++)
		CBReleaseObject(&self->outputs[x]);
	free(self->outputs);
	CBFreeMessage(CBGetObject(self));
}

//  Functions

void CBTransactionAddInput(CBTransaction * self, CBTransactionInput * input){
	CBTransactionTakeInput(self, input);
	CBRetainObject(input);
}
void CBTransactionAddOutput(CBTransaction * self, CBTransactionOutput * output){
	CBTransactionTakeOutput(self, output);
	CBRetainObject(output);
}
u_int32_t CBTransactionDeserialise(CBTransaction * self){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (!bytes) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_NULL_BYTES,"Attempting to deserialise a CBTransaction with no bytes.");
		return 0;
	}
	if (bytes->length < 10) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBTransaction with less than 10 bytes.");
		return 0;
	}
	self->version = CBByteArrayReadInt32(bytes, 0);
	CBVarInt inputOutputLen = CBVarIntDecode(bytes, 4);
	if (inputOutputLen.val*41 > bytes->length-10) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBTransaction with a bad var int for the number of inputs.");
		return 0;
	}
	u_int32_t cursor = 4 + inputOutputLen.size;
	self->inputNum = (u_int32_t)inputOutputLen.val;
	self->inputs = malloc(sizeof(*self->inputs) * self->inputNum);
	for (u_int32_t x = 0; x < self->inputNum; x++) {
		CBByteArray * data = CBByteArraySubReference(bytes, cursor, bytes->length-cursor);
		CBTransactionInput * input = CBNewTransactionInputFromData(data, CBGetMessage(self)->events);
		u_int32_t len = CBTransactionInputDeserialise(input);
		if (!len) {
			CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"CBTransaction cannot be deserialised because of an error with the input number %u.",x);
			// Free data
			CBReleaseObject(&input);
			for (u_int8_t y = 0; y < x; y++) {
				CBReleaseObject(&self->inputs[y]);
			}
			free(self->inputs);
			self->inputs = NULL;
			self->inputNum = 0;
			return 0;
		}
		// The input was deserialised correctly. Now adjust the length and add it to the transaction.
		data->length = len;
		CBReleaseObject(&data);
		self->inputs[x] = input;
		cursor += len; // Move along to next input
	}
	if (bytes->length < cursor + 5) { // Needs at least 5 more for the output CBVarInt and the lockTime
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBTransaction with not enough bytes for the outputs and lockTime.");
		for (u_int8_t y = 0; y < self->inputNum; y++) {
			CBReleaseObject(&self->inputs[y]);
		}
		free(self->inputs);
		self->inputs = NULL;
		self->inputNum = 0;
		return 0;
	}
	inputOutputLen = CBVarIntDecode(bytes, cursor);
	if (inputOutputLen.val*9 > bytes->length-10) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBTransaction with a bad var int for the number of outputs.");
		for (u_int8_t y = 0; y < self->inputNum; y++) {
			CBReleaseObject(&self->inputs[y]);
		}
		free(self->inputs);
		self->inputs = NULL;
		self->inputNum = 0;
		return 0;
	}
	cursor += inputOutputLen.size; // Move past output CBVarInt
	self->outputNum = (u_int32_t)inputOutputLen.val;
	self->outputs = malloc(sizeof(*self->outputs) * self->outputNum);
	for (u_int32_t x = 0; x < self->outputNum; x++) {
		CBByteArray * data = CBByteArraySubReference(bytes, cursor, bytes->length-cursor);
		CBTransactionOutput * output = CBNewTransactionOutputFromData(data, CBGetMessage(self)->events);
		u_int32_t len = CBTransactionOutputDeserialise(output);
		if (!len) {
			CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"CBTransaction cannot be deserialised because of an error with the output number %u.",x);
			// Free data
			CBReleaseObject(output);
			for (u_int8_t y = 0; y < self->inputNum; y++) {
				CBReleaseObject(&self->inputs[y]);
			}
			free(self->inputs);
			self->inputs = NULL;
			self->inputNum = 0;
			for (u_int8_t y = 0; y < x; y++) {
				CBReleaseObject(&self->outputs[y]);
			}
			free(self->outputs);
			self->outputs = NULL;
			self->outputNum = 0;
			return 0;
		}
		// The output was deserialised correctly. Now adjust the length and add it to the transaction.
		data->length = len;
		CBReleaseObject(&data);
		self->outputs[x] = output;
		cursor += len; // Move along to next output
	}
	if (bytes->length < cursor + 4) { // Ensure 4 bytes are available for lockTime
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBTransaction with not enough bytes for the lockTime.");
		for (u_int8_t y = 0; y < self->inputNum; y++) {
			CBReleaseObject(&self->inputs[y]);
		}
		self->inputs = NULL;
		self->inputNum = 0;
		for (u_int8_t y = 0; y < self->outputNum; y++) {
			CBReleaseObject(&self->outputs[y]);
		}
		self->outputs = NULL;
		self->outputNum = 0;
		return 0;
	}
	self->lockTime = CBByteArrayReadInt32(bytes, cursor);
	return cursor + 4;
}
u_int8_t * CBTransactionGetInputHashForSignature(CBTransaction * self, CBByteArray * prevOutSubScript, u_int32_t input, CBSignType signType){
	if (self->inputNum < input + 1) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_TRANSACTION_FEW_INPUTS,"Receiving transaction hash to sign cannot be done for because the input index goes past the number of inputs.");
		return NULL;
	}
	u_int8_t last5Bits = (signType & 0x1f); // For some reason this is what the C++ client does.
	u_int32_t sizeOfData = 12 + prevOutSubScript->length; // Version, lock time and the sign type make up 12 bytes.
	if (signType & CB_SIGHASH_ANYONECANPAY) {
		sizeOfData += 41; // Just this one input. 32 bytes for outPointerHash, 4 for outPointerIndex, 1 for varInt, 4 for sequence
	}else{
		sizeOfData += CBVarIntSizeOf(self->inputNum) + self->inputNum * 40; // All inputs
	}
	if (last5Bits == CB_SIGHASH_NONE){
		sizeOfData++; // Just for the CBVarInt and no outputs.
	}else if ((signType & 0x1f) == CB_SIGHASH_SINGLE){
		if (self->outputNum < input + 1) {
			CBGetMessage(self)->events->onErrorReceived(CB_ERROR_TRANSACTION_FEW_OUTPUTS,"Receiving transaction hash to sign cannot be done for CB_SIGHASH_SINGLE because there are not enough outputs.");
			return NULL;
		}
		sizeOfData += CBVarIntSizeOf(input + 1) + input * 9; // For outputs up to the input index
		// The size for the output at the input index.
		u_int32_t len = CBGetByteArray(self->outputs[input]->scriptObject)->length;
		sizeOfData += 8 + CBVarIntSizeOf(len) + len;
	}else{ // All outputs. Default to SIGHASH_ALL
		sizeOfData += CBVarIntSizeOf(self->outputNum);
		for (u_int32_t x = 0; x < self->outputNum; x++) {
			u_int32_t len = CBGetByteArray(self->outputs[x]->scriptObject)->length;
			sizeOfData += 8 + CBVarIntSizeOf(len) + len;
		}
	}
	CBByteArray * data = CBNewByteArrayOfSize(sizeOfData, CBGetMessage(self)->events);
	CBByteArraySetInt32(data, 0, self->version);
	// Copy input data. Scripts are not copied for the inputs.
	u_int32_t cursor;
	if (signType & CB_SIGHASH_ANYONECANPAY) {
		CBVarIntEncode(data, 4, CBVarIntFromUInt64(1)); // Only the input the signature is for.
		CBByteArrayCopyByteArray(data, 5, self->inputs[input]->outPointerHash);
		CBByteArraySetInt32(data, 37, self->inputs[input]->outPointerIndex);
		// Add prevOutSubScript
		CBByteArrayCopyByteArray(data, 41, prevOutSubScript);
		cursor = 41 + prevOutSubScript->length;
		CBByteArraySetInt32(data, cursor, self->inputs[input]->sequence);
		cursor += 4;
	}else{
		CBVarInt inputNum = CBVarIntFromUInt64(self->inputNum);
		CBVarIntEncode(data, 4, inputNum);
		cursor = 4 + inputNum.size;
		for (u_int32_t x = 0; x < self->inputNum; x++) {
			CBByteArrayCopyByteArray(data, cursor, self->inputs[x]->outPointerHash);
			cursor += 32;
			CBByteArraySetInt32(data, cursor, self->inputs[x]->outPointerIndex);
			cursor += 4;
			// Add prevOutSubScript if the input is for the signature.
			if (x == input) {
				CBByteArrayCopyByteArray(data, cursor, prevOutSubScript);
				cursor += prevOutSubScript->length;
			}
			if ((signType == CB_SIGHASH_NONE || signType == CB_SIGHASH_SINGLE) && x != input)
				CBByteArraySetInt32(data, cursor, 0);
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
		for (u_int32_t x = 0; x < input; x++) {
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
		for (u_int32_t x = 0; x < self->outputNum; x++) {
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
	u_int8_t * firstHash = CBSha256(CBByteArrayGetData(data),sizeOfData);
	u_int8_t * secondHash = CBSha256(firstHash,32);
	free(firstHash);
	return secondHash;
}
u_int32_t CBTransactionSerialise(CBTransaction * self){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (!bytes) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_SERIALISATION_NULL_BYTES,"Attempting to serialise a CBTransaction with no bytes. Now that you think about it, that was a bit stupid wasn't it?");
		return 0;
	}
	CBVarInt inputNum = CBVarIntFromUInt64(self->inputNum);
	CBVarInt outputNum = CBVarIntFromUInt64(self->outputNum);
	u_int32_t cursor = 4 + inputNum.size;
	if (bytes->length < cursor + 5) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_SERIALISATION_BAD_BYTES,"Attempting to serialise a CBTransaction with less bytes than required. %i < %i\n",bytes->length, cursor + 5);
		return 0;
	}
	// Serialise data into the CBByteArray and rereference objects to this CBByteArray to save memory.
	CBByteArraySetInt32(bytes, 0, self->version);
	CBVarIntEncode(bytes, 4, inputNum);
	for (u_int32_t x = 0; x < self->inputNum; x++) {
		CBGetMessage(self->inputs[x])->bytes = CBByteArraySubReference(bytes, cursor, bytes->length-cursor);
		u_int32_t len = CBTransactionInputSerialise(self->inputs[x]);
		if (!len) {
			CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"CBTransaction cannot be serialised because of an error with the input number %u.",x);
			// Release CBByteArray objects to avoid problems overwritting pointer without release, if serialisation is tried again.
			for (u_int32_t y = 0; y < x + 1; y++) {
				CBReleaseObject(&CBGetMessage(self->inputs[y])->bytes);
			}
			return 0;
		}
		CBGetMessage(self->inputs[x])->bytes->length = len;
		cursor += len;
	}
	if (bytes->length < cursor + 5) { // Check room for output number and lockTime.
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to serialise a CBTransaction with less bytes than required for output number and the lockTime. %i < %i\n",bytes->length, cursor + 5);
		return 0;
	}
	CBVarIntEncode(bytes, cursor, outputNum);
	cursor += outputNum.size;
	for (u_int32_t x = 0; x < self->outputNum; x++) {
		CBGetMessage(self->outputs[x])->bytes = CBByteArraySubReference(bytes, cursor, bytes->length-cursor);
		u_int32_t len = CBTransactionOutputSerialise(self->outputs[x]);
		if (!len) {
			CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"CBTransaction cannot be serialised because of an error with the output number %u.",x);
			// Release CBByteArray objects to avoid problems overwritting pointer without release, if serialisation is tried again.
			for (u_int32_t y = 0; y < self->inputNum; y++) {
				CBReleaseObject(&CBGetMessage(self->inputs[y])->bytes);
			}
			for (u_int32_t y = 0; y < x + 1; y++) {
				CBReleaseObject(&CBGetMessage(self->outputs[y])->bytes);
			}
			return 0;
		}
		CBGetMessage(self->outputs[x])->bytes->length = len;
		cursor += len;
	}
	if (bytes->length < cursor + 4) { // Check room for lockTime.
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to serialise a CBTransaction with less bytes than required for the lockTime. %i < %i\n",bytes->length, cursor + 4);
		// Release CBByteArray objects to avoid problems overwritting pointer without release, if serialisation is tried again.
		for (u_int32_t y = 0; y < self->inputNum; y++) {
			CBReleaseObject(&CBGetMessage(self->inputs[y])->bytes);
		}
		for (u_int32_t y = 0; y < self->outputNum; y++) {
			CBReleaseObject(&CBGetMessage(self->inputs[y])->bytes);
		}
		return 0;
	}
	CBByteArraySetInt32(bytes, cursor, self->lockTime);
	return cursor + 4;
}
void CBTransactionTakeInput(CBTransaction * self, CBTransactionInput * input){
	self->inputNum++;
	self->inputs = realloc(self->inputs, sizeof(*self->inputs) * self->inputNum);
	self->inputs[self->inputNum-1] = input;
}
void CBTransactionTakeOutput(CBTransaction * self, CBTransactionOutput * output){
	self->outputNum++;
	self->outputs = realloc(self->outputs, sizeof(*self->outputs) * self->outputNum);
	self->outputs[self->outputNum-1] = output;
}
bool CBTransactionValidateBasic(CBTransaction * self){
	if (!self->inputNum || !self->outputNum) {
		return false;
	}
	u_int32_t length;
	if (CBGetMessage(self)->bytes) { // Already have length
		length = CBGetMessage(self)->bytes->length;
	}else{
		// Calculate length. Worthwhile having a cache? ???
		length = 8 + CBVarIntSizeOf(self->inputNum) + CBVarIntSizeOf(self->outputNum) + self->inputNum*40 + self->outputNum*8;
		for (u_int32_t x = 0; x < self->inputNum; x++) {
			u_int32_t scriptLen = CBGetByteArray(self->inputs[x]->scriptObject)->length;
			length += CBVarIntSizeOf(scriptLen) + scriptLen;
		}
		for (u_int32_t x = 0; x < self->outputNum; x++) {
			u_int32_t scriptLen = CBGetByteArray(self->outputs[x]->scriptObject)->length;
			length += CBVarIntSizeOf(scriptLen) + scriptLen;
		}
	}
	if(length > CB_BLOCK_MAX_SIZE){
		return false;
	}
	// Check that outputs do not overflow by ensuring they do not go over 21 million bitcoins. There was once an vulnerability in the C++ client on this where an attacker could overflow very large outputs to equal small inputs.
	u_int64_t total = 0;
	for (u_int32_t x = 0; x < self->outputNum; x++) {
		if (self->outputs[x]->value > CB_MAX_MONEY) {
			return false;
		}
		total += self->outputs[x]->value;
		if (total > CB_MAX_MONEY) {
			return false;
		}
	}
	return true;
}
