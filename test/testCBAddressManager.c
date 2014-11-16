//
//  testCBNetworkAddressManager.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 07/08/2012.
//  Copyright (c) 2012 Matthew Mitchell
//
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

#include <stdio.h>
#include "CBNetworkAddressManager.h"
#include "CBDependencies.h"
#include <time.h>
#include "stdarg.h"
#include <sys/time.h>

void CBLogError(char * format, ...){
	va_list argptr;
    va_start(argptr, format);
    vfprintf(stderr, format, argptr);
    va_end(argptr);
	printf("\n");
}

long long int CBGetMilliseconds(void){
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

void onBadTime(void * foo);
void onBadTime(void * foo){

	UNUSED(foo);

	printf("BAD TIME FAIL\n");
	exit(EXIT_FAILURE);

}

int getLen(CBBTreeNode * self);
int getLen(CBBTreeNode * self){
	int len = self->numElements;
	if (self->children[0])
		for (int x = 0; x < self->numElements + 1; x++)
			len += getLen(self->children[x]);
	return len;
}

void addRandAddr(CBNetworkAddressManager * addrMan, int x) {

	CBByteArray * ip = CBNewByteArrayWithDataCopy((unsigned char []){0x20,0x01,0x0D,0xB8,0x85,0xA3,0x00,0x42,0x10,0x00,0x8A,0x2E,0x03,0x70,0x73,x/2}, 16);
	CBNetworkAddress * addr = CBNewNetworkAddress(1358856884 + rand() % 15, (CBSocketAddress){ip, 45562 + (rand() % 5) + 6 * (x % 2)}, CB_SERVICE_FULL_BLOCKS, true);

	addr->penalty = rand() % 20;

	CBNetworkAddressManagerTakeAddress(addrMan, addr);

	CBReleaseObject(ip);

}


int main(){
	unsigned int s = (unsigned int)time(NULL);
	printf("Session = %ui\n", s);
	puts("You may need to move your mouse around if this test stalls.");
	srand(s);
	CBNetworkAddressManager * addrMan = CBNewNetworkAddressManager(onBadTime);
	if (! addrMan) {
		printf("NEW ADDR MAN FAIL\n");
		return 1;
	}
	// Test adding addresses
	for (int x = 0; x < 255; x++) {

		addRandAddr(addrMan, x);

		// Check length
		int len = 0;
		for (int x = 0; x < CB_BUCKET_NUM; x++)
			len += getLen(addrMan->addresses[x].root);
		if (len != addrMan->addrNum) {
			printf("ASSOC ARRAY AND ADDR NUM MATCH FAIL");
			return 1;
		}
	}
	// Test ordering
	for (int x = 0; x < CB_BUCKET_NUM; x++) {
		CBPosition it;
		if (CBAssociativeArrayGetFirst(&addrMan->addresses[x], &it)) {
			unsigned long long int lastScore = 0xFFFFFFFFFFFFFFFF;
			unsigned char lastIP[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
			int lastPort = 0;
			for (;;) {
				CBNetworkAddress * addr = it.node->elements[it.index];
				unsigned long long int score = addr->lastSeen - addr->penalty;
				if (score > lastScore){
					printf("ADDR LAST SEEN ORDER FAIL\n");
					return 1;
				}else if (score < lastScore){
					memset(lastIP, 0, 16);
					lastPort = 0;
				}else{
					int res = memcmp(CBByteArrayGetData(addr->sockAddr.ip), lastIP, 16);
					if (res < 0) {
						printf("ADDR IP ORDER FAIL");
						return 1;
					}else if (res > 0){
						lastPort = 0;
					}else if (addr->sockAddr.port <= lastPort){
						printf("ADDR PORT ORDER/DUPLICATE FAIL");
						return 1;
					}
				}
				// Test getting addresses
				CBNetworkAddress * addr2 = CBNetworkAddressManagerGotNetworkAddress(addrMan, addr);
				if (! addr2 || addr2 != addr) {
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
	if (CBNetworkAddressManagerGetAddresses(addrMan, 1, addr) != 1) {
		printf("GET SINGLE ADDR FAIL\n");
		return 1;
	}
	CBNetworkAddressManagerRemoveAddress(addrMan, *addr);
	if (CBNetworkAddressManagerGotNetworkAddress(addrMan, *addr) != NULL) {
		printf("REMOVE NETWORK ADDR FAIL");
		return 1;
	}
	CBReleaseObject(*addr);
	// Test getting addresses under number
	CBNetworkAddress * addrs[255];
	if (CBNetworkAddressManagerGetAddresses(addrMan, 253, addrs) != 253) {
		printf("GET ADDRS BELOW FAIL\n");
		return 1;
	}
	for (int x = 0; x < 253; x++)
		CBReleaseObject(addrs[x]);
	// Test getting addresses at number
	if (CBNetworkAddressManagerGetAddresses(addrMan, 254, addrs) != 254) {
		printf("GET ADDRS AT FAIL\n");
		return 1;
	}
	for (int x = 0; x < 254; x++)
		CBReleaseObject(addrs[x]);
	// Test getting addresses over number
	if (CBNetworkAddressManagerGetAddresses(addrMan, 255, addrs) != 254) {
		printf("GET ADDRS OVER FAIL\n");
		return 1;
	}
	// Check no duplicates
	for (int x = 0; x < 254; x++) { 
		for (int y = 0; y < 254; y++) {
			if (y != x && CBNetworkAddressCompare(NULL, addrs[x], addrs[y]) == CB_COMPARE_EQUAL) {
				printf("GET ADDRS NO DUPLICATES FAIL\n");
				return 1;
			}
		}
	}
	// Add peers
	for (int x = 0; x < CB_BUCKET_NUM; x++){
		CBFreeAssociativeArray(&addrMan->addresses[x]);
		CBInitAssociativeArray(&addrMan->addresses[x], CBNetworkAddressCompare, NULL, CBReleaseObject);
	}
	CBPeer * peers[254];
	for (int x = 0; x < 254; x++) {
		peers[x] = CBNewPeer(addrs[x]);
		CBNetworkAddressManagerAddPeer(addrMan, peers[x]);
	}
	if (addrMan->peersNum != 254) {
		printf("ADD PEERS NUM FAIL\n");
		return 1;
	}
	for (int x = 0; x < 254; x++)
		CBReleaseObject(addrs[x]);
	// Test removing peers and looking for peers
	CBNetworkAddressManagerRemovePeer(addrMan, peers[0]);
	CBNetworkAddressManagerRemovePeer(addrMan, peers[34]);
	CBNetworkAddressManagerRemovePeer(addrMan, peers[253]);
	if (addrMan->peersNum != 251) {
		printf("REMOVE PEERS NUM FAIL\n");
		return 1;
	}
	for (int x = 0; x < 254; x++) {
		CBPeer * peer = CBNetworkAddressManagerGotPeer(addrMan, peers[x]->addr);
		if (x == 0 || x == 34 || x == 253) {
			if (peer) {
				printf("REMOVE PEER FAIL\n");
				return 1;
			}
		}else{
			if (peer != peers[x]){
				printf("ADD PEER FAIL\n");
				return 1;
			}
			CBReleaseObject(peer);
		}
	}
	// Test clear peers and then network time.
	CBNetworkAddressManagerClearPeers(addrMan);
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
	for (int x = 0; x < 6; x++){
		CBNetworkAddressManagerAddPeer(addrMan, peers[x]);
		CBNetworkAddressManagerTakePeerTimeOffset(addrMan, peers[x]);
	}
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
	for (int x = 0; x < 254; x++)
		CBReleaseObject(peers[x]);

	// Test select and remove
	
	addrMan = CBNewNetworkAddressManager(onBadTime);

	for (int x = 0; x < 5; x++) 
		addRandAddr(addrMan, x);
	
	if (addrMan->addrNum != 5) {
		printf("STORAGE ADDR NUM BEFORE REMOVE FAIL\n");
		return 1;
	}

	CBNetworkAddress * removed = CBNetworkAddressManagerSelectAndRemoveAddress(addrMan);

	if (addrMan->addrNum != 4) {
		printf("STORAGE ADDR NUM AFTER REMOVE FAIL\n");
		return 1;
	}

	CBReleaseObject(removed);
	CBReleaseObject(addrMan);

	return 0;

}

