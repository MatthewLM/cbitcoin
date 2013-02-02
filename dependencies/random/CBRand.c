//
//  CBRand.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 07/08/2012.
//  Copyright (c) 2012 Matthew Mitchell
//
//  This file is part of cbitcoin.
//
//  cbitcoin is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  cbitcoin is distributed in the hope that it will be useful, 
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with cbitcoin.  If not, see <http://www.gnu.org/licenses/>.

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
