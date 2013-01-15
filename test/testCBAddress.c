//
//  testCBAddress.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 16/05/2012.
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
	uint8_t v = CBVersionChecksumBytesGetVersion(CBGetVersionChecksumBytes(add));
	if (v != CB_PRODUCTION_NETWORK_BYTE) {
		printf("PRODUCTION NET VERSION DOES NOT MATCH %i != 0\n", v);
		return 1;
	}
	CBByteArray * str = CBVersionChecksumBytesGetString(CBGetVersionChecksumBytes(add));
	if (strcmp((char *)CBByteArrayGetData(str), "1D5A1q5d192j5gYuWiP3CSE5fcaaZxe6E9")){
		printf("NOT CACHED STRING WRONG %s != 1D5A1q5d192j5gYuWiP3CSE5fcaaZxe6E9\n", (char *)CBByteArrayGetData(str));
		return 1;
	}
	CBReleaseObject(str);
	CBReleaseObject(add);
	addstr = CBNewByteArrayFromString("mzCk9JXXF9we7MB2Gdt59tcfj6Lr2rSzpu", true);
	add = CBNewAddressFromString(addstr, false);
	CBReleaseObject(addstr);
	v = CBVersionChecksumBytesGetVersion(CBGetVersionChecksumBytes(add));
	if (v != CB_TEST_NETWORK_BYTE) {
		printf("TEST NET VERSION DOES NOT MATCH %i != 111\n", v);
		return 1;
	}
	CBReleaseObject(add);
	addstr = CBNewByteArrayFromString("19tknf38VozS4GfoxHe6vUP3NRghbGGT6H", true);
	add = CBNewAddressFromString(addstr, true);
	CBReleaseObject(addstr);
	str = CBVersionChecksumBytesGetString(CBGetVersionChecksumBytes(add));
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
	add = CBNewAddressFromRIPEMD160Hash(hash, CB_PRODUCTION_NETWORK_BYTE, false);
	free(hash);
	v = CBVersionChecksumBytesGetVersion(CBGetVersionChecksumBytes(add));
	if (v != CB_PRODUCTION_NETWORK_BYTE) {
		printf("PRODUCTION NET VERSION FOR RIPEMD160 TEST DOES NOT MATCH %i != 0\n", v);
		return 1;
	}
	CBReleaseObject(add);
	hash = malloc(20);
	for (int x = 0; x < 20; x++)
		hash[x] = rand();
	add = CBNewAddressFromRIPEMD160Hash(hash, CB_TEST_NETWORK_BYTE, false);
	free(hash);
	v = CBVersionChecksumBytesGetVersion(CBGetVersionChecksumBytes(add));
	if (v != CB_TEST_NETWORK_BYTE) {
		printf("TEST NET VERSION FOR RIPEMD160 TEST DOES NOT MATCH %i != 111\n", v);
		return 1;
	}
	CBReleaseObject(add);
	// Test CBNewAddressFromRIPEMD160Hash for pre-known address
	hash = malloc(20);
	for (int x = 0; x < 20; x++)
		hash[x] = x;
	add = CBNewAddressFromRIPEMD160Hash(hash, CB_PRODUCTION_NETWORK_BYTE, false);
	free(hash);
	str = CBVersionChecksumBytesGetString(CBGetVersionChecksumBytes(add));
	if (strcmp((char *)CBByteArrayGetData(str), "112D2adLM3UKy4Z4giRbReR6gjWuvHUqB")){
		printf("RIPEMD160 TEST STRING WRONG %s != 112D2adLM3UKy4Z4giRbReR6gjWuvHUqB\n", (char *)CBByteArrayGetData(str));
		return 1;
	}
	CBReleaseObject(str);
	CBReleaseObject(add);
	return 0;
}
