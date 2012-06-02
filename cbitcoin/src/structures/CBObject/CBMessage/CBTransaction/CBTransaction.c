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

//  Virtual Table Store

static void * VTStore = NULL;
static int objectNum = 0;

//  Constructor

CBTransaction * CBNewTransaction(CBNetworkParameters * params, u_int32_t lockTime, u_int32_t protocolVersion, CBEvents * events){
	CBTransaction * self = malloc(sizeof(*self));
	objectNum++;
	CBAddVTToObject(CBGetObject(self), &VTStore, CBCreateTransactionVT);
	CBInitTransaction(self, params, lockTime, protocolVersion, events);
	return self;
}
CBTransaction * CBNewTransactionFromData(CBNetworkParameters * params, CBByteArray * bytes, CBEvents * events){
	CBTransaction * self = malloc(sizeof(*self));
	objectNum++;
	CBAddVTToObject(CBGetObject(self), &VTStore, CBCreateTransactionVT);
	CBInitTransactionFromData(self, params, bytes, events);
	return self;
}

//  Virtual Table Creation

CBTransactionVT * CBCreateTransactionVT(){
	CBTransactionVT * VT = malloc(sizeof(*VT));
	CBSetTransactionVT(VT);
	return VT;
}
void CBSetTransactionVT(CBTransactionVT * VT){
	CBSetMessageVT((CBMessageVT *)VT);
	((CBObjectVT *)VT)->free = (void (*)(void *))CBFreeTransaction;
	((CBMessageVT *)VT)->serialise = (u_int32_t (*)(void *))CBTransactionSerialise;
	((CBMessageVT *)VT)->deserialise = (u_int32_t (*)(void *))CBTransactionDeserialise;
	VT->addInput = (void (*)(void *,CBTransactionInput *)) CBTransactionAddInput;
	VT->addOutput = (void (*)(void *,CBTransactionOutput *)) CBTransactionAddOutput;
}

//  Virtual Table Getter

CBTransactionVT * CBGetTransactionVT(void * self){
	return ((CBTransactionVT *)(CBGetObject(self))->VT);
}

//  Object Getter

CBTransaction * CBGetTransaction(void * self){
	return self;
}

//  Initialiser

bool CBInitTransaction(CBTransaction * self,CBNetworkParameters * params, u_int32_t lockTime, u_int32_t protocolVersion, CBEvents * events){
	self->lockTime = lockTime;
	self->inputNum = 0;
	self->outputNum = 0;
	self->inputs = NULL;
	self->outputs = NULL;
	self->protocolVersion = protocolVersion;
	if (!CBInitMessageByObject(CBGetMessage(self), params, events))
		return false;
	return true;
}
bool CBInitTransactionFromData(CBTransaction * self,CBNetworkParameters * params, CBByteArray * data,CBEvents * events){
	self->inputNum = 0;
	self->outputNum = 0;
	self->inputs = NULL;
	self->outputs = NULL;
	if (!CBInitMessageByData(CBGetMessage(self), params, data, events))
		return false;
	return true;
}

//  Destructor

void CBFreeTransaction(CBTransaction * self){
	CBFreeProcessTransaction(self);
	CBFree();
}
void CBFreeProcessTransaction(CBTransaction * self){
	for (u_int32_t x = 0; x < self->inputNum; x++)
		CBGetObjectVT(self->inputs[x])->release(&self->inputs[x]);
	free(self->inputs);
	for (u_int32_t x = 0; x < self->outputNum; x++)
		CBGetObjectVT(self->outputs[x])->release(&self->outputs[x]);
	free(self->outputs);
	CBFreeProcessObject(CBGetObject(self));
}

//  Functions

