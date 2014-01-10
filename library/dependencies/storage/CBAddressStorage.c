//
//  CBAddressStorage.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 18/01/2013.
//  Copyright (c) 2013 Matthew Mitchell
//
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

//  SEE HEADER FILE FOR DOCUMENTATION

#include "CBAddressStorage.h"

bool CBNewAddressStorage(CBDepObject * storage, CBDepObject database){
	CBAddressStorage * self = malloc(sizeof(*self));
	self->database = database.ptr;
	self->addrs = CBLoadIndex(self->database, CB_INDEX_ADDRS, 18, 10000);
	if (! self->addrs) {
		CBLogError("Could not load address index.");
		return false;
	}
	storage->ptr = self;
	return true;
}
void CBFreeAddressStorage(CBDepObject self){
	CBFreeIndex(((CBAddressStorage *)self.ptr)->addrs);
	free(self.ptr);
}
bool CBAddressStorageDeleteAddress(CBDepObject uself, void * address){
	CBAddressStorage * self = uself.ptr;
	CBNetworkAddress * addrObj = address;
	uint8_t key[18];
	memcpy(key, CBByteArrayGetData(addrObj->sockAddr.ip), 16);
	CBInt16ToArray(key, 16, addrObj->sockAddr.port);
	// Remove address
	if (! CBDatabaseRemoveValue(self->addrs, key, false)) {
		CBLogError("Could not remove an address from storage.");
		return false;
	}
	// Decrease number of addresses
	CBInt32ToArray(self->database->current.extraData, CB_NUM_ADDRS, CBArrayToInt32(self->database->current.extraData, CB_NUM_ADDRS) - 1);
	// Commit changes
	if (! CBDatabaseStage(self->database)) {
		CBLogError("Could not commit the removal of a network address.");
		return false;
	}
	return true;
}
uint64_t CBAddressStorageGetNumberOfAddresses(CBDepObject uself){
	return CBArrayToInt64(((CBAddressStorage *)uself.ptr)->database->current.extraData, 0);
}
bool CBAddressStorageLoadAddresses(CBDepObject uself, void * addrMan){
	CBAddressStorage * self = uself.ptr;
	CBNetworkAddressManager * addrManObj = addrMan;
	CBDatabaseRangeIterator it;
	CBInitDatabaseRangeIterator(&it, (uint8_t [18]){0}, (uint8_t [18]){0xFF}, self->addrs);
	CBIndexFindStatus status = CBDatabaseRangeIteratorFirst(&it);
	if (status == CB_DATABASE_INDEX_ERROR) {
		CBLogError("Could not get the first address from the database.");
		return false;
	}
	while(status == CB_DATABASE_INDEX_FOUND) {
		uint8_t key[18];
		CBDatabaseRangeIteratorGetKey(&it, key);
		uint8_t data[20];
		if (! CBDatabaseRangeIteratorRead(&it, data, 20, 0)) {
			CBLogError("Could not read address data from database.");
			return false;
		}
		// Create network address object and add to the address manager.
		CBByteArray * ip = CBNewByteArrayWithDataCopy(key, 16);
		CBNetworkAddress * addr = CBNewNetworkAddress(CBArrayToInt64(data, 0),
													  (CBSocketAddress){ip,CBArrayToInt16(key, 16)},
													  (CBVersionServices) CBArrayToInt64(data, 8),
													  true);
		addr->penalty = CBArrayToInt32(data, 16);
		CBNetworkAddressManagerTakeAddress(addrManObj, addr);
		CBReleaseObject(ip);
		status = CBDatabaseRangeIteratorNext(&it);
		if (status == CB_DATABASE_INDEX_ERROR) {
			CBLogError("Could not iterate to the next address in the database.");
			return false;
		}
	}
	return true;
}
bool CBAddressStorageSaveAddress(CBDepObject uself, void * address){
	CBAddressStorage * self = uself.ptr;
	CBNetworkAddress * addrObj = address;
	uint8_t key[18], data[20];
	// Create key
	memcpy(key, CBByteArrayGetData(addrObj->sockAddr.ip), 16);
	CBInt16ToArray(key, 16, addrObj->sockAddr.port);
	// Create data
	CBInt64ToArray(data, 0, addrObj->lastSeen);
	CBInt64ToArray(data, 8, (uint64_t) addrObj->services);
	CBInt32ToArray(data, 16, addrObj->penalty);
	// Write data
	CBDatabaseWriteValue(self->addrs, key, data, 20);
	// Increase the number of addresses
	CBInt32ToArray(self->database->current.extraData, CB_NUM_ADDRS, CBArrayToInt32(self->database->current.extraData, CB_NUM_ADDRS) + 1);
	// Commit changes
	if (! CBDatabaseStage(self->database)) {
		CBLogError("Could not commit adding a new network address to storage.");
		return false;
	}
	return true;
}
