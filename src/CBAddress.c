//
//  CBAddress.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 03/05/2012.
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

//  SEE HEADER FILE FOR DOCUMENTATION

#include "CBAddress.h"

//  Constructors

CBAddress * CBNewAddressFromRIPEMD160Hash(uint8_t * hash, uint8_t networkCode, bool cacheString){
	CBAddress * self = malloc(sizeof(*self));
	if (NOT self) {
		CBLogError("Cannot allocate %i bytes of memory in CBNewAddressFromRIPEMD160Hash\n", sizeof(*self));
		return NULL;
	}
	CBGetObject(self)->free = CBFreeAddress;
	if (CBInitAddressFromRIPEMD160Hash(self, networkCode, hash, cacheString))
		return self;
	free(self);
	return NULL;
}
CBAddress * CBNewAddressFromString(CBByteArray * string, bool cacheString){
	CBAddress * self = malloc(sizeof(*self));
	if (NOT self) {
		CBLogError("Cannot allocate %i bytes of memory in CBNewAddressFromString\n", sizeof(*self));
		return NULL;
	}
	CBGetObject(self)->free = CBFreeAddress;
	if (CBInitAddressFromString(self, string, cacheString))
		return self;
	free(self);
	return NULL;
}

//  Object Getter

CBAddress * CBGetAddress(void * self){
	return self;
}

//  Initialiser

bool CBInitAddressFromRIPEMD160Hash(CBAddress * self, uint8_t networkCode, uint8_t * hash, bool cacheString){
	// Build address and then complete intitialisation with CBVersionChecksumBytes
	uint8_t * data = malloc(25); // 1 Network byte, 20 hash bytes, 4 checksum bytes.
	if (NOT data) {
		CBLogError("Cannot allocate 25 bytes of memory in CBInitAddressFromRIPEMD160Hash\n");
		return false;
	}
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
	if (NOT CBInitVersionChecksumBytesFromBytes(CBGetVersionChecksumBytes(self), data, 25, cacheString))
		return false;
	return true;
}
bool CBInitAddressFromString(CBAddress * self, CBByteArray * string, bool cacheString){
	if (NOT CBInitVersionChecksumBytesFromString(CBGetVersionChecksumBytes(self), string, cacheString))
		return false;
	return true;
}

//  Destructor

void CBFreeAddress(void * self){
	CBFreeVersionChecksumBytes(CBGetVersionChecksumBytes(self));
}

//  Functions

