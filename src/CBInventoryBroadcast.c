//
//  CBInventoryBroadcast.c
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

#include "CBInventoryBroadcast.h"

//  Constructors

CBInventoryBroadcast * CBNewInventoryBroadcast(){
	CBInventoryBroadcast * self = malloc(sizeof(*self));
	if (NOT self) {
		CBLogError("Cannot allocate %i bytes of memory in CBNewInventoryBroadcast\n", sizeof(*self));
		return NULL;
	}
	CBGetObject(self)->free = CBFreeInventoryBroadcast;
	if(CBInitInventoryBroadcast(self))
		return self;
	free(self);
	return NULL;
}
CBInventoryBroadcast * CBNewInventoryBroadcastFromData(CBByteArray * data){
	CBInventoryBroadcast * self = malloc(sizeof(*self));
	if (NOT self) {
		CBLogError("Cannot allocate %i bytes of memory in CBNewInventoryBroadcastFromData\n", sizeof(*self));
		return NULL;
	}
	CBGetObject(self)->free = CBFreeInventoryBroadcast;
	if(CBInitInventoryBroadcastFromData(self, data))
		return self;
	free(self);
	return NULL;
}

//  Object Getter

CBInventoryBroadcast * CBGetInventoryBroadcast(void * self){
	return self;
}

//  Initialisers

bool CBInitInventoryBroadcast(CBInventoryBroadcast * self){
	self->itemNum = 0;
	self->items = NULL;
	if (NOT CBInitMessageByObject(CBGetMessage(self)))
		return false;
	return true;
}
bool CBInitInventoryBroadcastFromData(CBInventoryBroadcast * self, CBByteArray * data){
	self->itemNum = 0;
	self->items = NULL;
	if (NOT CBInitMessageByData(CBGetMessage(self), data))
		return false;
	return true;
}

//  Destructor

void CBFreeInventoryBroadcast(void * vself){
	CBInventoryBroadcast * self = vself;
	for (uint16_t x = 0; x < self->itemNum; x++) {
		CBReleaseObject(self->items[x]); // Free item
	}
	free(self->items); // Free item pointer memory block.
	CBFreeMessage(self);
}

//  Functions

uint32_t CBInventoryBroadcastDeserialise(CBInventoryBroadcast * self){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (NOT bytes) {
		CBLogError("Attempting to deserialise a CBInventoryBroadcast with no bytes.");
		return 0;
	}
	if (bytes->length < 37) {
		CBLogError("Attempting to deserialise a CBInventoryBroadcast with less bytes than required for one item.");
		return 0;
	}
	CBVarInt itemNum = CBVarIntDecode(bytes, 0);
	if (itemNum.val > 1388) {
		CBLogError("Attempting to deserialise a CBInventoryBroadcast with a var int over 1388.");
		return 0;
	}
	// Run through the items and deserialise each one.
	self->items = malloc(sizeof(*self->items) * (size_t)itemNum.val);
	if (NOT self->items) {
		CBLogError("Cannot allocate %i bytes of memory in CBInventoryBroadcastDeserialise", sizeof(*self->items) * (size_t)itemNum.val);
		return 0;
	}
	self->itemNum = itemNum.val;
	uint16_t cursor = itemNum.size;
	for (uint16_t x = 0; x < itemNum.val; x++) {
		// Make new CBInventoryItem from the rest of the data.
		CBByteArray * data = CBByteArraySubReference(bytes, cursor, bytes->length-cursor);
		if (NOT data) {
			CBLogError("Could not create a new CBByteArray in CBInventoryBroadcastDeserialise for inventory broadcast number %u.", x);
			return 0;
		}
		self->items[x] = CBNewInventoryItemFromData(data);
		if (NOT self->items[x]) {
			CBLogError("Could not create a new CBInventoryItem in CBInventoryBroadcastDeserialise for inventory broadcast number %u.", x);
			CBReleaseObject(data);
			return 0;
		}
		// Deserialise
		uint8_t len = CBInventoryItemDeserialise(self->items[x]);
		if (NOT len){
			CBLogError("CBInventoryBroadcast cannot be deserialised because of an error with the CBInventoryItem number %u.", x);
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
uint32_t CBInventoryBroadcastCalculateLength(CBInventoryBroadcast * self){
	return CBVarIntSizeOf(self->itemNum) + self->itemNum * 36;
}
uint32_t CBInventoryBroadcastSerialise(CBInventoryBroadcast * self, bool force){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (NOT bytes) {
		CBLogError("Attempting to serialise a CBInventoryBroadcast with no bytes.");
		return 0;
	}
	CBVarInt num = CBVarIntFromUInt64(self->itemNum);
	if (bytes->length < num.size + 36 * self->itemNum) {
		CBLogError("Attempting to deserialise a CBInventoryBroadcast with less bytes than required.");
		return 0;
	}
	CBVarIntEncode(bytes, 0, num);
	uint16_t cursor = num.size;
	for (uint16_t x = 0; x < num.val; x++) {
		if (NOT CBGetMessage(self->items[x])->serialised  // Serailise if not serialised yet.
			// Serialise if force is true.
			|| force
			// If the data shares the same data as the inventory boradcast, re-serialise the inventory item, in case it got overwritten.
			|| CBGetMessage(self->items[x])->bytes->sharedData == bytes->sharedData) {
			if (CBGetMessage(self->items[x])->serialised)
				// Release old byte array
				CBReleaseObject(CBGetMessage(self->items[x])->bytes);
			CBGetMessage(self->items[x])->bytes = CBByteArraySubReference(bytes, cursor, bytes->length-cursor);
			if (NOT CBGetMessage(self->items[x])->bytes) {
				CBLogError("Cannot create a new CBByteArray sub reference in CBInventoryBroadcastSerialise");
				return 0;
			}
			if (NOT CBInventoryItemSerialise(self->items[x])) {
				CBLogError("CBInventoryBroadcast cannot be serialised because of an error with the CBInventoryItem number %u.", x);
				// Release CBByteArray objects to avoid problems overwritting pointer without release, if serialisation is tried again.
				for (uint8_t y = 0; y < x + 1; y++)
					CBReleaseObject(CBGetMessage(self->items[y])->bytes);
				return 0;
			}
		}else{
			// Move serialsed data to one location
			CBByteArrayCopyByteArray(bytes, cursor, CBGetMessage(self->items[x])->bytes);
			CBByteArrayChangeReference(CBGetMessage(self->items[x])->bytes, bytes, cursor);
		}
		cursor += CBGetMessage(self->items[x])->bytes->length;
	}
	// Ensure length is correct
	bytes->length = cursor;
	// Is now serialised.
	CBGetMessage(self)->serialised = true;
	return cursor;
}

