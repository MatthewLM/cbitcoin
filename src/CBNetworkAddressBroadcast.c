//
//  CBNetworkAddressBroadcast.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 03/07/2012.
//  Copyright (c) 2012 Matthew Mitchell
//  
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

//  SEE HEADER FILE FOR DOCUMENTATION 

#include "CBNetworkAddressBroadcast.h"

//  Constructors

CBNetworkAddressBroadcast * CBNewNetworkAddressBroadcast(bool timeStamps){
	CBNetworkAddressBroadcast * self = malloc(sizeof(*self));
	if (NOT self) {
		CBLogError("Cannot allocate %i bytes of memory in CBNewNetworkAddressBroadcast\n", sizeof(*self));
		return NULL;
	}
	CBGetObject(self)->free = CBFreeNetworkAddressBroadcast;
	if (CBInitNetworkAddressBroadcast(self, timeStamps))
		return self;
	free(self);
	return NULL;
}
CBNetworkAddressBroadcast * CBNewNetworkAddressBroadcastFromData(CBByteArray * data, bool timeStamps){
	CBNetworkAddressBroadcast * self = malloc(sizeof(*self));
	if (NOT self) {
		CBLogError("Cannot allocate %i bytes of memory in CBNewNetworkAddressBroadcast\n", sizeof(*self));
		return NULL;
	}
	CBGetObject(self)->free = CBFreeNetworkAddressBroadcast;
	if (CBInitNetworkAddressBroadcastFromData(self, timeStamps, data))
		return self;
	free(self);
	return NULL;
}

//  Object Getter

CBNetworkAddressBroadcast * CBGetNetworkAddressBroadcast(void * self){
	return self;
}

//  Initialisers

bool CBInitNetworkAddressBroadcast(CBNetworkAddressBroadcast * self, bool timeStamps){
	self->timeStamps = timeStamps;
	self->addrNum = 0;
	self->addresses = NULL;
	if (NOT CBInitMessageByObject(CBGetMessage(self)))
		return false;
	return true;
}
bool CBInitNetworkAddressBroadcastFromData(CBNetworkAddressBroadcast * self, bool timeStamps, CBByteArray * data){
	self->timeStamps = timeStamps;
	self->addrNum = 0;
	self->addresses = NULL;
	if (NOT CBInitMessageByData(CBGetMessage(self), data))
		return false;
	return true;
}

//  Destructor

void CBDestroyNetworkAddressBroadcast(void * vself){
	CBNetworkAddressBroadcast * self = vself;
	for (uint8_t x = 0; x < self->addrNum; x++)
		CBReleaseObject(self->addresses[x]);
	if (self->addresses) free(self->addresses);
}
void CBFreeNetworkAddressBroadcast(void * self){
	CBDestroyNetworkAddressBroadcast(self);
	free(self);
}

//  Functions

