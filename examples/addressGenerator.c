//
//  addressGenerator.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 06/06/2012.
//  Copyright (c) 2012 Matthew Mitchell
//  
//  This file is part of cbitcoin. It is subject to the license terms
//  in the LICENSE file found in the top-level directory of this
//  distribution and at http://www.cbitcoin.com/license.html. No part of
//  cbitcoin, including this file, may be copied, modified, propagated,
//  or distributed except according to the terms contained in the
//  LICENSE file.

#include <stdio.h>
#include <ctype.h>
#include <openssl/ssl.h>
#include <openssl/ripemd.h>
#include <openssl/rand.h>
#include "CBAddress.h"

void err(CBError a, char * format, ...);
void err(CBError a, char * format, ...){
	va_list argptr;
    va_start(argptr, format);
    vfprintf(stderr, format, argptr);
    va_end(argptr);
	printf("\n");
}

void getLine(char * ptr);
void getLine(char * ptr) {
	int c;
	int len = 30;
    for(;;) {
        c = fgetc(stdin);
        if(c == EOF)
            break;
        if(--len == 0)
            break;
		*ptr = c;
        if(c == '\n')
            break;
		ptr++;
    }
	*ptr = 0;
}

int main(){
	printf("OpenSSL version: %s\n", OPENSSL_VERSION_TEXT);
	printf("Enter the number of keys: ");
	fflush(stdout);
	char stringMatch[31];
	getLine(stringMatch);
	unsigned long int i = strtol(stringMatch, NULL, 0);
	printf("Enter a string of text for the key (30 max): ");
	fflush(stdout);
	getLine(stringMatch);
	printf("Waiting for entropy... Move the cursor around...\n");
	fflush(stdout);
	char entropy[32];
	FILE * f = fopen("/dev/random", "r");
	if (fread(entropy, 32, 1, f) != 1){
		printf("FAILURING GETTING ENTROPY!");
		return 1;
	}
	RAND_add(entropy, 32, 32);
	fclose(f);
	printf("Making %lu addresses for \"%s\"\n\n", i, stringMatch);
	EC_KEY * key = EC_KEY_new_by_curve_name(NID_secp256k1);
	uint8_t * pubKey = NULL;
	int pubSize = 0;
	uint8_t * privKey = NULL;
	int privSize = 0;
	uint8_t * shaHash = malloc(32);
	uint8_t * ripemdHash = malloc(20);
	for (unsigned int x = 0; x < i;) {
		if(NOT EC_KEY_generate_key(key)){
			printf("GENERATE KEY FAIL\n"); 
			return 1;
		}
		int pubSizeNew = i2o_ECPublicKey(key, NULL);
		if(NOT pubSizeNew){
			printf("PUB KEY TO DATA ZERO\n"); 
			return 1;
		}
		if (pubSizeNew != pubSize) {
			pubSize = pubSizeNew;
			pubKey = realloc(pubKey, pubSize);
		}
		uint8_t * pubKey2 = pubKey;
		if(i2o_ECPublicKey(key, &pubKey2) != pubSize){
			printf("PUB KEY TO DATA FAIL\n");
			return 1;
		}
		SHA256(pubKey, pubSize, shaHash);
		RIPEMD160(shaHash, 32, ripemdHash);
		CBAddress * address = CBNewAddressFromRIPEMD160Hash(ripemdHash, CB_PRODUCTION_NETWORK_BYTE, false, err);
		CBByteArray * string = CBVersionChecksumBytesGetString(CBGetVersionChecksumBytes(address));
		CBReleaseObject(address);
		bool match = true;
		uint8_t offset = 1;
		size_t matchSize = strlen(stringMatch);
		for (uint8_t y = 0; y < matchSize;) {
			char other = islower(stringMatch[y]) ? toupper(stringMatch[y]) : (isupper(stringMatch[y])? tolower(stringMatch[y]) : '\0');
			if (CBByteArrayGetByte(string, y+offset) != stringMatch[y] && CBByteArrayGetByte(string, y+offset) != other) {
				offset++;
				y = 0;
				if (string->length < matchSize + offset) {
					match = false;
					break;
				}
			}else y++;
		}
		if (match) {
			// Get private key
			const BIGNUM * privKeyNum = EC_KEY_get0_private_key(key);
			if (NOT privKeyNum) {
				printf("PRIV KEY TO BN FAIL\n");
			}
			int privSizeNew = BN_num_bytes(privKeyNum);
			if (privSizeNew != privSize) {
				privSize = privSizeNew;
				privKey = realloc(privKey, privSize);
			}
			int res = BN_bn2bin(privKeyNum, privKey);
			if (res != privSize) {
				printf("PRIV KEY TO DATA FAIL\n");
			}
			// Print data to stdout
			printf("Private key (hex): ");
			for (int x = 0; x < privSize; x++) {
				printf(" %.2X", privKey[x]);
			}
			printf("\nPublic key (hex): ");
			for (int x = 0; x < pubSize; x++) {
				printf(" %.2X", pubKey[x]);
			}
			printf("\nAddress (base-58): %s\n\n", CBByteArrayGetData(string));
			x++; // Move to next
		}
		CBReleaseObject(string);
	}
	free(shaHash);
	free(ripemdHash);
	EC_KEY_free(key);
	return 0;
}