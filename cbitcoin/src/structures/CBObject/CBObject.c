//
//  CBObject.c
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

#include "CBObject.h"

//  Virtual Table Store

static CBObjectVT * VTStore = NULL;
static int objectNum = 0;

//  Constructor

CBObject * CBNewObject(){
	CBObject * self = malloc(sizeof(*self));
	objectNum++;
	CBAddVTToObject(self, VTStore, &CBCreateObjectVT);
	CBInitObject(self);
	return self;
}

//  Virtual Table Creation

CBObjectVT * CBCreateObjectVT(){
	CBObjectVT * VT = malloc(sizeof(*VT));
	CBSetObjectVT(VT);
	return VT;
}
void CBSetObjectVT(CBObjectVT * VT){
	VT->free = (void (*)(void *))CBFreeObject;
	VT->release = (void (*)(void *))CBReleaseObject;
	VT->retain = (void (*)(void *))CBRetainObject;
}

//  Virtual Table Getter

CBObjectVT * CBGetObjectVT(void * self){
	return ((CBObjectVT *)((CBObject *)self)->VT);
}

//  Object Getter

CBObject * CBGetObject(void * self){
	return self;
}

//  Initialiser

bool CBInitObject(CBObject * self){
	self->references = 1;
	return true;
}

//  Destructor

void CBFreeObject(CBObject * self){
	CBFreeProcessObject(self);
	CBFree();
}
void CBFreeProcessObject(CBObject * self){
	free(self);
}

//  Functions

void CBAddVTToObject(CBObject * self,void * VTStore,void * getVT){
	// Set the methods pointer and create a new virtual table store if needed by getting the functions with get_methods
	if (!VTStore) {
		VTStore = ((void * (*)())getVT)();
	}
	self->VT = VTStore;
}
void CBReleaseObject(CBObject ** self){
	// Decrement reference counter. Free if no more references.
	(*self)->references--;
	if ((*self)->references < 1){
		CBGetObjectVT(*self)->free(*self); // Remembering to dereference self from CBObject ** to CBOject *
	}
	*self = NULL; //Assign NULL to the pointer now that the object is released from the pointer's control.
}
void CBRetainObject(CBObject * self){
	// Increment reference counter.
	self->references++;
}
