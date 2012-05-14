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

#include "CBAddress.h"

//  Virtual Table Store

static CBAddressVT * VTStore = NULL;
static int objectNum = 0;

//  Constructor

CBAddress * CBNewAddress(){
	CBAddress * self = malloc(sizeof(*self));
	CBAddVTToObject(CBGetObject(self), VTStore, CBCreateAddressVT);
	CBInitAddress(self);
	return self;
}

//  Virtual Table Creation

CBAddressVT * CBCreateAddressVT(){
	CBAddressVT * VT = malloc(sizeof(*VT));
	CBSetAddressVT(VT);
	return VT;
}
void CBSetAddressVT(CBAddressVT * VT){
	CBSetObjectVT((CBObjectVT *)VT);
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

bool CBInitAddress(CBAddress * self){
	if (!CBInitObject(CBGetObject(self)))
		return false;
	return true;
}

//  Destructor

void CBFreeAddress(CBAddress * self){
	CBFreeProcessAddress(self);
	CBFree();
}
void CBFreeProcessAddress(CBAddress * self){
	CBFreeProcessObject(CBGetObject(self));
}

//  Functions

