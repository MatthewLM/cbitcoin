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

//  SEE HEADER FILE FOR DOCUMENTATION

#include "CBTransactionOutput.h"

//  Virtual Table Store

static void * VTStore = NULL;
static int objectNum = 0;

//  Constructors

CBTransactionOutput * CBNewTransactionOutput(CBNetworkParameters * params, CBTransaction * parent, u_int64_t value, CBByteArray * scriptBytes,u_int32_t protocolVersion,bool serialiseCache,CBEvents * events){
	CBTransactionOutput * self = malloc(sizeof(*self));
	objectNum++;
	CBAddVTToObject(CBGetObject(self), &VTStore, CBCreateTransactionOutputVT);
	CBInitTransactionOutput(self,params,parent,value,scriptBytes,protocolVersion,serialiseCache,events);
	return self;
}
CBTransactionOutput * CBNewTransactionOutputFromData(CBNetworkParameters * params, CBTransaction * parent, CBByteArray * bytes,u_int32_t protocolVersion,bool serialiseCache,CBEvents * events){
	CBTransactionOutput * self = malloc(sizeof(*self));
	objectNum++;
	CBAddVTToObject(CBGetObject(self), &VTStore, CBCreateTransactionOutputVT);
	CBInitTransactionOutputByData(self,params,parent,bytes,protocolVersion,serialiseCache,events);
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

//  Initialisers

bool CBInitTransactionOutput(CBTransactionOutput * self,CBNetworkParameters * params, CBTransaction * parent, u_int64_t value, CBByteArray * scriptBytes,u_int32_t protocolVersion,bool serialiseCache,CBEvents * events){
	self->scriptObject = CBNewScript(params, scriptBytes, events);
	self->value = value;
	if (!CBInitMessageByObject(CBGetMessage(self), params, 8 + CBVarIntSizeOf(scriptBytes->length) + scriptBytes->length, protocolVersion, serialiseCache, events))
		return false;
	return true;
}
bool CBInitTransactionOutputByData(CBTransactionOutput * self,CBNetworkParameters * params, CBTransaction * parent, CBByteArray * bytes, u_int32_t protocolVersion,bool serialiseCache,CBEvents * events){
	if (!CBInitMessageByData(CBGetMessage(self), params, bytes, protocolVersion, serialiseCache, events))
		return false;
	self->parentTransaction = parent;
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

