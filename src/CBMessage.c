//
//  CBMessage.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 27/04/2012.
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

void CBFreeMessage(void * vself){
	CBMessage * self = vself;
	if (self->bytes) CBReleaseObject(self->bytes);
	CBFreeObject(self);
}

//  Functions