bool CBNetworkAddressBroadcastAddNetworkAddress(CBNetworkAddressBroadcast * self, CBNetworkAddress * address){
	CBRetainObject(address);
	return CBNetworkAddressBroadcastTakeNetworkAddress(self, address);
}
uint32_t CBNetworkAddressBroadcastDeserialise(CBNetworkAddressBroadcast * self){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (NOT bytes) {
		CBLogError("Attempting to deserialise a CBNetworkAddressBroadcast with no bytes.");
		return 0;
	}
	if (bytes->length < 26 + self->timeStamps * 4) {
		CBLogError("Attempting to deserialise a CBNetworkAddressBroadcast without enough bytes to cover one address.");
		return 0;
	}
	CBVarInt num = CBVarIntDecode(bytes, 0);
	if (num.val > 30) {
		CBLogError("Attempting to deserialise a CBNetworkAddressBroadcast with a var int over 30.");
		return 0;
	}
	self->addresses = malloc(sizeof(*self->addresses) * (size_t)num.val);
	if (NOT self->addresses) {
		CBLogError("Cannot allocate %i bytes of memory in CBNetworkAddressBroadcastDeserialise\n", sizeof(*self->addresses) * (size_t)num.val);
		return 0;
	}
	self->addrNum = num.val;
	uint16_t cursor = num.size;
	for (uint8_t x = 0; x < num.val; x++) {
		// Make new CBNetworkAddress from the rest of the data.
		uint8_t len;
		CBByteArray * data = CBByteArraySubReference(bytes, cursor, bytes->length-cursor);
		if (data) {
			// Create a new network address object. It is public since it is in an address broadcast.
			self->addresses[x] = CBNewNetworkAddressFromData(data, true);
			if (self->addresses[x]){
				// Deserialise
				len = CBNetworkAddressDeserialise(self->addresses[x], self->timeStamps);
				if (NOT len)
					CBLogError("CBNetworkAddressBroadcast cannot be deserialised because of an error with the CBNetworkAddress number %u.", x);
			}else{
				len = 0;
				CBLogError("Could not create CBNetworkAddress in CBNetworkAddressBroadcastDeserialise for network address %u.", x);
			}
		}else{
			len = 0;
			CBLogError("Could not create CBByteArray in CBNetworkAddressBroadcastDeserialise for network address %u.", x);
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
uint32_t CBNetworkAddressBroadcastCalculateLength(CBNetworkAddressBroadcast * self){
	return CBVarIntSizeOf(self->addrNum) + self->addrNum * (self->timeStamps ? 30 : 26);
}
uint32_t CBNetworkAddressBroadcastSerialise(CBNetworkAddressBroadcast * self, bool force){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (NOT bytes) {
		CBLogError("Attempting to serialise a CBNetworkAddressBroadcast with no bytes.");
		return 0;
	}
	CBVarInt num = CBVarIntFromUInt64(self->addrNum);
	if (bytes->length < (26 + self->timeStamps * 4) * self->addrNum + num.size) {
		CBLogError("Attempting to serialise a CBNetworkAddressBroadcast without enough bytes.");
		return 0;
	}
	CBVarIntEncode(bytes, 0, num);
	uint16_t cursor = num.size;
	for (uint8_t x = 0; x < num.val; x++) {
		if (NOT CBGetMessage(self->addresses[x])->serialised // Serailise if not serialised yet.
			// Serialise if force is true.
			|| force
			// If the data shares the same data as this address broadcast, re-serialise the address, in case it got overwritten.
			|| CBGetMessage(self->addresses[x])->bytes->sharedData == bytes->sharedData
			|| (CBGetMessage(self->addresses[x])->bytes->length != 26) ^ self->timeStamps) {
			if (CBGetMessage(self->addresses[x])->serialised)
				// Release old byte array
				CBReleaseObject(CBGetMessage(self->addresses[x])->bytes);
			CBGetMessage(self->addresses[x])->bytes = CBByteArraySubReference(bytes, cursor, bytes->length-cursor);
			if (NOT CBNetworkAddressSerialise(self->addresses[x], self->timeStamps)) {
				CBLogError("CBNetworkAddressBroadcast cannot be serialised because of an error with the CBNetworkAddress number %u.", x);
				// Release CBByteArray objects to avoid problems overwritting pointer without release, if serialisation is tried again.
				for (uint8_t y = 0; y < x + 1; y++)
					CBReleaseObject(CBGetMessage(self->addresses[y])->bytes);
				return 0;
			}
		}else{
			// Move serialsed data to one location
			CBByteArrayCopyByteArray(bytes, cursor, CBGetMessage(self->addresses[x])->bytes);
			CBByteArrayChangeReference(CBGetMessage(self->addresses[x])->bytes, bytes, cursor);
		}
		cursor += CBGetMessage(self->addresses[x])->bytes->length;
	}
	// Change bytes length
	bytes->length = cursor;
	// Is now serialised
	CBGetMessage(self)->serialised = true;
	return cursor;
}
bool CBNetworkAddressBroadcastTakeNetworkAddress(CBNetworkAddressBroadcast * self, CBNetworkAddress * address){
	self->addrNum++;
	CBNetworkAddress ** temp = realloc(self->addresses, sizeof(*self->addresses) * self->addrNum);
	if (NOT temp) {
		return false;
	}
	self->addresses = temp;
	self->addresses[self->addrNum-1] = address;
	return true;
}
