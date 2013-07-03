//
//  CBAccounter.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 01/07/2013.
//  Copyright (c) 2012 Matthew Mitchell
//  
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

//  SEE HEADER FILE FOR DOCUMENTATION 

#include "CBAccounter.h"

//  Constructor

CBAccounter * CBNewAccounter(char * dataDir){
	CBAccounter * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeAccounter;
	if(CBInitAccounter(self, dataDir))
		return self;
	free(self);
	return NULL;
}

//  Object Getter

CBAccounter * CBGetAccounter(void * self){
	return self;
}

//  Initialiser

bool CBInitAccounter(CBAccounter * self, char * dataDir){
	CBInitObject(CBGetObject(self));
	// Create the storage object
	if (CBNewAccounterStorage(&self->storage, dataDir)) {
		CBLogError("Could not create an accounter storage object.");
		return false;
	}
	return true;
}

//  Destructor

void CBDestroyAccounter(void * self){
	CBFreeAccounterStorage(CBGetAccounter(self)->storage);
}

void CBFreeAccounter(void * self){
	CBDestroyAccounter(self);
	free(self);
}

//  Functions

