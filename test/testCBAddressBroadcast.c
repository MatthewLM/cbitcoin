//
//  testCBAddressBroadcast.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 04/07/2012.
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
#include "CBAddressBroadcast.h"
#include <time.h>
#include "stdarg.h"

void CBLogError(char * format, ...);
void CBLogError(char * format, ...){
	va_list argptr;
    va_start(argptr, format);
    vfprintf(stderr, format, argptr);
    va_end(argptr);
	printf("\n");
}

int main(){
	unsigned int s = (unsigned int)time(NULL);
	s = 1337544566;
	printf("Session = %ui\n", s);
	srand(s);
	// Test deserialisation with timestamps
	uint8_t data[61] = {
		0x02, // Two addresses
		0x46, 0xAE, 0xF4, 0x4F, // Time 1341435462
		0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // CB_SERVICE_FULL_BLOCKS
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x0A, 0x00, 0x00, 0x01, // IP ::ffff:10.0.0.1
		0x20, 0x8D, // Port 8333
		0x7E, 0xB7, 0xF4, 0x4F, // Time 1341437822
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // No services
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x24, 0x60, 0xA2, 0x08, // IP ::ffff:36.96.162.8
		0x5F, 0x2E, // Port 24366
	};
	CBByteArray * bytes = CBNewByteArrayWithDataCopy(data, 61);
	CBAddressBroadcast * addBroadcast = CBNewAddressBroadcastFromData(bytes, true);
	if(CBAddressBroadcastDeserialise(addBroadcast) != 61){
		printf("DESERIALISATION LEN FAIL\n");
		return 1;
	}
	if (addBroadcast->addrNum != 2) {
		printf("DESERIALISATION NUM FAIL\n");
		return 1;
	}
	if (addBroadcast->addresses[0]->lastSeen != 1341435462) {
		printf("DESERIALISATION FIRST TIME FAIL\n");
		return 1;
	}
	if (addBroadcast->addresses[0]->services != 1) {
		printf("DESERIALISATION FIRST SERVICES FAIL\n");
		return 1;
	}
	if (memcmp(CBByteArrayGetData(addBroadcast->addresses[0]->ip), (uint8_t []){0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x0A, 0x00, 0x00, 0x01}, 16)) {
		printf("DESERIALISATION FIRST IP FAIL\n0x");
		uint8_t * d = CBByteArrayGetData(addBroadcast->addresses[0]->ip);
		for (int x = 0; x < 16; x++) {
			printf("%.2X", d[x]);
		}
		printf("\n!=\n0x");
		d = (uint8_t []){0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x0A, 0x00, 0x00, 0x01};
		for (int x = 0; x < 16; x++) {
			printf("%.2X", d[x]);
		}
		return 1;
	}
	if (addBroadcast->addresses[0]->port != 8333) {
		printf("DESERIALISATION FIRST PORT FAIL\n");
		return 1;
	}
	if (addBroadcast->addresses[1]->lastSeen != 1341437822) {
		printf("DESERIALISATION SECOND TIME FAIL\n");
		return 1;
	}
	if (addBroadcast->addresses[1]->services != 0) {
		printf("DESERIALISATION SECOND SERVICES FAIL\n");
		return 1;
	}
	if (memcmp(CBByteArrayGetData(addBroadcast->addresses[1]->ip), (uint8_t []){0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x24, 0x60, 0xA2, 0x08}, 16)) {
		printf("DESERIALISATION SECOND IP FAIL\n0x");
		uint8_t * d = CBByteArrayGetData(addBroadcast->addresses[1]->ip);
		for (int x = 0; x < 16; x++) {
			printf("%.2X", d[x]);
		}
		printf("\n!=\n0x");
		d = (uint8_t []){0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x24, 0x60, 0xA2, 0x08};
		for (int x = 0; x < 16; x++) {
			printf("%.2X", d[x]);
		}
		return 1;
	}
	if (addBroadcast->addresses[1]->port != 24366) {
		printf("DESERIALISATION SECOND PORT FAIL\n");
		return 1;
	}
	// Test serialisation with timestamps
	memset(CBByteArrayGetData(bytes), 0, 61);
	CBReleaseObject(addBroadcast->addresses[0]->ip);
	addBroadcast->addresses[0]->ip = CBNewByteArrayWithDataCopy((uint8_t []){0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x0A, 0x00, 0x00, 0x01}, 16);
	CBReleaseObject(addBroadcast->addresses[1]->ip);
	addBroadcast->addresses[1]->ip = CBNewByteArrayWithDataCopy((uint8_t []){0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x24, 0x60, 0xA2, 0x08}, 16);
	if (CBAddressBroadcastSerialise(addBroadcast, true) != 61){
		printf("SERIALISATION LEN FAIL\n");
		return 1;
	}
	if (memcmp(data, CBByteArrayGetData(bytes), 61)) {
		printf("SERIALISATION FAIL\n0x");
		uint8_t * d = CBByteArrayGetData(bytes);
		for (int x = 0; x < 61; x++) {
			printf("%.2X", d[x]);
		}
		printf("\n!=\n0x");
		for (int x = 0; x < 61; x++) {
			printf("%.2X", data[x]);
		}
		return 1;
	}
	CBReleaseObject(addBroadcast);
	CBReleaseObject(bytes);
	// Test deserialisation without timestamps
	uint8_t data2[53] = {
		0x02, // Two addresses
		0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // CB_SERVICE_FULL_BLOCKS
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x0A, 0x00, 0x00, 0x01, // IP ::ffff:10.0.0.1
		0x20, 0x8D, // Port 8333
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // No services
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x24, 0x60, 0xA2, 0x08, // IP ::ffff:36.96.162.8
		0x5F, 0x2E, // Port 24366
	};
	bytes = CBNewByteArrayWithDataCopy(data2, 53);
	addBroadcast = CBNewAddressBroadcastFromData(bytes, false);
	if(CBAddressBroadcastDeserialise(addBroadcast) != 53){
		printf("DESERIALISATION NO TIME LEN FAIL\n");
		return 1;
	}
	if (addBroadcast->addrNum != 2) {
		printf("DESERIALISATION NO TIME NUM FAIL\n");
		return 1;
	}
	if (addBroadcast->addresses[0]->services != 1) {
		printf("DESERIALISATION NO TIME FIRST SERVICES FAIL\n");
		return 1;
	}
	if (memcmp(CBByteArrayGetData(addBroadcast->addresses[0]->ip), (uint8_t []){0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x0A, 0x00, 0x00, 0x01}, 16)) {
		printf("DESERIALISATION NO TIME FIRST IP FAIL\n0x");
		uint8_t * d = CBByteArrayGetData(addBroadcast->addresses[0]->ip);
		for (int x = 0; x < 16; x++) {
			printf("%.2X", d[x]);
		}
		printf("\n!=\n0x");
		d = (uint8_t []){0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x0A, 0x00, 0x00, 0x01};
		for (int x = 0; x < 16; x++) {
			printf("%.2X", d[x]);
		}
		return 1;
	}
	if (addBroadcast->addresses[0]->port != 8333) {
		printf("DESERIALISATION NO TIME FIRST PORT FAIL\n");
		return 1;
	}
	if (addBroadcast->addresses[1]->services != 0) {
		printf("DESERIALISATION NO TIME SECOND SERVICES FAIL\n");
		return 1;
	}
	if (memcmp(CBByteArrayGetData(addBroadcast->addresses[1]->ip), (uint8_t []){0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x24, 0x60, 0xA2, 0x08}, 16)) {
		printf("DESERIALISATION NO TIME SECOND IP FAIL\n0x");
		uint8_t * d = CBByteArrayGetData(addBroadcast->addresses[1]->ip);
		for (int x = 0; x < 16; x++) {
			printf("%.2X", d[x]);
		}
		printf("\n!=\n0x");
		d = (uint8_t []){0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x24, 0x60, 0xA2, 0x08};
		for (int x = 0; x < 16; x++) {
			printf("%.2X", d[x]);
		}
		return 1;
	}
	if (addBroadcast->addresses[1]->port != 24366) {
		printf("DESERIALISATION NO TIME SECOND PORT FAIL\n");
		return 1;
	}
	// Test serialisation without timestamps
	memset(CBByteArrayGetData(bytes), 0, 53);
	CBReleaseObject(addBroadcast->addresses[0]->ip);
	addBroadcast->addresses[0]->ip = CBNewByteArrayWithDataCopy((uint8_t []){0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x0A, 0x00, 0x00, 0x01}, 16);
	CBReleaseObject(addBroadcast->addresses[1]->ip);
	addBroadcast->addresses[1]->ip = CBNewByteArrayWithDataCopy((uint8_t []){0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x24, 0x60, 0xA2, 0x08}, 16);
	if(CBAddressBroadcastSerialise(addBroadcast, true) != 53){
		printf("SERIALISATION NO TIME LEN FAIL\n");
		return 1;
	}
	if (memcmp(data2, CBByteArrayGetData(bytes), 53)) {
		printf("SERIALISATION NO TIME FAIL\n0x");
		uint8_t * d = CBByteArrayGetData(bytes);
		for (int x = 0; x < 53; x++) {
			printf("%.2X", d[x]);
		}
		printf("\n!=\n0x");
		for (int x = 0; x < 53; x++) {
			printf("%.2X", data2[x]);
		}
		return 1;
	}
	CBReleaseObject(addBroadcast);
	CBReleaseObject(bytes);
	return 0;
}
