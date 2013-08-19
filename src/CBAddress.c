//
//  CBAddress.c
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

#include "CBAddress.h"

//  Constructors

CBAddress * CBNewAddressFromRIPEMD160Hash(uint8_t * hash, uint8_t networkCode, bool cacheString){
	CBAddress * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeAddress;
	CBInitAddressFromRIPEMD160Hash(self, networkCode, hash, cacheString);
	return self;
}
CBAddress * CBNewAddressFromString(CBByteArray * string, bool cacheString){
	CBAddress * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeAddress;
	if (CBInitAddressFromString(self, string, cacheString))
		return self;
	free(self);
	return NULL;
}

//  Initialiser

void CBInitAddressFromRIPEMD160Hash(CBAddress * self, uint8_t networkCode, uint8_t * hash, bool cacheString){
	// Build address and then complete intitialisation with CBVersionChecksumBytes
	uint8_t * data = malloc(25); // 1 Network byte, 20 hash bytes, 4 checksum bytes.
	// Set network byte
	data[0] = networkCode;
	// Move hash
	memmove(data+1, hash, 20);
	// Make checksum and move it into address
	uint8_t checksum[32];
	uint8_t checksum2[32];
	CBSha256(data, 21, checksum);
	CBSha256(checksum, 32, checksum2);
	memmove(data+21, checksum2, 4);
	// Initialise CBVersionChecksumBytes
	CBInitVersionChecksumBytesFromBytes(CBGetVersionChecksumBytes(self), data, 25, cacheString);
}
bool CBInitAddressFromString(CBAddress * self, CBByteArray * string, bool cacheString){
	if (! CBInitVersionChecksumBytesFromString(CBGetVersionChecksumBytes(self), string, cacheString))
		return false;
	return true;
}

//  Destructor

void CBDestroyAddress(void * self){
	CBDestroyVersionChecksumBytes(CBGetVersionChecksumBytes(self));
}
void CBFreeAddress(void * self){
	CBDestroyAddress(self);
	free(self);
}

//  Functions

