//
//  CBRand.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 07/08/2012.
//  Copyright (c) 2012 Matthew Mitchell
//
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

// Includes

#include "CBDependencies.h" // cbitcoin dependencies to implement
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Implementation

bool CBNewSecureRandomGenerator(CBDepObject * gen){
	gen->ptr = malloc(32);
	return true;
}
bool CBSecureRandomSeed(CBDepObject gen){
	return CBGet32RandomBytes(gen.ptr);
}
void CBRandomSeed(CBDepObject gen, uint64_t seed){
	memcpy(gen.ptr, &seed, 8);
	memset((uint8_t *)gen.ptr + 8, 0, 24); // Blank out the rest of the data
}
uint64_t CBSecureRandomInteger(CBDepObject gen){
	CBSha256(gen.ptr, 32, gen.ptr);
	uint64_t i;
	memcpy(&i, gen.ptr, 8);
	return i;
}
void CBFreeSecureRandomGenerator(CBDepObject gen){
	free(gen.ptr);
}
bool CBGet32RandomBytes(uint8_t * bytes){
	FILE * f = fopen("/dev/random", "r");
	bool result = fread(bytes, 1, 32, f) == 32;
	fclose(f);
	return result;
}
