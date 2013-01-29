//
//  testCBAddressManager.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 07/08/2012.
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

#include <stdio.h>
#include "CBAddressManager.h"
#include "CBDependencies.h"
#include <time.h>
#include "stdarg.h"

void CBLogError(char * format, ...){
	va_list argptr;
    va_start(argptr, format);
    vfprintf(stderr, format, argptr);
    va_end(argptr);
	printf("\n");
}
void onBadTime(void * foo);
void onBadTime(void * foo){
	printf("BAD TIME FAIL\n");
	exit(EXIT_FAILURE);
}
uint32_t getLen(CBBTreeNode * self);
uint32_t getLen(CBBTreeNode * self){
	uint32_t len = self->numElements;
	if (self->children[0])
		for (uint8_t x = 0; x < self->numElements + 1; x++)
			len += getLen(self->children[x]);
	return len;
}

int main(){
	unsigned int s = (unsigned int)time(NULL);
	s = 1337544566;
	printf("Session = %ui\n", s);
	srand(s);
	CBAddressManager * addrMan = CBNewAddressManager(onBadTime);
	if (NOT addrMan) {
		printf("NEW ADDR MAN FAIL\n");
		return 1;
	}
	// Test adding addresses
	for (uint8_t x = 0; x < 255; x++) {
		CBByteArray * ip = CBNewByteArrayWithDataCopy((uint8_t []){0x20,0x01,0x0D,0xB8,0x85,0xA3,0x00,0x42,0x10,0x00,0x8A,0x2E,0x03,0x70,0x73,x/2}, 16);
		CBNetworkAddress * addr = CBNewNetworkAddress(1358856884 + rand() % 15, ip, 45562 + (rand() % 5) + 6 * (x % 2), CB_SERVICE_FULL_BLOCKS, true);
		addr->penalty = rand() % 20;
		CBAddressManagerTakeAddress(addrMan, addr);
		CBReleaseObject(ip);
		// Check length
		uint8_t len = 0;
		for (uint8_t x = 0; x < CB_BUCKET_NUM; x++)
			len += getLen(addrMan->addresses[x].root);
		if (len != addrMan->addrNum) {
			printf("ASSOC ARRAY AND ADDR NUM MATCH FAIL");
			return 1;
		}
	}
	// Test ordering
	for (uint8_t x = 0; x < CB_BUCKET_NUM; x++) {
		CBPosition it;
		if (CBAssociativeArrayGetFirst(&addrMan->addresses[x], &it)) {
			uint64_t lastScore = 0xFFFFFFFFFFFFFFFF;
			uint8_t lastIP[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
			uint16_t lastPort = 0;
			for (;;) {
				CBNetworkAddress * addr = it.node->elements[it.index];
				uint64_t score = addr->lastSeen - addr->penalty;
				if (score > lastScore){
					printf("ADDR LAST SEEN ORDER FAIL\n");
					return 1;
				}else if (score < lastScore){
					memset(lastIP, 0, 16);
					lastPort = 0;
				}else{
					int res = memcmp(CBByteArrayGetData(addr->ip), lastIP, 16);
					if (res < 0) {
						printf("ADDR IP ORDER FAIL");
						return 1;
					}else if (res > 0){
						lastPort = 0;
					}else if (addr->port <= lastPort){
						printf("ADDR PORT ORDER/DUPLICATE FAIL");
						return 1;
					}
				}
				// Test getting addresses
				CBNetworkAddress * addr2 = CBAddressManagerGotNetworkAddress(addrMan, addr);
				if (NOT addr2 || addr2 != addr) {
					printf("GOT NETWORK ADDR FAIL");
					return 1;
				}
				CBReleaseObject(addr2);
				if (CBAssociativeArrayIterate(&addrMan->addresses[x], &it))
					break;
			}
		}
	}
	// Test removing an address
	CBNetworkAddress * addr[1];
	if (CBAddressManagerGetAddresses(addrMan, 1, addr) != 1) {
		printf("GET SINGLE ADDR FAIL\n");
		return 1;
	}
	CBRetainObject(*addr);
	CBAddressManagerRemoveAddress(addrMan, *addr);
	*addr = CBAddressManagerGotNetworkAddress(addrMan, *addr);
	if (*addr) {
		printf("REMOVE NETWORK ADDR FAIL");
		return 1;
	}
	// Test getting addresses under number
	CBNetworkAddress * addrs[255];
	if (CBAddressManagerGetAddresses(addrMan, 253, addrs) != 253) {
		printf("GET ADDRS BELOW FAIL\n");
		return 1;
	}
	// Test getting addresses at number
	if (CBAddressManagerGetAddresses(addrMan, 254, addrs) != 254) {
		printf("GET ADDRS AT FAIL\n");
		return 1;
	}
	// Test getting addresses over number
	if (CBAddressManagerGetAddresses(addrMan, 255, addrs) != 254) {
		printf("GET ADDRS OVER FAIL\n");
		return 1;
	}
	// Check no duplicates
	for (uint8_t x = 0; x < 254; x++) { 
		for (uint8_t y = 0; y < 254; y++) {
			if (y != x && CBNetworkAddressCompare(addrs[x], addrs[y]) == CB_COMPARE_EQUAL) {
				printf("GET ADDRS NO DUPLICATES FAIL\n");
				return 1;
			}
		}
	}
	// Add peers
	for (uint8_t x = 0; x < CB_BUCKET_NUM; x++){
		CBFreeAssociativeArray(&addrMan->addresses[x]);
		CBInitAssociativeArray(&addrMan->addresses[x], CBNetworkAddressCompare, CBReleaseObject);
	}
	CBPeer * peers[254];
	for (uint8_t x = 0; x < 254; x++) {
		peers[x] = CBNewPeerByTakingNetworkAddress(addrs[x]);
		if (NOT CBAddressManagerAddPeer(addrMan, peers[x])) {
			printf("ADD PEER FAIL\n");
			return 1;
		}
	}
	if (addrMan->peersNum != 254) {
		printf("ADD PEERS NUM FAIL\n");
		return 1;
	}
	// Test removing peers and looking for peers
	CBAddressManagerRemovePeer(addrMan, peers[0]);
	CBAddressManagerRemovePeer(addrMan, peers[34]);
	CBAddressManagerRemovePeer(addrMan, peers[253]);
	if (addrMan->peersNum != 251) {
		printf("REMOVE PEERS NUM FAIL\n");
		return 1;
	}
	for (uint8_t x = 0; x < 254; x++) {
		CBPeer * peer = CBAddressManagerGotPeer(addrMan, CBGetNetworkAddress(peers[x]));
		if (x == 0 || x == 34 || x == 253) {
			if (peer) {
				printf("REMOVE PEER FAIL\n");
				return 1;
			}
		}else if (peer != peers[x]){
			printf("ADD PEER FAIL\n");
			return 1;
		}
	}
	// Test clear peers and then network time.
	CBAddressManagerClearPeers(addrMan);
	if (addrMan->peersNum) {
		printf("CLEAR PEERS NUM FAIL\n");
		return 1;
	}
	if (getLen(addrMan->peers.root)) {
		printf("CLEAR PEERS GET LEN FAIL\n");
		return 1;
	}
	peers[0]->timeOffset = 483;
	peers[1]->timeOffset = 1839;
	peers[2]->timeOffset = 3795;
	peers[3]->timeOffset = 8394;
	peers[4]->timeOffset = 4603;
	peers[5]->timeOffset = 72787;
	for (uint8_t x = 0; x < 6; x++)
		CBAddressManagerAddPeer(addrMan, peers[x]);
	// Median is 4199
	if (addrMan->peersNum != 6) {
		printf("ADD SIX PEERS NUM FAIL\n");
		return 1;
	}
	if (addrMan->networkTimeOffset != 4199) {
		printf("MEDIAN TIME FAIL\n");
		return 1;
	}
	CBReleaseObject(addrMan);
	// Test address storage
	remove("./addr_0.dat");
	remove("./addr_1.dat");
	remove("./addr_2.dat");
	remove("./addr_log.dat");
	uint64_t storage = CBNewAddressStorage("./");
	if (NOT storage) {
		printf("NEW ADDRESS STORAGE FAIL\n");
		return 1;
	}
	// Add 10 addresses to storage
	for (uint8_t x = 0; x < 10; x++) {
		CBByteArray * ip = CBNewByteArrayWithDataCopy((uint8_t []){0x20,0x01,0x0D,0xB8,0x85,0xA3,0x00,0x42,0x10,0x00,0x8A,0x2E,0x03,0x70,0x73,x}, 16);
		addrs[x] = CBNewNetworkAddress(x, ip, x, x % 2, true);
		addrs[x]->penalty = x;
		CBReleaseObject(ip);
		if (NOT CBAddressStorageSaveAddress(storage, addrs[x])){
			printf("SAVE ADDRESS FAIL\n");
			return 1;
		}
	}
	// Remove 5 of them
	for (uint8_t x = 0; x < 10; x += 2) {
		if (NOT CBAddressStorageDeleteAddress(storage, addrs[x])){
			printf("REMOVE ADDRESS FAIL\n");
			return 1;
		}
	}
	// Test loading addresses
	addrMan = CBNewAddressManager(onBadTime);
	if (NOT CBAddressStorageLoadAddresses(storage, addrMan)) {
		printf("LOAD ADDRESSES FAIL\n");
		return 1;
	}
	if (addrMan->addrNum != 5) {
		printf("STORAGE ADDR NUM FAIL\n");
		return 1;
	}
	// Test select and remove
	CBNetworkAddress * removed = CBAddressManagerSelectAndRemoveAddress(addrMan);
	if (removed->penalty != 9) {
		printf("REMOVE PENALTY FAIL\n");
		return 1;
	}
	CBReleaseObject(removed);
	// Check each remaining address.
	if (CBAddressManagerGetAddresses(addrMan, 5, addrs) != 4) {
		printf("STORAGE GET ADDRS FAIL\n");
		return 1;
	}
	for (uint8_t x = 0; x < 4; x++) {
		uint8_t val = x*2 + 1;
		if (addrs[x]->lastSeen != val) {
			printf("STORAGE LOAD LAST SEEN FAIL\n");
			return 1;
		}
		if (addrs[x]->port != val) {
			printf("STORAGE LOAD PORT FAIL\n");
			return 1;
		}
		if (addrs[x]->penalty != val) {
			printf("STORAGE LOAD PENALTY FAIL\n");
			return 1;
		}
		if (addrs[x]->services != val % 2) {
			printf("STORAGE LOAD SERVICES FAIL\n");
			return 1;
		}
		if (memcmp((uint8_t []){0x20,0x01,0x0D,0xB8,0x85,0xA3,0x00,0x42,0x10,0x00,0x8A,0x2E,0x03,0x70,0x73,val}, CBByteArrayGetData(addrs[x]->ip), 16)) {
			printf("STORAGE LOAD IP FAIL\n");
			return 1;
		}
	}
	CBReleaseObject(addrMan);
	CBFreeAddressStorage(storage);
	return 0;
}
