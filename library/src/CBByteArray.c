//
//  CBByteArray.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 30/04/2012.
//  Copyright (c) 2012 Matthew Mitchell
//  
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

//  SEE HEADER FILE FOR DOCUMENTATION

#include "CBByteArray.h"

//  Constructor

CBByteArray * CBNewByteArrayFromString(char * string, bool terminator) {
	
	CBByteArray * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeByteArray;
	CBInitByteArrayFromString(self, string, terminator);
	
	return self;
	
}

CBByteArray * CBNewByteArrayOfSize(uint32_t size) {
	
	CBByteArray * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeByteArray;
	CBInitByteArrayOfSize(self, size);
	
	return self;
	
}

CBByteArray * CBNewByteArraySubReference(CBByteArray * ref, uint32_t offset, uint32_t length) {
	
	CBByteArray * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeByteArray;
	CBInitByteArraySubReference(self, ref, offset, length);
	
	return self;
	
}

CBByteArray * CBNewByteArrayWithData(uint8_t * data, uint32_t size) {
	
	CBByteArray * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeByteArray;
	CBInitByteArrayWithData(self, data, size);
	
	return self;
	
}

CBByteArray * CBNewByteArrayWithDataCopy(uint8_t * data, uint32_t size) {
	
	CBByteArray * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeByteArray;
	CBInitByteArrayWithDataCopy(self, data, size);
	
	return self;
	
}

CBByteArray * CBNewByteArrayFromHex(char * hex) {
	
	CBByteArray * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeByteArray;
	CBInitByteArrayFromHex(self, hex);
	
	return self;
	
}

//  Initialisers


void CBInitByteArrayFromString(CBByteArray * self, char * string, bool terminator) {
	
	CBInitObject(CBGetObject(self), false);
	
	self->length = (uint32_t)(strlen(string) + terminator);
	self->sharedData = malloc(sizeof(*self->sharedData));
	self->sharedData->data = malloc(self->length);
	self->sharedData->references = 1;
	self->offset = 0;
	
	memcpy(self->sharedData->data, string, self->length);
	
}

void CBInitByteArrayOfSize(CBByteArray * self, uint32_t size) {
	
	CBInitObject(CBGetObject(self), false);
	
	self->length = size;
	self->offset = 0;
	
	if (size){
		self->sharedData = malloc(sizeof(*self->sharedData));
		self->sharedData->references = 1;
		self->sharedData->data = malloc(size);
	}else
		self->sharedData = NULL;
	
}

void CBInitByteArraySubReference(CBByteArray * self, CBByteArray * ref, uint32_t offset, uint32_t length) {
	
	CBInitObject(CBGetObject(self), false);
	
	self->sharedData = ref->sharedData;
	
	// Since a new reference to the shared data is being made, an increase in the reference count must be made.
	self->sharedData->references++;
	
	self->length = length;
	self->offset = ref->offset + offset;
}

void CBInitByteArrayWithData(CBByteArray * self, uint8_t * data, uint32_t size) {
	
	CBInitObject(CBGetObject(self), false);
	
	self->sharedData = malloc(sizeof(*self->sharedData));
	self->sharedData->data = data;
	self->sharedData->references = 1;
	self->length = size;
	self->offset = 0;
	
}

void CBInitByteArrayWithDataCopy(CBByteArray * self, uint8_t * data, uint32_t size) {
	
	CBInitObject(CBGetObject(self), false);
	
	self->sharedData = malloc(sizeof(*self->sharedData));
	self->sharedData->data = malloc(size);
	self->sharedData->references = 1;
	self->length = size;
	self->offset = 0;
	
	memmove(self->sharedData->data, data, size);
}

void CBInitByteArrayFromHex(CBByteArray * self, char * hex) {
	
	CBInitByteArrayOfSize(self, (uint32_t)strlen(hex)/2);
	
	CBStrHexToBytes(hex, CBByteArrayGetData(self));
	
}

//  Destructor

