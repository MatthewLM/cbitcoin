//
//  CBObject.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 28/04/2012.
//  Copyright (c) 2012 Matthew Mitchell
//  
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

//  SEE HEADER FILE FOR DOCUMENTATION

#include "CBObject.h"

//  Object Getter

CBObject * CBGetObject(void * self){
	return self;
}

//  Initialiser

void CBInitObject(CBObject * self){
	self->references = 1;
}

//  Functions

void CBReleaseObject(void * self){
	CBObject * obj = self;
	// Decrement reference counter. Free if no more references.
	obj->references--;
	if (obj->references < 1){
		obj->free(obj);
	}
}
void CBRetainObject(void * self){
	// Increment reference counter.
	((CBObject *)self)->references++;
}
