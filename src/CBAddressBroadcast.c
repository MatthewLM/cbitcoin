//
//  CBAddressBroadcast.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 03/07/2012.
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

#include "CBAddressBroadcast.h"

//  Constructors

CBAddressBroadcast * CBNewAddressBroadcast(bool timeStamps,void (*logError)(char *,...)){
	CBAddressBroadcast * self = malloc(sizeof(*self));
	if (NOT self) {
		logError("Cannot allocate %i bytes of memory in CBNewAddressBroadcast\n",sizeof(*self));
		return NULL;
	}
	CBGetObject(self)->free = CBFreeAddressBroadcast;
	if (CBInitAddressBroadcast(self,timeStamps,logError))
		return self;
	free(self);
	return NULL;
}
CBAddressBroadcast * CBNewAddressBroadcastFromData(CBByteArray * data,bool timeStamps,void (*logError)(char *,...)){
	CBAddressBroadcast * self = malloc(sizeof(*self));
	if (NOT self) {
		logError("Cannot allocate %i bytes of memory in CBNewAddressBroadcast\n",sizeof(*self));
		return NULL;
	}
	CBGetObject(self)->free = CBFreeAddressBroadcast;
	if (CBInitAddressBroadcastFromData(self,timeStamps,data,logError))
		return self;
	free(self);
	return NULL;
}

//  Object Getter

CBAddressBroadcast * CBGetAddressBroadcast(void * self){
	return self;
}

//  Initialisers

bool CBInitAddressBroadcast(CBAddressBroadcast * self,bool timeStamps,void (*logError)(char *,...)){
	self->timeStamps = timeStamps;
	self->addrNum = 0;
	self->addresses = NULL;
	if (NOT CBInitMessageByObject(CBGetMessage(self), logError))
		return false;
	return true;
}
bool CBInitAddressBroadcastFromData(CBAddressBroadcast * self,bool timeStamps,CBByteArray * data,void (*logError)(char *,...)){
	self->timeStamps = timeStamps;
	self->addrNum = 0;
	self->addresses = NULL;
	if (NOT CBInitMessageByData(CBGetMessage(self), data, logError))
		return false;
	return true;
}

//  Destructor

void CBFreeAddressBroadcast(void * vself){
	CBAddressBroadcast * self = vself;
	for (uint8_t x = 0; x < self->addrNum; x++)
		CBReleaseObject(self->addresses[x]);
	if (self->addresses) free(self->addresses);
	CBFreeObject(self);
}

//  Functions

