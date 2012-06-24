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

CBMessage * CBNewMessageByObject(void * params,CBEvents * events){
	CBMessage * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeMessage;
	if (CBInitMessageByObject(self,params,events))
		return self;
	return NULL; //Initialisation failure
}

//  Object Getter

CBMessage * CBGetMessage(void * self){
	return self;
}

//  Initialiser

bool CBInitMessageByObject(CBMessage * self,void * params,CBEvents * events){
	if (!CBInitObject(CBGetObject(self)))
		return false;
	self->params = params;
	self->bytes = NULL;
	self->events = events;
	return true;
}
bool CBInitMessageByData(CBMessage * self,void * params,CBByteArray * data,CBEvents * events){
	if (!CBInitObject(CBGetObject(self)))
		return false;
	self->params = params;
	self->bytes = data;
	CBRetainObject(data); // Retain data for this object.
	self->events = events;
	return true;
}

//  Destructor

void CBFreeMessage(void * self){
	CBFreeProcessMessage(self);
}
void CBFreeProcessMessage(CBMessage * self){
	if (self->bytes) CBReleaseObject(&self->bytes);
	CBFreeProcessObject(CBGetObject(self));
}

//  Functions


