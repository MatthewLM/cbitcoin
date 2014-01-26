//
//  CBChecksumBytes.c
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

#include "CBChecksumBytes.h"

//  Constructors

CBChecksumBytes * CBNewChecksumBytesFromString(CBByteArray * string, bool cacheString){
	CBChecksumBytes * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeChecksumBytes;
	if(CBInitChecksumBytesFromString(self, string, cacheString))
		return self;
	free(self);
	return NULL;
}
CBChecksumBytes * CBNewChecksumBytesFromBytes(uint8_t * bytes, uint32_t size, bool cacheString){
	CBChecksumBytes * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeChecksumBytes;
	CBInitChecksumBytesFromBytes(self, bytes, size, cacheString);
	return self;
}
CBChecksumBytes * CBNewChecksumBytesFromHex(char * hex, bool cacheString){
	CBChecksumBytes * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeChecksumBytes;
	CBInitChecksumBytesFromHex(self, hex, cacheString);
	return self;
}

//  Initialisers

bool CBInitChecksumBytesFromString(CBChecksumBytes * self, CBByteArray * string, bool cacheString){
	// Cache string if needed
	if (cacheString) {
		self->cachedString = string;
		CBRetainObject(string);
	}else
		self->cachedString = NULL;
	self->cacheString = cacheString;
	// Get bytes from string conversion
	CBBigInt bytes;
	CBBigIntAlloc(&bytes, string->length-2); // Posible that the string is two more than the number of bytes.
	if (! CBDecodeBase58Checked(&bytes, (char *)CBByteArrayGetData(string)))
		return false;
	// Take over the bytes with the CBByteArray
	CBInitByteArrayWithData(CBGetByteArray(self), bytes.data, bytes.length);
	CBByteArrayReverseBytes(CBGetByteArray(self)); // CBBigInt is in little-endian. Conversion needed to make bitcoin address the right way.
	return true;
}
void CBInitChecksumBytesFromBytes(CBChecksumBytes * self, uint8_t * bytes, uint32_t size, bool cacheString){
	self->cacheString = cacheString;
	self->cachedString = NULL;
	// Make checksum and move it into bytes
	uint8_t checksum[32];
	uint8_t checksum2[32];
	CBSha256(bytes, size-4, checksum);
	CBSha256(checksum, 32, checksum2);
	memmove(bytes+size-4, checksum2, 4);
	CBInitByteArrayWithData(CBGetByteArray(self), bytes, size);
}
void CBInitChecksumBytesFromHex(CBChecksumBytes * self, char * hex, bool cacheString){
	uint32_t size = (uint32_t)strlen(hex)/2;
	CBInitByteArrayOfSize(CBGetByteArray(self), size + 4);
	uint8_t * bytes = CBByteArrayGetData(CBGetByteArray(self));
	CBStrHexToBytes(hex, bytes);
	// Make checksum
	uint8_t checksum[32];
	uint8_t checksum2[32];
	CBSha256(bytes, size, checksum);
	CBSha256(checksum, 32, checksum2);
	memmove(bytes + size, checksum2, 4);
}

//  Destructor

void CBDestroyChecksumBytes(void * vself){
	CBChecksumBytes * self = vself;
	if (self->cachedString) CBReleaseObject(self->cachedString);
	CBDestroyByteArray(CBGetByteArray(self));
}
void CBFreeChecksumBytes(void * self){
	CBDestroyChecksumBytes(self);
	free(self);
}

//  Functions

CBBase58Prefix CBChecksumBytesGetPrefix(CBChecksumBytes * self){
	return CBByteArrayGetByte(CBGetByteArray(self), 0);
}
CBByteArray * CBChecksumBytesGetString(CBChecksumBytes * self){
	if (self->cachedString) {
		// Return cached string
		CBRetainObject(self->cachedString);
		return self->cachedString;
	}else{
		// Make string
		CBByteArrayReverseBytes(CBGetByteArray(self)); // Make this into little-endian
		CBBigInt bytes;
		CBBigIntAlloc(&bytes, CBGetByteArray(self)->length);
		bytes.length = CBGetByteArray(self)->length;
		memcpy(bytes.data, CBByteArrayGetData(CBGetByteArray(self)), bytes.length);
		char * string = CBEncodeBase58(&bytes);
		free(bytes.data);
		CBByteArray * str = CBNewByteArrayFromString(string, true);
		free(string);
		CBByteArrayReverseBytes(CBGetByteArray(self)); // Now the string is got, back to big-endian.
		if (self->cacheString) {
			self->cachedString = str;
			CBRetainObject(str); // Retain for this object.
		}
		return str; // No additional retain. Retained from constructor.
	}
}
