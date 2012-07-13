//
//  CBChainDescriptor.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 11/07/2012.
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

#include "CBChainDescriptor.h"

//  Constructors

CBChainDescriptor * CBNewChainDescriptor(CBEvents * events){
	CBChainDescriptor * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeChainDescriptor;
	CBInitChainDescriptor(self,events);
	return self;
}
CBChainDescriptor * CBNewChainDescriptorFromData(CBByteArray * data,CBEvents * events){
	CBChainDescriptor * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeChainDescriptor;
	CBInitChainDescriptorFromData(self,data,events);
	return self;
}

//  Object Getter

CBChainDescriptor * CBGetChainDescriptor(void * self){
	return self;
}

//  Initialisers

bool CBInitChainDescriptor(CBChainDescriptor * self,CBEvents * events){
	self->hashNum = 0;
	self->hashes = NULL;
	if (!CBInitMessageByObject(CBGetMessage(self), events))
		return false;
	return true;
}
bool CBInitChainDescriptorFromData(CBChainDescriptor * self,CBByteArray * data,CBEvents * events){
	self->hashNum = 0;
	self->hashes = NULL;
	if (!CBInitMessageByData(CBGetMessage(self), data, events))
		return false;
	return true;
}

//  Destructor

void CBFreeChainDescriptor(void * vself){
	CBChainDescriptor * self = vself;
	for (u_int16_t x = 0; x < self->hashNum; x++) {
		CBReleaseObject(&self->hashes[x]);
	}
	free(self->hashes);
	CBFreeObject(self);
}

//  Functions

void CBChainDescriptorAddHash(CBChainDescriptor * self,CBByteArray * hash){
	CBChainDescriptorTakeHash(self,hash);
	CBRetainObject(hash);
}
u_int16_t CBChainDescriptorDeserialise(CBChainDescriptor * self){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (!bytes) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_NULL_BYTES,"Attempting to deserialise a CBChainDescriptor with no bytes.");
		return 0;
	}
	if (bytes->length < 33) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBChainDescriptor with less bytes than required for one hash.");
		return 0;
	}
	CBVarInt hashNum = CBVarIntDecode(bytes, 0);
	if (hashNum.val > 500) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBChainDescriptor with a var int over 500.");
		return 0;
	}
	if (bytes->length < hashNum.size + hashNum.val * 32) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_DESERIALISATION_BAD_BYTES,"Attempting to deserialise a CBChainDescriptor with less bytes than required for the hashes.");
		return 0;
	}
	// Deserialise each hash
	self->hashes = malloc(sizeof(*self->hashes) * (size_t)hashNum.val);
	self->hashNum = hashNum.val;
	u_int16_t cursor = hashNum.size;
	for (u_int16_t x = 0; x < self->hashNum; x++) {
		self->hashes[x] = CBNewByteArraySubReference(bytes, cursor, 32);
		cursor += 32;
	}
	return cursor;
}
u_int16_t CBChainDescriptorSerialise(CBChainDescriptor * self){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (!bytes) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_SERIALISATION_NULL_BYTES,"Attempting to serialise a CBChainDescriptor with no bytes.");
		return 0;
	}
	CBVarInt hashNum = CBVarIntFromUInt64(self->hashNum);
	if (bytes->length < hashNum.size + self->hashNum * 32) {
		CBGetMessage(self)->events->onErrorReceived(CB_ERROR_MESSAGE_SERIALISATION_BAD_BYTES,"Attempting to serialise a CBChainDescriptor with less bytes than required for the hashes.");
		return 0;
	}
	CBVarIntEncode(bytes, 0, hashNum);
	u_int16_t cursor = hashNum.size;
	for (u_int16_t x = 0; x < self->hashNum; x++) {
		CBByteArrayCopyByteArray(bytes, cursor, self->hashes[x]);
		CBByteArrayChangeReference(self->hashes[x], bytes, cursor);
		cursor += 32;
	}
	return cursor;
}
void CBChainDescriptorTakeHash(CBChainDescriptor * self,CBByteArray * hash){
	self->hashNum++;
	self->hashes = realloc(self->hashes, self->hashNum);
	self->hashes[self->hashNum-1] = hash;
}