void CBTransactionAddInput(CBTransaction * self, CBTransactionInput * input){
	self->inputNum++;
	self->inputs = realloc(self->inputs, sizeof(*self->inputs) * self->inputNum);
	self->inputs[self->inputNum-1] = input;
	CBGetObjectVT(input)->retain(input);
}
void CBTransactionAddOutput(CBTransaction * self, CBTransactionOutput * output){
	self->outputNum++;
	self->outputs = realloc(self->outputs, sizeof(*self->outputs) * self->outputNum);
	self->outputs[self->outputNum-1] = output;
	CBGetObjectVT(output)->retain(output);
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
	self->protocolVersion = CBGetByteArrayVT(bytes)->readUInt32(bytes,0);
	CBVarInt inputOutputLen = CBVarIntDecode(bytes, 4);
	u_int32_t cursor = 4 + inputOutputLen.size;
	self->inputNum = (u_int32_t)inputOutputLen.val;
	self->inputs = malloc(sizeof(*self->inputs) * self->inputNum);
	for (u_int32_t x = 0; x < self->inputNum; x++) {
		CBTransactionInput * input = CBNewTransactionInputFromData(CBGetMessage(self)->params, CBGetByteArrayVT(bytes)->subRef(bytes,cursor,bytes->length-cursor), self, CBGetMessage(self)->events);
		u_int32_t len = CBGetMessageVT(input)->deserialise(input);
		if (!len) {
			CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"CBTransaction cannot be deserialised because of an error with the input number %u.",x);
			// Free data
			CBGetObjectVT(input)->release(input);
			for (u_int8_t y = 0; y < x; y++) {
				CBGetObjectVT(self->inputs[y])->release(&self->inputs[y]);
			}
			return 0;
		}
		// The input was deserialised correctly. Now adjust the length and add it to the transaction.
		CBGetMessage(input)->bytes->length = len;
		self->inputs[x] = input;
		cursor += len; // Move along to next input
	}
	if (bytes->length < cursor + 5) { // Needs at least 5 more for the output CBVarInt and the lockTime
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBTransaction with not enough bytes for the outputs and lockTime.");
		for (u_int8_t y = 0; y < self->inputNum; y++) {
			CBGetObjectVT(self->inputs[y])->release(&self->inputs[y]);
		}
		return 0;
	}
	inputOutputLen = CBVarIntDecode(bytes, cursor);
	cursor += inputOutputLen.size; // Move past output CBVarInt
	self->outputNum = (u_int32_t)inputOutputLen.val;
	self->outputs = malloc(sizeof(*self->outputs) * self->outputNum);
	for (u_int32_t x = 0; x < self->outputNum; x++) {
		CBTransactionOutput * output = CBNewTransactionOutputFromData(CBGetMessage(self)->params, CBGetByteArrayVT(bytes)->subRef(bytes,cursor,bytes->length-cursor), self, CBGetMessage(self)->events);
		u_int32_t len = CBGetMessageVT(output)->deserialise(output);
		if (!len) {
			CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"CBTransaction cannot be deserialised because of an error with the output number %u.",x);
			// Free data
			CBGetObjectVT(output)->release(output);
			for (u_int8_t y = 0; y < self->inputNum; y++) {
				CBGetObjectVT(self->inputs[y])->release(&self->inputs[y]);
			}
			for (u_int8_t y = 0; y < x; y++) {
				CBGetObjectVT(self->outputs[y])->release(&self->outputs[y]);
			}
			return 0;
		}
		// The output was deserialised correctly. Now adjust the length and add it to the transaction.
		CBGetMessage(output)->bytes->length = len;
		self->outputs[x] = output;
		cursor += len; // Move along to next output
	}
	if (bytes->length < cursor + 4) { // Ensure 4 bytes are available for lockTime
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBTransaction with not enough bytes for the lockTime.");
		for (u_int8_t y = 0; y < self->inputNum; y++) {
			CBGetObjectVT(self->inputs[y])->release(&self->inputs[y]);
		}
		for (u_int8_t y = 0; y < self->outputNum; y++) {
			CBGetObjectVT(self->outputs[y])->release(&self->outputs[y]);
		}
		return 0;
	}
	self->lockTime = CBGetByteArrayVT(bytes)->readUInt32(bytes,cursor);
	return cursor + 4;
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
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_SERIALISATION_BAD_BYTES,"Attempting to serialise a CBTransaction with less bytes than required. %i < %i\n",bytes->length, cursor);
		return 0;
	}
	// Serialise data into the CBByteArray and rereference objects to this CBByteArray to save memory.
	CBGetByteArrayVT(bytes)->setUInt32(bytes,0,self->protocolVersion);
	CBVarIntEncode(bytes, 4, inputNum);
	for (u_int32_t x = 0; x < self->inputNum; x++) {
		CBGetMessage(self->inputs[x])->bytes = CBGetByteArrayVT(bytes)->subRef(bytes,cursor,bytes->length-cursor);
		u_int32_t len = CBGetMessageVT(self->inputs[x])->serialise(self->inputs[x]);
		if (!len) {
			CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"CBTransaction cannot be serialised because of an error with the input number %u.",x);
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
		CBGetMessage(self->outputs[x])->bytes = CBGetByteArrayVT(bytes)->subRef(bytes,cursor,bytes->length-cursor);
		u_int32_t len = CBGetMessageVT(self->outputs[x])->serialise(self->outputs[x]);
		if (!len) {
			CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"CBTransaction cannot be serialised because of an error with the output number %u.",x);
			return 0;
		}
		CBGetMessage(self->outputs[x])->bytes->length = len;
		cursor += len;
	}
	if (bytes->length < cursor + 4) { // Check room for lockTime.
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to serialise a CBTransaction with less bytes than required for the lockTime. %i < %i\n",bytes->length, cursor + 4);
		return 0;
	}
	CBGetByteArrayVT(bytes)->setUInt32(bytes,cursor,self->lockTime);
	return cursor + 4;
}
