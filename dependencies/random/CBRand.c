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

bool CBNewSecureRandomGenerator(uint64_t * gen){
	*gen = (uint64_t)malloc(32);
	return *gen;
}
bool CBSecureRandomSeed(uint64_t gen){
	FILE * f = fopen("/dev/urandom", "r"); // Using urandom for speed.
	return fread((void *)gen, 1, 32, f) == 32;
}
void CBRandomSeed(uint64_t gen, uint64_t seed){
	memcpy((void *)gen, &seed, 8);
	memset(((uint8_t *)gen) + 8, 0, 24); // Blank out the rest of the data
}
uint64_t CBSecureRandomInteger(uint64_t gen){
	CBSha256((uint8_t *)gen, 32, (void *)gen);
	uint64_t i;
	memcpy(&i, (void *)gen, 8);
	return i;
}
void CBFreeSecureRandomGenerator(uint64_t gen){
	free((void *)gen);
}
