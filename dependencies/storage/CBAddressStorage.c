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

uint8_t CB_ADDR_NUM_KEY[1] = {0};
uint8_t CB_ADDRESS_KEY[19] = {18,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
uint8_t CB_DATA_ARRAY[20];

uint64_t CBNewAddressStorage(char * dataDir){
	CBAddressStore * self = malloc(sizeof(*self));
	if (NOT self) {
		CBLogError("Could not create the address storage object.");
		return 0;
	}
	if (NOT CBInitDatabase(CBGetDatabase(self), dataDir, "addr")) {
		CBLogError("Could not create a database object for address storage.");
		free(self);
		return 0;
	}
	if (CBDatabaseGetLength(CBGetDatabase(self), CB_ADDR_NUM_KEY) != CB_DOESNT_EXIST) {
		// Get the number of addresses
		if (NOT CBDatabaseReadValue(CBGetDatabase(self), CB_ADDR_NUM_KEY, CB_DATA_ARRAY, 4, 0)) {
			CBLogError("Could not read the number of addresses from storage.");
			free(self);
			return 0;
		}
		self->numAddresses = CBArrayToInt32(CB_DATA_ARRAY, 0);
	}else{
		self->numAddresses = 0;
		// Insert zero number of addresses.
		CBInt32ToArray(CB_DATA_ARRAY, 0, 0);
		if (NOT CBDatabaseWriteValue(CBGetDatabase(self), CB_ADDR_NUM_KEY, CB_DATA_ARRAY, 4)) {
			CBLogError("Could not write the initial number of addresses to storage.");
			free(self);
			return 0;
		}
		// Commit changes
		if (NOT CBDatabaseCommit(CBGetDatabase(self))) {
			CBLogError("Could not commit the initial number of addresses to storage.");
			free(self);
			return 0;
		}
	}
	return (uint64_t) self;
}
void CBFreeAddressStorage(uint64_t iself){
	CBFreeDatabase((CBDatabase *)iself);
}
bool CBAddressStorageDeleteAddress(uint64_t iself, void * address){
	CBAddressStore * self = (CBAddressStore *)iself;
	CBNetworkAddress * addrObj = address;
	memcpy(CB_ADDRESS_KEY + 1, CBByteArrayGetData(addrObj->ip), 16);
	CBInt16ToArray(CB_ADDRESS_KEY, 17, addrObj->port);
	// Remove address
	if (NOT CBDatabaseRemoveValue(CBGetDatabase(self), CB_ADDRESS_KEY)) {
		CBLogError("Could not remove an address from storage.");
		return false;
	}
	// Decrease number of addresses
	CBInt32ToArray(CB_DATA_ARRAY, 0, --self->numAddresses);
	if (NOT CBDatabaseWriteValue(CBGetDatabase(self), CB_ADDR_NUM_KEY, CB_DATA_ARRAY, 4)) {
		CBLogError("Could not write the new number of addresses to storage.");
		return false;
	}
	// Commit changes
	if (NOT CBDatabaseCommit(CBGetDatabase(self))) {
		CBLogError("Could not commit the removal of a network address.");
		return false;
	}
	return true;
}
uint64_t CBAddressStorageGetNumberOfAddresses(uint64_t iself){
	return ((CBAddressStore *)iself)->numAddresses;
}
bool CBAddressStorageLoadAddresses(uint64_t iself, void * addrMan){
	CBDatabase * database = (CBDatabase *)iself;
	CBNetworkAddressManager * addrManObj = addrMan;
	CBPosition it;
	// The first element shoudl be the number of addresses so get second element to begin with.
	if (CBAssociativeArrayGetElement(&database->index, &it, 1)) for(;;) {
		uint8_t * key = it.node->elements[it.index];
		CBIndexValue * val = (CBIndexValue *)(key + *key + 1);
		if (val->length) {
			// Is not deleted.
			// Read data
			uint64_t file = CBDatabaseGetFile(database, val->fileID);
			if (NOT file) {
				CBLogError("Could not open a file for loading a network address.");
				return false;
			}
			if (NOT CBFileSeek(file, val->pos)){
				CBLogError("Could not read seek a file to a network address.");
				return false;
			}
			if (NOT CBFileRead(file, CB_DATA_ARRAY, 20)) {
				CBLogError("Could not read a network address from a file.");
				return false;
			}
			// Create network address object and add to the address manager.
			CBByteArray * ip = CBNewByteArrayWithDataCopy(key + 1, 16);
			CBNetworkAddress * addr = CBNewNetworkAddress(CBArrayToInt64(CB_DATA_ARRAY, 0),
														  ip,
														  CBArrayToInt16(key, 17),
														  (CBVersionServices) CBArrayToInt64(CB_DATA_ARRAY, 8),
														  true);
			addr->penalty = CBArrayToInt32(CB_DATA_ARRAY, 16);
			if (NOT CBNetworkAddressManagerTakeAddress(addrManObj, addr)) {
				CBLogError("Could not create and add a network address to the address manager.");
				return false;
			}
			CBReleaseObject(ip);
		}
		if (CBAssociativeArrayIterate(&database->index, &it))
			break;
	}
	return true;
}
bool CBAddressStorageSaveAddress(uint64_t iself, void * address){
	CBAddressStore * self = (CBAddressStore *)iself;
	CBNetworkAddress * addrObj = address;
	// Create key
	memcpy(CB_ADDRESS_KEY + 1, CBByteArrayGetData(addrObj->ip), 16);
	CBInt16ToArray(CB_ADDRESS_KEY, 17, addrObj->port);
	// Create data
	CBInt64ToArray(CB_DATA_ARRAY, 0, addrObj->lastSeen);
	CBInt64ToArray(CB_DATA_ARRAY, 8, (uint64_t) addrObj->services);
	CBInt32ToArray(CB_DATA_ARRAY, 16, addrObj->penalty);
	// Write data
	if (NOT CBDatabaseWriteValue(CBGetDatabase(self), CB_ADDRESS_KEY, CB_DATA_ARRAY, 20)) {
		CBLogError("Could not write an address to storage.");
		return false;
	}
	// Increase the number of addresses
	CBInt32ToArray(CB_DATA_ARRAY, 0, --self->numAddresses);
	if (NOT CBDatabaseWriteValue(CBGetDatabase(self), CB_ADDR_NUM_KEY, CB_DATA_ARRAY, 4)) {
		CBLogError("Could not write the new number of addresses to storage.");
		return false;
	}
	// Commit changes
	if (NOT CBDatabaseCommit(CBGetDatabase(self))) {
		CBLogError("Could not commit adding a new network address to storage.");
		return false;
	}
	return true;
}
