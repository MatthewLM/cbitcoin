//
//  testCBVersion.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 01/07/2012.
//  Copyright (c) 2012 Matthew Mitchell
//  
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

#include <stdio.h>
#include "CBVersion.h"
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
	// Test deserialisation
	unsigned char data[91] = {
		0x6A, 0x00, 0x00, 0x00, // Version 106
		0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // CB_SERVICE_FULL_BLOCKS
		0xFA, 0x3A, 0xF3, 0x4F, 0x00, 0x00, 0x00, 0x00, // Time 1341340410
		0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x0A, 0x00, 0x00, 0x01, 0x20, 0x8D, // Destination socket and services
		0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xBA, 0xF2, 0x30, 0x01, 0x7B, 0x10, // Source socket and services
		0xDD, 0x9D, 0x20, 0x2C, 0x3A, 0xB4, 0x57, 0x13, // Random nonce = 1393780771635895773
		0x06, 'H', 'e', 'l', 'l', 'o', '!', // User agent is "Hello!"
		0x55, 0x81, 0x01, 0x00 // Last block received is 98645 
	};
	CBByteArray * versionBytes = CBNewByteArrayWithDataCopy(data, 91);
	CBVersion * version = CBNewVersionFromData(versionBytes);
	if (CBVersionDeserialise(version) != 91){
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
	if (memcmp(CBByteArrayGetData(version->addRecv->sockAddr.ip), (unsigned char []){0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x0A, 0x00, 0x00, 0x01}, 16)) {
		printf("DESERIALISATION ADD RECV IP FAIL\n0x");
		unsigned char * d = CBByteArrayGetData(version->addRecv->sockAddr.ip);
		for (int x = 0; x < 16; x++) {
			printf("%.2X", d[x]);
		}
		printf("\n!=\n0x");
		d = (unsigned char []){0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x0A, 0x00, 0x00, 0x01};
		for (int x = 0; x < 16; x++) {
			printf("%.2X", d[x]);
		}
		return 1;
	}
	if (version->addRecv->sockAddr.port != 8333) {
		printf("DESERIALISATION ADD RECV PORT FAIL\n");
		return 1;
	}
	if (version->addSource->services != 1) {
		printf("DESERIALISATION ADD SOURCE SERVICES FAIL\n");
		return 1;
	}
	if (memcmp(CBByteArrayGetData(version->addSource->sockAddr.ip), (unsigned char []){0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xBA, 0xF2, 0x30, 0x01}, 16)) {
		printf("DESERIALISATION ADD SOURCE IP FAIL\n0x");
		unsigned char * d = CBByteArrayGetData(version->addSource->sockAddr.ip);
		for (int x = 0; x < 16; x++) {
			printf("%.2X", d[x]);
		}
		printf("\n!=\n0x");
		d = (unsigned char []){0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xBA, 0xF2, 0x30, 0x01};
		for (int x = 0; x < 16; x++) {
			printf("%.2X", d[x]);
		}
		return 1;
	}
	if (version->addSource->sockAddr.port != 31504) {
		printf("DESERIALISATION ADD SOURCE PORT FAIL 0x%2.X != 31504 \n", version->addRecv->sockAddr.port);
		return 1;
	}
	if (version->nonce != 1393780771635895773) {
		printf("DESERIALISATION NOUNCE FAIL\n");
		return 1;
	}
	if (strncmp((char *)CBByteArrayGetData(version->userAgent), "Hello!", 6)) {
		printf("DESERIALISATION STRING FAIL\n");
		return 1;
	}
	if (version->blockHeight != 98645) {
		printf("DESERIALISATION BLOCK HEIGHT FAIL\n");
		return 1;
	}
	// Test serialisation
	memset(CBByteArrayGetData(versionBytes), 0, 91);
	CBReleaseObject(version->userAgent);
	version->userAgent = CBNewByteArrayFromString("Hello!", false);
	CBReleaseObject(version->addRecv->sockAddr.ip);
	version->addRecv->sockAddr.ip = CBNewByteArrayWithDataCopy((unsigned char []){0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x0A, 0x00, 0x00, 0x01}, 16);
	CBReleaseObject(version->addSource->sockAddr.ip);
	version->addSource->sockAddr.ip = CBNewByteArrayWithDataCopy((unsigned char []){0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xBA, 0xF2, 0x30, 0x01}, 16);
	if(CBVersionSerialise(version, true) != 91){
		printf("SERIALISATION LEN FAIL\n");
		return 1;
	}
	if (memcmp(data, CBByteArrayGetData(versionBytes), 91)) {
		printf("SERIALISATION FAIL\n0x");
		unsigned char * d = CBByteArrayGetData(versionBytes);
		for (int x = 0; x < 91; x++) {
			printf("%.2X", d[x]);
		}
		printf("\n!=\n0x");
		for (int x = 0; x < 91; x++) {
			printf("%.2X", data[x]);
		}
		return 1;
	}
	CBReleaseObject(version);
	CBReleaseObject(versionBytes);
	return 0;
}
