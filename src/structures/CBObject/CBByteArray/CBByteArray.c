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
#include <assert.h>

//  Constructor

CBByteArray * CBNewByteArrayFromString(char * string, bool terminator, CBEvents * events){
	CBByteArray * self = malloc(sizeof(*self));
	if (NOT self) {
		events->onErrorReceived(CB_ERROR_OUT_OF_MEMORY,"Cannot allocate %i bytes of memory in CBNewByteArrayFromString\n",sizeof(*self));
		return NULL;
	}
	CBGetObject(self)->free = CBFreeByteArray;
	if(CBInitByteArrayFromString(self,string,terminator,events))
		return self;
	free(self);
	return NULL;
}
CBByteArray * CBNewByteArrayOfSize(uint32_t size,CBEvents * events){
	CBByteArray * self = malloc(sizeof(*self));
	if (NOT self) {
		events->onErrorReceived(CB_ERROR_OUT_OF_MEMORY,"Cannot allocate %i bytes of memory in CBNewByteArrayOfSize\n",sizeof(*self));
		return NULL;
	}
	CBGetObject(self)->free = CBFreeByteArray;
	if(CBInitByteArrayOfSize(self,size,events))
		return self;
	free(self);
	return NULL;
}
CBByteArray * CBNewByteArraySubReference(CBByteArray * ref,uint32_t offset,uint32_t length){
	CBByteArray * self = malloc(sizeof(*self));
	if (NOT self) {
		ref->events->onErrorReceived(CB_ERROR_OUT_OF_MEMORY,"Cannot allocate %i bytes of memory in CBNewByteArraySubReference\n",sizeof(*self));
		return NULL;
	}
	CBGetObject(self)->free = CBFreeByteArray;
	if(CBInitByteArraySubReference(self, ref, offset, length))
		return self;
	free(self);
	return NULL;
}
CBByteArray * CBNewByteArrayWithData(uint8_t * data,uint32_t size,CBEvents * events){
	CBByteArray * self = malloc(sizeof(*self));
	if (NOT self) {
		events->onErrorReceived(CB_ERROR_OUT_OF_MEMORY,"Cannot allocate %i bytes of memory in CBNewByteArrayWithData\n",sizeof(*self));
		return NULL;
	}
	CBGetObject(self)->free = CBFreeByteArray;
	if(CBInitByteArrayWithData(self, data, size, events))
		return self;
	free(self);
	return NULL;
}
CBByteArray * CBNewByteArrayWithDataCopy(uint8_t * data,uint32_t size,CBEvents * events){
	CBByteArray * self = malloc(sizeof(*self));
	if (NOT self) {
		events->onErrorReceived(CB_ERROR_OUT_OF_MEMORY,"Cannot allocate %i bytes of memory in CBNewByteArrayWithDataCopy\n",sizeof(*self));
		return NULL;
	}
	CBGetObject(self)->free = CBFreeByteArray;
	if(CBInitByteArrayWithDataCopy(self, data, size, events))
		return self;
	free(self);
	return NULL;
}

//  Object Getter

CBByteArray * CBGetByteArray(void * self){
	return self;
}

//  Initialisers


