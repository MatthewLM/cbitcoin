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

CBAddress * CBNewAddressFromRIPEMD160Hash(uint8_t * hash, CBBase58Prefix prefix, bool cacheString) {
	
	CBAddress * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeAddress;
	CBInitAddressFromRIPEMD160Hash(self, hash, prefix, cacheString);
	return self;
	
}

CBAddress * CBNewAddressFromString(CBByteArray * string, bool cacheString) {
	
	CBAddress * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeAddress;
	
	if (CBInitAddressFromString(self, string, cacheString))
		return self;
	
	free(self);
	return NULL;
	
}

//  Initialiser

void CBInitAddressFromRIPEMD160Hash(CBAddress * self, uint8_t * hash, CBBase58Prefix prefix, bool cacheString) {
	
	// Build address and then complete intitialisation with CBChecksumBytes
	uint8_t * data = malloc(25); // 1 Network byte, 20 hash bytes, 4 checksum bytes.
	
	// Set network byte
	data[0] = prefix;
	
	// Move hash
	memmove(data + 1, hash, 20);
	
	// Initialise CBChecksumBytes
	CBInitChecksumBytesFromBytes(CBGetChecksumBytes(self), data, 25, cacheString);
	
}

bool CBInitAddressFromString(CBAddress * self, CBByteArray * string, bool cacheString) {
	
	if (! CBInitChecksumBytesFromString(CBGetChecksumBytes(self), string, cacheString))
		return false;
	
	return true;
	
}

//  Destructor

void CBDestroyAddress(void * self) {
	
	CBDestroyChecksumBytes(CBGetChecksumBytes(self));
	
}

void CBFreeAddress(void * self) {
	
	CBDestroyAddress(self);
	free(self);
	
}

//  Functions

