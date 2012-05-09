//
//  CBEngine.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 28/04/2012.
//  Last modified by Matthew Mitchell on 29/04/2012.
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

#include "CBEngine.h"

//  Virtual Table Store

static CBEngineVT * VTStore = NULL;
static int objectNum = 0;

//  Constructor

CBEngine * CBNewEngine(){
	CBEngine * self = malloc(sizeof(*self));
	CBAddVTToObject(CBGetObject(self), VTStore, CBCreateEngineVT);
	CBInitEngine(self);
	return self;
}

//  Virtual Table Creation

CBEngineVT * CBCreateEngineVT(){
	CBEngineVT * VT = malloc(sizeof(*VT));
	CBSetEngineVT(VT);
	return VT;
}
void CBSetEngineVT(CBEngineVT * VT){
	CBSetObjectVT((CBObjectVT *)VT);
	((CBObjectVT *)VT)->free = (void (*)(void *))CBFreeEngine;
}

//  Virtual Table Getter

CBEngineVT * CBGetEngineVT(CBEngine * self){
	return ((CBEngineVT *)(CBGetObject(self))->VT);
}

//  Object Getter

CBEngine * CBGetEngine(void * self){
	return self;
}

//  Initialiser

bool CBInitEngine(CBEngine * self){
	if (!CBInitObject(CBGetObject(self)))
		return false;
	self->errorReceived = CBDummyErrorReceived;
	// Assign satoshi key
	self->satoshiKey = CBNewByteArrayOfSize(65,self->events);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,0,0x4);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,1,0xfc);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,2,0x97);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,3,0x2);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,4,0x84);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,5,0x78);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,6,0x40);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,7,0xaa);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,8,0xf1);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,9,0x95);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,10,0xde);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,11,0x84);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,12,0x42);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,13,0xeb);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,14,0xec);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,15,0xed);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,16,0xf5);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,17,0xb0);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,18,0x95);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,19,0xcd);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,20,0xbb);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,21,0x9b);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,22,0xc7);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,23,0x16);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,24,0xbd);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,25,0xa9);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,26,0x11);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,27,0x9);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,28,0x71);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,29,0xb2);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,30,0x8a);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,31,0x49);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,32,0xe0);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,33,0xea);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,34,0xd8);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,35,0x56);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,36,0x4f);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,37,0xf0);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,38,0xdb);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,39,0x22);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,40,0x20);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,41,0x9e);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,42,0x3);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,43,0x74);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,44,0x78);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,45,0x2c);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,46,0x9);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,47,0x3b);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,48,0xb8);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,49,0x99);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,50,0x69);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,51,0x2d);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,52,0x52);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,53,0x4e);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,54,0x9d);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,55,0x6a);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,56,0x69);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,57,0x56);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,58,0xe7);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,59,0xc5);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,60,0xec);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,61,0xbc);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,62,0xd6);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,63,0x82);
	CBGetByteArrayVT(self->satoshiKey)->insertByte(self->satoshiKey,64,0x84);
	return true;
}

//  Destructor

void CBFreeEngine(CBEngine * self){
	CBFreeProcessEngine(self);
	CBFree();
}
void CBFreeProcessEngine(CBEngine * self){
	CBGetObjectVT(self->satoshiKey)->release(&self->satoshiKey);
	CBFreeProcessObject(CBGetObject(self));
}

//  Functions

void CBDummyErrorReceived(CBError error,char * err,...){
	
}