bool CBInitByteArrayFromString(CBByteArray * self, char * string, bool terminator, CBEvents * events){
	if (NOT CBInitObject(CBGetObject(self)))
		return false;
	self->events = events;
	self->length = (uint32_t)(strlen(string) + terminator);
	self->sharedData = malloc(sizeof(*self->sharedData));
	if (NOT self->sharedData) {
		events->onErrorReceived(CB_ERROR_OUT_OF_MEMORY,"Cannot allocate %i bytes of memory in CBInitByteArrayFromString for the sharedData structure.\n",sizeof(*self->sharedData));
		return false;
	}
	self->sharedData->data = malloc(self->length);
	if (NOT self->sharedData->data) {
		events->onErrorReceived(CB_ERROR_OUT_OF_MEMORY,"Cannot allocate %i bytes of memory in CBInitByteArrayFromString for the shared data.\n",self->length);
		return false;
	}
	self->sharedData->references = 1;
	self->offset = 0;
	memmove(self->sharedData->data, string, self->length);
	return true;
}
bool CBInitByteArrayOfSize(CBByteArray * self,uint32_t size,CBEvents * events){
	if (NOT CBInitObject(CBGetObject(self)))
		return false;
	self->events = events;
	self->length = size;
	self->sharedData = malloc(sizeof(*self->sharedData));
	if (NOT self->sharedData) {
		events->onErrorReceived(CB_ERROR_OUT_OF_MEMORY,"Cannot allocate %i bytes of memory in CBInitByteArrayOfSize for the sharedData structure.\n",sizeof(*self->sharedData));
		return false;
	}
	self->sharedData->data = malloc(size);
	if (NOT self->sharedData->data) {
		events->onErrorReceived(CB_ERROR_OUT_OF_MEMORY,"Cannot allocate %i bytes of memory in CBInitByteArrayOfSize for the shared data.\n",size);
		return false;
	}
	self->sharedData->references = 1;
	self->offset = 0;
	return true;
}
bool CBInitByteArraySubReference(CBByteArray * self,CBByteArray * ref,uint32_t offset,uint32_t length){
	if (NOT CBInitObject(CBGetObject(self)))
		return false;
	self->events = ref->events;
	self->sharedData = ref->sharedData;
	self->sharedData->references++; // Since a new reference to the shared data is being made, an increase in the reference count must be made.
	self->length = length ? length : ref->length; // If length is 0, set to the reference length.
	self->offset = ref->offset + offset;
	return true;
}
bool CBInitByteArrayWithData(CBByteArray * self,uint8_t * data,uint32_t size,CBEvents * events){
	if (NOT CBInitObject(CBGetObject(self)))
		return false;
	self->events = events;
	self->sharedData = malloc(sizeof(*self->sharedData));
	if (NOT self->sharedData) {
		events->onErrorReceived(CB_ERROR_OUT_OF_MEMORY,"Cannot allocate %i bytes of memory in CBInitByteArrayWithData for the sharedData structure.\n",sizeof(*self->sharedData));
		return false;
	}
	self->sharedData->data = data;
	self->sharedData->references = 1;
	self->length = size;
	self->offset = 0;
	return true;
}
bool CBInitByteArrayWithDataCopy(CBByteArray * self,uint8_t * data,uint32_t size,CBEvents * events){
	if (NOT CBInitObject(CBGetObject(self)))
		return false;
	self->events = events;
	self->sharedData = malloc(sizeof(*self->sharedData));
	if (NOT self->sharedData) {
		events->onErrorReceived(CB_ERROR_OUT_OF_MEMORY,"Cannot allocate %i bytes of memory in CBInitByteArrayWithDataCopy for the sharedData structure.\n",sizeof(*self->sharedData));
		return false;
	}
	self->sharedData->data = malloc(size);
	if (NOT self->sharedData->data) {
		events->onErrorReceived(CB_ERROR_OUT_OF_MEMORY,"Cannot allocate %i bytes of memory in CBInitByteArrayWithDataCopy for the shared data.\n",size);
		return false;
	}
	memmove(self->sharedData->data,data,size);
	self->sharedData->references = 1;
	self->length = size;
	self->offset = 0;
	return true;
}

//  Destructor

