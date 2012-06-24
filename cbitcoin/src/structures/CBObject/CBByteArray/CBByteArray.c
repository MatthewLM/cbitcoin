//
//  CBByteArray.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 30/04/2012.
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

#include "CBByteArray.h"

//  Constructor

CBByteArray * CBNewByteArrayOfSize(u_int32_t size,CBEvents * events){
	CBByteArray * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeByteArray;
	CBInitByteArrayOfSize(self,size,events);
	return self;
}
CBByteArray * CBNewByteArraySubReference(CBByteArray * ref,u_int32_t offset,u_int32_t length){
	CBByteArray * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeByteArray;
	CBInitByteArraySubReference(self, ref, offset, length);
	return self;
}
CBByteArray * CBNewByteArrayWithData(u_int8_t * data,u_int32_t size,CBEvents * events){
	CBByteArray * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeByteArray;
	CBInitByteArrayWithData(self, data, size, events);
	return self;
}
CBByteArray * CBNewByteArrayWithDataCopy(u_int8_t * data,u_int32_t size,CBEvents * events){
	CBByteArray * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeByteArray;
	CBInitByteArrayWithDataCopy(self, data, size, events);
	return self;
}

//  Object Getter

CBByteArray * CBGetByteArray(void * self){
	return self;
}

//  Initialisers

bool CBInitByteArrayOfSize(CBByteArray * self,u_int32_t size,CBEvents * events){
	if (!CBInitObject(CBGetObject(self)))
		return false;
	self->events = events;
	self->length = size;
	self->sharedData = malloc(sizeof(*self->sharedData));
	self->sharedData->data = malloc(size);
	self->sharedData->references = 1;
	self->offset = 0;
	return true;
}
bool CBInitByteArraySubReference(CBByteArray * self,CBByteArray * ref,u_int32_t offset,u_int32_t length){
	if (!CBInitObject(CBGetObject(self)))
		return false;
	self->events = ref->events;
	self->sharedData = ref->sharedData;
	self->sharedData->references++; // Since a new reference to the shared data is being made, an increase in the reference count must be made.
	self->length = length ? length : ref->length; // If length is 0, set to the reference length.
	self->offset = ref->offset + offset;
	return true;
}
bool CBInitByteArrayWithData(CBByteArray * self,u_int8_t * data,u_int32_t size,CBEvents * events){
	if (!CBInitObject(CBGetObject(self)))
		return false;
	self->events = events;
	self->sharedData = malloc(sizeof(*self->sharedData));
	self->sharedData->data = data;
	self->sharedData->references = 1;
	self->length = size;
	self->offset = 0;
	return true;
}
bool CBInitByteArrayWithDataCopy(CBByteArray * self,u_int8_t * data,u_int32_t size,CBEvents * events){
	if (!CBInitObject(CBGetObject(self)))
		return false;
	self->events = events;
	self->sharedData = malloc(sizeof(*self->sharedData));
	self->sharedData->data = memmove(malloc(size),data,size);
	self->sharedData->references = 1;
	self->length = size;
	self->offset = 0;
	return true;
}

//  Destructor

void CBFreeByteArray(void * self){
	CBByteArrayReleaseSharedData(self);
	CBFreeProcessObject(CBGetObject(self));
}
void CBByteArrayReleaseSharedData(CBByteArray * self){
	self->sharedData->references--;
	if (self->sharedData->references < 1) {
		// Shared data now owned by nothing so free it 
		free(self->sharedData->data);
		free(self->sharedData);
	}
}

//  Functions

