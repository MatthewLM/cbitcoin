//
//  CBTransaction.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 02/05/2012.
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

#include "CBTransaction.h"

//  Virtual Table Store

static CBTransactionVT * VTStore = NULL;
static int objectNum = 0;

//  Constructor

CBTransaction * CBNewTransaction(){
	CBTransaction * self = malloc(sizeof(*self));
	CBAddVTToObject(CBGetObject(self), VTStore, CBCreateTransactionVT);
	CBInitTransaction(self);
	return self;
}

//  Virtual Table Creation

CBTransactionVT * CBCreateTransactionVT(){
	CBTransactionVT * VT = malloc(sizeof(*VT));
	CBSetTransactionVT(VT);
	return VT;
}
void CBSetTransactionVT(CBTransactionVT * VT){
	CBSetMessageVT((CBMessageVT *)VT);
	((CBObjectVT *)VT)->free = (void (*)(void *))CBFreeTransaction;
}

//  Virtual Table Getter

CBTransactionVT * CBGetTransactionVT(void * self){
	return ((CBTransactionVT *)(CBGetObject(self))->VT);
}

//  Object Getter

CBTransaction * CBGetTransaction(void * self){
	return self;
}

//  Initialiser

bool CBInitTransaction(CBTransaction * self){
	if (!CBInitObject(CBGetObject(self)))
		return false;
	return true;
}

//  Destructor

void CBFreeTransaction(CBTransaction * self){
	CBFreeProcessTransaction(self);
	CBFree();
}
void CBFreeProcessTransaction(CBTransaction * self){
	CBFreeProcessObject(CBGetObject(self));
}

//  Functions

