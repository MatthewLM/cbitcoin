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

CBInventoryItem * CBNewInventoryItem(CBInventoryItemType type,CBByteArray * hash,void (*onErrorReceived)(CBError error,char *,...)){
	CBInventoryItem * self = malloc(sizeof(*self));
	if (NOT self) {
		onErrorReceived(CB_ERROR_OUT_OF_MEMORY,"Cannot allocate %i bytes of memory in CBNewInventoryItem\n",sizeof(*self));
		return NULL;
	}
	CBGetObject(self)->free = CBFreeInventoryItem;
	if(CBInitInventoryItem(self,type,hash,onErrorReceived))
		return self;
	free(self);
	return NULL;
}
CBInventoryItem * CBNewInventoryItemFromData(CBByteArray * data,void (*onErrorReceived)(CBError error,char *,...)){
	CBInventoryItem * self = malloc(sizeof(*self));
	if (NOT self) {
		onErrorReceived(CB_ERROR_OUT_OF_MEMORY,"Cannot allocate %i bytes of memory in CBNewInventoryItemFromData\n",sizeof(*self));
		return NULL;
	}
	CBGetObject(self)->free = CBFreeInventoryItem;
	if(CBInitInventoryItemFromData(self,data,onErrorReceived))
		return self;
	free(self);
	return NULL;
}

//  Object Getter

CBInventoryItem * CBGetInventoryItem(void * self){
	return self;
}

//  Initialisers

bool CBInitInventoryItem(CBInventoryItem * self,CBInventoryItemType type,CBByteArray * hash,void (*onErrorReceived)(CBError error,char *,...)){
	self->type = type;
	self->hash = hash;
	CBRetainObject(hash);
	if (NOT CBInitMessageByObject(CBGetMessage(self), onErrorReceived))
		return false;
	return true;
}
bool CBInitInventoryItemFromData(CBInventoryItem * self,CBByteArray * data,void (*onErrorReceived)(CBError error,char *,...)){
	self->hash = NULL;
	if (NOT CBInitMessageByData(CBGetMessage(self), data, onErrorReceived))
		return false;
	return true;
}

//  Destructor

void CBFreeInventoryItem(void * vself){
	CBInventoryItem * self = vself;
	CBReleaseObject(self->hash);
	CBFreeMessage(self);
}

//  Functions

uint32_t CBInventoryItemDeserialise(CBInventoryItem * self){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (NOT bytes) {
		CBGetMessage(self)->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_NULL_BYTES,"Attempting to deserialise a CBInventoryItem with no bytes.");
		return 0;
	}
	if (bytes->length < 36) {
		CBGetMessage(self)->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBInventoryItem with less than 36 bytes.");
		return 0;
	}
	self->type = CBByteArrayReadInt32(bytes, 0);
	self->hash = CBByteArraySubReference(bytes, 4, 32);
	if (NOT self->hash)
		return 0;
	return 36;
}
uint32_t CBInventoryItemSerialise(CBInventoryItem * self){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (NOT bytes) {
		CBGetMessage(self)->onErrorReceived(CB_ERROR_MESSAGE_SERIALISATION_NULL_BYTES,"Attempting to serialise a CBInventoryItem with no bytes.");
		return 0;
	}
	if (bytes->length < 36) {
		CBGetMessage(self)->onErrorReceived(CB_ERROR_MESSAGE_SERIALISATION_BAD_BYTES,"Attempting to serialise a CBInventoryItem with less than 36 bytes.");
		return 0;
	}
	CBByteArraySetInt32(bytes, 0, self->type);
	CBByteArrayCopyByteArray(bytes, 4, self->hash);
	CBByteArrayChangeReference(self->hash, bytes, 4);
	return 36;
}

