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

CBNetworkAddressList * CBNewNetworkAddressList(bool timeStamps) {
	
	CBNetworkAddressList * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeNetworkAddressList;
	CBInitNetworkAddressList(self, timeStamps);
	
	return self;
	
}

CBNetworkAddressList * CBNewNetworkAddressListFromData(CBByteArray * data, bool timeStamps) {
	
	CBNetworkAddressList * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeNetworkAddressList;
	CBInitNetworkAddressListFromData(self, timeStamps, data);
	
	return self;
	
}

//  Initialisers

void CBInitNetworkAddressList(CBNetworkAddressList * self, bool timeStamps) {
	
	self->timeStamps = timeStamps;
	self->addrNum = 0;
	self->addresses = NULL;
	
	CBInitMessageByObject(CBGetMessage(self));
	
}

void CBInitNetworkAddressListFromData(CBNetworkAddressList * self, bool timeStamps, CBByteArray * data) {
	
	self->timeStamps = timeStamps;
	self->addrNum = 0;
	self->addresses = NULL;
	
	CBInitMessageByData(CBGetMessage(self), data);
	
}

//  Destructor

void CBDestroyNetworkAddressList(void * vself) {
	
	CBNetworkAddressList * self = vself;
	
	for (int x = 0; x < self->addrNum; x++)
		CBReleaseObject(self->addresses[x]);
	
	if (self->addresses) free(self->addresses);
	
	CBDestroyMessage(vself);
	
}

void CBFreeNetworkAddressList(void * self) {
	
	CBDestroyNetworkAddressList(self);
	free(self);
	
}

//  Functions

void CBNetworkAddressListAddNetworkAddress(CBNetworkAddressList * self, CBNetworkAddress * address) {
	
	CBRetainObject(address);
	CBNetworkAddressListTakeNetworkAddress(self, address);
	
}

int CBNetworkAddressListCalculateLength(CBNetworkAddressList * self) {
	
	return CBVarIntSizeOf(self->addrNum) + self->addrNum * (self->timeStamps ? 30 : 26);
	
}

int CBNetworkAddressListDeserialise(CBNetworkAddressList * self) {
	
	CBByteArray * bytes = CBGetMessage(self)->bytes;

	if (! bytes) {
		CBLogError("Attempting to deserialise a CBNetworkAddressList with no bytes.");
		return CB_DESERIALISE_ERROR;
	}

	if (bytes->length < 26 + (self->timeStamps ? 4 : 0)) {
		CBLogError("Attempting to deserialise a CBNetworkAddressList without enough bytes to cover one address.");
		return CB_DESERIALISE_ERROR;
	}
	
	CBVarInt num = CBByteArrayReadVarInt(bytes, 0);
	if (num.val > 1000) {
		CBLogError("Attempting to deserialise a CBNetworkAddressList with a var int over 1000.");
		return CB_DESERIALISE_ERROR;
	}
	
	self->addresses = malloc(sizeof(*self->addresses) * (size_t)num.val);
	self->addrNum = (int)num.val;
	
	int cursor = num.size;
	
	for (int x = 0; x < num.val; x++) {
		
		// Make new CBNetworkAddress from the rest of the data.
		int len;
		CBByteArray * data = CBByteArraySubReference(bytes, cursor, bytes->length-cursor);
		
		// Create a new network address object. It is public since it is in an address broadcast.
		self->addresses[x] = CBNewNetworkAddressFromData(data, true);
		
		// Deserialise
		len = CBNetworkAddressDeserialise(self->addresses[x], self->timeStamps);
		if (len == CB_DESERIALISE_ERROR) {
			
			CBLogError("CBNetworkAddressList cannot be deserialised because of an error with the CBNetworkAddress number %u.", x);
			
			// Release bytes
			CBReleaseObject(data);
			
			return CB_DESERIALISE_ERROR;
			
		}
		
		// Adjust length
		data->length = len;
		CBReleaseObject(data);
		cursor += len;
		
	}
	
	return cursor;
	
}

void CBNetworkAddressListPrepareBytes(CBNetworkAddressList * self) {
	
	CBMessagePrepareBytes(CBGetMessage(self), CBNetworkAddressListCalculateLength(self));
	
}

int CBNetworkAddressListSerialise(CBNetworkAddressList * self, bool force) {
	
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
	CBByteArraySetVarInt(bytes, 0, num);
	
	int cursor = num.size;
	
	for (int x = 0; x < num.val; x++) {
		
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
				for (int y = 0; y < x + 1; y++)
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

int CBNetworkAddressListStringMaxSize(CBNetworkAddressList * self) {
	
	return 3 + 49*self->addrNum;
	
}

void CBNetworkAddressListToString(CBNetworkAddressList * self, char * output) {
	
	*(output++) = '(';
	
	for (int x = 0; x < self->addrNum; x++) {
		output = CBNetworkAddressToString(self->addresses[x], output);
		if (x != self->addrNum - 1) {
			memcpy(output, ", ", 2);
			output += 2;
		}
	}
	
	strcpy(output, ")");
	
}

void CBNetworkAddressListTakeNetworkAddress(CBNetworkAddressList * self, CBNetworkAddress * address) {
	
	self->addresses = realloc(self->addresses, sizeof(*self->addresses) * ++self->addrNum);
	self->addresses[self->addrNum-1] = address;
	
}
