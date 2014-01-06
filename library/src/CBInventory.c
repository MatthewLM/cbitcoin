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
	self->itemFront = NULL;
	CBInitMessageByObject(CBGetMessage(self));
}
void CBInitInventoryFromData(CBInventory * self, CBByteArray * data){
	self->itemNum = 0;
	self->itemFront = NULL;
	CBInitMessageByData(CBGetMessage(self), data);
}

//  Destructor

void CBDestroyInventory(void * vself){
	CBInventory * self = vself;
	for (CBInventoryItem * item = self->itemFront; item != NULL;) {
		CBInventoryItem * next = item->next;
		CBReleaseObject(item); // Free item
		item = next;
	}
	CBDestroyMessage(self);
}
void CBFreeInventory(void * self) {
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
		return CB_DESERIALISE_ERROR;
	}
	if (bytes->length < 37) {
		CBLogError("Attempting to deserialise a CBInventory with less bytes than required for one item.");
		return CB_DESERIALISE_ERROR;
	}
	CBVarInt itemNum = CBVarIntDecode(bytes, 0);
	if (itemNum.val > 50000) {
		CBLogError("Attempting to deserialise a CBInventory with a var int over 50000.");
		return CB_DESERIALISE_ERROR;
	}
	self->itemNum = 0;
	self->itemFront = NULL;
	// Run through the items and deserialise each one.
	uint32_t cursor = itemNum.size;
	for (uint16_t x = 0; x < itemNum.val; x++) {
		// Make new CBInventoryItem from the rest of the data.
		CBByteArray * data = CBByteArraySubReference(bytes, cursor, bytes->length-cursor);
		CBInventoryItem * item = CBNewInventoryItemFromData(data);
		// Deserialise
		uint32_t len = CBInventoryItemDeserialise(item);
		if (len == CB_DESERIALISE_ERROR){
			CBLogError("CBInventory cannot be deserialised because of an error with the CBInventoryItem number %u.", x);
			CBReleaseObject(data);
			return CB_DESERIALISE_ERROR;
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
CBInventoryItem * CBInventoryPopInventoryItem(CBInventory * self){
	CBInventoryItem * item = self->itemFront;
	if (item != NULL){
		self->itemFront = item->next;
		self->itemNum--;
	}
	return item;
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
	for (CBInventoryItem * item = self->itemFront; item != NULL; item = item->next) {
		if (! CBGetMessage(item)->serialised  // Serialise if not serialised yet.
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
				CBLogError("CBInventory cannot be serialised because of an error with an CBInventoryItem.");
				// Release CBByteArray objects to avoid problems overwritting pointer without release, if serialisation is tried again.
				for (CBInventoryItem * item2 = self->itemFront; ; item2 = item->next){
					CBReleaseObject(item2);
					if (item2 == item)
						break;
				}
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
	if (self->itemFront == NULL)
		self->itemFront = self->itemBack = item;
	else
		self->itemBack = self->itemBack->next = item;
	item->next = NULL;
	self->itemNum++;
	return true;
}
