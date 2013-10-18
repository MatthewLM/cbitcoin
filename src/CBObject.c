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

//  Initialiser

void CBInitObject(CBObject * self, bool useMutex){
	self->references = 1;
	self->usesMutex = useMutex;
	if (useMutex)
		CBNewMutex(&((CBObjectMutex *)self)->refMutex);
}

//  Functions

void CBReleaseObject(void * self){
	CBObject * obj = self;
	CBDepObject refMutex;
	bool usesMutex = obj->usesMutex;
	if (usesMutex){
		refMutex = ((CBObjectMutex *)self)->refMutex;
		CBMutexLock(refMutex);
	}
	// Decrement reference counter. Free if no more references.
	obj->references--;
	if (obj->references < 1){
		obj->free(obj);
		if (usesMutex) {
			CBMutexUnlock(refMutex);
			CBFreeMutex(refMutex);
		}
	}else if (usesMutex)
		CBMutexUnlock(refMutex);
}
void CBRetainObject(void * self){
	// Increment reference counter.
	CBObject * obj = self;
	if (obj->usesMutex){
		CBDepObject mutex = ((CBObjectMutex *)self)->refMutex;
		CBMutexLock(mutex);
		obj->references++;
		CBMutexUnlock(mutex);
	}else
		obj->references++;
}
