//
//  CBNetworkAddress.c
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

#include "CBNetworkAddressList.h"

//  Constructors

CBNetworkAddressList * CBNewNetworkAddressList(bool timeStamps){
	CBNetworkAddressList * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeNetworkAddressList;
	CBInitNetworkAddressList(self, timeStamps);
	return self;
}
CBNetworkAddressList * CBNewNetworkAddressListFromData(CBByteArray * data, bool timeStamps){
	CBNetworkAddressList * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeNetworkAddressList;
	CBInitNetworkAddressListFromData(self, timeStamps, data);
	return self;
}

//  Initialisers

void CBInitNetworkAddressList(CBNetworkAddressList * self, bool timeStamps){
	self->timeStamps = timeStamps;
	self->addrNum = 0;
	self->addresses = NULL;
	CBInitMessageByObject(CBGetMessage(self));
}
void CBInitNetworkAddressListFromData(CBNetworkAddressList * self, bool timeStamps, CBByteArray * data){
	self->timeStamps = timeStamps;
	self->addrNum = 0;
	self->addresses = NULL;
	CBInitMessageByData(CBGetMessage(self), data);
}

//  Destructor

void CBDestroyNetworkAddressList(void * vself){
	CBNetworkAddressList * self = vself;
	for (uint8_t x = 0; x < self->addrNum; x++)
		CBReleaseObject(self->addresses[x]);
	if (self->addresses) free(self->addresses);
	CBDestroyMessage(vself);
}
void CBFreeNetworkAddressList(void * self){
	CBDestroyNetworkAddressList(self);
	free(self);
}

//  Functions

void CBNetworkAddressListAddNetworkAddress(CBNetworkAddressList * self, CBNetworkAddress * address){
	CBRetainObject(address);
	CBNetworkAddressListTakeNetworkAddress(self, address);
}
uint32_t CBNetworkAddressListDeserialise(CBNetworkAddressList * self){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (! bytes) {
		CBLogError("Attempting to deserialise a CBNetworkAddress with no bytes.");
		return 0;
	}
	if (bytes->length < 26 + self->timeStamps * 4) {
		CBLogError("Attempting to deserialise a CBNetworkAddress without enough bytes to cover one address.");
		return 0;
	}
	CBVarInt num = CBVarIntDecode(bytes, 0);
	if (num.val > 30) {
		CBLogError("Attempting to deserialise a CBNetworkAddress with a var int over 30.");
		return 0;
	}
	self->addresses = malloc(sizeof(*self->addresses) * (size_t)num.val);
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
				if (! len)
					CBLogError("CBNetworkAddress cannot be deserialised because of an error with the CBNetworkAddress number %u.", x);
			}else{
				len = 0;
				CBLogError("Could not create CBNetworkAddress in CBNetworkAddressDeserialise for network address %u.", x);
			}
		}else{
			len = 0;
			CBLogError("Could not create CBByteArray in CBNetworkAddressDeserialise for network address %u.", x);
		}
		if (! len) {
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
uint32_t CBNetworkAddressListCalculateLength(CBNetworkAddressList * self){
	return CBVarIntSizeOf(self->addrNum) + self->addrNum * (self->timeStamps ? 30 : 26);
}
uint32_t CBNetworkAddressListSerialise(CBNetworkAddressList * self, bool force){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (! bytes) {
		CBLogError("Attempting to serialise a CBNetworkAddress with no bytes.");
		return 0;
	}
	CBVarInt num = CBVarIntFromUInt64(self->addrNum);
	if (bytes->length < (26 + self->timeStamps * 4) * self->addrNum + num.size) {
		CBLogError("Attempting to serialise a CBNetworkAddress without enough bytes.");
		return 0;
	}
	CBVarIntEncode(bytes, 0, num);
	uint16_t cursor = num.size;
	for (uint8_t x = 0; x < num.val; x++) {
		if (! CBGetMessage(self->addresses[x])->serialised // Serailise if not serialised yet.
			// Serialise if force is true.
			|| force
			// If the data shares the same data as this address broadcast, re-serialise the address, in case it got overwritten.
			|| CBGetMessage(self->addresses[x])->bytes->sharedData == bytes->sharedData
			|| (CBGetMessage(self->addresses[x])->bytes->length != 26) ^ self->timeStamps) {
			if (CBGetMessage(self->addresses[x])->serialised)
				// Release old byte array
				CBReleaseObject(CBGetMessage(self->addresses[x])->bytes);
			CBGetMessage(self->addresses[x])->bytes = CBByteArraySubReference(bytes, cursor, bytes->length-cursor);
			if (! CBNetworkAddressSerialise(self->addresses[x], self->timeStamps)) {
				CBLogError("CBNetworkAddress cannot be serialised because of an error with the CBNetworkAddress number %u.", x);
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
uint16_t CBNetworkAddressListStringMaxSize(CBNetworkAddressList * self){
	return 3 + 49*self->addrNum;
}
void CBNetworkAddressListToString(CBNetworkAddressList * self, char * output){
	*(output++) = '(';
	for (uint8_t x = 0; x < self->addrNum; x++) {
		output = CBNetworkAddressToString(self->addresses[x], output);
		if (x != self->addrNum - 1) {
			memcpy(output, ", ", 2);
			output += 2;
		}
	}
	strcpy(output, ")");
}
void CBNetworkAddressListTakeNetworkAddress(CBNetworkAddressList * self, CBNetworkAddress * address){
	self->addrNum++;
	CBNetworkAddress ** temp = realloc(self->addresses, sizeof(*self->addresses) * self->addrNum);
	self->addresses = temp;
	self->addresses[self->addrNum-1] = address;
}
