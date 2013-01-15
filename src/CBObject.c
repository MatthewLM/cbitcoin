//
//  CBObject.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 28/04/2012.
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

#include "CBObject.h"

//  Constructor

CBObject * CBNewObject(){
	CBObject * self = malloc(sizeof(*self));
	if (NOT self)
		return NULL;
	self->free = CBFreeObject;
	if(CBInitObject(self))
		return self;
	free(self);
	return NULL;
}

//  Object Getter

CBObject * CBGetObject(void * self){
	return self;
}

//  Initialiser

bool CBInitObject(CBObject * self){
	self->references = 1;
	return true;
}

//  Destructor

void CBFreeObject(void * self){
	free(self);
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