void CBFreeByteArray(void * self){
	CBByteArrayReleaseSharedData(self);
	CBFreeObject(CBGetObject(self));
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
	if (NOT new)
		return NULL;
	memmove(new->sharedData->data,self->sharedData->data + self->offset,self->length);
	return new;
}
void CBByteArrayCopyByteArray(CBByteArray * self,uint32_t writeOffset,CBByteArray * source){
	memmove(self->sharedData->data + self->offset + writeOffset,source->sharedData->data + source->offset,source->length);
}
void CBByteArrayCopySubByteArray(CBByteArray * self,uint32_t writeOffset,CBByteArray * source,uint32_t readOffset,uint32_t length){
	assert(length);
	memmove(self->sharedData->data + self->offset + writeOffset,source->sharedData->data + source->offset + readOffset,length);
}
bool CBByteArrayEquals(CBByteArray * self,CBByteArray * second){
	if (self->length != second->length)
		return false;
	return NOT memcmp(CBByteArrayGetData(self), CBByteArrayGetData(second), self->length);
}
uint8_t CBByteArrayGetByte(CBByteArray * self,uint32_t index){
	return self->sharedData->data[self->offset+index];
}
uint8_t * CBByteArrayGetData(CBByteArray * self){
	return self->sharedData->data + self->offset;
}
uint8_t CBByteArrayGetLastByte(CBByteArray * self){
	return self->sharedData->data[self->offset+self->length];
}
bool CBByteArrayIsNull(CBByteArray * self){
	for (uint32_t x = 0; x < self->length; x++)
		if (self->sharedData->data[self->offset+x])
			return false;
	return true;
}
void CBByteArraySetByte(CBByteArray * self,uint32_t index,uint8_t byte){
	self->sharedData->data[self->offset+index] = byte;
}
void CBByteArraySetBytes(CBByteArray * self,uint32_t index,uint8_t * bytes,uint32_t length){
	memmove(self->sharedData->data + self->offset + index, bytes, length);
}
void CBByteArraySetInt16(CBByteArray * self,uint32_t offset,uint16_t integer){
	self->sharedData->data[self->offset+offset] = integer;
	self->sharedData->data[self->offset+offset + 1] = integer >> 8;
}
void CBByteArraySetInt32(CBByteArray * self,uint32_t offset,uint32_t integer){
	self->sharedData->data[self->offset+offset] = integer;
	self->sharedData->data[self->offset+offset + 1] = integer >> 8;
	self->sharedData->data[self->offset+offset + 2] = integer >> 16;
	self->sharedData->data[self->offset+offset + 3] = integer >> 24;
}
void CBByteArraySetInt64(CBByteArray * self,uint32_t offset,uint64_t integer){
	self->sharedData->data[self->offset+offset] = integer;
	self->sharedData->data[self->offset+offset + 1] = integer >> 8;
	self->sharedData->data[self->offset+offset + 2] = integer >> 16;
	self->sharedData->data[self->offset+offset + 3] = integer >> 24;
	self->sharedData->data[self->offset+offset + 4] = integer >> 32;
	self->sharedData->data[self->offset+offset + 5] = integer >> 40;
	self->sharedData->data[self->offset+offset + 6] = integer >> 48;
	self->sharedData->data[self->offset+offset + 7] = integer >> 56;
}
void CBByteArraySetPort(CBByteArray * self,uint32_t offset,uint16_t integer){
	self->sharedData->data[self->offset+offset + 1] = integer;
	self->sharedData->data[self->offset+offset] = integer >> 8;
}
uint16_t CBByteArrayReadInt16(CBByteArray * self,uint32_t offset){
	uint16_t result = self->sharedData->data[self->offset+offset];
	result |= (uint16_t)self->sharedData->data[self->offset+offset + 1] << 8;
	return result;
}
uint32_t CBByteArrayReadInt32(CBByteArray * self,uint32_t offset){
	uint32_t result = self->sharedData->data[self->offset+offset];
	result |= (uint32_t)self->sharedData->data[self->offset+offset + 1] << 8;
	result |= (uint32_t)self->sharedData->data[self->offset+offset + 2] << 16;
	result |= (uint32_t)self->sharedData->data[self->offset+offset + 3] << 24;
	return result;
}
uint64_t CBByteArrayReadInt64(CBByteArray * self,uint32_t offset){
	uint64_t result = self->sharedData->data[self->offset+offset];
	result |= (uint64_t)self->sharedData->data[self->offset+offset + 1] << 8;
	result |= (uint64_t)self->sharedData->data[self->offset+offset + 2] << 16;
	result |= (uint64_t)self->sharedData->data[self->offset+offset + 3] << 24;
	result |= (uint64_t)self->sharedData->data[self->offset+offset + 4] << 32;
	result |= (uint64_t)self->sharedData->data[self->offset+offset + 5] << 40;
	result |= (uint64_t)self->sharedData->data[self->offset+offset + 6] << 48;
	result |= (uint64_t)self->sharedData->data[self->offset+offset + 7] << 56;
	return result;
}
uint16_t CBByteArrayReadPort(CBByteArray * self,uint32_t offset){
	uint16_t result = self->sharedData->data[self->offset+offset + 1];
	result |= (uint16_t)self->sharedData->data[self->offset+offset] << 8;
	return result;
}
void CBByteArrayReverseBytes(CBByteArray * self){
	for (int x = 0; x < self->length / 2; x++) {
		uint8_t temp = self->sharedData->data[self->offset+x];
		self->sharedData->data[self->offset+x] = self->sharedData->data[self->offset+self->length-x-1];
		self->sharedData->data[self->offset+self->length-x-1] = temp;
	}
}
void CBByteArrayChangeReference(CBByteArray * self,CBByteArray * ref,uint32_t offset){
	CBByteArrayReleaseSharedData(self); // Release last shared data.
	self->sharedData = ref->sharedData;
	self->sharedData->references++; // Since a new reference to the shared data is being made, an increase in the reference count must be made.
	self->offset = ref->offset + offset; // New offset for shared data
}
CBByteArray * CBByteArraySubCopy(CBByteArray * self,uint32_t offset,uint32_t length){
	CBByteArray * new = CBNewByteArrayOfSize(length,self->events);
	if (NOT new)
		return NULL;
	memcpy(new->sharedData->data,self->sharedData->data + self->offset + offset,length);
	return new;
}
CBByteArray * CBByteArraySubReference(CBByteArray * self,uint32_t offset,uint32_t length){
	return CBNewByteArraySubReference(self, offset, length);
}