bool CBAddressBroadcastAddNetworkAddress(CBAddressBroadcast * self,CBNetworkAddress * address){
	CBRetainObject(address);
	return CBAddressBroadcastTakeNetworkAddress(self,address);
}
uint32_t CBAddressBroadcastDeserialise(CBAddressBroadcast * self){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (NOT bytes) {
		CBGetMessage(self)->logError("Attempting to deserialise a CBAddressBroadcast with no bytes.");
		return 0;
	}
	if (bytes->length < 26 + self->timeStamps * 4) {
		CBGetMessage(self)->logError("Attempting to deserialise a CBAddressBroadcast without enough bytes to cover one address.");
		return 0;
	}
	CBVarInt num = CBVarIntDecode(bytes, 0);
	if (num.val > 30) {
		CBGetMessage(self)->logError("Attempting to deserialise a CBAddressBroadcast with a var int over 30.");
		return 0;
	}
	self->addresses = malloc(sizeof(*self->addresses) * (size_t)num.val);
	if (NOT self->addresses) {
		CBGetMessage(self)->logError("Cannot allocate %i bytes of memory in CBAddressBroadcastDeserialise\n",sizeof(*self->addresses) * (size_t)num.val);
		return 0;
	}
	self->addrNum = num.val;
	uint16_t cursor = num.size;
	for (uint8_t x = 0; x < num.val; x++) {
		// Make new CBNetworkAddress from the rest of the data.
		uint8_t len;
		CBByteArray * data = CBByteArraySubReference(bytes, cursor, bytes->length-cursor);
		if (data) {
			self->addresses[x] = CBNewNetworkAddressFromData(data, CBGetMessage(self)->logError);
			if (self->addresses[x]){
				// Deserialise
				len = CBNetworkAddressDeserialise(self->addresses[x], self->timeStamps);
				if (NOT len)
					CBGetMessage(self)->logError("CBAddressBroadcast cannot be deserialised because of an error with the CBNetworkAddress number %u.",x);
			}else{
				len = 0;
				CBGetMessage(self)->logError("Could not create CBNetworkAddress in CBAddressBroadcastDeserialise for network address %u.",x);
			}
		}else{
			len = 0;
			CBGetMessage(self)->logError("Could not create CBByteArray in CBAddressBroadcastDeserialise for network address %u.",x);
		}
		if (NOT len) {
			// Release bytes
			CBReleaseObject(data);
			return 0;
		}
		// Adjust length
		data->length = len;
		CBReleaseObject(data);
		cursor += len;
	}
	return cursor;
}
uint32_t CBAddressBroadcastCalculateLength(CBAddressBroadcast * self){
	return CBVarIntSizeOf(self->addrNum) + self->addrNum * (self->timeStamps ? 30 : 26);
}
uint32_t CBAddressBroadcastSerialise(CBAddressBroadcast * self, bool force){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (NOT bytes) {
		CBGetMessage(self)->logError("Attempting to serialise a CBAddressBroadcast with no bytes.");
		return 0;
	}
	if (bytes->length < (26 + self->timeStamps * 4) * self->addrNum) {
		CBGetMessage(self)->logError("Attempting to serialise a CBAddressBroadcast without enough bytes.");
		return 0;
	}
	CBVarInt num = CBVarIntFromUInt64(self->addrNum);
	CBVarIntEncode(bytes, 0, num);
	uint16_t cursor = num.size;
	for (uint8_t x = 0; x < num.val; x++) {
		if (force && CBGetMessage(self->addresses[x])->serialised)
			CBReleaseObject(CBGetMessage(self->addresses[x])->bytes);
		if (NOT CBGetMessage(self->addresses[x])->serialised || force) {
			CBGetMessage(self->addresses[x])->bytes = CBByteArraySubReference(bytes, cursor, bytes->length-cursor);
			uint32_t len = CBNetworkAddressSerialise(self->addresses[x],self->timeStamps);
			if (NOT len) {
				CBGetMessage(self)->logError("CBAddressBroadcast cannot be serialised because of an error with the CBNetworkAddress number %u.",x);
				// Release CBByteArray objects to avoid problems overwritting pointer without release, if serialisation is tried again.
				for (uint8_t y = 0; y < x + 1; y++)
					CBReleaseObject(CBGetMessage(self->addresses[y])->bytes);
				return 0;
			}
			CBGetMessage(self->addresses[x])->bytes->length = len;
		}else if (CBGetMessage(self->addresses[x])->bytes->sharedData != bytes->sharedData){
			// Move serialsed data to one location
			CBByteArrayCopyByteArray(bytes, cursor, CBGetMessage(self->addresses[x])->bytes);
			CBByteArrayChangeReference(CBGetMessage(self->addresses[x])->bytes, bytes, cursor);
		}
		cursor += CBGetMessage(self->addresses[x])->bytes->length;
	}
	CBGetMessage(self)->serialised = true;
	return cursor;
}
bool CBAddressBroadcastTakeNetworkAddress(CBAddressBroadcast * self,CBNetworkAddress * address){
	self->addrNum++;
	CBNetworkAddress ** temp = realloc(self->addresses, sizeof(*self->addresses) * self->addrNum);
	if (NOT temp) {
		return false;
	}
	self->addresses = temp;
	self->addresses[self->addrNum-1] = address;
	return true;
}
