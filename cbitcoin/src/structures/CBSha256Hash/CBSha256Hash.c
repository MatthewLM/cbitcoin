//
//  CBSha256Hash.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 01/05/2012.
//  Last modified by Matthew Mitchell on 01/05/2012.
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

#include "CBSha256Hash.h"

//  Virtual Table Store

static CBSha256HashVT * VTStore = NULL;
static int objectNum = 0;

//  Constructors

CBSha256Hash * CBNewEmptySha256Hash(CBEvents * events){
	CBSha256Hash * self = malloc(sizeof(*self));
	CBAddVTToObject(CBGetObject(self), VTStore, CBCreateSha256HashVT);
	CBInitEmptySha256Hash(self,events);
	return self;
}
CBSha256Hash * CBNewSha256HashFromByteArray(CBByteArray * bytes,CBEvents * events){
	CBSha256Hash * self = malloc(sizeof(*self));
	CBAddVTToObject(CBGetObject(self), VTStore, CBCreateSha256HashVT);
	CBInitSha256HashFromByteArrayAndHash(self,bytes,-1,events);
	return self;
}
CBSha256Hash * CBNewSha256HashFromByteArrayAndHash(CBByteArray * bytes,int hash,CBEvents * events){
	CBSha256Hash * self = malloc(sizeof(*self));
	CBAddVTToObject(CBGetObject(self), VTStore, CBCreateSha256HashVT);
	CBInitSha256HashFromByteArrayAndHash(self,bytes,hash,events);
	return self;
}

//  Virtual Table Creation

CBSha256HashVT * CBCreateSha256HashVT(){
	CBSha256HashVT * VT = malloc(sizeof(*VT));
	CBSetSha256HashVT(VT);
	return VT;
}
void CBSetSha256HashVT(CBSha256HashVT * VT){
	CBSetByteArrayVT((CBByteArrayVT *)VT);
	((CBObjectVT *)VT)->free = (void (*)(void *))CBFreeSha256Hash;
}

//  Virtual Table Getter

CBSha256HashVT * CBGetSha256HashVT(void * self){
	return ((CBSha256HashVT *)(CBGetObject(self))->VT);
}

//  Object Getter

CBSha256Hash * CBGetSha256Hash(void * self){
	return self;
}

//  Initialiser

bool CBInitEmptySha256Hash(CBSha256Hash * self,CBEvents * events){
	CBInitByteArrayOfSize(CBGetByteArray(self), 32, events);
	self->hash = -1;
	memset(CBGetByteArray(self)->sharedData->data,0,32); // ZERO_HASH
	return true;
}
bool CBInitSha256HashFromByteArrayAndHash(CBSha256Hash * self,CBByteArray * bytes,int hash,CBEvents * events){
	if (bytes->length != 32) 
		events->onErrorReceived(CB_ERROR_SHA_256_HASH_BAD_BYTE_ARRAY_LENGTH,"Error: Cannot make SHA-256 hash from a byte array that is not 32 bytes in length.");
	self->hash = hash;
	CBGetByteArray(self)->events = events;
	CBGetByteArray(self)->length = 32;
	CBGetByteArray(self)->sharedData = bytes->sharedData;
	CBGetByteArray(self)->sharedData->references++;
	CBGetByteArray(self)->offset = bytes->offset;
	return true;
}

//  Destructor

void CBFreeSha256Hash(CBSha256Hash * self){
	CBFreeProcessSha256Hash(self);
	CBFree();
}
void CBFreeProcessSha256Hash(CBSha256Hash * self){
	CBFreeProcessObject(CBGetObject(self));
}

//  Functions