void CBDestroyByteArray(void * self) {
	
	CBByteArrayReleaseSharedData(self);
	
}

void CBFreeByteArray(void * self) {
	
	CBDestroyByteArray(self);
	free(self);
	
}

void CBByteArrayReleaseSharedData(CBByteArray * self) {
	
	if (! self->sharedData)
		return;
	
	self->sharedData->references--;
	if (self->sharedData->references < 1) {
		// Shared data now owned by nothing so free it 
		free(self->sharedData->data);
		free(self->sharedData);
	}
	
}

//  Functions

CBCompare CBByteArrayCompare(CBByteArray * self, CBByteArray * second) {
	
	if (self->length > second->length)
		return CB_COMPARE_MORE_THAN;
	
	else if (self->length < second->length)
		return CB_COMPARE_LESS_THAN;
	
	int res = memcmp(CBByteArrayGetData(self), CBByteArrayGetData(second), self->length);
	
	if (res > 0)
		return CB_COMPARE_MORE_THAN;
	else if (res < 0)
		return CB_COMPARE_LESS_THAN;
	
	return CB_COMPARE_EQUAL;
	
}

CBByteArray * CBByteArrayCopy(CBByteArray * self) {
	
	CBByteArray * new = CBNewByteArrayOfSize(self->length);
	
	memmove(new->sharedData->data, self->sharedData->data + self->offset, self->length);
	
	return new;
}

void CBByteArrayCopyByteArray(CBByteArray * self, uint32_t writeOffset, CBByteArray * source) {
	
	if (! source->length)
		return;
	
	memmove(self->sharedData->data + self->offset + writeOffset, source->sharedData->data + source->offset, source->length);
	
}

void CBByteArrayCopySubByteArray(CBByteArray * self, uint32_t writeOffset, CBByteArray * source, uint32_t readOffset, uint32_t length) {
	
	if (! length)
		return;
	
	memmove(self->sharedData->data + self->offset + writeOffset, source->sharedData->data + source->offset + readOffset, length);
	
}

uint8_t CBByteArrayGetByte(CBByteArray * self, uint32_t index) {
	
	return self->sharedData->data[self->offset+index];
	
}

uint8_t * CBByteArrayGetData(CBByteArray * self) {
	
	return self->sharedData->data + self->offset;
	
}

uint8_t CBByteArrayGetLastByte(CBByteArray * self) {
	
	return self->sharedData->data[self->offset+self->length];
	
}

bool CBByteArrayIsNull(CBByteArray * self) {
	
	for (uint32_t x = 0; x < self->length; x++)
		if (self->sharedData->data[self->offset+x])
			return false;
	
	return true;
	
}

void CBByteArraySanitise(CBByteArray * self) {
	
	CBSanitiseOutput((char *)CBByteArrayGetData(self), self->length);
	
}

void CBByteArraySetByte(CBByteArray * self, uint32_t index, uint8_t byte) {
	
	self->sharedData->data[self->offset+index] = byte;
	
}

void CBByteArraySetBytes(CBByteArray * self, uint32_t index, uint8_t * bytes, uint32_t length) {
	
	memmove(self->sharedData->data + self->offset + index, bytes, length);
	
}

void CBByteArraySetInt16(CBByteArray * self, uint32_t offset, uint16_t integer) {
	
	CBInt16ToArray(self->sharedData->data, self->offset+offset, integer);
	
}

void CBByteArraySetInt32(CBByteArray * self, uint32_t offset, uint32_t integer) {
	
	CBInt32ToArray(self->sharedData->data, self->offset+offset, integer);
	
}

void CBByteArraySetInt64(CBByteArray * self, uint32_t offset, uint64_t integer) {
	
	CBInt64ToArray(self->sharedData->data, self->offset+offset, integer);
	
}

void CBByteArraySetPort(CBByteArray * self, uint32_t offset, uint16_t integer) {
	
	self->sharedData->data[self->offset+offset + 1] = integer;
	self->sharedData->data[self->offset+offset] = integer >> 8;
	
}

