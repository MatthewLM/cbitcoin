//
//  CBInventoryBroadcast.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 10/07/2012.
//  Copyright (c) 2012 Matthew Mitchell
//  
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

//  SEE HEADER FILE FOR DOCUMENTATION 

#include "CBInventoryBroadcast.h"

//  Constructors

CBInventoryBroadcast * CBNewInventoryBroadcast(){
	CBInventoryBroadcast * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeInventoryBroadcast;
	CBInitInventoryBroadcast(self);
	return self;
}
CBInventoryBroadcast * CBNewInventoryBroadcastFromData(CBByteArray * data){
	CBInventoryBroadcast * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeInventoryBroadcast;
	CBInitInventoryBroadcastFromData(self, data);
	return self;
}

//  Initialisers

void CBInitInventoryBroadcast(CBInventoryBroadcast * self){
	self->itemNum = 0;
	self->items = NULL;
	CBInitMessageByObject(CBGetMessage(self));
}
void CBInitInventoryBroadcastFromData(CBInventoryBroadcast * self, CBByteArray * data){
	self->itemNum = 0;
	self->items = NULL;
	CBInitMessageByData(CBGetMessage(self), data);
}

//  Destructor

void CBDestroyInventoryBroadcast(void * vself){
	CBInventoryBroadcast * self = vself;
	for (uint16_t x = 0; x < self->itemNum; x++) {
		CBReleaseObject(self->items[x]); // Free item
	}
	free(self->items); // Free item pointer memory block.
	CBDestroyMessage(self);
}
void CBFreeInventoryBroadcast(void * self){
	CBDestroyInventoryBroadcast(self);
	free(self);
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
	self->itemNum = itemNum.val;
	uint16_t cursor = itemNum.size;
	for (uint16_t x = 0; x < itemNum.val; x++) {
		// Make new CBInventoryItem from the rest of the data.
		CBByteArray * data = CBByteArraySubReference(bytes, cursor, bytes->length-cursor);
		self->items[x] = CBNewInventoryItemFromData(data);
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

