//
//  CBMessage.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 27/04/2012.
//  Copyright (c) 2012 Matthew Mitchell
//  
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

//  SEE HEADER FILE FOR DOCUMENTATION

#include "CBMessage.h"

//  Constructor

CBMessage * CBNewMessageByObject(){
	CBMessage * self = malloc(sizeof(*self));
	if (NOT self) {
		CBLogError("Cannot allocate %i bytes of memory in CBNewMessageByObject\n", sizeof(*self));
		return NULL;
	}
	CBGetObject(self)->free = CBFreeMessage;
	if (CBInitMessageByObject(self))
		return self;
	free(self);
	return NULL;
}

//  Object Getter

CBMessage * CBGetMessage(void * self){
	return self;
}

//  Initialiser

bool CBInitMessageByObject(CBMessage * self){
	if (NOT CBInitObject(CBGetObject(self)))
		return false;
	self->bytes = NULL;
	self->expectResponse = false;
	self->serialised = false;
	return true;
}
bool CBInitMessageByData(CBMessage * self, CBByteArray * data){
	if (NOT CBInitObject(CBGetObject(self)))
		return false;
	self->bytes = data;
	CBRetainObject(data); // Retain data for this object.
	self->expectResponse = false;
	self->serialised = true;
	return true;
}

//  Destructor

void CBDestroyMessage(void * vself){
	CBMessage * self = vself;
	if (self->bytes) CBReleaseObject(self->bytes);
}
void CBFreeMessage(void * self){
	CBDestroyMessage(self);
	free(self);
}

//  Functions

