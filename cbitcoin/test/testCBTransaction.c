//
//  testCBTransaction.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 30/05/2012.
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
#include "CBTransactionInput.h"
#include <time.h>
#include <openssl/sha.h>
#include <openssl/ripemd.h>

u_int8_t * sha1(u_int8_t * data,u_int16_t len){
	u_int8_t * hash = malloc(SHA_DIGEST_LENGTH);
    SHA_CTX sha1;
    SHA_Init(&sha1);
    SHA_Update(&sha1, data, len);
    SHA_Final(hash, &sha1);
	return hash;
}
u_int8_t * sha256(u_int8_t * data,u_int16_t len){
	u_int8_t * hash = malloc(SHA256_DIGEST_LENGTH);
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, data, len);
    SHA256_Final(hash, &sha256);
	return hash;
}
u_int8_t * ripemd160(u_int8_t * data,u_int16_t len){
	u_int8_t * hash = malloc(RIPEMD160_DIGEST_LENGTH);
    RIPEMD160_CTX ripemd160;
    RIPEMD160_Init(&ripemd160);
    RIPEMD160_Update(&ripemd160, data, len);
    RIPEMD160_Final(hash, &ripemd160);
	return hash;
}

int main(){
	unsigned int s = (unsigned int)time(NULL);
	s = 1337544566;
	printf("Session = %ui\n",s);
	srand(s);
	CBNetworkParameters * net = CBNewNetworkParameters();
	net->networkCode = 0;
	CBEvents events;
	CBDependencies dep;
	dep.sha256 = sha256;
	dep.sha1 = sha1;
	dep.ripemd160 = ripemd160;
	// Test CBTransactionInput
	
	return 0;
}
