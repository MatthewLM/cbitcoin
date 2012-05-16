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

#include "CBByteArray.h"

//  Virtual Table Store

static CBByteArrayVT * VTStore = NULL;
static int objectNum = 0;

//  Constructor

CBByteArray * CBNewByteArrayOfSize(u_int32_t size,CBEvents * events){
	CBByteArray * self = malloc(sizeof(*self));
	CBAddVTToObject(CBGetObject(self), VTStore, CBCreateByteArrayVT);
	CBInitByteArrayOfSize(self,size,events);
	return self;
}
CBByteArray * CBNewByteArraySubReference(CBByteArray * ref,u_int32_t offset,u_int32_t length){
	CBByteArray * self = malloc(sizeof(*self));
	CBAddVTToObject(CBGetObject(self), VTStore, CBCreateByteArrayVT);
	CBInitByteArraySubReference(self, ref, offset, length);
	return self;
}
CBByteArray * CBNewByteArrayWithData(u_int8_t * data,u_int32_t size,CBEvents * events){
	CBByteArray * self = malloc(sizeof(*self));
	CBAddVTToObject(CBGetObject(self), VTStore, CBCreateByteArrayVT);
	CBInitByteArrayWithData(self, data, size, events);
	return self;
}

//  Virtual Table Creation

CBByteArrayVT * CBCreateByteArrayVT(){
	CBByteArrayVT * VT = malloc(sizeof(*VT));
	CBSetByteArrayVT(VT);
	return VT;
}
void CBSetByteArrayVT(CBByteArrayVT * VT){
	CBSetObjectVT((CBObjectVT *)VT);
	((CBObjectVT *)VT)->free = (void (*)(void *))CBFreeByteArray;
	VT->copy = (void * (*)(void *))CBByteArrayCopy;
	VT->getByte = (u_int8_t (*)(void *,u_int32_t))CBByteArrayGetByte;
	VT->getData = (u_int8_t * (*)(void *))CBByteArrayGetData;
	VT->getLastByte = (u_int8_t (*)(void *))CBByteArrayGetLastByte;
	VT->insertByte = (void (*)(void *,u_int32_t,u_int8_t))CBByteArrayInsertByte;
	VT->readUInt16 = (u_int16_t (*)(void *,u_int32_t))CBByteArrayReadUInt16;
	VT->readUInt32 = (u_int32_t (*)(void *,u_int32_t))CBByteArrayReadUInt32;
	VT->readUInt64 = (u_int64_t (*)(void *,u_int32_t))CBByteArrayReadUInt64;
	VT->reverse = (void (*)(void *))CBByteArrayReverseBytes;
	VT->subCopy = (void * (*)(void *,u_int32_t,u_int32_t))CBByteArraySubCopy;
}

//  Virtual Table Getter

CBByteArrayVT * CBGetByteArrayVT(void * self){
	return ((CBByteArrayVT *)(CBGetObject(self))->VT);
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
	CBGetObjectVT(self->events)->retain(self->events);
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
	CBGetObjectVT(self->events)->retain(self->events);
	self->sharedData = ref->sharedData;
	self->sharedData->references++; // Since a new reference to the shared data is being made, an increase in the reference count must be made.
	self->length = length;
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

//  Destructor

void CBFreeByteArray(CBByteArray * self){
	CBFreeProcessByteArray(self);
	CBFree();
}
void CBFreeProcessByteArray(CBByteArray * self){
	CBGetObjectVT(self->events)->release(&self->events);
	self->sharedData->references--;
	if (self->sharedData->references < 1) {
		// Shared data now owned by nothing so free it 
		free(self->sharedData->data);
		free(self->sharedData);
	}
	CBFreeProcessObject(CBGetObject(self));
}

//  Functions

void CBByteArrayAdjustLength(CBByteArray * self, int adjustment){
	if (self->length != CB_BYTE_ARRAY_UNKNOWN_LENGTH)
		if (adjustment == CB_BYTE_ARRAY_UNKNOWN_LENGTH)
			self->length = CB_BYTE_ARRAY_UNKNOWN_LENGTH;
		else
			self->length += adjustment;
}
CBByteArray * CBByteArrayCopy(CBByteArray * self){
	CBByteArray * new = CBNewByteArrayOfSize(self->length,self->events);
	memcpy(new->sharedData->data,self->sharedData->data + self->offset,self->length);
	return new;
}
u_int8_t CBByteArrayGetByte(CBByteArray * self,int index){
	return self->sharedData->data[self->offset+index];
}
u_int8_t * CBByteArrayGetData(CBByteArray * self){
	return self->sharedData->data + self->offset;
}
u_int8_t CBByteArrayGetLastByte(CBByteArray * self){
	return self->sharedData->data[self->offset+self->length];
}
void CBByteArrayInsertByte(CBByteArray * self,int index,u_int8_t byte){
	self->sharedData->data[self->offset+index] = byte;
}
u_int16_t CBByteArrayReadUInt16(CBByteArray * self,int offset){
	u_int16_t result = (u_int16_t)CBGetByteArrayVT(self)->getByte(self,offset) << 8;
	result |= CBGetByteArrayVT(self)->getByte(self,offset+1);
	return result;
}
u_int32_t CBByteArrayReadUInt32(CBByteArray * self,int offset){
	u_int32_t result = (u_int32_t)CBGetByteArrayVT(self)->getByte(self,offset) << 24;
	result |= (u_int32_t)CBGetByteArrayVT(self)->getByte(self,offset+1) << 16;
	result |= (u_int32_t)CBGetByteArrayVT(self)->getByte(self,offset+2) << 8;
	result |= CBGetByteArrayVT(self)->getByte(self,offset+3);
	return result;
}
u_int64_t CBByteArrayReadUInt64(CBByteArray * self,int offset){
	u_int64_t result = (u_int64_t)CBGetByteArrayVT(self)->getByte(self,offset) << 56;
	result |= (u_int64_t)CBGetByteArrayVT(self)->getByte(self,offset+1) << 48;
	result |= (u_int64_t)CBGetByteArrayVT(self)->getByte(self,offset+2) << 40;
	result |= (u_int64_t)CBGetByteArrayVT(self)->getByte(self,offset+3) << 32;
	result |= (u_int64_t)CBGetByteArrayVT(self)->getByte(self,offset+4) << 24;
	result |= (u_int64_t)CBGetByteArrayVT(self)->getByte(self,offset+5) << 16;
	result |= (u_int64_t)CBGetByteArrayVT(self)->getByte(self,offset+6) << 8;
	result |= CBGetByteArrayVT(self)->getByte(self,offset+7);
	return result;
}
void CBByteArrayReverseBytes(CBByteArray * self){
	for (int x = 0; x < self->length / 2; x++) {
		u_int8_t temp = CBGetByteArrayVT(self)->getByte(self,x);
		CBGetByteArrayVT(self)->insertByte(self,x,CBGetByteArrayVT(self)->getByte(self,self->length-x-1));
		CBGetByteArrayVT(self)->insertByte(self,self->length-x-1,temp);
	}
}
CBByteArray * CBByteArraySubCopy(CBByteArray * self,int offset,int length){
	CBByteArray * new = CBNewByteArrayOfSize(length,self->events);
	memcpy(new->sharedData->data,self->sharedData->data + self->offset + offset,length);
	return new;
}
