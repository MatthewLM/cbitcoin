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

//  SEE HEADER FILE FOR DOCUMENTATION

#include "CBNetworkParameters.h"

//  Constructor

CBNetworkParameters * CBNewNetworkParameters(){
	CBNetworkParameters * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeNetworkParameters;
	CBInitNetworkParameters(self);
	return self;
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

void CBFreeNetworkParameters(void * self){
	CBFreeProcessObject(CBGetObject(self));
}

//  Functions



