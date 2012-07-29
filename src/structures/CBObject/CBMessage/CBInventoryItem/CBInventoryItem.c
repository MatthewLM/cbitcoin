//
//  CBInventoryItem.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 10/07/2012.
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

#include "CBInventoryItem.h"

//  Constructors

CBInventoryItem * CBNewInventoryItem(CBInventoryItemType type,CBByteArray * hash,CBEvents * events){
	CBInventoryItem * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeInventoryItem;
	CBInitInventoryItem(self,type,hash,events);
	return self;
}
CBInventoryItem * CBNewInventoryItemFromData(CBByteArray * data,CBEvents * events){
	CBInventoryItem * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeInventoryItem;
	CBInitInventoryItemFromData(self,data,events);
	return self;
}

//  Object Getter

CBInventoryItem * CBGetInventoryItem(void * self){
	return self;
}

//  Initialisers

bool CBInitInventoryItem(CBInventoryItem * self,CBInventoryItemType type,CBByteArray * hash,CBEvents * events){
	self->type = type;
	self->hash = hash;
	CBRetainObject(hash);
	if (!CBInitMessageByObject(CBGetMessage(self), events))
		return false;
	return true;
}
bool CBInitInventoryItemFromData(CBInventoryItem * self,CBByteArray * data,CBEvents * events){
	self->hash = NULL;
	if (!CBInitMessageByData(CBGetMessage(self), data, events))
		return false;
	return true;
}

//  Destructor

void CBFreeInventoryItem(void * vself){
	CBInventoryItem * self = vself;
	CBReleaseObject(&self->hash);
	CBFreeMessage(self);
}

//  Functions

u_int32_t CBInventoryItemDeserialise(CBInventoryItem * self){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (!bytes) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_NULL_BYTES,"Attempting to deserialise a CBInventoryItem with no bytes.");
		return 0;
	}
	if (bytes->length < 36) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBInventoryItem with less than 36 bytes.");
		return 0;
	}
	self->type = CBByteArrayReadInt32(bytes, 0);
	self->hash = CBByteArraySubReference(bytes, 4, 32);
	return 36;
}
u_int32_t CBInventoryItemSerialise(CBInventoryItem * self){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (!bytes) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_SERIALISATION_NULL_BYTES,"Attempting to serialise a CBInventoryItem with no bytes.");
		return 0;
	}
	if (bytes->length < 36) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_SERIALISATION_BAD_BYTES,"Attempting to serialise a CBInventoryItem with less than 36 bytes.");
		return 0;
	}
	CBByteArraySetInt32(bytes, 0, self->type);
	CBByteArrayCopyByteArray(bytes, 4, self->hash);
	CBByteArrayChangeReference(self->hash, bytes, 4);
	return 36;
}

