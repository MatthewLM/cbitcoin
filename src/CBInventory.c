//
//  CBInventory.c
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

#include "CBInventory.h"

//  Constructors

CBInventory * CBNewInventory(){
	CBInventory * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeInventory;
	CBInitInventory(self);
	return self;
}
CBInventory * CBNewInventoryFromData(CBByteArray * data){
	CBInventory * self = malloc(sizeof(*self));
	CBGetObject(self)->free = CBFreeInventory;
	CBInitInventoryFromData(self, data);
	return self;
}

//  Initialisers

void CBInitInventory(CBInventory * self){
	self->itemNum = 0;
	CBInitMessageByObject(CBGetMessage(self));
}
void CBInitInventoryFromData(CBInventory * self, CBByteArray * data){
	self->itemNum = 0;
	CBInitMessageByData(CBGetMessage(self), data);
}

//  Destructor

void CBDestroyInventory(void * vself){
	CBInventory * self = vself;
	for (uint16_t x = 0; x < self->itemNum; x++) {
		CBReleaseObject(self->items[x / 250][x % 250]); // Free item
		if (x % 250 == 249)
			free(self->items[x / 250]); // Free item pointer memory block.
	}
	CBDestroyMessage(self);
}
void CBFreeInventory(void * self){
	CBDestroyInventory(self);
	free(self);
}

//  Functions

bool CBInventoryAddInventoryItem(CBInventory * self, CBInventoryItem * item){
	bool taken = CBInventoryTakeInventoryItem(self, item);
	if (taken)
		CBRetainObject(item);
	return taken;
}
uint32_t CBInventoryDeserialise(CBInventory * self){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (! bytes) {
		CBLogError("Attempting to deserialise a CBInventory with no bytes.");
		return 0;
	}
	if (bytes->length < 37) {
		CBLogError("Attempting to deserialise a CBInventory with less bytes than required for one item.");
		return 0;
	}
	CBVarInt itemNum = CBVarIntDecode(bytes, 0);
	if (itemNum.val > 50000) {
		CBLogError("Attempting to deserialise a CBInventory with a var int over 50000.");
		return 0;
	}
	// Run through the items and deserialise each one.
	uint32_t cursor = itemNum.size;
	for (uint16_t x = 0; x < itemNum.val; x++) {
		// Make new CBInventoryItem from the rest of the data.
		CBByteArray * data = CBByteArraySubReference(bytes, cursor, bytes->length-cursor);
		CBInventoryItem * item = CBNewInventoryItemFromData(data);
		// Deserialise
		uint8_t len = CBInventoryItemDeserialise(item);
		if (!len){
			CBLogError("CBInventory cannot be deserialised because of an error with the CBInventoryItem number %u.", x);
			CBReleaseObject(data);
			return 0;
		}
		// Take item
		CBInventoryTakeInventoryItem(self, item);
		// Adjust length
		data->length = len;
		CBReleaseObject(data);
		cursor += len;
	}
	return cursor;
}
CBInventoryItem * CBInventoryGetInventoryItem(CBInventory * self, uint16_t x){
	return self->items[x / 250][x % 250];
}
uint32_t CBInventoryCalculateLength(CBInventory * self){
	return CBVarIntSizeOf(self->itemNum) + self->itemNum * 36;
}
uint32_t CBInventorySerialise(CBInventory * self, bool force){
	CBByteArray * bytes = CBGetMessage(self)->bytes;
	if (! bytes) {
		CBLogError("Attempting to serialise a CBInventory with no bytes.");
		return 0;
	}
	CBVarInt num = CBVarIntFromUInt64(self->itemNum);
	if (bytes->length < num.size + 36 * self->itemNum) {
		CBLogError("Attempting to deserialise a CBInventory with less bytes than required.");
		return 0;
	}
	CBVarIntEncode(bytes, 0, num);
	uint32_t cursor = num.size;
	for (uint16_t x = 0; x < num.val; x++) {
		// Get the item
		CBInventoryItem * item = CBInventoryGetInventoryItem(self, x);
		if (! CBGetMessage(item)->serialised  // Serailise if not serialised yet.
			// Serialise if force is true.
			|| force
			// If the data shares the same data as the inventory boradcast, re-serialise the inventory item, in case it got overwritten.
			|| CBGetMessage(item)->bytes->sharedData == bytes->sharedData) {
			if (CBGetMessage(item)->serialised)
				// Release old byte array
				CBReleaseObject(CBGetMessage(item)->bytes);
			CBGetMessage(item)->bytes = CBByteArraySubReference(bytes, cursor, bytes->length-cursor);
			if (! CBGetMessage(item)->bytes) {
				CBLogError("Cannot create a new CBByteArray sub reference in CBInventorySerialise");
				return 0;
			}
			if (!CBInventoryItemSerialise(item)) {
				CBLogError("CBInventory cannot be serialised because of an error with the CBInventoryItem number %u.", x);
				// Release CBByteArray objects to avoid problems overwritting pointer without release, if serialisation is tried again.
				for (uint8_t y = 0; y < x + 1; y++)
					CBReleaseObject(CBGetMessage(self->items[y / 250][y % 250])->bytes);
				return 0;
			}
		}else{
			// Move serialsed data to one location
			CBByteArrayCopyByteArray(bytes, cursor, CBGetMessage(item)->bytes);
			CBByteArrayChangeReference(CBGetMessage(item)->bytes, bytes, cursor);
		}
		cursor += CBGetMessage(item)->bytes->length;
	}
	// Ensure length is correct
	bytes->length = cursor;
	// Is now serialised.
	CBGetMessage(self)->serialised = true;
	return cursor;
}
bool CBInventoryTakeInventoryItem(CBInventory * self, CBInventoryItem * item){
	if (self->itemNum == 50000)
		return false;
	if (self->itemNum % 250 == 0)
		self->items[self->itemNum / 250] = malloc(sizeof(CBInventoryItem) * 250);
	self->items[self->itemNum / 250][self->itemNum % 250] = item;
	self->itemNum++;
	return true;
}