//
//  testCBBase58.c
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
#include <openssl/sha.h>

void err(CBError a,char * b,...){
	printf("%s\n",b);
}

u_int8_t * sha256(u_int8_t * data,u_int16_t len){
	u_int8_t * hash = malloc(SHA256_DIGEST_LENGTH);
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, data, len);
    SHA256_Final(hash, &sha256);
	return hash;
}

int main(){
	unsigned int s = (unsigned int)time(NULL);
	printf("Session = %ui\n",s);
	srand(s);
	CBNetworkParameters * net = CBNewNetworkParameters();
	net->networkCode = 0;
	CBEvents events;
	events.onErrorReceived = err;
	CBDependencies dep;
	dep.sha256 = sha256;
	CBString * addstr = CBNewStringByCopyingCString("1D5A1q5d192j5gYuWiP3CSE5fcaaZxe6E9");
	CBAddress * add = CBNewAddressFromString(addstr, false, &events, &dep);
	CBGetObjectVT(addstr)->release(&addstr);
	u_int8_t v = CBGetVersionChecksumBytesVT(add)->getVersion(add);
	if (v != 0) {
		printf("PRODUCTION NET VERSION DOES NOT MATCH %i != 0\n",v);
		return 1;
	}
	CBString * str = CBGetVersionChecksumBytesVT(add)->getString(add);
	if (strcmp(str->string, "1D5A1q5d192j5gYuWiP3CSE5fcaaZxe6E9")){
		printf("NOT CACHED STRING WRONG %s != 1D5A1q5d192j5gYuWiP3CSE5fcaaZxe6E9\n",str->string);
		return 1;
	}
	CBGetObjectVT(str)->release(&str);
	CBGetObjectVT(add)->release(&add);
	addstr = CBNewStringByCopyingCString("mhFwRrjRNt8hYeWtm9LwqCpCgXjF38RJqn");
	add = CBNewAddressFromString(addstr, false, &events, &dep);
	CBGetObjectVT(addstr)->release(&addstr);
	v = CBGetVersionChecksumBytesVT(add)->getVersion(add);
	if (v != 0x6f) {
		printf("TEST NET VERSION DOES NOT MATCH %i != 111\n",v);
		return 1;
	}
	CBGetObjectVT(add)->release(&add);
	addstr = CBNewStringByCopyingCString("19tknf38VozS4GfoxHe6vUP3NRghbGGT6H");
	add = CBNewAddressFromString(addstr, true, &events, &dep);
	CBGetObjectVT(addstr)->release(&addstr);
	str = CBGetVersionChecksumBytesVT(add)->getString(add);
	if (strcmp(str->string, "19tknf38VozS4GfoxHe6vUP3NRghbGGT6H")){
		printf("CACHED STRING WRONG %s != 19tknf38VozS4GfoxHe6vUP3NRghbGGT6H\n",str->string);
		return 1;
	}
	CBGetObjectVT(str)->release(&str);
	CBGetObjectVT(add)->release(&add);
	// Test building from fake RIPEMD160 hash
	u_int8_t * hash = malloc(20);
	for (int x = 0; x < 20; x++)
		hash[x] = rand();
	add = CBNewAddressFromRIPEMD160Hash(net, hash, false, &events, &dep);
	free(hash);
	v = CBGetVersionChecksumBytesVT(add)->getVersion(add);
	if (v != 0) {
		printf("PRODUCTION NET VERSION FOR RIPEMD160 TEST DOES NOT MATCH %i != 0\n",v);
		return 1;
	}
	CBGetObjectVT(add)->release(&add);
	hash = malloc(20);
	for (int x = 0; x < 20; x++)
		hash[x] = rand();
	net->networkCode = 0x6f;
	add = CBNewAddressFromRIPEMD160Hash(net, hash, false, &events, &dep);
	free(hash);
	v = CBGetVersionChecksumBytesVT(add)->getVersion(add);
	if (v != 0x6f) {
		printf("TEST NET VERSION FOR RIPEMD160 TEST DOES NOT MATCH %i != 111\n",v);
		return 1;
	}
	CBGetObjectVT(add)->release(&add);
	// Test CBNewAddressFromRIPEMD160Hash for pre-known address
	hash = malloc(20);
	for (int x = 0; x < 20; x++)
		hash[x] = x;
	net->networkCode = 0;
	add = CBNewAddressFromRIPEMD160Hash(net, hash, false, &events, &dep);
	free(hash);
	str = CBGetVersionChecksumBytesVT(add)->getString(add);
	if (strcmp(str->string, "112D2adLM3UKy4Z4giRbReR6gjWuvHUqB")){
		printf("RIPEMD160 TEST STRING WRONG %s != 112D2adLM3UKy4Z4giRbReR6gjWuvHUqB\n",str->string);
		return 1;
	}
	CBGetObjectVT(str)->release(&str);
	CBGetObjectVT(add)->release(&add);
	return 0;
}
