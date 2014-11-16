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

CBByteArray * CBNewByteArrayOfSize(int size) {
	
	CBByteArray * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeByteArray;
	CBInitByteArrayOfSize(self, size);
	
	return self;
	
}

CBByteArray * CBNewByteArraySubReference(CBByteArray * ref, int offset, int length) {
	
	CBByteArray * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeByteArray;
	CBInitByteArraySubReference(self, ref, offset, length);
	
	return self;
	
}

CBByteArray * CBNewByteArrayWithData(unsigned char * data, int size) {
	
	CBByteArray * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeByteArray;
	CBInitByteArrayWithData(self, data, size);
	
	return self;
	
}

CBByteArray * CBNewByteArrayWithDataCopy(unsigned char * data, int size) {
	
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
	
	self->length = (int)(strlen(string) + terminator);
	self->sharedData = malloc(sizeof(*self->sharedData));
	self->sharedData->data = malloc(self->length);
	self->sharedData->references = 1;
	self->offset = 0;
	
	memcpy(self->sharedData->data, string, self->length);
	
}

void CBInitByteArrayOfSize(CBByteArray * self, int size) {
	
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

void CBInitByteArraySubReference(CBByteArray * self, CBByteArray * ref, int offset, int length) {
	
	CBInitObject(CBGetObject(self), false);
	
	self->sharedData = ref->sharedData;
	
	// Since a new reference to the shared data is being made, an increase in the reference count must be made.
	self->sharedData->references++;
	
	self->length = length;
	self->offset = ref->offset + offset;
}

void CBInitByteArrayWithData(CBByteArray * self, unsigned char * data, int size) {
	
	CBInitObject(CBGetObject(self), false);
	
	self->sharedData = malloc(sizeof(*self->sharedData));
	self->sharedData->data = data;
	self->sharedData->references = 1;
	self->length = size;
	self->offset = 0;
	
}

void CBInitByteArrayWithDataCopy(CBByteArray * self, unsigned char * data, int size) {
	
	CBInitObject(CBGetObject(self), false);
	
	self->sharedData = malloc(sizeof(*self->sharedData));
	self->sharedData->data = malloc(size);
	self->sharedData->references = 1;
	self->length = size;
	self->offset = 0;
	
	memmove(self->sharedData->data, data, size);
}

void CBInitByteArrayFromHex(CBByteArray * self, char * hex) {
	
	CBInitByteArrayOfSize(self, (int)strlen(hex)/2);
	
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

void CBByteArrayCopyByteArray(CBByteArray * self, int writeOffset, CBByteArray * source) {
	
	if (! source->length)
		return;
	
	memmove(self->sharedData->data + self->offset + writeOffset, source->sharedData->data + source->offset, source->length);
	
}

void CBByteArrayCopySubByteArray(CBByteArray * self, int writeOffset, CBByteArray * source, int readOffset, int length) {
	
	if (! length)
		return;
	
	memmove(self->sharedData->data + self->offset + writeOffset, source->sharedData->data + source->offset + readOffset, length);
	
}

int CBByteArrayGetByte(CBByteArray * self, int index) {
	
	return self->sharedData->data[self->offset+index];
	
}

unsigned char * CBByteArrayGetData(CBByteArray * self) {
	
	return self->sharedData->data + self->offset;
	
}

int CBByteArrayGetLastByte(CBByteArray * self) {
	
	return self->sharedData->data[self->offset+self->length];
	
}

bool CBByteArrayIsNull(CBByteArray * self) {
	
	for (int x = 0; x < self->length; x++)
		if (self->sharedData->data[self->offset+x])
			return false;
	
	return true;
	
}

void CBByteArraySanitise(CBByteArray * self) {
	
	CBSanitiseOutput((char *)CBByteArrayGetData(self), self->length);
	
}

void CBByteArraySetByte(CBByteArray * self, int index, int byte) {
	
	self->sharedData->data[self->offset+index] = byte;
	
}

void CBByteArraySetBytes(CBByteArray * self, int index, unsigned char * bytes, int length) {
	
	memmove(self->sharedData->data + self->offset + index, bytes, length);
	
}

void CBByteArraySetInt16(CBByteArray * self, int offset, int integer) {
	
	CBInt16ToArray(self->sharedData->data, self->offset+offset, integer);
	
}

void CBByteArraySetInt32(CBByteArray * self, int offset, int integer) {
	
	CBInt32ToArray(self->sharedData->data, self->offset+offset, integer);
	
}

void CBByteArraySetInt64(CBByteArray * self, int offset, long long int integer) {
	
	CBInt64ToArray(self->sharedData->data, self->offset+offset, integer);
	
}

void CBByteArraySetPort(CBByteArray * self, int offset, int integer) {
	
	self->sharedData->data[self->offset+offset + 1] = integer;
	self->sharedData->data[self->offset+offset] = integer >> 8;
	
}

void CBByteArraySetVarInt(CBByteArray * self, int offset, CBVarInt varInt) {
	
	CBByteArraySetVarIntData(CBByteArrayGetData(self), offset, varInt);
	
}

int CBByteArrayReadInt16(CBByteArray * self, int offset) {
	
	return CBArrayToInt16(self->sharedData->data, self->offset + offset);
	
}

int CBByteArrayReadInt32(CBByteArray * self, int offset) {
	
	return CBArrayToInt32(self->sharedData->data, self->offset + offset);
	
}

long long int CBByteArrayReadInt64(CBByteArray * self, int offset) {
	
	return CBArrayToInt64(self->sharedData->data, self->offset + offset);
	
}

int CBByteArrayReadPort(CBByteArray * self, int offset) {
	
	int result = self->sharedData->data[self->offset+offset + 1];
	result |= (int)self->sharedData->data[self->offset+offset] << 8;
	
	return result;
	
}

CBVarInt CBByteArrayReadVarInt(CBByteArray * self, int offset) {
	
	return CBVarIntDecodeData(CBByteArrayGetData(self), offset);
	
}

int CBByteArrayReadVarIntSize(CBByteArray * self, int offset) {
	
	return CBVarIntDecodeSize(CBByteArrayGetData(self), offset);
	
}

void CBByteArrayReverseBytes(CBByteArray * self) {
	
	for (int x = 0; x < self->length / 2; x++) {
		int temp = self->sharedData->data[self->offset+x];
		self->sharedData->data[self->offset+x] = self->sharedData->data[self->offset+self->length-x-1];
		self->sharedData->data[self->offset+self->length-x-1] = temp;
	}
	
}

void CBByteArrayChangeReference(CBByteArray * self, CBByteArray * ref, int offset) {
	
	// Release last shared data.
	CBByteArrayReleaseSharedData(self);
	
	self->sharedData = ref->sharedData;
	
	// Since a new reference to the shared data is being made, an increase in the reference count must be made.
	self->sharedData->references++;
	
	// New offset for shared data
	self->offset = ref->offset + offset;
	
}

CBByteArray * CBByteArraySubCopy(CBByteArray * self, int offset, int length) {
	
	CBByteArray * new = CBNewByteArrayOfSize(length);
	
	memcpy(new->sharedData->data, self->sharedData->data + self->offset + offset, length);
	
	return new;
	
}

CBByteArray * CBByteArraySubReference(CBByteArray * self, int offset, int length) {
	
	return CBNewByteArraySubReference(self, offset, length);
	
}

void CBByteArrayToString(CBByteArray * self, int offset, int length, char * output, bool backwards) {
	
	CBBytesToString(CBByteArrayGetData(self), offset, length, output, backwards);	
}

void CBBytesToString(unsigned char * bytes, int offset, int length, char * output, bool backwards) {

	for (int x = offset; x < offset + length; x++) {
		sprintf(output, "%02x", bytes[offset + (backwards ? length - x - 1 : x)]);
		output += 2;
	}

}

bool CBStrHexToBytes(char * hex, unsigned char * output) {
	
	char interpret[3];
	char * cursor = hex;
	
	for (int pos = 0;; cursor += 2, pos++) {
		
		for (int x = 0; x < 2; x++) {
			
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
