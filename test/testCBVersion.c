//
//  testCBTransaction.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 01/07/2012.
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
#include "CBVersion.h"
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
	u_int8_t data[91] = {
		0x6A,0x00,0x00,0x00, // Version 106
		0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // CB_SERVICE_FULL_BLOCKS
		0xFA,0x3A,0xF3,0x4F,0x00,0x00,0x00,0x00, // Time 1341340410
		0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0x0A,0x00,0x00,0x01,0x20,0x8D, // Destination socket and services
		0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xBA,0xF2,0x30,0x01,0x7B,0x10, // Source socket and services
		0xDD,0x9D,0x20,0x2C,0x3A,0xB4,0x57,0x13, // Random nounce = 1393780771635895773
		0x06,'H','e','l','l','o','!', // User agent is "Hello!"
		0x55,0x81,0x01,0x00 // Last block received is 98645 
	};
	CBByteArray * versionBytes = CBNewByteArrayWithDataCopy(data, 91, &events);
	CBVersion * version = CBNewVersionFromData(versionBytes, &events);
	if(CBVersionDeserialise(version) != 91){
		printf("DESERIALISATION LEN FAIL\n");
		return 1;
	}
	if (version->version != 106) {
		printf("DESERIALISATION VERSION FAIL\n");
		return 1;
	}
	if (version->services != 1) {
		printf("DESERIALISATION SERVICES FAIL\n");
		return 1;
	}
	if (version->time != 1341340410) {
		printf("DESERIALISATION TIME FAIL\n");
		return 1;
	}
	if (version->addRecv->services != 1) {
		printf("DESERIALISATION ADD RECV SERVICES FAIL\n");
		return 1;
	}
	if (memcmp(CBByteArrayGetData(version->addRecv->ip),(u_int8_t []){0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0x0A,0x00,0x00,0x01},16)) {
		printf("DESERIALISATION ADD RECV IP FAIL\n0x");
		u_int8_t * d = CBByteArrayGetData(version->addRecv->ip);
		for (int x = 0; x < 16; x++) {
			printf("%.2X",d[x]);
		}
		printf("\n!=\n0x");
		d = (u_int8_t []){0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0x0A,0x00,0x00,0x01};
		for (int x = 0; x < 16; x++) {
			printf("%.2X",d[x]);
		}
		return 1;
	}
	if (version->addRecv->port != 0x8D20) {
		printf("DESERIALISATION ADD RECV PORT FAIL\n");
		return 1;
	}
	if (version->addSource->services != 1) {
		printf("DESERIALISATION ADD SOURCE SERVICES FAIL\n");
		return 1;
	}
	if (memcmp(CBByteArrayGetData(version->addSource->ip),(u_int8_t []){0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xBA,0xF2,0x30,0x01},16)) {
		printf("DESERIALISATION ADD SOURCE IP FAIL\n0x");
		u_int8_t * d = CBByteArrayGetData(version->addSource->ip);
		for (int x = 0; x < 16; x++) {
			printf("%.2X",d[x]);
		}
		printf("\n!=\n0x");
		d = (u_int8_t []){0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xBA,0xF2,0x30,0x01};
		for (int x = 0; x < 16; x++) {
			printf("%.2X",d[x]);
		}
		return 1;
	}
	if (version->addSource->port != 0x107B) {
		printf("DESERIALISATION ADD SOURCE PORT FAIL 0x%2.X != 0x107B \n",version->addRecv->port);
		return 1;
	}
	if (version->nounce != 1393780771635895773) {
		printf("DESERIALISATION NOUNCE FAIL\n");
		return 1;
	}
	if (strncmp((char *)CBByteArrayGetData(version->userAgent),"Hello!",6)) {
		printf("DESERIALISATION STRING FAIL\n");
		return 1;
	}
	if (version->blockHeight != 98645) {
		printf("DESERIALISATION BLOCK HEIGHT FAIL\n");
		return 1;
	}
	// Test serialisation
	memset(CBByteArrayGetData(versionBytes), 0, 91);
	CBReleaseObject(&version->userAgent);
	version->userAgent = CBNewByteArrayFromString("Hello!", &events);
	CBReleaseObject(&version->addRecv->ip);
	version->addRecv->ip = CBNewByteArrayWithDataCopy((u_int8_t []){0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0x0A,0x00,0x00,0x01}, 16, &events);
	CBReleaseObject(&version->addSource->ip);
	version->addSource->ip = CBNewByteArrayWithDataCopy((u_int8_t []){0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xBA,0xF2,0x30,0x01}, 16, &events);
	if(CBVersionSerialise(version) != 91){
		printf("SERIALISATION LEN FAIL\n");
		return 1;
	}
	if (memcmp(data, CBByteArrayGetData(versionBytes), 91)) {
		printf("SERIALISATION FAIL\n0x");
		u_int8_t * d = CBByteArrayGetData(versionBytes);
		for (int x = 0; x < 91; x++) {
			printf("%.2X",d[x]);
		}
		printf("\n!=\n0x");
		for (int x = 0; x < 91; x++) {
			printf("%.2X",data[x]);
		}
		return 1;
	}
	CBReleaseObject(&version);
	CBReleaseObject(&versionBytes);
	// Test deserilisation for older version
	u_int8_t data2[46] = {
		0x69,0x00,0x00,0x00, // Version 105
		0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // CB_SERVICE_FULL_BLOCKS
		0xFA,0x3A,0xF3,0x4F,0x00,0x00,0x00,0x00, // Time 1341340410
		0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0x0A,0x00,0x00,0x01,0x20,0x8D, // Destination socket and services
	};
	versionBytes = CBNewByteArrayWithDataCopy(data2, 46, &events);
	version = CBNewVersionFromData(versionBytes, &events);
	if(CBVersionDeserialise(version) != 46){
		printf("DESERIALISATION OLD LEN FAIL\n");
		return 1;
	}
	if (version->version != 105) {
		printf("DESERIALISATION OLD VERSION FAIL\n");
		return 1;
	}
	if (version->services != 1) {
		printf("DESERIALISATION OLD SERVICES FAIL\n");
		return 1;
	}
	if (version->time != 1341340410) {
		printf("DESERIALISATION OLD TIME FAIL\n");
		return 1;
	}
	if (version->addRecv->services != 1) {
		printf("DESERIALISATION OLD ADD RECV SERVICES FAIL\n");
		return 1;
	}
	if (memcmp(CBByteArrayGetData(version->addRecv->ip),(u_int8_t []){0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0x0A,0x00,0x00,0x01},16)) {
		printf("DESERIALISATION OLD ADD RECV IP FAIL\n0x");
		u_int8_t * d = CBByteArrayGetData(version->addRecv->ip);
		for (int x = 0; x < 16; x++) {
			printf("%.2X",d[x]);
		}
		printf("\n!=\n0x");
		d = (u_int8_t []){0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0x0A,0x00,0x00,0x01};
		for (int x = 0; x < 16; x++) {
			printf("%.2X",d[x]);
		}
		return 1;
	}
	if (version->addRecv->port != 0x8D20) {
		printf("DESERIALISATION OLD ADD RECV PORT FAIL\n");
		return 1;
	}
	// Test old serialisation
	memset(CBByteArrayGetData(versionBytes), 0, 46);
	CBReleaseObject(&version->addRecv->ip);
	version->addRecv->ip = CBNewByteArrayWithDataCopy((u_int8_t []){0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0x0A,0x00,0x00,0x01}, 16, &events);
	if(CBVersionSerialise(version) != 46){
		printf("SERIALISATION OLD LEN FAIL\n");
		return 1;
	}
	if (memcmp(data2, CBByteArrayGetData(versionBytes), 46)) {
		printf("SERIALISATION OLD FAIL\n0x");
		u_int8_t * d = CBByteArrayGetData(versionBytes);
		for (int x = 0; x < 46; x++) {
			printf("%.2X",d[x]);
		}
		printf("\n!=\n0x");
		for (int x = 0; x < 46; x++) {
			printf("%.2X",data2[x]);
		}
		return 1;
	}
	CBReleaseObject(&version);
	CBReleaseObject(&versionBytes);
	return 0;
}