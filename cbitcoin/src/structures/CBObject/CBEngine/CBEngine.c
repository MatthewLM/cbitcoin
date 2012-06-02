//
//  CBEngine.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 28/04/2012.
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

#include "CBEngine.h"

//  Virtual Table Store

static void * VTStore = NULL;
static int objectNum = 0;

//  Constructor

CBEngine * CBNewEngine(){
	CBEngine * self = malloc(sizeof(*self));
	objectNum++;
	CBAddVTToObject(CBGetObject(self), &VTStore, CBCreateEngineVT);
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
	self->events.onErrorReceived = CBDummyErrorReceived;
	// Assign satoshi key
	self->satoshiKey = CBNewByteArrayOfSize(65,&self->events);
	// ??? Implement Satoshi key...
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