CBByteArray * CBByteArrayCopy(CBByteArray * self){
	CBByteArray * new = CBNewByteArrayOfSize(self->length,self->events);
	memmove(new->sharedData->data,self->sharedData->data + self->offset,self->length);
	return new;
}
void CBByteArrayCopyByteArray(CBByteArray * self,u_int32_t writeOffset,CBByteArray * source){
	memmove(self->sharedData->data + self->offset + writeOffset,source->sharedData->data + source->offset,source->length);
}
void CBByteArrayCopySubByteArray(CBByteArray * self,u_int32_t writeOffset,CBByteArray * source,u_int32_t readOffset,u_int32_t length){
	memmove(self->sharedData->data + self->offset + writeOffset,source->sharedData->data + source->offset + readOffset,length);
}
bool CBByteArrayEquals(CBByteArray * self,CBByteArray * second){
	if (self->length != second->length)
		return false;
	return memcmp(CBByteArrayGetData(self), CBByteArrayGetData(second), self->length) == 0;
}
u_int8_t CBByteArrayGetByte(CBByteArray * self,u_int32_t index){
	return self->sharedData->data[self->offset+index];
}
u_int8_t * CBByteArrayGetData(CBByteArray * self){
	return self->sharedData->data + self->offset;
}
u_int8_t CBByteArrayGetLastByte(CBByteArray * self){
	return self->sharedData->data[self->offset+self->length];
}
void CBByteArraySetByte(CBByteArray * self,u_int32_t index,u_int8_t byte){
	self->sharedData->data[self->offset+index] = byte;
}
void CBByteArraySetBytes(CBByteArray * self,u_int32_t index,u_int8_t * bytes,u_int32_t length){
	memmove(self->sharedData->data + self->offset + index, bytes, length);
}
void CBByteArraySetUInt16(CBByteArray * self,u_int32_t offset,u_int16_t integer){
	self->sharedData->data[self->offset+offset] = integer;
	self->sharedData->data[self->offset+offset + 1] = integer >> 8;
}
void CBByteArraySetUInt32(CBByteArray * self,u_int32_t offset,u_int32_t integer){
	self->sharedData->data[self->offset+offset] = integer;
	self->sharedData->data[self->offset+offset + 1] = integer >> 8;
	self->sharedData->data[self->offset+offset + 2] = integer >> 16;
	self->sharedData->data[self->offset+offset + 3] = integer >> 24;
}
void CBByteArraySetUInt64(CBByteArray * self,u_int32_t offset,u_int64_t integer){
	self->sharedData->data[self->offset+offset] = integer;
	self->sharedData->data[self->offset+offset + 1] = integer >> 8;
	self->sharedData->data[self->offset+offset + 2] = integer >> 16;
	self->sharedData->data[self->offset+offset + 3] = integer >> 24;
	self->sharedData->data[self->offset+offset + 4] = integer >> 32;
	self->sharedData->data[self->offset+offset + 5] = integer >> 40;
	self->sharedData->data[self->offset+offset + 6] = integer >> 48;
	self->sharedData->data[self->offset+offset + 7] = integer >> 56;
}
u_int16_t CBByteArrayReadUInt16(CBByteArray * self,u_int32_t offset){
	u_int16_t result = self->sharedData->data[self->offset+offset];
	result |= (u_int16_t)self->sharedData->data[self->offset+offset + 1] << 8;
	return result;
}
u_int32_t CBByteArrayReadUInt32(CBByteArray * self,u_int32_t offset){
	u_int32_t result = self->sharedData->data[self->offset+offset];
	result |= (u_int32_t)self->sharedData->data[self->offset+offset + 1] << 8;
	result |= (u_int32_t)self->sharedData->data[self->offset+offset + 2] << 16;
	result |= (u_int32_t)self->sharedData->data[self->offset+offset + 3] << 24;
	return result;
}
u_int64_t CBByteArrayReadUInt64(CBByteArray * self,u_int32_t offset){
	u_int64_t result = self->sharedData->data[self->offset+offset];
	result |= (u_int64_t)self->sharedData->data[self->offset+offset + 1] << 8;
	result |= (u_int64_t)self->sharedData->data[self->offset+offset + 2] << 16;
	result |= (u_int64_t)self->sharedData->data[self->offset+offset + 3] << 24;
	result |= (u_int64_t)self->sharedData->data[self->offset+offset + 4] << 32;
	result |= (u_int64_t)self->sharedData->data[self->offset+offset + 5] << 40;
	result |= (u_int64_t)self->sharedData->data[self->offset+offset + 6] << 48;
	result |= (u_int64_t)self->sharedData->data[self->offset+offset + 7] << 56;
	return result;
}
void CBByteArrayReverseBytes(CBByteArray * self){
	for (int x = 0; x < self->length / 2; x++) {
		u_int8_t temp = self->sharedData->data[self->offset+x];
		self->sharedData->data[self->offset+x] = self->sharedData->data[self->offset+self->length-x-1];
		self->sharedData->data[self->offset+self->length-x-1] = temp;
	}
}
void CBByteArrayChangeReference(CBByteArray * self,CBByteArray * ref,u_int32_t offset){
	CBByteArrayReleaseSharedData(self); // Release last shared data.
	self->sharedData = ref->sharedData;
	self->sharedData->references++; // Since a new reference to the shared data is being made, an increase in the reference count must be made.
	self->offset = ref->offset + offset; // New offset for shared data
}
CBByteArray * CBByteArraySubCopy(CBByteArray * self,u_int32_t offset,u_int32_t length){
	CBByteArray * new = CBNewByteArrayOfSize(length,self->events);
	memcpy(new->sharedData->data,self->sharedData->data + self->offset + offset,length);
	return new;
}
CBByteArray * CBByteArraySubReference(CBByteArray * self,u_int32_t offset,u_int32_t length){
	return CBNewByteArraySubReference(self, offset, length);
}
