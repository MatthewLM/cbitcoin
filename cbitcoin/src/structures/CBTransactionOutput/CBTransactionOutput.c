//
//  CBTransactionOutput.c
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

#include "CBTransactionOutput.h"

//  Virtual Table Store

static CBTransactionOutputVT * VTStore = NULL;
static int objectNum = 0;

//  Constructors

CBTransactionOutput * CBNewTransactionOutputDeserialisation(CBNetworkParameters * params, CBTransaction * parent, CBByteArray * payload,u_int32_t offset,bool parseLazy,bool parseRetain){
	CBTransactionOutput * self = malloc(sizeof(*self));
	CBAddVTToObject(CBGetObject(self), VTStore, CBCreateTransactionOutputVT);
	CBInitTransactionOutputDeserialisation(self,params,parent,payload,offset,parseLazy,parseRetain);
	return self;
}

//  Virtual Table Creation

CBTransactionOutputVT * CBCreateTransactionOutputVT(){
	CBTransactionOutputVT * VT = malloc(sizeof(*VT));
	CBSetTransactionOutputVT(VT);
	return VT;
}
void CBSetTransactionOutputVT(CBTransactionOutputVT * VT){
	CBSetMessageVT((CBMessageVT *)VT);
	((CBObjectVT *)VT)->free = (void (*)(void *))CBFreeTransactionOutput;
}

//  Virtual Table Getter

CBTransactionOutputVT * CBGetTransactionOutputVT(void * self){
	return ((CBTransactionOutputVT *)(CBGetObject(self))->VT);
}

//  Object Getter

CBTransactionOutput * CBGetTransactionOutput(void * self){
	return self;
}

//  Initialiser

bool CBInitTransactionOutputDeserialisation(CBTransactionOutput * self,CBNetworkParameters * params, CBTransaction * parent, CBByteArray * payload, u_int32_t offset,bool parseLazy,bool parseRetain){
	if (!CBInitMessage(CBGetMessage(self), params, payload, offset, CB_PROTOCOL_VERSION, parseLazy, parseRetain, CB_BYTE_ARRAY_UNKNOWN_LENGTH))
		return false;
	return true;
}

//  Destructor

void CBFreeTransactionOutput(CBTransactionOutput * self){
	CBFreeProcessTransactionOutput(self);
	CBFree();
}
void CBFreeProcessTransactionOutput(CBTransactionOutput * self){
	CBFreeProcessObject(CBGetObject(self));
}

//  Functions

