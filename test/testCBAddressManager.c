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

void err(CBError a,char * format,...);
void err(CBError a,char * format,...){
	va_list argptr;
    va_start(argptr, format);
    vfprintf(stderr, format, argptr);
    va_end(argptr);
	printf("\n");
}

int main(){
	unsigned int s = (unsigned int)time(NULL);
	s = 1337544566;
	printf("Session = %ui\n",s);
	srand(s);
	CBEvents events;
	events.onErrorReceived = err;
	// Test deserialisation
	uint8_t dataRepeat[94] = {
		0x02,0x00, // Two addresses.
		// Address 1
		0x46,0xAE,0xF4,0x4F, // Time 1341435462
		0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // CB_SERVICE_FULL_BLOCKS
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0x0A,0x00,0x00,0x01, // IP ::ffff:10.0.0.1
		0x20,0x8D, // Port 8333
		// Address 2
		0x7E,0xB7,0xF4,0x4F, // Time 1341437822
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // No services
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0x24,0x60,0xA2,0x08, // IP ::ffff:36.96.162.8
		0x5F,0x2E, // Port 24366
		0x01,0x00, // One address.
		// Address 1
		0x46,0xAE,0xF4,0x4F, // Time 1341435462
		0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // CB_SERVICE_FULL_BLOCKS
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0x0A,0x00,0x00,0x01, // IP ::ffff:10.0.0.1
		0x20,0x8D, // Port 8333
	};
	uint8_t * data = malloc(12012);
	// cbitcoin version 1
	data[0] = 0x01;
	data[1] = 0x00;
	data[2] = 0x00;
	data[3] = 0x00;
	// secret 0x56AFE32056AFE320
	data[12004] = 0x20;
	data[12005] = 0xE3;
	data[12006] = 0xAF;
	data[12007] = 0x56;
	data[12008] = 0x20;
	data[12009] = 0xE3;
	data[12010] = 0xAF;
	data[12011] = 0x56;
	// Repeat data
	for (uint8_t x = 0; x < (CB_BUCKET_NUM-1)/2; x++)
		memcpy(data + 4 + 94*x , dataRepeat, 94);
	memcpy(data + 11942, dataRepeat, 62);
	CBByteArray * bytes = CBNewByteArrayWithDataCopy(data, 12012, &events);
	CBAddressManager * addrMan = CBNewAddressManagerFromData(bytes, &events);
	addrMan->maxAddressesInBucket = 500;
	u_int32_t len = CBAddressManagerDeserialise(addrMan);
	if(len != 12012){
		printf("DESERIALISATION LEN FAIL %u != 12014\n",len);
		return 1;
	}
	if (addrMan->secret != 0x56AFE32056AFE320) {
		printf("DESERIALISATION SECRET FAIL %llx != 0x56AFE32056AFE320\n",addrMan->secret);
		return 1;
	}
	for (uint8_t x = 0; x < CB_BUCKET_NUM; x++) {
		CBBucket * bucket = addrMan->buckets + x;
		bool odd = x % 2;
		if (bucket->addrNum != (odd ? 1 : 2)) {
			printf("DESERIALISATION BUCKET ADDR NUM FAIL %u: %u != %u\n",x,bucket->addrNum,odd? 1 : 2);
			return 1;
		}
		if (bucket->addresses[0]->score != 1341435462) {
			printf("DESERIALISATION FIRST ADDR SCORE FAIL %u: %u != %u\n",x,bucket->addresses[0]->score,1341435462);
			return 1;
		}
		if (bucket->addresses[0]->services != CB_SERVICE_FULL_BLOCKS) {
			printf("DESERIALISATION ADDR SERVICES FAIL\n");
			return 1;
		}
		if (memcmp(CBByteArrayGetData(bucket->addresses[0]->ip),(uint8_t []){0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0x0A,0x00,0x00,0x01},16)) {
			printf("DESERIALISATION IP FAIL\n0x");
			return 1;
		}
		if (bucket->addresses[0]->port != 8333) {
			printf("DESERIALISATION PORT FAIL\n");
			return 1;
		}
		if (odd) {
			continue;
		}
		if (bucket->addresses[1]->score != 1341437822) {
			printf("DESERIALISATION ADDR SCORE FAIL\n");
			return 1;
		}
		if (bucket->addresses[1]->services != 0) {
			printf("DESERIALISATION ADDR SERVICES FAIL\n");
			return 1;
		}
		if (memcmp(CBByteArrayGetData(bucket->addresses[1]->ip),(uint8_t []){0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0x24,0x60,0xA2,0x08,},16)) {
			printf("DESERIALISATION IP FAIL\n0x");
			return 1;
		}
		if (bucket->addresses[1]->port != 24366) {
			printf("DESERIALISATION PORT FAIL\n");
			return 1;
		}
	}
	if (CBAddressManagerGetNumberOfAddresses(addrMan) != 383) {
		printf("CBAddressManagerGetNumberOfAddresses FAIL\n");
		return 1;
	}
	// Test serialisation
	memset(CBByteArrayGetData(bytes), 0, 12012);
	for (uint8_t x = 0; x < CB_BUCKET_NUM; x++) {
		CBBucket * bucket = addrMan->buckets + x;
		bool odd = x % 2;
		CBReleaseObject(bucket->addresses[0]->ip);
		bucket->addresses[0]->ip = CBNewByteArrayWithDataCopy((uint8_t []){0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0x0A,0x00,0x00,0x01}, 16, &events);
		if (!odd) {
			CBReleaseObject(bucket->addresses[1]->ip);
			bucket->addresses[1]->ip = CBNewByteArrayWithDataCopy((uint8_t []){0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0x24,0x60,0xA2,0x08}, 16, &events);
		}
	}
	if(CBAddressManagerSerialise(addrMan) != 12012){
		printf("SERIALISATION LEN FAIL\n");
		return 1;
	}
	if (memcmp(data, CBByteArrayGetData(bytes), 12012)) {
		printf("SERIALISATION FAIL 0x\n");
		uint8_t * d = CBByteArrayGetData(bytes);
		for (int x = 0; x < 12012; x++) {
			printf("%.2X",d[x]);
		}
		printf("\n!=\n0x");
		for (int x = 0; x < 12012; x++) {
			printf("%.2X",data[x]);
		}
		return 1;
	}
	CBReleaseObject(bytes);
	// Test adding 4 nodes. Check order.
	int16_t timeOffsets[] = {-4,-10,19,-5};
	CBByteArray * ip = CBNewByteArrayWithDataCopy((uint8_t [16]){0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 16, &events);
	for (int x = 0; x < 4; x++) {
		CBNetworkAddress * addr = CBNewNetworkAddress(0, ip, 0, 0, &events);
		CBNode * node = CBNewNodeByTakingNetworkAddress(addr);
		node->timeOffset = timeOffsets[x];
		CBAddressManagerTakeNode(addrMan, node);
	}
	int16_t orderedOffsets[] = {-10,-5,-4,19};
	for (int x = 0; x < 4; x++) {
		if (addrMan->nodes[x]->timeOffset != orderedOffsets[x]) {
			printf("NODE ORDER FAIL %i: %i != %i\n",x,addrMan->nodes[x]->timeOffset, orderedOffsets[x]);
			return 1;
		}
	}
	// Test if we got nodes
	CBNetworkAddress * addr = CBNewNetworkAddress(0, ip, 0, 0, &events);
	if(NOT CBAddressManagerGotNode(addrMan, addr)){
		printf("GOT NODE FAIL\n");
		return 1;
	}
	CBReleaseObject(addr);
	CBByteArray * temp = CBByteArrayCopy(ip);
	CBReleaseObject(ip);
	ip = temp;
	CBByteArraySetByte(ip, 0, CBByteArrayGetByte(ip, 0) + 1);
	addr = CBNewNetworkAddress(0, ip, 0, 0, &events);
	if(CBAddressManagerGotNode(addrMan, addr)){
		printf("GOT NOT NODE FAIL\n");
		return 1;
	}
	CBReleaseObject(addr);
	// Median should be -5
	if (addrMan->networkTimeOffset != -5) {
		printf("MEDIAN FAIL %i != -5\n",addrMan->networkTimeOffset);
		return 1;
	}
	// Remove the node with the timeoffset of -5 and check that the median now equals -4
	CBAddressManagerRemoveNode(addrMan, addrMan->nodes[1]);
	if (addrMan->networkTimeOffset != -4) {
		printf("MEDIAN FAIL %i != -4\n",addrMan->networkTimeOffset);
		return 1;
	}
	// Add 5 addresses with max addresses at 4.
	addrMan->maxAddressesInBucket = 4;
	for (int x = 0; x < 4; x++) {
		CBByteArray * temp = CBByteArrayCopy(ip);
		CBReleaseObject(ip);
		ip = temp;
		CBByteArraySetByte(ip, 0, CBByteArrayGetByte(ip, 0) + 1);
		CBAddressManagerTakeAddress(addrMan, CBNewNetworkAddress(0, ip, 0, 0, &events));
	}
	addr = CBNewNetworkAddress(0, ip, 0, 0, &events);
	if(NOT CBAddressManagerGotNetworkAddress(addrMan, addr)){
		printf("GOT ADDR FAIL\n");
		return 1;
	}
	CBReleaseObject(addr);
	temp = CBByteArrayCopy(ip);
	CBReleaseObject(ip);
	ip = temp;
	CBByteArraySetByte(ip, 0, CBByteArrayGetByte(ip, 0) + 1);
	addr = CBNewNetworkAddress(0, ip, 0, 0, &events);
	if(CBAddressManagerGotNetworkAddress(addrMan, addr)){
		printf("GOT NOT ADDR FAIL\n");
		return 1;
	}
	CBReleaseObject(addr);
	CBReleaseObject(ip);
	// Test reachability.
	CBAddressManagerSetReachability(addrMan, CB_IP_IPv6, true);
	CBAddressManagerSetReachability(addrMan, CB_IP_IPv4, true);
	if (NOT CBAddressManagerIsReachable(addrMan, CB_IP_IPv6)) {
		printf("REACHABILITY ONE FAIL\n");
		return 1;
	}
	if (CBAddressManagerIsReachable(addrMan, CB_IP_6TO4)) {
		printf("REACHABILITY TWO FAIL\n");
		return 1;
	}
	CBAddressManagerSetReachability(addrMan, CB_IP_IPv4, false);
	if (CBAddressManagerIsReachable(addrMan, CB_IP_IPv4)) {
		printf("REACHABILITY THREE FAIL\n");
		return 1;
	}
	CBReleaseObject(addrMan);
	return 0;
}