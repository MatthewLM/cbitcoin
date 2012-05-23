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

//  Virtual Table Store

static void * VTStore = NULL;
static int objectNum = 0;

//  Constructors

CBAddress * CBNewAddressFromRIPEMD160Hash(CBNetworkParameters * network,u_int8_t * hash,bool cacheString,CBEvents * events,CBDependencies * dependencies){
	CBAddress * self = malloc(sizeof(*self));
	objectNum++;
	CBAddVTToObject(CBGetObject(self), &VTStore, CBCreateAddressVT);
	CBInitAddressFromRIPEMD160Hash(self,network,hash,cacheString,events,dependencies);
	return self;
}
CBAddress * CBNewAddressFromString(CBString * string,bool cacheString,CBEvents * events,CBDependencies * dependencies){
	CBAddress * self = malloc(sizeof(*self));
	objectNum++;
	CBAddVTToObject(CBGetObject(self), &VTStore, CBCreateAddressVT);
	bool ok = CBInitAddressFromString(self,string,cacheString,events,dependencies);
	if (!ok) {
		return NULL;
	}
	return self;
}

//  Virtual Table Creation

CBAddressVT * CBCreateAddressVT(){
	CBAddressVT * VT = malloc(sizeof(*VT));
	CBSetAddressVT(VT);
	return VT;
}
void CBSetAddressVT(CBAddressVT * VT){
	CBSetVersionChecksumBytesVT((CBVersionChecksumBytesVT *)VT);
	((CBObjectVT *)VT)->free = (void (*)(void *))CBFreeAddress;
}

//  Virtual Table Getter

CBAddressVT * CBGetAddressVT(void * self){
	return ((CBAddressVT *)(CBGetObject(self))->VT);
}

//  Object Getter

CBAddress * CBGetAddress(void * self){
	return self;
}

//  Initialiser

bool CBInitAddressFromRIPEMD160Hash(CBAddress * self,CBNetworkParameters * network,u_int8_t * hash,bool cacheString,CBEvents * events,CBDependencies * dependencies){
	// Build address and then complete intitialisation with CBVersionChecksumBytes
	u_int8_t * data = malloc(25); // 1 Network byte, 20 hash bytes, 4 checksum bytes.
	// Set network byte
	data[0] = network->networkCode;
	// Move hash
	memmove(data+1, hash, 20);
	// Make checksum and move it into address
	u_int8_t * checksum = dependencies->sha256(data,21);
	u_int8_t * checksum2 = dependencies->sha256(checksum,32);
	free(checksum);
	memmove(data+21, checksum2, 4);
	free(checksum2);
	// Initialise CBVersionChecksumBytes
	if (!CBInitVersionChecksumBytesFromBytes(CBGetVersionChecksumBytes(self), data, 25,cacheString, events))
		return false;
	return true;
}
bool CBInitAddressFromString(CBAddress * self,CBString * string,bool cacheString,CBEvents * events,CBDependencies * dependencies){
	if (!CBInitVersionChecksumBytesFromString(CBGetVersionChecksumBytes(self), string,cacheString, events, dependencies))
		return false;
	return true;
}

//  Destructor

void CBFreeAddress(CBAddress * self){
	CBFreeProcessAddress(self);
	CBFree();
}
void CBFreeProcessAddress(CBAddress * self){
	CBFreeProcessVersionChecksumBytes(CBGetVersionChecksumBytes(self));
}

//  Functions

