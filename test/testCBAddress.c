//
//  testCBAddress.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 16/05/2012.
//  Copyright (c) 2012 Matthew Mitchell
//  
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

#include <stdio.h>
#include "CBAddress.h"
#include <time.h>

void CBLogError(char * b, ...);
void CBLogError(char * b, ...){
	printf("%s\n", b);
	exit(EXIT_FAILURE);
}

int main(){
	unsigned int s = (unsigned int)time(NULL);
	s = 1337544566;
	printf("Session = %ui\n", s);
	srand(s);
	CBByteArray * addstr = CBNewByteArrayFromString("1D5A1q5d192j5gYuWiP3CSE5fcaaZxe6E9", true);
	CBAddress * add = CBNewAddressFromString(addstr, false);
	CBReleaseObject(addstr);
	CBBase58Prefix v = CBChecksumBytesGetPrefix(CBGetChecksumBytes(add));
	if (v != CB_PREFIX_PRODUCTION_ADDRESS) {
		printf("PRODUCTION NET VERSION DOES NOT MATCH %i != 0\n", v);
		return 1;
	}
	CBByteArray * str = CBChecksumBytesGetString(CBGetChecksumBytes(add));
	if (strcmp((char *)CBByteArrayGetData(str), "1D5A1q5d192j5gYuWiP3CSE5fcaaZxe6E9")){
		printf("NOT CACHED STRING WRONG %s != 1D5A1q5d192j5gYuWiP3CSE5fcaaZxe6E9\n", (char *)CBByteArrayGetData(str));
		return 1;
	}
	CBReleaseObject(str);
	CBReleaseObject(add);
	addstr = CBNewByteArrayFromString("mzCk9JXXF9we7MB2Gdt59tcfj6Lr2rSzpu", true);
	add = CBNewAddressFromString(addstr, false);
	CBReleaseObject(addstr);
	v = CBChecksumBytesGetPrefix(CBGetChecksumBytes(add));
	if (v != CB_PREFIX_TEST_ADDRESS) {
		printf("TEST NET VERSION DOES NOT MATCH %i != 111\n", v);
		return 1;
	}
	CBReleaseObject(add);
	addstr = CBNewByteArrayFromString("19tknf38VozS4GfoxHe6vUP3NRghbGGT6H", true);
	add = CBNewAddressFromString(addstr, true);
	CBReleaseObject(addstr);
	str = CBChecksumBytesGetString(CBGetChecksumBytes(add));
	if (strcmp((char *)CBByteArrayGetData(str), "19tknf38VozS4GfoxHe6vUP3NRghbGGT6H")){
		printf("CACHED STRING WRONG %s != 19tknf38VozS4GfoxHe6vUP3NRghbGGT6H\n", (char *)CBByteArrayGetData(str));
		return 1;
	}
	CBReleaseObject(str);
	CBReleaseObject(add);
	// Test building from fake RIPEMD160 hash
	uint8_t * hash = malloc(20);
	for (int x = 0; x < 20; x++)
		hash[x] = rand();
	add = CBNewAddressFromRIPEMD160Hash(hash, CB_NETWORK_PRODUCTION, false);
	free(hash);
	v = CBChecksumBytesGetPrefix(CBGetChecksumBytes(add));
	if (v != CB_PREFIX_PRODUCTION_ADDRESS) {
		printf("PRODUCTION NET VERSION FOR RIPEMD160 TEST DOES NOT MATCH %i != 0\n", v);
		return 1;
	}
	CBReleaseObject(add);
	hash = malloc(20);
	for (int x = 0; x < 20; x++)
		hash[x] = rand();
	add = CBNewAddressFromRIPEMD160Hash(hash, CB_NETWORK_TEST, false);
	free(hash);
	v = CBChecksumBytesGetPrefix(CBGetChecksumBytes(add));
	if (v != CB_PREFIX_TEST_ADDRESS) {
		printf("TEST NET VERSION FOR RIPEMD160 TEST DOES NOT MATCH %i != 111\n", v);
		return 1;
	}
	CBReleaseObject(add);
	// Test CBNewAddressFromRIPEMD160Hash for pre-known address
	hash = malloc(20);
	for (int x = 0; x < 20; x++)
		hash[x] = x;
	add = CBNewAddressFromRIPEMD160Hash(hash, CB_NETWORK_PRODUCTION, false);
	free(hash);
	str = CBChecksumBytesGetString(CBGetChecksumBytes(add));
	if (strcmp((char *)CBByteArrayGetData(str), "112D2adLM3UKy4Z4giRbReR6gjWuvHUqB")){
		printf("RIPEMD160 TEST STRING WRONG %s != 112D2adLM3UKy4Z4giRbReR6gjWuvHUqB\n", (char *)CBByteArrayGetData(str));
		return 1;
	}
	CBReleaseObject(str);
	CBReleaseObject(add);
	return 0;
}
