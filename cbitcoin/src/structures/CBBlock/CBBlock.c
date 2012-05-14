//
//  CBBlock.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 01/05/2012.
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

#include "CBBlock.h"

//  Virtual Table Store

static CBBlockVT * VTStore = NULL;
static int objectNum = 0;

//  Constructor

CBBlock * CBNewBlock(){
	CBBlock * self = malloc(sizeof(*self));
	CBAddVTToObject(CBGetObject(self), VTStore, CBCreateBlockVT);
	CBInitBlock(self);
	return self;
}

//  Virtual Table Creation

CBBlockVT * CBCreateBlockVT(){
	CBBlockVT * VT = malloc(sizeof(*VT));
	CBSetBlockVT(VT);
	return VT;
}
void CBSetBlockVT(CBBlockVT * VT){
	CBSetMessageVT((CBMessageVT *)VT);
	((CBObjectVT *)VT)->free = (void (*)(void *))CBFreeBlock;
}

//  Virtual Table Getter

CBBlockVT * CBGetBlockVT(void * self){
	return ((CBBlockVT *)(CBGetObject(self))->VT);
}

//  Object Getter

CBBlock * CBGetBlock(void * self){
	return self;
}

//  Initialiser

bool CBInitBlock(CBBlock * self){
	if (!CBInitObject(CBGetObject(self)))
		return false;
	return true;
}

//  Destructor

void CBFreeBlock(CBBlock * self){
	CBFreeProcessBlock(self);
	CBFree();
}
void CBFreeProcessBlock(CBBlock * self){
	CBFreeProcessObject(CBGetObject(self));
}

//  Functions

