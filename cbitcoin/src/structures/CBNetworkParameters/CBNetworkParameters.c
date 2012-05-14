//
//  CBNetworkParameters.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 29/04/2012.
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

#include "CBNetworkParameters.h"

//  Virtual Table Store

static CBNetworkParametersVT * VTStore = NULL;
static int objectNum = 0;

//  Constructor

CBNetworkParameters * CBNewNetworkParameters(){
	CBNetworkParameters * self = malloc(sizeof(*self));
	CBAddVTToObject((CBObject *)self, VTStore, CBCreateNetworkParametersVT);
	CBInitNetworkParameters(self);
	return self;
}

//  Virtual Table Creation

CBNetworkParametersVT * CBCreateNetworkParametersVT(){
	CBNetworkParametersVT * VT = malloc(sizeof(*VT));
	CBSetNetworkParametersVT(VT);
	return VT;
}
void CBSetNetworkParametersVT(CBNetworkParametersVT * VT){
	CBSetObjectVT((CBObjectVT *)VT);
	((CBObjectVT *)VT)->free = (void (*)(void *))CBFreeNetworkParameters;
}

//  Virtual Table Getter

CBNetworkParametersVT * CBGetNetworkParametersVT(void * self){
	return ((CBNetworkParametersVT *)(CBGetObject(self))->VT);
}

//  Object Getter

CBNetworkParameters * CBGetNetworkParameters(void * self){
	return self;
}

//  Initialiser

bool CBInitNetworkParameters(CBNetworkParameters * self){
	if (!CBInitObject(CBGetObject(self)))
		return false;
	return true;
}

//  Destructor

void CBFreeNetworkParameters(CBNetworkParameters * self){
	CBFreeProcessNetworkParameters(self);
	CBFree();
}
void CBFreeProcessNetworkParameters(CBNetworkParameters * self){
	CBFreeProcessObject(CBGetObject(self));
}

//  Functions



