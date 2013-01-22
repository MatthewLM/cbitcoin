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
		printf("NEW ADDR MAN FAIL");
		return 1;
	}
	// Test adding addresses
	for (uint8_t x = 0; x < 255; x++) {
		CBByteArray * ip = CBNewByteArrayWithDataCopy((uint8_t []){0x20,0x01,0x0D,0xB8,0x85,0xA3,0x00,0x42,0x10,0x00,0x8A,0x2E,0x03,0x70,0x73,x/2}, 16);
		CBAddressManagerTakeAddress(addrMan, CBNewNetworkAddress(1358856884 + rand() % 7, ip, 45562 + rand() % 5 + 5 * (x % 2), CB_SERVICE_FULL_BLOCKS));
		CBReleaseObject(ip);
		// Check length
		if (getLen(addrMan->addresses.root) != addrMan->addrNum) {
			printf("ASSOC ARRAY AND ADDR NUM MATCH FAIL");
			return 1;
		}
	}
	// Test ordering
	CBIterator it;
	if (NOT CBAssociativeArrayGetFirst(&addrMan->addresses, &it)) {
		printf("NEW ADDR MAN FAIL");
		return 1;
	}
	uint8_t lastBucket = 0;
	uint64_t lastLastSeen = 0;
	uint8_t lastIP[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	uint16_t lastPort = 0;
	for (uint8_t x = 0; x < 255; x++) {
		CBNetworkAddress * addr = it.node->elements[it.pos];
		if (addr->bucket < lastBucket) {
			printf("ADDR BUCKET ORDER FAIL");
			return 1;
		}else if (addr->bucket > lastBucket) {
			lastLastSeen = 0;
			memset(lastIP, 0, 16);
			lastPort = 0;
		}else if (addr->lastSeen < lastLastSeen){
			printf("ADDR LAST SEEN ORDER FAIL");
			return 1;
		}else if (addr->lastSeen > lastLastSeen){
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
		if (CBAssociativeArrayIterate(&addrMan->addresses, &it) ^ (x == 254)) {
			printf("ITERATE FAIL");
			return 1;
		}
	}
	CBReleaseObject(addrMan);
	return 0;
}
