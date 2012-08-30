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

CBAddress * CBNewAddressFromRIPEMD160Hash(uint8_t * hash,uint8_t networkCode,bool cacheString,CBEvents * events){
	CBAddress * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeAddress;
	CBInitAddressFromRIPEMD160Hash(self,networkCode,hash,cacheString,events);
	return self;
}
CBAddress * CBNewAddressFromString(CBByteArray * string,bool cacheString,CBEvents * events){
	CBAddress * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeAddress;
	bool ok = CBInitAddressFromString(self,string,cacheString,events);
	if (NOT ok) {
		return NULL;
	}
	return self;
}

//  Object Getter

CBAddress * CBGetAddress(void * self){
	return self;
}

//  Initialiser

bool CBInitAddressFromRIPEMD160Hash(CBAddress * self,uint8_t networkCode,uint8_t * hash,bool cacheString,CBEvents * events){
	// Build address and then complete intitialisation with CBVersionChecksumBytes
	uint8_t * data = malloc(25); // 1 Network byte, 20 hash bytes, 4 checksum bytes.
	// Set network byte
	data[0] = networkCode;
	// Move hash
	memmove(data+1, hash, 20);
	// Make checksum and move it into address
	uint8_t checksum[32];
	uint8_t checksum2[32];
	CBSha256(data,21,checksum);
	CBSha256(checksum,32,checksum2);
	memmove(data+21, checksum2, 4);
	// Initialise CBVersionChecksumBytes
	if (NOT CBInitVersionChecksumBytesFromBytes(CBGetVersionChecksumBytes(self), data, 25,cacheString, events))
		return false;
	return true;
}
bool CBInitAddressFromString(CBAddress * self,CBByteArray * string,bool cacheString,CBEvents * events){
	if (NOT CBInitVersionChecksumBytesFromString(CBGetVersionChecksumBytes(self), string, cacheString, events))
		return false;
	return true;
}

//  Destructor

void CBFreeAddress(void * self){
	CBFreeVersionChecksumBytes(CBGetVersionChecksumBytes(self));
}

//  Functions

