//
//  CBTransactionInput.c
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

//  SEE HEADER FILE FOR DOCUMENTATION

#include "CBTransactionInput.h"

//  Virtual Table Store

static void * VTStore = NULL;
static int objectNum = 0;

//  Constructor

CBTransactionInput * CBNewTransactionInput(CBNetworkParameters * params, void * parentTransaction, CBByteArray * scriptData,CBSha256Hash * outPointerHash,u_int32_t outPointerIndex,CBEvents * events){
	CBTransactionInput * self = malloc(sizeof(*self));
	objectNum++;
	CBAddVTToObject(CBGetObject(self), &VTStore, CBCreateTransactionInputVT);
	CBInitTransactionInput(self,params,parentTransaction,scriptData,outPointerHash,outPointerIndex,events);
	return self;
}

//  Virtual Table Creation

CBTransactionInputVT * CBCreateTransactionInputVT(){
	CBTransactionInputVT * VT = malloc(sizeof(*VT));
	CBSetTransactionInputVT(VT);
	return VT;
}
void CBSetTransactionInputVT(CBTransactionInputVT * VT){
	CBSetMessageVT((CBMessageVT *)VT);
	((CBObjectVT *)VT)->free = (void (*)(void *))CBFreeTransactionInput;
}

//  Virtual Table Getter

CBTransactionInputVT * CBGetTransactionInputVT(void * self){
	return ((CBTransactionInputVT *)(CBGetObject(self))->VT);
}

//  Object Getter

CBTransactionInput * CBGetTransactionInput(void * self){
	return self;
}

//  Initialiser

bool CBInitTransactionInput(CBTransactionInput * self,CBNetworkParameters * params, void * parentTransaction, CBByteArray * scriptData,CBSha256Hash * outPointerHash,u_int32_t outPointerIndex,CBEvents * events){
	if (!CBInitObject(CBGetObject(self)))
		return false;
	self->scriptData = scriptData;
	self->outPointerHash = CBNewEmptySha256Hash(events);
	self->outPointerIndex = -1;
	self->sequence = CB_TRANSACTION_INPUT_FINAL;
	self->parentTransaction = parentTransaction;
	CBGetMessage(self)->length = 40 + (scriptData == NULL ? 1 : CBVarIntSizeOf(scriptData->length) + scriptData->length);
	return true;
}

//  Destructor

void CBFreeTransactionInput(CBTransactionInput * self){
	CBFreeProcessTransactionInput(self);
	CBFree();
}
void CBFreeProcessTransactionInput(CBTransactionInput * self){
	CBFreeProcessObject(CBGetObject(self));
}

//  Functions

