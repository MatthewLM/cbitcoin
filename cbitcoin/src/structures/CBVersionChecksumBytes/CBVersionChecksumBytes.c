//
//  CBVersionChecksumBytes.c
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

#include "CBVersionChecksumBytes.h"

//  Virtual Table Store

static CBVersionChecksumBytesVT * VTStore = NULL;
static int objectNum = 0;

//  Constructors

CBVersionChecksumBytes * CBNewVersionChecksumBytesFromString(char * string,CBEvents * events,CBDependencies * dependencies){
	CBVersionChecksumBytes * self = malloc(sizeof(*self));
	CBAddVTToObject(CBGetObject(self), VTStore, CBCreateVersionChecksumBytesVT);
	bool ok = CBInitVersionChecksumBytesFromString(self,string,events,dependencies);
	if (!ok) {
		return NULL;
	}
	return self;
}
CBVersionChecksumBytes * CBNewVersionChecksumBytesFromBytes(u_int8_t * bytes,u_int32_t size,CBEvents * events){
	CBVersionChecksumBytes * self = malloc(sizeof(*self));
	CBAddVTToObject(CBGetObject(self), VTStore, CBCreateVersionChecksumBytesVT);
	CBInitVersionChecksumBytesFromBytes(self,bytes,size,events);
	return self;
}

//  Virtual Table Creation

CBVersionChecksumBytesVT * CBCreateVersionChecksumBytesVT(){
	CBVersionChecksumBytesVT * VT = malloc(sizeof(*VT));
	CBSetVersionChecksumBytesVT(VT);
	return VT;
}
void CBSetVersionChecksumBytesVT(CBVersionChecksumBytesVT * VT){
	CBSetByteArrayVT((CBByteArrayVT *)VT);
	((CBObjectVT *)VT)->free = (void (*)(void *))CBFreeVersionChecksumBytes;
	VT->getVersion = (u_int8_t (*)(void *))CBVersionChecksumBytesGetVersion;
}

//  Virtual Table Getter

CBVersionChecksumBytesVT * CBGetVersionChecksumBytesVT(void * self){
	return ((CBVersionChecksumBytesVT *)(CBGetObject(self))->VT);
}

//  Object Getter

CBVersionChecksumBytes * CBGetVersionChecksumBytes(void * self){
	return self;
}

//  Initialisers

bool CBInitVersionChecksumBytesFromString(CBVersionChecksumBytes * self,char * string,CBEvents * events,CBDependencies * dependencies){
	CBBigInt bytes = CBDecodeBase58Checked(string, events, dependencies);
	if (bytes.length == 1) {
		return false;
	}
	// Take over the bytes with the CBByteArray
	if (!CBInitByteArrayWithData(CBGetByteArray(self), bytes.data, bytes.length, events))
		return false;
	return true;
}
bool CBInitVersionChecksumBytesFromBytes(CBVersionChecksumBytes * self,u_int8_t * bytes,u_int32_t size,CBEvents * events){
	if (!CBInitByteArrayWithData(CBGetByteArray(self), bytes, size, events))
		return false;
	return true;
}

//  Destructor

void CBFreeVersionChecksumBytes(CBVersionChecksumBytes * self){
	CBFreeProcessVersionChecksumBytes(self);
	CBFree();
}
void CBFreeProcessVersionChecksumBytes(CBVersionChecksumBytes * self){
	CBFreeProcessObject(CBGetObject(self));
}

//  Functions

u_int8_t CBVersionChecksumBytesGetVersion(CBVersionChecksumBytes * self){
	return CBGetByteArrayVT(self)->getLastByte(self);
}
