//
//  CBAddressStorage.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 18/01/2013.
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

#include "CBDatabase.h"
#include "CBAddressManager.h"

uint8_t CB_ADDR_NUM_KEY[1] = {0};
uint8_t CB_ADDRESS_KEY[19] = {18,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
uint8_t CB_DATA_ARRAY[20];

uint64_t CBNewAddressStorage(char * dataDir){
	CBDatabase * database = CBNewDatabase(dataDir, "addr");
	if (NOT database) {
		CBLogError("Could not create a database object for address storage.");
		return 0;
	}
	if (NOT CBDatabaseGetLength(database, CB_ADDR_NUM_KEY)) {
		// Insert zero number of addresses.
		CBInt32ToArray(CB_DATA_ARRAY, 0, 0);
		if (NOT CBDatabaseWriteValue(database, CB_ADDR_NUM_KEY, CB_DATA_ARRAY, 4)) {
			CBLogError("Could not write the initial number of addresses to storage.");
			return 0;
		}
		// Commit changes
		if (NOT CBDatabaseCommit(database)) {
			CBLogError("Could not commit the initial number of addresses to storage.");
			return 0;
		}
	}
	return (uint64_t) database;
}
void CBFreeAddressStorage(uint64_t iself){
	CBFreeDatabase((CBDatabase *)iself);
}
bool CBAddressStorageDeleteAddress(uint64_t iself, void * address){
	CBDatabase * database = (CBDatabase *)iself;
	CBNetworkAddress * addrObj = address;
	memcpy(CB_ADDRESS_KEY + 1, CBByteArrayGetData(addrObj->ip), 16);
	CBInt16ToArray(CB_ADDRESS_KEY, 17, addrObj->port);
	// Remove address
	if (NOT CBDatabaseRemoveValue(database, CB_ADDRESS_KEY)) {
		CBLogError("Could not remove an address from storage.");
		return false;
	}
	// Decrease number of addresses
	uint32_t addrNum;
	if (NOT CBAddressStorageGetNumberOfAddresses(iself, &addrNum))
		return false;
	CBInt32ToArray(CB_DATA_ARRAY, 0, addrNum - 1);
	if (NOT CBDatabaseWriteValue(database, CB_ADDR_NUM_KEY, CB_DATA_ARRAY, 4)) {
		CBLogError("Could not write the new number of addresses to storage.");
		return false;
	}
	// Commit changes
	if (NOT CBDatabaseCommit(database)) {
		CBLogError("Could not commit the removal of a network address.");
		return false;
	}
	return true;
}
bool CBAddressStorageGetNumberOfAddresses(uint64_t iself, uint32_t * num){
	CBDatabase * database = (CBDatabase *)iself;
	if (NOT CBDatabaseReadValue(database, CB_ADDR_NUM_KEY, CB_DATA_ARRAY, 4, 0)) {
		CBLogError("Could not read the number of addresses from storage.");
		return false;
	}
	*num = CBArrayToInt32(CB_DATA_ARRAY, 0);
	return true;
}
bool CBAddressStorageLoadAddresses(uint64_t iself, void * addrMan){
	CBDatabase * database = (CBDatabase *)iself;
	CBAddressManager * addrManObj = addrMan;
	CBIterator it;
	// The first element shoudl be the number of addresses so get second element to begin with.
	if (CBAssociativeArrayGetElement(&database->index, &it, 1)) for(;;) {
		uint8_t * key = it.node->elements[it.pos];
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
														  (CBVersionServices) CBArrayToInt64(CB_DATA_ARRAY, 8));
			addr->penalty = CBArrayToInt32(CB_DATA_ARRAY, 16);
			if (NOT CBAddressManagerTakeAddress(addrManObj, addr)) {
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
	CBDatabase * database = (CBDatabase *)iself;
	CBNetworkAddress * addrObj = address;
	// Create key
	memcpy(CB_ADDRESS_KEY + 1, CBByteArrayGetData(addrObj->ip), 16);
	CBInt16ToArray(CB_ADDRESS_KEY, 17, addrObj->port);
	// Create data
	CBInt64ToArray(CB_DATA_ARRAY, 0, addrObj->lastSeen);
	CBInt64ToArray(CB_DATA_ARRAY, 8, addrObj->services);
	CBInt32ToArray(CB_DATA_ARRAY, 16, addrObj->penalty);
	// Write data
	if (NOT CBDatabaseWriteValue(database, CB_ADDRESS_KEY, CB_DATA_ARRAY, 20)) {
		CBLogError("Could not write an address to storage.");
		return false;
	}
	// Increase the number of addresses
	uint32_t addrNum;
	if (NOT CBAddressStorageGetNumberOfAddresses(iself, &addrNum))
		return false;
	CBInt32ToArray(CB_DATA_ARRAY, 0, addrNum + 1);
	if (NOT CBDatabaseWriteValue(database, CB_ADDR_NUM_KEY, CB_DATA_ARRAY, 4)) {
		CBLogError("Could not write the new number of addresses to storage.");
		return false;
	}
	// Commit changes
	if (NOT CBDatabaseCommit(database)) {
		CBLogError("Could not commit adding a new network address to storage.");
		return false;
	}
	return true;
}
