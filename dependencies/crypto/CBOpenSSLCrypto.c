//
//  CBOpenSSLCrypto.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 08/08/2012.
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

// Includes

#include "CBDependencies.h" // cbitcoin dependencies to implement
#include <stdio.h>
#include <stdlib.h>
#include <openssl/sha.h>
#include <openssl/ripemd.h>
#include <openssl/ssl.h>

#pragma GCC diagnostic ignored "-Wdeprecated-declarations" // For OSX Lion

// Implementation

void CBSha160(uint8_t * data, uint16_t len, uint8_t * output){
    SHA1(data, len, output);
}
void CBSha256(uint8_t * data, uint16_t len, uint8_t * output){
	SHA256(data, len, output);
}
void CBRipemd160(uint8_t * data, uint16_t len, uint8_t * output){
	RIPEMD160(data, len, output);
}
bool CBEcdsaVerify(uint8_t * signature, uint8_t sigLen, uint8_t * hash, const uint8_t * pubKey, uint8_t keyLen){
	EC_KEY * key = EC_KEY_new_by_curve_name(NID_secp256k1);
	o2i_ECPublicKey(&key, &pubKey, keyLen);
	int res = ECDSA_verify(0, hash, 32, signature, sigLen, key);
	EC_KEY_free(key);
	return res == 1;
}
