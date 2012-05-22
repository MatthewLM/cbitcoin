//
//  CBTestStruct.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 29/04/2012.
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

#include "CBTestStruct.h"

//  Virtual Table Store

static CBTestStructVT * VTStore = NULL;
static int objectNum = 0;

//  Constructor

CBTestStruct * CBNewTestStruct(int a, int b){
	CBTestStruct * self = malloc(sizeof(*self));
	CBAddVTToObject((CBObject *)self, &VTStore, CBCreateTestStructVT);
	objectNum++;
	CBInitTestStruct(self,a,b);
	return self;
}

//  Virtual Table Creation

CBTestStructVT * CBCreateTestStructVT(){
	CBTestStructVT * VT = malloc(sizeof(*VT));
	CBSetTestStructVT(VT);
	return VT;
}
void CBSetTestStructVT(CBTestStructVT * VT){
	CBSetObjectVT((CBObjectVT *)VT);
	((CBObjectVT *)VT)->free = (void (*)(void *))CBFreeTestStruct;
	VT->add = (int (*)(void *))CBTestStrustAdd;
}

//  Virtual Table Getter

CBTestStructVT * CBGetTestStructVT(void * self){
	return ((CBTestStructVT *)((CBObject *)self)->VT);
}

//  Object Getter

CBTestStruct * CBGetTestStruct(void * self){
	return self;
}

//  Initialiser

bool CBInitTestStruct(CBTestStruct * self,int a, int b){
	if (!CBInitObject(CBGetObject(self)))
		return false;
	self->a = a;
	self->b = b;
	return true;
}

//  Destructor

void CBFreeTestStruct(CBTestStruct * self){
	CBFreeProcessTestStruct(self);
	CBFree();
}
void CBFreeProcessTestStruct(CBTestStruct * self){
	printf("Freeing!\n");
	CBFreeProcessObject(CBGetObject(self));
}

//  Functions

int CBTestStrustAdd(CBTestStruct * self){
	return self->a + self->b;
}

