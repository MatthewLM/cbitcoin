//
//  CBWIF.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 20/10/2013.
//  Copyright (c) 2013 Matthew Mitchell
//  
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

//  SEE HEADER FILE FOR DOCUMENTATION

#include "CBWIF.h"

//  Constructors

CBWIF * CBNewWIFFromPrivateKey(uint8_t * privKey, bool useCompression, CBBase58Prefix prefix, bool cacheString){
	CBWIF * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeWIF;
	CBInitWIFFromPrivateKey(self, privKey, useCompression, prefix, cacheString);
	return self;
}
CBWIF * CBNewWIFFromString(CBByteArray * string, bool cacheString){
	CBWIF * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeWIF;
	if (CBInitWIFFromString(self, string, cacheString))
		return self;
	free(self);
	return NULL;
}

//  Initialiser

void CBInitWIFFromPrivateKey(CBWIF * self, uint8_t * privKey, bool useCompression, CBBase58Prefix prefix, bool cacheString){
	// Build WIF and then complete intitialisation with CBChecksumBytes
	uint8_t * data = malloc(37 + useCompression); // 1 byte for 0x80, 32 key bytes, 4 checksum bytes. Extra byte for compression.
	// Set prefix
	data[0] = prefix;
	// Move key
	memmove(data+1, privKey, 32);
	if (useCompression)
		data[33] = 1;
	// Initialise CBChecksumBytes
	CBInitChecksumBytesFromBytes(CBGetChecksumBytes(self), data, 37 + useCompression, cacheString);
}
bool CBInitWIFFromString(CBWIF * self, CBByteArray * string, bool cacheString){
	if (! CBInitChecksumBytesFromString(CBGetChecksumBytes(self), string, cacheString))
		return false;
	return true;
}

//  Destructor

void CBDestroyWIF(void * self){
	CBDestroyChecksumBytes(CBGetChecksumBytes(self));
}
void CBFreeWIF(void * self){
	CBDestroyWIF(self);
	free(self);
}

//  Functions

void CBWIFGetPrivateKey(CBWIF * self, uint8_t * privKey){
	memcpy(privKey, CBByteArrayGetData(CBGetByteArray(self)) + 1, 32);
}

bool CBWIFUseCompression(CBWIF * self){
	return CBGetByteArray(self)->length == 38;
}
