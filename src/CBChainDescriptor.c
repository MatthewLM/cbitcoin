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

CBChainDescriptor * CBNewChainDescriptor() {
	
	CBChainDescriptor * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeChainDescriptor;
	CBInitChainDescriptor(self);
	
	return self;
	
}

CBChainDescriptor * CBNewChainDescriptorFromData(CBByteArray * data) {
	
	CBChainDescriptor * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeChainDescriptor;
	CBInitChainDescriptorFromData(self, data);
	
	return self;
	
}

//  Initialisers

void CBInitChainDescriptor(CBChainDescriptor * self) {
	
	self->hashNum = 0;
	self->hashes = NULL;
	
	CBInitMessageByObject(CBGetMessage(self));
	
}

void CBInitChainDescriptorFromData(CBChainDescriptor * self, CBByteArray * data) {
	
	self->hashNum = 0;
	self->hashes = NULL;
	
	CBInitMessageByData(CBGetMessage(self), data);
	
}

//  Destructor

void CBDestroyChainDescriptor(void * vself) {
	
	CBChainDescriptor * self = vself;
	
	for (uint16_t x = 0; x < self->hashNum; x++) {
		CBReleaseObject(self->hashes[x]);
	}
	
	free(self->hashes);
	CBDestroyMessage(self);
	
}

void CBFreeChainDescriptor(void * self) {
	
	CBDestroyChainDescriptor(self);
	free(self);
	
}

//  Functions

void CBChainDescriptorAddHash(CBChainDescriptor * self, CBByteArray * hash) {
	
	CBRetainObject(hash);
	CBChainDescriptorTakeHash(self, hash);
	
}

uint32_t CBChainDescriptorDeserialise(CBChainDescriptor * self) {
	
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (! bytes) {
		CBLogError("Attempting to deserialise a CBChainDescriptor with no bytes.");
		return CB_DESERIALISE_ERROR;
	}
	if (bytes->length < 1) {
		CBLogError("Attempting to deserialise a CBChainDescriptor with no bytes");
		return CB_DESERIALISE_ERROR;
	}
	
	CBVarInt hashNum = CBByteArrayReadVarInt(bytes, 0);
	if (hashNum.val > 500) {
		CBLogError("Attempting to deserialise a CBChainDescriptor with a var int over 500.");
		return CB_DESERIALISE_ERROR;
	}
	if (bytes->length < hashNum.size + hashNum.val * 32) {
		CBLogError("Attempting to deserialise a CBChainDescriptor with less bytes than required for the hashes.");
		return CB_DESERIALISE_ERROR;
	}
	
	// Deserialise each hash
	self->hashes = malloc(sizeof(*self->hashes) * (size_t)hashNum.val);
	self->hashNum = hashNum.val;
	uint16_t cursor = hashNum.size;
	for (uint16_t x = 0; x < self->hashNum; x++) {
		self->hashes[x] = CBNewByteArraySubReference(bytes, cursor, 32);
		cursor += 32;
	}
	
	return cursor;
	
}

uint16_t CBChainDescriptorSerialise(CBChainDescriptor * self) {
	
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (! bytes) {
		CBLogError("Attempting to serialise a CBChainDescriptor with no bytes.");
		return 0;
	}
	
	CBVarInt hashNum = CBVarIntFromUInt64(self->hashNum);
	if (bytes->length < hashNum.size + self->hashNum * 32) {
		CBLogError("Attempting to serialise a CBChainDescriptor with less bytes than required for the hashes.");
		return 0;
	}
	CBByteArraySetVarInt(bytes, 0, hashNum);
	
	uint16_t cursor = hashNum.size;
	for (uint16_t x = 0; x < self->hashNum; x++) {
		CBByteArrayCopyByteArray(bytes, cursor, self->hashes[x]);
		CBByteArrayChangeReference(self->hashes[x], bytes, cursor);
		cursor += 32;
	}
	
	CBGetMessage(self)->serialised = true;
	
	return cursor;
	
}

void CBChainDescriptorTakeHash(CBChainDescriptor * self, CBByteArray * hash) {

	self->hashes = realloc(self->hashes, sizeof(*self->hashes) * ++self->hashNum);
	self->hashes[self->hashNum-1] = hash;
	
}
