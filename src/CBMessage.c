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
	CBGetObject(self)->free = CBFreeMessage;
	CBInitMessageByObject(self);
	return self;
}

//  Initialiser

void CBInitMessageByObject(CBMessage * self){
	CBInitObject(CBGetObject(self));
	self->bytes = NULL;
	self->expectResponse = false;
	self->serialised = false;
}
void CBInitMessageByData(CBMessage * self, CBByteArray * data){
	CBInitObject(CBGetObject(self));
	self->bytes = data;
	CBRetainObject(data); // Retain data for this object.
	self->expectResponse = false;
	self->serialised = true;
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