void CBByteArraySetVarInt(CBByteArray * self, uint32_t offset, CBVarInt varInt) {
	
	CBByteArraySetVarIntData(CBByteArrayGetData(self), offset, varInt);
	
}

uint16_t CBByteArrayReadInt16(CBByteArray * self, uint32_t offset) {
	
	return CBArrayToInt16(self->sharedData->data, self->offset + offset);
	
}

uint32_t CBByteArrayReadInt32(CBByteArray * self, uint32_t offset) {
	
	return CBArrayToInt32(self->sharedData->data, self->offset + offset);
	
}

uint64_t CBByteArrayReadInt64(CBByteArray * self, uint32_t offset) {
	
	return CBArrayToInt64(self->sharedData->data, self->offset + offset);
	
}

uint16_t CBByteArrayReadPort(CBByteArray * self, uint32_t offset) {
	
	uint16_t result = self->sharedData->data[self->offset+offset + 1];
	result |= (uint16_t)self->sharedData->data[self->offset+offset] << 8;
	
	return result;
	
}

CBVarInt CBByteArrayReadVarInt(CBByteArray * self, uint32_t offset) {
	
	return CBVarIntDecodeData(CBByteArrayGetData(self), offset);
	
}

uint8_t CBByteArrayReadVarIntSize(CBByteArray * self, uint32_t offset) {
	
	return CBVarIntDecodeSize(CBByteArrayGetData(self), offset);
	
}

void CBByteArrayReverseBytes(CBByteArray * self) {
	
	for (int x = 0; x < self->length / 2; x++) {
		uint8_t temp = self->sharedData->data[self->offset+x];
		self->sharedData->data[self->offset+x] = self->sharedData->data[self->offset+self->length-x-1];
		self->sharedData->data[self->offset+self->length-x-1] = temp;
	}
	
}

void CBByteArrayChangeReference(CBByteArray * self, CBByteArray * ref, uint32_t offset) {
	
	// Release last shared data.
	CBByteArrayReleaseSharedData(self);
	
	self->sharedData = ref->sharedData;
	
	// Since a new reference to the shared data is being made, an increase in the reference count must be made.
	self->sharedData->references++;
	
	// New offset for shared data
	self->offset = ref->offset + offset;
	
}

CBByteArray * CBByteArraySubCopy(CBByteArray * self, uint32_t offset, uint32_t length) {
	
	CBByteArray * new = CBNewByteArrayOfSize(length);
	
	memcpy(new->sharedData->data, self->sharedData->data + self->offset + offset, length);
	
	return new;
	
}

CBByteArray * CBByteArraySubReference(CBByteArray * self, uint32_t offset, uint32_t length) {
	
	return CBNewByteArraySubReference(self, offset, length);
	
}

void CBByteArrayToString(CBByteArray * self, uint32_t offset, uint32_t length, char * output, bool backwards) {
	
	for (uint32_t x = offset; x < offset + length; x++) {
		sprintf(output, "%02x", CBByteArrayGetByte(self, offset + (backwards ? length - x - 1 : x)));
		output += 2;
	}
	
}

bool CBStrHexToBytes(char * hex, uint8_t * output) {
	
	char interpret[3];
	char * cursor = hex;
	
	for (uint32_t pos = 0;; cursor += 2, pos++) {
		
		for (uint8_t x = 0; x < 2; x++) {
			
			interpret[x] = cursor[x];
			
			if ((interpret[x] < '0' || interpret[x] > '9')
				&& (interpret[x] < 'a' || interpret[x] > 'f')
				&& (interpret[x] < 'A' || interpret[x] > 'F')) {
				
				if (x == 1)
					return false;
				
				if (interpret[0] == ' '
				    || interpret[0] == '\n'
					|| interpret[0] == '\0') {
					return true;
				}else
					return false;
				
			}
		}
		
		interpret[2] = '\0';
		output[pos] = strtoul(interpret, NULL, 16);
		
	}
	
}
