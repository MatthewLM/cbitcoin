//
//  CBChainDescriptor.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 11/07/2012.
//  Copyright (c) 2012 Matthew Mitchell
//  
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

//  SEE HEADER FILE FOR DOCUMENTATION 

#include "CBChainDescriptor.h"

//  Constructors

CBChainDescriptor * CBNewChainDescriptor(){
	CBChainDescriptor * self = malloc(sizeof(*self));
	if (NOT self) {
		CBLogError("Cannot allocate %i bytes of memory in CBNewChainDescriptor\n", sizeof(*self));
		return NULL;
	}
	CBGetObject(self)->free = CBFreeChainDescriptor;
	if(CBInitChainDescriptor(self))
		return self;
	free(self);
	return NULL;
}
CBChainDescriptor * CBNewChainDescriptorFromData(CBByteArray * data){
	CBChainDescriptor * self = malloc(sizeof(*self));
	if (NOT self) {
		CBLogError("Cannot allocate %i bytes of memory in CBNewChainDescriptorFromData\n", sizeof(*self));
		return NULL;
	}
	CBGetObject(self)->free = CBFreeChainDescriptor;
	if(CBInitChainDescriptorFromData(self, data))
		return self;
	free(self);
	return NULL;
}

//  Object Getter

CBChainDescriptor * CBGetChainDescriptor(void * self){
	return self;
}

//  Initialisers

bool CBInitChainDescriptor(CBChainDescriptor * self){
	self->hashNum = 0;
	self->hashes = NULL;
	if (NOT CBInitMessageByObject(CBGetMessage(self)))
		return false;
	return true;
}
bool CBInitChainDescriptorFromData(CBChainDescriptor * self, CBByteArray * data){
	self->hashNum = 0;
	self->hashes = NULL;
	if (NOT CBInitMessageByData(CBGetMessage(self), data))
		return false;
	return true;
}

//  Destructor

void CBDestroyChainDescriptor(void * vself){
	CBChainDescriptor * self = vself;
	for (uint16_t x = 0; x < self->hashNum; x++) {
		CBReleaseObject(self->hashes[x]);
	}
	free(self->hashes);
	CBDestroyMessage(self);
}
void CBFreeChainDescriptor(void * self){
	CBDestroyChainDescriptor(self);
	free(self);
}

//  Functions

bool CBChainDescriptorAddHash(CBChainDescriptor * self, CBByteArray * hash){
	CBRetainObject(hash);
	return CBChainDescriptorTakeHash(self, hash);
}
uint16_t CBChainDescriptorDeserialise(CBChainDescriptor * self){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (NOT bytes) {
		CBLogError("Attempting to deserialise a CBChainDescriptor with no bytes.");
		return 0;
	}
	if (bytes->length < 33) {
		CBLogError("Attempting to deserialise a CBChainDescriptor with less bytes than required for one hash.");
		return 0;
	}
	CBVarInt hashNum = CBVarIntDecode(bytes, 0);
	if (hashNum.val > 500) {
		CBLogError("Attempting to deserialise a CBChainDescriptor with a var int over 500.");
		return 0;
	}
	if (bytes->length < hashNum.size + hashNum.val * 32) {
		CBLogError("Attempting to deserialise a CBChainDescriptor with less bytes than required for the hashes.");
		return 0;
	}
	// Deserialise each hash
	self->hashes = malloc(sizeof(*self->hashes) * (size_t)hashNum.val);
	if (NOT self->hashes) {
		CBLogError("Cannot allocate %i bytes of memory in CBChainDescriptorDeserialise\n", sizeof(*self->hashes) * (size_t)hashNum.val);
		return 0;
	}
	self->hashNum = hashNum.val;
	uint16_t cursor = hashNum.size;
	for (uint16_t x = 0; x < self->hashNum; x++) {
		self->hashes[x] = CBNewByteArraySubReference(bytes, cursor, 32);
		if (NOT self->hashes[x]){
			CBLogError("Cannot create new CBByteArray in CBChainDescriptorDeserialise\n");
			return 0;
		}
		cursor += 32;
	}
	return cursor;
}
uint16_t CBChainDescriptorSerialise(CBChainDescriptor * self){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (NOT bytes) {
		CBLogError("Attempting to serialise a CBChainDescriptor with no bytes.");
		return 0;
	}
	CBVarInt hashNum = CBVarIntFromUInt64(self->hashNum);
	if (bytes->length < hashNum.size + self->hashNum * 32) {
		CBLogError("Attempting to serialise a CBChainDescriptor with less bytes than required for the hashes.");
		return 0;
	}
	CBVarIntEncode(bytes, 0, hashNum);
	uint16_t cursor = hashNum.size;
	for (uint16_t x = 0; x < self->hashNum; x++) {
		CBByteArrayCopyByteArray(bytes, cursor, self->hashes[x]);
		CBByteArrayChangeReference(self->hashes[x], bytes, cursor);
		cursor += 32;
	}
	CBGetMessage(self)->serialised = true;
	return cursor;
}
bool CBChainDescriptorTakeHash(CBChainDescriptor * self, CBByteArray * hash){
	self->hashNum++;
	CBByteArray ** temp = realloc(self->hashes, sizeof(*self->hashes) * self->hashNum);
	if (NOT temp)
		return false;
	self->hashes = temp;
	self->hashes[self->hashNum-1] = hash;
	return true;
}
